bits 64
global _start
extern message

section .text
_start:
  mov eax, 1
  mov edi, 1
  mov rsi, message
  mov edx, 14
  syscall

  mov eax, 60
  xor edi, edi
  syscall
  ret

section .note.GNU-stack noalloc noexec nowrite progbits
