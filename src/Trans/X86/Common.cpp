#include "Trans/X86/Common.h"

#include "IR/IR0.h"
#include "IR/IR1.h"
#include "IR/IR1Converter.h"
#include "IR/X86.h"
#include "Target/X86/MCTargetDesc/X86MCTargetDesc.h"

#include <cassert>

using namespace mlir;

namespace z8::x86::trans {

Value buildMemoryAddress(ConversionContext &context, size_t operandOffset) {
  assert(context.Src.size() >= operandOffset + 5);
  auto *ir = BaseIR1Converter::Ctx;
  Location loc = context.getNameLoc();
  auto i64 = ir->iTy();
  Value scaledIndex =
      ir1::MulIOp::create(ir->Builder, loc, i64, context.Src[operandOffset + 1],
                          context.Src[operandOffset + 2]);
  Value address = ir1::AddIOp::create(ir->Builder, loc, i64,
                                      context.Src[operandOffset], scaledIndex);
  address = ir1::AddIOp::create(ir->Builder, loc, i64,
                                context.Src[operandOffset + 3], address);
  return ir1::AddIOp::create(ir->Builder, loc, i64,
                             context.Src[operandOffset + 4], address);
}

void loadAccumulatorImmediate(ConversionContext &context, unsigned reg) {
  assert(context.IR.Inst.getNumOperands() == 1);
  assert(context.IR.Inst.getOperand(0).isImm());
  auto *ir = BaseIR1Converter::Ctx;
  Location loc = context.getNameLoc();
  Value accumulator =
      x86ir::ReadRegOp::create(ir->Builder, loc, ir->getState(), reg);
  Value immediate = ir1::ConstIntOp::create(
      ir->Builder, loc, context.IR.Inst.getOperand(0).getImm());
  context.Src = {accumulator, immediate};
}

void storeAccumulatorResult(ConversionContext &context, unsigned reg) {
  assert(context.Dst.size() == 1 && context.ImplicitDst.size() == 1);
  auto *ir = BaseIR1Converter::Ctx;
  Location loc = context.getNameLoc();
  x86ir::WriteRegOp::create(ir->Builder, loc, ir->getState(), context.Dst[0],
                            reg);
  x86ir::WriteRegOp::create(ir->Builder, loc, ir->getState(),
                            context.ImplicitDst[0], llvm::X86::RFLAGS);
}

} // namespace z8::x86::trans
