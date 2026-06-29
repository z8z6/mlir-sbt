//
// Created by root on 25-12-9.
//

#pragma once

#include "BaseMachine.h"

namespace z8
{
class X86Machine final : public BaseMachine{
private:
  static bool LLVMInitializeX86();
  inline static bool isInitialized = LLVMInitializeX86();

public:
  X86Machine();
  explicit X86Machine(llvm::Triple);
};

X86Machine& getX86Machine(const llvm::Triple& triple = {});

}





