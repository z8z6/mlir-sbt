section .data
section .bss
section .text
global main

main:
    add rax, rbx
    add eax, ebx
    add ax, bx
    add al, bl

    add rax, 0x1
    add eax, 0x1
    add ax, 0x1
    add al, 0x1
    add ah, 0x1

    add eax, dword [rbx]
    add eax, dword [rbx + 4]
    add eax, dword [rbx + rsi * 4]
    add eax, dword [rbx + rsi * 4 + 16]