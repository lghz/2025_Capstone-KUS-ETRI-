
import json, asyncio, hmac, hashlib, random
from typing import List, Tuple

def sign(x: int) -> int:
    return 1 if x >= 0 else -1

class TPM:
    def __init__(self, K:int, N:int, L:int, seed:int):
        self.K, self.N, self.L = K, N, L
        rng = random.Random(seed)
        # weights in [-L, L]
        self.w = [[rng.randint(-L, L) for _ in range(N)] for _ in range(K)]

    def tau(self, x: List[List[int]]) -> Tuple[int, List[int]]:
        sigmas = []
        for k in range(self.K):
            acc = 0
            for j in range(self.N):
                acc += self.w[k][j] * x[k][j]
            sigmas.append(1 if acc >= 0 else -1)
        t = 1
        for s in sigmas:
            t *= s
        return t, sigmas

    def update_random_walk(self, x, sigmas, tau_peer:int, tau_me:int):
        # Only update when taus agree
        if tau_peer != tau_me:
            return
        L = self.L
        for k in range(self.K):
            if sigmas[k] != tau_me:
                continue
            row = self.w[k]
            xv = x[k]
            for j in range(self.N):
                v = row[j] + xv[j]*tau_me
                if v > L: v = L
                elif v < -L: v = -L
                row[j] = v

    def key_bytes(self) -> bytes:
        h = hashlib.sha256()
        for k in range(self.K):
            for j in range(self.N):
                h.update(int(self.w[k][j]).to_bytes(2, 'big', signed=True))
        return h.digest()

    def key_hex(self) -> str:
        return self.key_bytes().hex()

def gen_input(K:int, N:int, rng: random.Random):
    # entries in {-1, +1}
    return [[1 if rng.getrandbits(1) else -1 for _ in range(N)] for _ in range(K)]

async def send_json(writer: asyncio.StreamWriter, obj):
    data = (json.dumps(obj, separators=(',', ':')) + "\n").encode('utf-8')
    writer.write(data)
    await writer.drain()

async def recv_json(reader: asyncio.StreamReader):
    line = await reader.readline()
    if not line:
        raise ConnectionError("connection closed")
    return json.loads(line.decode('utf-8'))

def hmac_tag(key_bytes: bytes, nonce: bytes) -> bytes:
    return hmac.new(key_bytes, nonce, hashlib.sha256).digest()
