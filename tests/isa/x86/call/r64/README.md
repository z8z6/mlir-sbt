# r64

被测指令：`call rax`。

RAX 初始值由 runner 解析为 `fixture_call_target` 的宿主地址。目标函数返回
`RDI + 24`，用于验证寄存器间接目标、六个 SysV 整数参数和 RAX 返回值；CALL
不修改算术标志，返回后 RSP 恢复。当前翻译把 r/m64 目标约束为可直接调用的
宿主函数指针，尚未进行 guest 地址到已翻译函数的查找。

体系结构语义依据 [Intel 64 and IA-32 Architectures Software Developer's
Manual](https://www.intel.com/content/www/us/en/developer/articles/technical/intel-sdm.html)
中 CALL 指令的 near indirect r/m64 形式。
