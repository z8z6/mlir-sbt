# rr64_carry

被测指令：`add rax, rbx`。

验证无符号进位但结果非零的边界。

预期 ADD 标志位：CF、AF。

输入状态见 `input.toml`；需要验证的输出寄存器和 `rdi+24` 内存槽见
`output.toml`。汇编文件除返回 runner 所需的 `ret` 外只包含上述 ADD。

