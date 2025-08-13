# Innovation-and-Entrepreneurship-Practice

本课程项目均为本人独自完成

## 项目及完成情况：

Project 1: 做SM4的软件实现和优化 

a): 从基本实现出发 优化SM4的软件执行效率，至少应该覆盖T-table、AESNI以及最新的指令集（GFNI、VPROLD等）

b): 基于SM4的实现，做SM4-GCM工作模式的软件优化实现

见 [项目一](https://github.com/LiRunze2022/Innovation-and-Entrepreneurship-Practice/tree/main/project%201)

Project 2: 基于数字水印的图片泄露检测 

编程实现图片水印嵌入和提取（可依托开源项目二次开发），并进行鲁棒性测试，包括不限于翻转、平移、截取、调对比度等

见 [项目二](https://github.com/LiRunze2022/Innovation-and-Entrepreneurship-Practice/tree/main/project2)

Project 3: 用circom实现poseidon2哈希算法的电路
1) poseidon2哈希算法参数参考参考文档1的Table1，用(n,t,d)=(256,3,5)或(256,2,5)
2）电路的公开输入用poseidon2哈希值，隐私输入为哈希原象，哈希算法的输入只考虑一个block即可。
3) 用Groth16算法生成证明
参考文档：
1. poseidon2哈希算法https://eprint.iacr.org/2023/323.pdf
2. circom说明文档https://docs.circom.io/
3. circom电路样例 https://github.com/iden3/circomlib

见 [项目三](https://github.com/LiRunze2022/Innovation-and-Entrepreneurship-Practice/tree/main/project3)

Project 4: SM3的软件实现与优化 

见 [项目四](https://github.com/LiRunze2022/Innovation-and-Entrepreneurship-Practice/tree/main/project4)

Project 5: SM2的软件实现优化 

见 [项目五](https://github.com/LiRunze2022/Innovation-and-Entrepreneurship-Practice/tree/main/project%205)

Project 6:  Google Password Checkup验证
来自刘巍然老师的报告  google password checkup，参考论文 https://eprint.iacr.org/2019/723.pdf 的 section 3.1 ，也即 Figure 2 中展示的协议，尝试实现该协议，（编程语言不限）。

见 [项目六](https://github.com/LiRunze2022/Innovation-and-Entrepreneurship-Practice/tree/main/project6)
