# rr64-not-taken

被测指令：`cmovne rax, rbx`。

验证 CMOVNE 的 rr64-not-taken 编码；输入 ZF=1，检查条件成立/不成立时的选择结果、子寄存器写回、内存效果以及所有 EFLAGS 保持。

参考：Intel® 64 and IA-32 Architectures Software Developer's Manual，CMOVcc/SETcc。
