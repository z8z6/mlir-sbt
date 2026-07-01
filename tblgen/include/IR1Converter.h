//
// Created by zzm on 2026/6/30
// Part of RVision
//

#pragma once

#include "llvm/TableGen/TableGenBackend.h"

namespace z8
{
class IR1Converter {
  const llvm::RecordKeeper &RK;

public:
  explicit IR1Converter(const llvm::RecordKeeper &RK) : RK(RK) {}
  void run(llvm::raw_ostream &OS) const;
};

bool EmitIR1(llvm::raw_ostream &OS, const llvm::RecordKeeper &RK);
}