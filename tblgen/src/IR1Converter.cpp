//
// Created by zzm on 2026/6/30
// Part of RVision
//
//
// Created by zzm on 25-12-10
// Part of RVision
//
#include "IR1Converter.h"
#include <llvm/MC/LaneBitmask.h>
#include "llvm/TableGen/TableGenBackend.h"

using namespace llvm;
using namespace z8;

void IR1Converter::run(llvm::raw_ostream &OS) const {
  emitSourceFileHeader("IR1 Converter Classes",OS);

  OS << "#include <llvm/MC/MCInst.h>\n";
  OS << "#include <mlir/IR/Location.h>\n";
  OS << "#include <Target/X86/MCTargetDesc/X86BaseInfo.h>\n";
  OS << "\n";

  OS << "using namespace llvm;\n";
  OS << "using namespace llvm::X86;\n";
  OS << "\n";

  OS << "bool translateMCInst(const llvm::MCInst& Inst, mlir::Location Loc) {\n";
  OS << "  switch (Inst.getOpcode()) {\n";

  auto Insts = RK.getAllDerivedDefinitions("Instruction");

  for (auto *Rec : Insts) {
    auto OpcodeName = Rec->getName();
    if (!OpcodeName.starts_with("ADD")) continue;

    OS << "  case " << OpcodeName << ": {\n";

    OS << "    break;\n";
    OS << "  }\n";
  }

  OS << "  default: return false;\n";
  OS << "  }\n";
  OS << "  return true;\n";
  OS << "}\n";
}

bool z8::EmitIR1(raw_ostream& OS, const RecordKeeper& RK)
{
    IR1Converter Emitter(RK);
    Emitter.run(OS);
    return false;
}