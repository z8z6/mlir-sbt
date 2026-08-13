#include "IR/X86.h"
#include "Target/X86Register.h"

using namespace z8::x86ir;
using namespace mlir;

#include "mlir/X86Dialect.cpp.inc"

void X86Dialect::initialize() {
  addOperations<
#define GET_OP_LIST
#include "mlir/X86Ops.cpp.inc"
      >();
}

void ReadRegOp::build(OpBuilder &builder, OperationState &state,
                      Value registerState, unsigned id) {
  auto reg = z8::getX86RegisterDesc(id);
  assert(reg && "unsupported x86 register");
  build(builder, state, builder.getIntegerType(reg->width), registerState,
        builder.getI32IntegerAttr(id));
}

#define GET_OP_CLASSES
#include "mlir/X86Ops.cpp.inc"
