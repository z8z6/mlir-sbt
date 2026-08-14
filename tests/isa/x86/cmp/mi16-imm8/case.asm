bits 64
section .text
global run_case
run_case:
  cmp word [rdi + 24], byte 3
  ret
section .note.GNU-stack noalloc noexec nowrite progbits
