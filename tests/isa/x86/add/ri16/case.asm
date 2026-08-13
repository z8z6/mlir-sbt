bits 64
section .text
global run_case
run_case:
  add ax, strict word 0x7fff
  ret
section .note.GNU-stack noalloc noexec nowrite progbits

