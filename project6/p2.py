import socket, pickle
import random
import hashlib
from gmpy2 import *
from phe import paillier

# parameter
p = mpz(gmpy2.next_prime(2**256))
g = mpz(3)

# P2 input: ID-value_set, k2 and homomorphic encryption key pair (pk, sk)
W = [("user1", 100), ("user3", 300), ("user4", 400)]
k2 = random.randint(1, p - 1)
pk, sk = paillier.generate_paillier_keypair()

server = socket.socket()
server.bind(('localhost', 1234))
server.listen(1)
print("P2 awaits connection from P1‌ ...")

conn, _ = server.accept()
print("P2 connects P1!")

# 1. receive H(vi)^k1
A = pickle.loads(conn.recv(40960))
print("P2 receives A!")

# 2. compute Bi = Ai^k2
B = [powmod(x, k2, p) for x in A]

random.shuffle(B)

# 3. compute H(wi)^k2 and AEnc(ti)

# hash function
def H(x):
    digest = hashlib.sha3_512(str(x).encode()).hexdigest()
    return mpz(int(digest, 16)) % p

C = []
for w, t in W:
    Ci = powmod(H(w), k2, p)
    Di = pk.encrypt(t)
    C.append((Ci, Di))

random.shuffle(C)

# 4. send B, C and pk
conn.sendall(pickle.dumps((B, C, pk)))
print("P2 sends E sucessfully!")

# 5. 接收 AEnc(S)
ct_sum = pickle.loads(conn.recv(40960))
S = sk.decrypt(ct_sum)
print("P2 解密交集值总和为 S =", S)

conn.close()
