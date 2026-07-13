section .data
section .bss
section .text
global main

main:
  add eax, dword [rbx]
  add eax, dword [rbx + 4]
  add eax, dword [rbx + rsi * 4]
  add eax, dword [rbx + rsi * 4 + 16]
