#include "Trans/X86/Stack.h"
#include "IR/IR1.h"
#include "IR/IR1Converter.h"
#include "IR/X86.h"
#include "Target/X86/MCTargetDesc/X86MCTargetDesc.h"
#include "tblgen/X86IR1Converter.h"
#include <cassert>

using namespace mlir;
using namespace z8;

// ---- shared helpers ----
namespace {
Value stackPointer(Location loc) {
  auto *ir = BaseIR1Converter::Ctx;
  return x86ir::ReadRegOp::create(ir->Builder, loc, ir->getState(),
                                  llvm::X86::RSP);
}

Value stackOffset(Location loc, Value pointer, bool increment) {
  auto *ir = BaseIR1Converter::Ctx;
  Value eight = ir1::ConstIntOp::create(ir->Builder, loc, 8);
  if (increment)
    return ir1::AddIOp::create(ir->Builder, loc, ir->iTy(), pointer, eight);
  return ir1::SubIOp::create(ir->Builder, loc, ir->iTy(), pointer, eight);
}

void writeStackPointer(Location loc, Value pointer) {
  auto *ir = BaseIR1Converter::Ctx;
  x86ir::WriteRegOp::create(ir->Builder, loc, ir->getState(), pointer,
                            llvm::X86::RSP);
}
} // namespace

void z8::translatePush64Register(ConversionContext &context) {
  assert(context.Src.size() == 1);
  auto *ir = BaseIR1Converter::Ctx;
  Location loc = context.getNameLoc();
  Value next = stackOffset(loc, stackPointer(loc), false);
  ir1::StoreOp::create(ir->Builder, loc, context.Src.front(), next);
  writeStackPointer(loc, next);
}

void z8::translatePop64Register(ConversionContext &context) {
  assert(context.Src.empty());
  auto *ir = BaseIR1Converter::Ctx;
  Location loc = context.getNameLoc();
  Value current = stackPointer(loc);
  context.Dst.push_back(
      ir1::LoadOp::create(ir->Builder, loc, ir->iTy(), current));
  writeStackPointer(loc, stackOffset(loc, current, true));
}

// ---- PUSH converters ----
void X86_PUSH64r_IR1Converter::op(ConversionContext &context) {
  translatePush64Register(context);
}

// ---- POP converters ----
void X86_POP64r_IR1Converter::op(ConversionContext &context) {
  translatePop64Register(context);
}
