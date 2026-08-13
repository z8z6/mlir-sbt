# rr32_zero_extend

被测指令：`add eax, ebx`。

验证 32 位通用寄存器写回会将对应 64 位寄存器的高 32 位清零。

预期 ADD 标志位：CF、PF、AF、ZF。

输入状态见 `input.toml`；需要验证的输出寄存器和 `rdi+24` 内存槽见
`output.toml`。汇编文件除返回 runner 所需的 `ret` 外只包含上述 ADD。

