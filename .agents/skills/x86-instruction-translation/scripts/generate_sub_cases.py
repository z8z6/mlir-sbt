#!/usr/bin/env python3
"""Generate the deterministic SUB encoding/semantic fixture matrix."""

from dataclasses import dataclass
from pathlib import Path

REGISTERS = ["rax", "rbx", "rcx", "rdx", "rsi", "rdi", "rbp", "rsp",
             "r8", "r9", "r10", "r11", "r12", "r13", "r14", "r15",
             "rip", "eflags", "cs", "ss", "ds", "es", "fs", "gs"]
FLAGS = ["cf", "pf", "af", "zf", "sf", "tf", "if", "df", "of"]
DEFAULTS = {
    "rax": 0x1111111111111111, "rbx": 0x2222222222222222,
    "rcx": 0, "rdx": 0x4444444444444444,
    "rsi": 0x5555555555555555, "rbp": 0x7777777777777777,
    "r8": 0x8888888888888888, "r9": 0x9999999999999999,
    "r10": 0xAAAAAAAAAAAAAAAA, "r11": 0xBBBBBBBBBBBBBBBB,
    "r12": 0xCCCCCCCCCCCCCCCC, "r13": 0xDDDDDDDDDDDDDDDD,
    "r14": 0xEEEEEEEEEEEEEEEE, "r15": 0xF0F0F0F0F0F0F0F0,
}
FLAG_MASK = 0x8D5


@dataclass(frozen=True)
class Case:
    name: str
    asm: str
    width: int
    lhs: int
    rhs: int
    destination: str
    offset: int = 0
    memory_destination: bool = False
    memory_input: int = 0x4444444444444444
    note: str = ""


CASES = [
    Case("rr8_low", "sub al, bl", 8, 0x10, 1, "rax"),
    Case("rr8_high", "sub ah, bh", 8, 0x10, 1, "rax", 8),
    Case("rr16_partial", "sub ax, bx", 16, 0x10, 1, "rax"),
    Case("rr32_zero_extend", "sub eax, ebx", 32, 0x10, 1, "rax"),
    Case("rr64_basic", "sub rax, rbx", 64, 0x10, 1, "rax"),
    Case("rr64_borrow", "sub rax, rbx", 64, 0, 1, "rax",
         note="Unsigned borrow sets CF."),
    Case("rr64_overflow", "sub rax, rbx", 64, 1 << 63, 1, "rax",
         note="INT64_MIN minus one sets OF."),
    Case("acc8_implicit", "sub al, 1", 8, 0x10, 1, "rax"),
    Case("acc16_implicit", "sub ax, strict word 1", 16, 0x10, 1, "rax"),
    Case("acc32_implicit", "sub eax, strict dword 1", 32, 0x10, 1, "rax"),
    Case("acc64_implicit", "sub rax, strict dword 1", 64, 0x10, 1, "rax"),
    Case("ri8", "sub bl, byte 1", 8, 0x10, 1, "rbx"),
    Case("ri16_imm8_signed", "sub bx, byte -1", 16, 1, 0xFFFF, "rbx"),
    Case("ri16_imm16", "sub bx, strict word 1", 16, 0x10, 1, "rbx"),
    Case("ri32_imm8_signed", "sub ebx, byte -1", 32, 1, 0xFFFFFFFF, "rbx"),
    Case("ri32_imm32", "sub ebx, strict dword 1", 32, 0x10, 1, "rbx"),
    Case("ri64_imm8_signed", "sub rbx, byte -1", 64, 1, 0xFFFFFFFFFFFFFFFF, "rbx"),
    Case("ri64_imm32", "sub rbx, strict dword 1", 64, 0x10, 1, "rbx"),
    Case("rm8", "sub al, byte [rdi + 24]", 8, 0x10, 1, "rax", memory_input=1),
    Case("rm16_sib", "sub ax, word [rdi + rcx * 4 + 24]", 16, 0x10, 1,
         "rax", memory_input=1),
    Case("rm32", "sub eax, dword [rdi + 24]", 32, 0x10, 1, "rax", memory_input=1),
    Case("rm64", "sub rax, qword [rdi + 24]", 64, 0x10, 1, "rax", memory_input=1),
    Case("mr8_partial", "sub byte [rdi + 24], al", 8, 0x10, 1, "memory",
         memory_destination=True, memory_input=0x1122334455667710),
    Case("mr16_partial", "sub word [rdi + 24], ax", 16, 0x10, 1, "memory",
         memory_destination=True, memory_input=0x1122334455660010),
    Case("mr32_partial", "sub dword [rdi + 24], eax", 32, 0x10, 1, "memory",
         memory_destination=True, memory_input=0x1122334400000010),
    Case("mr64", "sub qword [rdi + 24], rax", 64, 0x10, 1, "memory",
         memory_destination=True, memory_input=0x10),
    Case("mi8", "sub byte [rdi + 24], 1", 8, 0x10, 1, "memory",
         memory_destination=True, memory_input=0x1122334455667710),
    Case("mi16", "sub word [rdi + 24], strict word 1", 16, 0x10, 1, "memory",
         memory_destination=True, memory_input=0x1122334455660010),
    Case("mi32", "sub dword [rdi + 24], strict dword 1", 32, 0x10, 1, "memory",
         memory_destination=True, memory_input=0x1122334400000010),
    Case("mi64_imm8_signed", "sub qword [rdi + 24], byte -1", 64, 1,
         0xFFFFFFFFFFFFFFFF, "memory", memory_destination=True, memory_input=1),
    Case("mi64_imm32", "sub qword [rdi + 24], strict dword 1", 64, 0x10, 1,
         "memory", memory_destination=True, memory_input=0x10),
    Case("lock_mr64", "lock sub qword [rdi + 24], rax", 64, 0x10, 1,
         "memory", memory_destination=True, memory_input=0x10,
         note="Checks single-threaded LOCK result; atomicity needs a concurrency test."),
]


def flags(lhs: int, rhs: int, width: int) -> tuple[int, dict[str, int]]:
    mask = (1 << width) - 1
    lhs, rhs = lhs & mask, rhs & mask
    result = (lhs - rhs) & mask
    values = {
        "cf": int(lhs < rhs),
        "pf": int((result & 0xFF).bit_count() % 2 == 0),
        "af": int(bool((lhs ^ rhs ^ result) & 0x10)),
        "zf": int(result == 0),
        "sf": (result >> (width - 1)) & 1,
        "of": (((lhs ^ rhs) & (lhs ^ result)) >> (width - 1)) & 1,
    }
    packed = (0x202 & ~FLAG_MASK)
    for name, bit in {"cf": 0, "pf": 2, "af": 4, "zf": 6,
                      "sf": 7, "of": 11}.items():
        packed |= values[name] << bit
    return packed, values


def apply_field(old: int, value: int, width: int, offset: int,
                zero_extend: bool) -> int:
    mask = (1 << width) - 1
    if zero_extend:
        return value & mask
    field = mask << offset
    return (old & ~field) | ((value << offset) & field)


def emit(case: Case, root: Path) -> None:
    directory = root / case.name
    directory.mkdir(parents=True, exist_ok=True)
    input_regs = dict(DEFAULTS)
    input_regs.update({"rdi": "memory_base", "rsp": "runtime",
                       "rip": "run_case", "eflags": 0x202,
                       "cs": "runtime", "ss": "runtime", "ds": "runtime",
                       "es": "runtime", "fs": "runtime", "gs": "runtime"})

    if case.memory_destination:
        input_regs["rax"] = case.rhs
    elif case.destination == "rax":
        input_regs["rax"] = apply_field(DEFAULTS["rax"], case.lhs,
                                         case.width, case.offset, False)
        if case.name.startswith("rr"):
            input_regs["rbx"] = apply_field(DEFAULTS["rbx"], case.rhs,
                                             case.width, case.offset, False)
    elif case.destination == "rbx":
        input_regs["rbx"] = apply_field(DEFAULTS["rbx"], case.lhs,
                                         case.width, case.offset, False)

    result = (case.lhs - case.rhs) & ((1 << case.width) - 1)
    output_regs = {name: "unchanged" for name in REGISTERS}
    output_regs["rip"] = output_regs["rsp"] = "ignore"
    if not case.memory_destination:
        old = input_regs[case.destination]
        output_regs[case.destination] = apply_field(
            old, result, case.width, case.offset, case.width == 32)
    packed, flag_values = flags(case.lhs, case.rhs, case.width)
    output_regs["eflags"] = packed

    memory_output = case.memory_input
    if case.memory_destination:
        memory_output = apply_field(case.memory_input, result, case.width, 0,
                                    False)

    def value(v):
        return f'"{v}"' if isinstance(v, str) else hex(v)

    input_lines = ["[registers]"]
    output_lines = ["[registers]"]
    for name in REGISTERS:
        input_lines.append(f"{name} = {value(input_regs[name])}")
        output_lines.append(f"{name} = {value(output_regs[name])}")
    input_lines += ["", "[flags]"]
    output_lines += ["", "[flags]"]
    for name in FLAGS:
        input_lines.append(f"{name} = {1 if name == 'if' else 0}")
        expected = ("unchanged" if name in {"tf", "if", "df"}
                    else flag_values[name])
        output_lines.append(f"{name} = {value(expected)}")
    input_lines += ["", "[memory]", f'"rdi+24" = {hex(case.memory_input)}', ""]
    output_lines += ["", "[memory]", f'"rdi+24" = {hex(memory_output)}', ""]

    (directory / "case.asm").write_text(
        "bits 64\nsection .text\nglobal run_case\nrun_case:\n"
        f"  {case.asm}\n  ret\n"
        "section .note.GNU-stack noalloc noexec nowrite progbits\n")
    (directory / "input.toml").write_text("\n".join(input_lines))
    (directory / "output.toml").write_text("\n".join(output_lines))
    detail = case.note or "Covers the documented operand encoding and result flags."
    (directory / "README.md").write_text(
        f"# {case.name}\n\n被测指令：`{case.asm}`。\n\n{detail}\n\n"
        "语义依据：[SUB — Subtract](https://www.felixcloutier.com/x86/sub)。"
        "输入和完整期待状态分别见 `input.toml`、`output.toml`。\n")


def main() -> None:
    root = Path("tests/isa/x86/sub")
    root.mkdir(parents=True, exist_ok=True)
    for case in CASES:
        emit(case, root)
    (root / "README.md").write_text(
        "# x86 SUB instruction fixtures\n\n"
        "覆盖 Felix Cloutier SUB 页列出的 I、MI、MR、RM 操作数编码类别、"
        "8/16/32/64 位宽、符号扩展立即数、寄存器别名、flags 和内存写回。\n")
    print(f"generated {len(CASES)} SUB cases in {root}")


if __name__ == "__main__":
    main()
