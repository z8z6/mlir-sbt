# mi64_imm32

被测指令：`add qword [rdi + 24], 0x1234`。

验证 ADD 的 mi64_imm32 编码、立即数扩展、窄内存写回及 CF/PF/AF/ZF/SF/OF。

参考：Intel® 64 and IA-32 Architectures Software Developer's Manual，ADD。
