# IR 设计

## 1. 分层目的

项目需要区分“机器码中实际出现了什么”和“这条指令做了什么”：

- **IR0** 是保真解码层，接近 LLVM MC；
- **源架构 Dialect**（当前为 X86 Dialect）保存 flags、隐式状态等架构特有语义；
- **IR1** 是架构通用语义层，以基础操作描述二进制翻译所需的计算和程序状态；
- **标准 MLIR/LLVM Dialect** 是可代码生成层。

IR1 不应复制完整 LLVM IR，也不应包含以 x86 指令命名或绑定 x86 flags 规则的操作。无法方便、准确展开的 x86 语义保留在 X86 Dialect，并由独立 Pass 展开为 IR1 基础操作；随后 IR1 才继续降到标准 MLIR/LLVM Dialect。

## 2. IR0：保真机器指令层

### 2.1 当前结构

IR0 已按程序、函数、CFG、基本块和指令分层：

```text
IR0Context
  Functions[] : IR0Function
  EntryFunctionIndex

IR0Function
  Name / Aliases / Address / Size / SectionIndex
  IRs[] : IR0
  CFG : IR0CFG

IR0CFG
  Blocks[] : IR0BasicBlock

IR0BasicBlock
  Address / [BeginIndex, EndIndex) / Reachable
  Successors[] : {Branch|Fallthrough, TargetAddress, optional TargetBlock}

IR0
  Addr / Size / SectionIndex / AddressBias
  ExternalSymbol
  llvm::MCInst
```

基本块引用函数内连续的指令索引范围，不复制 `MCInst`。直接分支目标在函数内
可解析时记录 `TargetBlock`；函数外或尚未发现的目标保留地址并标为 unresolved。
可达性从函数入口块沿已解析的边计算，IR1 翻译只生成可达块。

### 2.2 后续补充

为支持更完整的重定位、诊断和代码发现，还应逐步补充：

```text
DecodedInst
  address           : u64
  size              : u8
  bytes             : byte[]
  mc_inst           : llvm::MCInst
  opcode_desc       : MCInstrDesc reference/key
  section_id        : SectionId
  symbol             : optional SymbolId
  relocations       : Relocation[]
  decode_status     : success | soft-fail | fail
```

IR0 的不变量：

- 同一 section 内指令范围不重叠；
- `size > 0`，且原始字节与地址映射一致；
- decode failure 不能被无声吞掉；
- 所有 relocation 均能追溯到原始对象文件记录。

## 3. X86 Dialect 与架构通用 IR1

### 3.1 当前 Dialect 分层

`mlir/X86.td` 当前定义 `x86.addi/subi/andi/ori/xori`、`x86.condition`、
`x86.syscall`、legacy scalar FP、`x86.convert` 和 x87 状态/算术操作，用来
封装 flags、Jcc 条件读取、Linux x86-64 syscall ABI、CVT lane 布局、XMM
高位保留与 x87 逻辑栈等 x86 专属语义。`mlir/IR1.td`
当前定义：

| 操作 | 意图 | 当前降级状态 |
|---|---|---|
| `ir1.ci`, `ir1.cf` | 整数/浮点常量 | 降到 `arith.constant` |
| `ir1.load_state`, `ir1.store_state` | 按规范化槽、偏移、宽度和掩码访问状态 | 降到 LLVM GEP/load/store |
| `ir1.load`, `ir1.store` | 访存 | 降到 LLVM load/store |
| `ir1.syscall` | 七参数宿主 syscall 边界 | 降到外部 `__sbt_syscall6` 调用 |
| `ir1.addi/subi/muli/divi` | 通用整数运算 | 降到 `arith` |
| `ir1.andi/ori/xori/shli/shrui/cmpi` | 通用位运算、移位与比较 | 降到 `arith` |
| `ir1.addf/subf/mulf/divf` | 通用浮点运算 | 降到 `arith` |
| `ir1.casti`, `ir1.bitcast` | 通用整数宽度转换与位转换 | 降到 `arith` |
| `ir1.sitofp`, `ir1.fptosi`, `ir1.extf`, `ir1.truncf` | 标量或向量的通用数值转换 | 降到 `arith` |
| `ir1.roundevenf` | 标量或向量的 nearest-even 舍入 | 经 `math` 降到 LLVM intrinsic |

现有 ADD 提升以 `NameLoc` 保存反汇编文本，这是有价值的调试信息；未来还应增加结构化的源 PC 和字节属性。

### 3.2 当前模型中的关键问题

1. `ConstIntOp::build` 的便捷重载固定产生 `i64`，调用方仍需按编码规则显式符号扩展或截断立即数。
2. X86 Dialect 的算术操作需要补充 verifier，限制输入与结果宽度并验证 flags 结果类型。
3. X86 Dialect 的寄存器操作仍以 LLVM 枚举编号作为暂存属性；第一阶段降级会立即规范化成 IR1 state slot，不允许该编号进入 IR1。
4. flags 被整体建模为 `i64`，但 CF/PF/AF/ZF/SF/OF 的定义与未定义状态尚未区分。
5. 内存操作缺少地址空间、volatile/alignment、endianness 和 fault 行为。
6. 当前按 ELF 函数边界恢复可达的直接 JMP/Jcc CFG，并逐函数降为
   `cf.br`/`cf.cond_br`；跨函数直接 CALL、跨 section relocation、间接跳转和
   多入口仍未建模。
7. 操作尚未完整声明 memory effects、traits 和 verifier，优化器无法安全分析全部副作用。
8. 当前 `ir1.syscall` 明确采用 Linux x86-64 ABI；runtime helper 直接返回内核
   RAX，X86 提升层另外回写 RCX（下一 RIP）和 R11（调用前 RFLAGS）。其他
   操作系统 ABI、信号重启及完整特权态入口状态尚未建模。
9. x87 当前把 ST0-ST7 规范化为八个逻辑 f80 值，支持常用二元算术和 pop；
   尚未建模 control/status/tag word、物理 TOP、动态精度/舍入和异常状态。

在这些问题解决前，不应把当前 IR1 文本视为稳定格式。

## 4. 目标状态模型

### 4.1 寄存器

建议用“规范物理状态 + 子寄存器视图”表示 x86 GPR：

- 规范存储槽为 64 位 `rax`、`rbx` 等；
- 读取 EAX/AX/AL/AH 转换为对 RAX 的 extract；
- 写 AX/AL/AH 转换为 read-modify-write；
- 写 EAX 明确将 RAX 高 32 位清零；
- 写 RAX 覆盖完整 64 位。

转换后的核心操作可类似：

```mlir
%rax = ir1.reg.read @rax : i64
%al  = arith.trunci %rax : i64 to i8
%sum = arith.addi %al, %rhs : i8
%new_rax = ir1.reg.insert %sum into %rax[0, 8] : i8, i64
ir1.reg.write @rax, %new_rax : i64
```

寄存器标识宜使用自定义 enum attribute（如 `#ir1.reg<rax>`），而不是裸整数。向 SSA 规范化后，局部的 `reg.read/write` 应尽量被 block arguments 和 SSA value 替换。

### 4.2 标志位

flags 是翻译 IR 相对普通 LLVM IR 最需要补充的语义之一。建议早期使用显式 flag bundle，随后按需拆分：

```text
x86.add_with_flags(lhs, rhs)
  → result, {cf, pf, af, zf, sf, of}
```

每个 flag 需要表示 `defined`、`preserved` 或 `undefined`。优化 Pass 根据后续使用只展开需要的标志位。例如 ADD 的核心公式为：

- `ZF = (result == 0)`；
- `SF = msb(result)`；
- `CF = unsigned_add_overflow(lhs, rhs)`；
- `OF = signed_add_overflow(lhs, rhs)`；
- `PF` 为低 8 位的偶校验；
- `AF` 为 bit 3 到 bit 4 的进位。

不能简单用宿主 LLVM `add` 推断所有 x86 flags；它们必须在 X86 Dialect 降级阶段显式展开为 IR1 通用计算，或交给经过验证的 intrinsic/runtime helper。

### 4.3 内存

有效地址应与访存分开：

```text
addr = segment_base + base + index * scale + displacement
value = load(addr, width, attributes)
```

建议的访存操作至少包含：

- value type/访问位宽；
- guest address type（x86-64 通常为 `i64`）；
- address space；
- alignment（未知时为 1）；
- volatile/atomic 属性；
- fault/ordering 语义。

段寄存器不能一律作为普通数值相加。64 位用户态早期可约束 CS/DS/ES/SS base 为 0，但 FS/GS base 必须明确建模。

### 4.4 控制流与 PC

直接控制流应尽快转成 MLIR block 和 `cf.br`/`cf.cond_br`，而不是将 PC 永久保存在普通寄存器槽。源地址以属性保留，用于异常和调试映射。

建议加入仅服务于翻译的操作：

- `ir1.indirect_br`：尚未解析的 guest 间接跳转；
- `ir1.call_external`：携带 guest ABI 的外部调用；
- `ir1.trap`：精确异常或不支持语义；
- `ir1.state.pack/unpack`：runtime 边界处序列化 guest state。

### 4.5 未定义值与异常

x86 某些指令会让部分 flags 未定义。IR 应使用显式 poison/undef 策略，并规定何时允许传播，不能随意选择固定值而掩盖错误。

可能 fault 的访存、除法等操作必须保持相对顺序。第一阶段如果不实现精确异常，应把“输入不得触发异常”写成执行契约，而不是默认已经具备精确语义。

## 5. 推荐的最小操作集合

优先直接使用标准 Dialect：

- `arith`：整数/浮点计算、比较、扩展与截断；
- `cf`：基本块控制流；
- `func`：翻译函数；
- LLVM Dialect：指针、调用、最终内存与 LLVM intrinsic。

IR1 自有操作控制在以下类别：

| 类别 | 示例 | 保留原因 |
|---|---|---|
| 架构状态 | `reg.read`, `reg.write` | LLVM IR 没有 guest 寄存器概念 |
| 架构特有语义 | `x86.addi`, `x86.scalar_f` | 只存在于源架构 Dialect，进入 IR1 前展开 |
| guest 内存 | `mem.load`, `mem.store` | 在内存模型确定前隔离 guest/host 地址 |
| 未解析控制流 | `indirect_br`, `call_external` | 需要 runtime 或后续解析 |
| 异常/退出 | `trap`, `exit` | 表达翻译器特有的执行边界 |
| 状态桥接 | `state.pack`, `state.unpack` | 与 runtime/外部代码交换 guest state |

常量、普通 `add/mul/div` 和无语义的 `mov` 通常不需要长期保留为 IR1 自有操作。

## 6. ADD 的目标提升示例

以 `add eax, ebx` 为例，第一阶段语义应为：

```mlir
%lhs = x86.read_reg ... {reg = eax} : i32
%rhs = x86.read_reg ... {reg = ebx} : i32
%sum, %flags = x86.addi %lhs, %rhs : i32
x86.write_reg ... , %sum {reg = eax}
x86.write_reg ... , %flags {reg = rflags}
```

X86 降级后，寄存器编号不再存在：EAX 写入变成覆盖 64 位 state slot 的
`ir1.store_state`，AX/AL/AH 写入变成带位偏移和写掩码的合并写；RFLAGS 也通过写掩码只更新指令定义的位。随后 IR1 降级只处理通用 state 访问。若后继仅使用 ZF，未来的 flags 优化还可以只生成 ZF 计算。

## 7. 降级顺序与验证

建议 Pass pipeline：

```text
verify-ir1
→ canonicalize-register-aliases
→ build-ssa-state
→ materialize-used-flags
→ convert-x86-to-ir1
→ lower-guest-memory
→ convert-ir1-to-standard
→ convert-to-llvm-dialect
→ reconcile-unrealized-casts
→ verify-no-ir1-ops
→ translate-to-llvm-ir
```

每个操作都应定义 verifier。最终转换使用 full conversion，并将 IR1 Dialect 标记为 illegal，以保证不会带着未降级操作进入 LLVM IR。
