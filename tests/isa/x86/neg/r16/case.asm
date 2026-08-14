bits 64
section .text
global run_case
run_case:
  neg ax
  ret
section .note.GNU-stack noalloc noexec nowrite progbits
