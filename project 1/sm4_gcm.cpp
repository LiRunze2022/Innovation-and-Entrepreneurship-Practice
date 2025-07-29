#include <iostream>
#include <vector>
#include <cstring>
#include <omp.h>
#include<random>
#include <immintrin.h> // 用于AES-NI指令集（这里用于参考）
#include<chrono>

// SM4算法相关常量
constexpr uint32_t SM4_BLOCK_SIZE = 16;
constexpr uint32_t SM4_KEY_SIZE = 16;
constexpr uint32_t SM4_NUM_ROUNDS = 32;

// SM4 S盒
constexpr uint8_t SM4_SBOX[256] = {
    0xd6, 0x90, 0xe9, 0xfe, 0xcc, 0xe1, 0x3d, 0xb7, 0x16, 0xb6, 0x14, 0xc2, 0x28, 0xfb, 0x2c, 0x05,
    0x2b, 0x67, 0x9a, 0x76, 0x2a, 0xbe, 0x04, 0xc3, 0xaa, 0x44, 0x13, 0x26, 0x49, 0x86, 0x06, 0x99,
    0x9c, 0x42, 0x50, 0xf4, 0x91, 0xef, 0x98, 0x7a, 0x33, 0x54, 0x0b, 0x43, 0xed, 0xcf, 0xac, 0x62,
    0xe4, 0xb3, 0x1c, 0xa9, 0xc9, 0x08, 0xe8, 0x95, 0x80, 0xdf, 0x94, 0xfa, 0x75, 0x8f, 0x3f, 0xa6,
    0x47, 0x07, 0xa7, 0xfc, 0xf3, 0x73, 0x17, 0xba, 0x83, 0x59, 0x3c, 0x19, 0xe6, 0x85, 0x4f, 0xa8,
    0x68, 0x6b, 0x81, 0xb2, 0x71, 0x64, 0xda, 0x8b, 0xf8, 0xeb, 0x0f, 0x4b, 0x70, 0x56, 0x9d, 0x35,
    0x1e, 0x24, 0x0e, 0x5e, 0x63, 0x58, 0xd1, 0xa2, 0x25, 0x22, 0x7c, 0x3b, 0x01, 0x21, 0x78, 0x87,
    0xd4, 0x00, 0x46, 0x57, 0x9f, 0xd3, 0x27, 0x52, 0x4c, 0x36, 0x02, 0xe7, 0xa0, 0xc4, 0xc8, 0x9e,
    0xea, 0xbf, 0x8a, 0xd2, 0x40, 0xc7, 0x38, 0xb5, 0xa3, 0xf7, 0xf2, 0xce, 0xf9, 0x61, 0x15, 0xa1,
    0xe0, 0xae, 0x5d, 0xa4, 0x9b, 0x34, 0x1a, 0x55, 0xad, 0x93, 0x32, 0x30, 0xf5, 0x8c, 0xb1, 0xe3,
    0x1d, 0xf6, 0xe2, 0x2e, 0x82, 0x66, 0xca, 0x60, 0xc0, 0x29, 0x23, 0xab, 0x0d, 0x53, 0x4e, 0x6f,
    0xd5, 0xdb, 0x37, 0x45, 0xde, 0xfd, 0x8e, 0x2f, 0x03, 0xff, 0x6a, 0x72, 0x6d, 0x6c, 0x5b, 0x51,
    0x8d, 0x1b, 0xaf, 0x92, 0xbb, 0xdd, 0xbc, 0x7f, 0x11, 0xd9, 0x5c, 0x41, 0x1f, 0x10, 0x5a, 0xd8,
    0x0a, 0xc1, 0x31, 0x88, 0xa5, 0xcd, 0x7b, 0xbd, 0x2d, 0x74, 0xd0, 0x12, 0xb8, 0xe5, 0xb4, 0xb0,
    0x89, 0x69, 0x97, 0x4a, 0x0c, 0x96, 0x77, 0x7e, 0x65, 0xb9, 0xf1, 0x09, 0xc5, 0x6e, 0xc6, 0x84,
    0x18, 0xf0, 0x7d, 0xec, 0x3a, 0xdc, 0x4d, 0x20, 0x79, 0xee, 0x5f, 0x3e, 0xd7, 0xcb, 0x39, 0x48
};

// SM4系统参数FK
constexpr uint32_t FK[4] = {
    0xa3b1bac6, 0x56aa3350, 0x677d9197, 0xb27022dc
};

// SM4固定参数CK
constexpr uint32_t CK[32] = {
    0x00070e15, 0x1c232a31, 0x383f464d, 0x545b6269,
    0x70777e85, 0x8c939aa1, 0xa8afb6bd, 0xc4cbd2d9,
    0xe0e7eef5, 0xfc030a11, 0x181f262d, 0x343b4249,
    0x50575e65, 0x6c737a81, 0x888f969d, 0xa4abb2b9,
    0xc0c7ced5, 0xdce3eaf1, 0xf8ff060d, 0x141b2229,
    0x30373e45, 0x4c535a61, 0x686f767d, 0x848b9299,
    0xa0a7aeb5, 0xbcc3cad1, 0xd8dfe6ed, 0xf4fb0209,
    0x10171e25, 0x2c333a41, 0x484f565d, 0x646b7279
};

// SM4轮函数中的线性变换L
uint32_t SM4_L(uint32_t b) {
    return b ^ ((b << 2) | (b >> 30)) ^ ((b << 10) | (b >> 22)) ^ 
           ((b << 18) | (b >> 14)) ^ ((b << 24) | (b >> 8));
}

// SM4轮函数中的合成变换T
uint32_t SM4_T(uint32_t x) {
    uint32_t b0 = SM4_SBOX[x >> 24];
    uint32_t b1 = SM4_SBOX[(x >> 16) & 0xff];
    uint32_t b2 = SM4_SBOX[(x >> 8) & 0xff];
    uint32_t b3 = SM4_SBOX[x & 0xff];
    uint32_t y = (b0 << 24) | (b1 << 16) | (b2 << 8) | b3;
    return SM4_L(y);
}

// SM4密钥扩展
void SM4_KeySchedule(const uint8_t key[SM4_KEY_SIZE], uint32_t rk[SM4_NUM_ROUNDS]) {
    uint32_t mk[4];
    for (int i = 0; i < 4; i++) {
        mk[i] = (key[4*i] << 24) | (key[4*i+1] << 16) | (key[4*i+2] << 8) | key[4*i+3];
        mk[i] ^= FK[i];
    }
    
    for (int i = 0; i < SM4_NUM_ROUNDS; i++) {
        uint32_t x = mk[1] ^ mk[2] ^ mk[3] ^ CK[i];
        uint32_t t = SM4_T(x);
        rk[i] = mk[0] ^ t;
        
        // 循环左移
        mk[0] = mk[1];
        mk[1] = mk[2];
        mk[2] = mk[3];
        mk[3] = rk[i];
    }
}

// SM4一轮加密/解密
void SM4_Round(const uint32_t rk[SM4_NUM_ROUNDS], const uint8_t input[SM4_BLOCK_SIZE], uint8_t output[SM4_BLOCK_SIZE], bool decrypt = false) {
    uint32_t x[4];
    for (int i = 0; i < 4; i++) {
        x[i] = (input[4*i] << 24) | (input[4*i+1] << 16) | (input[4*i+2] << 8) | input[4*i+3];
    }
    
    for (int i = 0; i < SM4_NUM_ROUNDS; i++) {
        int round = decrypt ? (SM4_NUM_ROUNDS - 1 - i) : i;
        uint32_t t = x[1] ^ x[2] ^ x[3] ^ rk[round];
        t = SM4_T(t);
        uint32_t tmp = x[0] ^ t;
        
        x[0] = x[1];
        x[1] = x[2];
        x[2] = x[3];
        x[3] = tmp;
    }
    
    // 最终反序变换
    for (int i = 0; i < 4; i++) {
        output[4*i] = (x[3-i] >> 24) & 0xff;
        output[4*i+1] = (x[3-i] >> 16) & 0xff;
        output[4*i+2] = (x[3-i] >> 8) & 0xff;
        output[4*i+3] = x[3-i] & 0xff;
    }
}

// GCM模式下的乘法运算 (GF(2^128))
void gfmul(const uint8_t *a, const uint8_t *b, uint8_t *c) {
    uint8_t v[16];
    uint8_t z[16] = {0};
    
    memcpy(v, b, 16);
    
    for (int i = 0; i < 16; i++) {
        uint8_t d = a[i];
        for (int j = 0; j < 8; j++) {
            if (d & 0x80) {
                for (int k = 0; k < 16; k++) {
                    z[k] ^= v[k];
                }
            }
            
            int carry = v[0] & 0x01;
            for (int k = 0; k < 15; k++) {
                v[k] = (v[k] >> 1) | ((v[k+1] & 0x01) << 7);
            }
            v[15] >>= 1;
            if (carry) {
                v[15] ^= 0xe1;
            }
            
            d <<= 1;
        }
    }
    
    memcpy(c, z, 16);
}

// 并行计算GHASH
void parallel_ghash(const std::vector<uint8_t>& data, const uint8_t* h, uint8_t* output, size_t block_size = SM4_BLOCK_SIZE) {
    size_t num_blocks = data.size() / block_size;
    if (data.size() % block_size != 0) num_blocks++;
    
    // 分配线程局部存储
    std::vector<uint8_t> thread_results(omp_get_max_threads() * block_size, 0);
    
    #pragma omp parallel
    {
        int tid = omp_get_thread_num();
        uint8_t* local_result = thread_results.data() + tid * block_size;
        
        #pragma omp for schedule(static)
        for (size_t i = 0; i < num_blocks; i++) {
            size_t offset = i * block_size;
            size_t len = std::min(block_size, data.size() - offset);
            
            uint8_t block[SM4_BLOCK_SIZE] = {0};
            memcpy(block, data.data() + offset, len);
            
            // XOR with current result then multiply by H
            for (int j = 0; j < block_size; j++) {
                block[j] ^= local_result[j];
            }
            
            gfmul(block, h, local_result);
        }
    }
    
    // 合并线程结果
    memset(output, 0, block_size);
    for (int i = 0; i < omp_get_max_threads(); i++) {
        uint8_t* local_result = thread_results.data() + i * block_size;
        for (int j = 0; j < block_size; j++) {
            output[j] ^= local_result[j];
        }
    }
    
    // 最后再乘以H一次
    uint8_t final_block[SM4_BLOCK_SIZE];
    gfmul(output, h, final_block);
    memcpy(output, final_block, block_size);
}

// SM4-GCM加密
void SM4_GCM_Encrypt(const uint8_t* key, const uint8_t* iv, size_t iv_len,
                     const uint8_t* aad, size_t aad_len,
                     const uint8_t* plaintext, size_t plaintext_len,
                     uint8_t* ciphertext, uint8_t* tag, size_t tag_len) {
    // 1. 生成H = E_K(0^128)
    uint32_t rk[SM4_NUM_ROUNDS];
    SM4_KeySchedule(key, rk);
    
    uint8_t H[SM4_BLOCK_SIZE] = {0};
    SM4_Round(rk, H, H);
    
    // 2. 生成初始计数器J0
    uint8_t J0[SM4_BLOCK_SIZE];
    if (iv_len == 12) {
        memcpy(J0, iv, 12);
        memset(J0 + 12, 0, 4);
        J0[15] = 1;
    } else {
        // 需要完整实现GHASH计算J0
        // 这里简化处理
        memset(J0, 0, SM4_BLOCK_SIZE);
    }
    
    // 3. 加密数据 (CTR模式)
    #pragma omp parallel for schedule(static)
    for (size_t i = 0; i < plaintext_len; i += SM4_BLOCK_SIZE) {
        uint8_t counter[SM4_BLOCK_SIZE];
        memcpy(counter, J0, SM4_BLOCK_SIZE);
        
        // 增加计数器
        for (int j = SM4_BLOCK_SIZE - 1; j >= 0; j--) {
            if (++counter[j] != 0) break;
        }
        
        uint8_t encrypted_counter[SM4_BLOCK_SIZE];
        SM4_Round(rk, counter, encrypted_counter);
        
        size_t len = std::min(SM4_BLOCK_SIZE, (uint32_t)(plaintext_len - i));
        for (size_t j = 0; j < len; j++) {
            ciphertext[i + j] = plaintext[i + j] ^ encrypted_counter[j];
        }
    }
    
    // 4. 计算GHASH (并行)
    std::vector<uint8_t> ghash_input;
    
    // 添加AAD
    if (aad_len > 0) {
        size_t aad_padded_len = ((aad_len + SM4_BLOCK_SIZE - 1) / SM4_BLOCK_SIZE) * SM4_BLOCK_SIZE;
        ghash_input.resize(aad_padded_len + ((plaintext_len + SM4_BLOCK_SIZE - 1) / SM4_BLOCK_SIZE) * SM4_BLOCK_SIZE + SM4_BLOCK_SIZE);
        
        memcpy(ghash_input.data(), aad, aad_len);
        memset(ghash_input.data() + aad_len, 0, aad_padded_len - aad_len);
        
        // 添加密文
        memcpy(ghash_input.data() + aad_padded_len, ciphertext, plaintext_len);
        memset(ghash_input.data() + aad_padded_len + plaintext_len, 0, 
               ((plaintext_len + SM4_BLOCK_SIZE - 1) / SM4_BLOCK_SIZE) * SM4_BLOCK_SIZE - plaintext_len);
        
        // 添加长度信息 (AAD长度和明文长度，各64位)
        uint64_t aad_len_bits = aad_len * 8;
        uint64_t plaintext_len_bits = plaintext_len * 8;
        
        size_t len_pos = aad_padded_len + ((plaintext_len + SM4_BLOCK_SIZE - 1) / SM4_BLOCK_SIZE) * SM4_BLOCK_SIZE;
        for (int i = 0; i < 8; i++) {
            ghash_input[len_pos + i] = (aad_len_bits >> (56 - i*8)) & 0xff;
            ghash_input[len_pos + 8 + i] = (plaintext_len_bits >> (56 - i*8)) & 0xff;
        }
    } else {
        // 类似处理，但没有AAD部分
    }
    
    uint8_t S[SM4_BLOCK_SIZE];
    parallel_ghash(ghash_input, H, S);
    
    // 5. 计算认证标签
    uint8_t T[SM4_BLOCK_SIZE];
    SM4_Round(rk, J0, T);
    
    for (size_t i = 0; i < SM4_BLOCK_SIZE && i < tag_len; i++) {
        tag[i] = S[i] ^ T[i];
    }
}

int main() {
    const int NUM_TESTS = 10000;
    const size_t PLAINTEXT_LEN = 1024; // 测试用明文长度
    std::vector<double> encryption_times;

    // 初始化随机数生成器
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<uint8_t> dis(0, 255);

    // 预分配内存
    uint8_t key[SM4_KEY_SIZE];
    uint8_t iv[12];
    size_t aad_len = 16;
    std::vector<uint8_t> aad(aad_len);
    std::vector<uint8_t> plaintext(PLAINTEXT_LEN);
    std::vector<uint8_t> ciphertext(PLAINTEXT_LEN);
    std::vector<uint8_t> tag(16);

    for (int i = 0; i < NUM_TESTS; i++) {
        // 生成随机测试数据
        for (auto& b : key) b = dis(gen);
        for (auto& b : iv) b = dis(gen);
        for (auto& b : aad) b = dis(gen);
        for (auto& b : plaintext) b = dis(gen);

        // 开始计时
        auto start = std::chrono::high_resolution_clock::now();
        
        SM4_GCM_Encrypt(
            key, iv, sizeof(iv),
            aad.data(), aad_len,
            plaintext.data(), plaintext.size(),
            ciphertext.data(), tag.data(), tag.size()
        );
        
        // 结束计时
        auto end = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double> elapsed = end - start;
        encryption_times.push_back(elapsed.count());
    }

    // 计算平均加密时间
    double total_time = 0;
    for (double t : encryption_times) {
        total_time += t;
    }
    double avg_time = total_time / NUM_TESTS;

    // 输出结果
    std::cout << "测试完成! 结果:" << std::endl;
    std::cout << "测试次数: " << NUM_TESTS << std::endl;
    std::cout << "明文长度: " << PLAINTEXT_LEN << " 字节" << std::endl;
    std::cout << "平均加密时间: " << avg_time * 1000 << " 毫秒" << std::endl;
    std::cout << "总耗时: " << total_time << " 秒" << std::endl;
    return 0;
}