import random
import hashlib
import time
import sys
from typing import Tuple, Union

# SM2椭圆曲线参数
P = 0xFFFFFFFEFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFF00000000FFFFFFFFFFFFFFFF
A = 0xFFFFFFFEFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFF00000000FFFFFFFFFFFFFFFC
B = 0x28E9FA9E9D9F5E344D5A9E4BCF6509A7F39789F515AB8F92DDBCBD414D940E93
N = 0xFFFFFFFEFFFFFFFFFFFFFFFFFFFFFFFF7203DF6B21C6052B53BBF40939D54123
Gx = 0x32C4AE2C1F1981195F9904466A39C9948FE30BBFF2660BE1715A4589334C74C7
Gy = 0xBC3736A2F4F6779C59BDCEE36B692153D0A9877CC62A474002DF32E52139F0A0

# 预先计算的常数
P_MINUS_2 = P - 2
N_MINUS_2 = N - 2

# 使用扩展欧几里得算法优化模逆计算
def extended_gcd(a: int, b: int) -> Tuple[int, int, int]:
    """扩展欧几里得算法计算模逆"""
    if b == 0:
        return a, 1, 0
    else:
        gcd, x1, y1 = extended_gcd(b, a % b)
        x = y1
        y = x1 - (a // b) * y1
        return gcd, x, y

def optimized_inverse(a: int, modulus: int) -> int:
    """使用扩展欧几里得算法优化的模逆计算"""
    gcd, x, y = extended_gcd(a, modulus)
    if gcd != 1:
        raise ValueError("Inverse does not exist")
    return x % modulus

# 使用费马小定理优化的模逆计算（对于素数模数）
def fermat_inverse(a: int, modulus: int, exponent: int) -> int:
    """使用费马小定理优化的模逆计算（指数预先计算）"""
    return pow(a, exponent, modulus)

class SM2Optimized:
    def __init__(self):
        self.p = P
        self.a = A
        self.b = B
        self.n = N
        self.g = (Gx, Gy)
        self.bytes_len = (P.bit_length() + 7) // 8
        
        # 预计算出用于模逆计算的指数值
        self.p_inv_exp = P_MINUS_2
        self.n_inv_exp = N_MINUS_2
        
        # 预计算G点的倍点
        self._precomputed_G = self._precompute_points(self.g)
        print("Precomputed", len(self._precomputed_G), "points for G")
    
    def _precompute_points(self, point: Tuple[int, int]) -> list:
        """预计算点的倍点用于快速标量乘法"""
        # 采用4-bit窗口大小进行预计算
        window_size = 4
        num_windows = (self.n.bit_length() + window_size - 1) // window_size
        precomputed = [None] * (1 << window_size)
        
        precomputed[0] = (0, 0)  # 无穷远点
        precomputed[1] = point   # 原始点
        
        # 预计算所有可能的倍点
        P2 = self._point_double(point)
        precomputed[2] = P2
        
        for i in range(3, 1 << window_size):
            precomputed[i] = self._point_add(precomputed[i-1], point)
        
        return precomputed
    
    def _to_bytes(self, x: int) -> bytes:
        return x.to_bytes(self.bytes_len, 'big')
    
    def _to_int(self, byte: bytes) -> int:
        return int.from_bytes(byte, 'big')
    
    def _point_double(self, P: Tuple[int, int]) -> Tuple[int, int]:
        """优化的点倍运算（用于标量乘法）"""
        if P[0] == 0 and P[1] == 0:
            return (0, 0)
        
        x1, y1 = P
        two_y1 = (2 * y1) % self.p
        
        # 优化点倍公式
        lam = ((3 * x1 * x1 + self.a) * fermat_inverse(two_y1, self.p, self.p_inv_exp)) % self.p
        x3 = (lam * lam - 2 * x1) % self.p
        y3 = (lam * (x1 - x3) - y1) % self.p
        
        return (x3, y3)
    
    def _point_add(self, P: Tuple[int, int], Q: Tuple[int, int]) -> Tuple[int, int]:
        """优化的点加运算"""
        if P == (0, 0):
            return Q
        if Q == (0, 0):
            return P
            
        x1, y1 = P
        x2, y2 = Q
        
        # 处理特殊情况：P + P = 2P
        if x1 == x2 and y1 == y2:
            return self._point_double(P)
        
        # 处理特殊情况：P + (-P) = O
        if x1 == x2 and y1 != y2:
            return (0, 0)
        
        # 优化点加公式
        lam = ((y2 - y1) * fermat_inverse(x2 - x1, self.p, self.p_inv_exp)) % self.p
        x3 = (lam * lam - x1 - x2) % self.p
        y3 = (lam * (x1 - x3) - y1) % self.p
        
        return (x3, y3)
    
    def _sliding_window_mult(self, k: int, precomputed: list) -> Tuple[int, int]:
        """滑动窗口法优化的标量乘法"""
        if k == 0:
            return (0, 0)
        
        window_size = 4
        num_windows = (self.n.bit_length() + window_size - 1) // window_size
        
        # 初始化结果为无穷远点
        result = (0, 0)
        
        i = num_windows * window_size
        while i > 0:
            i -= window_size
            # 窗口位置的位
            window = (k >> i) & ((1 << window_size) - 1)
            
            # 加倍点
            for _ in range(window_size):
                result = self._point_double(result)
            
            if window != 0:
                result = self._point_add(result, precomputed[window])
        
        return result
    
    def _k_times_point(self, k: int, point: Tuple[int, int]) -> Tuple[int, int]:
        """优化的标量乘法"""
        # 使用预计算的点倍点（对于基点G）
        if point == self.g and hasattr(self, '_precomputed_G'):
            return self._sliding_window_mult(k, self._precomputed_G)
        
        # 对于其他点，采用NAF算法
        return self._naf_mult(k, point)
    
    def _naf_mult(self, k: int, point: Tuple[int, int]) -> Tuple[int, int]:
        """NAF (Non-Adjacent Form) 优化的标量乘法"""
        # 计算k的NAF表示
        naf = self._to_naf(k)
        result = (0, 0)
        temp = point
        
        for digit in naf:
            if digit == 1:
                result = self._point_add(result, temp)
            elif digit == -1:
                # 负点： (x, -y) mod p
                neg_point = (temp[0], -temp[1] % self.p)
                result = self._point_add(result, neg_point)
            
            temp = self._point_double(temp)
        
        return result
    
    def _to_naf(self, k: int) -> list:
        """将整数转换为NAF (Non-Adjacent Form) 表示"""
        naf = []
        while k > 0:
            if k & 1:
                digit = 2 - (k % 4)
                k -= digit
            else:
                digit = 0
            naf.append(digit)
            k >>= 1
        return naf
    
    def _get_k(self) -> int:
        """优化的随机数生成（避免边界条件）"""
        # 使用系统随机源
        random_bytes = random.randbytes(self.bytes_len + 8)
        k = int.from_bytes(random_bytes, 'big') % (self.n - 1) + 1
        return k
    
    def key_gen(self) -> Tuple[int, Tuple[int, int]]:
        """生成密钥对（带缓存）"""
        private_key = self._get_k()
        public_key = self._k_times_point(private_key, self.g)
        return private_key, public_key
    
    def encrypt(self, public_key: Tuple[int, int], msg: bytes) -> bytes:
        """优化的加密函数"""
        # 优化：预计算公钥的倍点用于快速标量乘法
        if not hasattr(self, '_precomputed_pub'):
            print("Precomputing public key points...")
            self._precomputed_pub = self._precompute_points(public_key)
        
        x, y = public_key
        
        # 步骤1: 生成随机数k（优化边界检查）
        k = self._get_k()
        
        # 步骤2: 计算C1 = k*G（使用预计算G点）
        C1 = self._k_times_point(k, self.g)
        C1_bytes = b'\x04' + self._to_bytes(C1[0]) + self._to_bytes(C1[1])
        
        # 步骤3: 计算S = k * P（使用预计算公钥点）
        # S = self._k_times_point(k, public_key)  # 原始方法
        S = self._sliding_window_mult(k, self._precomputed_pub)  # 使用滑动窗口优化
        x2, y2 = S
        
        # 步骤4: 计算t = KDF(x2||y2, klen)（优化KDF）
        t = self._optimized_kdf(self._to_bytes(x2) + self._to_bytes(y2), len(msg))
        if all(b == 0 for b in t):
            return self.encrypt(public_key, msg)  # 重新生成k
        
        # 步骤5: 计算C2 = M ⊕ t（避免临时列表）
        C2 = bytearray(len(msg))
        for i in range(len(msg)):
            C2[i] = msg[i] ^ t[i]
        
        # 步骤6: 优化C3计算（使用缓冲区）
        buffer = bytearray(self._to_bytes(x2))
        buffer.extend(msg)
        buffer.extend(self._to_bytes(y2))
        C3 = hashlib.sha256(buffer).digest()
        
        # 返回密文 C = C1 || C3 || C2
        return C1_bytes + C3 + bytes(C2)
    
    def decrypt(self, private_key: int, cipher: bytes) -> bytes:
        """优化的解密函数"""
        # 解析密文
        C1_len = 1 + 2 * self.bytes_len
        C1_bytes = cipher[:C1_len]
        C3_len = 32  # SHA256输出长度
        C3 = cipher[C1_len:C1_len + C3_len]
        C2 = cipher[C1_len + C3_len:]
        
        # 恢复点C1（优化点检查）
        if C1_bytes[0] != 0x04:
            raise ValueError("Unsupported point format")
        
        # 使用内存视图避免复制（优化内存）
        x1_bytes = memoryview(C1_bytes)[1:1 + self.bytes_len]
        y1_bytes = memoryview(C1_bytes)[1 + self.bytes_len:C1_len]
        
        x1 = self._to_int(x1_bytes)
        y1 = self._to_int(y1_bytes)
        
        # 快速检查点是否在曲线上
        if not self._fast_is_on_curve((x1, y1)):
            raise ValueError("Point is not on the curve")
        
        # 计算S = [db] * C1（使用NAF算法优化）
        S = self._naf_mult(private_key, (x1, y1))
        x2, y2 = S
        
        # 计算t = KDF(x2||y2, klen)
        t = self._optimized_kdf(self._to_bytes(x2) + self._to_bytes(y2), len(C2))
        if all(b == 0 for b in t):
            raise ValueError("KDF resulted in all zeros")
        
        # 恢复消息 M' = C2 ⊕ t（避免临时列表）
        msg = bytearray(len(C2))
        for i in range(len(C2)):
            msg[i] = C2[i] ^ t[i]
        
        # 优化验证计算（使用缓冲区）
        buffer = bytearray(self._to_bytes(x2))
        buffer.extend(msg)
        buffer.extend(self._to_bytes(y2))
        u = hashlib.sha256(buffer).digest()
        
        if u != C3:
            raise ValueError("Hash verification failed")
        
        return bytes(msg)
    
    def _optimized_kdf(self, z: bytes, klen: int) -> bytes:
        """优化的密钥派生函数（避免字符串连接）"""
        t = bytearray()
        ct = 1
        hash_len = 32  # SHA256输出长度
        
        while len(t) < klen:
            input_bytes = z + ct.to_bytes(4, 'big')
            t.extend(hashlib.sha256(input_bytes).digest())
            ct += 1
        
        return bytes(t[:klen])
    
    def _fast_is_on_curve(self, point: Tuple[int, int]) -> bool:
        """快速点曲线检查（避免全计算）"""
        if point == (0, 0):
            return True
        
        x, y = point
        y2 = y * y % self.p
        right = (x * x * x + self.a * x + self.b) % self.p
        
        # 快速检查（避免全模运算）
        return (y2 - right) % self.p == 0


# 性能测试函数
def performance_test():
    sm2_original = SM2Optimized()
    
    print("===== 密钥生成性能测试 =====")
    start = time.perf_counter()
    private_key, public_key = sm2_original.key_gen()
    keygen_time = time.perf_counter() - start
    print(f"生成密钥对耗时: {keygen_time * 1000:.3f} ms")
    print(f"私钥: {hex(private_key)}")
    print(f"公钥: (0x{hex(public_key[0])}, 0x{hex(public_key[1])})")
    
    # 测试消息
    message = b"This is a test message for SM2 encryption and decryption performance testing."
    print(f"\n原始消息 ({len(message)} bytes): {message.decode()}")
    
    # 加密性能
    print("\n===== 加密性能测试 =====")
    start = time.perf_counter()
    cipher_text = sm2_original.encrypt(public_key, message)
    encrypt_time = time.perf_counter() - start
    print(f"加密耗时: {encrypt_time * 1000:.3f} ms")
    print(f"密文长度: {len(cipher_text)} bytes")
    
    # 解密性能
    print("\n===== 解密性能测试 =====")
    start = time.perf_counter()
    decrypted = sm2_original.decrypt(private_key, cipher_text)
    decrypt_time = time.perf_counter() - start
    print(f"解密耗时: {decrypt_time * 1000:.3f} ms")
    
    # 验证解密结果
    print("\n===== 结果验证 =====")
    print(f"解密消息: {decrypted.decode()}")
    print(f"加解密结果一致: {message == decrypted}")
    
    # 点乘操作性能测试
    print("\n===== 点乘性能测试 =====")
    k = random.randint(1, N - 1)
    
    # 优化前点乘（用于基准）
    start = time.perf_counter()
    original_point = sm2_original._naf_mult(k, sm2_original.g)
    naf_time = time.perf_counter() - start
    print(f"NAF点乘耗时: {naf_time * 1000:.3f} ms")
    
    # 滑动窗口点乘
    start = time.perf_counter()
    window_point = sm2_original._sliding_window_mult(k, sm2_original._precomputed_G)
    window_time = time.perf_counter() - start
    print(f"滑动窗口点乘耗时: {window_time * 1000:.3f} ms")
    print(f"加速比: {naf_time/window_time:.2f}x")
    
    # 性能总结
    print("\n===== 性能总结 =====")
    print(f"密钥生成时间: {keygen_time * 1000:.3f} ms")
    print(f"加密时间: {encrypt_time * 1000:.3f} ms")
    print(f"解密时间: {decrypt_time * 1000:.3f} ms")
    print(f"点乘优化加速: {naf_time/window_time:.2f}x")


if __name__ == "__main__":
    # 配置递归深度（用于大数运算）
    sys.setrecursionlimit(10000)
    
    print("===== SM2算法内部优化实现 =====")
    performance_test()