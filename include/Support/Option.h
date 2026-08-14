//
// Created by zzm on 2026/5/6
// Part of RVision
//

#pragma once

#include <llvm/Support/CommandLine.h>

namespace z8 {
class Option {
public:
  inline static llvm::cl::OptionCategory SbtCategory = {"Sbt Category",
                                                        "Sbt Option Category"};

  inline static llvm::cl::opt<std::string> InputBinary{
      "i", llvm::cl::Required, llvm::cl::cat(SbtCategory),
      llvm::cl::desc("Input binary file name")};

  inline static llvm::cl::opt<std::string> OutputObject{
      "o", llvm::cl::init(""), llvm::cl::cat(SbtCategory),
      llvm::cl::desc(
          "Output translated object file (default: <input>.sbt.o)")};

  inline static llvm::cl::opt<bool> Quiet{
      "quiet", llvm::cl::init(false), llvm::cl::cat(SbtCategory),
      llvm::cl::desc("Suppress the default discovered-function output")};

  inline static llvm::cl::opt<bool> PrintFunctions{
      "print-functions", llvm::cl::init(true), llvm::cl::cat(SbtCategory),
      llvm::cl::desc("Print discovered function names (enabled by default)")};

  inline static llvm::cl::opt<bool> PrintIR0{
      "print-ir0", llvm::cl::init(false), llvm::cl::cat(SbtCategory),
      llvm::cl::desc("Print decoded IR0 functions and CFGs")};

  inline static llvm::cl::opt<bool> PrintIR1{
      "print-ir1", llvm::cl::init(false), llvm::cl::cat(SbtCategory),
      llvm::cl::desc("Print IR1 before lowering")};

  inline static llvm::cl::opt<bool> PrintLoweredIR{
      "print-lowered-ir", llvm::cl::init(false), llvm::cl::cat(SbtCategory),
      llvm::cl::desc("Print the fully lowered MLIR module")};

  inline static llvm::cl::opt<bool> PrintStats{
      "print-stats", llvm::cl::init(false), llvm::cl::cat(SbtCategory),
      llvm::cl::desc("Print function discovery coverage and code inflation")};

  inline static llvm::cl::opt<std::string> Function{
      "function", llvm::cl::init(""), llvm::cl::cat(SbtCategory),
      llvm::cl::desc("Translate one discovered ELF function")};

  inline static llvm::cl::opt<uint64_t> AddressBias{
      "address-bias", llvm::cl::init(0), llvm::cl::cat(SbtCategory),
      llvm::cl::desc("Runtime bias added to decoded virtual addresses")};

  inline static llvm::cl::opt<bool> AuditOpcodes{
      "audit-opcodes", llvm::cl::init(false), llvm::cl::cat(SbtCategory),
      llvm::cl::desc("List decoded LLVM MC opcodes and converter coverage")};
};
} // namespace z8
