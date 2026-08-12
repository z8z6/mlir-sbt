# IR 设计

## 1. 分层目的

项目需要区分“机器码中实际出现了什么”和“这条指令做了什么”：

- **IR0** 是保真解码层，接近 LLVM MC；
- **IR1** 是架构语义层，显式描述二进制翻译所需的程序状态；
- **标准 MLIR/LLVM Dialect** 是可代码生成层。

IR1 不应复制完整 LLVM IR，也不应为每个 x86 opcode 永久保留一个操作。理想状态是少量架构状态操作加上 MLIR 的 `arith`、`cf`、`func` 和 LLVM Dialect 操作；只有无法方便、准确展开的语义才暂时使用 x86 特化操作。

## 2. IR0：保真机器指令层

### 2.1 当前结构

当前 `IR0` 只有两个字段：

```cpp
uint64_t Addr;
llvm::MCInst Inst;
```

`IR0Context` 使用 `std::vector<IR0>` 保存反汇编结果。这足够验证 opcode 分派，却不足以支撑 CFG、重定位或精确诊断。

### 2.2 建议结构

建议逐步补充：

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

## 3. IR1：架构语义层

### 3.1 当前 Dialect

`mlir/IR1.td` 当前定义：

| 操作 | 意图 | 当前降级状态 |
|---|---|---|
| `ir1.ci`, `ir1.cf` | 整数/浮点常量 | 仅 `ci` 降为 `arith.constant` |
| `ir1.load_reg`, `ir1.store_reg` | 读写架构寄存器 | 未降级 |
| `ir1.load`, `ir1.store` | 访存 | 未降级 |
| `ir1.addi/addf` | 加法 | 未降级 |
| `ir1.muli/mulf` | 乘法 | 未降级 |
| `ir1.divi/divf` | 除法 | 未降级 |
| `ir1.mov` | 值复制 | 未降级 |
| `ir1.x86_addi` | x86 加法结果与 flags | 未降级 |

现有 ADD 提升以 `NameLoc` 保存反汇编文本，这是有价值的调试信息；未来还应增加结构化的源 PC 和字节属性。

### 3.2 当前模型中的关键问题

1. `LoadRegOp::build` 固定返回 `i32`，无法正确表示 AL/AX/RAX 等宽度。
2. `ConstIntOp::build` 固定产生 `i64`，立即数在不同指令中需要按编码规则符号扩展或截断。
3. `x86_addi` 允许任意整数宽度的两个输入，当前样例会出现 `i32 + i64 → i8/i16/i32/i64`，类型合法但语义没有定义。
4. 寄存器用 LLVM 枚举编号作为 `AnyAttr`，文本不可读，且把 LLVM 内部编号变成了潜在持久格式。
5. RAX/EAX/AX/AL/AH 的重叠、32 位写入清零高 32 位等别名语义尚未表达。
6. flags 被整体建模为 `i64`，但 CF/PF/AF/ZF/SF/OF 的定义与未定义状态没有区分。
7. 内存操作缺少宽度、地址空间、volatile/alignment、endianness 和 fault 行为。
8. 当前 module 中没有函数、基本块或 terminator，无法表达真实控制流。
9. 操作尚未声明 memory effects、traits 和 verifier，优化器无法安全分析副作用。

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

不能简单用宿主 LLVM `add` 推断所有 x86 flags；它们必须在 IR1 中显式计算或交给经过验证的 intrinsic/runtime helper。

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
| 精确 flags | `x86.add_flags`, `flag.get` | 保留 x86 隐式状态和按需计算机会 |
| guest 内存 | `mem.load`, `mem.store` | 在内存模型确定前隔离 guest/host 地址 |
| 未解析控制流 | `indirect_br`, `call_external` | 需要 runtime 或后续解析 |
| 异常/退出 | `trap`, `exit` | 表达翻译器特有的执行边界 |
| 状态桥接 | `state.pack`, `state.unpack` | 与 runtime/外部代码交换 guest state |

常量、普通 `add/mul/div` 和无语义的 `mov` 通常不需要长期保留为 IR1 自有操作。

## 6. ADD 的目标提升示例

以 `add eax, ebx` 为例，语义应为：

```mlir
%rax = ir1.reg.read #ir1.reg<rax> : i64
%rbx = ir1.reg.read #ir1.reg<rbx> : i64
%lhs = arith.trunci %rax : i64 to i32
%rhs = arith.trunci %rbx : i64 to i32
%sum, %flags = ir1.x86.add_with_flags %lhs, %rhs : i32
%sum64 = arith.extui %sum : i32 to i64
ir1.reg.write #ir1.reg<rax>, %sum64 : i64
ir1.flags.write %flags
```

关键点是 EAX 写入产生零扩展，而不是保留 RAX 高位。若后继仅使用 ZF，flags lowering 只需生成 ZF 计算。

## 7. 降级顺序与验证

建议 Pass pipeline：

```text
verify-ir1
→ canonicalize-register-aliases
→ build-ssa-state
→ materialize-used-flags
→ lower-x86-semantic-ops
→ lower-guest-memory
→ convert-ir1-to-standard
→ convert-to-llvm-dialect
→ reconcile-unrealized-casts
→ verify-no-ir1-ops
→ translate-to-llvm-ir
```

每个操作都应定义 verifier。最终转换使用 full conversion，并将 IR1 Dialect 标记为 illegal，以保证不会带着未降级操作进入 LLVM IR。

