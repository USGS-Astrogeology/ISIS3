cmake_minimum_required(VERSION 3.10)

find_package(Threads)

include(../cmake/gtest.cmake)
include(GoogleTest)

set(test_source ${PROJECT_SOURCE_DIR}/tests/ConstantsTests.cpp
                ${PROJECT_SOURCE_DIR}/tests/IsisCoreTestMain.cpp
                ${PROJECT_SOURCE_DIR}/tests/TableTests.cpp)

# Link runISISTests with what we want to test and the GTest and pthread library
add_executable(runISISCoreTests
               ${test_source})

target_link_libraries(runISISCoreTests core gtest gmock Threads::Threads)

gtest_discover_tests(runISISCoreTests WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}/tests)

# Add old ISIS unitTests
add_subdirectory(unitTests)
