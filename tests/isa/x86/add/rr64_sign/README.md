# rr64_sign

被测指令：`add rax, rbx`。

验证负结果置 SF 且不产生 OF。

预期 ADD 标志位：PF、SF。

输入状态见 `input.toml`；需要验证的输出寄存器和 `rdi+24` 内存槽见
`output.toml`。汇编文件除返回 runner 所需的 `ret` 外只包含上述 ADD。

