# r64-base-disp

被测指令：`lea rax, [rdi+24]`，对应 LLVM MC `LEA64r`。

该用例覆盖 64 位基址加位移的有效地址计算。LEA 不读取该地址中的内存，
只把 `rdi + 24` 写入 RAX；RDI、内存及全部标志保持不变。32 位目的、
address-size override、复杂 SIB 和非规范地址异常不属于本用例。

参考：https://www.felixcloutier.com/x86/lea
