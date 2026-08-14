bits 64
section .text
global run_case
run_case:
  fmulp st1, st0
  ret
section .note.GNU-stack noalloc noexec nowrite progbits
