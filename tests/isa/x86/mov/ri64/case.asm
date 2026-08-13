bits 64
section .text
global run_case
run_case:
  mov rax, 0x1122334455667788
  ret
section .note.GNU-stack noalloc noexec nowrite progbits
