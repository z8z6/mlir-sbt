# m16

被测指令：`nop word [rdi + 24]`。

验证带操作数大小前缀的多字节 NOP 不读取或修改内存、寄存器和 EFLAGS。

参考：Intel® 64 and IA-32 Architectures Software Developer's Manual，NOP。
