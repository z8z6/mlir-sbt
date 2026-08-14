bits 64
section .text
global run_case
run_case:
  cvttsd2si eax, xmm1
  ret
section .note.GNU-stack noalloc noexec nowrite progbits
