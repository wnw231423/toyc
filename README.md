# Compiler for ToyC

## Configure dev environment
- run `docker build -t ubuntu-cpp-env .` to build an image.
- use `docker run -it --rm D:\\your-project:/home/compiler ubuntu-cpp-env bash` to mount your project dir to the container and run.

## Road Map
### lexing, parsing, ast generation
Basic features to meat the requirements of the ToyC language.
- [x] Very easy main.
- [x] Unary Expression (`+`, `-`, `!`).
- [x] Binary Arithmatic Expression (`+`, `-`, `*`, `/`, `%`).
- [x] Compare and Logic Expression (`<`, `>`, `<=`, `>=`, `==`, `!=`, `&&`, `||`).
- [ ] Easy VarDecl (i.e. `int a = 1;`).
...
Extended features to meat the requirements of the SysY language.
- [ ] ConstDecl (i.e. `const int a = 1;`) and extended VarDecl (`int a = 1, b = 2;`).
...

## Debugging Chronicles
1. Remove 'BOM' of `.l`, `.y` and the input `.tc` file for testing.

## PS Area
1. registry-mirrors for docker:
```
"registry-mirrors": [
    "https://dockerproxy.com",
    "https://docker.m.daocloud.io",
    "https://cr.console.aliyun.com",
    "https://ccr.ccs.tencentyun.com",
    "https://hub-mirror.c.163.com",
    "https://mirror.baidubce.com",
    "https://docker.nju.edu.cn",
    "https://docker.mirrors.sjtug.sjtu.edu.cn",
    "https://github.com/ustclug/mirrorrequest",
    "https://registry.docker-cn.com"
]
```