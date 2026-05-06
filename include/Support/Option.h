//
// Created by zzm on 2026/5/6
// Part of RVision
//

#pragma once

#include <llvm/Support/CommandLine.h>

namespace sbt{
class Option {
public:
  inline static llvm::cl::OptionCategory SbtCategory = {"Sbt Category", "Sbt Category"};
};
}
