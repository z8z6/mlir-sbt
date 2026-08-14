#include "Trans/X86/Logical.h"
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

struct LogicalResult {
  Value value;
  Value flags;
};

LogicalResult buildOperation(ConversionContext &context, unsigned width,
                             BinaryLogicalKind kind, Value lhs, Value rhs) {
  auto *ir = BaseIR1Converter::Ctx;
  TypeRange resultTypes{ir->iTy(width), ir->iTy()};
  Location loc = context.getNameLoc();
  switch (kind) {
  case BinaryLogicalKind::And: {
    auto op = x86ir::AndIOp::create(ir->Builder, loc, resultTypes, lhs, rhs);
    return {op.getRes(), op.getFlags()};
  }
  case BinaryLogicalKind::Or: {
    auto op = x86ir::OrIOp::create(ir->Builder, loc, resultTypes, lhs, rhs);
    return {op.getRes(), op.getFlags()};
  }
  case BinaryLogicalKind::Xor: {
    auto op = x86ir::XorIOp::create(ir->Builder, loc, resultTypes, lhs, rhs);
    return {op.getRes(), op.getFlags()};
  }
  }
  llvm_unreachable("unknown binary logical kind");
}

void recordFlags(ConversionContext &context, Value flags) {
  context.ImplicitDst.push_back(flags);
  context.ImplicitOperand.emplace_back(llvm::X86::RFLAGS);
}

} // namespace

void translateBinaryLogicalRegisterDestination(ConversionContext &context,
                                               unsigned width,
                                               BinaryLogicalKind kind) {
  assert(context.Src.size() == 2 || context.Src.size() == 6);
  auto *ir = BaseIR1Converter::Ctx;
  Value rhs = context.Src[1];
  if (context.Src.size() == 6)
    rhs = ir1::LoadOp::create(ir->Builder, context.getNameLoc(), ir->iTy(width),
                              buildMemoryAddress(context, 1));
  LogicalResult result =
      buildOperation(context, width, kind, context.Src[0], rhs);
  context.Dst.push_back(result.value);
  recordFlags(context, result.flags);
}

void translateBinaryLogicalMemoryDestination(ConversionContext &context,
                                             unsigned width,
                                             BinaryLogicalKind kind) {
  assert(context.Src.size() == 6);
  auto *ir = BaseIR1Converter::Ctx;
  Location loc = context.getNameLoc();
  Value address = buildMemoryAddress(context);
  Value lhs = ir1::LoadOp::create(ir->Builder, loc, ir->iTy(width), address);
  LogicalResult result =
      buildOperation(context, width, kind, lhs, context.Src[5]);
  ir1::StoreOp::create(ir->Builder, loc, result.value, address);
  recordFlags(context, result.flags);
}

void translateBinaryLogicalRegisterFlags(ConversionContext &context,
                                         unsigned width,
                                         BinaryLogicalKind kind) {
  assert(context.Src.size() == 2 || context.Src.size() == 6);
  auto *ir = BaseIR1Converter::Ctx;
  Value rhs = context.Src[1];
  if (context.Src.size() == 6)
    rhs = ir1::LoadOp::create(ir->Builder, context.getNameLoc(), ir->iTy(width),
                              buildMemoryAddress(context, 1));
  recordFlags(context,
              buildOperation(context, width, kind, context.Src[0], rhs).flags);
}

void translateBinaryLogicalMemoryFlags(ConversionContext &context,
                                       unsigned width, BinaryLogicalKind kind) {
  assert(context.Src.size() == 6);
  auto *ir = BaseIR1Converter::Ctx;
  Value lhs = ir1::LoadOp::create(ir->Builder, context.getNameLoc(),
                                  ir->iTy(width), buildMemoryAddress(context));
  recordFlags(context,
              buildOperation(context, width, kind, lhs, context.Src[5]).flags);
}

Value buildNot(ConversionContext &context, unsigned width, Value value) {
  auto *ir = BaseIR1Converter::Ctx;
  Location loc = context.getNameLoc();
  Value allOnes = ir1::ConstIntOp::create(ir->Builder, loc, -1);
  return ir1::XOrIOp::create(ir->Builder, loc, ir->iTy(width), value, allOnes);
}

void translateNotRegisterDestination(ConversionContext &context,
                                     unsigned width) {
  assert(context.Src.size() == 1);
  context.Dst.push_back(buildNot(context, width, context.Src.front()));
}

void translateNotMemoryDestination(ConversionContext &context, unsigned width) {
  assert(context.Src.size() == 5);
  auto *ir = BaseIR1Converter::Ctx;
  Location loc = context.getNameLoc();
  Value address = buildMemoryAddress(context);
  Value value = ir1::LoadOp::create(ir->Builder, loc, ir->iTy(width), address);
  ir1::StoreOp::create(ir->Builder, loc, buildNot(context, width, value),
                       address);
}

} // namespace z8::x86::trans

// ---- NOT converters ----
#define DEFINE_NOT_REGISTER(SUFFIX, WIDTH)                                     \
  void X86_NOT##SUFFIX##_IR1Converter::op(ConversionContext &context) {        \
    translateNotRegisterDestination(context, WIDTH);                           \
  }

DEFINE_NOT_REGISTER(64r, 64)
DEFINE_NOT_REGISTER(32r, 32)
DEFINE_NOT_REGISTER(16r, 16)
DEFINE_NOT_REGISTER(8r, 8)

#undef DEFINE_NOT_REGISTER

#define DEFINE_NOT_MEMORY(SUFFIX, WIDTH)                                       \
  void X86_NOT##SUFFIX##_IR1Converter::op(ConversionContext &context) {        \
    translateNotMemoryDestination(context, WIDTH);                             \
  }

DEFINE_NOT_MEMORY(64m, 64)
DEFINE_NOT_MEMORY(32m, 32)
DEFINE_NOT_MEMORY(16m, 16)
DEFINE_NOT_MEMORY(8m, 8)

#undef DEFINE_NOT_MEMORY

// ---- AND converters ----
#define DEFINE_AND_REGISTER(SUFFIX, WIDTH)                                     \
  void X86_AND##SUFFIX##_IR1Converter::op(ConversionContext &context) {        \
    translateBinaryLogicalRegisterDestination(context, WIDTH,                  \
                                              BinaryLogicalKind::And);         \
  }

DEFINE_AND_REGISTER(64rr, 64)
DEFINE_AND_REGISTER(32rr, 32)
DEFINE_AND_REGISTER(16rr, 16)
DEFINE_AND_REGISTER(8rr, 8)
DEFINE_AND_REGISTER(64ri8, 64)
DEFINE_AND_REGISTER(32ri8, 32)
DEFINE_AND_REGISTER(16ri8, 16)
DEFINE_AND_REGISTER(8ri8, 8)
DEFINE_AND_REGISTER(64ri32, 64)
DEFINE_AND_REGISTER(32ri, 32)
DEFINE_AND_REGISTER(16ri, 16)
DEFINE_AND_REGISTER(8ri, 8)
DEFINE_AND_REGISTER(64rm, 64)
DEFINE_AND_REGISTER(32rm, 32)
DEFINE_AND_REGISTER(16rm, 16)
DEFINE_AND_REGISTER(8rm, 8)

#undef DEFINE_AND_REGISTER

#define DEFINE_AND_ACCUMULATOR(SUFFIX, WIDTH, REGISTER)                        \
  void X86_AND##SUFFIX##_IR1Converter::loadSrcOperand(                         \
      ConversionContext &context) {                                            \
    loadAccumulatorImmediate(context, llvm::X86::REGISTER);                    \
  }                                                                            \
  void X86_AND##SUFFIX##_IR1Converter::storeDstOperand(                        \
      ConversionContext &context) {                                            \
    storeAccumulatorResult(context, llvm::X86::REGISTER);                      \
  }                                                                            \
  void X86_AND##SUFFIX##_IR1Converter::op(ConversionContext &context) {        \
    translateBinaryLogicalRegisterDestination(context, WIDTH,                  \
                                              BinaryLogicalKind::And);         \
  }

DEFINE_AND_ACCUMULATOR(8i8, 8, AL)
DEFINE_AND_ACCUMULATOR(16i16, 16, AX)
DEFINE_AND_ACCUMULATOR(32i32, 32, EAX)
DEFINE_AND_ACCUMULATOR(64i32, 64, RAX)

#undef DEFINE_AND_ACCUMULATOR

#define DEFINE_AND_MEMORY(SUFFIX, WIDTH)                                       \
  void X86_AND##SUFFIX##_IR1Converter::op(ConversionContext &context) {        \
    translateBinaryLogicalMemoryDestination(context, WIDTH,                    \
                                            BinaryLogicalKind::And);           \
  }

DEFINE_AND_MEMORY(64mr, 64)
DEFINE_AND_MEMORY(32mr, 32)
DEFINE_AND_MEMORY(16mr, 16)
DEFINE_AND_MEMORY(8mr, 8)
DEFINE_AND_MEMORY(64mi8, 64)
DEFINE_AND_MEMORY(64mi32, 64)
DEFINE_AND_MEMORY(32mi, 32)
DEFINE_AND_MEMORY(16mi, 16)
DEFINE_AND_MEMORY(8mi, 8)

#undef DEFINE_AND_MEMORY

// ---- OR converters ----
#define DEFINE_OR_REGISTER(SUFFIX, WIDTH)                                      \
  void X86_OR##SUFFIX##_IR1Converter::op(ConversionContext &context) {         \
    translateBinaryLogicalRegisterDestination(context, WIDTH,                  \
                                              BinaryLogicalKind::Or);          \
  }

DEFINE_OR_REGISTER(64rr, 64)
DEFINE_OR_REGISTER(32rr, 32)
DEFINE_OR_REGISTER(16rr, 16)
DEFINE_OR_REGISTER(8rr, 8)
DEFINE_OR_REGISTER(64ri8, 64)
DEFINE_OR_REGISTER(32ri8, 32)
DEFINE_OR_REGISTER(16ri8, 16)
DEFINE_OR_REGISTER(8ri8, 8)
DEFINE_OR_REGISTER(64ri32, 64)
DEFINE_OR_REGISTER(32ri, 32)
DEFINE_OR_REGISTER(16ri, 16)
DEFINE_OR_REGISTER(8ri, 8)
DEFINE_OR_REGISTER(64rm, 64)
DEFINE_OR_REGISTER(32rm, 32)
DEFINE_OR_REGISTER(16rm, 16)
DEFINE_OR_REGISTER(8rm, 8)

#undef DEFINE_OR_REGISTER

#define DEFINE_OR_ACCUMULATOR(SUFFIX, WIDTH, REGISTER)                         \
  void X86_OR##SUFFIX##_IR1Converter::loadSrcOperand(                          \
      ConversionContext &context) {                                            \
    loadAccumulatorImmediate(context, llvm::X86::REGISTER);                    \
  }                                                                            \
  void X86_OR##SUFFIX##_IR1Converter::storeDstOperand(                         \
      ConversionContext &context) {                                            \
    storeAccumulatorResult(context, llvm::X86::REGISTER);                      \
  }                                                                            \
  void X86_OR##SUFFIX##_IR1Converter::op(ConversionContext &context) {         \
    translateBinaryLogicalRegisterDestination(context, WIDTH,                  \
                                              BinaryLogicalKind::Or);          \
  }

DEFINE_OR_ACCUMULATOR(8i8, 8, AL)
DEFINE_OR_ACCUMULATOR(16i16, 16, AX)
DEFINE_OR_ACCUMULATOR(32i32, 32, EAX)
DEFINE_OR_ACCUMULATOR(64i32, 64, RAX)

#undef DEFINE_OR_ACCUMULATOR

#define DEFINE_OR_MEMORY(SUFFIX, WIDTH)                                        \
  void X86_OR##SUFFIX##_IR1Converter::op(ConversionContext &context) {         \
    translateBinaryLogicalMemoryDestination(context, WIDTH,                    \
                                            BinaryLogicalKind::Or);            \
  }

DEFINE_OR_MEMORY(64mr, 64)
DEFINE_OR_MEMORY(32mr, 32)
DEFINE_OR_MEMORY(16mr, 16)
DEFINE_OR_MEMORY(8mr, 8)
DEFINE_OR_MEMORY(64mi8, 64)
DEFINE_OR_MEMORY(64mi32, 64)
DEFINE_OR_MEMORY(32mi, 32)
DEFINE_OR_MEMORY(16mi, 16)
DEFINE_OR_MEMORY(8mi, 8)

#undef DEFINE_OR_MEMORY

// ---- XOR converters ----
#define DEFINE_XOR_REGISTER(SUFFIX, WIDTH)                                     \
  void X86_XOR##SUFFIX##_IR1Converter::op(ConversionContext &context) {        \
    translateBinaryLogicalRegisterDestination(context, WIDTH,                  \
                                              BinaryLogicalKind::Xor);         \
  }

DEFINE_XOR_REGISTER(64rr, 64)
DEFINE_XOR_REGISTER(32rr, 32)
DEFINE_XOR_REGISTER(16rr, 16)
DEFINE_XOR_REGISTER(8rr, 8)
DEFINE_XOR_REGISTER(64ri8, 64)
DEFINE_XOR_REGISTER(32ri8, 32)
DEFINE_XOR_REGISTER(16ri8, 16)
DEFINE_XOR_REGISTER(8ri8, 8)
DEFINE_XOR_REGISTER(64ri32, 64)
DEFINE_XOR_REGISTER(32ri, 32)
DEFINE_XOR_REGISTER(16ri, 16)
DEFINE_XOR_REGISTER(8ri, 8)
DEFINE_XOR_REGISTER(64rm, 64)
DEFINE_XOR_REGISTER(32rm, 32)
DEFINE_XOR_REGISTER(16rm, 16)
DEFINE_XOR_REGISTER(8rm, 8)

#undef DEFINE_XOR_REGISTER

#define DEFINE_XOR_ACCUMULATOR(SUFFIX, WIDTH, REGISTER)                        \
  void X86_XOR##SUFFIX##_IR1Converter::loadSrcOperand(                         \
      ConversionContext &context) {                                            \
    loadAccumulatorImmediate(context, llvm::X86::REGISTER);                    \
  }                                                                            \
  void X86_XOR##SUFFIX##_IR1Converter::storeDstOperand(                        \
      ConversionContext &context) {                                            \
    storeAccumulatorResult(context, llvm::X86::REGISTER);                      \
  }                                                                            \
  void X86_XOR##SUFFIX##_IR1Converter::op(ConversionContext &context) {        \
    translateBinaryLogicalRegisterDestination(context, WIDTH,                  \
                                              BinaryLogicalKind::Xor);         \
  }

DEFINE_XOR_ACCUMULATOR(8i8, 8, AL)
DEFINE_XOR_ACCUMULATOR(16i16, 16, AX)
DEFINE_XOR_ACCUMULATOR(32i32, 32, EAX)
DEFINE_XOR_ACCUMULATOR(64i32, 64, RAX)

#undef DEFINE_XOR_ACCUMULATOR

#define DEFINE_XOR_MEMORY(SUFFIX, WIDTH)                                       \
  void X86_XOR##SUFFIX##_IR1Converter::op(ConversionContext &context) {        \
    translateBinaryLogicalMemoryDestination(context, WIDTH,                    \
                                            BinaryLogicalKind::Xor);           \
  }

DEFINE_XOR_MEMORY(64mr, 64)
DEFINE_XOR_MEMORY(32mr, 32)
DEFINE_XOR_MEMORY(16mr, 16)
DEFINE_XOR_MEMORY(8mr, 8)
DEFINE_XOR_MEMORY(64mi8, 64)
DEFINE_XOR_MEMORY(64mi32, 64)
DEFINE_XOR_MEMORY(32mi, 32)
DEFINE_XOR_MEMORY(16mi, 16)
DEFINE_XOR_MEMORY(8mi, 8)

#undef DEFINE_XOR_MEMORY

// ---- TEST converters ----
#define TEST_REGISTER(SUFFIX, WIDTH)                                           \
  void X86_TEST##SUFFIX##_IR1Converter::op(ConversionContext &context) {       \
    translateBinaryLogicalRegisterFlags(context, WIDTH,                        \
                                        BinaryLogicalKind::And);               \
  }

TEST_REGISTER(64rr, 64)
TEST_REGISTER(32rr, 32)
TEST_REGISTER(16rr, 16)
TEST_REGISTER(8rr, 8)
TEST_REGISTER(64ri32, 64)
TEST_REGISTER(32ri, 32)
TEST_REGISTER(16ri, 16)
TEST_REGISTER(8ri, 8)

#undef TEST_REGISTER

#define TEST_MEMORY(SUFFIX, WIDTH)                                             \
  void X86_TEST##SUFFIX##_IR1Converter::op(ConversionContext &context) {       \
    translateBinaryLogicalMemoryFlags(context, WIDTH, BinaryLogicalKind::And); \
  }

TEST_MEMORY(64mr, 64)
TEST_MEMORY(32mr, 32)
TEST_MEMORY(16mr, 16)
TEST_MEMORY(8mr, 8)
TEST_MEMORY(64mi32, 64)
TEST_MEMORY(32mi, 32)
TEST_MEMORY(16mi, 16)
TEST_MEMORY(8mi, 8)

#undef TEST_MEMORY

#define TEST_ACCUMULATOR(SUFFIX, WIDTH, REGISTER)                              \
  void X86_TEST##SUFFIX##_IR1Converter::loadSrcOperand(                        \
      ConversionContext &context) {                                            \
    loadAccumulatorImmediate(context, llvm::X86::REGISTER);                    \
  }                                                                            \
  void X86_TEST##SUFFIX##_IR1Converter::op(ConversionContext &context) {       \
    translateBinaryLogicalRegisterFlags(context, WIDTH,                        \
                                        BinaryLogicalKind::And);               \
  }

TEST_ACCUMULATOR(8i8, 8, AL)
TEST_ACCUMULATOR(16i16, 16, AX)
TEST_ACCUMULATOR(32i32, 32, EAX)
TEST_ACCUMULATOR(64i32, 64, RAX)

#undef TEST_ACCUMULATOR
