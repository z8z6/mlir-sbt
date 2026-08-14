#include "Pass/IR1Lowering.h"

#include "IR/IR1.h"

#include "mlir/Conversion/ArithToLLVM/ArithToLLVM.h"
#include "mlir/Conversion/ControlFlowToLLVM/ControlFlowToLLVM.h"
#include "mlir/Conversion/FuncToLLVM/ConvertFuncToLLVM.h"
#include "mlir/Conversion/LLVMCommon/ConversionTarget.h"
#include "mlir/Conversion/LLVMCommon/TypeConverter.h"
#include "mlir/Conversion/MathToLLVM/MathToLLVM.h"
#include "mlir/Dialect/Arith/IR/Arith.h"
#include "mlir/Dialect/ControlFlow/IR/ControlFlowOps.h"
#include "mlir/Dialect/Func/IR/FuncOps.h"
#include "mlir/Dialect/LLVMIR/LLVMDialect.h"
#include "mlir/Dialect/Math/IR/Math.h"
#include "mlir/IR/BuiltinDialect.h"
#include "mlir/Pass/Pass.h"
#include "mlir/Transforms/DialectConversion.h"

using namespace mlir;
using namespace z8;

namespace {

Value integerConstant(OpBuilder &builder, Location loc, IntegerType type,
                      uint64_t value) {
  return arith::ConstantIntOp::create(builder, loc, type,
                                      APInt(type.getWidth(), value,
                                            /*isSigned=*/false,
                                            /*implicitTrunc=*/true));
}

Value integerConstant(OpBuilder &builder, Location loc, IntegerType type,
                      const APInt &value) {
  return arith::ConstantIntOp::create(builder, loc, type, value);
}

Value castInteger(PatternRewriter &rewriter, Location loc, Value value,
                  IntegerType targetType) {
  auto sourceType = cast<IntegerType>(value.getType());
  if (sourceType == targetType)
    return value;
  if (sourceType.getWidth() > targetType.getWidth())
    return arith::TruncIOp::create(rewriter, loc, targetType, value);
  return arith::ExtUIOp::create(rewriter, loc, targetType, value);
}

Value stateSlotPointer(PatternRewriter &rewriter, Location loc, Value state,
                       unsigned slot) {
  auto ptrType = LLVM::LLVMPointerType::get(rewriter.getContext());
  auto i64Type = rewriter.getI64Type();
  return LLVM::GEPOp::create(
      rewriter, loc, ptrType, i64Type, state,
      ArrayRef<LLVM::GEPArg>{static_cast<int32_t>(slot)});
}

struct ConstIntOpLowering : public OpConversionPattern<ir1::ConstIntOp> {
  using OpConversionPattern::OpConversionPattern;

  LogicalResult
  matchAndRewrite(ir1::ConstIntOp op, OpAdaptor,
                  ConversionPatternRewriter &rewriter) const final {
    auto value = cast<IntegerAttr>(op.getValue());
    auto type = cast<IntegerType>(op.getType());
    rewriter.replaceOpWithNewOp<arith::ConstantIntOp>(op, type,
                                                      value.getValue());
    return success();
  }
};

struct LoadStateOpLowering : public OpConversionPattern<ir1::LoadStateOp> {
  using OpConversionPattern::OpConversionPattern;

  LogicalResult
  matchAndRewrite(ir1::LoadStateOp op, OpAdaptor adaptor,
                  ConversionPatternRewriter &rewriter) const final {
    Location loc = op.getLoc();
    auto storageType = rewriter.getIntegerType(op.getStorageWidth());
    Value ptr =
        stateSlotPointer(rewriter, loc, adaptor.getState(), op.getSlot());
    Value value = LLVM::LoadOp::create(rewriter, loc, storageType, ptr, 8);
    if (op.getBitOffset() != 0)
      value = arith::ShRUIOp::create(
          rewriter, loc, value,
          integerConstant(rewriter, loc, storageType, op.getBitOffset()));
    auto resultType = cast<IntegerType>(op.getType());
    value = castInteger(rewriter, loc, value, resultType);
    rewriter.replaceOp(op, value);
    return success();
  }
};

struct StoreStateOpLowering : public OpConversionPattern<ir1::StoreStateOp> {
  using OpConversionPattern::OpConversionPattern;

  LogicalResult
  matchAndRewrite(ir1::StoreStateOp op, OpAdaptor adaptor,
                  ConversionPatternRewriter &rewriter) const final {
    Location loc = op.getLoc();
    auto storageType = rewriter.getIntegerType(op.getStorageWidth());
    Value ptr =
        stateSlotPointer(rewriter, loc, adaptor.getState(), op.getSlot());
    Value value = castInteger(rewriter, loc, adaptor.getValue(), storageType);
    if (op.getBitOffset() != 0)
      value = arith::ShLIOp::create(
          rewriter, loc, value,
          integerConstant(rewriter, loc, storageType, op.getBitOffset()));
    APInt mask = cast<IntegerAttr>(op.getWriteMask()).getValue();
    if (!mask.isAllOnes()) {
      Value maskValue = integerConstant(rewriter, loc, storageType, mask);
      value = arith::AndIOp::create(rewriter, loc, value, maskValue);
      Value old = LLVM::LoadOp::create(rewriter, loc, storageType, ptr, 8);
      Value keepMask = integerConstant(rewriter, loc, storageType, ~mask);
      old = arith::AndIOp::create(rewriter, loc, old, keepMask);
      value = arith::OrIOp::create(rewriter, loc, old, value);
    }

    LLVM::StoreOp::create(rewriter, loc, value, ptr, 8);
    rewriter.eraseOp(op);
    return success();
  }
};

struct AddIOpLowering : public OpConversionPattern<ir1::AddIOp> {
  using OpConversionPattern::OpConversionPattern;

  LogicalResult
  matchAndRewrite(ir1::AddIOp op, OpAdaptor adaptor,
                  ConversionPatternRewriter &rewriter) const final {
    auto type = cast<IntegerType>(op.getType());
    Value lhs = castInteger(rewriter, op.getLoc(), adaptor.getLhs(), type);
    Value rhs = castInteger(rewriter, op.getLoc(), adaptor.getRhs(), type);
    rewriter.replaceOpWithNewOp<arith::AddIOp>(op, lhs, rhs);
    return success();
  }
};

struct MulIOpLowering : public OpConversionPattern<ir1::MulIOp> {
  using OpConversionPattern::OpConversionPattern;

  LogicalResult
  matchAndRewrite(ir1::MulIOp op, OpAdaptor adaptor,
                  ConversionPatternRewriter &rewriter) const final {
    auto type = cast<IntegerType>(op.getType());
    Value lhs = castInteger(rewriter, op.getLoc(), adaptor.getLhs(), type);
    Value rhs = castInteger(rewriter, op.getLoc(), adaptor.getRhs(), type);
    rewriter.replaceOpWithNewOp<arith::MulIOp>(op, lhs, rhs);
    return success();
  }
};

struct LoadOpLowering : public OpConversionPattern<ir1::LoadOp> {
  using OpConversionPattern::OpConversionPattern;

  LogicalResult
  matchAndRewrite(ir1::LoadOp op, OpAdaptor adaptor,
                  ConversionPatternRewriter &rewriter) const final {
    auto ptrType = LLVM::LLVMPointerType::get(rewriter.getContext());
    Value address = castInteger(rewriter, op.getLoc(), adaptor.getAddr(),
                                rewriter.getI64Type());
    Value ptr =
        LLVM::IntToPtrOp::create(rewriter, op.getLoc(), ptrType, address);
    rewriter.replaceOpWithNewOp<LLVM::LoadOp>(op, op.getType(), ptr, 1);
    return success();
  }
};

struct StoreOpLowering : public OpConversionPattern<ir1::StoreOp> {
  using OpConversionPattern::OpConversionPattern;

  LogicalResult
  matchAndRewrite(ir1::StoreOp op, OpAdaptor adaptor,
                  ConversionPatternRewriter &rewriter) const final {
    auto ptrType = LLVM::LLVMPointerType::get(rewriter.getContext());
    Value address = castInteger(rewriter, op.getLoc(), adaptor.getAddr(),
                                rewriter.getI64Type());
    Value ptr =
        LLVM::IntToPtrOp::create(rewriter, op.getLoc(), ptrType, address);
    LLVM::StoreOp::create(rewriter, op.getLoc(), adaptor.getValue(), ptr, 1);
    rewriter.eraseOp(op);
    return success();
  }
};

struct SyscallOpLowering : public OpConversionPattern<ir1::SyscallOp> {
  using OpConversionPattern::OpConversionPattern;

  LogicalResult
  matchAndRewrite(ir1::SyscallOp op, OpAdaptor adaptor,
                  ConversionPatternRewriter &rewriter) const final {
    constexpr StringLiteral RuntimeName = "__sbt_syscall6";
    ModuleOp module = op->getParentOfType<ModuleOp>();
    auto runtime = module.lookupSymbol<LLVM::LLVMFuncOp>(RuntimeName);
    if (!runtime) {
      OpBuilder::InsertionGuard guard(rewriter);
      rewriter.setInsertionPointToStart(module.getBody());
      SmallVector<Type> arguments(7, rewriter.getI64Type());
      auto functionType = LLVM::LLVMFunctionType::get(
          rewriter.getI64Type(), arguments, /*isVarArg=*/false);
      runtime = LLVM::LLVMFuncOp::create(rewriter, op.getLoc(), RuntimeName,
                                         functionType);
    }
    SmallVector<Value> arguments{adaptor.getNumber(), adaptor.getArg0(),
                                 adaptor.getArg1(),   adaptor.getArg2(),
                                 adaptor.getArg3(),   adaptor.getArg4(),
                                 adaptor.getArg5()};
    auto call = LLVM::CallOp::create(rewriter, op.getLoc(), runtime, arguments);
    rewriter.replaceOp(op, call.getResults());
    return success();
  }
};

struct ExternalCallOpLowering
    : public OpConversionPattern<ir1::ExternalCallOp> {
  using OpConversionPattern::OpConversionPattern;

  LogicalResult
  matchAndRewrite(ir1::ExternalCallOp op, OpAdaptor adaptor,
                  ConversionPatternRewriter &rewriter) const final {
    ModuleOp module = op->getParentOfType<ModuleOp>();
    StringRef name = op.getCallee();
    auto function = module.lookupSymbol<LLVM::LLVMFuncOp>(name);
    if (!function) {
      OpBuilder::InsertionGuard guard(rewriter);
      rewriter.setInsertionPointToStart(module.getBody());
      SmallVector<Type> arguments(6, rewriter.getI64Type());
      auto functionType = LLVM::LLVMFunctionType::get(
          rewriter.getI64Type(), arguments, /*isVarArg=*/false);
      function =
          LLVM::LLVMFuncOp::create(rewriter, op.getLoc(), name, functionType);
    }
    SmallVector<Value> arguments{adaptor.getArg0(), adaptor.getArg1(),
                                 adaptor.getArg2(), adaptor.getArg3(),
                                 adaptor.getArg4(), adaptor.getArg5()};
    auto call =
        LLVM::CallOp::create(rewriter, op.getLoc(), function, arguments);
    rewriter.replaceOp(op, call.getResults());
    return success();
  }
};

struct IndirectCallOpLowering
    : public OpConversionPattern<ir1::IndirectCallOp> {
  using OpConversionPattern::OpConversionPattern;

  LogicalResult
  matchAndRewrite(ir1::IndirectCallOp op, OpAdaptor adaptor,
                  ConversionPatternRewriter &rewriter) const final {
    auto pointerType = LLVM::LLVMPointerType::get(rewriter.getContext());
    Value callee = LLVM::IntToPtrOp::create(rewriter, op.getLoc(), pointerType,
                                            adaptor.getCallee());
    SmallVector<Value> operands{callee,
                                adaptor.getArg0(),
                                adaptor.getArg1(),
                                adaptor.getArg2(),
                                adaptor.getArg3(),
                                adaptor.getArg4(),
                                adaptor.getArg5()};
    SmallVector<Type> arguments(6, rewriter.getI64Type());
    auto functionType = LLVM::LLVMFunctionType::get(
        rewriter.getI64Type(), arguments, /*isVarArg=*/false);
    auto call =
        LLVM::CallOp::create(rewriter, op.getLoc(), functionType, operands);
    rewriter.replaceOp(op, call.getResults());
    return success();
  }
};

template <typename SourceOp, typename TargetOp>
struct BinaryIOpLowering : OpConversionPattern<SourceOp> {
  using OpConversionPattern<SourceOp>::OpConversionPattern;
  LogicalResult
  matchAndRewrite(SourceOp op, typename SourceOp::Adaptor adaptor,
                  ConversionPatternRewriter &rewriter) const final {
    auto type = cast<IntegerType>(op.getType());
    Value lhs = castInteger(rewriter, op.getLoc(), adaptor.getLhs(), type);
    Value rhs = castInteger(rewriter, op.getLoc(), adaptor.getRhs(), type);
    rewriter.replaceOpWithNewOp<TargetOp>(op, lhs, rhs);
    return success();
  }
};

struct CmpIOpLowering : OpConversionPattern<ir1::CmpIOp> {
  using OpConversionPattern::OpConversionPattern;
  LogicalResult
  matchAndRewrite(ir1::CmpIOp op, OpAdaptor adaptor,
                  ConversionPatternRewriter &rewriter) const final {
    rewriter.replaceOpWithNewOp<arith::CmpIOp>(
        op, static_cast<arith::CmpIPredicate>(op.getPredicate()),
        adaptor.getLhs(), adaptor.getRhs());
    return success();
  }
};

struct CastIOpLowering : OpConversionPattern<ir1::CastIOp> {
  using OpConversionPattern::OpConversionPattern;
  LogicalResult
  matchAndRewrite(ir1::CastIOp op, OpAdaptor adaptor,
                  ConversionPatternRewriter &rewriter) const final {
    rewriter.replaceOp(op,
                       castInteger(rewriter, op.getLoc(), adaptor.getValue(),
                                   cast<IntegerType>(op.getType())));
    return success();
  }
};

struct ExtSIOpLowering : OpConversionPattern<ir1::ExtSIOp> {
  using OpConversionPattern::OpConversionPattern;
  LogicalResult
  matchAndRewrite(ir1::ExtSIOp op, OpAdaptor adaptor,
                  ConversionPatternRewriter &rewriter) const final {
    rewriter.replaceOpWithNewOp<arith::ExtSIOp>(op, op.getType(),
                                                adaptor.getValue());
    return success();
  }
};

struct BitcastOpLowering : OpConversionPattern<ir1::BitcastOp> {
  using OpConversionPattern::OpConversionPattern;
  LogicalResult
  matchAndRewrite(ir1::BitcastOp op, OpAdaptor adaptor,
                  ConversionPatternRewriter &rewriter) const final {
    rewriter.replaceOpWithNewOp<arith::BitcastOp>(op, op.getType(),
                                                  adaptor.getValue());
    return success();
  }
};

template <typename SourceOp, typename TargetOp>
struct UnaryCastOpLowering : OpConversionPattern<SourceOp> {
  using OpConversionPattern<SourceOp>::OpConversionPattern;
  LogicalResult
  matchAndRewrite(SourceOp op, typename SourceOp::Adaptor adaptor,
                  ConversionPatternRewriter &rewriter) const final {
    rewriter.replaceOpWithNewOp<TargetOp>(op, op.getType(), adaptor.getValue());
    return success();
  }
};

using SIToFPOpLowering = UnaryCastOpLowering<ir1::SIToFPOp, arith::SIToFPOp>;
using FPToSIOpLowering = UnaryCastOpLowering<ir1::FPToSIOp, arith::FPToSIOp>;
using ExtFOpLowering = UnaryCastOpLowering<ir1::ExtFOp, arith::ExtFOp>;
using TruncFOpLowering = UnaryCastOpLowering<ir1::TruncFOp, arith::TruncFOp>;
using RoundEvenFOpLowering =
    UnaryCastOpLowering<ir1::RoundEvenFOp, math::RoundEvenOp>;

using SubIOpLowering = BinaryIOpLowering<ir1::SubIOp, arith::SubIOp>;
using AndIOpLowering = BinaryIOpLowering<ir1::AndIOp, arith::AndIOp>;
using OrIOpLowering = BinaryIOpLowering<ir1::OrIOp, arith::OrIOp>;
using XOrIOpLowering = BinaryIOpLowering<ir1::XOrIOp, arith::XOrIOp>;
using ShRUIOpLowering = BinaryIOpLowering<ir1::ShRUIOp, arith::ShRUIOp>;
using ShLIOpLowering = BinaryIOpLowering<ir1::ShLIOp, arith::ShLIOp>;

template <typename SourceOp, typename TargetOp>
struct BinaryFOpLowering : OpConversionPattern<SourceOp> {
  using OpConversionPattern<SourceOp>::OpConversionPattern;
  LogicalResult
  matchAndRewrite(SourceOp op, typename SourceOp::Adaptor adaptor,
                  ConversionPatternRewriter &rewriter) const final {
    rewriter.replaceOpWithNewOp<TargetOp>(op, adaptor.getLhs(),
                                          adaptor.getRhs());
    return success();
  }
};

using AddFOpLowering = BinaryFOpLowering<ir1::AddFOp, arith::AddFOp>;
using SubFOpLowering = BinaryFOpLowering<ir1::SubFOp, arith::SubFOp>;
using MulFOpLowering = BinaryFOpLowering<ir1::MulFOp, arith::MulFOp>;
using DivFOpLowering = BinaryFOpLowering<ir1::DivFOp, arith::DivFOp>;

struct IR1LoweringPass
    : public PassWrapper<IR1LoweringPass, OperationPass<ModuleOp>> {
  MLIR_DEFINE_EXPLICIT_INTERNAL_INLINE_TYPE_ID(IR1LoweringPass)

  void getDependentDialects(DialectRegistry &registry) const override {
    registry
        .insert<arith::ArithDialect, math::MathDialect, cf::ControlFlowDialect,
                func::FuncDialect, LLVM::LLVMDialect>();
  }

  void runOnOperation() final {
    LLVMConversionTarget target(getContext());
    target.addLegalOp<ModuleOp>();

    LLVMTypeConverter typeConverter(&getContext());
    RewritePatternSet patterns(&getContext());
    arith::populateArithToLLVMConversionPatterns(typeConverter, patterns);
    populateMathToLLVMConversionPatterns(typeConverter, patterns);
    cf::populateControlFlowToLLVMConversionPatterns(typeConverter, patterns);
    populateFuncToLLVMConversionPatterns(typeConverter, patterns);
    patterns
        .add<ConstIntOpLowering, LoadStateOpLowering, StoreStateOpLowering,
             AddIOpLowering, MulIOpLowering, LoadOpLowering, StoreOpLowering,
             SyscallOpLowering, ExternalCallOpLowering, IndirectCallOpLowering,
             SubIOpLowering, AndIOpLowering, OrIOpLowering, XOrIOpLowering,
             ShRUIOpLowering, ShLIOpLowering, CmpIOpLowering, CastIOpLowering,
             ExtSIOpLowering, BitcastOpLowering, AddFOpLowering, SubFOpLowering,
             MulFOpLowering, DivFOpLowering, SIToFPOpLowering, FPToSIOpLowering,
             ExtFOpLowering, TruncFOpLowering, RoundEvenFOpLowering>(
            typeConverter, &getContext());

    if (failed(
            applyFullConversion(getOperation(), target, std::move(patterns))))
      signalPassFailure();
  }
};

} // namespace

std::unique_ptr<Pass> z8::createIR1LowerPass() {
  return std::make_unique<IR1LoweringPass>();
}
