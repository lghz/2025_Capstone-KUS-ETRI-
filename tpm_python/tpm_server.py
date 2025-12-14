#!/usr/bin/env python3
# Simplified TPM server with defaults and robust telemetry handling
import argparse, asyncio, os, random
from typing import List
from tpm_lib import TPM, gen_input, send_json, recv_json, hmac_tag

# ---- Defaults requested ----
DEF_K, DEF_N, DEF_L = 3, 4, 3
DEF_RANDOM_SEEDS = True
DEF_RANDOM_INPUTS = True
DEF_STOP_ON_SYNC = True
DEF_CHECK_EVERY = 1
DEF_DEBUG_TELEMETRY = "weights"  # {"off", "hash", "weights"}
DEF_LOG_EVERY = 0  # no periodic log (we print telemetry lines instead)
DEF_MAX_ROUNDS = 3000

def urand_seed(bits=64):
    return int.from_bytes(os.urandom(bits // 8), 'big')

def mismatch_count(a_w: List[List[int]], b_w: List[List[int]]) -> int:
    return sum(
        1
        for k in range(len(a_w))
        for j in range(len(a_w[0]))
        if a_w[k][j] != b_w[k][j]
    )

async def recv_skip_tele(reader, tpm, args_expect: dict, expect_type: str):
    """
    Consume any number of 'tele' frames arriving before the expected frame.
    Print terminal visualization lines. Return the first non-tele frame.
    """
    while True:
        msg = await recv_json(reader)
        mtype = msg.get("type")
        if mtype == "tele" and args_expect["debug_telemetry"] != "off":
            key8_b = msg.get("key8", "????????")
            mode = args_expect["debug_telemetry"]
            if mode == "weights" and isinstance(msg.get("w"), list):
                mis = mismatch_count(tpm.w, msg["w"])
                print(f"Round {msg.get('round',0):4d} | tele | mismatch={mis:2d} | "
                      f"hashA[0..7]={tpm.key_hex()[:8]} | hashB[0..7]={key8_b}")
            else:
                print(f"Round {msg.get('round',0):4d} | tele | "
                      f"hashA[0..7]={tpm.key_hex()[:8]} | hashB[0..7]={key8_b}")
            continue
        return msg

async def handle_client(reader, writer, host, port):
    # Bundle expected behavior based on defaults
    args_expect = dict(
        K=DEF_K, N=DEF_N, L=DEF_L,
        random_seeds=DEF_RANDOM_SEEDS, random_inputs=DEF_RANDOM_INPUTS,
        stop_on_sync=DEF_STOP_ON_SYNC, check_every=DEF_CHECK_EVERY,
        debug_telemetry=DEF_DEBUG_TELEMETRY, log_every=DEF_LOG_EVERY,
        max_rounds=DEF_MAX_ROUNDS
    )

    seed_a = urand_seed() if args_expect["random_seeds"] else 42

    if args_expect["random_inputs"]:
        sysrng = random.SystemRandom()
        def gen_input_sys(K, N):
            return [[1 if sysrng.getrandbits(1) else -1 for _ in range(N)] for _ in range(K)]
        make_x = gen_input_sys
    else:
        rng_inputs = random.Random(3141592)
        make_x = lambda K, N: gen_input(K, N, rng_inputs)

    tpm = TPM(args_expect["K"], args_expect["N"], args_expect["L"], seed=seed_a)

    addr = writer.get_extra_info('peername')
    print(f"[A] Client connected: {addr}")
    rounds = 0
    synced_early = False

    try:
        while rounds < args_expect["max_rounds"]:
            rounds += 1
            x = make_x(args_expect["K"], args_expect["N"])
            await send_json(writer, {"type": "x", "x": x, "round": rounds})

            tau_a, sig_a = tpm.tau(x)
            # Expect tau from client (but skip any telemetry that arrives first)
            msg = await recv_skip_tele(reader, tpm, args_expect, expect_type="tau")
            if msg.get("type") != "tau":
                raise RuntimeError(f"Unexpected message while waiting tau: {msg}")
            tau_b = int(msg["tau"])

            # Send my tau
            await send_json(writer, {"type": "tau", "tau": tau_a, "round": rounds})

            # Update
            tpm.update_random_walk(x, sig_a, tau_peer=tau_b, tau_me=tau_a)

            # Optional early-stop probe (every round, default=1)
            if args_expect["stop_on_sync"] and args_expect["check_every"] and (rounds % args_expect["check_every"] == 0):
                nonce = os.urandom(16)
                await send_json(writer, {"type": "probe", "nonce": nonce.hex()})
                # Skip any telemetry that might race before the probe_resp
                resp = await recv_skip_tele(reader, tpm, args_expect, expect_type="probe_resp")
                if resp.get("type") != "probe_resp":
                    raise RuntimeError(f"Unexpected probe response: {resp}")
                tag_b = bytes.fromhex(resp["tag"])
                tag_a = hmac_tag(tpm.key_bytes(), nonce)
                if tag_a == tag_b:
                    print(f"== sync detected at round {rounds} ==")
                    synced_early = True
                    break

        if not synced_early:
            nonce = os.urandom(16)
            await send_json(writer, {"type": "mac_chal", "nonce": nonce.hex()})
            resp = await recv_skip_tele(reader, tpm, args_expect, expect_type="mac_resp")
            if resp.get("type") != "mac_resp":
                raise RuntimeError(f"Unexpected mac response: {resp}")
            tag_b = bytes.fromhex(resp["tag"])
            tag_a = hmac_tag(tpm.key_bytes(), nonce)
            ok = (tag_a == tag_b)
        else:
            ok = True

        await send_json(writer, {"type": "result", "ok": ok, "rounds": rounds, "key_hex": tpm.key_hex()})
        print(f"[A] Done. rounds={rounds} | match={ok} | key={tpm.key_hex()}")

    except Exception as e:
        print("[A] error:", e)
    finally:
        writer.close()
        await writer.wait_closed()

async def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('--host', default='127.0.0.1')
    ap.add_argument('--port', type=int, default=8080)
    args = ap.parse_args()

    server = await asyncio.start_server(lambda r, w: handle_client(r, w, args.host, args.port), args.host, args.port)
    addrs = ", ".join(str(sock.getsockname()) for sock in server.sockets)
    print(f"[A] Listening on {addrs}")
    async with server:
        await server.serve_forever()

if __name__ == '__main__':
    asyncio.run(main())
