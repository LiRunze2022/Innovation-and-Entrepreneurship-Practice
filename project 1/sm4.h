#ifndef __SM4_H__
#define __SM4_H__

#include<iostream>
#include<iomanip>

using namespace std;

typedef unsigned char word8;
typedef uint32_t word32;

extern word8 Sbox[16][16];
extern word32 FK[4];
extern word32 CK[32];
extern word8 key[16];
extern word32 roundkey[32];

word8 sm4Sbox(word8 x);
word32 sm4T(word32 x);
word32 sm4F(word32 x0, word32 x1, word32 x2, word32 x3, word32 rk);
word32 sm4T_prime(word32 x);
void sm4gen_rk(word32 roundkey[32], word8 key[16]);
void sm4enc(word8 c[16], word8 m[16], word8 key[16]);
void sm4dec(word8 c[16], word8 m[16], word8 key[16]);

#endif
