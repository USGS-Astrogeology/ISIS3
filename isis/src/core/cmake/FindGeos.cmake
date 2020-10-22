# CMake module for find_package(Geos)
# Finds include directory and all applicable libraries
#
# Sets the following:
#   GEOS_INCLUDE_DIR
#   GEOS_LIBRARY

find_path(GEOS_INCLUDE_DIR
  NAMES geos.h
  PATH_SUFFIXES "geos/geos${Geos_FIND_VERSION}" "geos"
)

find_library(GEOS_LIBRARY
  NAMES geos
)
find_library(GEOS_C_LIBRARY
  NAMES geos_c
)

message(STATUS "GEOS INCLUDE DIR: "  ${GEOS_INCLUDE_DIR} )
message(STATUS "GEOS LIB: "  ${GEOS_LIBRARY} )
message(STATUS "GEOS C LIB: "  ${GEOS_C_LIBRARY} )

get_filename_component(GEOS_ROOT_INCLUDE_DIR "${GEOS_INCLUDE_DIR}" DIRECTORY)
