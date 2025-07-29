#ifndef SM4_AESNI_H
#define SM4_AESNI_H

#include<iostream>
#include<immintrin.h>

using namespace std;

typedef uint8_t word8;
typedef uint32_t word32;

void sm4_keyinit(word8 key[16], word32 sm4_key[32]);
void sm4_aesni_enc(word8 m[16], word8 c[16], word32 sm4_key[32]);
void sm4_aesni_dec(word8 m[16], word8 c[16], word32 sm4_key[32]);
__m128i MulMatrix(__m128i x, __m128i higherMask, __m128i lowerMask);
__m128i MulMatrixATA(__m128i x);
__m128i MulMatrixTA(__m128i x);
__m128i AddTC(__m128i x);
__m128i AddATAC(__m128i x);
__m128i SM4_SBox(__m128i x);
void SM4_AESNI_do(word8 input[16], word8 output[16], word32 sm4_key[32], int enc);

#endif
