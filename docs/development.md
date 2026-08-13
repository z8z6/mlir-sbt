# 开发指南与实现状态

## 1. 依赖

当前 CMake 配置要求：

- CMake 3.31 或更新版本；
- C++17 编译器；
- Ninja（README 中的 LLVM 构建方式使用 Ninja）；
- LLVM 与 MLIR；仓库当前约定源码位于 `third/llvm`，构建目录为 `third/llvm/build`；
- NASM，用于汇编 `tests/isa/x86` 中的指令 fixture；
- GoogleTest，用于 lowering 和目标发射单元测试。

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

`sbt` 的输入是目标文件，`-o` 指定翻译后的目标文件：

```sh
./build/sbt -i input.o -o translated.o
```

当前程序会依次向输出流打印：

- IR0 反汇编；
- 提升后的 IR1 module；
- 完整 lowering 后的 LLVM Dialect module。

随后通过 LLVM TargetMachine 写出 `-o` 指定的 relocatable object。测试构建使用
`--quiet` 关闭中间打印。

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

## 5. 当前测试

`tests/isa/x86/<instruction>/<case>` 已接入 CTest。每个用例使用同一份 TOML
状态分别运行宿主原生指令和 `sbt` 发射的 `translated_block`。详细目录契约、
状态 ABI、比较规则和运行命令见[指令测试流程](testing.md)。

此外，`sbt-unit` 使用 GoogleTest 检查 lowering、寄存器别名、flags 公式和目标
文件发射。指令 fixture 是端到端验证，不能由手工构造 IR 的单元测试替代。

## 6. 已知技术债

以下问题是继续扩大指令覆盖前的高优先级事项：

- `File::CurrentFile`、`IR1Context::Instance()` 和静态 x86 machine 带来全局状态和生命周期限制；
- `getMachine` 对非 x86 triple 没有明确失败路径；
- `File::disas()` 线性扫描所有 text section，忽略符号、relocation 和代码/数据边界；
- decode fail 仅跳过一个字节；
- `convertMCInst` 对完全没有分派 case 的 opcode 仍缺少统一的 unsupported 诊断；
- IR1 的整数类型约束过宽，builder 又固定使用 i32/i64；
- IR1 操作没有完整 verifier、side effect interface 和 traits；
- module 当前只有单个 `translated_block`，尚没有从输入恢复的真实 CFG；
- translated block state ABI 仅覆盖 GPR 和 RFLAGS，尚未覆盖完整控制流和异常状态。

## 7. 修改约定

- 将“当前实现”和“目标设计”分开描述；
- 新增 IR 操作时同时添加 verifier、effects、lowering 和测试；
- 新增 opcode 时更新支持矩阵，禁止无声忽略；
- 不提交由构建产生的样例 object 或生成文件；
- 先建立一个端到端纵向切片，再批量扩展 opcode。
