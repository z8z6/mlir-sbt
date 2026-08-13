# rr64_parity_clear

被测指令：`add rax, rbx`。

验证结果低字节为奇校验时 PF 清零。

预期 ADD 标志位：CF、PF、AF、ZF、SF、OF 均清零。

输入状态见 `input.toml`；需要验证的输出寄存器和 `rdi+24` 内存槽见
`output.toml`。汇编文件除返回 runner 所需的 `ret` 外只包含上述 ADD。

