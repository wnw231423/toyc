# Compiler for ToyC

## Configure dev environment
- run `docker build -t ubuntu-cpp-env .` to build an image.
- use `docker run -it --rm D:\\your-project:/home/compiler ubuntu-cpp-env bash` to mount your project dir to the container and run.

## Road Map
### lexing, parsing, ast generation
- [x] Very easy main.
- [x] Unary Expression. (`+`, `-`, `!`)
- [x] Binary Arithmatic Expression. (`+`, `-`, `*`, `/`, `%`)
- [ ] Compare and Logic Expression. (`<`, `>`, `<=`, `>=`, `==`, `!=`, `&&`, `||`)
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