if(NOT DEFINED SBT OR NOT DEFINED INPUT)
  message(FATAL_ERROR "SBT and INPUT are required")
endif()

set(OUTPUT "${INPUT}.sbt.o")
file(REMOVE "${OUTPUT}")
execute_process(
  COMMAND "${SBT}" --quiet -i "${INPUT}"
  RESULT_VARIABLE RESULT)
if(NOT RESULT EQUAL 0)
  message(FATAL_ERROR "sbt failed with exit code ${RESULT}")
endif()
if(NOT EXISTS "${OUTPUT}")
  message(FATAL_ERROR "default output was not created: ${OUTPUT}")
endif()
file(REMOVE "${OUTPUT}")
