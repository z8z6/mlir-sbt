# 总体设计

## 1. 项目定位

mlir-sbt 面向静态二进制翻译（Static Binary Translation, SBT）。与普通编译器从结构化源程序开始不同，它需要从机器码恢复指令边界、控制流和架构状态，并处理寄存器别名、隐式操作数、标志位以及难以静态确定的间接跳转。

近期限定为：

- 源架构：x86-64；
- 目标架构：x86-64；
- 输入：优先支持 ELF relocatable object，随后支持受约束的 ELF executable；
- 输出：先生成等价 LLVM IR，再通过 LLVM x86 后端生成目标文件；
- 执行环境：先覆盖用户态、单线程、无自修改代码的整数程序子集。

x86 → x86 并非多余步骤：它可以隔离跨架构指令选择问题，先验证解码、状态建模、控制流恢复和代码生成链路。闭环稳定后，再增加其他目标架构。

## 2. 当前实现

### 2.1 实际调用路径

`src/main.cpp` 中的主流程为：

```text
命令行 -i <binary>
  → File::File
      → ObjectFile::createObjectFile
      → ObjectFile triple
      → getMachine(triple)
  → File::disas
      → 遍历 isText() section
      → MCDisassembler::getInstruction
      → IR0Context
  → IR0Context::print
  → IR1Context::convert
      → convertMCInst（生成代码中的 opcode switch）
      → BaseIR1Converter::run
      → loadSrcOperand / op / storeDstOperand
  → IR1Context::lower
      → X86LoweringPass
      → IR1LoweringPass
      → LLVM Dialect
  → IR1Context::emitObject
      → translateModuleToLLVMIR
      → LLVM TargetMachine
      → x86 relocatable object
```

### 2.2 模块职责

| 模块 | 路径 | 当前职责 |
|---|---|---|
| 命令行与错误处理 | `include/Support`, `src/Support` | 解析 `-i`、`-o` 和 `--quiet`，解包 LLVM `Expected` |
| 对象文件 | `include/Object`, `src/Object` | 打开目标文件、遍历代码 section |
| 目标机抽象 | `include/Target`, `src/Target` | 初始化 LLVM x86 MC 组件，提供反汇编器和指令描述 |
| IR0 | `include/IR/IR0.h`, `src/IR/IR0.cpp` | 保存地址与 `MCInst`，打印汇编文本 |
| X86 Dialect | `mlir/X86.td`, `include/IR/X86.h` | 封装 flags、legacy XMM 等 x86 特有语义 |
| IR1 Dialect | `mlir/IR1.td`, `include/IR/IR1.h` | 定义架构通用基础计算与状态操作 |
| 指令提升框架 | `src/IR/IR1Converter.cpp` | 统一读取显式操作数并写回结果 |
| x86 指令翻译 | `src/Trans/X86/` | 按指令名组织翻译函数，并复用寻址、操作数与架构状态辅助逻辑 |
| 转换器生成 | `tblgen/` | 从 LLVM x86 TableGen 记录生成转换器类及 opcode switch |
| X86 降级 Pass | `src/Pass/X86Lowering.cpp` | 将 x86 flags 和 scalar FP 语义展开成 IR1 基础操作 |
| IR1 降级 Pass | `src/Pass/IR1Lowering.cpp` | 将通用状态、计算和内存操作降到标准/LLVM Dialect |
| 目标发射 | `src/IR/IR1.cpp` | 翻译到 LLVM IR，并通过宿主 TargetMachine 发射目标文件 |

### 2.3 已验证的范围

仓库中的 `tests/isa/x86/add` 覆盖 ADD 的寄存器、立即数、内存读取和内存写回
形式。每个 fixture 都有宿主原生基准和 `sbt` 静态翻译测试，两条路径使用相同
输入与期待状态；具体流程见[指令测试流程](testing.md)。这验证了 LLVM MC、
生成式分派、IR1、完整 lowering、目标文件发射和 translated block 执行的纵向
链路。

这条路径仍有以下边界：

- 生成器仍会为所有名称以 `ADD` 开头的 x86 指令建立分派，但只有显式标记的
  converter 已实现语义；已分派但未实现的 converter 会令翻译失败；
- 完全没有进入生成 switch 的 opcode 尚缺少统一的 unsupported 诊断；
- 反汇编失败时跳过一个字节，没有记录 gap 或终止当前基本块；
- 所有代码 section 被线性扫描，没有函数、符号、重定位和 CFG；
- 当前 state ABI 覆盖 GPR、RFLAGS 与 XMM0-XMM15；RIP、段状态、异常和程序级
  控制流尚未进入 translated block ABI。每个 XMM 寄存器作为两个连续的 64 位槽建模，
  但 MXCSR、YMM/ZMM 和浮点异常状态尚未进入 ABI；
- `lock add` 的现有 fixture 只验证单线程结果，尚未验证翻译产物的并发原子性。

## 3. 目标架构

建议将翻译器拆为六个显式阶段。各阶段输入输出都应能独立序列化或打印，以便定位语义错误。

### 3.1 装载与映像（Loader/Image）

职责：

- 读取 ELF header、section、symbol 和 relocation；
- 建立虚拟地址到文件内容/权限的映射；
- 确定入口点和待翻译代码范围；
- 保存 ABI、endianness、架构 feature 等模块级信息。

不要把 `File::CurrentFile` 作为长期接口。目标设计应使用显式的 `BinaryImage`/`TranslationSession` 所有权，使多文件、测试隔离和并行翻译成为可能。

### 3.2 解码与 IR0

职责：

- 使用 LLVM MC 解码单条机器指令；
- 保存地址、字节长度、原始字节、opcode、显式/隐式操作数；
- 关联 section、symbol、relocation 和源映射；
- 对无法解码的区域产生显式错误或 data/gap 记录。

IR0 是机器指令层的事实记录，不承担优化，也不应该丢失重编码或诊断所需的信息。

### 3.3 控制流恢复

职责：

- 从入口、符号和直接分支目标递归发现代码；
- 划分函数和基本块；
- 建立 direct branch/call/fallthrough 边；
- 将间接跳转标记为待解析对象；
- 区分代码与内嵌数据，避免盲目线性扫描。

第一阶段可以只接受无间接跳转的输入，但必须遇到不支持情况时失败，而不是继续产生缺失代码的结果。

### 3.4 语义提升到 X86 Dialect 与 IR1

每条源指令应被转换为：

```text
读架构状态 → 基础计算/访存 → 写架构状态 → 控制流或异常效果
```

转换器负责 x86 编码细节。具有 flags、隐式寄存器或 legacy XMM 行为的操作先进入 X86 Dialect；无架构特征的值搬运和状态访问可直接进入 IR1。X86 降级 Pass 负责把特殊语义展开为 IR1 通用操作。转换过程应由“指令 schema + 少量手写语义模板”驱动：TableGen 可生成 opcode 分类、操作数角色和分派，复杂语义仍由可测试的 C++ lowering 实现。

### 3.5 规范化与优化

在 IR1 层执行：

- 寄存器读写转 SSA（基本块入口/出口使用显式状态或 block argument）；
- 标志位按需物化；
- 常量传播、死代码删除和地址计算折叠；
- x86 特化操作逐步展开为基础操作；
- 验证每个基本块的状态输入输出完整。

优化不能跨越可能的内存、异常或外部调用效果，除非别名和副作用分析能够证明安全。

### 3.6 降级与代码生成

推荐路径：

```text
X86 Dialect（flags、隐式状态、legacy XMM 等源架构语义）
  → IR1（架构通用基础操作）
  → arith / cf / func / LLVM Dialect
  → mlir-translate 或 translateModuleToLLVMIR
  → llvm::Module
  → LLVM TargetMachine
  → x86 object
  → 系统链接器
```

LLVM IR 只承担已经显式化后的普通计算、控制流和运行时调用。源架构寄存器、精确 flags、间接分支解析等翻译辅助语义应在进入 LLVM IR 前处理完毕，或明确降级成 runtime ABI。

## 4. 运行时边界

即使源/目标都是 x86，也建议预留一个小型 runtime。它负责纯静态代码难以表达的部分：

- 间接分支目标查找；
- 系统调用和外部符号桥接；
- 异常/信号与源 PC 映射；
- 需要时保存完整 guest state；
- 未支持指令的受控 fallback（可选）。

第一阶段可以完全不实现 runtime，但输入约束必须排除依赖上述能力的程序。

## 5. 正确性与可观测性

每个翻译单元至少保留源地址属性；诊断应同时给出源地址、原始字节、反汇编文本和 opcode。建议提供以下调试出口：

- `--dump-ir0`：机器指令与元数据；
- `--dump-cfg`：函数、基本块和边；
- `--dump-ir1`：提升后的语义；
- `--dump-llvm`：最终 LLVM IR；
- `--stop-after=<stage>`：在指定阶段停止；
- `--verify-each`：每个 Pass 后执行 verifier。

核心验证方式是差分执行：在相同初始寄存器/内存状态下运行原始片段和翻译后片段，比较定义的寄存器、flags、内存写入和退出原因。
