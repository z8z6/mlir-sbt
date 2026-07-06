
include_directories(${CMAKE_SOURCE_DIR}/third/llvm/mlir/include)

set(LLVM_TARGET_DEFINITIONS IR1.td)

mlir_tablegen(IR1Ops.h.inc -gen-op-decls)
mlir_tablegen(IR1Ops.cpp.inc -gen-op-defs)
mlir_tablegen(IR1Dialect.h.inc -gen-dialect-decls -dialect=ir1)
mlir_tablegen(IR1Dialect.cpp.inc -gen-dialect-defs -dialect=ir1)

add_public_tablegen_target(ir1-mlir-gen)