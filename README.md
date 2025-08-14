﻿# Compiler for ToyC

## info
- Branch `interface` is used for testing on school server.
- Branch `main` is used for better development.

## Configure dev environment
- run `docker build -t ubuntu-cpp-env .` to build an image.
- use `docker run -it --rm -v D:\\your-project:/home/compiler ubuntu-cpp-env bash` to mount your project dir to the container and run.

## Road Map

### lexing, parsing, ast generation

Basic features to meat the requirements of the ToyC language.

- [x] Very easy main.
- [x] Unary Expression (`+`, `-`, `!`).
- [x] Binary Arithmatic Expression (`+`, `-`, `*`, `/`, `%`).
- [x] Compare and Logic Expression (`<`, `>`, `<=`, `>=`, `==`, `!=`, `&&`, `||`).
- [x] Easy VarDecl and VarAssign (i.e. `int a = 1; a = a + 1;`).
- [x] Block as statement (i.e. `{ int a = 1; }`).
- [x] If-Else statement (i.e. `if (a > 1) { a = 2; } else { a = 3; }`).
- [x] While statement (i.e. `while (a < 10) { a = a + 1; }`).
- [x] Function declaration and call (i.e. `int f(int a) { return a + 1; } f(1);`).

### Symbol table

- [x] Symbol table implementation.
...

### IR generation

- [x] IR implementation
- [x] evaluate AST to IR, using symbol table.
- [x] correctness test
...

### optimization
- [x] consprop
- [ ] inline (未完成)
...

### RISCV generation
- [x] reg allocation
- [x] RISCV generation
- [x] correctness test

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
