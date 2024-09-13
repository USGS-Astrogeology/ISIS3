/** This is free and unencumbered software released into the public domain.
The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */

 /**
 * @author 2003-03-13 Jeff Anderson?
 *
 * @internal
 *  @history 2012-10-11 Debbie A. Cook - Updated to use new Target and Shape Model classes.
 *  @history 2016-10-19 Kristin Berry - Updated to test new hasTime() function and new exception for
 *                                      unset time. 
 */
#include <iostream>
#include <iomanip>

#include "Cube.h"
#include "Distance.h"
#include "FileName.h"
#include "IException.h"
#include "iTime.h"
#include "Longitude.h"
#include "Spice.h"
#include "Preference.h"
#include "Target.h"

using namespace Isis;
using namespace std;

   // public methods not tested:
    // Spice(Pvl &lab, bool noTables);
    // SpicePosition *sunPosition() const;
    // SpicePosition *instrumentPosition() const;

/**
 * UnitTest for Spice class.
 *
 * @internal
 *   @history 2009-03-23 Tracie Sucharski - Removed old keywords SpacecraftPosition and
 *                           SpacecraftPointing with the corrected InstrumentPosition and
 *                           InstrumentPointing.
 *   @history 2016-10-27 Kristin Berry - 
 */
class MySpice : public Spice {
  public:
    MySpice(Cube &cube) : Spice(cube) {
      cout << "BodyCode        = " << naifBodyCode()      << endl;
      cout << "SpkCode         = " << naifSpkCode()       << endl;
      cout << "CkCode          = " << naifCkCode()        << endl;
      cout << "IkCode          = " << naifIkCode()        << endl;
      cout << "SclkCode        = " << naifSclkCode()      << endl;
      cout << "BodyFrameCode   = " << naifBodyFrameCode() << endl;
      cout << endl;
    }

    int MyInteger(QString key) {
      return getInteger(key, 0);
    }

    double MyDouble(QString key) {
      return getDouble(key, 0);
    }

    QString MyString(QString key) {
      return getString(key, 0);
    }

    void MyOutput() {
      cout << "BJ is " << endl;
      vector<double> BJ = bodyRotation()->Matrix();
      for(int i = 0; i < (int)BJ.size(); i++) {
        cout << BJ[i] << endl;
      }
      vector<double> IJ = instrumentRotation()->Matrix();
      vector<double> BI = IJ;
      mxmt_c((SpiceDouble( *)[3])&BJ[0], (SpiceDouble( *)[3])&IJ[0],
             (SpiceDouble( *)[3])&BI[0]);

      cout << "BP is " << endl;
      for(int i = 0; i < (int)BI.size(); i++) {
        cout << BI[i] << endl;
      }
    }

    double resolution() {
      return 1.;
    }
};



int main(int argc, char *argv[]) {
  Preference::Preferences(true);

  cout << setprecision(10);
  cout << "Unit test for Isis::Spice" << endl;

  Cube dummyCube("$ISISTESTDATA/isis/src/base/unitTestData/isisTruth.cub", "r");

  PvlGroup instrumentGroup("Instrument");
  instrumentGroup += PvlKeyword("TargetName", "Mard");

  PvlGroup kernelsGroup("Kernels");
  FileName f("$ISISTESTDATA/isis/src/base/unitTestData/kernels");
  std::string dir = f.expanded() + "/";
  kernelsGroup += PvlKeyword("NaifFrameCode", "-94031");
  kernelsGroup += PvlKeyword("LeapSecond", dir + "naif0007.tls");
  kernelsGroup += PvlKeyword("SpacecraftClock", dir + "MGS_SCLKSCET.00045.tsc");
  kernelsGroup += PvlKeyword("TargetPosition", dir + "de405.bsp");
  kernelsGroup += PvlKeyword("TargetAttitudeShape", dir + "pck00006.tpc");
  kernelsGroup += PvlKeyword("Instrument", dir + "mocSpiceUnitTest.ti");
  kernelsGroup += PvlKeyword("InstrumentAddendum", dir + "mocAddendum.ti");
  kernelsGroup += PvlKeyword("InstrumentPosition", dir + "moc.bsp");
  kernelsGroup += PvlKeyword("InstrumentPointing", dir + "moc.bc");
  kernelsGroup += PvlKeyword("Frame", "");

  // Time Setup
  double startTime = -69382819.0;
  double endTime = -69382512.0;
  double slope = (endTime - startTime) / (10 - 1);

  kernelsGroup += PvlKeyword("StartPadding", std::to_string(slope));
  kernelsGroup += PvlKeyword("EndPadding", std::to_string(slope));

  Pvl &lab = *dummyCube.label();
  PvlObject &isisCubeObj = lab.findObject("IsisCube");
  isisCubeObj.addGroup(instrumentGroup);
  isisCubeObj.addGroup(kernelsGroup);

  // Test bad target
  try {
    cout << "Testing unknown target ..." << endl;
    MySpice spi(dummyCube);
  }
  catch(IException &e) {
    e.print();
    cout << endl;
  }

  // Test bad getInteger
  PvlGroup &temp = lab.findGroup("Instrument", Pvl::Traverse);
  temp.addKeyword(PvlKeyword("TargetName", "Mars"), Pvl::Replace);
  cout << "Creating Spice object ..." << endl;
  MySpice spi(dummyCube);
  spi.instrumentRotation()->SetTimeBias(-1.15);
  cout << endl;

  try {
    cout << "Testing unknown integer keyword ... " << endl;
    spi.MyInteger("BadInteger");
  }
  catch(IException &e) {
    e.print();
    cout << endl;
  }

  // Test bad getDouble
  try {
    cout << "Testing unknown double keyword ... " << endl;
    spi.MyDouble("BadDouble");
  }
  catch(IException &e) {
    e.print();
    cout << endl;
  }

  // Test bad getString
  try {
    cout << "Testing unknown string keyword ... " << endl;
    spi.MyString("BadString");
  }
  catch(IException &e) {
    e.print();
    cout << endl;
  }

  // Test attempt to get time before time has been set
  try {
    cout << "Testing time() before time has been set... " << endl;
    cout << "Has time been set? " << spi.isTimeSet() << endl; 
    spi.time(); 
  } 
  catch (IException &e) {
    e.print();
    cout << endl;
  }

  // Test good gets
  cout << "Testing convience get methods ... " << endl;
  cout << "Label has kernels? " << spi.hasKernels(lab) << endl;
  cout << spi.MyInteger("FRAME_MGS_MOC") << endl;
  cout << spi.MyDouble("INS-94030_NA_FOCAL_LENGTH") << endl;
  cout << spi.MyString("FRAME_-94031_NAME").toStdString() << endl;
  cout << endl;

  // Testing radius
  cout << "Testing radius ... " << endl;
  Distance radii[3];
  spi.radii(radii);
  cout << "Radii[0]:  " << radii[0].kilometers() << endl;
  cout << "Radii[1]:  " << radii[1].kilometers() << endl;
  cout << "Radii[2]:  " << radii[2].kilometers() << endl;
  cout << endl;
  cout << "Solar Longitude = " << spi.solarLongitude().positiveEast() << endl;
  cout << "Resolution      = " << spi.resolution() << endl;

  // Normal testing (no cache)
  cout << "Testing without cache ... " << endl;
  for(int i = 0; i < 10; i++) {
    double t = startTime + (double) i * slope;
    spi.setTime(t);
    cout << "Time             = " << spi.time().Et() << endl;
    cout << "Clock Time (ET)  = " << spi.getClockTime("895484264:57204").Et() << endl;
    double p[3];
    spi.instrumentPosition(p);
    cout << "Spacecraft          (B) = " << p[0] << " " << p[1] << " " << p[2] << endl;
    double v[3];
    spi.instrumentBodyFixedVelocity(v);
    cout << "Spacecraft Velocity (B) = " << v[0] << " " << v[1] << " " << v[2] << endl;
    spi.sunPosition(p);
    cout << "Sun                 (B) = " << p[0] << " " << p[1] << " " << p[2] << endl;
    spi.MyOutput();
    double lat, lon;
    spi.subSpacecraftPoint(lat, lon);
    cout << "SubSpacecraft  = " << lat << " " << lon << endl;
    spi.subSolarPoint(lat, lon);
    cout << "SubSolar       = " << lat << " " << lon << endl;
  }
  cout << endl;

  // Testing with cache
  cout << "Testing with cache ... " << endl;
  double tol = .0022; //estimate resolution pixelPitch*alt/fl*1000.
  spi.createCache(startTime + slope, endTime - slope, 10, tol);
  for(int i = 0; i < 10; i++) {
    double t = startTime + (double) i * slope;
    spi.setTime(t);
    cout << "Time           = " << spi.time().Et() << endl;
    double p[3];
    spi.instrumentPosition(p);
    cout << "Spacecraft (B) = " << p[0] << " " << p[1] << " " << p[2] << endl;
    double v[3];
    spi.instrumentBodyFixedVelocity(v);
    cout << "Spacecraft Velocity (B) = " << v[0] << " " << v[1] << " " << v[2] << endl;
    spi.sunPosition(p);
    cout << "Sun        (B) = " << p[0] << " " << p[1] << " " << p[2] << endl;
    spi.MyOutput();
    cout << "Cache Start Time = " << spi.cacheStartTime().Et() << endl;
    cout << "Cache End Time   = " << spi.cacheEndTime().Et() << endl;
    cout << "Target Center Distance = " << spi.targetCenterDistance() << endl;
  }
  cout << endl;

  // Test attempt to get time after time has been set
  try {
    cout << "Testing time() after time has been set... " << endl;
    cout << "Has time been set? " << spi.isTimeSet() << endl; 
    cout << "The time is: " << spi.time().EtString().toStdString() << endl << endl;; 
  } 
  catch (IException &e) {
    e.print();
    cout << endl;
  }

  cout << "Testing Utility methods" << endl;
  cout << "Target Name = " << spi.target()->name().toStdString() << endl;
  cout << endl;
  PvlObject naifKeywords(spi.getStoredNaifKeywords());
  cout << "Get Stored Naif Keywords..." << endl;
  cout << "Object = " << naifKeywords.name() << endl;
  for (int i = 0; i < naifKeywords.keywords(); i++) {
    cout << "  " << naifKeywords[i] << endl;
  }
  cout << "EndObject" << endl;
}
