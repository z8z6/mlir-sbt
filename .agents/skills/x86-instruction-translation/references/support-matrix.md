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
| AND | partial | GPR I/MI/MR/RM for 8/16/32/64-bit widths; logical flags; register aliases and memory | AF is architecturally undefined and normalized to 0 internally; LOCK atomicity; exhaustive fixtures |
| OR | partial | GPR I/MI/MR/RM for 8/16/32/64-bit widths; logical flags; register aliases and memory | AF is architecturally undefined and normalized to 0 internally; LOCK atomicity; exhaustive fixtures |
| XOR | partial | GPR I/MI/MR/RM for 8/16/32/64-bit widths; logical flags; register aliases and memory | AF is architecturally undefined and normalized to 0 internally; LOCK atomicity; exhaustive fixtures |
| ADDSS/ADDSD | partial | Legacy SSE register and memory source; low scalar update with upper XMM preservation | MXCSR rounding/status/exceptions, NaN/denormal edge cases, VEX/EVEX forms |
| SUBSS/SUBSD | partial | Legacy SSE register and memory source; low scalar update with upper XMM preservation | MXCSR rounding/status/exceptions, NaN/denormal edge cases, VEX/EVEX forms |
| MULSS/MULSD | partial | Legacy SSE register and memory source; low scalar update with upper XMM preservation | MXCSR rounding/status/exceptions, NaN/denormal edge cases, VEX/EVEX forms |
| DIVSS/DIVSD | partial | Legacy SSE register and memory source; low scalar update with upper XMM preservation | MXCSR rounding/status/exceptions, divide exceptional cases, VEX/EVEX forms |
| Other x86 mnemonics | unsupported | None | Implement one family end to end before adding dispatch |

The matrix describes implemented semantics, not every LLVM opcode whose name
matches a mnemonic. EVEX/APX/NF/ND forms remain unsupported unless separately
listed and tested.
