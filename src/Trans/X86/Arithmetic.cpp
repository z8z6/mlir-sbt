#include "Trans/X86/Arithmetic.h"
#include "IR/IR1.h"
#include "IR/IR1Converter.h"
#include "IR/X86.h"
#include "Target/X86/MCTargetDesc/X86MCTargetDesc.h"
#include "tblgen/X86IR1Converter.h"
#include "llvm/Support/ErrorHandling.h"
#include <cassert>

using namespace mlir;
using namespace z8;
using namespace z8::x86::trans;

// ---- shared helpers ----
namespace z8::x86::trans {
namespace {

struct ArithmeticResult {
  Value value;
  Value flags;
};

ArithmeticResult buildOperation(ConversionContext &context, unsigned width,
                                BinaryArithmeticKind kind, Value lhs,
                                Value rhs) {
  auto *ir = BaseIR1Converter::Ctx;
  TypeRange resultTypes{ir->iTy(width), ir->iTy()};
  Location loc = context.getNameLoc();
  switch (kind) {
  case BinaryArithmeticKind::Add: {
    auto op = x86ir::AddIOp::create(ir->Builder, loc, resultTypes, lhs, rhs);
    return {op.getRes(), op.getFlags()};
  }
  case BinaryArithmeticKind::Sub: {
    auto op = x86ir::SubIOp::create(ir->Builder, loc, resultTypes, lhs, rhs);
    return {op.getRes(), op.getFlags()};
  }
  }
  llvm_unreachable("unknown binary arithmetic kind");
}

void recordFlags(ConversionContext &context, Value flags) {
  context.ImplicitDst.push_back(flags);
  context.ImplicitOperand.emplace_back(llvm::X86::RFLAGS);
}

} // namespace

void translateBinaryRegisterDestination(ConversionContext &context,
                                        unsigned width,
                                        BinaryArithmeticKind kind) {
  assert(context.Src.size() == 2 || context.Src.size() == 6);
  auto *ir = BaseIR1Converter::Ctx;
  Value rhs = context.Src[1];
  if (context.Src.size() == 6)
    rhs = ir1::LoadOp::create(ir->Builder, context.getNameLoc(), ir->iTy(width),
                              buildMemoryAddress(context, 1));
  ArithmeticResult result =
      buildOperation(context, width, kind, context.Src[0], rhs);
  context.Dst.push_back(result.value);
  recordFlags(context, result.flags);
}

void translateBinaryMemoryDestination(ConversionContext &context,
                                      unsigned width,
                                      BinaryArithmeticKind kind) {
  assert(context.Src.size() == 6);
  auto *ir = BaseIR1Converter::Ctx;
  Location loc = context.getNameLoc();
  Value address = buildMemoryAddress(context);
  Value lhs = ir1::LoadOp::create(ir->Builder, loc, ir->iTy(width), address);
  ArithmeticResult result =
      buildOperation(context, width, kind, lhs, context.Src[5]);
  ir1::StoreOp::create(ir->Builder, loc, result.value, address);
  recordFlags(context, result.flags);
}

void translateBinaryArithmeticRegisterFlags(ConversionContext &context,
                                            unsigned width,
                                            BinaryArithmeticKind kind) {
  assert(context.Src.size() == 2 || context.Src.size() == 6);
  auto *ir = BaseIR1Converter::Ctx;
  Value rhs = context.Src[1];
  if (context.Src.size() == 6)
    rhs = ir1::LoadOp::create(ir->Builder, context.getNameLoc(), ir->iTy(width),
                              buildMemoryAddress(context, 1));
  recordFlags(context,
              buildOperation(context, width, kind, context.Src[0], rhs).flags);
}

void translateBinaryArithmeticMemoryFlags(ConversionContext &context,
                                          unsigned width,
                                          BinaryArithmeticKind kind) {
  assert(context.Src.size() == 6);
  auto *ir = BaseIR1Converter::Ctx;
  Value lhs = ir1::LoadOp::create(ir->Builder, context.getNameLoc(),
                                  ir->iTy(width), buildMemoryAddress(context));
  recordFlags(context,
              buildOperation(context, width, kind, lhs, context.Src[5]).flags);
}

void translateNegRegisterDestination(ConversionContext &context,
                                     unsigned width) {
  assert(context.Src.size() == 1);
  auto *ir = BaseIR1Converter::Ctx;
  Location loc = context.getNameLoc();
  Value zero = ir1::ConstIntOp::create(ir->Builder, loc, 0);
  ArithmeticResult result = buildOperation(
      context, width, BinaryArithmeticKind::Sub, zero, context.Src.front());
  context.Dst.push_back(result.value);
  recordFlags(context, result.flags);
}

void translateNegMemoryDestination(ConversionContext &context, unsigned width) {
  assert(context.Src.size() == 5);
  auto *ir = BaseIR1Converter::Ctx;
  Location loc = context.getNameLoc();
  Value address = buildMemoryAddress(context);
  Value value = ir1::LoadOp::create(ir->Builder, loc, ir->iTy(width), address);
  Value zero = ir1::ConstIntOp::create(ir->Builder, loc, 0);
  ArithmeticResult result =
      buildOperation(context, width, BinaryArithmeticKind::Sub, zero, value);
  ir1::StoreOp::create(ir->Builder, loc, result.value, address);
  recordFlags(context, result.flags);
}

} // namespace z8::x86::trans

// ---- NEG converters ----
#define DEFINE_NEG_REGISTER(SUFFIX, WIDTH)                                     \
  void X86_NEG##SUFFIX##_IR1Converter::op(ConversionContext &context) {        \
    translateNegRegisterDestination(context, WIDTH);                           \
  }

DEFINE_NEG_REGISTER(64r, 64)
DEFINE_NEG_REGISTER(32r, 32)
DEFINE_NEG_REGISTER(16r, 16)
DEFINE_NEG_REGISTER(8r, 8)

#undef DEFINE_NEG_REGISTER

#define DEFINE_NEG_MEMORY(SUFFIX, WIDTH)                                       \
  void X86_NEG##SUFFIX##_IR1Converter::op(ConversionContext &context) {        \
    translateNegMemoryDestination(context, WIDTH);                             \
  }

DEFINE_NEG_MEMORY(64m, 64)
DEFINE_NEG_MEMORY(32m, 32)
DEFINE_NEG_MEMORY(16m, 16)
DEFINE_NEG_MEMORY(8m, 8)

#undef DEFINE_NEG_MEMORY

// ---- ADD converters ----
#define DEFINE_ADD_REGISTER(SUFFIX, WIDTH)                                     \
  void X86_ADD##SUFFIX##_IR1Converter::op(ConversionContext &context) {        \
    translateBinaryRegisterDestination(context, WIDTH,                         \
                                       BinaryArithmeticKind::Add);             \
  }

DEFINE_ADD_REGISTER(64rr, 64)
DEFINE_ADD_REGISTER(32rr, 32)
DEFINE_ADD_REGISTER(16rr, 16)
DEFINE_ADD_REGISTER(8rr, 8)
DEFINE_ADD_REGISTER(64ri8, 64)
DEFINE_ADD_REGISTER(32ri8, 32)
DEFINE_ADD_REGISTER(16ri8, 16)
DEFINE_ADD_REGISTER(8ri8, 8)
DEFINE_ADD_REGISTER(8ri, 8)
DEFINE_ADD_REGISTER(64rm, 64)
DEFINE_ADD_REGISTER(32rm, 32)
DEFINE_ADD_REGISTER(16rm, 16)
DEFINE_ADD_REGISTER(8rm, 8)

#undef DEFINE_ADD_REGISTER

#define DEFINE_ADD_ACCUMULATOR(SUFFIX, WIDTH, REGISTER)                        \
  void X86_ADD##SUFFIX##_IR1Converter::loadSrcOperand(                         \
      ConversionContext &context) {                                            \
    loadAccumulatorImmediate(context, llvm::X86::REGISTER);                    \
  }                                                                            \
  void X86_ADD##SUFFIX##_IR1Converter::storeDstOperand(                        \
      ConversionContext &context) {                                            \
    storeAccumulatorResult(context, llvm::X86::REGISTER);                      \
  }                                                                            \
  void X86_ADD##SUFFIX##_IR1Converter::op(ConversionContext &context) {        \
    translateBinaryRegisterDestination(context, WIDTH,                         \
                                       BinaryArithmeticKind::Add);             \
  }

DEFINE_ADD_ACCUMULATOR(8i8, 8, AL)
DEFINE_ADD_ACCUMULATOR(16i16, 16, AX)
DEFINE_ADD_ACCUMULATOR(32i32, 32, EAX)
DEFINE_ADD_ACCUMULATOR(64i32, 64, RAX)

#undef DEFINE_ADD_ACCUMULATOR

#define DEFINE_ADD_MEMORY(SUFFIX, WIDTH)                                       \
  void X86_ADD##SUFFIX##_IR1Converter::op(ConversionContext &context) {        \
    translateBinaryMemoryDestination(context, WIDTH,                           \
                                     BinaryArithmeticKind::Add);               \
  }

DEFINE_ADD_MEMORY(64mr, 64)
DEFINE_ADD_MEMORY(32mr, 32)
DEFINE_ADD_MEMORY(16mr, 16)
DEFINE_ADD_MEMORY(8mr, 8)
DEFINE_ADD_MEMORY(64mi8, 64)
DEFINE_ADD_MEMORY(64mi32, 64)
DEFINE_ADD_MEMORY(32mi8, 32)
DEFINE_ADD_MEMORY(32mi, 32)
DEFINE_ADD_MEMORY(16mi8, 16)
DEFINE_ADD_MEMORY(16mi, 16)
DEFINE_ADD_MEMORY(8mi, 8)

#undef DEFINE_ADD_MEMORY

// ---- SUB converters ----
#define DEFINE_SUB_REGISTER(SUFFIX, WIDTH)                                     \
  void X86_SUB##SUFFIX##_IR1Converter::op(ConversionContext &context) {        \
    translateBinaryRegisterDestination(context, WIDTH,                         \
                                       BinaryArithmeticKind::Sub);             \
  }

DEFINE_SUB_REGISTER(64rr, 64)
DEFINE_SUB_REGISTER(32rr, 32)
DEFINE_SUB_REGISTER(16rr, 16)
DEFINE_SUB_REGISTER(8rr, 8)
DEFINE_SUB_REGISTER(64ri8, 64)
DEFINE_SUB_REGISTER(32ri8, 32)
DEFINE_SUB_REGISTER(16ri8, 16)
DEFINE_SUB_REGISTER(8ri8, 8)
DEFINE_SUB_REGISTER(8ri, 8)
DEFINE_SUB_REGISTER(16ri, 16)
DEFINE_SUB_REGISTER(32ri, 32)
DEFINE_SUB_REGISTER(64ri32, 64)
DEFINE_SUB_REGISTER(64rm, 64)
DEFINE_SUB_REGISTER(32rm, 32)
DEFINE_SUB_REGISTER(16rm, 16)
DEFINE_SUB_REGISTER(8rm, 8)

#undef DEFINE_SUB_REGISTER

#define DEFINE_SUB_ACCUMULATOR(SUFFIX, WIDTH, REGISTER)                        \
  void X86_SUB##SUFFIX##_IR1Converter::loadSrcOperand(                         \
      ConversionContext &context) {                                            \
    loadAccumulatorImmediate(context, llvm::X86::REGISTER);                    \
  }                                                                            \
  void X86_SUB##SUFFIX##_IR1Converter::storeDstOperand(                        \
      ConversionContext &context) {                                            \
    storeAccumulatorResult(context, llvm::X86::REGISTER);                      \
  }                                                                            \
  void X86_SUB##SUFFIX##_IR1Converter::op(ConversionContext &context) {        \
    translateBinaryRegisterDestination(context, WIDTH,                         \
                                       BinaryArithmeticKind::Sub);             \
  }

DEFINE_SUB_ACCUMULATOR(8i8, 8, AL)
DEFINE_SUB_ACCUMULATOR(16i16, 16, AX)
DEFINE_SUB_ACCUMULATOR(32i32, 32, EAX)
DEFINE_SUB_ACCUMULATOR(64i32, 64, RAX)

#undef DEFINE_SUB_ACCUMULATOR

#define DEFINE_SUB_MEMORY(SUFFIX, WIDTH)                                       \
  void X86_SUB##SUFFIX##_IR1Converter::op(ConversionContext &context) {        \
    translateBinaryMemoryDestination(context, WIDTH,                           \
                                     BinaryArithmeticKind::Sub);               \
  }

DEFINE_SUB_MEMORY(64mr, 64)
DEFINE_SUB_MEMORY(32mr, 32)
DEFINE_SUB_MEMORY(16mr, 16)
DEFINE_SUB_MEMORY(8mr, 8)
DEFINE_SUB_MEMORY(64mi8, 64)
DEFINE_SUB_MEMORY(32mi, 32)
DEFINE_SUB_MEMORY(16mi, 16)
DEFINE_SUB_MEMORY(8mi, 8)
DEFINE_SUB_MEMORY(64mi32, 64)

#undef DEFINE_SUB_MEMORY

// ---- CMP converters ----
#define CMP_REGISTER(SUFFIX, WIDTH)                                            \
  void X86_CMP##SUFFIX##_IR1Converter::op(ConversionContext &context) {        \
    translateBinaryArithmeticRegisterFlags(context, WIDTH,                     \
                                           BinaryArithmeticKind::Sub);         \
  }

CMP_REGISTER(64rr, 64)
CMP_REGISTER(32rr, 32)
CMP_REGISTER(16rr, 16)
CMP_REGISTER(8rr, 8)
CMP_REGISTER(64ri8, 64)
CMP_REGISTER(32ri8, 32)
CMP_REGISTER(16ri8, 16)
CMP_REGISTER(64ri32, 64)
CMP_REGISTER(32ri, 32)
CMP_REGISTER(16ri, 16)
CMP_REGISTER(8ri, 8)
CMP_REGISTER(64rm, 64)
CMP_REGISTER(32rm, 32)
CMP_REGISTER(16rm, 16)
CMP_REGISTER(8rm, 8)

#undef CMP_REGISTER

#define CMP_MEMORY(SUFFIX, WIDTH)                                              \
  void X86_CMP##SUFFIX##_IR1Converter::op(ConversionContext &context) {        \
    translateBinaryArithmeticMemoryFlags(context, WIDTH,                       \
                                         BinaryArithmeticKind::Sub);           \
  }

CMP_MEMORY(64mr, 64)
CMP_MEMORY(32mr, 32)
CMP_MEMORY(16mr, 16)
CMP_MEMORY(8mr, 8)
CMP_MEMORY(64mi8, 64)
CMP_MEMORY(64mi32, 64)
CMP_MEMORY(32mi8, 32)
CMP_MEMORY(32mi, 32)
CMP_MEMORY(16mi8, 16)
CMP_MEMORY(16mi, 16)
CMP_MEMORY(8mi, 8)

#undef CMP_MEMORY

#define CMP_ACCUMULATOR(SUFFIX, WIDTH, REGISTER)                               \
  void X86_CMP##SUFFIX##_IR1Converter::loadSrcOperand(                         \
      ConversionContext &context) {                                            \
    loadAccumulatorImmediate(context, llvm::X86::REGISTER);                    \
  }                                                                            \
  void X86_CMP##SUFFIX##_IR1Converter::op(ConversionContext &context) {        \
    translateBinaryArithmeticRegisterFlags(context, WIDTH,                     \
                                           BinaryArithmeticKind::Sub);         \
  }

CMP_ACCUMULATOR(8i8, 8, AL)
CMP_ACCUMULATOR(16i16, 16, AX)
CMP_ACCUMULATOR(32i32, 32, EAX)
CMP_ACCUMULATOR(64i32, 64, RAX)

#undef CMP_ACCUMULATOR
