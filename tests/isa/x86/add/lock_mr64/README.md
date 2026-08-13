# lock_mr64

被测指令：`lock add qword [rdi + 24], rax`。

验证带 LOCK 前缀的内存读改写 ADD 及其标志位结果。

预期 ADD 标志位：PF。

输入状态见 `input.toml`；需要验证的输出寄存器和 `rdi+24` 内存槽见
`output.toml`。汇编文件除返回 runner 所需的 `ret` 外只包含上述 ADD。

