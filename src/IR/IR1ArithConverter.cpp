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
void AddOp(ConversionContext& CC, int width, IntegerType::SignednessSemantics signedness = IntegerType::Signless) {
  assert(CC.Src.size() >= 2);
  static IR1Context* Ctx = &IR1Context::Instance();
  auto loc = CC.getNameLoc();
  auto type = Ctx->iTy(width, signedness);
  Value v;
  switch (CC.Src.size()) {
    case 2: {
      v = ir1::AddIOp::create(Ctx->Builder, loc, type, CC.Src[0], CC.Src[1]);
      break;
    }
    // addl fs:8(%rbx,%rsi,4), %eax
    //  0  + [ 1 + 2 * 3 + 4 + 5]
    // eax    rbx  4  rsi  8  fs
    case 6: {
      auto i64 = Ctx->iTy();
      auto t0 = ir1::MulIOp::create(Ctx->Builder, loc, i64, CC.Src[2], CC.Src[3]);
      auto t1 = ir1::AddIOp::create(Ctx->Builder, loc, i64, CC.Src[1], t0);
      auto t2 = ir1::AddIOp::create(Ctx->Builder, loc, i64, CC.Src[4], t1);
      auto t3 = ir1::AddIOp::create(Ctx->Builder, loc, i64, CC.Src[5], t2);
      auto t4 = ir1::LoadOp::create(Ctx->Builder, loc, type, t3);
      v = ir1::AddIOp::create(Ctx->Builder, loc, type, CC.Src[0],t4);
      break;
    }
    default:
      assert(false);
  }
  CC.Dst.push_back(v);
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
  auto loc = CC.getNameLoc();
  auto v = ir1::LoadRegOp::create(Ctx->Builder, loc, llvm::X86::AL);
  CC.Src.push_back(v);
}
void X86_ADD8i8_IR1Converter::storeDstOperand(ConversionContext &CC) {
  auto loc = CC.getNameLoc();
  ir1::StoreRegOp::create(Ctx->Builder, loc, CC.Dst[0], llvm::X86::AL);
}
void X86_ADD8i8_IR1Converter::op(ConversionContext& CC) {
  AddOp(CC, 8);
}


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
