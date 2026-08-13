---
name: x86-instruction-test
description: Create, update, and validate mlir-sbt x86 instruction test fixtures. Use when adding instruction cases under tests/isa/x86, defining complete GDB-style register and flag input/output state, adding memory expectations, keeping assembly cases instruction-pure, or expanding CTest instruction coverage.
---

# x86 Instruction Test

Create one flat case directory at `tests/isa/x86/<instruction>/<case-name>/`.

## Workflow

1. Read `references/fixture-schema.md` before editing a fixture or runner.
2. Run `scripts/create_case.py` to scaffold new cases. Do not hand-copy an
   existing case and risk stale state.
3. Put exactly one instruction under test in `case.asm`; allow only the `ret`
   required to return to the runner.
4. Fill every register and every flag in `input.toml` and `output.toml`.
   Preserve unrelated registers with `"unchanged"`. Mark only genuinely
   run-dependent `rip` and `rsp` as `"ignore"` in output.
5. Add every read memory location to input and every location that must be
   checked to output. Use the symbolic address spelling used by the fixture
   runner, currently `"rdi+24"`.
6. Explain the instruction form, boundary condition, aliases/extension rules,
   memory effects, and every defined flag in the case `README.md`.
7. Build the case and require both its native ISA oracle and its
   `.translated` CTest to pass. The translated test must invoke `sbt` on the
   assembled case, link the emitted object, and execute `translated_block`;
   do not substitute a hand-built IR program for this end-to-end path.
8. Run `scripts/validate_cases.py` and then the complete CTest suite. Never
   accept a fixture that passes only because a register or flag was omitted.

## Commands

```sh
python3 .agents/skills/x86-instruction-test/scripts/create_case.py \
  --instruction add --case rr64-example --asm 'add rax, rbx'
python3 .agents/skills/x86-instruction-test/scripts/validate_cases.py \
  tests/isa/x86/add
cmake --build build -j 8
ctest --test-dir build --output-on-failure -j 8
```

To isolate static-translation regressions, run:

```sh
ctest --test-dir build --output-on-failure -R '\.translated$'
```

Edit the scaffold's sentinel values and README after creation. Keep the
project-level runner generic; do not add preparation or observation
instructions to individual `case.asm` files.
