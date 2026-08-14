#include "Trans/X86/Move.h"
#include "IR/IR1.h"
#include "IR/IR1Converter.h"
#include "Trans/X86/Common.h"
#include "tblgen/X86IR1Converter.h"
#include <cassert>

using namespace mlir;
using namespace z8;
using namespace z8::x86::trans;

// ---- shared helpers ----
namespace z8::x86::trans {
namespace {

Value normalizeInteger(ConversionContext &context, Value value,
                       unsigned width) {
  auto *ir = BaseIR1Converter::Ctx;
  Location loc = context.getNameLoc();
  Value zero = ir1::ConstIntOp::create(ir->Builder, loc, 0);
  return ir1::AddIOp::create(ir->Builder, loc, ir->iTy(width), value, zero);
}

} // namespace

void translateMoveRegisterDestination(ConversionContext &context,
                                      unsigned width) {
  assert(context.Src.size() == 1 || context.Src.size() == 5);
  auto *ir = BaseIR1Converter::Ctx;
  Value value = context.Src[0];
  if (context.Src.size() == 5)
    value = ir1::LoadOp::create(ir->Builder, context.getNameLoc(),
                                ir->iTy(width), buildMemoryAddress(context));
  context.Dst.push_back(normalizeInteger(context, value, width));
}

void translateMoveMemoryDestination(ConversionContext &context,
                                    unsigned width) {
  assert(context.Src.size() == 6);
  auto *ir = BaseIR1Converter::Ctx;
  Value address = buildMemoryAddress(context);
  Value value = normalizeInteger(context, context.Src[5], width);
  ir1::StoreOp::create(ir->Builder, context.getNameLoc(), value, address);
}

void translateVectorMoveRegisterDestination(ConversionContext &context) {
  assert(context.Src.size() == 1 || context.Src.size() == 5);
  auto *ir = BaseIR1Converter::Ctx;
  Value value = context.Src.front();
  if (context.Src.size() == 5)
    value = ir1::LoadOp::create(ir->Builder, context.getNameLoc(), ir->iTy(128),
                                buildMemoryAddress(context));
  context.Dst.push_back(value);
}

void translateVectorMoveMemoryDestination(ConversionContext &context) {
  assert(context.Src.size() == 6);
  auto *ir = BaseIR1Converter::Ctx;
  ir1::StoreOp::create(ir->Builder, context.getNameLoc(), context.Src[5],
                       buildMemoryAddress(context));
}

} // namespace z8::x86::trans

// ---- MOV converters ----
#define DEFINE_MOV_REGISTER(SUFFIX, WIDTH)                                     \
  void X86_MOV##SUFFIX##_IR1Converter::op(ConversionContext &context) {        \
    translateMoveRegisterDestination(context, WIDTH);                          \
  }

DEFINE_MOV_REGISTER(64rr, 64)
DEFINE_MOV_REGISTER(32rr, 32)
DEFINE_MOV_REGISTER(16rr, 16)
DEFINE_MOV_REGISTER(8rr, 8)
DEFINE_MOV_REGISTER(64ri, 64)
DEFINE_MOV_REGISTER(64ri32, 64)
DEFINE_MOV_REGISTER(32ri, 32)
DEFINE_MOV_REGISTER(16ri, 16)
DEFINE_MOV_REGISTER(8ri, 8)
DEFINE_MOV_REGISTER(64rm, 64)
DEFINE_MOV_REGISTER(32rm, 32)
DEFINE_MOV_REGISTER(16rm, 16)
DEFINE_MOV_REGISTER(8rm, 8)

#undef DEFINE_MOV_REGISTER

#define DEFINE_MOV_MEMORY(SUFFIX, WIDTH)                                       \
  void X86_MOV##SUFFIX##_IR1Converter::op(ConversionContext &context) {        \
    translateMoveMemoryDestination(context, WIDTH);                            \
  }

DEFINE_MOV_MEMORY(64mr, 64)
DEFINE_MOV_MEMORY(32mr, 32)
DEFINE_MOV_MEMORY(16mr, 16)
DEFINE_MOV_MEMORY(8mr, 8)
DEFINE_MOV_MEMORY(64mi32, 64)
DEFINE_MOV_MEMORY(32mi, 32)
DEFINE_MOV_MEMORY(16mi, 16)
DEFINE_MOV_MEMORY(8mi, 8)

#undef DEFINE_MOV_MEMORY

// ---- legacy 128-bit SSE MOV converters ----
#define DEFINE_VECTOR_MOV_REGISTER(NAME)                                      \
  void X86_##NAME##_IR1Converter::op(ConversionContext &context) {             \
    translateVectorMoveRegisterDestination(context);                           \
  }

DEFINE_VECTOR_MOV_REGISTER(MOVAPSrr)
DEFINE_VECTOR_MOV_REGISTER(MOVAPSrm)
DEFINE_VECTOR_MOV_REGISTER(MOVUPSrr)
DEFINE_VECTOR_MOV_REGISTER(MOVUPSrm)
DEFINE_VECTOR_MOV_REGISTER(MOVDQArr)
DEFINE_VECTOR_MOV_REGISTER(MOVDQArm)
DEFINE_VECTOR_MOV_REGISTER(MOVDQUrr)
DEFINE_VECTOR_MOV_REGISTER(MOVDQUrm)

#undef DEFINE_VECTOR_MOV_REGISTER

#define DEFINE_VECTOR_MOV_MEMORY(NAME)                                        \
  void X86_##NAME##_IR1Converter::op(ConversionContext &context) {             \
    translateVectorMoveMemoryDestination(context);                             \
  }

DEFINE_VECTOR_MOV_MEMORY(MOVAPSmr)
DEFINE_VECTOR_MOV_MEMORY(MOVUPSmr)
DEFINE_VECTOR_MOV_MEMORY(MOVDQAmr)
DEFINE_VECTOR_MOV_MEMORY(MOVDQUmr)

#undef DEFINE_VECTOR_MOV_MEMORY

// ---- MOVSX/MOVSXD/MOVZX converters ----
namespace {
void translateExtension(ConversionContext &context, unsigned sourceWidth,
                        unsigned destinationWidth, bool signExtend,
                        bool memorySource) {
  assert(context.Src.size() == (memorySource ? 5 : 1));
  auto *ir = BaseIR1Converter::Ctx;
  Location loc = context.getNameLoc();
  Value source = context.Src.front();
  if (memorySource)
    source = ir1::LoadOp::create(ir->Builder, loc, ir->iTy(sourceWidth),
                                 x86::trans::buildMemoryAddress(context));
  Value result;
  if (signExtend)
    result = ir1::ExtSIOp::create(ir->Builder, loc, ir->iTy(destinationWidth),
                                  source);
  else
    result = ir1::CastIOp::create(ir->Builder, loc, ir->iTy(destinationWidth),
                                  source);
  context.Dst.push_back(result);
}
} // namespace

#define MOV_EXT(NAME, SRC, DST, SIGNED, MEMORY)                                \
  void X86_##NAME##_IR1Converter::op(ConversionContext &context) {             \
    translateExtension(context, SRC, DST, SIGNED, MEMORY);                     \
  }

MOV_EXT(MOVSX16rr8, 8, 16, true, false)
MOV_EXT(MOVSX16rm8, 8, 16, true, true)
MOV_EXT(MOVSX32rr8, 8, 32, true, false)
MOV_EXT(MOVSX32rm8, 8, 32, true, true)
MOV_EXT(MOVSX32rr16, 16, 32, true, false)
MOV_EXT(MOVSX32rm16, 16, 32, true, true)
MOV_EXT(MOVSX64rr8, 8, 64, true, false)
MOV_EXT(MOVSX64rm8, 8, 64, true, true)
MOV_EXT(MOVSX64rr16, 16, 64, true, false)
MOV_EXT(MOVSX64rm16, 16, 64, true, true)
MOV_EXT(MOVSX64rr32, 32, 64, true, false)
MOV_EXT(MOVSX64rm32, 32, 64, true, true)

MOV_EXT(MOVZX16rr8, 8, 16, false, false)
MOV_EXT(MOVZX16rm8, 8, 16, false, true)
MOV_EXT(MOVZX32rr8, 8, 32, false, false)
MOV_EXT(MOVZX32rm8, 8, 32, false, true)
MOV_EXT(MOVZX32rr16, 16, 32, false, false)
MOV_EXT(MOVZX32rm16, 16, 32, false, true)

#undef MOV_EXT
