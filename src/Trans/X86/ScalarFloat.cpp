#include "Trans/X86/ScalarFloat.h"

#include "IR/IR1.h"
#include "IR/IR1Converter.h"
#include "IR/X86.h"

#include <cassert>

using namespace mlir;

namespace z8::x86::trans {

void translateScalarFloat(ConversionContext &context, unsigned width,
                          ScalarFloatKind kind) {
  assert((width == 32 || width == 64) && "scalar SSE width must be 32 or 64");
  assert(context.Src.size() == 2 || context.Src.size() == 6);
  auto *ir = BaseIR1Converter::Ctx;
  Location loc = context.getNameLoc();
  Value rhs = context.Src[1];
  if (context.Src.size() == 6)
    rhs = ir1::LoadOp::create(ir->Builder, loc, ir->iTy(width),
                              buildMemoryAddress(context, 1));
  auto kindAttr = ir->Builder.getI32IntegerAttr(static_cast<int32_t>(kind));
  auto widthAttr = ir->Builder.getI32IntegerAttr(width);
  auto result = x86ir::ScalarFOp::create(
      ir->Builder, loc, ir->iTy(128), context.Src[0], rhs, kindAttr, widthAttr);
  context.Dst.push_back(result);
}

} // namespace z8::x86::trans
