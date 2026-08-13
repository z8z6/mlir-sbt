bits 64
section .text
global run_case
run_case:
  add word [rdi + 24], ax
  ret
section .note.GNU-stack noalloc noexec nowrite progbits

