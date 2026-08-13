# ri64_imm32

被测指令：`add rax, strict dword 0x7fffffff`。

验证全宽/imm32 立即数编码及相应算术边界。

预期 ADD 标志位：PF、AF。

输入状态见 `input.toml`；需要验证的输出寄存器和 `rdi+24` 内存槽见
`output.toml`。汇编文件除返回 runner 所需的 `ret` 外只包含上述 ADD。

