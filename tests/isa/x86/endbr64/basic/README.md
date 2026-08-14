# basic

被测指令：`endbr64`。

验证 ENDBR64 不修改通用寄存器、EFLAGS 或内存。翻译器当前不模拟 CET
tracker 状态；在 translated block 的软件控制流模型中将其作为合法间接入口
标记并保留零数据状态效果。

参考：Intel® 64 and IA-32 Architectures Software Developer's Manual，ENDBR64。
