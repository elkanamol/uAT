# CMake generated Testfile for 
# Source directory: /home/runner/work/uAT/uAT/tests
# Build directory: /home/runner/work/uAT/uAT/tests/build
# 
# This file includes the relevant testing commands required for 
# testing this directory and lists subdirectories to be tested as well.
add_test(ParserTests "/home/runner/work/uAT/uAT/tests/build/test_parser")
set_tests_properties(ParserTests PROPERTIES  TIMEOUT "30" _BACKTRACE_TRIPLES "/home/runner/work/uAT/uAT/tests/CMakeLists.txt;80;add_test;/home/runner/work/uAT/uAT/tests/CMakeLists.txt;0;")
add_test(FreeRTOSTests "/home/runner/work/uAT/uAT/tests/build/test_freertos")
set_tests_properties(FreeRTOSTests PROPERTIES  TIMEOUT "30" _BACKTRACE_TRIPLES "/home/runner/work/uAT/uAT/tests/CMakeLists.txt;81;add_test;/home/runner/work/uAT/uAT/tests/CMakeLists.txt;0;")
