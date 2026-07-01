//
// Created by zzm on 2026/6/30
// Part of RVision
//

#pragma once

#include "mlir/Bytecode/BytecodeOpInterface.h"
#include "mlir/IR/Dialect.h"
#include "mlir/IR/Value.h"
#include "mlir/IR/Builders.h"
#include "mlir/IR/MLIRContext.h"
#include "mlir/IR/OwningOpRef.h"
#include "mlir/IR/SymbolTable.h"
#include "mlir/IR/BuiltinOps.h"
#include "mlir/Interfaces/CallInterfaces.h"
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
  mlir::ModuleOp Module; // 持有 MLIR 模块
  mlir::OpBuilder Builder;            // 用于创建 IR

  IR1Context();
  void transform(IR0Context&);
  void print();
  void verify();

private:
  mlir::Value imm();
  mlir::Value load();
  void store();

};
}