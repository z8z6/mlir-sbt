bits 64
section .text
global run_case
run_case:
  xor [rdi+24], rax
  ret
section .note.GNU-stack noalloc noexec nowrite progbits
