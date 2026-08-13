#include "IR/IR1.h"
#include "IR/X86.h"
#include "Target/X86Register.h"

#include "Target/X86/MCTargetDesc/X86MCTargetDesc.h"
#include "mlir/ExecutionEngine/ExecutionEngine.h"
#include "llvm/Support/Error.h"
#include "llvm/Support/FileSystem.h"
#include "llvm/Support/TargetSelect.h"
#include "llvm/Support/raw_ostream.h"
#include "gtest/gtest.h"

#include <array>
#include <cstdint>
#include <memory>

using namespace mlir;
using namespace z8;

namespace {

using RegisterState =
    std::array<uint64_t, static_cast<size_t>(X86RegisterSlot::Count)>;
using TranslatedBlock = void (*)(uint64_t *);

uint64_t computeFlags(uint64_t lhs, uint64_t rhs, unsigned width) {
  const uint64_t mask = width == 64 ? ~uint64_t{0} : (uint64_t{1} << width) - 1;
  lhs &= mask;
  rhs &= mask;
  const uint64_t result = (lhs + rhs) & mask;
  const bool cf = width == 64 ? result < lhs : lhs + rhs > mask;
  const bool pf =
      (__builtin_popcount(static_cast<unsigned>(result & 0xff)) & 1) == 0;
  const bool af = ((lhs ^ rhs ^ result) & 0x10) != 0;
  const bool zf = result == 0;
  const bool sf = ((result >> (width - 1)) & 1) != 0;
  const bool of = ((~(lhs ^ rhs) & (lhs ^ result)) >> (width - 1)) & 1;
  return (uint64_t{cf} << 0) | (uint64_t{pf} << 2) | (uint64_t{af} << 4) |
         (uint64_t{zf} << 6) | (uint64_t{sf} << 7) | (uint64_t{of} << 11);
}

uint64_t readRegister(const RegisterState &state, unsigned llvmRegister) {
  auto desc = getX86RegisterDesc(llvmRegister);
  EXPECT_TRUE(desc.has_value());
  uint64_t value = state[static_cast<size_t>(desc->slot)] >> desc->bitOffset;
  return desc->width == 64 ? value : value & ((uint64_t{1} << desc->width) - 1);
}

void writeRegister(RegisterState &state, unsigned llvmRegister,
                   uint64_t value) {
  auto desc = getX86RegisterDesc(llvmRegister);
  ASSERT_TRUE(desc.has_value());
  uint64_t &slot = state[static_cast<size_t>(desc->slot)];
  if (desc->width == 64) {
    slot = value;
  } else if (desc->zeroExtendOnWrite) {
    slot = value & 0xffffffffU;
  } else {
    uint64_t mask = ((uint64_t{1} << desc->width) - 1) << desc->bitOffset;
    slot = (slot & ~mask) | ((value << desc->bitOffset) & mask);
  }
}

class AddProgram {
public:
  AddProgram(unsigned destination, unsigned source, unsigned width) {
    auto &builder = context.Builder;
    Location loc = builder.getUnknownLoc();
    Value lhs =
        x86ir::ReadRegOp::create(builder, loc, context.getState(), destination);
    Value rhs =
        x86ir::ReadRegOp::create(builder, loc, context.getState(), source);
    TypeRange resultTypes{builder.getIntegerType(width), builder.getI64Type()};
    auto add = x86ir::AddIOp::create(builder, loc, resultTypes, lhs, rhs);
    x86ir::WriteRegOp::create(builder, loc, context.getState(), add.getRes(),
                              destination);
    x86ir::WriteRegOp::create(builder, loc, context.getState(), add.getFlags(),
                              llvm::X86::RFLAGS);

    EXPECT_TRUE(succeeded(context.lower()));
    auto engineOrError = ExecutionEngine::create(context.Module);
    if (!engineOrError) {
      ADD_FAILURE() << llvm::toString(engineOrError.takeError());
      return;
    }
    engine = std::move(*engineOrError);
    auto addressOrError = engine->lookup("translated_block");
    if (!addressOrError) {
      ADD_FAILURE() << llvm::toString(addressOrError.takeError());
      return;
    }
    function = reinterpret_cast<TranslatedBlock>(*addressOrError);
  }

  void run(RegisterState &state) const {
    ASSERT_NE(function, nullptr);
    function(state.data());
  }

private:
  IR1Context context;
  std::unique_ptr<ExecutionEngine> engine;
  TranslatedBlock function = nullptr;
};

void checkAdd(unsigned destination, unsigned source, unsigned width,
              uint64_t destinationValue, uint64_t sourceValue,
              uint64_t initialFlags = 0x202) {
  AddProgram program(destination, source, width);
  RegisterState actual{};
  for (size_t i = 0; i < actual.size(); ++i)
    actual[i] = 0xa5a5000000000000ULL ^ (0x1111111111111111ULL * i);
  writeRegister(actual, destination, destinationValue);
  writeRegister(actual, source, sourceValue);
  actual[static_cast<size_t>(X86RegisterSlot::RFLAGS)] = initialFlags;
  RegisterState expected = actual;

  uint64_t lhs = readRegister(expected, destination);
  uint64_t rhs = readRegister(expected, source);
  const uint64_t result = lhs + rhs;
  writeRegister(expected, destination, result);
  uint64_t &flags = expected[static_cast<size_t>(X86RegisterSlot::RFLAGS)];
  flags = (flags & ~X86ArithmeticFlagsMask) | computeFlags(lhs, rhs, width);

  program.run(actual);
  EXPECT_EQ(actual, expected);
}

TEST(Lowering, Add64RegisterStateMatches) {
  checkAdd(llvm::X86::RAX, llvm::X86::RBX, 64, 0xffffffffffffffffULL, 1);
  checkAdd(llvm::X86::RAX, llvm::X86::RBX, 64, 0x7fffffffffffffffULL, 1);
}

TEST(Lowering, Add64DefinedFlagMatrixMatches) {
  // PF clear, with every arithmetic flag otherwise clear.
  checkAdd(llvm::X86::RAX, llvm::X86::RBX, 64, 0, 1);
  // CF and AF set while ZF remains clear.
  checkAdd(llvm::X86::RAX, llvm::X86::RBX, 64, 0xffffffffffffffffULL, 2);
  // SF set without signed overflow.
  checkAdd(llvm::X86::RAX, llvm::X86::RBX, 64, 0x8000000000000000ULL, 0);
  // Negative plus negative overflows to positive and carries unsigned.
  checkAdd(llvm::X86::RAX, llvm::X86::RBX, 64, 0x8000000000000000ULL,
           0xffffffffffffffffULL);
}

TEST(Lowering, Add32ZeroExtendsDestination) {
  checkAdd(llvm::X86::EAX, llvm::X86::EBX, 32, 0xffffffffU, 1);
  checkAdd(llvm::X86::EAX, llvm::X86::EBX, 32, 0x7fffffffU, 1);
}

TEST(Lowering, Add16PreservesUpperBits) {
  checkAdd(llvm::X86::AX, llvm::X86::BX, 16, 0xffff, 1);
}

TEST(Lowering, AddLowAndHighBytePreserveAliases) {
  checkAdd(llvm::X86::AL, llvm::X86::BL, 8, 0xff, 1);
  checkAdd(llvm::X86::AH, llvm::X86::BH, 8, 0x7f, 1);
}

TEST(CodeEmission, WritesNativeObject) {
  IR1Context context;
  ASSERT_TRUE(succeeded(context.lower()));
  SmallString<128> path;
  int fd = -1;
  ASSERT_FALSE(llvm::sys::fs::createTemporaryFile("mlir-sbt", "o", fd, path));
  if (fd >= 0)
    llvm::sys::fs::closeFile(fd);
  ASSERT_TRUE(succeeded(context.emitObject(path)));
  uint64_t size = 0;
  ASSERT_FALSE(llvm::sys::fs::file_size(path, size));
  EXPECT_GT(size, 0u);
  llvm::sys::fs::remove(path);
}

} // namespace

int main(int argc, char **argv) {
  llvm::InitializeNativeTarget();
  llvm::InitializeNativeTargetAsmPrinter();
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
