bits 64
section .text
global run_case
run_case:
  lock sub qword [rdi + 24], rax
  ret
section .note.GNU-stack noalloc noexec nowrite progbits
