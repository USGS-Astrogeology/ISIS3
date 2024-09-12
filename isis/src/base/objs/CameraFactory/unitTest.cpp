/** This is free and unencumbered software released into the public domain.
The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */
#include <iostream>
#include "IException.h"
#include "CameraFactory.h"
#include "Cube.h"
#include "FileName.h"
#include "Pvl.h"
#include "Preference.h"

using namespace Isis;
using namespace std;

void doit(Cube &cube);

int main(int argc, char *argv[]) {
  Preference::Preferences(true);

  // Get system's Camera.plugin file and add two new cameras of the same name to the bottom of
  // Camera.plugin with different version numbers
  Pvl pluginFile;
  PvlGroup oldCamera = PvlGroup("BOGUS/BOGUS");
  PvlGroup newCamera = PvlGroup("BOGUS/BOGUS");
  oldCamera += PvlKeyword("Version", "1");
  newCamera += PvlKeyword("Version", "2");
  pluginFile.addGroup(oldCamera);
  pluginFile.addGroup(newCamera);
  pluginFile.write("Camera.plugin");

  cerr << "Unit test for CameraFactory" << endl;
  cerr << "Testing missing Instrument Group ..." << endl;
  Cube dummyCube("$ISISTESTDATA/isis/src/base/unitTestData/isisTruth.cub", "r");
  Pvl &lab = *dummyCube.label();
  doit(dummyCube);

  lab.addGroup(PvlGroup("Kernels"));

  cerr << "Testing missing spacecraft name ..." << endl;
  lab.addGroup(PvlGroup("Instrument"));
  doit(dummyCube);

  cerr << "Testing missing instrument id ..." << endl;
  PvlGroup &inst = lab.findGroup("Instrument");
  inst += PvlKeyword("SpacecraftName", "Bogus Spacecraft");
  doit(dummyCube);

  cerr << "Testing unsupported camera mode ..." << endl;
  inst += PvlKeyword("InstrumentId", "Bogus Instrument");
  doit(dummyCube);

  // Create a new label from the same cube and add keywords to match cameras added to the plugin
  // file above
  cerr << "Testing that newest camera version is read from plugin ..." << endl;
  Cube dummyCube2("$ISISTESTDATA/isis/src/base/unitTestData/isisTruth.cub", "r");
  Pvl &lab2 = *dummyCube2.label();
  lab2.addGroup(PvlGroup("Kernels"));
  lab2.addGroup(PvlGroup("Instrument"));
  PvlGroup &inst2 = lab2.findGroup("Instrument");
  inst2 += PvlKeyword("SpacecraftName", "Bogus");
  inst2 += PvlKeyword("InstrumentId", "Bogus");
  cout << "(Expecting Version 2)" << endl;
  doit(dummyCube2);
  remove("Camera.plugin");
}

void doit(Cube &cube) {
  try {
    cerr << "Version: ";
    cerr << CameraFactory::CameraVersion(cube) << endl;
  }
  catch(IException &error) {
    IString errorStr = error.toString();
    errorStr = errorStr.replace(QRegExp(" in file.*"), ".");
    cerr << errorStr << endl;
  }

  cerr << endl;

  try {
    CameraFactory::Create(cube);
  }
  catch(IException &error) {
    IString errorStr = error.toString();
    errorStr = errorStr.replace(QRegExp(" in file.*"), ".");
    cerr << errorStr << endl;
  }

  cerr << endl;
}
