#include "Trans/X86/Move.h"

#include "IR/IR1.h"
#include "IR/IR1Converter.h"

#include <cassert>

using namespace mlir;

namespace z8::x86::trans {
namespace {

Value normalizeInteger(ConversionContext &context, Value value,
                       unsigned width) {
  auto *ir = BaseIR1Converter::Ctx;
  Location loc = context.getNameLoc();
  Value zero = ir1::ConstIntOp::create(ir->Builder, loc, 0);
  return ir1::AddIOp::create(ir->Builder, loc, ir->iTy(width), value, zero);
}

} // namespace

void translateMoveRegisterDestination(ConversionContext &context,
                                      unsigned width) {
  assert(context.Src.size() == 1 || context.Src.size() == 5);
  auto *ir = BaseIR1Converter::Ctx;
  Value value = context.Src[0];
  if (context.Src.size() == 5)
    value = ir1::LoadOp::create(ir->Builder, context.getNameLoc(),
                                ir->iTy(width), buildMemoryAddress(context));
  context.Dst.push_back(normalizeInteger(context, value, width));
}

void translateMoveMemoryDestination(ConversionContext &context,
                                    unsigned width) {
  assert(context.Src.size() == 6);
  auto *ir = BaseIR1Converter::Ctx;
  Value address = buildMemoryAddress(context);
  Value value = normalizeInteger(context, context.Src[5], width);
  ir1::StoreOp::create(ir->Builder, context.getNameLoc(), value, address);
}

} // namespace z8::x86::trans
