#!/usr/bin/env python3
import argparse
import re
import sys
import tomllib
from pathlib import Path

REGISTERS = {"rax", "rbx", "rcx", "rdx", "rsi", "rdi", "rbp", "rsp",
             "r8", "r9", "r10", "r11", "r12", "r13", "r14", "r15",
             "rip", "eflags", "cs", "ss", "ds", "es", "fs", "gs"}
FLAGS = {"cf", "pf", "af", "zf", "sf", "tf", "if", "df", "of"}
FILES = {"case.asm", "input.toml", "output.toml", "README.md"}
RUNTIME_REGISTERS = {"rsp", "cs", "ss", "ds", "es", "fs", "gs"}

def validate(case: Path) -> list[str]:
    errors = []
    if {p.name for p in case.iterdir() if p.is_file()} != FILES:
        errors.append(f"{case}: must contain exactly {sorted(FILES)}")
    for filename in ("input.toml", "output.toml"):
        try:
            data = tomllib.loads((case / filename).read_text())
        except Exception as error:
            errors.append(f"{case / filename}: {error}")
            continue
        if set(data.get("registers", {})) != REGISTERS:
            errors.append(f"{case / filename}: incomplete register set")
        if set(data.get("flags", {})) != FLAGS:
            errors.append(f"{case / filename}: incomplete flag set")
        if "rdi+24" not in data.get("memory", {}):
            errors.append(f"{case / filename}: missing memory address rdi+24")
        registers = data.get("registers", {})
        if filename == "input.toml":
            runtime = {name for name, value in registers.items() if value == "runtime"}
            if runtime - RUNTIME_REGISTERS:
                errors.append(f"{case / filename}: runtime used for stable register(s) {sorted(runtime - RUNTIME_REGISTERS)}")
            if registers.get("rdi") != "memory_base" or registers.get("rip") != "run_case":
                errors.append(f"{case / filename}: rdi/rip must use memory_base/run_case")
        else:
            ignored = {name for name, value in registers.items() if value == "ignore"}
            if ignored - {"rip", "rsp"}:
                errors.append(f"{case / filename}: only rip/rsp may be ignored")
        vectors = data.get("vectors", {})
        invalid_vectors = set(vectors) - {f"xmm{i}" for i in range(16)}
        if invalid_vectors:
            errors.append(f"{case / filename}: invalid vector register(s) {sorted(invalid_vectors)}")
    asm = (case / "case.asm").read_text().splitlines()
    instructions = [line.strip() for line in asm if line.strip() and not
                    re.match(r"^(bits|section|global|run_case:|ret$)", line.strip())]
    if len(instructions) != 1:
        errors.append(f"{case / 'case.asm'}: expected one tested instruction")
    elif instructions:
        words = instructions[0].lower().split()
        opcode = words[1] if words[0] == "lock" and len(words) > 1 else words[0]
        if opcode != case.parent.name.lower():
            errors.append(
                f"{case / 'case.asm'}: opcode {opcode!r} does not match "
                f"instruction directory {case.parent.name!r}")
    return errors

def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("root", type=Path)
    args = parser.parse_args()
    errors = []
    for case in sorted(p.parent for p in args.root.glob("*/case.asm")):
        errors.extend(validate(case))
    if errors:
        print("\n".join(errors), file=sys.stderr)
        return 1
    print(f"validated {len(list(args.root.glob('*/case.asm')))} cases")
    return 0

if __name__ == "__main__":
    raise SystemExit(main())
