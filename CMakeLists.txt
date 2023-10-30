cmake_minimum_required(VERSION 3.1.0)
project(assign1)

find_package(Clang REQUIRED CONFIG HINTS ${LLVM_DIR} ${LLVM_DIR}/lib/cmake/clang NO_DEFAULT_PATH)

include_directories(${LLVM_INCLUDE_DIRS} ${CLANG_INCLUDE_DIRS} SYSTEM)
link_directories(${LLVM_LIBRARY_DIRS})

file(GLOB SOURCE "./*.cpp")

add_executable(ast-interpreter ${SOURCE})

set( LLVM_LINK_COMPONENTS
  ${LLVM_TARGETS_TO_BUILD}
  Option
  Support
  )


target_link_libraries(ast-interpreter
  clangAST
  clangBasic
  clangFrontend
  clangTooling
  )

install(TARGETS ast-interpreter
  RUNTIME DESTINATION bin)

enable_testing()

add_test(NAME test
  COMMAND bash -c "echo 100 | $<TARGET_FILE:ast-interpreter> \"$(cat ${CMAKE_CURRENT_SOURCE_DIR}/tests/test.c)\""
)

set_tests_properties(test PROPERTIES
  PASS_REGULAR_EXPRESSION "^Please Input an Integer Value : 100\n$"
  LABELS "example"
)

set(test_data
  "test00\;^100\n$"
  "test01\;^10\n$"
  "test02\;^20\n$"
  "test03\;^200\n$"
  "test04\;^10\n$"
  "test05\;^10\n$"
  "test06\;^20\n$"
  "test07\;^10\n$"
  "test08\;^20\n$"
  "test09\;^20\n$"
  "test10\;^5\n$"
  "test11\;^100\n$"
  "test12\;^4\n$"
  "test13\;^20\n$"
  "test14\;^12\n$"
  "test15\;^-8\n$"
  "test16\;^30\n$"
  "test17\;^10\n$"
  "test18\;^1020\n$"
  "test19\;^1020\n$"
)

foreach(test_info ${test_data})
  list(GET test_info 0 test_name)
  list(GET test_info 1 test_val)
  add_test(
    NAME ${test_name}
    COMMAND bash -c "$<TARGET_FILE:ast-interpreter> \"$(cat ${CMAKE_CURRENT_SOURCE_DIR}/tests/${test_name}.c)\""
  )
  set_tests_properties(${test} PROPERTIES
    PASS_REGULAR_EXPRESSION ${test_val}
    LABELS "official"
  )
endforeach()

set(extest_data
  "test010\;^10\n$"
  "test011\;^10\n$"
  "test012\;^10\n$"
  "test013\;^10\n$"
  "test014\;^10\n$"
  "test015\;^200\n$"
  "test016\;^10000005\n$"
  "test017\;^55\n$"
  "test120\;^1\n$"
  "test150\;^1\n$"
  "test170\;^10\n$"
  "test171\;^10\n$"
  "test172\;^10\n$"
  "test180\;^33\n$"
  "test181\;^1\n$"
  "test182\;^1020\n$"
)

foreach(test_info ${extest_data})
  list(GET test_info 0 test_name)
  list(GET test_info 1 test_val)
  add_test(
    NAME ${test_name}
    COMMAND bash -c "$<TARGET_FILE:ast-interpreter> \"$(cat ${CMAKE_CURRENT_SOURCE_DIR}/extests/${test_name}.c)\""
  )
  set_tests_properties(${test} PROPERTIES
    PASS_REGULAR_EXPRESSION ${test_val}
    LABELS "extra"
  )
endforeach()