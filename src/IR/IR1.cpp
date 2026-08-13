//
// Created by zzm on 2026/6/30
// Part of RVision
//
#include "IR/IR1.h"
#include "IR/IR0.h"
#include "IR/IR1Converter.h"
#include "Pass/IR1Lowering.h"
#include "Target/X86Register.h"
#include "mlir/Dialect/Arith/IR/Arith.h"
#include "mlir/Dialect/Func/IR/FuncOps.h"
#include "mlir/Dialect/LLVMIR/LLVMDialect.h"
#include "mlir/Dialect/MemRef/IR/MemRef.h"
#include "mlir/Target/LLVMIR/Dialect/LLVMIR/LLVMToLLVMIRTranslation.h"
#include "mlir/Target/LLVMIR/Dialect/Builtin/BuiltinToLLVMIRTranslation.h"
#include "mlir/Target/LLVMIR/Export.h"
#include "mlir/Transforms/Passes.h"

#include "mlir/IR/BuiltinTypes.h"
#include "mlir/IR/MLIRContext.h"
#include "mlir/IR/Verifier.h"
#include "mlir/IR/Diagnostics.h"

#include "llvm/IR/LegacyPassManager.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Module.h"
#include "llvm/MC/TargetRegistry.h"
#include "llvm/Support/CodeGen.h"
#include "llvm/Support/FileSystem.h"
#include "llvm/Support/TargetSelect.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Target/TargetMachine.h"
#include "llvm/TargetParser/Host.h"

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

void ConstIntOp::build(OpBuilder &builder, OperationState &state, int64_t value) {
  auto dataType = builder.getI64Type();
  auto dataAttribute = IntegerAttr::get(dataType, value);
  build(builder, state, dataType, dataAttribute);
}

void LoadRegOp::build(OpBuilder &builder, OperationState &state,
                      mlir::Value registerState, unsigned id) {
  auto reg = z8::getX86RegisterDesc(id);
  assert(reg && "unsupported x86 register");
  auto dataType = builder.getIntegerType(reg->width);
  auto dataAttribute = IntegerAttr::get(dataType, id);
  build(builder, state, dataType, registerState, dataAttribute);
}

void StoreRegOp::build(OpBuilder &builder, OperationState &state,
                       mlir::Value registerState, mlir::Value value,
                       unsigned id) {
  auto dataType = builder.getI32Type();
  auto dataAttribute = IntegerAttr::get(dataType, id);
  build(builder, state, TypeRange{}, registerState, value, dataAttribute);
}


#define GET_OP_CLASSES
#include "mlir/IR1Ops.cpp.inc"


IR1Context::IR1Context() : Builder(&Ctx)  {
  Ctx.getOrLoadDialect<IR1Dialect>();
  Ctx.getOrLoadDialect<arith::ArithDialect>();
  Ctx.getOrLoadDialect<func::FuncDialect>();
  Ctx.getOrLoadDialect<LLVM::LLVMDialect>();
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
  PM.addPass(createIR1LowerPass());
  if (failed(PM.run(Module)))
    return failure();
  return mlir::verify(Module);
}

LogicalResult IR1Context::emitObject(StringRef outputPath) {
  if (llvm::InitializeNativeTarget() || llvm::InitializeNativeTargetAsmPrinter())
    return Module.emitError("cannot initialize native LLVM target");
  llvm::LLVMContext llvmContext;
  std::unique_ptr<llvm::Module> llvmModule =
      translateModuleToLLVMIR(Module, llvmContext, "translated_block");
  if (!llvmModule)
    return Module.emitError("failed to translate LLVM dialect to LLVM IR");

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
  if (targetMachine->addPassesToEmitFile(
          passes, output, nullptr, llvm::CodeGenFileType::ObjectFile))
    return Module.emitError("target cannot emit an object file");
  passes.run(*llvmModule);
  output.flush();
  return success();
}

mlir::Type IR1Context::iTy(
    int width, mlir::IntegerType::SignednessSemantics signedness) {
  return mlir::IntegerType::get(&Ctx, width, signedness);
}

LogicalResult IR1Context::convert(IR0Context& IR0Ctx) {
  ConversionFailed = false;
  for (auto &IR : IR0Ctx.IRs) {
    convertMCInst(IR);
  }
  return ConversionFailed ? failure() : success();
}

void IR1Context::print(bool withLoc) {
  OpPrintingFlags flags;
  flags.enableDebugInfo(withLoc);
  Module->print(dbgs(), flags);
}
