//
// Created by zzm on 2026/6/29
// Part of RVision
//

#include "IR/IR0.h"
#include "llvm/Support/Format.h"
#include "Target/BaseMachine.h"
#include "llvm/MC/MCInstPrinter.h"

using namespace llvm;
using namespace z8;

IR0::IR0(llvm::MCInst inst, uint64_t addr)
: Addr(addr), Inst(std::move(inst))
{
}

void IR0::print(BaseMachine* M, raw_ostream& out) const
{
  out <<  format_hex(Addr, 10) << ": ";
  M->getIP().printInst(&Inst, Addr, "", M->getSTI(), out);
  out << "\n";
}

void IR0Context::print(BaseMachine* Machine) {
  for (const auto& IR : IRs) IR.print(Machine);
}