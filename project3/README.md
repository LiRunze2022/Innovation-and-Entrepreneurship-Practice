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

## 生成 Groth16 证明

### 生成 trusted setup

```shell
# 1. 生成 ptau（powers of tau）
snarkjs powersoftau new bn128 12 pot12_0000.ptau -v
snarkjs powersoftau contribute pot12_0000.ptau pot12_0001.ptau --name="First Contributor" -v

# 2. 生成 zkey（Groth16 密钥对）
snarkjs groth16 setup poseidon2.r1cs pot12_0001.ptau poseidon2_0000.zkey
snarkjs zkey contribute poseidon2_0000.zkey poseidon2_0001.zkey --name="Second Contributor" -v
snarkjs zkey export verificationkey poseidon2_0001.zkey verification_key.json
```

### 生成证明

创建输入文件 input.json：

```json
{
  "private_input": ["10", "20"]
  "out": "978769251912840039204148718141236740445251797222770407128323859497250111719"
}
```

运行

```shell
# 1. 计算 witness
node poseidon2_js/generate_witness.js poseidon2_js/poseidon2.wasm input.json witness.wtns

# 2. 生成证明
snarkjs groth16 prove poseidon2_0001.zkey witness.wtns proof.json public.json

# 3. 验证证明
snarkjs groth16 verify verification_key.json public.json proof.json
```
