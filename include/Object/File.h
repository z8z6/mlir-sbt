//
// Created by zzm on 2026/6/29
// Part of RVision
//

#pragma once

#include "IR/IR0.h"
#include "IR/IR1.h"
#include "llvm/Object/ObjectFile.h"

#include <unordered_map>
#include <vector>

namespace z8 {
struct FunctionInfo {
  std::string Name;
  std::vector<std::string> Aliases;
  uint64_t Address;
  uint64_t Size;
  uint64_t SectionIndex;

  bool matches(llvm::StringRef name) const;
};

struct FunctionDiscoveryStats {
  uint64_t TextBytes = 0;
  uint64_t FunctionBytes = 0;
};

class File {
public:
  std::string Name;
  llvm::object::OwningBinary<llvm::object::ObjectFile> OB;
  llvm::object::ObjectFile *O;
  BaseMachine *Machine;

  IR0Context IR0Ctx;
  std::vector<FunctionInfo> Functions;
  std::unordered_map<uint64_t, std::string> PLTSymbols;

  explicit File(std::string Name);
  bool disas(llvm::StringRef functionName = {}, uint64_t addressBias = 0);
  uint64_t textSize() const;
  FunctionDiscoveryStats functionDiscoveryStats() const;

  inline static File *CurrentFile = nullptr;
};
} // namespace z8
