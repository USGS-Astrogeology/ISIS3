/** This is free and unencumbered software released into the public domain.
The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */
#include <iostream>
#include <iomanip>
#include <cmath>

#include "Camera.h"
#include "CameraFactory.h"
#include "IException.h"
#include "LambertAzimuthalEqualArea.h"
#include "Projection.h"
#include "ProjectionFactory.h"
#include "Preference.h"
#include "Pvl.h"
#include "PvlGroup.h"
#include "PvlKeyword.h"
#include "TProjection.h"

using namespace std;
using namespace Isis;
int main(int argc, char *argv[]) {
  Preference::Preferences(true);

  cout << "UNIT TEST FOR LambertAzimuthalEqualArea projection" << endl << endl;

  Pvl lab;
  lab.addGroup(PvlGroup("Mapping"));
  PvlGroup &mapGroup = lab.findGroup("Mapping");
  mapGroup += PvlKeyword("EquatorialRadius", "1.0");
  mapGroup += PvlKeyword("PolarRadius", "1.0");
  mapGroup += PvlKeyword("LatitudeType", "Planetographic");
  mapGroup += PvlKeyword("LongitudeDirection", "PositiveEast");
  mapGroup += PvlKeyword("LongitudeDomain", "180");
  mapGroup += PvlKeyword("MinimumLatitude", "20.0");
  mapGroup += PvlKeyword("MaximumLatitude", "80.0");
  mapGroup += PvlKeyword("MinimumLongitude", "-180.0");
  mapGroup += PvlKeyword("MaximumLongitude", "180.0");
  mapGroup += PvlKeyword("ProjectionName", "LambertAzimuthalEqualArea");
  mapGroup += PvlKeyword("CenterLatitude", "0");
  mapGroup += PvlKeyword("CenterLongitude", "0");
  mapGroup += PvlKeyword("PixelResolution", ".001");

  try {
    string border = "||||||||||||||||||||||||||||||||||||||||"
                    "||||||||||||||||||||||||||||||||||||||||";

    //||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||
    cout << border << endl;
    cout << "\t\t\t SPHERICAL-PLANETOGRAPHIC-POSITIVEEAST-EQUATORIAL-180" << endl;
    cout << border  << endl << endl;
    //||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||
    Projection &proj = *ProjectionFactory::Create(lab);
    TProjection *p1 = (TProjection *) &proj;
    cout << mapGroup["CenterLatitude"] << endl;
    cout << mapGroup["CenterLongitude"] << endl;
    cout << mapGroup["EquatorialRadius"] << endl;
    cout << mapGroup["PolarRadius"] << endl;
    cout << "Eccentricity = " << p1->Eccentricity() << endl;
    cout << "TrueScaleLatitude = " << p1->TrueScaleLatitude() << endl << endl;

    cout << std::fixed;
    cout << std::setprecision(5);
    cout << endl;
    cout << "\t\t\t\t/-----------------------------------------/" << endl;
    cout << endl;
    cout << "    Testing SetGround method using Snyder Table 28, "
            "page 188" << endl;
    for (double lat = 90; lat >= 0; lat-=10) {
      for (double lon = 0; lon < 50; lon+=10) {
        p1->SetGround(lat, lon);
        cout << p1->XCoord() << "/" << p1->YCoord() << " ";
      }
      cout << endl;
    }
    cout << endl;
    cout << "\t\t\t\t/-----------------------------------------/" << endl;
    cout << endl;
    cout << "    Testing SetGround method using Snyder Table 28, "
            "page 189" << endl;
    for (double lat = 90; lat >= 0; lat-=10) {
      for (double lon = 50; lon < 100; lon+=10) {
        p1->SetGround(lat, lon);
        cout << p1->XCoord() << "/" << p1->YCoord() << " ";
      }
      cout << endl;
    }
    cout << std::setprecision(7);
    cout << endl;
    cout << "\t\t\t\t/-----------------------------------------/" << endl;
    cout << endl;
    cout << "    Testing SetCoordinate(0.03941, 1.28702)\n"
            "    from Snyder Table 28, page 188, line 2 column 2" << endl;
    p1->SetCoordinate(0.03941, 1.28702);
    cout << "Latitude:            " << p1->Latitude() << endl;
    cout << "Longitude:           " << p1->Longitude() << endl;
    cout << "XCoord:              " << p1->XCoord() << endl;
    cout << "YCoord:              " << p1->YCoord() << endl;
    cout << endl;
    cout << "\t\t\t\t/-----------------------------------------/" << endl;
    cout << endl;
    cout << "    Testing projection to origin and back\n"
            "    SetGround(0, 0)" << endl;
    p1->SetGround(0.0, 0.0);
    cout << "Latitude:            " << p1->Latitude() << endl;
    cout << "Longitude:           " << p1->Longitude() << endl;
    cout << "XCoord:              " << p1->XCoord() << endl;
    cout << "YCoord:              " << p1->YCoord() << endl;
    cout << "    SetCoordinate(0, 0)" << endl;
    p1->SetCoordinate(0.0, 0.0);
    cout << "Latitude:            " << p1->Latitude() << endl;
    cout << "Longitude:           " << p1->Longitude() << endl;
    cout << "XCoord:              " << p1->XCoord() << endl;
    cout << "YCoord:              " << p1->YCoord() << endl;
    cout << endl;
    cout << "\t\t\t\t/-----------------------------------------/" << endl;
    cout << endl;   
    cout << "    Testing projection to north pole and back\n"
            "    SetGround(90, 0)" << endl;
    p1->SetGround(90.0, 0.0);
    cout << "Latitude:            " << p1->Latitude() << endl;
    cout << "Longitude:             " << p1->Longitude() << endl;
    cout << "XCoord:                " << p1->XCoord() << endl;
    cout << "YCoord:                " << p1->YCoord() << endl;
    cout << "    SetCoordinate(0, sqrt(2)*sphRad)" << endl;
    p1->SetCoordinate(0.0, sqrt(2.0));
    cout << "Latitude:             " << p1->Latitude() << endl;
    cout << "Longitude:           " << p1->Longitude() << endl;
    cout << "XCoord:                " << p1->XCoord() << endl;
    cout << "YCoord:                " << p1->YCoord() << endl;
    cout << endl;
    cout << "\t\t\t\t/-----------------------------------------/" << endl;
    cout << endl;   
    // CHECKING OTHER KNOWN POINTS
    double rad = p1->LocalRadius(mapGroup.findKeyword("CenterLatitude"));
    cout << "    Testing other known points..." << endl;
    cout << endl;   
    cout << "        Comparison Values" << endl;
    cout << "        SphRad (Spherical Radius) = " << rad << endl;
    cout << "        sqrt2*SphRad              = " << rad*sqrt(2) << endl;
    cout << "        sqrt2*SphRad*sqrt3/2      = " << rad*sqrt(6)/2 << endl;
    cout << "        sqrt2*SphRad/2            = " << rad*sqrt(2)/2 << endl;
    cout << endl;   
    // points on circle sqrt(2)*ER in diameter (hemisphere map)
    cout << "    Check known values on hemispherical map" << endl;
    cout << "        FORWARD" << endl;
    p1->SetGround(0, 90);
    cout << "            SetGround(0, 90) returns ";
    cout << "(x,y) = (" << p1->XCoord() << ", " << p1->YCoord() << ")"<< endl;
    p1->SetGround(90, 0);
    cout << "            SetGround(90, 0) returns ";
    cout << "(x,y) = (" << p1->XCoord() << ", " << p1->YCoord() << ")"<< endl;
    p1->SetGround(0, -90);
    cout << "            SetGround(0, -90) returns ";
    cout << "(x,y) = (" << p1->XCoord() << ", " << p1->YCoord() << ")"<< endl;
    p1->SetGround(0, 270);
    cout << "            SetGround(0, 270) returns ";
    cout << "(x,y) = (" << p1->XCoord() << ", " << p1->YCoord() << ")"<< endl;
    p1->SetGround(-90, 0);
    cout << "            SetGround(-90, 0) returns ";
    cout << "(x,y) = (" << p1->XCoord() << ", " << p1->YCoord() << ")"<< endl;
    p1->SetGround(-45, 90);
    cout << "            SetGround(-45, 90) returns ";
    cout << "(x,y) = (" << p1->XCoord() << ", " << p1->YCoord() << ")"<< endl;
    p1->SetGround(30, -90);
    cout << "            SetGround(30, -90) returns ";
    cout << "(x,y) = (" << p1->XCoord() << ", " << p1->YCoord() << ")"<< endl;
    cout << "        BACKWARD" << endl;
    cout << std::setprecision(5);
    p1->SetCoordinate(rad*sqrt(2), 0);
    cout << "            SetCoordinate(sqrt2*SphRad, 0) returns ";
    cout << "lat/lon = " << p1->Latitude() 
                             << " / " << p1->Longitude() << endl;
    p1->SetCoordinate(0, rad*sqrt(2));
    cout << "            SetCoordinate(0, sqrt2*SphRad) returns ";
    cout << "lat/lon = " << p1->Latitude() 
                             << " / " << p1->Longitude() << endl;
    p1->SetCoordinate(-rad*sqrt(2), 0);
    cout << "            SetCoordinate(-sqrt2*SphRad, 0) returns ";
    cout << "lat/lon = " << p1->Latitude() 
                             << " / " << p1->Longitude() << endl;
    p1->SetCoordinate(0, -rad*sqrt(2));
    cout << "            SetCoordinate(0, -sqrt2*SphRad) returns ";
    cout << "lat/lon = " << p1->Latitude() 
                             << " / " << p1->Longitude() << endl;
    p1->SetCoordinate(1, 1);
    cout << "            SetCoordinate(1, 1) returns ";
    cout << "lat/lon = " << p1->Latitude() 
                             << " / " << p1->Longitude() << endl;
    p1->SetCoordinate(sqrt(6)/2*rad, sqrt(2)/2*rad);
    cout << "            SetCoordinate(sqrt6/2*rad, sqrt2/2*rad) returns ";
    cout << "lat/lon = " << p1->Latitude() 
                             << " / " << p1->Longitude() << endl;
    cout << endl;
    cout << endl;
    // points on circle 2*ER in diameter (whole planet map)
    cout << endl;
    cout << "    Check known values on (almost) whole planet map" << endl;
    // The following was removed due to roundoff errors on prog8 (mac)
    // cout << "        FORWARD - Project to opposite side of planet " << endl;
    // cout << std::setprecision(7);
    // if (p1->SetGround(0, 179.99999)) {
    //   cout << "            SetGround(0, 179.99999) returns ";
    //   cout << "(x,y) = (" << p1->XCoord() << ", " << p1->YCoord() << ")"<< endl;
    //   cout << "                 Expect a value near (2*SphRad,0) = (" 
    //        << 2*rad << ", 0.0000000)" << endl;
    // }
    // if (p1->SetGround(0, -179.99999)) {
    //   cout << "            SetGround(0, -179.99999) returns ";
    //   cout << "(x,y) = (" << p1->XCoord() << ", " << p1->YCoord() << ")"<< endl;
    //   cout << "                 Expect a value near (-2*SphRad,0) = (" 
    //        << -2*rad << ", 0.0000000)" << endl;
    // }
    cout << std::setprecision(7);
    cout << "        BACKWARD - Project from opposite side of planet " << endl;
    cout << "        For each of these, expect a value near" << endl;
    cout << "            - centerLatitude / centerLongitude+180 = 0 / 180" << endl;
    if (p1->SetCoordinate(rad*2, 0)) {
      cout << "            SetCoordinate(2*SphRad, 0) returns ";
      cout << "lat/lon = " << p1->Latitude() 
                               << " / " << p1->Longitude() << endl;
    }
    cout << std::setprecision(5);
    if (p1->SetCoordinate(0, rad*2)) {
      cout << "            SetCoordinate(0, 2*SphRad) returns ";
      cout << "lat/lon = " << p1->Latitude() 
                               << " / " << p1->Longitude() << endl;
    }
    if (p1->SetCoordinate(-rad*2, 0)) {
      cout << "            SetCoordinate(-2*SphRad, 0) returns ";
      cout << "lat/lon = " << p1->Latitude() 
                               << " / " << p1->Longitude() << endl;
    }
    if (p1->SetCoordinate(0, -rad*2)) {
      cout << "            SetCoordinate(0, -2*SphRad) returns ";
      cout << "lat/lon = " << p1->Latitude() 
                               << " / " << p1->Longitude() << endl;
    }
    if (p1->SetCoordinate(sqrt(2)*rad, sqrt(2)*rad)) {
      cout << "            SetCoordinate(sqrt2*SphRad, sqrt2*SphRad) returns ";
      cout << "lat/lon = " << p1->Latitude() 
                               << " / " << p1->Longitude() << endl;
    }
    if (p1->SetCoordinate(sqrt(3)*rad, rad)) {
      cout << "            SetCoordinate(sqrt3*SphRad, SphRad) returns ";
      cout << "lat/lon = " << p1->Latitude() 
                               << " / " << p1->Longitude() << endl;
    }
    cout << endl;
    cout << "\t\t\t\t/-----------------------------------------/" << endl;
    cout << endl;
    cout << std::setprecision(7);
    cout << "    Testing XYRange method" << endl;
    cout << "Given: " << endl;
    cout << "    Minimum Latitude:  " << p1->MinimumLatitude() << endl;
    cout << "    Maximum Latitude:  " << p1->MaximumLatitude() << endl;
    cout << "    Minimum Longitude: " << p1->MinimumLongitude() << endl;
    cout << "    Maximum Longitude: " << p1->MaximumLongitude() << endl;
    double minX, maxX, minY, maxY;
    p1->XYRange(minX, maxX, minY, maxY);
    cout << "XYRange method returns" << endl;
    cout << "    Minimum X:  " << minX << endl;
    cout << "    Maximum X:  " << maxX << endl;
    cout << "    Minimum Y:  " << minY << endl;
    cout << "    Maximum Y:  " << maxY << endl;
    cout << endl;
    p1->SetCoordinate(maxX,0);
    cout << "            SetCoordinate(maxX,0) returns lat/lon = " 
         << p1->Latitude() << " / " << p1->Longitude() << endl;
    p1->SetCoordinate(0, maxY);
    cout << "            SetCoordinate(0,maxY) returns lat/lon = " 
         << p1->Latitude() << " / " << p1->Longitude()  << endl;
    p1->SetCoordinate(minX,0);
    cout << "            SetCoordinate(minX,0) returns lat/lon = " 
         << p1->Latitude() << " / " << p1->Longitude()  << endl;
    p1->SetCoordinate(0, minY);
    cout << "            SetCoordinate(0,minY) returns lat/lon = " 
         << p1->Latitude() << " / " << p1->Longitude()  << endl;
    cout << endl;
    p1->SetGround(20, 0);
    cout << "            SetGround(20, 0) returns y min? ";
    cout << "(x,y) = (" << p1->XCoord() << ", " << p1->YCoord() << ")"<< endl;
    p1->SetGround(20, 180);
    cout << "            SetGround(20, 180) returns y max? ";
    cout << "(x,y) = (" << p1->XCoord() << ", " << p1->YCoord() << ")"<< endl;
    cout << endl;
    cout << endl;
    mapGroup.findKeyword("MinimumLatitude").setValue("-90.0");
    mapGroup.findKeyword("MaximumLatitude").setValue("90.0");
    mapGroup.findKeyword("MinimumLongitude").setValue("-179.99999");
    mapGroup.findKeyword("MaximumLongitude").setValue("179.99999");
    TProjection *p1a = (TProjection *) ProjectionFactory::Create(lab);
    cout << "Given: " << endl;
    cout << "    Minimum Latitude:  " << p1a->MinimumLatitude() << endl;
    cout << "    Maximum Latitude:  " << p1a->MaximumLatitude() << endl;
    cout << "    Minimum Longitude: " << p1a->MinimumLongitude() << endl;
    cout << "    Maximum Longitude: " << p1a->MaximumLongitude() << endl << endl;
    p1a->XYRange(minX, maxX, minY, maxY);
    cout << "XYRange method returns" << endl;
    cout << "    Minimum X:  " << minX << endl;
    cout << "    Maximum X:  " << maxX << endl;
    cout << "    Minimum Y:  " << minY << endl;
    cout << "    Maximum Y:  " << maxY << endl;
    cout << endl;
    p1a->SetCoordinate(maxX,0);
    cout << "            SetCoordinate(maxX,0) returns lat/lon = " 
         << p1a->Latitude() << " / " << p1a->Longitude()  << endl;
    p1a->SetCoordinate(0, maxY);
    cout << "            SetCoordinate(0,maxY) returns lat/lon = " 
         << p1a->Latitude() << " / " << p1a->Longitude()  << endl;
    p1a->SetCoordinate(minX,0);
    cout << "            SetCoordinate(minX,0) returns lat/lon = " 
         << p1a->Latitude() << " / " << p1a->Longitude()  << endl;
    p1a->SetCoordinate(0, minY);
    cout << "            SetCoordinate(0,minY) returns lat/lon = " 
         << p1a->Latitude() << " / " << p1a->Longitude()  << endl;
    mapGroup.findKeyword("MinimumLongitude").setValue("-90.0");
    mapGroup.findKeyword("MaximumLongitude").setValue("90.0");
    cout << "Given: " << endl;
    TProjection *p1b = (TProjection *) ProjectionFactory::Create(lab);
    cout << "    Minimum Latitude:  " << p1b->MinimumLatitude() << endl;
    cout << "    Maximum Latitude:  " << p1b->MaximumLatitude() << endl;
    cout << "    Minimum Longitude: " << p1b->MinimumLongitude() << endl;
    cout << "    Maximum Longitude: " << p1b->MaximumLongitude() << endl << endl;
    p1b->XYRange(minX, maxX, minY, maxY);
    cout << "XYRange method returns" << endl;
    cout << "    Minimum X:  " << minX << endl;
    cout << "    Maximum X:  " << maxX << endl;
    cout << "    Minimum Y:  " << minY << endl;
    cout << "    Maximum Y:  " << maxY << endl;
    cout << endl;
    p1b->SetCoordinate(maxX,0);
    cout << "            SetCoordinate(maxX,0) returns lat/lon = " 
         << p1b->Latitude() << " / " << p1b->Longitude()  << endl;
    p1b->SetCoordinate(0, maxY);
    cout << "            SetCoordinate(0,maxY) returns lat/lon = " 
         << p1b->Latitude() << " / " << p1b->Longitude()  << endl;
    p1b->SetCoordinate(minX,0);
    cout << "            SetCoordinate(minX,0) returns lat/lon = " 
         << p1b->Latitude() << " / " << p1b->Longitude()  << endl;
    p1b->SetCoordinate(0, minY);
    cout << "            SetCoordinate(0,minY) returns lat/lon = " 
         << p1b->Latitude() << " / " << p1b->Longitude()  << endl;
    cout << endl;
    cout << endl;
    cout << endl;
    cout << endl;
    //||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||
    cout << border << endl;
    cout << "\t\t\t SPHERICAL-PLANETOGRAPHIC-POSITIVEEAST-OBLIQUE-360" << endl;
    cout << border << endl << endl;
    //||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||
    mapGroup.deleteKeyword("EquatorialRadius");
    mapGroup += PvlKeyword("EquatorialRadius", "3.0");
    mapGroup.findKeyword("PolarRadius").setValue("3.0");
    mapGroup.findKeyword("CenterLatitude").setValue("40.0");
    mapGroup.findKeyword("CenterLongitude").setValue("-100.0");
    mapGroup.findKeyword("MinimumLongitude").setValue("-279.99999");
    mapGroup.findKeyword("MaximumLongitude").setValue("79.99999");
    mapGroup.findKeyword("LongitudeDomain").setValue("360");
    TProjection *p2 = (TProjection *) ProjectionFactory::Create(lab);
    cout << mapGroup["CenterLatitude"] << endl;
    cout << mapGroup["CenterLongitude"] << endl;
    cout << "EquatorialRadius = " << p2->EquatorialRadius() << endl;
    cout << "PolarRadius = " << p2->PolarRadius() << endl;
    cout << "Eccentricity = " << p2->Eccentricity() << endl;
    cout << "TrueScaleLatitude = " << p2->TrueScaleLatitude() << endl << endl;
    cout << "\t\t\t\t/-----------------------------------------/" << endl;
    cout << endl;
    cout << "    Testing SetGround(-20, 100) from Snyder pages 332-333" << endl;
    p2->SetGround(-20, 100);
    cout << "Latitude:            " << p2->Latitude() << endl;
    cout << "Longitude:           " << p2->Longitude() << endl;
    cout << "XCoord:              " << p2->XCoord() << endl;
    cout << "YCoord:              " << p2->YCoord() << endl;
    // we do not have relative scale factor methods in Projection as of 06/2012
    // so these need to be tested with a LambertAzimuthalEqualArea object
    // specifically.  The following values were calculated by hand to verify.
    LambertAzimuthalEqualArea lam2(lab);
    lam2.SetGround(-20, 100);
    cout << "RelativeScaleLatitude:  " << lam2.relativeScaleFactorLatitude() << endl;
    cout << "RelativeScaleLongitude: " << lam2.relativeScaleFactorLongitude() << endl;
    cout << endl;
    cout << "\t\t\t\t/-----------------------------------------/" << endl;
    cout << endl;
    cout << "    Testing SetCoordinate(-4.2339303, 4.0257775) from Snyder "
            "pages 332-333" << endl;
    p2->SetCoordinate(-4.2339303, 4.0257775);
    cout << "Latitude:            " << p2->Latitude() << endl;
    cout << "Longitude:           " << p2->Longitude() << endl;
    cout << "XCoord:              " << p2->XCoord() << endl;
    cout << "YCoord:              " << p2->YCoord() << endl;
    cout << endl;
    cout << "\t\t\t\t/-----------------------------------------/" << endl;
    cout << endl;
    cout << "    Testing projection to origin and back\n"
            "    SetGround(40, -100)" << endl;
    p2->SetGround(40.0, -100.0);
    cout << "Latitude:              " << p2->Latitude() << endl;
    cout << "Longitude:           " << p2->Longitude() << endl;
    cout << "XCoord:                 " << p2->XCoord() << endl;
    cout << "YCoord:                 " << p2->YCoord() << endl;
    cout << "    SetCoordinate(0, 0)" << endl;
    p2->SetCoordinate(0.0, 0.0);
    cout << "Latitude:              " << p2->Latitude() << endl;
    cout << "Longitude:           " << p2->Longitude() << endl;
    cout << "XCoord:                 " << p2->XCoord() << endl;
    cout << "YCoord:                 " << p2->YCoord() << endl;
    cout << endl;
    cout << "\t\t\t\t/-----------------------------------------/" << endl;
    cout << endl;   
    cout << "    Testing projection to north pole and back\n"
            "    SetGround(90, 0)" << endl;
    p2->SetGround(90.0, 0.0);
    cout << "Latitude:            " << p2->Latitude() << endl;
    cout << "Longitude:            " << p2->Longitude() << endl;
    cout << "XCoord:               " << p2->XCoord() << endl;
    cout << "YCoord:               " << p2->YCoord() << endl;
    cout << "    SetCoordinate(0, 2.5357096)" << endl;
    p2->SetCoordinate(0.0, 2.5357096);
    cout << "Latitude:            " << p2->Latitude() << endl;
    cout << "Longitude:           " << p2->Longitude() << endl;
    cout << "XCoord:               " << p2->XCoord() << endl;
    cout << "YCoord:               " << p2->YCoord() << endl;
    cout << endl;
    cout << "\t\t\t\t/-----------------------------------------/" << endl;
    cout << endl;   
    // CHECKING OTHER KNOWN POINTS
    //rad = mapGroup.findKeyword("EquatorialRadius");
    rad = p2->LocalRadius(mapGroup.findKeyword("CenterLatitude"));
    cout << "    Testing other known points..." << endl;
    cout << endl;   
    cout << "        Comparison Values" << endl;
    cout << "        SphRad (Spherical Radius) = " << rad << endl;
    cout << "        sqrt2*SphRad              = " << rad*sqrt(2) << endl;
    cout << endl;
    // points on circle sqrt(2)*ER in diameter (hemisphere map)
    cout << "    Check known values on hemispherical map" << endl;
    cout << "        FORWARD" << endl;
    p2->SetGround(0, -10);
    cout << "            SetGround(0, -10) returns ";
    cout << "(x,y) = (" << p2->XCoord() << ", " << p2->YCoord() << ")"<< endl;
    p2->SetGround(50, 80);
    cout << "            SetGround(50, 80) returns ";
    cout << "(x,y) = (" << p2->XCoord() << ", " << p2->YCoord() << ")"<< endl;
    p2->SetGround(0, -190);
    cout << "            SetGround(0, -190) returns ";
    cout << "(x,y) = (" << p2->XCoord() << ", " << p2->YCoord() << ")"<< endl;
    p2->SetGround(-50, -100);
    cout << "            SetGround(-50, -100) returns ";
    cout << "(x,y) = (" << p2->XCoord() << ", " << p2->YCoord() << ")"<< endl;
    cout << "        BACKWARD" << endl;
    cout << std::setprecision(5);
    p2->SetCoordinate(rad*sqrt(2), 0);
    cout << "            SetCoordinate(sqrt2*SphRad, 0) returns ";
    cout << "lat/lon = " << p2->Latitude() 
                             << " / " << p2->Longitude() << endl;
    p2->SetCoordinate(0, rad*sqrt(2));
    cout << "            SetCoordinate(0, sqrt2*SphRad) returns ";
    cout << "lat/lon = " << p2->Latitude() 
                             << " / " << p2->Longitude() << endl;
    p2->SetCoordinate(-rad*sqrt(2), 0);
    cout << "            SetCoordinate(-sqrt2*SphRad, 0) returns ";
    cout << "lat/lon = " << p2->Latitude() 
                             << " / " << p2->Longitude() << endl;
    p2->SetCoordinate(0, -rad*sqrt(2));
    cout << "            SetCoordinate(0, -sqrt2*SphRad) returns ";
    cout << "lat/lon = " << p2->Latitude() 
                             << " / " << p2->Longitude() << endl;
    cout << endl;
    cout << endl;

    // points on circle 2*ER in diameter (whole planet map)
    cout << "    Check known values on (almost) whole planet map" << endl;
    // The following was removed due to roundoff errors on prog8 (mac)
    // cout << " FORWARD - Project to opposite side of planet " << endl;
    // p2->SetGround(-40, 79.99999);
    // p2->SetGround(-40,-279.99999);
    // p2->SetGround(-39.99999, 80);
    cout << "        BACKWARD - Project from opposite side of planet " << endl;
    cout << "        For each of these, expect a value near" << endl;
    cout << "            - centerLatitude / centerLongitude+180 "
            "= -40 / 80" << endl;
    cout << std::setprecision(5);
    p2->SetCoordinate(rad*2, 0);
    
    cout << "            SetCoordinate(2*SphRad, 0) returns ";
    cout << "lat/lon = " << p2->Latitude() 
                             << " / " << p2->Longitude() << endl;
    p2->SetCoordinate(0, rad*2);
    cout << "            SetCoordinate(0, 2*SphRad) returns ";
    cout << "lat/lon = " << p2->Latitude() 
                             << " / " << p2->Longitude() << endl;
    p2->SetCoordinate(-rad*2, 0);
    cout << "            SetCoordinate(-2*SphRad, 0) returns ";
    cout << "lat/lon = " << p2->Latitude() 
                             << " / " << p2->Longitude() << endl;
    p2->SetCoordinate(0, -rad*2);
    cout << "            SetCoordinate(0, -2*SphRad) returns ";
    cout << "lat/lon = " << p2->Latitude() 
                             << " / " << p2->Longitude() << endl;
    cout << endl;
    cout << "\t\t\t\t/-----------------------------------------/" << endl;
    cout << "    Testing XYRange method" << endl;
    cout << endl;
    cout << "Given: " << endl;
    cout << std::setprecision(7);
    cout << "    Minimum Latitude:  " << p2->MinimumLatitude() << endl;
    cout << "    Maximum Latitude:  " << p2->MaximumLatitude() << endl;
    cout << "    Minimum Longitude: " << p2->MinimumLongitude() << endl;
    cout << "    Maximum Longitude: " << p2->MaximumLongitude() << endl << endl;
    minX=0; maxX=0; minY=0; maxY=0;
    p2->XYRange(minX, maxX, minY, maxY);
    cout << "XYRange method returns" << endl;
    cout << "    Minimum X:  " << minX << endl;
    cout << "    Maximum X:  " << maxX << endl;
    cout << "    Minimum Y:  " << minY << endl;
    cout << "    Maximum Y:  " << maxY << endl;
    cout << endl;
    p2->SetCoordinate(maxX,0);
    cout << std::setprecision(3);
    cout << "            SetCoordinate(maxX,0) returns lat/lon = " 
         << p2->Latitude() << " / " << p2->Longitude()  << endl;
    p2->SetCoordinate(0, maxY);
    cout << "            SetCoordinate(0,maxY) returns lat/lon = " << 
         p2->Latitude() << " / " << p2->Longitude()  << endl;
    p2->SetCoordinate(minX,0);
    cout << "            SetCoordinate(minX,0) returns lat/lon = " 
         << p2->Latitude() << " / " << p2->Longitude()  << endl;
    p2->SetCoordinate(0, minY);
    cout << "            SetCoordinate(0,minY) returns lat/lon = " 
         << p2->Latitude() << " / " << p2->Longitude()  << endl;
    cout << endl;
    cout << endl;
    mapGroup.findKeyword("MinimumLongitude").setValue("-110.0");
    mapGroup.findKeyword("MaximumLongitude").setValue("70.0");
    TProjection *p2a = (TProjection *) ProjectionFactory::Create(lab);
    cout << std::setprecision(7);
    cout << "Given: " << endl;
    cout << "    Minimum Latitude:  " << p2a->MinimumLatitude() << endl;
    cout << "    Maximum Latitude:  " << p2a->MaximumLatitude() << endl;
    cout << "    Minimum Longitude: " << p2a->MinimumLongitude() << endl;
    cout << "    Maximum Longitude: " << p2a->MaximumLongitude() << endl << endl;
    minX=0; maxX=0; minY=0; maxY=0;
    p2a->XYRange(minX, maxX, minY, maxY);
    cout << "XYRange method returns" << endl;
    cout << "    Minimum X:  " << minX << endl;
    cout << "    Maximum X:  " << maxX << endl;
    cout << "    Minimum Y:  " << minY << endl;
    cout << "    Maximum Y:  " << maxY << endl;
    cout << endl;
    p2a->SetCoordinate(maxX,0);
    cout << "            SetCoordinate(maxX,0) returns lat/lon = " 
         << p2a->Latitude() << " / " << p2a->Longitude()  << endl;
    p2a->SetCoordinate(0, maxY);
    cout << "            SetCoordinate(0,maxY) returns lat/lon = " << 
         p2a->Latitude() << " / " << p2a->Longitude()  << endl;
    p2a->SetCoordinate(minX,0);
    cout << "            SetCoordinate(minX,0) returns lat/lon = " 
         << p2a->Latitude() << " / " << p2a->Longitude()  << endl;
    p2a->SetCoordinate(0, minY);
    cout << "            SetCoordinate(0,minY) returns lat/lon = " 
         << p2a->Latitude() << " / " << p2a->Longitude()  << endl;
    cout << endl;
    cout << endl;
    cout << endl;
    cout << endl;
    //||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||
    cout << border << endl;
    cout << "\t\t\t SPHERICAL-PLANETOGRAPHIC-POSITIVEEAST-SOUTH POLAR-180" << endl;
    cout << border << endl << endl;
    //||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||
    mapGroup.findKeyword("CenterLatitude").setValue("-90.0");
    mapGroup.findKeyword("CenterLongitude").setValue("-96.0");
    mapGroup.findKeyword("LongitudeDomain").setValue("180");
    TProjection *p3 = (TProjection *) ProjectionFactory::Create(lab);
    cout << mapGroup["CenterLatitude"] << endl;
    cout << mapGroup["CenterLongitude"] << endl;
    cout << "EquatorialRadius = " << p3->EquatorialRadius() << endl;
    cout << "PolarRadius = " << p3->PolarRadius() << endl;
    cout << "Eccentricity = " << p3->Eccentricity() << endl;
    cout << "TrueScaleLatitude = " << p3->TrueScaleLatitude() << endl << endl;
    cout << "\t\t\t\t/-----------------------------------------/" << endl;
    cout << endl;
    cout << "    Testing SetGround(-20, 100) from hand calculation" << endl;
    p3->SetGround(-20, 100);
    cout << "Latitude:            " << p3->Latitude() << endl;
    cout << "Longitude:           " << p3->Longitude() << endl;
    cout << "XCoord:              " << p3->XCoord() << endl;
    cout << "YCoord:              " << p3->YCoord() << endl;
    // we do not have relative scale factor methods in Projection as of 06/2012
    // so these need to be tested with a LambertAzimuthalEqualArea object
    // specifically.  The following values were calculated by hand to verify.
    LambertAzimuthalEqualArea lam3(lab);
    lam3.SetGround(-20, 100);
    cout << "RelativeScaleLatitude:  " << lam3.relativeScaleFactorLatitude() << endl;
    cout << "RelativeScaleLongitude: " << lam3.relativeScaleFactorLongitude() << endl;
    cout << endl;
    cout << "\t\t\t\t/-----------------------------------------/" << endl;
    cout << endl;
    cout << "    Testing SetCoordinate(-0.9485946, -3.3081423)" << endl;
    p3->SetCoordinate(-0.9485946, -3.3081423);
    cout << "Latitude:            " << p3->Latitude() << endl;
    cout << "Longitude:           " << p3->Longitude() << endl;
    cout << "XCoord:              " << p3->XCoord() << endl;
    cout << "YCoord:              " << p3->YCoord() << endl;
    cout << endl;
    cout << "\t\t\t\t/-----------------------------------------/" << endl;
    cout << endl;
    cout << "    Testing projection to origin and back\n"
            "    SetGround(-90, -96)" << endl;
    p3->SetGround(-90.0, -96.0);
    cout << "Latitude:            " << p3->Latitude() << endl;
    cout << "Longitude:           " << p3->Longitude() << endl;
    cout << "XCoord:                " << p3->XCoord() << endl;
    cout << "YCoord:                " << p3->YCoord() << endl;
    cout << "    SetCoordinate(0, 0)" << endl;
    p3->SetCoordinate(0.0, 0.0);
    cout << "Latitude:            " << p3->Latitude() << endl;
    cout << "Longitude:           " << p3->Longitude() << endl;
    cout << "XCoord:                " << p3->XCoord() << endl;
    cout << "YCoord:                " << p3->YCoord() << endl;
    cout << endl;
    cout << "\t\t\t\t/-----------------------------------------/" << endl;
    cout << endl;   
    cout << "    Testing projection to north pole and back\n"
            "    SetGround(90, 0)" << endl;
    p3->SetGround(90.0, 0.0);
    cout << "Latitude:            " << p3->Latitude() << endl;
    cout << "Longitude:            " << p3->Longitude() << endl;
    cout << "XCoord:               " << p3->XCoord() << endl;
    cout << "YCoord:              " << p3->YCoord() << endl;
    cout << "    SetCoordinate(2*sphRad, -0.6271708)" << endl;
    p3->SetCoordinate(6.0, -0.6271708);
    cout << "Latitude:            " << p3->Latitude() << endl;
    cout << "Longitude:            " << p3->Longitude() << endl;
    cout << "XCoord:               " << p3->XCoord() << endl;
    cout << "YCoord:              " << p3->YCoord() << endl;
    cout << endl;
    cout << "\t\t\t\t/-----------------------------------------/" << endl;
    cout << endl;   
    // CHECKING OTHER KNOWN POINTS
    //rad = mapGroup.findKeyword("EquatorialRadius");
    rad = p3->LocalRadius(mapGroup.findKeyword("CenterLatitude"));
    cout << "    Testing other known points..." << endl;
    cout << endl;   
    cout << "        Comparison Values" << endl;
    cout << "        SphRad (Spherical Radius) = " << rad << endl;
    cout << "        sqrt2*SphRad              = " << rad*sqrt(2)   << endl;
    cout << "        sqrt2*SphRad*sqrt3/2      = " << rad*sqrt(6)/2 << endl;
    cout << "        sqrt2*SphRad/2            = " << rad*sqrt(2)/2 << endl;
    cout << endl;
    // points on circle sqrt(2)*ER in diameter (hemisphere map)
    cout << "    Check known values on hemispherical map" << endl;
    cout << "        FORWARD" << endl;
    p3->SetGround(0, -6);
    cout << "            SetGround(0, -6) returns ";
    cout << "(x,y) = (" << p3->XCoord() << ", " << p3->YCoord() << ")"<< endl;
    p3->SetGround(0, -96);
    cout << "            SetGround(0, -96) returns ";
    cout << "(x,y) = (" << p3->XCoord() << ", " << p3->YCoord() << ")"<< endl;
    p3->SetGround(0, -186);
    cout << "            SetGround(0, -186) returns ";
    cout << "(x,y) = (" << p3->XCoord() << ", " << p3->YCoord() << ")"<< endl;
    p3->SetGround(0, 84);
    cout << "            SetGround(0, 84) returns ";
    cout << "(x,y) = (" << p3->XCoord() << ", " << p3->YCoord() << ")"<< endl;
    p3->SetGround(0, -36);
    cout << "            SetGround(0, -36) returns ";
    cout << "(x,y) = (" << p3->XCoord() << ", " << p3->YCoord() << ")"<< endl;
    p3->SetGround(0, 129);
    cout << "            SetGround(0, 129) returns ";
    cout << "(x,y) = (" << p3->XCoord() << ", " << p3->YCoord() << ")"<< endl;
    cout << "        BACKWARD" << endl;
    cout << std::setprecision(5);
    p3->SetCoordinate(rad*sqrt(2), 0);
    cout << "            SetCoordinate(sqrt2*SphRad, 0) returns ";
    cout << "lat/lon = " << p3->Latitude()  
                             << " / " << p3->Longitude() << endl;
    p3->SetCoordinate(0, rad*sqrt(2));
    cout << "            SetCoordinate(0, sqrt2*SphRad) returns ";
    cout << "lat/lon = " << p3->Latitude() 
                             << " / " << p3->Longitude() << endl;
    p3->SetCoordinate(-rad*sqrt(2), 0);
    cout << "            SetCoordinate(-sqrt2*SphRad, 0) returns ";
    cout << "lat/lon = " << p3->Latitude() 
                             << " / " << p3->Longitude() << endl;
    p3->SetCoordinate(0, -rad*sqrt(2));
    cout << "            SetCoordinate(0, -sqrt2*SphRad) returns ";
    cout << "lat/lon = " << p3->Latitude() 
                             << " / " << p3->Longitude() << endl;
    p3->SetCoordinate(rad*sqrt(6)/2, rad*sqrt(2)/2);
    cout << "            SetCoordinate(SphRad*sqrt6/2, SphRad*sqrt2/2)"
            " returns lat/lon=" << p3->Latitude() 
                              << " / " << p3->Longitude() << endl;
    p3->SetCoordinate(-3, -3);
    cout << "            SetCoordinate(-SphRad, -SphRad) returns ";
    cout << "lat/lon = " << p3->Latitude() 
                             << " / " << p3->Longitude() << endl;
    cout << endl;
    cout << endl;
    // points on circle 2*ER in diameter (whole planet map)
    cout << "        Comparison Values" << endl;
    cout << std::setprecision(7);
    cout << "        SphRad (Spherical Radius) = " << rad << endl;
    cout << "        2*SphRad*sqrt3/2          = " << rad*sqrt(3)   << endl;
    cout << "        2*SphRad*sqrt2/2          = " << rad*sqrt(2)   << endl;
    cout << endl;
    cout << "    Check known values on whole planet map" << endl;
    cout << "        FORWARD - Project to opposite side of planet " << endl;
    p3->SetGround(90, -6);
    cout << "            SetGround(90, -6) returns ";
    cout << "(x,y) = (" << p3->XCoord() << ", " << p3->YCoord() << ")"<< endl;
    p3->SetGround(90, -96);
    cout << "            SetGround(90, -96) returns ";
    cout << "(x,y) = (" << p3->XCoord() << ", " << p3->YCoord() << ")"<< endl;
    p3->SetGround(90, 174);
    cout << "            SetGround(90, 174) returns ";
    cout << "(x,y) = (" << p3->XCoord() << ", " << p3->YCoord() << ")"<< endl;
    p3->SetGround(90, 84);
    cout << "            SetGround(90, 84) returns ";
    cout << "(x,y) = (" << p3->XCoord() << ", " << p3->YCoord() << ")"<< endl;
    p3->SetGround(90, -36);
    cout << "            SetGround(90, -36) returns ";
    cout << "(x,y) = (" << p3->XCoord() << ", " << p3->YCoord() << ")"<< endl;
    p3->SetGround(90, 129);
    cout << "            SetGround(90, 129) returns ";
    cout << "(x,y) = (" << p3->XCoord() << ", " << p3->YCoord() << ")"<< endl;
    cout << "        BACKWARD - Project from opposite side of planet " << endl;
    cout << std::setprecision(5);
    p3->SetCoordinate(rad*2, 0);
    cout << "            SetCoordinate(2*SphRad, 0) returns ";
    cout << "lat/lon = " << p3->Latitude() 
                             << " / " << p3->Longitude() << endl;
    p3->SetCoordinate(0, rad*2);
    cout << "            SetCoordinate(0, 2*SphRad) returns ";
    cout << "lat/lon = " << p3->Latitude() 
                             << " / " << p3->Longitude() << endl;
    p3->SetCoordinate(-rad*2, 0);
    cout << "            SetCoordinate(-2*SphRad, 0) returns ";
    cout << "lat/lon = " << p3->Latitude() 
                             << " / " << p3->Longitude() << endl;
    p3->SetCoordinate(0, -rad*2);
    cout << "            SetCoordinate(0, -2*SphRad) returns ";
    cout << "lat/lon = " << p3->Latitude() << " / " << p3->Longitude() << endl;
    p3->SetCoordinate(rad*sqrt(3), rad);
    cout << "            SetCoordinate(2*SphRad*sqrt3/2, 2*SphRad*1/2) "
            "returns lat/lon=" << p3->Latitude() 
                             << " / " << p3->Longitude() << endl;
    p3->SetCoordinate(-rad*sqrt(2), -rad*sqrt(2));
    cout << "            SetCoordinate(-SphRad*sqrt2, -SphRad*sqrt2) "
            "returns lat/lon = " << p3->Latitude() 
                             << " / " << p3->Longitude() << endl;
    cout << endl;
    cout << "\t\t\t\t/-----------------------------------------/" << endl;
    cout << endl;
    cout << "    Testing XYRange method" << endl;
    cout << "Given: " << endl;
    cout << std::setprecision(7);
    cout << "    Minimum Latitude:  " << p3->MinimumLatitude() << endl;
    cout << "    Maximum Latitude:  " << p3->MaximumLatitude() << endl;
    cout << "    Minimum Longitude: " << p3->MinimumLongitude() << endl;
    cout << "    Maximum Longitude: " << p3->MaximumLongitude() << endl << endl;
    minX=0; maxX=0; minY=0; maxY=0;
    p3->XYRange(minX, maxX, minY, maxY);
    cout << "XYRange method returns" << endl;
    cout << "    Minimum X:  " << minX << endl;
    cout << "    Maximum X:  " << maxX << endl;
    cout << "    Minimum Y:  " << minY << endl;
    cout << "    Maximum Y:  " << maxY << endl;
    cout << endl;
    p3->SetCoordinate(maxX,0);
    cout << "            SetCoordinate(maxX,0) returns lat/lon = " 
         << p3->Latitude() << " / " << p3->Longitude()  << endl;
    p3->SetCoordinate(0, maxY);
    cout << "            SetCoordinate(0,maxY) returns lat/lon = " 
         << p3->Latitude() << " / " << p3->Longitude()  << endl;
    p3->SetCoordinate(minX,0);
    cout << "            SetCoordinate(minX,0) returns lat/lon = " 
         << p3->Latitude() << " / " << p3->Longitude()  << endl;
    p3->SetCoordinate(0, minY);
    cout << "            SetCoordinate(0,minY) returns lat/lon = " 
         << p3->Latitude() << " / " << p3->Longitude()  << endl;
    p3->SetGround(90, 90);
    cout << "To check MinimumX: SetGround(90, 90) returns ";
    cout << "(x,y) = (" << p3->XCoord() << ", " << p3->YCoord() << ")"<< endl;
    p3->SetGround(90, -90);
    cout << "To check MaximumY: SetGround(90, -90) returns ";
    cout << "(x,y) = (" << p3->XCoord() << ", " << p3->YCoord() << ")"<< endl;
    cout << endl;
    cout << endl;
    mapGroup.findKeyword("MinimumLatitude").setValue("-90.0");
    mapGroup.findKeyword("MaximumLatitude").setValue("0.0");
    mapGroup.findKeyword("MinimumLongitude").setValue("-336.0");//24
    mapGroup.findKeyword("MaximumLongitude").setValue("-66.0");//294
    TProjection *p3a = (TProjection *) ProjectionFactory::Create(lab);
    cout << "Given: " << endl;
    cout << "    Minimum Latitude:  " << p3a->MinimumLatitude() << endl;
    cout << "    Maximum Latitude:  " << p3a->MaximumLatitude() << endl;
    cout << "    Minimum Longitude: " << p3a->MinimumLongitude() << endl;
    cout << "    Maximum Longitude: " << p3a->MaximumLongitude() << endl << endl;
    minX=0; maxX=0; minY=0; maxY=0;
    p3a->XYRange(minX, maxX, minY, maxY);
    cout << "XYRange method returns" << endl;
    cout << "    Minimum X:  " << minX << endl;
    cout << "    Maximum X:  " << maxX << endl;
    cout << "    Minimum Y:  " << minY << endl;
    cout << "    Maximum Y:  " << maxY << endl;
    cout << endl;
    cout << "        Comparison Values" << endl;
    cout << "        SphRad (Spherical Radius) = " << rad << endl;
    cout << "        maxX:    sqrt2*SphRad*sqrt3/2  = " << rad*sqrt(6)/2 << endl;
    cout << "        others:  sqrt2*SphRad          = " << rad*sqrt(2) << endl;
    cout << endl;
    cout << "        For y = -sqrt2*SphRad/2 = " 
         << -rad*sqrt(2)/2 << endl;
    p3a->SetCoordinate(maxX,-sqrt(2*rad*rad-maxX*maxX));
    cout << "            SetCoordinate(maxX, y) returns lat/lon = " 
         << p3a->Latitude() << " / " << p3a->Longitude()  << endl;
    p3a->SetCoordinate(0, maxY);
    cout << "            SetCoordinate(0,maxY) returns lat/lon = " << 
         p3a->Latitude() << " / " << p3a->Longitude()  << endl;
    p3a->SetCoordinate(minX,0);
    cout << "            SetCoordinate(minX,0) returns lat/lon = " 
         << p3a->Latitude() << " / " << p3a->Longitude()  << endl;
    p3a->SetCoordinate(0, minY);
    cout << "            SetCoordinate(0,minY) returns lat/lon = " 
         << p3a->Latitude() << " / " << p3a->Longitude()  << endl;
    cout << endl;
    cout << endl;
    mapGroup.findKeyword("MinimumLongitude").setValue("-66.0");
    mapGroup.findKeyword("MaximumLongitude").setValue("-51.0");
    TProjection *p3b = (TProjection *) ProjectionFactory::Create(lab);
    cout << "Given: " << endl;
    cout << "    Minimum Latitude:  " << p3b->MinimumLatitude() << endl;
    cout << "    Maximum Latitude:  " << p3b->MaximumLatitude() << endl;
    cout << "    Minimum Longitude: " << p3b->MinimumLongitude() << endl;
    cout << "    Maximum Longitude: " << p3b->MaximumLongitude() << endl << endl;
    minX=0; maxX=0; minY=0; maxY=0;
    p3b->XYRange(minX, maxX, minY, maxY);
    cout << "XYRange method returns" << endl;
    cout << "    Minimum X:  " << minX << endl;
    cout << "    Maximum X:  " << maxX << endl;
    cout << "    Minimum Y:  " << minY << endl;
    cout << "    Maximum Y:  " << maxY << endl;
    cout << endl;
    cout << "        Comparison Values" << endl;
    cout << "        SphRad (Spherical Radius)   = " << rad << endl;
    cout << "        maxX:  sqrt2*SphRad*sqrt2/2 = " << rad << endl;
    cout << "        maxY:  sqrt2*SphRad*sqrt3/2 = " << rad*sqrt(6)/2 << endl;
    cout << endl;
    p3b->SetCoordinate(maxX,rad);
    cout << "            SetCoordinate(maxX, SphRad) returns lat/lon = " 
         << p3b->Latitude() << " / " << p3b->Longitude()  << endl;
    cout << "For x = sqrt2*SphRad/2 = " 
         << rad*sqrt(2)/2 << endl;
    p3b->SetCoordinate(rad*sqrt(2)/2, maxY);
    cout << "            SetCoordinate(0,maxY) returns lat/lon = " << 
         p3b->Latitude() << " / " << p3b->Longitude()  << endl;
    p3b->SetCoordinate(minX,0);
    cout << "            SetCoordinate(minX,0) returns lat/lon = " 
         << p3b->Latitude() << " / " << p3b->Longitude()  << endl;
    p3b->SetCoordinate(0, minY);
    cout << "            SetCoordinate(0,minY) returns lat/lon = " 
         << p3b->Latitude() << " / " << p3b->Longitude()  << endl;
    cout << endl;
    cout << endl;
    cout << endl;
    cout << endl;
    //||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||
    cout << border << endl;
    cout << "\t\t\t SPHERICAL-PLANETOCENTRIC-POSITIVEWEST-NORTH POLAR-180" << endl;
    cout << border << endl << endl;
    //||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||
    mapGroup.findKeyword("CenterLatitude").setValue("90.0");
    mapGroup.findKeyword("LatitudeType").setValue("Planetocentric");
    mapGroup.findKeyword("LongitudeDirection").setValue("PositiveWest");
    mapGroup.findKeyword("MinimumLatitude").setValue("-89.99999");
    mapGroup.findKeyword("MaximumLatitude").setValue("0.0");
    mapGroup.findKeyword("MinimumLongitude").setValue("-51.0");
    mapGroup.findKeyword("MaximumLongitude").setValue("129.0");
    TProjection *p4 = (TProjection *) ProjectionFactory::Create(lab);
    cout << mapGroup["CenterLatitude"] << endl;
    cout << mapGroup["CenterLongitude"] << endl;
    cout << "EquatorialRadius = " << p4->EquatorialRadius() << endl;
    cout << "PolarRadius = " << p4->PolarRadius() << endl;
    cout << "Eccentricity = " << p4->Eccentricity() << endl;
    cout << "TrueScaleLatitude = " << p4->TrueScaleLatitude() << endl << endl;
    cout << "\t\t\t\t/-----------------------------------------/" << endl;
    cout << endl;
    cout << "    Testing SetGround(-20, 100) hand calculation" << endl;
    p4->SetGround(-20, 100);
    cout << "Latitude:            " << p4->Latitude() << endl;
    cout << "Longitude:           " << p4->Longitude() << endl;
    cout << "XCoord:              " << p4->XCoord() << endl;
    cout << "YCoord:              " << p4->YCoord() << endl;
    LambertAzimuthalEqualArea lam4(lab);
    lam4.SetGround(-20, 100);
    // we do not have relative scale factor methods in Projection as of 06/2012
    // so these need to be tested with a LambertAzimuthalEqualArea object
    // specifically.  The following values were calculated by hand to verify.
    cout << "RelativeScaleLatitude:  " << lam4.relativeScaleFactorLatitude() << endl;
    cout << "RelativeScaleLongitude: " << lam4.relativeScaleFactorLongitude() << endl;
    cout << endl;
    cout << "\t\t\t\t/-----------------------------------------/" << endl;
    cout << endl;
    cout << "    Testing SetCoordinate(1.3547334, 4.7245169)" << endl;
    p4->SetCoordinate(1.3547334, 4.7245169);
    cout << "Latitude:            " << p4->Latitude() << endl;
    cout << "Longitude:           " << p4->Longitude() << endl;
    cout << "XCoord:              " << p4->XCoord() << endl;
    cout << "YCoord:              " << p4->YCoord() << endl;
    cout << endl;
    cout << "\t\t\t\t/-----------------------------------------/" << endl;
    cout << endl;
    cout << "    Testing projection to origin and back\n"
            "    SetGround(90, -96)" << endl;
    p4->SetGround(90.0, -96.0);
    cout << "Latitude:             " << p4->Latitude() << endl;
    cout << "Longitude:           " << p4->Longitude() << endl;
    cout << "XCoord:               " << p4->XCoord() << endl;
    cout << "YCoord:                " << p4->YCoord() << endl;
    cout << "    SetCoordinate(0, 0)" << endl;
    p4->SetCoordinate(0.0, 0.0);
    cout << "Latitude:             " << p4->Latitude() << endl;
    cout << "Longitude:           " << p4->Longitude() << endl;
    cout << "XCoord:                " << p4->XCoord() << endl;
    cout << "YCoord:                " << p4->YCoord() << endl;
    cout << endl;
    cout << "\t\t\t\t/-----------------------------------------/" << endl;
    cout << endl;   
    cout << "    Testing projection to south pole and back\n"
            "    SetGround(-90, 0)" << endl;
    p4->SetGround(-90.0, 0.0);
    cout << "Latitude:            " << p4->Latitude() << endl;
    cout << "Longitude:             " << p4->Longitude() << endl;
    cout << "XCoord:               " << p4->XCoord() << endl;
    cout << "YCoord:                " << p4->YCoord() << endl;
    cout << "    SetCoordinate(-2*sphRad, -0.6271708)" << endl;
    p4->SetCoordinate(6.0, -0.6271708);
    cout << "Latitude:            " << p4->Latitude() << endl;
    cout << "Longitude:             " << p4->Longitude() << endl;
    cout << "XCoord:                " << p4->XCoord() << endl;
    cout << "YCoord:               " << p4->YCoord() << endl;
    cout << endl;
    cout << "\t\t\t\t/-----------------------------------------/" << endl;
    cout << endl;   
    // CHECKING OTHER KNOWN POINTS
    //rad = mapGroup.findKeyword("EquatorialRadius");
    rad = p4->LocalRadius(mapGroup.findKeyword("CenterLatitude"));
    cout << "    Testing other known points..." << endl;
    cout << endl;   
    cout << "        Comparison Values" << endl;
    cout << "        SphRad (Spherical Radius) = " << rad << endl;
    cout << "        sqrt2*SphRad              = " << rad*sqrt(2) << endl;
    cout << endl;
    // points on circle sqrt(2)*ER in diameter (hemisphere map)
    cout << "    Check known values on hemispherical map" << endl;
    cout << "        FORWARD" << endl;
    p4->SetGround(0, -186);
    cout << "            SetGround(0, -186) returns ";
    cout << "(x,y) = (" << p4->XCoord() << ", " << p4->YCoord() << ")"<< endl;
    p4->SetGround(0, 84);
    cout << "            SetGround(0, 84) returns ";
    cout << "(x,y) = (" << p4->XCoord() << ", " << p4->YCoord() << ")"<< endl;
    p4->SetGround(0, -6);
    cout << "            SetGround(0, -6) returns ";
    cout << "(x,y) = (" << p4->XCoord() << ", " << p4->YCoord() << ")"<< endl;
    p4->SetGround(0, -96);
    cout << "            SetGround(0, -96) returns ";
    cout << "(x,y) = (" << p4->XCoord() << ", " << p4->YCoord() << ")"<< endl;
    p4->SetGround(0, 129);
    cout << "            SetGround(0, 129) returns ";
    cout << "(x,y) = (" << p4->XCoord() << ", " << p4->YCoord() << ")"<< endl;
    cout << "        BACKWARD" << endl;
    cout << std::setprecision(5);
    p4->SetCoordinate(rad*sqrt(2), 0);
    cout << "            SetCoordinate(sqrt2*SphRad, 0) returns ";
    cout << "lat/lon = " << p4->Latitude() 
                             << " / " << p4->Longitude() << endl;
    p4->SetCoordinate(0, rad*sqrt(2));
    cout << "            SetCoordinate(0, sqrt2*SphRad) returns ";
    cout << "lat/lon = " << p4->Latitude() 
                             << " / " << p4->Longitude() << endl;
    p4->SetCoordinate(-rad*sqrt(2), 0);
    cout << "            SetCoordinate(-sqrt2*SphRad, 0) returns ";
    cout << "lat/lon = " << p4->Latitude() 
                             << " / " << p4->Longitude() << endl;
    p4->SetCoordinate(0, -rad*sqrt(2));
    cout << "            SetCoordinate(0, -sqrt2*SphRad) returns ";
    cout << "lat/lon = " << p4->Latitude() 
                             << " / " << p4->Longitude() << endl;
    p4->SetCoordinate(rad, rad);
    cout << "            SetCoordinate(SphRad, SphRad) returns ";
    cout << "lat/lon = " << p4->Latitude() 
                             << " / " << p4->Longitude() << endl;
    cout << endl;
    cout << endl;
    // points on circle 2*ER in diameter (whole planet map)
    cout << "        Comparison Values" << endl;
    cout << std::setprecision(7);
    cout << "        SphRad (Spherical Radius) = " << rad << endl;
    cout << "        2*SphRad*sqrt3/2          = " << rad*sqrt(3)   << endl;
    cout << endl;
    cout << "    Check known values on whole planet map" << endl;
    cout << "        FORWARD - Project to opposite side of planet " << endl;
    p4->SetGround(-90, 174);
    cout << "            SetGround(-90, 174) returns ";
    cout << "(x,y) = (" << p4->XCoord() << ", " << p4->YCoord() << ")"<< endl;
    p4->SetGround(-90, 84);
    cout << "            SetGround(-90, 84) returns ";
    cout << "(x,y) = (" << p4->XCoord() << ", " << p4->YCoord() << ")"<< endl;
    p4->SetGround(-90, -6);
    cout << "            SetGround(-90, -6) returns ";
    cout << "(x,y) = (" << p4->XCoord() << ", " << p4->YCoord() << ")"<< endl;
    p4->SetGround(-90, -96);
    cout << "            SetGround(-90, -96) returns ";
    cout << "(x,y) = (" << p4->XCoord() << ", " << p4->YCoord() << ")"<< endl;
    p4->SetGround(-90, -126);
    cout << "            SetGround(-90, -126) returns ";
    cout << "(x,y) = (" << p4->XCoord() << ", " << p4->YCoord() << ")"<< endl;
    cout << "        BACKWARD - Project from opposite side of planet " << endl;
    cout << std::setprecision(5);
    p4->SetCoordinate(rad*2, 0);
    cout << "            SetCoordinate(2*SphRad, 0) returns ";
    cout << "lat/lon = " << p4->Latitude() 
                             << " / " << p4->Longitude() << endl;
    p4->SetCoordinate(0, rad*2);
    cout << "            SetCoordinate(0, 2*SphRad) returns ";
    cout << "lat/lon = " << p4->Latitude() 
                             << " / " << p4->Longitude() << endl;
    p4->SetCoordinate(-rad*2, 0);
    cout << "            SetCoordinate(-2*SphRad, 0) returns ";
    cout << "lat/lon = " << p4->Latitude() 
                             << " / " << p4->Longitude() << endl;
    p4->SetCoordinate(0, -rad*2);
    cout << "            SetCoordinate(0, -2*SphRad) returns ";
    cout << "lat/lon = " << p4->Latitude() 
                             << " / " << p4->Longitude() << endl;
    p4->SetCoordinate(rad, -rad*sqrt(3));
    cout << "            SetCoordinate(SphRad, -SphRad*sqrt3) returns ";
    cout << "lat/lon = " << p4->Latitude() 
                             << " / " << p4->Longitude() << endl;
    cout << endl;
    cout << "\t\t\t\t/-----------------------------------------/" << endl;
    cout << endl;
    cout << "    Testing XYRange method " << endl;
    cout << "Given: " << endl;
    cout << std::setprecision(7);// so all of the 9's are visible for minlat
    cout << "    Minimum Latitude:  " << p4->MinimumLatitude() << endl;
    cout << "    Maximum Latitude:  " << p4->MaximumLatitude() << endl;
    cout << "    Minimum Longitude: " << p4->MinimumLongitude() << endl;
    cout << "    Maximum Longitude: " << p4->MaximumLongitude() << endl << endl;
    minX=0; maxX=0; minY=0; maxY=0;
    p4->XYRange(minX, maxX, minY, maxY);
    cout << "XYRange method returns" << endl;
    cout << "    Minimum X:  " << minX << endl;
    cout << "    Maximum X:  " << maxX << endl;
    cout << "    Minimum Y:  " << minY << endl;
    cout << "    Maximum Y:  " << maxY << endl;
    cout << endl;
    cout << "        Comparison Values" << endl;
    cout << "        SphRad (Spherical Radius)  = " << rad << endl;
    cout << "        maxX, -minY:  sqrt2*SphRad = " << rad*sqrt(2) << endl;
    cout << "        -minX, maxY:  2*SphRad     = " << 2*rad << endl;
    cout << endl;   
    cout << std::setprecision(5);
    p4->SetCoordinate(maxX,maxX);
    cout << "            SetCoordinate(maxX,sqrt2*SphRad) returns lat/lon = " 
         << p4->Latitude() << " / " << p4->Longitude()  << endl;
    p4->SetCoordinate(0, maxY);
    cout << "            SetCoordinate(0,maxY) returns lat/lon = " 
         << p4->Latitude() << " / " << p4->Longitude()  << endl;
    p4->SetCoordinate(minX,0);
    cout << "            SetCoordinate(minX,0) returns lat/lon = " << 
         p4->Latitude() << " / " << p4->Longitude()  << endl;
    p4->SetCoordinate(minY, minY);
    cout << "            SetCoordinate(sqrt2*SphRad,minY) returns lat/lon = " << 
         p4->Latitude() << " / " << p4->Longitude()  << endl;
    p4->SetGround(-90, 90);
    cout << endl;
    cout << endl;
    mapGroup.findKeyword("MinimumLongitude").setValue("-6.0");
    mapGroup.findKeyword("MaximumLongitude").setValue("24.0");
    TProjection *p4a = (TProjection *) ProjectionFactory::Create(lab);
    cout << "Given: " << endl;
    cout << std::setprecision(7);// so all of the 9's are visible for minlat
    cout << "    Minimum Latitude:  " << p4a->MinimumLatitude() << endl;
    cout << "    Maximum Latitude:  " << p4a->MaximumLatitude() << endl;
    cout << "    Minimum Longitude: " << p4a->MinimumLongitude() << endl;
    cout << "    Maximum Longitude: " << p4a->MaximumLongitude() << endl << endl;
    minX=0; maxX=0; minY=0; maxY=0;
    p4a->XYRange(minX, maxX, minY, maxY);
    cout << "XYRange method returns" << endl;
    cout << "    Minimum X:  " << minX << endl;
    cout << "    Maximum X:  " << maxX << endl;
    cout << "    Minimum Y:  " << minY << endl;
    cout << "    Maximum Y:  " << maxY << endl;
    cout << endl;
    cout << "        Comparison Values" << endl;
    cout << "        SphRad (Spherical Radius)    = " << rad << endl;
    cout << "        maxX:  -sqrt2*SphRad*sqrt3/2 = " << -rad*sqrt(6)/2 << endl;
    cout << "        maxY:  2*SphRad/2            = " << rad << endl;
    cout << "        minX:  -2*SphRad             = " << -2*rad << endl;
    cout << endl;
    cout << std::setprecision(5);
    p4a->SetCoordinate(maxX,sqrt(2)*rad/2);
    double lat = p4a->Latitude();
    if (lat <= 1E-13) {
      lat = 0.0;
    }
    cout << "            SetCoordinate(maxX, sqrt2*SphRad/2) returns lat/lon = " 
         << lat << " / " << p4a->Longitude()  << endl;
    p4a->SetCoordinate(-rad*sqrt(3), maxY);
    cout << "            SetCoordinate(-2*SphRad*sqrt3/2, maxY) returns lat/lon = " << 
         p4a->Latitude() << " / " << p4a->Longitude()  << endl;
    p4a->SetCoordinate(minX,0);
    cout << "            SetCoordinate(minX,0) returns lat/lon = " 
         << p4a->Latitude() << " / " << p4a->Longitude()  << endl;
    p4a->SetCoordinate(0, minY);
    cout << "            SetCoordinate(0,minY) returns lat/lon = " 
         << p4a->Latitude() << " / " << p4a->Longitude()  << endl;
    cout << endl;
    cout << endl;
    mapGroup.findKeyword("MinimumLatitude").setValue("-90.0");
    TProjection *p4b = (TProjection *) ProjectionFactory::Create(lab);
    cout << "Given: " << endl;
    cout << std::setprecision(7);
    cout << "    Minimum Latitude:  " << p4b->MinimumLatitude() << endl;
    cout << "    Maximum Latitude:  " << p4b->MaximumLatitude() << endl;
    cout << "    Minimum Longitude: " << p4b->MinimumLongitude() << endl;
    cout << "    Maximum Longitude: " << p4b->MaximumLongitude() << endl << endl;
    minX=0; maxX=0; minY=0; maxY=0;
    p4b->XYRange(minX, maxX, minY, maxY);
    cout << "XYRange method returns" << endl;
    cout << "    Minimum X:  " << minX << endl;
    cout << "    Maximum X:  " << maxX << endl;
    cout << "    Minimum Y:  " << minY << endl;
    cout << "    Maximum Y:  " << maxY << endl;
    cout << endl;
    cout << "        Comparison Values" << endl;
    cout << "        2*SphRad (Spherical Radius)    = " << 2*rad << endl;
    cout << endl;
    cout << std::setprecision(5);
    p4b->SetCoordinate(maxX, 0);
    cout << "            SetCoordinate(maxX, 0) returns lat/lon = " 
         << p4b->Latitude() << " / " << p4b->Longitude()  << endl;
    p4b->SetCoordinate(0, maxY);
    cout << "            SetCoordinate(0, maxY) returns lat/lon = " << 
         p4b->Latitude() << " / " << p4b->Longitude()  << endl;
    p4b->SetCoordinate(minX, 0);
    cout << "            SetCoordinate(minX, 0) returns lat/lon = " 
         << p4b->Latitude() << " / " << p4b->Longitude()  << endl;
    p4b->SetCoordinate(0, minY);
    cout << "            SetCoordinate(0, minY) returns lat/lon = " 
         << p4b->Latitude() << " / " << p4b->Longitude()  << endl;
    cout << endl;
    cout << endl;
    cout << endl;
    cout << endl;
    //||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||
    cout << border << endl;
    cout << "\t\t \t ELLIPSOIDAL-PLANETOGRAPHIC-POSITIVEEAST-NORTH POLAR-180" <<endl;
    cout << border << endl << endl;
    //||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||
    mapGroup.findKeyword("LatitudeType").setValue("Planetographic");
    mapGroup.findKeyword("LongitudeDirection").setValue("PositiveEast");
    mapGroup.deleteKeyword("EquatorialRadius");
    mapGroup += PvlKeyword("EquatorialRadius", "6378388.0");
    mapGroup.findKeyword("PolarRadius").setValue(toString(6378388.0*sqrt(1-.00672267)));
    mapGroup.findKeyword("MinimumLatitude").setValue("-89.99999");
    mapGroup.findKeyword("CenterLongitude").setValue("-100.0");
    mapGroup.findKeyword("MinimumLongitude").setValue("-100.0");        
    mapGroup.findKeyword("MaximumLongitude").setValue("215.0");         
    // we do not have relative scale factor methods in Projection as of 06/2012
    // so these need to be tested with a LambertAzimuthalEqualArea object
    // specifically.  The following values were compared to the table in Snyder
    LambertAzimuthalEqualArea p5Lamb(lab);
    cout << std::setprecision(6);
    cout << endl;
    cout << "\t\t\t\t/-----------------------------------------/" << endl;
    cout << endl;
    cout << "    Testing SetGround method to find the relative scale factors "
            "along the latitudes (k) and along the longitudes (h)."
            "\nThis table also includes a radius value that represents radius "
            "on the projected disk at the computed point. That is, the "
            "distance from the pole (center latitude) to the (x,y) coordinate."
            "\nSee Snyder Table 29, page 190" << endl;

    cout << "Latitude" << "\t" << "Projection Radius" << "\t" << "h" << "\t\t\t" << "k" << endl;
    p5Lamb.SetGround(90, -100);
    double projDiskRadius = sqrt(p5Lamb.XCoord()*p5Lamb.XCoord() 
                                 + p5Lamb.YCoord()*p5Lamb.YCoord());
    cout << std::setprecision(1);
    cout << IString(90.0) << "\t\t\t " << projDiskRadius ;
    cout << std::setprecision(6);
    cout << "\t\t\t" << p5Lamb.relativeScaleFactorLongitude()
         << "\t" << p5Lamb.relativeScaleFactorLatitude() << endl;
    for (double lat = 89; lat >= 70; lat-=1) {
      p5Lamb.SetGround(lat, 0);
      projDiskRadius = sqrt(p5Lamb.XCoord()*p5Lamb.XCoord() 
                                   + p5Lamb.YCoord()*p5Lamb.YCoord());
      cout << std::setprecision(1);
      cout << IString(lat) << "\t\t" << projDiskRadius ;
      cout << std::setprecision(6);
      cout << "\t\t\t" << p5Lamb.relativeScaleFactorLongitude()
           << "\t" << p5Lamb.relativeScaleFactorLatitude() << endl;
    }
    cout << endl;
    cout << "\t\t\t\t/-----------------------------------------/" << endl;
    cout << endl;
    //||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||
    //||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||
    TProjection *p5 = (TProjection *) ProjectionFactory::Create(lab);
    cout << std::setprecision(7);
    cout << mapGroup["CenterLatitude"] << endl;
    cout << mapGroup["CenterLongitude"] << endl;
    cout << "EquatorialRadius = " << p5->EquatorialRadius() << endl;
    cout << "PolarRadius = " << p5->PolarRadius() << endl;
    cout << "Eccentricity = " << p5->Eccentricity() << endl;
    cout << "TrueScaleLatitude = " << p5->TrueScaleLatitude() << endl << endl;
    cout << "\t\t\t\t/-----------------------------------------/" << endl;
    cout << endl;
    cout << "    Testing SetGround(80, 5) from Snyder pages 334-335" << endl;
    p5->SetGround(80, 5);
    cout << "Latitude:            " << p5->Latitude() << endl;
    cout << "Longitude:           " << p5->Longitude() << endl;
    cout << "XCoord:              " << p5->XCoord() << endl;
    cout << "YCoord:              " << p5->YCoord() << endl;
    LambertAzimuthalEqualArea lam5(lab);
    lam5.SetGround(80, 5);
    cout << "RelativeScaleLatitude:  " << lam5.relativeScaleFactorLatitude() << endl;
    cout << "RelativeScaleLongitude: " << lam5.relativeScaleFactorLongitude() << endl;
    cout << endl;
    cout << "\t\t\t\t/-----------------------------------------/" << endl;
    cout << endl;
    cout << "    Testing SetCoordinate(1077459.7, 288704.5) "
            "from Snyder pages 336-337" << endl;
    p5->SetCoordinate(1077459.7, 288704.5);
    cout << "Latitude:            " << p5->Latitude() << endl;
    cout << "Longitude:           " << p5->Longitude() << endl;
    cout << "XCoord:              " << p5->XCoord() << endl;
    cout << "YCoord:              " << p5->YCoord() << endl;
    cout << endl;
    cout << "\t\t\t\t/-----------------------------------------/" << endl;
    cout << endl;
    cout << "    Testing projection to origin and back\n"
            "    SetGround(90, -100)" << endl;
    p5->SetGround(90.0, -100.0);
    cout << "Latitude:              " << p5->Latitude() << endl;
    cout << "Longitude:           " << p5->Longitude() << endl;
    cout << "XCoord:                 " << p5->XCoord() << endl;
    cout << "YCoord:                " << p5->YCoord() << endl;
    cout << "    SetCoordinate(0, 0)" << endl;
    p5->SetCoordinate(0.0, 0.0);
    cout << "Latitude:              " << p5->Latitude() << endl;
    cout << "Longitude:             " << p5->Longitude() << endl;
    cout << "XCoord:                 " << p5->XCoord() << endl;
    cout << "YCoord:                 " << p5->YCoord() << endl;
    cout << endl;
    cout << "\t\t\t\t/-----------------------------------------/" << endl;
    cout << endl;   
    cout << "    Testing projection to south pole and back\n"
            "    SetGround(-90, 0)" << endl;
    p5->SetGround(-90.0, 0.0);
    cout << "Latitude:                 " << p5->Latitude() << endl;
    cout << "Longitude:                  " << p5->Longitude() << endl;
    cout << "XCoord:              " << p5->XCoord() << endl;
    cout << "YCoord:               " << p5->YCoord() << endl;
    cout << "    SetCoordinate(12548868.8927037, 2212704.1631568)" << endl;
    p5->SetCoordinate(12548868.8927037, 2212704.1631568);
    cout << "Latitude:                 " << p5->Latitude() << endl;
    cout << "Longitude:                 " << p5->Longitude() << endl;
    cout << "XCoord:              " << p5->XCoord() << endl;
    cout << "YCoord:               " << p5->YCoord() << endl;
    cout << endl;
    cout << "\t\t\t\t/-----------------------------------------/" << endl;
    cout << endl;   
    // CHECKING OTHER KNOWN POINTS
    // dist to equator = sqrt(ER^2 + PR^2)
    // This ellipsoidal projection estimates:
    //     d = sqrt(ER^2 + PR^2*factor)
    //     where factor = ln( (1+e) / (1-e) ) / (2e)
    double poleToEquator = sqrt(p5->EquatorialRadius() * p5->EquatorialRadius()
                                + p5->PolarRadius() * p5->PolarRadius()
                                  *1/(2*p5->Eccentricity())
                                  *log((1+p5->Eccentricity())
                                        /(1-p5->Eccentricity())));
    rad = p5->PolarRadius();
    cout << "    Testing other known points..." << endl;
    cout << endl;   
    cout << "        Comparison Values" << endl;
    cout << "        poleToEquator = " << poleToEquator << endl;
    cout << endl;   
    // points on circle sqrt(2)*ER in diameter (hemisphere map)
    cout << "    Check known values on hemispherical map" << endl;
    cout << "        FORWARD" << endl;
    p5->SetGround(0, -10);
    cout << "            SetGround(0, -10) returns ";
    cout << "(x,y) = (" << p5->XCoord() << ", " << p5->YCoord() << ")"<< endl;
    p5->SetGround(0, 80);
    cout << "            SetGround(0, 80) returns ";
    cout << "(x,y) = (" << p5->XCoord() << ", " << p5->YCoord() << ")"<< endl;
    p5->SetGround(0, -190);
    cout << "            SetGround(0, -190) returns ";
    cout << "(x,y) = (" << p5->XCoord() << ", " << p5->YCoord() << ")"<< endl;
    p5->SetGround(0, -100);
    cout << "            SetGround(0, -100) returns ";
    cout << "(x,y) = (" << p5->XCoord() << ", " << p5->YCoord() << ")"<< endl;
    cout << "        BACKWARD" << endl;
    p5->SetCoordinate(poleToEquator, 0);
    cout << "            SetCoordinate(poleToEquator, 0) returns ";
    cout << "lat/lon = " << p5->Latitude() << " / " << p5->Longitude() << endl;
    p5->SetCoordinate(0, poleToEquator);
    cout << "            SetCoordinate(0, poleToEquator) returns ";
    cout << "lat/lon = " << p5->Latitude() << " / " << p5->Longitude() << endl;
    p5->SetCoordinate(-poleToEquator, 0);
    cout << "            SetCoordinate(-poleToEquator, 0) returns ";
    cout << "lat/lon = " << p5->Latitude() << " / " << p5->Longitude() << endl;
    p5->SetCoordinate(0, -poleToEquator);
    cout << "            SetCoordinate(0, -poleToEquator) returns ";
    cout << "lat/lon = " << p5->Latitude() << " / " << p5->Longitude() << endl;
    cout << endl;
    // points on whole planet map
    double poleToPole = poleToEquator*sqrt(2);
    cout << "    Check known values on whole planet map" << endl;
    cout << endl;
    cout << "        Comparison Values" << endl;
    cout << "        poleToOppositePole = " << poleToPole << endl;
    cout << endl;
    cout << "        FORWARD - Project to opposite side of planet " << endl;
    p5->SetGround(-90, 80);
    cout << "            SetGround(-90, 80) returns ";
    cout << "(x,y) = (" << p5->XCoord() << ", " << p5->YCoord() << ")"<< endl;
    p5->SetGround(-90, -280);
    cout << "            SetGround(-90, -280) returns ";
    cout << "(x,y) = (" << p5->XCoord() << ", " << p5->YCoord() << ")"<< endl;
    cout << "        BACKWARD - Project from opposite side of planet " << endl;
    p5->SetCoordinate(poleToPole, 0);
    cout << "            SetCoordinate(poleToOppositePole, 0) returns ";
    cout << "lat/lon = " << p5->Latitude() << " / " << p5->Longitude() << endl;
    p5->SetCoordinate(0, poleToPole);
    cout << "            SetCoordinate(0, poleToOppositePole) returns ";
    cout << "lat/lon = " << p5->Latitude() << " / " << p5->Longitude() << endl;
    p5->SetCoordinate(-poleToPole, 0);
    cout << "            SetCoordinate(-poleToOppositePole, 0) returns ";
    cout << "lat/lon = " << p5->Latitude() << " / " << p5->Longitude() << endl;
    p5->SetCoordinate(0, -poleToPole);
    cout << "            SetCoordinate(0, -poleToOppositePole) returns ";
    cout << "lat/lon = " << p5->Latitude() << " / " << p5->Longitude() << endl;
    cout << endl;
    cout << "\t\t\t\t/-----------------------------------------/" << endl;
    cout << endl;
    cout << "    Testing XYRange method " << endl;
    cout << endl;
    cout << "Given: " << endl;
    cout << std::setprecision(7);// so all of the 9's are visible for minlat
    cout << "    Minimum Latitude:  " << p5->MinimumLatitude() << endl;
    cout << "    Maximum Latitude:  " << p5->MaximumLatitude() << endl;
    cout << "    Minimum Longitude: " << p5->MinimumLongitude() << endl;
    cout << "    Maximum Longitude: " << p5->MaximumLongitude() << endl << endl;
    minX=0; maxX=0; minY=0; maxY=0;
    p5->XYRange(minX, maxX, minY, maxY);
    cout << "XYRange method returns" << endl;
    cout << "    Minimum X:  " << minX << endl;
    cout << "    Maximum X:  " << maxX << endl;
    cout << "    Minimum Y:  " << minY << endl;
    cout << "    Maximum Y:  " << maxY << endl;
    cout << endl;
    cout << "        Comparison Values" << endl;
    cout << "        sqrt2*poleToEquator = poleToOppositePole = " 
         << poleToPole << endl;
    cout << endl;   
    cout << std::setprecision(5);
    p5->SetCoordinate(maxX, 0);
    cout << "            SetCoordinate(maxX, 0) returns lat/lon = " 
         << p5->Latitude() << " / " << p5->Longitude()  << endl;
    p5->SetCoordinate(0, maxY);
    cout << "            SetCoordinate(0,maxY) returns lat/lon = " 
         << p5->Latitude() << " / " << p5->Longitude()  << endl;
    p5->SetCoordinate(minX,0);
    cout << "            SetCoordinate(minX,0) returns lat/lon = " << 
         p5->Latitude() << " / " << p5->Longitude()  << endl;
    p5->SetCoordinate(0, minY);
    cout << "            SetCoordinate(0,minY) returns lat/lon = " << 
         p5->Latitude() << " / " << p5->Longitude()  << endl;
    cout << endl;
    cout << endl;
    mapGroup.findKeyword("MinimumLongitude").setValue("-55.0");
    mapGroup.findKeyword("MaximumLongitude").setValue("-10.0");
    TProjection *p5a = (TProjection *) ProjectionFactory::Create(lab);
    cout << "Given: " << endl;
    cout << std::setprecision(7);// so all of the 9's are visible for minlat
    cout << "    Minimum Latitude:  " << p5a->MinimumLatitude() << endl;
    cout << "    Maximum Latitude:  " << p5a->MaximumLatitude() << endl;
    cout << "    Minimum Longitude: " << p5a->MinimumLongitude() << endl;
    cout << "    Maximum Longitude: " << p5a->MaximumLongitude() << endl << endl;
    minX=0; maxX=0; minY=0; maxY=0;
    p5a->XYRange(minX, maxX, minY, maxY);
    cout << "XYRange method returns" << endl;
    cout << "    Minimum X:  " << minX << endl;
    cout << "    Maximum X:  " << maxX << endl;
    cout << "    Minimum Y:  " << minY << endl;
    cout << "    Maximum Y:  " << maxY << endl;
    cout << endl;
    cout << "        Comparison Values" << endl;
    cout << "        maxX:  poleToOppositePole          = " 
         << poleToPole << endl;
    cout << "        minX:  poleToEquator*sqrt2/2       = " 
         << poleToEquator*sqrt(2)/2 << endl;
    cout << "        minY:  -poleToOppositePole*sqrt2/2 = " 
         << -poleToPole*sqrt(2)/2 << endl;
    cout << endl;
    cout << std::setprecision(7);
    p5a->SetCoordinate(maxX,0);
    cout << "            SetCoordinate(maxX, 0) returns lat/lon = " 
         << p5a->Latitude() << " / " << p5a->Longitude()  << endl;
    p5a->SetCoordinate(poleToEquator, maxY);
    cout << "            SetCoordinate(poleToEquator, maxY) returns lat/lon = "
         << p5a->Latitude() << " / " << p5a->Longitude()  << endl;
    p5a->SetCoordinate(minX,-poleToEquator*sqrt(2)/2);
    cout << "            SetCoordinate(minX,-poleToEquator*sqrt2/2) returns lat/lon = " 
         << p5a->Latitude() << " / " << p5a->Longitude()  << endl;
    p5a->SetCoordinate(poleToPole*sqrt(2)/2, minY);
    cout << "            SetCoordinate(poleToOppositePole*sqrt(2)/2,minY) returns lat/lon = " 
         << p5a->Latitude() << " / " << p5a->Longitude()  << endl;
    cout << endl;
    cout << endl;
    cout << endl;
    cout << endl;
    cout << endl;
    //||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||
    cout << border << endl;
    cout << "\t\t \t ELLIPSOIDAL-PLANETOGRAPHIC-POSITIVEEAST-SOUTH POLAR-180" <<endl;
    cout << border << endl << endl;
    //||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||
    mapGroup.findKeyword("CenterLatitude").setValue("-90.0");
    TProjection *p6 = (TProjection *) ProjectionFactory::Create(lab);
    cout << std::setprecision(7);
    cout << mapGroup["CenterLatitude"] << endl;
    cout << mapGroup["CenterLongitude"] << endl;
    cout << "EquatorialRadius = " << p6->EquatorialRadius() << endl;
    cout << "PolarRadius = " << p6->PolarRadius() << endl;
    cout << "Eccentricity = " << p6->Eccentricity() << endl;
    cout << "TrueScaleLatitude = " << p6->TrueScaleLatitude() << endl << endl;
    cout << "\t\t\t\t/-----------------------------------------/" << endl;
    cout << endl;
    cout << "    Testing SetGround(-80, -25)" << endl;
    p6->SetGround(-80, -25);
    cout << "Latitude:            " << p6->Latitude() << endl;
    cout << "Longitude:           " << p6->Longitude() << endl;
    cout << "XCoord:              " << p6->XCoord() << endl;
    cout << "YCoord:              " << p6->YCoord() << endl;
    LambertAzimuthalEqualArea lam6(lab);
    lam6.SetGround(-80, -25);
    cout << "RelativeScaleLatitude:  " << lam6.relativeScaleFactorLatitude() << endl;
    cout << "RelativeScaleLongitude: " << lam6.relativeScaleFactorLongitude() << endl;
    cout << endl;
    cout << "\t\t\t\t/-----------------------------------------/" << endl;
    cout << endl;
    cout << "    Testing SetCoordinate(1077459.7, 288704.5)" << endl;
    p6->SetCoordinate(1077459.7, 288704.5);
    cout << "Latitude:            " << p6->Latitude() << endl;
    cout << "Longitude:           " << p6->Longitude() << endl;
    cout << "XCoord:              " << p6->XCoord() << endl;
    cout << "YCoord:              " << p6->YCoord() << endl;
    cout << endl;
    cout << "\t\t\t\t/-----------------------------------------/" << endl;
    cout << endl;
    cout << "    Testing projection to origin and back\n"
            "    SetGround(-90, -100)" << endl;
    p6->SetGround(-90.0, -100.0);
    cout << "Latitude:             " << p6->Latitude() << endl;
    cout << "Longitude:           " << p6->Longitude() << endl;
    cout << "XCoord:                 " << p6->XCoord() << endl;
    cout << "YCoord:                 " << p6->YCoord() << endl;
    cout << "    SetCoordinate(0, 0)" << endl;
    p6->SetCoordinate(0.0, 0.0);
    cout << "Latitude:               " << p6->Latitude() << endl;
    cout << "Longitude:             " << p6->Longitude() << endl;
    cout << "XCoord:                   " << p6->XCoord() << endl;
    cout << "YCoord:                   " << p6->YCoord() << endl;
    cout << endl;
    cout << "\t\t\t\t/-----------------------------------------/" << endl;
    cout << endl;   
    cout << "    Testing projection to north pole and back\n"
            "    SetGround(90, -100)" << endl;
    p6->SetGround(90.0, -100.0);
    cout << "Latitude:                  " << p6->Latitude() << endl;
    cout << "Longitude:               " << p6->Longitude() << endl;
    cout << "XCoord:                     " << p6->XCoord() << endl;
    cout << "YCoord:              " << p6->YCoord() << endl;
    cout << "    SetCoordinate(0, 2*eqRad)" << endl;
    p6->SetCoordinate(0, 2*p6->EquatorialRadius());
    cout << "Latitude:                  " << p6->Latitude() << endl;
    cout << "Longitude:               " << p6->Longitude() << endl;
    cout << "XCoord:                     " << p6->XCoord() << endl;
    cout << "YCoord:              " << p6->YCoord() << endl;
    cout << endl;
    cout << "\t\t\t\t/-----------------------------------------/" << endl;
    cout << endl;   
    // CHECKING OTHER KNOWN POINTS
    // dist to equator = sqrt(ER^2 + PR^2)
    // This ellipsoidal projection estimates:
    //     d = sqrt(ER^2 + PR^2*factor)
    //     where factor = ln( (1+e) / (1-e) ) / (2e)
    poleToEquator = sqrt(p6->EquatorialRadius() * p6->EquatorialRadius()
                                + p6->PolarRadius() * p6->PolarRadius()
                                  *1/(2*p6->Eccentricity())
                                  *log((1+p6->Eccentricity())
                                        /(1-p6->Eccentricity())));
    rad = p6->PolarRadius();
    cout << "    Testing other known points..." << endl;
    cout << endl;   
    cout << "        Comparison Values" << endl;
    cout << "        poleToEquator = " << poleToEquator << endl;
    cout << endl;   
    // points on circle sqrt(2)*ER in diameter (hemisphere map)
    cout << "    Check known values on hemispherical map" << endl;
    cout << "        FORWARD" << endl;
    p6->SetGround(0, -10);
    cout << "            SetGround(0, -10) returns ";
    cout << "(x,y) = (" << p6->XCoord() << ", " << p6->YCoord() << ")"<< endl;
    p6->SetGround(0, -100);
    cout << "            SetGround(0, 80) returns ";
    cout << "(x,y) = (" << p6->XCoord() << ", " << p6->YCoord() << ")"<< endl;
    p6->SetGround(0, -190);
    cout << "            SetGround(0, -190) returns ";
    cout << "(x,y) = (" << p6->XCoord() << ", " << p6->YCoord() << ")"<< endl;
    p6->SetGround(0, 80);
    cout << "            SetGround(0, -100) returns ";
    cout << "(x,y) = (" << p6->XCoord() << ", " << p6->YCoord() << ")"<< endl;
    cout << "        BACKWARD" << endl;
    p6->SetCoordinate(poleToEquator, 0);
    cout << "            SetCoordinate(poleToEquator, 0) returns ";
    cout << "lat/lon = " << p6->Latitude() << " / " << p6->Longitude() << endl;
    p6->SetCoordinate(0, poleToEquator);
    cout << "            SetCoordinate(0, poleToEquator) returns ";
    cout << "lat/lon = " << p6->Latitude() << " / " << p6->Longitude() << endl;
    p6->SetCoordinate(-poleToEquator, 0);
    cout << "            SetCoordinate(-poleToEquator, 0) returns ";
    cout << "lat/lon = " << p6->Latitude() << " / " << p6->Longitude() << endl;
    p6->SetCoordinate(0, -poleToEquator);
    cout << "            SetCoordinate(0, -poleToEquator) returns ";
    cout << "lat/lon = " << p6->Latitude() << " / " << p6->Longitude() << endl;
    cout << endl;
    // points on whole planet map
    poleToPole = poleToEquator*sqrt(2);
    cout << "    Check known values on whole planet map" << endl;
    cout << endl;
    cout << "        Comparison Values" << endl;
    cout << "        poleToOppositePole = " << poleToPole << endl;
    cout << endl;
    cout << "        FORWARD - Project to opposite side of planet " << endl;
    p6->SetGround(90, 80);
    cout << "            SetGround(-90, 80) returns ";
    cout << "(x,y) = (" << p6->XCoord() << ", " << p6->YCoord() << ")"<< endl;
    p6->SetGround(90, -280);
    cout << "            SetGround(-90, -280) returns ";
    cout << "(x,y) = (" << p6->XCoord() << ", " << p6->YCoord() << ")"<< endl;
    cout << "        BACKWARD - Project from opposite side of planet " << endl;
    p6->SetCoordinate(poleToPole, 0);
    cout << "            SetCoordinate(poleToOppositePole, 0) returns ";
    cout << "lat/lon = " << p6->Latitude() << " / " << p6->Longitude() << endl;
    p6->SetCoordinate(0, poleToPole);
    cout << "            SetCoordinate(0, poleToOppositePole) returns ";
    cout << "lat/lon = " << p6->Latitude() << " / " << p6->Longitude() << endl;
    p6->SetCoordinate(-poleToPole, 0);
    cout << "            SetCoordinate(-poleToOppositePole, 0) returns ";
    cout << "lat/lon = " << p6->Latitude() << " / " << p6->Longitude() << endl;
    p6->SetCoordinate(0, -poleToPole);
    cout << "            SetCoordinate(0, -poleToOppositePole) returns ";
    cout << "lat/lon = " << p6->Latitude() << " / " << p6->Longitude() << endl;
    cout << endl;
    cout << "\t\t\t\t/-----------------------------------------/" << endl;
    cout << endl;
    cout << "    Testing XYRange method " << endl;
    cout << endl;
    cout << "Given: " << endl;
    cout << std::setprecision(7);// so all of the 9's are visible for minlat
    cout << "    Minimum Latitude:  " << p6->MinimumLatitude() << endl;
    cout << "    Maximum Latitude:  " << p6->MaximumLatitude() << endl;
    cout << "    Minimum Longitude: " << p6->MinimumLongitude() << endl;
    cout << "    Maximum Longitude: " << p6->MaximumLongitude() << endl << endl;
    minX=0; maxX=0; minY=0; maxY=0;
    p6->XYRange(minX, maxX, minY, maxY);
    cout << "XYRange method returns" << endl;
    cout << "    Minimum X:  " << minX << endl;
    cout << "    Maximum X:  " << maxX << endl;
    cout << "    Minimum Y:  " << minY << endl;
    cout << "    Maximum Y:  " << maxY << endl;
    cout << endl;
    cout << "        Comparison Values" << endl;
    cout << "        poleToEquator = " << poleToEquator << endl;
    cout << "        sqrt(2)/2 * poleToEquator = " << sqrt(2)/2 * poleToEquator << endl;
    cout << endl;   
    cout << std::setprecision(5);
    p6->SetCoordinate(maxX, 0);
    cout << "            SetCoordinate(maxX, 0) returns lat/lon = " 
         << p6->Latitude() << " / " << p6->Longitude()  << endl;
    p6->SetCoordinate(maxY, maxY);
    cout << "            SetCoordinate(maxY,maxY) returns lat/lon = " 
         << p6->Latitude() << " / " << p6->Longitude()  << endl;
    p6->SetCoordinate(minX,0);
    cout << "            SetCoordinate(minX,0) returns lat/lon = " << 
         p6->Latitude() << " / " << p6->Longitude()  << endl;
    p6->SetCoordinate(0, minY);
    cout << "            SetCoordinate(0,minY) returns lat/lon = " << 
         p6->Latitude() << " / " << p6->Longitude()  << endl;
    cout << endl;
    cout << endl;
    cout << endl;
    cout << endl;
    //||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||
    cout << border << endl;
    cout << "\t\t\t ELLIPSOIDAL-PLANETOGRAPHIC-POSITIVEEAST-OBLIQUE-180" << endl;
    cout << border << endl << endl;
    //||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||
    mapGroup.deleteKeyword("EquatorialRadius");
    mapGroup += PvlKeyword("EquatorialRadius", "6378206.4");
    mapGroup.findKeyword("PolarRadius").setValue(toString(6378206.4*sqrt(1-.00676866)));  
    mapGroup.findKeyword("CenterLatitude").setValue("40.0");                      
    mapGroup.findKeyword("CenterLongitude").setValue("-100.0");                   
    mapGroup.findKeyword("MinimumLatitude").setValue("-50");
    mapGroup.findKeyword("MaximumLatitude").setValue("90.0");
    mapGroup.findKeyword("MinimumLongitude").setValue("-190.0");
    mapGroup.findKeyword("MaximumLongitude").setValue("-10.0");
    cout << std::setprecision(7);
    TProjection *p7 = (TProjection *) ProjectionFactory::Create(lab);                           
    cout << mapGroup["CenterLatitude"] << endl;                                 
    cout << mapGroup["CenterLongitude"] << endl;                                
    cout << "EquatorialRadius = " << p7->EquatorialRadius() << endl;             
    cout << "PolarRadius = " << p7->PolarRadius() << endl;                       
    cout << "Eccentricity = " << p7->Eccentricity() << endl;                     
    cout << "TrueScaleLatitude = " << p7->TrueScaleLatitude() << endl << endl;   
    cout << "\t\t\t\t/-----------------------------------------/" << endl;
    cout << endl;                                                               
    cout << "    Testing SetGround(30, -110) from Snyder pages 333-334" << endl;   
    p7->SetGround(30, -110);                                                     
    cout << "Latitude:            " << p7->Latitude() << endl;                  
    cout << "Longitude:           " << p7->Longitude() << endl;                 
    cout << "XCoord:              " << p7->XCoord() << endl;                    
    cout << "YCoord:              " << p7->YCoord() << endl;                    
    cout << endl;                                                               
    cout << "\t\t\t\t/-----------------------------------------/" << endl;
    cout << endl;                                                               
    cout << "    Testing SetCoordinate(-965932.1, -1056814.9) "                    
            "from Snyder pages 335-336" << endl;                                
    p7->SetCoordinate(-965932.1, -1056814.9);                                    
    cout << "Latitude:            " << p7->Latitude() << endl;                  
    cout << "Longitude:           " << p7->Longitude() << endl;                 
    cout << "XCoord:              " << p7->XCoord() << endl;                    
    cout << "YCoord:              " << p7->YCoord() << endl;                    
    cout << endl;                                                               
    cout << "\t\t\t\t/-----------------------------------------/" << endl;
    cout << endl;
    cout << "    Testing projection to origin and back\n"
            "    SetGround(40, -100)" << endl;
    p7->SetGround(40.0, -100.0);
    cout << "Latitude:              " << p7->Latitude() << endl;
    cout << "Longitude:           " << p7->Longitude() << endl;
    cout << "XCoord:                 " << p7->XCoord() << endl;
    cout << "YCoord:                 " << p7->YCoord() << endl;
    cout << "    SetCoordinate(0, 0)" << endl;
    p7->SetCoordinate(0.0, 0.0);
    cout << "Latitude:              " << p7->Latitude() << endl;
    cout << "Longitude:           " << p7->Longitude() << endl;
    cout << "XCoord:                 " << p7->XCoord() << endl;
    cout << "YCoord:                 " << p7->YCoord() << endl;
    cout << endl;
    cout << "\t\t\t\t/-----------------------------------------/" << endl;
    cout << endl;   
    cout << "    Testing projection to north pole and back\n"
            "    SetGround(90, 0)" << endl;
    p7->SetGround(90.0, 0.0);
    cout << "Latitude:                 " << p7->Latitude() << endl;
    cout << "Longitude:                 " << p7->Longitude() << endl;
    cout << "XCoord:                    " << p7->XCoord() << endl;
    cout << "YCoord:              " << p7->YCoord() << endl;
    cout << "    SetCoordinate(0, 5394277.8222559)" << endl;
    p7->SetCoordinate(0.0, 5394277.8222559);
    cout << "Latitude:                 " << p7->Latitude() << endl;
    cout << "Longitude:                " << p7->Longitude() << endl;
    cout << "XCoord:                    " << p7->XCoord() << endl;
    cout << "YCoord:              " << p7->YCoord() << endl;
    cout << endl;
    cout << "\t\t\t\t/-----------------------------------------/" << endl;
    cout << endl;   
    // CHECKING OTHER KNOWN POINTS
    double a = p7->LocalRadius(0);                                              
    double c = p7->LocalRadius(mapGroup["CenterLatitude"]);                      
    double distToSide = sqrt(a*a + c*c);
    double a2 = p7->LocalRadius(50);
    double c2 = p7->LocalRadius(mapGroup["CenterLatitude"]);
    double distToTop = sqrt(a2*a2 + c2*c2);
    double a3 = p7->LocalRadius(-50);
    double c3 = p7->LocalRadius(mapGroup["CenterLatitude"]);
    double distToBottom = sqrt(a3*a3+c3*c3);
    rad = p7->LocalRadius();
    cout << "    Testing other known points..." << endl;
    cout << endl;
    cout << "        Comparison Values" << endl;
    cout << "        DistanceToSide              = " << distToSide << endl;
    cout << "        DistanceToTop               = " << distToTop << endl;
    cout << "        DistanceToBottom            = " << distToBottom << endl;
    cout << endl;
    // points on circle sqrt(2)*ER in diameter (hemisphere map)
    cout << "    Check known values on hemispherical map" << endl;
    cout << "        FORWARD" << endl;
    p7->SetGround(0, -10);
    cout << "            SetGround(0, -10) returns ";                       
    cout << "(x,y) = (" << p7->XCoord() << ", " << p7->YCoord() << ")"<< endl;
    p7->SetGround(50, 80);                                                   
    cout << "            SetGround(50, 80) returns ";                       
    cout << "(x,y) = (" << p7->XCoord() << ", " << p7->YCoord() << ")"<< endl;
    p7->SetGround(0, -190);                                                  
    cout << "            SetGround(0, -190) returns ";                      
    cout << "(x,y) = (" << p7->XCoord() << ", " << p7->YCoord() << ")"<< endl;
    p7->SetGround(-50, -100);                                                
    cout << "            SetGround(-50, -100) returns ";                    
    cout << "(x,y) = (" << p7->XCoord() << ", " << p7->YCoord() << ")"<< endl;
    cout << "        BACKWARD" << endl;                                     
    p7->SetCoordinate(distToSide, 0);                                        
    cout << "            SetCoordinate(DistanceToSide, 0) returns ";        
    cout << "lat/lon = " << p7->Latitude()                                   
                             << " / " << p7->Longitude() << endl;            
    p7->SetCoordinate(0, distToTop);                                         
    cout << "            SetCoordinate(0, DistanceToTop) returns ";         
    cout << "lat/lon = " << p7->Latitude() << " / "                          
                             << p7->Longitude() << endl;                     
    p7->SetCoordinate(-distToSide, 0);                                       
    cout << "            SetCoordinate(-DistanceToSide, 0) returns ";       
    cout << "lat/lon = " << p7->Latitude()                                   
                             << " / " << p7->Longitude() << endl;            
    p7->SetCoordinate(0, -distToBottom);                                     
    cout << "            SetCoordinate(0, -DistanceToBottom) returns ";     
    cout << "lat/lon = " << p7->Latitude()                                   
                             << " / " << p7->Longitude() << endl;            
    cout << endl;                                                           
    cout << "\t\t\t\t/-----------------------------------------/" << endl;
    cout << endl;
    cout << "    Testing XYRange method " << endl;
    cout << "Given: " << endl;
    cout << "    Minimum Latitude:  " << p7->MinimumLatitude() << endl;
    cout << "    Maximum Latitude:  " << p7->MaximumLatitude() << endl;
    cout << "    Minimum Longitude: " << p7->MinimumLongitude() << endl;
    cout << "    Maximum Longitude: " << p7->MaximumLongitude() << endl << endl;
    minX=0; maxX=0; minY=0; maxY=0;
    p7->XYRange(minX, maxX, minY, maxY);
    cout << "XYRange method returns" << endl;
    cout << "    Minimum X:  " << minX << endl;
    cout << "    Maximum X:  " << maxX << endl;
    cout << "    Minimum Y:  " << minY << endl;
    cout << "    Maximum Y:  " << maxY << endl;
    cout << endl;
    p7->SetGround(90, -100);                                                 
    cout << "            SetGround(90, -100) returns ";                     
    cout << "(x,y) = (" << p7->XCoord() << ", " << p7->YCoord() << ")"<< endl;
    p7->SetGround(-50, -100);                                               
    cout << "            SetGround(-50, -100) returns ";                    
    cout << "(x,y) = (" << p7->XCoord() << ", " << p7->YCoord() << ")"<< endl;
    cout << endl;



    p7->SetGround(50, 80);                                               
    cout << "            SetGround(50, 80) returns ";                       
    cout << "(x,y) = (" << p7->XCoord() << ", " << p7->YCoord() << ")"<< endl;
    cout << endl;



    p7->SetGround(0, -190);                                                 
    cout << "            SetGround(0, -190) returns ";                      
    cout << "(x,y) = (" << p7->XCoord() << ", " << p7->YCoord() << ")"<< endl;
    p7->SetGround(0, -10);                                                  
    cout << "            SetGround(0, -10) returns ";                       
    cout << "(x,y) = (" << p7->XCoord() << ", " << p7->YCoord() << ")"<< endl;
    p7->SetGround(-40, -190);                                               
    cout << "            SetGround(-40, -190) returns ";                    
    cout << "(x,y) = (" << p7->XCoord() << ", " << p7->YCoord() << ")"<< endl;
    p7->SetGround(-40, -10);                                                
    cout << "            SetGround(-40, -10) returns ";                     
    cout << "(x,y) = (" << p7->XCoord() << ", " << p7->YCoord() << ")"<< endl;
    cout << endl;
    p7->SetGround(-20, -190);                                               
    cout << "            SetGround(-20, -190) returns ";                    
    cout << "(x,y) = (" << p7->XCoord() << ", " << p7->YCoord() << ")"<< endl;
    p7->SetGround(-20, -10);                                                
    cout << "            SetGround(-20, -10) returns ";                     
    cout << "(x,y) = (" << p7->XCoord() << ", " << p7->YCoord() << ")"<< endl;
    p7->SetGround(-21.5, -190);                                              
    cout << "            SetGround(-21.5, -190) returns ";                  
    cout << "(x,y) = (" << p7->XCoord() << ", " << p7->YCoord() << ")"<< endl;
    p7->SetGround(-21.5, -10);                                               
    cout << "            SetGround(-21.5, -10) returns ";                   
    cout << "(x,y) = (" << p7->XCoord() << ", " << p7->YCoord() << ")"<< endl;
    cout << endl;
    p7->SetCoordinate(maxX, -2882473.6383627);                               
    cout << "            SetCoordinate(maxX,-2882473.6383627) returns lat/lon = "
         << p7->Latitude() << " / " << p7->Longitude()  << endl;                   
    p7->SetCoordinate(0, maxY);                                                  
    cout << "            SetCoordinate(0,maxY) returns lat/lon = "               
         << p7->Latitude() << " / " << p7->Longitude()  << endl;                   
    p7->SetCoordinate(minX, -2882473.6383627);                                    
    cout << "            SetCoordinate(minX,-2882473.6383627) returns lat/lon = "
         << p7->Latitude() << " / " << p7->Longitude()  << endl;                   
    p7->SetCoordinate(0, minY);                                                  
    cout << "            SetCoordinate(0,minY) returns lat/lon = "               
         << p7->Latitude() << " / " << p7->Longitude()  << endl;                   
    cout << endl;
    cout << endl;
    cout << endl;
    cout << endl;
    //||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||
    cout << border << endl;
    cout << "\t\t\t ELLIPSOIDAL-PLANETOGRAPHIC-POSITIVEEAST-EQUATORIAL-180" << endl;
    cout << border << endl << endl;
    //||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||
    mapGroup.findKeyword("CenterLatitude").setValue("0");
    mapGroup.findKeyword("CenterLongitude").setValue("0");
    mapGroup.findKeyword("MinimumLatitude").setValue("-90");
    mapGroup.findKeyword("MaximumLatitude").setValue("90.0");
    mapGroup.findKeyword("MinimumLongitude").setValue("-90.0");
    TProjection *p8 = (TProjection *) ProjectionFactory::Create(lab);
    cout << mapGroup["CenterLatitude"] << endl;
    cout << mapGroup["CenterLongitude"] << endl << endl;
    cout << "EquatorialRadius = " << p8->EquatorialRadius() << endl;
    cout << "PolarRadius = " << p8->PolarRadius() << endl;
    cout << "Eccentricity = " << p8->Eccentricity() << endl;
    cout << "TrueScaleLatitude = " << p8->TrueScaleLatitude() << endl << endl;
    cout << "\t\t\t\t/-----------------------------------------/" << endl;   
    cout << endl;                                                            
    cout << "    Testing SetGround(30, -110)" << endl;                       
    p8->SetGround(30, -110);                                                  
    cout << "Latitude:            " << p8->Latitude() << endl;               
    cout << "Longitude:           " << p8->Longitude() << endl;              
    cout << "XCoord:              " << p8->XCoord() << endl;                 
    cout << "YCoord:              " << p8->YCoord() << endl;                 
    cout << "TrueScaleLat:           " << p8->TrueScaleLatitude() << endl;    
    cout << endl;                                                            
    cout << "\t\t\t\t/-----------------------------------------/" << endl;   
    cout << endl;                                                            
    cout << "    Testing SetCoordinate"
            "(-8761895.7861122, 5346904.00797488)" << endl;
    // choose output from forward direction to make sure we get lat=30, lon=-110
    p8->SetCoordinate(-8761895.7861122, 5346904.00797488);                       
    cout << "Latitude:            " << p8->Latitude() << endl;                   
    cout << "Longitude:           " << p8->Longitude() << endl;                  
    cout << "XCoord:              " << p8->XCoord() << endl;                     
    cout << "YCoord:              " << p8->YCoord() << endl;                     
    cout << endl;                                                               
    cout << "\t\t\t\t/-----------------------------------------/" << endl;      
    cout << endl;
    cout << "    Testing projection to origin and back\n"
            "    SetGround(0, 0)" << endl;
    p8->SetGround(0.0, 0.0);
    cout << "Latitude:            " << p8->Latitude() << endl;
    cout << "Longitude:           " << p8->Longitude() << endl;
    cout << "XCoord:              " << p8->XCoord() << endl;
    cout << "YCoord:              " << p8->YCoord() << endl;
    cout << "    SetCoordinate(0, 0)" << endl;
    p8->SetCoordinate(0.0, 0.0);
    cout << "Latitude:            " << p8->Latitude() << endl;
    cout << "Longitude:           " << p8->Longitude() << endl;
    cout << "XCoord:              " << p8->XCoord() << endl;
    cout << "YCoord:              " << p8->YCoord() << endl;
    cout << endl;
    cout << "\t\t\t\t/-----------------------------------------/" << endl;
    cout << endl;   
    cout << "    Testing projection to north pole and back\n"
            "    SetGround(90, 0)" << endl;
    p8->SetGround(90.0, 0.0);
    cout << "Latitude:                 " << p8->Latitude() << endl;
    cout << "Longitude:                 " << p8->Longitude() << endl;
    cout << "XCoord:                    " << p8->XCoord() << endl;
    cout << "YCoord:              " << p8->YCoord() << endl;
    cout << "    SetCoordinate(0, distFromEquatorToPole)" << endl;
    p8->SetCoordinate(0.0, 8999766.9300905);
    cout << "Latitude:                 " << p8->Latitude() << endl;
    cout << "Longitude:               " << p8->Longitude() << endl;
    cout << "XCoord:                    " << p8->XCoord() << endl;
    cout << "YCoord:              " << p8->YCoord() << endl;
    cout << endl;
    cout << "\t\t\t\t/-----------------------------------------/" << endl;
    cout << endl;   
        // CHECKING OTHER KNOWN POINTS
    // dist from equator to pole = sqrt(ER^2 + PR^2)
    // This ellipsoidal projection estimates:
    //     d = sqrt( (ER^2 + PR^2*factor) * (qp/2) )
    //     where factor = ln( (1+e) / (1-e) ) / (2e)
    //     and qp = 1 + (1 - e^2)*factor
    rad = p8->EquatorialRadius();
    // double pRad = p8->PolarRadius();                                             
    // double distToPole = sqrt(rad*rad + pRad*pRad);                              
    double equatorToPole = sqrt(2)/2 
                           * rad 
                           * (1 - log((1 - p8->Eccentricity())
                                       /(1 + p8->Eccentricity()))
                                  * (1 - p8->Eccentricity() * p8->Eccentricity())
                                  / (2 * p8->Eccentricity()));
    // double factor = log( (1 + p8->Eccentricity()) / (1 - p8->Eccentricity()) ) / (2 * p8->Eccentricity());
    // poleToEquator = sqrt(rad*rad + pRad*pRad*factor);     
    // double qp = 1 + (1 - p8->Eccentricity() * p8->Eccentricity()) * factor;
    // double eTp = poleToEquator*sqrt( qp / 2 );
    cout << "    Testing other known points..." << endl;                                    
    cout << endl;                                                                           
    cout << "        Comparison Values" << endl;                                            
    cout << "        sqrt2*EquatorialRadius (EquatorToEquator) = " 
         << rad*sqrt(2) << endl;                        
    // cout << "        DistanceToPole = " << distToPole << endl;
    cout << "        EquatorToPole                             = " 
         << equatorToPole<< endl;
    cout << endl;                                               
    // points on circle sqrt(2)*ER in diameter (hemisphere map) 
    cout << "    Check known values on hemispherical map" << endl;
    cout << "        FORWARD" << endl;                            
                                                                  
    p8->SetGround(0, 90);                                          
    cout << "            SetGround(0, 90) returns ";              
    cout << "(x,y) = (" << p8->XCoord() << ", " << p8->YCoord() << ")"<< endl;
    p8->SetGround(90, 0);                                                    
    cout << "            SetGround(90, 0) returns ";                        
    cout << "(x,y) = (" << p8->XCoord() << ", " << p8->YCoord() << ")"<< endl;
    p8->SetGround(0, -90);                                                   
    cout << "            SetGround(0, -90) returns ";                       
    cout << "(x,y) = (" << p8->XCoord() << ", " << p8->YCoord() << ")"<< endl;
    p8->SetGround(-90, 0);                                                   
    cout << "            SetGround(-90, 0) returns ";                       
    cout << "(x,y) = (" << p8->XCoord() << ", " << p8->YCoord() << ")"<< endl;
    cout << endl;                                                           
    cout << "\t\t\t\t/-----------------------------------------/" << endl;  
    cout << endl;
    cout << "    Testing XYRange method " << endl;
    cout << endl;
    cout << "Given: " << endl;
    cout << std::setprecision(7);
    cout << "    Minimum Latitude:  " << p8->MinimumLatitude() << endl;
    cout << "    Maximum Latitude:  " << p8->MaximumLatitude() << endl;
    cout << "    Minimum Longitude: " << p8->MinimumLongitude() << endl;
    cout << "    Maximum Longitude: " << p8->MaximumLongitude() << endl << endl;
    minX=0; maxX=0; minY=0; maxY=0;
    p8->XYRange(minX, maxX, minY, maxY);
    cout << "XYRange method returns" << endl;
    cout << "    Minimum X:  " << minX << endl;
    cout << "    Maximum X:  " << maxX << endl;
    cout << "    Minimum Y:  " << minY << endl;
    cout << "    Maximum Y:  " << maxY << endl;
    p8->SetCoordinate(maxX,0);
    cout << "            SetCoordinate(maxX,0) returns lat/lon = " 
         << p8->Latitude() << " / " << p8->Longitude()  << endl;
    p8->SetCoordinate(0, maxY);
    cout << "            SetCoordinate(0,maxY) returns lat/lon = " 
         << p8->Latitude() << " / " << p8->Longitude()  << endl;
    p8->SetCoordinate(minX,0);
    cout << "            SetCoordinate(minX,0) returns lat/lon = " 
         << p8->Latitude() << " / " << p8->Longitude()  << endl;
    p8->SetCoordinate(0, minY);
    cout << "            SetCoordinate(0,minY) returns lat/lon = " 
         << p8->Latitude() << " / " << p8->Longitude()  << endl;
    cout << endl;
    mapGroup.findKeyword("MaximumLongitude").setValue("90.0");
    TProjection *p8a = (TProjection *) ProjectionFactory::Create(lab);
    cout << "\t\t\t\t/-----------------------------------------/" << endl;  
    cout << endl;
    cout << "    Testing XYRange method " << endl;
    cout << endl;
    cout << "Given: " << endl;
    cout << std::setprecision(7);
    cout << "    Minimum Latitude:  " << p8a->MinimumLatitude() << endl;
    cout << "    Maximum Latitude:  " << p8a->MaximumLatitude() << endl;
    cout << "    Minimum Longitude: " << p8a->MinimumLongitude() << endl;
    cout << "    Maximum Longitude: " << p8a->MaximumLongitude() << endl << endl;
    minX=0; maxX=0; minY=0; maxY=0;
    p8a->XYRange(minX, maxX, minY, maxY);
    cout << "XYRange method returns" << endl;
    cout << "    Minimum X:  " << minX << endl;
    cout << "    Maximum X:  " << maxX << endl;
    cout << "    Minimum Y:  " << minY << endl;
    cout << "    Maximum Y:  " << maxY << endl;
    p8a->SetCoordinate(maxX,0);
    cout << "            SetCoordinate(maxX,0) returns lat/lon = " 
         << p8a->Latitude() << " / " << p8a->Longitude()  << endl;
    p8a->SetCoordinate(0, maxY);
    cout << "            SetCoordinate(0,maxY) returns lat/lon = " 
         << p8a->Latitude() << " / " << p8a->Longitude()  << endl;
    p8a->SetCoordinate(minX,0);
    cout << "            SetCoordinate(minX,0) returns lat/lon = " 
         << p8a->Latitude() << " / " << p8a->Longitude()  << endl;
    p8a->SetCoordinate(0, minY);
    cout << "            SetCoordinate(0,minY) returns lat/lon = " 
         << p8a->Latitude() << " / " << p8a->Longitude()  << endl;
    cout << endl;
    cout << endl;
    cout << endl;
    cout << endl;
    //||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||
    cout << border << endl;
    cout << "\t\t\t\t\t TESTING OTHER METHODS          " << endl;
    cout << border << endl << endl;
    //||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||
    mapGroup.findKeyword("PolarRadius").setValue(toString(p8->EquatorialRadius()));
    TProjection *p9 = (TProjection *) ProjectionFactory::Create(lab);
    TProjection *s = p1;
    cout << "Name:                       " << s->Name().toStdString() << endl;
    cout << "Version:                    " << p1->Version().toStdString() << endl;
    cout << "Rotation:                   " << p1->Rotation() << endl;
    cout << "TrueScaleLatitude:          " << p1->TrueScaleLatitude() << endl;
    cout << "Testing operator==  (True): " << (*s == *s) << endl;
    cout << "Testing operator==  (True): " << (*s == proj) << endl;
    // different lat/lon range, all other properties the same
    cout << "Testing operator==  (True): " << (*p1 == *p1a) << endl;
    // different CenterLatitude
    cout << "Testing operator==  (False-different CenterLatitude):   " << (p3 == p4) << endl;
    // same CenterLatitude, different CenterLongitude
    cout << "Testing operator==  (False-different CenterLongitude):  " << (p4 == p5) << endl;
    // same CenterLatitude/CenterLongitude, different EquatorialRadius
    cout << "Testing operator==  (False-different EquatorialRadius): " << (p2 == p7) << endl;
    // same CenterLatitude/CenterLongitude/EquatorialRadius, different eccentricity
    cout << "Testing operator==  (False-different Eccentricity):     " << (p8 == p9) << endl;
    cout << endl;
    cout << "\t\t\t\t/-----------------------------------------/" << endl;
    cout << endl;
    cout << "Testing default computations of CenterLatitude and CenterLongitude by Constructor" << endl;
    mapGroup.deleteKeyword("CenterLongitude");
    mapGroup.deleteKeyword("CenterLatitude");
    LambertAzimuthalEqualArea allowDefaultsTrue(lab, true);
    cout << lab << endl;
    cout << endl;
    cout << "\t\t\t\t/-----------------------------------------/" << endl;
    cout << endl;
    cout << "Testing Mapping() methods" << endl;
    Pvl tmp1;
    Pvl tmp2;
    Pvl tmp3;
    tmp1.addGroup(p1->Mapping());
    tmp2.addGroup(p1->MappingLatitudes());
    tmp3.addGroup(p1->MappingLongitudes());
    cout << "Mapping() = " << endl;
    cout << tmp1 << endl;
    cout << "MappingLatitudes() = " << endl;
    cout << tmp2 << endl;
    cout << "MappingLongitudes() = " << endl;
    cout << tmp3 << endl;
    cout << endl;
    cout << endl;
    cout << endl;
    cout << endl;

    //||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||
    cout << border << endl;
    cout << "\t\t\t\t\t\t\t\t\t TESTING    GOOD = FALSE" << endl;
    cout << border << endl << endl;
    //||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||
    cout << "Set Ground using invalid Latitude/Longitude value" << endl;
    cout << "SetGround(-91, 0):     " << p1->SetGround(-91.0,  0.0) << endl;
    cout << "SetGround(Null, 0):    " << p1->SetGround( Null,  0.0) << endl;
    cout << "SetGround(0, Null):    " << p1->SetGround( 0.0,  Null) << endl;
    cout << "Antipodal point for center lat/lon = 0/0:" << endl;
    cout << "SetGround(0, 180):     " << p1->SetGround( 0.0,   180) << endl;
    cout << "SetGround(0, -180):    " << p1->SetGround( 0.0,  -180) << endl;
    cout << "SetGround(0, 180):     " << p8->SetGround( 0.0,   180) << endl;
    cout << "SetGround(0, -180):    " << p8->SetGround( 0.0,  -180) << endl;
    cout << "Antipodal point for center lat/lon = 40/-100:" << endl;
    cout << "SetGround(-40,   80):     " << p2->SetGround(-40.0,   80) << endl;
    cout << "SetGround(-40, -280):     " << p2->SetGround(-40.0, -280) << endl;
    cout << "SetGround(-40,   80):     " << p7->SetGround(-40.0,   80) << endl;
    cout << "SetGround(-40, -280):     " << p7->SetGround(-40.0, -280) << endl;
    cout << "Set Coordinate using invalid x/y value" << endl;
    cout << "SetCoordinate(Null, 0):    " << p1->SetCoordinate(Null, 0.0) << endl;
    cout << "SetCoordinate(0, Null):    " << p1->SetCoordinate(0.0,  Null) << endl;
    cout << "Set Coordinate using x/y value off the planet" << endl;
    cout << "SetCoordinate(100, 0):    " << p1->SetCoordinate(100,  0) << endl;
    cout << "SetCoordinate(0, -100):   " << p1->SetCoordinate(0, -100) << endl;
    cout << endl;
    cout << "\t\t\t\t/-----------------------------------------/" << endl;
    cout << endl;

    //||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||
    cout << border << endl;
    cout << "\t\t\t\t\t\t\t\t\t TESTING ERRORS                     " << endl;
    cout << border << endl << endl;
    //||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||
    mapGroup.deleteKeyword("CenterLongitude");
    cout << "Error check: Missing center longitude keyword" << endl;
    try {
      LambertAzimuthalEqualArea p(lab);
    }
    catch(IException &e) {
      e.print();
      mapGroup += PvlKeyword("CenterLongitude", "180");
    }
    cout << endl;
    cout << "\t\t\t\t/-----------------------------------------/" << endl;
    cout << endl;
    mapGroup.deleteKeyword("CenterLatitude");
    cout << "Error check: Missing center latitude keyword" << endl;
    try {
      LambertAzimuthalEqualArea p(lab);
    }
    catch(IException &e) {
      e.print();
      mapGroup += PvlKeyword("CenterLatitude","0");
    }
    cout << endl;
    cout << "\t\t\t\t/-----------------------------------------/" << endl;
    cout << endl;
    cout << "Error check: MinimumLongitude more than 360 degrees "
            "from CenterLongitude" << endl;
    try {
      mapGroup.findKeyword("MinimumLongitude").setValue("-181.0");
      mapGroup.findKeyword("MaximumLongitude").setValue("270.0");
      LambertAzimuthalEqualArea p(lab);
    }
    catch(IException &e) {
      e.print();
      mapGroup.findKeyword("MinimumLongitude").setValue("-180.0");
    }
    cout << endl;
    cout << "\t\t\t\t/-----------------------------------------/" << endl;
    cout << endl;
    cout << "Error check: MaximumLongitude more than 360 degrees "
            "from CenterLongitude" << endl;
    try {
      mapGroup.findKeyword("MaximumLongitude").setValue("541.0");
      LambertAzimuthalEqualArea p(lab);
    }
    catch(IException &e) {
      e.print();
      mapGroup.findKeyword("MaximumLongitude").setValue("270.0");
    }
    cout << endl;
    cout << "\t\t\t\t/-----------------------------------------/" << endl;
    cout << endl;
    cout << "Error check: Longitude min/max range greater than 360" << endl;
    try {
      LambertAzimuthalEqualArea p(lab);
    }
    catch(IException &e) {
      e.print();
      mapGroup.findKeyword("MinimumLongitude").setValue("90.0");
    }
    cout << endl;
    cout << "\t\t\t\t/-----------------------------------------/" << endl;
    cout << endl;
    cout << "Error check: Center longitude out of valid range" << endl;
    try {
      mapGroup.findKeyword("CenterLongitude").setValue("361.0");
      LambertAzimuthalEqualArea p(lab);
    }
    catch(IException &e) {
      e.print();
      mapGroup.findKeyword("CenterLongitude").setValue("180.0");
    }
    cout << endl;
    cout << "\t\t\t\t/-----------------------------------------/" << endl;
    cout << endl;
    cout << "Error check: Center latitude out of valid range" << endl;
    try {
      mapGroup.findKeyword("CenterLatitude").setValue("-91.0");
      LambertAzimuthalEqualArea p(lab);
    }
    catch(IException &e) {
      e.print();
      mapGroup.findKeyword("CenterLatitude").setValue("0.0");
    }
    cout << endl;
    cout << "\t\t\t\t/-----------------------------------------/" << endl;
    cout << endl;
    cout << "Error check: Invalid Latitude and Longitude range. Can't project "
            "antipodal point for equatorial projections." << endl;
    try {
      // non-polar, clat = 0 (equatorial)
      mapGroup.findKeyword("CenterLatitude").setValue("0");
      // minlat(-90) <= -clat <= maxlat(90)
      mapGroup.findKeyword("MinimumLatitude").setValue("-90");
      mapGroup.findKeyword("MaximumLatitude").setValue("90.0");
      // minlon(-180) <= clon-180 <= maxlon(90)
      mapGroup.findKeyword("CenterLongitude").setValue("0");
      mapGroup.findKeyword("MinimumLongitude").setValue("-180.0");
      mapGroup.findKeyword("MaximumLongitude").setValue("90.0");
      LambertAzimuthalEqualArea p(lab);
    }
    catch(IException &e) {
      e.print();
    }
    cout << endl;
    cout << "\t\t\t\t/-----------------------------------------/" << endl;
    cout << endl;
    cout << "Error check: Invalid Latitude and Longitude range. Can't project "
            "antipodal point for oblique projections." << endl;
    try {
      // non-polar, clat = 10 (oblique)
      mapGroup.findKeyword("CenterLatitude").setValue("10");
      // minlat(-20) <= -clat(-10) <= maxlat(0)
      mapGroup.findKeyword("MinimumLatitude").setValue("-20");
      mapGroup.findKeyword("MaximumLatitude").setValue("0");
      // minlon(180) <= clon+180(190) <= maxlon(200)
      mapGroup.findKeyword("CenterLongitude").setValue("10");
      mapGroup.findKeyword("MinimumLongitude").setValue("180.0");
      mapGroup.findKeyword("MaximumLongitude").setValue("200.0");
      LambertAzimuthalEqualArea p(lab);
    }
    catch(IException &e) {
      e.print();
      mapGroup.findKeyword("MaximumLongitude").setValue("189.0");
    }
    cout << endl;
    cout << "\t\t\t\t/-----------------------------------------/" << endl;
    cout << endl;
    cout << "Error check: Relative scale factor. "
            "Uncomputed or failed projection." << endl;
    LambertAzimuthalEqualArea p(lab);
    try {
      p.relativeScaleFactorLongitude();
    }
    catch(IException &e) {
      e.print();
    }
    p.SetCoordinate(0,0);
    cout << endl;
    cout << "\t\t\t\t/-----------------------------------------/" << endl;
    cout << endl;
    cout << "Error check: Relative scale factor. Null value." << endl;
    try {
      p.relativeScaleFactorLatitude();
    }
    catch(IException &e) {
      e.print();
    }
    cout << endl;
    cout << "\t\t\t\t/-----------------------------------------/" << endl;
    cout << endl;
    mapGroup.findKeyword("EquatorialRadius").setValue("2.0");
    mapGroup.findKeyword("PolarRadius").setValue("1.0");
    cout << "Error check: Relative scale factor. "
            "Ellipsoidal oblique aspect projection." << endl;
    LambertAzimuthalEqualArea pp(lab);
    pp.SetGround(10,10);
    try {
      pp.relativeScaleFactorLongitude();
    }
    catch(IException &e) {
      e.print();
    }
    cout << endl;
    cout << "\t\t\t\t/-----------------------------------------/" << endl;
    cout << endl;
    cout << "Error check: Relative scale factor. "
            "Polar aspect projection projected at opposite pole." << endl;
    mapGroup.findKeyword("CenterLatitude").setValue("90");
    LambertAzimuthalEqualArea nPole(lab);
    nPole.SetGround(-90,0);
    try {
      nPole.relativeScaleFactorLongitude();
    }
    catch(IException &e) {
      e.print();
    }
    mapGroup.findKeyword("CenterLatitude").setValue("-90");
    LambertAzimuthalEqualArea sPole(lab);
    sPole.SetGround(90,0);
    try {
      sPole.relativeScaleFactorLatitude();
    }
    catch(IException &e) {
      e.print();
    }
    cout << endl;
    cout << endl;
    cout << endl;
    cout << endl;
    cout << border << endl;
    cout << "Much of this unit test was obtained from:" << endl << endl;
    cout << "  Map Projections - A Working Manual" << endl;
    cout << "  USGS Professional Paper 1395 by John P. Snyder" << endl;
    cout << "  pages 188-190, 332-337" << endl;
    cout << border << endl;
  }
  catch(IException &e) {
    e.print();
  }
}


