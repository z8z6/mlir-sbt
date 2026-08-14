#include "IR/IR0.h"
#include "IR/IR1.h"
#include "IR/X86.h"
#include "Target/X86Register.h"

#include "Target/X86/MCTargetDesc/X86MCTargetDesc.h"
#include "mlir/Dialect/ControlFlow/IR/ControlFlowOps.h"
#include "mlir/ExecutionEngine/ExecutionEngine.h"
#include "mlir/IR/Verifier.h"
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

class ConditionProgram {
public:
  explicit ConditionProgram(unsigned condition) {
    auto &builder = context.Builder;
    Location loc = builder.getUnknownLoc();
    Block &entry = context.Function.getBody().front();
    entry.getTerminator()->erase();
    auto *taken = new Block();
    auto *notTaken = new Block();
    context.Function.getBody().push_back(taken);
    context.Function.getBody().push_back(notTaken);

    builder.setInsertionPointToEnd(&entry);
    Value flags = x86ir::ReadRegOp::create(builder, loc, context.getState(),
                                           llvm::X86::RFLAGS);
    Value take = x86ir::ConditionOp::create(
        builder, loc, builder.getI1Type(), flags,
        builder.getI32IntegerAttr(static_cast<int32_t>(condition)));
    cf::CondBranchOp::create(builder, loc, take, taken, notTaken);

    builder.setInsertionPointToEnd(taken);
    Value one = ir1::ConstIntOp::create(builder, loc, 1);
    x86ir::WriteRegOp::create(builder, loc, context.getState(), one,
                              llvm::X86::RAX);
    func::ReturnOp::create(builder, loc);

    builder.setInsertionPointToEnd(notTaken);
    Value zero = ir1::ConstIntOp::create(builder, loc, 0);
    x86ir::WriteRegOp::create(builder, loc, context.getState(), zero,
                              llvm::X86::RAX);
    func::ReturnOp::create(builder, loc);

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

  bool run(uint64_t flags) const {
    RegisterState state{};
    state[static_cast<size_t>(X86RegisterSlot::RFLAGS)] = flags;
    function(state.data());
    return state[static_cast<size_t>(X86RegisterSlot::RAX)] != 0;
  }

private:
  IR1Context context;
  std::unique_ptr<ExecutionEngine> engine;
  TranslatedBlock function = nullptr;
};

bool evaluateCondition(unsigned condition, uint64_t flags) {
  bool cf = (flags >> 0) & 1;
  bool pf = (flags >> 2) & 1;
  bool zf = (flags >> 6) & 1;
  bool sf = (flags >> 7) & 1;
  bool of = (flags >> 11) & 1;
  switch (condition) {
  case 0:
    return of;
  case 1:
    return !of;
  case 2:
    return cf;
  case 3:
    return !cf;
  case 4:
    return zf;
  case 5:
    return !zf;
  case 6:
    return cf || zf;
  case 7:
    return !cf && !zf;
  case 8:
    return sf;
  case 9:
    return !sf;
  case 10:
    return pf;
  case 11:
    return !pf;
  case 12:
    return sf != of;
  case 13:
    return sf == of;
  case 14:
    return zf || (sf != of);
  case 15:
    return !zf && (sf == of);
  }
  return false;
}

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

TEST(Lowering, AllX86JumpConditionsMatchFlags) {
  for (unsigned condition = 0; condition < 16; ++condition) {
    ConditionProgram program(condition);
    for (unsigned bits = 0; bits < 32; ++bits) {
      uint64_t flags = ((bits >> 0) & 1) << 0 | ((bits >> 1) & 1) << 2 |
                       ((bits >> 2) & 1) << 6 | ((bits >> 3) & 1) << 7 |
                       ((bits >> 4) & 1) << 11;
      EXPECT_EQ(program.run(flags), evaluateCondition(condition, flags))
          << "condition=" << condition << " flags=" << flags;
    }
  }
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
  auto permissions = llvm::sys::fs::getPermissions(path);
  ASSERT_TRUE(static_cast<bool>(permissions));
  EXPECT_EQ(*permissions & llvm::sys::fs::perms::all_exe,
            llvm::sys::fs::perms::all_exe);
  llvm::sys::fs::remove(path);
}

TEST(IR0CFG, SplitsConditionalBranchIntoBasicBlocks) {
  llvm::MCInst branch;
  branch.setOpcode(llvm::X86::JCC_1);
  branch.addOperand(llvm::MCOperand::createImm(2));
  branch.addOperand(llvm::MCOperand::createImm(4));

  llvm::MCInst fallthroughReturn;
  fallthroughReturn.setOpcode(llvm::X86::RET64);
  llvm::MCInst takenReturn;
  takenReturn.setOpcode(llvm::X86::RET64);

  IR0Function function;
  function.Name = "branch";
  function.Address = 0x100;
  function.Size = 5;
  function.IRs.emplace_back(branch, 0x100, 2, 1);
  function.IRs.emplace_back(fallthroughReturn, 0x102, 2, 1);
  function.IRs.emplace_back(takenReturn, 0x104, 1, 1);

  ASSERT_TRUE(function.buildCFG());
  ASSERT_EQ(function.CFG.Blocks.size(), 3u);
  const IR0BasicBlock &entry = function.CFG.Blocks[0];
  EXPECT_EQ(entry.BeginIndex, 0u);
  EXPECT_EQ(entry.EndIndex, 1u);
  ASSERT_EQ(entry.Successors.size(), 2u);
  EXPECT_EQ(entry.Successors[0].Kind, IR0CFGEdgeKind::Branch);
  EXPECT_EQ(entry.Successors[0].TargetBlock, 2u);
  EXPECT_EQ(entry.Successors[1].Kind, IR0CFGEdgeKind::Fallthrough);
  EXPECT_EQ(entry.Successors[1].TargetBlock, 1u);
  EXPECT_TRUE(function.CFG.Blocks[0].Reachable);
  EXPECT_TRUE(function.CFG.Blocks[1].Reachable);
  EXPECT_TRUE(function.CFG.Blocks[2].Reachable);
}

TEST(IR0Translation, CreatesOneIR1FunctionPerIR0Function) {
  IR0Context source;
  for (size_t index = 0; index < 2; ++index) {
    llvm::MCInst instruction;
    instruction.setOpcode(llvm::X86::RET64);
    IR0Function function;
    function.Name = index == 0 ? "helper" : "main";
    function.Address = 0x100 + index * 0x10;
    function.Size = 1;
    function.IRs.emplace_back(instruction, function.Address, 1, 1);
    function.IRs.back().FunctionName = function.Name;
    function.IRs.back().DebugText =
        index == 0 ? "0x00000100: ret" : "0x00000110: ret";
    ASSERT_TRUE(function.buildCFG());
    source.Functions.push_back(std::move(function));
  }
  source.EntryFunctionIndex = 1;
  EXPECT_EQ(source.translatedCodeSize(), 2u);

  IR1Context destination;
  ASSERT_TRUE(succeeded(destination.convert(source)));
  EXPECT_EQ(destination.TranslatedFunctions.size(), 2u);
  EXPECT_TRUE(destination.Module.lookupSymbol<func::FuncOp>("helper"));
  EXPECT_TRUE(destination.Module.lookupSymbol<func::FuncOp>("main"));
  EXPECT_TRUE(
      destination.Module.lookupSymbol<func::FuncOp>("translated_block"));
  EXPECT_FALSE(destination.Module.lookupSymbol<func::FuncOp>(
      "translated_function_0"));
  EXPECT_TRUE(succeeded(mlir::verify(destination.Module)));
  std::string text;
  llvm::raw_string_ostream output(text);
  OpPrintingFlags flags;
  flags.enableDebugInfo();
  destination.Module->print(output, flags);
  output.flush();
  EXPECT_NE(text.find("ir0.function = \"main\""), std::string::npos);
  EXPECT_NE(text.find("loc(\"main\""), std::string::npos);
  EXPECT_NE(text.find("loc(\"0x00000110: ret\")"), std::string::npos);
}

TEST(IR0Translation, UnsupportedFunctionBeforeEntryIsSkipped) {
  IR0Context source;

  llvm::MCInst unsupported;
  unsupported.setOpcode(llvm::X86::CLC);
  llvm::MCInst returnInstruction;
  returnInstruction.setOpcode(llvm::X86::RET64);
  IR0Function fini;
  fini.Name = "_fini";
  fini.Address = 0x100;
  fini.Size = 2;
  fini.IRs.emplace_back(unsupported, fini.Address, 1, 1);
  fini.IRs.emplace_back(returnInstruction, fini.Address + 1, 1, 1);
  for (IR0 &instruction : fini.IRs)
    instruction.FunctionName = fini.Name;
  ASSERT_TRUE(fini.buildCFG());
  source.Functions.push_back(std::move(fini));

  IR0Function entry;
  entry.Name = "main";
  entry.Address = 0x200;
  entry.Size = 1;
  entry.IRs.emplace_back(returnInstruction, entry.Address, 1, 1);
  entry.IRs.back().FunctionName = entry.Name;
  entry.IRs.back().DebugText = "0x00000200: ret";
  ASSERT_TRUE(entry.buildCFG());
  source.Functions.push_back(std::move(entry));
  source.EntryFunctionIndex = 1;

  IR1Context destination;
  testing::internal::CaptureStderr();
  EXPECT_TRUE(succeeded(destination.convert(source)));
  std::string warning = testing::internal::GetCapturedStderr();
  EXPECT_NE(warning.find("warning: skipping function '_fini'"),
            std::string::npos);
  EXPECT_NE(warning.find("unsupported instruction"), std::string::npos);
  ASSERT_EQ(destination.TranslatedFunctions.size(), 1u);
  EXPECT_TRUE(destination.Function);
  EXPECT_TRUE(destination.State);
  EXPECT_TRUE(
      destination.Module.lookupSymbol<func::FuncOp>("translated_block"));
  EXPECT_TRUE(destination.Module.lookupSymbol<func::FuncOp>("main"));
  EXPECT_FALSE(destination.Module.lookupSymbol<func::FuncOp>("_fini"));
  EXPECT_TRUE(succeeded(mlir::verify(destination.Module)));
}

TEST(IR0Translation, UnsupportedEntryDoesNotStopRemainingFunctions) {
  IR0Context source;

  llvm::MCInst unsupported;
  unsupported.setOpcode(llvm::X86::CLC);
  llvm::MCInst returnInstruction;
  returnInstruction.setOpcode(llvm::X86::RET64);

  IR0Function entry;
  entry.Name = "_start";
  entry.Address = 0x100;
  entry.Size = 2;
  entry.IRs.emplace_back(unsupported, entry.Address, 1, 1);
  entry.IRs.emplace_back(returnInstruction, entry.Address + 1, 1, 1);
  ASSERT_TRUE(entry.buildCFG());
  source.Functions.push_back(std::move(entry));

  IR0Function helper;
  helper.Name = "helper";
  helper.Address = 0x200;
  helper.Size = 1;
  helper.IRs.emplace_back(returnInstruction, helper.Address, 1, 1);
  helper.IRs.back().FunctionName = helper.Name;
  helper.IRs.back().DebugText = "0x00000200: ret";
  ASSERT_TRUE(helper.buildCFG());
  source.Functions.push_back(std::move(helper));
  source.EntryFunctionIndex = 0;

  IR1Context destination;
  testing::internal::CaptureStderr();
  EXPECT_TRUE(succeeded(destination.convert(source)));
  std::string warning = testing::internal::GetCapturedStderr();
  EXPECT_NE(warning.find("warning: skipping function '_start'"),
            std::string::npos);
  ASSERT_EQ(destination.TranslatedFunctions.size(), 1u);
  EXPECT_FALSE(destination.Function);
  EXPECT_FALSE(destination.State);
  EXPECT_FALSE(destination.Module.lookupSymbol<func::FuncOp>(
      "translated_block"));
  EXPECT_TRUE(destination.Module.lookupSymbol<func::FuncOp>("helper"));
  EXPECT_TRUE(succeeded(mlir::verify(destination.Module)));
}

} // namespace

int main(int argc, char **argv) {
  llvm::InitializeNativeTarget();
  llvm::InitializeNativeTargetAsmPrinter();
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
