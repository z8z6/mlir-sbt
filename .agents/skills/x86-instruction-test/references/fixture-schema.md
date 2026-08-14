# Fixture schema

Each case contains exactly `case.asm`, `input.toml`, `output.toml`, and
`README.md`.

## Register set

Model the common x86-64 `gdb info registers` view in this exact order:

`rax rbx rcx rdx rsi rdi rbp rsp r8 r9 r10 r11 r12 r13 r14 r15 rip eflags cs ss ds es fs gs`

All names must occur in both TOML files. Numeric values use hexadecimal.

- Input `rdi = "memory_base"` gives the case a valid base for `rdi+24`.
- Input `rip = "run_case"` documents the instruction entry.
- Input register or memory value `"call_target"` resolves to the generic
  returning SysV helper used by indirect CALL fixtures.
- Input `rsp`, and segment registers may use `"runtime"` when owned by the
  harness/environment.
- Output `rip` and `rsp` may use `"ignore"` because calls and ASLR vary them.
- Output `rcx = "rip+2"` expresses SYSCALL's fixed two-byte next-instruction
  address without embedding an ASLR-dependent native code address.
- Output `rax = "rdi+24"` can express an effective-address result using the
  fixture's symbolic memory base, as required by LEA cases.
- Output `eflags = "flags"` delegates comparison to the complete `[flags]`
  table. Use this when an instruction leaves at least one flag undefined.
- Output registers unaffected by the instruction use `"unchanged"`.
- Do not ignore ordinary GPRs, segment selectors, or flags merely for
  convenience.

## Flags

Both files contain `[flags]` with every field:

`cf pf af zf sf tf if df of`

Use `0` or `1` for specified inputs/results. Use `"unchanged"` in output for
flags the instruction preserves. ADD and SUB define CF, PF, AF, ZF, SF, OF and
preserve TF, IF, DF; consult the architecture reference for every other
instruction rather than copying this flag set blindly.

`eflags` remains present in `[registers]` to match GDB. Keep it consistent with
the flag fields; the runner also validates individual flags so failures remain
diagnosable.

Use `"ignore"` only for architecturally undefined individual flags. Defined
and preserved flags must still use `0`, `1`, or `"unchanged"`.

## Memory

Use a `[memory]` table whose quoted keys are aligned symbolic effective
addresses from `"rdi+24"` through `"rdi+80"`:

```toml
[memory]
"rdi+24" = 0x1122334455667788
```

Input describes initialization. Output lists addresses that must be checked
and their expected complete 64-bit values, including preservation around
narrow writes. A 128-bit memory operand uses two consecutive entries, for
example `"rdi+24"` for the low 64 bits and `"rdi+32"` for the high 64 bits.

## Assembly purity

`case.asm` contains assembler directives, the `run_case` label, exactly one
instruction under test, and `ret`. It must not contain `mov`, `push`, `pop`, or
other preparation/observation instructions.

## x87 stack

x87 fixtures add an `[x87]` table containing every logical stack register from
`st0` through `st7`. Values are exact 80-bit extended-precision hexadecimal
bit patterns (20 hexadecimal digits), independent of the host `long double`
layout. Output entries use an exact value or `"unchanged"`. A popping
instruction shifts the remaining logical stack registers down; only the newly
empty `st7` output may use `"ignore"`.

The fixture runner initializes a full eight-entry x87 stack and compares the
post-instruction logical order. Control word, status word, tag word, physical
TOP, and unmasked exception behavior are outside this fixture schema.

## Execution paths

Every fixture has two consumers with the same TOML state:

1. The native fixture executes `run_case` on the host CPU and provides the ISA
   oracle.
2. The translated fixture assembles `case.asm`, runs the `sbt` executable to
   emit an object containing `translated_block(uint64_t *state)`, links that
   object into the fixture runner, and checks the resulting state.

Both tests must pass. A lowering-only/JIT unit test is useful additional
coverage but does not replace the static translation path.

## Vector registers

SSE fixtures may add a `[vectors]` table containing `xmm0` through `xmm15` as
128-bit hexadecimal strings. Output entries use an exact 128-bit value or
`"unchanged"`. Scalar legacy SSE cases must check the preserved upper 96/64
bits of the destination as well as the computed low element.
