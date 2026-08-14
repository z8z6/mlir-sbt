#pragma once

#include "Trans/X86/Common.h"

namespace z8::x86::trans {

enum class CvtKind { SignedIntToFloat, FloatToSignedInt, FloatExtend,
                     FloatTruncate };
enum class CvtRound { Default, NearestEven, Truncate };

struct CvtSpec {
  CvtKind kind;
  CvtRound round;
  unsigned sourceWidth;
  unsigned destinationWidth;
  unsigned lanes;
  unsigned resultWidth;
  bool memory;
  bool preserveUpper;
};

void translateCvt(ConversionContext &context, const CvtSpec &spec);

} // namespace z8::x86::trans
