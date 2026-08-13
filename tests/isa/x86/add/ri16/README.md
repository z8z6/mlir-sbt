# ri16

被测指令：`add ax, strict word 0x7fff`。

验证基础 ADD 结果及全部已定义算术标志位。

预期 ADD 标志位：PF、AF、SF、OF。

输入状态见 `input.toml`；需要验证的输出寄存器和 `rdi+24` 内存槽见
`output.toml`。汇编文件除返回 runner 所需的 `ret` 外只包含上述 ADD。

