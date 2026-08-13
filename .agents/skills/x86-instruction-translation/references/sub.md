# SUB implementation record

Reference: [Felix Cloutier, SUB — Subtract](https://www.felixcloutier.com/x86/sub)

## Architecture contract

`DEST := DEST - SRC`. The destination is a register or memory; the source is a
register, immediate, or memory, but both operands cannot be memory. In 64-bit
mode, imm8 and imm32 forms are sign-extended to the destination width.

SUB defines CF, PF, AF, ZF, SF, OF and preserves the other modeled arithmetic
state. For width `w`, masked operands `lhs`, `rhs`, and result `r`:

```text
r  = (lhs - rhs) mod 2^w
CF = lhs <u rhs
PF = even_parity(r[7:0])
AF = bit4(lhs xor rhs xor r)
ZF = r == 0
SF = r[w-1]
OF = bit[w-1]((lhs xor rhs) and (lhs xor r))
```

The page permits LOCK only when the destination is memory; LOCK on a register
destination raises #UD. Memory accesses can raise the normal segment,
canonical-address, page-fault, and alignment exceptions.

## mlir-sbt implementation

- Semantic IR: `ir1.x86_subi(lhs, rhs) -> (result, flags)`.
- Lowering uses `arith.subi`; CF is unsigned borrow, and OF uses the subtraction
  formula above. `StoreRegOpLowering` merges the six defined flags with the old
  RFLAGS value.
- Accumulator encodings explicitly synthesize AL/AX/EAX/RAX because LLVM MC's
  accumulator-immediate opcodes expose only the immediate operand.
- RM reads memory, MR/MI write memory, and all use the x86
  `base + scale*index + displacement + segment` operand layout.
- Narrow memory writes preserve surrounding bytes; 32-bit GPR destinations
  zero extend, while 8/16-bit and AH writes preserve unrelated bits.

Implemented fixture categories: I, MI, MR, RM; 8/16/32/64-bit widths; full and
sign-extended immediates; AL/AX/EAX/RAX encodings; high byte; SIB addressing;
borrow and signed overflow boundaries.

LOCK fixtures currently validate only single-threaded result/flags. The
translation does not yet prove atomicity, so LOCK is not marked fully supported.
