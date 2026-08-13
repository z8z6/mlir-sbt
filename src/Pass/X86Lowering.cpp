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

enum class ArithmeticKind { Add, Sub };

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
    patterns
        .add<ReadRegLowering, WriteRegLowering,
             ArithmeticLowering<x86ir::AddIOp, ArithmeticKind::Add>,
             ArithmeticLowering<x86ir::SubIOp, ArithmeticKind::Sub>,
             LogicalLowering<x86ir::AndIOp, LogicalKind::And>,
             LogicalLowering<x86ir::OrIOp, LogicalKind::Or>,
             LogicalLowering<x86ir::XorIOp, LogicalKind::Xor>, ScalarFLowering>(
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
