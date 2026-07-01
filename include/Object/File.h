//
// Created by zzm on 2026/6/29
// Part of RVision
//

#pragma once

#include "IR/IR0.h"
#include "IR/IR1.h"
#include "llvm/Object/ObjectFile.h"

namespace z8 {
class File {
public:
  std::string Name;
  llvm::object::OwningBinary<llvm::object::ObjectFile> OB;
  llvm::object::ObjectFile* O;
  BaseMachine* Machine;

  IR0Context IR0Ctx;
  IR1Context IR1Ctx;

  explicit File(std::string Name);
  void disas();
};
}

