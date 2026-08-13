#include "Trans/X86/Arithmetic.h"

#include "IR/IR1.h"
#include "IR/IR1Converter.h"
#include "IR/X86.h"
#include "Target/X86/MCTargetDesc/X86MCTargetDesc.h"

#include "llvm/Support/ErrorHandling.h"

#include <cassert>

using namespace mlir;

namespace z8::x86::trans {
namespace {

struct ArithmeticResult {
  Value value;
  Value flags;
};

ArithmeticResult buildOperation(ConversionContext &context, unsigned width,
                                BinaryArithmeticKind kind, Value lhs,
                                Value rhs) {
  auto *ir = BaseIR1Converter::Ctx;
  TypeRange resultTypes{ir->iTy(width), ir->iTy()};
  Location loc = context.getNameLoc();
  switch (kind) {
  case BinaryArithmeticKind::Add: {
    auto op = x86ir::AddIOp::create(ir->Builder, loc, resultTypes, lhs, rhs);
    return {op.getRes(), op.getFlags()};
  }
  case BinaryArithmeticKind::Sub: {
    auto op = x86ir::SubIOp::create(ir->Builder, loc, resultTypes, lhs, rhs);
    return {op.getRes(), op.getFlags()};
  }
  }
  llvm_unreachable("unknown binary arithmetic kind");
}

void recordFlags(ConversionContext &context, Value flags) {
  context.ImplicitDst.push_back(flags);
  context.ImplicitOperand.emplace_back(llvm::X86::RFLAGS);
}

} // namespace

void translateBinaryRegisterDestination(ConversionContext &context,
                                        unsigned width,
                                        BinaryArithmeticKind kind) {
  assert(context.Src.size() == 2 || context.Src.size() == 6);
  auto *ir = BaseIR1Converter::Ctx;
  Value rhs = context.Src[1];
  if (context.Src.size() == 6)
    rhs = ir1::LoadOp::create(ir->Builder, context.getNameLoc(), ir->iTy(width),
                              buildMemoryAddress(context, 1));
  ArithmeticResult result =
      buildOperation(context, width, kind, context.Src[0], rhs);
  context.Dst.push_back(result.value);
  recordFlags(context, result.flags);
}

void translateBinaryMemoryDestination(ConversionContext &context,
                                      unsigned width,
                                      BinaryArithmeticKind kind) {
  assert(context.Src.size() == 6);
  auto *ir = BaseIR1Converter::Ctx;
  Location loc = context.getNameLoc();
  Value address = buildMemoryAddress(context);
  Value lhs = ir1::LoadOp::create(ir->Builder, loc, ir->iTy(width), address);
  ArithmeticResult result =
      buildOperation(context, width, kind, lhs, context.Src[5]);
  ir1::StoreOp::create(ir->Builder, loc, result.value, address);
  recordFlags(context, result.flags);
}

} // namespace z8::x86::trans
