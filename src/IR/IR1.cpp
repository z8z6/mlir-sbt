//
// Created by zzm on 2026/6/30
// Part of RVision
//
#include "IR/IR1.h"
#include "IR/IR0.h"
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

void AddOp::build(OpBuilder &builder, OperationState &state,
                  Value lhs, Value rhs) {
  state.addOperands({lhs, rhs});
}

#define GET_OP_CLASSES
#include "mlir/IR1Ops.cpp.inc"


IR1Context::IR1Context() : Builder(&Ctx)  {
  Ctx.getOrLoadDialect<IR1Dialect>();
  Ctx.getOrLoadDialect<arith::ArithDialect>();
  Ctx.getOrLoadDialect<memref::MemRefDialect>();

  Module = ModuleOp::create(Builder.getUnknownLoc());
  Builder = OpBuilder(Module.getBody(), Module.getBody()->end());
}

void IR1Context::verify() {
  if (failed(mlir::verify(Module)))
    Module.emitError("module verification error");
}


Value IR1Context::imm() {
  auto loc = Builder.getUnknownLoc();
  Type Ty = Builder.getI32Type();
  return arith::ConstantOp::create(Builder, loc, Ty, Builder.getIntegerAttr(Ty, 1));
}

Value IR1Context::load() {
  auto loc = Builder.getUnknownLoc();
  Value idx = arith::ConstantIndexOp::create(Builder, loc, 5);
  Value Array;
  return memref::LoadOp::create(Builder, loc, Array, idx);
}

void IR1Context::store() {
  auto loc = Builder.getUnknownLoc();
  Value idx = arith::ConstantIndexOp::create(Builder, loc, 5);
  Value Array;
  memref::StoreOp::create(Builder, loc, Array, idx);
}

void IR1Context::transform(IR0Context& IR0Ctx) {
  for (auto &IR : IR0Ctx.IRs) {
    auto op = IR.Inst.getOpcode();
    auto addr = IR.Addr;
    auto loc = Builder.getUnknownLoc();
    auto AddOp = arith::AddIOp::create(Builder, loc, imm(), imm());
  }
}

void IR1Context::print() {
  Module.dump();
}

bool translateMCInst(const llvm::MCInst& Inst, mlir::Location Loc);