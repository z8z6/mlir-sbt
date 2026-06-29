//
// Created by root on 25-12-9.
// Part of RVision
//

#pragma once

#include "llvm/Target/TargetMachine.h"

namespace llvm
{
class Target;
class MCRegisterInfo;
class MCAsmInfo;
class MCInstrInfo;
class MCSubtargetInfo;
class MCDisassembler;
class MCInstPrinter;
class MCContext;
}

namespace z8
{
class BaseMachine {
private:
  const llvm::Triple Triple;
  const llvm::Target* Target;
  std::string CPU;
  std::string Features;

  // MachineCode Level
  std::unique_ptr<llvm::MCRegisterInfo> MRI;
  std::unique_ptr<llvm::MCAsmInfo> MAI;
  std::unique_ptr<llvm::MCInstrInfo> MII;
  std::unique_ptr<llvm::MCSubtargetInfo> STI;
  std::unique_ptr<llvm::MCDisassembler> DisAsm;
  std::unique_ptr<llvm::MCInstPrinter> IP;
  std::unique_ptr<llvm::MCContext> Ctx;

public:
  explicit BaseMachine(llvm::Triple);
  virtual ~BaseMachine();

public:
  const llvm::Triple& getTriple() { return Triple; }
  const llvm::Target* getTarget() { return Target; }

  // Machine Level
  const llvm::MCRegisterInfo& getMRI() const { return *MRI; }
  const llvm::MCAsmInfo& getMAI() const { return *MAI; }
  const llvm::MCInstrInfo& getMII() const { return *MII; }
  const llvm::MCSubtargetInfo& getSTI() const { return *STI; }
  const llvm::MCDisassembler& getDisAsm() const { return *DisAsm; }
  llvm::MCInstPrinter& getIP() const { return *IP; }
  llvm::MCContext& getCtx() const { return *Ctx; }
};

BaseMachine& getMachine(const llvm::Triple& triple);

}

