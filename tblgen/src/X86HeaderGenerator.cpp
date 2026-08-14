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

static bool isImplementedX87Binary(StringRef name) {
  return StringSwitch<bool>(name)
      .Cases({"ADD_FST0r", "ADD_FrST0", "ADD_FPrST0", "ADD_F32m", "ADD_F64m",
              "ADD_FI16m", "ADD_FI32m"},
             true)
      .Cases({"MUL_FST0r", "MUL_FrST0", "MUL_FPrST0", "MUL_F32m", "MUL_F64m",
              "MUL_FI16m", "MUL_FI32m"},
             true)
      .Cases({"SUB_FST0r", "SUB_FrST0", "SUB_FPrST0", "SUB_F32m", "SUB_F64m",
              "SUB_FI16m", "SUB_FI32m"},
             true)
      .Cases({"SUBR_FST0r", "SUBR_FrST0", "SUBR_FPrST0", "SUBR_F32m",
              "SUBR_F64m", "SUBR_FI16m", "SUBR_FI32m"},
             true)
      .Cases({"DIV_FST0r", "DIV_FrST0", "DIV_FPrST0", "DIV_F32m", "DIV_F64m",
              "DIV_FI16m", "DIV_FI32m"},
             true)
      .Cases({"DIVR_FST0r", "DIVR_FrST0", "DIVR_FPrST0", "DIVR_F32m",
              "DIVR_F64m", "DIVR_FI16m", "DIVR_FI32m"},
             true)
      .Default(false);
}

static bool isImplemented(StringRef name) {
  return StringSwitch<bool>(name)
             .Cases({"ENDBR64", "NOOP", "NOOPL", "NOOPW"}, true)
             .Cases({"CMP64rr", "CMP32rr", "CMP16rr", "CMP8rr"}, true)
             .Cases({"CMP64ri8", "CMP32ri8", "CMP16ri8"}, true)
             .Cases({"CMP64ri32", "CMP32ri", "CMP16ri", "CMP8ri"}, true)
             .Cases({"CMP64rm", "CMP32rm", "CMP16rm", "CMP8rm"}, true)
             .Cases({"CMP64mr", "CMP32mr", "CMP16mr", "CMP8mr"}, true)
             .Cases({"CMP64mi8", "CMP64mi32", "CMP32mi8", "CMP32mi", "CMP16mi8",
                     "CMP16mi", "CMP8mi"},
                    true)
             .Cases({"CMP8i8", "CMP16i16", "CMP32i32", "CMP64i32"}, true)
             .Cases({"TEST64rr", "TEST32rr", "TEST16rr", "TEST8rr"}, true)
             .Cases({"TEST64ri32", "TEST32ri", "TEST16ri", "TEST8ri"}, true)
             .Cases({"TEST64mr", "TEST32mr", "TEST16mr", "TEST8mr"}, true)
             .Cases({"TEST64mi32", "TEST32mi", "TEST16mi", "TEST8mi"}, true)
             .Cases({"TEST8i8", "TEST16i16", "TEST32i32", "TEST64i32"}, true)
             .Cases({"CMOV16rr", "CMOV32rr", "CMOV64rr", "CMOV16rm", "CMOV32rm",
                     "CMOV64rm"},
                    true)
             .Cases({"SETCCr", "SETCCm"}, true)
             .Cases({"MOVSX16rr8", "MOVSX16rm8", "MOVSX32rr8", "MOVSX32rm8",
                     "MOVSX32rr16", "MOVSX32rm16", "MOVSX64rr8", "MOVSX64rm8",
                     "MOVSX64rr16", "MOVSX64rm16", "MOVSX64rr32",
                     "MOVSX64rm32"},
                    true)
             .Cases({"MOVZX16rr8", "MOVZX16rm8", "MOVZX32rr8", "MOVZX32rm8",
                     "MOVZX32rr16", "MOVZX32rm16"},
                    true)
             .Case("LEA64_32r", true)
             .Default(false) ||
         isImplementedX87Binary(name) ||
         StringSwitch<bool>(name)
             .Cases({"ADD64rr", "ADD32rr", "ADD16rr", "ADD8rr"}, true)
             .Cases({"ADD64ri8", "ADD32ri8", "ADD16ri8", "ADD8ri8"}, true)
             .Cases({"ADD8ri", "ADD8i8", "ADD16i16", "ADD32i32"}, true)
             .Case("ADD64i32", true)
             .Cases({"ADD64rm", "ADD32rm", "ADD16rm", "ADD8rm"}, true)
             .Cases({"ADD64mr", "ADD32mr", "ADD16mr", "ADD8mr"}, true)
             .Cases({"ADD64mi8", "ADD64mi32", "ADD32mi8", "ADD32mi", "ADD16mi8",
                     "ADD16mi", "ADD8mi"},
                    true)
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
             .Cases({"MOVAPSrr", "MOVAPSrm", "MOVAPSmr"}, true)
             .Cases({"MOVUPSrr", "MOVUPSrm", "MOVUPSmr"}, true)
             .Cases({"MOVDQArr", "MOVDQArm", "MOVDQAmr"}, true)
             .Cases({"MOVDQUrr", "MOVDQUrm", "MOVDQUmr"}, true)
             .Cases({"NEG64r", "NEG32r", "NEG16r", "NEG8r"}, true)
             .Cases({"NEG64m", "NEG32m", "NEG16m", "NEG8m"}, true)
             .Cases({"NOT64r", "NOT32r", "NOT16r", "NOT8r"}, true)
             .Cases({"NOT64m", "NOT32m", "NOT16m", "NOT8m"}, true)
             .Cases(
                 {"ADDSSrr_Int", "ADDSSrm_Int", "ADDSDrr_Int", "ADDSDrm_Int"},
                 true)
             .Cases(
                 {"SUBSSrr_Int", "SUBSSrm_Int", "SUBSDrr_Int", "SUBSDrm_Int"},
                 true)
             .Cases(
                 {"MULSSrr_Int", "MULSSrm_Int", "MULSDrr_Int", "MULSDrm_Int"},
                 true)
             .Cases(
                 {"DIVSSrr_Int", "DIVSSrm_Int", "DIVSDrr_Int", "DIVSDrm_Int"},
                 true)
             .Cases({"JMP_1", "JMP_4", "JCC_1", "JCC_4"}, true)
             .Case("SYSCALL", true)
             .Cases({"LEA64r", "PUSH64r", "POP64r", "CALL64pcrel32", "CALL64r",
                     "CALL64m"},
                    true)
             .Cases({"CVTDQ2PDrr", "CVTDQ2PDrm", "CVTDQ2PSrr", "CVTDQ2PSrm",
                     "CVTPD2DQrr", "CVTPD2DQrm", "CVTPD2PSrr", "CVTPD2PSrm"},
                    true)
             .Cases({"CVTPS2DQrr", "CVTPS2DQrm", "CVTPS2PDrr", "CVTPS2PDrm",
                     "CVTTPD2DQrr", "CVTTPD2DQrm", "CVTTPS2DQrr",
                     "CVTTPS2DQrm"},
                    true)
             .Cases({"CVTSD2SIrr_Int", "CVTSD2SIrm_Int", "CVTSD2SI64rr_Int",
                     "CVTSD2SI64rm_Int", "CVTSS2SIrr_Int", "CVTSS2SIrm_Int",
                     "CVTSS2SI64rr_Int", "CVTSS2SI64rm_Int"},
                    true)
             .Cases({"CVTTSD2SIrr_Int", "CVTTSD2SIrm_Int", "CVTTSD2SI64rr_Int",
                     "CVTTSD2SI64rm_Int", "CVTTSS2SIrr_Int", "CVTTSS2SIrm_Int",
                     "CVTTSS2SI64rr_Int", "CVTTSS2SI64rm_Int"},
                    true)
             .Cases({"CVTSI2SDrr_Int", "CVTSI2SDrm_Int", "CVTSI642SDrr_Int",
                     "CVTSI642SDrm_Int", "CVTSI2SSrr_Int", "CVTSI2SSrm_Int",
                     "CVTSI642SSrr_Int", "CVTSI642SSrm_Int"},
                    true)
             .Cases({"CVTSD2SSrr_Int", "CVTSD2SSrm_Int", "CVTSS2SDrr_Int",
                     "CVTSS2SDrm_Int"},
                    true)
             .Default(false);
}

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
        !OpcodeName.starts_with("MUL") && !OpcodeName.starts_with("DIV") &&
        !OpcodeName.starts_with("JMP") && !OpcodeName.starts_with("JCC") &&
        !OpcodeName.starts_with("LEA") && !OpcodeName.starts_with("PUSH") &&
        !OpcodeName.starts_with("POP") && !OpcodeName.starts_with("CALL") &&
        !OpcodeName.starts_with("CVT") && !OpcodeName.starts_with("CMP") &&
        !OpcodeName.starts_with("TEST") && !OpcodeName.starts_with("CMOV") &&
        !OpcodeName.starts_with("SETCC") && !OpcodeName.starts_with("NEG") &&
        !OpcodeName.starts_with("NOT") && OpcodeName != "SYSCALL" &&
        OpcodeName != "ENDBR64" && OpcodeName != "NOOP" &&
        OpcodeName != "NOOPL" && OpcodeName != "NOOPW")
      continue;

    string ClassName = "X86_" + OpcodeName.str() + "_IR1Converter";
    bool HasImplementation = isImplemented(OpcodeName);
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
    if (isImplementedX87Binary(OpcodeName) ||
        StringSwitch<bool>(OpcodeName)
            .Cases({"ENDBR64", "NOOP", "NOOPL", "NOOPW"}, true)
            .Cases({"JMP_1", "JMP_4", "JCC_1", "JCC_4"}, true)
            .Case("SYSCALL", true)
            .Cases({"LEA64r", "LEA64_32r", "CALL64pcrel32"}, true)
            .Default(false))
      OS << "  void loadSrcOperand(ConversionContext&) override;\n";
    if (StringSwitch<bool>(OpcodeName)
            .Cases({"ADD8i8", "ADD16i16", "ADD32i32", "ADD64i32"}, true)
            .Cases({"SUB8i8", "SUB16i16", "SUB32i32", "SUB64i32"}, true)
            .Cases({"AND8i8", "AND16i16", "AND32i32", "AND64i32"}, true)
            .Cases({"OR8i8", "OR16i16", "OR32i32", "OR64i32"}, true)
            .Cases({"XOR8i8", "XOR16i16", "XOR32i32", "XOR64i32"}, true)
            .Default(false))
      OS << "  void loadSrcOperand(ConversionContext&) override;\n"
            "  void storeDstOperand(ConversionContext&) override;\n";
    if (StringSwitch<bool>(OpcodeName)
            .Cases({"CMP8i8", "CMP16i16", "CMP32i32", "CMP64i32"}, true)
            .Cases({"TEST8i8", "TEST16i16", "TEST32i32", "TEST64i32"}, true)
            .Default(false))
      OS << "  void loadSrcOperand(ConversionContext&) override;\n";
    OS << "};\n\n";
  }

  OS << "inline bool hasX86Converter(unsigned opcode) {\n"
        "  switch (opcode) {\n";
  OS << "  case llvm::X86::RET64: return true;\n";
  for (auto *Rec : Insts)
    if (isImplemented(Rec->getName()))
      OS << "  case llvm::X86::" << Rec->getName() << ": return true;\n";
  OS << "  default: return false;\n"
        "  }\n"
        "}\n\n"
        "}\n";
}

bool z8::EmitX86Header(raw_ostream &OS, const RecordKeeper &RK) {
  X86HeaderGenerator Emitter(RK);
  Emitter.run(OS);
  return false;
}
