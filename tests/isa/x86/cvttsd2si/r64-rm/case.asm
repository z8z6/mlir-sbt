bits 64
section .text
global run_case
run_case:
  cvttsd2si rax, [rdi+24]
  ret
section .note.GNU-stack noalloc noexec nowrite progbits
