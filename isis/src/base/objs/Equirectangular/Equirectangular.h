#ifndef Equirectangular_h
#define Equirectangular_h
/** This is free and unencumbered software released into the public domain.
The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */

#include "TProjection.h"
#include <QString>

namespace Isis {
  class Pvl;
  /**
   * @brief Equirectangular Map Projection
   * 
   * This class provides methods for the forward and inverse equations of a
   * Equirectangular map projection (for a sphere). 
   *  
   * The Equirectangluar projection is a cylindrical projection in which the 
   * poles, latitudes, and longitudes are represented as straight, equidistant, 
   * lines.  The poles and latitudes are horizontal lines and the longitudes are 
   * vertical lines.  These lines intersect at right angles. 
   *  
   * The code was converted to C++ from the Fortran version of the USGS General 
   * Cartographic Transformation Package (GCTP).  In particular it was modified 
   * from the Equidistant Cylindrical code. This class inherits IsisProjection 
   * and provides the two virtual methods SetGround (forward) and SetCoordinate 
   * (inverse) and a third virtual method, XYRange, for obtaining projection 
   * coordinate coverage for a latitude/longitude window.  
   *
   * Please see the Projection class for a full accounting of all the methods 
   * available. 
   *  
   * @ingroup MapProjection
   *  
   * @author 2003-11-13 Jeff Anderson
   *
   * @internal
   *   @history 2004-02-07 Jeff Anderson - added plug-in capability.
   *   @history 2004-02-24 Jeff Anderson - Modified forward and inverse methods
   *                           to use the local radius at the center latitude
   *                           instead of the equitorial radius.
   *   @history 2005-03-11 Elizabeth Ribelin - added TrueScaleLatitude method
   *                           test to the unitTest
   *   @history 2007-06-29 Steven Lambright - Added Mapping, MappingLatitudes
   *                           and MappingLongitudes methods.
   *   @history 2008-05-09 Steven Lambright - Added Name, Version,
   *                           IsEquatorialCylindrical methods
   *   @history 2008-05-09 Steven Lambright - Fixed test for being too close to
   *                           a pole
   *   @history 2008-11-12 Steven Lambright - Commented some unclear code
   *                           (CenterLatitudeRadius keyword)
   *   @history 2012-06-15 Jeannie Backer - Added documentation.  Added forward
   *                           declaration of Pvl to header file.  Ordered
   *                           includes in implementation file. Moved Name,
   *                           Version, IsEquatorialCylindrical to the
   *                           implementation file. Minor modifications to
   *                           comply with some coding standards. References
   *                           #928.
   *   @history 2012-01-20 Debbie A. Cook - Changed to use TProjection instead of Projection.  
   *                           References #775.
   *   @history 2013-04-26 Jeannie Backer - Modified constructor so that default
   *                           center lat/lon values are at the center of the
   *                           lat/lon ranges, respectively. This was done to be
   *                           consistent with other projection defaults.
   *                           Improved test coverage. Fixes #1597.
   *   @history 2013-05-14 Jeannie Backer - Fixed unitTest merge error. References #775.
   */
  class Equirectangular : public TProjection {
    public:
      Equirectangular(Pvl &label, bool allowDefaults = false);
      ~Equirectangular();
      bool operator==(const Projection &proj);

      QString Name() const;
      QString Version() const;
      double TrueScaleLatitude() const;
      bool IsEquatorialCylindrical();

      bool SetGround(const double lat, const double lon);
      bool SetCoordinate(const double x, const double y);
      bool XYRange(double &minX, double &maxX, double &minY, double &maxY);

      virtual PvlGroup Mapping();
      virtual PvlGroup MappingLatitudes();
      virtual PvlGroup MappingLongitudes();

    private:
      double m_centerLongitude; //!< The center longitude for the map projection
      double m_centerLatitude;  //!< The center latitude for the map projection
      double m_cosCenterLatitude;//!< Cosine of the center latitude
      double m_clatRadius;       /**< The radius of the target planet at the 
                                      center latitude.*/
  };
};

#endif

