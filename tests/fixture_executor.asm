bits 64
default rel

section .text
global execute_case
extern run_case
extern fixture_input
extern fixture_output
extern fixture_xmm_input
extern fixture_xmm_output

%define RAX 0
%define RBX 8
%define RCX 16
%define RDX 24
%define RSI 32
%define RDI 40
%define RBP 48
%define RSP 56
%define R8_ 64
%define R9_ 72
%define R10_ 80
%define R11_ 88
%define R12_ 96
%define R13_ 104
%define R14_ 112
%define R15_ 120
%define RIP 128
%define EFLAGS 136
%define CS 144
%define SS 152
%define DS 160
%define ES 168
%define FS 176
%define GS 184

execute_case:
  push rbx
  push rbp
  push r12
  push r13
  push r14
  push r15

  ; Record environment-owned input registers before the guest instruction.
  mov [rel fixture_input + RSP], rsp
  lea rax, [rel run_case]
  mov [rel fixture_input + RIP], rax
  xor eax, eax
  mov ax, cs
  mov [rel fixture_input + CS], rax
  mov ax, ss
  mov [rel fixture_input + SS], rax
  mov ax, ds
  mov [rel fixture_input + DS], rax
  mov ax, es
  mov [rel fixture_input + ES], rax
  mov ax, fs
  mov [rel fixture_input + FS], rax
  mov ax, gs
  mov [rel fixture_input + GS], rax

  mov rax, [rel fixture_input + EFLAGS]
  push rax
  popfq
  mov rax, [rel fixture_input + RAX]
  mov rbx, [rel fixture_input + RBX]
  mov rcx, [rel fixture_input + RCX]
  mov rdx, [rel fixture_input + RDX]
  mov rsi, [rel fixture_input + RSI]
  mov rbp, [rel fixture_input + RBP]
  mov r8,  [rel fixture_input + R8_]
  mov r9,  [rel fixture_input + R9_]
  mov r10, [rel fixture_input + R10_]
  mov r11, [rel fixture_input + R11_]
  mov r12, [rel fixture_input + R12_]
  mov r13, [rel fixture_input + R13_]
  mov r14, [rel fixture_input + R14_]
  mov r15, [rel fixture_input + R15_]
  mov rdi, [rel fixture_input + RDI]

  movdqu xmm0,  [rel fixture_xmm_input + 0]
  movdqu xmm1,  [rel fixture_xmm_input + 16]
  movdqu xmm2,  [rel fixture_xmm_input + 32]
  movdqu xmm3,  [rel fixture_xmm_input + 48]
  movdqu xmm4,  [rel fixture_xmm_input + 64]
  movdqu xmm5,  [rel fixture_xmm_input + 80]
  movdqu xmm6,  [rel fixture_xmm_input + 96]
  movdqu xmm7,  [rel fixture_xmm_input + 112]
  movdqu xmm8,  [rel fixture_xmm_input + 128]
  movdqu xmm9,  [rel fixture_xmm_input + 144]
  movdqu xmm10, [rel fixture_xmm_input + 160]
  movdqu xmm11, [rel fixture_xmm_input + 176]
  movdqu xmm12, [rel fixture_xmm_input + 192]
  movdqu xmm13, [rel fixture_xmm_input + 208]
  movdqu xmm14, [rel fixture_xmm_input + 224]
  movdqu xmm15, [rel fixture_xmm_input + 240]

  call run_case

  movdqu [rel fixture_xmm_output + 0], xmm0
  movdqu [rel fixture_xmm_output + 16], xmm1
  movdqu [rel fixture_xmm_output + 32], xmm2
  movdqu [rel fixture_xmm_output + 48], xmm3
  movdqu [rel fixture_xmm_output + 64], xmm4
  movdqu [rel fixture_xmm_output + 80], xmm5
  movdqu [rel fixture_xmm_output + 96], xmm6
  movdqu [rel fixture_xmm_output + 112], xmm7
  movdqu [rel fixture_xmm_output + 128], xmm8
  movdqu [rel fixture_xmm_output + 144], xmm9
  movdqu [rel fixture_xmm_output + 160], xmm10
  movdqu [rel fixture_xmm_output + 176], xmm11
  movdqu [rel fixture_xmm_output + 192], xmm12
  movdqu [rel fixture_xmm_output + 208], xmm13
  movdqu [rel fixture_xmm_output + 224], xmm14
  movdqu [rel fixture_xmm_output + 240], xmm15

  mov [rel fixture_output + RAX], rax
  mov [rel fixture_output + RBX], rbx
  mov [rel fixture_output + RCX], rcx
  mov [rel fixture_output + RDX], rdx
  mov [rel fixture_output + RSI], rsi
  mov [rel fixture_output + RDI], rdi
  mov [rel fixture_output + RBP], rbp
  mov [rel fixture_output + RSP], rsp
  mov [rel fixture_output + R8_], r8
  mov [rel fixture_output + R9_], r9
  mov [rel fixture_output + R10_], r10
  mov [rel fixture_output + R11_], r11
  mov [rel fixture_output + R12_], r12
  mov [rel fixture_output + R13_], r13
  mov [rel fixture_output + R14_], r14
  mov [rel fixture_output + R15_], r15
  lea rax, [rel .after_case]
  mov [rel fixture_output + RIP], rax
.after_case:
  pushfq
  pop qword [rel fixture_output + EFLAGS]
  xor eax, eax
  mov ax, cs
  mov [rel fixture_output + CS], rax
  mov ax, ss
  mov [rel fixture_output + SS], rax
  mov ax, ds
  mov [rel fixture_output + DS], rax
  mov ax, es
  mov [rel fixture_output + ES], rax
  mov ax, fs
  mov [rel fixture_output + FS], rax
  mov ax, gs
  mov [rel fixture_output + GS], rax

  pop r15
  pop r14
  pop r13
  pop r12
  pop rbp
  pop rbx
  ret

section .note.GNU-stack noalloc noexec nowrite progbits
