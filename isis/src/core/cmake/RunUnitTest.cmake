#=========================================================================
# Script to run a basic unit test and compare the result to a truth file.
# - This replaces the ISIS UnitTester script.
#=========================================================================

# Multiple ISIS files need to find things relative to ISISROOT
#  and ISISDATA so make sure those are set.
set(ENV{ISISDATA} "${DATA_ROOT}")

message(STATUS "ISISROOT = $ENV{ISISROOT}")
message(STATUS "ISISDATA = $ENV{ISISDATA}")

# Set up a file for program output
set(outputFile "${TEST_PROG}.output")
message("outputFile = ${outputFile}")
file(REMOVE ${outputFile}) # Make sure no old file exists

# The test programs need to be run from their source folders
#  so that they can find input data files.
get_filename_component(truthFolder ${TRUTH_FILE} DIRECTORY)

# Test programs also need to be run with the EXACT name "unitTest",
#  otherwise a GUI will pop up and ruin the test.
get_filename_component(binFolder ${TEST_PROG} DIRECTORY)
get_filename_component(binName   ${TEST_PROG} NAME)
set(tempDir  ${binFolder}/${binName}_temp)
execute_process(COMMAND rm -rf ${tempDir})
execute_process(COMMAND mkdir -p ${tempDir})
execute_process(COMMAND ln -s ${TEST_PROG} ${truthFolder}/unitTest)

# Run the unit test executable and pipe the output to a text file.
execute_process(COMMAND  ./unitTest
                WORKING_DIRECTORY ${truthFolder}
                OUTPUT_FILE ${outputFile}
                ERROR_FILE ${outputFile}
                OUTPUT_VARIABLE result
                RESULT_VARIABLE code)
if(result)
    message("Test failed: ${result}, ${code}")
endif()

# If an exclusion file is present, use it to filter out selected lines.
# - Do this by comparing filtered versions of the two files, then
#   running the diff on those two temporary files.
set(comp1 ${outputFile})
set(comp2 ${TRUTH_FILE})
set(exclusionPath ${truthFolder}/unitTest.exclude)
if(EXISTS ${exclusionPath})
  set(comp1 ${tempDir}/output_exclude.txt)
  set(comp2 ${tempDir}/truth_exclude.txt)
  # This throws out all lines containing a word from the exclusion file.
  execute_process(COMMAND cat ${outputFile} COMMAND grep -v -f ${exclusionPath}
                  OUTPUT_FILE "${comp1}")
  execute_process(COMMAND cat ${TRUTH_FILE} COMMAND grep -v -f ${exclusionPath}
                  OUTPUT_FILE "${comp2}")

endif()

# Verify that the files are exactly the same
execute_process(COMMAND ${CMAKE_COMMAND} -E compare_files
    ${comp1} ${comp2}
    RESULT_VARIABLE DIFFERENT)

if(DIFFERENT)
    message("------------------ DIFFERENCES ------------------ ")
    execute_process(COMMAND diff ${comp1} ${comp2} OUTPUT_VARIABLE compdiff)
    message("${compdiff}")
    message("------------------------------------------------- ")
    message(FATAL_ERROR "Test failed - files differ")        
    # On error the result file is left around to aid in debugging.
else()
  file(REMOVE ${outputFile}) # On success, clean out the result file.
  execute_process(COMMAND rm -rf ${tempdir})
endif()

# Clean up our temporary folder
execute_process(COMMAND rm -f ${truthFolder}/unitTest)
