#include<iostream>
#include<iomanip>
#include <ctime>
#include <random>
#include <chrono> 
#include<vector>
#include"sm3.h"

using namespace std;

word32 IV[8] = {0x7380166F, 0x4914B2B9, 0x172442D7, 0xDA8A0600, 0xA96F30BC, 0x163138AA, 0xE38DEE4D, 0xB0FB0E4E};

vector<word8> padding(vector<word8>& message)
{
    size_t length = message.size();
    size_t bitlength = length * 8;
    size_t k = (64 + 56 - ((length + 1) % 64)) % 64;

    vector<word8> padded = message;
    padded.push_back(0x80);

    for(int i = 0; i < k; i++)
    {
        padded.push_back(0x00);
    }

    for(int i = 7; i >= 0; i--)
    {
        padded.push_back(static_cast<word8>((bitlength >> (i * 8)) & 0xFF));
    }

    return padded;
}

word32 rotateleft (word32 x, int n)
{
    return (x << n) | (x >> (32 - n));
}

word32 FF(word32 X, word32 Y, word32 Z, int j)
{
    if (j <= 15) 
    {
        return X ^ Y ^ Z;
    } 
    else
    {
        return (X & Y) | (X & Z) | (Y & Z);    
    }
}

word32 GG(word32 X, word32 Y, word32 Z, int j)
{
    if (j <= 15) 
    {
        return X ^ Y ^ Z;
    } 
    else
    {
        return (X & Y) | (~X & Z);
    }
}

word32 P0(word32 X)
{
    return X ^ rotateleft(X, 9) ^ rotateleft(X, 17);
}

word32 P1(word32 x)
{
    return x ^ rotateleft(x, 15) ^ rotateleft(x, 23);
}

void expand(vector<word8>& block, word32 W[68], word32 W1[64])
{
    for(int i = 0; i < 16; i++)
    {
        W[i] = ((word32)block[i * 4] << 24) | ((word32)block[i * 4 + 1] << 16) | ((word32)block[i * 4 + 2] << 8) | (word32)block[i * 4 + 3];
    }

    for(int j = 16; j < 68; j++)
    {
        W[j] = P1(W[j-16] ^ W[j-9] ^ rotateleft(W[j-3], 15)) ^ rotateleft(W[j-13], 7) ^ W[j-6];
    }

    for(int j = 0; j < 64; j++)
    {
        W1[j] = W[j] ^ W[j + 4];
    }
}

void compress(word32 V[8], word32 W[68], word32 W1[64])
{
    word32 A = V[0];
    word32 B = V[1];
    word32 C = V[2];
    word32 D = V[3];
    word32 E = V[4];
    word32 F = V[5];
    word32 G = V[6];
    word32 H = V[7];

    word32 SS1, SS2, TT1, TT2;

    for(int j = 0; j < 64; j++)
    {
        SS1 = rotateleft(rotateleft(A, 12) + E + rotateleft(0x79CC4519, j % 32), 7);

        SS2 = SS1 ^ rotateleft(A, 12);

        TT1 = FF(A, B, C, j) + D + SS2 + W1[j];

        TT2 = GG(E, F, G, j) + H + SS1 + W[j];

        D = C;
        C = rotateleft(B, 9);
        B = A;
        A = TT1;
        H = G;
        G = rotateleft(F, 19);
        F = E;
        E = P0(TT2);
    }

    V[0] ^= A;
    V[1] ^= B;
    V[2] ^= C;
    V[3] ^= D;
    V[4] ^= E;
    V[5] ^= F;
    V[6] ^= G;
    V[7] ^= H;
}

vector<word8> sm3(vector<word8>& message)
{
    //初始化寄存器
    word32 V[8];
    for(int i = 0; i < 8; i++)
    {
        V[i] = IV[i];
    }

    vector<word8> padded = padding(message);

    for(int i = 0; i < padded.size(); i += 64)
    {
        vector<word8> block(padded.begin() + i, padded.begin() + i + 64);
        word32 W[68];
        word32 W1[64];
        expand(block, W, W1);

        compress(V, W, W1);
    }

    vector<word8> digest(32);
    for (int i = 0; i < 8; ++i) 
    {
        digest[i * 4] = static_cast<uint8_t>((V[i] >> 24) & 0xFF);
        digest[i * 4 + 1] = static_cast<uint8_t>((V[i] >> 16) & 0xFF);
        digest[i * 4 + 2] = static_cast<uint8_t>((V[i] >> 8) & 0xFF);
        digest[i * 4 + 3] = static_cast<uint8_t>(V[i] & 0xFF);
    }

    return digest;
}

int main()
{
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
        vector<word8> hash = sm3(message);
        
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