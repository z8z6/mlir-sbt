# hello_world

最小的 Linux x86-64 `_start` 程序，通过 `write` 系统调用输出
`Hello, world!`，随后通过 `exit` 系统调用正常退出。

原生测试负责验证 ELF 能运行且 stdout 与 `expected.txt` 完全一致。
翻译测试当前标记为 expected-failure，因为项目尚未实现 `SYSCALL`、guest
进程退出和完整程序 runtime 边界。这里的失败是受控的待办状态；一旦这些
能力落地，应删除 `translation.xfail` 并要求 `sbt` 成功生成目标文件。
