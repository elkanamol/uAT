cmake_minimum_required(VERSION 3.16)
project(uAT_Tests C)

# Set C standard
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

# Enable testing
enable_testing()

# Add compile flags for better debugging
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Wextra -g -O0")

# Define source directories
set(UAT_CORE_DIR "${CMAKE_SOURCE_DIR}/../Core")
set(UAT_SRC_DIR "${UAT_CORE_DIR}/Src")
set(UAT_INC_DIR "${UAT_CORE_DIR}/Inc")

# Include directories
include_directories(
    ${UAT_INC_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/mocks
    ${CMAKE_CURRENT_SOURCE_DIR}/framework
)

# Create a simple test framework
add_library(test_framework STATIC
    framework/test_framework.c
)

# Create mocks for dependencies
add_library(uat_mocks STATIC
    mocks/stm32_hal_mock.c
    mocks/freertos_mock.c
)

# Parser tests (can run standalone without FreeRTOS)
add_library(uat_parser_lib STATIC
    ${UAT_SRC_DIR}/uat_parser.c
)

target_include_directories(uat_parser_lib PUBLIC ${UAT_INC_DIR})

# Parser test executable
add_executable(test_parser
    test_parser.c
)

target_link_libraries(test_parser 
    uat_parser_lib
    test_framework
)

# FreeRTOS tests (with mocks)
add_library(uat_freertos_lib STATIC
    ${UAT_SRC_DIR}/uat_freertos.c
)

target_include_directories(uat_freertos_lib PUBLIC 
    ${UAT_INC_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/mocks
)

target_link_libraries(uat_freertos_lib
    uat_mocks
)

# FreeRTOS test executable
add_executable(test_freertos
    test_freertos.c
)

target_link_libraries(test_freertos
    uat_freertos_lib
    uat_mocks
    test_framework
)

# Add tests to CTest
add_test(NAME ParserTests COMMAND test_parser)
# Note: FreeRTOS tests are placeholder - uncomment when fully implemented
# add_test(NAME FreeRTOSTests COMMAND test_freertos)

# Set test properties
set_tests_properties(ParserTests PROPERTIES TIMEOUT 30)
# set_tests_properties(FreeRTOSTests PROPERTIES TIMEOUT 30)