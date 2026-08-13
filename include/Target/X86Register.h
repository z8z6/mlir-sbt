#pragma once

#include <cstdint>
#include <optional>

namespace z8 {

enum class X86RegisterSlot : uint32_t {
  RAX,
  RBX,
  RCX,
  RDX,
  RSI,
  RDI,
  RBP,
  RSP,
  R8,
  R9,
  R10,
  R11,
  R12,
  R13,
  R14,
  R15,
  RFLAGS,
  XMM0 = 17,
  XMM1 = 19,
  XMM2 = 21,
  XMM3 = 23,
  XMM4 = 25,
  XMM5 = 27,
  XMM6 = 29,
  XMM7 = 31,
  XMM8 = 33,
  XMM9 = 35,
  XMM10 = 37,
  XMM11 = 39,
  XMM12 = 41,
  XMM13 = 43,
  XMM14 = 45,
  XMM15 = 47,
  Count = 49
};

struct X86RegisterDesc {
  X86RegisterSlot slot;
  unsigned width;
  unsigned bitOffset;
  bool zeroExtendOnWrite;
};

std::optional<X86RegisterDesc> getX86RegisterDesc(unsigned llvmRegister);

inline constexpr uint64_t X86ArithmeticFlagsMask = (1ULL << 0) | (1ULL << 2) |
                                                   (1ULL << 4) | (1ULL << 6) |
                                                   (1ULL << 7) | (1ULL << 11);

} // namespace z8
