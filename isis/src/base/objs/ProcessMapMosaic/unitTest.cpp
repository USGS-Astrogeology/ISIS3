/** This is free and unencumbered software released into the public domain.
The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */
#include "Isis.h"

#include <string>
#include <regex>

#include "ProcessMapMosaic.h"
#include "Application.h"
#include "CubeAttribute.h"
#include "FileList.h"
#include "FileName.h"
#include "ProcessMosaic.h"
#include "LineManager.h"

using namespace Isis;
using namespace std;

/** 
 * Unit test for ProcessMapMosaic class
 *  
 * @author ????-??-?? Unknown 
 *  
 *  @internal
 *   @history 2018-06-06 Jeannie Backer - Removed file paths from error message written to
 *                           test output.
 *  
 */
void IsisMain() {

  Preference::Preferences(true);

  cout << "Testing Isis::ProcessMapMosaic Class ... " << endl;

  // Create the temp parent cube
  FileList cubes;
  cubes.read("unitTest.lis");

  cout << "Testing Mosaic 1" << endl;
  ProcessMapMosaic m1;
  CubeAttributeOutput oAtt;
  ProcessMosaic::ImageOverlay priority = ProcessMapMosaic::PlaceImagesOnTop;
  m1.SetBandBinMatch(false);
  m1.SetOutputCube(cubes, oAtt, "./unitTest.cub");

  //set priority
  m1.SetImageOverlay(priority);

  for(int i = 0; i < cubes.size(); i++) {
    std::regex pattern(".*base/unitTestData");
    if(m1.StartProcess(QString::fromStdString(cubes[i].toString()))) {
      cout << std::regex_replace(cubes[i].toString(), pattern, "base/unitTestData") << " is inside the mosaic" << endl;
    }
    else {
      cout << std::regex_replace(cubes[i].toString(), pattern, "base/unitTestData") << " is outside the mosaic" << endl;
    }
  }

  m1.EndProcess();
  cout << "Mosaic label: " << endl;

  Pvl labels("./unitTest.cub");
  cout << labels << endl;

  remove("./unitTest.cub");

  cout << "Testing Mosaic 2" << endl;
  ProcessMapMosaic m2;
  m2.SetBandBinMatch(false);
  m2.SetOutputCube(cubes, -6, -4, 29, 31, oAtt, "./unitTest.cub");

  //set priority
  m2.SetImageOverlay(priority);

  for(int i = 0; i < cubes.size(); i++) {
    std::regex pattern(".*base/unitTestData");
    if(m2.StartProcess(QString::fromStdString(cubes[i].toString()))) {
      cout << std::regex_replace(cubes[i].toString(), pattern, "base/unitTestData") << " is inside the mosaic" << endl;
    }
    else {
      cout << std::regex_replace(cubes[i].toString(), pattern, "base/unitTestData") << " is outside the mosaic" << endl;
    }
  }

  m2.EndProcess();
  cout << "Mosaic label: " << endl;

  labels.clear();
  labels.read("./unitTest.cub");
  cout << labels << endl;

  Cube tmp;
  tmp.open("./unitTest.cub");
  LineManager lm(tmp);
  lm.SetLine(1, 1);

  while(!lm.end()) {
    tmp.read(lm);
    cout << "Mosaic Data: " << lm[lm.SampleDimension()/4] << '\t' <<
              lm[lm.SampleDimension()/2] << '\t' <<
              lm[(3*lm.SampleDimension())/4] << endl;
    lm++;
  }

  tmp.close();
  remove("./unitTest.cub");  // Create the temp parent cube

  cout << endl << "Testing Mosaic where the input (x, y) is negative,"
          " according to the output cube." << endl;
  std::string inputFile = FileName("$ISISTESTDATA/isis/src/base/unitTestData/ProcessMapMosaic/unitTest1.cub").expanded();
  Cube inCube;
  inCube.open(QString::fromStdString(inputFile));
  PvlGroup mapGroup = inCube.label()->findGroup("Mapping", Pvl::Traverse);

  mapGroup.addKeyword(PvlKeyword("MinimumLatitude",  toString(-4.9)), Pvl::Replace);
  mapGroup.addKeyword(PvlKeyword("MaximumLatitude",  toString(-4.7)), Pvl::Replace);
  mapGroup.addKeyword(PvlKeyword("MinimumLongitude", toString(30.7)), Pvl::Replace);
  mapGroup.addKeyword(PvlKeyword("MaximumLongitude", toString(31)), Pvl::Replace);
  
  inCube.close();
  CubeAttributeOutput oAtt2( FileName("./unitTest3.cub") );
  ProcessMapMosaic m3;
  
  m3.SetBandBinMatch(false);
  m3.SetOutputCube(QString::fromStdString(inputFile), mapGroup, oAtt2, "./unitTest3.cub");

  //set priority
  m3.SetImageOverlay(priority);
  m3.SetHighSaturationFlag(false);
  m3.SetLowSaturationFlag(false);
  m3.SetNullFlag(false);

  if(m3.StartProcess(QString::fromStdString(inputFile))) {
    cout << "The mosaic was successfull." << endl;
  }
  else {
    cout << "The mosaic was not successfull." << endl;
  }

  m3.EndProcess();
  cout << "Mosaic label: " << endl;

  Pvl labels2("./unitTest3.cub");
  cout << labels2 << endl;

  remove("./unitTest3.cub");

  // Create the temp parent cube
  FileList cubes_crop;
  cubes_crop.read("unitTest_crop.lis");

  cout << endl << "Testing Mosaic containing cropped image." << endl;
  ProcessMapMosaic m4;
  CubeAttributeOutput oAtt3;
  priority = ProcessMapMosaic::PlaceImagesOnTop;
  m4.SetBandBinMatch(false);
  m4.SetOutputCube(cubes_crop, oAtt3, "./unitTest4.cub");

  //set priority
  m4.SetImageOverlay(priority);

  for(int i = 0; i < cubes_crop.size(); i++) {
    std::regex pattern(".*base/unitTestData");
    if(m4.StartProcess(QString::fromStdString(cubes_crop[i].toString()))) {
      cout << std::regex_replace(cubes_crop[i].toString(), pattern, "base/unitTestData") << " is inside the mosaic" << endl;
    }
    else {
      cout << std::regex_replace(cubes_crop[i].toString(), pattern, "base/unitTestData") << " is outside the mosaic" << endl;
    }
  }

  m4.EndProcess();
  cout << "Mosaic label: " << endl;

  Pvl labels_crop("./unitTest4.cub");
  cout << labels_crop << endl;

  remove("./unitTest4.cub");
}
