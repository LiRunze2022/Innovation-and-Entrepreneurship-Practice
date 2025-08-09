# DDH-based Private Intersection-Sum Protocol

协议（DDH-based Protocol）旨在解决 Private Intersection-Sum with Cardinality (PI-Sum-C) 问题，即在保护隐私的前提下计算两方数据集的交集大小（Cardinality）和交集内关联值的总和（Sum）。以下是其核心协议流程：

## 协议目标

1. 输入：
  - P1：持有标识符集合 $V=\lbrace v_{i​}\rbrace$
  - P2：持有标识符-值对集合 $W=\lbrace ( w_{i​},t_{i} )\rbrace$

2. 输出：
  - P1：交集大小 $C=|V\bigcap W|$
  - P2：交集大小 $C$ 和关联值之和 $S=\Sigma_{j:w_{j}\in V}t_{j}$

 ## 协议流程

 ### 1.初始化阶段

1. 双方约定：
  - 素数阶群 $G$ (DDH假设成立）
  - 哈希函数 $H: U\rightarrow G$

2. 密钥生成：
  - P1随机选择 $k_{1} \in G$
  - P2随机选择 $k_{2} \in G$ 和同态加密密钥对 $(pk,sk) \leftarrow AGen(\lambda)$ ,并公开 $pk$

### 2.Round 1 (P1 $\rightarrow$ P2)

1. P1对每个 $v_{i}$ 计算：
  - $A_{i}=H(v_{i})^{k_{1}}$

2. P1将 $\lbrace A_{i} \rbrace$ 乱序发送给P2

### 3.Round 2 (P2 $\rightarrow$ P1)

1. 处理P1的数据：
  - P2对每个 $A_{i}$ 计算: $B_{i}=A_{i}^{k_{2}}=H(v_{i})^{k_{1}k_{2}}$
  - P2将 $\lbrace B_{i} \rbrace$ 乱序发送给P1

2. 处理自身数据：
  - P2对每个 $(w_{j}, t_{j})$ 计算：
    1. $C_{j}=H(w_{j})^{k_{2}}$
    2. $D_{j}=AEnc(pk,t_{j})$
  - P2将 $\lbrace (C_{j}, D_{j}) \rbrace$ 乱序发送给P1

### 4.Round 3 (P1 $\rightarrow$ P2)
1. P1对每个 $(C_{j},D_{j})$ 计算： $E_{j}=C_{j}^{k_{1}}=H(w_{j})^{k_{1}k_{2}}$
2. 计算交集：
  - P1构造集合 $Z=\lbrace B_{i} \rbrace$
  - 确定交集索引： $J=\lbrace j|E_{j}\in Z\rbrace$
3. 同态聚合：
  - P1计算交集值之和的加密： $AEnc(S)=ASum(\lbrace D_{j} \rbrace)$
  - 刷新密文随机性： $AEnc^{\prime}(S)\rightarrow ARefresh(AEnc(S))$
4. P1发送 $$AEnc^{\prime}(S)$ 给P2

### 5.输出阶段

1. P2：解密 $S\rleft
