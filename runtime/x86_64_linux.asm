bits 64
default rel

section .text
global __sbt_syscall6

; uint64_t __sbt_syscall6(number, arg0, arg1, arg2, arg3, arg4, arg5)
; Convert the SysV function-call ABI to the Linux x86-64 syscall ABI. The
; seventh function argument is on the stack; the raw kernel result stays in
; RAX, including negative errno values.
__sbt_syscall6:
  mov rax, rdi
  mov rdi, rsi
  mov rsi, rdx
  mov rdx, rcx
  mov r10, r8
  mov r8, r9
  mov r9, [rsp + 8]
  syscall
  ret

section .note.GNU-stack noalloc noexec nowrite progbits
