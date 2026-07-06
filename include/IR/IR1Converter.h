//
// Created by zzm on 2026/7/3
// Part of RVision
//

#pragma once

#include <vector>

namespace llvm {
class MCInstrDesc;
class MCInst;
}

namespace mlir {
class Value;
}

namespace z8 {
class IR1Context;
struct ConversionContext {
  const llvm::MCInst& MI;
  std::vector<mlir::Value> Src;
  std::vector<mlir::Value> Dst;

  ConversionContext(const llvm::MCInst&);
};
class BaseIR1Converter {
public:
  const llvm::MCInstrDesc* MID;
  unsigned Opcode;

  BaseIR1Converter();
  virtual ~BaseIR1Converter();

  virtual void op(ConversionContext&) = 0;
  virtual void loadSrcOperand(ConversionContext&);
  virtual void storeDstOperand(ConversionContext&);
  void run(const llvm::MCInst&);

  static IR1Context* Ctx;
};

void convertMCInst(const llvm::MCInst& MI);
}
