#pragma once

#include "IR/IR1Converter.h"

namespace z8::x86::trans {

enum class X87BinaryKind { Add, Sub, Mul, Div };

struct X87BinarySpec {
  X87BinaryKind kind;
  unsigned memoryWidth;
  bool integerMemory;
  bool destinationIsSTi;
  bool reverse;
  bool pop;
};

void loadX87Operands(ConversionContext &context);
void translateX87Binary(ConversionContext &context,
                        const X87BinarySpec &spec);

} // namespace z8::x86::trans
