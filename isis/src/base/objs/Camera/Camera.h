#ifndef Camera_h
#define Camera_h
/** This is free and unencumbered software released into the public domain.
The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */

#include "Sensor.h"

#include <QList>
#include <QPointF>
#include <QString>

#include "AlphaCube.h"

namespace Isis {
  class Angle;
  class CameraDetectorMap;
  class CameraFocalPlaneMap;
  class CameraDistortionMap;
  class CameraGroundMap;
  class CameraSkyMap;
  class Distance;
  class Latitude;
  class Longitude;
  class Projection;
  class SurfacePoint;

  /**
   * @author ????-??-?? Jeff Anderson
   *
   * @ingroup SpiceInstrumentsAndCameras
   * @internal
   *   @todo Finish documentation.
   *
   *   @history 2005-11-09 Tracie Sucharski - Added HasProjection method.
   *   @history 2006-04-11 Tracie Sucharski - Added IgnoreProjection method and p_ignoreProjection
   *                           so that the Camera is used rather than Projection.
   *   @history 2006-04-19 Elizabeth Miller - Added SpacecraftRoll method
   *   @history 2006-06-08 Elizabeth Miller - Added static Distance method that calculates the
   *                           distance between 2 lat/lon pts (given the radius)
   *   @history 2006-07-25 Elizabeth Miller - Fixed bug in Distance method
   *   @history 2006-07-31 Elizabeth Miller - Added OffNadirAngle method and removed
   *                           SpacecraftRoll() method
   *   @history 2007-06-11 Debbie A. Cook - Added overloaded method SetUniversalGround that includes
   *                           a radius argument and method RawFocalPlanetoImage() to handle
   *                           the common functionality between the SetUniversalGround methods.
   *   @history 2008-01-28 Christopher Austin - Added error throw when minlon range isn't set beyond
   *                           initialization.
   *   @history 2008-02-15 Stacy Alley - In the GroundRangeResolution () method we had to subtract
   *                           0.5 when looking at the far left of pixels and add 0.5 to ensure we
   *                           are seeing the far right of pixels.
   *   @history 2008-05-21 Steven Lambright - Fixed boundary condition in the
   *                           GroundRangeResolution() method.
   *   @history 2008-06-18 Christopher Austin - Fixed documentation errors
   *   @history 2008-07-15 Steven Lambright - Added NaifStatus calls
   *   @history 2008-07-24 Steven Lambright - Fixed memory leaks: the alpha cube, distortion map,
   *                           focal plane map, sky map, detector map, and ground map were not being
   *                           deleted.
   *   @history 2008-08-08 Steven Lambright - Added the LoadCache() method which tries to find the
   *                           correct time range and calls Spice::CreateCache
   *   @history 2008-09-10 Steven Lambright - Added the geometric tiling methods in order to
   *                           optimize push frame cameras and prevent corruption of data when
   *                           running cam2map with push frame cameras
   *   @history 2008-11-13 Janet Barrett - Added the GroundAzimuth method. This method computes and
   *                           returns the ground azimuth between the ground point and another point
   *                           of interest, such as the subspacecraft point or the subsolar point.
   *                           The ground azimuth is the clockwise angle on the ground between a
   *                           line drawn from the ground point to the North pole of the body and a
   *                           line drawn from the ground point to the point of interest (such as
   *                           the subsolar point or the subspacecraft point).
   *   @history 2009-01-05 Steven Lambright - Added InCube method
   *   @history 2009-03-02 Steven Lambright - This class now keeps track of the current child band,
   *                           has more error checks, and now hopefully resets state when methods
   *                           like GroundRangeResolution are called.
   *   @history 2009-05-21 Steven Lambright - The geometric tiling hint can now be 2,2 as a special
   *                           case meaning no tiling will be used (the initial box will be a 2x2
   *                           box - only the 4 corners - is the idea behind using this value).
   *   @history 2009-05-22 Debbie A. Cook - Added Resolution method for Sensor (parent) virtual
   *   @history 2009-06-05 Mackenzie Boyd - Updated samson truthdata
   *   @history 2009-07-08 Janet Barrett - Added RadarGroundMap and RadarSlantRangeMap as friends to
   *                           this class so that they have access to the SetFocalLength() method.
   *                           The Radar instrument does not have a focal length and these classes
   *                           need to be able to change the focal length value each time the slant
   *                           range changes. This insures that the detector resolution always comes
   *                           out to the pixel width/height for the Radar instrument.
   *   @history 2009-07-09 Debbie A. Cook - Set p_hasIntersection in SetImage if successful instead
   *                           of just returning the bool to that other methods will know a ground
   *                           point was successfully set.
   *   @history 2009-08-03 Debbie A. Cook - Added computation of tolerance to support change in
   *                           Spice class for supporting downsizing of Spice tables
   *   @history 2009-08-14 Debbie A. Cook - Corrected alternate tolerance
   *   @history 2009-08-17 Debbie A. Cook - Added default tolerance for sky images
   *   @history 2009-08-19 Janet Barrett - Fixed the GroundAzimuth method so that it checks the
   *                           quadrant that the subspacecraft or subsolar point lies in to
   *                           calculate the correct azimuth value.
   *   @history 2009-08-28 Steven Lambright - Added GetCameraType method and returned enumeration
   *                           value
   *   @history 2009-09-23  Tracie Sucharski - Convert negative longitudes coming out of
   *                           NAIF's reclat() when computing azimuths.
   *   @history 2009-12-14  Steven Lambright - BasicMapping(...) will now populate the map Pvl
   *                           parameter with a valid Pvl
   *   @history 2010-03-19 Debbie A. Cook - Added members p_ckFrameId and p_ckReferenceId and
   *                           members SetCkFrameId(), SetCkReferenceId(), CkFrameId(), and
   *                           CkReferenceId() needed by the ckwriter application
   *   @history 2010-03-29 Debbie A. Cook - Modified SetCkFrameid and SetCkReferenceFrame to set new
   *                           bool value p_ckwriteReady to true if the kernels have values and
   *                           false if they don't instead of bombing.
   *   @history 2010-11-04 Steven Lambright - Added SetGround() methods with the SurfacePoint
   *                           version being commented out until the SurfacePoint class is available
   *   @history 2010-11-09 Eric Hyer - Added GetLocalNormal() and LocalPhotometricAngles() methods
   *   @history 2010-11-22 Janet Barrett - Added checks to make sure that the normal can be
   *                           determined for DEM surface. Also modified the LocalPhotometricAngles
   *                           method to report if the angles could successfully be calculated.
   *   @history 2011-01-14 Travis Addair - Added new CK/SPK accessor methods, pure virtual in
   *                           Camera, implemented in mission specific cameras.
   *   @history 2011-01-14 Kris Becker - Added the (pure) virtual methods CkFrameId(),
   *                           CkReferenceId(), SpkTargetId(), SpkCenterId() and SpkReferenceId().
   *                           Camera model developers must provide, at a minimum, implementations
   *                           for CkFrameId(), CkReferenceId() and SpkReferenceId().
   *                           A determination must be made if default implementations of
   *                           SpkTargetId() and SpkCenterId() are sufficient.  These methods are
   *                           required in order to write proper CKs and SPKs NAIF kernels for
   *                           instruments from updated pointing and position data; Removed
   *                           SetCkFrameId() and SetCkReferenceId() and their implementations as
   *                           the aforementioned routines replace them; also corrected a bug in the
   *                           computation of North, Sun and Spacecraft azimuths when a shape model
   *                           is present/active. The call to LocalRadius() should be
   *                           LocalRadius(lat,lon) in these cases. (Note at this time, those two
   *                           routines return meters and kilometers, respectively - we need to
   *                           address this inconsistancy!)
   *   @history 2011-01-25 Eric Hyer - SurfacePoint class now exists so uncommented Steven's
   *                           new SetGround() methods (see history for 2010-11-04)
   *   @history 2011-01-26  Steven Lambright - ComputeAzimuth now takes a Distance for the radius.
   *                           The LocalRadius() methods now return instances of the Distance class.
   *   @history 2011-02-10 Jeannie Walldren - Moved code from LoadCache() methods to create new
   *                           methods CacheSize() and StartEndEphemerisTime().  Removed unused
   *                           input parameter from LoadCache(). Set pointers to null in
   *                           constructor. Added documentation to methods, enum, and private
   *                           variables.
   *   @history 2011-02-09 Steven Lambright - Interfaces using Latitude, Longitude, and SurfacePoint
   *                           are now more efficient. Updated to work with changes to the parent.
   *                           Now uses more abstraction internally also. These changes were for
   *                           readability (you have more explicit units and less of a change to
   *                           misuse them).
   *   @history 2011-02-11 Steven Lambright - Moved Distance() method to SurfacePoint
   *   @history 2011-02-18 Steven Lambright - Fixed a problem where using a planetographic mapping
   *                           group in GroundRange would still output planetocentric latitudes.
   *   @history 2011-05-03 Jeannie Walldren - Added Isis Disclaimer to files.
   *   @history 2012-01-19 Janet Barrett - Fixes issue #625 - Fixed a problem with the ground
   *                           azimuth calculation in the GroundAzimuth method. The algorithm being
   *                           used was the Napier's analogies and was not providing the correct
   *                           azimuth value. The Napier's analogies also have the weakness of
   *                           producing a divide by zero error. The new algorithm being used is the
   *                           spherical law of cosines for sides which comes out of the
   *                           "CRC Standard Mathematical Tables and Formulae 30th Edition on page
   *                           469.
   *   @history 2012-06-01 Janet Barrett - Modified the GroundAzimuth method to detect which
   *                           hemisphere the ground point is in so that the closer pole will be
   *                           used to obtain the azimuth value. If the closer pole is the South
   *                           pole, then the azimuth is adjusted to be relative to the North pole.
   *   @history 2012-06-04 Janet Barrett - Got rid of redundant calls to Sample(), Line(), and
   *                           SetImage() in the ComputeAzimuth method.
   *   @history 2012-07-06 Debbie A. Cook - Updated Spice members to be more compliant with Isis
   *                           coding standards. References #972.
   *   @history 2012-10-12 Debbie A. Cook - Updated to use new Target and ShapeModel classes.
   *                           References Mantis ticket #775 and #1114.
   *   @history 2012-10-25 Jeannie Backer - Changed resolution() method to lower
   *                           camel case. References #1181.
   *   @history 2012-11-20 Janet Barrett - Fixed a problem with the GroundAzimuth method so
   *                           that the azimuth value is calculated correctly for radar data.
   *                           Fixes #1117.
   *   @history 2012-04-24 Jeannie Backer - Added a programmer error if NorthAzimuth() is called on
   *                           a plane projection since this value is meaningless for ring plane
   *                           projections. References #775.
   *   @history 2012-12-20 Debbie A. Cook - Changed to use TProjection and RingPlaneProjection
   *                           instead of Projection as needed.  References #775.
   *   @history 2013-03-04 Tracie Sucharski - Added new method, PixelIfovOffsets, which will return
   *                           the ifov offsets,in x and y, from the center of the pixel in mm.  The
   *                           default camera offsets will be half of the pixel pitch in both the x
   *                           and y directions.  References #1604.
   *   @history 2014-04-17 Jeannie Backer - Modified ComputeAzimuth() to return an Isis::Null if the
   *                           method fails (instead of -1.0). Add a check in ComputeAzimuth() to
   *                           make sure the "SetUniversalGround()" call succeeds, if not, reset to
   *                           the original sample/line and return Null. References #1659.
   *   @history 2015-03-14 Jeannie Backer - Modified GetLocalNormal() to call non-DEM shapes'
   *                           calculateLocalNormal() with an empty vector of neighbors since only
   *                           DemShape uses this vector. References #2035.
   *   @history 2015-04-30 Jeannie Backer - Fixed bug in GetLocalNormal() introduced in prior
   *                           check-in. Fixes #2243.
   *   @history 2015-05-15 Kris Becker - Set ShapeModel state in SetImage() to reflect no
   *                           intersection before attempting to map to surface. Fixes #2252.
   *   @history 2015-09-01 Ian Humphrey and Makayla Shepherd - Modified unit test to override
   *                           Sensor's pure virtual methods.
   *   @history 2015-10-16 Ian Humphrey - Added protected members for spacecraft and instrument
   *                           names as well as public member getters. Updated unit test.
   *                           References #2335.
   *   @history 2016-06-27 Kelvin Rodriguez - Added member function to compute celestial north
   *                           clock angle. References #2365
   *   @history 2016-08-01 Curtis Rose - Changed return values of resolutions from -1 to Isis::Null.
   *                           Fixes #2065.
   *   @history 2016-08-16 Tyler Wilson - Added ObliqueDectectorResolution,ObliqueLineResolution,
   *                           ObliqueSampleResolution, and ObliquePixelResolution functions
   *                           which give greatly improved approximations compared to their
   *                           non-oblique counterpart functions when the Look vector is pointing
   *                           off nadir and near the limb.  Fixes #476.  References #4100.
   *   @history 2016-10-16 Jesse Mapel - Added exposureDuration methods to access pixel exposure
   *                           durations.  References #4476.
   *   @history 2016-10-18 Kristin Berry - Added a new virtual SetImage which includes a time
   *                           offset. Refactored SetImage to minimize repeated code by creating
   *                           helper functions SetImageSkyMapProjection() and
   *                           SetImageMapProjection(). Updated unitTest. References #4476.
   *   @history 2016-10-21 Jesse Mapel - Modified exposureDuration(sample, line, band) to default
   *                           to using the band the camera is set to. References #4476.
   *   @history 2016-10-21 Jeannie Backer - Reorder method signatures and member variable
   *                           declarations to fit ISIS coding standards. References #4476.
   *   @history 2016-12-02 Victor Silva - Made changes to GetLocalNormal to calculate local normal
   *                           accurately for LRO by changing 4 corner surrounding points from adding
   *                           0.5 to adding 0.5 - DBL_MIN. Fixes #4560.
   *   @history 2017-03-03 Tyler Wilson - Updated the oblique detector resolution function to correct
   *                           an error in the original formula, and updated the documention for this
   *                           function.  Fixes #4614.
   *   @history 2017-08-30 Summer Stapleton - Updated documentation. References #4807.
   *   @history 2017-01-11 Christopher Combs - Added bool deleteExisting to SetDistortionMap to
   *                           prevent a segfault when the distortion map is incomplete. Fixes $5163.
   *   @history 2018-07-12 Summer Stapleton - Added m_instrumentId and instrumentId() in order to
   *                           collect the InstrumentId from the original cube label for
   *                           comparisons related to image imports in ipce. References #5460.
   *   @history 2021-03-04 Victor Silva - Made changes to GetLocalNormal to calculate local normal
   *                           accurately for LRO by changing 4 corner surrounding points from adding
   *                           0.5 to line and sample and wrapping value with nexttoward.Fixes #4018.
   */

  class Camera : public Sensor {
    public:
      // constructors 
      Camera(Cube &cube);

      // destructor
      //! Destroys the Camera Object
      virtual ~Camera();

      // Methods
      virtual bool SetImage(const double sample, const double line);
      virtual bool SetImage(const double sample, const double line, const double deltaT);

      virtual bool SetUniversalGround(const double latitude, const double longitude);
      virtual bool SetUniversalGround(const double latitude, const double longitude,
                                      const double radius);
      virtual bool SetGround(Latitude latitude, Longitude longitude);
      virtual bool SetGround(const SurfacePoint & surfacePt);
      bool SetRightAscensionDeclination(const double ra, const double dec);

      void LocalPhotometricAngles(Angle & phase, Angle & incidence,
                                  Angle & emission, bool &success);
      void Slope(double &slope, bool &success);

      void GetLocalNormal(double normal[3]);

      bool HasProjection();
      virtual bool IsBandIndependent();
      int ReferenceBand() const;
      bool HasReferenceBand() const;
      virtual void SetBand(const int band);
      virtual double Sample() const;
      int Band() const;
      virtual double Line() const;

      bool GroundRange(double &minlat, double &maxlat, double &minlon,
                       double &maxlon, Pvl &pvl);
      bool ringRange(double &minRingRadius, double &maxRingRadius,
                     double &minRingLongitude, double &maxRingLongitude, Pvl &pvl);
      bool IntersectsLongitudeDomain(Pvl &pvl);

      virtual double PixelResolution();
      virtual double LineResolution();
      virtual double SampleResolution();
      virtual double DetectorResolution();

      virtual double ObliqueDetectorResolution(bool useLocal = true);
      virtual double ObliqueSampleResolution();
      virtual double ObliqueLineResolution();
      virtual double ObliquePixelResolution();


      virtual double resolution();
      double LowestImageResolution();
      double HighestImageResolution();
      double LowestObliqueImageResolution();
      double HighestObliqueImageResolution();

      void BasicMapping(Pvl &map);
      void basicRingMapping(Pvl &map);

      double FocalLength() const;
      double PixelPitch() const;
      virtual double exposureDuration() const;
      virtual double exposureDuration(const double sample,
                                      const double line,
                                      const int band = -1) const;
      virtual QList<QPointF> PixelIfovOffsets();

      int Samples() const;
      int Lines() const;
      int Bands() const;
      int ParentLines() const;
      int ParentSamples() const;

      double CelestialNorthClockAngle();

      bool RaDecRange(double &minra, double &maxra,
                      double &mindec, double &maxdec);
      double RaDecResolution();

      CameraDistortionMap *DistortionMap();
      CameraFocalPlaneMap *FocalPlaneMap();
      CameraDetectorMap *DetectorMap();
      CameraGroundMap *GroundMap();
      CameraSkyMap *SkyMap();

      QString instrumentId();

      QString instrumentNameLong() const;
      QString instrumentNameShort() const;
      QString spacecraftNameLong() const;
      QString spacecraftNameShort() const;

      void SetDistortionMap(CameraDistortionMap *map, bool deleteExisting = true);
      void SetFocalPlaneMap(CameraFocalPlaneMap *map);
      void SetDetectorMap(CameraDetectorMap *map);
      void SetGroundMap(CameraGroundMap *map);
      void SetSkyMap(CameraSkyMap *map);

      double NorthAzimuth();
      double SunAzimuth();
      double SpacecraftAzimuth();
      double OffNadirAngle();

      static double GroundAzimuth(double glat, double glon, double slat,
                                  double slon);

      void IgnoreProjection(bool ignore);

      void LoadCache();
      std::pair< double, double > StartEndEphemerisTimes();
      int CacheSize(double startTime, double endTime);


      void GetGeometricTilingHint(int &startSize, int &endSize);

      bool InCube();

      /**
       * This enum defines the types of cameras supported in this class
       */
      enum CameraType {
        Framing,        //!< Framing Camera
        PushFrame,      //!< Push Frame Camera
        LineScan,       //!< Line Scan Camera
        Radar,          //!< Radar Camera
        Point,          //!< Point Camera
        RollingShutter, //!< RollingShutter
        Csm             //!< Community Sensor Model Camera
      };

      /**
       * Returns the type of camera that was created.  This is a pure virtual method, so all child
       * classes must define and identify themselves as one of the enumerated camera types defined
       * in this class.
       *
       * @return @b CameraType Type of camera that was created.
       */
      virtual CameraType GetCameraType() const = 0;

      /**
       * @brief Provides the NAIF frame code for an instruments CK kernel
       *
       * This pure virtual method must be implemented in each camera model providing the reference
       * frame NAIF ID code found in the mission CK kernel.
       *
       * This value can be easily determined by using the NAIF @b spacit application that sumarizes
       * binary CK kernels a particular instrument on a spacecraft. @b spacit will additionally
       * require a spacecraft clock kernel (SCLK) and a leap seconds kernel (LSK). For example,
       * the output of the MESSENGER camera CK supporting the MDIS camera below indicates it is the
       * MESSENGER spacecraft.
       *
       *
       * @code
       *   Segment ID     : MSGR_SPACECRAFT
       *   Instrument Code: -236000
       *   Spacecraft     : Body -236, MESSENGER
       *   Reference Frame: Frame 1, J2000
       *   CK Data Type   : Type 3
       *   Description : Continuous Pointing: Linear Interpolation
       *   Available Data : Pointing and Angular Velocity
       *   UTC Start Time : 2004 AUG 12 17:17:42.558
       *   UTC Stop Time  : 2010 JUL 23 12:35:22.814
       *   SCLK Start Time: 1/000818300:000000
       *   SCLK Stop Time : 1/188375996:000000
       * @endcode
       *
       * The CkFrameId value is found in the "Instrument Code" entry (-236000).
       *
       * @return @b int NAIF code for CK frame for an instrument
       */
      virtual int CkFrameId() const = 0;

      /**
       * @brief Provides the NAIF reference code for an instruments CK kernel
       *
       * This virtual method must be implemented in each camera model providing the reference frame
       * NAIF ID code found in the mission CK kernel.
       *
       * This value can be easily determined by using the NAIF @b spacit application that sumarizes
       * binary CK kernels a particular instrument on a spacecraft.  @b spacit will additionally
       * require a spacecraft clock kernel (SCLK) and a leap seconds kernel (LSK).For example,
       * the output of the MESSENGER camera CK supporting the MDIS camera below indicates it is the
       * MESSENGER spacecraft.
       *
       *
       * @code
       *   Segment ID     : MSGR_SPACECRAFT
       *   Instrument Code: -236000
       *   Spacecraft     : Body -236, MESSENGER
       *   Reference Frame: Frame 1, J2000
       *   CK Data Type   : Type 3
       *   Description : Continuous Pointing: Linear Interpolation
       *   Available Data : Pointing and Angular Velocity
       *   UTC Start Time : 2004 AUG 12 17:17:42.558
       *   UTC Stop Time  : 2010 JUL 23 12:35:22.814
       *   SCLK Start Time: 1/000818300:000000
       *   SCLK Stop Time : 1/188375996:000000
       * @endcode
       *
       * The CkReferenced value is found in the "Reference Frame" entry (1).
       *
       * @return @b int NAIF code for CK reference for an instrument
       */
      virtual int CkReferenceId() const = 0;

      /**
       * @brief Provides reference frame for instruments SPK NAIF kernel
       *
       * This pure virtual method must be implemented in each camera model providing the reference
       * frame NAIF ID code found in the mission SPK kernel.  This is typically J2000, but may be
       * relative to other frames.
       *
       * This value can be easily determined by using the NAIF @b spacit application that sumarizes
       * binary kernels on the SPK kernel used for a particular instrument on a spacecraft.
       * @b spacit will additionally require a leap seconds kernel (LSK).  For example, the output
       * of the MESSENGER SPK camera supporting the MDIS camera below indicates it is indeed the
       * J2000 reference frame:
       *
       * @code
       *     Segment ID     : msgr_20050903_20061125_recon002.nio
       *     Target Body    : Body -236, MESSENGER
       *     Center Body    : Body 2, VENUS BARYCENTER
       *     Reference frame: Frame 1, J2000
       *     SPK Data Type  : Type 1
       *     Description : Modified Difference Array
       *     UTC Start Time : 2006 OCT 16 19:25:41.111
       *     UTC Stop Time  : 2006 OCT 31 22:14:24.040
       *     ET Start Time  : 2006 OCT 16 19:26:46.293
       *     ET Stop time   : 2006 OCT 31 22:15:29.222
       * @endcode
       *
       * The SpkReferenceId value is found in the "Reference frame" entry (1).
       *
       * @return @b int NAIF code for SPK reference frame for an instrument
       */
      virtual int SpkReferenceId() const = 0;
      virtual int SpkTargetId() const;
      virtual int SpkCenterId() const;


    protected:

      void SetFocalLength(double v);
      void SetPixelPitch(double v);

      void SetFocalLength();
      void SetPixelPitch();

      void SetGeometricTilingHint(int startSize = 128, int endSize = 8);

      // These 2 classes need to be friends of the Camera class because
      // of the way Radar works - there is no set focal length for the
      // instrument, so the focal length needs to be set each time the
      // slant range changes.
      friend class RadarGroundMap;      //!< A friend class to calculate focal length
      friend class RadarSlantRangeMap;  //!< A friend class to calculate focal length

      QString m_instrumentId;        //!< The InstrumentId as it appears on the cube.

      QString m_instrumentNameLong;  //!< Full instrument name
      QString m_instrumentNameShort; //!< Shortened instrument name
      QString m_spacecraftNameLong;  //!< Full spacecraft name
      QString m_spacecraftNameShort; //!< Shortened spacecraft name

      double p_childSample;                  //!< Sample value for child
      double p_childLine;                    //!< Line value for child
      AlphaCube *p_alphaCube;                //!< A pointer to the AlphaCube

      bool p_pointComputed;                  //!< Flag showing if Sample/Line has been computed

    private:
      void GroundRangeResolution();
      void ringRangeResolution();
      double ComputeAzimuth(const double lat, const double lon);
      bool RawFocalPlanetoImage();
      // SetImage helper functions:
      // bool SetImageNoProjection(const double sample, const double line);
      bool SetImageMapProjection(const double sample, const double line, ShapeModel *shape);
      bool SetImageSkyMapProjection(const double sample, const double line, ShapeModel *shape);


      double p_focalLength;                  //!< The focal length, in units of millimeters
      double p_pixelPitch;                   //!< The pixel pitch, in millimeters per pixel

      double p_minlat;                       //!< The minimum latitude
      double p_maxlat;                       //!< The maximum latitude
      double p_minlon;                       //!< The minimum longitude
      double p_maxlon;                       //!< The maximum longitude
      double p_minres;                       //!< The minimum resolution
      double p_maxres;                       //!< The maximum resolution
      double p_minobliqueres;                //!< The minimum oblique resolution
      double p_maxobliqueres;                //!< The maximum oblique resolution
      double p_minlon180;                    //!< The minimum longitude in the 180 domain
      double p_maxlon180;                    //!< The maximum longitude in the 180 domain
      /** Flag showing if ground range was computed successfully.*/
      bool p_groundRangeComputed;


      int p_samples;                         //!< The number of samples in the image
      int p_lines;                           //!< The number of lines in the image
      int p_bands;                           //!< The number of bands in the image

      int p_referenceBand;                   //!< The reference band

      Projection *p_projection;              //!< A pointer to the Projection
      bool p_ignoreProjection;               //!< Whether or no to ignore the Projection

      double p_mindec;                       //!< The minimum declination
      double p_maxdec;                       //!< The maximum declination
      double p_minra;                        //!< The minimum right ascension
      double p_maxra;                        //!< The maxumum right ascension
      double p_minra180;                     //!< The minimum right ascension in the 180 domain
      double p_maxra180;                     //!< The maximum right ascension in the 180 domain
      /** Flag showing if the raDec range has been computed successfully.*/
      bool p_raDecRangeComputed;

      double p_minRingRadius;                //!< The minimum ring radius
      double p_maxRingRadius;                //!< The maximum ring radius
      double p_minRingLongitude;             //!< The minimum ring longitude (azimuth)
      double p_maxRingLongitude;             //!< The maximum ring longitude (azimuth)
      double p_minRingLongitude180;          //!< The minimum ring longitude in the 180 domain
      double p_maxRingLongitude180;          //!< The maximum ring longitude in the 180 domain
      /** Flag showing if ring range was computed successfully.*/
      bool p_ringRangeComputed;


      int p_childBand;                       //!< Band value for child
      CameraDistortionMap *p_distortionMap;  //!< A pointer to the DistortionMap
      CameraFocalPlaneMap *p_focalPlaneMap;  //!< A pointer to the FocalPlaneMap
      CameraDetectorMap *p_detectorMap;      //!< A pointer to the DetectorMap
      CameraGroundMap *p_groundMap;          //!< A pointer to the GroundMap
      CameraSkyMap *p_skyMap;                //!< A pointer to the SkyMap

      /** The ideal geometric tile size to start with when projecting*/
      int p_geometricTilingStartSize;
      /** The ideal geometric tile size to end with when projecting*/
      int p_geometricTilingEndSize;

  };
};

#endif
