# 指令测试流程

## 1. 测试目标

x86 指令 fixture 同时验证两件事：

1. 测试用例描述的 ISA 语义与宿主 x86-64 CPU 的实际结果一致；
2. `sbt` 生成的静态翻译代码与相同的 ISA 期待结果一致。

因此，每个用例都运行“原生基准”和“静态翻译”两条路径。两条路径共享
`input.toml` 和 `output.toml`，不会为翻译代码维护一套较宽松的期待结果。

```text
                         ┌─ 原生链接 run_case ────────┐
case.asm ── NASM/ELF ────┤                            ├─ 完整状态比较
                         └─ sbt → translated_block ───┘
                                      ▲
input.toml ── 初始化寄存器、flags、内存 ┘
output.toml ───────────────────────────── 期待状态
```

## 2. 用例布局

用例位于 `tests/isa/x86/<instruction>/<case>/`。当前 ADD 使用扁平目录，例如：

```text
tests/isa/x86/add/rr64_basic/
  case.asm
  input.toml
  output.toml
  README.md
```

- `case.asm` 只包含一条被测指令，以及返回 runner 所需的 `ret`。寄存器准备、
  保存结果等操作不能放入这里，否则会污染被测指令的状态。
- `input.toml` 描述执行前的寄存器、标志位和内存。
- `output.toml` 描述执行后的寄存器、标志位和需要检查的内存。
- `README.md` 解释操作数形式、位宽、边界情形、内存效果及 flags 期待值。

## 3. 架构状态约定

### 3.1 寄存器

两个 TOML 都必须列出常见 `gdb info registers` 视图中的完整集合：

```text
rax rbx rcx rdx rsi rdi rbp rsp
r8 r9 r10 r11 r12 r13 r14 r15
rip eflags cs ss ds es fs gs
```

输出中的普通寄存器不能因“预计不受影响”而省略，应使用 `"unchanged"`。
当前只有每次调用或地址随机化都会改变的 `rip`、`rsp` 可以使用 `"ignore"`。
输入中的 `"runtime"` 表示该值由 fixture 执行环境采集；`"run_case"` 表示
原始指令入口。

### 3.2 标志位

`[flags]` 必须包含：

```text
cf pf af zf sf tf if df of
```

`eflags` 同时保留在 `[registers]`，以对应 GDB 的整体寄存器显示。runner 会先
根据 `[flags]` 合成输入 EFLAGS，再分别检查每个标志，使失败信息能指出具体
flag。对于 ADD，CF、PF、AF、ZF、SF、OF 检查计算结果，TF、IF、DF 检查
保持不变。

### 3.3 内存

当前 fixture 提供一个以 `rdi+24` 表示的 64 位测试槽：

```toml
[memory]
"rdi+24" = 0x1122334455667788
```

窄写入仍比较完整 64 位槽，因此可以检查目标字节或字之外的内容是否保持。
`rdi = "memory_base"` 由 runner 替换为能使 `rdi+24` 指向该槽的实际地址。

## 4. 原生基准路径

CMake 为每个 case 建立一个原生测试程序，由以下文件组成：

```text
fixture_runner.cpp + fixture_executor.asm + case.asm
```

运行过程如下：

1. `fixture_runner.cpp` 读取并验证两个 TOML，初始化测试内存；
2. `fixture_executor.asm` 保存宿主 ABI 要求保持的寄存器；
3. executor 装载输入 GPR 和 EFLAGS，并采集运行时拥有的 RSP、RIP、段寄存器；
4. executor 调用 `run_case`，在宿主 CPU 上执行被测指令；
5. executor 保存全部 GPR、EFLAGS 和段寄存器，然后恢复宿主状态；
6. runner 按 `output.toml` 比较寄存器、各 flag 和内存。

对应的 CTest 名称为：

```text
isa.x86.add.<case>
```

原生路径的作用是提供真实 CPU 的 ISA 基准，同时验证 TOML 期待值和 fixture
本身是否正确。

## 5. `sbt` 静态翻译路径

静态翻译测试不是直接构造 IR，也不是调用原始 `run_case`。构建每个 case 时，
CMake 实际执行以下流程：

```text
case.asm
  → nasm -f elf64
  → <case>.input.o
  → sbt --quiet -i <case>.input.o -o <case>.translated.o
  → 链接 fixture_runner.cpp + <case>.translated.o
  → 执行 translated_block(register_state)
```

`sbt` 内部覆盖完整的当前翻译链路：

```text
ELF Object
  → LLVM MC 反汇编
  → IR0
  → x86 opcode converter
  → IR1
  → IR1 lowering
  → LLVM Dialect / LLVM IR
  → LLVM TargetMachine
  → x86 relocatable object
```

翻译目标文件导出：

```cpp
extern "C" void translated_block(uint64_t *state);
```

当前 guest state ABI 是 16 个 64 位 GPR 槽，随后是一个 RFLAGS 槽。runner 将
TOML 输入复制到该状态数组，调用 `translated_block`，再把结果映射回统一的
fixture 状态进行比较。ADD 不修改段寄存器；这些字段在翻译路径中按输入值
保持。RIP、RSP 当前不属于 translated block 的可观察控制流输出，仍按 schema
忽略。

对应的 CTest 名称为：

```text
isa.x86.add.<case>.translated
```

未实现的 converter 必须让 `sbt` 返回失败，不能静默生成空的
`translated_block`。因此，构建翻译目标本身也是测试的一部分：反汇编、提升、
lowering 或目标发射任一阶段失败，测试都不能建立成功。

## 6. 结果判定

runner 对输出值支持三种写法：

- 数值：必须精确相等；
- `"unchanged"`：必须等于该次执行的输入值；
- `"ignore"`：不比较，只允许用于 schema 明确声明为运行时不稳定的字段。

一次完整回归包括：

- 1 个 `sbt-unit`，检查 lowering、寄存器别名、flags 公式和目标发射；
- 每个 ISA case 的 1 个原生测试；
- 每个 ISA case 的 1 个静态翻译测试。

当前 30 个 ADD case 因而共有 61 条 CTest。

## 7. 运行测试

构建并运行全部测试：

```sh
cmake --build build -j 8
ctest --test-dir build --output-on-failure -j 8
```

只运行静态翻译路径：

```sh
ctest --test-dir build --output-on-failure -R '\.translated$'
```

只运行某个 case 的原生和翻译测试：

```sh
ctest --test-dir build --output-on-failure -R 'isa\.x86\.add\.rr64_basic'
```

提交前检查 fixture 结构：

```sh
python3 .agents/skills/x86-instruction-test/scripts/validate_cases.py \
  tests/isa/x86/add
```

添加用例时可以使用仓库内的 `x86-instruction-test` skill 及其脚手架；它会生成
完整寄存器/flags 模板，并拒绝 ADD 目录中出现其他 opcode。

## 8. 当前边界

- 原生基准要求测试主机为 x86-64，并且支持被测编码。
- 当前内存模型只暴露 `rdi+24` 测试槽，尚不是通用 guest address space。
- translated block ABI 尚未建模 RIP、段基址、异常和控制流退出原因。
- `lock add` 当前只验证单线程结果、flags 和内存值，不能证明并发原子性；原子性
  需要多线程竞争测试以及 IR/LLVM 原子操作语义。
- 这些是指令级 fixture，不替代多指令基本块、CFG、relocation、调用和系统接口
  的程序级测试。
