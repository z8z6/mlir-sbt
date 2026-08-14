bits 64
section .text
global run_case
run_case:
  test word [rdi + 24], bx
  ret
section .note.GNU-stack noalloc noexec nowrite progbits
