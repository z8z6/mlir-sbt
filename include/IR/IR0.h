//
// Created by zzm on 2026/6/29
// Part of RVision
//

#pragma once

#include <llvm/MC/MCInst.h>
#include <llvm/Support/raw_ostream.h>

namespace llvm
{
class MCInstrDesc;
}

namespace z8 {
class BaseMachine;

class IR0 {
public:
  uint64_t Addr;
  llvm::MCInst Inst;

  IR0(llvm::MCInst inst, uint64_t addr);

  void print(BaseMachine* M, llvm::raw_ostream& out = llvm::outs()) const;
};
};
