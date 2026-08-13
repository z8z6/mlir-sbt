# rr64_overflow

被测指令：`sub rax, rbx`。

INT64_MIN minus one sets OF.

语义依据：[SUB — Subtract](https://www.felixcloutier.com/x86/sub)。输入和完整期待状态分别见 `input.toml`、`output.toml`。
