# mr64

被测指令：`add qword [rdi + 24], rax`。

验证内存目标的读改写语义。

预期 ADD 标志位：CF、PF、AF、ZF。

输入状态见 `input.toml`；需要验证的输出寄存器和 `rdi+24` 内存槽见
`output.toml`。汇编文件除返回 runner 所需的 `ret` 外只包含上述 ADD。

