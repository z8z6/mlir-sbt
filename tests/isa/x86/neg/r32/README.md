# r32

被测指令：`neg eax`。

验证 32 位最小负数取负、OF/CF，并验证写 EAX 后高 32 位清零。

架构依据：Intel® 64 and IA-32 Architectures Software Developer's Manual,
Volume 2 instruction reference：
https://www.intel.com/content/www/us/en/developer/articles/technical/intel-sdm.html
