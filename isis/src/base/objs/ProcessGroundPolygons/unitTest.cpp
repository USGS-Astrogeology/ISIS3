/** This is free and unencumbered software released into the public domain.
The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */
#include "Isis.h"

#include <QFile>

#include "ProcessGroundPolygons.h"
#include "ProcessByLine.h"
#include "ProcessByBrick.h"
#include "SpecialPixel.h"
#include "TextFile.h"

using namespace Isis;
using namespace std;

void writeAscii(Isis::Buffer &in);
void readValues(Isis::Buffer &in);

std::vector<double>lat, lon, values, vimsValues;

void IsisMain() {

  PvlGroup mapping("Mapping");
  {
    mapping += PvlKeyword("ProjectionName", "SimpleCylindrical");
    mapping += PvlKeyword("CenterLongitude", "227.83197474005");
    mapping += PvlKeyword("TargetName", "Mars");
    mapping += PvlKeyword("EquatorialRadius", "3396190.0");
    mapping += PvlKeyword("PolarRadius", "3376200.0");
    mapping += PvlKeyword("LatitudeType", "Planetocentric");
    mapping += PvlKeyword("LongitudeDirection", "PositiveEast");
    mapping += PvlKeyword("LongitudeDomain", "360");
    mapping += PvlKeyword("MinimumLatitude", "10.786871290848");
    mapping += PvlKeyword("MaximumLatitude", "34.44480953463");
    mapping += PvlKeyword("MinimumLongitude", "219.70338590854");
    mapping += PvlKeyword("MaximumLongitude", "235.96056357156");
    mapping += PvlKeyword("UpperLeftCornerX", "-483000.0");
    mapping += PvlKeyword("UpperLeftCornerY", "2043000.0 ");
    mapping += PvlKeyword("PixelResolution", "1000.0");
    mapping += PvlKeyword("Scale", "59.274697523306");
  }

  Pvl pvl;
  pvl.addGroup(mapping);

  PvlGroup vimsMapping("Mapping");
  {
    vimsMapping += PvlKeyword("ProjectionName", "Sinusoidal");
    vimsMapping += PvlKeyword("CenterLongitude", "308.47990638953");
    vimsMapping += PvlKeyword("TargetName", "TITAN");
    vimsMapping += PvlKeyword("EquatorialRadius", "2575000.0");
    vimsMapping += PvlKeyword("PolarRadius", "575000.0");
    vimsMapping += PvlKeyword("LatitudeType", "Planetocentric");
    vimsMapping += PvlKeyword("LongitudeDirection", "PositiveEast");
    vimsMapping += PvlKeyword("LongitudeDomain", "360");

    vimsMapping += PvlKeyword("MinimumLatitude", "9.916425315167");
    vimsMapping += PvlKeyword("MaximumLatitude", "10.087679837429");
    vimsMapping += PvlKeyword("MinimumLongitude", "308.42684479508");
    vimsMapping += PvlKeyword("MaximumLongitude", "308.53296798387");

    vimsMapping += PvlKeyword("PixelResolution", "498.46721637851");
    vimsMapping += PvlKeyword("Scale", "90.160850627992");
  }

  cout << "Testing Isis::ProcessGroundPolygons Class ... " << endl;

  ProcessGroundPolygons p;
  ProcessByLine pbl;
  CubeAttributeInput atts0;

  QStringList vimsCubes;
  vimsCubes << "$ISISTESTDATA/isis/src/cassini/unitTestData/CM_1540484927_1_001.ir.cub"
            << "$ISISTESTDATA/isis/src/cassini/unitTestData/CM_1540484927_1_002.ir.cub"
            << "$ISISTESTDATA/isis/src/cassini/unitTestData/CM_1540484927_1_003.ir.cub";

  std::vector<double> vimsSamps, vimsLines, vect;
  Pvl vimsPvl;
  vimsPvl.addGroup(vimsMapping);

  CubeAttributeOutput out_atts;
  out_atts.setPixelType(Real);

  QString output1 = "ProcessGroundPolygonsTest.cub";
  QString output2 = "ProcessGroundPolygonsTest_count.cub";

  p.SetStatCubes(output1 , output2 , out_atts, vimsPvl, 2);

  for(int f = 0; f < vimsCubes.size(); f++) {
    Cube vimsCube(vimsCubes[f].toStdString(), "r");
    UniversalGroundMap *groundMap = new UniversalGroundMap(vimsCube);

    vimsValues.clear();
    pbl.SetInputCube(vimsCubes[f], atts0, 0);
    pbl.StartProcess(readValues);
    pbl.EndProcess();
    double latitude, longitude;
    int i = 0;

    for(int b = 1; b < 3; b++) {
      for(int l = 0; l < 1; l++) {

        for(int s = 0; s < 13; s++) {

          vimsSamps.push_back(s + 0.5);
          vimsSamps.push_back(s + 1.5);
          vimsSamps.push_back(s + 1.5);
          vimsSamps.push_back(s + 0.5);

          vimsLines.push_back(l + 0.5);
          vimsLines.push_back(l + 0.5);
          vimsLines.push_back(l + 1.4375);
          vimsLines.push_back(l + 1.4375);

          //Now need to convert all samps and lines to lat lon.
          for(unsigned int j = 0; j < vimsSamps.size(); j++) {

            if(groundMap->SetImage(vimsSamps[j], vimsLines[j])) {
              latitude = groundMap->UniversalLatitude();
              longitude = groundMap->UniversalLongitude();
              lat.push_back(latitude);
              lon.push_back(longitude);
            }

          }

          if(lat.size() > 3) {
            p.Rasterize(lat, lon, b, vimsValues[i]);
          }

          lat.clear();
          lon.clear();
          vimsSamps.clear();
          vimsLines.clear();
          i++;

        }//end for each s -- sample
      }// end for each l -- line
    }//end for each b -- band

    delete groundMap;

  } //end for each vims file.

  p.EndProcess();

  /*   Write out the data    */

  CubeAttributeInput atts;

  std::cout << "File Name: " << output1.toStdString() << std::endl;
  pbl.SetInputCube(output1, atts, 0);
  pbl.StartProcess(writeAscii);
  pbl.EndProcess();

  std::cout << "File Name: " << output2.toStdString() << std::endl;
  pbl.SetInputCube(output2, atts, 0);
  pbl.StartProcess(writeAscii);
  pbl.EndProcess();

  QFile::remove(output1);
  QFile::remove(output2);


}

/**
 * This method reads in the cube file line by line and prints
 * out the DN value of each non-null pixel.
 *
 * @param in
 */
void writeAscii(Isis::Buffer &in) {
  bool notNull = false;
  int index = in.size() - 1;
  for(int i = 0; i < in.size(); i++) {
    if(in[index -i] > 0) {
      cout << "Band: " << in.Band() << " DN: " << in[index -i] << " ";
      notNull = true;
    }
  }
  if(notNull) {
    cout << std::endl;
  }

}

void readValues(Isis::Buffer &in) {
  //int index = in.size() - 1;

  for(int i = 0; i < in.size(); i++) {
    vimsValues.push_back(in[i]);
  }
  //if(in.Band() != 96){
  //vimsValues.clear();
  //}

}


