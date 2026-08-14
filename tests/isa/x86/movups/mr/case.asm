bits 64
section .text
global run_case
run_case:
  movups [rdi+32], xmm1
  ret
section .note.GNU-stack noalloc noexec nowrite progbits
