/** This is free and unencumbered software released into the public domain.
The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */
#include <iostream>
#include <iomanip>

#include <nlohmann/json.hpp>

#include "FileName.h"
#include "IException.h"
#include "IString.h"
#include "Preference.h"
#include "SpicePosition.h"
#include "Table.h"

using json = nlohmann::json;
using namespace Isis;
using namespace std;

int main(int argc, char *argv[]) {
  Preference::Preferences(true);

  cout << setprecision(8);
  cout << "Unit test for SpicePosition" << endl;

  // Test case is taken from moc red wide angle image ab102401
  // sn = MGS/561812335:32/MOC-WA/RED
  FileName f("$ISISTESTDATA/isis/src/base/unitTestData/kernels");
  QString dir = QString::fromStdString(f.expanded()) + "/";
  QString moc(dir + "moc.bsp");
  QString de(dir + "de405.bsp");
  QString pck(dir + "pck00006.tpc");
  furnsh_c(moc.toLatin1().data());
  furnsh_c(de.toLatin1().data());
  furnsh_c(pck.toLatin1().data());

  double startTime = -69382819.0;
  double endTime = -69382512.0;
  double slope = (endTime - startTime) / (10 - 1);

  SpicePosition pos(-94, 499);

  // Normal testing (no cache)
  cout << "Testing without cache ... " << endl;
  for(int i = 0; i < 10; i++) {
    double t = startTime + (double) i * slope;
    vector<double> p = pos.SetEphemerisTime(t);
    vector<double> v = pos.Velocity();
    cout << "Time           = " << pos.EphemerisTime() << endl;
    cout << "Spacecraft (J) = " << p[0] << " " << p[1] << " " << p[2] << endl;
    cout << "Velocity (J) = " << v[0] << " " << v[1] << " " << v[2] << endl;
  }
  cout << endl;


  // Testing with cache
  cout << "Testing with cache ... " << endl;
  pos.LoadCache(startTime, endTime, 10);
  for(int i = 0; i < 10; i++) {
    double t = startTime + (double) i * slope;
    vector<double> p = pos.SetEphemerisTime(t);
    vector<double> v = pos.Velocity();
    cout << "Time           = " << pos.EphemerisTime() << endl;
    cout << "Spacecraft (J) = " << p[0] << " " << p[1] << " " << p[2] << endl;
    cout << "Velocity (J) = " << v[0] << " " << v[1] << " " << v[2] << endl;
  }
  std::cout << "Cache Size: " << pos.cacheSize() << '\n';
  cout << endl;

  // Test table options
  cout << "Testing tables ... " << endl;
  Table tab = pos.Cache("Test");
  SpicePosition pos2(-94, 499);
  pos2.LoadCache(tab);
  for(int i = 0; i < 10; i++) {
    double t = startTime + (double) i * slope;
    pos2.SetEphemerisTime(t);
    vector<double> p = pos2.Coordinate();
    vector<double> v = pos2.Velocity();
    cout << "Time           = " << pos2.EphemerisTime() << endl;
    cout << "Spacecraft (J) = " << p[0] << " " << p[1] << " " << p[2] << endl;
    cout << "Velocity (J) = " << v[0] << " " << v[1] << " " << v[2] << endl;
  }
  cout << endl;

  // Test polynomial functions
  cout << "Testing with polynomial functions..." << endl;
  std::vector<double> abcPos1, abcPos2, abcPos3;
  //  pos.SetOverrideBaseTime(0.,1.);
  pos.ComputeBaseTime();
  pos.SetPolynomialDegree(7);
  pos.SetPolynomial();
  pos.GetPolynomial(abcPos1, abcPos2, abcPos3);
  //  cout << "Source = " << pos.GetSource() << endl;

  for (int i=0; i<10; i++) {
    double t = startTime + (double) i * slope;
    pos.SetEphemerisTime(t);
    vector<double> p = pos.Coordinate();
    vector<double> v = pos.Velocity();
    cout << "Time           = " << pos.EphemerisTime() << endl;
    cout << "Scaled Time    = " << pos.scaledTime() << endl;
    cout << "Spacecraft (J) = " << p[0] << " " << p[1] << " " << p[2] << endl;
    cout << "Velocity (J) = " << v[0] << " " << v[1] << " " << v[2] << endl;
  }
  cout << endl;

  // Now use LineCache method
  cout << "Testing line cache..." << endl;
  tab = pos.LineCache("Test2");
  SpicePosition pos3(-94, 499);
  pos3.LoadCache(tab);

  for (int i=0; i<10; i++) {
    double t = startTime + (double) i * slope;
    pos3.SetEphemerisTime(t);
    vector<double> p = pos3.Coordinate();
    vector<double> v = pos3.Velocity();
    cout << "Time           = " << pos3.EphemerisTime() << endl;
    cout << "Spacecraft (J) = " << p[0] << " " << p[1] << " " << p[2] << endl;
    cout << "Velocity (J) = " << v[0] << " " << v[1] << " " << v[2] << endl;
  }
  cout << endl;


  // Also test Extrapolate method
  cout << "Testing extrapolation..." << std::endl;
  pos3.SetEphemerisTime(endTime);
  cout << "Time           = " << pos3.EphemerisTime() << endl;
  vector<double> p = pos3.Coordinate();
  cout << "Spacecraft (J) = " << p[0] << " " << p[1] << " " << p[2] << endl;
  double timediff = .0001;
  cout << "Time           = " << pos3.EphemerisTime()+timediff << endl;
  p = pos3.Extrapolate(endTime+timediff);
  cout << "Spacecraft (J) = " << p[0] << " " << p[1] << " " << p[2] << endl;
  cout << endl;

  // Test Hermite input
  cout << "Testing Hermite function input ..." << endl;
  SpicePosition pos4(-94, 499);

  // Build a Hermite table
  TableRecord record;
  // add x,y,z position labels to record
  TableField x("J2000X", TableField::Double);
  TableField y("J2000Y", TableField::Double);
  TableField z("J2000Z", TableField::Double);
  record += x;
  record += y;
  record += z;
  // add x,y,z velocity labels to record
  TableField vx("J2000XV", TableField::Double);
  TableField vy("J2000YV", TableField::Double);
  TableField vz("J2000ZV", TableField::Double);
  record += vx;
  record += vy;
  record += vz;
  // add time label to record
  TableField t("ET", TableField::Double);
  record += t;

  // create input table with 6 nodes
  Table table("InitialHermite", record);
  record[0] = -1728.9713397204;
  record[1] = -1562.6465460511;
  record[2] = 2691.9864927899;
  record[3] = -4.1422722085589;
  record[4] = 1.1948376584683;
  record[5] = -1.9574609670906;
  record[6] = -69382819.360519;
  table += record;
  record[0] = -2041.9959598783;
  record[1] = -1466.5990522275;
  record[2] = 2534.2522152208;
  record[3] = -4.005762088109;
  record[4] = 1.3051257331886;
  record[5] = -2.1480934326045;
  record[6] = -69382742.560519;
  table += record;
  record[0] = -2343.7363144022;
  record[1] = -1362.4004944961;
  record[2] = 2362.4059212785;
  record[3] = -3.8489623869883;
  record[4] = 1.4067798977224;
  record[5] = -2.3244281718215;
  record[6] = -69382665.760519;
  table += record;
  record[0] = -2632.770640571;
  record[1] = -1250.7838483535;
  record[2] = 2177.6616219398;
  record[3] = -3.6755588456255;
  record[4] = 1.4980557337916;
  record[5] = -2.4835262350828;
  record[6] = -69382588.960519;
  table += record;
  record[0] = -2772.1635426316;
  record[1] = -1192.4549771491;
  record[2] = 2080.889410441;
  record[3] = -3.5840397554224;
  record[4] = 1.5394185289999;
  record[5] = -2.5558651797083;
  record[6] = -69382550.560519;
  table += record;
  record[0] = -2907.9942854884;
  record[1] = -1132.5942258221;
  record[2] = 1981.4352866816;
  record[3] = -3.4901273599211;
  record[4] = 1.5778355225805;
  record[5] = -2.6231980246908;
  record[6] = -69382512.160519;
  table += record;

  // Add table label
  table.Label() += PvlKeyword("CacheType", "HermiteSpline");
  table.Label() += PvlKeyword("SpkTableStartTime");
  table.Label()["SpkTableStartTime"].addValue(std::to_string(-69382819.360519));
  table.Label() += PvlKeyword("SpkTableEndTime");
  table.Label()["SpkTableEndTime"].addValue(std::to_string(-69382512.160519));
  table.Label() += PvlKeyword("SpkTableOriginalSize");
  table.Label()["SpkTableOriginalSize"].addValue(std::to_string(769));

  // Load table into the object
  pos4.LoadCache(table);
  cout << "Source = " << pos4.GetSource() << endl;

  for (int i=0; i<10; i++) {
    double t = startTime + (double) i * slope;
    pos4.SetEphemerisTime(t);
    vector<double> p = pos4.Coordinate();
    vector<double> v = pos4.Velocity();
    cout << "Time           = " << t << endl;
    cout << "Spacecraft (J) = " << p[0] << " " << p[1] << " " << p[2] << endl;
    cout << "Velocity (J) = " << v[0] << " " << v[1] << " " << v[2] << endl;
  }
  cout << endl;


  // Test polynomial over Hermite cubic spline
  cout << "Testing with polynomial function over Hermite..." << endl;
  pos4.ComputeBaseTime();
  abcPos1.clear();
  abcPos2.clear();
  abcPos3.clear();
  pos4.SetPolynomialDegree(2);
  abcPos1.push_back(-0.000166791);
  abcPos1.push_back(0.0);
  abcPos1.push_back(0.0);
  abcPos2.push_back(-0.0000796095);
  abcPos2.push_back(0.0);
  abcPos2.push_back(0.0);
  abcPos3.push_back(0.00014653);
  abcPos3.push_back(0.0);
  abcPos3.push_back(0.0);
  pos4.SetPolynomial(abcPos1, abcPos2, abcPos3,
                     SpicePosition::PolyFunctionOverHermiteConstant);

   cout << "Source = " << pos.GetSource() << endl;
   for(int i = 0; i < 10; i++) {
    double t = startTime + (double) i * slope;
    pos4.SetEphemerisTime(t);
    vector<double> p = pos4.Coordinate();
    vector<double> v = pos4.Velocity();
    cout << "Time           = " << t << endl;
    cout << "Spacecraft (J) = " << p[0] << " " << p[1] << " " << p[2] << endl;
    cout << "Velocity (J) = " << v[0] << " " << v[1] << " " << v[2] << endl;
  }
  cout << endl;

  // Test polynomial over Hermite conversion to Hermite
  cout << "Test fitting polynomial function over Hermite to new Hermite" << endl;

  // Fit new Hermite using existing Hermite and polynomial
  Table table2 = pos4.Cache("OutputHermite");
  SpicePosition pos5(-94, 499);

  // Load table2 into the object
  pos5.LoadCache(table2);

  cout << "Source = " << pos.GetSource() << endl;

  for(int i = 0; i < 10; i++) {
    double t = startTime + (double) i * slope;
    pos5.SetEphemerisTime(t);
    vector<double> p = pos5.Coordinate();
    vector<double> v = pos5.Velocity();
    cout << "Time           = " << t << endl;
    cout << "Spacecraft (J) = " << p[0] << " " << p[1] << " " << p[2] << endl;
    cout << "Velocity (J) = " << v[0] << " " << v[1] << " " << v[2] << endl;
  }
  cout << endl;

  // Test radar  nan case when et = baseTime and attempt to calculate velocity partial for first
  // coefficient
  cout << "Test calculation of first coefficient for spacecraft velocity" << endl;
  std::vector<double> dvelocity(3,0.);
  pos.SetEphemerisTime((startTime + endTime) / 2.);
  dvelocity = pos.VelocityPartial(SpicePosition::WRT_X, 0);
  cout << "  Velocity vector for center time = (" << dvelocity[0] <<  "," << dvelocity[1] <<"," << dvelocity[2] << ")"<< endl;

  cout <<endl;

  // Test loading cache from an ALE ISD without velocities
  cout << "Test loading cache from an ALE ISD with velocities" << endl;
  json aleIsd = {{"spk_table_start_time"    , -10.0},
                 {"spk_table_end_time"      , 10.0},
                 {"spk_table_original_size" , 3},
                 {"ephemeris_times"       , {-10.0, 0.0, 10.0}},
                 {"positions"            , {{-10.0, -10.0, -10.0},
                                            {0.0,  0.0,  0.0},
                                            {10.0,  10.0,  10.0}}}};
  SpicePosition alePos(-94, 499);
  alePos.LoadCache(aleIsd);
  cout << "Source = " << alePos.GetSource() << endl;
  cout << "Has velocity? " << (alePos.HasVelocity() ? "Yes" : "No") << endl;
  for(int t = -10; t <= 10; t++) {
   alePos.SetEphemerisTime(t);
   vector<double> p = alePos.Coordinate();
   cout << "Time           = " << t << endl;
   cout << "Spacecraft (J) = " << p[0] << " " << p[1] << " " << p[2] << endl;
 }

  cout <<endl;

  // Test loading cache from an ALE ISD with velocities
  cout << "Test loading cache from an ALE ISD with velocities" << endl;
  json aleVelIsd = aleIsd;
  aleVelIsd["velocities"] = {{1.0, 1.0, 1.0},
                             {1.0, 1.0, 1.0},
                             {1.0, 1.0, 1.0}};
  SpicePosition aleVelPos(-94, 499);
  aleVelPos.LoadCache(aleVelIsd);
  cout << "Source = " << aleVelPos.GetSource() << endl;
  cout << "Has velocity? " << (aleVelPos.HasVelocity() ? "Yes" : "No") << endl;
  for(int t = -10; t <= 10; t++) {
   aleVelPos.SetEphemerisTime(t);
   vector<double> p = aleVelPos.Coordinate();
   vector<double> v = aleVelPos.Velocity();
   cout << "Time           = " << t << endl;
   cout << "Spacecraft (J) = " << p[0] << " " << p[1] << " " << p[2] << endl;
   cout << "Velocity (J) = " << v[0] << " " << v[1] << " " << v[2] << endl;
 }

  cout <<endl;

  // Test loading cache from an ALE ISD with non-SPICE SpicePosition
  cout << "Test loading cache from an ALE ISD with non-SPICE SpicePosition" << endl;
  try {
    pos5.LoadCache(aleIsd);
  }
  catch (IException &e) {
    e.print();
  }

  cout <<endl;

  // Test loading cache from 0th degree polynomial
  cout << "Test loading cache from 0th degree polynomial" << endl;
  json zeroDegreeIsd = {{"spk_table_start_time"    , 1.0},
                        {"spk_table_end_time"      , 1.0},
                        {"spk_table_original_size" , 1},
                        {"ephemeris_times"       , {1.0}},
                        {"positions"            , {{1.0, 2.0, 3.0}}}};
  SpicePosition zeroDegreePoly(-94, 499);
  zeroDegreePoly.LoadCache(zeroDegreeIsd);
  zeroDegreePoly.ComputeBaseTime();
  zeroDegreePoly.SetPolynomialDegree(0);
  zeroDegreePoly.SetPolynomial();
  Table zeroDegreeTable = zeroDegreePoly.Cache("TestZeroDegree");
  SpicePosition singlePosition(-94, 499);
  singlePosition.LoadCache(zeroDegreeTable);
  cout << "Source = " << singlePosition.GetSource() << endl;
  cout << "Has velocity? " << (singlePosition.HasVelocity() ? "Yes" : "No") << endl;
  singlePosition.SetEphemerisTime(1.0);
  vector<double> singlePos = singlePosition.Coordinate();
  cout << "Time           = " << 1.0 << endl;
  cout << "Spacecraft (J) = " << singlePos[0] << " " << singlePos[1] << " " << singlePos[2] << endl;

}
