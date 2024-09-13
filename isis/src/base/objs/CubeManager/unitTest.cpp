/** This is free and unencumbered software released into the public domain.
The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */
#include "CubeManager.h"
#include "FileName.h"
#include "Cube.h"
#include "Preference.h"

#include <iostream>

#include <QVector>

using namespace std;
using namespace Isis;

int main(int argc, char *argv[]) {
  Isis::Preference::Preferences(true);

  cout << "CubeManager Unit Test" << endl;
  QVector<Cube *> cubes;

  // Read Cubes Into Memory
  cubes.push_back(CubeManager::Open("$ISISTESTDATA/isis/src/base/unitTestData/isisTruth.cub"));
  cubes.push_back(CubeManager::Open("$ISISTESTDATA/isis/src/base/unitTestData/isisTruth.cub"));
  cubes.push_back(CubeManager::Open("$ISISTESTDATA/isis/src/base/unitTestData/blobTruth.cub"));
  cubes.push_back(CubeManager::Open("$ISISTESTDATA/isis/src/base/unitTestData/blobTruth.cub"));
  cubes.push_back(CubeManager::Open("$ISISTESTDATA/isis/src/base/unitTestData/isisTruth.cub+1"));
  cubes.push_back(CubeManager::Open("$ISISTESTDATA/isis/src/base/unitTestData/isisTruth.cub+1"));

  // Print Cube FileNames To Verify We Have Correct Ones
  cout << "Verify proper cubes have been read" << endl;
  cout << "Cube FileNames: " << endl;

  for(int i = 0; i < (int)cubes.size(); i++) {
    cout << "  " << i + 1 << " : " << FileName(cubes[i]->fileName().toStdString()).baseName() << endl;
  }

  cout << endl;

  // Print Cube Pointer Comparisons (1 if match, 0 if differ)
  cout << "Verify We Don't Have Duplicates..." << endl;

  // Print Table Header... up to 9 (for formatting)
  cout << "  Cube #";
  for(int i = 0; i < 9 && i < (int)cubes.size(); i++) {
    cout << " | Equals " << i + 1;
  }
  cout << endl;

  // Print Comparison Table Data
  for(int i = 0; i < (int)cubes.size(); i++) {
    cout << "  Cube " << i + 1;

    for(int j = 0; j < (int)cubes.size(); j++) {
      cout << " |        " << (int)(cubes[i] == cubes[j]);
    }

    cout << endl;
  }

  cout << endl;

  // Test cube attributes presence, input attributes only affect bands
  cout << "Verify cube attributes have been taken into account" << endl;
  cout << "  Cube # | # of Bands" << endl;
  for(int i = 0; i < (int)cubes.size(); i++) {
    cout << "  Cube " << i + 1 << " | " << cubes[i]->bandCount() << endl;
  }
  cout << endl;

  // Test CleanUp() methods

  // Try cleaning up a cube that isn't managed by CubeManager
  CubeManager::CleanUp("unmanagedCube.cub");

  // Clean up one of the managed cubes
  CubeManager::CleanUp("$ISISTESTDATA/isis/src/base/unitTestData/blobTruth.cub");

  // Print Cube FileNames to verify that we have cleaned blobTruth cubes correctly
  cout << "Verify blobTruth cubes have been cleaned" << endl;
  cout << "Cube FileNames: " << endl;

  for (int i = 0; i < (int)cubes.size(); i++) {
    cout << "  " << i + 1 << " : ";
    // only isisTruth pointers are valid now, so let's verify those
    if (i < 2 or i > 3) {
      cout << FileName(cubes[i]->fileName().toStdString()).baseName() << endl;
    }
    else {
      cout << "(cleaned)" << endl;
    }
  }
  cout << endl;

  // Clean up remaining cubes
  CubeManager::CleanUp();

  // Create a CubeManager instance (implicitly test destructor)
  QVector<Cube *> cubes2;
  CubeManager mgr;
  cubes2.push_back(mgr.OpenCube("$ISISTESTDATA/isis/src/base/unitTestData/isisTruth.cub"));
  cubes2.push_back(mgr.OpenCube("$ISISTESTDATA/isis/src/base/unitTestData/blobTruth.cub"));

  // Test setting an opened cube limit
  mgr.SetNumOpenCubes(2);
  cout << "Set number of open cubes to 2" << endl;
  cout << endl;

  // Print Cube FileNames to verify the currently managed cubes
  cout << "Currently managed cubes:" << endl;
  for (int i = 0; i < (int)cubes2.size(); i++) {
    cout << "  " << i + 1 << " : ";
    cout << FileName(cubes2[i]->fileName().toStdString()).baseName() << endl;

  }
  cout << endl;

  // This will test the cleanup in OpenCube(const QString &)
  cubes2.push_back(mgr.OpenCube("$ISISTESTDATA/isis/src/base/unitTestData/isisTruth2.cub"));
  // we pop the front because OpenCube should dequeue and clean 1 item to enforce limit
  cubes2.pop_front();
  cout << "Opened isisTruth2.cub." << endl;
  cout << "Verify isisTruth2.cub is now managed and limit of 2 is enforced:" << endl;
  for (int i = 0; i < (int)cubes2.size(); i++) {
    cout << "  " << i + 1 << " : ";
    if (cubes2[i]) {
      cout << FileName(cubes2[i]->fileName().toStdString()).baseName() << endl;
    }
    else {
      cout << "(NULL)" << endl;
    }
  }
  cout << endl;

  // Cleanup
  mgr.CleanCubes();

  // Set a open cube limit that exceeds the system open file limit
  cout << "Setting number of open cubes > 60 percent of system open file limit" << endl;
  mgr.SetNumOpenCubes(100000);
  cout << endl;

  // Open a cube that DNE
  cout << "Attempting to open a file that does not exist:" << endl;
  try {
    mgr.OpenCube("dne.cub");
  } catch (IException &e) {
    cout << "  " << e.what() << endl;
  }

}
