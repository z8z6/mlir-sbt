#ifndef X86_DIALECT
#define X86_DIALECT

include "mlir/IR/DialectBase.td"
include "mlir/IR/BuiltinTypes.td"
include "mlir/IR/OpBase.td"

def X86_Dialect : Dialect {
  let name = "x86";
  let cppNamespace = "z8::x86ir";
}

class X86_Op<string mnemonic> : Op<X86_Dialect, mnemonic>;

def ReadRegOp : X86_Op<"read_reg"> {
  let arguments = (ins AnyType:$state, I32Attr:$reg_id);
  let results = (outs AnyInteger:$res);
  let builders = [
    OpBuilder<(ins "mlir::Value":$state, "unsigned":$reg_id)>
  ];
}

def WriteRegOp : X86_Op<"write_reg"> {
  let arguments = (ins AnyType:$state, AnyInteger:$value, I32Attr:$reg_id);
}

class X86_BinaryFlagsOp<string mnemonic> : X86_Op<mnemonic> {
  let arguments = (ins AnyInteger:$lhs, AnyInteger:$rhs);
  let results = (outs AnyInteger:$res, I64:$flags);
  let assemblyFormat = "attr-dict `(`$lhs `:` type($lhs)`,` $rhs `:` type($rhs)`)` `:` type($res)";
}

def AddIOp : X86_BinaryFlagsOp<"addi">;
def SubIOp : X86_BinaryFlagsOp<"subi">;
def AndIOp : X86_BinaryFlagsOp<"andi">;
def OrIOp  : X86_BinaryFlagsOp<"ori">;
def XorIOp : X86_BinaryFlagsOp<"xori">;

def ScalarFOp : X86_Op<"scalar_f"> {
  let arguments = (ins AnyInteger:$lhs, AnyInteger:$rhs, I32Attr:$kind,
                       I32Attr:$width);
  let results = (outs AnyInteger:$res);
  let assemblyFormat = "attr-dict `(`$lhs `:` type($lhs)`,` $rhs `:` type($rhs)`)` `:` type($res)";
}

#endif
