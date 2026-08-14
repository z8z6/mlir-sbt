bits 64
section .text
global run_case
run_case:
  endbr64
  ret
section .note.GNU-stack noalloc noexec nowrite progbits
