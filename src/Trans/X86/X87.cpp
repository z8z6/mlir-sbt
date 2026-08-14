#include "Trans/X86/X87.h"
#include "IR/IR0.h"
#include "IR/IR1.h"
#include "IR/X86.h"
#include "Target/X86/MCTargetDesc/X86MCTargetDesc.h"
#include "Trans/X86/Common.h"
#include "tblgen/X86IR1Converter.h"
#include <cassert>
#include <optional>

using namespace mlir;
using namespace z8;
using namespace z8::x86::trans;

// ---- shared helpers ----
namespace z8::x86::trans {

static std::optional<unsigned> x87Index(unsigned reg) {
  switch (reg) {
  case llvm::X86::ST0:
    return 0;
  case llvm::X86::ST1:
    return 1;
  case llvm::X86::ST2:
    return 2;
  case llvm::X86::ST3:
    return 3;
  case llvm::X86::ST4:
    return 4;
  case llvm::X86::ST5:
    return 5;
  case llvm::X86::ST6:
    return 6;
  case llvm::X86::ST7:
    return 7;
  default:
    return std::nullopt;
  }
}

static Value readX87(Location loc, unsigned index) {
  auto *ir = BaseIR1Converter::Ctx;
  return x86ir::ReadX87Op::create(
      ir->Builder, loc, ir->iTy(80), ir->getState(),
      ir->Builder.getI32IntegerAttr(static_cast<int32_t>(index)));
}

void loadX87Operands(ConversionContext &context) {
  auto *ir = BaseIR1Converter::Ctx;
  Location loc = context.getNameLoc();
  for (const llvm::MCOperand &operand : context.IR.Inst) {
    if (operand.isImm()) {
      context.Src.push_back(
          ir1::ConstIntOp::create(ir->Builder, loc, operand.getImm()));
      continue;
    }
    if (!operand.isReg())
      continue;
    if (auto index = x87Index(operand.getReg())) {
      context.Src.push_back(readX87(loc, *index));
    } else if (operand.getReg() == 0) {
      context.Src.push_back(ir1::ConstIntOp::create(ir->Builder, loc, 0));
    } else {
      context.Src.push_back(x86ir::ReadRegOp::create(
          ir->Builder, loc, ir->getState(), operand.getReg()));
    }
  }
}

static Value loadMemoryOperand(ConversionContext &context,
                               const X87BinarySpec &spec) {
  assert(spec.memoryWidth == 16 || spec.memoryWidth == 32 ||
         spec.memoryWidth == 64);
  auto *ir = BaseIR1Converter::Ctx;
  Location loc = context.getNameLoc();
  Value bits = ir1::LoadOp::create(ir->Builder, loc, ir->iTy(spec.memoryWidth),
                                   buildMemoryAddress(context));
  Value fp;
  if (spec.integerMemory) {
    fp =
        ir1::SIToFPOp::create(ir->Builder, loc, ir->Builder.getF80Type(), bits);
  } else {
    Type sourceType = spec.memoryWidth == 32 ? Type(ir->Builder.getF32Type())
                                             : Type(ir->Builder.getF64Type());
    fp = ir1::BitcastOp::create(ir->Builder, loc, sourceType, bits);
    fp = ir1::ExtFOp::create(ir->Builder, loc, ir->Builder.getF80Type(), fp);
  }
  return ir1::BitcastOp::create(ir->Builder, loc, ir->iTy(80), fp);
}

void translateX87Binary(ConversionContext &context, const X87BinarySpec &spec) {
  auto *ir = BaseIR1Converter::Ctx;
  Location loc = context.getNameLoc();
  Value st0 = readX87(loc, 0);
  unsigned destinationIndex = 0;
  Value other;
  if (spec.memoryWidth) {
    other = loadMemoryOperand(context, spec);
  } else {
    assert(context.IR.Inst.getNumOperands() == 1 && context.Src.size() == 1);
    auto index = x87Index(context.IR.Inst.getOperand(0).getReg());
    assert(index && "x87 register form must name ST(i)");
    destinationIndex = spec.destinationIsSTi ? *index : 0;
    other = spec.destinationIsSTi ? st0 : context.Src[0];
  }
  Value destination = destinationIndex == 0 ? st0 : context.Src[0];
  Value lhs = spec.reverse ? other : destination;
  Value rhs = spec.reverse ? destination : other;
  Value result = x86ir::X87BinaryOp::create(
      ir->Builder, loc, ir->iTy(80), lhs, rhs,
      ir->Builder.getI32IntegerAttr(static_cast<int32_t>(spec.kind)));
  x86ir::WriteX87Op::create(
      ir->Builder, loc, ir->getState(), result,
      ir->Builder.getI32IntegerAttr(static_cast<int32_t>(destinationIndex)));
  if (spec.pop)
    x86ir::PopX87Op::create(ir->Builder, loc, ir->getState());
}

} // namespace z8::x86::trans

// ---- x87 arithmetic converters ----
#define X87_IMPL(OPCODE, KIND, WIDTH, INTEGER, DEST_STI, REVERSE, POP)         \
  void X86_##OPCODE##_IR1Converter::loadSrcOperand(ConversionContext &ctx) {   \
    loadX87Operands(ctx);                                                      \
  }                                                                            \
  void X86_##OPCODE##_IR1Converter::op(ConversionContext &ctx) {               \
    translateX87Binary(                                                        \
        ctx, {X87BinaryKind::KIND, WIDTH, INTEGER, DEST_STI, REVERSE, POP});   \
  }

#define X87_FAMILY(NAME, KIND)                                                 \
  X87_IMPL(NAME##_FST0r, KIND, 0, false, false, false, false)                  \
  X87_IMPL(NAME##_FrST0, KIND, 0, false, true, false, false)                   \
  X87_IMPL(NAME##_FPrST0, KIND, 0, false, true, false, true)                   \
  X87_IMPL(NAME##_F32m, KIND, 32, false, false, false, false)                  \
  X87_IMPL(NAME##_F64m, KIND, 64, false, false, false, false)                  \
  X87_IMPL(NAME##_FI16m, KIND, 16, true, false, false, false)                  \
  X87_IMPL(NAME##_FI32m, KIND, 32, true, false, false, false)

X87_FAMILY(ADD, Add)
X87_FAMILY(MUL, Mul)

#define X87_ORDERED_FAMILY(NAME, KIND, REVERSE)                                \
  X87_IMPL(NAME##_FST0r, KIND, 0, false, false, REVERSE, false)                \
  X87_IMPL(NAME##_FrST0, KIND, 0, false, true, REVERSE, false)                 \
  X87_IMPL(NAME##_FPrST0, KIND, 0, false, true, REVERSE, true)                 \
  X87_IMPL(NAME##_F32m, KIND, 32, false, false, REVERSE, false)                \
  X87_IMPL(NAME##_F64m, KIND, 64, false, false, REVERSE, false)                \
  X87_IMPL(NAME##_FI16m, KIND, 16, true, false, REVERSE, false)                \
  X87_IMPL(NAME##_FI32m, KIND, 32, true, false, REVERSE, false)

X87_ORDERED_FAMILY(SUB, Sub, false)
X87_ORDERED_FAMILY(SUBR, Sub, true)
X87_ORDERED_FAMILY(DIV, Div, false)
X87_ORDERED_FAMILY(DIVR, Div, true)

#undef X87_ORDERED_FAMILY
#undef X87_FAMILY
#undef X87_IMPL
