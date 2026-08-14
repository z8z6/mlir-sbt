# rr

被测指令：`movdqa xmm0, xmm1`。

验证对齐整数数据的 XMM 寄存器完整 128 位复制，RFLAGS 不变。

架构依据：Intel® 64 and IA-32 Architectures Software Developer's Manual,
Volume 2 instruction reference：
https://www.intel.com/content/www/us/en/developer/articles/technical/intel-sdm.html
