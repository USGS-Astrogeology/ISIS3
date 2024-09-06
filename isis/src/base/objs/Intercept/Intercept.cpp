/** This is free and unencumbered software released into the public domain.
The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */

#include "Intercept.h"

#include <numeric>
#include <string>
#include <vector>

#include <QtGlobal>

#include "Angle.h"
#include "AbstractPlate.h"
#include "IException.h"
#include "IString.h"
#include "NaifDskApi.h"
#include "Latitude.h"
#include "Longitude.h"
#include "SurfacePoint.h"

using namespace std;

namespace Isis {

  /** Default empty constructor */
  Intercept::Intercept() : m_observer(), m_raydir(), m_point(0), m_shape(0) { }
  
  
    
  /**
   * @brief Constructor of predetermined intercept point 
   *  
   * This constructor provides all the elements that comprise an observer, look 
   * direction, intercept point and the shape it intersects.  This is well suited 
   * for plates (TIN) intercept senarios. 
   * 
   * 
   * @param observer Location of observer in body fixed position
   * @param raydir   Look direction of observer presumably at the target body
   * @param ipoint   Surface point of the intercept location on the body
   * @param shape    Shape of the intercept point 
   *  
   * @internal
   *   @history 2014-02-10 Kris Becker - Original version
   */
  Intercept::Intercept(const NaifVertex &observer, const NaifVector &raydir,
                       SurfacePoint *ipoint, AbstractPlate *shape) :
                       m_observer(observer.copy()), m_raydir(raydir.copy()),
                       m_point(ipoint), m_shape(shape) {}
  



  /** Empty destructor */
  Intercept::~Intercept() {}



  /** 
   *  This method tests the vailidty of the intercept point.  A point is invalid
   *  if the intercept point is null, if the shape is null, or if either the
   *  observer position or look direction vector is not size 3.
   *  
   *  @return <b>bool</b> Indicates whether this object is valid.
   */
  bool Intercept::isValid() const {
    bool valid = true;
    if ( !validate( m_observer ) ) valid = false;
    if ( !validate( m_raydir ) ) valid = false;
    if ( m_point.isNull() ) valid = false;
    if ( m_shape.isNull() ) valid = false;
    return valid;
  }
  


  /** 
   *  Accessor for the observer position of the intercept.
   *  
   *  @return <b>NaifVertex</b> A 3D point representing the position of the observer in body
   *          fixed coordinates
   */
  const NaifVertex &Intercept::observer() const {
    return (m_observer);
  }
  


  /** 
   *  Accessor for the look direction of the intercept.
   *  
   *  @return <b>NaifVector</b> A 3D ray representing the look direction of the
   *          observer in body fixed coordinates.
   */
  const NaifVector &Intercept::lookDirectionRay() const {
    return (m_raydir);
  }
  


  /**
   * @brief Returns the location of the intercept location on the shape 
   *  
   * This method returns the point of intercept from the observer and a given look 
   * direction from the perspective of the observer. 
   * 
   * @return SurfacePoint Intercept point from the observer and look direction
   *  
   * @internal
   *   @history 2014-02-13 Kris Becker - Original version
   */
  SurfacePoint Intercept::location() const {
    verify( isValid(), "Unable to return Intercept location. Invalid/undefined Intercept point.");
    return (SurfacePoint(*m_point));
  }
  


  /** 
   *  Gets the normal vector to the shape for this plate.
   *  
   *  @return <b>NaifVector</b> A 3D ray representing the normal direction at
   *          the intercept point in body fixed coordinates.
   */
  NaifVector Intercept::normal() const {
    verify( isValid(), "Unable to return Intercept normal. Invalid/undefined Intercept point.");
    return ( m_shape->normal() );
  }
  



  /**
   * @brief Compute the emission of the intercept point from the observer 
   *  
   * This method computes the emission angle from the observer to the plate 
   * intercept point if the representation of the observer, look direction and 
   * intercept point are valid. 
   * 
   * @return Angle Returns the emission angle as computed from the observer given 
   *               as determine from the intercept point
   *  
   * @internal
   *   @history 2013-12-05 Kris Becker - Original version.
   */
  Angle Intercept::emission() const {
    verify( isValid(), 
            "Unable to return Intercept emission angle. Invalid/undefined Intercept point." );
  
    // Point back toward the center of the body 
    NaifVertex point(3);
    m_point->ToNaifArray(&point[0]);
  
    NaifVector raydir(3);
    vsub_c(&m_observer[0], &point[0], &raydir[0]);
  
    // Return the separation angle between them
    return (separationAngle(raydir));
  }
  


  /**
   * @brief Returns the separation angle of the observer and the plate normal 
   *  
   * This method computes the seperation between the look direction (from the 
   * observer) and the plate normal if the object contains a valid intercept 
   * point. 
   * 
   * @param raydir Look direction to compute the separation angle between it and 
   *               the plate normal.
   * 
   * @return Angle Angle of separation
   *  
   * @internal
   *   @history 2013-12-05 Kris Becker - Original version.
   */
  Angle Intercept::separationAngle(const NaifVector &raydir) const {
    verify( isValid(), 
            "Unable to return Intercept separation angle. Invalid/undefined Intercept point.");
    return (m_shape->separationAngle(raydir));
  }
  


  /**
   * @brief Convenient error handler 
   *  
   * This convenience method provides an abtraction from which to throw errors. 
   * 
   * @param test   Boolean parameter to test for an error condition
   * @param errmsg String containing the error message to include in the exception
   * @param action Specifies action to take if error is specfied
   * 
   * @return bool Always returns the value of test if the specfied action does not 
   *              request an exception to be thrown
   *  
   * @internal
   *   @history 2013-12-05 Kris Becker - Original version.
   */
  bool Intercept::verify(const bool &test, const QString &errmsg,
                         const Intercept::ErrAction &action) const {
    if ( ( Throw == action ) && ( !test ) ) {
      throw IException(IException::Programmer, errmsg.toStdString(), _FILEINFO_);
    }
  
    // Looks good
    return ( test );
  }
  


  /** 
   *  Access the plate for this intercept.  NOTE: A null pointer may be returned
   *  if this Intercept was constructed without a shape.
   *  
   *  @return <b>NaifVector</b> A pointer to the AbstractPlate associated with this intercept.
   */
  const AbstractPlate *Intercept::shape() const {
    return ( m_shape.data() );
  }

}
// namespace Isis
