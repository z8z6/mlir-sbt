//
// Created by zzm on 2026/6/29
// Part of RVision
//

#include "IR/IR0.h"

#include "Object/File.h"

#include "Target/BaseMachine.h"
#include "llvm/MC/MCInstPrinter.h"
#include "llvm/Support/Format.h"

using namespace llvm;
using namespace z8;

IR0::IR0(llvm::MCInst inst, uint64_t addr)
: Addr(addr), Inst(std::move(inst))
{
}

void IR0::print(raw_ostream& out) const
{
  auto* M = File::CurrentFile->Machine;
  out <<  format_hex(Addr, 10) << ": ";
  M->getIP().printInst(&Inst, Addr, "", M->getSTI(), out);
  out << "\n";
}

std::string IR0::str() const {
  std::string s;
  raw_string_ostream so(s);
  auto* M = File::CurrentFile->Machine;
  M->getIP().printInst(&Inst, Addr, "", M->getSTI(), so);
  so.flush();
  std::replace(s.begin(), s.end(), '\t', ' ');
  return s;
}

void IR0Context::print() const {
  for (const auto& IR : IRs) IR.print();
}