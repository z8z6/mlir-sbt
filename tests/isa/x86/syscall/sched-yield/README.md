# sched-yield

被测指令：`syscall`，RAX 输入为 Linux x86-64 的 `SYS_sched_yield`
编号 24。该系统调用没有参数且成功时稳定返回 0，因此适合作为不依赖
PID、UID、文件描述符或内存映射的 native/translated 对照用例。

`syscall` 没有显式操作数。RAX 接收返回值；RCX 保存两字节指令后的 RIP，
R11 保存进入内核前的 RFLAGS。其余通用寄存器和内存保持不变。Linux 返回
用户态后 RFLAGS 与调用前一致，本用例逐位检查全部建模标志。
