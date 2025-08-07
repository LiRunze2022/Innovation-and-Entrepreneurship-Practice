import random
import hashlib
import time
import multiprocessing as mp
from typing import Tuple, Union, List

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
        """椭圆曲线点加法"""
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
        """椭圆曲线点的标量乘法"""
        result = (0, 0)
        temp = point
        
        while k:
            if k & 1:
                result = self._point_add(result, temp)
            temp = self._point_add(temp, temp)
            k >>= 1
        return result
    
    def _get_k(self) -> int:
        """生成随机数k"""
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
        
        # 步骤3: 计算S = k * P (点乘)
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


# 使用multiprocessing进行并行处理
class ParallelSM2:
    def __init__(self, num_processes: int = None):
        self.sm2 = SM2()
        self.num_processes = num_processes or mp.cpu_count()
        print(f"Initializing ParallelSM2 with {self.num_processes} processes")
    
    def generate_key_pairs(self, num_pairs: int) -> list:
        """并行生成多个密钥对"""
        with mp.Pool(self.num_processes) as pool:
            results = pool.starmap(self._generate_key_pair, [(self.sm2,) for _ in range(num_pairs)])
        return results
    
    def _generate_key_pair(self, sm2) -> Tuple[int, Tuple[int, int]]:
        """生成单个密钥对（用于并行处理）"""
        return sm2.key_gen()
    
    def encrypt_messages(self, public_keys: list, messages: list) -> list:
        """并行加密多条消息"""
        if len(public_keys) != len(messages):
            raise ValueError("Public keys and messages must have the same length")
        
        with mp.Pool(self.num_processes) as pool:
            # 为每个加密任务创建参数元组
            tasks = [(self.sm2, public_keys[i], messages[i]) for i in range(len(public_keys))]
            results = pool.starmap(self._encrypt_message, tasks)
        return results
    
    def _encrypt_message(self, sm2, public_key, message) -> bytes:
        """加密单条消息（用于并行处理）"""
        return sm2.encrypt(public_key, message)
    
    def decrypt_messages(self, private_keys: list, ciphers: list) -> list:
        """并行解密密文"""
        if len(private_keys) != len(ciphers):
            raise ValueError("Private keys and ciphers must have the same length")
        
        with mp.Pool(self.num_processes) as pool:
            # 为每个解密任务创建参数元组
            tasks = [(self.sm2, private_keys[i], ciphers[i]) for i in range(len(private_keys))]
            results = pool.starmap(self._decrypt_message, tasks)
        return results
    
    def _decrypt_message(self, sm2, private_key, cipher) -> bytes:
        """解密单条密文（用于并行处理）"""
        return sm2.decrypt(private_key, cipher)


# 性能测试和演示
def performance_test(num_items: int):
    """性能测试函数"""
    # 创建并行SM2处理器，使用物理CPU核心数
    parallel_sm2 = ParallelSM2()
    
    # 生成测试数据
    print(f"\nGenerating {num_items} test messages and keys...")
    key_pairs = parallel_sm2.generate_key_pairs(num_items)
    private_keys = [pair[0] for pair in key_pairs]
    public_keys = [pair[1] for pair in key_pairs]
    
    # 生成测试消息
    messages = [f"Message {i} @ {time.time()}".encode() for i in range(num_items)]
    
    # 并行加密测试
    print("\nStarting parallel encryption test...")
    start = time.time()
    ciphers = parallel_sm2.encrypt_messages(public_keys, messages)
    encryption_time = time.time() - start
    print(f"Encrypted {num_items} messages in {encryption_time:.4f} seconds")
    
    # 并行解密测试
    print("\nStarting parallel decryption test...")
    start = time.time()
    decrypted_messages = parallel_sm2.decrypt_messages(private_keys, ciphers)
    decryption_time = time.time() - start
    print(f"Decrypted {num_items} messages in {decryption_time:.4f} seconds")
    
    # 验证解密结果
    print("\nVerifying decryption results...")
    for i, (original, decrypted) in enumerate(zip(messages, decrypted_messages)):
        assert original == decrypted, f"Message {i} decryption failed"
    print("All messages decrypted successfully!")
    
    # 性能比较（单线程）
    print("\nComparing with single thread performance...")
    sm2_single = SM2()
    
    start = time.time()
    single_ciphers = []
    for i in range(num_items):
        single_ciphers.append(sm2_single.encrypt(public_keys[i], messages[i]))
    single_encrypt_time = time.time() - start
    
    start = time.time()
    for i in range(num_items):
        sm2_single.decrypt(private_keys[i], single_ciphers[i])
    single_decrypt_time = time.time() - start
    
    # 输出性能比较结果
    print("\nPerformance Summary:")
    print(f"Parallel Encryption: {encryption_time:.4f} sec")
    print(f"Single-thread Encryption: {single_encrypt_time:.4f} sec")
    print(f"Speedup: {single_encrypt_time / encryption_time:.2f}x")
    print(f"\nParallel Decryption: {decryption_time:.4f} sec")
    print(f"Single-thread Decryption: {single_decrypt_time:.4f} sec")
    print(f"Speedup: {single_decrypt_time / decryption_time:.2f}x")
    
    return {
        "num_items": num_items,
        "parallel_encrypt": encryption_time,
        "parallel_decrypt": decryption_time,
        "single_encrypt": single_encrypt_time,
        "single_decrypt": single_decrypt_time
    }

# 主测试程序
if __name__ == "__main__":
    # Windows系统需要添加这个保护
    mp.freeze_support()
    
    print("===== SM2 Algorithm with Multiprocessing Optimization =====")
    print(f"Available CPU cores: {mp.cpu_count()}")
    
    # 运行不同规模的性能测试
    test_results = []
    
    # 测试10个数据项
    print("\nRunning test with 10 items...")
    test_results.append(performance_test(10))
    
    # 测试100个数据项
    print("\nRunning test with 100 items...")
    test_results.append(performance_test(100))
    
    # 测试1000个数据项（大规模）
    if mp.cpu_count() >= 4:  # 只在多核系统上运行大规模测试
        print("\nRunning test with 1000 items...")
        test_results.append(performance_test(1000))
    
    # 打印所有测试结果
    print("\nFinal Performance Results:")
    for result in test_results:
        n = result["num_items"]
        p_enc = result["parallel_encrypt"]
        s_enc = result["single_encrypt"]
        p_dec = result["parallel_decrypt"]
        s_dec = result["single_decrypt"]
        enc_speedup = s_enc / p_enc if p_enc > 0 else 0
        dec_speedup = s_dec / p_dec if p_dec > 0 else 0
        
        print(f"\nFor {n} items:")
        print(f"  Encryption Speedup: {enc_speedup:.2f}x")
        print(f"  Decryption Speedup: {dec_speedup:.2f}x")
    
    print("\nTest completed!")