#===============================================================================
#      The main build file for building ISIS using CMake.
#===============================================================================
# CMake initialization

# Specify the required version of CMake.  If your machine does not
#  have this, it should be easy to build from https://cmake.org/download/
cmake_minimum_required(VERSION 3.10)

# Point cmake to our other CMake files.
list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake")

set(CMAKE_FRAMEWORK_PATH /System/Library/Frameworks/)
set(CMAKE_FIND_FRAMEWORK LAST)

include(AddIsisModule)
include(Utilities)
include(TestSetup)
include(InstallThirdParty)
include(cmake/gtest.cmake)
include(GoogleTest)

include(CMakePrintHelpers)

#===============================================================================
#===============================================================================
# Project information

project (USGS_ISIS)

# Short and long name of this package
set(PACKAGE            "ISIS")
set(PACKAGE_NAME       "USGS ISIS")

# Version number
set(VERSION            "6.0.0")
string(REPLACE "." ";" VERSION_LIST ${VERSION})
list(GET VERSION_LIST 0 VERSION_MAJOR)
list(GET VERSION_LIST 1 VERSION_MINOR)

set(PACKAGE_VERSION    ${VERSION})

# Full name and version number
set(PACKAGE_STRING     "${PACKAGE_NAME} ${VERSION}")

# Other release information
string(TIMESTAMP VERSION_DATE "%Y-%m-%d" UTC)
set(RELEASE_STAGE             "alpha") # (alpha, beta, stable)

# Define to the address where bug reports for this package should be sent.
set(PACKAGE_BUGREPORT  "https://github.com/USGS-Astrogeology/ISIS3/issues")

# Main website associated with the software package
set(PACKAGE_URL        "https://isis.astrogeology.usgs.gov/")

# Retrieve a string describing the OS this is built on.
get_os_version(osVersionString)
message("Detected Operating System: ${osVersionString}")

#===============================================================================
#===============================================================================
# Configuration options

# All libraries are build as shared.  The main library is also built
#  as a static library using some specialized code in Utilities.cmake.
set(BUILD_SHARED_LIBS ON)

# make sure to leave rpaths untouched on install
set(CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE)

# Specify user options that can be passed in with the initial CMake command.
option(isisData        "Directory containing IsisData"                  OFF )
option(isisTestData    "Directory containing IsisTestData"              OFF )
option(testOutputDir   "Directory to store app test output folders"     OFF )
option(buildCore       "Build the core ISIS modules"                    ON  )
option(buildMissions   "Build the mission specific modules"             ON  )
option(buildStaticCore "Build libisis static as well as dynamic"        OFF )
option(buildTests      "Set up unit, application, and module tests."    ON  )
option(JP2KFLAG        "Whether or not to build using JPEG2000 support" OFF )
option(pybindings      "Turn on to build Python bindings"               OFF )

# if cmake install prefix is not set, and conda env is activated, use the
# conda env as the install directory.
if(DEFINED ENV{CONDA_PREFIX} AND CMAKE_INSTALL_PREFIX STREQUAL "/usr/local")
 set(CMAKE_INSTALL_PREFIX $ENV{CONDA_PREFIX})
endif()

# Prioritize passed in variables over env vars, probably a better way to do this
if(DEFINED ENV{ISISDATA} AND NOT isisData)
 set(isisData $ENV{ISISDATA})
endif()
if(DEFINED ENV{ISISTESTDATA} AND NOT isisTestData)
 set(isisTestData $ENV{ISISTESTDATA})
endif()

if(EXISTS ${isisData})
 set(ENV{ISISDATA} "${isisData}")
else()
 message(WARNING "Isis Data directory ${isisData} not found, unit tests will fail.")
 set(isisData OFF)
endif()

if(EXISTS ${isisTestData})
 set(ENV{ISISTESTDATA} "${isisTestData}")
else()
 message(WARNING "IsisTestData directory ${isisTestData} not found, application and module tests will fail.")
 set(isisTestData OFF)
endif()

if(${testOutputDir} STREQUAL "OFF")
 message("Writing test data folders to = ${CMAKE_BINARY_DIR}/testOutputDir")
 set(testOutputDir "${CMAKE_BINARY_DIR}/testOutputDir")
 execute_process(COMMAND mkdir -p ${CMAKE_BINARY_DIR}/testOutputDir)
else()
 # User specified a test output folder, make sure it exists.
 message("Writing test data folders to = ${testOutputDir}")
 execute_process(COMMAND mkdir -p ${testOutputDir})
endif()

# inject ISISROOT
add_definitions( -DISISROOT="${CMAKE_SOURCE_DIR}" )
add_definitions( -DISISBUILDDIR="${CMAKE_BINARY_DIR}" )

message("CONFIGURATION")
message("\tBUILD STATIC CORE: ${buildStaticCore}")
message("\tBUILD TESTS: ${buildTests}")
message("\tBUILD CORE: ${buildCore}")
message("\tBUILD MISSIONS: ${buildMissions}")
message("\tJP2K SUPPORT: ${JP2KFLAG}")
message("\tPYTHON BINDINGS: ${pybindings}")
message("\tISISDATA: ${isisData}")
message("\tISISTESTDATA: ${isisTestData}")
message("\tTEST OUTPUT DIR: ${testOutputDir}")
message("\tINSTALL PREFIX: ${CMAKE_INSTALL_PREFIX}")

#===============================================================================
#===============================================================================

# Set up the ctest tool which is used to run all of the tests.
enable_testing()
include(CTest)

# Set up Anaconda prefix in the case with a non-default conda env is activated
if(EXISTS $ENV{CONDA_PREFIX})
  message("CONDA PREFIX: $ENV{CONDA_PREFIX}")
  list(APPEND CMAKE_FIND_ROOT_PATH $ENV{CONDA_PREFIX}
                                   $ENV{CONDA_PREFIX}/lib/cmake/Qt5)
endif()

# options only allow on/off but this flag is piped into ISIS as ENABLEJP2K
# needs to be either 1 or 0 for C style true false
if(JP2KFLAG)
 set(JP2KFLAG 1)
endif()

# Set up the ctest tool which is used to run all of the tests.
enable_testing()
include(CTest)

# Specify flags used
# on linux, add the conda prefix to handle possible issues with rpaths at link time
# sometimes third parties do not set their rpaths correctly
set(thirdPartyCppFlags -Wall
                       -fPIC
                       -std=c++14
                       -DISIS_LITTLE_ENDIAN=1
                       -Wno-unused-parameter
                       -Wno-overloaded-virtual
                       -Wno-strict-aliasing
                       -DUSE_UNSTABLE_GEOS_CPP_API=1
                       -Wno-strict-overflow
                       -DENABLEJP2K=${JP2KFLAG}
                     )

set(CMAKE_CXX_STANDARD 14)

# Append CPP flags set in the third party lib file to the string set in this file.
string(REPLACE ";" " " FLAGS_STR "${thirdPartyCppFlags}")

set(CMAKE_CXX_FLAGS  "${CMAKE_CXX_FLAGS} ${FLAGS_STR}" )


# Flag to fix numeric literals problem with boost on linux
# Add gold linker (and therefore, phtread) to speed up linux (spec. Ubuntu18.04) builds
if(NOT APPLE)
  set(thirdPartyCppFlags ${thirdPartyCppFlags} -fuse-ld=gold
	                                       -pthread
					       -fext-numeric-literals
                                               -Wl,-rpath,$ENV{CONDA_PREFIX}/lib)
endif()

# Append CPP flags set in the third party lib file to the string set in this file.
string(REPLACE ";" " " FLAGS_STR "${thirdPartyCppFlags}")
set(CMAKE_CXX_FLAGS  "${CMAKE_CXX_FLAGS} ${FLAGS_STR}" )

if(NOT CMAKE_BUILD_TYPE)
   set(CMAKE_BUILD_TYPE "RELEASE")
endif()

if(APPLE)
  set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -g -O0")
else()
  set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -g -Og")
endif()

# Paths to required executables
find_program(XALAN Xalan REQUIRED)
find_program(LATEX latex)
find_program(DOXYGEN NAME doxygen PATH_SUFFIXES doxygen REQUIRED)
find_program(UIC uic REQUIRED)
find_program(MOC moc REQUIRED)
find_program(RCC rcc REQUIRED)
find_program(PROTOC protoc REQUIRED)

find_package(Qt5 COMPONENTS
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

find_package(Boost 1.59.0 REQUIRED atomic
                                        log
                                        regex
                                        log_setup
                                        serialization
                                        chrono
                                        math_c99
                                        container
                                        math_c99f
                                        context
                                        math_c99l
                                        system
                                        math_tr1
                                        math_tr1f
                                        math_tr1l
                                        test_exec_monitor
                                        date_time
                                        thread
                                        exception
                                        timer
                                        filesystem
                                        prg_exec_monitor
                                        program_options
                                        unit_test_framework
                                        iostreams
                                        wave
                                        random
                                        wserialization)

find_package(Ale               REQUIRED)
find_package(Json              REQUIRED)
find_package(Bullet    2.86    REQUIRED)
find_package(Cholmod   4.4.5   REQUIRED)
find_package(CSM       3.0.3.3 REQUIRED)
find_package(CSPICE    65      REQUIRED)
find_package(Eigen             REQUIRED)
find_package(Embree    2.15.0  REQUIRED)
find_package(GeoTIFF   2       REQUIRED)
SET(GSL_ROOT_DIR $ENV{CONDA_PREFIX})
find_package(GSL       2.2.1   REQUIRED)
find_package(HDF5      1.8.15  REQUIRED)
find_package(Jama      125     REQUIRED)
find_package(NN                REQUIRED)
find_package(OpenCV    3.1.0   REQUIRED)
find_package(PCL               REQUIRED)
find_package(Protobuf  2.6.1   REQUIRED)
find_package(Qwt       6       REQUIRED)
find_package(SuperLU   4.3     REQUIRED)
find_package(TIFF      4.0.0   REQUIRED)
find_package(TNT       126     REQUIRED)
find_package(XercesC   3.1.2   REQUIRED)
find_package(X11       6       REQUIRED)
find_package(nanoflann         REQUIRED)
find_package(PNG               REQUIRED)
find_package(Kakadu)
find_package(Geos    3.5.0     REQUIRED)
find_package(Armadillo         REQUIRED)
find_package(Threads)
find_package(inja              REQUIRED)



# In this case, we specify the version numbers being searched for in the non-traditional installs.
if(APPLE)
  find_package(OpenGL REQUIRED)
endif(APPLE)

if(pybindings)
 find_package(Python REQUIRED)
endif()

# Iterate through all variables and extract the libraries and include directories
get_cmake_property(_variableNames VARIABLES) # Get All VARIABLES

set(ALLLIBDIRS "")
set(ALLLIBS "")
set(ALLINCDIRS "")

# Get all include dir variables
foreach (_variableName ${_variableNames})
  if (_variableName MATCHES ".+_INCLUDE_DIR$")
    list(APPEND ALLINCDIRS "${${_variableName}}")
  elseif (_variableName MATCHES ".+_INCLUDE_PATH$")
    list(APPEND ALLINCDIRS "${${_variableName}}")
  endif()
endforeach()

# get all Library variables
foreach (_variableName ${_variableNames})
  get_filename_component(LIBDIR "${${_variableName}}" DIRECTORY)
  get_filename_component(LIBNAME "${${_variableName}}" NAME)

  if (_variableName MATCHES "^CMAKE+")
  elseif (_variableName MATCHES ".+_LIBRARY$|.+_LIBRARIES")
    foreach(ITEM IN LISTS ${_variableName})
      get_filename_component(LIBDIR ${ITEM} DIRECTORY)
      get_filename_component(LIBNAME ${ITEM} NAME)
      # Some library dependencies identified by the find_package commands are not
      # libraries (i.e., *.so,*.dylib,*.a) or name spaces (e.g., Qt5::Xml).
      # Remove the ones that are not libraries or name spaces  (e.g., OPENGL.framework).
      if(LIBNAME MATCHES ".+\.so$|.+\.dylib$|.+\.a$|.+::.+$|.+\.SO$|.+\.DYLIB$|.+\.A$")
        # For MacOS, .so's are not the same as .dylib's, need to add the full path to the .so
        if((APPLE) AND (LIBNAME MATCHES ".+\.so$|.+\.SO"))
          list(APPEND ALLLIBS "${ITEM}")
        else()
          list(APPEND ALLLIBDIRS ${LIBDIR})
          list(APPEND ALLLIBS ${LIBNAME})
        endif()
      endif()
    endforeach()
  endif()
endforeach()

# add target based linkages to ALLLIBS variable
list(APPEND ALLLIBS pantor::inja)

# Sometimes we add the same lib more than once (especially with LIBDIRS)
list(REMOVE_DUPLICATES ALLLIBDIRS)
list(REMOVE_DUPLICATES ALLLIBS)
list(REMOVE_DUPLICATES ALLINCDIRS)

#===============================================================================
#===============================================================================

# Start setting up the build
# Add extension to find fortran until .so symlink can be added to /usr/lib64
list(APPEND CMAKE_FIND_LIBRARY_SUFFIXES .so.3 .so.6 .so.5)

# Allow everything to include the 3rd party libraries
include_directories(SYSTEM ${ALLINCDIRS})
link_directories(${ALLLIBDIRS})

include_directories(${CMAKE_BINARY_DIR}/inc)
set(CORE_LIB_NAME isis)
message(STATUS "CORE LIB:  ${CORE_LIB_NAME}")


# Specify relative library include paths which will be set up on
#  the installed files.
if(APPLE)
  set(CMAKE_INSTALL_RPATH "${CMAKE_INSTALL_RPATH};@loader_path/../lib;@loader_path/../3rdParty/lib")
else()
  set(CMAKE_INSTALL_RPATH "${CMAKE_INSTALL_RPATH};$ORIGIN/../lib;$ORIGIN/../3rdParty/lib")
endif()

# We will set up some links with these files at the end of the install process so
#  make sure they are cleared at the start of the install process.
install(CODE "EXECUTE_PROCESS(COMMAND rm -f ${CMAKE_INSTALL_PREFIX}/lib/libisis${VERSION}${SO})")
install(CODE "EXECUTE_PROCESS(COMMAND rm -f ${CMAKE_INSTALL_PREFIX}/lib/libisis${VERSION_MAJOR}.${VERSION_MINOR}${SO})")
install(CODE "EXECUTE_PROCESS(COMMAND rm -f ${CMAKE_INSTALL_PREFIX}/lib/libisis${VERSION_MAJOR}${SO})")
EXECUTE_PROCESS(COMMAND cp -f ${CMAKE_SOURCE_DIR}/TestPreferences ${CMAKE_BINARY_DIR}/)
install(CODE "EXECUTE_PROCESS(COMMAND cp -f ${CMAKE_SOURCE_DIR}/IsisPreferences ${CMAKE_INSTALL_PREFIX}/)")

# Install the header files
install(CODE "EXECUTE_PROCESS(COMMAND mkdir -p ${CMAKE_INSTALL_PREFIX}/include/isis)")
file(GLOB ISIS_INCLUDE_FILES ${CMAKE_BINARY_DIR}/inc/*)
install(DIRECTORY ${CMAKE_BINARY_DIR}/inc/ DESTINATION  ${CMAKE_INSTALL_PREFIX}/include/isis)

# Delete any existing plugin files in the build folder so they
#  don't get filled with duplicate entries.
file(GLOB existingPlugins "${CMAKE_BINARY_DIR}/plugins/*.plugin")
if(existingPlugins)
  file(REMOVE ${existingPlugins})
endif()

# Add a config file to the install bin directory so that QT can find the plugin libraries.
file(WRITE "${CMAKE_BINARY_DIR}/qt.conf" "[Paths]\nPlugins=../3rdParty/plugins/\n")
install(FILES "${CMAKE_BINARY_DIR}/qt.conf" DESTINATION ${CMAKE_INSTALL_PREFIX}/bin/xml)

# Create the inc directory
execute_process(COMMAND mkdir -p ${CMAKE_BINARY_DIR}/inc)

# Create an xml folder in the source directory that we will need later
set(sourceXmlFolder ${CMAKE_BINARY_DIR}/bin/xml)
execute_process(COMMAND mkdir -p ${CMAKE_BINARY_DIR}/bin/xml)

# Create the version file
configure_file(${CMAKE_SOURCE_DIR}/cmake/version.in ${CMAKE_BINARY_DIR}/version)

# Set up install of the make folder.
install(DIRECTORY ${CMAKE_SOURCE_DIR}/make DESTINATION ${CMAKE_INSTALL_PREFIX})

# Set up install of appdata folder
install(DIRECTORY ${CMAKE_BINARY_DIR}/appdata DESTINATION ${CMAKE_INSTALL_PREFIX})

# Create the appdata directory with translations directory, images directory, templates directories
file(MAKE_DIRECTORY ${CMAKE_BINARY_DIR}/appdata/translations
                    ${CMAKE_BINARY_DIR}/appdata/templates/apollo
        ${CMAKE_BINARY_DIR}/appdata/templates/autoseed
        ${CMAKE_BINARY_DIR}/appdata/templates/autoreg
        ${CMAKE_BINARY_DIR}/appdata/templates/noproj
        ${CMAKE_BINARY_DIR}/appdata/templates/cnetref
        ${CMAKE_BINARY_DIR}/appdata/templates/cnetstats
        ${CMAKE_BINARY_DIR}/appdata/templates/cnet_validmeasure
        ${CMAKE_BINARY_DIR}/appdata/templates/controlnetworks
        ${CMAKE_BINARY_DIR}/appdata/export
        ${CMAKE_BINARY_DIR}/appdata/templates/hidtmgen
        ${CMAKE_BINARY_DIR}/appdata/import
        ${CMAKE_BINARY_DIR}/appdata/templates/findfeatures
        ${CMAKE_BINARY_DIR}/appdata/templates/fullcnetdiff
        ${CMAKE_BINARY_DIR}/appdata/templates/jigsaw
        ${CMAKE_BINARY_DIR}/appdata/templates/kernels
        ${CMAKE_BINARY_DIR}/appdata/templates/labels
        ${CMAKE_BINARY_DIR}/appdata/templates/maps
        ${CMAKE_BINARY_DIR}/appdata/templates/photometry
        ${CMAKE_BINARY_DIR}/appdata/images/icons
        ${CMAKE_BINARY_DIR}/appdata/images/targets)


# Have CMake process all of the source code and tests.
add_subdirectory(src objects)

if(APPLE)
 set(SO ".dylib")
else()
 set(SO ".so")
endif()

# Set up documentation build target.
# - This script is called by running "ninja docs".
# - This long call passes all desired variables to the script.
add_custom_target(docs COMMAND ${CMAKE_COMMAND}
                  -DPROJECT_SOURCE_DIR=${PROJECT_SOURCE_DIR}
                  -DDOXYGEN=${DOXYGEN}  -DXALAN=${XALAN}
                  -DLATEX=${LATEX}
                  -DCMAKE_INSTALL_PREFIX=${CMAKE_INSTALL_PREFIX}
                  -P ${CMAKE_MODULE_PATH}/BuildDocs.cmake)

# Add custom build target to copy modified header files to the build/incs directory.
# ALL is specified so that the target is added to the default build target, i.e. the copy command
# will be executed when running "ninja install"
# On a clean build, all files will be copied over.
add_custom_target(incs ALL COMMAND ${CMAKE_COMMAND} -E copy_if_different
  ${CMAKE_SOURCE_DIR}/src/*/objs/*/*.h ${CMAKE_SOURCE_DIR}/src/*/objs/*/*.hpp
  ${CMAKE_SOURCE_DIR}/src/*/apps/*/*.h ${CMAKE_BINARY_DIR}/inc)
add_dependencies(isis incs)

# Add custom build target to copy modified translation files to the build/appdata/translations
## directory.
add_custom_target(appdata ALL COMMAND ${CMAKE_COMMAND} -E copy_if_different
  ${CMAKE_SOURCE_DIR}/appdata/serialnumbers/*.trn ${CMAKE_SOURCE_DIR}/appdata/translations/*.trn
  ${CMAKE_SOURCE_DIR}/appdata/translations/*.typ ${CMAKE_SOURCE_DIR}/src/*/apps/*/*.typ
  ${CMAKE_SOURCE_DIR}/src/*/apps/*/*.trn  ${CMAKE_SOURCE_DIR}/src/*/apps/*/*.pft
  ${CMAKE_SOURCE_DIR}/src/*/apps/*/*.def ${CMAKE_SOURCE_DIR}/appdata/translations/*.def
  ${CMAKE_SOURCE_DIR}/appdata/translations/*.pvl
  ${CMAKE_BINARY_DIR}/appdata/translations)
add_dependencies(isis appdata)

# Add custom build targets to copy modified template files to build/appdata/templates
add_custom_target(apollotemplate ALL COMMAND ${CMAKE_COMMAND} -E copy_if_different
  ${CMAKE_SOURCE_DIR}/src/*/templates/*.def ${CMAKE_BINARY_DIR}/appdata/templates/apollo/)
add_dependencies(isis apollotemplate)

add_custom_target(autoreg ALL COMMAND ${CMAKE_COMMAND} -E copy_if_different
  ${CMAKE_SOURCE_DIR}/appdata/templates/autoreg/* ${CMAKE_BINARY_DIR}/appdata/templates/autoreg/)
add_dependencies(isis autoreg)

add_custom_target(autoseed ALL COMMAND ${CMAKE_COMMAND} -E copy_if_different
  ${CMAKE_SOURCE_DIR}/appdata/templates/autoseed/* ${CMAKE_BINARY_DIR}/appdata/templates/autoseed/)

add_custom_target(noproj ALL COMMAND ${CMAKE_COMMAND} -E copy_if_different
  ${CMAKE_SOURCE_DIR}/appdata/templates/noproj/* ${CMAKE_BINARY_DIR}/appdata/templates/noproj/)
add_dependencies(isis noproj)

add_custom_target(cnetref ALL COMMAND ${CMAKE_COMMAND} -E copy_if_different
  ${CMAKE_SOURCE_DIR}/appdata/templates/cnetref/* ${CMAKE_BINARY_DIR}/appdata/templates/cnetref/)
add_dependencies(isis cnetref)

add_custom_target(cnetstats ALL COMMAND ${CMAKE_COMMAND} -E copy_if_different
  ${CMAKE_SOURCE_DIR}/appdata/templates/cnetstats/* ${CMAKE_BINARY_DIR}/appdata/templates/cnetstats/)
add_dependencies(isis cnetstats)

add_custom_target(cnetvalid ALL COMMAND ${CMAKE_COMMAND} -E copy_if_different
  ${CMAKE_SOURCE_DIR}/appdata/templates/cnet_validmeasure/* ${CMAKE_BINARY_DIR}/appdata/templates/cnet_validmeasure/)
add_dependencies(isis cnetvalid)

add_custom_target(controlnetworks ALL COMMAND ${CMAKE_COMMAND} -E copy_if_different
  ${CMAKE_SOURCE_DIR}/appdata/templates/controlnetworks/* ${CMAKE_BINARY_DIR}/appdata/templates/controlnetworks/)
add_dependencies(isis controlnetworks)

add_custom_target(findfeatures ALL COMMAND ${CMAKE_COMMAND} -E copy_if_different
  ${CMAKE_SOURCE_DIR}/appdata/templates/findfeatures/* ${CMAKE_BINARY_DIR}/appdata/templates/findfeatures/)
add_dependencies(isis findfeatures)

add_custom_target(fullcnetdiff ALL COMMAND ${CMAKE_COMMAND} -E copy_if_different
  ${CMAKE_SOURCE_DIR}/appdata/templates/fullcnetdiff/* ${CMAKE_BINARY_DIR}/appdata/templates/fullcnetdiff/)
add_dependencies(isis fullcnetdiff)

add_custom_target(jigsaw ALL COMMAND ${CMAKE_COMMAND} -E copy_if_different
  ${CMAKE_SOURCE_DIR}/appdata/templates/jigsaw/* ${CMAKE_BINARY_DIR}/appdata/templates/jigsaw/)
add_dependencies(isis jigsaw)

add_custom_target(kernels ALL COMMAND ${CMAKE_COMMAND} -E copy_if_different
  ${CMAKE_SOURCE_DIR}/appdata/templates/kernels/* ${CMAKE_BINARY_DIR}/appdata/templates/kernels/)
add_dependencies(isis kernels)

add_custom_target(photometry ALL COMMAND ${CMAKE_COMMAND} -E copy_if_different
  ${CMAKE_SOURCE_DIR}/appdata/templates/photometry/* ${CMAKE_BINARY_DIR}/appdata/templates/photometry/)
add_dependencies(isis photometry)

add_custom_target(icons ALL COMMAND ${CMAKE_COMMAND} -E copy_if_different
  ${CMAKE_SOURCE_DIR}/appdata/images/icons/* ${CMAKE_SOURCE_DIR}/appdata/images/icons/licenses/*
  ${CMAKE_BINARY_DIR}/appdata/images/icons)
add_dependencies(isis icons)

add_custom_target(targets ALL COMMAND ${CMAKE_COMMAND} -E copy_if_different
  ${CMAKE_SOURCE_DIR}/appdata/images/targets/* ${CMAKE_BINARY_DIR}/appdata/images/targets)
add_dependencies(isis targets)

add_custom_target(maps ALL COMMAND ${CMAKE_COMMAND} -E copy_if_different
  ${CMAKE_SOURCE_DIR}/appdata/templates/maps/* ${CMAKE_BINARY_DIR}/appdata/templates/maps)
add_dependencies(isis maps)

add_custom_target(labels ALL COMMAND ${CMAKE_COMMAND} -E copy_if_different
  ${CMAKE_SOURCE_DIR}/appdata/templates/labels/* ${CMAKE_BINARY_DIR}/appdata/templates/labels)
add_dependencies(isis labels)

add_custom_target(hidtmgen ALL COMMAND ${CMAKE_COMMAND} -E copy_if_different
  ${CMAKE_SOURCE_DIR}/appdata/templates/hidtmgen/* ${CMAKE_BINARY_DIR}/appdata/templates/hidtmgen/)
add_dependencies(isis hidtmgen)

add_custom_target(import ALL COMMAND ${CMAKE_COMMAND} -E copy_if_different
  ${CMAKE_SOURCE_DIR}/appdata/import/* ${CMAKE_BINARY_DIR}/appdata/import)
add_dependencies(isis import)

add_custom_target(export ALL COMMAND ${CMAKE_COMMAND} -E copy_if_different
  ${CMAKE_SOURCE_DIR}/appdata/export/* ${CMAKE_BINARY_DIR}/appdata/export)
add_dependencies(isis export)

# Add a custom build target to clean out everything that gets added to the source
#  directory during the build process.
# - Only a few things are added in order to make the tests work properly so
#   this is very straightforward.
add_custom_target(clean_source COMMAND rm -rf "${CMAKE_BINARY_DIR}/*" "${CMAKE_INSTALL_PREFIX}/*")

# Set up a few top level files for installation.
EXECUTE_PROCESS(COMMAND cp -f  ${CMAKE_SOURCE_DIR}/IsisPreferences   ${CMAKE_BINARY_DIR})
EXECUTE_PROCESS(COMMAND cp -rf ${CMAKE_SOURCE_DIR}/scripts           ${CMAKE_BINARY_DIR})
EXECUTE_PROCESS(COMMAND cp -f  ${CMAKE_SOURCE_DIR}/../LICENSE.md    ${CMAKE_BINARY_DIR})
EXECUTE_PROCESS(COMMAND cp -rf ${CMAKE_SOURCE_DIR}/make              ${CMAKE_BINARY_DIR})

# Copy the files on make install as well
install(FILES     ${CMAKE_SOURCE_DIR}/IsisPreferences DESTINATION  ${CMAKE_INSTALL_PREFIX})
install(FILES     ${CMAKE_SOURCE_DIR}/../LICENSE.md   DESTINATION  ${CMAKE_INSTALL_PREFIX})
install(FILES     ${CMAKE_SOURCE_DIR}/../AUTHORS.rst  DESTINATION  ${CMAKE_INSTALL_PREFIX})
install(FILES     ${CMAKE_SOURCE_DIR}/../CHANGELOG.md DESTINATION  ${CMAKE_INSTALL_PREFIX})
install(FILES     ${CMAKE_BINARY_DIR}/version         DESTINATION  ${CMAKE_INSTALL_PREFIX})
install(DIRECTORY ${CMAKE_SOURCE_DIR}/scripts         DESTINATION  ${CMAKE_INSTALL_PREFIX})

# Trigger all post-install behavior.
# - The only way to run commands post-install in CMake is to add a subdirectory at
#   the end of this file containing a CMakeLists.txt file which includes all of
#   the desired post-install commands inside.
add_subdirectory(cmake)
option (BUILD_TESTS "Build tests" ON)
if(BUILD_TESTS)
  include(CTest)
  enable_testing()
  add_subdirectory(tests)
endif()
