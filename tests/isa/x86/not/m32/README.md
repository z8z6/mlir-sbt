# m32

被测指令：`not dword [rdi+24]`。

验证 32 位内存逐位取反、相邻字节保留且 RFLAGS 不变。

架构依据：Intel® 64 and IA-32 Architectures Software Developer's Manual,
Volume 2 instruction reference：
https://www.intel.com/content/www/us/en/developer/articles/technical/intel-sdm.html
