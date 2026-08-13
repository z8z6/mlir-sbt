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
  emitSourceFileHeader("IR1 X86 Header Generator", OS);

  OS << "#pragma once\n\n"
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
    if (!OpcodeName.starts_with("ADD") && !OpcodeName.starts_with("SUB") &&
        !OpcodeName.starts_with("AND") && !OpcodeName.starts_with("OR") &&
        !OpcodeName.starts_with("XOR") && !OpcodeName.starts_with("MOV") &&
        !OpcodeName.starts_with("MUL") && !OpcodeName.starts_with("DIV"))
      continue;

    string ClassName = "X86_" + OpcodeName.str() + "_IR1Converter";
    bool HasImplementation =
        StringSwitch<bool>(OpcodeName)
            .Cases({"ADD64rr", "ADD32rr", "ADD16rr", "ADD8rr"}, true)
            .Cases({"ADD64ri8", "ADD32ri8", "ADD16ri8", "ADD8ri8"}, true)
            .Cases({"ADD8ri", "ADD8i8", "ADD16i16", "ADD32i32"}, true)
            .Case("ADD64i32", true)
            .Cases({"ADD64rm", "ADD32rm", "ADD16rm", "ADD8rm"}, true)
            .Cases({"ADD64mr", "ADD32mr", "ADD16mr", "ADD8mr"}, true)
            .Cases({"ADD64mi8", "ADD32mi", "ADD16mi", "ADD8mi"}, true)
            .Cases({"SUB64rr", "SUB32rr", "SUB16rr", "SUB8rr"}, true)
            .Cases({"SUB64ri8", "SUB32ri8", "SUB16ri8", "SUB8ri8"}, true)
            .Cases({"SUB64ri32", "SUB32ri", "SUB16ri"}, true)
            .Cases({"SUB8ri", "SUB8i8", "SUB16i16", "SUB32i32"}, true)
            .Case("SUB64i32", true)
            .Cases({"SUB64rm", "SUB32rm", "SUB16rm", "SUB8rm"}, true)
            .Cases({"SUB64mr", "SUB32mr", "SUB16mr", "SUB8mr"}, true)
            .Cases({"SUB64mi8", "SUB32mi", "SUB16mi", "SUB8mi"}, true)
            .Case("SUB64mi32", true)
            .Cases({"AND64rr", "AND32rr", "AND16rr", "AND8rr"}, true)
            .Cases({"AND64ri8", "AND32ri8", "AND16ri8", "AND8ri8"}, true)
            .Cases({"AND64ri32", "AND32ri", "AND16ri", "AND8ri"}, true)
            .Cases({"AND8i8", "AND16i16", "AND32i32", "AND64i32"}, true)
            .Cases({"AND64rm", "AND32rm", "AND16rm", "AND8rm"}, true)
            .Cases({"AND64mr", "AND32mr", "AND16mr", "AND8mr"}, true)
            .Cases({"AND64mi8", "AND64mi32", "AND32mi", "AND16mi", "AND8mi"},
                   true)
            .Cases({"OR64rr", "OR32rr", "OR16rr", "OR8rr"}, true)
            .Cases({"OR64ri8", "OR32ri8", "OR16ri8", "OR8ri8"}, true)
            .Cases({"OR64ri32", "OR32ri", "OR16ri", "OR8ri"}, true)
            .Cases({"OR8i8", "OR16i16", "OR32i32", "OR64i32"}, true)
            .Cases({"OR64rm", "OR32rm", "OR16rm", "OR8rm"}, true)
            .Cases({"OR64mr", "OR32mr", "OR16mr", "OR8mr"}, true)
            .Cases({"OR64mi8", "OR64mi32", "OR32mi", "OR16mi", "OR8mi"}, true)
            .Cases({"XOR64rr", "XOR32rr", "XOR16rr", "XOR8rr"}, true)
            .Cases({"XOR64ri8", "XOR32ri8", "XOR16ri8", "XOR8ri8"}, true)
            .Cases({"XOR64ri32", "XOR32ri", "XOR16ri", "XOR8ri"}, true)
            .Cases({"XOR8i8", "XOR16i16", "XOR32i32", "XOR64i32"}, true)
            .Cases({"XOR64rm", "XOR32rm", "XOR16rm", "XOR8rm"}, true)
            .Cases({"XOR64mr", "XOR32mr", "XOR16mr", "XOR8mr"}, true)
            .Cases({"XOR64mi8", "XOR64mi32", "XOR32mi", "XOR16mi", "XOR8mi"},
                   true)
            .Cases({"MOV64rr", "MOV32rr", "MOV16rr", "MOV8rr"}, true)
            .Cases({"MOV64ri", "MOV64ri32", "MOV32ri", "MOV16ri", "MOV8ri"},
                   true)
            .Cases({"MOV64rm", "MOV32rm", "MOV16rm", "MOV8rm"}, true)
            .Cases({"MOV64mr", "MOV32mr", "MOV16mr", "MOV8mr"}, true)
            .Cases({"MOV64mi32", "MOV32mi", "MOV16mi", "MOV8mi"}, true)
            .Cases({"ADDSSrr_Int", "ADDSSrm_Int", "ADDSDrr_Int", "ADDSDrm_Int"},
                   true)
            .Cases({"SUBSSrr_Int", "SUBSSrm_Int", "SUBSDrr_Int", "SUBSDrm_Int"},
                   true)
            .Cases({"MULSSrr_Int", "MULSSrm_Int", "MULSDrr_Int", "MULSDrm_Int"},
                   true)
            .Cases({"DIVSSrr_Int", "DIVSSrm_Int", "DIVSDrr_Int", "DIVSDrm_Int"},
                   true)
            .Default(false);
    OS << "class " << ClassName
       << " : public BaseIR1Converter {\n"
          "public:\n"
          "  "
       << ClassName
       << "() {\n"
          "    Opcode = llvm::X86::"
       << OpcodeName
       << ";\n"
          "    MID = &getX86Machine().getMII().get(Opcode);\n"
          "  }\n"
          "  std::string getName() const override { return \""
       << OpcodeName
       << "\"; }\n"
          "  static "
       << ClassName
       << "& Instance() {\n"
          "    static "
       << ClassName
       << " _;\n"
          "    return _;\n"
          "  }\n";
    if (HasImplementation)
      OS << "  void op(ConversionContext&) override;\n";
    if (StringSwitch<bool>(OpcodeName)
            .Cases({"ADD8i8", "ADD16i16", "ADD32i32", "ADD64i32"}, true)
            .Cases({"SUB8i8", "SUB16i16", "SUB32i32", "SUB64i32"}, true)
            .Cases({"AND8i8", "AND16i16", "AND32i32", "AND64i32"}, true)
            .Cases({"OR8i8", "OR16i16", "OR32i32", "OR64i32"}, true)
            .Cases({"XOR8i8", "XOR16i16", "XOR32i32", "XOR64i32"}, true)
            .Default(false))
      OS << "  void loadSrcOperand(ConversionContext&) override;\n"
            "  void storeDstOperand(ConversionContext&) override;\n";
    OS << "};\n\n";
  }

  OS << "}\n";
}

bool z8::EmitX86Header(raw_ostream &OS, const RecordKeeper &RK) {
  X86HeaderGenerator Emitter(RK);
  Emitter.run(OS);
  return false;
}
