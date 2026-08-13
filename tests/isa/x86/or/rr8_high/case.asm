bits 64
section .text
global run_case
run_case:
  or ah, bh
  ret
section .note.GNU-stack noalloc noexec nowrite progbits
