#include "tblgen/X86IR1Converter.h"

using namespace z8;

#define X86_NOOP(OPCODE)                                                       \
  void X86_##OPCODE##_IR1Converter::loadSrcOperand(ConversionContext &) {}     \
  void X86_##OPCODE##_IR1Converter::op(ConversionContext &) {}

X86_NOOP(ENDBR64)
X86_NOOP(NOOP)
X86_NOOP(NOOPL)
X86_NOOP(NOOPW)

#undef X86_NOOP
