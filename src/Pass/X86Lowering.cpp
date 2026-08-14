#include "Pass/X86Lowering.h"

#include "IR/IR1.h"
#include "IR/X86.h"
#include "Target/X86Register.h"

#include "mlir/Dialect/Arith/IR/Arith.h"
#include "mlir/IR/BuiltinDialect.h"
#include "mlir/Pass/Pass.h"
#include "mlir/Transforms/DialectConversion.h"

#include <algorithm>

using namespace mlir;
using namespace z8;

namespace {

Value constant(PatternRewriter &rewriter, Location loc, IntegerType type,
               uint64_t value) {
  if (type.getWidth() < 64)
    value &= (uint64_t{1} << type.getWidth()) - 1;
  auto attr = IntegerAttr::get(type, APInt(type.getWidth(), value));
  return ir1::ConstIntOp::create(rewriter, loc, type, attr);
}

Value castTo(PatternRewriter &rewriter, Location loc, Value value, Type type) {
  if (value.getType() == type)
    return value;
  return ir1::CastIOp::create(rewriter, loc, type, value);
}

Value binary(PatternRewriter &rewriter, Location loc, StringRef kind,
             IntegerType type, Value lhs, Value rhs) {
  lhs = castTo(rewriter, loc, lhs, type);
  rhs = castTo(rewriter, loc, rhs, type);
  if (kind == "add")
    return ir1::AddIOp::create(rewriter, loc, type, lhs, rhs);
  if (kind == "sub")
    return ir1::SubIOp::create(rewriter, loc, type, lhs, rhs);
  if (kind == "and")
    return ir1::AndIOp::create(rewriter, loc, type, lhs, rhs);
  if (kind == "or")
    return ir1::OrIOp::create(rewriter, loc, type, lhs, rhs);
  if (kind == "xor")
    return ir1::XOrIOp::create(rewriter, loc, type, lhs, rhs);
  if (kind == "shru")
    return ir1::ShRUIOp::create(rewriter, loc, type, lhs, rhs);
  return ir1::ShLIOp::create(rewriter, loc, type, lhs, rhs);
}

Value cmp(PatternRewriter &rewriter, Location loc, arith::CmpIPredicate pred,
          Value lhs, Value rhs) {
  auto attr = rewriter.getI32IntegerAttr(static_cast<int32_t>(pred));
  return ir1::CmpIOp::create(rewriter, loc, rewriter.getI1Type(), lhs, rhs,
                             attr);
}

Value packFlag(PatternRewriter &rewriter, Location loc, Value flag,
               unsigned bit) {
  auto i64 = rewriter.getI64Type();
  Value result = castTo(rewriter, loc, flag, i64);
  if (bit)
    result = binary(rewriter, loc, "shl", i64, result,
                    constant(rewriter, loc, i64, bit));
  return result;
}

Value unpackFlag(PatternRewriter &rewriter, Location loc, Value flags,
                 unsigned bit) {
  auto i64 = rewriter.getI64Type();
  Value shifted = flags;
  if (bit)
    shifted = binary(rewriter, loc, "shru", i64, flags,
                     constant(rewriter, loc, i64, bit));
  shifted = binary(rewriter, loc, "and", i64, shifted,
                   constant(rewriter, loc, i64, 1));
  return castTo(rewriter, loc, shifted, rewriter.getI1Type());
}

Value invert(PatternRewriter &rewriter, Location loc, Value value) {
  return binary(rewriter, loc, "xor", rewriter.getI1Type(), value,
                constant(rewriter, loc, rewriter.getI1Type(), 1));
}

enum class ArithmeticKind { Add, Sub };

struct ConditionLowering : OpConversionPattern<x86ir::ConditionOp> {
  using OpConversionPattern::OpConversionPattern;
  LogicalResult
  matchAndRewrite(x86ir::ConditionOp op, OpAdaptor adaptor,
                  ConversionPatternRewriter &rewriter) const final {
    if (op.getCondition() > 15)
      return failure();
    Location loc = op.getLoc();
    Value cf = unpackFlag(rewriter, loc, adaptor.getFlags(), 0);
    Value pf = unpackFlag(rewriter, loc, adaptor.getFlags(), 2);
    Value zf = unpackFlag(rewriter, loc, adaptor.getFlags(), 6);
    Value sf = unpackFlag(rewriter, loc, adaptor.getFlags(), 7);
    Value of = unpackFlag(rewriter, loc, adaptor.getFlags(), 11);
    Value sfXorOf = binary(rewriter, loc, "xor", rewriter.getI1Type(), sf, of);
    Value result;
    switch (op.getCondition()) {
    case 0:
      result = of;
      break;
    case 1:
      result = invert(rewriter, loc, of);
      break;
    case 2:
      result = cf;
      break;
    case 3:
      result = invert(rewriter, loc, cf);
      break;
    case 4:
      result = zf;
      break;
    case 5:
      result = invert(rewriter, loc, zf);
      break;
    case 6:
      result = binary(rewriter, loc, "or", rewriter.getI1Type(), cf, zf);
      break;
    case 7:
      result =
          invert(rewriter, loc,
                 binary(rewriter, loc, "or", rewriter.getI1Type(), cf, zf));
      break;
    case 8:
      result = sf;
      break;
    case 9:
      result = invert(rewriter, loc, sf);
      break;
    case 10:
      result = pf;
      break;
    case 11:
      result = invert(rewriter, loc, pf);
      break;
    case 12:
      result = sfXorOf;
      break;
    case 13:
      result = invert(rewriter, loc, sfXorOf);
      break;
    case 14:
      result = binary(rewriter, loc, "or", rewriter.getI1Type(), zf, sfXorOf);
      break;
    case 15:
      result = invert(
          rewriter, loc,
          binary(rewriter, loc, "or", rewriter.getI1Type(), zf, sfXorOf));
      break;
    }
    rewriter.replaceOp(op, result);
    return success();
  }
};

struct ReadRegLowering : OpConversionPattern<x86ir::ReadRegOp> {
  using OpConversionPattern::OpConversionPattern;
  LogicalResult
  matchAndRewrite(x86ir::ReadRegOp op, OpAdaptor adaptor,
                  ConversionPatternRewriter &rewriter) const final {
    auto reg = getX86RegisterDesc(op.getRegId());
    if (!reg)
      return failure();
    unsigned storageWidth = std::max(64u, reg->width);
    rewriter.replaceOpWithNewOp<ir1::LoadStateOp>(
        op, op.getType(), adaptor.getState(),
        rewriter.getI32IntegerAttr(static_cast<int32_t>(reg->slot)),
        rewriter.getI32IntegerAttr(reg->bitOffset),
        rewriter.getI32IntegerAttr(storageWidth));
    return success();
  }
};

struct WriteRegLowering : OpConversionPattern<x86ir::WriteRegOp> {
  using OpConversionPattern::OpConversionPattern;
  LogicalResult
  matchAndRewrite(x86ir::WriteRegOp op, OpAdaptor adaptor,
                  ConversionPatternRewriter &rewriter) const final {
    auto reg = getX86RegisterDesc(op.getRegId());
    if (!reg)
      return failure();
    unsigned storageWidth = std::max(64u, reg->width);
    APInt mask = APInt::getAllOnes(storageWidth);
    if (reg->slot == X86RegisterSlot::RFLAGS) {
      mask = APInt(storageWidth, X86ArithmeticFlagsMask);
    } else if (reg->width < 64 && !reg->zeroExtendOnWrite) {
      mask = APInt::getLowBitsSet(storageWidth, reg->width).shl(reg->bitOffset);
    }
    auto storageType = rewriter.getIntegerType(storageWidth);
    ir1::StoreStateOp::create(
        rewriter, op.getLoc(), adaptor.getState(), adaptor.getValue(),
        rewriter.getI32IntegerAttr(static_cast<int32_t>(reg->slot)),
        rewriter.getI32IntegerAttr(reg->bitOffset),
        rewriter.getI32IntegerAttr(storageWidth),
        IntegerAttr::get(storageType, mask));
    rewriter.eraseOp(op);
    return success();
  }
};

struct SyscallLowering : OpConversionPattern<x86ir::SyscallOp> {
  using OpConversionPattern::OpConversionPattern;
  LogicalResult
  matchAndRewrite(x86ir::SyscallOp op, OpAdaptor adaptor,
                  ConversionPatternRewriter &rewriter) const final {
    rewriter.replaceOpWithNewOp<ir1::SyscallOp>(
        op, rewriter.getI64Type(), adaptor.getNumber(), adaptor.getArg0(),
        adaptor.getArg1(), adaptor.getArg2(), adaptor.getArg3(),
        adaptor.getArg4(), adaptor.getArg5());
    return success();
  }
};

struct ExternalCallLowering : OpConversionPattern<x86ir::ExternalCallOp> {
  using OpConversionPattern::OpConversionPattern;
  LogicalResult
  matchAndRewrite(x86ir::ExternalCallOp op, OpAdaptor adaptor,
                  ConversionPatternRewriter &rewriter) const final {
    rewriter.replaceOpWithNewOp<ir1::ExternalCallOp>(
        op, rewriter.getI64Type(), adaptor.getArg0(), adaptor.getArg1(),
        adaptor.getArg2(), adaptor.getArg3(), adaptor.getArg4(),
        adaptor.getArg5(), op.getCalleeAttr());
    return success();
  }
};

struct IndirectCallLowering : OpConversionPattern<x86ir::IndirectCallOp> {
  using OpConversionPattern::OpConversionPattern;
  LogicalResult
  matchAndRewrite(x86ir::IndirectCallOp op, OpAdaptor adaptor,
                  ConversionPatternRewriter &rewriter) const final {
    rewriter.replaceOpWithNewOp<ir1::IndirectCallOp>(
        op, rewriter.getI64Type(), adaptor.getCallee(), adaptor.getArg0(),
        adaptor.getArg1(), adaptor.getArg2(), adaptor.getArg3(),
        adaptor.getArg4(), adaptor.getArg5());
    return success();
  }
};

std::pair<Value, Value> expandArithmetic(PatternRewriter &rewriter,
                                         Location loc, IntegerType type,
                                         Value lhs, Value rhs,
                                         ArithmeticKind kind) {
  lhs = castTo(rewriter, loc, lhs, type);
  rhs = castTo(rewriter, loc, rhs, type);
  Value result =
      binary(rewriter, loc, kind == ArithmeticKind::Add ? "add" : "sub", type,
             lhs, rhs);
  Value zero = constant(rewriter, loc, type, 0);
  Value cf = kind == ArithmeticKind::Add
                 ? cmp(rewriter, loc, arith::CmpIPredicate::ult, result, lhs)
                 : cmp(rewriter, loc, arith::CmpIPredicate::ult, lhs, rhs);
  Value zf = cmp(rewriter, loc, arith::CmpIPredicate::eq, result, zero);
  Value shift = constant(rewriter, loc, type, type.getWidth() - 1);
  Value sf =
      castTo(rewriter, loc, binary(rewriter, loc, "shru", type, result, shift),
             rewriter.getI1Type());
  Value lhsXorRhs = binary(rewriter, loc, "xor", type, lhs, rhs);
  Value overflowInput = lhsXorRhs;
  if (kind == ArithmeticKind::Add)
    overflowInput = binary(rewriter, loc, "xor", type, lhsXorRhs,
                           constant(rewriter, loc, type, ~uint64_t{0}));
  Value overflow = binary(rewriter, loc, "and", type, overflowInput,
                          binary(rewriter, loc, "xor", type, lhs, result));
  Value of = castTo(rewriter, loc,
                    binary(rewriter, loc, "shru", type, overflow, shift),
                    rewriter.getI1Type());
  Value afBits = binary(rewriter, loc, "and", type,
                        binary(rewriter, loc, "xor", type, lhsXorRhs, result),
                        constant(rewriter, loc, type, 0x10));
  Value af = cmp(rewriter, loc, arith::CmpIPredicate::ne, afBits, zero);
  Value parity = result;
  for (unsigned amount : {4u, 2u, 1u})
    parity = binary(rewriter, loc, "xor", type, parity,
                    binary(rewriter, loc, "shru", type, parity,
                           constant(rewriter, loc, type, amount)));
  parity = binary(rewriter, loc, "and", type, parity,
                  constant(rewriter, loc, type, 1));
  Value pf = cmp(rewriter, loc, arith::CmpIPredicate::eq, parity, zero);
  Value flags = packFlag(rewriter, loc, cf, 0);
  for (auto [flag, bit] :
       {std::pair<Value, unsigned>{pf, 2}, {af, 4}, {zf, 6}, {sf, 7}, {of, 11}})
    flags = binary(rewriter, loc, "or", rewriter.getI64Type(), flags,
                   packFlag(rewriter, loc, flag, bit));
  return {result, flags};
}

template <typename Op, ArithmeticKind Kind>
struct ArithmeticLowering : OpConversionPattern<Op> {
  using OpConversionPattern<Op>::OpConversionPattern;
  LogicalResult
  matchAndRewrite(Op op, typename Op::Adaptor adaptor,
                  ConversionPatternRewriter &rewriter) const final {
    auto values = expandArithmetic(rewriter, op.getLoc(),
                                   cast<IntegerType>(op.getRes().getType()),
                                   adaptor.getLhs(), adaptor.getRhs(), Kind);
    rewriter.replaceOp(op, {values.first, values.second});
    return success();
  }
};

enum class LogicalKind { And, Or, Xor };

template <typename Op, LogicalKind Kind>
struct LogicalLowering : OpConversionPattern<Op> {
  using OpConversionPattern<Op>::OpConversionPattern;
  LogicalResult
  matchAndRewrite(Op op, typename Op::Adaptor adaptor,
                  ConversionPatternRewriter &rewriter) const final {
    Location loc = op.getLoc();
    auto type = cast<IntegerType>(op.getRes().getType());
    StringRef name = Kind == LogicalKind::And  ? "and"
                     : Kind == LogicalKind::Or ? "or"
                                               : "xor";
    Value result =
        binary(rewriter, loc, name, type, adaptor.getLhs(), adaptor.getRhs());
    Value zero = constant(rewriter, loc, type, 0);
    Value zf = cmp(rewriter, loc, arith::CmpIPredicate::eq, result, zero);
    Value sf =
        castTo(rewriter, loc,
               binary(rewriter, loc, "shru", type, result,
                      constant(rewriter, loc, type, type.getWidth() - 1)),
               rewriter.getI1Type());
    Value parity = result;
    for (unsigned amount : {4u, 2u, 1u})
      parity = binary(rewriter, loc, "xor", type, parity,
                      binary(rewriter, loc, "shru", type, parity,
                             constant(rewriter, loc, type, amount)));
    parity = binary(rewriter, loc, "and", type, parity,
                    constant(rewriter, loc, type, 1));
    Value pf = cmp(rewriter, loc, arith::CmpIPredicate::eq, parity, zero);
    Value flags = packFlag(rewriter, loc, pf, 2);
    flags = binary(rewriter, loc, "or", rewriter.getI64Type(), flags,
                   packFlag(rewriter, loc, zf, 6));
    flags = binary(rewriter, loc, "or", rewriter.getI64Type(), flags,
                   packFlag(rewriter, loc, sf, 7));
    rewriter.replaceOp(op, {result, flags});
    return success();
  }
};

struct ScalarFLowering : OpConversionPattern<x86ir::ScalarFOp> {
  using OpConversionPattern::OpConversionPattern;
  LogicalResult
  matchAndRewrite(x86ir::ScalarFOp op, OpAdaptor adaptor,
                  ConversionPatternRewriter &rewriter) const final {
    unsigned width = op.getWidth(), kind = op.getKind();
    if ((width != 32 && width != 64) || kind > 3)
      return failure();
    Location loc = op.getLoc();
    auto i128 = rewriter.getIntegerType(128);
    auto scalarInt = rewriter.getIntegerType(width);
    Type scalarFloat =
        width == 32 ? Type(rewriter.getF32Type()) : Type(rewriter.getF64Type());
    Value lhs = castTo(rewriter, loc, adaptor.getLhs(), i128);
    Value lhsBits = castTo(rewriter, loc, lhs, scalarInt);
    Value rhsBits = castTo(rewriter, loc, adaptor.getRhs(), scalarInt);
    Value lhsFloat =
        ir1::BitcastOp::create(rewriter, loc, scalarFloat, lhsBits);
    Value rhsFloat =
        ir1::BitcastOp::create(rewriter, loc, scalarFloat, rhsBits);
    Value fp;
    if (kind == 0)
      fp = ir1::AddFOp::create(rewriter, loc, scalarFloat, lhsFloat, rhsFloat);
    else if (kind == 1)
      fp = ir1::SubFOp::create(rewriter, loc, scalarFloat, lhsFloat, rhsFloat);
    else if (kind == 2)
      fp = ir1::MulFOp::create(rewriter, loc, scalarFloat, lhsFloat, rhsFloat);
    else
      fp = ir1::DivFOp::create(rewriter, loc, scalarFloat, lhsFloat, rhsFloat);
    Value bits = ir1::BitcastOp::create(rewriter, loc, scalarInt, fp);
    bits = castTo(rewriter, loc, bits, i128);
    Value shift = constant(rewriter, loc, i128, width);
    Value upper =
        binary(rewriter, loc, "shl", i128,
               binary(rewriter, loc, "shru", i128, lhs, shift), shift);
    rewriter.replaceOp(op, binary(rewriter, loc, "or", i128, upper, bits));
    return success();
  }
};

Type scalarOrVector(Type elementType, unsigned lanes) {
  return lanes == 1 ? elementType
                    : Type(VectorType::get({static_cast<int64_t>(lanes)},
                                           elementType));
}

struct ConvertLowering : OpConversionPattern<x86ir::ConvertOp> {
  using OpConversionPattern::OpConversionPattern;
  LogicalResult
  matchAndRewrite(x86ir::ConvertOp op, OpAdaptor adaptor,
                  ConversionPatternRewriter &rewriter) const final {
    unsigned kind = op.getKind(), sourceWidth = op.getSrcWidth();
    unsigned destinationWidth = op.getDstWidth(), lanes = op.getLanes();
    unsigned rounding = op.getRound();
    if (kind > 3 || rounding > 2 || lanes == 0 ||
        (sourceWidth != 32 && sourceWidth != 64) ||
        (destinationWidth != 32 && destinationWidth != 64))
      return failure();

    Location loc = op.getLoc();
    unsigned sourceBits = sourceWidth * lanes;
    unsigned destinationBits = destinationWidth * lanes;
    auto sourceInteger = rewriter.getIntegerType(sourceBits);
    auto destinationInteger = rewriter.getIntegerType(destinationBits);
    Type sourceElement = kind == 0 ? Type(rewriter.getIntegerType(sourceWidth))
                         : sourceWidth == 32 ? Type(rewriter.getF32Type())
                                             : Type(rewriter.getF64Type());
    Type destinationElement =
        kind == 1 ? Type(rewriter.getIntegerType(destinationWidth))
        : destinationWidth == 32 ? Type(rewriter.getF32Type())
                                 : Type(rewriter.getF64Type());
    Type sourceType = scalarOrVector(sourceElement, lanes);
    Type destinationType = scalarOrVector(destinationElement, lanes);
    Value sourceBitsValue =
        castTo(rewriter, loc, adaptor.getSource(), sourceInteger);
    Value source =
        ir1::BitcastOp::create(rewriter, loc, sourceType, sourceBitsValue);
    Value converted;
    if (kind == 0) {
      converted = ir1::SIToFPOp::create(rewriter, loc, destinationType, source);
    } else if (kind == 1) {
      if (rounding == 1)
        source = ir1::RoundEvenFOp::create(rewriter, loc, sourceType, source);
      converted = ir1::FPToSIOp::create(rewriter, loc, destinationType, source);
    } else if (kind == 2) {
      converted = ir1::ExtFOp::create(rewriter, loc, destinationType, source);
    } else {
      converted = ir1::TruncFOp::create(rewriter, loc, destinationType, source);
    }

    Value result =
        ir1::BitcastOp::create(rewriter, loc, destinationInteger, converted);
    auto resultType = cast<IntegerType>(op.getType());
    result = castTo(rewriter, loc, result, resultType);
    if (op.getPreserveUpper()) {
      if (resultType.getWidth() != 128 || destinationBits >= 128)
        return failure();
      auto i128 = rewriter.getIntegerType(128);
      Value oldDestination = castTo(rewriter, loc, adaptor.getOldDest(), i128);
      Value shift = constant(rewriter, loc, i128, destinationBits);
      Value upper = binary(
          rewriter, loc, "shl", i128,
          binary(rewriter, loc, "shru", i128, oldDestination, shift), shift);
      result = binary(rewriter, loc, "or", i128, upper, result);
    }
    rewriter.replaceOp(op, result);
    return success();
  }
};

constexpr unsigned X87BaseSlot = static_cast<unsigned>(X86RegisterSlot::X87ST0);

struct ReadX87Lowering : OpConversionPattern<x86ir::ReadX87Op> {
  using OpConversionPattern::OpConversionPattern;
  LogicalResult
  matchAndRewrite(x86ir::ReadX87Op op, OpAdaptor adaptor,
                  ConversionPatternRewriter &rewriter) const final {
    if (op.getIndex() > 7)
      return failure();
    rewriter.replaceOpWithNewOp<ir1::LoadStateOp>(
        op, rewriter.getIntegerType(80), adaptor.getState(),
        rewriter.getI32IntegerAttr(X87BaseSlot + op.getIndex() * 2),
        rewriter.getI32IntegerAttr(0), rewriter.getI32IntegerAttr(128));
    return success();
  }
};

void storeX87(PatternRewriter &rewriter, Location loc, Value state, Value value,
              unsigned index) {
  auto i128 = rewriter.getIntegerType(128);
  ir1::StoreStateOp::create(rewriter, loc, state, value,
                            rewriter.getI32IntegerAttr(X87BaseSlot + index * 2),
                            rewriter.getI32IntegerAttr(0),
                            rewriter.getI32IntegerAttr(128),
                            IntegerAttr::get(i128, APInt::getAllOnes(128)));
}

struct WriteX87Lowering : OpConversionPattern<x86ir::WriteX87Op> {
  using OpConversionPattern::OpConversionPattern;
  LogicalResult
  matchAndRewrite(x86ir::WriteX87Op op, OpAdaptor adaptor,
                  ConversionPatternRewriter &rewriter) const final {
    if (op.getIndex() > 7)
      return failure();
    storeX87(rewriter, op.getLoc(), adaptor.getState(), adaptor.getValue(),
             op.getIndex());
    rewriter.eraseOp(op);
    return success();
  }
};

struct PopX87Lowering : OpConversionPattern<x86ir::PopX87Op> {
  using OpConversionPattern::OpConversionPattern;
  LogicalResult
  matchAndRewrite(x86ir::PopX87Op op, OpAdaptor adaptor,
                  ConversionPatternRewriter &rewriter) const final {
    SmallVector<Value, 7> shifted;
    auto i80 = rewriter.getIntegerType(80);
    for (unsigned index = 1; index < 8; ++index)
      shifted.push_back(ir1::LoadStateOp::create(
          rewriter, op.getLoc(), i80, adaptor.getState(),
          rewriter.getI32IntegerAttr(X87BaseSlot + index * 2),
          rewriter.getI32IntegerAttr(0), rewriter.getI32IntegerAttr(128)));
    for (unsigned index = 0; index < shifted.size(); ++index)
      storeX87(rewriter, op.getLoc(), adaptor.getState(), shifted[index],
               index);
    storeX87(rewriter, op.getLoc(), adaptor.getState(),
             constant(rewriter, op.getLoc(), i80, 0), 7);
    rewriter.eraseOp(op);
    return success();
  }
};

struct X87BinaryLowering : OpConversionPattern<x86ir::X87BinaryOp> {
  using OpConversionPattern::OpConversionPattern;
  LogicalResult
  matchAndRewrite(x86ir::X87BinaryOp op, OpAdaptor adaptor,
                  ConversionPatternRewriter &rewriter) const final {
    if (op.getKind() > 3 || op.getType() != rewriter.getIntegerType(80))
      return failure();
    Location loc = op.getLoc();
    Type f80 = rewriter.getF80Type();
    Value lhs = ir1::BitcastOp::create(rewriter, loc, f80, adaptor.getLhs());
    Value rhs = ir1::BitcastOp::create(rewriter, loc, f80, adaptor.getRhs());
    Value result;
    if (op.getKind() == 0)
      result = ir1::AddFOp::create(rewriter, loc, f80, lhs, rhs);
    else if (op.getKind() == 1)
      result = ir1::SubFOp::create(rewriter, loc, f80, lhs, rhs);
    else if (op.getKind() == 2)
      result = ir1::MulFOp::create(rewriter, loc, f80, lhs, rhs);
    else
      result = ir1::DivFOp::create(rewriter, loc, f80, lhs, rhs);
    rewriter.replaceOpWithNewOp<ir1::BitcastOp>(op, rewriter.getIntegerType(80),
                                                result);
    return success();
  }
};

struct X86LoweringPass : PassWrapper<X86LoweringPass, OperationPass<ModuleOp>> {
  MLIR_DEFINE_EXPLICIT_INTERNAL_INLINE_TYPE_ID(X86LoweringPass)
  void getDependentDialects(DialectRegistry &registry) const override {
    registry.insert<ir1::IR1Dialect>();
  }
  void runOnOperation() final {
    ConversionTarget target(getContext());
    target.addIllegalDialect<x86ir::X86Dialect>();
    target.addLegalDialect<ir1::IR1Dialect>();
    target.markUnknownOpDynamicallyLegal([](Operation *) { return true; });
    RewritePatternSet patterns(&getContext());
    patterns.add<ReadRegLowering, WriteRegLowering, ConditionLowering,
                 SyscallLowering, ExternalCallLowering, IndirectCallLowering,
                 ArithmeticLowering<x86ir::AddIOp, ArithmeticKind::Add>,
                 ArithmeticLowering<x86ir::SubIOp, ArithmeticKind::Sub>,
                 LogicalLowering<x86ir::AndIOp, LogicalKind::And>,
                 LogicalLowering<x86ir::OrIOp, LogicalKind::Or>,
                 LogicalLowering<x86ir::XorIOp, LogicalKind::Xor>,
                 ScalarFLowering, ConvertLowering, ReadX87Lowering,
                 WriteX87Lowering, PopX87Lowering, X87BinaryLowering>(
        &getContext());
    if (failed(applyPartialConversion(getOperation(), target,
                                      std::move(patterns))))
      signalPassFailure();
  }
};

} // namespace

std::unique_ptr<Pass> z8::createX86LowerPass() {
  return std::make_unique<X86LoweringPass>();
}
