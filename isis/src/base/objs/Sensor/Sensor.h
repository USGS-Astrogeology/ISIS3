#ifndef Sensor_h
#define Sensor_h
/** This is free and unencumbered software released into the public domain.
The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */
#include "Spice.h"

#include <QList>
#include <QPointF>
#include <QString>
#include <QVariant>

#include "Cube.h"
#include "Interpolator.h"
#include "Portal.h"
#include "ProjectionFactory.h"

namespace Isis {
  class Distance;
  class EllipsoidShape;
  class iTime;
  class Latitude;
  class Longitude;
  class ShapeModel;
  class SurfacePoint;
  class Target;

  /**
   * @brief Class for computing sensor ground coordinates
   *
   * The sensor class allows for the computation of parameters related to orbiting instruments.
   * In particular, a time and look direction can be set and from those the ground coordinate
   * (latitude/longitude) along with phase, incidence, and emission angles can be computed.
   * Likewise, a ground point can be set and look direction can be computed. This class is derived
   * from the Spice class.
   *
   * An important capability of this class is the ability to use a surface model other than an
   * ellipsoid when intersecting the look direction of the sensor with the planetary body. This
   * allows for the generation of othrorectified products. The file containing the surface model
   * is a cube and is obtained from the labels in the follow form:
   *   @code
   *     Group = Kernels
   *       ElevationModel = file.cub
   *     EndGroup
   *   @endcode
   *
   * @ingroup SpiceInstrumentsAndCameras
   *
   * @author 2003-04-11 Jeff Anderson
   *
   * @internal
   *   @history 2003-05-16 Stuart Sides -  Modified schema from astrogeology...
   *                           isis.astrogeology...
   *   @history 2003-05-30 Jeff Anderson - Updated unitTest and truth to account for precision due
   *                           to optimization
   *   @history 2003-10-16 Jeff Anderson - Added LoadEulerMounting() and LoadFrameMounting()
   *                           methods
   *   @history 2003-11-26 Jeff Anderson - Modified FrameMounting() methods to allow for fixed
   *                           frames or time dependent frames
   *   @history 2004-01-14 Jeff Anderson - Remove an unused constructor
   *   @history 2004-01-20 Jeff Anderson - Added an option to the SetUniversalGround() method to
   *                           eliminate checks for points on the backside of the target
   *   @history 2004-02-18 Jeff Anderson - Fixed a problem with the FrameMounting() methods as the
   *                           frame kernel is unloaded if a cache is created.
   *   @history 2004-02-23 Jeff Anderson - Fixed two bugs in the handling of DEMs. Used universal
   *                           lat/lon when appropriate and the radius needed to be converted
   *                           to km when read from the DEM file.
   *   @history 2005-02-15 Elizabeth Ribelin - Modified file to support Doxygen documentation
   *   @history 2005-02-24 Jeff Anderson - Added SlantDistance() method and made the constructor
   *                           ignore the ElevationModel keyword if it is null
   *   @history 2005-02-25 Jeff Anderson - Added LocalSolarTime() method
   *   @history 2005-06-09 Elizabeth Ribelin - Added LoadEulerMounting() method that accepts a
   *                           matrix as a parameter
   *   @history 2005-08-24 Jeff Anderson - Make sure LocalSolarTime always return positive hours
   *   @history 2005-09-20 Jeff Anderson - Added tests for trying to intersect the sky
   *   @history 2006-03-31 Elizabeth Miller - Added SpacecraftAltitude() and SolarDistance() methods
   *   @history 2006-09-07 Debbie A. Cook - Changed back-of-planet test to use emission angle
   *                           instead of length of vector to surface point
   *   @history 2006-10-11 Jeff Anderson - Modified reading radius from DEM using bilinear
   *                           interpolation
   *   @history 2006-10-12 Jeff Anderson - Modified the SetLookDirection() method to use the DEM (if
   *                           available) when computing a ground point
   *   @history 2007-05-18 Jeff Anderson - Modify SpacecraftAltitude() method to use DEM
   *   @history 2007-06-11 Debbie A. Cook - Added alternative method that includes radius
   *   @history 2007-08-24 Debbie A. Cook - Replaced references to m_sB since it was removed from
   *                           Spice
   *   @history 2007-11-27 Debbie A. Cook - Added overloaded method
   *                           SetUniversalGround(lat, lon, radius)
   *   @history 2008-05-21 Steven Lambright - CubeManager is now used to speed up DEM Cube I/O
   *   @history 2008-06-18 Debbie A. Cook - Made DemRadius radius public instead of private and
   *                           added method HasElevationModel()
   *   @history 2008-08-06 Stuart Sides   - Modified SetLookDirection() to better handle oblique
   *                           views. In the past it would oscillate and run out of iterations.
   *   @history 2009-02-06 Debbie A. Cook - Changed the tolerance from 1e-6 to 1e-12
   *                           for dist**2 (mm)
   *   @history 2009-02-06 Debbie A. Cook - Changed the tolerance back to 1e-6 (mm)
   *   @history 2009-02-15 Debbie A. Cook - Added virtual Resolution() method
   *   @history 2009-06-30 Steven Lambright - Added IgnoreElevationModel() and fixed DemRadius()
   *   @history 2009-07-09 Debbie A. Cook - Corrected documentation on Resolution() method
   *   @history 2009-09-23 Tracie Sucharski - Convert negative longitudes returned by reclat in
   *                           SetLookDirection().
   *   @history 2010-09-15 Janet Barrett - Modified the SetLookDirection() method to use a new
   *                           algorithm for finding the intersection of a ray with the DEM. This
   *                           was required to take care of problems that were encountered at the
   *                           poles of global DEM files. The algorithm that is being used was
   *                           taken from "Intersection between spacecraft viewing vectors and
   *                           digital elevation models" by N.A. Teanby.
   *   @history 2010-11-09 Eric Hyer - moved some private members to protected section for Camera
   *   @history 2010-12-07 Steven Lambright - Added LocalRadius(lat,lon) and removed a sqrt
   *                           from SetLookDirection()
   *   @history 2011-01-26 Steven Lambright - The LocalRadius methods now return Distance objects
   *                           and added a LocalRadius() method that uses the Latitude and
   *                           Longitude classes.
   *   @history 2011-02-08 Jeannie Walldren - Added method parameter documentation.
   *   @history 2011-02-09 Steven Lambright & Debbie Cook - Refactored heavily to use Latitude,
   *                           Longitude, SurfacePoint, and iTime where applicable. Optimized
   *                           SetLookDirection. These changes were meant primarily for readability
   *                           and reducing error-proneness of the code.
   *   @history 2011-03-28 Janet Barrett - Fixed the SetLookDirection routine so that it checks to
   *                           make sure that the projection of the DEM is in an Equatorial
   *                           cylindrical projection before using the new ray tracing routine.
   *                           The check for this was moved when other code was added. This was
   *                           causing a problem with footprintinit.
   *   @history 2011-05-03 Jeannie Walldren - Added Isis Disclaimer to files.
   *   @history 2011-05-25 Janet Barrett and Steven Lambright - Moved the ownership of the DEM
   *                           projection to the DEM cube which is owned by CubeManager.
   *   @history 2011-08-22 Steven Lambright - The DEM cube now uses a UniqueIOCachingAlgorithm(5).
   *                           This will probably behave better in all cases. Fixes #364
   *   @history 2011-08-16 Jeff Anderson - Fixed a problem with an infinite loop in the ray tracing
   *                           algorithm.  The problem was first exposed when trying to intersect
   *                           the Vesta DEM on the limb.
   *   @history 2011-12-20 Tracie Sucharski - Added  SpacecraftSurfaceVector method which returns
   *                           the vector between the spacecraft and the surface point in
   *                           body-fixed.
   *   @history 2012-05-04 Steven Lambright - Re-enabled a safety check in the DemRadius() method
   *                           which was needed due to Projection not uniformly handling Null
   *                           inputs. Fixes #807.
   *   @history 2012-07-06 Debbie A. Cook - Updated Spice members to be more compliant with Isis
   *                           coding standards. References #972.
   *   @history 2012-09-06 Steven Lambright and Stuart Sides - Changed the constructors to take
   *                           Cube instead of Pvl to prevent redundant parsing. This
   *                           should eventually be refactored into a CubeLabel or similar object
   *                           so that an actual cube isn't required in the future, but for now
   *                           this enables the control net GUI to create a camera from a cube
   *                           with no dimensions in the label.
   *   @history 2012-10-10 Debbie A. Cook - Moved the functionality related to the shape model into
   *                           new classes:  ShapeModel, EllipsoidShape, DemShape, and
   *                           EquatorialCylindricalShape.  Also modified to use new Target class.
   *                           References #775 and #1114
   *   @history 2012-10-25 Jeannie Backer - Changed resolution() method to lower
   *                           camel case. References #1181.
   *   @history 2012-03-04 Tracie Sucharski - Added new method, PixelIfovOffsets, which will return
   *                           the ifov offsets from the center of the pixel in mm.  This is a
   *                           virtual method and if not implemented in the specific instrument
   *                           camera, this class will throw an error.  References #1604.
   *   @history 2015-08-11 Ian Humphrey and Makayla Shepherd - Added pure virtual methods to return
   *                           the long and short spacecraft and instrument names.
   *   @history 2015-10-01 Jeannie Backer - Made improvements to documentation and brought code
   *                           closer to ISIS coding standards. References #1438
   *   @history 2017-03-23 Kris Becker - Added support for occlusion checks. Modified
   *                            SetLocalGround(bool backCheck) to make a callback to the
   *                            ShapeModel::isOccludedFrom() to test for point
   *                            visability.
   *   @history 2021-02-17 Kristin Berry, Jesse Mapel, and Stuart Sides - Made several functions
   *                           virtual and moved look vector member variable to protected. Ensured
   *                           that m_newLookB always initializes to the same value.
   */
  class Sensor : public Spice {
    public:
      Sensor(Cube &cube);

      virtual ~Sensor();

      void setTime(const iTime &time);
      bool SetLookDirection(const double v[3]);
      bool SetRightAscensionDeclination(const double ra, const double dec);
      bool SetUniversalGround(const double latitude, const double longitude,
                              bool backCheck = true);
      bool SetUniversalGround(const double latitude, const double longitude,
                              const double radius, bool backCheck = true);
      bool SetGround(const SurfacePoint &surfacePt, bool backCheck = true);
      bool HasSurfaceIntersection() const;
      void Coordinate(double p[3]) const;

      virtual double UniversalLatitude() const;
      Latitude GetLatitude() const;
      virtual double UniversalLongitude() const;
      Longitude GetLongitude() const;
      virtual SurfacePoint GetSurfacePoint() const;

      Distance LocalRadius() const;
      Distance LocalRadius(Latitude lat, Longitude lon);
      Distance LocalRadius(double lat, double lon);

      virtual double PhaseAngle() const;
      virtual double EmissionAngle() const;
      virtual double IncidenceAngle() const;

      void LookDirection(double v[3]) const;
      virtual std::vector<double> lookDirectionJ2000() const;
      virtual std::vector<double> lookDirectionBodyFixed() const;

      virtual double RightAscension();
      virtual double Declination();

      // Return vector between spacecraft and surface point in body-fixed
      void SpacecraftSurfaceVector(double scSurfaceVector[3]) const;
      virtual double SlantDistance() const;
      double LocalSolarTime();
      virtual double SolarDistance() const;
      double SpacecraftAltitude();

      /**
       * Virtual method that returns the pixel resolution of the sensor in
       * meters/pix.
       *
       * @return @b double Resolution value of 1.0
       */
      virtual double resolution() {
        return 1.0;
      };
      void IgnoreElevationModel(bool ignore);

      virtual QList<QPointF> PixelIfovOffsets();

      virtual QString instrumentNameLong() const = 0;
      virtual QString instrumentNameShort() const = 0;
      virtual QString spacecraftNameLong() const = 0;
      virtual QString spacecraftNameShort() const = 0;

    protected:
      SpiceDouble m_lookB[3];  //!< Look direction in body fixed
      bool m_newLookB;      //!< flag to indicate we need to recompute ra/dec

    private:
      void CommonInitialize(const std::string &demCube);


      SpiceDouble m_ra;     //!< Right ascension (sky longitude)
      SpiceDouble m_dec;    //!< Decliation (sky latitude)
      void computeRaDec();  //!< Computes the ra/dec from the look direction
      bool SetGroundLocal(bool backCheck);   //!< Computes look vector
  };
};

#endif
