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
