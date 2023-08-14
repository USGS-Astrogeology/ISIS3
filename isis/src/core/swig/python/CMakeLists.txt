# Include python
include(FindPythonAnaconda.cmake)
include_directories(${PYTHON_INCLUDE_DIR})

set(CMAKE_SWIG_FLAGS "")
set(CMAKE_SWIG_OUTDIR isisio)
set_source_files_properties(../isisio.i
                            PROPERTIES CPLUSPLUS ON SWIG_FLAGS "-py3")

#Find dependencies
find_path(INCLUDE_DIR NAMES Pvl.h
                      PATH_SUFFIXES include
                      PATHS ${PROJECT_SOURCE_DIR}/)
message("-- Found Include: ${INCLUDE_DIR}")
include_directories(${INCLUDE_DIR})

# Add and link
swig_add_library(isisio
                 LANGUAGE python
                 SOURCES ../isisio.i)

set_target_properties(_isisio PROPERTIES LIBRARY_OUTPUT_DIRECTORY ${CMAKE_SWIG_OUTDIR})

SET(CMAKE_INSTALL_RPATH "${CMAKE_INSTALL_PREFIX}/lib")

if (APPLE)
     set_target_properties(_isisio PROPERTIES LINK_FLAGS "-undefined dynamic_lookup")
     # On OSX, do not link python - this causes segfaults
     swig_link_libraries(isisio core)
else()
     swig_link_libraries(isisio core ${PYTHON_LIBRARY})
endif()

# Build out a standard directory structure
# file(MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/tests)

# Move the static files
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/setup.py.in
               ${CMAKE_CURRENT_BINARY_DIR}/setup.py)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/__init__.py
              ${CMAKE_CURRENT_BINARY_DIR}/isisio/__init__.py
              COPYONLY)

## Move the tests
#file(COPY ${CMAKE_SOURCE_DIR}/python/tests
#     DESTINATION ${CMAKE_CURRENT_BINARY_DIR})
