# rel8-next

被测指令：`jmp short $+2`。

验证 `EB cb` 的 `rel8` 直接近跳转编码。目标是紧随其后的 runner `ret`，
因此跳转必须完成且不得改变任何通用寄存器、flags 或测试内存。

参考：[Intel SDM 指令集手册](https://www.intel.com/content/www/us/en/content-details/812389/intel-64-and-ia-32-architectures-software-developer-s-manual-combined-volumes-2a-2b-2c-and-2d-instruction-set-reference-a-z.html)、
[JMP 索引页](https://www.felixcloutier.com/x86/jmp)。
