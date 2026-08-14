# mi32_imm8

被测指令：`add dword [rdi + 24], byte 1`。

验证 ADD 的 mi32_imm8 编码、立即数扩展、窄内存写回及 CF/PF/AF/ZF/SF/OF。

参考：Intel® 64 and IA-32 Architectures Software Developer's Manual，ADD。
