# lock_mr64

被测指令：`lock sub qword [rdi + 24], rax`。

Checks single-threaded LOCK result; atomicity needs a concurrency test.

语义依据：[SUB — Subtract](https://www.felixcloutier.com/x86/sub)。输入和完整期待状态分别见 `input.toml`、`output.toml`。
