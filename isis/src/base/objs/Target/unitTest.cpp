/** This is free and unencumbered software released into the public domain.
The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */
#include <iostream>
#include <iomanip>

#include "Cube.h"
#include "Distance.h"
#include "FileName.h"
#include "IException.h"
#include "iTime.h"
#include "Spice.h"
#include "Target.h"
#include "Preference.h"

using namespace Isis;
using namespace std;

void printRadiiGroupInfo(bool found, Pvl &label, PvlGroup &mappingGroup);

/**
 * UnitTest for Target class.
 * @author 2012-03-20 Debbie A. Cook
 *
 * @internal
 * @history 2009-03-23  Tracie Sucharski - Removed old keywords
 *          SpacecraftPosition and SpacecraftPointing with the corrected
 *          InstrumentPosition and InstrumentPointing.
 */
int main(int argc, char *argv[]) {
  Preference::Preferences(true);

  cout << setprecision(10);
  cout << "Unit test for Isis::Target" << endl;

  // This group should be included in the Target test
  PvlGroup inst1("Instrument");
  inst1 += PvlKeyword("TargetName", "Mars");
  PvlGroup inst2("Instrument");
  inst2 += PvlKeyword("TargetName", "Sky");
  PvlGroup inst3("Instrument");
  inst3 += PvlKeyword("TargetName", "Mard");

  // These are not part of the Target class funtionality except to test missing body code
  PvlGroup kern1("Kernels");
  FileName f("$ISISTESTDATA/isis/src/base/unitTestData/kernels");
  FileName f2("$base/dems");
  std::string dir = f.expanded() + "/";
  std::string dir2 = f2.expanded() + "/";
  kern1 += PvlKeyword("NaifFrameCode", Isis::toString(-94031));
  kern1 += PvlKeyword("LeapSecond", dir + "naif0007.tls");
  kern1 += PvlKeyword("SpacecraftClock", dir + "MGS_SCLKSCET.00045.tsc");
  kern1 += PvlKeyword("TargetPosition", dir + "de405.bsp");
  kern1 += PvlKeyword("TargetAttitudeShape", dir + "pck00006.tpc");
  kern1 += PvlKeyword("Instrument", dir + "mocSpiceUnitTest.ti");
  kern1 += PvlKeyword("InstrumentAddendum", dir + "mocAddendum.ti");
  kern1 += PvlKeyword("InstrumentPosition", dir + "moc.bsp");
  kern1 += PvlKeyword("InstrumentPointing", dir + "moc.bc");
  kern1 += PvlKeyword("Frame", "");

  PvlGroup kern2("Kernels");
  kern2 += PvlKeyword("NaifIkCode", Isis::toString(-94031));
  kern2 += PvlKeyword("LeapSecond", dir + "naif0007.tls");
  kern2 += PvlKeyword("SpacecraftClock", dir + "MGS_SCLKSCET.00045.tsc");
  kern2 += PvlKeyword("TargetPosition", dir + "de405.bsp");
  kern2 += PvlKeyword("TargetAttitudeShape", dir + "pck00006.tpc");
  kern2 += PvlKeyword("Instrument", dir + "mocSpiceUnitTest.ti");
  kern2 += PvlKeyword("InstrumentAddendum", dir + "mocAddendum.ti");
  kern2 += PvlKeyword("InstrumentPosition", dir + "moc.bsp");
  kern2 += PvlKeyword("InstrumentPointing", dir + "moc.bc");
  kern2 += PvlKeyword("Frame", "");
  kern2 += PvlKeyword("NaifBodyCode", Isis::toString(499));

  PvlGroup kern3 = kern2;
  kern3 += PvlKeyword("ShapeModel", dir2  + "molaMarsPlanetaryRadius0005.cub");

  // Time Setup
  double startTime = -69382819.0;
  double endTime = -69382512.0;
  double slope = (endTime - startTime) / (10 - 1);

  kern1 += PvlKeyword("StartPadding", Isis::toString(slope));
  kern1 += PvlKeyword("EndPadding", Isis::toString(slope));

  Pvl lab1;
  lab1.addGroup(inst1);
  lab1.addGroup(kern1);

  // Create a Spice object to test radii
  Cube tmp("$ISISTESTDATA/isis/src/base/unitTestData/isisTruth.cub", "r");
  *tmp.label() = lab1;
  Spice spi(tmp);

  // Test good target
  Target tGood(NULL, lab1);
  cout << endl;
  cout << "  Good target test..." << endl;
  cout << "     NaifBodyCode = " << tGood.naifBodyCode() << endl;
  cout << "     TargetName = " << tGood.name().toStdString() << endl;
  cout << "     IsSky = " << tGood.isSky() << endl;

  // Use a Spice object to test radii
  vector<Distance> r(3);
  r = spi.target()->radii();
  cout << "     Target radii = " << r[0].kilometers()
                                 << "/" << r[1].kilometers() 
                                 << "/" << r[2].kilometers()
                                 << endl;

  // Test Sky
  Pvl lab2;
  lab2.addGroup(inst2);
  lab2.addGroup(kern1);
  Target tSky(NULL, lab2);
  cout << endl;
  cout << "  Testing Sky..." << endl;
  cout << "     IsSky = " << tSky.isSky() << endl;
  r = tSky.radii();
  cout << "     Sky Target radii = " << r[0].kilometers() << "/" << r[1].kilometers() << "/" << r[2].kilometers() << endl;
  cout << "     NaifBodyCode = " << tSky.naifBodyCode() << endl;

  // Test Sky with NaifSpkCode override
  PvlGroup kern4 = kern1;
  kern4 += PvlKeyword("NaifSpkCode", "-93");
  Pvl lab3;
  lab3.addGroup(inst2);
  lab3.addGroup(kern4);
  Target tSky2(NULL, lab3);
  cout << endl;
  cout << "  Testing Sky with NaifSpkCode..." << endl;
  cout << "     IsSky = " << tSky.isSky() << endl;
  cout << "     NaifBodyCode = " << tSky2.naifBodyCode() << endl;
  r = tSky2.radii();
  cout << "     Sky Target radii = " << r[0].kilometers() << "/" << r[1].kilometers() << "/" << r[2].kilometers() << endl;

  Pvl lab4;
  // Test without instrument group
  try {
    cout << endl << "  Testing no instrument group ..." << endl;
    lab4.addGroup(kern2);
    Target tNoInstrument(NULL, lab4);
  }
  catch(IException &e) {
    e.print();
    cout << endl;
  }
  
  Pvl lab5;
  lab4.addGroup(inst3);

  // Test without kernels group
  try {
    cout << endl << "  Testing no kernels group ..." << endl;
    Target tNoKernels(NULL, lab5);
  }
  catch(IException &e) {
    e.print();
    cout << endl;
  }

  // Test bad target
  try {
    cout << endl << "  Testing unknown target ..." << endl;
    Target tUnknownTarget(NULL, lab4);
  }
  catch(IException &e) {
    e.print();
    cout << endl;
  }

  // Test methods setShapeEllipsoid and restoreShape
  Pvl lab6;
  lab6.addGroup(inst1);
  lab6.addGroup(kern3);
  Target target3(NULL, lab6);
  cout << endl << "  Testing methods setShapeEllipsoid and restoreShape..." << endl;
  cout << "    Original shape is " << target3.shape()->name().toStdString() << endl;
  target3.setShapeEllipsoid();
  cout << "    Shape changed to  " << target3.shape()->name().toStdString() << endl;
  target3.restoreShape();
  cout << "    Shape restored to  " << target3.shape()->name().toStdString() << endl;

  //Test the default constructor
  Target defaultTarget;
  cout << endl << "  Testing default constructor..." << endl << "    Is it Sky? "  << defaultTarget.isSky() << endl;
  cout << "    Number of radii = " << defaultTarget.radii().size() << endl;

  cout << endl << endl;
  cout << "///////////////////////////////////////////////////////////" << endl << endl << endl;
  cout << "Testing radiiGroup() static methods " << endl;
  Pvl label;
  PvlGroup mappingGroup("Mapping");

  // Throw errors for incomplete label/mapping info
  try {
    // Mapping group does not have TargetRadii
    // Mapping group does not have TargetName
    // No IsisCube in label
    Target::radiiGroup(label, mappingGroup);
  }
  catch (IException &error) {
    printRadiiGroupInfo(false, label, mappingGroup);
    error.print();
    cout << "-------------------------------" << endl << endl;
  }

  label += PvlObject("IsisCube");
  try {
    // Mapping group does not have TargetRadii
    // Mapping group does not have TargetName
    // Instrument group not found in label
    Target::radiiGroup(label, mappingGroup);
  }
  catch (IException &error) {
    printRadiiGroupInfo(false, label, mappingGroup);
    error.print();
    cout << "-------------------------------" << endl << endl;
  }

  mappingGroup += PvlKeyword("TargetName", "");
  label.findObject("IsisCube").addGroup(PvlGroup("Instrument"));
  try {
    // Mapping group has TargetName=""
    // Instrument group found in label
    //     Instrument group does not have TargetName 
    Target::radiiGroup(label, mappingGroup);
  }
  catch (IException &error) {
    printRadiiGroupInfo(false, label, mappingGroup);
    error.print();
    cout << "-------------------------------" << endl << endl;
  }

  label.findObject("IsisCube").findGroup("Instrument").addKeyword(PvlKeyword("TargetName", ""));
  try {
    // Mapping group has TargetName=""
    // Instrument group found in label
    //     Instrument group has TargetName=""
    Target::radiiGroup(label, mappingGroup);
  }
  catch (IException &error) {
    printRadiiGroupInfo(false, label, mappingGroup);
    error.print();
    cout << "-------------------------------" << endl << endl;
  }

  mappingGroup.addKeyword(PvlKeyword("TargetName", "Chewbaca"), PvlContainer::Replace);
  try {
    // Mapping group has TargetName="Chewbaca" (not recognized by NAIF)
    // NaifKeywords object not found in label
    Target::radiiGroup(label, mappingGroup);
  }
  catch (IException &error) {
    printRadiiGroupInfo(false, label, mappingGroup);
    error.print();
    cout << "-------------------------------" << endl << endl;
  }

  label += PvlObject("NaifKeywords");
  try {
    // Mapping group has TargetName="Chewbaca" (not recognized by NAIF)
    // NaifKeywords object exists
    //     BODY_FRAME_CODE not found 
    Target::radiiGroup(label, mappingGroup);
  }
  catch (IException &error) {
    printRadiiGroupInfo(false, label, mappingGroup);
    error.print();
    cout << "-------------------------------" << endl << endl;
  }

  PvlObject &naifKeywords = label.findObject("NaifKeywords");
  naifKeywords += PvlKeyword("BODY_FRAME_CODE", "2101955");
  try {
    // Mapping group has TargetName="Chewbaca" (not recognized by NAIF)
    // NaifKeywords object exists
    //     BODY_FRAME_CODE exists
    //     BODY<code>_RADII does not exist
    //     BODY_FRAME_CODE not recognized by NAIF
    Target::radiiGroup(label, mappingGroup);
  }
  catch (IException &error) {
    printRadiiGroupInfo(false, label, mappingGroup);
    error.print();
    cout << "-------------------------------" << endl << endl;
  }

  // TargetName="Chewbaca" (not recognized by NAIF)
  // NaifKeywords object exists
  //     BODY_FRAME_CODE exists
  //     BODY<code>_RADII exists
  PvlKeyword bennuRadii("BODY2101955_RADII", "0.2825");
  bennuRadii.addValue("0.2675");
  bennuRadii.addValue("0.254");
  naifKeywords.addKeyword(bennuRadii);
  PvlGroup radii = Target::radiiGroup(label, mappingGroup);
  printRadiiGroupInfo(true, label, mappingGroup);
  radii.addComment("Set radii to BODY RADII values in NaifKeywords Object.");
  cout << radii;
  cout << endl << "-------------------------------" << endl << endl;

  // Valid TargetName found in Mapping group is recognized by NAIF
  mappingGroup.addKeyword(PvlKeyword("TargetName", "Mars"), PvlContainer::Replace);
  radii = Target::radiiGroup(label, mappingGroup);
  printRadiiGroupInfo(true, label, mappingGroup);
  radii.addComment("Find radii using known NAIF TargetName, Mars.");
  cout << radii;
  cout << endl << "-------------------------------" << endl << endl;

  // Radii values found in given mapping group
  printRadiiGroupInfo(true, label, radii);
  radii = Target::radiiGroup(label, radii);
  radii.addComment("Read radii from given Mapping group.");
  cout << radii;
  cout << endl << "-------------------------------" << endl << endl;

  cout << "///////////////////////////////////////////////////////////" << endl << endl << endl;
  cout << "Testing lookupNaifBodyCode() methods " << endl << endl;
  // known target
  cout << "FOUND NAIF BODY CODE FOR TARGET 'Mars': " 
       << toString((int)Target::lookupNaifBodyCode("Mars"))
       << endl << endl;
  try {
    // unknown target
    cout << Target::lookupNaifBodyCode("HanSolo");
  }
  catch (IException &error) {
    cout << "FAILED TO FIND NAIF BODY CODE FOR TARGET 'HanSolo." << endl;
    cout << "THROWS:" << endl << endl;
    error.print();
    cout << "-------------------------------" << endl << endl;
  }
  cout << endl << "///////////////////////////////////////////////////////////" << endl << endl << endl;

}

void printRadiiGroupInfo(bool found, Pvl &label, PvlGroup &mappingGroup) {
  cout << "-------------------------------" << endl;
  if (!found) {
    cout << "FAILED TO FIND RADII FOR LABEL: " << endl;
  }
  else {
    cout << "FOUND RADII FOR LABEL: " << endl;
  }
  cout << endl << label << endl << endl;
  cout << "AND MAPPING GROUP: " << endl;
  cout << endl << mappingGroup << endl << endl;
  cout << "RETURNS: " << endl << endl;
}
