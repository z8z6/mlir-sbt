//
// Created by zzm on 2026/7/11
// Part of RVision
//

#include "tblgen/X86IR1Converter.h"
#include "IR/IR1.h"
#include "mlir/IR/BuiltinTypes.h"

using namespace mlir;
using namespace z8;

namespace {
Value MemoryAddress(ConversionContext &CC, size_t offset = 0) {
  assert(CC.Src.size() >= offset + 5);
  static IR1Context *Ctx = &IR1Context::Instance();
  auto loc = CC.getNameLoc();
  auto i64 = Ctx->iTy();
  // LLVM's x86 memory operand is base + scale * index + displacement + segment.
  auto scaledIndex = ir1::MulIOp::create(Ctx->Builder, loc, i64,
                                         CC.Src[offset + 1],
                                         CC.Src[offset + 2]);
  auto address = ir1::AddIOp::create(Ctx->Builder, loc, i64,
                                     CC.Src[offset], scaledIndex);
  address = ir1::AddIOp::create(Ctx->Builder, loc, i64,
                                CC.Src[offset + 3], address);
  return ir1::AddIOp::create(Ctx->Builder, loc, i64,
                             CC.Src[offset + 4], address);
}

void AddOp(ConversionContext& CC, int width, IntegerType::SignednessSemantics signedness = IntegerType::Signless) {
  assert(CC.Src.size() >= 2);
  static IR1Context* Ctx = &IR1Context::Instance();
  auto loc = CC.getNameLoc();
  auto resTy = Ctx->iTy(width, signedness);
  auto i64 = Ctx->iTy();
  TypeRange type = {resTy, i64};
  ir1::X86AddIOp op;
  switch (CC.Src.size()) {
    case 2: {
      op = ir1::X86AddIOp::create(Ctx->Builder, loc, type, CC.Src[0], CC.Src[1]);
      break;
    }
    // addl fs:8(%rbx,%rsi,4), %eax
    //  0  + [ 1 + 2 * 3 + 4 + 5]
    // eax    rbx  4  rsi  8  fs
    case 6: {
      auto address = MemoryAddress(CC, 1);
      auto t4 = ir1::LoadOp::create(Ctx->Builder, loc, resTy, address);
      op = ir1::X86AddIOp::create(Ctx->Builder, loc, type, CC.Src[0],t4);
      break;
    }
    default:
      assert(false);
  }
  CC.Dst.push_back(op.getRes());
  CC.ImplicitDst.push_back(op.getFlags());
  CC.ImplicitOperand.emplace_back(llvm::X86::RFLAGS);
}

void AddMemoryDestinationOp(ConversionContext &CC, int width) {
  assert(CC.Src.size() == 6);
  static IR1Context *Ctx = &IR1Context::Instance();
  auto loc = CC.getNameLoc();
  auto resultType = Ctx->iTy(width);
  auto address = MemoryAddress(CC);
  auto lhs = ir1::LoadOp::create(Ctx->Builder, loc, resultType, address);
  TypeRange resultTypes{resultType, Ctx->iTy()};
  auto add = ir1::X86AddIOp::create(Ctx->Builder, loc, resultTypes, lhs,
                                    CC.Src[5]);
  ir1::StoreOp::create(Ctx->Builder, loc, add.getRes(), address);
  CC.ImplicitDst.push_back(add.getFlags());
  CC.ImplicitOperand.emplace_back(llvm::X86::RFLAGS);
}

void LoadAccumulator(ConversionContext &CC, unsigned reg) {
  auto loc = CC.getNameLoc();
  CC.Src.push_back(ir1::LoadRegOp::create(BaseIR1Converter::Ctx->Builder, loc,
                                          BaseIR1Converter::Ctx->getState(),
                                          reg));
}

void StoreAccumulator(ConversionContext &CC, unsigned reg) {
  auto loc = CC.getNameLoc();
  ir1::StoreRegOp::create(BaseIR1Converter::Ctx->Builder, loc,
                          BaseIR1Converter::Ctx->getState(), CC.Dst[0], reg);
  ir1::StoreRegOp::create(BaseIR1Converter::Ctx->Builder, loc,
                          BaseIR1Converter::Ctx->getState(),
                          CC.ImplicitDst[0], llvm::X86::RFLAGS);
}
}

// ADDrr
void X86_ADD64rr_IR1Converter::op(ConversionContext& CC) {
  AddOp(CC, 64);
}
void X86_ADD32rr_IR1Converter::op(ConversionContext& CC) {
  AddOp(CC, 32);
}
void X86_ADD16rr_IR1Converter::op(ConversionContext& CC) {
  AddOp(CC, 16);
}
void X86_ADD8rr_IR1Converter::op(ConversionContext& CC) {
  AddOp(CC, 8);
}

// ADDri
void X86_ADD64ri8_IR1Converter::op(ConversionContext& CC) {
  AddOp(CC, 64);
}
void X86_ADD32ri8_IR1Converter::op(ConversionContext& CC) {
  AddOp(CC, 32);
}
void X86_ADD16ri8_IR1Converter::op(ConversionContext& CC) {
  AddOp(CC, 16);
}
void X86_ADD8ri8_IR1Converter::op(ConversionContext& CC) {
  AddOp(CC, 8);
}

void X86_ADD8ri_IR1Converter::op(ConversionContext& CC) {
  AddOp(CC, 8);
}

// 只用于 al 寄存器，且无操作数定义
// addb	$1, %al
// <MCInst 681 <MCOperand Imm:1>>
void X86_ADD8i8_IR1Converter::loadSrcOperand(ConversionContext &CC) {
  BaseIR1Converter::loadSrcOperand(CC);
  LoadAccumulator(CC, llvm::X86::AL);
}
void X86_ADD8i8_IR1Converter::storeDstOperand(ConversionContext &CC) {
  StoreAccumulator(CC, llvm::X86::AL);
}
void X86_ADD8i8_IR1Converter::op(ConversionContext& CC) {
  AddOp(CC, 8);
}

#define DEFINE_ACCUMULATOR_ADD(WIDTH, OPCODE_SUFFIX, REGISTER)                \
  void X86_ADD##OPCODE_SUFFIX##_IR1Converter::loadSrcOperand(                 \
      ConversionContext &CC) {                                                \
    BaseIR1Converter::loadSrcOperand(CC);                                     \
    LoadAccumulator(CC, llvm::X86::REGISTER);                                \
  }                                                                           \
  void X86_ADD##OPCODE_SUFFIX##_IR1Converter::storeDstOperand(                \
      ConversionContext &CC) {                                                \
    StoreAccumulator(CC, llvm::X86::REGISTER);                               \
  }                                                                           \
  void X86_ADD##OPCODE_SUFFIX##_IR1Converter::op(ConversionContext &CC) {     \
    AddOp(CC, WIDTH);                                                         \
  }

DEFINE_ACCUMULATOR_ADD(16, 16i16, AX)
DEFINE_ACCUMULATOR_ADD(32, 32i32, EAX)
DEFINE_ACCUMULATOR_ADD(64, 64i32, RAX)

#undef DEFINE_ACCUMULATOR_ADD


// ADDrm
void X86_ADD64rm_IR1Converter::op(ConversionContext& CC) {
  AddOp(CC, 64);
}
void X86_ADD32rm_IR1Converter::op(ConversionContext& CC) {
  AddOp(CC, 32);
}
void X86_ADD16rm_IR1Converter::op(ConversionContext& CC) {
  AddOp(CC, 16);
}
void X86_ADD8rm_IR1Converter::op(ConversionContext& CC) {
  AddOp(CC, 8);
}

// ADDmr and ADDmi both use a memory destination. The sixth source is either
// the register or immediate added to the loaded memory value.
void X86_ADD64mr_IR1Converter::op(ConversionContext &CC) {
  AddMemoryDestinationOp(CC, 64);
}
void X86_ADD32mr_IR1Converter::op(ConversionContext &CC) {
  AddMemoryDestinationOp(CC, 32);
}
void X86_ADD16mr_IR1Converter::op(ConversionContext &CC) {
  AddMemoryDestinationOp(CC, 16);
}
void X86_ADD8mr_IR1Converter::op(ConversionContext &CC) {
  AddMemoryDestinationOp(CC, 8);
}
void X86_ADD64mi8_IR1Converter::op(ConversionContext &CC) {
  AddMemoryDestinationOp(CC, 64);
}
void X86_ADD32mi_IR1Converter::op(ConversionContext &CC) {
  AddMemoryDestinationOp(CC, 32);
}
void X86_ADD16mi_IR1Converter::op(ConversionContext &CC) {
  AddMemoryDestinationOp(CC, 16);
}
void X86_ADD8mi_IR1Converter::op(ConversionContext &CC) {
  AddMemoryDestinationOp(CC, 8);
}
