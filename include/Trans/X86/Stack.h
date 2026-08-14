#pragma once

namespace z8 {
struct ConversionContext;

void translatePush64Register(ConversionContext &context);
void translatePop64Register(ConversionContext &context);
} // namespace z8
