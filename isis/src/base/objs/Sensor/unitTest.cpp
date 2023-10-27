/** This is free and unencumbered software released into the public domain.
The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */
#include <iostream>
#include <iomanip>

#include <QString>

#include "Angle.h"
#include "Distance.h"
#include "FileName.h"
#include "IException.h"
#include "iTime.h"
#include "Latitude.h"
#include "Longitude.h"
#include "Sensor.h"
#include "SurfacePoint.h"

#include "Preference.h"

using namespace Isis;
using namespace std;
/**
 * UnitTest for Spice class.
 *
 * @internal
 * @history 2009-03-24  Tracie Sucharski - Replace obsolete keywords
 *          SpacecraftPosition and SpacecraftPointing with InstrumentPosition
 *          and InstrumentPointing.
 * @history 2015-09-01 Ian Humphrey and Makayla Shepherd - Added MySensor class to allow
 *                         Sensor object instantiation.
 */

class MySensor : public Sensor {
  public:
    MySensor(Cube &cube) : Sensor(cube) { }
    // implement Sensor's pure virtual methods as necessary
    virtual QString instrumentNameLong() const { return QString("My Sensor"); }
    virtual QString instrumentNameShort() const { return QString("MySensor"); }
    virtual QString spacecraftNameLong() const { return QString("My Spacecraft"); }
    virtual QString spacecraftNameShort() const { return QString("MySpacecraft"); }
};

int main(int argc, char *argv[]) {

  Preference::Preferences(true);

  try {

    cerr << setprecision(9);
    cerr << "Unit test for Sensor" << endl;

    Cube dummyCube("$ISISTESTDATA/isis/src/base/unitTestData/isisTruth.cub", "r");

    Pvl &lab = *dummyCube.label();
    PvlGroup inst("INSTRUMENT");
    inst += PvlKeyword("TargetName", "Mars");
    lab.addGroup(inst);

    PvlGroup kern("Kernels");
    FileName f("$ISISTESTDATA/isis/src/base/unitTestData/kernels");
    QString dir = f.expanded() + "/";

    kern += PvlKeyword("NaifFrameCode", "-94031");
    kern += PvlKeyword("LeapSecond", dir.toStdString() + "naif0007.tls");
    kern += PvlKeyword("SpacecraftClock", dir.toStdString() + "MGS_SCLKSCET.00045.tsc");
    kern += PvlKeyword("TargetPosition", dir.toStdString() + "de405.bsp");
    kern += PvlKeyword("TargetAttitudeShape", dir.toStdString() + "pck00006.tpc");
    kern += PvlKeyword("Instrument", dir.toStdString() + "mocSensorUnitTest.ti");
//  kern += PvlKeyword("InstrumentAddendum",dir.toStdString()+"mocAddendum.ti");
    kern += PvlKeyword("InstrumentAddendum", "");
    kern += PvlKeyword("InstrumentPosition", dir.toStdString() + "moc.bsp");
    kern += PvlKeyword("InstrumentPointing", dir.toStdString() + "moc.bc");
    kern += PvlKeyword("Frame", "");
    lab.addGroup(kern);

    // Setup
    double startTime = -69382819.0;
    double endTime = -69382512.0;
    double slope = (endTime - startTime) / (10 - 1);
    MySensor spi(dummyCube);
    spi.instrumentRotation()->SetTimeBias(-1.15);

    double v[3] = { 0.0, 0.0, 1.0 };
    double p[3];
    double scSurf[3];

    cerr << setprecision(8);

    // Testing Set Look Direction
    cerr << "Test SetLookDirection using ShapeModel=Null" << endl;
    for(int i = 0; i < 10; i++) {
      double t = startTime + (double) i * slope;
      spi.setTime(iTime(t));
      cerr << "Has Intersection    = " << spi.HasSurfaceIntersection() << endl;

      spi.SetLookDirection(v);

      cerr << "Has Intersection    = " << spi.HasSurfaceIntersection() << endl;
      cerr << "Latitude            = " << spi.UniversalLatitude() << endl;
      cerr << "Longitude           = " << spi.UniversalLongitude() << endl;
      spi.Coordinate(p);
      cerr << "Point               = " << p[0] << " " << p[1] << " " << p[2]
           << endl;
      cerr << "Local Radius        = " //< setprecision(6)
           << spi.LocalRadius().meters() << endl;
      cerr << "Phase               = " << spi.PhaseAngle() << endl;
      cerr << "Emission            = " << spi.EmissionAngle() << endl;
      cerr << "Incidence           = " << spi.IncidenceAngle() << endl;
      spi.LookDirection(p);
      cerr << "Look Direction      = " << p[0] << " " << p[1] << " " << p[2]
           << endl;
      spi.SpacecraftSurfaceVector(scSurf);
      cerr << "Spacecraft Surface Vector = " << scSurf[0] << " " << scSurf[1]
           << " " << scSurf[2] <<endl;
      cerr << "Slant Distance      = " << spi.SlantDistance() << endl;
      cerr << "Local Solar Time    = " << spi.LocalSolarTime() << endl;
      cerr << "Spacecraft Altitude = " << spi.SpacecraftAltitude() << endl;
      cerr << "Solar Distance      = " << spi.SolarDistance() << endl;
    }
    cerr << endl;

    // Test bad look direction
    cerr << "Test bad look direction using ShapeModel=Null" << endl;
    p[0] = 0.0;
    p[1] = 0.0;
    p[2] = -1.0;
    spi.SetLookDirection(p);
    cerr << "Has Intersection    = " << spi.HasSurfaceIntersection() << endl;
    cerr << endl;

    // Test SetUniversalGround
    cerr << "Test SetUniversalGround (lat/lon only) using ShapeModel=Null"
         << endl;
    spi.SetUniversalGround(11.57143551329, 223.328646604);
    cerr << "Has Intersection    = " << spi.HasSurfaceIntersection() << endl;
    cerr << "Latitude            = " << spi.UniversalLatitude() << endl;
    cerr << "Longitude           = " << spi.UniversalLongitude() << endl;
    spi.Coordinate(p);
    cerr << "Point               = " << p[0] << " " << p[1] << " " << p[2]
         << endl;
    cerr << "Local Radius        = " << spi.LocalRadius().meters() << endl;
    cerr << "Phase               = " << spi.PhaseAngle() << endl;
    cerr << "Emission            = " << spi.EmissionAngle() << endl;
    cerr << "Incidence           = " << spi.IncidenceAngle() << endl;
    spi.LookDirection(p);
    cerr << "Look Direction      = " << p[0] << " " << p[1] << " " << p[2]
         << endl;
    spi.SpacecraftSurfaceVector(scSurf);
    cerr << "Spacecraft Surface Vector = " << scSurf[0] << " " << scSurf[1]
         << " " << scSurf[2] <<endl;
    cerr << "Slant Distance      = " << spi.SlantDistance() << endl;
    cerr << "Local Solar Time    = " << spi.LocalSolarTime() << endl;
    cerr << "Spacecraft Altitude = " << spi.SpacecraftAltitude() << endl;
    cerr << "Solar Distance      = " << spi.SolarDistance() << endl;
    cerr << endl;

    // Test SetUniversalGround
    cerr << "Test SetUniversalGround (lat/lon/radius) using ShapeModel=Null"
         << endl;
    Latitude lat(11.57143551329, Angle::Degrees);
    Longitude lon(223.328646604, Angle::Degrees);
    Distance radius(3400., Distance::Meters);
    SurfacePoint tmp(lat, lon, radius);
    spi.SetGround(tmp);

    cerr << "Has Intersection    = " << spi.HasSurfaceIntersection() << endl;
    cerr << "Latitude            = " << spi.UniversalLatitude() << endl;
    cerr << "Longitude           = " << spi.UniversalLongitude() << endl;
    cerr << "Radius              = " << spi.LocalRadius().meters() << endl;
    spi.Coordinate(p);
    cerr << "Point               = " << p[0] << " " << p[1] << " " << p[2]
         << endl;
    cerr << "Local Radius        = " << spi.LocalRadius().meters() << endl;
    cerr << "Phase               = " << spi.PhaseAngle() << endl;
    cerr << "Emission            = " << spi.EmissionAngle() << endl;
    cerr << "Incidence           = " << spi.IncidenceAngle() << endl;
    spi.LookDirection(p);
    cerr << "Look Direction      = " << p[0] << " " << p[1] << " " << p[2]
         << endl;
    spi.SpacecraftSurfaceVector(scSurf);
    cerr << "Spacecraft Surface Vector = " << scSurf[0] << " " << scSurf[1]
         << " " << scSurf[2] <<endl;
    cerr << "Slant Distance      = " << spi.SlantDistance() << endl;
    cerr << "Local Solar Time    = " << spi.LocalSolarTime() << endl;
    cerr << "Spacecraft Altitude = " << spi.SpacecraftAltitude() << endl;
    cerr << "Solar Distance      = " << spi.SolarDistance() << endl;
    cerr << endl;

    // Test bad ground point
    cerr << "Test Bad ground point using ShapeModel=Null" << endl;
    spi.SetUniversalGround(11.57143551329, 43.328646604);
    cerr << "Has Intersection    = " << spi.HasSurfaceIntersection() << endl
         << endl << endl;

    kern.clear();

    kern += PvlKeyword("NaifFrameCode", "-94031");
    kern += PvlKeyword("LeapSecond", dir.toStdString() + "naif0007.tls");
    kern += PvlKeyword("SpacecraftClock",
                              dir.toStdString() + "MGS_SCLKSCET.00045.tsc");
    kern += PvlKeyword("TargetPosition", dir.toStdString() + "de405.bsp");
    kern += PvlKeyword("TargetAttitudeShape", dir.toStdString() + "pck00006.tpc");
    kern += PvlKeyword("Instrument", dir.toStdString() + "mocSensorUnitTest.ti");
    kern += PvlKeyword("InstrumentAddendum", "");
    kern += PvlKeyword("InstrumentPosition", dir.toStdString() + "moc.bsp");
    kern += PvlKeyword("InstrumentPointing", dir.toStdString() + "moc.bc");
    kern += PvlKeyword("Frame", "");
    kern += PvlKeyword("ShapeModel",
                        "$ISISDATA/base/dems/molaMarsPlanetaryRadius0004.cub");
    lab.deleteGroup("Kernels");
    lab.addGroup(kern);

    // Setup
    MySensor spi2(dummyCube);
    spi2.instrumentRotation()->SetTimeBias(-1.15);

    // Testing Set Look Direction
    cerr << "Test SetLookDirection using ShapeModel="
            "$ISISDATA/base/dems/molaMarsPlanetaryRadius0004.cub" << endl;
    for(int i = 0; i < 10; i++) {
      double t = startTime + (double) i * slope;
      spi2.setTime(iTime(t));
      cerr << "Has Intersection    = " << spi2.HasSurfaceIntersection() << endl;

      spi2.SetLookDirection(v);

      cerr << "Has Intersection    = " << spi2.HasSurfaceIntersection() << endl;
      cerr << "Latitude            = " << spi2.UniversalLatitude() << endl;
      cerr << "Longitude           = " << spi2.UniversalLongitude() << endl;
      spi2.Coordinate(p);
      cerr << "Point               = " << p[0] << " " << p[1] << " " << p[2]
           << endl;
      cerr << "Local Radius        = " << spi2.LocalRadius().meters() << endl;
      cerr << "Phase               = " << spi2.PhaseAngle() << endl;
      cerr << "Emission            = " << spi2.EmissionAngle() << endl;
      cerr << "Incidence           = " << spi2.IncidenceAngle() << endl;
      spi2.LookDirection(p);
      cerr << "Look Direction      = " << p[0] << " " << p[1] << " " << p[2]
           << endl;
      spi2.SpacecraftSurfaceVector(scSurf);
      cerr << "Spacecraft Surface Vector = " << scSurf[0] << " " << scSurf[1]
           << " " << scSurf[2] <<endl;
      cerr << "Slant Distance      = " << spi2.SlantDistance() << endl;
      cerr << "Local Solar Time    = " << spi2.LocalSolarTime() << endl;
      cerr << "Spacecraft Altitude = " << spi2.SpacecraftAltitude() << endl;
      cerr << "Solar Distance      = " << spi2.SolarDistance() << endl;
    }
    cerr << endl;

    // Test bad look direction
    cerr << "Test bad look direction using "
            "ShapeModel=$ISISDATA/base/dems/molaMarsPlanetaryRadius0004.cub"
         << endl;
    p[0] = 0.0;
    p[1] = 0.0;
    p[2] = -1.0;
    spi2.SetLookDirection(p);
    cerr << "Has Intersection    = " << spi2.HasSurfaceIntersection() << endl;
    cerr << endl;

    // Test SetUniversalGround
    cerr << "Test SetUniversalGround (lat/lon only) using "
            "ShapeModel=$ISISDATA/base/dems/molaMarsPlanetaryRadius0004.cub"
         << endl;
    spi2.SetUniversalGround(11.57143551329, 223.328646604);
    cerr << "Has Intersection    = " << spi2.HasSurfaceIntersection() << endl;
    cerr << "Latitude            = " << spi2.UniversalLatitude() << endl;
    cerr << "Longitude           = " << spi2.UniversalLongitude() << endl;
    spi2.Coordinate(p);
    cerr << "Point               = " << p[0] << " " << p[1] << " " << p[2]
         << endl;
    cerr << "Local Radius        = " << spi2.LocalRadius().meters() << endl;
    cerr << "Phase               = " << spi2.PhaseAngle() << endl;
    cerr << "Emission            = " << spi2.EmissionAngle() << endl;
    cerr << "Incidence           = " << spi2.IncidenceAngle() << endl;
    spi2.LookDirection(p);
    cerr << "Look Direction      = " << p[0] << " " << p[1] << " " << p[2]
         << endl;
    spi2.SpacecraftSurfaceVector(scSurf);
    cerr << "Spacecraft Surface Vector = " << scSurf[0] << " " << scSurf[1]
         << " " << scSurf[2] <<endl;
    cerr << "Slant Distance      = " << spi2.SlantDistance() << endl;
    cerr << "Local Solar Time    = " << spi2.LocalSolarTime() << endl;
    cerr << "Spacecraft Altitude = " << spi2.SpacecraftAltitude() << endl;
    cerr << "Solar Distance      = " << spi2.SolarDistance() << endl;
    cerr << endl;

    // Test SetUniversalGround
    cerr << "Test SetUniversalGround (lat/lon/radius) using "
            "ShapeModel=$ISISDATA/base/dems/molaMarsPlanetaryRadius0004.cub"
         << endl;
    spi2.SetUniversalGround(11.57143551329, 223.328646604, 3400.);
    cerr << "Has Intersection    = " << spi2.HasSurfaceIntersection() << endl;
    cerr << "Latitude            = " << spi2.UniversalLatitude() << endl;
    cerr << "Longitude           = " << spi2.UniversalLongitude() << endl;
    cerr << "Radius              = " << spi2.LocalRadius().meters() << endl;
    spi2.Coordinate(p);
    cerr << "Point               = " << p[0] << " " << p[1] << " " << p[2]
         << endl;
    cerr << "Local Radius        = " << spi2.LocalRadius().meters() << endl;
    cerr << "Phase               = " << spi2.PhaseAngle() << endl;
    cerr << "Emission            = " << spi2.EmissionAngle() << endl;
    cerr << "Incidence           = " << spi2.IncidenceAngle() << endl;
    spi2.LookDirection(p);
    cerr << "Look Direction      = " << p[0] << " " << p[1] << " " << p[2]
         << endl;
    spi2.SpacecraftSurfaceVector(scSurf);
    cerr << "Spacecraft Surface Vector = " << scSurf[0] << " " << scSurf[1]
         << " " << scSurf[2] <<endl;
    cerr << "Slant Distance      = " << spi2.SlantDistance() << endl;
    cerr << "Local Solar Time    = " << spi2.LocalSolarTime() << endl;
    cerr << "Spacecraft Altitude = " << spi2.SpacecraftAltitude() << endl;
    cerr << "Solar Distance      = " << spi2.SolarDistance() << endl;
    cerr << endl;

    // Test bad ground point
    cerr << "Test Bad ground point using "
            "ShapeModel=$ISISDATA/base/dems/molaMarsPlanetaryRadius0004.cub"
         << endl;
    spi2.SetUniversalGround(11.57143551329, 43.328646604);
    cerr << "Has Intersection    = " << spi2.HasSurfaceIntersection() << endl;
    
    // Test name methods
    cerr << "Test name methods ..." << endl;
    MySensor s(dummyCube);
    cout << "Spacecraft Name Long: " << s.spacecraftNameLong() << endl;
    cout << "Spacecraft Name Short: " << s.spacecraftNameShort() << endl;
    cout << "Instrument Name Long: " << s.instrumentNameLong() << endl;
    cout << "Instrument Name Short: " << s.instrumentNameShort() << endl << endl;
  }
  catch(IException &e) {
    e.print();
  }
}
