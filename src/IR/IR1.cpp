//
// Created by zzm on 2026/6/30
// Part of RVision
//
#include "IR/IR1.h"
#include "IR/IR0.h"
#include "IR/IR1Converter.h"
#include "IR/X86.h"
#include "Pass/IR1Lowering.h"
#include "Pass/X86Lowering.h"
#include "mlir/Dialect/Arith/IR/Arith.h"
#include "mlir/Dialect/ControlFlow/IR/ControlFlowOps.h"
#include "mlir/Dialect/Func/IR/FuncOps.h"
#include "mlir/Dialect/LLVMIR/LLVMDialect.h"
#include "mlir/Dialect/Math/IR/Math.h"
#include "mlir/Dialect/MemRef/IR/MemRef.h"
#include "mlir/Target/LLVMIR/Dialect/Builtin/BuiltinToLLVMIRTranslation.h"
#include "mlir/Target/LLVMIR/Dialect/LLVMIR/LLVMToLLVMIRTranslation.h"
#include "mlir/Target/LLVMIR/Export.h"
#include "mlir/Transforms/Passes.h"

#include "mlir/IR/BuiltinTypes.h"
#include "mlir/IR/Diagnostics.h"
#include "mlir/IR/MLIRContext.h"
#include "mlir/IR/Verifier.h"

#include "llvm/ADT/DenseMap.h"
#include "llvm/ADT/ScopeExit.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/LegacyPassManager.h"
#include "llvm/IR/Module.h"
#include "llvm/MC/TargetRegistry.h"
#include "llvm/Support/CodeGen.h"
#include "llvm/Support/FileSystem.h"
#include "llvm/Support/TargetSelect.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Target/TargetMachine.h"
#include "llvm/TargetParser/Host.h"

#include "Target/X86/MCTargetDesc/X86MCTargetDesc.h"
#include "tblgen/X86IR1Converter.h"

#include <vector>

using namespace llvm;
using namespace mlir;
using namespace z8;
using namespace z8::ir1;

#include "mlir/IR1Dialect.cpp.inc"

void IR1Dialect::initialize() {
  addOperations<
#define GET_OP_LIST
#include "mlir/IR1Ops.cpp.inc"
      >();
}

void ConstIntOp::build(OpBuilder &builder, OperationState &state,
                       int64_t value) {
  auto dataType = builder.getI64Type();
  auto dataAttribute = IntegerAttr::get(dataType, value);
  build(builder, state, dataType, dataAttribute);
}

#define GET_OP_CLASSES
#include "mlir/IR1Ops.cpp.inc"

IR1Context::IR1Context() : Builder(&Ctx) {
  Ctx.getOrLoadDialect<IR1Dialect>();
  Ctx.getOrLoadDialect<x86ir::X86Dialect>();
  Ctx.getOrLoadDialect<arith::ArithDialect>();
  Ctx.getOrLoadDialect<cf::ControlFlowDialect>();
  Ctx.getOrLoadDialect<func::FuncDialect>();
  Ctx.getOrLoadDialect<LLVM::LLVMDialect>();
  Ctx.getOrLoadDialect<math::MathDialect>();
  Ctx.getOrLoadDialect<memref::MemRefDialect>();
  registerBuiltinDialectTranslation(Ctx);
  registerLLVMDialectTranslation(Ctx);

  Module = ModuleOp::create(Builder.getUnknownLoc());
  Builder.setInsertionPointToStart(Module.getBody());
  auto ptrType = LLVM::LLVMPointerType::get(&Ctx);
  auto functionType = Builder.getFunctionType(TypeRange{ptrType}, TypeRange{});
  Function = func::FuncOp::create(Builder, Builder.getUnknownLoc(),
                                  "translated_block", functionType);
  Block *entry = Function.addEntryBlock();
  State = entry->getArgument(0);
  Builder.setInsertionPointToEnd(entry);
  auto returnOp = func::ReturnOp::create(Builder, Builder.getUnknownLoc());
  Builder.setInsertionPoint(returnOp);
}

void IR1Context::verify() {
  if (failed(mlir::verify(Module)))
    Module.emitError("module verification error");
}

LogicalResult IR1Context::lower() {
  if (failed(mlir::verify(Module)))
    return failure();
  mlir::PassManager PM(Module->getName());
  PM.addPass(createX86LowerPass());
  PM.addPass(createIR1LowerPass());
  if (failed(PM.run(Module)))
    return failure();
  return mlir::verify(Module);
}

LogicalResult IR1Context::emitObject(StringRef outputPath) {
  if (llvm::InitializeNativeTarget() ||
      llvm::InitializeNativeTargetAsmPrinter())
    return Module.emitError("cannot initialize native LLVM target");
  llvm::LLVMContext llvmContext;
  std::unique_ptr<llvm::Module> llvmModule =
      translateModuleToLLVMIR(Module, llvmContext, "translated_block");
  if (!llvmModule)
    return Module.emitError("failed to translate LLVM dialect to LLVM IR");
  if (!EntryFunctionName.empty() && EntryFunctionName != "translated_block") {
    llvm::Function *entry = llvmModule->getFunction(EntryFunctionName);
    if (!entry)
      return Module.emitError("cannot find translated entry function: ")
             << EntryFunctionName;
    entry->setLinkage(llvm::GlobalValue::InternalLinkage);
  }

  std::string triple = llvm::sys::getDefaultTargetTriple();
  std::string error;
  const llvm::Target *target =
      llvm::TargetRegistry::lookupTarget(llvm::Triple(triple), error);
  if (!target)
    return Module.emitError("cannot find target: ") << error;

  llvm::TargetOptions options;
  std::unique_ptr<llvm::TargetMachine> targetMachine(
      target->createTargetMachine(llvm::Triple(triple), "generic", "", options,
                                  std::nullopt));
  if (!targetMachine)
    return Module.emitError("cannot create target machine");

  llvmModule->setTargetTriple(targetMachine->getTargetTriple());
  llvmModule->setDataLayout(targetMachine->createDataLayout());

  std::error_code ec;
  llvm::raw_fd_ostream output(outputPath, ec, llvm::sys::fs::OF_None);
  if (ec)
    return Module.emitError("cannot open output file: ") << ec.message();

  llvm::legacy::PassManager passes;
  if (targetMachine->addPassesToEmitFile(passes, output, nullptr,
                                         llvm::CodeGenFileType::ObjectFile))
    return Module.emitError("target cannot emit an object file");
  passes.run(*llvmModule);
  output.flush();
  output.close();

  ErrorOr<sys::fs::perms> permissions = sys::fs::getPermissions(outputPath);
  if (!permissions)
    return Module.emitError("cannot read output permissions: ")
           << permissions.getError().message();
  if (std::error_code permissionsError =
          sys::fs::setPermissions(outputPath,
                                  *permissions | sys::fs::perms::all_exe))
    return Module.emitError("cannot make output executable: ")
           << permissionsError.message();
  return success();
}

mlir::Type IR1Context::iTy(int width,
                           mlir::IntegerType::SignednessSemantics signedness) {
  return mlir::IntegerType::get(&Ctx, width, signedness);
}

LogicalResult IR1Context::convert(IR0Context &IR0Ctx) {
  IR1Context *previousContext = BaseIR1Converter::Ctx;
  BaseIR1Converter::Ctx = this;
  llvm::scope_exit restoreContext(
      [previousContext] { BaseIR1Converter::Ctx = previousContext; });
  ConversionFailed = false;
  EntryFunctionName.clear();
  if (IR0Ctx.empty())
    return success();
  if (IR0Ctx.EntryFunctionIndex >= IR0Ctx.Functions.size())
    return failure();

  llvm::SmallVector<func::FuncOp> oldFunctions;
  for (func::FuncOp function : Module.getOps<func::FuncOp>())
    oldFunctions.push_back(function);
  for (func::FuncOp function : oldFunctions)
    function.erase();
  TranslatedFunctions.clear();
  Builder.setInsertionPointToEnd(Module.getBody());

  auto ptrType = LLVM::LLVMPointerType::get(&Ctx);
  auto functionType = Builder.getFunctionType(TypeRange{ptrType}, TypeRange{});
  func::FuncOp entryFunction;

  for (size_t functionIndex = 0; functionIndex < IR0Ctx.Functions.size();
       ++functionIndex) {
    IR0Function &source = IR0Ctx.Functions[functionIndex];
    if (source.IRs.empty() || source.CFG.Blocks.empty()) {
      errs() << "warning: skipping function '" << source.Name
             << "': empty instruction list or CFG\n";
      continue;
    }

    const IR0 *unsupportedInstruction = nullptr;
    for (const IR0BasicBlock &block : source.CFG.Blocks) {
      if (!block.Reachable)
        continue;
      for (size_t instructionIndex = block.BeginIndex;
           instructionIndex < block.EndIndex; ++instructionIndex) {
        const IR0 &instruction = source.IRs[instructionIndex];
        unsigned opcode = instruction.Inst.getOpcode();
        if (opcode != X86::RET64 && opcode != X86::LOCK_PREFIX &&
            !hasX86Converter(opcode)) {
          unsupportedInstruction = &instruction;
          break;
        }
      }
      if (unsupportedInstruction)
        break;
    }
    if (unsupportedInstruction) {
      errs() << "warning: skipping function '" << source.Name
             << "': unsupported instruction at "
             << format_hex(unsupportedInstruction->Addr, 10) << " (opcode "
             << unsupportedInstruction->Inst.getOpcode() << ")\n";
      continue;
    }

    const IR0 *failedInstruction = nullptr;
    StringRef failureReason;
    ConversionFailed = false;

    ModuleOp stagedModule = ModuleOp::create(Builder.getUnknownLoc());
    Builder.setInsertionPointToEnd(stagedModule.getBody());
    Location functionLoc = NameLoc::get(Builder.getStringAttr(source.Name));
    Function =
        func::FuncOp::create(Builder, functionLoc, source.Name, functionType);
    Function->setAttr("ir0.function", Builder.getStringAttr(source.Name));
    Block *entry = Function.addEntryBlock();
    State = entry->getArgument(0);

    std::vector<Block *> blocks(source.CFG.Blocks.size());
    for (size_t blockIndex = 0; blockIndex < source.CFG.Blocks.size();
         ++blockIndex) {
      if (!source.CFG.Blocks[blockIndex].Reachable)
        continue;
      blocks[blockIndex] = new Block();
      Function.getBody().push_back(blocks[blockIndex]);
    }
    if (!blocks.front()) {
      markConversionFailure();
      failureReason = "CFG entry block is unreachable";
    }

    if (!hasConversionFailed()) {
      Builder.setInsertionPointToEnd(entry);
      cf::BranchOp::create(Builder, functionLoc, blocks.front());
    }

    auto edgeTarget = [&](const IR0BasicBlock &block,
                          IR0CFGEdgeKind kind) -> Block * {
      auto edge = llvm::find_if(block.Successors, [&](const IR0CFGEdge &value) {
        return value.Kind == kind;
      });
      if (edge == block.Successors.end() || !edge->TargetBlock ||
          *edge->TargetBlock >= blocks.size())
        return nullptr;
      return blocks[*edge->TargetBlock];
    };

    for (size_t blockIndex = 0; blockIndex < source.CFG.Blocks.size();
         ++blockIndex) {
      const IR0BasicBlock &sourceBlock = source.CFG.Blocks[blockIndex];
      if (!sourceBlock.Reachable)
        continue;
      Block *destinationBlock = blocks[blockIndex];
      Builder.setInsertionPointToEnd(destinationBlock);

      for (size_t instructionIndex = sourceBlock.BeginIndex;
           instructionIndex < sourceBlock.EndIndex; ++instructionIndex) {
        const IR0 &instruction = source.IRs[instructionIndex];
        unsigned opcode = instruction.Inst.getOpcode();
        bool last = instructionIndex + 1 == sourceBlock.EndIndex;
        bool jump = opcode == X86::JMP_1 || opcode == X86::JMP_4;
        bool conditional = opcode == X86::JCC_1 || opcode == X86::JCC_4;

        if (!last && (jump || conditional || opcode == X86::RET64)) {
          markConversionFailure();
          failedInstruction = &instruction;
          failureReason = "terminator is not the final instruction in its block";
          break;
        }
        if (opcode == X86::RET64) {
          ConversionContext context(instruction);
          func::ReturnOp::create(Builder, context.getNameLoc());
          continue;
        }

        Block *branchTarget = nullptr;
        Block *fallthrough = nullptr;
        if (jump || conditional) {
          branchTarget = edgeTarget(sourceBlock, IR0CFGEdgeKind::Branch);
          if (conditional)
            fallthrough = edgeTarget(sourceBlock, IR0CFGEdgeKind::Fallthrough);
          if (!branchTarget || (conditional && !fallthrough)) {
            markConversionFailure();
            failedInstruction = &instruction;
            failureReason = "control-flow target is unresolved";
            break;
          }
        }
        convertMCInst(instruction, branchTarget, fallthrough);
        if (hasConversionFailed()) {
          failedInstruction = &instruction;
          failureReason = "instruction converter is unavailable or failed";
          break;
        }
      }
      if (hasConversionFailed())
        break;

      const IR0 &last = source.IRs[sourceBlock.EndIndex - 1];
      unsigned opcode = last.Inst.getOpcode();
      bool terminated = opcode == X86::RET64 || opcode == X86::JMP_1 ||
                        opcode == X86::JMP_4 || opcode == X86::JCC_1 ||
                        opcode == X86::JCC_4;
      if (terminated)
        continue;
      Block *fallthrough = edgeTarget(sourceBlock, IR0CFGEdgeKind::Fallthrough);
      if (!fallthrough) {
        markConversionFailure();
        failedInstruction = &last;
        failureReason = "fallthrough target is unresolved";
        break;
      }
      ConversionContext context(last);
      cf::BranchOp::create(Builder, context.getNameLoc(), fallthrough);
    }
    if (hasConversionFailed()) {
      errs() << "warning: skipping function '" << source.Name << "': "
             << failureReason;
      if (failedInstruction)
        errs() << " at " << format_hex(failedInstruction->Addr, 10)
               << " (opcode " << failedInstruction->Inst.getOpcode() << ')';
      errs() << '\n';
      Builder.setInsertionPointToEnd(Module.getBody());
      Function = {};
      State = {};
      ConversionFailed = false;
      stagedModule.erase();
      continue;
    }

    Function->remove();
    Module.getBody()->push_back(Function);
    stagedModule.erase();
    TranslatedFunctions.push_back(Function);
    if (functionIndex == IR0Ctx.EntryFunctionIndex)
      entryFunction = Function;
  }

  if (entryFunction) {
    EntryFunctionName = entryFunction.getSymName().str();
    if (entryFunction.getSymName() != "translated_block") {
      entryFunction.setPrivate();
      Builder.setInsertionPointToEnd(Module.getBody());
      Location wrapperLoc = NameLoc::get(
          Builder.getStringAttr(entryFunction.getSymName()));
      func::FuncOp wrapper = func::FuncOp::create(
          Builder, wrapperLoc, "translated_block", functionType);
      Block *wrapperEntry = wrapper.addEntryBlock();
      Builder.setInsertionPointToEnd(wrapperEntry);
      func::CallOp::create(Builder, wrapperLoc, entryFunction,
                           wrapperEntry->getArguments());
      func::ReturnOp::create(Builder, wrapperLoc);
    }
    Function = entryFunction;
    State = Function.getArgument(0);
  } else {
    Function = {};
    State = {};
  }
  ConversionFailed = false;
  return success();
}

void IR1Context::print(bool withLoc) {
  OpPrintingFlags flags;
  flags.enableDebugInfo(withLoc);
  Module->print(outs(), flags);
  outs() << '\n';
}
