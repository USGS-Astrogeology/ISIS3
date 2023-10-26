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
#include "Projection.h"
#include "ProjectionFactory.h"
#include "Preference.h"
#include "Pvl.h"
#include "PvlGroup.h"
#include "PvlKeyword.h"
#include "TProjection.h"
#include "UpturnedEllipsoidTransverseAzimuthal.h"

using namespace std;
using namespace Isis;

void testSetGround(TProjection *p);
void printErrors(double origLat, double projectedLat, double origLon, double projectedLon);

/**
 * Unit test for UpturnedEllipsoidTransverseAzimuthal map projection class
 * 
 * @author 2016-05-25 Jeannie Backer
 * @internal
 *   @history 2016-05-25 Jeannie Backer - Original version.
 *   @history 2016-12-28 Kristin Berry - Updated for longitude restriction to center longitue +/-
 *                                       90 degrees and fixes for some output values.
 *  
 *   @todo Test coverage:
 *     CONSTRUCTOR:
 *         if (mapGroup.hasKeyword("CenterLongitude"))
 *         if (!mapGroup.hasKeyword("CenterLongitude") && allowDefaults)
 *         if (!mapGroup.hasKeyword("CenterLongitude") && !allowDefaults) THROWS  ERROR
 *         if (MaximumLongitude() - MinimumLongitude() > 360.0) THROWS ERROR
 *     INIT
 *         if (IsPositiveEast())
 *         if (IsPositiveWest())
 *         if (Has180Domain()
 *         if (Has360Domain()
 *         if (qFuzzyCompare(0.0, m_e)) (EQUATORIAL RAD = POLAR RAD)
 *     OPERATOR==
 *         if (!Projection::operator==(proj)) return false;
 *         ELSE
 *     SETGROUND
 *         if (lat == Null)
 *         if (lon == Null)
 *         if (qFuzzyCompare(90.0, qAbs(lat)) && lat > 90.0) 
 *         if (qFuzzyCompare(90.0, qAbs(lat)) && lat < -90.0)
 *         if lat no where near poles
 *         if (IsPlanetographic())
 *         if (IsPlanetocentric())
 *         if (IsPositiveEast())
 *         if (IsPositiveWest())
 *         if the given longitude is within tolerance of -PI and less than -PI,
 *             set it to -PI - tolerance)
 *         if the given longitude is within tolerance of -PI and greater than -PI, 
 *             set it to -zmax (i.e. -PI + tolerance)
 *         if the given longitude is non-negative and within tolerance of zero,
 *             set it to zmin (i.e 0 + tolerance)
 *         if the given longitude is within tolerance of PI and less than PI,
 *             set it to zmax (i.e. PI - tolerance)
 *         if the given longitude is within tolerance of PI and greater than PI,
 *             set it to PI + tolerance
 *         if the given longitude is within tolerance of 2PI and less than 2PI,
 *             set it to 2PI - tolerance
 *         if (cosaz <= -1) {
 *         else if (cosaz >= 1) {
 *         else if (phiNorm >= 0) { // northern hemisphere, i.e. cos(phiNorm)sin(phiNorm) >= 0
 *         else { // phiNorm is in southern hemisphere i.e. cos(phiNorm)sin(phiNorm) < 0
 *         // if azimuth is acute or negative, increase by 2pi
 *         if (az <= HALFPI)
 *     SETCOORDINATE
 *         if (x == Null)
 *         if (y == Null
 *         if (IsPlanetographic())
 *         if (IsPlanetocentric())
 *         if (y >= 0) {
 *         if (y < 0) {
 *         if (IsPositiveEast())
 *         if (IsPositiveWest())
 *         if (Has180Domain()
 *         if (Has360Domain()
 *      XYRANGE
 *      CHECKLONGITUDE
 *  
 */
int main(int argc, char *argv[]) {
  Preference::Preferences(true);

  cout << "UNIT TEST FOR UpturnedEllipsoidTransverseAzimuthal projection" << endl << endl;

  Pvl lab;
  lab.addGroup(PvlGroup("Mapping"));
  PvlGroup &mapGroup = lab.findGroup("Mapping");
  mapGroup += PvlKeyword("EquatorialRadius", "1.0");
  mapGroup += PvlKeyword("PolarRadius", "1.0");
  mapGroup += PvlKeyword("LatitudeType", "Planetographic");
  mapGroup += PvlKeyword("LongitudeDirection", "PositiveWest");
  mapGroup += PvlKeyword("LongitudeDomain", "180");
  mapGroup += PvlKeyword("MinimumLatitude", "20.0");
  mapGroup += PvlKeyword("MaximumLatitude", "80.0");
  mapGroup += PvlKeyword("MinimumLongitude", "-90.0");
  mapGroup += PvlKeyword("MaximumLongitude", "90.0");
  mapGroup += PvlKeyword("ProjectionName", "UpturnedEllipsoidTransverseAzimuthal");
  mapGroup += PvlKeyword("PixelResolution", ".001");

  try {
    string border = "||||||||||||||||||||||||||||||||||||||||"
                    "||||||||||||||||||||||||||||||||||||||||";

    //||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||
    cout << border << endl;
    cout << "\t\t\t PLANETOGRAPHIC-POSITIVEWEST-180" << endl;
    cout << border  << endl << endl;
    //||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||
    Projection &proj = *ProjectionFactory::Create(lab, true);
    TProjection *p1 = (TProjection *) &proj;
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
    cout << "    Testing SetGround..." << endl;
    testSetGround(p1);
    cout << endl;
    cout << "\t\t\t\t/-----------------------------------------/" << endl;
    cout << endl;
    cout << "    Testing SetGround..." << endl;
    if (p1->SetGround(-20, 100)) {
      cout << "Latitude:            " << p1->Latitude() << endl;
      cout << "Longitude:           " << p1->Longitude() << endl;
      cout << "XCoord:              " << p1->XCoord() << endl;
      cout << "YCoord:              " << p1->YCoord() << endl;
      cout << "    Reverse (SetCoordinate)..." << endl;
      if (p1->SetCoordinate(p1->XCoord(), p1->YCoord())) {
        cout << "Latitude:            " << p1->Latitude() << endl;
        cout << "Longitude:           " << p1->Longitude() << endl;
        cout << "XCoord:              " << p1->XCoord() << endl;
        cout << "YCoord:              " << p1->YCoord() << endl;
      }
      else {
        cout << "SetCoordinate failed for x/y ("
             << toString(p1->XCoord()) << ", " << toString(p1->XCoord()) << ")" << endl;
      }
    }
    else {
      cout << "SetGround failed for lat/lon (-20, 100)" << endl;
    }
    cout << "\t\t\t\t/-----------------------------------------/" << endl;
    cout << endl;
    cout << "    Testing projection to origin and back\n"
            "    SetGround(0, center longitude)" << endl;
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
    cout << "    SetCoordinate(0, PI*sphRad)" << endl;
    p1->SetCoordinate(0.0, PI);
    cout << "Latitude:             " << p1->Latitude() << endl;
    cout << "Longitude:           " << p1->Longitude() << endl;
    cout << "XCoord:                " << p1->XCoord() << endl;
    cout << "YCoord:                " << p1->YCoord() << endl;
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
    cout << "    Mapping Group: " << endl;
    Pvl pvl1;
    pvl1.addGroup(p1->Mapping());
    cout << pvl1 << endl << endl;

    double minX, maxX, minY, maxY;
    p1->XYRange(minX, maxX, minY, maxY);
    cout << "XYRange method returns" << endl;
    cout << "    Minimum X:  " << minX << endl;
    cout << "    Maximum X:  " << maxX << endl;
    cout << "    Minimum Y:  " << minY << endl;
    cout << "    Maximum Y:  " << maxY << endl;
    cout << endl;
    if (p1->SetCoordinate(maxX, 0.0)) {
      cout << "            SetCoordinate(maxX, 0) returns lat/lon = " 
           << p1->Latitude() << " / " << p1->Longitude() << endl;
    }
    if (p1->SetCoordinate(0.0, maxY)) {
      cout << "            SetCoordinate(0, maxY) returns lat/lon = " 
           << p1->Latitude() << " / " << p1->Longitude()  << endl;
    }
    if (p1->SetCoordinate(minX, 0.0)) {
      cout << "            SetCoordinate(minX, 0) returns lat/lon = " 
           << p1->Latitude() << " / " << p1->Longitude()  << endl;
    }
    if (p1->SetCoordinate(0, minY)) {
      cout << "            SetCoordinate(0, minY) returns lat/lon = " 
           << p1->Latitude() << " / " << p1->Longitude()  << endl;
    }
    cout << endl;
    if (p1->SetGround(20.0, -90.0)) {
      cout << "            SetGround(20, -90) returns x max? ";
      cout << "(x,y) = (" << p1->XCoord() << ", " << p1->YCoord() << ")"<< endl;
    }
    if (p1->SetGround(20.0, 0.0)) {
      cout << "            SetGround(20, 0) returns y min? ";
      cout << "(x,y) = (" << p1->XCoord() << ", " << p1->YCoord() << ")"<< endl;
    }
    if (p1->SetGround(20.0, 90.0)) {
      cout << "            SetGround(20, 90) returns x min? ";
      cout << "(x,y) = (" << p1->XCoord() << ", " << p1->YCoord() << ")"<< endl;
    }
    if (p1->SetGround(80.0, 90.0)) {
      cout << "            SetGround(80, 90) returns y max? ";
      cout << "(x,y) = (" << p1->XCoord() << ", " << p1->YCoord() << ")"<< endl;
    }
    cout << endl;
    cout << endl;
    mapGroup.findKeyword("MinimumLatitude").setValue("-90.0");
    mapGroup.findKeyword("MaximumLatitude").setValue("90.0");
    mapGroup.findKeyword("MinimumLongitude").setValue("-80.0");
    mapGroup.findKeyword("MaximumLongitude").setValue("-30.0");
    /* 
     * 
     * 
     *                      :               :
     *                     :::             :::
     *                   ::::::::       ::::::::
     *                 ::::::::::::::::::::::::::
     *                ::::::::::::::::::::::::::::
     *                 ::::::::::::::::::::::::::
     *                    ::::::::::::::::::::
     *                        :::::::::::::
     *                           :::::::    
     * 
     */
    TProjection *p1a = (TProjection *) ProjectionFactory::Create(lab);
    cout << "Given: " << endl;
    cout << "    Minimum Latitude:  " << p1a->MinimumLatitude() << endl;
    cout << "    Maximum Latitude:  " << p1a->MaximumLatitude() << endl;
    cout << "    Minimum Longitude: " << p1a->MinimumLongitude() << endl;
    cout << "    Maximum Longitude: " << p1a->MaximumLongitude() << endl << endl;
    cout << "    Mapping Group: " << endl;
    Pvl pvl1a;
    pvl1a.addGroup(p1a->Mapping());
    cout << pvl1a << endl << endl;

    p1a->XYRange(minX, maxX, minY, maxY);
    cout << "XYRange method returns" << endl;
    cout << "    Minimum X:  " << minX << endl;
    cout << "    Maximum X:  " << maxX << endl;
    cout << "    Minimum Y:  " << minY << endl;
    cout << "    Maximum Y:  " << maxY << endl;
    cout << endl;
    if (p1a->SetCoordinate(maxX, 0.0)) {
      cout << "            SetCoordinate(maxX, 0) returns lat/lon = " 
           << p1a->Latitude() << " / " << p1a->Longitude()  << endl;
    }
    if (p1a->SetCoordinate(minX, minY)) {
      cout << "            SetCoordinate(minX, minY) returns lat/lon = " 
           << p1a->Latitude() << " / " << p1a->Longitude()  << endl;
    }
    if (p1a->SetCoordinate(minX, maxY)) {
      cout << "            SetCoordinate(minX, maxY) returns lat/lon = " 
           << p1a->Latitude() << " / " << p1a->Longitude()  << endl;
    }
    cout << endl;
    if (p1a->SetGround(90.0, 0.0)) {
      cout << "            SetGround(90, 0) returns x min and y max? ";
      cout << "(x,y) = (" << p1a->XCoord() << ", " << p1a->YCoord() << ")"<< endl;
    }
    if (p1a->SetGround(-90.0, 0.0)) {
      cout << "            SetGround(-90, 0) returns x min and y min? ";
      cout << "(x,y) = (" << p1a->XCoord() << ", " << p1a->YCoord() << ")"<< endl;
    }
    if (p1a->SetGround(0.0, -80.0)) {
      cout << "            SetGround(equator, minLon) returns x max? ";
      cout << "(x,y) = (" << p1a->XCoord() << ", " << p1a->YCoord() << ")"<< endl;
    }
    cout << endl;
    cout << endl;
    mapGroup.findKeyword("MinimumLongitude").setValue("-90.0");
    mapGroup.findKeyword("MaximumLongitude").setValue("90.0");
    cout << "Given: " << endl;
    TProjection *p1b = (TProjection *) ProjectionFactory::Create(lab);
    cout << "    Minimum Latitude:  " << p1b->MinimumLatitude() << endl;
    cout << "    Maximum Latitude:  " << p1b->MaximumLatitude() << endl;
    cout << "    Minimum Longitude: " << p1b->MinimumLongitude() << endl;
    cout << "    Maximum Longitude: " << p1b->MaximumLongitude() << endl << endl;
    Pvl pvl1b;
    pvl1b.addGroup(p1b->Mapping());
    cout << pvl1b << endl << endl;

    p1b->XYRange(minX, maxX, minY, maxY);
    p1b->XYRange(minX, maxX, minY, maxY);
    cout << "XYRange method returns" << endl;
    cout << "    Minimum X:  " << minX << endl;
    cout << "    Maximum X:  " << maxX << endl;
    cout << "    Minimum Y:  " << minY << endl;
    cout << "    Maximum Y:  " << maxY << endl;
    cout << endl;
    if (p1b->SetCoordinate(maxX,0)) {
      cout << "            SetCoordinate(maxX,0) returns lat/lon = " 
           << p1b->Latitude() << " / " << p1b->Longitude()  << endl;
    }
    if (p1b->SetCoordinate(0, maxY)) {
      cout << "            SetCoordinate(0,maxY) returns lat/lon = " 
           << p1b->Latitude() << " / " << p1b->Longitude()  << endl;
    }
    if (p1b->SetCoordinate(minX,0)) {
      cout << "            SetCoordinate(minX,0) returns lat/lon = " 
           << p1b->Latitude() << " / " << p1b->Longitude()  << endl;
    }
    if (p1b->SetCoordinate(0, minY)) {
      cout << "            SetCoordinate(0,minY) returns lat/lon = " 
           << p1b->Latitude() << " / " << p1b->Longitude()  << endl;
    }
    cout << endl;
    if (p1b->SetGround(0.0, -90.0)) {
      cout << "            SetGround(0, -90) returns x max? ";
      cout << "(x,y) = (" << p1b->XCoord() << ", " << p1b->YCoord() << ")"<< endl;
    }
    if (p1b->SetGround(90.0, 0.0)) {
      cout << "            SetGround(90, 0) returns near y max? ";
      cout << "(x,y) = (" << p1b->XCoord() << ", " << p1b->YCoord() << ")"<< endl;
    }
    if (p1b->SetGround(0.0, 90.0)) {
      cout << "            SetGround(0, 90) returns x min? ";
      cout << "(x,y) = (" << p1b->XCoord() << ", " << p1b->YCoord() << ")"<< endl;
    }
    if (p1b->SetGround(-90.0, 0.0)) {
      cout << "            SetGround(-90, 0) returns y min? ";
      cout << "(x,y) = (" << p1b->XCoord() << ", " << p1b->YCoord() << ")"<< endl;
    }
    cout << endl;
    cout << endl;
    cout << endl;
    cout << endl;
    //||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||
    cout << border << endl;
    cout << "\t\t\t PLANETOCENTRIC-POSITIVEEAST-360" << endl;
    cout << border << endl << endl;
    //||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||
    mapGroup.findKeyword("LatitudeType").setValue("Planetocentric");
    mapGroup.findKeyword("LongitudeDirection").setValue("PositiveEast");
    mapGroup.findKeyword("EquatorialRadius").setValue("3.0");
    mapGroup.findKeyword("PolarRadius").setValue("1.0");

    mapGroup.findKeyword("CenterLongitude").setValue("180.0");
    mapGroup.findKeyword("MinimumLatitude").setValue("-90.0");
    mapGroup.findKeyword("MaximumLatitude").setValue("90.0");
    mapGroup.findKeyword("MinimumLongitude").setValue("90");
    mapGroup.findKeyword("MaximumLongitude").setValue("270");
    mapGroup.findKeyword("LongitudeDomain").setValue("360");

    TProjection *p2 = (TProjection *) ProjectionFactory::Create(lab);
    cout << mapGroup["CenterLongitude"] << endl;
    cout << "EquatorialRadius = " << p2->EquatorialRadius() << endl;
    cout << "PolarRadius = " << p2->PolarRadius() << endl;
    cout << "Eccentricity = " << p2->Eccentricity() << endl;
    cout << "TrueScaleLatitude = " << p2->TrueScaleLatitude() << endl << endl;
    cout << "\t\t\t\t/-----------------------------------------/" << endl;
    cout << endl;

    cout << "    Testing SetGround..." << endl;
    p2->SetGround(-20, 100);
    cout << "Latitude:            " << p2->Latitude() << endl;
    cout << "Longitude:           " << p2->Longitude() << endl;
    cout << "XCoord:              " << p2->XCoord() << endl;
    cout << "YCoord:              " << p2->YCoord() << endl;
    // we do not have relative scale factor methods in Projection as of 06/2012
    // so these need to be tested with a UpturnedEllipsoidTransverseAzimuthal object
    // specifically.  The following values were calculated by hand to verify.
    cout << endl;
    cout << "\t\t\t\t/-----------------------------------------/" << endl;
    cout << endl;

    cout << "    Testing SetCoordinate..." << endl;
    p2->SetCoordinate(-4.2339303, 4.0257775);
    cout << "Latitude:            " << p2->Latitude() << endl;
    cout << "Longitude:           " << p2->Longitude() << endl;
    cout << "XCoord:              " << p2->XCoord() << endl;
    cout << "YCoord:              " << p2->YCoord() << endl;
    cout << endl;
    cout << "\t\t\t\t/-----------------------------------------/" << endl;
    cout << endl;

    cout << "    Testing projection to origin and back\n"
            "    SetGround(0, 180)" << endl;
    p2->SetGround(0.0, 180.0);
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

    cout << "    SetCoordinate(0, 0.7336148)" << endl;
    p2->SetCoordinate(0.0, 0.7336148);
    cout << "Latitude:            " << p2->Latitude() << endl;
    cout << "Longitude:           " << p2->Longitude() << endl;
    cout << "XCoord:               " << p2->XCoord() << endl;
    cout << "YCoord:               " << p2->YCoord() << endl;
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
    cout << "    Mapping Group: " << endl;

    Pvl pvl2;
    pvl2.addGroup(p2->Mapping());
    cout << pvl2 << endl << endl;

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
    if (p2->SetCoordinate(maxX,0)) {
      cout << "            SetCoordinate(maxX,0) returns lat/lon = " 
           << p2->Latitude() << " / " << p2->Longitude()  << endl;
    }
    if (p2->SetCoordinate(0, maxY)) {
      cout << "            SetCoordinate(0,maxY) returns lat/lon = " << 
           p2->Latitude() << " / " << p2->Longitude()  << endl;
    }
    if (p2->SetCoordinate(minX,0)) {
      cout << "            SetCoordinate(minX,0) returns lat/lon = " 
           << p2->Latitude() << " / " << p2->Longitude()  << endl;
    }
    if (p2->SetCoordinate(0, minY)) {
      cout << "            SetCoordinate(0,minY) returns lat/lon = " 
           << p2->Latitude() << " / " << p2->Longitude()  << endl;
    }
    cout << endl;
    if (p2->SetGround(0.0, 270.0)) {
      cout << "            SetGround(0, 270) returns x max? ";
      cout << "(x,y) = (" << p2->XCoord() << ", " << p2->YCoord() << ")"<< endl;
    }
    if (p2->SetGround(90.0, 180.0)) {
      cout << "            SetGround(90, 180) returns y max? ";
      cout << "(x,y) = (" << p2->XCoord() << ", " << p2->YCoord() << ")"<< endl;
    }
    if (p2->SetGround(0.0, 90.0)) {
      cout << "            SetGround(0, 90) returns x min? ";
      cout << "(x,y) = (" << p2->XCoord() << ", " << p2->YCoord() << ")"<< endl;
    }
    if (p2->SetGround(-90.0, 180.0)) {
      cout << "            SetGround(-90, 180) returns y min? ";
      cout << "(x,y) = (" << p2->XCoord() << ", " << p2->YCoord() << ")"<< endl;
    }
    cout << endl;
    cout << endl;

    mapGroup.findKeyword("MaximumLatitude").setValue("90.0");
    mapGroup.findKeyword("MinimumLongitude").setValue("100.0");
    mapGroup.findKeyword("MaximumLongitude").setValue("110.0");

    TProjection *p2a = (TProjection *) ProjectionFactory::Create(lab);
    cout << std::setprecision(7);
    cout << "Given: " << endl;
    cout << "    Minimum Latitude:  " << p2a->MinimumLatitude() << endl;
    cout << "    Maximum Latitude:  " << p2a->MaximumLatitude() << endl;
    cout << "    Minimum Longitude: " << p2a->MinimumLongitude() << endl;
    cout << "    Maximum Longitude: " << p2a->MaximumLongitude() << endl << endl;
    cout << "    Mapping Group: " << endl;
    Pvl pvl2a;
    pvl2a.addGroup(p2a->Mapping());
    cout << pvl2a << endl << endl;

    minX=0; maxX=0; minY=0; maxY=0;
    p2a->XYRange(minX, maxX, minY, maxY);
    cout << "XYRange method returns" << endl;
    cout << "    Minimum X:  " << minX << endl;
    cout << "    Maximum X:  " << maxX << endl;
    cout << "    Minimum Y:  " << minY << endl;
    cout << "    Maximum Y:  " << maxY << endl;
    cout << endl;    
    cout << endl;

    if(p2a->SetCoordinate(maxX, 0.0)) {
      cout << "            SetCoordinate(maxX, 0) returns lat/lon = " 
           << p2a->Latitude() << " / " << p2a->Longitude()  << endl;
    }
    if(p2a->SetCoordinate(0.0, maxY)) {
      cout << "            SetCoordinate(0, maxY) returns lat/lon = " << 
           p2a->Latitude() << " / " << p2a->Longitude()  << endl;
    }
    if(p2a->SetCoordinate(minX, 0.0)) {
      cout << "            SetCoordinate(minX, 0) returns lat/lon = " 
           << p2a->Latitude() << " / " << p2a->Longitude()  << endl;
    }
    if(p2a->SetCoordinate(maxX, minY)) {
      cout << "            SetCoordinate(maxX, minY) returns lat/lon = " 
           << p2a->Latitude() << " / " << p2a->Longitude()  << endl;
    }
    cout << endl;
    if (p2a->SetGround(0.0, 180.0)) {
      cout << "            SetGround(0, 180) returns x max? ";
      cout << "(x,y) = (" << p2a->XCoord() << ", " << p2a->YCoord() << ")"<< endl;
    }
    if (p2a->SetGround(90.0, 180.0)) {
      cout << "            SetGround(90, 180) returns y max? ";
      cout << "(x,y) = (" << p2a->XCoord() << ", " << p2a->YCoord() << ")"<< endl;
    }
    if (p2a->SetGround(0.0, 100.0)) {
      cout << "            SetGround(0, 100) returns x min? ";
      cout << "(x,y) = (" << p2a->XCoord() << ", " << p2a->YCoord() << ")"<< endl;
    }
    if (p2a->SetGround(-90.0, 180.0)) {
      cout << "            SetGround(-90, 180) returns y min? ";
      cout << "(x,y) = (" << p2a->XCoord() << ", " << p2a->YCoord() << ")"<< endl;
    }
    cout << endl;
    cout << endl;
    cout << endl;
    cout << endl;

    //||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||
    cout << border << endl;
    cout << "\t\t\t TESTING OTHER METHODS" << endl;
    cout << border << endl << endl;
    //||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||
    mapGroup.findKeyword("PolarRadius").setValue(std::to_string(p2a->EquatorialRadius()));
    TProjection *p9 = (TProjection *) ProjectionFactory::Create(lab);
    TProjection *s = p1;
    cout << "Name:                       " << s->Name() << endl;
    cout << "Version:                    " << p1->Version() << endl;
    cout << "Rotation:                   " << p1->Rotation() << endl;
    cout << "TrueScaleLatitude:          " << p1->TrueScaleLatitude() << endl;
    cout << "Testing operator==  (True): " << (*s == *s) << endl;
    cout << "Testing operator==  (True): " << (*s == proj) << endl;
    // different lat/lon range, all other properties the same
    cout << "Testing operator==  (True): " << (*p1 == *p1a) << endl;
    // same CenterLatitude, different CenterLongitude
    cout << "Testing operator==  (False-different CenterLongitude):  " << (p2a == p1) << endl;
    // same CenterLatitude/CenterLongitude, different EquatorialRadius
    cout << "Testing operator==  (False-different EquatorialRadius): " << (p2 == p1) << endl;
    // same CenterLatitude/CenterLongitude/EquatorialRadius, different eccentricity
    cout << "Testing operator==  (False-different Eccentricity):     " << (p2a == p9) << endl;
    cout << endl;
    cout << "\t\t\t\t/-----------------------------------------/" << endl;
    cout << endl;
    cout << "Testing Mapping() methods" << endl;
    Pvl tmp1;
    tmp1.addGroup(p1->Mapping());
    cout << "Mapping() = " << endl;
    cout << tmp1 << endl;
    Pvl tmp2;
    tmp2.addGroup(p1->MappingLatitudes());
    cout << "MappingLatitudes() = " << endl;
    cout << tmp2 << endl;
    Pvl tmp3;
    tmp3.addGroup(p1->MappingLongitudes());
    cout << "MappingLongitudes() = " << endl;
    cout << tmp3 << endl;
    cout << endl;
    cout << endl;
    cout << endl;
    cout << endl;

    //||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||
    cout << border << endl;
    cout << "\t\t\t TESTING    GOOD = FALSE" << endl;
    cout << border << endl << endl;
    //||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||
    cout << "Set Ground using invalid Latitude/Longitude value" << endl;
    cout << "SetGround(Null, 0):    " << p1->SetGround( Null,  0.0) << endl;
    cout << "SetGround(0, Null):    " << p1->SetGround( 0.0,  Null) << endl;
    cout << "SetGround(-91, 0):     " << p1->SetGround(-91.0,  0.0) << endl;
    cout << "Set Coordinate using invalid x/y value" << endl;
    cout << "SetCoordinate(Null, 0):    " << p1->SetCoordinate(Null, 0.0) << endl;
    cout << "SetCoordinate(0, Null):    " << p1->SetCoordinate(0.0,  Null) << endl;
    cout << "Set Coordinate using x/y value off the planet" << endl;
    cout << "SetCoordinate(100000, 0):    " << p1->SetCoordinate(100000,  0) << endl;
    cout << endl;
    cout << "\t\t\t\t/-----------------------------------------/" << endl;
    cout << endl;

    //||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||
    cout << border << endl;
    cout << "\t\t\t TESTING ERRORS" << endl;
    cout << border << endl << endl;
    //||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||
    mapGroup.deleteKeyword("CenterLongitude");
    cout << "Error check: Missing center longitude keyword when default is not allowed" << endl;
    try {
      UpturnedEllipsoidTransverseAzimuthal p(lab);
    }
    catch(IException &e) {
      e.print();
    }
    cout << endl;
    cout << "\t\t\t\t/-----------------------------------------/" << endl;
    cout << endl;
    cout << "Error check: Min/Max Longitude not within 90 degrees of Center Longitude" << endl;

    // Minimum Longitude too small case
    try {
      mapGroup += PvlKeyword("CenterLongitude", "180.0");
      mapGroup.findKeyword("MinimumLongitude").setValue("0.0");
      mapGroup.findKeyword("MaximumLongitude").setValue("270.0");
      UpturnedEllipsoidTransverseAzimuthal p(lab);
    }
    catch(IException &e) {
      e.print();
    }

    // Maximum Longitude too large case
    try {
      mapGroup += PvlKeyword("CenterLongitude", "180.0");
      mapGroup.findKeyword("MinimumLongitude").setValue("90.0");
      mapGroup.findKeyword("MaximumLongitude").setValue("360.0");
      UpturnedEllipsoidTransverseAzimuthal p(lab);
    }
    catch(IException &e) {
      e.print();
    }
    cout << endl;
    cout << "\t\t\t\t/-----------------------------------------/" << endl;
  }
  catch(IException &e) {
    e.print();
  }
}

// The Latitude step of 18 is 180/10 (Test values at 10 equally-spaced latitudes.)
// The Longitude step of 72 is 360/10 (Test values at 10 equally-space longitudes for each lat.) 
void testSetGround(TProjection *p) {
  for (double lat = 90.0; lat >= -90.0; lat-=18.0) {
    for (double lon = -360.0; lon <= 360.0; lon+=72.0) {
      cout << "[Lat/Lon  (" 
           << toString(lat) << ", " << toString(lon) << ")]";
      bool success = p->SetGround(lat, lon);
      if (success) {
             cout << "  ---->  [x/y  ("
             << p->XCoord() << ",   " << p->YCoord() << ")]";
        success = p->SetCoordinate(p->XCoord(), p->YCoord());
        if (success) {
          cout << "  ---->  [Lat/Lon  (" 
               << toString(p->Latitude()) << ", " << toString(p->Longitude()) << ")]";
          printErrors(lat, p->Latitude(), lon, p->Longitude());
        }
        else {
          cout << "  ---->  Reverse Projection Fails";
        }
        cout << endl;
      }
      else {
        cout << "  ---->  Forward Projection Fails";
      }
    }
    cout << endl;
  }
}


void printErrors(double origLat, double projectedLat, double origLon, double projectedLon) {
  double latErr = qAbs(origLat - projectedLat);
  double lonErr = qAbs(TProjection::To360Domain(origLon) - TProjection::To360Domain(projectedLon));

  if (latErr < 1.0e-13) {
    cout << "  ****[Lat Ok]    ";
  }
  else {
    cout << "[Lat Error: " << toString(latErr) << "]    ";
  }

  if (lonErr > 180) {
    lonErr = 360.0 - lonErr;
  }
  if (lonErr < 1.0e-10 || qFuzzyCompare(90.0, qAbs(origLat))) {
    cout << "[Lon Ok]****";
  }
  else {
    cout << "[Lon Error: " << toString(lonErr) << "]****";
  }
}
