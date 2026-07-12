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
  let results = (outs AnyInteger:$res);
  let builders = [
    OpBuilder<(ins "int64_t":$value)>
  ];
  let assemblyFormat = "attr-dict `:` type($res)";
}

def ConstFloatOp : IR1_Op<"cf"> {
  let arguments = (ins AnyAttr:$value);
  let results = (outs AnyFloat:$res);
  let assemblyFormat = "attr-dict `:` type($res)";
}

def LoadRegOp : IR1_Op<"load_reg"> {
  let arguments = (ins AnyAttr:$reg_id);
  let results = (outs AnyType:$res);
  let builders = [
    OpBuilder<(ins "unsigned":$reg_id)>
  ];
  let assemblyFormat = "attr-dict `:` type($res)";
}

def StoreRegOp : IR1_Op<"store_reg"> {
  let arguments = (ins AnyType:$value, AnyAttr:$reg_id);
  let builders = [
    OpBuilder<(ins "mlir::Value":$value, "unsigned":$reg_id)>
  ];
  let assemblyFormat = "attr-dict $value `:` type($value)";
}

def AddIOp : IR1_Op<"addi"> {
  let arguments = (ins AnyInteger:$lhs, AnyInteger:$rhs);
  let results = (outs AnyInteger:$res);
  let assemblyFormat = "attr-dict `(`$lhs `:` type($lhs)`,` $rhs `:` type($rhs)`)` `:` type($res)";
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
