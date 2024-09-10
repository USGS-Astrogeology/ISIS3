#include "BundleLidarRangeConstraint.h"

// Qt Library
#include <QDebug>

// Boost uBLAS
#include <boost/numeric/ublas/vector_proxy.hpp>
#include <boost/numeric/ublas/matrix_proxy.hpp>

// Isis Library
#include "Camera.h"
#include "CameraGroundMap.h"
#include "IsisBundleObservation.h"
#include "SpicePosition.h"

using namespace boost::numeric::ublas;

namespace Isis {

  /**
   * constructor
   *
   * @param lidarControlPoint input lidar control point
   * @param measure associated BundleMeasureQsp.
   */
  BundleLidarRangeConstraint::BundleLidarRangeConstraint(LidarControlPointQsp lidarControlPoint,
                                                         BundleMeasureQsp measure) {
    m_lidarControlPoint   = lidarControlPoint;
    m_simultaneousMeasure = measure;
    m_bundleObservation   = measure->parentBundleObservation();
    m_rangeObserved       = m_lidarControlPoint->range();

    if (m_rangeObserved <= 0) {
      std::string msg ="In BundleLidarRangeConstraint::BundleLidarRangeConstraint():"
                   "observed range for lidar point must be positive (Point Id: "
                   + measure->parentControlPoint()->id().toStdString() + ")\n.";
      throw IException(IException::Programmer, msg, _FILEINFO_);
    }

    m_rangeObservedSigma = m_lidarControlPoint->sigmaRange() * 0.001; // converting from m to km

    if (m_rangeObservedSigma <= 0) {
      std::string msg ="In BundleLidarRangeConstraint::BundleLidarRangeConstraint():"
                   "observed range sigma for lidar point must be positive (Point Id: "
                   + measure->parentControlPoint()->id().toStdString() + ")\n.";
      throw IException(IException::Programmer, msg, _FILEINFO_);
    }

    m_rangeObservedWeightSqrt = 1.0/m_rangeObservedSigma;
    m_adjustedSigma = 0.0;
    m_dX = m_dY = m_dZ = 0.0;

    // Check that the simultaneous image has an ISIS camera
    if (m_simultaneousMeasure->camera()->GetCameraType() == Camera::Csm) {
      std::string msg = "Cannot apply a Lidar range constraint to a CSM camera model"
                    "(Point Id: "
                    + measure->parentControlPoint()->id().toStdString() + ", Measure Serial:"
                    + measure->cubeSerialNumber().toStdString() + ").\n";
      throw IException(IException::Programmer, msg, _FILEINFO_);
    }

    // initialize computed range
    computeRange();
  }


  /**
   * Destructor
   */
  BundleLidarRangeConstraint::~BundleLidarRangeConstraint() {
  }


  /**
   * Copy constructor
   *
   * Constructs a BundleLidarRangeConstraint from another.
   *
   * @param src Source BundleLidarRangeConstraint to copy.
   */
  BundleLidarRangeConstraint::BundleLidarRangeConstraint(const BundleLidarRangeConstraint &src) {
    m_dX = src.m_dX;
    m_dY = src.m_dY;
    m_dZ = src.m_dZ;
    m_rangeObserved = src.m_rangeObserved;
    m_rangeComputed = src.m_rangeComputed;
    m_rangeObservedSigma = src.m_rangeObservedSigma;
    m_rangeObservedWeightSqrt = src.m_rangeObservedWeightSqrt;
    m_adjustedSigma = src.m_adjustedSigma;
    m_vtpv = src.m_vtpv;
  }


  /**
   * Assignment operator
   *
   * Assigns state of this BundleLidarRangeConstraint from another.
   *
   * @param src Source BundleLidarRangeConstraint to assign state from.
   *
   * @return    BundleLidarRangeConstraint& Returns reference to this
   *            BundleLidarRangeConstraint.
   */
  BundleLidarRangeConstraint &BundleLidarRangeConstraint::
      operator=(const BundleLidarRangeConstraint &src) {

    // Prevent self assignment
    if (this != &src) {
      m_dX = src.m_dX;
      m_dY = src.m_dY;
      m_dZ = src.m_dZ;
      m_rangeObserved = src.m_rangeObserved;
      m_rangeComputed = src.m_rangeComputed;
      m_rangeObservedSigma = src.m_rangeObservedSigma;
      m_rangeObservedWeightSqrt = src.m_rangeObservedWeightSqrt;
      m_adjustedSigma = src.m_adjustedSigma;
      m_vtpv = src.m_vtpv;
    }

    return *this;
  }


  /**
   * Compute range between spacecraft & lidar point on surface given the current values of the
   * spacecraft position & lidar point coordinates in the bundle adjustment. Steps are as follows...
   *
   * 1) set image to establish camera model for simultaneous measure
   * 2) get spacecraft & lidar point coordinates in body-fixed reference system
   * 3) compute & save dX, dY, dZ between spacecraft & lidar point
   * 4) compute range with dX, dY, dZ
   *
   */
  void BundleLidarRangeConstraint::computeRange() {

    // establish camera model for simultaneous measure
    m_simultaneousMeasure->setImage();

    // get spacecraft coordinates in J2000 reference system
    std::vector<double> camPositionJ2K
        = m_simultaneousMeasure->camera()->instrumentPosition()->Coordinate();

    // body rotation "ReferenceVector" rotates spacecraft coordinates from J2000 to body-fixed
    std::vector<double> camPositionBodyFixed
        = m_simultaneousMeasure->camera()->bodyRotation()->ReferenceVector(camPositionJ2K);

    // current body fixed XYZ coordinates of lidar control point
    SurfacePoint adjustedSurfacePoint = m_lidarControlPoint->GetAdjustedSurfacePoint();
    std::vector<double> pointBodyFixed(3);
    pointBodyFixed[0] = adjustedSurfacePoint.GetX().kilometers();
    pointBodyFixed[1] = adjustedSurfacePoint.GetY().kilometers();
    pointBodyFixed[2] = adjustedSurfacePoint.GetZ().kilometers();

    // compute and store dX, dY, dZ between body-fixed coordinates of spacecraft and lidar point
    m_dX = camPositionBodyFixed[0] - pointBodyFixed[0];
    m_dY = camPositionBodyFixed[1] - pointBodyFixed[1];
    m_dZ = camPositionBodyFixed[2] - pointBodyFixed[2];

    m_rangeComputed = sqrt(m_dX*m_dX+m_dY*m_dY+m_dZ*m_dZ);

    if (m_rangeComputed <= 0.0) {
      std::string msg = "In BundleLidarRangeConstraint::computeRange(): "
          "m_rangeComputed must be positive\n";
      throw IException(IException::Programmer, msg, _FILEINFO_);
    }
  }


  /**
   * Computes partial derivatives of range condition equation and adds contribution into the bundle
   * adjustment normal equation matrices.
   *
   * @param normalsMatrix Bundle Adjustment normal equations matrix.
   * @param N22 Normal equation matrix for the point.
   * @param N12 Normal equations block between image and point.
   * @param n1 Right hand side vector for the images and target body.
   * @param n2 Right hand side vector for the point.
   *
   * @return bool success.
   */
  bool BundleLidarRangeConstraint::applyConstraint(SparseBlockMatrix &normalsMatrix,
                                                   LinearAlgebra::MatrixUpperTriangular& N22,
                                                   SparseBlockColumnMatrix& N12,
                                                   LinearAlgebra::VectorCompressed& n1,
                                                   LinearAlgebra::Vector& n2) {

    if (m_simultaneousMeasure->isRejected()) {
      return false;
    }

    // establish camera model for simultaneous measure
    m_simultaneousMeasure->setImage();

    // time of current location of simultaneous measure
    double scaledTime = m_simultaneousMeasure->camera()->instrumentPosition()->scaledTime();

    // current body fixed XYZ coordinates of lidar control point
    SurfacePoint adjustedSurfacePoint = m_lidarControlPoint->GetAdjustedSurfacePoint();

    static LinearAlgebra::Matrix coeff_range_image;          //!< Partials w/r to image position
    static LinearAlgebra::Matrix coeff_range_point3D(1,3);   //!< Partials w/r to lidar point
    static LinearAlgebra::Vector coeff_range_RHS(1);         //!< Right hand side of normals

    // index into normal equations for this measure
    int positionBlockIndex = m_simultaneousMeasure->observationIndex();

    // resize coeff_range_image matrix if necessary
    IsisBundleObservationQsp isisObservation = m_bundleObservation.dynamicCast<IsisBundleObservation>();
    if (!isisObservation) {
      std::string msg = "Failed to cast BundleObservation to IsisBundleObservation when applying "
                    "lidar constraint (Point Id: "
                    + m_simultaneousMeasure->parentControlPoint()->id().toStdString() + ", Measure Serial:"
                    + m_simultaneousMeasure->cubeSerialNumber().toStdString() + ").\n";
      throw IException(IException::Programmer, msg, _FILEINFO_);
    }
    int numPositionParameters = isisObservation->numberPositionParameters();
    if ((int) coeff_range_image.size2() != numPositionParameters) {
      coeff_range_image.resize(1, numPositionParameters, false);
    }

    coeff_range_image.clear();
    coeff_range_point3D.clear();
    coeff_range_RHS.clear();

    // get matrix that rotates spacecraft from J2000 to body-fixed
    std::vector<double> matrixTargetToJ2K =
        m_simultaneousMeasure->camera()->bodyRotation()->Matrix();

    double m11 = matrixTargetToJ2K[0];
    double m12 = matrixTargetToJ2K[1];
    double m13 = matrixTargetToJ2K[2];
    double m21 = matrixTargetToJ2K[3];
    double m22 = matrixTargetToJ2K[4];
    double m23 = matrixTargetToJ2K[5];
    double m31 = matrixTargetToJ2K[6];
    double m32 = matrixTargetToJ2K[7];
    double m33 = matrixTargetToJ2K[8];

    // a1, a2, a3 are auxiliary variables used in partial derivatives
    double a1 = -(m11*m_dX + m21*m_dY + m31*m_dZ)/m_rangeComputed;
    double a2 = -(m12*m_dX + m22*m_dY + m32*m_dZ)/m_rangeComputed;
    double a3 = -(m13*m_dX + m23*m_dY + m33*m_dZ)/m_rangeComputed;

    // partials with respect to camera body fixed X polynomial coefficients
    double c = 1.0;
    int numPositionParametersPerCoordinate = numPositionParameters/3;
    int index = 0;
    for (int i = 0; i < numPositionParametersPerCoordinate; i++) {
      coeff_range_image(0,index) = a1*c;
      c *= scaledTime;
      index++;
    }

    // partials with respect to camera body fixed Y polynomial coefficients
    c = 1.0;
    for (int i = 0; i < numPositionParametersPerCoordinate; i++) {
      coeff_range_image(0,index) = a2*c;
      c *= scaledTime;
      index++;
    }

    // partials with respect to camera body fixed Z polynomial coefficients
    c = 1.0;
    for (int i = 0; i < numPositionParametersPerCoordinate; i++) {
      coeff_range_image(0,index) = a3*c;
      c *= scaledTime;
      index++;
    }

    // partials w/r to point
    double lat    = adjustedSurfacePoint.GetLatitude().radians();
    double lon    = adjustedSurfacePoint.GetLongitude().radians();
    double radius = adjustedSurfacePoint.GetLocalRadius().kilometers();

    double sinlat = sin(lat);
    double coslat = cos(lat);
    double sinlon = sin(lon);
    double coslon = cos(lon);

    // partials with respect to point latitude, longitude, and radius
    coeff_range_point3D(0,0)
        = radius*(-sinlat*coslon*m_dX - sinlat*sinlon*m_dY + coslat*m_dZ)/m_rangeComputed;
    coeff_range_point3D(0,1)
        = radius*(-coslat*sinlon*m_dX + coslat*coslon*m_dY)/m_rangeComputed;
    coeff_range_point3D(0,2)
        = (coslat*coslon*m_dX + coslat*sinlon*m_dY + sinlat*m_dZ)/m_rangeComputed;

    // right hand side (observed distance - computed distance)
    coeff_range_RHS(0) = m_rangeObserved - m_rangeComputed;

    // multiply coefficients by observation weight
    // TODO: explain negative sign here
    coeff_range_image   *= -m_rangeObservedWeightSqrt;
    coeff_range_point3D *= -m_rangeObservedWeightSqrt;
    coeff_range_RHS     *= m_rangeObservedWeightSqrt;

    // add range condition contribution to N11 portion of normal equations matrix
    matrix_range<LinearAlgebra::Matrix> normal_range(
          *(*normalsMatrix[positionBlockIndex])[positionBlockIndex],
          range(0, coeff_range_image.size2()),
          range(0, coeff_range_image.size2()));

    normal_range += prod(trans(coeff_range_image), coeff_range_image);

    // add range condition contribution to N12 portion of normal equations matrix
    matrix_range<LinearAlgebra::Matrix> N12_range(
          *N12[positionBlockIndex],
          range(0, coeff_range_image.size2()),
          range(0, coeff_range_point3D.size2()));

    N12_range += prod(trans(coeff_range_image), coeff_range_point3D);

    // contribution to n1 vector
    int startColumn = normalsMatrix.at(positionBlockIndex)->startColumn();
    vector_range<LinearAlgebra::VectorCompressed >
        n1_range (n1, range (startColumn, startColumn+coeff_range_image.size2()));

    n1_range += prod(trans(coeff_range_image), coeff_range_RHS);

    // form N22
    N22 += prod(trans(coeff_range_point3D), coeff_range_point3D);

    // contribution to n2 vector
    n2 += prod(trans(coeff_range_point3D), coeff_range_RHS);

    return true;
  }


  /**
   * Return observed lidar range.
   *
   * @return double Observed lidar range.
   *
   */
  double BundleLidarRangeConstraint::rangeObserved() {
    return m_rangeObserved;
  }


  /**
   * Return computed lidar range.
   *
   * @return double Computed lidar range.
   *
   */
  double BundleLidarRangeConstraint::rangeComputed() {
    return m_rangeComputed;
  }


  /**
   * Return sigma of range observation.
   *
   * @return double Sigma of range observation.
   *
   */
  double BundleLidarRangeConstraint::rangeObservedSigma() {
    return m_rangeObservedSigma;
  }


  /**
   * Return adjusted sigma of range observation.
   *
   * @return double Adjusted sigma of range observation.
   *
   */
  double BundleLidarRangeConstraint::rangeAdjustedSigma() {
    return m_adjustedSigma;
  }


  /**
   * Return current value of weighted sum-of-squares of residual.
   *
   * @return double Current value of weighted sum-of-squares of residual.
   *
   */
  double BundleLidarRangeConstraint::vtpv() {
    if (m_simultaneousMeasure->isRejected()) {
      return 0.0;
    }

    // update computed range
    // Note that this prepares us for the next iteration of the bundle adjustment
    computeRange();

    // residual
    double v = m_rangeObserved - m_rangeComputed;

    // contribution to weighted sum of squares of residuals
    m_vtpv = v * v * m_rangeObservedWeightSqrt * m_rangeObservedWeightSqrt;

    return m_vtpv;
  }


  /**
   * TODO: to be completed.
   * Error propagation for adjusted range sigma using distance equation.
   *
   * @todo need more documentation on technical aspects of approach.
   */
  void BundleLidarRangeConstraint::errorPropagation() {

  }


  /**
   * Creates & returns formatted QString for lidar range constraint to output to bundleout_lidar.csv
   * file.
   *
   * @param errorProp Bool indicating if error propagation is ON in the bundle adjustment.
   *
   * @return QString Formatted QString summarizing lidar range constraint to output to
   *                 bundleout_lidar.csv.
   */
  QString BundleLidarRangeConstraint::formatBundleOutputString(bool errorProp) {

    QString outstr;

    FileName imageFileName = m_simultaneousMeasure->parentBundleObservation()->imageNames().at(0).toStdString();
    QString imageName = QString::fromStdString(imageFileName.baseName());

    //                     measured   apriori   adjusted    adjusted
    //                      range      sigma     range       sigma     residual
    // point id  image       (km)       (km)      (km)        (km)       (km)

    if (errorProp) {
      outstr = QString("%1,%2,%3,%4,%5,%6,%7\n").arg(m_lidarControlPoint->GetId(), 16)
                                          .arg(imageName, 16)
                                          .arg(m_rangeObserved, -16, 'f', 8)
                                          .arg(m_rangeObservedSigma, -16, 'f', 2)
                                          .arg(m_rangeComputed, -16, 'f', 8)
                                          .arg(m_adjustedSigma, -16, 'f', 6)
                                          .arg(m_rangeObserved - m_rangeComputed, -16, 'f', 8);
    }
    else {
      outstr = QString("%1,%2,%3,%4,%5,%6\n").arg(m_lidarControlPoint->GetId())
                                          .arg(imageName)
                                          .arg(m_rangeObserved)
                                          .arg(m_rangeObservedSigma)
                                          .arg(m_rangeComputed)
                                          .arg(m_rangeObserved - m_rangeComputed);
    }

    return outstr;
  }
}