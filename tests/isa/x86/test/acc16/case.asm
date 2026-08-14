bits 64
section .text
global run_case
run_case:
  test ax, 3
  ret
section .note.GNU-stack noalloc noexec nowrite progbits
