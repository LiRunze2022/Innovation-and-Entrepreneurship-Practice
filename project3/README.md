# 用circom实现poseidon2哈希算法的电路

1) poseidon2哈希算法参数参考参考文档1的Table1，用(n,t,d)=(256,3,5)或(256,2,5)

2) 电路的公开输入用poseidon2哈希值，隐私输入为哈希原象，哈希算法的输入只考虑一个block即可。

3) 用Groth16算法生成证明

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

## circom电路编写

