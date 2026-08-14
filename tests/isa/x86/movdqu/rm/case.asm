bits 64
section .text
global run_case
run_case:
  movdqu xmm0, [rdi+32]
  ret
section .note.GNU-stack noalloc noexec nowrite progbits
