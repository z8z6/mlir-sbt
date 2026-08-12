# 开发指南与实现状态

## 1. 依赖

当前 CMake 配置要求：

- CMake 3.31 或更新版本；
- C++17 编译器；
- Ninja（README 中的 LLVM 构建方式使用 Ninja）；
- LLVM 与 MLIR；仓库当前约定源码位于 `third/llvm`，构建目录为 `third/llvm/build`；
- NASM，用于汇编 `tests/arith/add` 中的样例；
- GoogleTest 子模块已经存在，但当前主工程尚未建立测试目标。

README 记录的 LLVM 基线为 `llvmorg-22.1.5`，配置时启用 `llvm;mlir` 项目和 `X86;RISCV` targets。当前代码实际只实现了 x86 machine 选择。

## 2. 构建

先按仓库根目录 README 构建 LLVM/MLIR，再构建本项目：

```sh
cmake -S . -B cmake-build-debug -G Ninja -DCMAKE_BUILD_TYPE=Debug
cmake --build cmake-build-debug -j 12
```

构建过程包含两个代码生成步骤：

1. `mlir-tblgen` 从 `mlir/IR1.td` 生成 Dialect 和 Op 声明/定义；
2. 项目自有的 `ir1-tblgen` 读取 LLVM x86 TableGen 记录，生成 `X86IR1Converter.h/.cpp`。

生成文件位于构建目录，不应手工编辑。需要修改操作定义时编辑 `mlir/IR1.td`；需要修改转换器生成规则时编辑 `tblgen/` 下的生成器源码。

## 3. 运行

当前唯一必需参数是输入目标文件：

```sh
./cmake-build-debug/sbt -i tests/arith/add/main
```

生成 ADD 样例可执行：

```sh
cd tests/arith/add
./run.sh
```

注意：`run.sh` 当前固定汇编 `ADDri.asm` 并把 ELF relocatable object 命名为 `main`。该名称不是可执行文件的保证；`sbt` 当前只是读取其中的代码 section。

当前程序会依次向输出流打印：

- IR0 反汇编；
- 提升后的 IR1 module；
- 执行 partial lowering 后的 module。

它不会写出 LLVM IR、目标文件或可执行程序。

## 4. 添加一条 x86 指令语义

现有机制的工作方式是：

1. `tblgen/src/X86HeaderGenerator.cpp` 按 opcode 名筛选 LLVM x86 指令记录；
2. 生成的类继承 `BaseIR1Converter`，保存 opcode 和 `MCInstrDesc`；
3. `X86ImplGenerator.cpp` 生成 `convertMCInst` 的 switch；
4. 手写 C++ 覆盖 `op()`，必要时也覆盖操作数 load/store。

目前筛选条件硬编码为 `OpcodeName.starts_with("ADD")`，而生成头文件中的 `op()` 声明被注释。新增指令族前，建议先把生成模型改为显式 TableGen 标注：只为已实现语义的 opcode 生成分派，并生成一致的 override 声明。这样可以避免“已经接受但实际使用基类空实现”。

每个转换器至少要处理：

- 显式 def/use 与 tied operand；
- 隐式寄存器读写；
- 操作数位宽和立即数扩展；
- 寄存器别名；
- flags 的 defined/preserved/undefined 集合；
- 内存副作用、异常和控制流；
- 不支持变体的确定性错误。

## 5. 当前测试与建议布局

现有 `tests/arith/add` 是汇编输入集合，尚未接入 CTest，也没有断言预期结果。建议演进为：

```text
tests/
  unit/                 # IR builder、寄存器别名、flags 公式
  lift/                 # 汇编 → 期望 IR1/FileCheck
  lower/                # IR1 → LLVM Dialect/LLVM IR
  differential/         # 原始代码与翻译代码状态对比
  negative/             # 非法文件、decode fail、unsupported opcode
```

测试层次：

- 单元测试检查纯语义函数；
- FileCheck 风格测试检查 IR 结构，不依赖 SSA 临时名；
- 差分测试检查真实执行结果；
- fuzz 测试在已声明支持的 opcode/operand 范围内随机生成输入。

## 6. 已知技术债

以下问题是继续扩大指令覆盖前的高优先级事项：

- `File::CurrentFile`、`IR1Context::Instance()` 和静态 x86 machine 带来全局状态和生命周期限制；
- `getMachine` 对非 x86 triple 没有明确失败路径；
- `File::disas()` 线性扫描所有 text section，忽略符号、relocation 和代码/数据边界；
- decode fail 仅跳过一个字节；
- 未支持 opcode 静默跳过；
- `BaseIR1Converter::run()` 无条件 dump `MCInst`，缺少可控日志级别；
- IR1 的整数类型约束过宽，builder 又固定使用 i32/i64；
- IR1 操作没有完整 verifier、side effect interface 和 traits；
- `IR1Context::lower()` 忽略 PassManager 返回值；
- `IR1Context::verify()` 只打印错误，主流程也没有调用它；
- lowering 仅处理整数常量，且使用 partial conversion；
- module 没有函数、region 中的真实 CFG 和 terminator；
- CMake 尚未配置 LLVM Dialect 到 LLVM IR 的 translation 与目标发射。

## 7. 修改约定

- 将“当前实现”和“目标设计”分开描述；
- 新增 IR 操作时同时添加 verifier、effects、lowering 和测试；
- 新增 opcode 时更新支持矩阵，禁止无声忽略；
- 不提交由构建产生的样例 object 或生成文件；
- 先建立一个端到端纵向切片，再批量扩展 opcode。

