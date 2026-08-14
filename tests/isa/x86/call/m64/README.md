# m64

被测指令：`call [rdi+24]`。

`[RDI+24]` 保存 runner 提供的 `fixture_call_target` 宿主地址。目标函数返回
`RDI + 24`，验证内存间接目标读取、六个 SysV 整数参数、RAX 返回值和目标槽
不被修改；CALL 不修改算术标志，返回后 RSP 恢复。当前翻译尚未把 guest 函数
地址解析为 translated function。

体系结构语义依据 [Intel 64 and IA-32 Architectures Software Developer's
Manual](https://www.intel.com/content/www/us/en/developer/articles/technical/intel-sdm.html)
中 CALL 指令的 near indirect r/m64 形式。
