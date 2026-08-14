# rel32-not-taken

被测指令：`jne near run_case`。

验证 `0F 85 cd` 的 `rel32` 条件跳转和负位移回边。目标是该 JNE 自身；
输入 `ZF=1`，所以条件不成立并沿 fallthrough 到 runner `ret`。若条件或
回边恢复错误，translated 路径将无法正常返回。Jcc 保留全部架构状态。

参考：[Intel SDM 指令集手册](https://www.intel.com/content/www/us/en/content-details/812389/intel-64-and-ia-32-architectures-software-developer-s-manual-combined-volumes-2a-2b-2c-and-2d-instruction-set-reference-a-z.html)、
[Jcc 索引页](https://www.felixcloutier.com/x86/jcc)。
