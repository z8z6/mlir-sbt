#pragma once

#include "Trans/X86/Common.h"

namespace z8 {
struct ConversionContext;

namespace x86::trans {

enum class BinaryArithmeticKind {
  Add,
  Sub,
};

/// Translate r,r / r,imm / r,[mem]. The generic converter stores the returned
/// destination and RFLAGS after this helper records them in the context.
void translateBinaryRegisterDestination(ConversionContext &context,
                                        unsigned width,
                                        BinaryArithmeticKind kind);

/// Translate [mem],r / [mem],imm, including the memory write and RFLAGS result.
void translateBinaryMemoryDestination(ConversionContext &context,
                                      unsigned width,
                                      BinaryArithmeticKind kind);

/// Compute arithmetic flags without writing the arithmetic result. These two
/// operand layouts are shared by CMP register/immediate/memory encodings.
void translateBinaryArithmeticRegisterFlags(ConversionContext &context,
                                            unsigned width,
                                            BinaryArithmeticKind kind);
void translateBinaryArithmeticMemoryFlags(ConversionContext &context,
                                          unsigned width,
                                          BinaryArithmeticKind kind);
void translateNegRegisterDestination(ConversionContext &context,
                                     unsigned width);
void translateNegMemoryDestination(ConversionContext &context, unsigned width);

} // namespace x86::trans
} // namespace z8
