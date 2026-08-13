//
// Created by zzm on 2026/6/30
// Part of RVision
//

#pragma once

#include "mlir/IR/BuiltinTypes.h"
#include "mlir/IR/Dialect.h"
#include "mlir/IR/Value.h"
#include "mlir/IR/Builders.h"
#include "mlir/IR/MLIRContext.h"
#include "mlir/IR/OwningOpRef.h"
#include "mlir/IR/SymbolTable.h"
#include "mlir/IR/BuiltinOps.h"
#include "mlir/Dialect/Func/IR/FuncOps.h"
#include "llvm/ADT/StringRef.h"

#include "mlir/Interfaces/FunctionInterfaces.h"
#include "mlir/Interfaces/SideEffectInterfaces.h"

#include "mlir/IR1Dialect.h.inc"

#define GET_OP_CLASSES
#include "mlir/IR1Ops.h.inc"

namespace mlir {
class MLIRContext;
template <typename OpTy>
class OwningOpRef;
class ModuleOp;
}

namespace llvm {
class MCOperand;
}

namespace z8 {
class IR0Context;

class IR1Context {
public:
  mlir::MLIRContext Ctx;
  mlir::ModuleOp Module;
  mlir::OpBuilder Builder;
  mlir::func::FuncOp Function;
  mlir::Value State;

  IR1Context();
  mlir::LogicalResult convert(IR0Context&);
  void markConversionFailure() { ConversionFailed = true; }
  bool hasConversionFailed() const { return ConversionFailed; }
  void print(bool withLoc = false);
  void verify();
  mlir::LogicalResult lower();
  mlir::LogicalResult emitObject(llvm::StringRef outputPath);
  mlir::Value getState() const { return State; }
  mlir::Type iTy(int width = 64, mlir::IntegerType::SignednessSemantics signedness = mlir::IntegerType::Signless);

  static IR1Context& Instance() {
    static IR1Context instance;
    return instance;
  }

private:
  bool ConversionFailed = false;
};
}
