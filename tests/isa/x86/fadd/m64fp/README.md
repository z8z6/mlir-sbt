# m64fp

被测指令：`fadd qword [rdi + 24]`。

验证 FADD 的 m64fp 形式、80 位 x87 结果、逻辑栈顺序、内存保持及 EFLAGS 不变。
