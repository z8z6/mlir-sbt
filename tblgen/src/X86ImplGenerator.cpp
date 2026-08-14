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
        "void z8::convertMCInst(const IR0& IR, mlir::Block* BranchTarget, "
        "mlir::Block* Fallthrough) {\n"
        "  switch (IR.Inst.getOpcode()) {\n";

  auto Insts = RK.getAllDerivedDefinitions("X86Inst");

  for (auto *Rec : Insts) {
    StringRef OpcodeName = Rec->getName();
    if (!OpcodeName.starts_with("ADD") && !OpcodeName.starts_with("SUB") &&
        !OpcodeName.starts_with("AND") && !OpcodeName.starts_with("OR") &&
        !OpcodeName.starts_with("XOR") && !OpcodeName.starts_with("MOV") &&
        !OpcodeName.starts_with("MUL") && !OpcodeName.starts_with("DIV") &&
        !OpcodeName.starts_with("JMP") && !OpcodeName.starts_with("JCC") &&
        !OpcodeName.starts_with("LEA") && !OpcodeName.starts_with("PUSH") &&
        !OpcodeName.starts_with("POP") && !OpcodeName.starts_with("CALL") &&
        !OpcodeName.starts_with("CVT") && !OpcodeName.starts_with("CMP") &&
        !OpcodeName.starts_with("TEST") &&
        !OpcodeName.starts_with("CMOV") && !OpcodeName.starts_with("SETCC") &&
        !OpcodeName.starts_with("NEG") && !OpcodeName.starts_with("NOT") &&
        OpcodeName != "SYSCALL" && OpcodeName != "ENDBR64" &&
        OpcodeName != "NOOP" && OpcodeName != "NOOPL" &&
        OpcodeName != "NOOPW")
      continue;

    string ClassName = "X86_" + OpcodeName.str() + "_IR1Converter";

    OS << "  case " << OpcodeName
       << ":\n"
          "    "
       << ClassName
       << "::Instance().run(IR, BranchTarget, Fallthrough);\n"
          "    break;\n";
  }
  OS << "  case LOCK_PREFIX:\n"
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
