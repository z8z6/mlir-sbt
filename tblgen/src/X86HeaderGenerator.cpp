//
// Created by zzm on 2026/6/30
// Part of RVision
//
//
// Created by zzm on 25-12-10
// Part of RVision
//
#include "X86HeaderGenerator.h"

using namespace llvm;
using namespace z8;
using namespace std;

void X86HeaderGenerator::run(llvm::raw_ostream &OS) const {
  emitSourceFileHeader("IR1 X86 Header Generator",OS);

  OS << "#include <llvm/MC/MCInst.h>\n";
  OS << "#include <llvm/MC/MCInstrInfo.h>\n";
  OS << "#include <IR/IR1Converter.h>\n";
  OS << "#include <Target/X86Machine.h>\n";
  OS << "#include <Target/X86/MCTargetDesc/X86BaseInfo.h>\n";
  OS << "\n";
  OS << "namespace z8 {\n";

  auto Insts = RK.getAllDerivedDefinitions("X86Inst");

  for (auto *Rec : Insts) {
    StringRef OpcodeName = Rec->getName();
    if (!OpcodeName.starts_with("ADD")) continue;

    string ClassName = "X86_" + OpcodeName.str() + "_IR1Converter";
    OS << "class " << ClassName << " : public BaseIR1Converter {\n";
    OS << "public:\n";
    OS << "  " << ClassName << "() {\n";
    OS << "    Opcode = llvm::X86::" << OpcodeName << ";\n";
    OS << "    MID = &getX86Machine().getMII().get(Opcode);\n";
    OS << "  }\n";
    OS << "  " << "void op(ConversionContext&) override {};\n";
    OS << "  static " << ClassName << "& Instance() {\n";
    OS << "    static " << ClassName << " _;\n";
    OS << "    return _;\n";
    OS << "  }\n";
    OS << "};\n\n";
  }

  OS << "}\n";
}

bool z8::EmitX86Header(raw_ostream& OS, const RecordKeeper& RK)
{
    X86HeaderGenerator Emitter(RK);
    Emitter.run(OS);
    return false;
}