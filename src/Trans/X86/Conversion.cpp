#include "Trans/X86/Conversion.h"
#include "IR/IR1.h"
#include "IR/IR1Converter.h"
#include "IR/X86.h"
#include "tblgen/X86IR1Converter.h"
#include <cassert>

using namespace mlir;
using namespace z8;
using namespace z8::x86::trans;

// ---- shared helpers ----
namespace z8::x86::trans {

void translateCvt(ConversionContext &context, const CvtSpec &spec) {
  assert(spec.lanes > 0);
  auto *ir = BaseIR1Converter::Ctx;
  Location loc = context.getNameLoc();
  size_t sourceOffset = spec.preserveUpper ? 1 : 0;
  Value oldDestination = spec.preserveUpper
                             ? context.Src.front()
                             : ir1::ConstIntOp::create(ir->Builder, loc, 0);
  Value source;
  if (spec.memory) {
    unsigned loadWidth = spec.sourceWidth * spec.lanes;
    source = ir1::LoadOp::create(ir->Builder, loc, ir->iTy(loadWidth),
                                 buildMemoryAddress(context, sourceOffset));
  } else {
    assert(context.Src.size() > sourceOffset);
    source = context.Src[sourceOffset];
  }
  auto attr = [&](unsigned value) {
    return ir->Builder.getI32IntegerAttr(static_cast<int32_t>(value));
  };
  Value result = x86ir::ConvertOp::create(
      ir->Builder, loc, ir->iTy(spec.resultWidth), source, oldDestination,
      attr(static_cast<unsigned>(spec.kind)), attr(spec.sourceWidth),
      attr(spec.destinationWidth), attr(spec.lanes),
      attr(static_cast<unsigned>(spec.round)), attr(spec.preserveUpper));
  context.Dst.push_back(result);
}

} // namespace z8::x86::trans

// ---- CVT converters ----
#define CVT_IMPL(OPCODE, KIND, ROUND, SRC, DST, LANES, RESULT, MEMORY,         \
                 PRESERVE)                                                     \
  void X86_##OPCODE##_IR1Converter::op(ConversionContext &context) {           \
    translateCvt(context, {CvtKind::KIND, CvtRound::ROUND, SRC, DST, LANES,    \
                           RESULT, MEMORY, PRESERVE});                         \
  }

// Packed signed integer / floating-point conversions.
CVT_IMPL(CVTDQ2PDrr, SignedIntToFloat, Default, 32, 64, 2, 128, false, false)
CVT_IMPL(CVTDQ2PDrm, SignedIntToFloat, Default, 32, 64, 2, 128, true, false)
CVT_IMPL(CVTDQ2PSrr, SignedIntToFloat, Default, 32, 32, 4, 128, false, false)
CVT_IMPL(CVTDQ2PSrm, SignedIntToFloat, Default, 32, 32, 4, 128, true, false)
CVT_IMPL(CVTPD2DQrr, FloatToSignedInt, NearestEven, 64, 32, 2, 128, false,
         false)
CVT_IMPL(CVTPD2DQrm, FloatToSignedInt, NearestEven, 64, 32, 2, 128, true, false)
CVT_IMPL(CVTPD2PSrr, FloatTruncate, Default, 64, 32, 2, 128, false, false)
CVT_IMPL(CVTPD2PSrm, FloatTruncate, Default, 64, 32, 2, 128, true, false)
CVT_IMPL(CVTPS2DQrr, FloatToSignedInt, NearestEven, 32, 32, 4, 128, false,
         false)
CVT_IMPL(CVTPS2DQrm, FloatToSignedInt, NearestEven, 32, 32, 4, 128, true, false)
CVT_IMPL(CVTPS2PDrr, FloatExtend, Default, 32, 64, 2, 128, false, false)
CVT_IMPL(CVTPS2PDrm, FloatExtend, Default, 32, 64, 2, 128, true, false)
CVT_IMPL(CVTTPD2DQrr, FloatToSignedInt, Truncate, 64, 32, 2, 128, false, false)
CVT_IMPL(CVTTPD2DQrm, FloatToSignedInt, Truncate, 64, 32, 2, 128, true, false)
CVT_IMPL(CVTTPS2DQrr, FloatToSignedInt, Truncate, 32, 32, 4, 128, false, false)
CVT_IMPL(CVTTPS2DQrm, FloatToSignedInt, Truncate, 32, 32, 4, 128, true, false)

// Scalar float-to-integer conversions.
#define FP_TO_INT_PAIR(NAME, KIND, ROUND, SRC, DST)                            \
  CVT_IMPL(NAME##rr_Int, KIND, ROUND, SRC, DST, 1, DST, false, false)          \
  CVT_IMPL(NAME##rm_Int, KIND, ROUND, SRC, DST, 1, DST, true, false)
FP_TO_INT_PAIR(CVTSD2SI, FloatToSignedInt, NearestEven, 64, 32)
FP_TO_INT_PAIR(CVTSD2SI64, FloatToSignedInt, NearestEven, 64, 64)
FP_TO_INT_PAIR(CVTSS2SI, FloatToSignedInt, NearestEven, 32, 32)
FP_TO_INT_PAIR(CVTSS2SI64, FloatToSignedInt, NearestEven, 32, 64)
FP_TO_INT_PAIR(CVTTSD2SI, FloatToSignedInt, Truncate, 64, 32)
FP_TO_INT_PAIR(CVTTSD2SI64, FloatToSignedInt, Truncate, 64, 64)
FP_TO_INT_PAIR(CVTTSS2SI, FloatToSignedInt, Truncate, 32, 32)
FP_TO_INT_PAIR(CVTTSS2SI64, FloatToSignedInt, Truncate, 32, 64)
#undef FP_TO_INT_PAIR

// Scalar integer-to-float and float-width conversions preserve the untouched
// high XMM bits from the legacy two-operand destination.
#define SCALAR_XMM_PAIR(NAME, KIND, SRC, DST)                                  \
  CVT_IMPL(NAME##rr_Int, KIND, Default, SRC, DST, 1, 128, false, true)         \
  CVT_IMPL(NAME##rm_Int, KIND, Default, SRC, DST, 1, 128, true, true)
SCALAR_XMM_PAIR(CVTSI2SD, SignedIntToFloat, 32, 64)
SCALAR_XMM_PAIR(CVTSI642SD, SignedIntToFloat, 64, 64)
SCALAR_XMM_PAIR(CVTSI2SS, SignedIntToFloat, 32, 32)
SCALAR_XMM_PAIR(CVTSI642SS, SignedIntToFloat, 64, 32)
SCALAR_XMM_PAIR(CVTSD2SS, FloatTruncate, 64, 32)
SCALAR_XMM_PAIR(CVTSS2SD, FloatExtend, 32, 64)
#undef SCALAR_XMM_PAIR
#undef CVT_IMPL
