# m16int

被测指令：`fidiv word [rdi + 24]`。

验证 FIDIV 的 m16int 形式、80 位 x87 结果、逻辑栈顺序、内存保持及 EFLAGS 不变。
