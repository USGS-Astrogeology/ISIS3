cmake_minimum_required(VERSION 3.10)

file(GLOB test_source "${CMAKE_SOURCE_DIR}/tests/*.cpp")

# Link runISISTests with what we want to test and the GTest and pthread library
add_executable(runISISCoreTests
               ${test_source})

target_link_libraries(runISISCoreTests core ${GTEST_LIBRARIES} ${GTEST_MAIN_LIBRARIES} Threads::Threads)

gtest_discover_tests(runISISCoreTests WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/tests)
