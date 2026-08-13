//
// Created by zzm on 2026/6/30
// Part of RVision
//
//
// Created by zzm on 25-12-10
// Part of RVision
//
#include "X86HeaderGenerator.h"

#include "llvm/ADT/StringSwitch.h"

using namespace llvm;
using namespace z8;
using namespace std;

void X86HeaderGenerator::run(llvm::raw_ostream &OS) const {
  emitSourceFileHeader("IR1 X86 Header Generator",OS);

  OS <<
    "#pragma once\n\n"
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
    bool HasImplementation = StringSwitch<bool>(OpcodeName)
      .Cases({"ADD64rr", "ADD32rr", "ADD16rr", "ADD8rr"}, true)
      .Cases({"ADD64ri8", "ADD32ri8", "ADD16ri8", "ADD8ri8"}, true)
      .Cases({"ADD8ri", "ADD8i8", "ADD16i16", "ADD32i32"}, true)
      .Case("ADD64i32", true)
      .Cases({"ADD64rm", "ADD32rm", "ADD16rm", "ADD8rm"}, true)
      .Cases({"ADD64mr", "ADD32mr", "ADD16mr", "ADD8mr"}, true)
      .Cases({"ADD64mi8", "ADD32mi", "ADD16mi", "ADD8mi"}, true)
      .Default(false);
    OS <<
      "class " << ClassName << " : public BaseIR1Converter {\n"
      "public:\n"
      "  " << ClassName << "() {\n"
      "    Opcode = llvm::X86::" << OpcodeName << ";\n"
      "    MID = &getX86Machine().getMII().get(Opcode);\n"
      "  }\n"
      "  std::string getName() const override { return \"" << OpcodeName << "\"; }\n"
      "  static " << ClassName << "& Instance() {\n"
      "    static " << ClassName << " _;\n"
      "    return _;\n"
      "  }\n";
    if (HasImplementation)
      OS << "  void op(ConversionContext&) override;\n";
    if (StringSwitch<bool>(OpcodeName)
          .Cases({"ADD8i8", "ADD16i16", "ADD32i32", "ADD64i32"}, true)
          .Default(false))
      OS << "  void loadSrcOperand(ConversionContext&) override;\n"
            "  void storeDstOperand(ConversionContext&) override;\n";
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
