#include "Trans/X86/Logical.h"

#include "tblgen/X86IR1Converter.h"

using namespace z8;
using namespace z8::x86::trans;

#define DEFINE_XOR_REGISTER(SUFFIX, WIDTH)                                     \
  void X86_XOR##SUFFIX##_IR1Converter::op(ConversionContext &context) {        \
    translateBinaryLogicalRegisterDestination(context, WIDTH,                  \
                                              BinaryLogicalKind::Xor);         \
  }

DEFINE_XOR_REGISTER(64rr, 64)
DEFINE_XOR_REGISTER(32rr, 32)
DEFINE_XOR_REGISTER(16rr, 16)
DEFINE_XOR_REGISTER(8rr, 8)
DEFINE_XOR_REGISTER(64ri8, 64)
DEFINE_XOR_REGISTER(32ri8, 32)
DEFINE_XOR_REGISTER(16ri8, 16)
DEFINE_XOR_REGISTER(8ri8, 8)
DEFINE_XOR_REGISTER(64ri32, 64)
DEFINE_XOR_REGISTER(32ri, 32)
DEFINE_XOR_REGISTER(16ri, 16)
DEFINE_XOR_REGISTER(8ri, 8)
DEFINE_XOR_REGISTER(64rm, 64)
DEFINE_XOR_REGISTER(32rm, 32)
DEFINE_XOR_REGISTER(16rm, 16)
DEFINE_XOR_REGISTER(8rm, 8)

#undef DEFINE_XOR_REGISTER

#define DEFINE_XOR_ACCUMULATOR(SUFFIX, WIDTH, REGISTER)                        \
  void X86_XOR##SUFFIX##_IR1Converter::loadSrcOperand(                         \
      ConversionContext &context) {                                            \
    loadAccumulatorImmediate(context, llvm::X86::REGISTER);                    \
  }                                                                            \
  void X86_XOR##SUFFIX##_IR1Converter::storeDstOperand(                        \
      ConversionContext &context) {                                            \
    storeAccumulatorResult(context, llvm::X86::REGISTER);                      \
  }                                                                            \
  void X86_XOR##SUFFIX##_IR1Converter::op(ConversionContext &context) {        \
    translateBinaryLogicalRegisterDestination(context, WIDTH,                  \
                                              BinaryLogicalKind::Xor);         \
  }

DEFINE_XOR_ACCUMULATOR(8i8, 8, AL)
DEFINE_XOR_ACCUMULATOR(16i16, 16, AX)
DEFINE_XOR_ACCUMULATOR(32i32, 32, EAX)
DEFINE_XOR_ACCUMULATOR(64i32, 64, RAX)

#undef DEFINE_XOR_ACCUMULATOR

#define DEFINE_XOR_MEMORY(SUFFIX, WIDTH)                                       \
  void X86_XOR##SUFFIX##_IR1Converter::op(ConversionContext &context) {        \
    translateBinaryLogicalMemoryDestination(context, WIDTH,                    \
                                            BinaryLogicalKind::Xor);           \
  }

DEFINE_XOR_MEMORY(64mr, 64)
DEFINE_XOR_MEMORY(32mr, 32)
DEFINE_XOR_MEMORY(16mr, 16)
DEFINE_XOR_MEMORY(8mr, 8)
DEFINE_XOR_MEMORY(64mi8, 64)
DEFINE_XOR_MEMORY(64mi32, 64)
DEFINE_XOR_MEMORY(32mi, 32)
DEFINE_XOR_MEMORY(16mi, 16)
DEFINE_XOR_MEMORY(8mi, 8)

#undef DEFINE_XOR_MEMORY
