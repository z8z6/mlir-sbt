bits 64
section .text
global run_case
run_case:
  sub ebx, strict dword 1
  ret
section .note.GNU-stack noalloc noexec nowrite progbits
