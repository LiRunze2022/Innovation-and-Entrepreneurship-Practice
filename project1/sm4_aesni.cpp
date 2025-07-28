#include<iostream>
#include<iomanip>
#include <ctime>
#include"sm4_aesni.h"
#include<immintrin.h>

using namespace std;

#define rotl(x, n) (((x) << (n)) | ((x) >> (32 - (n))))

#define MM_PACK0_EPI32(a, b, c, d) _mm_unpacklo_epi64(_mm_unpacklo_epi32(a, b), _mm_unpacklo_epi32(c, d))
#define MM_PACK1_EPI32(a, b, c, d) _mm_unpackhi_epi64(_mm_unpacklo_epi32(a, b), _mm_unpacklo_epi32(c, d))
#define MM_PACK2_EPI32(a, b, c, d) _mm_unpacklo_epi64(_mm_unpackhi_epi32(a, b), _mm_unpackhi_epi32(c, d))
#define MM_PACK3_EPI32(a, b, c, d) _mm_unpackhi_epi64(_mm_unpackhi_epi32(a, b), _mm_unpackhi_epi32(c, d))

#define MM_XOR2(a, b) _mm_xor_si128(a, b)
#define MM_XOR3(a, b, c) MM_XOR2(a, MM_XOR2(b, c))
#define MM_XOR4(a, b, c, d) MM_XOR2(a, MM_XOR3(b, c, d))
#define MM_XOR5(a, b, c, d, e) MM_XOR2(a, MM_XOR4(b, c, d, e))
#define MM_XOR6(a, b, c, d, e, f) MM_XOR2(a, MM_XOR5(b, c, d, e, f))
#define MM_ROTL_EPI32(a, n) MM_XOR2(_mm_slli_epi32(a, n), _mm_srli_epi32(a, 32 - n))

word8 Sbox[16][16] =
    {
        {0xd6, 0x90, 0xe9, 0xfe, 0xcc, 0xe1, 0x3d, 0xb7, 0x16, 0xb6, 0x14, 0xc2, 0x28, 0xfb, 0x2c, 0x05},
        {0x2b, 0x67, 0x9a, 0x76, 0x2a, 0xbe, 0x04, 0xc3, 0xaa, 0x44, 0x13, 0x26, 0x49, 0x86, 0x06, 0x99},
        {0x9c, 0x42, 0x50, 0xf4, 0x91, 0xef, 0x98, 0x7a, 0x33, 0x54, 0x0b, 0x43, 0xed, 0xcf, 0xac, 0x62},
        {0xe4, 0xb3, 0x1c, 0xa9, 0xc9, 0x08, 0xe8, 0x95, 0x80, 0xdf, 0x94, 0xfa, 0x75, 0x8f, 0x3f, 0xa6},
        {0x47, 0x07, 0xa7, 0xfc, 0xf3, 0x73, 0x17, 0xba, 0x83, 0x59, 0x3c, 0x19, 0xe6, 0x85, 0x4f, 0xa8},
        {0x68, 0x6b, 0x81, 0xb2, 0x71, 0x64, 0xda, 0x8b, 0xf8, 0xeb, 0x0f, 0x4b, 0x70, 0x56, 0x9d, 0x35},
        {0x1e, 0x24, 0x0e, 0x5e, 0x63, 0x58, 0xd1, 0xa2, 0x25, 0x22, 0x7c, 0x3b, 0x01, 0x21, 0x78, 0x87},
        {0xd4, 0x00, 0x46, 0x57, 0x9f, 0xd3, 0x27, 0x52, 0x4c, 0x36, 0x02, 0xe7, 0xa0, 0xc4, 0xc8, 0x9e},
        {0xea, 0xbf, 0x8a, 0xd2, 0x40, 0xc7, 0x38, 0xb5, 0xa3, 0xf7, 0xf2, 0xce, 0xf9, 0x61, 0x15, 0xa1},
        {0xe0, 0xae, 0x5d, 0xa4, 0x9b, 0x34, 0x1a, 0x55, 0xad, 0x93, 0x32, 0x30, 0xf5, 0x8c, 0xb1, 0xe3},
        {0x1d, 0xf6, 0xe2, 0x2e, 0x82, 0x66, 0xca, 0x60, 0xc0, 0x29, 0x23, 0xab, 0x0d, 0x53, 0x4e, 0x6f},
        {0xd5, 0xdb, 0x37, 0x45, 0xde, 0xfd, 0x8e, 0x2f, 0x03, 0xff, 0x6a, 0x72, 0x6d, 0x6c, 0x5b, 0x51},
        {0x8d, 0x1b, 0xaf, 0x92, 0xbb, 0xdd, 0xbc, 0x7f, 0x11, 0xd9, 0x5c, 0x41, 0x1f, 0x10, 0x5a, 0xd8},
        {0x0a, 0xc1, 0x31, 0x88, 0xa5, 0xcd, 0x7b, 0xbd, 0x2d, 0x74, 0xd0, 0x12, 0xb8, 0xe5, 0xb4, 0xb0},
        {0x89, 0x69, 0x97, 0x4a, 0x0c, 0x96, 0x77, 0x7e, 0x65, 0xb9, 0xf1, 0x09, 0xc5, 0x6e, 0xc6, 0x84},
        {0x18, 0xf0, 0x7d, 0xec, 0x3a, 0xdc, 0x4d, 0x20, 0x79, 0xee, 0x5f, 0x3e, 0xd7, 0xcb, 0x39, 0x48}
    }; 

word32 FK[4] = {0xa3b1bac6, 0x56aa3350, 0x677d9197, 0xb27022dc};

word32 CK[32] =
    {
        0x00070e15, 0x1c232a31, 0x383f464d, 0x545b6269,
        0x70777e85, 0x8c939aa1, 0xa8afb6bd, 0xc4cbd2d9,
        0xe0e7eef5, 0xfc030a11, 0x181f262d, 0x343b4249,
        0x50575e65, 0x6c737a81, 0x888f969d, 0xa4abb2b9,
        0xc0c7ced5, 0xdce3eaf1, 0xf8ff060d, 0x141b2229,
        0x30373e45, 0x4c535a61, 0x686f767d, 0x848b9299,
        0xa0a7aeb5, 0xbcc3cad1, 0xd8dfe6ed, 0xf4fb0209,
        0x10171e25, 0x2c333a41, 0x484f565d, 0x646b7279
    };

word8 sm4Sbox(word8 x)
{
    int a = x >> 4, b = x & 0xf;
    word8 result = Sbox[a][b];
    return result;
}

word32 sm4T_prime(word32 x)
{
    word32 result = 0;
    result |= ((word32)sm4Sbox((word8)(x)));
    result |= ((word32)sm4Sbox((word8)(x >> 8))) << 8;
    result |= ((word32)sm4Sbox((word8)(x >> 16))) << 16;
    result |= ((word32)sm4Sbox((word8)(x >> 24))) << 24;

    return result ^ rotl(result, 13) ^ rotl(result, 23);
}

void sm4_keyinit(word8 key[16], word32 sm4_key[32])
{
    word32 MK[4];
    MK[0] = (((word32)key[0]) << 24) ^ (((word32)key[1]) << 16) ^ (((word32)key[2]) << 8) ^ ((word32)key[3]);
    MK[1] = (((word32)key[4]) << 24) ^ (((word32)key[5]) << 16) ^ (((word32)key[6]) << 8) ^ ((word32)key[7]);
    MK[2] = (((word32)key[8]) << 24) ^ (((word32)key[9]) << 16) ^ (((word32)key[10]) << 8) ^ ((word32)key[11]);
    MK[3] = (((word32)key[12]) << 24) ^ (((word32)key[13]) << 16) ^ (((word32)key[14]) << 8) ^ ((word32)key[15]);

    word32 k[36];
    k[0] = MK[0] ^ FK[0];
    k[1] = MK[1] ^ FK[1];
    k[2] = MK[2] ^ FK[2];
    k[3] = MK[3] ^ FK[3];

    for(int i = 0; i < 32; i++)
    {
        k[i + 4] = k[i + 0] ^ sm4T_prime(k[i + 1] ^ k[i + 2] ^ k[i + 3] ^ CK[i]);
        sm4_key[i] = k[i + 4];
    }
}

__m128i MulMatrix(__m128i x, __m128i higherMask, __m128i lowerMask)
{
    __m128i tmp1, tmp2;
    __m128i andMask = _mm_set1_epi32(0x0f0f0f0f);
    tmp2 = _mm_srli_epi16(x, 4);
    tmp1 = _mm_and_si128(x, andMask);
    tmp2 = _mm_and_si128(tmp2, andMask);
    tmp1 = _mm_shuffle_epi8(lowerMask, tmp1);
    tmp2 = _mm_shuffle_epi8(higherMask, tmp2);
    tmp1 = _mm_xor_si128(tmp1, tmp2);
    return tmp1;
}

__m128i MulMatrixATA(__m128i x) {
    __m128i higherMask =
        _mm_set_epi8(0x14, 0x07, 0xc6, 0xd5, 0x6c, 0x7f, 0xbe, 0xad, 0xb9, 0xaa,
                     0x6b, 0x78, 0xc1, 0xd2, 0x13, 0x00);
    __m128i lowerMask =
        _mm_set_epi8(0xd8, 0xb8, 0xfa, 0x9a, 0xc5, 0xa5, 0xe7, 0x87, 0x5f, 0x3f,
                     0x7d, 0x1d, 0x42, 0x22, 0x60, 0x00);
    return MulMatrix(x, higherMask, lowerMask);
}

__m128i MulMatrixTA(__m128i x) {
    __m128i higherMask =
        _mm_set_epi8(0x22, 0x58, 0x1a, 0x60, 0x02, 0x78, 0x3a, 0x40, 0x62, 0x18,
                     0x5a, 0x20, 0x42, 0x38, 0x7a, 0x00);
    __m128i lowerMask =
        _mm_set_epi8(0xe2, 0x28, 0x95, 0x5f, 0x69, 0xa3, 0x1e, 0xd4, 0x36, 0xfc,
                     0x41, 0x8b, 0xbd, 0x77, 0xca, 0x00);
    return MulMatrix(x, higherMask, lowerMask);
}

__m128i AddTC(__m128i x) {
    __m128i TC = _mm_set1_epi8(0b00100011);
    return _mm_xor_si128(x, TC);
}

__m128i AddATAC(__m128i x) {
    __m128i ATAC = _mm_set1_epi8(0b00111011);
    return _mm_xor_si128(x, ATAC);
}

__m128i SM4_SBox(__m128i x) {
    __m128i MASK = _mm_set_epi8(0x03, 0x06, 0x09, 0x0c, 0x0f, 0x02, 0x05, 0x08,
                                0x0b, 0x0e, 0x01, 0x04, 0x07, 0x0a, 0x0d, 0x00);
    x = _mm_shuffle_epi8(x, MASK);  //逆行移位
    x = AddTC(MulMatrixTA(x));
    x = _mm_aesenclast_si128(x, _mm_setzero_si128());
    return AddATAC(MulMatrixATA(x));
}

void SM4_AESNI_do(word8 in[16], word8 out[16], word32 sm4_key[32], int enc)
{
    __m128i X[4], Tmp[4];
    __m128i vindex;
    // Load Data
    Tmp[0] = _mm_loadu_si128((const __m128i*)in + 0);
    Tmp[1] = _mm_loadu_si128((const __m128i*)in + 1);
    Tmp[2] = _mm_loadu_si128((const __m128i*)in + 2);
    Tmp[3] = _mm_loadu_si128((const __m128i*)in + 3);
    vindex =
        _mm_setr_epi8(3, 2, 1, 0, 7, 6, 5, 4, 11, 10, 9, 8, 15, 14, 13, 12);
    // Pack Data
    X[0] = MM_PACK0_EPI32(Tmp[0], Tmp[1], Tmp[2], Tmp[3]);
    X[1] = MM_PACK1_EPI32(Tmp[0], Tmp[1], Tmp[2], Tmp[3]);
    X[2] = MM_PACK2_EPI32(Tmp[0], Tmp[1], Tmp[2], Tmp[3]);
    X[3] = MM_PACK3_EPI32(Tmp[0], Tmp[1], Tmp[2], Tmp[3]);
    // Shuffle Endian
    X[0] = _mm_shuffle_epi8(X[0], vindex);
    X[1] = _mm_shuffle_epi8(X[1], vindex);
    X[2] = _mm_shuffle_epi8(X[2], vindex);
    X[3] = _mm_shuffle_epi8(X[3], vindex);

    for (int i = 0; i < 32; i++) {
        __m128i k = _mm_set1_epi32((enc == 0) ? sm4_key[i] : sm4_key[31 - i]);
        Tmp[0] = MM_XOR4(X[1], X[2], X[3], k);
        // SBox
        Tmp[0] = SM4_SBox(Tmp[0]);
        // L
        Tmp[0] = MM_XOR6(X[0], Tmp[0], MM_ROTL_EPI32(Tmp[0], 2), MM_ROTL_EPI32(Tmp[0], 10), MM_ROTL_EPI32(Tmp[0], 18), MM_ROTL_EPI32(Tmp[0], 24));
        X[0] = X[1];
        X[1] = X[2];
        X[2] = X[3];
        X[3] = Tmp[0];
    }
    // Shuffle Endian
    X[0] = _mm_shuffle_epi8(X[0], vindex);
    X[1] = _mm_shuffle_epi8(X[1], vindex);
    X[2] = _mm_shuffle_epi8(X[2], vindex);
    X[3] = _mm_shuffle_epi8(X[3], vindex);
    // Pack and Store
    _mm_storeu_si128((__m128i*)out + 0, MM_PACK0_EPI32(X[3], X[2], X[1], X[0]));
    _mm_storeu_si128((__m128i*)out + 1, MM_PACK1_EPI32(X[3], X[2], X[1], X[0]));
    _mm_storeu_si128((__m128i*)out + 2, MM_PACK2_EPI32(X[3], X[2], X[1], X[0]));
    _mm_storeu_si128((__m128i*)out + 3, MM_PACK3_EPI32(X[3], X[2], X[1], X[0]));
}

void sm4_aesni_enc(word8 m[16], word8 c[16], word32 sm4_key[32])
{
    SM4_AESNI_do(m, c, sm4_key, 0);
}

void sm4_aesni_dec(word8 m[16], word8 c[16], word32 sm4_key[32])
{
    SM4_AESNI_do(c, m, sm4_key, 1);
}

int main()
{
    word8 m[16];
    word8 c[16];
    word8 k[16];

    for(int i = 0; i < 16; i++)
    {
        m[i] = 0x08;
        k[i] = 0x16;
    }

    clock_t start = clock();
    word32 sm4_key[32];
    sm4_keyinit(k, sm4_key);
    sm4_aesni_enc(m, c, sm4_key);
    clock_t end = clock();

    double duration = (double)(end - start) / CLOCKS_PER_SEC * 1000;

    std::cout << "耗时: " << duration << " 毫秒" << std::endl;

    for(int i = 0; i < 16; i++)
    {
        cout << hex << (int)c[i] << " ";
    }
    cout << endl;

    sm4_aesni_dec(m, c, sm4_key);

    for(int i = 0; i < 16; i++)
    {
        cout << hex << (int)m[i] << " ";
    }
    cout<<endl;

    return 0;
}