cmake_minimum_required(VERSION 3.31)
project(sbt)

include(CTest)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

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

file(GLOB_RECURSE SOURCE_FILES CONFIGURE_DEPENDS
        src/*.cpp
        include/*.h
)
list(APPEND SOURCE_FILES
        ${CMAKE_BINARY_DIR}/tblgen/X86IR1Converter.cpp
        ${CMAKE_BINARY_DIR}/tblgen/X86IR1Converter.h
)
list(REMOVE_ITEM SOURCE_FILES ${CMAKE_CURRENT_SOURCE_DIR}/src/main.cpp)
add_library(sbt-lib STATIC ${SOURCE_FILES})
add_dependencies(sbt-lib ir1-mlir-gen x86-mlir-gen ir1-x86-converter-gen)
add_executable(sbt src/main.cpp)

target_include_directories(sbt-lib PUBLIC
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
        ExecutionEngine
        OrcJIT
        native
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
        MLIRArithToLLVM
        MLIRFuncDialect
        MLIRFuncToLLVM
        MLIRLLVMDialect
        MLIRLLVMCommonConversion
        MLIRLLVMToLLVMIRTranslation
        MLIRTargetLLVMIRExport
        MLIRExecutionEngine
        MLIRMemRefDialect
        MLIRTransformUtils
)


target_link_libraries(sbt-lib PUBLIC ${LLVM_LIBS} ${MLIR_LIBS})
target_link_libraries(sbt PRIVATE sbt-lib)

set_target_properties(sbt-lib sbt PROPERTIES
        CXX_STANDARD 17
        CXX_STANDARD_REQUIRED ON
        COMPILE_FLAGS "-fno-rtti" # 关闭RTTI
)

if(BUILD_TESTING)
  add_subdirectory(tests)
endif()
