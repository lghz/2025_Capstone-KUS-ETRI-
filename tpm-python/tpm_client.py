#!/usr/bin/env python3
# Simplified TPM client with defaults and telemetry every round
import argparse, asyncio, os
from tpm_lib import TPM, send_json, recv_json, hmac_tag

DEF_K, DEF_N, DEF_L = 3, 4, 3
DEF_RANDOM_SEEDS = True
DEF_DEBUG_TELEMETRY = "weights"  # {"off","hash","weights"}
DEF_TELEMETRY_EVERY = 1

def urand_seed(bits=64):
    return int.from_bytes(os.urandom(bits // 8), 'big')

async def client_main(host: str, port: int):
    reader, writer = await asyncio.open_connection(host, port)
    seed_b = urand_seed() if DEF_RANDOM_SEEDS else 777
    tpm = TPM(DEF_K, DEF_N, DEF_L, seed=seed_b)
    print(f"[B] connected to {host}:{port}")

    try:
        while True:
            msg = await recv_json(reader)
            mtype = msg.get("type")
            if mtype == "x":
                x = msg["x"]
                round_no = int(msg.get("round", 0))

                tau_b, sig_b = tpm.tau(x)
                await send_json(writer, {"type": "tau", "tau": tau_b, "round": round_no})

                msg2 = await recv_json(reader)
                if msg2.get("type") != "tau":
                    raise RuntimeError(f"Unexpected after tau send: {msg2}")
                tau_a = int(msg2["tau"])

                tpm.update_random_walk(x, sig_b, tau_peer=tau_a, tau_me=tau_b)

                # Telemetry after update
                if DEF_DEBUG_TELEMETRY != "off" and (DEF_TELEMETRY_EVERY == 1 or (round_no % DEF_TELEMETRY_EVERY) == 0):
                    payload = {"type":"tele","round": round_no, "key8": tpm.key_hex()[:8]}
                    if DEF_DEBUG_TELEMETRY == "weights":
                        payload["w"] = tpm.w
                    await send_json(writer, payload)

            elif mtype == "probe":
                nonce = bytes.fromhex(msg["nonce"])
                tag = hmac_tag(tpm.key_bytes(), nonce).hex()
                await send_json(writer, {"type": "probe_resp", "tag": tag})

            elif mtype == "mac_chal":
                nonce = bytes.fromhex(msg["nonce"])
                tag = hmac_tag(tpm.key_bytes(), nonce).hex()
                await send_json(writer, {"type": "mac_resp", "tag": tag})

            elif mtype == "result":
                ok = bool(msg["ok"])
                rounds = int(msg["rounds"])
                key_hex = msg.get("key_hex")
                print(f"[B] Done. rounds={rounds} | match={ok} | key={key_hex}")
                break

            else:
                raise RuntimeError(f"Unexpected message: {msg}")

    except Exception as e:
        print("[B] error:", e)
    finally:
        writer.close()
        await writer.wait_closed()

def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('--host', default='127.0.0.1')
    ap.add_argument('--port', type=int, default=8080)
    args = ap.parse_args()
    asyncio.run(client_main(args.host, args.port))

if __name__ == '__main__':
    main()
