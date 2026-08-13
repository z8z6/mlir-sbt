bits 64
section .text
global run_case
run_case:
  sub bx, byte -1
  ret
section .note.GNU-stack noalloc noexec nowrite progbits
