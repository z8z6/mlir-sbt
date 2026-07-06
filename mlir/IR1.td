#ifndef IR1
#define IR1

include "mlir/IR/DialectBase.td"
include "mlir/IR/AttrTypeBase.td"
include "mlir/IR/BuiltinTypes.td"
include "mlir/IR/BuiltinAttributes.td"
include "mlir/IR/OpBase.td"
include "mlir/IR/SymbolInterfaces.td"
include "mlir/Interfaces/FunctionInterfaces.td"
include "mlir/Interfaces/SideEffectInterfaces.td"


def IR1_Dialect : Dialect {
  let name = "ir1";
  let cppNamespace = "z8::ir1";
}

class IR1_Op<string mnemonic, list<Trait> traits = []> :
    Op<IR1_Dialect, mnemonic, traits>;

def ConstIntOp : IR1_Op<"ci"> {
  let arguments = (ins AnyAttr:$value);
  let results = (outs AnyInteger);
  let builders = [
    OpBuilder<(ins "mlir::IntegerAttr":$value), [{
      return build($_builder, $_state, value.getType(), value);
    }]>
  ];
}

def ConstFloatOp : IR1_Op<"cf"> {
  let arguments = (ins AnyAttr:$value);
  let results = (outs AnyFloat);
  let builders = [
    OpBuilder<(ins "mlir::FloatAttr":$value), [{
      return build($_builder, $_state, value.getType(), value);
    }]>
  ];
}

def LoadRegOp : IR1_Op<"lr"> {
  let arguments = (ins AnyInteger:$reg_id);
  let results = (outs AnyType);
}

def StoreRegOp : IR1_Op<"sr"> {
  let arguments = (ins AnyType:$value, AnyInteger:$reg_id);
}

def AddIOp : IR1_Op<"addi"> {
  let arguments = (ins AnyInteger:$lhs, AnyInteger:$rhs);
  let results = (outs AnyInteger);
}

def AddFOp : IR1_Op<"addf"> {
  let arguments = (ins AnyFloat:$lhs, AnyFloat:$rhs);
  let results = (outs AnyFloat);
}

def MovOp : IR1_Op<"mov"> {
  let arguments = (ins AnyType:$value);
  let results = (outs AnyType);
}

#endif // IR1
