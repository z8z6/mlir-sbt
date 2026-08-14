# m8

被测指令：`neg byte [rdi+24]`。

验证 8 位内存原地取负、相邻字节保留及完整算术标志。

架构依据：Intel® 64 and IA-32 Architectures Software Developer's Manual,
Volume 2 instruction reference：
https://www.intel.com/content/www/us/en/developer/articles/technical/intel-sdm.html
