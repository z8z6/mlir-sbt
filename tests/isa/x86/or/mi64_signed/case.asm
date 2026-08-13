bits 64
section .text
global run_case
run_case:
  or qword [rdi+24], byte -16
  ret
section .note.GNU-stack noalloc noexec nowrite progbits
