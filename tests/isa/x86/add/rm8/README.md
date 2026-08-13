# rm8

被测指令：`add al, byte [rdi + 24]`。

验证内存源操作数，内存本身保持不变。

预期 ADD 标志位：AF。

输入状态见 `input.toml`；需要验证的输出寄存器和 `rdi+24` 内存槽见
`output.toml`。汇编文件除返回 runner 所需的 `ret` 外只包含上述 ADD。

