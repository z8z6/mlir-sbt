//
// Created by zzm on 2026/6/29
// Part of RVision
//

#include "IR/IR0.h"

#include "Object/File.h"

#include "Target/BaseMachine.h"
#include "Target/X86/MCTargetDesc/X86MCTargetDesc.h"
#include "llvm/ADT/DenseMap.h"
#include "llvm/ADT/STLExtras.h"
#include "llvm/ADT/SmallVector.h"
#include "llvm/MC/MCInstPrinter.h"
#include "llvm/Support/Format.h"

#include <algorithm>

using namespace llvm;
using namespace z8;

IR0::IR0(llvm::MCInst inst, uint64_t addr, uint64_t size, uint64_t sectionIndex)
    : Addr(addr), Size(size), SectionIndex(sectionIndex),
      Inst(std::move(inst)) {}

void IR0::print(raw_ostream &out) const {
  auto *M = File::CurrentFile->Machine;
  out << format_hex(Addr, 10) << ": ";
  M->getIP().printInst(&Inst, Addr, "", M->getSTI(), out);
  out << "\n";
}

std::string IR0::str() const {
  if (!DebugText.empty())
    return DebugText;
  std::string s;
  raw_string_ostream so(s);
  auto *M = File::CurrentFile->Machine;
  M->getIP().printInst(&Inst, Addr, "", M->getSTI(), so);
  so.flush();
  std::replace(s.begin(), s.end(), '\t', ' ');
  return s;
}

namespace {
bool isDirectJump(unsigned opcode) {
  return opcode == llvm::X86::JMP_1 || opcode == llvm::X86::JMP_4;
}

bool isConditionalJump(unsigned opcode) {
  return opcode == llvm::X86::JCC_1 || opcode == llvm::X86::JCC_4;
}

std::optional<uint64_t> directTarget(const IR0 &instruction) {
  if (instruction.Inst.getNumOperands() == 0 ||
      !instruction.Inst.getOperand(0).isImm())
    return std::nullopt;
  return instruction.Addr + instruction.Size +
         instruction.Inst.getOperand(0).getImm();
}
} // namespace

std::optional<size_t> IR0CFG::findBlock(uint64_t address) const {
  auto block = llvm::find_if(Blocks, [&](const IR0BasicBlock &candidate) {
    return candidate.Address == address;
  });
  if (block == Blocks.end())
    return std::nullopt;
  return static_cast<size_t>(std::distance(Blocks.begin(), block));
}

bool IR0CFG::build(const std::vector<IR0> &instructions) {
  Blocks.clear();
  if (instructions.empty())
    return false;

  llvm::DenseMap<uint64_t, size_t> instructionIndices;
  for (size_t index = 0; index < instructions.size(); ++index) {
    if (instructionIndices.count(instructions[index].Addr))
      return false;
    instructionIndices[instructions[index].Addr] = index;
  }

  llvm::SmallVector<size_t> leaders{0};
  for (size_t index = 0; index < instructions.size(); ++index) {
    const IR0 &instruction = instructions[index];
    unsigned opcode = instruction.Inst.getOpcode();
    bool branch = isDirectJump(opcode) || isConditionalJump(opcode);
    bool terminator = branch || opcode == llvm::X86::RET64;
    if (branch) {
      auto target = directTarget(instruction);
      if (!target)
        return false;
      if (auto found = instructionIndices.find(*target);
          found != instructionIndices.end())
        leaders.push_back(found->second);
    }
    if (terminator && index + 1 < instructions.size())
      leaders.push_back(index + 1);
  }
  llvm::sort(leaders);
  leaders.erase(std::unique(leaders.begin(), leaders.end()), leaders.end());

  llvm::DenseMap<size_t, size_t> indexToBlock;
  for (size_t blockIndex = 0; blockIndex < leaders.size(); ++blockIndex) {
    size_t begin = leaders[blockIndex];
    size_t end = blockIndex + 1 < leaders.size() ? leaders[blockIndex + 1]
                                                 : instructions.size();
    Blocks.push_back({instructions[begin].Addr, begin, end, false, {}});
    for (size_t index = begin; index < end; ++index)
      indexToBlock[index] = blockIndex;
  }

  auto addEdge = [&](IR0BasicBlock &block, IR0CFGEdgeKind kind,
                     uint64_t address) {
    std::optional<size_t> targetBlock;
    if (auto instruction = instructionIndices.find(address);
        instruction != instructionIndices.end())
      targetBlock = indexToBlock.lookup(instruction->second);
    block.Successors.push_back({kind, address, targetBlock});
  };

  for (IR0BasicBlock &block : Blocks) {
    const IR0 &last = instructions[block.EndIndex - 1];
    unsigned opcode = last.Inst.getOpcode();
    if (opcode == llvm::X86::RET64)
      continue;
    if (isDirectJump(opcode) || isConditionalJump(opcode)) {
      auto target = directTarget(last);
      if (!target)
        return false;
      addEdge(block, IR0CFGEdgeKind::Branch, *target);
      if (isDirectJump(opcode))
        continue;
    }
    uint64_t fallthrough = last.Addr + last.Size;
    addEdge(block, IR0CFGEdgeKind::Fallthrough, fallthrough);
  }

  llvm::SmallVector<size_t> worklist{0};
  while (!worklist.empty()) {
    size_t index = worklist.pop_back_val();
    if (Blocks[index].Reachable)
      continue;
    Blocks[index].Reachable = true;
    for (const IR0CFGEdge &edge : Blocks[index].Successors)
      if (edge.TargetBlock && !Blocks[*edge.TargetBlock].Reachable)
        worklist.push_back(*edge.TargetBlock);
  }
  return true;
}

bool IR0Function::buildCFG() { return CFG.build(IRs); }

bool IR0Function::matches(StringRef name) const {
  return Name == name || llvm::is_contained(Aliases, name);
}

void IR0Function::print(raw_ostream &out) const {
  out << "function " << Name << " " << format_hex(Address, 10) << "\n";
  for (size_t index = 0; index < CFG.Blocks.size(); ++index) {
    const IR0BasicBlock &block = CFG.Blocks[index];
    out << "  block " << index << " " << format_hex(block.Address, 10)
        << (block.Reachable ? " reachable" : " unreachable") << "\n";
    for (size_t instruction = block.BeginIndex; instruction < block.EndIndex;
         ++instruction) {
      out << "    ";
      IRs[instruction].print(out);
    }
    for (const IR0CFGEdge &edge : block.Successors)
      out << "    -> " << format_hex(edge.TargetAddress, 10)
          << (edge.Kind == IR0CFGEdgeKind::Branch ? " branch" : " fallthrough")
          << (edge.TargetBlock ? "" : " unresolved") << "\n";
  }
}

void IR0Context::print() const {
  for (const IR0Function &function : Functions)
    function.print();
}

uint64_t IR0Context::translatedCodeSize() const {
  uint64_t size = 0;
  for (const IR0Function &function : Functions)
    for (const IR0BasicBlock &block : function.CFG.Blocks) {
      if (!block.Reachable)
        continue;
      for (size_t index = block.BeginIndex; index < block.EndIndex; ++index)
        size += function.IRs[index].Size;
    }
  return size;
}

void IR0Context::printFunctionNames(raw_ostream &out) const {
  for (const IR0Function &function : Functions)
    out << function.Name << '\n';
}
