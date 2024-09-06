#ifndef Projection_h
#define Projection_h
/** This is free and unencumbered software released into the public domain.
The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */
#include <string>
#include <vector>
#include <QString>

#include "PvlGroup.h" // protected data member object (m_mappingGrp)

namespace Isis {
  class Displacement;
  class Pvl;
  class WorldMapper;
  /**
   *
   *
   * @brief Base class for Map Projections
   *
   * This is a virtual base class for map projections. It must be used to create
   * specific map projection classes such as Sinusoidal, Mercator, etc. The
   * foundation of this class is the ability to convert ground coordinates
   * (latitude and longitude) into projection coordinates (x and y) and vice
   * versa. Options exist to allow conversion to and from programmer specified
   * world coordinates. The world coordinates can be cube pixels, paper units in
   * millimeters, or any other unit the program may need. Generally, you should
   * never directly instantiate this class.
   *
   * Here is an example of how to use Projection
   * @code
   *   Pvl lab;
   *   lab.Read("projection.map");
   *   Projection *p = ProjectionFactory::Create(lab);
   * @endcode
   * If you would like to see Projection being used in implementation,
   * see mappos.cpp
   * @ingroup MapProjection
   * @author 2003-01-29 Jeff Anderson
   * @internal
   *   @history 2003-01-30 Jeff Anderson - Add the SetWorldMapper() method and
   *                           removed setting the mapper from the constructor
   *   @history 2003-05-16 Stuart Sides - Modified schema from astrogeology...
   *                           isis.astrogeology...
   *   @history 2003-05-30 Jeff Anderson - Updated unit test to fix error for
   *                           optimization
   *   @history 2003-08-25 Jeff Anderson - Added LatitudeTypeString(),
   *                           LongitudeDirectionString(), and
   *                           LongitudeDomainString() methods
   *   @history 2003-09-26 Jeff Anderson - Added ToWorldX(), ToWorldY(),
   *                           ToProjectionX(), ToProjectionY(), and
   *                           Resolution() methods
   *   @history 2003-09-26 Jeff Anderson - Added virtual Name, operator==, and
   *                           operator!= methods
   *   @history 2003-09-26 Jeff Anderson - Remove virtual from operator!=
   *   @history 2003-10-14 Jeff Anderson - Added Scale() and TrueScaleLatitude()
   *                           methods
   *   @history 2003-11-04 Jeff Anderson - Replace the pure virtual methods for
   *                           SetGround() and SetCoordinate() with virtual
   *                           method which simply copy lat/lon to x/y and vice
   *                           versa. This is essentially no projection.
   *   @history 2003-11-04 Jeff Anderson - Added LocalRadius() methods
   *   @history 2004-02-23 Jeff Anderson - Added Eccentricity(), tCompute(),
   *                           mCompute(), e4Compute(), and phi2Compute()
   *                           methods
   *   @history 2004-02-24 Jeff Anderson - Fixed bug in eccentricity computation
   *   @history 2005-02-11 Elizabeth Ribelin - Modified file to support Doxygen
   *                           documentation
   *   @history 2005-02-25 Elizabeth Ribelin - Added 3 static conversion
   *                           methods: ToHours(), ToDMS(), and  ToHMS()
   *   @history 2005-03-08 Elizabeth Ribelin - Fixed unitTest
   *   @history 2005-06-24 Jeff Anderson - Tweaked format for DMS and HMS
   *                           methods
   *   @history 2005-07-22 Jeff Anderson - Added static ToPlanetographic()
   *                           method
   *   @history 2006-04-20 Tracie Sucharski - Fixed bug looking at Target
   *                           instead of TargetName for sky.
   *   @history 2006-06-20 Stuart Sides - Added ability to lookup the radii if
   *                           given a "TargetName" in the mapping group.
   *   @history 2006-07-10 Elizabeth Miller - Fixed bugs in new static method
   *   @history 2007-06-27 Steven Lambright - Added static ToPlanetocentric(),
   *                           ToPositiveWest() and ToPositiveEast() methods.
   *                           Changed To180Domain() and To360Domain() to be
   *                           static instead of const. Added Mapping(),
   *                           MappingLatitudes() and MappingLongitudes()
   *                           methods for map2map.
   *   @history 2007-08-14 Steven Lambright - Mapping() fixed: should not return
   *                           cube specific parameters, because they did not go
   *                           into making the projection.
   *   @history 2008-05-09 Steven Lambright - Added Name(), Version(),
   *                           IsEquatorialCylindrical() methods
   *   @history 2008-06-12 Christopher Austin - Elaborated error messages.
   *   @history 2008-06-19 Steven Lambright - Fixed memory leak
   *   @history 2009-01-29 Stacy Alley - added a overloaded STATIC
   *                           method for convenience.  TargetRadii, which takes
   *                           a Pvl, the cube label, and a PvlGroup, a mapping
   *                           group.
   *   @history 2011-02-10 Jai Rideout - added SetUpperLeftCorner() because
   *                           ProjectionFactory needed a way to set
   *                           UpperLeftCornerX and UpperLeftCornerY keywords
   *                           after creating the projection. Mapping() now adds
   *                           optional keywords UpperLeftCornerX,
   *                           UpperLeftCornerY, PixelResolution, and Scale to
   *                           original mapping group so that cam2map can
   *                           properly display a clean mapping group.
   *   @history 2011-07-05 Jeannie Backer - Added qCompute() method. Updated
   *                           documentation.
   *   @history 2011-08-11 Steven Lambright - phi2Compute was running out of
   *                           iterations for vesta (an asteroid). Fixes #279
   *   @history 2012-03-01 Jeff Anderson - Fixed bug in SetUpperLeftCorner by
   *                           adding Pvl::Replace when updating the mapping
   *                           labels
   *   @history 2012-03-30 Steven Lambright and Stuart Sides - To360Domain() and
   *                           To180Domain() are now constant time operations.
   *                           Fixes #656.
   *   @history 2012-06-15 Jeannie Backer - Added documentation.  Added forward
   *                           declarations to header file. Ordered includes in
   *                           implementation file. Moved destructor and
   *                           accessor methods to the implementation file.
   *                           Moved the following methods from the
   *                           ObliqueCylindrical class for
   *                           generalized xyRangeOblique() method - doSearch(),
   *                           findExtreme(), setSearchGround(). Changed member
   *                           variable prefix to m_. Improved unitTest.
   *                           Resolves #928.
   *   @history 2012-07-25 Jeannie Backer - Modified the new methods related to
   *                           xyRangeOblique() to verify the validity of the
   *                           values when setSearchGround() is called.
   *                           References #954.
   *   @history 2012-08-01 Kimberly Oyama - Added a qFuzzyCompare() to ToPlanetographic() to
   *                           account for rounding error at the latitude boundaries (-90 and
   *                           90 degrees). Updated the unit test to exercise this change.
   *                           References #604.
   *   @history 2012-08-09 Steven Lambright - Added NaifStatus calls to protect
   *                           the TargetRadii() method from naif throwing an
   *                           error/crashing with inputs that have non-target naif codes.
   *   @history 2012-01-19 Debbie A. Cook - Moved nongeneral functionality into another virtual
   *                           base class layer of TProjection and PlaneProjection.  Added m_projectionType
   *                           and ProjectionType enum and protected method PixelResolution().
   *   @history 2013-03-06 Steven Lambright - Cleaned up and fixed an issue with the Mapping()
   *                           method which sometimes erroneously threw an exception. Added caching
   *                           to the TargetRadii(QString) method for performance reasons.
   *                           Fixes #1534.
   *   @history 2017-06-26 Jesse Mapel - Added a new method to set the universal ground point
   *                           without adjusting for the longitude domain. Fixes #2185.
   *
   *   @todo Continue to modify Projection class to comply with coding
   *         standards. Some of these include, but may not be limited to remove
   *         "Get" from methods GetX and GetY, change methods to lower camel
   *         case, add methods for RelativeScaleFactorLatitude and
   *         RelativeScaleFactorLongitude (see LambertAzimuthalEqualArea.cpp).
   */
  class Projection {
    public:
      // constructor
      Projection(Pvl &label);
      virtual ~Projection();
      virtual bool operator== (const Projection &proj);
      bool operator!= (const Projection &proj);

      /**
       * This enum defines the subclasses of Projection supported in Isis
       */
      enum ProjectionType { Triaxial, /**< These projections are used to map
                                                   triaxial and irregular-shaped bodies. */
                                RingPlane  /**< These projections are used to map
                                                   ring planes.*/
                              };

      // These are the accessors for the projection type
      void setProjectionType(const ProjectionType ptype);
      ProjectionType projectionType() const;

      // These return properties of the target
      bool IsSky() const;

      // These return or change properties of the projection, independent of calculations
      /**
       * This method returns the name of the map projection.  It is a pure
       * virtual method (requires all subclasses to override).
       *
       * @return string The name of the map projection.
       */
      virtual QString Name() const = 0;

      // These return properties of the target
      virtual double LocalRadius() const = 0; // requires SetGround or SetCoordinate

      /**
       * This method returns the Version of the map projection.  It is a pure
       * virtual method (requires all subclasses to override).
       *
       * @return string The Version number of the map projection.
       */
      virtual QString Version() const = 0;
      virtual bool IsEquatorialCylindrical();

      // Check azimuth /longitude domain or get domain as a string
      // TODO** check implementation to see if this can be generalized to
      // work for azimuth and longitude and go in Projection

      // Check longitude domain or get longitude domain as a string
      /*      bool Has180Domain() const;
      bool Has360Domain() const;
      QString LongitudeDomainString() const;
      */

      // Check if labels contain min/max lat/lon or comparable
      virtual bool HasGroundRange() const;

      // get rotation
      double Rotation() const;

      // Set world mapper
      void SetWorldMapper(WorldMapper *mapper);

      //  Calculations
      // Set ground position or x/y coordinate
      virtual bool SetGround(const double lat, const double lon) = 0;
      virtual bool SetCoordinate(const double x, const double y) = 0;
      bool IsGood() const;

      // Methods that depend on successful completion
      // of SetCoordinate Get x,y
      double XCoord() const;
      double YCoord() const;

      // Set the universal ground coordinate (calls SetGround)
      virtual bool SetUniversalGround(const double coord1, const double coord2);
      virtual bool SetUnboundUniversalGround(const double coord1, const double coord2);

      // Set world position (calls SetCoordinate on projected x/y)
      virtual bool SetWorld(const double x, const double y);

      // Get computed world X/Y after successful SetGround
      virtual double WorldX() const;
      virtual double WorldY() const;

      // convert from projected coordinate to world coordinate
      double ToWorldX(const double projectionX) const;
      double ToWorldY(const double projectionY) const;

      // convert from world coordinate to projected coordinate
      double ToProjectionX(const double worldX) const;
      double ToProjectionY(const double worldY) const;

      // get resolution and scale for mapping world coordinates
      double Resolution() const;
      virtual double Scale() const = 0;

      // Return the x/y range which covers the lat/lon range in the labels
      virtual bool XYRange(double &minX, double &maxX,
                           double &minY, double &maxY) = 0;

      // set UpperLeftCornerX and UpperLeftCornerY in mapping group
      void SetUpperLeftCorner(const Displacement &x, const Displacement &y);

      // get mapping information
      virtual PvlGroup Mapping() = 0;

      // Static conversion methods
      static double ToHours(double angle);
      static QString ToDMS(double angle);
      static QString ToHMS(double angle);

    protected:
      virtual void XYRangeCheck(const double latitude, const double longitude) = 0;
      //      bool xyRangeOblique(double &minX, double &maxX,
      //                          double &minY, double &maxY);
      void SetXY(double x, double y);
      void SetComputedXY(double x, double y);
      double GetX() const;
      double GetY() const;
      double PixelResolution() const;

    private:
      // This is currently only used in triaxial projections.  In the future it may be needed by other types
      /*      virtual void doSearch(double minBorder, double maxBorder,
                    double &extremeVal, const double constBorder,
                    bool searchX, bool searchLongitude, bool findMin) = 0;
      virtual void findExtreme(double &minBorder,  double &maxBorder,
                       double &minBorderX, double &minBorderY,
                       double &maxBorderX, double &maxBorderY,
                       const double constBorder, bool searchX,
                       bool searchLongitude, bool findMin) = 0;
      virtual void setSearchGround(const double variableBorder,
                           const double constBorder, bool variableIsLat) = 0;
      */
    protected:
      WorldMapper *m_mapper;  /**< This points to a mapper passed into the
                                   SetWorldMapper method. This mapper allows the
                                   programmer to specify a different world
                                   coordinate system. Thus the programmer could
                                   pass in line/sample positions in order to
                                   obtain a latitude/longitude or set a lat/lon
                                   and get a line/sample.*/

      bool m_good;         /**< Indicates if the contents of m_x, m_y, 
                                m_latitude, and m_longitude are valid.*/

      // TODO** Can this be generalized for both longitude and azimuth???
      //            int m_longitudeDomain; /**< This integer is either 180 or 360 and is read
      //                                  from the labels. It represents the longitude
      //                                  domain when returning values through Longitude
      //                                  method. The domain is either -180 to 180 or
      //                                  0 to 360.**/

      bool m_sky;                 /**< Indicates whether projection is sky or
                                       land.**/

      bool m_groundRangeGood;     /**< Indicates if the ground range (min/max
                                       lat/lons) were read from the labels.*/

      // Convenience data for XYRange virtual function
      double m_minimumX;  /**< The data elements m_minimumX, m_minimumY,
                              m_maximumX, and m_maximumY are convience data
                              elements when you write the XYRange virtual
                              function. They are used in conjuction with the
                              XYRangeCheck convience method. After utilizing
                              XYRangeCheck to test boundary conditions in the
                              XYRange method these values will contain the
                              projection x/y coverage for the ground range
                              specified by min/max lat/lon.*/
      double m_maximumX;  //!< See minimumX description.
      double m_minimumY;  //!< See minimumX description.
      double m_maximumY;  //!< See minimumX description.
      PvlGroup m_mappingGrp; //!< Mapping group that created this projection

    private:
      ProjectionType m_projectionType;
      double m_rotation;   //!< Rotation of map (usually zero)

      double m_x;          /**< This contains the rotated X coordinate for a
                               specific projection at theposition indicated by
                               m_latitude/m_longitude. The value is only
                               usable if m_good is true.*/
      double m_y;          /**< This contains the rotated Y coordinate for a
                               specific projection at the position indicated by
                               m_latitude/m_longitude. The value is only
                               usable if m_good is true.*/

      double m_pixelResolution; /**< Pixel resolution value from the PVL mapping
                                     group, in meters/pixel.**/
  };
};
#endif
