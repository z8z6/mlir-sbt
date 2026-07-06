//
// Created by zzm on 2026/7/3
// Part of RVision
//
#include "IR/IR1Converter.h"

#include "IR/IR1.h"

#include "llvm/MC/MCInst.h"
#include "llvm/MC/MCInstrDesc.h"
#include <Target/X86/MCTargetDesc/X86BaseInfo.h>

#include "Target/X86Machine.h"

#include "llvm/MC/MCInstrInfo.h"

using namespace llvm;
using namespace z8;

ConversionContext::ConversionContext(const llvm::MCInst &MI) : MI(MI){}

IR1Context* BaseIR1Converter::Ctx = &IR1Context::Instance();

BaseIR1Converter::BaseIR1Converter() {
}

BaseIR1Converter::~BaseIR1Converter() {}

void z8::BaseIR1Converter::loadSrcOperand(ConversionContext& CC) {
  for (int i = 0; i < CC.MI.getNumOperands(); ++i) {
    const MCOperand &Op = CC.MI.getOperand(i);
    if (i < MID->getNumDefs()) continue;

    static auto loc = Ctx->Builder.getUnknownLoc();
    if (Op.isImm()) {
      auto imm = Ctx->Builder.getI32IntegerAttr(Op.getImm());
      auto ci = ir1::ConstIntOp::create(Ctx->Builder, loc, imm);
      CC.Src.push_back(ci);
    }
    if (Op.isReg()) {
      auto id = Ctx->Builder.getI32IntegerAttr(Op.getReg());
      auto ci = ir1::ConstIntOp::create(Ctx->Builder, loc, id);
      auto v = ir1::LoadRegOp::create(Ctx->Builder, loc, ci.getType(), ci);
      CC.Src.push_back(v);
    }
  }
}

void z8::BaseIR1Converter::storeDstOperand(ConversionContext& CC) {
  for (int i = 0; i < MID->getNumDefs(); ++i) {
    const MCOperand &Op = CC.MI.getOperand(i);
    if (!Op.isReg()) continue;

    static auto loc = Ctx->Builder.getUnknownLoc();
    auto id = Ctx->Builder.getI32IntegerAttr(Op.getReg());
    auto ci = ir1::ConstIntOp::create(Ctx->Builder, loc, id);
    if (i >= CC.Dst.size()) continue;
    ir1::StoreRegOp::create(Ctx->Builder, loc, CC.Dst[i], ci);
  }
}

void BaseIR1Converter::run(const MCInst& MI) {
  ConversionContext CC(MI);
  loadSrcOperand(CC);
  op(CC);
  storeDstOperand(CC);
}
