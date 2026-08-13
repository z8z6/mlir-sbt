#include "Trans/X86/Move.h"

#include "tblgen/X86IR1Converter.h"

using namespace z8;
using namespace z8::x86::trans;

#define DEFINE_MOV_REGISTER(SUFFIX, WIDTH)                                     \
  void X86_MOV##SUFFIX##_IR1Converter::op(ConversionContext &context) {        \
    translateMoveRegisterDestination(context, WIDTH);                          \
  }

DEFINE_MOV_REGISTER(64rr, 64)
DEFINE_MOV_REGISTER(32rr, 32)
DEFINE_MOV_REGISTER(16rr, 16)
DEFINE_MOV_REGISTER(8rr, 8)
DEFINE_MOV_REGISTER(64ri, 64)
DEFINE_MOV_REGISTER(64ri32, 64)
DEFINE_MOV_REGISTER(32ri, 32)
DEFINE_MOV_REGISTER(16ri, 16)
DEFINE_MOV_REGISTER(8ri, 8)
DEFINE_MOV_REGISTER(64rm, 64)
DEFINE_MOV_REGISTER(32rm, 32)
DEFINE_MOV_REGISTER(16rm, 16)
DEFINE_MOV_REGISTER(8rm, 8)

#undef DEFINE_MOV_REGISTER

#define DEFINE_MOV_MEMORY(SUFFIX, WIDTH)                                       \
  void X86_MOV##SUFFIX##_IR1Converter::op(ConversionContext &context) {        \
    translateMoveMemoryDestination(context, WIDTH);                            \
  }

DEFINE_MOV_MEMORY(64mr, 64)
DEFINE_MOV_MEMORY(32mr, 32)
DEFINE_MOV_MEMORY(16mr, 16)
DEFINE_MOV_MEMORY(8mr, 8)
DEFINE_MOV_MEMORY(64mi32, 64)
DEFINE_MOV_MEMORY(32mi, 32)
DEFINE_MOV_MEMORY(16mi, 16)
DEFINE_MOV_MEMORY(8mi, 8)

#undef DEFINE_MOV_MEMORY
