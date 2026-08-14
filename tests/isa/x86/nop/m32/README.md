# m32

被测指令：`nop dword [rdi + 24]`。

验证带 32 位内存寻址编码的多字节 NOP 不读取或修改该内存，也不修改寄存器
和 EFLAGS。地址操作数只参与编码长度选择，不产生架构访存。

参考：Intel® 64 and IA-32 Architectures Software Developer's Manual，NOP。
