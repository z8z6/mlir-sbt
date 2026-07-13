//
// Created by zzm on 2026/7/13
// Part of RVision
//

#pragma once

#include <memory>

namespace mlir {
class Pass;
}

namespace z8 {
std::unique_ptr<mlir::Pass> createIR1LowerPass();
}
