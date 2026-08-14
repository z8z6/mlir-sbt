# rm32

被测指令：`cmp eax, dword [rdi + 24]`。

验证 CMP 的 rm32 编码。输入覆盖寄存器或内存操作数；结果不写回，完整检查 CF/PF/AF/ZF/SF/OF（TEST 的 AF 架构未定义）以及内存保持。

参考：Intel® 64 and IA-32 Architectures Software Developer's Manual，CMP。
