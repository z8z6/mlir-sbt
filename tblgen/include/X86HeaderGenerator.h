//
// Created by zzm on 2026/6/30
// Part of RVision
//

#pragma once

#include "llvm/TableGen/TableGenBackend.h"

namespace z8
{
class X86HeaderGenerator {
  const llvm::RecordKeeper &RK;

public:
  explicit X86HeaderGenerator(const llvm::RecordKeeper &RK) : RK(RK) {}
  void run(llvm::raw_ostream &OS) const;
};

bool EmitX86Header(llvm::raw_ostream &OS, const llvm::RecordKeeper &RK);
}