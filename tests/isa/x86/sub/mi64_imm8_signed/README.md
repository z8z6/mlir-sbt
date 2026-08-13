# mi64_imm8_signed

被测指令：`sub qword [rdi + 24], byte -1`。

Covers the documented operand encoding and result flags.

语义依据：[SUB — Subtract](https://www.felixcloutier.com/x86/sub)。输入和完整期待状态分别见 `input.toml`、`output.toml`。
