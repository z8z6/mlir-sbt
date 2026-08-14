//
// Created by zzm on 2026/6/29
// Part of RVision
//

#pragma once

#include <llvm/ADT/StringRef.h>
#include <llvm/MC/MCInst.h>
#include <llvm/Support/raw_ostream.h>

#include <optional>
#include <string>
#include <vector>

namespace llvm {
class MCInstrDesc;
}

namespace z8 {
class BaseMachine;

class IR0 {
public:
  uint64_t Addr;
  uint64_t Size;
  uint64_t SectionIndex;
  uint64_t AddressBias = 0;
  std::optional<std::string> ExternalSymbol;
  std::string FunctionName;
  std::string DebugText;
  llvm::MCInst Inst;

  IR0(llvm::MCInst inst, uint64_t addr, uint64_t size, uint64_t sectionIndex);

  void print(llvm::raw_ostream &out = llvm::outs()) const;
  std::string str() const;
};

enum class IR0CFGEdgeKind {
  Branch,
  Fallthrough,
};

struct IR0CFGEdge {
  IR0CFGEdgeKind Kind;
  uint64_t TargetAddress;
  std::optional<size_t> TargetBlock;
};

class IR0BasicBlock {
public:
  uint64_t Address = 0;
  size_t BeginIndex = 0;
  size_t EndIndex = 0;
  bool Reachable = false;
  std::vector<IR0CFGEdge> Successors;
};

class IR0CFG {
public:
  std::vector<IR0BasicBlock> Blocks;

  bool build(const std::vector<IR0> &instructions);
  std::optional<size_t> findBlock(uint64_t address) const;
};

class IR0Function {
public:
  std::string Name;
  std::vector<std::string> Aliases;
  uint64_t Address = 0;
  uint64_t Size = 0;
  uint64_t SectionIndex = 0;
  std::vector<IR0> IRs;
  IR0CFG CFG;

  bool buildCFG();
  bool matches(llvm::StringRef name) const;
  void print(llvm::raw_ostream &out = llvm::outs()) const;
};

class IR0Context {
public:
  std::vector<IR0Function> Functions;
  size_t EntryFunctionIndex = 0;

  bool empty() const { return Functions.empty(); }
  uint64_t translatedCodeSize() const;
  void printFunctionNames(llvm::raw_ostream &out = llvm::outs()) const;
  void print() const;
};
}; // namespace z8
