bits 64
section .text
global run_case
run_case:
  cmove rax, rbx
  ret
section .note.GNU-stack noalloc noexec nowrite progbits
