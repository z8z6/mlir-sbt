//
// Created by zzm on 2026/6/30
// Part of RVision
//

#include "llvm/TableGen/Main.h"
#include "X86HeaderGenerator.h"
#include "X86ImplGenerator.h"
#include "llvm/Support/CommandLine.h"
#include <iostream>

using namespace llvm;
using namespace llvm::cl;
using namespace z8;

static opt<bool> GenX86Header(
    "gen-x86-header",
    desc("Generate X86 IR1 CXX Headers")
);
static opt<bool> GenX86Impl(
    "gen-x86-impl",
    desc("Generate X86 IR1 CXX Imple")
);

int main(int argc, char **argv) {
    std::cout << "IR1 tableGen is working, please wait for about 10s..." << std::endl;
    ParseCommandLineOptions(argc, argv, "IR1 CXX Headers Generator\n");
    if (GenX86Header) TableGenMain(argv[0], EmitX86Header);
    if (GenX86Impl) TableGenMain(argv[0], EmitX86Impl);
    return 0;
}