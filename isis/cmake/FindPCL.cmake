# CMake module for find_package(PCL)
# Finds include directory and all applicable libraries
#
# Sets the following:
#   PCL_INCLUDE_DIR
#   PCL_LIBRARY

find_path(PCL_INCLUDE_DIR
  NAME pcl_base.h
  PATH_SUFFIXES "pcl-1.8/pcl" "pcl-1.9/pcl" "pcl-1.11/pcl" "pcl-1.12/pcl" "pcl-1.13/pcl" "pcl-1.14/pcl"
)

find_library(PCL_COMMON_LIBRARY NAMES pcl_common)
find_library(PCL_OCTREE_LIBRARY NAMES pcl_octree)
find_library(PCL_IO_LIBRARY     NAMES pcl_io)

get_filename_component(PCL_ROOT_INCLUDE_DIR "${PCL_INCLUDE_DIR}" DIRECTORY)

message(STATUS "PCL INCLUDE DIR: ${PCL_INCLUDE_DIR}")
message(STATUS "PCL COMMON LIB: ${PCL_COMMON_LIBRARY}")
message(STATUS "PCL OCTREE LIB: ${PCL_OCTREE_LIBRARY}")
message(STATUS "PCL IO LIB: ${PCL_IO_LIBRARY}")
