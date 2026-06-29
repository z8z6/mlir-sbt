//
// Created by zzm on 2026/6/29
// Part of RVision
//

#pragma once

#include "IR/IR0.h"

#include "llvm/Object/ObjectFile.h"


namespace z8 {
class File {
public:
  std::string Name;
  llvm::object::OwningBinary<llvm::object::ObjectFile> OB;
  llvm::object::ObjectFile* O;
  BaseMachine* Machine;

  std::vector<IR0> IR0s;

  explicit File(std::string Name);
  void disas();
  void printIR0();
};
}

