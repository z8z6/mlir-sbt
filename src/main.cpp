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
  F.IR0Ctx.print(F.Machine);
  F.IR1Ctx.transform(F.IR0Ctx);
  F.IR1Ctx.print();

  return 0;
}