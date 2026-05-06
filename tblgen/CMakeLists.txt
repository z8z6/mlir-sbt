# Intrinsics.td
include_directories(${CMAKE_SOURCE_DIR}/third/llvm/llvm/include/llvm/IR/)

set(LLVM_TARGET_DEFINITIONS DirectTrans.td)

tablegen(LLVM IntrinsicImpl.inc -gen-intrinsic-impl)
tablegen(LLVM IntrinsicEnums.inc -gen-intrinsic-enums)
tablegen(LLVM IntrinsicsRISCV.h -gen-intrinsic-enums -intrinsic-prefix=riscv)

add_public_tablegen_target(direct_trans_gen)