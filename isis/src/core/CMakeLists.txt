cmake_minimum_required(VERSION 3.10)

project (core VERSION 0.0.1)
option(buildStaticCore "Build libisis static as well as dynamic"        OFF )

list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake")

# Short and long name of this package
set(PACKAGE            "CORE")
set(PACKAGE_NAME       "USGS ISIS PVL CORE")

# specify the C++ standard
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED True)

FILE(GLOB_RECURSE Pvl_source_files ${PROJECT_SOURCE_DIR}/src *.cpp)

# Define a target
add_library(core "${Pvl_source_files}")

target_include_directories(core PUBLIC ${PROJECT_SOURCE_DIR}/include)

find_package(Qt5 REQUIRED COMPONENTS
                Core
                Concurrent
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

# find_package(Json REQUIRED)
find_package(Geos REQUIRED)
find_package(nlohmann_json REQUIRED)
find_package(CSPICE    65      REQUIRED)

#include_directories(SYSTEM
#                    ${Qt5Widgets_INCLUDE_DIRS}
#                    ${Qt5Concurrent_INCLUDE_DIRS}
#                    ${Qt5Network_INCLUDE_DIRS}
#                    ${Qt5Xml_INCLUDE_DIRS}
#                    ${CSPICE_INCLUDE_DIR}
#                    ${JSON_INCLUDE_DIR})
#include_directories(${CMAKE_BINARY_DIR}/inc)
#link_directories(${JSON_LIBRARY} ${CSPICE_LIBRARY} {Qt5_Library})

target_link_libraries(core PUBLIC Qt5::Core
                                  Qt5::Concurrent
                                  Qt5::Widgets
                                  Qt5::Network
                                  Qt5::Xml
                                  nlohmann_json::nlohmann_json)

target_link_libraries(core PUBLIC ${GEOS_LIBRARY} ${CSPICE_LIBRARY})
target_include_directories(core PUBLIC ${GEOS_INCLUDE_DIR} ${CSPICE_INCLUDE_DIR})
install(DIRECTORY ${PROJECT_SOURCE_DIR}/include DESTINATION  ${CMAKE_INSTALL_PREFIX}/include/isis)
install(TARGETS core DESTINATION ${CMAKE_INSTALL_PREFIX}/lib)
