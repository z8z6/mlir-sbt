//
// Created by zzm on 2026/6/30
// Part of RVision
//
#include "IR/IR1.h"
#include "IR/IR0.h"
#include "mlir/Dialect/Arith/IR/Arith.h"
#include "mlir/IR/BuiltinOps.h"
#include "mlir/IR/BuiltinTypes.h"
#include "mlir/IR/MLIRContext.h"
#include "mlir/IR/Verifier.h"
#include "mlir/IR/Diagnostics.h"

using namespace llvm;
using namespace mlir;
using namespace z8;
using namespace z8::ir1;

#include "mlir/IR1Dialect.cpp.inc"

void IR1Dialect::initialize() {
  addOperations<
#define GET_OP_LIST
#include "mlir/IR1Ops.cpp.inc"
    >();
}

void AddOp::build(OpBuilder &builder, OperationState &state,
                  Value lhs, Value rhs) {
  state.addOperands({lhs, rhs});
}

#define GET_OP_CLASSES
#include "mlir/IR1Ops.cpp.inc"

namespace {
class IR1Context {
public:
  MLIRContext Ctx;
  ModuleOp Module; // 持有 MLIR 模块
  OpBuilder Builder;            // 用于创建 IR

  IR1Context();
  void transform(const std::vector<IR0>& IRs);
  void verify();
};

} // namespace


IR1Context::IR1Context() : Builder(&Ctx)  {
  Ctx.getOrLoadDialect<IR1Dialect>();
  Ctx.getOrLoadDialect<arith::ArithDialect>();

  Module = ModuleOp::create(Builder.getUnknownLoc());
}

void IR1Context::verify() {
  if (failed(mlir::verify(Module)))
    Module.emitError("module verification error");
}

void IR1Context::transform(const std::vector<IR0> &IRs) {
  for (auto &IR : IRs) {
    auto op = IR.Inst.getOpcode();
  }
}

bool translateMCInst(const llvm::MCInst& Inst, mlir::Location Loc);