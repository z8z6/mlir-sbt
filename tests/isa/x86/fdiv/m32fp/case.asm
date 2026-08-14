bits 64
section .text
global run_case
run_case:
  fdiv dword [rdi + 24]
  ret
section .note.GNU-stack noalloc noexec nowrite progbits
