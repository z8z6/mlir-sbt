bits 64
section .text
global run_case
run_case:
  cmove ax, word [rdi + 24]
  ret
section .note.GNU-stack noalloc noexec nowrite progbits
