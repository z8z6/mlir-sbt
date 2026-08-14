#pragma once

namespace z8 {
struct ConversionContext;

void translateDirectJump(ConversionContext &context);
void translateConditionalJump(ConversionContext &context);
} // namespace z8
