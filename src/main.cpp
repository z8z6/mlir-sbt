#include "Object/File.h"
#include "Support/Option.h"
#include "Target/X86Machine.h"

#include "llvm/Support/InitLLVM.h"

using namespace llvm::cl;
using namespace llvm;
using namespace z8;

int main(int argc, char** argv) {
  InitLLVM X(argc, argv);

  HideUnrelatedOptions(z8::Option::SbtCategory);
  ParseCommandLineOptions(argc, argv, "Binary Translator\n");

  File F(z8::Option::InputBinary);
  F.disas();
  if (!z8::Option::Quiet)
    F.IR0Ctx.print();
  if (failed(IR1Context::Instance().convert(F.IR0Ctx)))
    return 1;
  if (!z8::Option::Quiet)
    IR1Context::Instance().print(true);
  if (failed(IR1Context::Instance().lower()))
    return 1;
  if (!z8::Option::Quiet)
    IR1Context::Instance().print(true);
  if (failed(IR1Context::Instance().emitObject(z8::Option::OutputObject)))
    return 1;

  return 0;
}
