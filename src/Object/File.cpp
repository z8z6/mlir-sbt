//
// Created by zzm on 2026/6/29
// Part of RVision
//

#include "Object/File.h"
#include "Support/Error.h"
#include "Target/BaseMachine.h"
#include "llvm/BinaryFormat/ELF.h"
#include "llvm/MC/MCDisassembler/MCDisassembler.h"
#include "llvm/Object/ELFObjectFile.h"

#include "Target/X86/MCTargetDesc/X86MCTargetDesc.h"
#include "llvm/ADT/DenseMap.h"
#include "llvm/ADT/STLExtras.h"

#include <algorithm>
#include <tuple>

using namespace z8;
using namespace llvm;
using namespace llvm::object;

uint64_t File::textSize() const {
  uint64_t size = 0;
  for (SectionRef section : O->sections())
    if (section.isText())
      size += section.getSize();
  return size;
}

FunctionDiscoveryStats File::functionDiscoveryStats() const {
  FunctionDiscoveryStats stats;
  stats.TextBytes = textSize();
  std::vector<std::tuple<uint64_t, uint64_t, uint64_t>> ranges;
  for (const FunctionInfo &function : Functions)
    if (function.Size)
      ranges.emplace_back(function.SectionIndex, function.Address,
                          function.Address + function.Size);
  llvm::sort(ranges);
  uint64_t section = 0;
  uint64_t begin = 0;
  uint64_t end = 0;
  bool active = false;
  for (auto [nextSection, nextBegin, nextEnd] : ranges) {
    if (!active || nextSection != section || nextBegin > end) {
      if (active)
        stats.FunctionBytes += end - begin;
      section = nextSection;
      begin = nextBegin;
      end = nextEnd;
      active = true;
    } else {
      end = std::max(end, nextEnd);
    }
  }
  if (active)
    stats.FunctionBytes += end - begin;
  return stats;
}

bool FunctionInfo::matches(StringRef name) const {
  return Name == name || llvm::is_contained(Aliases, name);
}

File::File(std::string Name) : Name(Name) {
  OB = unwrapOrError(ObjectFile::createObjectFile(Name));
  O = OB.getBinary();
  Machine = &getMachine(O->makeTriple());
  CurrentFile = this;
}

bool File::disas(StringRef functionName, uint64_t addressBias) {
  Functions.clear();
  PLTSymbols.clear();
  IR0Ctx.Functions.clear();
  IR0Ctx.EntryFunctionIndex = 0;

  if (auto *elf = dyn_cast<ELFObjectFileBase>(O)) {
    for (const ELFPltEntry &entry : elf->getPltEntries(Machine->getSTI())) {
      if (!entry.Symbol)
        continue;
      SymbolRef symbol(*entry.Symbol, O);
      if (auto name = symbol.getName())
        PLTSymbols.emplace(entry.Address, name->str());
      else
        consumeError(name.takeError());
    }

    auto addFunction = [&](ELFSymbolRef symbol) {
      if (symbol.getELFType() != ELF::STT_FUNC)
        return;
      auto sectionOrError = symbol.getSection();
      if (!sectionOrError) {
        consumeError(sectionOrError.takeError());
        return;
      }
      if (*sectionOrError == O->section_end() || !(**sectionOrError).isText())
        return;
      auto name = symbol.getName();
      auto address = symbol.getAddress();
      if (!name || !address) {
        if (!name)
          consumeError(name.takeError());
        if (!address)
          consumeError(address.takeError());
        return;
      }
      Functions.push_back({name->str(),
                           {},
                           *address,
                           symbol.getSize(),
                           (**sectionOrError).getIndex()});
    };
    for (ELFSymbolRef symbol : elf->symbols())
      addFunction(symbol);
    // Stripped shared objects such as the system glibc commonly retain only
    // .dynsym.  Function selection must inspect it as well as the optional
    // regular symbol table or --function cannot disassemble library code.
    for (ELFSymbolRef symbol : elf->getDynamicSymbolIterators())
      addFunction(symbol);
  }
  llvm::sort(Functions, [](const FunctionInfo &lhs, const FunctionInfo &rhs) {
    return std::tie(lhs.SectionIndex, lhs.Address, lhs.Name) <
           std::tie(rhs.SectionIndex, rhs.Address, rhs.Name);
  });
  std::vector<FunctionInfo> canonicalFunctions;
  for (FunctionInfo &function : Functions) {
    if (!canonicalFunctions.empty() &&
        canonicalFunctions.back().SectionIndex == function.SectionIndex &&
        canonicalFunctions.back().Address == function.Address) {
      FunctionInfo &canonical = canonicalFunctions.back();
      canonical.Size = std::max(canonical.Size, function.Size);
      if (canonical.Name != function.Name &&
          !llvm::is_contained(canonical.Aliases, function.Name))
        canonical.Aliases.push_back(std::move(function.Name));
      continue;
    }
    canonicalFunctions.push_back(std::move(function));
  }
  Functions = std::move(canonicalFunctions);

  DenseMap<uint64_t, uint64_t> sectionEnds;
  for (SectionRef section : O->sections())
    sectionEnds[section.getIndex()] = section.getAddress() + section.getSize();
  for (size_t index = 0; index < Functions.size(); ++index) {
    FunctionInfo &function = Functions[index];
    uint64_t sectionEnd = sectionEnds.lookup(function.SectionIndex);
    if (function.Address >= sectionEnd)
      return false;
    uint64_t inferredEnd = sectionEnd;
    if (index + 1 < Functions.size() &&
        Functions[index + 1].SectionIndex == function.SectionIndex)
      inferredEnd = Functions[index + 1].Address;
    if (function.Size == 0)
      function.Size = inferredEnd - function.Address;
    function.Size = std::min(function.Size, sectionEnd - function.Address);
  }

  auto decodeRange = [&](SectionRef section, const FunctionInfo &info) -> bool {
    IR0Function function;
    function.Name = info.Name;
    function.Aliases = info.Aliases;
    function.Address = info.Address;
    function.Size = info.Size;
    function.SectionIndex = info.SectionIndex;
    uint64_t start = info.Address;
    uint64_t size = info.Size;
    uint64_t sectionAddress = section.getAddress();
    uint64_t sectionSize = section.getSize();
    if (start < sectionAddress || start + size > sectionAddress + sectionSize)
      return false;
    auto bytes = unwrapOrError(section.getContents());
    ArrayRef data(reinterpret_cast<const uint8_t *>(bytes.data()),
                  bytes.size());
    uint64_t offset = start - sectionAddress;
    uint64_t end = offset + size;
    while (offset < end) {
      MCInst Inst;
      uint64_t currentAddress = sectionAddress + offset;
      ArrayRef<uint8_t> slice = data.slice(offset, end - offset);
      uint64_t instructionSize = 0;
      auto status = Machine->getDisAsm().getInstruction(
          Inst, instructionSize, slice, currentAddress, nulls());
      if (status == MCDisassembler::Fail || instructionSize == 0) {
        errs() << "cannot decode instruction at "
               << format_hex(currentAddress, 10) << "\n";
        return false;
      }
      function.IRs.emplace_back(Inst, currentAddress, instructionSize,
                                section.getIndex());
      IR0 &ir = function.IRs.back();
      ir.AddressBias = addressBias;
      ir.FunctionName = function.Name;
      std::string instructionText = ir.str();
      raw_string_ostream debugText(ir.DebugText);
      debugText << format_hex(currentAddress, 10) << ": " << instructionText;
      if (Inst.getOpcode() == llvm::X86::CALL64pcrel32 &&
          Inst.getNumOperands() > 0 && Inst.getOperand(0).isImm()) {
        uint64_t target =
            currentAddress + instructionSize + Inst.getOperand(0).getImm();
        if (auto external = PLTSymbols.find(target);
            external != PLTSymbols.end())
          ir.ExternalSymbol = external->second;
      }
      offset += instructionSize;
    }
    if (!function.buildCFG()) {
      errs() << "cannot build CFG for function '" << function.Name << "'\n";
      return false;
    }
    IR0Ctx.Functions.push_back(std::move(function));
    return true;
  };

  std::vector<const FunctionInfo *> selectedFunctions;
  if (!functionName.empty()) {
    auto found = llvm::find_if(Functions, [&](const FunctionInfo &function) {
      return function.matches(functionName);
    });
    if (found == Functions.end()) {
      errs() << "cannot find ELF function '" << functionName << "'\n";
      return false;
    }
    selectedFunctions.push_back(&*found);
  } else {
    for (const FunctionInfo &function : Functions)
      selectedFunctions.push_back(&function);
  }

  for (const FunctionInfo *info : selectedFunctions) {
    bool decoded = false;
    for (SectionRef section : O->sections()) {
      if (section.getIndex() != info->SectionIndex)
        continue;
      if (!decodeRange(section, *info))
        return false;
      decoded = true;
      break;
    }
    if (!decoded)
      return false;
  }

  // Relocatable assembly fixtures often omit STT_FUNC. Represent every
  // non-empty text section as a synthetic function instead of flattening all
  // sections into one instruction stream.
  if (IR0Ctx.Functions.empty() && functionName.empty()) {
    for (SectionRef section : SectionFilter(
             [](const SectionRef &candidate) { return candidate.isText(); },
             *O)) {
      if (!section.getSize())
        continue;
      std::string name = "text_" + std::to_string(section.getIndex());
      if (!decodeRange(section, {name,
                                 {},
                                 section.getAddress(),
                                 section.getSize(),
                                 section.getIndex()}))
        return false;
    }
  }

  if (IR0Ctx.Functions.empty())
    return false;
  if (!functionName.empty()) {
    IR0Ctx.EntryFunctionIndex = 0;
  } else {
    for (StringRef preferred : {"run_case", "_start", "main"}) {
      auto found =
          llvm::find_if(IR0Ctx.Functions, [&](const IR0Function &function) {
            return function.matches(preferred);
          });
      if (found != IR0Ctx.Functions.end()) {
        IR0Ctx.EntryFunctionIndex =
            static_cast<size_t>(std::distance(IR0Ctx.Functions.begin(), found));
        break;
      }
    }
  }
  return true;
}
