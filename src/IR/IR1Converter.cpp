//
// Created by zzm on 2026/7/3
// Part of RVision
//
#include "IR/IR1Converter.h"
#include "llvm/MC/MCInst.h"
#include "llvm/MC/MCInstrDesc.h"
#include <Target/X86/MCTargetDesc/X86BaseInfo.h>

#include "Target/X86Machine.h"

#include "llvm/MC/MCInstrInfo.h"

using namespace llvm;
using namespace z8;

BaseIR1Converter::BaseIR1Converter() {}

BaseIR1Converter::~BaseIR1Converter() {}

void z8::BaseIR1Converter::loadSrcOperand(const MCInst& MI) {

}

void z8::BaseIR1Converter::storeDstOperand(const MCInst& MI) {

}

void BaseIR1Converter::run(const MCInst& MI) {
  loadSrcOperand(MI);
  op(MI);
  storeDstOperand(MI);
}
