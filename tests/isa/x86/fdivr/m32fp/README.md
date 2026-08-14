# m32fp

被测指令：`fdivr dword [rdi + 24]`。

验证 FDIVR 的 m32fp 形式、80 位 x87 结果、逻辑栈顺序、内存保持及 EFLAGS 不变。
