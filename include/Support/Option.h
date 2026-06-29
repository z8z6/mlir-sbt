//
// Created by zzm on 2026/5/6
// Part of RVision
//

#pragma once

#include <llvm/Support/CommandLine.h>

namespace z8{
class Option {
public:
  inline static llvm::cl::OptionCategory SbtCategory = {"Sbt Category", "Sbt Option Category"};

  inline static llvm::cl::opt<std::string> InputBinary {
    "i",
    llvm::cl::Required,
    llvm::cl::cat(SbtCategory),
    llvm::cl::desc("Input binary file name")
  };
};
}
