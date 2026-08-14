#include "IR/IR0.h"
#include "IR/IR1.h"
#include "IR/IR1Converter.h"
#include "IR/X86.h"
#include "tblgen/X86IR1Converter.h"

#include "Target/X86/MCTargetDesc/X86MCTargetDesc.h"

using namespace mlir;
using namespace z8;

namespace {
Value buildLea(ConversionContext &context) {
  const llvm::MCInst &instruction = context.IR.Inst;
  if (instruction.getNumOperands() != 6 || !instruction.getOperand(0).isReg() ||
      !instruction.getOperand(1).isReg() ||
      !instruction.getOperand(2).isImm() ||
      !instruction.getOperand(3).isReg() ||
      !instruction.getOperand(4).isImm() ||
      !instruction.getOperand(5).isReg()) {
    BaseIR1Converter::Ctx->markConversionFailure();
    return {};
  }
  auto *ir = BaseIR1Converter::Ctx;
  Location loc = context.getNameLoc();
  auto valueForRegister = [&](unsigned reg) -> Value {
    if (reg == 0)
      return ir1::ConstIntOp::create(ir->Builder, loc, 0);
    if (reg == llvm::X86::RIP)
      return ir1::ConstIntOp::create(
          ir->Builder, loc,
          static_cast<int64_t>(context.IR.Addr + context.IR.Size +
                               context.IR.AddressBias));
    return x86ir::ReadRegOp::create(ir->Builder, loc, ir->getState(), reg);
  };
  if (instruction.getOperand(5).getReg() != 0) {
    ir->markConversionFailure();
    return {};
  }
  Value base = valueForRegister(instruction.getOperand(1).getReg());
  Value scale = ir1::ConstIntOp::create(ir->Builder, loc,
                                        instruction.getOperand(2).getImm());
  Value index = valueForRegister(instruction.getOperand(3).getReg());
  Value displacement = ir1::ConstIntOp::create(
      ir->Builder, loc, instruction.getOperand(4).getImm());
  Value scaled = ir1::MulIOp::create(ir->Builder, loc, ir->iTy(), index, scale);
  Value address =
      ir1::AddIOp::create(ir->Builder, loc, ir->iTy(), base, scaled);
  address =
      ir1::AddIOp::create(ir->Builder, loc, ir->iTy(), address, displacement);
  return address;
}
} // namespace

void X86_LEA64r_IR1Converter::loadSrcOperand(ConversionContext &) {}
void X86_LEA64_32r_IR1Converter::loadSrcOperand(ConversionContext &) {}

void X86_LEA64r_IR1Converter::op(ConversionContext &context) {
  Value address = buildLea(context);
  if (address)
    context.Dst.push_back(address);
}

void X86_LEA64_32r_IR1Converter::op(ConversionContext &context) {
  auto *ir = BaseIR1Converter::Ctx;
  Value address = buildLea(context);
  if (!address)
    return;
  context.Dst.push_back(ir1::CastIOp::create(ir->Builder, context.getNameLoc(),
                                             ir->iTy(32), address));
}
