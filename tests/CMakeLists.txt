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
  add_executable("${TRANSLATED_TARGET}" fixture_runner.cpp
                 "${CASE_TRANSLATED_OBJECT}")
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
