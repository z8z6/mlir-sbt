# x86 ADD instruction fixtures

目录采用扁平结构：`add/<case>/`。每个 case 包含：

- `case.asm`：一条被测 ADD；另有一个仅用于返回 runner 的 `ret`。
- `input.toml`：输入寄存器和测试内存初值。
- `output.toml`：需要检查的输出寄存器，以及内存地址和值。
- `README.md`：该用例的操作数、语义重点和预期标志位。

寄存器/标志位的准备和采集均由 `tests/fixture_executor.asm` 和
`tests/fixture_runner.cpp` 完成，不属于被测汇编。每个 TOML 都完整列出
GDB `info registers` 风格的通用寄存器、指令指针、EFLAGS 和段寄存器，
并单列 CF、PF、AF、ZF、SF、TF、IF、DF、OF。ADD 定义的 6 个算术标志按
期待值检查，TF、IF、DF 则检查保持不变；只有每次运行会变化的 RIP、RSP
允许在输出中标为 `ignore`。

`rdi+24` 表示 runner 提供给汇编的测试内存槽地址（即 `rdi + 24`）。

每个 case 会注册两条 CTest：

- `isa.x86.add.<case>`：直接在宿主 x86-64 CPU 上执行 `case.asm`，作为 ISA
  语义基准。
- `isa.x86.add.<case>.translated`：先把同一份 `case.asm` 汇编为输入目标文件，
  再由 `sbt` 完成反汇编、IR1 转换、lowering 和目标文件发射，最后调用
  `translated_block`，使用同一份 TOML 检查架构状态。

两条路径共享输入和期待结果；翻译测试不是模拟期待值，而是实际链接并执行
`sbt` 生成的目标文件。

命名直接编码操作数形式、位宽和特殊语义，例如 `rr32_zero_extend`、
`ri64_imm8_signed`、`rm16_sib`、`lock_mr64`。
