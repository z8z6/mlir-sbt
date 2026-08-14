bits 64
section .text
global run_case
run_case:
  test byte [rdi + 24], bl
  ret
section .note.GNU-stack noalloc noexec nowrite progbits
