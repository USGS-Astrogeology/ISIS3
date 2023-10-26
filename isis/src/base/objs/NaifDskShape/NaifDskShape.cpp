/** This is free and unencumbered software released into the public domain.
The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */
#include "NaifDskShape.h"

#include <numeric>

#include <QtGlobal>
#include <QVector>

#include "IException.h"
#include "Intercept.h"
#include "IString.h"
#include "Latitude.h"
#include "Longitude.h"
#include "NaifDskApi.h"
#include "NaifDskPlateModel.h"
#include "NaifStatus.h"
#include "Pvl.h"
#include "ShapeModel.h"
#include "SpecialPixel.h"
#include "Statistics.h"
#include "SurfacePoint.h"
#include "Target.h"


using namespace std;

namespace Isis {

  /** Generic constructor sets type to a TIN */
  NaifDskShape::NaifDskShape() : ShapeModel(), m_intercept(NULL) {
    // defaults for ShapeModel parent class include:
    //     name = empty string
    //     surfacePoint = null sp
    //     hasIntersection = false
    //     hasNormal = false
    //     normal = (0,0,0)
    //     hasEllipsoidIntersection = false
    setName("DSK");
  }


  /**
   * @brief Constructor provided for instantiation from an ISIS cube
   *
   * This constructor is typically used for and ISIS cube that has been
   * initialized by spiceinit.  The DEM name should be that of a NAIF DSK file.
   * This constructor will throw an exception if it fails to open the DSK file.
   *
   * @author 2014-02-12 Kris Becker
   *
   * @param target Target object describing the observed body
   * @param pvl    ISIS Cube label.  Extract the name of the DEM from the Kernels
   *               group
   */
  NaifDskShape::NaifDskShape(Target *target, Pvl &pvl) : ShapeModel(target),
                                       m_intercept(NULL) {

    // defaults for ShapeModel parent class include:
    //     name = empty string
    //     surfacePoint = null sp
    //     hasIntersection = false
    //     hasNormal = false
    //     normal = (0,0,0)
    //     hasEllipsoidIntersection = false

    setName("DSK");  // Really is used as type in the system at present!

    PvlGroup &kernels = pvl.findGroup("Kernels", Pvl::Traverse);

    QString dskFile;
    if (kernels.hasKeyword("ElevationModel")) {
      dskFile = QString::fromStdString(kernels["ElevationModel"]);
    }
    else { // if (kernels.hasKeyword("ShapeModel")) {
      dskFile = QString::fromStdString(kernels["ShapeModel"]);
    }

    // Attempt to initialize the DSK file - exception ensues if errors occur
    // error thrown if ShapeModel=Null (i.e. Ellipsoid)
    m_model = NaifDskPlateModel(dskFile);

  }


  /**
   * @brief Constructor for creating new shape model from the same DSK file
   *
   * This constructor provides the ability to create a formal shape model from the
   * NAIF DSK plate model file already opened. This approach allows multiple
   * threads to access the same DSK file interface without the overhead of opening
   * many instances of the same file.
   *
   * @author 2014-02-12 Kris Becker
   *
   * @param model DSK plate model from an existing NaidDskPlateModel (see the
   *              model() method
   */
  NaifDskShape::NaifDskShape(const NaifDskPlateModel &model) :
                                       m_model(model), m_intercept(NULL) {

    // TODO create valid Target
    // Using this constructor, ellipsoidNormal(),
    // calculateSurfaceNormal(), and setLocalNormalFromIntercept()
    // methods can not be called
  }


  /** Destructor - cleanup is handled automagically */
  NaifDskShape::~NaifDskShape() { }


  /**
   * @brief Compute a DEM intersection from and observer and look direction
   *
   * This method computes a DEM intercept point given an observer location and
   * direction vector in body fixed coordinates.  This method provides true ray
   * intercept techiques as implemented by NAIF's DSK API.
   *
   * If the intercept is successful, its state is retain in this class for further
   * application.  Use of a scoped pointer handles the memory management and
   * indication of success.
   *
   * @author 2014-02-13 Kris Becker
   *
   * @param observerPos    Position of observer in body fixed coordiates
   * @param lookDirection  Look direction (ray) from the observer
   *
   * @return bool Returns true if an intercept was successful, false otherwise
   */
  bool NaifDskShape::intersectSurface(std::vector<double> observerPos,
                                           std::vector<double> lookDirection) {
    NaifVertex obs(3, &observerPos[0]);
    NaifVector raydir(3, &lookDirection[0]);
    m_intercept.reset(m_model.intercept(obs, raydir));

    bool success = !m_intercept.isNull();
    if (success) {
      SurfacePoint point = m_intercept->location();
      setSurfacePoint(point); // sets ShapeModel::m_hasIntersection=t, ShapeModel::m_hasNormal=f
    }
    return ( success );
  }


  /**
  * @brief Compute surface intersection with optional occlusion check
  *
  * This method sets the surface point at the given latitude, longitude. The
  * derived model is called to get the radius at that location to complete the
  * accuracy of the surface point, them the derived method is called to complete
  * the intersection.
  *
  * @author 2022-07-14 Stuart Sides, Jesse Mapel
  *
  * @param surfpt       Absolute point on the surface to check
  * @param observerPos  Position of the observer
  * @param backCheck    Flag to indicate occlusion check
  *
  * @return bool        True if the intersection point is valid (visable)
  */
  bool NaifDskShape::intersectSurface(const SurfacePoint &surfpt,
                                    const std::vector<double> &observerPos,
                                    const bool &backCheck) {

    std::vector<double> look(3);
    look[0] = surfpt.GetX().kilometers() - observerPos[0];
    look[1] = surfpt.GetY().kilometers() - observerPos[1];
    look[2] = surfpt.GetZ().kilometers() - observerPos[2];

    return intersectSurface(observerPos, look);

  }


  /**
   * @brief Determine DEM radius at a given lat/lon grid point
   *
   * This method computes the radius value of a point on an ellipsoid.  A vector
   * from the center of the body through the lart/lon location on the ellipsiod.
   * From this, a look direction back toward the center of the body is genrated
   * and then an intercept point is determined.  See NaifDskPlateMode::point() for
   * details.
   *
   * @author 2014-02-10 Kris Becker
   *
   * @param lat Latitude coordinate of grid point
   * @param lon Longitude coordinate of grid point
   *
   * @return Distance Radius value of the intercept grid point
   */
  Distance NaifDskShape::localRadius(const Latitude &lat,
                                          const Longitude &lon) {
    QScopedPointer<SurfacePoint> pnt(m_model.point(lat, lon));
    if ( !pnt.isNull() )  return (pnt->GetLocalRadius());
    return (Distance());
  }


  /**
   * @brief Set the local normal vector to the intercept point normal
   *
   * This method will reassign the ShapeModel normal to the current intecept point
   * shape (which is a triangular plate) normal.  If an intercept point is not
   * defined, an error will ensue.
   *
   * @author 2014-02-14 Kris Becker
   */
  void NaifDskShape::setLocalNormalFromIntercept()  {

    // Sanity check
    if ( !hasIntersection() ) { // hasIntersection()  <==>  !m_intercept.isNull()
      QString mess = "Intercept point does not exist - cannot provide normal vector";
      throw IException(IException::Programmer, mess, _FILEINFO_);
    }

    // Got it, use the existing intercept point (plate) normal
    NaifVector norm(m_intercept->normal());
    setLocalNormal(norm[0], norm[1], norm[2]); // this also takes care of setHasLocalNormal(true);
    return;
  }


  /**
   * Indicates that this shape model is not from a DEM. Since this method
   * returns false for this class, the Camera class will not calculate the
   * local normal using neighbor points.
   *
   * @return bool Indicates that this is not a DEM shape model.
   */
  bool NaifDskShape::isDEM() const {
    return false;
  }


  /**
   * @brief Compute the normal for a local region of surface points
   *
   * This method will calculate the surface normal of an assumed very local
   * region of points.  This method is provided to fullfil the specs of the
   * ShapeModel class but this approach is not the most efficent means to
   * accomplish this for a pre-exising intercept point.  See
   * setLocalNormalFromIntercept() for this.
   *
   * The ShapeModel class makes the assumption that the four pixel corners of the
   * center intercept point forms a plane from which a surface normal can be
   * computed.  For the Naif DSK plate model, we have already identified the plate
   * (see m_intercept) from the DSK plate model (m_model) of the intercept point
   * that provides it directly.  That is what setLocalNormalFromIntercept()
   * provides.
   *
   * So, this implementation will compute the centroid of the neighboring points
   * and make a determination if it intercepts the current intercept plate as
   * defined by m_intercept - if it is valid.  If it does not exist or does not
   * intercept the plate, a new intercept point is computed and returned here.
   *
   * @author 2014-02-14 Kris Becker
   *
   * @param neighborPoints Input body-fixed points to compute normal for
   */
  void NaifDskShape::calculateLocalNormal(QVector<double *> neighborPoints) {
    // Sanity check
    if ( !hasIntersection() ) { // hasIntersection()  <==>  !m_intercept.isNull()
      QString mess = "Intercept point does not exist - cannot provide normal vector";
      throw IException(IException::Programmer, mess, _FILEINFO_);
    }

    setLocalNormalFromIntercept();
    return;
  }


  /** Return the surface normal of the ellipsoid as the default */
  void NaifDskShape::calculateDefaultNormal() {
    // ShapeModel (parent class) throws error if no intersection
     calculateSurfaceNormal();
  }


  /** Return the surface normal of the ellipsoid */
  void NaifDskShape::calculateSurfaceNormal() {
    // ShapeModel (parent class) throws error if no intersection
    QVector<double> norm = ellipsoidNormal(); 
    setNormal(std::vector<double>(norm.begin(), norm.end()));// this takes care of setHasNormal(true);
    return;
  }


  /**
   * @brief Compute the true surface normal vector of an ellipsoid
   *
   * This routine is used instead of the one provided by the ShapeModel
   * implementation.  This is primarly because
   * ShapeModel::calculateEllipsoidalSurfaceNormal() it is only suitable for a
   * spheroid. This implementation is intended for irregular bodies so we expect
   * triaxial ellipsoids.
   *
   * @author 2014-02-12 Kris Becker
   *
   * @return QVector<double> Normal vector at the intercept point relative to
   *                             the ellipsoid (not the plate model)
   */
  QVector<double> NaifDskShape::ellipsoidNormal()  {

    // Sanity check on state
    if ( !hasIntersection() ) {
       QString msg = "An intersection must be defined before computing the surface normal.";
       throw IException(IException::Programmer, msg, _FILEINFO_);
    }
    if ( !surfaceIntersection()->Valid() ) {
       QString msg = "The surface point intersection must be valid to compute the surface normal.";
       throw IException(IException::Programmer, msg, _FILEINFO_);
    }
    if (!hasValidTarget()) {
       QString msg = "A valid target must be defined before computing the surface normal.";
       throw IException(IException::Programmer, msg, _FILEINFO_);
    }

    // Get the coordinates of the current surface point
    SpiceDouble pB[3];
    surfaceIntersection()->ToNaifArray(pB);

    // Get the body radii and compute the true normal of the ellipsoid
    QVector<double> norm(3);
    // need a case for target == NULL
    std::vector<Distance> stdRadii = targetRadii();
    QVector<Distance> radii = QVector<Distance>(stdRadii.begin(), stdRadii.end());
    NaifStatus::CheckErrors();
    surfnm_c(radii[0].kilometers(), radii[1].kilometers(), radii[2].kilometers(),
             pB, &norm[0]);
    NaifStatus::CheckErrors();

    return (norm);
  }


  /** Returns a direct reference to the DSK plate model file interface */
  const NaifDskPlateModel &NaifDskShape::model() const {
    return (m_model);
  }


  /**
   * @brief Returns a pointer to the current intercept
   *
   * This method returns a const pointer to the intercept last computed from the
   * intersectSurface() call.  Note it is left to the caller to ensure it is
   * non-NULL.
   *
   * @author 2014-02-12 Kris Becker
   *
   * @return const Intercept* Pointer reference to the last computed intercept
   *                          point.  Could be NULL so check it!
   */
  const Intercept *NaifDskShape::intercept() const {
    return ( m_intercept.data() );
  }

}; // namespace Isis 
