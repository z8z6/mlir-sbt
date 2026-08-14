# r64-rm

被测指令：`cvtsi2sd xmm0, qword [rdi+24]`，覆盖 内存源 编码。

输入包含正负数及非整数值；该 case 验证 精确或目标格式舍入、32 位 GPR 写零扩展、legacy XMM 高位保留或 packed 窄结果高位清零（按该指令适用项），并验证所有 EFLAGS 位和源操作数保持不变。

架构参考：[Intel® 64 and IA-32 Architectures Software Developer’s Manual, Volume 2](https://www.intel.com/content/www/us/en/developer/articles/technical/intel-sdm.html)。
