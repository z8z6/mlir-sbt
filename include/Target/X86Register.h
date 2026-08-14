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
  // Logical x87 stack entries. Each 80-bit value occupies two 64-bit state
  // slots so loads/stores can use a naturally aligned 128-bit container.
  X87ST0 = 49,
  X87ST1 = 51,
  X87ST2 = 53,
  X87ST3 = 55,
  X87ST4 = 57,
  X87ST5 = 59,
  X87ST6 = 61,
  X87ST7 = 63,
  Count = 65
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
