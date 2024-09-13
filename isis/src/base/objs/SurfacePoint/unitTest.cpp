/** This is free and unencumbered software released into the public domain.
The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */
#include <iostream>
#include <iomanip>
#include <vector>

#include "Constants.h"
#include "Camera.h"
#include "Displacement.h"
#include "Distance.h"
#include "IException.h"
#include "Latitude.h"
#include "Longitude.h"
#include "Preference.h"
#include "SurfacePoint.h"

#include <boost/numeric/ublas/symmetric.hpp>

using namespace Isis;
using namespace std;
using namespace boost::numeric::ublas;

/**
 *
 * @author 2010-07-30 Tracie Sucharski, Ken L. Edmundson, and Debbie A. Cook
 *
 * @internal
 *   @history 2015-02-20 Jeannie Backer - Added print statement for
 *            latitude, longitude and radius weight accessor methods.
 *   @history 2018-06-28 Debbie A.Cook - Removed test of obsolete method
 *            SetRadii
 *   @history 2018-09-06 (added to BundleXYZ branch on 2017-05-27)  
 *            Debbie A. Cook - Added tests for new methods:
 *            GetCoord, GetSigma, GetWeight, GetXSigma, GetYSigma, 
 *            GetZsigma, LatToDouble, LonToDouble, DistanceToDouble,
 *            DisplacementToDouble, and enum types CoordinateType, 
 *            CoordUnits, and CoordIndex.  Fixed incorrect units in report for
 *            latSig and lonSig.
 *   @history 2018-09-06 (added to BundleXYZ branch on 2017-11-20) 
 *            Debbie A. Cook - Added tests for new options in
 *            SetRectangularMatrix and SetSphericalMatrix.
 *   @history 2018-09-21 Debbie A. Cook - Added tests for new 
 *            SurfacePoint modifications.
 *   
 */
int main(int argc, char *argv[]) {
  Isis::Preference::Preferences(true);
  symmetric_matrix<double,upper> cvRect,cvOc;

  try {
    cout << "UnitTest for SurfacePoint" << endl << endl;
    cout << "1-Test rectangular set of point and variance only (variance in m^2) ..." << endl;
    cout << " with x=-424.024048 m, y=734.4311949 m, z=529.919264 m,"
          "sigmaX=10. m, sigmaY=50. m, sigmaZ=20. m" << endl << endl;
    Isis::SurfacePoint spRec;

    symmetric_matrix<double,upper> covar; // Units are m**2
    covar.resize(3);
    covar.clear();
    // Units are m**2
    covar(0,0) = 100.;
    covar(1,1) = 2500.;
    covar(2,2) = 400.;

    // Default is to set covar in meters**2 for length units
    spRec.SetRectangular(Displacement(-424.024048, Displacement::Meters),
                         Displacement(734.4311949, Displacement::Meters),
                         Displacement(529.919264, Displacement::Meters), covar);

    double lat = spRec.GetLatitude().degrees();
    double lon = spRec.GetLongitude().degrees();
    double radius = spRec.GetLocalRadius().meters();
    double latSig = spRec.GetLatSigma().degrees();
    double lonSig = spRec.GetLonSigma().degrees();
    double radSig = spRec.GetLocalRadiusSigma().meters();
;
    symmetric_matrix<double,upper> covarSphere(3);
    covarSphere.clear();
    // Default is to get matrix with length units in meters**2
    covarSphere = spRec.GetSphericalMatrix(SurfacePoint::Kilometers);
    
    cout << setprecision(9);
    cout << "  Output spherical..." << endl;
    cout << "    lat = " << lat << " degrees, lon = " << lon <<
            " degrees, radius = " << radius << " meters" << endl;
    cout << "    lat = " << spRec.GetLatitude().radians() <<
            " radians, lon = " << spRec.GetLongitude().radians() 
         << " radians, radius = " << spRec.GetLocalRadius().meters() <<
            " meters" << endl;
    cout << "    latitude sigma=" << latSig << " degrees, longitude sigma=" <<
            lonSig << " degrees, radius sigma=" << radSig << " m" << endl;
    cout << "    latitude sigma=" << spRec.GetLatSigma().radians() <<
            " radians, longitude sigma=" << spRec.GetLonSigma().radians()
         << " radians, radius sigma=" << spRec.GetLocalRadiusSigma().meters()
         << " m" << endl;
    cout << "    latitude sigma=" << spRec.GetLatSigmaDistance().meters() <<
         " m, longitude sigma=" << spRec.GetLonSigmaDistance().meters() <<
         " m, radius sigma=" << spRec.GetLocalRadiusSigma().meters()
         <<  " m" << endl;
    cout << "    latitude weight =" << spRec.GetLatWeight() <<
         ", longitude weight =" << spRec.GetLonWeight() <<
         ", radius weight =" << spRec.GetLocalRadiusWeight() << endl;

    cout << "    spherical covariance matrix = " << covarSphere(0,0) << "  " <<
         covarSphere(0,1) << "  " << covarSphere(0,2) << endl;
    cout << "                                  " << covarSphere(1,0) << "  " <<
         covarSphere(1,1) << "  " << covarSphere(1,2) << endl;
    cout << "                                  " << covarSphere(2,0) << "  " <<
         covarSphere(2,1) << "  " << covarSphere(2,2) << endl;
    cout << "  Input rectangular sigmas = " << spRec.GetXSigma().meters()
         << "/" << spRec.GetYSigma().meters() << "/"
         << spRec.GetZSigma().meters() << std::endl;



    cout << endl;
    cout << "2-Testing spherical set of point and variance/covariance matrix (in meters^2)..."
            << endl;
      // Usage note:  In order to get accurate results, the full correlation matrix should be
      // used as opposed to only setting the diagonal elements with the sigmas. 
    cout << " with lat=" << lat << " degrees, lon=" << lon << " degrees, radius="
         << radius << " m" << endl;
    cout << " latitude sigma=" << latSig << " deg, longitude sigma=" << lonSig
         << " deg, radiusSig=" << radSig << " m" << endl;
    Isis::SurfacePoint spSphere;
    // Convert covarSphere to meters to be able to test default set
    covarSphere(0,2) *= 1000.;
    covarSphere(1,2) *= 1000.;
    covarSphere(2,2) *= 1.0e6;
    spSphere.SetSpherical(Latitude(lat, Angle::Degrees),
                          Longitude(lon, Angle::Degrees),
                          Distance(radius, Distance::Meters),
                          covarSphere);
    symmetric_matrix<double,upper> covarRec(3);
    covarRec.clear();
    covarRec = spSphere.GetRectangularMatrix(SurfacePoint::Kilometers);

    if(fabs(covarRec(0,1)) < 1E-12) covarRec(0,1) = 0.0;
    if(fabs(covarRec(0,2)) < 1E-12) covarRec(0,2) = 0.0;
    if(fabs(covarRec(1,0)) < 1E-12) covarRec(1,0) = 0.0;
    if(fabs(covarRec(1,2)) < 1E-12) covarRec(1,2) = 0.0;
    if(fabs(covarRec(2,0)) < 1E-12) covarRec(2,0) = 0.0;
    if(fabs(covarRec(2,2)) < 1E-12) covarRec(2,2) = 0.0;

    cout << "  Output rectangular..." << endl;
    cout << "    x=" << spSphere.GetX().meters()
         << " m, y=" << spSphere.GetY().meters()
         << " m, z=" << spSphere.GetZ().meters() << " m" << endl;
    cout << "    X sigma=" << spSphere.GetXSigma().meters() << " m, Y sigma="
         << spSphere.GetYSigma().meters() << " m, Z sigma=" <<
            spSphere.GetZSigma().meters() << " m" << endl;
    cout << "    rectangular covariance matrix = " 
         << setw(10) << covarRec(0,0) << setw(10) << covarRec(0,1)
         << setw(10) << covarRec(0,2) << endl;
    cout << "                                    "
         << setw(10) << covarRec(1,0) << setw(10) << covarRec(1,1)
         << setw(10) << covarRec(1,2) << endl;
    cout << "                                    "
         << setw(10) << covarRec(2,0) << setw(10) << covarRec(2,1)
         << setw(10) << covarRec(2,2) << endl;
  }
  catch(Isis::IException &e) {
    e.print();
  }

    try {
    cout << "3-Testing rectangular set with point and sigmas only..." << endl;
    Isis::SurfacePoint spRec;
    spRec.SetRectangular(Displacement(-424.024048, Displacement::Meters),
                         Displacement(734.4311949, Displacement::Meters),
                         Displacement(529.919264, Displacement::Meters),
                         Distance(10., Distance::Meters),
                         Distance(50., Distance::Meters),
                         Distance(20., Distance::Meters));
    double lat = (spRec.GetLatitude()).degrees();
    double lon = (spRec.GetLongitude()).degrees();
    double radius = spRec.GetLocalRadius().meters();
    double latSig = (spRec.GetLatSigma()).degrees();
    double lonSig = (spRec.GetLonSigma()).degrees();
    double radSig = spRec.GetLocalRadiusSigma().meters();
    symmetric_matrix<double,upper> covarSphere(3);
    covarSphere.clear();
    covarSphere = spRec.GetSphericalMatrix(SurfacePoint::Kilometers);
    
    cout << setprecision(9);
    cout << "  Output spherical..." << endl;
    cout << "    lat=" << lat << " degrees, lon=" << lon << " degrees, radius="
         << radius << "m" << endl;
    cout << "    latitude sigma=" << latSig << " degrees, longitude sigma=" 
         << lonSig
         << " degrees, radius sigma=" << radSig << "m" << endl;
    cout << "    ocentric covariance matrix = " << covarSphere(0,0) << "  " 
         << covarSphere(0,1) << "  " << covarSphere(0,2) << endl;
    cout << "                                 " << covarSphere(1,0) << "  "
         << covarSphere(1,1) << "  " << covarSphere(1,2) << endl;
    cout << "                                 " << covarSphere(2,0) << "  "
         << covarSphere(2,1) << "  " << covarSphere(2,2) << endl;

    // The next test will not match previous results because only the sigmas are set
    // and not the whole variance/covariance matrix
    cout << endl;
    cout << "4-Testing planetocentric set with point and sigmas  ..."
         << endl;
    Isis::SurfacePoint spSphere1,spSphere2,spSphere4;
    spSphere1.SetSpherical(Latitude(32., Angle::Degrees),
                         Longitude(120., Angle::Degrees),
                         Distance(1000., Distance::Meters),
                         Angle(1.64192315,Angle::Degrees),
                         Angle(1.78752107, Angle::Degrees),
                         Distance( 38.454887335682053718134171237789,
                                  Distance::Meters));
    symmetric_matrix<double,upper> covarRec(3);
    covarRec.clear();
    covarRec = spSphere1.GetRectangularMatrix(SurfacePoint::Kilometers);
    spSphere2.SetSpherical(Latitude(0.55850536, Angle::Radians),
                              Longitude(2.0943951, Angle::Radians),
                              Distance(1000., Distance::Meters),
                              Angle(0.028656965, Angle::Radians),
                              Angle(0.0311981281, Angle::Radians),
                              Distance(38.4548873, Distance::Meters));
    spSphere4.SetSpherical(Latitude(0.55850536, Angle::Radians),
                              Longitude(2.0943951, Angle::Radians),
                              Distance(1000., Distance::Meters));
    spSphere4.SetSphericalSigmasDistance(
                              Distance(28.6569649, Distance::Meters),
                              Distance(26.4575131, Distance::Meters),
                              Distance(38.4548873, Distance::Meters));

    // TODO try to convert ocentric sigmas to meters and get error to test error

    cout << "  4a-Output rectangular from degrees..." << endl;
    cout << "    x=" << spSphere1.GetX().meters() << " m, y=" <<
            spSphere1.GetY().meters()
         << " m, z=" << spSphere1.GetZ().meters() << " m" << endl;
    cout << "    X sigma =" << spSphere1.GetXSigma().meters()
         << " m, Y sigma = " << spSphere1.GetYSigma().meters()
         << " m, Z sigma = " << spSphere1.GetZSigma().meters() << "m"
         << endl;
    cout << "    rectangular covariance matrix = " << covarRec(0,0) << "  "
         << covarRec(0,1) << "  " << covarRec(0,2) << endl;
    cout << "                                    " << covarRec(1,0) << "  "
         << covarRec(1,1) << "  " << covarRec(1,2) << endl;
    cout << "                                    " << covarRec(2,0) << "  "
         << covarRec(2,1) << "  " << covarRec(2,2) << endl;

    covarRec.clear();
    covarRec = spSphere2.GetRectangularMatrix(SurfacePoint::Kilometers);
    cout << "  4b-Output rectangular from radians..." << endl;
    cout << "    x=" << spSphere2.GetX().meters()
         << " m, y=" << spSphere2.GetY().meters()
         << " m, z=" << spSphere2.GetZ().meters() << " m" << endl;
    cout << "    X sigma = " << spSphere2.GetXSigma().meters()
         << " m, Y sigma = " << spSphere2.GetYSigma().meters()
         << " m, Z sigma = " << spSphere2.GetZSigma().meters()
         << "m" << endl;
    cout << "    rectangular covariance matrix = " << covarRec(0,0) << "  "
         << covarRec(0,1) << "  " << covarRec(0,2) << endl;
    cout << "                                    " << covarRec(1,0) << "  "
         << covarRec(1,1) << "  " << covarRec(1,2) << endl;
    cout << "                                    " << covarRec(2,0) << "  "
         << covarRec(2,1) << "  " << covarRec(2,2) << endl;
    cout << "  4c-Output spherical sigmas from meters..." << endl;
    cout << "    latitude sigma=" << spSphere4.GetLatSigma().radians() <<
            " radians, longitude sigma=" << spSphere4.GetLonSigma().radians()
         << " radians, radius sigma=" << spSphere4.GetLocalRadiusSigma().meters()
         << " m" << endl;
    cout << endl << "5-Testing copy constructor" << endl;

    Isis::SurfacePoint spRec2(spSphere1);
    lat = (spRec2.GetLatitude()).degrees();
    lon = (spRec2.GetLongitude()).degrees();
    radius = spRec2.GetLocalRadius().meters();
    latSig = (spRec2.GetLatSigma()).degrees();
    lonSig = (spRec2.GetLonSigma()).degrees();
    radSig = spRec2.GetLocalRadiusSigma().meters();
    
    cout << setprecision(9);
    cout << "  Output spherical..." << endl;
    cout << "    lat=" << lat << " degrees, lon=" << lon << " degrees, radius="
         << radius << " m" << endl;
    cout << "    latitude sigma = " << latSig << " degrees, longitude sigma = "
         << lonSig << " degrees, radius sigma = " << radSig << " m" << endl;
    cout << "    ocentric covariance matrix = " << covarSphere(0,0) << "  "
         << covarSphere(0,1) << "  " << covarSphere(0,2) << endl;
    cout << "                                 " << covarSphere(1,0) << "  "
         << covarSphere(1,1) << "  " << covarSphere(1,2) << endl;
    cout << "                                 " << covarSphere(2,0) << "  "
         << covarSphere(2,1) << "  " << covarSphere(2,2) << endl << endl;

    cout << "Testing Longitude Accessor..." << endl;
    Isis::SurfacePoint spSphere3(Latitude(15, Angle::Degrees),
        Longitude(-45, Angle::Degrees), Distance(10, Distance::Kilometers));
    cout << "Longitude (from -45): " << spSphere3.GetLongitude().degrees()
         << endl << endl;

    cout << "6-Testing set of matrices in spherical and rectangular coordinates in km ..."
         << endl;
    cout << "  6a-Test rectangular set of point and variance only (variance in km^2) ..." << endl;
    cout << "    with x=-424.024048 m, y=734.4311949 m, z=529.919264 m,"
          "sigmaX=.01 km, sigmaY=.05 km, sigmaZ=.02 km" << endl << endl;
    Isis::SurfacePoint spRecKm;

    // spRecKm.SetRadii(Distance(1000., Distance::Meters),
    //                Distance(1000., Distance::Meters),
    //                Distance(1000., Distance::Meters));

    symmetric_matrix<double,upper> covar;
    covar.resize(3);
    covar.clear();
    // Units are km**2
    covar(0,0) = .0001;
    covar(1,1) = .0025;
    covar(2,2) = .0004;

    spRecKm.SetRectangular(Displacement(-424.024048, Displacement::Meters),
                           Displacement(734.4311949, Displacement::Meters),
                         Displacement(529.919264, Displacement::Meters));
    spRecKm.SetMatrix(SurfacePoint::Rectangular, covar);

    lat = spRecKm.GetLatitude().degrees();
    lon = spRecKm.GetLongitude().degrees();
    radius = spRecKm.GetLocalRadius().meters();
    latSig = spRecKm.GetLatSigma().degrees();
    lonSig = spRecKm.GetLonSigma().degrees();
    radSig = spRecKm.GetLocalRadiusSigma().meters();
    symmetric_matrix<double,upper> covarSphKm(3);
    covarSphKm.clear();
    covarSphKm = spRecKm.GetSphericalMatrix(SurfacePoint::Kilometers);
    
    cout << setprecision(9);
    cout << "    Output spherical..." << endl;
    cout << "      lat = " << lat << " degrees, lon = " << lon <<
            " degrees, radius = " << radius << " meters" << endl;
    cout << "      lat = " << spRecKm.GetLatitude().radians() <<
            " radians, lon = " << spRecKm.GetLongitude().radians() 
         << " radians, radius = " << spRecKm.GetLocalRadius().meters() <<
            " meters" << endl;
    cout << "      latitude sigma=" << spRecKm.GetLatSigma().radians() <<
            " radians, longitude sigma=" << spRecKm.GetLonSigma().radians()
         << " radians, radius sigma=" << spRecKm.GetLocalRadiusSigma().meters()
         << " m" << endl;

    cout << "      spherical covariance matrix = " << covarSphKm(0,0) << "  " <<
         covarSphKm(0,1) << "  " << covarSphKm(0,2) << endl;
    cout << "                                    " << covarSphKm(1,0) << "  " <<
         covarSphKm(1,1) << "  " << covarSphKm(1,2) << endl;
    cout << "                                    " << covarSphKm(2,0) << "  " <<
         covarSphKm(2,1) << "  " << covarSphKm(2,2) << endl;
    cout << "    Input rectangular sigmas = " << spRecKm.GetXSigma().kilometers()
         << "/" << spRecKm.GetYSigma().kilometers() << "/"
         << spRecKm.GetZSigma().kilometers() << std::endl;
    
    cout << endl;
    cout << "  6b-Testing spherical set of point and variance/covariance matrix (in km^2)..."
            << endl;
      // Usage note:  In order to get accurate results, the full correlation matrix should be
      // used as opposed to only setting the diagonal elements with the sigmas. 
    cout << "    with lat=" << lat << " degrees, lon=" << lon << " degrees, radius="
         << radius << " m" << endl;
    cout << "    latitude sigma=" << latSig << " deg, longitude sigma=" << lonSig
         << " deg, radiusSig=" << radSig << " m" << endl;
    Isis::SurfacePoint spSphereKm;
    spSphereKm.SetSpherical(Latitude(lat, Angle::Degrees),
                          Longitude(lon, Angle::Degrees),
                            Distance(radius, Distance::Meters));
    spSphereKm.SetMatrix(SurfacePoint::Latitudinal, covarSphKm);
    symmetric_matrix<double,upper> covarRecKm(3);
    covarRecKm.clear();
    covarRecKm = spSphereKm.GetRectangularMatrix(SurfacePoint::Kilometers);

    if(fabs(covarRecKm(0,1)) < 1E-12) covarRecKm(0,1) = 0.0;
    if(fabs(covarRecKm(0,2)) < 1E-12) covarRecKm(0,2) = 0.0;
    if(fabs(covarRecKm(1,0)) < 1E-12) covarRecKm(1,0) = 0.0;
    if(fabs(covarRecKm(1,2)) < 1E-12) covarRecKm(1,2) = 0.0;
    if(fabs(covarRecKm(2,0)) < 1E-12) covarRecKm(2,0) = 0.0;
    if(fabs(covarRecKm(2,2)) < 1E-12) covarRecKm(2,2) = 0.0;

    cout << "    Output rectangular..." << endl;
    cout << "      x=" << spSphereKm.GetX().meters()
         << " m, y=" << spSphereKm.GetY().meters()
         << " m, z=" << spSphereKm.GetZ().meters() << " m" << endl;
    cout << "      X sigma=" << spSphereKm.GetXSigma().meters() << " m, Y sigma="
         << spSphereKm.GetYSigma().meters() << " m, Z sigma=" <<
            spSphereKm.GetZSigma().meters() << " m" << endl;
    cout << "      rectangular covariance matrix = " 
         << setw(10) << covarRecKm(0,0) << setw(10) << covarRecKm(0,1)
         << setw(10) << covarRecKm(0,2) << endl;
    cout << "                                    "
         << setw(10) << covarRecKm(1,0) << setw(10) << covarRecKm(1,1)
         << setw(10) << covarRecKm(1,2) << endl;
    cout << "                                    "
         << setw(10) << covarRecKm(2,0) << setw(10) << covarRecKm(2,1)
         << setw(10) << covarRecKm(2,2) << endl << endl;

  }
  catch(Isis::IException &e) {
    e.print();
  }

    // Test new code added to support solving and outputting control points in body-fixed
    // x/y/z (RECTANGULAR)
    
    // Set a point for testing - Use coordinate data from test 1
    Isis::SurfacePoint spRec1;
    spRec1.SetRectangular(Displacement(-424.024048, Displacement::Meters),
                          Displacement(734.4311949, Displacement::Meters),
                          Displacement(529.919264, Displacement::Meters));
    
    cout << "Testing error conditions in GetCoord, GetSigma, GetWeight, LatToDouble, "
         << endl << "LonToDouble,  GetSigmaDistance, DistanceToDouble, and DisplacementToDouble..."
         << endl << endl;

    try {
    // Test DisplacementToDouble error conditions. (DistanceToDouble, LatToDouble, and LonToDouble)
      cout << "  ...Test Rectangular GetCoord with incorrect unit Degrees" << endl;
    double value = spRec1.GetCoord(SurfacePoint::Rectangular, SurfacePoint::One,
                                   SurfacePoint::Degrees);
    cout << "X-degrees = " << value << endl;
   }
   catch(Isis::IException &e) {
    e.print();
  }

    try {
    // Test DisplacementToDouble error conditions. (DistanceToDouble, LatToDouble, and LonToDouble)
      cout << "  ...Test Rectangular GetCoord with incorrect unit Radians" << endl;
    double value = spRec1.GetCoord(SurfacePoint::Rectangular, SurfacePoint::One,
                                   SurfacePoint::Radians);
    cout << "X-radians = " << value << endl;
   }
   catch(Isis::IException &e) {
    e.print();
  }

    try {
    // Test DisplacementToDouble error conditions. (DistanceToDouble, LatToDouble, and LonToDouble)
      cout << "  ...Test Rectangular GetCoord with incorrect unit Degrees" << endl;
    double value = spRec1.GetCoord(SurfacePoint::Latitudinal, SurfacePoint::Three,
                                   SurfacePoint::Degrees);
    cout << "Local Radius-degrees = " << value << endl;
   }
   catch(Isis::IException &e) {
    e.print();
  }

    try {
    // Test DisplacementToDouble error conditions. (DistanceToDouble, LatToDouble, and LonToDouble)
      cout << "  ...Test Latitudinal GetCoord with incorrect unit Radians for local radius" << endl;
    double value = spRec1.GetCoord(SurfacePoint::Latitudinal, SurfacePoint::Three,
                                   SurfacePoint::Radians);
    cout << "Local Radius-radians = " << value << endl;
   }
   catch(Isis::IException &e) {
    e.print();
  }

    try {
    // Test DisplacementToDouble error conditions. (DistanceToDouble, LatToDouble, and LonToDouble)
      cout << "  ...Test Latitudinal GetCoord with incorrect unit Kilometers for latitude" << endl;
    double value = spRec1.GetCoord(SurfacePoint::Latitudinal, SurfacePoint::One,
                                   SurfacePoint::Kilometers);
    cout << "Latitude-kilometers = " << value << endl;
   }
   catch(Isis::IException &e) {
    e.print();
  }

    try {
    // Test DisplacementToDouble error conditions. (DistanceToDouble, LatToDouble, and LonToDouble)
      cout << "  ...Test Latitudinal GetCoord with incorrect unit Meters for longitude" << endl;
    double value = spRec1.GetCoord(SurfacePoint::Latitudinal, SurfacePoint::Two,
                                   SurfacePoint::Kilometers);
    cout << "Longitude-meters = " << value << endl;
   }
   catch(Isis::IException &e) {
    e.print();
  }

    try {
    // Test point coordinates only set.  Try GetWeight.  Set sigmas. Try GetWeight.
    // Try GetWeight and GetCoord for all possibilities
      cout << endl;
      cout << "Test error statements: case where weights are requested after only coordinates "
              << "have  been set..." << endl;
    double value = spRec1.GetWeight(SurfacePoint::Rectangular, SurfacePoint::One);
    cout << "X sigma-degrees = " << value << endl;
   }
   catch(Isis::IException &e) {
    e.print();
  }

    try {
    // Test SetSphericalSigmasDistance error condition
      cout << endl;
      cout << "Test error statements: case of invalid SurfacePoint in"
              << " SetSphericalSigmasDistance " << endl;
      SurfacePoint sp;
      sp.SetSphericalSigmasDistance(Distance(3.,Distance::Meters),
           Distance(3.,Distance::Meters),Distance(3.,Distance::Meters));
   }
   catch(Isis::IException &e) {
    e.print();
  }

    try {
    // Test GetWeight after point coordinates and sigmas have been set. Try GetWeight. Set body 
    // radii.  Try GetWeight, GetSigma and GetCoord for all possibilities.
      cout << endl;
      cout << "...Testing GetCoord, GetSigma, and GetWeight...  " << endl << endl;
      spRec1.SetRectangularSigmas(Distance(10.,Distance::Meters),
                                Distance(50., Distance::Meters),
                                Distance(20., Distance::Meters));

      cout << "Rectangular Coordinates kilometers:  "  << "X= ";
      cout << spRec1.GetCoord(SurfacePoint::Rectangular, SurfacePoint::One,
                              SurfacePoint::Kilometers);
      cout << "   Y = ";
      cout << spRec1.GetCoord(SurfacePoint::Rectangular, SurfacePoint::Two,
                              SurfacePoint::Kilometers);
      cout << "   Z = ";
      cout << spRec1.GetCoord(SurfacePoint::Rectangular, SurfacePoint::Three,
                              SurfacePoint::Kilometers);
      cout << endl;

      cout << "Rectangular Coordinates meters:  "  << "X= ";
      cout << spRec1.GetCoord(SurfacePoint::Rectangular, SurfacePoint::One, SurfacePoint::Meters);
      cout << "   Y = ";
      cout << spRec1.GetCoord(SurfacePoint::Rectangular, SurfacePoint::Two, SurfacePoint::Meters);
      cout << "   Z = ";
      cout << spRec1.GetCoord(SurfacePoint::Rectangular, SurfacePoint::Three, SurfacePoint::Meters);
      cout << endl;
      
      cout << "Rectangular sigmas kilometers:  "  << "X= ";
      cout << spRec1.GetSigma(SurfacePoint::Rectangular, SurfacePoint::One,
                              SurfacePoint::Kilometers);
      cout << "   Y = ";
      cout << spRec1.GetSigma(SurfacePoint::Rectangular, SurfacePoint::Two,
                              SurfacePoint::Kilometers);
      cout << "   Z = ";
      cout << spRec1.GetSigma(SurfacePoint::Rectangular, SurfacePoint::Three,
                              SurfacePoint::Kilometers);
      cout << endl;
      
      cout << "Using GetSigmaDistance, Rectangular sigmas kilometers:  "  << "X= ";
      cout << spRec1.GetSigmaDistance(SurfacePoint::Rectangular, SurfacePoint::One)
        .kilometers();
      cout << "   Y = ";
      cout << spRec1.GetSigmaDistance(SurfacePoint::Rectangular, SurfacePoint::Two)
        .kilometers();
      cout << "   Z = ";
      cout << spRec1.GetSigmaDistance(SurfacePoint::Rectangular, SurfacePoint::Three)
        .kilometers();
      cout << endl;

    cout << "Rectangular sigmas meters:  "  << "X= ";
    cout << spRec1.GetSigma(SurfacePoint::Rectangular, SurfacePoint::One, SurfacePoint::Meters);
    cout << "   Y = ";
    cout << spRec1.GetSigma(SurfacePoint::Rectangular, SurfacePoint::Two, SurfacePoint::Meters);
    cout << "   Z = ";
    cout << spRec1.GetSigma(SurfacePoint::Rectangular, SurfacePoint::Three, SurfacePoint::Meters);
    cout << endl;

    cout << "Using GetSigmaDistance, Rectangular sigmas meters:  "  << "X= ";
    cout << spRec1.GetSigmaDistance(SurfacePoint::Rectangular, SurfacePoint::One).meters();
    cout << "   Y = ";
    cout << spRec1.GetSigmaDistance(SurfacePoint::Rectangular, SurfacePoint::Two).meters();
    cout << "   Z = ";
    cout << spRec1.GetSigmaDistance(SurfacePoint::Rectangular, SurfacePoint::Three).meters();
    cout << endl;
    
    cout << "Rectangular Weights kilometers:  "  << "X = "; 
    cout << spRec1.GetWeight(SurfacePoint::Rectangular, SurfacePoint::One) <<  "  Y = " 
             << spRec1.GetWeight(SurfacePoint::Rectangular, SurfacePoint::Two) << "  Z = " 
             << spRec1.GetWeight(SurfacePoint::Rectangular, SurfacePoint::Three) << endl;

    cout << "Latitudinal Coordinates radians:  "  << "Latitude= ";
    cout << spRec1.GetCoord(SurfacePoint::Latitudinal, SurfacePoint::One, SurfacePoint::Radians);
    cout << "   Longitude = ";
    cout << spRec1.GetCoord(SurfacePoint::Latitudinal, SurfacePoint::Two, SurfacePoint::Radians);
    cout << "   Local Radius = ";
    cout << spRec1.GetCoord(SurfacePoint::Latitudinal, SurfacePoint::Three,
                            SurfacePoint::Kilometers);
    cout << endl;

    cout << "Latitudinal Coordinates degrees:  "  << "Latitude= ";
    cout << spRec1.GetCoord(SurfacePoint::Latitudinal, SurfacePoint::One, SurfacePoint::Degrees);
    cout << "   Longitude = ";
    cout << spRec1.GetCoord(SurfacePoint::Latitudinal, SurfacePoint::Two, SurfacePoint::Degrees);
    cout << "   Local Radius = ";
    cout << spRec1.GetCoord(SurfacePoint::Latitudinal, SurfacePoint::Three, SurfacePoint::Meters);
    cout << endl;
    
    cout << "Latitudinal weight degrees:  "  << "Latitude= ";
    cout << spRec1.GetWeight(SurfacePoint::Latitudinal, SurfacePoint::One);
    cout << "   Longitude = ";
    cout << spRec1.GetWeight(SurfacePoint::Latitudinal, SurfacePoint::Two);
    cout << "   Local Radius = ";
    cout << spRec1.GetWeight(SurfacePoint::Latitudinal, SurfacePoint::Three);
    cout << endl << endl;
   }
   catch(Isis::IException &e) {
    e.print();
  }

    // Test stringToCoordType and coordinateTypeToString
    QString ctypeStr = "Latitudinal";
    SurfacePoint::CoordinateType ctype = spRec1.stringToCoordinateType(ctypeStr);
    cout << "Testing stringToCoordinateType with Latitudinal:  coordType = " << ctype << endl;
    cout << "Testing coordinateTypeToString with coordType = 0:  coordTypeStr = " <<
      spRec1.coordinateTypeToString(ctype).toStdString()  << endl;
    ctypeStr = "RectanGular";
    ctype = spRec1.stringToCoordinateType(ctypeStr);
    cout << "Testing stringToCoordinateType with Rectangular:  coordType = " << ctype << endl; 
    cout << "Testing coordinateTypeToString with coordType = 1:  coordTypeStr = " << 
         spRec1.coordinateTypeToString(ctype).toStdString() << endl;
    try {
    // Test invalid coordinate type error condition
      cout << endl;
      cout << "Test invalid coordinate type error condition." << endl;
      ctypeStr = "Garbage";
      ctype = spRec1.stringToCoordinateType(ctypeStr);
      cout << "Testing stringToCoordinateType with Garbage:  coordType = " << ctype << endl;
   }
   catch(Isis::IException &e) {
     e.print();
  }

  cout << endl;
  cout << "Test computational methods..." << endl;
  cout << "  SphericalDistanceToPoint (i.e. haversine): ";

  SurfacePoint point1(Latitude(0, Angle::Degrees),
      Longitude(90, Angle::Degrees), Distance(1.5, Distance::Kilometers));
  SurfacePoint point2(Latitude(0, Angle::Degrees),
      Longitude(180, Angle::Degrees), Distance(0.5, Distance::Kilometers));

  Distance result = point1.GetDistanceToPoint(point2);
  cout << result.meters() << " meters" << endl;
}







