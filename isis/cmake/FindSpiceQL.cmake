# CMake module for find_package(SpiceQL)
# Finds include directory and all applicable libraries
#
# Sets the following:
#   SpiceQL_INCLUDE_DIR
#   SpiceQL_LIBRARY

find_path(SPICEQL_INCLUDE_DIR
  NAME spiceql.h
  PATH_SUFFIXES "SpiceQL"
)

find_library(SPICEQL_LIBRARY
  NAMES spiceql
)

get_filename_component(SUPERLU_ROOT_INCLUDE_DIR "${SUPERLU_INCLUDE_DIR}" DIRECTORY)


message(STATUS "SPICEQL INCLUDE DIR: "  ${SPICEQL_INCLUDE_DIR} )
message(STATUS "SPICEQL LIB: "  ${SPICEQL_LIBRARY} )

get_filename_component(SPICEQL_ROOT_INCLUDE_DIR "${SPICEQL_INCLUDE_DIR}" DIRECTORY)
