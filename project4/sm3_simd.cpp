#include<iostream>
#include<iomanip>
#include <ctime>
#include <random>
#include <chrono> 
#include<vector>
#include <immintrin.h>  // AVX2指令集头文件

using namespace std;

typedef uint32_t word32;
typedef uint8_t word8;

word32 IV[8] = {0x7380166F, 0x4914B2B9, 0x172442D7, 0xDA8A0600, 
                0xA96F30BC, 0x163138AA, 0xE38DEE4D, 0xB0FB0E4E};

word32 rotateleft (word32 x, int n)
{
    return (x << n) | (x >> (32 - n));
}

// SIMD优化的循环左移
inline __m256i rotate_left_avx(__m256i x, int n) {
    return _mm256_or_si256(_mm256_slli_epi32(x, n), 
                          _mm256_srli_epi32(x, 32 - n));
}

// SIMD优化的FF函数
inline __m256i FF_avx(__m256i X, __m256i Y, __m256i Z, int j) {
    if (j <= 15) {
        return _mm256_xor_si256(_mm256_xor_si256(X, Y), Z);
    } else {
        return _mm256_or_si256(_mm256_and_si256(X, Y),
                             _mm256_or_si256(_mm256_and_si256(X, Z),
                                            _mm256_and_si256(Y, Z)));
    }
}

// SIMD优化的GG函数
inline __m256i GG_avx(__m256i X, __m256i Y, __m256i Z, int j) {
    if (j <= 15) {
        return _mm256_xor_si256(_mm256_xor_si256(X, Y), Z);
    } else {
        return _mm256_or_si256(_mm256_and_si256(X, Y),
                             _mm256_andnot_si256(X, Z));
    }
}

// SIMD优化的P0函数
inline __m256i P0_avx(__m256i X) {
    return _mm256_xor_si256(_mm256_xor_si256(X, rotate_left_avx(X, 9)),
                          rotate_left_avx(X, 17));
}

// SIMD优化的P1函数
inline __m256i P1_avx(__m256i X) {
    return _mm256_xor_si256(_mm256_xor_si256(X, rotate_left_avx(X, 15)),
                          rotate_left_avx(X, 23));
}

vector<word8> padding(vector<word8>& message) {
    size_t length = message.size();
    size_t bitlength = length * 8;
    size_t k = (64 + 56 - ((length + 1) % 64)) % 64;

    vector<word8> padded = message;
    padded.push_back(0x80);

    for(int i = 0; i < k; i++) {
        padded.push_back(0x00);
    }

    for(int i = 7; i >= 0; i--) {
        padded.push_back(static_cast<word8>((bitlength >> (i * 8)) & 0xFF));
    }

    return padded;
}

void expand_avx(vector<word8>& block, __m256i W[68], __m256i W1[64]) {
    // 加载16个初始字
    for(int i = 0; i < 16; i++) {
        word32 val = ((word32)block[i * 4] << 24) | 
                    ((word32)block[i * 4 + 1] << 16) | 
                    ((word32)block[i * 4 + 2] << 8) | 
                    (word32)block[i * 4 + 3];
        W[i] = _mm256_set1_epi32(val);
    }

    // 扩展生成W[16..67]
    for(int j = 16; j < 68; j++) {
        __m256i temp = _mm256_xor_si256(_mm256_xor_si256(W[j-16], W[j-9]), 
                                      rotate_left_avx(W[j-3], 15));
        W[j] = _mm256_xor_si256(P1_avx(temp),
                              _mm256_xor_si256(rotate_left_avx(W[j-13], 7), W[j-6]));
    }

    // 计算W1[0..63]
    for(int j = 0; j < 64; j++) {
        W1[j] = _mm256_xor_si256(W[j], W[j + 4]);
    }
}

void compress_avx(__m256i V[8], __m256i W[68], __m256i W1[64]) {
    __m256i A = V[0];
    __m256i B = V[1];
    __m256i C = V[2];
    __m256i D = V[3];
    __m256i E = V[4];
    __m256i F = V[5];
    __m256i G = V[6];
    __m256i H = V[7];

    __m256i SS1, SS2, TT1, TT2;
    __m256i T_j;

    for(int j = 0; j < 64; j++) {
        // 预计算rotateleft(0x79CC4519, j % 32)
        word32 t_j = rotateleft(0x79CC4519, j % 32);
        T_j = _mm256_set1_epi32(t_j);

        // 计算SS1
        SS1 = rotate_left_avx(_mm256_add_epi32(
                                _mm256_add_epi32(rotate_left_avx(A, 12), E), 
                                T_j), 7);

        // 计算SS2
        SS2 = _mm256_xor_si256(SS1, rotate_left_avx(A, 12));

        // 计算TT1
        TT1 = _mm256_add_epi32(FF_avx(A, B, C, j), D);
        TT1 = _mm256_add_epi32(TT1, SS2);
        TT1 = _mm256_add_epi32(TT1, W1[j]);

        // 计算TT2
        TT2 = _mm256_add_epi32(GG_avx(E, F, G, j), H);
        TT2 = _mm256_add_epi32(TT2, SS1);
        TT2 = _mm256_add_epi32(TT2, W[j]);

        // 更新寄存器
        D = C;
        C = rotate_left_avx(B, 9);
        B = A;
        A = TT1;
        H = G;
        G = rotate_left_avx(F, 19);
        F = E;
        E = P0_avx(TT2);
    }

    // 更新V
    V[0] = _mm256_xor_si256(V[0], A);
    V[1] = _mm256_xor_si256(V[1], B);
    V[2] = _mm256_xor_si256(V[2], C);
    V[3] = _mm256_xor_si256(V[3], D);
    V[4] = _mm256_xor_si256(V[4], E);
    V[5] = _mm256_xor_si256(V[5], F);
    V[6] = _mm256_xor_si256(V[6], G);
    V[7] = _mm256_xor_si256(V[7], H);
}

vector<word8> sm3_avx(vector<word8>& message) {
    // 初始化寄存器
    __m256i V[8];
    for(int i = 0; i < 8; i++) {
        V[i] = _mm256_set1_epi32(IV[i]);
    }

    vector<word8> padded = padding(message);

    // 处理消息分组
    for(size_t i = 0; i < padded.size(); i += 64) {
        vector<word8> block(padded.begin() + i, padded.begin() + i + 64);
        __m256i W[68];
        __m256i W1[64];
        expand_avx(block, W, W1);
        compress_avx(V, W, W1);
    }

    // 将结果转换为字节
    vector<word8> digest(32);
    alignas(32) word32 V_arr[8][8]; // 8个__m256i，每个包含8个word32
    
    for(int i = 0; i < 8; i++) {
        _mm256_store_si256((__m256i*)V_arr[i], V[i]);
    }

    // 提取第一个通道的结果(因为我们只处理一个消息块)
    for(int i = 0; i < 8; i++) {
        digest[i * 4]     = static_cast<word8>((V_arr[i][0] >> 24) & 0xFF);
        digest[i * 4 + 1] = static_cast<word8>((V_arr[i][0] >> 16) & 0xFF);
        digest[i * 4 + 2] = static_cast<word8>((V_arr[i][0] >> 8) & 0xFF);
        digest[i * 4 + 3] = static_cast<word8>(V_arr[i][0] & 0xFF);
    }

    return digest;
}

int main() {
    const int NUM_TESTS = 1000;  // 测试次数
    const size_t MAX_SIZE = 4096; // 最大消息长度(4KB)
    const size_t MIN_SIZE = 1;    // 最小消息长度

    // 初始化随机数生成器
    random_device rd;
    mt19937_64 gen(rd());
    uniform_int_distribution<size_t> size_dist(MIN_SIZE, MAX_SIZE);
    uniform_int_distribution<word8> byte_dist(0, 255);

    double total_time = 0.0;

    for (int i = 0; i < NUM_TESTS; i++) {
        // 生成随机长度的消息
        size_t msg_size = size_dist(gen);
        vector<word8> message(msg_size);
        
        // 用随机字节填充消息
        for (auto& byte : message) {
            byte = byte_dist(gen);
        }

        // 计时开始
        auto start = chrono::high_resolution_clock::now();
        
        // 计算SM3哈希
        vector<word8> hash = sm3_avx(message);
        
        // 计时结束
        auto end = chrono::high_resolution_clock::now();
        chrono::duration<double, milli> duration = end - start;
        total_time += duration.count();

        // 每100次测试输出一次进度
        if ((i + 1) % 100 == 0) {
            cout << "完成测试: " << (i + 1) << "/" << NUM_TESTS 
                 << "  平均用时: " << fixed << setprecision(3) 
                 << (total_time / (i + 1)) << " ms" << endl;
        }
    }

    // 计算并输出最终结果
    double average_time = total_time / NUM_TESTS;
    double throughput = (MAX_SIZE * NUM_TESTS) / (total_time / 1000) / (1024 * 1024); // MB/s
    
    cout << "\n===== SM3 性能测试结果 =====" << endl;
    cout << "测试次数: " << NUM_TESTS << endl;
    cout << "消息长度范围: " << MIN_SIZE << " - " << MAX_SIZE << " 字节" << endl;
    cout << "总耗时: " << fixed << setprecision(2) << total_time << " ms" << endl;
    cout << "平均时间: " << fixed << setprecision(3) << average_time << " ms" << endl;
    cout << "吞吐量: " << fixed << setprecision(2) << throughput << " MB/s" << endl;

    return 0;
}