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

} // namespace x86::trans
} // namespace z8
