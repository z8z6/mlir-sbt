#!/usr/bin/env python3
import argparse
from pathlib import Path

REGISTERS = ["rax", "rbx", "rcx", "rdx", "rsi", "rdi", "rbp", "rsp",
             "r8", "r9", "r10", "r11", "r12", "r13", "r14", "r15",
             "rip", "eflags", "cs", "ss", "ds", "es", "fs", "gs"]
FLAGS = ["cf", "pf", "af", "zf", "sf", "tf", "if", "df", "of"]

INPUT_SPECIAL = {"rdi": '"memory_base"', "rsp": '"runtime"',
                 "rip": '"run_case"', "cs": '"runtime"',
                 "ss": '"runtime"', "ds": '"runtime"',
                 "es": '"runtime"', "fs": '"runtime"', "gs": '"runtime"'}

def toml(output: bool) -> str:
    lines = ["[registers]"]
    for index, name in enumerate(REGISTERS):
        if output:
            value = '"ignore"' if name in {"rip", "rsp"} else '"unchanged"'
        else:
            value = INPUT_SPECIAL.get(name, hex(0x1111111111111111 * (index + 1) & ((1 << 64) - 1)))
            if name == "eflags": value = "0x202"
        lines.append(f"{name} = {value}")
    lines += ["", "[flags]"]
    for name in FLAGS:
        value = '"unchanged"' if output else ("1" if name == "if" else "0")
        lines.append(f"{name} = {value}")
    lines += ["", "[memory]", '"rdi+24" = 0x4444444444444444', ""]
    return "\n".join(lines)

def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("--instruction", required=True)
    parser.add_argument("--case", required=True)
    parser.add_argument("--asm", required=True)
    parser.add_argument("--root", default="tests/isa/x86")
    args = parser.parse_args()
    opcode = args.asm.split()[0].lower() if args.asm.split() else ""
    if opcode == "lock" and len(args.asm.split()) > 1:
        opcode = args.asm.split()[1].lower()
    if opcode != args.instruction.lower():
        parser.error(f"assembly opcode {opcode!r} does not match instruction {args.instruction!r}")
    case = Path(args.root) / args.instruction.lower() / args.case
    case.mkdir(parents=True, exist_ok=False)
    (case / "case.asm").write_text(
        f"bits 64\nsection .text\nglobal run_case\nrun_case:\n  {args.asm}\n  ret\n"
        "section .note.GNU-stack noalloc noexec nowrite progbits\n")
    (case / "input.toml").write_text(toml(False))
    (case / "output.toml").write_text(toml(True))
    (case / "README.md").write_text(
        f"# {args.case}\n\n被测指令：`{args.asm}`。\n\n"
        "说明操作数形式、边界条件、寄存器/内存效果和标志位预期。\n")

if __name__ == "__main__":
    main()
