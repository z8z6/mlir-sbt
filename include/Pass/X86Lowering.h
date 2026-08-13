#pragma once

#include <memory>

namespace mlir {
class Pass;
}

namespace z8 {
std::unique_ptr<mlir::Pass> createX86LowerPass();
}
