# rel32-next

被测指令：`jmp near $+5`。

验证 64 位模式下 `E9 cd` 的 `rel32` 直接近跳转编码。目标是紧随其后的
runner `ret`；位移按带符号 32 位值加到下一条指令地址，架构状态保持不变。

参考：[Intel SDM 指令集手册](https://www.intel.com/content/www/us/en/content-details/812389/intel-64-and-ia-32-architectures-software-developer-s-manual-combined-volumes-2a-2b-2c-and-2d-instruction-set-reference-a-z.html)、
[JMP 索引页](https://www.felixcloutier.com/x86/jmp)。
