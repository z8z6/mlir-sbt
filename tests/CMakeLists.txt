add_executable(sbt-unit main.cpp)
target_link_libraries(sbt-unit PRIVATE sbt-lib gtest)
set_target_properties(sbt-unit PROPERTIES
  CXX_STANDARD 17
  CXX_STANDARD_REQUIRED ON
  COMPILE_FLAGS "-fno-rtti"
)
add_test(NAME sbt-unit COMMAND sbt-unit)

# Architecture fixtures are deliberately one executable per leaf directory.
# This keeps every assembly file self-contained and gives CTest a precise case
# name when an encoding or an expected architectural state regresses.
enable_language(ASM_NASM)
set(X86_SYSCALL_RUNTIME
    "${CMAKE_SOURCE_DIR}/runtime/x86_64_linux.asm")
set_source_files_properties("${X86_SYSCALL_RUNTIME}" PROPERTIES
  LANGUAGE ASM_NASM)
file(GLOB X86_CASES CONFIGURE_DEPENDS
  "${CMAKE_CURRENT_SOURCE_DIR}/isa/x86/*/*/case.asm")

foreach(CASE_ASM IN LISTS X86_CASES)
  get_filename_component(CASE_DIR "${CASE_ASM}" DIRECTORY)
  get_filename_component(INSTRUCTION_DIR "${CASE_DIR}" DIRECTORY)
  get_filename_component(INSTRUCTION "${INSTRUCTION_DIR}" NAME)
  file(RELATIVE_PATH CASE_REL
    "${INSTRUCTION_DIR}" "${CASE_DIR}")
  set(CASE_TARGET "x86_${INSTRUCTION}_${CASE_REL}")
  string(REPLACE "-" "_" CASE_TARGET "${CASE_TARGET}")
  set_source_files_properties("${CASE_ASM}" PROPERTIES LANGUAGE ASM_NASM)
  add_executable("${CASE_TARGET}" fixture_runner.cpp fixture_executor.asm
                 "${CASE_ASM}")
  target_compile_definitions("${CASE_TARGET}" PRIVATE
    INPUT_FILE="${CASE_DIR}/input.toml"
    OUTPUT_FILE="${CASE_DIR}/output.toml")
  set_target_properties("${CASE_TARGET}" PROPERTIES
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED ON)
  add_test(NAME "isa.x86.${INSTRUCTION}.${CASE_REL}"
           COMMAND "${CASE_TARGET}")

  set(CASE_INPUT_OBJECT
      "${CMAKE_CURRENT_BINARY_DIR}/${CASE_TARGET}.input.o")
  set(CASE_TRANSLATED_OBJECT
      "${CMAKE_CURRENT_BINARY_DIR}/${CASE_TARGET}.translated.o")
  add_custom_command(
    OUTPUT "${CASE_INPUT_OBJECT}"
    COMMAND "${CMAKE_ASM_NASM_COMPILER}" -f elf64
            -o "${CASE_INPUT_OBJECT}" "${CASE_ASM}"
    DEPENDS "${CASE_ASM}"
    VERBATIM)
  add_custom_command(
    OUTPUT "${CASE_TRANSLATED_OBJECT}"
    COMMAND $<TARGET_FILE:sbt> --quiet -i "${CASE_INPUT_OBJECT}"
            -o "${CASE_TRANSLATED_OBJECT}"
    DEPENDS sbt "${CASE_INPUT_OBJECT}"
    VERBATIM)
  set_source_files_properties("${CASE_TRANSLATED_OBJECT}" PROPERTIES
    EXTERNAL_OBJECT TRUE
    GENERATED TRUE)
  set(TRANSLATED_TARGET "${CASE_TARGET}_translated")
  set(TRANSLATED_SOURCES fixture_runner.cpp "${CASE_TRANSLATED_OBJECT}")
  if(INSTRUCTION STREQUAL "syscall")
    list(APPEND TRANSLATED_SOURCES "${X86_SYSCALL_RUNTIME}")
  endif()
  add_executable("${TRANSLATED_TARGET}" ${TRANSLATED_SOURCES})
  target_compile_definitions("${TRANSLATED_TARGET}" PRIVATE
    TRANSLATED_FIXTURE=1
    INPUT_FILE="${CASE_DIR}/input.toml"
    OUTPUT_FILE="${CASE_DIR}/output.toml")
  set_target_properties("${TRANSLATED_TARGET}" PROPERTIES
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED ON)
  add_test(NAME "isa.x86.${INSTRUCTION}.${CASE_REL}.translated"
           COMMAND "${TRANSLATED_TARGET}")
endforeach()

add_test(
  NAME cli.default_output_path
  COMMAND "${CMAKE_COMMAND}"
          "-DSBT=$<TARGET_FILE:sbt>"
          "-DINPUT=${CMAKE_CURRENT_BINARY_DIR}/mini_x86_hello_world.native"
          -P "${CMAKE_CURRENT_SOURCE_DIR}/default_output_path.cmake")
set_tests_properties(cli.default_output_path PROPERTIES
  DEPENDS mini.x86.hello_world.native
  TIMEOUT 10)

# Mini programs exercise whole ELF inputs separately from instruction-pure ISA
# fixtures. Each case must run natively and match expected.txt. Translation is
# tested independently so translation and execution failures remain distinct.
file(GLOB MINI_X86_PROGRAMS CONFIGURE_DEPENDS
  "${CMAKE_CURRENT_SOURCE_DIR}/mini/x86/*/program.asm"
  "${CMAKE_CURRENT_SOURCE_DIR}/mini/x86/*/program.c")

foreach(PROGRAM_ASM IN LISTS MINI_X86_PROGRAMS)
  get_filename_component(PROGRAM_DIR "${PROGRAM_ASM}" DIRECTORY)
  get_filename_component(PROGRAM_NAME "${PROGRAM_DIR}" NAME)
  set(PROGRAM_TARGET "mini_x86_${PROGRAM_NAME}")
  string(REPLACE "-" "_" PROGRAM_TARGET "${PROGRAM_TARGET}")
  set(PROGRAM_OBJECT
      "${CMAKE_CURRENT_BINARY_DIR}/${PROGRAM_TARGET}.o")
  set(PROGRAM_NATIVE
      "${CMAKE_CURRENT_BINARY_DIR}/${PROGRAM_TARGET}.native")
  set(PROGRAM_TRANSLATED
      "${CMAKE_CURRENT_BINARY_DIR}/${PROGRAM_TARGET}.translated.o")
  set(PROGRAM_TRANSLATED_EXE
      "${CMAKE_CURRENT_BINARY_DIR}/${PROGRAM_TARGET}.translated")
  set(PROGRAM_DATA_ASM "${PROGRAM_DIR}/data.asm")
  set(PROGRAM_DATA_OBJECT
      "${CMAKE_CURRENT_BINARY_DIR}/${PROGRAM_TARGET}.data.o")
  set(PROGRAM_RUNTIME_OBJECT
      "${CMAKE_CURRENT_BINARY_DIR}/${PROGRAM_TARGET}.runtime.o")
  set(PROGRAM_LAUNCHER_OBJECT
      "${CMAKE_CURRENT_BINARY_DIR}/${PROGRAM_TARGET}.launcher.o")

  if(PROGRAM_NAME STREQUAL "hello_world_glibc")
    add_custom_command(
      OUTPUT "${PROGRAM_NATIVE}"
      COMMAND "${CMAKE_C_COMPILER}" "${PROGRAM_ASM}" -o "${PROGRAM_NATIVE}"
      DEPENDS "${PROGRAM_ASM}"
      VERBATIM)
    add_custom_command(
      OUTPUT "${PROGRAM_DATA_OBJECT}"
      COMMAND "${CMAKE_ASM_NASM_COMPILER}" -f elf64
              -o "${PROGRAM_DATA_OBJECT}" "${PROGRAM_DATA_ASM}"
      DEPENDS "${PROGRAM_DATA_ASM}"
      VERBATIM)
    add_custom_command(
      OUTPUT "${PROGRAM_TRANSLATED}"
      COMMAND $<TARGET_FILE:sbt> --quiet --function=main
              --address-bias=0x400000 -i "${PROGRAM_NATIVE}"
              -o "${PROGRAM_TRANSLATED}"
      DEPENDS sbt "${PROGRAM_NATIVE}"
      VERBATIM)
    add_custom_command(
      OUTPUT "${PROGRAM_LAUNCHER_OBJECT}"
      COMMAND "${CMAKE_C_COMPILER}" -c
              -o "${PROGRAM_LAUNCHER_OBJECT}"
              "${CMAKE_CURRENT_SOURCE_DIR}/mini/glibc_launcher.c"
      DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/mini/glibc_launcher.c"
      VERBATIM)
    add_custom_command(
      OUTPUT "${PROGRAM_TRANSLATED_EXE}"
      COMMAND "${CMAKE_C_COMPILER}" -no-pie -o "${PROGRAM_TRANSLATED_EXE}"
              "${PROGRAM_LAUNCHER_OBJECT}" "${PROGRAM_TRANSLATED}"
              "${PROGRAM_DATA_OBJECT}"
      DEPENDS "${PROGRAM_LAUNCHER_OBJECT}" "${PROGRAM_TRANSLATED}"
              "${PROGRAM_DATA_OBJECT}"
      VERBATIM)
    add_custom_target("${PROGRAM_TARGET}" ALL
      DEPENDS "${PROGRAM_NATIVE}" "${PROGRAM_TRANSLATED_EXE}")

    set(NATIVE_TEST "mini.x86.${PROGRAM_NAME}.native")
    add_test(
      NAME "${NATIVE_TEST}"
      COMMAND "${CMAKE_COMMAND}"
              "-DPROGRAM=${PROGRAM_NATIVE}"
              "-DEXPECTED=${PROGRAM_DIR}/expected.txt"
              -P "${CMAKE_CURRENT_SOURCE_DIR}/mini/run_and_compare.cmake")
    set_tests_properties("${NATIVE_TEST}" PROPERTIES
      LABELS "mini-program;glibc"
      TIMEOUT 10)
    set(TRANSLATE_TEST "mini.x86.${PROGRAM_NAME}.translate")
    add_test(
      NAME "${TRANSLATE_TEST}"
      COMMAND $<TARGET_FILE:sbt> --quiet --function=main
              --address-bias=0x400000 -i "${PROGRAM_NATIVE}"
              -o "${PROGRAM_TRANSLATED}")
    set_tests_properties("${TRANSLATE_TEST}" PROPERTIES
      DEPENDS "${NATIVE_TEST}"
      LABELS "mini-program;glibc"
      TIMEOUT 10)
    set(TRANSLATED_RUN_TEST "mini.x86.${PROGRAM_NAME}.translated")
    add_test(
      NAME "${TRANSLATED_RUN_TEST}"
      COMMAND "${CMAKE_COMMAND}"
              "-DPROGRAM=${PROGRAM_TRANSLATED_EXE}"
              "-DEXPECTED=${PROGRAM_DIR}/expected.txt"
              -P "${CMAKE_CURRENT_SOURCE_DIR}/mini/run_and_compare.cmake")
    set_tests_properties("${TRANSLATED_RUN_TEST}" PROPERTIES
      DEPENDS "${TRANSLATE_TEST}"
      LABELS "mini-program;glibc"
      TIMEOUT 10)
    continue()
  endif()

  get_filename_component(PROGRAM_EXTENSION "${PROGRAM_ASM}" EXT)
  if(PROGRAM_EXTENSION STREQUAL ".c")
    add_custom_command(
      OUTPUT "${PROGRAM_OBJECT}"
      COMMAND "${CMAKE_C_COMPILER}" -c -O2 -ffreestanding
              -fno-stack-protector -fno-asynchronous-unwind-tables
              -fno-pie -fno-pic -o "${PROGRAM_OBJECT}" "${PROGRAM_ASM}"
      DEPENDS "${PROGRAM_ASM}"
      VERBATIM)
  else()
    add_custom_command(
      OUTPUT "${PROGRAM_OBJECT}"
      COMMAND "${CMAKE_ASM_NASM_COMPILER}" -f elf64
              -o "${PROGRAM_OBJECT}" "${PROGRAM_ASM}"
      DEPENDS "${PROGRAM_ASM}"
      VERBATIM)
  endif()
  set(PROGRAM_DATA_OBJECTS)
  if(EXISTS "${PROGRAM_DATA_ASM}")
    add_custom_command(
      OUTPUT "${PROGRAM_DATA_OBJECT}"
      COMMAND "${CMAKE_ASM_NASM_COMPILER}" -f elf64
              -o "${PROGRAM_DATA_OBJECT}" "${PROGRAM_DATA_ASM}"
      DEPENDS "${PROGRAM_DATA_ASM}"
      VERBATIM)
    list(APPEND PROGRAM_DATA_OBJECTS "${PROGRAM_DATA_OBJECT}")
  endif()
  add_custom_command(
    OUTPUT "${PROGRAM_NATIVE}"
    COMMAND "${CMAKE_LINKER}" -static
            -T "${CMAKE_CURRENT_SOURCE_DIR}/mini/x86_64.ld"
            -o "${PROGRAM_NATIVE}" "${PROGRAM_OBJECT}"
            ${PROGRAM_DATA_OBJECTS}
    DEPENDS "${PROGRAM_OBJECT}" ${PROGRAM_DATA_OBJECTS}
            "${CMAKE_CURRENT_SOURCE_DIR}/mini/x86_64.ld"
    VERBATIM)
  add_custom_command(
    OUTPUT "${PROGRAM_TRANSLATED}"
    COMMAND $<TARGET_FILE:sbt> --quiet -i "${PROGRAM_NATIVE}"
            -o "${PROGRAM_TRANSLATED}"
    DEPENDS sbt "${PROGRAM_NATIVE}"
    VERBATIM)
  add_custom_command(
    OUTPUT "${PROGRAM_RUNTIME_OBJECT}"
    COMMAND "${CMAKE_ASM_NASM_COMPILER}" -f elf64
            -o "${PROGRAM_RUNTIME_OBJECT}" "${X86_SYSCALL_RUNTIME}"
    DEPENDS "${X86_SYSCALL_RUNTIME}"
    VERBATIM)
  add_custom_command(
    OUTPUT "${PROGRAM_LAUNCHER_OBJECT}"
    COMMAND "${CMAKE_ASM_NASM_COMPILER}" -f elf64
            -o "${PROGRAM_LAUNCHER_OBJECT}"
            "${CMAKE_CURRENT_SOURCE_DIR}/mini/x86/launcher.asm"
    DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/mini/x86/launcher.asm"
    VERBATIM)
  add_custom_command(
    OUTPUT "${PROGRAM_TRANSLATED_EXE}"
    COMMAND "${CMAKE_LINKER}" -static
            -T "${CMAKE_CURRENT_SOURCE_DIR}/mini/x86_64.ld"
            -o "${PROGRAM_TRANSLATED_EXE}" "${PROGRAM_LAUNCHER_OBJECT}"
            "${PROGRAM_RUNTIME_OBJECT}" "${PROGRAM_TRANSLATED}"
            ${PROGRAM_DATA_OBJECTS}
    DEPENDS "${PROGRAM_LAUNCHER_OBJECT}" "${PROGRAM_RUNTIME_OBJECT}"
            "${PROGRAM_TRANSLATED}" ${PROGRAM_DATA_OBJECTS}
            "${CMAKE_CURRENT_SOURCE_DIR}/mini/x86_64.ld"
    VERBATIM)
  add_custom_target("${PROGRAM_TARGET}" ALL
    DEPENDS "${PROGRAM_NATIVE}" "${PROGRAM_TRANSLATED_EXE}")

  set(NATIVE_TEST "mini.x86.${PROGRAM_NAME}.native")
  add_test(
    NAME "${NATIVE_TEST}"
    COMMAND "${CMAKE_COMMAND}"
            "-DPROGRAM=${PROGRAM_NATIVE}"
            "-DEXPECTED=${PROGRAM_DIR}/expected.txt"
            -P "${CMAKE_CURRENT_SOURCE_DIR}/mini/run_and_compare.cmake")
  set_tests_properties("${NATIVE_TEST}" PROPERTIES
    LABELS "mini-program"
    TIMEOUT 10)

  set(TRANSLATE_TEST "mini.x86.${PROGRAM_NAME}.translate")
  add_test(
    NAME "${TRANSLATE_TEST}"
    COMMAND $<TARGET_FILE:sbt> --quiet -i "${PROGRAM_NATIVE}"
            -o "${PROGRAM_TRANSLATED}")
  set_tests_properties("${TRANSLATE_TEST}" PROPERTIES
    DEPENDS "${NATIVE_TEST}"
    LABELS "mini-program"
    TIMEOUT 10)
  set(TRANSLATED_RUN_TEST "mini.x86.${PROGRAM_NAME}.translated")
  add_test(
    NAME "${TRANSLATED_RUN_TEST}"
    COMMAND "${CMAKE_COMMAND}"
            "-DPROGRAM=${PROGRAM_TRANSLATED_EXE}"
            "-DEXPECTED=${PROGRAM_DIR}/expected.txt"
            -P "${CMAKE_CURRENT_SOURCE_DIR}/mini/run_and_compare.cmake")
  set_tests_properties("${TRANSLATED_RUN_TEST}" PROPERTIES
    DEPENDS "${TRANSLATE_TEST}"
    LABELS "mini-program"
    TIMEOUT 10)
endforeach()
