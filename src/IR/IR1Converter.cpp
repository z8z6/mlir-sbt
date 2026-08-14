//
// Created by zzm on 2026/7/3
// Part of RVision
//
#include "IR/IR1Converter.h"
#include "IR/IR1.h"
#include "IR/X86.h"
#include "Target/X86Register.h"
#include "llvm/MC/MCInst.h"
#include "llvm/MC/MCInstrDesc.h"
#include <IR/IR0.h>

using namespace llvm;
using namespace mlir;
using namespace z8;

ConversionContext::ConversionContext(const IR0 &IR, Block *branchTarget,
                                     Block *fallthrough)
    : IR(IR), BranchTarget(branchTarget), Fallthrough(fallthrough) {}

NameLoc ConversionContext::getNameLoc() const {
  MLIRContext *context = &BaseIR1Converter::Ctx->Ctx;
  NameLoc instruction = NameLoc::get(StringAttr::get(context, IR.str()));
  if (IR.FunctionName.empty())
    return instruction;
  return NameLoc::get(StringAttr::get(context, IR.FunctionName), instruction);
}

IR1Context *BaseIR1Converter::Ctx = &IR1Context::Instance();

BaseIR1Converter::BaseIR1Converter() {}

BaseIR1Converter::~BaseIR1Converter() {}

void BaseIR1Converter::op(ConversionContext &) {
  Ctx->markConversionFailure();
}

void z8::BaseIR1Converter::loadSrcOperand(ConversionContext &CC) {
  for (int i = 0; i < CC.IR.Inst.getNumOperands(); ++i) {
    const MCOperand &Op = CC.IR.Inst.getOperand(i);
    if (i < MID->getNumDefs())
      continue;

    auto loc = CC.getNameLoc();
    if (Op.isImm()) {
      auto ci = ir1::ConstIntOp::create(Ctx->Builder, loc, Op.getImm());
      CC.Src.push_back(ci);
    }
    if (Op.isReg()) {
      if (Op.getReg() == 0) {
        auto zero = ir1::ConstIntOp::create(Ctx->Builder, loc, 0);
        CC.Src.push_back(zero);
      } else {
        if (!getX86RegisterDesc(Op.getReg())) {
          Ctx->markConversionFailure();
          return;
        }
        auto v = x86ir::ReadRegOp::create(Ctx->Builder, loc, Ctx->getState(),
                                          Op.getReg());
        CC.Src.push_back(v);
      }
    }
  }
}

void z8::BaseIR1Converter::storeDstOperand(ConversionContext &CC) {
  auto loc = CC.getNameLoc();
  for (unsigned i = 0; i < MID->getNumDefs(); ++i) {
    const MCOperand &Op = CC.IR.Inst.getOperand(i);
    if (!Op.isReg())
      continue;
    if (!getX86RegisterDesc(Op.getReg())) {
      Ctx->markConversionFailure();
      return;
    }
    assert(i < CC.Dst.size());
    x86ir::WriteRegOp::create(Ctx->Builder, loc, Ctx->getState(), CC.Dst[i],
                              Op.getReg());
  }
  for (unsigned i = 0; i < CC.ImplicitOperand.size(); ++i) {
    auto reg = CC.ImplicitOperand[i];
    x86ir::WriteRegOp::create(Ctx->Builder, loc, Ctx->getState(),
                              CC.ImplicitDst[i], reg);
  }
}

void BaseIR1Converter::run(const IR0 &IR, Block *branchTarget,
                           Block *fallthrough) {
  ConversionContext CC(IR, branchTarget, fallthrough);
  loadSrcOperand(CC);
  if (Ctx->hasConversionFailed())
    return;
  op(CC);
  if (Ctx->hasConversionFailed())
    return;
  storeDstOperand(CC);
}
