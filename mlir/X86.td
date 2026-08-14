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

def ConditionOp : X86_Op<"condition"> {
  let arguments = (ins I64:$flags, I32Attr:$condition);
  let results = (outs I1:$res);
  let assemblyFormat = "attr-dict `(`$flags `:` type($flags)`)` `:` type($res)";
  let hasVerifier = 1;
}

// A Linux x86-64 syscall round trip. The operands follow the kernel ABI:
// number, RDI, RSI, RDX, R10, R8 and R9. RCX/R11 architectural clobbers are
// made explicit by the instruction converter rather than hidden in this op.
def SyscallOp : X86_Op<"syscall"> {
  let arguments = (ins I64:$number, I64:$arg0, I64:$arg1, I64:$arg2,
                       I64:$arg3, I64:$arg4, I64:$arg5);
  let results = (outs I64:$res);
}

// A returning SysV x86-64 external call. The source CALL converter resolves
// a PLT target to its dynamic symbol before creating this operation.
def ExternalCallOp : X86_Op<"external_call"> {
  let arguments = (ins I64:$arg0, I64:$arg1, I64:$arg2,
                       I64:$arg3, I64:$arg4, I64:$arg5,
                       StrAttr:$callee);
  let results = (outs I64:$res);
}

// A returning SysV x86-64 call through a host function pointer. The converter
// obtains the target from the CALL r/m64 operand and makes guest argument
// registers explicit.
def IndirectCallOp : X86_Op<"indirect_call"> {
  let arguments = (ins I64:$callee, I64:$arg0, I64:$arg1, I64:$arg2,
                       I64:$arg3, I64:$arg4, I64:$arg5);
  let results = (outs I64:$res);
}

def ScalarFOp : X86_Op<"scalar_f"> {
  let arguments = (ins AnyInteger:$lhs, AnyInteger:$rhs, I32Attr:$kind,
                       I32Attr:$width);
  let results = (outs AnyInteger:$res);
  let assemblyFormat = "attr-dict `(`$lhs `:` type($lhs)`,` $rhs `:` type($rhs)`)` `:` type($res)";
}

// Legacy SSE CVT semantics over the low scalar/packed lanes of an XMM value.
// kind: 0 signed-int-to-float, 1 float-to-signed-int, 2 float extension,
// 3 float truncation. round: 0 conversion default, 1 nearest-even, 2 truncate.
def ConvertOp : X86_Op<"convert"> {
  let arguments = (ins AnyInteger:$source, AnyInteger:$old_dest,
                       I32Attr:$kind, I32Attr:$src_width,
                       I32Attr:$dst_width, I32Attr:$lanes,
                       I32Attr:$round, I32Attr:$preserve_upper);
  let results = (outs AnyInteger:$res);
}

def ReadX87Op : X86_Op<"read_x87"> {
  let arguments = (ins AnyType:$state, I32Attr:$index);
  let results = (outs AnyInteger:$res);
}

def WriteX87Op : X86_Op<"write_x87"> {
  let arguments = (ins AnyType:$state, AnyInteger:$value, I32Attr:$index);
}

def PopX87Op : X86_Op<"pop_x87"> {
  let arguments = (ins AnyType:$state);
}

// kind: 0 add, 1 subtract, 2 multiply, 3 divide. Operands and result are the
// raw 80-bit x87 extended-precision representation.
def X87BinaryOp : X86_Op<"x87_binary"> {
  let arguments = (ins AnyInteger:$lhs, AnyInteger:$rhs, I32Attr:$kind);
  let results = (outs AnyInteger:$res);
}

#endif
