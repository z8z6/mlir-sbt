#include "Object/File.h"
#include "Support/Option.h"
#include "Target/X86Machine.h"
#include "tblgen/X86IR1Converter.h"

#include "llvm/Support/Format.h"
#include "llvm/Support/InitLLVM.h"

#include <map>

using namespace llvm::cl;
using namespace llvm;
using namespace z8;

int main(int argc, char **argv) {
  InitLLVM X(argc, argv);

  HideUnrelatedOptions(z8::Option::SbtCategory);
  ParseCommandLineOptions(argc, argv, "Binary Translator\n");
  std::string outputObject = z8::Option::OutputObject.empty()
                                 ? z8::Option::InputBinary + ".sbt.o"
                                 : z8::Option::OutputObject;

  File F(z8::Option::InputBinary);
  if (!F.disas(z8::Option::Function, z8::Option::AddressBias))
    return 1;
  if (z8::Option::AuditOpcodes) {
    std::map<unsigned, size_t> counts;
    size_t supportedInstructions = 0;
    size_t missingInstructions = 0;
    size_t supportedOpcodes = 0;
    size_t missingOpcodes = 0;
    for (const IR0Function &function : F.IR0Ctx.Functions)
      for (const IR0 &ir : function.IRs)
        ++counts[ir.Inst.getOpcode()];
    for (const auto &[opcode, count] : counts) {
      bool supported = hasX86Converter(opcode);
      (supported ? supportedInstructions : missingInstructions) += count;
      (supported ? supportedOpcodes : missingOpcodes)++;
      outs() << (supported ? "supported" : "missing") << '\t' << count << '\t'
             << F.Machine->getMII().getName(opcode) << '\n';
    }
    outs() << "summary\t" << supportedInstructions
           << " supported instructions, " << missingInstructions
           << " missing instructions; " << supportedOpcodes
           << " supported opcodes, " << missingOpcodes << " missing opcodes\n";
    if (z8::Option::PrintStats) {
      FunctionDiscoveryStats discovery = F.functionDiscoveryStats();
      double coverage = discovery.TextBytes ? 100.0 * discovery.FunctionBytes /
                                                  discovery.TextBytes
                                            : 0.0;
      outs() << "function-discovery " << discovery.FunctionBytes << '/'
             << discovery.TextBytes << " bytes (" << format("%.2f", coverage)
             << "%)\n";
    }
    return 0;
  }
  if (!z8::Option::Quiet && z8::Option::PrintFunctions)
    F.IR0Ctx.printFunctionNames();
  if (z8::Option::PrintIR0)
    F.IR0Ctx.print();
  if (z8::Option::PrintStats) {
    FunctionDiscoveryStats discovery = F.functionDiscoveryStats();
    double coverage = discovery.TextBytes ? 100.0 * discovery.FunctionBytes /
                                                discovery.TextBytes
                                          : 0.0;
    outs() << "function-discovery " << discovery.FunctionBytes << '/'
           << discovery.TextBytes << " bytes (" << format("%.2f", coverage)
           << "%)\n";
  }
  if (failed(IR1Context::Instance().convert(F.IR0Ctx)))
    return 1;
  if (z8::Option::PrintIR1)
    IR1Context::Instance().print(true);
  if (failed(IR1Context::Instance().lower()))
    return 1;
  if (z8::Option::PrintLoweredIR)
    IR1Context::Instance().print(true);
  if (failed(IR1Context::Instance().emitObject(outputObject)))
    return 1;

  if (z8::Option::PrintStats) {
    uint64_t sourceBytes = F.IR0Ctx.translatedCodeSize();
    File translated(outputObject);
    uint64_t translatedBytes = translated.textSize();
    double inflation =
        sourceBytes ? static_cast<double>(translatedBytes) / sourceBytes : 0.0;
    outs() << "code-inflation " << translatedBytes << '/' << sourceBytes
           << " bytes (" << format("%.2f", inflation) << "x)\n";
  }

  return 0;
}
