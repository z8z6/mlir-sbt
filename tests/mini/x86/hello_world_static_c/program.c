extern const char message[];

// Freestanding Linux x86-64 entry point. Keeping the kernel ABI explicit
// makes this a compiler-generated static mini program without pulling in the
// unrelated glibc startup/runtime instruction surface.
__attribute__((used)) void _start(void) {
  register long number __asm__("rax") = 1;
  register long fd __asm__("rdi") = 1;
  register const char *buffer __asm__("rsi") = message;
  register long length __asm__("rdx") = 14;
  __asm__ volatile("syscall"
                   : "+a"(number)
                   : "D"(fd), "S"(buffer), "d"(length)
                   : "rcx", "r11", "memory");

  number = 60;
  register long status __asm__("rdi") = 0;
  __asm__ volatile("syscall"
                   : "+a"(number)
                   : "D"(status)
                   : "rcx", "r11", "memory");
}
