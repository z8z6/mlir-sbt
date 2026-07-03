//
// Created by zzm on 2026/7/3
// Part of RVision
//

#pragma once

namespace llvm {
class MCInstrDesc;
class MCInst;
}

namespace z8 {
class BaseIR1Converter {
public:
  const llvm::MCInstrDesc* MID;
  unsigned Opcode;

  BaseIR1Converter();
  virtual ~BaseIR1Converter();

  virtual void op(const llvm::MCInst&) = 0;
  virtual void loadSrcOperand(const llvm::MCInst&);
  virtual void storeDstOperand(const llvm::MCInst&);
  void run(const llvm::MCInst&);
};

void convertMCInst(const llvm::MCInst& MI);
}
