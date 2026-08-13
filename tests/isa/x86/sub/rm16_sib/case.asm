bits 64
section .text
global run_case
run_case:
  sub ax, word [rdi + rcx * 4 + 24]
  ret
section .note.GNU-stack noalloc noexec nowrite progbits
