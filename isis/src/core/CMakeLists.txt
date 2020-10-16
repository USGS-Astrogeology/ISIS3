cmake_minimum_required(VERSION 3.10)

project (CORE VERSION 0.0.1)
option(buildStaticCore "Build libisis static as well as dynamic"        OFF )


# Short and long name of this package
set(PACKAGE            "CORE")
set(PACKAGE_NAME       "USGS ISIS PVL CORE")

# specify the C++ standard
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED True)

#include cmake files from isis root
get_filename_component(PARENT_DIR ../../ ABSOLUTE)
list(APPEND CMAKE_MODULE_PATH ${PARENT_DIR}/cmake)
message(${CMAKE_MODULE_PATH})

include(Utilities)

FILE(GLOB_RECURSE Pvl_source_files ${PROJECT_SOURCE_DIR} *.h *.cpp)
list(FILTER Pvl_source_files EXCLUDE REGEX .*unitTest.cpp)

set(reqLibs
    geos
    Qt
)


# Define a target
add_library(CORE "${Pvl_source_files}")

MACRO(SUBDIRLIST result curdir)
  FILE(GLOB children RELATIVE ${curdir} ${curdir}/*)
  SET(dirlist "")
  FOREACH(child ${children})
    IF(IS_DIRECTORY ${curdir}/${child})
      LIST(APPEND dirlist ${child})
    ENDIF()
  ENDFOREACH()
  SET(${result} ${dirlist})
ENDMACRO()

SUBDIRLIST(subdirs ${PROJECT_SOURCE_DIR})

target_include_directories(CORE PUBLIC ${subdirs})

find_package(Qt5 REQUIRED COMPONENTS
                Core
                Concurrent
                Gui
                Multimedia
                MultimediaWidgets
                Network
                OpenGL # Needed to install mesa-common-dev for this!
                PrintSupport
                Qml
                Quick
                Script
                ScriptTools
                Sql
                Svg
                Test
                WebChannel
                Widgets
                Xml
                XmlPatterns
                # Search this path explicitly for MacOS OpenGL Framework
                PATHS /System/Library/Frameworks/ REQUIRED)

find_package(Json REQUIRED)
find_package(Geos REQUIRED)
find_package(nlohmann_json REQUIRED)
find_package(CSPICE    65      REQUIRED)

include_directories(SYSTEM
                    ${Qt5Widgets_INCLUDE_DIRS}
                    ${Qt5Concurrent_INCLUDE_DIRS}
                    ${Qt5Network_INCLUDE_DIRS}
                    ${Qt5Xml_INCLUDE_DIRS}
                    ${CSPICE_INCLUDE_DIR}
                    ${JSON_INCLUDE_DIR})
include_directories(${CMAKE_BINARY_DIR}/inc)
link_directories(${JSON_LIBRARY} ${CSPICE_LIBRARY})

target_link_libraries(CORE PUBLIC Qt5 Geos Json nlohmann_json::nlohmann_json)
