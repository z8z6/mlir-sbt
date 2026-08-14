#include "Trans/X86/ControlFlow.h"
#include "IR/IR0.h"
#include "IR/IR1.h"
#include "IR/IR1Converter.h"
#include "IR/X86.h"
#include "Target/X86/MCTargetDesc/X86MCTargetDesc.h"
#include "Trans/X86/Common.h"
#include "mlir/Dialect/ControlFlow/IR/ControlFlowOps.h"
#include "tblgen/X86IR1Converter.h"

using namespace mlir;
using namespace z8;

// ---- direct JMP/Jcc converters ----
#define EMPTY_BRANCH_OPERAND_LOAD(OPCODE)                                      \
  void X86_##OPCODE##_IR1Converter::loadSrcOperand(ConversionContext &) {}

EMPTY_BRANCH_OPERAND_LOAD(JMP_1)
EMPTY_BRANCH_OPERAND_LOAD(JMP_4)
EMPTY_BRANCH_OPERAND_LOAD(JCC_1)
EMPTY_BRANCH_OPERAND_LOAD(JCC_4)

#undef EMPTY_BRANCH_OPERAND_LOAD

void z8::translateDirectJump(ConversionContext &context) {
  if (!context.BranchTarget) {
    BaseIR1Converter::Ctx->markConversionFailure();
    return;
  }
  cf::BranchOp::create(BaseIR1Converter::Ctx->Builder, context.getNameLoc(),
                       context.BranchTarget);
}

void z8::translateConditionalJump(ConversionContext &context) {
  if (!context.BranchTarget || !context.Fallthrough ||
      context.IR.Inst.getNumOperands() < 2 ||
      !context.IR.Inst.getOperand(1).isImm()) {
    BaseIR1Converter::Ctx->markConversionFailure();
    return;
  }
  auto *ir = BaseIR1Converter::Ctx;
  Location loc = context.getNameLoc();
  Value flags = x86ir::ReadRegOp::create(ir->Builder, loc, ir->getState(),
                                         llvm::X86::RFLAGS);
  auto condition = ir->Builder.getI32IntegerAttr(
      static_cast<int32_t>(context.IR.Inst.getOperand(1).getImm()));
  Value take = x86ir::ConditionOp::create(
      ir->Builder, loc, ir->Builder.getI1Type(), flags, condition);
  cf::CondBranchOp::create(ir->Builder, loc, take, context.BranchTarget,
                           context.Fallthrough);
}

void X86_JMP_1_IR1Converter::op(ConversionContext &context) {
  translateDirectJump(context);
}
void X86_JMP_4_IR1Converter::op(ConversionContext &context) {
  translateDirectJump(context);
}
void X86_JCC_1_IR1Converter::op(ConversionContext &context) {
  translateConditionalJump(context);
}
void X86_JCC_4_IR1Converter::op(ConversionContext &context) {
  translateConditionalJump(context);
}

// ---- CALL converters ----
void X86_CALL64pcrel32_IR1Converter::loadSrcOperand(ConversionContext &) {}

void X86_CALL64pcrel32_IR1Converter::op(ConversionContext &context) {
  if (!context.IR.ExternalSymbol) {
    BaseIR1Converter::Ctx->markConversionFailure();
    return;
  }
  auto *ir = BaseIR1Converter::Ctx;
  Location loc = context.getNameLoc();
  auto read = [&](unsigned reg) -> Value {
    return x86ir::ReadRegOp::create(ir->Builder, loc, ir->getState(), reg);
  };
  Value result = x86ir::ExternalCallOp::create(
      ir->Builder, loc, ir->Builder.getI64Type(), read(llvm::X86::RDI),
      read(llvm::X86::RSI), read(llvm::X86::RDX), read(llvm::X86::RCX),
      read(llvm::X86::R8), read(llvm::X86::R9),
      ir->Builder.getStringAttr(*context.IR.ExternalSymbol));
  x86ir::WriteRegOp::create(ir->Builder, loc, ir->getState(), result,
                            llvm::X86::RAX);
}

namespace {
void translateIndirectCall(ConversionContext &context, bool memory) {
  auto *ir = BaseIR1Converter::Ctx;
  Location loc = context.getNameLoc();
  Value callee;
  if (memory) {
    if (context.Src.size() != 5) {
      ir->markConversionFailure();
      return;
    }
    Value address = x86::trans::buildMemoryAddress(context);
    callee = ir1::LoadOp::create(ir->Builder, loc, ir->iTy(), address);
  } else {
    if (context.Src.size() != 1) {
      ir->markConversionFailure();
      return;
    }
    callee = context.Src.front();
  }
  auto read = [&](unsigned reg) -> Value {
    return x86ir::ReadRegOp::create(ir->Builder, loc, ir->getState(), reg);
  };
  Value result = x86ir::IndirectCallOp::create(
      ir->Builder, loc, ir->Builder.getI64Type(), callee, read(llvm::X86::RDI),
      read(llvm::X86::RSI), read(llvm::X86::RDX), read(llvm::X86::RCX),
      read(llvm::X86::R8), read(llvm::X86::R9));
  x86ir::WriteRegOp::create(ir->Builder, loc, ir->getState(), result,
                            llvm::X86::RAX);
}
} // namespace

void X86_CALL64r_IR1Converter::op(ConversionContext &context) {
  translateIndirectCall(context, false);
}

void X86_CALL64m_IR1Converter::op(ConversionContext &context) {
  translateIndirectCall(context, true);
}
