# rel8-taken

被测指令：`je short $+2`。

验证 `74 cb` 的 `rel8` 条件跳转。输入 `ZF=1`，因此 JE 条件成立并跳到
紧随其后的 runner `ret`。Jcc 只读取 flags，不修改寄存器、flags 或内存。

参考：[Intel SDM 指令集手册](https://www.intel.com/content/www/us/en/content-details/812389/intel-64-and-ia-32-architectures-software-developer-s-manual-combined-volumes-2a-2b-2c-and-2d-instruction-set-reference-a-z.html)、
[Jcc 索引页](https://www.felixcloutier.com/x86/jcc)。
