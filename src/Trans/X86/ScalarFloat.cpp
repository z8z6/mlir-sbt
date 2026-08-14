#include "Trans/X86/ScalarFloat.h"
#include "IR/IR1.h"
#include "IR/IR1Converter.h"
#include "IR/X86.h"
#include "tblgen/X86IR1Converter.h"
#include <cassert>

using namespace mlir;
using namespace z8;
using namespace z8::x86::trans;

// ---- shared helpers ----
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

// ---- ADDSS converters ----
void X86_ADDSSrr_Int_IR1Converter::op(ConversionContext &context) {
  translateScalarFloat(context, 32, ScalarFloatKind::Add);
}
void X86_ADDSSrm_Int_IR1Converter::op(ConversionContext &context) {
  translateScalarFloat(context, 32, ScalarFloatKind::Add);
}

// ---- ADDSD converters ----
void X86_ADDSDrr_Int_IR1Converter::op(ConversionContext &context) {
  translateScalarFloat(context, 64, ScalarFloatKind::Add);
}
void X86_ADDSDrm_Int_IR1Converter::op(ConversionContext &context) {
  translateScalarFloat(context, 64, ScalarFloatKind::Add);
}

// ---- SUBSS converters ----
void X86_SUBSSrr_Int_IR1Converter::op(ConversionContext &context) {
  translateScalarFloat(context, 32, ScalarFloatKind::Sub);
}
void X86_SUBSSrm_Int_IR1Converter::op(ConversionContext &context) {
  translateScalarFloat(context, 32, ScalarFloatKind::Sub);
}

// ---- SUBSD converters ----
void X86_SUBSDrr_Int_IR1Converter::op(ConversionContext &context) {
  translateScalarFloat(context, 64, ScalarFloatKind::Sub);
}
void X86_SUBSDrm_Int_IR1Converter::op(ConversionContext &context) {
  translateScalarFloat(context, 64, ScalarFloatKind::Sub);
}

// ---- MULSS converters ----
void X86_MULSSrr_Int_IR1Converter::op(ConversionContext &context) {
  translateScalarFloat(context, 32, ScalarFloatKind::Mul);
}
void X86_MULSSrm_Int_IR1Converter::op(ConversionContext &context) {
  translateScalarFloat(context, 32, ScalarFloatKind::Mul);
}

// ---- MULSD converters ----
void X86_MULSDrr_Int_IR1Converter::op(ConversionContext &context) {
  translateScalarFloat(context, 64, ScalarFloatKind::Mul);
}
void X86_MULSDrm_Int_IR1Converter::op(ConversionContext &context) {
  translateScalarFloat(context, 64, ScalarFloatKind::Mul);
}

// ---- DIVSS converters ----
void X86_DIVSSrr_Int_IR1Converter::op(ConversionContext &context) {
  translateScalarFloat(context, 32, ScalarFloatKind::Div);
}
void X86_DIVSSrm_Int_IR1Converter::op(ConversionContext &context) {
  translateScalarFloat(context, 32, ScalarFloatKind::Div);
}

// ---- DIVSD converters ----
void X86_DIVSDrr_Int_IR1Converter::op(ConversionContext &context) {
  translateScalarFloat(context, 64, ScalarFloatKind::Div);
}
void X86_DIVSDrm_Int_IR1Converter::op(ConversionContext &context) {
  translateScalarFloat(context, 64, ScalarFloatKind::Div);
}
