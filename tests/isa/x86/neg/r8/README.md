# r8

被测指令：`neg al`。

验证 8 位寄存器原地取负，覆盖最小负数、OF/CF 以及高 56 位保留。

架构依据：Intel® 64 and IA-32 Architectures Software Developer's Manual,
Volume 2 instruction reference：
https://www.intel.com/content/www/us/en/developer/articles/technical/intel-sdm.html
