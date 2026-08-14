bits 64
section .text
global run_case
run_case:
  fmul st0, st1
  ret
section .note.GNU-stack noalloc noexec nowrite progbits
