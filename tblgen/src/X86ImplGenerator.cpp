//
// Created by zzm on 2026/7/3
// Part of RVision
//

#include "X86ImplGenerator.h"

using namespace llvm;
using namespace z8;
using namespace std;

void X86ImplGenerator::run(raw_ostream &OS) const {
  emitSourceFileHeader("IR1 X86 Impl Generator", OS);

  OS << "#include <llvm/MC/MCInst.h>\n"
        "#include <IR/IR0.h>\n"
        "#include <IR/IR1.h>\n"
        "#include <tblgen/X86IR1Converter.h>\n"
        "\n"
        "using namespace z8;\n"
        "using namespace llvm;\n"
        "using namespace llvm::X86;\n\n"
        "void z8::convertMCInst(const IR0& IR) {\n"
        "  switch (IR.Inst.getOpcode()) {\n";

  auto Insts = RK.getAllDerivedDefinitions("X86Inst");

  for (auto *Rec : Insts) {
    StringRef OpcodeName = Rec->getName();
    if (!OpcodeName.starts_with("ADD") && !OpcodeName.starts_with("SUB") &&
        !OpcodeName.starts_with("AND") && !OpcodeName.starts_with("OR") &&
        !OpcodeName.starts_with("XOR") && !OpcodeName.starts_with("MOV") &&
        !OpcodeName.starts_with("MUL") && !OpcodeName.starts_with("DIV"))
      continue;

    string ClassName = "X86_" + OpcodeName.str() + "_IR1Converter";

    OS << "  case " << OpcodeName
       << ":\n"
          "    "
       << ClassName
       << "::Instance().run(IR);\n"
          "    break;\n";
  }
  OS << "  case RET64:\n"
        "  case LOCK_PREFIX:\n"
        "    break;\n"
        "  default:\n"
        "    BaseIR1Converter::Ctx->markConversionFailure();\n"
        "    break;\n"
        "  }\n"
        "}\n";
}

bool z8::EmitX86Impl(llvm::raw_ostream &OS, const llvm::RecordKeeper &RK) {
  X86ImplGenerator Emitter(RK);
  Emitter.run(OS);
  return false;
}
