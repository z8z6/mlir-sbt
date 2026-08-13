#pragma once

#include "Trans/X86/Common.h"

namespace z8::x86::trans {

enum class ScalarFloatKind { Add, Sub, Mul, Div };

void translateScalarFloat(ConversionContext &context, unsigned width,
                          ScalarFloatKind kind);

} // namespace z8::x86::trans
