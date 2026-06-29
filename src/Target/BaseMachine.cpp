//
// Created by zzm on 2026/6/29
// Part of RVision
//

#include "Target/BaseMachine.h"
#include "Target/X86Machine.h"

#include "llvm/MC/TargetRegistry.h"
#include "llvm/MC/MCDisassembler/MCDisassembler.h"
#include "llvm/MC/MCInstPrinter.h"
#include "llvm/MC/MCSubtargetInfo.h"
#include "llvm/MC/MCInstrInfo.h"
#include "llvm/MC/MCRegisterInfo.h"
#include "llvm/MC/MCAsmInfo.h"
#include "llvm/MC/MCContext.h"


using namespace llvm;
using namespace z8;


BaseMachine::BaseMachine(const llvm::Triple triple) : Triple(triple)
{
  std::string Error;
  Target = TargetRegistry::lookupTarget(Triple, Error);
  assert(Target);

  STI = std::unique_ptr<MCSubtargetInfo>(Target->createMCSubtargetInfo(Triple, CPU, Features));
  MII = std::unique_ptr<MCInstrInfo>(Target->createMCInstrInfo());
  MRI = std::unique_ptr<MCRegisterInfo>(Target->createMCRegInfo(Triple));
  MAI = std::unique_ptr<MCAsmInfo>(Target->createMCAsmInfo(*MRI, Triple, {}));
  assert(STI && MII && MAI && MRI);

  Ctx = std::make_unique<MCContext>(Triple, MAI.get(), MRI.get(), STI.get());
  assert(Ctx);

  DisAsm = std::unique_ptr<MCDisassembler>(Target->createMCDisassembler(*STI, *Ctx));
  IP = std::unique_ptr<MCInstPrinter>(Target->createMCInstPrinter(Triple, MAI->getAssemblerDialect(), *MAI, *MII, *MRI));
  assert(DisAsm && IP);

}

BaseMachine::~BaseMachine() = default;


BaseMachine& z8::getMachine(const Triple &triple) {
  if (triple.isX86()) return getX86Machine(triple);
}