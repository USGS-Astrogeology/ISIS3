#===============================================================================
#        Functions to add ISIS modules to the CMake build
#===============================================================================

include(CodeGeneration)

# Incorporate an application folder
function(add_isis_app folder libDependencies)
  # The internal build name will be different than the output name
  # - This deals with problems compiling same-named targets on case-insensitive machines.
  get_filename_component(appName ${folder}  NAME)
  set(internalAppName ${appName}_app)

  # Get the main and xml files
  file(GLOB sources "${folder}/main.cpp")
  file(GLOB xmlFiles "${folder}/*.xml")

  # All the XML files need to be copied to the install directory
  # - They also need be put in the source folder for the app tests
  install(FILES ${xmlFiles} DESTINATION "${CMAKE_INSTALL_PREFIX}/bin/xml")
  if(NOT EXISTS ${CMAKE_BINARY_DIR}/bin/xml)
      execute_process(COMMAND mkdir ${CMAKE_BINARY_DIR}/bin/xml)
  endif()

  foreach(xml ${xmlFiles})
    get_filename_component(folder ${xml} DIRECTORY)
    get_filename_component(name ${folder} NAME)
    if(NOT EXISTS ${CMAKE_BINARY_DIR}/bin/xml/${name}.xml)
      execute_process(COMMAND ln -s "${xml}" "${name}.xml"
                      WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/bin/xml)
    endif()
  endforeach()

  # Generate required QT files
  generate_moc_files(mocFiles ${folder})
  set (CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

  # Set up the executable
  add_executable(${internalAppName} ${headers} ${sources} ${mocFiles})
  set_target_properties(${internalAppName} PROPERTIES LINKER_LANGUAGE CXX)

  # Have the app install with the real name, not the internal name.
  target_link_libraries(${internalAppName} ${libDependencies})

  set_target_properties(${internalAppName} PROPERTIES OUTPUT_NAME ${appName})
  install(TARGETS ${internalAppName} DESTINATION bin)

  if(${buildTests})
    # Set up the app tests
    # - There may be multiple test folders in the /tsts directory, each
    #   with its own Makefile describing the test.
    set(testFolder ${folder}/tsts)
    file(GLOB tests "${testFolder}/*")
    foreach(f ${tests})
      add_makefile_test_folder(${f} ${appName}_app)
    endforeach()
  endif()
endfunction(add_isis_app)


# Set up the lone unit test in an obj folder
function(make_obj_unit_test moduleName testFile truthFile reqLibs pluginLibs)
  if(${testFile} STREQUAL "NOTFOUND")
    # Skip if no unitest
    return()
  endif()
  # Get the object name (last folder part)
  get_filename_component(folder ${testFile} DIRECTORY)
  get_filename_component(filename ${folder} NAME)

  # See if there are any plugin libraries that match the name
  # - If there are, we need to link to them!
  set(matchedLibs)
  foreach (f ${pluginLibs})
    if(${f} STREQUAL ${filename})
      set(matchedLibs ${f})
    endif()
  endforeach()

  # Generate a name for the executable
  set(executableName "${moduleName}_unit_test_${filename}")

  # Create the executable and link it to the module library
  add_executable( ${executableName} ${testFile})
  set(depLibs "${reqLibs};${matchedLibs}")
  target_link_libraries(${executableName} ${moduleName} ${depLibs})

  # Call function to add the test
  add_unit_test_target(${executableName} ${truthFile} ${moduleName})

endfunction(make_obj_unit_test)


# Incorporate a single obj folder
function(add_isis_obj folder reqLibs)

  get_filename_component(folderName ${folder} NAME)

  # Look inside this folder for include files

  # Find the source and header files
  file(GLOB headers "${folder}/*.h" "${folder}/*.hpp")
  # ignore app.cpp
  file(GLOB sources "${folder}/*.c" "${folder}/*.cpp")
  file(GLOB truths  "${folder}/*.truth")
  file(GLOB plugins "${folder}/*.plugin")

  list(REMOVE_ITEM sources "${folder}/main.cpp")

  # Generate protobuf, ui, and moc files if needed.
  generate_protobuf_files(protoFiles ${folder})
  generate_ui_files(uiFiles ${folder})
  generate_moc_files(mocFiles ${folder})

  # Don't include the unit test in the main source list
  set(unitTest ${folder}/unitTest.cpp)
  list(REMOVE_ITEM sources "${unitTest}")

  # Add the unit test file for this folder if it exists.
  if(EXISTS "${unitTest}")
    set(thisTestFiles ${unitTest})
  else()
    set(thisTestFiles)
  endif()

  set(thisSourceFiles ${headers} ${sources} ${protoFiles} ${uiFiles} ${mocFiles})
  set(thisTruthFiles  ${truths} )

  # If there are multiple truth files, select based on the OS.
  list(LENGTH thisTestFiles numTest)
  list(LENGTH thisTruthFiles numTruth)
  if(((${numTruth} EQUAL 0) OR (${numTest} EQUAL 0)) AND NOT (${numTruth} EQUAL ${numTest}))
    message( FATAL_ERROR "Error: Found different number of truth {${numTruth}} and test {${numTest}} files for ${folderName}!\n"
                         "There should be at least one object.truth file and one unittest.cpp file." )
  endif()

  if(${numTruth} GREATER ${numTest})

    # Look for a truth file that contains the OS string
    set(matchedTruth "NONE")
    foreach(truthFile ${thisTruthFiles})

      # If the truth file contains the OS string, use it.
      string(FIND ${truthFile} ${osVersionString} position)
      if(NOT ${position} EQUAL -1)
        set(matchedTruth ${truthFile})
        break()
      endif()

    endforeach()

    # If no OS matched, use the default truth file.
    if(${matchedTruth} STREQUAL "NONE")
      set(matchedTruth "${folder}/${folderName}.truth")
    endif()
    set(thisTruthFiles ${matchedTruth})
  endif()

  # Always pass the test and truth files to the caller
  set(newTestFiles   ${thisTestFiles}   PARENT_SCOPE)
  set(newTruthFiles  ${thisTruthFiles}  PARENT_SCOPE)

  list(LENGTH plugins numPlugins)
  if(${numPlugins} EQUAL 0)
    # No plugins, pass the source files back to the caller to add to the larger library.
    set(newSourceFiles ${thisSourceFiles} PARENT_SCOPE)
  else()
    # Folder with a plugin means that this is a separate library!
    # Add it here and then we are done with the source files.

    set(newSourceFiles ${thisSourceFiles} PARENT_SCOPE)
    if(NOT (${numPlugins} EQUAL 1))
      message( FATAL_ERROR "Error: Multiple plugins found in folder!" )
    endif()

    get_filename_component(libName    ${folder}  NAME)
    get_filename_component(pluginName ${plugins} NAME)
    message("Adding plugin library: ${libName}")

    add_library_wrapper(${libName} "${thisSourceFiles}" "${reqLibs}")

    # Append the plugin file to a single file in the build directory
    # where the .so files will be created.  During installation copy these
    # plugin files to the installation library folder.
    set(pluginPath ${CMAKE_BINARY_DIR}/lib/${pluginName})
    cat(${plugins} ${pluginPath})
    install(PROGRAMS ${pluginPath} DESTINATION ${CMAKE_INSTALL_PREFIX}/lib/)
    # Record this library name for the caller
    set(newPluginLib ${libName}  PARENT_SCOPE)
  endif()


endfunction(add_isis_obj)




# Adds an entire module folder.
# - This includes the "base" folder and all the mission specific folders.
# - Each call of this function generates one library.
function(add_isis_module name)

  # First argument is the module name.
  # Arguments after the first are the folders to look in.
  set(topFolders ${ARGN})

  message("Adding ISIS module with folders: ${topFolders}")

  set(objFolders)
  set(appFolders)
  set(tstFolders)
  foreach(f ${topFolders})

    # Folders: apps, lib, tests
    set(appsDir "${CMAKE_CURRENT_LIST_DIR}/${f}/apps")
    set(objsDir "${CMAKE_CURRENT_LIST_DIR}/${f}/objs")
    set(tstsDir "${CMAKE_CURRENT_LIST_DIR}/${f}/tsts")

    # Start with the objs folder
    get_subdirectory_list(${objsDir} thisObjFolders)
    get_subdirectory_list(${appsDir} thisAppFolders)
    get_subdirectory_list(${tstsDir} thisTstFolders)

    set(objFolders ${objFolders} ${thisObjFolders} ${thisAppFolders})
    set(appFolders ${appFolders} ${thisAppFolders})
    set(tstFolders ${tstFolders} ${thisTstFolders})

  endforeach()
  # Now that we have the library info, call function to add it to the build!
  # - Base module depends on 3rd party libs, other libs also depend on base.
  # - Only the base module gets both a static and shared library.
  if(${name} STREQUAL ${CORE_LIB_NAME})
    set(reqLibs "${ALLLIBS};core;${CMAKE_THREAD_LIBS_INIT}")
    set(alsoStatic ON)
  else()
    set(reqLibs "${CORE_LIB_NAME};${ALLLIBS};${CMAKE_THREAD_LIBS_INIT}")
    set(alsoStatic OFF)
  endif()

  set(sourceFiles)
  set(unitTestFiles)
  set(truthFiles)
  set(pluginLibs)
  foreach(f ${objFolders})
    set(newSourceFiles)
    set(newTestFiles)
    set(newTruthFiles)
    set(newPluginLib)
    add_isis_obj(${f} "${reqLibs}") # Library add function
    set(sourceFiles   ${sourceFiles}   ${newSourceFiles})
    set(unitTestFiles ${unitTestFiles} ${newTestFiles})
    set(truthFiles    ${truthFiles}    ${newTruthFiles})
    set(pluginLibs    ${pluginLibs}    ${newPluginLib})

  endforeach(f)

  # Some modules don't generate a library
  list(LENGTH sourceFiles temp)
  if (NOT ${temp} EQUAL 0)
    message("Adding library: ${name}")
    add_library_wrapper(${name} "${sourceFiles}" "${reqLibs}" ${alsoStatic})

    # Have the plugin libraries depend on the module library
    foreach(plug ${pluginLibs})
      target_link_libraries(${plug} ${name})
    endforeach()

    # For everything beyond the module library, require the module library.
    set(reqLibs "${reqLibs};${name}")

    if(${buildTests})
      set (CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/unitTest)

      # Now that the library is added, add all the unit tests for it.
      list(LENGTH unitTestFiles temp)
      math(EXPR numTests "${temp} - 1")
      foreach(val RANGE ${numTests})
        list(GET unitTestFiles ${val} testFile )
        list(GET truthFiles    ${val} truthFile)

        make_obj_unit_test(${name} ${testFile} ${truthFile} "${reqLibs}" "${pluginLibs}")
      endforeach()
    endif()

  endif()

  # Process the apps (core library always required)
  foreach(f ${appFolders})
    add_isis_app(${f} "${reqLibs}")
  endforeach()

  if(${buildTests})
    # Process the tests
    # - The test suite in qisis/SquishTests are not properly located or handled by this code!
    foreach(f ${tstFolders})
      add_makefile_test_folder(${f} ${name}_module)
    endforeach()
  endif()

endfunction(add_isis_module)
