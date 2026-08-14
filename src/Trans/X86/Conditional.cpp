#include "IR/IR0.h"
#include "IR/IR1.h"
#include "IR/X86.h"
#include "Target/X86/MCTargetDesc/X86MCTargetDesc.h"
#include "Trans/X86/Common.h"
#include "mlir/Dialect/Arith/IR/Arith.h"
#include "tblgen/X86IR1Converter.h"
#include <cassert>

using namespace mlir;
using namespace z8;

// ---- CMOVcc converters ----
namespace {
void translateCMov(ConversionContext &context, unsigned width,
                   bool memorySource) {
  assert(context.Src.size() == (memorySource ? 7 : 3));
  auto *ir = BaseIR1Converter::Ctx;
  Location loc = context.getNameLoc();
  Value source = context.Src[1];
  if (memorySource)
    source = ir1::LoadOp::create(ir->Builder, loc, ir->iTy(width),
                                 x86::trans::buildMemoryAddress(context, 1));
  Value flags = x86ir::ReadRegOp::create(ir->Builder, loc, ir->getState(),
                                         llvm::X86::RFLAGS);
  Value take = x86ir::ConditionOp::create(
      ir->Builder, loc, ir->Builder.getI1Type(), flags,
      ir->Builder.getI32IntegerAttr(static_cast<int32_t>(
          context.IR.Inst.getOperand(context.IR.Inst.getNumOperands() - 1)
              .getImm())));
  context.Dst.push_back(
      arith::SelectOp::create(ir->Builder, loc, take, source, context.Src[0]));
}
} // namespace

#define CMOV_REGISTER(WIDTH)                                                   \
  void X86_CMOV##WIDTH##rr_IR1Converter::op(ConversionContext &context) {      \
    translateCMov(context, WIDTH, false);                                      \
  }

#define CMOV_MEMORY(WIDTH)                                                     \
  void X86_CMOV##WIDTH##rm_IR1Converter::op(ConversionContext &context) {      \
    translateCMov(context, WIDTH, true);                                       \
  }

CMOV_REGISTER(16)
CMOV_REGISTER(32)
CMOV_REGISTER(64)
CMOV_MEMORY(16)
CMOV_MEMORY(32)
CMOV_MEMORY(64)

#undef CMOV_MEMORY
#undef CMOV_REGISTER

// ---- SETcc converters ----
namespace {
Value condition(ConversionContext &context, int64_t code) {
  auto *ir = BaseIR1Converter::Ctx;
  Location loc = context.getNameLoc();
  Value flags = x86ir::ReadRegOp::create(ir->Builder, loc, ir->getState(),
                                         llvm::X86::RFLAGS);
  Value bit = x86ir::ConditionOp::create(
      ir->Builder, loc, ir->Builder.getI1Type(), flags,
      ir->Builder.getI32IntegerAttr(static_cast<int32_t>(code)));
  return ir1::CastIOp::create(ir->Builder, loc, ir->iTy(8), bit);
}
} // namespace

void X86_SETCCr_IR1Converter::op(ConversionContext &context) {
  assert(context.Src.size() == 1 && context.IR.Inst.getOperand(1).isImm());
  context.Dst.push_back(
      condition(context, context.IR.Inst.getOperand(1).getImm()));
}

void X86_SETCCm_IR1Converter::op(ConversionContext &context) {
  assert(context.Src.size() == 6 && context.IR.Inst.getOperand(5).isImm());
  auto *ir = BaseIR1Converter::Ctx;
  ir1::StoreOp::create(
      ir->Builder, context.getNameLoc(),
      condition(context, context.IR.Inst.getOperand(5).getImm()),
      x86::trans::buildMemoryAddress(context));
}
