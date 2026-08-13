# mi64_signed

被测指令：`mov qword [rdi+24], -1`。

将符号扩展的 imm32 -1 写入 64 位内存；MOV 不修改标志位。
