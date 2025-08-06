#ifndef __SM3_H__
#define __SM3_H__

#include<iostream>
#include<iomanip>

using namespace std;

typedef unsigned char word8;
typedef uint32_t word32;

extern word32 IV[8];

vector<word8> padding(vector<word8>& message);
word32 rotateleft (word32 x, int n);
word32 FF(word32 X, word32 Y, word32 Z, int j);
word32 GG(word32 X, word32 Y, word32 Z, int j);
word32 P0(word32 X);
word32 P1(word32 x);
void expand(vector<word8>& block, word32 W[68], word32 W1[64]);
void compress(word32 V[8], word32 W[68], word32 W1[64]);
vector<word8> sm3(vector<word8>& message);

#endif