# GCC static hello world

`program.c` is compiled as freestanding x86-64 C and linked by `ld -static`
without an ELF interpreter or dynamic section. The explicit Linux `write` and
`exit` syscall ABI keeps this mini program focused on compiler-generated code
rather than glibc startup internals.

With the repository's GCC flags, the linked `.text` contains these LLVM MC
opcode families, all of which must translate successfully:

- `MOV32ri`
- `XOR32rr`
- `SYSCALL`
- `RET64`

The native and translated executables must both write exactly
`Hello, world!\n`.
