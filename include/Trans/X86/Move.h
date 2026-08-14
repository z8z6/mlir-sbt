#pragma once

#include "Trans/X86/Common.h"

namespace z8::x86::trans {

void translateMoveRegisterDestination(ConversionContext &context,
                                      unsigned width);
void translateMoveMemoryDestination(ConversionContext &context, unsigned width);
void translateVectorMoveRegisterDestination(ConversionContext &context);
void translateVectorMoveMemoryDestination(ConversionContext &context);

} // namespace z8::x86::trans
