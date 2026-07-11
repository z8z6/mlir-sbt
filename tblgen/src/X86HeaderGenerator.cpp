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

  OS <<
    "#include <llvm/MC/MCInst.h>\n"
    "#include <llvm/MC/MCInstrInfo.h>\n"
    "#include <llvm/Support/Debug.h>\n"
    "#include <llvm/Support/raw_ostream.h>\n"
    "#include <IR/IR1Converter.h>\n"
    "#include <Target/X86Machine.h>\n"
    "#include <Target/X86/MCTargetDesc/X86BaseInfo.h>\n"
    "\n"
    "namespace z8 {\n";

  auto Insts = RK.getAllDerivedDefinitions("X86Inst");

  for (auto *Rec : Insts) {
    StringRef OpcodeName = Rec->getName();
    if (!OpcodeName.starts_with("ADD")) continue;

    string ClassName = "X86_" + OpcodeName.str() + "_IR1Converter";
    OS <<
      "class " << ClassName << " : public BaseIR1Converter {\n"
      "public:\n"
      "  " << ClassName << "() {\n"
      "    Opcode = llvm::X86::" << OpcodeName << ";\n"
      "    MID = &getX86Machine().getMII().get(Opcode);\n"
      "  }\n"
      "  std::string getName() const override { return \"" << OpcodeName << "\"; };\n"
      "  void op(ConversionContext&) override { \n"
      "    llvm::dbgs() << getName() << \" converter is not impl yet!\\n\"; \n"
      "  };\n"
      "  static " << ClassName << "& Instance() {\n"
      "    static " << ClassName << " _;\n"
      "    return _;\n"
      "  }\n"
      "};\n\n";
  }

  OS << "}\n";
}

bool z8::EmitX86Header(raw_ostream& OS, const RecordKeeper& RK)
{
    X86HeaderGenerator Emitter(RK);
    Emitter.run(OS);
    return false;
}