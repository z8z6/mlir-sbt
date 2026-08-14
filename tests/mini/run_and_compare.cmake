if(NOT DEFINED PROGRAM OR NOT DEFINED EXPECTED)
  message(FATAL_ERROR "PROGRAM and EXPECTED are required")
endif()

execute_process(
  COMMAND "${PROGRAM}"
  RESULT_VARIABLE program_result
  OUTPUT_VARIABLE actual_output
  ERROR_VARIABLE program_error)
if(NOT program_result EQUAL 0)
  message(FATAL_ERROR
    "${PROGRAM} exited with ${program_result}: ${program_error}")
endif()

file(READ "${EXPECTED}" expected_output)
if(NOT actual_output STREQUAL expected_output)
  string(REPLACE "\n" "\\n" actual_display "${actual_output}")
  string(REPLACE "\n" "\\n" expected_display "${expected_output}")
  message(FATAL_ERROR
    "stdout mismatch: expected '${expected_display}', got '${actual_display}'")
endif()
