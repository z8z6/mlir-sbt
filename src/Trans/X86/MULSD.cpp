#include "Trans/X86/ScalarFloat.h"
#include "tblgen/X86IR1Converter.h"

using namespace z8;
using namespace z8::x86::trans;

void X86_MULSDrr_Int_IR1Converter::op(ConversionContext &context) {
  translateScalarFloat(context, 64, ScalarFloatKind::Mul);
}
void X86_MULSDrm_Int_IR1Converter::op(ConversionContext &context) {
  translateScalarFloat(context, 64, ScalarFloatKind::Mul);
}
