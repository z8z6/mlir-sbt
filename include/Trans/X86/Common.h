#pragma once

#include "mlir/IR/Value.h"

#include <cstddef>

namespace z8 {
struct ConversionContext;

namespace x86::trans {

/// Build base + scale * index + displacement + segment from an LLVM x86
/// five-operand memory tuple already loaded into ConversionContext::Src.
mlir::Value buildMemoryAddress(ConversionContext &context,
                               size_t operandOffset = 0);

/// LLVM MC accumulator-immediate opcodes expose only the immediate. Materialize
/// the implicit AL/AX/EAX/RAX destination before the immediate source.
void loadAccumulatorImmediate(ConversionContext &context, unsigned reg);

/// Store an implicit accumulator result and the arithmetic flags.
void storeAccumulatorResult(ConversionContext &context, unsigned reg);

} // namespace x86::trans
} // namespace z8
