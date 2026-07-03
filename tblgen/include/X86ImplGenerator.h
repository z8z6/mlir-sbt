//
// Created by zzm on 2026/7/3
// Part of RVision
//

#pragma once

#include "llvm/TableGen/TableGenBackend.h"

namespace z8 {
class X86ImplGenerator {
  const llvm::RecordKeeper &RK;

public:
  explicit X86ImplGenerator(const llvm::RecordKeeper &RK) : RK(RK) {}
  void run(llvm::raw_ostream &OS) const;
};

bool EmitX86Impl(llvm::raw_ostream &OS, const llvm::RecordKeeper &RK);
}