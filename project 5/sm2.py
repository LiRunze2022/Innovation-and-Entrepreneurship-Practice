import random
import hashlib
from typing import Tuple, Union

# SM2椭圆曲线参数
P = 0xFFFFFFFEFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFF00000000FFFFFFFFFFFFFFFF
A = 0xFFFFFFFEFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFF00000000FFFFFFFFFFFFFFFC
B = 0x28E9FA9E9D9F5E344D5A9E4BCF6509A7F39789F515AB8F92DDBCBD414D940E93
N = 0xFFFFFFFEFFFFFFFFFFFFFFFFFFFFFFFF7203DF6B21C6052B53BBF40939D54123
Gx = 0x32C4AE2C1F1981195F9904466A39C9948FE30BBFF2660BE1715A4589334C74C7
Gy = 0xBC3736A2F4F6779C59BDCEE36B692153D0A9877CC62A474002DF32E52139F0A0

class SM2:
    def __init__(self):
        self.p = P
        self.a = A
        self.b = B
        self.n = N
        self.g = (Gx, Gy)
        self.bytes_len = (P.bit_length() + 7) // 8
    
    def _to_bytes(self, x: int) -> bytes:
        return x.to_bytes(self.bytes_len, 'big')
    
    def _to_int(self, byte: bytes) -> int:
        return int.from_bytes(byte, 'big')
    
    def _point_add(self, P: Tuple[int, int], Q: Tuple[int, int]) -> Tuple[int, int]:
        if P == (0, 0):
            return Q
        if Q == (0, 0):
            return P
        x1, y1 = P
        x2, y2 = Q
        
        if x1 == x2 and y1 == y2:
            # 相同点加倍
            lam = (3 * x1 * x1 + self.a) * pow(2 * y1, -1, self.p) % self.p
        else:
            lam = (y2 - y1) * pow(x2 - x1, -1, self.p) % self.p
        
        x3 = (lam * lam - x1 - x2) % self.p
        y3 = (lam * (x1 - x3) - y1) % self.p
        return (x3, y3)
    
    def _k_times_point(self, k: int, point: Tuple[int, int]) -> Tuple[int, int]:
        result = (0, 0)
        temp = point
        
        while k:
            if k & 1:
                result = self._point_add(result, temp)
            temp = self._point_add(temp, temp)
            k >>= 1
        return result
    
    def _get_k(self) -> int:
        return random.randint(1, self.n - 1)
    
    def key_gen(self) -> Tuple[int, Tuple[int, int]]:
        """生成密钥对"""
        private_key = random.randint(1, self.n - 1)
        public_key = self._k_times_point(private_key, self.g)
        return private_key, public_key
    
    def encrypt(self, public_key: Tuple[int, int], msg: bytes) -> bytes:
        """使用公钥加密消息"""
        x, y = public_key
        
        # 步骤1: 生成随机数k
        k = self._get_k()
        
        # 步骤2: 计算C1 = k*G
        C1 = self._k_times_point(k, self.g)
        C1_bytes = b'\x04' + self._to_bytes(C1[0]) + self._to_bytes(C1[1])
        
        # 步骤3: 计算S = h * Pb (h=1)
        S = self._k_times_point(k, public_key)
        x2, y2 = S
        
        # 步骤4: 计算t = KDF(x2||y2, klen)
        t = self._kdf(self._to_bytes(x2) + self._to_bytes(y2), len(msg))
        if all(b == 0 for b in t):
            return self.encrypt(public_key, msg)  # 重新生成k
        
        # 步骤5: 计算C2 = M ⊕ t
        C2 = bytes([m ^ t_i for m, t_i in zip(msg, t)])
        
        # 步骤6: 计算C3 = Hash(x2 || M || y2)
        C3 = hashlib.sha256(self._to_bytes(x2) + msg + self._to_bytes(y2)).digest()
        
        # 返回密文 C = C1 || C3 || C2
        return C1_bytes + C3 + C2
    
    def decrypt(self, private_key: int, cipher: bytes) -> bytes:
        """使用私钥解密密文"""
        # 解析密文
        C1_len = 1 + 2 * self.bytes_len
        C1_bytes = cipher[:C1_len]
        C3_len = 32  # SHA256输出长度
        C3 = cipher[C1_len:C1_len + C3_len]
        C2 = cipher[C1_len + C3_len:]
        
        # 恢复点C1
        if C1_bytes[0] != 0x04:
            raise ValueError("Unsupported point format")
        x1 = self._to_int(C1_bytes[1:1 + self.bytes_len])
        y1 = self._to_int(C1_bytes[1 + self.bytes_len:C1_len])
        C1 = (x1, y1)
        
        # 检查点是否在曲线上
        if not self._is_on_curve(C1):
            raise ValueError("Point is not on the curve")
        
        # 计算S = [db] * C1
        S = self._k_times_point(private_key, C1)
        x2, y2 = S
        
        # 计算t = KDF(x2||y2, klen)
        t = self._kdf(self._to_bytes(x2) + self._to_bytes(y2), len(C2))
        if all(b == 0 for b in t):
            raise ValueError("KDF resulted in all zeros")
        
        # 恢复消息 M' = C2 ⊕ t
        msg = bytes([c ^ t_i for c, t_i in zip(C2, t)])
        
        # 验证 u = Hash(x2 || M' || y2) 是否等于C3
        u = hashlib.sha256(self._to_bytes(x2) + msg + self._to_bytes(y2)).digest()
        if u != C3:
            raise ValueError("Hash verification failed")
        
        return msg
    
    def _kdf(self, z: bytes, klen: int) -> bytes:
        """密钥派生函数（基于SHA256）"""
        c = 0
        t = b''
        for ct in range(1, (klen + 31) // 32 + 1):
            t += hashlib.sha256(z + ct.to_bytes(4, 'big')).digest()
        return t[:klen]
    
    def _is_on_curve(self, point: Tuple[int, int]) -> bool:
        """检查点是否在椭圆曲线上"""
        if point == (0, 0):
            return True
        x, y = point
        return (y * y - x * x * x - self.a * x - self.b) % self.p == 0

# 测试示例
if __name__ == "__main__":
    sm2 = SM2()
    
    # 生成密钥对
    private_key, public_key = sm2.key_gen()
    print(f"私钥: {hex(private_key)}")
    print(f"公钥: (0x{hex(public_key[0])}, 0x{hex(public_key[1])})")
    
    # 加密消息
    message = b"Hello, SM2 encryption!"
    cipher_text = sm2.encrypt(public_key, message)
    print(f"密文 (hex): {cipher_text.hex()}")
    
    # 解密消息
    decrypted = sm2.decrypt(private_key, cipher_text)
    print(f"解密结果: {decrypted.decode()}")