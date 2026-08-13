#pragma once

#include "Trans/X86/Common.h"

namespace z8::x86::trans {

enum class BinaryLogicalKind { And, Or, Xor };

void translateBinaryLogicalRegisterDestination(ConversionContext &context,
                                               unsigned width,
                                               BinaryLogicalKind kind);
void translateBinaryLogicalMemoryDestination(ConversionContext &context,
                                             unsigned width,
                                             BinaryLogicalKind kind);

} // namespace z8::x86::trans
