//
// Created by zzm on 2026/6/30
// Part of RVision
//
#include "IR/IR1.h"
#include "IR/IR0.h"
#include "IR/IR1Converter.h"
#include "mlir/Dialect/Arith/IR/Arith.h"
#include "mlir/Dialect/MemRef/IR/MemRef.h"

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

void ConstIntOp::build(OpBuilder &builder, OperationState &state, int64_t value) {
  auto dataType = builder.getI64Type();
  auto dataAttribute = IntegerAttr::get(dataType, value);
  build(builder, state, dataType, dataAttribute);
}

void LoadRegOp::build(OpBuilder &builder, OperationState &state, unsigned id) {
  auto dataType = builder.getI32Type();
  auto dataAttribute = IntegerAttr::get(dataType, id);
  build(builder, state, dataType, dataAttribute);
}

void StoreRegOp::build(OpBuilder &builder, OperationState &state, Value value, unsigned id) {
  auto dataType = builder.getI32Type();
  auto dataAttribute = IntegerAttr::get(dataType, id);
  // 返回值类型为空
  build(builder, state, TypeRange{}, value, dataAttribute);
}


#define GET_OP_CLASSES
#include "mlir/IR1Ops.cpp.inc"


IR1Context::IR1Context() : Builder(&Ctx)  {
  Ctx.getOrLoadDialect<IR1Dialect>();
  Ctx.getOrLoadDialect<arith::ArithDialect>();
  Ctx.getOrLoadDialect<memref::MemRefDialect>();

  Module = ModuleOp::create(Builder.getUnknownLoc());
  Builder.setInsertionPointToStart(Module.getBody());
}

void IR1Context::verify() {
  if (failed(mlir::verify(Module)))
    Module.emitError("module verification error");
}

void IR1Context::convert(IR0Context& IR0Ctx) {
  for (auto &IR : IR0Ctx.IRs) {
    convertMCInst(IR);
  }
}

void IR1Context::print(bool withLoc) {
  OpPrintingFlags flags;
  flags.enableDebugInfo(withLoc);
  Module->print(dbgs(), flags);
}

