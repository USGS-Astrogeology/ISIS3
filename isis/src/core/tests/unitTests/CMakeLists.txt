# List of classes with old ISIS unitTests
set(unitTestClasses "Blob"
                    "FileName"
                    "IException"
                    "IString"
                    "Message"
                    "Preference"
                    "Pvl"
                    "PvlContainer"
                    "PvlFormat"
                    "PvlFormatPds"
                    "PvlGroup"
                    "PvlSequence"
                    "TableField"
                    "TableRecord")

foreach (class ${unitTestClasses})
  set(testFile "${CMAKE_CURRENT_SOURCE_DIR}/${class}/unitTest.cpp")
  set(truthFile "${CMAKE_CURRENT_SOURCE_DIR}/${class}/${class}.truth")
  set(executableName "core_unit_test_${class}")

  # Create the test executable
  add_executable(${executableName} ${testFile})
  target_link_libraries(${executableName} core)

  # Set up a cmake script which will run the executable
  # and then check the results against the truth file.
  set(testScript "${PROJECT_SOURCE_DIR}/cmake/RunUnitTest.cmake")
  set(fullTestPath $<TARGET_FILE:${executableName}>)

  set(testName ${executableName})
  add_test(NAME ${testName}
           COMMAND ${CMAKE_COMMAND}
           -DTEST_PROG=${fullTestPath}
           -DTRUTH_FILE=${truthFile}
           -DDATA_ROOT=$ENV{ISISDATA}
           -DCODE_ROOT=${PROJECT_SOURCE_DIR}
           -P ${testScript})
  set_tests_properties(${testName} PROPERTIES LABELS "unit;core")
endforeach()
