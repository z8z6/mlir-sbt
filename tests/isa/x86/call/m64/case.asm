bits 64
section .text
global run_case
run_case:
  call [rdi+24]
  ret
section .note.GNU-stack noalloc noexec nowrite progbits
