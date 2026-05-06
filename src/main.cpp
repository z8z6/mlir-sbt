#include "Support/Option.h"

using namespace llvm::cl;
using namespace llvm;
using namespace sbt;

int main(int argc, char** argv) {
  HideUnrelatedOptions(sbt::Option::SbtCategory);
  ParseCommandLineOptions(argc, argv);
  return 0;
}