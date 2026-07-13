//
// Created by zzm on 2026/7/13
// Part of RVision
//

#include "Pass/IR1Lowering.h"
#include "IR/IR1.h"
#include "mlir/Dialect/Affine/IR/AffineOps.h"
#include "mlir/Dialect/Arith/IR/Arith.h"
#include "mlir/Dialect/Func/IR/FuncOps.h"
#include "mlir/Dialect/MemRef/IR/MemRef.h"
#include "mlir/Pass/Pass.h"
#include "mlir/IR/BuiltinDialect.h"
#include "mlir/Transforms/DialectConversion.h"

using namespace mlir;
using namespace z8;

struct ConstIntOpLowering : public OpRewritePattern<ir1::ConstIntOp> {
  using OpRewritePattern::OpRewritePattern;

  LogicalResult matchAndRewrite(ir1::ConstIntOp op,
                                PatternRewriter &rewriter) const final {
    auto value = llvm::cast<IntegerAttr>(op.getValue());
    auto type = llvm::cast<IntegerType>(op.getType());
    Location loc = op.getLoc();

    auto newOp = arith::ConstantIntOp::create(rewriter, loc, type, value.getInt());
    rewriter.replaceOp(op, newOp.getResult());
    return success();
  }
};

namespace {
struct IR1LoweringPass
    : public PassWrapper<IR1LoweringPass, OperationPass<ModuleOp>> {
  MLIR_DEFINE_EXPLICIT_INTERNAL_INLINE_TYPE_ID(IR1LoweringPass)

  void getDependentDialects(DialectRegistry &registry) const override {
    registry.insert<arith::ArithDialect, memref::MemRefDialect>();
  }
  void runOnOperation() final;
};
} // namespace

void IR1LoweringPass::runOnOperation() {
  ConversionTarget target(getContext());

  target.addLegalDialect<BuiltinDialect,
                         arith::ArithDialect,
                         memref::MemRefDialect>();

  //target.addIllegalDialect<ir1::IR1Dialect>();

  RewritePatternSet patterns(&getContext());
  patterns.add<ConstIntOpLowering>(&getContext());

  if (failed(applyPartialConversion(getOperation(), target, std::move(patterns))))
    signalPassFailure();
}


std::unique_ptr<Pass> z8::createIR1LowerPass() {
  return std::make_unique<IR1LoweringPass>();
}
