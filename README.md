# mlir-sbt

## 项目结构

1. 依赖开源库

- LLVM

```sh
git submodule add https://github.com/llvm/llvm-project.git third/llvm
cd third/llvm
git checkout llvmorg-22.1.5
```

- GoogleTest

```sh
git submodule add https://github.com/google/googletest.git third/gtest
cd third/gtest
git checkout v1.17.0
```