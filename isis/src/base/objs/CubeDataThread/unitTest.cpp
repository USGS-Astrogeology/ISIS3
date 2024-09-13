/** This is free and unencumbered software released into the public domain.
The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */
#include "Isis.h"

#include <iostream>
#include <unistd.h>

#include <QString>

#include "Cube.h"
#include "CubeDataThread.h"
#include "CubeDataThreadTester.h"
#include "FileName.h"
#include "ProgramLauncher.h"

using namespace Isis;

using namespace std;

void IsisMain() {
  QString testFile = QString::fromStdString(FileName("$ISISTESTDATA/isis/src/base/unitTestData/CubeDataThread/unitTest.cub").expanded());
  QString sysCommand = "cp " + testFile;
  ProgramLauncher::RunSystemCommand(sysCommand + " unitTest2.cub");
  ProgramLauncher::RunSystemCommand(sysCommand + " unitTest3.cub");

  CubeDataThread *cubeData = new CubeDataThread();

  int readOnly1 = cubeData->AddCube(FileName("$ISISTESTDATA/isis/src/base/unitTestData/isisTruth.cub"));
  int readOnly2 = cubeData->AddCube(FileName("$ISISTESTDATA/isis/src/base/unitTestData/blobTruth.cub"));
  int readWrite1 = cubeData->AddCube(FileName("./unitTest2.cub"));
  int readWrite2 = cubeData->AddCube(FileName("./unitTest3.cub"));

  Cube someCube;
  cubeData->AddCube(&someCube);

  cout << "Testing FindCubeId method :  " << cubeData->FindCubeId(&someCube)
       << endl << endl;

  CubeDataThreadTester *tester = new CubeDataThreadTester(cubeData);
  tester->Connect();

  // Basic Read Test (increments NumberOfTestsDone() once)
  tester->ReadCubeTest(readOnly1);

  // Wait for test 1 complete
  while(cubeData->BricksInMemory() != 0 ||
        tester->NumberOfTestsDone() != 1) {
    QThread::yieldCurrentThread();
  }
  // Test 1 is complete

  cout << endl << endl;

  // Simultaneous Read Test (increments NumberOfTestsDone() twice)
  tester->ReadCubeTest2(readOnly1, readOnly2);

  // Wait for test 2 complete
  while(cubeData->BricksInMemory() != 0 ||
        tester->NumberOfTestsDone() != 3) {
    QThread::yieldCurrentThread();
  }
  // Test 2 is complete

  cout << endl << endl;

  // Overlapping Read Test (increments NumberOfTestsDone() twice)
  tester->ReadCubeTest3(readOnly1);

  // Wait for test 3 complete
  while(cubeData->BricksInMemory() != 0 ||
        tester->NumberOfTestsDone() != 5) {
    QThread::yieldCurrentThread();
  }
  // Test 3 is complete

  cout << endl << endl;

  // Basic Write Test (increments NumberOfTestsDone() twice)
  tester->WriteCubeTest(readWrite1);

  // Wait for test 4 complete
  while(cubeData->BricksInMemory() != 0 ||
        tester->NumberOfTestsDone() != 7) {
    QThread::yieldCurrentThread();
  }
  // Test 4 is complete

  cout << endl << endl;

  // Simultaneous Write Test (increments NumberOfTestsDone() twice)
  tester->WriteCubeTest2(readWrite1, readWrite2);

  // Wait for test 5 complete
  while(cubeData->BricksInMemory() != 0 ||
        tester->NumberOfTestsDone() != 9) {
    QThread::yieldCurrentThread();
  }
  // Test 5 is complete

  cout << endl << endl;

  // Conflicting Write Test -- deadlocks (increments NumberOfTestsDone() once,
  //   deadlocks, then again)
  tester->WriteCubeTest3(readWrite1);

  // Wait for test 6 deadlock
  while(cubeData->BricksInMemory() != 1 ||
        tester->NumberOfTestsDone() != 10) {
    QThread::yieldCurrentThread();
  }
  // Wait test 6 is deadlocked

  tester->WriteCubeTest3BreakDeadlock();

  // Wait for test 6 complete
  while(cubeData->BricksInMemory() != 0 ||
        tester->NumberOfTestsDone() != 11) {
    QThread::yieldCurrentThread();
  }
  // Test 6 is complete

  cout << endl << endl;

  // Tests the BrickChanged signal (increments NumberOfTestsDone() twice)
  tester->NotifyChangeTest(readWrite1);

  // Wait for test 7 complete
  while(cubeData->BricksInMemory() != 0 ||
        tester->NumberOfTestsDone() != 13) {
    QThread::yieldCurrentThread();
  }
  // Test 7 is complete

  // Create a deadlock

  cout << endl << endl << "Creating Deadlock then trying to remove cubes"
       << endl;
  tester->WriteCubeTest3(readWrite1);

  // Wait for test deadlock
  while(cubeData->BricksInMemory() != 1 ||
        tester->NumberOfTestsDone() != 14) {
    QThread::yieldCurrentThread();
  }

  try {
   cubeData->RemoveCube(readWrite1);
   cout << "Remove cube didn't throw an exception, PROBLEM!" << endl;
  }
  catch(IException &e) {
    e.print();
  }

  try {
   cubeData->RemoveCube(-1);
   cout << "Remove cube didn't throw an exception, PROBLEM!" << endl;
  }
  catch(IException &e) {
    e.print();
  }

  try {
   cubeData->RemoveCube(readOnly1);
   cout << "Remove cube succeeded" << endl;
  }
  catch(IException &e) {
    e.print();
  }

  cout << endl << endl << "Deleting CubeDataThread with allocated bricks..."
       << endl;
  delete cubeData;

  cout << endl << endl << "Cleanup Tester" << endl;
  delete tester;

  cout << "Deleting Temporary R/W Cubes" << endl;
  remove("unitTest2.cub");
  remove("unitTest3.cub");

  cout << "Unit Test Complete" << endl;
}


