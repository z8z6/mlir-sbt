bits 64
section .text
global run_case
run_case:
  add dword [rdi + 24], strict dword 0x7fffffff
  ret
section .note.GNU-stack noalloc noexec nowrite progbits

