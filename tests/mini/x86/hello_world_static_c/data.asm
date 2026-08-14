bits 64

section .rodata
global message
message: db "Hello, world!", 10

section .note.GNU-stack noalloc noexec nowrite progbits
