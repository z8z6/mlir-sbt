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
void translateBinaryLogicalRegisterFlags(ConversionContext &context,
                                         unsigned width,
                                         BinaryLogicalKind kind);
void translateBinaryLogicalMemoryFlags(ConversionContext &context,
                                       unsigned width,
                                       BinaryLogicalKind kind);
void translateNotRegisterDestination(ConversionContext &context,
                                     unsigned width);
void translateNotMemoryDestination(ConversionContext &context, unsigned width);

} // namespace z8::x86::trans
