#include "Trans/X86/ScalarFloat.h"
#include "tblgen/X86IR1Converter.h"

using namespace z8;
using namespace z8::x86::trans;

void X86_SUBSSrr_Int_IR1Converter::op(ConversionContext &context) {
  translateScalarFloat(context, 32, ScalarFloatKind::Sub);
}
void X86_SUBSSrm_Int_IR1Converter::op(ConversionContext &context) {
  translateScalarFloat(context, 32, ScalarFloatKind::Sub);
}
