//
// Created by zzm on 2026/7/3
// Part of RVision
//
#include "IR/IR1Converter.h"
#include "IR/IR1.h"
#include <IR/IR0.h>
#include "llvm/MC/MCInst.h"
#include "llvm/MC/MCInstrDesc.h"
#include "llvm/Support/Debug.h"

using namespace llvm;
using namespace mlir;
using namespace z8;

ConversionContext::ConversionContext(const IR0 &IR) : IR(IR){}

NameLoc ConversionContext::getNameLoc() const {
  static IR1Context* Ctx = &IR1Context::Instance();
  auto name = StringAttr::get(&Ctx->Ctx, IR.str());
  return NameLoc::get(name);
}

IR1Context* BaseIR1Converter::Ctx = &IR1Context::Instance();

BaseIR1Converter::BaseIR1Converter() {
}

BaseIR1Converter::~BaseIR1Converter() {}

void BaseIR1Converter::op(ConversionContext &) {
  dbgs() << getName() << " converter is not impl yet!\n";
}

void z8::BaseIR1Converter::loadSrcOperand(ConversionContext& CC) {
  for (int i = 0; i < CC.IR.Inst.getNumOperands(); ++i) {
    const MCOperand &Op = CC.IR.Inst.getOperand(i);
    if (i < MID->getNumDefs()) continue;

    auto loc = CC.getNameLoc();
    if (Op.isImm()) {
      auto ci = ir1::ConstIntOp::create(Ctx->Builder, loc, Op.getImm());
      CC.Src.push_back(ci);
    }
    if (Op.isReg()) {
      auto v = ir1::LoadRegOp::create(Ctx->Builder, loc, Op.getReg());
      CC.Src.push_back(v);
    }
  }
}

void z8::BaseIR1Converter::storeDstOperand(ConversionContext& CC) {
  for (int i = 0; i < MID->getNumDefs(); ++i) {
    const MCOperand &Op = CC.IR.Inst.getOperand(i);
    if (!Op.isReg()) continue;

    auto loc = CC.getNameLoc();
    if (i >= CC.Dst.size()) continue;
    ir1::StoreRegOp::create(Ctx->Builder, loc, CC.Dst[i], Op.getReg());
  }
}

void BaseIR1Converter::run(const IR0& IR) {
  ConversionContext CC(IR);
  loadSrcOperand(CC);
  op(CC);
  storeDstOperand(CC);
}
