//
// Created by zzm on 2026/7/3
// Part of RVision
//

#pragma once

#include <string>
#include <vector>

namespace llvm {
class MCInstrDesc;
class MCInst;
}

namespace mlir {
class Value;
}

namespace z8 {
class IR0;
class IR1Context;
struct ConversionContext {
  const IR0& IR;
  std::vector<mlir::Value> Src;
  std::vector<mlir::Value> Dst;

  ConversionContext(const IR0&);
};

class BaseIR1Converter {
public:
  const llvm::MCInstrDesc* MID;
  unsigned Opcode;

  BaseIR1Converter();
  virtual ~BaseIR1Converter();

  virtual std::string getName() const = 0;
  virtual void op(ConversionContext&) = 0;
  virtual void loadSrcOperand(ConversionContext&);
  virtual void storeDstOperand(ConversionContext&);
  void run(const IR0&);

  static IR1Context* Ctx;
};

void convertMCInst(const IR0& IR);
}
