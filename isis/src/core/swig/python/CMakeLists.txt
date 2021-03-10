# Include python
include(FindPythonAnaconda.cmake)
include_directories(${PYTHON_INCLUDE_DIR})

set(CMAKE_SWIG_FLAGS "")
set(CMAKE_SWIG_OUTDIR isispvl)
message(">>>> ${CMAKE_SWIG_OUTDIR}")
set_source_files_properties(../isispvl.i
                            PROPERTIES CPLUSPLUS ON)

#Find dependencies
message(">>>> ${PROJECT_SOURCE_DIR}")
message(">>>> ${PYTHON_LIBRARY}")
message(">>>> ${CMAKE_SOURCE_DIR}")
find_path(PVL_INCLUDE_DIR NAMES Pvl.h
                          PATH_SUFFIXES include
                          PATHS ${PROJECT_SOURCE_DIR}/)
find_library(PVL_LIBRARY NAMES core
            PATHS ${CMAKE_SOURCE_DIR}/../build/objects/core/)
message("-- Found PVL Include: ${PVL_INCLUDE_DIR}")
message("-- Found PVL Lib: ${PVL_LIBRARY}")
include_directories(${PVL_INCLUDE_DIR})

# Add and link
swig_add_library(isispvl
                 LANGUAGE python
                 SOURCES ../isispvl.i)

set_target_properties(_isispvl PROPERTIES LIBRARY_OUTPUT_DIRECTORY ${CMAKE_SWIG_OUTDIR})

SET(CMAKE_INSTALL_RPATH "${CMAKE_INSTALL_PREFIX}/lib")

if (APPLE)
     set_target_properties(_isispvl PROPERTIES LINK_FLAGS "-undefined dynamic_lookup")
     # On OSX, do not link python - this causes segfaults
     swig_link_libraries(isispvl ${PVL_LIBRARY})
else()
     swig_link_libraries(isispvl ${PVL_LIBRARY} ${PYTHON_LIBRARY})
endif()

# Build out a standard directory structure
# file(MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/tests)

# Move the static files
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/setup.py.in
               ${CMAKE_CURRENT_BINARY_DIR}/setup.py
               COPYONLY)
#configure_file(${CMAKE_CURRENT_SOURCE_DIR}/__init__.py
#               ${CMAKE_CURRENT_BINARY_DIR}/isispvl/__init__.py
#               COPYONLY)

## Move the tests
#file(COPY ${CMAKE_SOURCE_DIR}/python/tests
#     DESTINATION ${CMAKE_CURRENT_BINARY_DIR})
