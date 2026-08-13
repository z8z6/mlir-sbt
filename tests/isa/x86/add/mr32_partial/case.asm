bits 64
section .text
global run_case
run_case:
  add dword [rdi + 24], eax
  ret
section .note.GNU-stack noalloc noexec nowrite progbits

