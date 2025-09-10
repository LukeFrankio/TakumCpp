#[[\n  ctest_log.cmake - Run CTest, capture output to a log file, optionally emit JUnit XML,\n  but do NOT make the overall build fail if tests fail.\n\n  Variables expected (passed via -D):\n    BUILD_DIR   : binary dir where ctest should run (usually CMAKE_BINARY_DIR)\n    LOG_FILE    : path to write textual combined output (required)\n    JUNIT_FILE  : optional path to write JUnit XML (if provided and supported)\n\n  Usage example (in CMakeLists.txt):\n    add_custom_target(test_log\n      COMMAND ${CMAKE_COMMAND} -DBUILD_DIR=${CMAKE_BINARY_DIR} -DLOG_FILE=${TEST_LOG_FILE} -P ${CMAKE_SOURCE_DIR}/scripts/ctest_log.cmake)\n\n  This script always returns success (exit code 0).\n]]

if(NOT DEFINED BUILD_DIR)
  message(WARNING "ctest_log.cmake: BUILD_DIR not set; defaulting to current directory")
  set(BUILD_DIR "${CMAKE_CURRENT_LIST_DIR}")
endif()
if(NOT DEFINED LOG_FILE)
  message(FATAL_ERROR "ctest_log.cmake: LOG_FILE must be specified")
endif()

set(_ctest_cmd ctest)

message(STATUS "[ctest_log] Running tests in '${BUILD_DIR}' -> '${LOG_FILE}'")
execute_process(
  COMMAND ${_ctest_cmd} --output-on-failure
  WORKING_DIRECTORY "${BUILD_DIR}"
  RESULT_VARIABLE _res
  OUTPUT_VARIABLE _out
  ERROR_VARIABLE  _err
)

file(WRITE "${LOG_FILE}" "${_out}${_err}")
message(STATUS "[ctest_log] CTest exit code: ${_res}; log saved (${LOG_FILE})")

if(DEFINED JUNIT_FILE)
  # Only attempt if ctest likely supports --output-junit (>=3.20)
  execute_process(COMMAND ${_ctest_cmd} --version OUTPUT_VARIABLE _ver_raw)
  string(REGEX MATCH "([0-9]+)\\.([0-9]+)" _ver_match "${_ver_raw}")
  set(_gen_junit OFF)
  if(_ver_match)
    set(_maj "${CMAKE_MATCH_1}")
    set(_min "${CMAKE_MATCH_2}")
    if(_maj GREATER 3 OR (_maj EQUAL 3 AND _min GREATER_EQUAL 20))
      set(_gen_junit ON)
    endif()
  endif()
  if(_gen_junit)
    message(STATUS "[ctest_log] Generating JUnit XML '${JUNIT_FILE}'")
    execute_process(
      COMMAND ${_ctest_cmd} --output-junit "${JUNIT_FILE}"
      WORKING_DIRECTORY "${BUILD_DIR}"
      RESULT_VARIABLE _jres
      OUTPUT_VARIABLE _jout
      ERROR_VARIABLE  _jerr
    )
    if(EXISTS "${JUNIT_FILE}")
      message(STATUS "[ctest_log] JUnit written (${JUNIT_FILE}), exit code ${_jres}")
    else()
      message(WARNING "[ctest_log] JUnit generation attempted but file missing; exit code ${_jres}")
    endif()
  else()
    message(STATUS "[ctest_log] CTest version too old for --output-junit; skipping JUnit generation")
  endif()
endif()

# Always succeed
message(STATUS "[ctest_log] Completed (non-fatal).")