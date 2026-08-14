bits 64

; GCC's default PIE places _IO_stdin_used at .rodata+0 and the string at +4.
; The translated non-PIE launcher contributes the same four-byte prefix.
section .rodata
global translated_message
translated_message: db "Hello, world!", 0

section .note.GNU-stack noalloc noexec nowrite progbits
