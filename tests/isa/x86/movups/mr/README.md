# mr

被测指令：`movups [rdi+32], xmm1`。

验证向非 16 字节对齐内存存储完整 128 位 XMM 数据。

架构依据：Intel® 64 and IA-32 Architectures Software Developer's Manual,
Volume 2 instruction reference：
https://www.intel.com/content/www/us/en/developer/articles/technical/intel-sdm.html
