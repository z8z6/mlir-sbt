bits 64
section .text
global run_case
run_case:
  cmp dword [rdi + 24], 0x1234
  ret
section .note.GNU-stack noalloc noexec nowrite progbits
