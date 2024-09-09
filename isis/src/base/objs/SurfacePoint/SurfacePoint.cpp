/** This is free and unencumbered software released into the public domain.
The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */
#include "SurfacePoint.h"

#include <SpiceUsr.h>

#include "IException.h"
#include "IString.h"
#include "Latitude.h"
#include "Longitude.h"
#include "SpecialPixel.h"

using namespace boost::numeric::ublas;
using namespace std;

namespace Isis {

  /**
   * Constructs an empty SurfacePoint object
   *
   */
  SurfacePoint::SurfacePoint() {
    InitCovariance();
    InitPoint();
  }

  /**
   * Constructs a new SurfacePoint object from an existing SurfacePoint.
   *
   */
  SurfacePoint::SurfacePoint(const SurfacePoint &other) {
    p_localRadius = other.p_localRadius;

    if(other.p_x) {
      p_x = new Displacement(*other.p_x);
    }
    else {
      p_x = NULL;
    }

    if(other.p_y) {
      p_y = new Displacement(*other.p_y);
    }
    else {
      p_y = NULL;
    }

    if(other.p_z) {
      p_z = new Displacement(*other.p_z);
    }
    else {
      p_z = NULL;
    }

    if(other.p_rectCovar) {
      p_rectCovar = new symmetric_matrix<double, upper>(*other.p_rectCovar);
    }
    else {
      p_rectCovar = NULL;
    }

    if(other.p_sphereCovar) {
      p_sphereCovar = new symmetric_matrix<double, upper>(*other.p_sphereCovar);
    }
    else {
      p_sphereCovar = NULL;
    }
  }


  /**
   * Constructs a SurfacePoint object with a spherical point only
   *
   * @param lat  The latitude of the surface point
   * @param lon  The longitude of the surface point
   * @param radius The radius of the surface point
   */
  SurfacePoint::SurfacePoint(const Latitude &lat, const Longitude &lon,
      const Distance &radius) {
    InitCovariance();
    InitPoint();
    SetSphericalPoint(lat, lon, radius);
  }



  /**
   * Constructs a SurfacePoint object with a spherical point and its sigmas
   *
   * @param lat  The latitude of the surface point
   * @param lon  The longitude of the surface point
   * @param radius The radius of the surface point
   *
   *               The sigmas indicate the accuracy of the point.  For instance,
   *               a latitude sigma of 5 degrees would indicate that the
   *               latitiude value could have an error or + or - 5 degrees.
   * @param sigmaLat  The sigma of the latitude
   * @param sigmaLon  The sigma of the longitude
   * @param sigmaRadius  The sigma of the local radius
   */
  SurfacePoint::SurfacePoint(const Latitude &lat, const Longitude &lon,
      const Distance &radius, const Angle &latSigma, const Angle &lonSigma,
      const Distance &radiusSigma) {
    InitCovariance();
    InitPoint();
    SetSpherical(lat, lon, radius, latSigma, lonSigma, radiusSigma);
  }


  /**
   * Constructs a SurfacePoint object with both a spherical point and
   *   its variance/covariance matrix.
   *
   */
  SurfacePoint::SurfacePoint(const Latitude &lat, const Longitude &lon,
      const Distance &radius, const symmetric_matrix<double, upper> &covar) {
    InitCovariance();
    InitPoint();
    SetSpherical(lat, lon, radius, covar);
  }


  /**
   * Constructs a SurfacePoint object with a rectangular point only
   *
   * @param x  The x coordinate of the surface point
   * @param y  The y coordinate of the surface point
   * @param z  The z coordinate of the surface point
   */
  SurfacePoint::SurfacePoint(const Displacement &x, const Displacement &y,
      const Displacement &z) {
    InitCovariance();
    InitPoint();
    SetRectangular(x, y, z);
  }


  /**
   * Constructs a SurfacePoint object with a rectangular point and sigmas
   *
   * @param x  The x coordinate of the surface point
   * @param y  The y coordinate of the surface point
   * @param z  The z coordinate of the surface point
   *
   *           The sigmas indicate the accuracy of the point.  For instance,
   *           a sigmaX=100 m, would indicate that the x coordinate was accurate
   *           to within 100 meters.
   * @param xSigma  The x coordinate of the surface point
   * @param ySigma  The y coordinate of the surface point
   * @param zSigma  The z coordinate of the surface point
   */
  SurfacePoint::SurfacePoint(const Displacement &x, const Displacement &y,
      const Displacement &z, const Distance &xSigma, const Distance &ySigma,
      const Distance &zSigma) {
    InitCovariance();
    InitPoint();
    SetRectangular(x, y, z, xSigma, ySigma, zSigma);
  }


  /**
   * Constructs a SurfacePoint object with a rectangular point and its
   *   variance/covariance matrix
   *
   * @param x  The x coordinate of the surface point
   * @param y  The y coordinate of the surface point
   * @param z  The z coordinate of the surface point
   * @param covar  The variance/covariance matrix of the point
   */
  SurfacePoint::SurfacePoint(const Displacement &x, const Displacement &y,
      const Displacement &z, const symmetric_matrix<double, upper> &covar) {
    InitCovariance();
    InitPoint();
    SetRectangular(x, y, z, covar);
  }


  /**
   * Destroys a SurfacePoint object/
   *
   */
  SurfacePoint::~SurfacePoint() {
    FreeAllocatedMemory();
  }


  /**
   * Initialize the variance/covariance matrices
   *
   */
  void SurfacePoint::InitCovariance() {
    p_rectCovar = NULL;
    p_sphereCovar = NULL;
  }


  /**
   * Initialize a surface point
   *
   */
  void SurfacePoint::InitPoint() {
    p_x = NULL;
    p_y = NULL;
    p_z = NULL;
    p_localRadius = Distance();
  }

  
  /**
   * This is a private method to set a surface point in rectangular, body-fixed
   *   coordinates.  This method isolates the procedure for setting a
   *   rectangular point in one place.
   *
   *
   * @param x  x value of body-fixed coordinate of surface point
   * @param y  y value of body-fixed coordinate of surface point
   * @param z  z value of body-fixed coordinate of surface point
   *
   * @return void
   */
  void SurfacePoint::SetRectangularPoint(const Displacement &x,
      const Displacement &y, const Displacement &z) {
 
    if (!x.isValid() || !y.isValid() || !z.isValid()) {
      IString msg = "x, y, and z must be set to valid displacements.  One or "
        "more coordinates have been set to an invalid displacement.";
      throw IException(IException::User, msg, _FILEINFO_);
    }

    if(p_x) {
      *p_x = x;
    }
    else {
      p_x = new Displacement(x);
    }

    if(p_y) {
      *p_y = y;
    }
    else {
      p_y = new Displacement(y);
    }

    if(p_z) {
      *p_z = z;
    }
    else {
      p_z = new Displacement(z);
    }

    // Added 07-30-2018 to avoid trying to set an invalid SurfacePoint.  This breaks the ringspt and cnetnewradii tests.  Also the pole...mean test.
    // if (!(this->Valid())) {
    //   IString msg = "All coodinates of the point are zero. At least one coordinate"
    //     " must be nonzero to be a valid SurfacePoint.";
    //   throw IException(IException::User, msg, _FILEINFO_);
    // }

    if (!p_localRadius.isValid()) {
      ComputeLocalRadius();
      p_localRadius = GetLocalRadius();
    }
  }


  /**
   * Set surface point in rectangular body-fixed coordinates wtih optional
   *   sigmas.
   *
   *
   * @param x  x value of body-fixed coordinate of surface point
   * @param y  y value of body-fixed coordinate of surface point
   * @param z  z value of body-fixed coordinate of surface point
   * @param xSigma  x sigma of body-fixed coordinate of surface point
   * @param ySigma  y sigma of body-fixed coordinate of surface point
   * @param zSigma  z sigma of body-fixed coordinate of surface point
   *
   * @return void
   */
  void SurfacePoint::SetRectangular(const Displacement &x,
      const Displacement &y, const Displacement &z, const Distance &xSigma,
      const Distance &ySigma, const Distance &zSigma) {
    
    // Wipe out current local radius to ensure it will be recalculated in SetRectangularPoint
     p_localRadius = Distance();
    
    SetRectangularPoint(x, y, z);

    if (xSigma.isValid() && ySigma.isValid() && zSigma.isValid())
      SetRectangularSigmas(xSigma, ySigma, zSigma);
  }


  /**
   * Set surface point in rectangular coordinates with its variance/covariance
   *   matrix in meters squared.
   *
   * @param x  x value of body-fixed coordinate of surface point
   * @param y  y value of body-fixed coordinate of surface point
   * @param z  z value of body-fixed coordinate of surface point
   * @param covar Rectangular variance/covariance matrix in m*m
   *
   * @return void
   */
  void SurfacePoint::SetRectangular(Displacement x, Displacement y, Displacement z,
                                    const symmetric_matrix<double,upper>& covar) {
    // Wipe out current local radius to ensure it will be recalulated in SetRectangularPoint
    p_localRadius = Distance();

    SetRectangularPoint(x, y, z);
    SetRectangularMatrix(covar);
  }


  /**
   * Set surface point in rectangular coordinates
   *
   * @param x  x value of body-fixed coordinate of surface point
   * @param y  y value of body-fixed coordinate of surface point
   * @param z  z value of body-fixed coordinate of surface point
   *
   * @return void
   */
  void SurfacePoint::SetRectangularCoordinates(const Displacement &x,
                                                                                     const Displacement &y,
                                                                                     const Displacement &z) {
    // Wipe out current local radius to ensure it will be recalculated in SetRectangularPoint
     p_localRadius = Distance();
     
    SetRectangularPoint(x, y, z);
  }


  /**
   * Set the rectangular sigmas into the rectangular variance/covariance
   *   matrix with diagonal element units of km^2, km^2, and km^2..
   *
   * @param xSigma x sigma of body-fixed coordinate of surface point
   * @param ySigma y sigma of body-fixed coordinate of surface point
   * @param zSigma z sigma of body-fixed coordinate of surface point
   *
   * @internal
   *   @history  2017-07-25 Debbie A. Cook  Added documentation and corrected units for diagonal 
   *                                                       elements to be km^2 instead of m^2.
   *   @history  2017-11-22 Debbie A. Cook  Updated call to SetRectangularMatrix 
   */
  void SurfacePoint::SetRectangularSigmas(const Distance &xSigma,
                                          const Distance &ySigma,
                                          const Distance &zSigma) {
    if (!xSigma.isValid() || !ySigma.isValid() || !zSigma.isValid()) {
      IString msg = "x sigma, y sigma , and z sigma must be set to valid "
        "distances.  One or more sigmas have been set to an invalid distance.";
      throw IException(IException::User, msg, _FILEINFO_);
    }

    symmetric_matrix<double,upper> covar(3);
    covar.clear();
    covar(0,0) = xSigma.meters() * xSigma.meters();
    covar(1,1) = ySigma.meters() * ySigma.meters();
    covar(2,2) = zSigma.meters() * zSigma.meters();
    SetRectangularMatrix(covar, Meters);
  }


  /**
   * Set rectangular covariance matrix and store in units of km**2
   *
   * @param covar Rectangular variance/covariance matrix 
   * @param units Units of matrix are units**2
   *
   * @return void
   */
  void SurfacePoint::SetRectangularMatrix(const symmetric_matrix<double, upper> &covar,
                                          SurfacePoint::CoordUnits units) {

    // Make sure the point is set first
    if (!Valid()) {
      IString msg = "A point must be set before a variance/covariance matrix "
        "can be set.";
      throw IException(IException::Programmer, msg, _FILEINFO_);
    }

    // Make sure the units are valid
    if (units != Kilometers && units != Meters) {
      IString msg = "Units must be Kilometers or Meters";
      throw IException(IException::Programmer, msg, _FILEINFO_);
    }
    
    // covar units are km**2
    if (p_rectCovar) {
      *p_rectCovar = covar;
    }
    else {
      p_rectCovar = new symmetric_matrix<double, upper>(covar);
    }

    if (units == Meters) {
      // Convert input matrix to km to hold in memory
      (*p_rectCovar)(0,0) = covar(0,0)/1.0e6;
      (*p_rectCovar)(0,1) = covar(0,1)/1.0e6;
      (*p_rectCovar)(0,2) = covar(0,2)/1.0e6;
      (*p_rectCovar)(1,1) = covar(1,1)/1.0e6;
      (*p_rectCovar)(1,2) = covar(1,2)/1.0e6;
      (*p_rectCovar)(2,2) = covar(2,2)/1.0e6;
    }

    SpiceDouble rectMat[3][3];

    // Compute the local radius of the surface point in meters.  We will convert to km before saving the matrix.
    double x2  = p_x->meters() * p_x->meters();
    double y2  = p_y->meters() * p_y->meters();
    double z   = p_z->meters();
    double radius = sqrt(x2 + y2 + z*z);

    // *** TODO *** Replace this section with LinearAlgebra multiply calls and avoid having to create a Spice matrix
    rectMat[0][0] = covar(0,0);
    rectMat[0][1] = rectMat[1][0] = covar(0,1);
    rectMat[0][2] = rectMat[2][0] = covar(0,2);
    rectMat[1][1] = covar(1,1);
    rectMat[1][2] = rectMat[2][1] = covar(1,2);
    rectMat[2][2] = covar(2,2);

    // Compute the Jacobian in meters.  Don't deal with unit mismatch yet to preserve precision.
    SpiceDouble J[3][3];
    double zOverR = p_z->meters() / radius;
    double r2 = radius*radius;
    double denom = r2*radius*sqrt(1.0 - (zOverR*zOverR));
    J[0][0] = -p_x->meters() * p_z->meters() / denom;
    J[0][1] = -p_y->meters() * p_z->meters() / denom;
    J[0][2] = (r2 - p_z->meters() * p_z->meters()) / denom;
    J[1][0] = -p_y->meters() / (x2 + y2);
    J[1][1] = p_x->meters() / (x2 + y2);
    J[1][2] = 0.0;
    J[2][0] = p_x->meters() / radius;  // This row is unitless
    J[2][1] = p_y->meters() / radius;
    J[2][2] = p_z->meters() / radius;

    if(!p_sphereCovar)
      p_sphereCovar = new symmetric_matrix<double, upper>(3);

    SpiceDouble mat[3][3];
    mxm_c (J, rectMat, mat);
    mxmt_c (mat, J, mat);
    if (units == Kilometers) {
      // Now take care of unit mismatch between rect matrix in km and Jacobian in m
      (*p_sphereCovar)(0,0) = mat[0][0] * 1.0e6;
      (*p_sphereCovar)(0,1) = mat[0][1] * 1.0e6;
      (*p_sphereCovar)(0,2) = mat[0][2] * 1000.0;
      (*p_sphereCovar)(1,1) = mat[1][1] * 1.0e6;

      (*p_sphereCovar)(1,2) = mat[1][2] * 1000.0;
      (*p_sphereCovar)(2,2) = mat[2][2];
    }
    else { // (units == Meters) 
      // Convert matrix lengths from m to km
      (*p_sphereCovar)(0,0) = mat[0][0];
      (*p_sphereCovar)(0,1) = mat[0][1];
      (*p_sphereCovar)(0,2) = mat[0][2] / 1000.0;
      (*p_sphereCovar)(1,1) = mat[1][1];
      (*p_sphereCovar)(1,2) = mat[1][2] / 1000.0;
      (*p_sphereCovar)(2,2) = mat[2][2] / 1.0e6;
    }
  }


  /**
   * This is a private method to set a surface point in spherical
   *   (lat/lon/radius), body-fixed coordinates.  This method isolates the
   *   procedure for setting a spherical point in one place.
   *
   * @param lat Body-fixed latitude of surface point
   * @param lon Body-fixed longitude of surface point
   * @param radius Local radius of surface point
   *
   * @return void
   */
  void SurfacePoint::SetSphericalPoint(const Latitude  &lat,
                                       const Longitude &lon,
                                       const Distance  &radius) {
// Is error checking necessary or does Latitude, Longitude, and Distance handle it?????
    if (!lat.isValid()  ||  !lon.isValid()  ||  !radius.isValid()) {
      IString msg = "Latitude, longitude, or radius is an invalid value.";
      throw IException(IException::User, msg, _FILEINFO_);
    }

    SpiceDouble dlat = (double) lat.radians();
    SpiceDouble dlon = (double) lon.radians();
    SpiceDouble dradius = radius.kilometers();

    SpiceDouble rect[3];
    latrec_c ( dradius, dlon, dlat, rect);

    // Set local radius now since we have it and avoid calculating it later
    p_localRadius = radius;

    SetRectangularPoint(Displacement(rect[0], Displacement::Kilometers),
                        Displacement(rect[1], Displacement::Kilometers),
                        Displacement(rect[2], Displacement::Kilometers));
  }


  /**
   * Set surface point in spherical body-fixed coordinates (lat/lon/radius) wtih
   *   optional sigmas.
   *
   * @param lat  Body-fixed latitude of surface point
   * @param lon  Body-fixed longitude of surface point
   * @param radius  Local radius of surface point
   * @param latSigma  Latitude sigma of of spherical coordinate of surface point
   * @param lonSigma  Longitude sigma of of spherical coordinate of surface
   *                   point
   * @param radiusSigma  Local radius sigma of of spherical coordinate of
   *                   surface point
   */
  void SurfacePoint::SetSpherical(const Latitude &lat, const Longitude &lon,
      const Distance &radius, const Angle &latSigma, const Angle &lonSigma,
      const Distance &radiusSigma) {

    SetSphericalPoint(lat, lon, radius);

    if (latSigma.isValid() && lonSigma.isValid() && radiusSigma.isValid())
      SetSphericalSigmas(latSigma, lonSigma, radiusSigma);
  }


  /**
   * Set surface point in spherical body-fixed coordinates (lat/lon/radius) with
   *   its variance/covariance matrix in radians squared.
   *
   * @param lat  Body-fixed latitude of surface point
   * @param lon  Body-fixed longitude of surface point
   * @param radius  Local radius of surface point
   * @param covar Spherical variance/covariance matrix in m*m
   */
  void SurfacePoint::SetSpherical(const Latitude &lat, const Longitude &lon,
      const Distance &radius, const symmetric_matrix<double, upper> &covar) {
    SetSphericalPoint(lat, lon, radius);
    SetSphericalMatrix(covar);
  }


  /**
   * Update spherical coordinates (lat/lon/radius)
   *
   * @param lat
   * @param lon
   * @param radius
   *
   */
  void SurfacePoint::SetSphericalCoordinates(const Latitude &lat,
                                                const Longitude &lon, const Distance &radius) {
    SetSphericalPoint(lat, lon, radius);
  }


  /**
   * Set the spherical sigmas into the spherical variance/covariance matrix in diagonal units of 
   *  radians^2, radians^2, km^2.
   *
   * @param latSigma Latitude sigma of body-fixed coordinate of surface point
   * @param lonSigma Longitude sigma of body-fixed coordinate of surface point
   * @param radiusSigma Radius sigma of body-fixed coordinate of surface point
   *
   * @internal
   *   @history  2017-07-25 Debbie A. Cook  Added documentation and corrected units for covar(2,2) 
   *                                                                 to be km^2 instead of m^2.
   *   @history  2017-11-22 Debbie A. Cook  Set units for covar(2,2) back to m^2 which is not the default
   *                                                                 for the set method. 
   */
  void SurfacePoint::SetSphericalSigmas(const Angle &latSigma,
                                        const Angle &lonSigma,
                                        const Distance &radiusSigma) {
    if (latSigma.isValid() && lonSigma.isValid() && radiusSigma.isValid()) {
      symmetric_matrix<double,upper> covar(3);
      covar.clear();

      double sphericalCoordinate;
      sphericalCoordinate = (double) latSigma.radians();
      covar(0,0) =  sphericalCoordinate*sphericalCoordinate;
      sphericalCoordinate = (double) lonSigma.radians();
      covar(1,1) = sphericalCoordinate*sphericalCoordinate;
      sphericalCoordinate = (double) radiusSigma.meters();
      covar(2,2) = sphericalCoordinate*sphericalCoordinate;

      // Use default set units for radius of meters*meters
      SetSphericalMatrix(covar);
    }
    else {
      delete p_sphereCovar;
      p_sphereCovar = NULL;

      delete p_rectCovar;
      p_rectCovar = NULL;
    }
  }


  /**
   * Set the spherical sigmas (in Distance units) into the spherical variance/covariance
   *   matrix.
   *
   * @param latSigma Latitude sigma of body-fixed coordinate of surface point
   *                  as a Distance
   * @param lonSigma Longitude sigma of body-fixed coordinate of surface point
   *                  as a Distance
   * @param radiusSigma Radius sigma of body-fixed coordinate of surface point
   *                  in meters
   * @internal
   *   @history  2018-06-28 Debbie A. Cook  Revised to use the local radius of
   *                 the SurfacePoint to convert distance to angle instead of the
   *                 major equatorial axis.  Also corrected longitude conversion.
   *                 See note in SurfacePoint.h.  References #5457.
   */
  void SurfacePoint::SetSphericalSigmasDistance(const Distance &latSigma,
    const Distance &lonSigma, const Distance &radiusSigma) {

    if (!Valid()) {
      IString msg = "Cannot set spherical sigmas on an invalid surface point";
      throw IException(IException::Programmer, msg, _FILEINFO_);
    }

    SetSphericalSigmas(Angle(MetersToLatitude(latSigma.meters()),Angle::Radians),
           Angle(MetersToLongitude(lonSigma.meters()), Angle::Radians), radiusSigma);
  }


  /**
   * Set spherical covariance matrix
   *
   * @param covar Spherical variance/covariance matrix (radians**2 for lat and lon)
   *
   * @return void
   */
  void SurfacePoint::SetSphericalMatrix(
                                        const symmetric_matrix<double, upper> & covar,
                                        SurfacePoint::CoordUnits units) {

    // Make sure the point is set first
    if (!Valid()) {
      IString msg = "A point must be set before a variance/covariance matrix "
        "can be set.";
      throw IException(IException::Programmer, msg, _FILEINFO_);
    }

    // Make sure the units are valid
    if (units != Kilometers && units != Meters) {
      IString msg = "Units must be Kilometers or Meters";
      throw IException(IException::Programmer, msg, _FILEINFO_);
    }

    // Get the radius of the point in the same units as the input matrix when saving the input matrix
    double radius = (double) GetLocalRadius().kilometers();
    
    // Save the spherical matrix in km and km**2
    if (p_sphereCovar) {
        *p_sphereCovar = covar;
    }
    else {
      p_sphereCovar = new symmetric_matrix<double, upper>(covar);
    }
        
    if (units == Meters) {
      // Convert input matrix to km to store
      (*p_sphereCovar)(0,0) = covar(0,0);
      (*p_sphereCovar)(0,1) = covar(0,1);
      (*p_sphereCovar)(0,2) = covar(0,2) / 1000.;
      (*p_sphereCovar)(1,1) = covar(1,1);
      (*p_sphereCovar)(1,2) = covar(1,2) / 1000.;
      (*p_sphereCovar)(2,2) = covar(2,2) / 1.0e6;
      radius = (double) GetLocalRadius().meters();
    }

    // ***TODO*** Consider using LinearAlgebra matrix multiply and avoid creating SpiceDouble matrix.
    SpiceDouble sphereMat[3][3];

    sphereMat[0][0] = covar(0,0);
    sphereMat[0][1] = sphereMat[1][0] = covar(0,1);
    sphereMat[0][2] = sphereMat[2][0] = covar(0,2);
    sphereMat[1][1] = covar(1,1);
    sphereMat[1][2] = sphereMat[2][1] = covar(1,2);
    sphereMat[2][2] = covar(2,2);

//     std::cout<<"Ocovar = "<<sphereMat[0][0]<<" "<<sphereMat[0][1]<<" "<<sphereMat[0][2]<<std::endl
//              <<"         "<<sphereMat[1][0]<<" "<<sphereMat[1][1]<<" "<<sphereMat[1][2]<<std::endl
//              <<"         "<<sphereMat[2][0]<<" "<<sphereMat[2][1]<<" "<<sphereMat[2][2]<<std::endl;

    // Get the lat/lon/radius of the point
    double lat = (double) GetLatitude().radians();
    double lon = (double) GetLongitude().radians();

    // Compute the Jacobian in same units as input matrix.
    SpiceDouble J[3][3];
    double cosPhi = cos(lat);
    double sinPhi = sin(lat);
    double cosLamda = cos(lon);
    double sinLamda = sin(lon);
    double rcosPhi = radius*cosPhi;
    double rsinPhi = radius*sinPhi;
    J[0][0] = -rsinPhi * cosLamda;
    J[0][1] = -rcosPhi * sinLamda;
    J[0][2] = cosPhi * cosLamda;
    J[1][0] = -rsinPhi * sinLamda;
    J[1][1] = rcosPhi * cosLamda;
    J[1][2] = cosPhi * sinLamda;
    J[2][0] = rcosPhi;
    J[2][1] = 0.0;
    J[2][2] = sinPhi;

    if(!p_rectCovar)
      p_rectCovar = new symmetric_matrix<double, upper>(3);

    SpiceDouble mat[3][3];
    mxm_c (J, sphereMat, mat);
    mxmt_c (mat, J, mat);

    if (units == Kilometers) {
      (*p_rectCovar)(0,0) = mat[0][0];
      (*p_rectCovar)(0,1) = mat[0][1];
      (*p_rectCovar)(0,2) = mat[0][2];
      (*p_rectCovar)(1,1) = mat[1][1];
      (*p_rectCovar)(1,2) = mat[1][2];
      (*p_rectCovar)(2,2) = mat[2][2];
    }
    else { // (units == Meters) 
      //  Convert to km
      (*p_rectCovar)(0,0) = mat[0][0]/1.0e6;
      (*p_rectCovar)(0,1) = mat[0][1]/1.0e6;
      (*p_rectCovar)(0,2) = mat[0][2]/1.0e6;
      (*p_rectCovar)(1,1) = mat[1][1]/1.0e6;
      (*p_rectCovar)(1,2) = mat[1][2]/1.0e6;
      (*p_rectCovar)(2,2) = mat[2][2]/1.0e6;
    }
//     std::cout<<"Rcovar = "<<p_rectCovar(0,0)<<" "<<p_rectCovar(0,1)<<" "<<p_rectCovar(0,2)<<std::endl
//              <<"         "<<p_rectCovar(1,0)<<" "<<p_rectCovar(1,1)<<" "<<p_rectCovar(1,2)<<std::endl
//              <<"         "<<p_rectCovar(2,0)<<" "<<p_rectCovar(2,1)<<" "<<p_rectCovar(2,2)<<std::endl;
  }
  

  /**
   * Set  covariance matrix in km
   *
   * @param covar variance/covariance matrix
   *
   * @return void
   */
  void SurfacePoint::SetMatrix(CoordinateType type, const symmetric_matrix<double, upper>& covar) {

    switch (type) {   
      case Latitudinal:
        SetSphericalMatrix(covar, SurfacePoint::Kilometers);
        break;
      case Rectangular:
        SetRectangularMatrix(covar, SurfacePoint::Kilometers);
        break;
      default:
         IString msg ="Unknown surface point coordinate type enum [" + toString(type) + "]." ;
         throw IException(IException::Programmer, msg, _FILEINFO_);
    }
  }


   /**
   * Compute partial derivative of the conversion of the body-fixed surface point coordinate to the 
   * specified coordinate type.
   *
   * @param covar variance/covariance matrix
   *
   * @return void
   */
  std::vector<double> SurfacePoint::Partial(CoordinateType type, CoordIndex index) {
    std::vector<double> derivative(3);
    switch (type) {
      case Latitudinal:
        derivative = LatitudinalDerivative(index);
        break;
      case Rectangular:
        derivative = RectangularDerivative(index);
        break;
      default:
         IString msg ="Unknown surface point coordinate type enum [" + toString(type) + "]." ;
         throw IException(IException::Programmer, msg, _FILEINFO_);
    }
    return derivative;
  } 


   /**
   * Compute partial derivative of the conversion of the latitudinal coordinates to body-fixed 
   *   rectangular coordinates with respect to the indicated coordinate.
   *
   * @param index Coordinate index
   *
   * @return @b std::vector<double>  The derivative of the latitudinal to body-fixed vector
   */
  std::vector<double> SurfacePoint::LatitudinalDerivative(CoordIndex index) {
    std::vector<double> derivative(3);
    double rlat = GetLatitude().radians();
    double rlon = GetLongitude().radians();
    double sinLon = sin(rlon);
    double cosLon = cos(rlon);
    double sinLat = sin(rlat);
    double cosLat = cos(rlat);
    double radkm = GetLocalRadius().kilometers();

    switch (index) {
      case One:
        derivative[0] = -radkm * sinLat * cosLon;
        derivative[1] = -radkm * sinLon * sinLat;
        derivative[2] =  radkm * cosLat;
        break;
      case Two:
        derivative[0] = -radkm * cosLat * sinLon;
        derivative[1] =  radkm * cosLat * cosLon;
        derivative[2] =  0.0;
        break;
      case Three:
        derivative[0] = cosLon * cosLat;
        derivative[1] = sinLon * cosLat;
        derivative[2] = sinLat;
        break;
      default:
        IString msg = "Invalid coordinate index  (must be less than 3).";
        throw IException(IException::Programmer, msg, _FILEINFO_);
    }
    return derivative;
  } 


   /**
   * Compute partial derivative of the body-fixed rectangular coordinates with respect to the 
   *   indicated coordinate.
   *
   * @param index Coordinate index
   *
   * @return @b std::vector<double>  The derivative of the body-fixed vector
   */
  std::vector<double> SurfacePoint::RectangularDerivative(CoordIndex index) {
    std::vector<double> derivative(3,0.0);

    switch (index) {
      case One:
        derivative[0] = 1.;
        break;
      case Two:
        derivative[1] =  1.;
        break;
      case Three:
        derivative[2] = 1.;
        break;
      default:
        IString msg = "Invalid coordinate index  (must be less than 3).";
        throw IException(IException::Programmer, msg, _FILEINFO_);
    }
    return derivative;
  }

  
  /**
   * A naif array is a c-style array of size 3. The element types are double...
   * keep in mind a SpiceDouble is a double. The values' units are
   * kilometers because that is the unit naif works in. The first element is X,
   * the second Y, and the third Z.
   *
   * @param naifOutput The naif array to populate with the surface point's
   *                   XYZ position.
   */
  void SurfacePoint::ToNaifArray(double naifOutput[3]) const {
    if(Valid()) {
      naifOutput[0] = p_x->kilometers();
      naifOutput[1] = p_y->kilometers();
      naifOutput[2] = p_z->kilometers();
    }
    else {
      IString msg = "Cannot convert an invalid surface point to a naif array";
      throw IException(IException::Programmer, msg, _FILEINFO_);
    }
  }

  /**
   * A naif array is a c-style array of size 3. The element types are double...
   * keep in mind a SpiceDouble is a double. The values' units are
   * kilometers because that is the unit naif works in. The first element is X,
   * the second Y, and the third Z. This loads the naif array into the surface
   * point.
   *
   * @param naifValues The naif array to use as rectangular coordinates
   */
  void SurfacePoint::FromNaifArray(const double naifValues[3]) {
    if(p_x && p_y && p_z) {
      p_x->setKilometers(naifValues[0]);
      p_y->setKilometers(naifValues[1]);
      p_z->setKilometers(naifValues[2]);
    }
    else {
      p_x = new Displacement(naifValues[0], Displacement::Kilometers);
      p_y = new Displacement(naifValues[1], Displacement::Kilometers);
      p_z = new Displacement(naifValues[2], Displacement::Kilometers);
    }
    
    ComputeLocalRadius();
    p_localRadius = GetLocalRadius();
  }


  /**
   * This method resets the local radius of a SurfacePoint
   *
   * @param radius The new local radius value to set
   *
   */
  void SurfacePoint::ResetLocalRadius(const Distance &radius) {

    if (!radius.isValid()) {
      IString msg = "Radius value must be a valid Distance.";
      throw IException(IException::Programmer, msg, _FILEINFO_);
    }

    if (!p_x || !p_y || !p_z || !p_x->isValid() || !p_y->isValid() ||
        !p_z->isValid()) {
        IString msg = "In order to reset the local radius, a Surface Point must "
          "already be set.";
        throw IException(IException::Programmer, msg, _FILEINFO_);
    }

    // Set latitudinal coordinates
    SpiceDouble lat = (double) GetLatitude().radians();
    SpiceDouble lon = (double) GetLongitude().radians();
    
    p_localRadius = radius;
    
    // Set rectangular coordinates
    SpiceDouble rect[3];
    latrec_c ((SpiceDouble) radius.kilometers(), lon, lat, rect);
    p_x->setKilometers(rect[0]);
    p_y->setKilometers(rect[1]);
    p_z->setKilometers(rect[2]);

    // TODO What should be done to the variance/covariance matrix when the
    // radius is reset??? With Bundle updates will this functionality be
    // obsolete??? Ask Ken
  }


  bool SurfacePoint::Valid() const {
    static const Displacement zero(0, Displacement::Meters);
    return p_x && p_y && p_z && p_x->isValid() && p_y->isValid() && p_z->isValid() &&
           (*p_x != zero || *p_y != zero || *p_z != zero);
  }


  /**
   * This method returns a coordinate of a SurfacePoint
   *
   * @param type The coordinate type to return (see CoordinateType in .h file)
   * @param index The coordinate index to return (1 <= index <= 3)
   * @param units The units in which to return the coordinate value (see CoordinateUnits in .h file)
   *
   */
  double SurfacePoint::GetCoord(CoordinateType type, CoordIndex index, CoordUnits units) {
    // TODO *** Is there a better way to satisfy the compiler that a value will be initialized? 
    //                 Don't the enums take care of preventing any other possibilities? no
    double value = 0.;
    
    switch (type) {
      
      case Latitudinal:
      
        switch (index) {
            
          case One:  // Latitude
            value = LatToDouble(GetLatitude(), units);
            break;
            
          case Two:  // Longitude
            value = LonToDouble(GetLongitude(), units);
            break;
            
          case Three:  // LocalRadius
            value = DistanceToDouble(GetLocalRadius(), units);
            break;
            
          default:
            IString msg = "Invalid coordinate index  (must be less than 3).";
            throw IException(IException::Programmer, msg, _FILEINFO_);
          }
        break;

      case Rectangular:
      
        switch (index) {
            
          case One:  // X
            value = DisplacementToDouble(GetX(), units);
            break;
            
          case Two: // Y
            value = DisplacementToDouble(GetY(), units);
            break;
            
          case Three:  // Z
            value = DisplacementToDouble(GetZ(), units);
            break;
            
          default:
            IString msg = "Invalid coordinate index  enum (must be less than 3).";
            throw IException(IException::Programmer, msg, _FILEINFO_);
          }
        break;

       default:
         IString msg ="Unknown surface point coordinate type enum [" + toString(type) + "]." ;
         throw IException(IException::Programmer, msg, _FILEINFO_);
    }
    return value;
  }


  /**
   * This method returns a sigma of a SurfacePoint coordinate
   *
   * @param type The coordinate type to return (see CoordinateType in .h file)
   * @param index The coordinate index to return (1 <= index <= 3)
   * @param units The units in which to return the coordinate value (see CoordinateUnits in .h file)
   *
   */
  double SurfacePoint::GetSigma(CoordinateType type, CoordIndex index, CoordUnits units) {
    double value = 0;  // See first TODO in GetCoord
    
    switch (type) {
      
      case Latitudinal:
      
        switch (index) {
            
          case One:
            value = DistanceToDouble(GetLatSigmaDistance(), units);
            break;
            
          case Two:
             value = DistanceToDouble(GetLonSigmaDistance(), units);
             break;
            
          case Three:
             value = DistanceToDouble(GetLocalRadiusSigma(), units);
             break;
            
          default:
            IString msg = "Invalid coordinate index  (must be less than 3).";
            throw IException(IException::Programmer, msg, _FILEINFO_);
          }
        break;

      case Rectangular:
      
        switch (index) {
            
          case One:
            value = DistanceToDouble(GetXSigma(), units);
            break;
            
          case Two:
             value = DistanceToDouble(GetYSigma(), units);
             break;
            
          case Three:
             value = DistanceToDouble(GetZSigma(), units);
             break;
            
          default:
            IString msg = "Invalid coordinate index  (must be less than 3).";
            throw IException(IException::Programmer, msg, _FILEINFO_);
          }
        break;

       default:
         IString msg ="Unknown surface point coordinate type enum [" + toString(type) + "]." ;
         throw IException(IException::Programmer, msg, _FILEINFO_);
    }
            
    return value;
  }


  /**
   * This method returns a sigma of a SurfacePoint coordinate as a Distance
   *
   * @param type The coordinate type to return (see CoordinateType in .h file)
   * @param index The coordinate index to return (1 <= index <= 3)
   *
   */
  Distance SurfacePoint::GetSigmaDistance(CoordinateType type,
                                                         CoordIndex index) {
    Distance dist = Distance();  // See first TODO in GetCoord
    
    switch (type) {
      
      case Latitudinal:
      
        switch (index) {
            
          case One:
            dist = GetLatSigmaDistance();
            break;
            
          case Two:
             dist = GetLonSigmaDistance();
             break;
            
          case Three:
             dist = GetLocalRadiusSigma();
             break;
            
          default:
            IString msg = "Invalid coordinate index  (must be less than 3).";
            throw IException(IException::Programmer, msg, _FILEINFO_);
          }
        break;

      case Rectangular:
      
        switch (index) {
            
          case One:
            dist = GetXSigma();
            break;
            
          case Two:
             dist = GetYSigma();
             break;
            
          case Three:
             dist = GetZSigma();
             break;
            
          default:
            IString msg = "Invalid coordinate index  (must be less than 3).";
            throw IException(IException::Programmer, msg, _FILEINFO_);
          }
        break;

       default:
         IString msg ="Unknown surface point coordinate type enum [" + toString(type) + "]." ;
         throw IException(IException::Programmer, msg, _FILEINFO_);
    }
            
    return dist;
  }


  /**
   * This method returns a double version of a Displacement in the specified units
   *
   * @param disp The displacement to convert to a double
   * @param units The units in which to return the displacement (see CoordinateUnits in .h file)
   *
   */
  double SurfacePoint::DisplacementToDouble(Displacement disp, CoordUnits units) {
    double value;
    
    switch (units) {
      
        case Meters:
          value = disp.meters();
          break;
          
        case Kilometers:
          value = disp.kilometers();
          break;
          
        default:
           IString msg = "Unrecognized unit for a Displacement (must be meters or kilometers).";
           throw IException(IException::Programmer, msg, _FILEINFO_);
      }
    return value;
  }
      

  /**
   * This method returns a double version of a Distance in the specified units
   *
   * @param dist The distance to convert to a double
   * @param units The units in which to return the distance (see CoordinateUnits in .h file)
   *
   */
  double SurfacePoint::DistanceToDouble(Distance dist, CoordUnits units) {
    double value;
    
    switch (units) {
      
        case Meters:
          value = dist.meters();
          break;
          
        case Kilometers:
          value = dist.kilometers();
          break;
          
        default:
           IString msg = "Unrecognized unit for a Distance (not meters or kilometers).";
           throw IException(IException::Programmer, msg, _FILEINFO_);
      }
    return value;
  }


  /**
   * This method returns a double version of a Latitude in the specified units
   *
   * @param lat The latitude to convert to a double
   * @param units The units in which to return the latitude (see CoordinateUnits in .h file)
   *
   */
  double SurfacePoint::LatToDouble(Latitude lat, CoordUnits units) {
    double value;
    
    switch (units) {
      
      case Radians:
        value = GetLatitude().radians();
        break;

      case Degrees:
        value = GetLatitude().degrees();
        break;

       default:
         IString msg = "Invalid unit for latitude (must be Radians or Degrees).";
         throw IException(IException::Programmer, msg, _FILEINFO_);
    }
    return value;
  }


  /**
   * This method returns an angular measure of a distance in the direction 
   * of and relative to the latitude of the SurfacePoint.  It should only be used to convert 
   * lengths relative to the SurfacePoint and in the direction of latitude.
   * Typical usage would be to convert latitude sigmas and point 
   * corrections for the SurfacePoint.
   *
   * @param latLength The latitude in meters to convert to radian units
   * @return @b LatDispAngle The converted linear measure in radian units
   *
   */
  double SurfacePoint::MetersToLatitude(double latLength) {
    if (Valid() && !IsSpecial(latLength)) {
      // Convert angle measure in meters to radians relative to latitude of SurfacePoint.
      return latLength / GetLocalRadius().meters();
    }
    else {
      // Return Null to be consistent with the bundle classes
      return Isis::Null;
    }
  }


  /**
   * This method returns an angular measure in radians of a distance in the direction 
   * of and relative to the longitude of the SurfacePoint.  It should only be used to  
   * convert lengths relative to the SurfacePoint and in the direction of longitude.
   * Typical usage is to convert longitude sigmas and point corrections for the 
   * SurfacePoint.
   *
   * @param lonLength The delta longitude distance in meters  to convert to radians
   * @return @b LonDistAngle The converted delta length in radians
   *
   * @internal
   *   @history  2019-05-29 Debbie A. Cook  Changed test constant from DBL_EPSILON
   *                                            to 1.0e-50 to fix possible false errors from reasonably 
   *                                            small distances on fixed or tightly constrained points 
   *                                            occurring during error propagation in jigsaw.
   */
  double SurfacePoint::MetersToLongitude(double deltaLonMeters) {
    
    if (Valid() && !IsSpecial(deltaLonMeters)) {
      double convFactor = cos((double)GetLatitude().radians());
      double deltaLonRadians;

      // Convert angle displacement to radians relative to longitude of SurfacePoint.      
      if (convFactor > 1.0e-50) {             
        deltaLonRadians = deltaLonMeters / (convFactor*GetLocalRadius().meters());
      }
      else {
        //  Brent Archinal suggested setting sigma to pi in the case of a point near the pole
        deltaLonRadians = PI;
      }
      return deltaLonRadians;
    }
    else {
      // Return Null to be consistent with the bundle classes
      return Isis::Null;
    }
  }


  /**
   * This method returns a Displacement of an Angle relative to the current 
   * SurfacePoint latitude.  It should only be used to convert relative latitudes 
   * near the SurfacePoint latitude.  Typical usage would be to convert
   * latitude sigmas and point corrections for the SurfacePoint.
   *
   * @param latRadians The latitude in Angle units to convert to Displacement units
   * @return @b LatDisp The converted latitude in displacement units at the 
   *                                  SurfacePoint latitude
   *
   */
  double SurfacePoint::LatitudeToMeters(double relativeLat) const {
    // Returns are consistent with the bundle classes
    if (relativeLat == 0.) {
      return 0.;
    }
    else if (Valid() && !IsSpecial(relativeLat) && GetLocalRadius().isValid() ) {
      return relativeLat * GetLocalRadius().meters();
    }
    else {
      return Isis::Null;
    }
  }


  /**
   * This method returns a length in meters version of a delta longitude angle 
   * in radians  relative to the current SurfacePoint longitude.  It should only be 
   * used to convert delta longitudes relative to the SurfacePoint longitude.  
   * Typical usage would be to convert longitude sigmas and point corrections 
   * for the SurfacePoint.
   *
   * @param lonRadians The delta longitude in radians to convert to meters
   * @return @b relativeLonDist The delta longitude in meters from the 
   *                                  SurfacePoint longitude
   *
   */
  double SurfacePoint::LongitudeToMeters(double deltaLonRadians) const{
    // Returns are consistent with the bundle classes
    double deltaLonMeters = Isis::Null;

    if (deltaLonRadians == 0.) {
      deltaLonMeters = 0.;
    }
    else if (Valid() && !IsSpecial(deltaLonRadians) && GetLocalRadius().isValid() ) {
      // Convert from radians to meters and return
      double scalingRadius = cos(GetLatitude().radians()) * GetLocalRadius().meters();
      deltaLonMeters = deltaLonRadians * scalingRadius;
    }
    
    return deltaLonMeters;
  }


    /**
   * This method converts the given string value to a SurfacePoint::CoordinateType
   * enumeration.  Currently (March 31, 2017) accepted inputs are listed below.  
   * This method is case insensitive.
   *
   * @param type The coordinate type name to be converted
   * @return @b CoordinateType The enumeration corresponding to the input name
   *
   */
  SurfacePoint::CoordinateType
                 SurfacePoint::stringToCoordinateType(QString type)  {
    if (type.compare("LATITUDINAL", Qt::CaseInsensitive) == 0) {
      return SurfacePoint::Latitudinal;
    }
    else if (type.compare("RECTANGULAR", Qt::CaseInsensitive) == 0) {
      return SurfacePoint::Rectangular;
    }
    else {
      throw IException(IException::Programmer,
                          "Unknown coordinate type for a SurfacePoint [" + type.toStdString() + "].",
                          _FILEINFO_);
    }
  }


  /**
   * Converts the given SurfacePoint::CoordinateType enumeration to a string. 
   * This method is used to print the type of control point coordinates used in 
   * the bundle adjustment. 
   * 
   * @param type The Coordinate Type enumeration to be converted.
   * 
   * @return @b QString The name associated with the given CoordinateType.
   * 
   * @throw Isis::Exception::Programmer "Unknown SurfacePoint CoordinateType enum."
   */
  QString SurfacePoint::coordinateTypeToString(
                                               SurfacePoint::CoordinateType type) {
    switch (type) {
      
      case Latitudinal:
        return "Latitudinal";
        break;
        
      case Rectangular:
        return "Rectangular";
        break;

       default:
         IString msg ="Unknown surface point coordinate type enum [" + toString(type) + "]." ;
         throw IException(IException::Programmer, msg, _FILEINFO_);
    }
  }

  
  /**
   * This method returns a double version of a Longitude in the specified units
   *
   * @param lon The longitude to convert to a double
   * @param units The units in which to return the longitude (see CoordinateUnits in .h file)
   *
   */
  double SurfacePoint::LonToDouble(Longitude lon, CoordUnits units) {
    double value;
    
    switch (units) {
      
      case Radians:
        value = GetLongitude().radians();
        break;

      case Degrees:
        value = GetLongitude().degrees();
        break;

       default:
         IString msg = "Invalid unit for longitude (must be Radians of Degrees).";
         throw IException(IException::Programmer, msg, _FILEINFO_);
    }
    return value;
  }
  
    
  Displacement SurfacePoint::GetX() const {
    if(!p_x) return Displacement();

    return *p_x;
  }


  Displacement SurfacePoint::GetY() const {
    if(!p_y) return Displacement();

    return *p_y;
  }


  Displacement SurfacePoint::GetZ() const {
    if(!p_z) return Displacement();

    return *p_z;
  }


  Distance SurfacePoint::GetXSigma() const {
    if(!p_rectCovar) return Distance();

    return Distance(sqrt((*p_rectCovar)(0, 0)), Distance::Kilometers);
  }


  Distance SurfacePoint::GetYSigma() const {
    if(!p_rectCovar) return Distance();

    return Distance(sqrt((*p_rectCovar)(1, 1)), Distance::Kilometers);
  }


  Distance SurfacePoint::GetZSigma() const {
    if(!p_rectCovar) return Distance();

    return Distance(sqrt((*p_rectCovar)(2, 2)), Distance::Kilometers);
  }


  /**
   * This method returns the weight of a SurfacePoint coordinate
   * Note:  At this time a units argument is not included.  If BundleAdjust
   *            is modified to allow different distance or displacement units
   *            other than kilometers, the units argument can be added similar
   *            to the GetCoord and GetSigma methods.  Angle weights are in
   *            1/rad^2 and distance and displacements are in 1/km^2
   *
   * @param type The coordinate type to return (see CoordinateType in .h file)
   * @param index The coordinate index to return (1 <= index <= 3)
   *
   *
   */
  double SurfacePoint::GetWeight(CoordinateType type, CoordIndex index) {
    double value = 0;  // See first TODO in GetCoord
    
    switch (type) {
      
      case Latitudinal:
      
        switch (index) {
            
          case One:
            value = GetLatWeight();  // 1/radians**2
            break;
            
          case Two:
            value = GetLonWeight();  // 1/radians**2
            break;
            
          case Three:
            value = GetLocalRadiusWeight();  // 1/km**2
            break;
            
          default:
            IString msg = "Invalid coordinate index  (must be less than 3).";
            throw IException(IException::Programmer, msg, _FILEINFO_);
          }
        break;

      case Rectangular:
      
        switch (index) {
            
          case One:
            value = GetXWeight();   // 1/km**2
            break;
            
          case Two:
            value = GetYWeight();   // 1/km**2
            break;
            
          case Three:
            value = GetZWeight();  // 1/km**2
            break;
            
          default:
            IString msg = "Invalid coordinate index  (must be less than 3).";
            throw IException(IException::Programmer, msg, _FILEINFO_);
          }
        break;

       default:
         IString msg ="Unknown surface point coordinate type enum [" + toString(type) + "]." ;
         throw IException(IException::Programmer, msg, _FILEINFO_);
    }
    return value;
  }

  
  /**
  * Return X weight for bundle adjustment
  * Units are 1/(kilometers)^2
  *
  */
  double SurfacePoint::GetXWeight() const {

    double dXSigma = GetXSigma().kilometers();

    if (dXSigma <= 0.0 ) {
        IString msg = "SurfacePoint::GetXWeight(): Sigma <= 0.0";
        throw IException(IException::Programmer, msg, _FILEINFO_);
    }

    return 1.0/(dXSigma*dXSigma);
  }


  /**
  * Return Y weight for bundle adjustment
  * Units are 1/(kilometers)^2
  *
  */
  double SurfacePoint::GetYWeight() const {

    double dYSigma = GetYSigma().kilometers();

    if (dYSigma <= 0.0 ) {
        IString msg = "SurfacePoint::GetYWeight(): Sigma <= 0.0";
        throw IException(IException::Programmer, msg, _FILEINFO_);
    }

    return 1.0/(dYSigma*dYSigma);
  }


  /**
  * Return Z weight for bundle adjustment
  * Units are 1/(kilometers)^2
  *
  */
  double SurfacePoint::GetZWeight() const {

    double dZSigma = GetZSigma().kilometers();

    if (dZSigma <= 0.0 ) {
        IString msg = "SurfacePoint::GetZWeight(): Sigma <= 0.0";
        throw IException(IException::Programmer, msg, _FILEINFO_);
    }

    return 1.0/(dZSigma*dZSigma);
  }


  symmetric_matrix<double, upper> SurfacePoint::GetRectangularMatrix
                               (SurfacePoint::CoordUnits units) const {
    if(!p_rectCovar) {
      symmetric_matrix<double, upper> tmp(3);
      tmp.clear();
      return tmp;
    }

    // Make sure the units are valid
    if (units != Kilometers && units != Meters) {
      IString msg = "Units must be Kilometers or Meters";
      throw IException(IException::Programmer, msg, _FILEINFO_);
    }
    
    symmetric_matrix<double, upper> covar(3);
    covar.clear();
    
    switch (units) {
      
      case Meters:
        // Convert member matrix to Meters to return
        covar(0,0) = (*p_rectCovar)(0,0)*1.0e6;
        covar(0,1) = (*p_rectCovar)(0,1)*1.0e6;
        covar(0,2) = (*p_rectCovar)(0,2)*1.0e6;
        covar(1,1) = (*p_rectCovar)(1,1)*1.0e6;
        covar(1,2) = (*p_rectCovar)(1,2)*1.0e6;
        covar(2,2) = (*p_rectCovar)(2,2)*1.0e6;
        return covar;
        break;
        
      case Kilometers:
        return *p_rectCovar;
        break;
        
    default:
        IString msg = "Unrecognized unit for a Displacement (must be meters or kilometers).";
        throw IException(IException::Programmer, msg, _FILEINFO_);
    }
    
    return *p_rectCovar;
  }


  Angle SurfacePoint::GetLatSigma() const {
    if(!p_sphereCovar)
      return Angle();

    return Angle(sqrt((*p_sphereCovar)(0, 0)), Angle::Radians);
  }


  Angle SurfacePoint::GetLonSigma() const {
    if(!p_sphereCovar)
      return Angle();

    return Angle(sqrt((*p_sphereCovar)(1, 1)), Angle::Radians);
  }


  /**
   * Return the body-fixed latitude for the surface point
   *
   */
    Latitude SurfacePoint::GetLatitude() const {
      if (!Valid())
        return Latitude();

      // TODO Scale for accuracy with coordinate of largest magnitude
      double x = p_x->meters();
      double y = p_y->meters();
      double z = p_z->meters();

      if (x != 0.  ||  y != 0.  || z != 0.)
        return Latitude(atan2(z, sqrt(x*x + y*y) ), Angle::Radians);
      else
        return Latitude();
    }


    /**
     * Return the body-fixed longitude for the surface point
     *
     */
    Longitude SurfacePoint::GetLongitude() const {
      if (!Valid())
        return Longitude();

      double x = p_x->meters();
      double y = p_y->meters();

      if(x == 0.0 && y == 0.0) {
        return Longitude(0, Angle::Radians);
      }

      double lon = atan2(y, x);
      if (lon < 0) {
        lon += 2 * PI;
      }

      return Longitude(lon, Angle::Radians);
    }


  /**
   * Compute the local radius of the surface point
   *
   */
    void SurfacePoint::ComputeLocalRadius() {
    static const Displacement zero(0, Displacement::Meters);
      if (Valid()) {
        double x = p_x->meters();
        double y = p_y->meters();
        double z = p_z->meters();

        p_localRadius = Distance(sqrt(x*x + y*y + z*z), Distance::Meters);
      }
      else if (*p_x == zero && *p_y == zero && *p_z == zero) { // for backwards compatability
        p_localRadius = Distance(0., Distance::Meters);
      }
      else { // Invalid point
        IString msg = "SurfacePoint::Can't compute local radius on invalid point";
        throw IException(IException::Programmer, msg, _FILEINFO_);
      }
    }


  /**
   * Return the radius of the surface point
   *
   */
    Distance SurfacePoint::GetLocalRadius() const {
     return p_localRadius;
    }


  /**
   * Return the latitude sigma as a Distance
   *
   * @internal
   *   @history  2019-05-29 Debbie A. Cook  Changed test constant from DBL_EPSILON
   *                                            to 1.0e-50 to fix possible false errors from reasonably 
   *                                            small distances on fixed or tightly constrained points 
   *                                            occurring during error propagation in jigsaw.
   *
   */
  Distance SurfacePoint::GetLatSigmaDistance() const {
    double d = LatitudeToMeters(GetLatSigma().radians());

    if (d > 1.0e-50)  {
      return Distance(d,  Distance::Meters);
    }
    else {
      return Distance();
    }
  }


  /**
   * Return the longitude sigma in meters
   *
   */
  Distance SurfacePoint::GetLonSigmaDistance() const{
    // return lonSigmaDistance;
    double d = LongitudeToMeters(GetLonSigma().radians());
// TODO What do we do when the scaling radius is 0 (at the pole)?
    // if (d > DBL_EPSILON)  {  
      return Distance(d,  Distance::Meters);
    // }
    // else { // Too close to the pole
    //   return Distance();
    // }
  }


  Distance SurfacePoint::GetLocalRadiusSigma() const {
    if(!p_sphereCovar)
      return Distance();

    return Distance(sqrt((*p_sphereCovar)(2, 2)), Distance::Kilometers);
  }


  symmetric_matrix<double, upper> SurfacePoint::GetSphericalMatrix
                               (SurfacePoint::CoordUnits units) const {
    if(!p_sphereCovar) {
      symmetric_matrix<double, upper> tmp(3);
      tmp.clear();
      return tmp;
    }

    // Make sure the units are valid
    if (units != Kilometers && units != Meters) {
      IString msg = "Units must be Kilometers or Meters";
      throw IException(IException::Programmer, msg, _FILEINFO_);
    }
    
    symmetric_matrix<double, upper> covar(3);
    covar.clear();
    
    switch (units) {
      
      case Meters:
        // Convert member matrix to Meters to return
        covar(0,0) = (*p_sphereCovar)(0,0);
        covar(0,1) = (*p_sphereCovar)(0,1);
        covar(0,2) = (*p_sphereCovar)(0,2)*1000.;
        covar(1,1) = (*p_sphereCovar)(1,1);
        covar(1,2) = (*p_sphereCovar)(1,2)*1000.;
        covar(2,2) = (*p_sphereCovar)(2,2)*1.0e6;
        return covar;
        break;
        
      case Kilometers:
        return *p_sphereCovar;
        break;
        
    default:
        IString msg = "Unrecognized unit for a Displacement (must be meters or kilometers).";
        throw IException(IException::Programmer, msg, _FILEINFO_);
    }

    return *p_sphereCovar;
  }


  /**
   * Return latitude weight for bundle adjustment
   * Units are 1/(radians)^2
   *
   */
  double SurfacePoint::GetLatWeight() const {
    double dlatSigma = GetLatSigma().radians();

      if( dlatSigma <= 0.0 ) {
          IString msg = "SurfacePoint::GetLatWeight(): Sigma <= 0.0";
          throw IException(IException::Programmer, msg, _FILEINFO_);
      }

      return 1.0/(dlatSigma*dlatSigma);
  }

  /**
  * Return longitude weight for bundle adjustment
  * Units are 1/(radians)^2
  *
  */
  double SurfacePoint::GetLonWeight() const {
    double dlonSigma = GetLonSigma().radians();

        if( dlonSigma <= 0.0 ) {
            IString msg = "SurfacePoint::GetLonWeight(): Sigma <= 0.0";
            throw IException(IException::Programmer, msg, _FILEINFO_);
        }

        return 1.0/(dlonSigma*dlonSigma);
      }

  /**
  * Return radius weight for bundle adjustment
  * Units are 1/(kilometers)^2
  *
  */
  double SurfacePoint::GetLocalRadiusWeight() const {

    double dlocalRadiusSigma = GetLocalRadiusSigma().kilometers();

        if (dlocalRadiusSigma <= 0.0 ) {
            IString msg = "SurfacePoint::GetRadWeight(): Sigma <= 0.0";
            throw IException(IException::Programmer, msg, _FILEINFO_);
        }

        return 1.0/(dlocalRadiusSigma*dlocalRadiusSigma);
      }

  /**
   * Computes and returns the distance between two surface points.  The average of 
   * the local radii will be used.
   */
  Distance SurfacePoint::GetDistanceToPoint(const SurfacePoint &other) const {
    
    if(!Valid() || !other.Valid())
      return Distance();

    return GetDistanceToPoint(other,
        ((GetLocalRadius() + other.GetLocalRadius()) / 2.0));
  }


  /**
   * Computes and returns the distance between two surface points,
   * assuming both points are on a sphere with the given radius.
   *
   * This uses the haversine formula to compute the distance.
   * Using a spherical model gives errors typically <1%
   */
  Distance SurfacePoint::GetDistanceToPoint(const SurfacePoint &other,
      const Distance &sphereRadius) const {
    if(!Valid() || !other.Valid())
      return Distance();

    // Convert lat/lon values to radians
    const Angle &latitude = GetLatitude();
    const Angle &longitude = GetLongitude();
    const Angle &otherLatitude = other.GetLatitude();
    const Angle &otherLongitude = other.GetLongitude();

    // The harvestine method:
    //   http://en.wikipedia.org/wiki/Haversine_formula
    Angle deltaLat = latitude - otherLatitude;
    Angle deltaLon = longitude - otherLongitude;

    double haversinLat = sin(deltaLat.radians() / 2.0);
    haversinLat *= haversinLat;

    double haversinLon = sin(deltaLon.radians() / 2.0);
    haversinLon *= haversinLon;

    double a = haversinLat + cos(latitude.radians()) *
               cos(otherLatitude.radians()) *
               haversinLon;

    double c = 2 * atan(sqrt(a) / sqrt(1 - a));

    return sphereRadius * c;
  }


  bool SurfacePoint::operator==(const SurfacePoint &other) const {
    bool equal = true;

    if(equal && p_x && p_y && p_z && other.p_x && other.p_y && other.p_z) {
      equal = equal && *p_x == *other.p_x;
      equal = equal && *p_y == *other.p_y;
      equal = equal && *p_z == *other.p_z;
    }
    else if(equal) {
      equal = equal && p_x == NULL && other.p_x == NULL;
      equal = equal && p_y == NULL && other.p_y == NULL;
      equal = equal && p_z == NULL && other.p_z == NULL;
    }

    if(equal && p_rectCovar) {
      equal = equal && (*p_rectCovar)(0, 0) == (*other.p_rectCovar)(0, 0);
      equal = equal && (*p_rectCovar)(0, 1) == (*other.p_rectCovar)(0, 1);
      equal = equal && (*p_rectCovar)(0, 2) == (*other.p_rectCovar)(0, 2);
      equal = equal && (*p_rectCovar)(1, 1) == (*other.p_rectCovar)(1, 1);
      equal = equal && (*p_rectCovar)(1, 2) == (*other.p_rectCovar)(1, 2);
      equal = equal && (*p_rectCovar)(2, 2) == (*other.p_rectCovar)(2, 2);
    }
    else if(equal) {
      equal = equal && p_rectCovar == NULL && other.p_rectCovar == NULL;
    }

    if(equal && p_sphereCovar) {
      equal = equal && (*p_sphereCovar)(0, 0) == (*other.p_sphereCovar)(0, 0);
      equal = equal && (*p_sphereCovar)(0, 1) == (*other.p_sphereCovar)(0, 1);
      equal = equal && (*p_sphereCovar)(0, 2) == (*other.p_sphereCovar)(0, 2);
      equal = equal && (*p_sphereCovar)(1, 1) == (*other.p_sphereCovar)(1, 1);
      equal = equal && (*p_sphereCovar)(1, 2) == (*other.p_sphereCovar)(1, 2);
      equal = equal && (*p_sphereCovar)(2, 2) == (*other.p_sphereCovar)(2, 2);
    }
    else if(equal) {
      equal = equal && p_sphereCovar == NULL && other.p_sphereCovar == NULL;
    }

    return equal;
  }

  SurfacePoint &SurfacePoint::operator=(const SurfacePoint &other) {
    // The lazy way of doing this (free all memory and copy) is too expensive
    // in the default case!
    if(p_x && other.p_x &&
       p_y && other.p_y &&
       p_z && other.p_z &&
       !p_rectCovar && !other.p_rectCovar &&
       !p_sphereCovar && !other.p_sphereCovar) {
      *p_x = *other.p_x;
      *p_y = *other.p_y;
      *p_z = *other.p_z;
    }
    else {
      FreeAllocatedMemory();

      if(other.p_x) {
        p_x = new Displacement(*other.p_x);
      }

      if(other.p_y) {
        p_y = new Displacement(*other.p_y);
      }

      if(other.p_z) {
        p_z = new Displacement(*other.p_z);
      }

      if(other.p_rectCovar) {
        p_rectCovar = new symmetric_matrix<double, upper>(*other.p_rectCovar);
      }

      if(other.p_sphereCovar) {
        p_sphereCovar = new symmetric_matrix<double, upper>(*other.p_sphereCovar);
      }
    }

    // Finally initialize local radius to avoid using a previous value
    p_localRadius = other.GetLocalRadius();

    return *this;
  }

  void SurfacePoint::FreeAllocatedMemory() {
    if(p_x) {
      delete p_x;
      p_x = NULL;
    }

    if(p_y) {
      delete p_y;
      p_y = NULL;
    }

    if(p_z) {
      delete p_z;
      p_z = NULL;
    }

    if(p_rectCovar) {
      delete p_rectCovar;
      p_rectCovar = NULL;
    }

    if(p_sphereCovar) {
      delete p_sphereCovar;
      p_sphereCovar = NULL;
    }
  }
}
