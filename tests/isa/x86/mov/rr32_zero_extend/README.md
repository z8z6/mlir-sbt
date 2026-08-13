# rr32_zero_extend

被测指令：`mov eax, ebx`。

写入 EAX，并验证 32 位写入将 RAX 高 32 位清零；MOV 不修改标志位。
