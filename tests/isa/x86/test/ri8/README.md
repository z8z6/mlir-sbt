# ri8

被测指令：`test cl, 3`。

验证 TEST 的 ri8 编码。输入覆盖寄存器或内存操作数；结果不写回，完整检查 CF/PF/AF/ZF/SF/OF（TEST 的 AF 架构未定义）以及内存保持。

参考：Intel® 64 and IA-32 Architectures Software Developer's Manual，TEST。
