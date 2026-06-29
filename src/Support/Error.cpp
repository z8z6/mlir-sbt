

#include "Support/Error.h"
#include <llvm/Support/WithColor.h>


using namespace z8;
using namespace llvm;

void z8::reportError(llvm::Error E)
{
  assert(E);
  outs().flush();
  WithColor::error(errs());
  logAllUnhandledErrors(std::move(E), errs());
  exit(1);
}

void z8::reportError(llvm::Error E, StringRef ToolName, StringRef string)
{
  assert(E);
  outs().flush();
  WithColor::error(errs(), ToolName);
  errs() << "'" << string << "'";
  logAllUnhandledErrors(std::move(E), errs());
  exit(1);
}