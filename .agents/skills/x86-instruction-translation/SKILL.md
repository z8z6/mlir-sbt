---
name: x86-instruction-translation
description: Implement and audit mlir-sbt x86 instruction translation families from architecture references through LLVM MC opcode dispatch, IR1 semantics, full lowering, object emission, and native-versus-translated tests. Use when adding a new x86 mnemonic, completing missing operand encodings, changing flag or register semantics, updating the ISA support matrix, or diagnosing an unsupported converter.
---

# X86 Instruction Translation

Implement one documented mnemonic family at a time. A generated converter
class or a successfully emitted object does not prove semantic support.

## Workflow

1. Read the instruction page requested by the user. Use the Felix Cloutier page
   as the convenient index and record its URL; consult the Intel SDM when a
   detail is ambiguous or correctness is high-risk.
2. Extract every 64-bit-mode encoding row, operand role, width, immediate
   extension rule, implicit operand, defined/preserved/undefined flag, LOCK
   rule, and relevant exception. Write the implementation checklist before
   changing dispatch.
3. Inspect the actual LLVM MC opcode produced by NASM for every encoding. Never
   infer opcode suffixes solely from the assembly spelling.
4. Update generated dispatch only for opcode classes whose semantics are
   implemented. An unimplemented converter must cause translation failure; it
   must never emit an empty translated block and return success.
5. Emit architecture-specific semantics such as x86 flags, implicit operands,
   legacy XMM lane preservation, atomicity, and exceptions in `X86_Dialect`.
   Lower `X86_Dialect` to architecture-neutral primitive IR1 operations first;
   IR1 lowering must not depend on X86 op classes. Add both lowering stages in
   the same change.
6. Put mnemonic dispatch in `src/Trans/X86/<MNEMONIC>.cpp`. Before adding
   operand decoding, address construction, state access, or flag calculation,
   inspect `include/Trans/X86` and `src/Trans/X86` for a reusable family helper.
   Related instructions such as ADD/SUB must share operand and lowering
   skeletons while retaining their distinct arithmetic formulas.
7. Preserve the register-state ABI and alias rules: 32-bit writes zero extend;
   8/16-bit and high-byte writes merge; arithmetic flags update only the bits
   defined by the instruction. Keep LLVM x86 register IDs and alias policy in
   `X86_Dialect`; its lowering must normalize them to IR1 state slot, bit
   offset, storage width, and write mask attributes.
8. Use `$x86-instruction-test` to create a flat fixture matrix covering every
   operand encoding and width plus boundary flags, aliases, immediate sign
   extension, memory preservation, and negative/unsupported cases.
9. Require both the native oracle and the `.translated` CTest for every case.
   Run the whole suite so a new family cannot regress an existing family.
10. Update `references/support-matrix.md` with exact support and limitations.
   Do not label LOCK, exceptions, or control flow complete when only the
   single-threaded value result is tested.

## Repository map

- Opcode class generation: `tblgen/src/X86HeaderGenerator.cpp`
- Opcode dispatch generation: `tblgen/src/X86ImplGenerator.cpp`
- X86 family translators: `src/Trans/X86/<MNEMONIC>.cpp`
- Shared X86 translation helpers: `include/Trans/X86/`,
  `src/Trans/X86/Common.cpp`
- Shared arithmetic-family translation: `src/Trans/X86/Arithmetic.cpp`
- IR1 definitions: `mlir/IR1.td`
- X86 semantic definitions: `mlir/X86.td`
- X86 to IR1 lowering: `src/Pass/X86Lowering.cpp`
- IR1 to generic/LLVM lowering: `src/Pass/IR1Lowering.cpp`
- Register aliases/state ABI: `include/Target/X86Register.h`
- End-to-end fixtures: `tests/isa/x86/<mnemonic>/<case>/`
- Generic fixture build/runner: `tests/CMakeLists.txt`,
  `tests/fixture_runner.cpp`

Read `references/sub.md` when changing SUB or reusing its subtraction flag
formulas. Read `references/support-matrix.md` before choosing the next family.

## Validation

```sh
python3 .agents/skills/x86-instruction-test/scripts/validate_cases.py \
  tests/isa/x86/<mnemonic>
cmake --build build -j 8
ctest --test-dir build --output-on-failure \
  -R '^isa\.x86\.<mnemonic>\.' -j 8
ctest --test-dir build --output-on-failure -j 8
git diff --check
```

For a new family, also assemble one intentionally unimplemented encoding and
confirm `sbt --quiet` returns nonzero. Keep reference-derived claims linked in
the case README or family summary.
