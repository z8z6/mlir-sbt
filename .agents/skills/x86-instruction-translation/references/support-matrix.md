# x86 translation support matrix

Status meanings:

- `complete-fixture`: supported forms have native and statically translated
  deterministic fixtures for register, flags, and memory state.
- `partial`: a useful subset works, but a documented architectural property is
  not implemented or tested.
- `unsupported`: translation must fail rather than silently emit no semantics.

| Mnemonic | Status | Implemented scope | Known gaps |
|---|---|---|---|
| ADD | complete-fixture | I/MI/MR/RM fixtures; 8/16/32/64-bit register aliases, immediates, memory and flags | LOCK atomicity; general exceptions; random differential tests |
| SUB | complete-fixture | 32 cases covering I/MI/MR/RM, all scalar widths, full/sign-extended immediates, SIB, aliases, borrow/overflow and memory preservation | LOCK atomicity; general exceptions; random differential tests |
| MOV | partial | GPR RR/RI/RM/MR/MI for 8/16/32/64-bit widths; fixture coverage for aliases, imm64, sign-extended imm32, and memory | Segment/moffs/control/debug forms; exceptions; exhaustive fixtures |
| MOVSX/MOVSXD/MOVZX | complete-fixture | All 18 ordinary GPR register/memory extension encodings: 8/16/32-bit sources and 16/32/64-bit destinations; negative sign-extension and high-half clearing fixtures | Codegen-only NOREX aliases; exceptions |
| CMP | complete-fixture | All 30 scalar RR/RI/RM/MR/MI/accumulator encodings at 8/16/32/64-bit widths; arithmetic flags without destination write | Exceptions; random differential tests |
| TEST | complete-fixture | All 20 scalar RR/RI/MR/MI/accumulator encodings at 8/16/32/64-bit widths; logical flags without destination write | AF is architecturally undefined and ignored by fixtures; exceptions |
| CMOVcc | complete-fixture | 16/32/64-bit register and memory sources; condition decoding through RFLAGS; taken and not-taken fixtures | Fault-suppression subtleties and exhaustive 16-condition fixtures |
| SETcc | complete-fixture | Byte register and memory destinations; condition decoding through RFLAGS; taken and not-taken fixtures | Exhaustive 16-condition fixtures; exceptions |
| NOP/ENDBR64 | partial | Single/multi-byte NOP encodings and ENDBR64 as data-state no-ops, with native/translated fixtures | CET indirect-branch tracking and control-protection exceptions |
| AND | partial | GPR I/MI/MR/RM for 8/16/32/64-bit widths; logical flags; register aliases and memory | AF is architecturally undefined and normalized to 0 internally; LOCK atomicity; exhaustive fixtures |
| OR | partial | GPR I/MI/MR/RM for 8/16/32/64-bit widths; logical flags; register aliases and memory | AF is architecturally undefined and normalized to 0 internally; LOCK atomicity; exhaustive fixtures |
| XOR | partial | GPR I/MI/MR/RM for 8/16/32/64-bit widths; logical flags; register aliases and memory | AF is architecturally undefined and normalized to 0 internally; LOCK atomicity; exhaustive fixtures |
| NEG | complete-fixture | 8/16/32/64-bit register and memory destinations; exact SUB-from-zero flags; partial-register preservation and 32-bit zero extension; eight native/translated fixtures | LOCK atomicity; APX NDD/NF and EVEX forms; exceptions |
| NOT | complete-fixture | 8/16/32/64-bit register and memory destinations; preserved RFLAGS; partial-register preservation and 32-bit zero extension; eight native/translated fixtures | LOCK atomicity; APX NDD/NF and EVEX forms; exceptions |
| MOVAPS/MOVUPS/MOVDQA/MOVDQU | complete-fixture | Legacy 128-bit XMM register copies, memory loads and memory stores; aligned and deliberately unaligned memory cases; twelve native/translated fixtures | VEX/EVEX forms; alignment/general exceptions |
| JMP | partial | Direct near `rel8`/`rel32` (`JMP_1`/`JMP_4`); section-aware target resolution; reachable CFG recovery; forward/backward MLIR `cf.br` lowering | Indirect and far jumps; cross-section relocations; external targets; precise exceptions |
| Jcc | partial | All 16 flag conditions for direct near `rel8`/`rel32` (`JCC_1`/`JCC_4`); taken/fallthrough CFG edges; exhaustive CF/PF/ZF/SF/OF truth-table unit test | JCXZ/JECXZ/JRCXZ and LOOP family; cross-section relocations; branch hints; precise exceptions |
| LEA | partial | `LEA64r` and `LEA64_32r`; base/index/scale/displacement and RIP-relative address calculation; configurable PIE runtime bias; 32-bit destination zero-extension fixtures | 16-bit destination; address-size override; relocation-backed symbolic addresses; exhaustive SIB fixtures |
| PUSH/POP | partial | `PUSH64r` and `POP64r`; 64-bit guest RSP update and guest-memory transfer; exercised by default-GCC glibc `main` | Immediate/memory/16-bit forms; stack faults; isolated fixture coverage under the current call/ret harness |
| CALL | partial | `CALL64pcrel32` through ELF PLT plus `CALL64r`/`CALL64m` returning SysV host calls; six integer argument registers and RAX result; native/translated indirect register and memory fixtures | Guest address to translated-function lookup; far calls; exact caller-saved register clobbers; varargs/vector arguments; unwind/exceptions |
| SYSCALL | partial | Linux x86-64 number/argument ABI; raw RAX result; RCX/R11 clobbers; external `__sbt_syscall6` runtime boundary; deterministic sched_yield fixture and translated hello-world write/exit test | Other operating systems/ABIs; privileged entry-state MSRs; signal/restart semantics; syscall sandboxing |
| Legacy SSE CVT* | partial | All 44 decoded rr/rm forms of `CVTDQ2PD`, `CVTDQ2PS`, `CVTPD2DQ`, `CVTPD2PS`, `CVTPS2DQ`, `CVTPS2PD`, `CVTSD2SI`, `CVTSD2SS`, `CVTSI2SD`, `CVTSI2SS`, `CVTSS2SD`, `CVTSS2SI`, `CVTTPD2DQ`, `CVTTPS2DQ`, `CVTTSD2SI`, and `CVTTSS2SI`; 32/64-bit GPR forms; packed lane ordering; legacy XMM preservation/zeroing; nearest-even versus truncation; 44 native and translated fixtures | Dynamic MXCSR rounding modes and exception/status flags; NaN/out-of-range indefinite results; MMX/x87 aliases; all VEX/EVEX `VCVT*`, FP16/BF16/FP8 and unsigned conversion extensions |
| x87 arithmetic | partial | All 42 decoded forms of `FADD`, `FSUB`, `FSUBR`, `FMUL`, `FDIV`, `FDIVR`, their `*P` forms, and `FI*` integer-memory variants; ST0/ST(i), ST(i)/ST0, m32fp, m64fp, m16int and m32int operands; exact 80-bit results and logical stack pop; 42 native and translated fixtures | x87 control/status/tag words and physical TOP; dynamic rounding/precision control; exception flags, unmasked exceptions, NaN/denormal/infinity edge cases; other x87 instructions |
| ADDSS/ADDSD | partial | Legacy SSE register and memory source; low scalar update with upper XMM preservation | MXCSR rounding/status/exceptions, NaN/denormal edge cases, VEX/EVEX forms |
| SUBSS/SUBSD | partial | Legacy SSE register and memory source; low scalar update with upper XMM preservation | MXCSR rounding/status/exceptions, NaN/denormal edge cases, VEX/EVEX forms |
| MULSS/MULSD | partial | Legacy SSE register and memory source; low scalar update with upper XMM preservation | MXCSR rounding/status/exceptions, NaN/denormal edge cases, VEX/EVEX forms |
| DIVSS/DIVSD | partial | Legacy SSE register and memory source; low scalar update with upper XMM preservation | MXCSR rounding/status/exceptions, divide exceptional cases, VEX/EVEX forms |
| Other x86 mnemonics | unsupported | None | Implement one family end to end before adding dispatch |

The matrix describes implemented semantics, not every LLVM opcode whose name
matches a mnemonic. In particular, `CVT*` means the legacy SSE spellings listed
above, not the nearly 3000 LLVM opcodes containing `CVT` across MMX, VEX, EVEX,
AVX10, FP16, BF16, and FP8. EVEX/APX/NF/ND forms remain unsupported unless
separately listed and tested.
