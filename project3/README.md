# 用circom实现poseidon2哈希算法的电路

1) poseidon2哈希算法参数参考参考文档1的Table1，用(n,t,d)=(256,3,5)或(256,2,5)

2) 电路的公开输入用poseidon2哈希值，隐私输入为哈希原象，哈希算法的输入只考虑一个block即可。

3) 用Groth16算法生成证明

## 概述

Poseidon2 是第二代**基于置换的哈希函数**，专为算术电路设计，具有以下特点：
- **零知识友好**：仅使用域内加法和乘法（适合 Groth16、Plonk 等 ZKP 系统）
- **高效性**：相比 Poseidon，减少轮数（`d=5` 即可安全），降低约束数
- **可调参数**：支持灵活配置状态大小（`t`）、轮数（`d`）和 S-box 指数（`α`）


## 环境配置

- Node.js：命令行中输入

```shell
curl -fsSL https://deb.nodesource.com/setup_18.x | sudo -E bash -  
sudo apt install -y nodejs
```

- circom：命令行中输入

```shell
sudo npm install -g circom
```

- snarkjs：命令行中输入

```shell
sudo npm install -g snarkjs
```

## Poseidon2轮函数

每轮包含 AddRoundConstants、S-box 和 MixLayer 三步：
- AddRoundConstants：每轮添加预计算的常数 C[r][i]
- S-box 层：对每个状态元素应用幂函数（ $\alpha=5$ ）
- MixLayer（线性扩散）：进行矩阵乘法

## circom电路编写

参考 [代码](./poseidon2.circom)

## 编译电路

```shell
circom poseidon2.circom --r1cs --wasm --sym
```
