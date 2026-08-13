#include "Target//X86Machine.h"
#include "llvm/Support/TargetSelect.h"
#include "llvm/TargetParser/Host.h"

using namespace llvm;
using namespace z8;

bool X86Machine::LLVMInitializeX86() {
  LLVMInitializeX86Target();
  LLVMInitializeX86TargetInfo();
  LLVMInitializeX86TargetMC();
  LLVMInitializeX86Disassembler();
  LLVMInitializeX86AsmPrinter();
  return true;
}

X86Machine::X86Machine()
: X86Machine(llvm::Triple(sys::getProcessTriple())) {}

X86Machine::X86Machine(const llvm::Triple triple) : BaseMachine(triple)
{
}

X86Machine& z8::getX86Machine(const llvm::Triple& triple)
{
  static X86Machine machine(triple);
  return machine;
}
