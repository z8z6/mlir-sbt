bits 64
section .text
global run_case
run_case:
  cmp qword [rdi + 24], rbx
  ret
section .note.GNU-stack noalloc noexec nowrite progbits
