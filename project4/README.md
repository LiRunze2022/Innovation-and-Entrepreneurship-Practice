# SM3算法的软件实现与优化

## SM3算法

SM3 是中国国家密码管理局发布的密码哈希算法，输出 256 位（32 字节） 哈希值，适用于数字签名、消息认证等场景。其核心流程分为消息填充、消息扩展、压缩函数迭代三部分。

### 消息填充
输入消息长度必须为512位（64字节）的整数倍，填充规则：

1. **补位规则**
   - 在消息末尾补`1`（`0x80`）
   - 补`k`个`0`，使得：  
     `消息长度 + 1 + k ≡ 448 mod 512`

2. **附加长度**
   - 最后64位写入原始消息的**位长度**（大端序）

### 消息扩展（Message Expansion）
将512位分组扩展为：
- 68个32位字（W₀~W₆₇）
- 64个32位字（W'₀~W'₆₃）

扩展步骤:

1. **前16个字**  
   `W_i = Block[4i:4i+4]`（大端序）

2. **后续字（W₁₆~W₆₇）**

$W_{j} = P1(W_{j-16} \oplus W_{j-9} \oplus ROL(W_{j-3},15)) \oplus ROL(W_{j-13},7) \oplus W_{j-6}$

- `P1(X) = X ⊕ (X <<< 15) ⊕ (X <<< 23)`
- `ROL(x,n)`：循环左移n位

3. **计算W'**

`W'_j = W_j ⊕ W_{j+4}`

### 压缩函数（Compression Function）
采用Merkle-Damgård结构，64轮迭代更新8个寄存器（V₀~V₇）

寄存器初始化
```cpp
uint32_t IV[8] = {
 0x7380166F, 0x4914B2B9, 0x172442D7, 0xDA8A0600,
 0xA96F30BC, 0x163138AA, 0xE38DEE4D, 0xB0FB0E4E
};
```

64轮迭代：

1. 常量计算：根据轮数j选择Tj常量(前16轮0x79CC4519，后48轮0x7A879D8A)
2. 中间值计算：
   1. SS1 = rotateleft(rotateleft(A,12) + E + rotateleft(Tj,j%32), 7)
   2. SS2 = SS1 ^ rotateleft(A, 12)
3. 数据混合：
   1. TT1 = FF(A,B,C,j) + D + SS2 + W1[j]
   2. TT2 = GG(E,F,G,j) + H + SS1 + W[j]
4. 寄存器更新：
   1. (D, C, B, A) ← (C, rotateleft(B,9), A, TT1)
   2. (H, G, F, E) ← (G, rotateleft(F,19), E, P0(TT2))

## SM3算法SIMD优化

1. SIMD指令使用：

  - 使用AVX2指令集(__m256i类型)并行处理数据
  - 实现了SIMD版本的rotateleft, FF, GG, P0, P1等核心函数
2. 关键优化点：
  - expand_avx和compress_avx函数使用AVX2指令重写
  - 使用_mm256_set1_epi32广播标量值到整个SIMD寄存器
  - 使用_mm256_xor_si256等指令进行并行位操作
3. 内存访问优化：
  - 使用alignas(32)确保内存对齐，提高AVX指令效率
  - 减少不必要的内存访问，尽量在寄存器中操作

## 结果对比

