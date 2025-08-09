import socket, pickle
import random
import hashlib
from gmpy2 import *
from phe import paillier

# parameter
p = mpz(gmpy2.next_prime(2**256))
g = mpz(3)

# P1 input: ID_set and k1
V = ["user1", "user2", "user3"]
k1 = random.randint(1, p - 1)

client = socket.socket()
client.connect(('localhost', 1234))

# 1. compute H(vi)^k1 and non-sequentially send

# hash function
def H(x):
    digest = hashlib.sha3_512(str(x).encode()).hexdigest()
    return mpz(int(digest, 16)) % p

A = [powmod(H(v), k1, p) for v in V]
random.shuffle(A)
client.sendall(pickle.dumps(A))
print("P1 sends A successfully!")

# 2. receive B, C and pk
B, C, pk = pickle.loads(client.recv(40960))
print("P1 receives B, C and pk!")

# 3. 对C中每个元素计算得到 H(w)^k1k2,并判断是否属于B,从而得到交集大小
count = 0
E = []
for c, d in C:
    e = powmod(c, k1, p)
    if e in B:
        count += 1
        E.append(d)

# 4. 同态加密求和
if E:
    ct_sum = E[0]
    for ct in E[1:]:
        ct_sum = ct_sum + ct
else:
    ct_sum = pk.encrypt(0)

# 5. 发送 AEnc(S_J) 给 P2
client.sendall(pickle.dumps(ct_sum))
print("P1发送 AEnc(S) 完成!")
print("P1求得交集大小为：", count)
client.close()
