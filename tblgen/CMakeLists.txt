## Intrinsics.td
#include_directories(${CMAKE_SOURCE_DIR}/third/llvm/llvm/include/llvm/IR/)
#
#set(LLVM_TARGET_DEFINITIONS DirectTrans.td)
#
#tablegen(LLVM IntrinsicImpl.inc -gen-intrinsic-impl)
#tablegen(LLVM IntrinsicEnums.inc -gen-intrinsic-enums)
#tablegen(LLVM IntrinsicsRISCV.h -gen-intrinsic-enums -intrinsic-prefix=riscv)
#
#add_public_tablegen_target(direct_trans_gen)

cmake_minimum_required(VERSION 3.13)

project(RVision-tblgen)

file(GLOB_RECURSE SOURCE src/*.cpp include/*.h)

llvm_map_components_to_libnames(
        LLVM_LIBS
        TableGen
        MC
        Support
)

add_executable(ir1-tblgen ${SOURCE})

target_link_libraries(ir1-tblgen ${LLVM_LIBS})

target_include_directories(ir1-tblgen PUBLIC include)

set_target_properties(ir1-tblgen PROPERTIES
        CXX_STANDARD 17
        CXX_STANDARD_REQUIRED ON
        COMPILE_FLAGS "-fno-rtti" # 关闭RTTI
)

set(IR1_X86_Converter "${CMAKE_CURRENT_BINARY_DIR}/IR1Converter.cpp")
add_custom_command(
        OUTPUT ${IR1_X86_Converter}
        # 依赖 td 文件，td 修改自动重新生成
        DEPENDS IR1X86.td ir1-tblgen
        # tblgen 执行命令
        COMMAND $<TARGET_FILE:ir1-tblgen>
        -I ${CMAKE_SOURCE_DIR}/third/llvm/llvm/include
        -I ${CMAKE_SOURCE_DIR}/third/llvm/llvm/include/llvm/Target
        -I ${CMAKE_SOURCE_DIR}/third/llvm/llvm/lib/Target/X86
        --gen-x86-ir1
        ${CMAKE_CURRENT_SOURCE_DIR}/IR1X86.td
        -o ${IR1_X86_Converter}
        COMMENT "Generating IR1Converter using ir1-tblgen"
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
)

add_custom_target(
        ir1-x86-converter-gen
        DEPENDS ${IR1_X86_Converter}
)