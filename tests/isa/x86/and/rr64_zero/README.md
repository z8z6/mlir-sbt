# rr64_zero

被测指令：`and rax, rbx`。

64 位寄存器 AND 得到零；CF/OF 清零，ZF/PF 置位，SF 清零；AF 未定义，本实现规范化为 0。
