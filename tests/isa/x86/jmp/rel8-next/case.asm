bits 64
section .text
global run_case
run_case:
  jmp short $+2
  ret
section .note.GNU-stack noalloc noexec nowrite progbits
