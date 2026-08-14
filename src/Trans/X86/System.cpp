#include "Trans/X86/System.h"

#include "IR/IR0.h"
#include "IR/IR1.h"
#include "IR/IR1Converter.h"
#include "IR/X86.h"
#include "tblgen/X86IR1Converter.h"

#include "Target/X86/MCTargetDesc/X86MCTargetDesc.h"

using namespace mlir;
using namespace z8;

void X86_SYSCALL_IR1Converter::loadSrcOperand(ConversionContext &) {}

void z8::translateSyscall(ConversionContext &context) {
  auto *ir = BaseIR1Converter::Ctx;
  Location loc = context.getNameLoc();
  auto read = [&](unsigned reg) -> Value {
    return x86ir::ReadRegOp::create(ir->Builder, loc, ir->getState(), reg);
  };

  Value number = read(llvm::X86::RAX);
  Value arg0 = read(llvm::X86::RDI);
  Value arg1 = read(llvm::X86::RSI);
  Value arg2 = read(llvm::X86::RDX);
  Value arg3 = read(llvm::X86::R10);
  Value arg4 = read(llvm::X86::R8);
  Value arg5 = read(llvm::X86::R9);
  Value flags = read(llvm::X86::RFLAGS);
  Value result =
      x86ir::SyscallOp::create(ir->Builder, loc, ir->Builder.getI64Type(),
                               number, arg0, arg1, arg2, arg3, arg4, arg5);

  auto write = [&](Value value, unsigned reg) {
    x86ir::WriteRegOp::create(ir->Builder, loc, ir->getState(), value, reg);
  };
  write(result, llvm::X86::RAX);
  Value nextRip = ir1::ConstIntOp::create(
      ir->Builder, loc,
      static_cast<int64_t>(context.IR.Addr + context.IR.Size));
  write(nextRip, llvm::X86::RCX);
  write(flags, llvm::X86::R11);
}

void X86_SYSCALL_IR1Converter::op(ConversionContext &context) {
  translateSyscall(context);
}
