ENTRY(_start)

SECTIONS
{
  . = 0x401000;
  .text : { *(.text) }

  . = 0x402000;
  .rodata : { *(.rodata) }
  .data : { *(.data) }
  .bss : { *(.bss) *(COMMON) }

  /DISCARD/ : { *(.note.GNU-stack) }
}
