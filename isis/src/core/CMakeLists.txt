cmake_minimum_required(VERSION 3.10)

project (core VERSION 0.0.1)
option(buildStaticCore "Build libisis static as well as dynamic"        OFF )
option (BUILD_CORE_TESTS "Build core module tests" ON)

list(APPEND CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/cmake")

# Short and long name of this package
set(PACKAGE            "CORE")
set(PACKAGE_NAME       "USGS ISIS PVL CORE")

# specify the C++ standard
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED True)

set(Pvl_source_files ${PROJECT_SOURCE_DIR}/src/ArraySubscriptNotInRange.cpp
                     ${PROJECT_SOURCE_DIR}/src/FileCreate.cpp
                     ${PROJECT_SOURCE_DIR}/src/FileName.cpp
                     ${PROJECT_SOURCE_DIR}/src/FileOpen.cpp
                     ${PROJECT_SOURCE_DIR}/src/FileRead.cpp
                     ${PROJECT_SOURCE_DIR}/src/FileWrite.cpp
                     ${PROJECT_SOURCE_DIR}/src/IException.cpp
                     ${PROJECT_SOURCE_DIR}/src/IString.cpp
                     ${PROJECT_SOURCE_DIR}/src/KeywordAmbiguous.cpp
                     ${PROJECT_SOURCE_DIR}/src/KeywordBlockEndMissing.cpp
                     ${PROJECT_SOURCE_DIR}/src/KeywordBlockInvalid.cpp
                     ${PROJECT_SOURCE_DIR}/src/KeywordBlockStartMissing.cpp
                     ${PROJECT_SOURCE_DIR}/src/KeywordDuplicated.cpp
                     ${PROJECT_SOURCE_DIR}/src/KeywordNotArray.cpp
                     ${PROJECT_SOURCE_DIR}/src/KeywordNotFound.cpp
                     ${PROJECT_SOURCE_DIR}/src/KeywordUnrecognized.cpp
                     ${PROJECT_SOURCE_DIR}/src/KeywordValueBad.cpp
                     ${PROJECT_SOURCE_DIR}/src/KeywordValueExpected.cpp
                     ${PROJECT_SOURCE_DIR}/src/KeywordValueNotInList.cpp
                     ${PROJECT_SOURCE_DIR}/src/KeywordValueNotInRange.cpp
                     ${PROJECT_SOURCE_DIR}/src/LabelTranslationManager.cpp
                     ${PROJECT_SOURCE_DIR}/src/MemoryAllocationFailed.cpp
                     ${PROJECT_SOURCE_DIR}/src/MissingDelimiter.cpp
                     ${PROJECT_SOURCE_DIR}/src/Preference.cpp
                     ${PROJECT_SOURCE_DIR}/src/Pvl.cpp
                     ${PROJECT_SOURCE_DIR}/src/PvlContainer.cpp
                     ${PROJECT_SOURCE_DIR}/src/PvlFlatMap.cpp
                     ${PROJECT_SOURCE_DIR}/src/PvlFormat.cpp
                     ${PROJECT_SOURCE_DIR}/src/PvlFormatPds.cpp
                     ${PROJECT_SOURCE_DIR}/src/PvlGroup.cpp
                     ${PROJECT_SOURCE_DIR}/src/PvlKeyword.cpp
                     ${PROJECT_SOURCE_DIR}/src/PvlObject.cpp
                     ${PROJECT_SOURCE_DIR}/src/PvlSequence.cpp
                     ${PROJECT_SOURCE_DIR}/src/PvlToPvlTranslationManager.cpp
                     ${PROJECT_SOURCE_DIR}/src/PvlToXmlTranslationManager.cpp
                     ${PROJECT_SOURCE_DIR}/src/PvlToken.cpp
                     ${PROJECT_SOURCE_DIR}/src/PvlTokenizer.cpp
                     ${PROJECT_SOURCE_DIR}/src/PvlTranslationTable.cpp
                     ${PROJECT_SOURCE_DIR}/src/TextFile.cpp)
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

include_directories(SYSTEM
                    ${Qt5Widgets_INCLUDE_DIRS}
                    ${Qt5Concurrent_INCLUDE_DIRS}
                    ${Qt5Network_INCLUDE_DIRS}
                    ${Qt5Xml_INCLUDE_DIRS}
                    ${CSPICE_INCLUDE_DIR}
                    ${JSON_INCLUDE_DIR})

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

if (BUILD_CORE_TESTS)
  enable_testing()
  add_subdirectory(tests)
endif()

if(buildSwig)
  add_subdirectory(swig)
endif()
