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

  inline static llvm::cl::opt<std::string> OutputObject {
    "o",
    llvm::cl::init("translated.o"),
    llvm::cl::cat(SbtCategory),
    llvm::cl::desc("Output translated object file")
  };

  inline static llvm::cl::opt<bool> Quiet {
    "quiet",
    llvm::cl::init(false),
    llvm::cl::cat(SbtCategory),
    llvm::cl::desc("Suppress disassembly and intermediate IR output")
  };
};
}
