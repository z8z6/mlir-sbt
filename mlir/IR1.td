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

def LoadStateOp : IR1_Op<"load_state"> {
  let arguments = (ins AnyType:$state, I32Attr:$slot, I32Attr:$bit_offset,
                       I32Attr:$storage_width);
  let results = (outs AnyInteger:$res);
  let assemblyFormat = "attr-dict $state `:` type($state) `->` type($res)";
}

def StoreStateOp : IR1_Op<"store_state"> {
  let arguments = (ins AnyType:$state, AnyInteger:$value, I32Attr:$slot,
                       I32Attr:$bit_offset, I32Attr:$storage_width,
                       AnyAttr:$write_mask);
  let assemblyFormat = "attr-dict $state `,` $value `:` type($state) `,` type($value)";
}

def LoadOp : IR1_Op<"load"> {
  let arguments = (ins AnyInteger:$addr);
  let results = (outs AnyType:$res);
}

def StoreOp : IR1_Op<"store"> {
  let arguments = (ins AnyType:$value, AnyInteger:$addr);
}

def AddIOp : IR1_Op<"addi"> {
  let arguments = (ins AnyInteger:$lhs, AnyInteger:$rhs);
  let results = (outs AnyInteger:$res);
  let assemblyFormat = "attr-dict `(`$lhs `:` type($lhs)`,` $rhs `:` type($rhs)`)` `:` type($res)";
}

class IR1_BinaryIOp<string mnemonic> : IR1_Op<mnemonic> {
  let arguments = (ins AnyInteger:$lhs, AnyInteger:$rhs);
  let results = (outs AnyInteger:$res);
  let assemblyFormat = "attr-dict `(`$lhs `:` type($lhs)`,` $rhs `:` type($rhs)`)` `:` type($res)";
}

def SubIOp  : IR1_BinaryIOp<"subi">;
def AndIOp  : IR1_BinaryIOp<"andi">;
def OrIOp   : IR1_BinaryIOp<"ori">;
def XOrIOp  : IR1_BinaryIOp<"xori">;
def ShRUIOp : IR1_BinaryIOp<"shrui">;
def ShLIOp  : IR1_BinaryIOp<"shli">;

def CmpIOp : IR1_Op<"cmpi"> {
  let arguments = (ins AnyInteger:$lhs, AnyInteger:$rhs, I32Attr:$predicate);
  let results = (outs I1:$res);
}

def CastIOp : IR1_Op<"casti"> {
  let arguments = (ins AnyInteger:$value);
  let results = (outs AnyInteger:$res);
}

def BitcastOp : IR1_Op<"bitcast"> {
  let arguments = (ins AnyType:$value);
  let results = (outs AnyType:$res);
}

def AddFOp : IR1_Op<"addf"> {
  let arguments = (ins AnyFloat:$lhs, AnyFloat:$rhs);
  let results = (outs AnyFloat:$res);
  let assemblyFormat = "attr-dict `(`$lhs `:` type($lhs)`,` $rhs `:` type($rhs)`)` `:` type($res)";
}

def SubFOp : IR1_Op<"subf"> {
  let arguments = (ins AnyFloat:$lhs, AnyFloat:$rhs);
  let results = (outs AnyFloat:$res);
  let assemblyFormat = "attr-dict `(`$lhs `:` type($lhs)`,` $rhs `:` type($rhs)`)` `:` type($res)";
}

def MulIOp : IR1_Op<"muli"> {
  let arguments = (ins AnyInteger:$lhs, AnyInteger:$rhs);
  let results = (outs AnyInteger:$res);
  let assemblyFormat = "attr-dict `(`$lhs `:` type($lhs)`,` $rhs `:` type($rhs)`)` `:` type($res)";
}

def MulFOp : IR1_Op<"mulf"> {
  let arguments = (ins AnyFloat:$lhs, AnyFloat:$rhs);
  let results = (outs AnyFloat:$res);
  let assemblyFormat = "attr-dict `(`$lhs `:` type($lhs)`,` $rhs `:` type($rhs)`)` `:` type($res)";
}

def DivIOp : IR1_Op<"divi"> {
  let arguments = (ins AnyInteger:$lhs, AnyInteger:$rhs);
  let results = (outs AnyInteger:$res);
  let assemblyFormat = "attr-dict `(`$lhs `:` type($lhs)`,` $rhs `:` type($rhs)`)` `:` type($res)";
}

def DivFOp : IR1_Op<"divf"> {
  let arguments = (ins AnyFloat:$lhs, AnyFloat:$rhs);
  let results = (outs AnyFloat:$res);
  let assemblyFormat = "attr-dict `(`$lhs `:` type($lhs)`,` $rhs `:` type($rhs)`)` `:` type($res)";
}

def MovOp : IR1_Op<"mov"> {
  let arguments = (ins AnyType:$value);
  let results = (outs AnyType);
}

#endif // IR1
