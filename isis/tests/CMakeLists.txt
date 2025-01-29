cmake_minimum_required(VERSION 3.10)

include(GoogleTest)

add_dependencies(isis isis)

file(GLOB test_source "${CMAKE_SOURCE_DIR}/tests/*.cpp")

# Non-ideal temporary solution to link against other shared libraries for mission tests, etc
file(GLOB isis_libs "${CMAKE_BINARY_DIR}/lib/*.so")
file(GLOB isis_mac_libs "${CMAKE_BINARY_DIR}/lib/*.dylib")

set(MISSION_LIBS
  apollo
  cassini
  chandrayaan1
  clementine
  clipper
  dawn
  galileo
  hayabusa
  hayabusa2
  juno
  kaguya
  lo
  lro
  mariner
  mer
  messenger
  mex
  mgs
  mro
  near
  newhorizons
  odyssey
  osirisrex
  rosetta
  tgo
  viking
  voyager)


# Link runISISTests with what we want to test and the GTest and pthread library
add_executable(runISISTests
               IsisTestMain.cpp
               ${test_source})

target_link_libraries(runISISTests isis ${MISSION_LIBS} ${ALLLIBS} gmock_main)

gtest_discover_tests(runISISTests WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/tests PROPERTIES DISCOVERY_TIMEOUT 6000)
