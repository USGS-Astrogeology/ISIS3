/** This is free and unencumbered software released into the public domain.
The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */

// Uncomment this definition and you get thread-safe methods.  Note this is not
// done in the header as it would change the header footprint and require a
// complete system recompile.  Done here only invokes locking with defined mutex
// classes in the header.  To make this happen on demand, uncomment the define,
// recompile, reinsert in libisisx.y.z.a and recreate the shared library.
// 
// #define MAKE_THREAD_SAFE 1

#include "NaifDskPlateModel.h"

#include <iostream>
#include <iomanip>
#include <numeric>
#include <sstream>

#if defined(MAKE_THREAD_SAFE)
#include <QMutexLocker>
#endif
#include <QScopedPointer>

#include "FileName.h"
#include "IException.h"
#include "Intercept.h"
#include "IString.h"
#include "Latitude.h"
#include "Longitude.h"
#include "NaifDskApi.h"
#include "NaifStatus.h"
#include "SurfacePoint.h"
#include "TriangularPlate.h"

using namespace std;

namespace Isis {

  /** Default empty constructor */
  NaifDskPlateModel::NaifDskPlateModel() : m_dsk(0) { }
  


  /** Construct given a file name - the only way to create with a DSK file */
  NaifDskPlateModel::NaifDskPlateModel(const QString &dskfile) : m_dsk(0) {
    m_dsk = SharedNaifDskDescriptor(openDSK(dskfile));
    if ( !isValid() ) {
      std::string mess = "Could not open DSK file " + dskfile.toStdString();
      throw IException(IException::User, mess, _FILEINFO_);
    }
  }
  


  NaifDskPlateModel::~NaifDskPlateModel() { }



  /** Checks validity of the object */
  bool NaifDskPlateModel::isValid() const {
    return ( !m_dsk.isNull() );
  }
  


  /** Returns the nane of the NAIF DSK file */
  QString NaifDskPlateModel::filename() const {
    if ( isValid() ) { return (m_dsk->m_dskfile); }
    return ( QString() );
  }
  


  /** Returns the number of plates in the DSK file - mostly for conformity */
  int NaifDskPlateModel::size() const {
    return ( numberPlates() );
  }
  


  /** Returns the number of plates in the model */
  int NaifDskPlateModel::numberPlates() const {
    if ( !isValid() )  return (0);
    return ( m_dsk->m_plates );
  }
  


  /** Returns the number of vertices in the plate model */
  int NaifDskPlateModel::numberVertices() const {
    if ( !isValid() )  return (0);
    return ( m_dsk->m_vertices );
  }
  


  /**
   * @brief Get surface intersection for a lat/lon grid point 
   *  
   * This method will return a surface point intercept given a lat/lon coordinate. 
   * Primary use of this method is to determine radius values at the grid point. 
   *  
   * Essentially a fixed body ray is created from the lat/lon location that 
   * extends beyond the highest radius of the body as defined by the plate model. 
   * The endpoint of this ray serves as the observer position. A look direction 
   * vector is created from the observer point by reversing the direction of the 
   * vector from the center of the body to the observer point.  Theoretically, 
   * this routine should not fail based upon this technique. 
   *  
   * @history 2013-12-05 Kris Becker Original Version 
   * 
   * @param lat Latitide of the grid coordinate point
   * @param lon Longitude of the grid coordinate point
   * 
   * @return SurfacePoint*  Returns a pointer to a valid intercept point
   */
  SurfacePoint *NaifDskPlateModel::point(const Latitude &lat, 
                                         const Longitude &lon) const {
  
    // Sanity check on input point
    verify ( lat.isValid(), "Latitude parameter invalid in NaifDskPlateMode::point()" );
    verify ( lon.isValid(), "Longitude parameter invalid in NaifDskPlateMode::point()" );
  
    // Ensure a DSK file is opened or exception is thrown
    verify( isValid(), "NAIF DSK file not opened/valid!");
  
    // Get the lon/lat point in radians
    SpiceDouble lonlat[2];
    lonlat[0] = lon.positiveEast(Angle::Radians);
    lonlat[1] = lat.planetocentric(Angle::Radians);
    SpiceInt npoints(1);
    NaifVertex spoint(3, 0.0);
    SpiceInt plateId(-1);
  
  #if defined(MAKE_THREAD_SAFE)
    QMutexLocker lock(&m_dsk->m_mutex);  // Thread locking for NAIF I/O
  #endif
  
    llgrid_pl02( m_dsk->m_handle, &m_dsk->m_dladsc, npoints, 
                 (ConstSpiceDouble (*)[2]) lonlat, 
                 (SpiceDouble (*)[3]) &spoint[0], &plateId);
    NaifStatus::CheckErrors();
  
  #if 0
    if ( !isPlateIdValid(plateId) ) {
      std::string mess = "Plateid = " + QString::number(plateId) + " is invalid";
      throw IException(IException::Programmer, mess, _FILEINFO_);
    }
  #endif
  
    // Other error checks???
    return  ( makePoint(spoint) );
  }
  


  /**
   * @brief Determine a target body intercept point from an observer and look 
   *        direction
   *  
   * This method will compute an intercept point on the target body given a body 
   * fixed vertex of an observer and a look direction.  This implements a true ray
   * intersection algortihm. 
   *  
   * If an intercept point cannot be found a null pointer is returned. 
   * 
   * @history 2013-12-05 Kris Becker Original Version
   * 
   * @param vertex  Body fixed X/Y/Z coordinate of the observer
   * @param raydir  Vector that specifies a look direction
   * 
   * @return Intercept* Pointer to a valid intercept point.  If no intercept is 
   *                    found, a null pointer is returned.
   */
  Intercept *NaifDskPlateModel::intercept(const NaifVertex &vertex,
                                          const NaifVector &raydir) const {
    // Get the plate
    NaifVertex xpoint;
    SpiceInt plateid = plateIdOfIntercept(vertex, raydir, xpoint);
    if ( !isPlateIdValid(plateid) ) return (0);
  
    NaifTriangle triangle = plate(plateid); 
  
    // Return the intercept
    return (new Intercept(vertex, raydir, makePoint(xpoint), 
                          new TriangularPlate(triangle, plateid)));
  }
  


  /**
   * @brief Determines if the plate ID is valid 
   *  
   * A valid plateId is between the values of 1 and the number of plates in the 
   * model. 
   *  
   * This method is useful for checking for errors when determining the plate of 
   * intersection.  Valid intersections are initially return by the NAID DSK API 
   * as identifying the plate ID where the intersection occurs.  A DSK routine 
   * then retreives the triangle plate (TIN) by this ID number. 
   * 
   * @history 2013-12-05 Kris Becker Original Version
   * 
   * @param plateid Plate ID to check for validity
   * 
   * @return bool Returns true if a valid plate ID is provided, otherwise false 
   *              is returned
   */
  bool NaifDskPlateModel::isPlateIdValid(const SpiceInt plateid) const {
    if ( !isValid() ) return (false);
    return ( (plateid >= 1) && (plateid <= m_dsk->m_plates) );
  }
  


  /**
   * @brief Primary API to determine ray intercept from observer/look direction 
   *  
   * This method determines an intercept point given an observer position and a 
   * look direction vector.  It uses the NAIF DSK API to determine the ray 
   * intercept point given the position and look direction from the observer. 
   * 
   * @history 2013-12-05 Kris Becker Original Version
   * 
   * @param vertex  Position in body fixed coordinates of observer
   * @param raydir  Look direction vector to use as intercept ray
   * @param xpoint  Output of point of intercept in body fixed coordinates if an 
   *                intercept point is found
   * 
   * @return SpiceInt Returns the plate ID of the intercept point
   */
  SpiceInt NaifDskPlateModel::plateIdOfIntercept(const NaifVertex &vertex, 
                                                    const NaifVector &raydir,
                                                    NaifVertex &xpoint) const {
  
    // Sanity check on input parameters
    try {
      verify( validate(vertex), "Invalid/bad dimensions on intercept source point");
      verify( validate(raydir), "Invalid/bad dimensions on ray direction vector");
    }
    catch ( IException &ie ) {
      throw IException(ie, IException::Programmer, 
                       "Invalid point source data to determine intercept", 
                       _FILEINFO_);
    }
  
    // Ensure a DSK file is opened or exception is thrown
    verify( isValid(), "NAIF DSK file not opened/valid!");
  
    // Get the lon/lat point in radians
    SpiceInt plateid;
    NaifVertex xpt(3, 0.0);
    SpiceBoolean found;
  
  #if defined(MAKE_THREAD_SAFE)
    QMutexLocker lock(&m_dsk->m_mutex);  // Thread locking for NAIF I/O
  #endif
    // Find the plate of intersection and intercept point
    NaifStatus::CheckErrors();
    dskx02_c( m_dsk->m_handle, &m_dsk->m_dladsc, &vertex[0], &raydir[0],
              &plateid, &xpt[0], &found);
    // Check status
    NaifStatus::CheckErrors();
    if ( !found ) return (0);
  
    // Return succesful results
    xpoint = xpt;
    return ( plateid );
  }
  

  
  /**
   * @brief Retrieve the triangular plate identified by its ID
   *  
   * This method can be used to retrieve a particular plate specified by its ID. 
   * This is useful for general retrieval and can be used to easily read all 
   * plates in a DSK file. 
   *  
   * @author 2014-02-05 Kris Becker Original Version
   * 
   * @param plateid Valid id of the plate to retrieve
   * 
   * @return NaifTriangle Triahgle associated with the plate id
   */
  NaifTriangle NaifDskPlateModel::plate(SpiceInt plateid) const {
  
    // Ensure a DSK file is opened or exception is thrown
    verify( isValid(), "NAIF DSK file not opened/valid!");
  
    // Sanity check on plateid
    if ( !isPlateIdValid(plateid) ) {
      std::string mess = "Plateid = " + toString(plateid) + " is invalid";
      throw IException(IException::Programmer, mess, _FILEINFO_);
    }
  
   // Get the plate
    SpiceInt nplates;
    SpiceInt iplate[3];
  
  #if defined(MAKE_THREAD_SAFE)
    QMutexLocker lock(&m_dsk->m_mutex);  // Thread locking for NAIF I/O
  #endif
  
    NaifStatus::CheckErrors();
    dskp02_c(m_dsk->m_handle, &m_dsk->m_dladsc, plateid, 1, &nplates, 
             ( SpiceInt(*)[3] )(iplate));
    NaifStatus::CheckErrors();
  
    // Get the verticies of the plates
    NaifTriangle plate(3, 3);
    SpiceInt n;
    for (int i = 0 ; i < 3 ; i++) {
    dskv02_c(m_dsk->m_handle, &m_dsk->m_dladsc, iplate[i], 1, &n, 
             ( SpiceDouble(*)[3] )(plate[i]));
    }
    NaifStatus::CheckErrors();
  
    return (plate);
  }
  


  /**
   * @brief Opens a valid NAIF DSK plate model file 
   *  
   * This method opens a NAIF DSK plate model file and initializes internal 
   * components for TIN processing. 
   *  
   * If the file is valid a pointer to a NAIF DSK descriptor is returned for use. 
   * 
   * @history 2013-12-05 Kris Becker Original Version
   * 
   * @param dskfile Name of NAIF DSK file to open.  These files typically end in a 
   *                ".bds" extension
   * 
   * @return NaifDskPlateModel::NaifDskDescriptor* Returns a pointer to a NAIF DSK 
   *             file descriptor if operations successful. An exception is thrown
   *             if an error should occur.
   */
  NaifDskPlateModel::NaifDskDescriptor *NaifDskPlateModel::openDSK(const QString &dskfile) {
  
    // Sanity check
    FileName dskFile(dskfile.toStdString());
    if ( !dskFile.fileExists() ) {
      std::string mess = "NAIF DSK file [" + dskfile.toStdString() + "] does not exist.";
      throw IException(IException::User, mess, _FILEINFO_);
    }
  
    // Open the NAIF Digital Shape Kernel (DSK)
    QScopedPointer<NaifDskDescriptor> dsk(new NaifDskDescriptor());
    dsk->m_dskfile = dskfile;
    NaifStatus::CheckErrors();
    dasopr_c( dskFile.expanded().c_str(), &dsk->m_handle );
    NaifStatus::CheckErrors();
  
    // Search to the first DLA segment
    SpiceBoolean found;
    dlabfs_c( dsk->m_handle, &dsk->m_dladsc, &found );
    NaifStatus::CheckErrors();
    if ( !found ) {
      std::string mess = "No segments found in DSK file " + dskfile.toStdString(); 
      throw IException(IException::User, mess, _FILEINFO_);
    }

    NaifStatus::CheckErrors();
    dskgd_c( dsk->m_handle, &dsk->m_dladsc, &dsk->m_dskdsc );
  
    // Get size/counts
    dskz02_c( dsk->m_handle, &dsk->m_dladsc, &dsk->m_vertices, 
                     &dsk->m_plates );
    NaifStatus::CheckErrors();
  
    // return pointer
    return ( dsk.take() );
  }
  


  /** Convenience method for generalized error reporting */
  bool NaifDskPlateModel::verify(const bool &test, const QString &errmsg,
                                 const NaifDskPlateModel::ErrAction &action) 
                                 const {
    if ( ( Throw == action ) && ( !test ) ) {
      throw IException(IException::Programmer, errmsg.toStdString(), _FILEINFO_);
    }
  
    // Looks good
    return ( test );
  }
  


  /** Construct and return a SurfacePoint pointer  */
  SurfacePoint *NaifDskPlateModel::makePoint(const NaifVertex &v) const {
    verify( validate(v), "Vertex/point invalid - not a 3 vector" );
    return (new SurfacePoint(Displacement(v[0], Displacement::Kilometers),
                             Displacement(v[1], Displacement::Kilometers),
                             Displacement(v[2], Displacement::Kilometers)));
  
  }


  NaifDskPlateModel::NaifDskDescriptor::NaifDskDescriptor() : m_dskfile(), m_handle(-1), 
                                                              m_dladsc(), m_dskdsc(), m_plates(0), 
                                                              m_vertices(0), m_mutex() { 
  }


  NaifDskPlateModel::NaifDskDescriptor::~NaifDskDescriptor() {
    if ( -1 != m_handle ) { 
      NaifStatus::CheckErrors();
      dascls_c ( m_handle ); 
      NaifStatus::CheckErrors();
    }
  }
  

}  // namespace Isis
