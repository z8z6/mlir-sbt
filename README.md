# mlir-sbt

## 项目结构

1. 依赖开源库

- LLVM

```sh
# pull
git submodule add https://github.com/llvm/llvm-project.git third/llvm
cd third/llvm
git checkout llvmorg-22.1.5

# build
mkdir build
cmake -S llvm -G "Ninja" -B build       \
    -DLLVM_ENABLE_PROJECTS="llvm;mlir"  \
    -DLLVM_TARGETS_TO_BUILD="X86;RISCV" \
    -DCMAKE_BUILD_TYPE="Debug"
cmake --build build --config Debug -j 12
```

- GoogleTest

```sh
# pull
git submodule add https://github.com/google/googletest.git third/gtest
cd third/gtest
git checkout v1.17.0

# build
mkdir build
cd build
cmake ..
make -j
```