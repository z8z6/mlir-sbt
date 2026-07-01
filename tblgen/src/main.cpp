//
// Created by zzm on 2026/6/30
// Part of RVision
//

#include "IR1Converter.h"
#include <iostream>
#include "llvm/TableGen/Main.h"
#include "llvm/Support/CommandLine.h"

using namespace llvm;
using namespace llvm::cl;
using namespace z8;

static opt<bool> GenIR1(
    "gen-x86-ir1",
    desc("Generate X86 IR1 CXX Headers")
);

int main(int argc, char **argv) {
    std::cout << "IR1 tableGen is working, please wait for about 10s..." << std::endl;
    ParseCommandLineOptions(argc, argv, "IR1 CXX Headers Generator\n");
    if (GenIR1) TableGenMain(argv[0], EmitIR1);
    return 0;
}