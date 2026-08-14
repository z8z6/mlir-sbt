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

// Architecture-neutral host syscall boundary. The x86 lowering supplies the
// Linux x86-64 register mapping; the LLVM lowering calls __sbt_syscall6.
def SyscallOp : IR1_Op<"syscall"> {
  let arguments = (ins I64:$number, I64:$arg0, I64:$arg1, I64:$arg2,
                       I64:$arg3, I64:$arg4, I64:$arg5);
  let results = (outs I64:$res);
}

def ExternalCallOp : IR1_Op<"external_call"> {
  let arguments = (ins I64:$arg0, I64:$arg1, I64:$arg2,
                       I64:$arg3, I64:$arg4, I64:$arg5,
                       StrAttr:$callee);
  let results = (outs I64:$res);
}

def IndirectCallOp : IR1_Op<"indirect_call"> {
  let arguments = (ins I64:$callee, I64:$arg0, I64:$arg1, I64:$arg2,
                       I64:$arg3, I64:$arg4, I64:$arg5);
  let results = (outs I64:$res);
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

def ExtSIOp : IR1_Op<"extsi"> {
  let arguments = (ins AnyInteger:$value);
  let results = (outs AnyInteger:$res);
}

def BitcastOp : IR1_Op<"bitcast"> {
  let arguments = (ins AnyType:$value);
  let results = (outs AnyType:$res);
}

def SIToFPOp : IR1_Op<"sitofp"> {
  let arguments = (ins AnyType:$value);
  let results = (outs AnyType:$res);
}

def FPToSIOp : IR1_Op<"fptosi"> {
  let arguments = (ins AnyType:$value);
  let results = (outs AnyType:$res);
}

def ExtFOp : IR1_Op<"extf"> {
  let arguments = (ins AnyType:$value);
  let results = (outs AnyType:$res);
}

def TruncFOp : IR1_Op<"truncf"> {
  let arguments = (ins AnyType:$value);
  let results = (outs AnyType:$res);
}

def RoundEvenFOp : IR1_Op<"roundevenf"> {
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
