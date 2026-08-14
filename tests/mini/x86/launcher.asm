bits 64
default rel

section .text
global _start
extern translated_block

_start:
  lea rdi, [rel translated_state]
  call translated_block

  ; A translated SYS_exit never returns. Keep a deterministic fallback for a
  ; mini program that reaches the end of translated_block normally.
  mov eax, 60
  xor edi, edi
  syscall

section .bss
align 16
translated_state: resq 65

section .note.GNU-stack noalloc noexec nowrite progbits
