cmake_minimum_required(VERSION 3.31)
project(sbt)

set(CMAKE_CXX_STANDARD 17)

# 1. LLVM 相关配置
# 获取 LLVM 和 MLIR 的位置
find_package(LLVM REQUIRED CONFIG HINTS
        ${CMAKE_CURRENT_SOURCE_DIR}/third/llvm/build/lib/cmake/llvm
        NO_DEFAULT_PATH)
find_package(MLIR REQUIRED CONFIG HINTS
        ${CMAKE_CURRENT_SOURCE_DIR}/third/llvm/build/lib/cmake/mlir
        NO_DEFAULT_PATH)
# 获取 tblgen 程序的位置
find_program(LLVM_TABLEGEN_EXE "llvm-tblgen" ${LLVM_TOOLS_BINARY_DIR} NO_DEFAULT_PATH)
find_program(MLIR_TABLEGEN_EXE "mlir-tblgen" ${LLVM_TOOLS_BINARY_DIR} NO_DEFAULT_PATH)

message(STATUS "Using LLVMConfig.cmake in: ${LLVM_DIR}")
message(STATUS "Using MLIRConfig.cmake in: ${MLIR_DIR}")

# 添加文件目录
include_directories(${LLVM_INCLUDE_DIRS})
include_directories(${MLIR_INCLUDE_DIRS})
link_directories(${LLVM_LIBRARY_DIRS})
link_directories(${MLIR_LIBRARY_DIRS})

# 添加 cmake 文件
list(APPEND CMAKE_MODULE_PATH ${LLVM_CMAKE_DIR})
list(APPEND CMAKE_MODULE_PATH ${MLIR_CMAKE_DIR})

include(TableGen)
include(AddLLVM)
include(AddMLIR)
include(HandleLLVMOptions)

# 2. 本项目的代码文件
# tblgen 先生成定义
add_subdirectory(tblgen)
add_subdirectory(mlir)

file(GLOB_RECURSE SOURCE_FILES
        src/*.cpp
        include/*.h
        ${CMAKE_BINARY_DIR}/tblgen/*.cpp
)
add_executable(sbt ${SOURCE_FILES})

target_include_directories(sbt PUBLIC
        include
        ${CMAKE_BINARY_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR}/third/llvm/build/lib/Target/X86
        ${CMAKE_CURRENT_SOURCE_DIR}/third/llvm/llvm/lib
)

llvm_map_components_to_libnames(LLVM_LIBS
        Support
        Core
        Option
        CodeGen
        X86CodeGen
        X86Info
        X86Desc
        X86Disassembler
        RISCVInfo
        RISCVDesc
        RISCVCodeGen
        MC
        Analysis
        Object
        IRReader
        AsmParser
        BitReader
        TargetParser
)

set(MLIR_LIBS
        MLIRIR
        MLIRSupport
        MLIRParser
        MLIRDialect
        MLIRArithDialect
        MLIRTransformUtils
)


target_link_libraries(sbt ${LLVM_LIBS} ${MLIR_LIBS})

set_target_properties(sbt PROPERTIES
        CXX_STANDARD 17
        CXX_STANDARD_REQUIRED ON
        COMPILE_FLAGS "-fno-rtti" # 关闭RTTI
)
