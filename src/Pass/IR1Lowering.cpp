#include "Pass/IR1Lowering.h"

#include "IR/IR1.h"
#include "Target/X86Register.h"

#include "mlir/Conversion/ArithToLLVM/ArithToLLVM.h"
#include "mlir/Conversion/FuncToLLVM/ConvertFuncToLLVM.h"
#include "mlir/Conversion/LLVMCommon/ConversionTarget.h"
#include "mlir/Conversion/LLVMCommon/TypeConverter.h"
#include "mlir/Dialect/Arith/IR/Arith.h"
#include "mlir/Dialect/Func/IR/FuncOps.h"
#include "mlir/Dialect/LLVMIR/LLVMDialect.h"
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
                       X86RegisterSlot slot) {
  auto ptrType = LLVM::LLVMPointerType::get(rewriter.getContext());
  auto i64Type = rewriter.getI64Type();
  return LLVM::GEPOp::create(
      rewriter, loc, ptrType, i64Type, state,
      ArrayRef<LLVM::GEPArg>{static_cast<int32_t>(slot)});
}

Value packFlag(PatternRewriter &rewriter, Location loc, Value flag,
               unsigned bit) {
  auto i64Type = rewriter.getI64Type();
  Value wide = arith::ExtUIOp::create(rewriter, loc, i64Type, flag);
  if (bit == 0)
    return wide;
  return arith::ShLIOp::create(rewriter, loc, wide,
                               integerConstant(rewriter, loc, i64Type, bit));
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

struct LoadRegOpLowering : public OpConversionPattern<ir1::LoadRegOp> {
  using OpConversionPattern::OpConversionPattern;

  LogicalResult
  matchAndRewrite(ir1::LoadRegOp op, OpAdaptor adaptor,
                  ConversionPatternRewriter &rewriter) const final {
    auto reg = getX86RegisterDesc(cast<IntegerAttr>(op.getRegId()).getInt());
    if (!reg)
      return rewriter.notifyMatchFailure(op, "unsupported x86 register");

    Location loc = op.getLoc();
    auto i64Type = rewriter.getI64Type();
    Value ptr = stateSlotPointer(rewriter, loc, adaptor.getState(), reg->slot);
    Value value = LLVM::LoadOp::create(rewriter, loc, i64Type, ptr, 8);
    if (reg->bitOffset != 0)
      value = arith::ShRUIOp::create(
          rewriter, loc, value,
          integerConstant(rewriter, loc, i64Type, reg->bitOffset));
    auto resultType = cast<IntegerType>(op.getType());
    value = castInteger(rewriter, loc, value, resultType);
    rewriter.replaceOp(op, value);
    return success();
  }
};

struct StoreRegOpLowering : public OpConversionPattern<ir1::StoreRegOp> {
  using OpConversionPattern::OpConversionPattern;

  LogicalResult
  matchAndRewrite(ir1::StoreRegOp op, OpAdaptor adaptor,
                  ConversionPatternRewriter &rewriter) const final {
    auto reg = getX86RegisterDesc(cast<IntegerAttr>(op.getRegId()).getInt());
    if (!reg)
      return rewriter.notifyMatchFailure(op, "unsupported x86 register");

    Location loc = op.getLoc();
    auto i64Type = rewriter.getI64Type();
    Value ptr = stateSlotPointer(rewriter, loc, adaptor.getState(), reg->slot);
    Value value = castInteger(rewriter, loc, adaptor.getValue(), i64Type);

    if (reg->slot == X86RegisterSlot::RFLAGS) {
      Value old = LLVM::LoadOp::create(rewriter, loc, i64Type, ptr, 8);
      Value keepMask = integerConstant(rewriter, loc, i64Type,
                                       ~X86ArithmeticFlagsMask);
      Value flagMask = integerConstant(rewriter, loc, i64Type,
                                       X86ArithmeticFlagsMask);
      old = arith::AndIOp::create(rewriter, loc, old, keepMask);
      value = arith::AndIOp::create(rewriter, loc, value, flagMask);
      value = arith::OrIOp::create(rewriter, loc, old, value);
    } else if (reg->width < 32 || !reg->zeroExtendOnWrite) {
      if (reg->width != 64) {
        Value old = LLVM::LoadOp::create(rewriter, loc, i64Type, ptr, 8);
        const uint64_t fieldMask =
            ((uint64_t{1} << reg->width) - 1) << reg->bitOffset;
        Value keepMask = integerConstant(rewriter, loc, i64Type, ~fieldMask);
        old = arith::AndIOp::create(rewriter, loc, old, keepMask);
        if (reg->bitOffset != 0)
          value = arith::ShLIOp::create(
              rewriter, loc, value,
              integerConstant(rewriter, loc, i64Type, reg->bitOffset));
        value = arith::AndIOp::create(
            rewriter, loc, value,
            integerConstant(rewriter, loc, i64Type, fieldMask));
        value = arith::OrIOp::create(rewriter, loc, old, value);
      }
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
    Value ptr = LLVM::IntToPtrOp::create(rewriter, op.getLoc(), ptrType, address);
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
    Value ptr = LLVM::IntToPtrOp::create(rewriter, op.getLoc(), ptrType, address);
    LLVM::StoreOp::create(rewriter, op.getLoc(), adaptor.getValue(), ptr, 1);
    rewriter.eraseOp(op);
    return success();
  }
};

struct X86AddIOpLowering : public OpConversionPattern<ir1::X86AddIOp> {
  using OpConversionPattern::OpConversionPattern;

  LogicalResult
  matchAndRewrite(ir1::X86AddIOp op, OpAdaptor adaptor,
                  ConversionPatternRewriter &rewriter) const final {
    Location loc = op.getLoc();
    auto type = cast<IntegerType>(op.getRes().getType());
    Value lhs = castInteger(rewriter, loc, adaptor.getLhs(), type);
    Value rhs = castInteger(rewriter, loc, adaptor.getRhs(), type);
    Value result = arith::AddIOp::create(rewriter, loc, lhs, rhs);
    Value zero = integerConstant(rewriter, loc, type, 0);

    Value cf = arith::CmpIOp::create(rewriter, loc,
                                     arith::CmpIPredicate::ult, result, lhs);
    Value zf = arith::CmpIOp::create(rewriter, loc,
                                     arith::CmpIPredicate::eq, result, zero);

    Value signShift = integerConstant(rewriter, loc, type, type.getWidth() - 1);
    Value sfWide = arith::ShRUIOp::create(rewriter, loc, result, signShift);
    Value sf = arith::TruncIOp::create(rewriter, loc, rewriter.getI1Type(),
                                       sfWide);

    Value xorLhsRhs = arith::XOrIOp::create(rewriter, loc, lhs, rhs);
    Value notXor = arith::XOrIOp::create(
        rewriter, loc, xorLhsRhs,
        integerConstant(rewriter, loc, type, ~uint64_t{0}));
    Value xorLhsResult = arith::XOrIOp::create(rewriter, loc, lhs, result);
    Value overflowBits =
        arith::AndIOp::create(rewriter, loc, notXor, xorLhsResult);
    overflowBits =
        arith::ShRUIOp::create(rewriter, loc, overflowBits, signShift);
    Value of = arith::TruncIOp::create(rewriter, loc, rewriter.getI1Type(),
                                       overflowBits);

    Value afBits = arith::XOrIOp::create(rewriter, loc, lhs, rhs);
    afBits = arith::XOrIOp::create(rewriter, loc, afBits, result);
    afBits = arith::AndIOp::create(
        rewriter, loc, afBits, integerConstant(rewriter, loc, type, 0x10));
    Value af = arith::CmpIOp::create(rewriter, loc,
                                     arith::CmpIPredicate::ne, afBits, zero);

    Value parity = result;
    for (unsigned shift : {4u, 2u, 1u}) {
      Value shifted = arith::ShRUIOp::create(
          rewriter, loc, parity,
          integerConstant(rewriter, loc, type, shift));
      parity = arith::XOrIOp::create(rewriter, loc, parity, shifted);
    }
    parity = arith::AndIOp::create(
        rewriter, loc, parity, integerConstant(rewriter, loc, type, 1));
    Value pf = arith::CmpIOp::create(rewriter, loc,
                                     arith::CmpIPredicate::eq, parity, zero);

    Value flags = packFlag(rewriter, loc, cf, 0);
    for (auto [flag, bit] :
         {std::pair<Value, unsigned>{pf, 2}, {af, 4}, {zf, 6}, {sf, 7},
          {of, 11}})
      flags = arith::OrIOp::create(rewriter, loc, flags,
                                   packFlag(rewriter, loc, flag, bit));

    rewriter.replaceOp(op, ValueRange{result, flags});
    return success();
  }
};

struct IR1LoweringPass
    : public PassWrapper<IR1LoweringPass, OperationPass<ModuleOp>> {
  MLIR_DEFINE_EXPLICIT_INTERNAL_INLINE_TYPE_ID(IR1LoweringPass)

  void getDependentDialects(DialectRegistry &registry) const override {
    registry.insert<arith::ArithDialect, func::FuncDialect,
                    LLVM::LLVMDialect>();
  }

  void runOnOperation() final {
    LLVMConversionTarget target(getContext());
    target.addLegalOp<ModuleOp>();

    LLVMTypeConverter typeConverter(&getContext());
    RewritePatternSet patterns(&getContext());
    arith::populateArithToLLVMConversionPatterns(typeConverter, patterns);
    populateFuncToLLVMConversionPatterns(typeConverter, patterns);
    patterns.add<ConstIntOpLowering, LoadRegOpLowering, StoreRegOpLowering,
                 AddIOpLowering, MulIOpLowering, LoadOpLowering,
                 StoreOpLowering,
                 X86AddIOpLowering>(typeConverter, &getContext());

    if (failed(applyFullConversion(getOperation(), target,
                                   std::move(patterns))))
      signalPassFailure();
  }
};

} // namespace

std::unique_ptr<Pass> z8::createIR1LowerPass() {
  return std::make_unique<IR1LoweringPass>();
}
