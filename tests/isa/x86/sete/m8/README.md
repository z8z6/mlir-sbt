# m8

被测指令：`sete byte [rdi + 24]`。

验证 SETE 的 m8 编码；输入 ZF=1，检查条件成立/不成立时的选择结果、子寄存器写回、内存效果以及所有 EFLAGS 保持。

参考：Intel® 64 and IA-32 Architectures Software Developer's Manual，CMOVcc/SETcc。
