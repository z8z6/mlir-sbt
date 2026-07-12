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
  assert(CC.Src.size() == 2);
  static IR1Context* Ctx = &IR1Context::Instance();
  auto loc = CC.getNameLoc();
  auto type = IntegerType::get(&Ctx->Ctx, width, signedness);
  auto v = ir1::AddIOp::create(Ctx->Builder, loc, type, CC.Src[0], CC.Src[1]);
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


// ADDrm
void X86_ADD32rm_IR1Converter::op(ConversionContext& CC) {
  AddOp(CC, 32);
}
