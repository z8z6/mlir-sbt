# m32int

被测指令：`fisub dword [rdi + 24]`。

验证 FISUB 的 m32int 形式、80 位 x87 结果、逻辑栈顺序、内存保持及 EFLAGS 不变。
