#include "Target/X86Register.h"

#include "Target/X86/MCTargetDesc/X86MCTargetDesc.h"

using namespace llvm;
using namespace z8;

std::optional<X86RegisterDesc> z8::getX86RegisterDesc(unsigned reg) {
#define REG_CASE(NAME, SLOT, WIDTH, OFFSET, ZERO_EXTEND)                       \
  case X86::NAME:                                                              \
    return X86RegisterDesc { X86RegisterSlot::SLOT, WIDTH, OFFSET, ZERO_EXTEND }

  switch (reg) {
    REG_CASE(RAX, RAX, 64, 0, false);
    REG_CASE(EAX, RAX, 32, 0, true);
    REG_CASE(AX, RAX, 16, 0, false);
    REG_CASE(AL, RAX, 8, 0, false);
    REG_CASE(AH, RAX, 8, 8, false);
    REG_CASE(RBX, RBX, 64, 0, false);
    REG_CASE(EBX, RBX, 32, 0, true);
    REG_CASE(BX, RBX, 16, 0, false);
    REG_CASE(BL, RBX, 8, 0, false);
    REG_CASE(BH, RBX, 8, 8, false);
    REG_CASE(RCX, RCX, 64, 0, false);
    REG_CASE(ECX, RCX, 32, 0, true);
    REG_CASE(CX, RCX, 16, 0, false);
    REG_CASE(CL, RCX, 8, 0, false);
    REG_CASE(CH, RCX, 8, 8, false);
    REG_CASE(RDX, RDX, 64, 0, false);
    REG_CASE(EDX, RDX, 32, 0, true);
    REG_CASE(DX, RDX, 16, 0, false);
    REG_CASE(DL, RDX, 8, 0, false);
    REG_CASE(DH, RDX, 8, 8, false);
    REG_CASE(RSI, RSI, 64, 0, false);
    REG_CASE(ESI, RSI, 32, 0, true);
    REG_CASE(SI, RSI, 16, 0, false);
    REG_CASE(SIL, RSI, 8, 0, false);
    REG_CASE(RDI, RDI, 64, 0, false);
    REG_CASE(EDI, RDI, 32, 0, true);
    REG_CASE(DI, RDI, 16, 0, false);
    REG_CASE(DIL, RDI, 8, 0, false);
    REG_CASE(RBP, RBP, 64, 0, false);
    REG_CASE(EBP, RBP, 32, 0, true);
    REG_CASE(BP, RBP, 16, 0, false);
    REG_CASE(BPL, RBP, 8, 0, false);
    REG_CASE(RSP, RSP, 64, 0, false);
    REG_CASE(ESP, RSP, 32, 0, true);
    REG_CASE(SP, RSP, 16, 0, false);
    REG_CASE(SPL, RSP, 8, 0, false);
#define EXTENDED_GPR(N)                                                        \
  REG_CASE(R##N, R##N, 64, 0, false);                                          \
  REG_CASE(R##N##D, R##N, 32, 0, true);                                        \
  REG_CASE(R##N##W, R##N, 16, 0, false);                                       \
  REG_CASE(R##N##B, R##N, 8, 0, false)
    EXTENDED_GPR(8);
    EXTENDED_GPR(9);
    EXTENDED_GPR(10);
    EXTENDED_GPR(11);
    EXTENDED_GPR(12);
    EXTENDED_GPR(13);
    EXTENDED_GPR(14);
    EXTENDED_GPR(15);
#undef EXTENDED_GPR
    REG_CASE(RFLAGS, RFLAGS, 64, 0, false);
#define XMM_REGISTER(N) REG_CASE(XMM##N, XMM##N, 128, 0, false)
    XMM_REGISTER(0);
    XMM_REGISTER(1);
    XMM_REGISTER(2);
    XMM_REGISTER(3);
    XMM_REGISTER(4);
    XMM_REGISTER(5);
    XMM_REGISTER(6);
    XMM_REGISTER(7);
    XMM_REGISTER(8);
    XMM_REGISTER(9);
    XMM_REGISTER(10);
    XMM_REGISTER(11);
    XMM_REGISTER(12);
    XMM_REGISTER(13);
    XMM_REGISTER(14);
    XMM_REGISTER(15);
#undef XMM_REGISTER
  default:
    return std::nullopt;
  }
#undef REG_CASE
}
