bits 64
section .text
global run_case
run_case:
  movsx eax, bx
  ret
section .note.GNU-stack noalloc noexec nowrite progbits
