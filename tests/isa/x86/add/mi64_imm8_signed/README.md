# mi64_imm8_signed

被测指令：`add qword [rdi + 24], byte -1`。

验证 imm8 在运算位宽内进行符号扩展。

预期 ADD 标志位：PF、SF。

输入状态见 `input.toml`；需要验证的输出寄存器和 `rdi+24` 内存槽见
`output.toml`。汇编文件除返回 runner 所需的 `ret` 外只包含上述 ADD。

