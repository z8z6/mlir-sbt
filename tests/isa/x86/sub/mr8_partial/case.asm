bits 64
section .text
global run_case
run_case:
  sub byte [rdi + 24], al
  ret
section .note.GNU-stack noalloc noexec nowrite progbits
