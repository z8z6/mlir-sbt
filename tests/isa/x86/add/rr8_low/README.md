# rr8_low

被测指令：`add al, bl`。

验证窄位宽写回只修改目标字段，并保留同一寄存器或内存槽的其余位。

预期 ADD 标志位：CF、PF、AF、ZF。

输入状态见 `input.toml`；需要验证的输出寄存器和 `rdi+24` 内存槽见
`output.toml`。汇编文件除返回 runner 所需的 `ret` 外只包含上述 ADD。

