//
// Created by zzm on 2026/7/3
// Part of RVision
//

#include "X86ImplGenerator.h"


using namespace llvm;
using namespace z8;
using namespace std;

void X86ImplGenerator::run(raw_ostream &OS) const {
  emitSourceFileHeader("IR1 X86 Impl Generator",OS);

  OS << "#include <llvm/MC/MCInst.h>\n";
  OS << "#include <tblgen/X86IR1Converter.h>\n";

  OS << "\n";
  OS << "using namespace z8;\n";
  OS << "using namespace llvm;\n";
  OS << "using namespace llvm::X86;\n\n";
  OS << "void z8::convertMCInst(const MCInst& MI) {\n";
  OS << "  switch (MI.getOpcode()) {\n";

  auto Insts = RK.getAllDerivedDefinitions("X86Inst");

  for (auto *Rec : Insts) {
    StringRef OpcodeName = Rec->getName();
    if (!OpcodeName.starts_with("ADD")) continue;

    string ClassName = "X86_" + OpcodeName.str() + "_IR1Converter";

    OS << "  case " << OpcodeName << ":\n";
    OS << "    " << ClassName << "::Instance().run(MI);\n";
    OS << "    break;\n";
  }
  OS << "  default: break;\n";
  OS << "  }\n";
  OS << "}\n";
}

bool z8::EmitX86Impl(llvm::raw_ostream &OS, const llvm::RecordKeeper &RK) {
  X86ImplGenerator Emitter(RK);
  Emitter.run(OS);
  return false;
}