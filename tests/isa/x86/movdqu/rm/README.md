# rm

被测指令：`movdqu xmm0, [rdi+32]`。

验证从非 16 字节对齐内存加载完整 128 位整数数据。

架构依据：Intel® 64 and IA-32 Architectures Software Developer's Manual,
Volume 2 instruction reference：
https://www.intel.com/content/www/us/en/developer/articles/technical/intel-sdm.html
