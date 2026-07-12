//
// Created by zzm on 2026/6/29
// Part of RVision
//

#include "Object/File.h"
#include "Support/Error.h"
#include "llvm/Object/ELFObjectFile.h"
#include "llvm/MC/MCDisassembler/MCDisassembler.h"
#include "Target/BaseMachine.h"

using namespace z8;
using namespace llvm;
using namespace llvm::object;


File::File(std::string Name) : Name(Name) {
  OB = unwrapOrError(ObjectFile::createObjectFile(Name));
  O = OB.getBinary();
  Machine = &getMachine(O->makeTriple());
  CurrentFile = this;
}


void File::disas()
{
  auto isText = [](const SectionRef& Section) {
    return Section.isText();
  };

  // .init
  // .plt
  // .text
  // .fini

  // 遍历所有 section
  for (auto Section : SectionFilter(isText, *O)) {
    auto Name = unwrapOrError(Section.getName());

    uint64_t SecAddr = Section.getAddress();
    uint64_t SecSize = Section.getSize();
    if (SecSize == 0) continue;

    auto Bytes = unwrapOrError(Section.getContents());

    ArrayRef Data(reinterpret_cast<const uint8_t *>(Bytes.data()), Bytes.size());
    uint64_t Offset = 0;

    while (Offset < SecSize) {
      MCInst Inst;
      uint64_t CurAddr = SecAddr + Offset;
      ArrayRef<uint8_t> Slice = Data.slice(Offset);

      uint64_t InstSize;

      auto S =
          Machine->getDisAsm().getInstruction(Inst, InstSize, Slice, CurAddr, nulls());

      if (S == MCDisassembler::Fail) {
        // 无法解码：跳过一个字节
        Offset += 1;
        continue;
      }

      IR0Ctx.IRs.emplace_back(Inst, CurAddr);

      Offset += InstSize;
    }
  }
}
