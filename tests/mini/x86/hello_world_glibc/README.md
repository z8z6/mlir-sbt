# Default GCC + glibc hello world

The native oracle is built with the unmodified command shape
`gcc program.c -o program`: it is a PIE executable using the system dynamic
loader and glibc `puts`.

The translator discovers `main` from the ELF `STT_FUNC` symbol and resolves
its direct call target through `.rela.plt` to `puts`. The translated object is
linked into a small non-PIE glibc launcher. An address bias of `0x400000` maps
the PIE virtual address of the original `.rodata` string to the launcher's
corresponding non-PIE address.
