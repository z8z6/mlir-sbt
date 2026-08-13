# mr32_partial

被测指令：`sub dword [rdi + 24], eax`。

Covers the documented operand encoding and result flags.

语义依据：[SUB — Subtract](https://www.felixcloutier.com/x86/sub)。输入和完整期待状态分别见 `input.toml`、`output.toml`。
