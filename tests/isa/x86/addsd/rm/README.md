# rm

被测指令：`addsd xmm0, [rdi+24]`。

验证 legacy ADDSD 的 内存源形式。仅更新 XMM0 的低 64 位，保留高 64 位；RFLAGS 不变。测试使用有限普通数和默认 MXCSR 舍入模式。
