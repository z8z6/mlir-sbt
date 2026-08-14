# r32

被测指令：`not eax`。

验证 32 位寄存器逐位取反、写 EAX 后高 32 位清零且 RFLAGS 不变。

架构依据：Intel® 64 and IA-32 Architectures Software Developer's Manual,
Volume 2 instruction reference：
https://www.intel.com/content/www/us/en/developer/articles/technical/intel-sdm.html
