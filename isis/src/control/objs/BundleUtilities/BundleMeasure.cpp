/** This is free and unencumbered software released into the public domain.

The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */

#include "BundleMeasure.h"
#include "BundleObservation.h"
#include "BundleObservationSolveSettings.h"
#include "Camera.h"
#include "IException.h"

#include "ControlMeasure.h"

namespace Isis {

  /**
   * Constructor
   *
   * Constructs a BundleMeasure from a ControlMeasure with the passed BundleControlPoint as its
   * parent control point
   *
   * @param controlMeasure Pointer to the ControlMeasure to store
   * @param bundleControlPoint Pointer to the BundleControlPoint that contains this
   *                           BundleMeasure
   */
  BundleMeasure::BundleMeasure(ControlMeasure *controlMeasure,
                               BundleControlPoint *bundleControlPoint) {
    m_controlMeasure = controlMeasure;
    m_parentControlPoint = bundleControlPoint;
    m_normalsPositionBlockIndex = -1;
    m_normalsPointingBlockIndex = -1;
  }


  /**
   * Destructor
   */
  BundleMeasure::~BundleMeasure() {
  }


  /**
   * Copy constructor
   *
   * Constructs a BundleMeasure from another BundleMeasure
   *
   * @param src The source BundleMeasure to copy
   */
  BundleMeasure::BundleMeasure(const BundleMeasure &src) {
    m_controlMeasure = src.m_controlMeasure;
    m_parentControlPoint = src.m_parentControlPoint;
    m_parentBundleImage = src.m_parentBundleImage;
    m_parentObservation = src.m_parentObservation;
    m_normalsPositionBlockIndex = src.m_normalsPositionBlockIndex;
    m_normalsPointingBlockIndex = src.m_normalsPointingBlockIndex;
  }


  /**
   * Assignment operator
   *
   * Assigns the state of this BundleMeasure from another BundleMeasure
   *
   * @param src The source BundleMeasure to assign state from
   *
   * @return @b BundleMeasure& Returns a reference to this BundleMeasure
   */
  BundleMeasure &BundleMeasure::operator=(const BundleMeasure &src) {
    // Prevent self assignment
    if (this != &src) {
      m_controlMeasure = src.m_controlMeasure;
      m_parentControlPoint = src.m_parentControlPoint;
      m_parentBundleImage = src.m_parentBundleImage;
      m_parentObservation = src.m_parentObservation;
      m_normalsPositionBlockIndex = src.m_normalsPositionBlockIndex;
      m_normalsPointingBlockIndex = src.m_normalsPointingBlockIndex;
    }

    return *this;
  }


  /**
   * Sets the parent bundle observation
   *
   * @param observation Pointer to the parent BundleObservation
   */
  void BundleMeasure::setParentObservation(QSharedPointer<BundleObservation> observation) {
    m_parentObservation = observation;
  }


  /**
   * Sets the parent bundle image
   *
   * @param image Pointer to the parent BundleImage
   */
  void BundleMeasure::setParentImage(QSharedPointer<BundleImage> image) {
    m_parentBundleImage = image;
  }


  /**
   * Sets the BundleMeasure's status to rejected or not rejected.
   *
   * @param reject True will set the BundleMeasure to rejected.
   *
   * @see ControlMeasure::SetRejected(bool reject)
   */
  void BundleMeasure::setRejected(bool reject) {
    m_controlMeasure->SetRejected(reject);
  }


  /**
   * Sets the BundleMeasure's status to rejected or not rejected.
   *
   * @param reject True will set the BundleMeasure to rejected.
   *
   * @see ControlMeasure::SetRejected(bool reject)
   */
  void BundleMeasure::setImage() {
    m_controlMeasure->Camera()->SetImage(m_controlMeasure->GetSample(),
                                         m_controlMeasure->GetLine());
  }


  /**
     * Sets block index into normal equations for position piecewise polynomial segment.
     *
     * @param index normal equations matrix block index.
     *
     */
  void BundleMeasure::setNormalsPositionBlockIndex(int index) {
    m_normalsPositionBlockIndex = index;
  }


  /**
     * Sets block index into normal equations for pointing piecewise polynomial segment.
   *
   * @param index normal equations matrix block index.
   *
   */
  void BundleMeasure::setNormalsPointingBlockIndex(int index) {
    m_normalsPointingBlockIndex = index;
  }


  /**
   * Accesses block index into normal equations matrix of position piecewise polynomial segment.
   *
   * @return int block index into normal equations matrix of position piecewise polynomial.
   *                segment
   */
  int BundleMeasure::positionNormalsBlockIndex() const {
    return m_normalsPositionBlockIndex;
  }


  /**
   * Accesses block index into normal equations matrix of pointing piecewise polynomial segment
   *
   * @return int block index into normal equations matrix of pointing piecewise polynomial
   *                segment
   */
  int BundleMeasure::pointingNormalsBlockIndex() const {
    return m_normalsPointingBlockIndex;
  }


  /**
   * Determines whether or not this BundleMeasure is rejected
   *
   * @return @b bool Returns a boolean indicating whether this BundleMeasure is rejected
   */
  bool BundleMeasure::isRejected() const {
    return m_controlMeasure->IsRejected();
  }


  /**
   * Accesses the associated camera for this bundle measure
   *
   * @see ControlMeasure::camera()
   *
   * @return @b Camera* Returns a pointer to the camera associated with this bundle measure
   */
  Camera *BundleMeasure::camera() const {
    return m_controlMeasure->Camera();
  }


  /**
   * Accesses the parent BundleControlPoint for this bundle measure
   *
   * @return @b BundleControlPoint* Returns a pointer to the parent BundleControlPoint
   */
  BundleControlPoint *BundleMeasure::parentControlPoint() {
    return m_parentControlPoint;
  }


  /**
   * Access the parent BundleImage for this bundle measure
   *
   * @return @b QSharedPointer<BundleImage> Returns a pointer to the parent BundleImage
   */
  QSharedPointer<BundleImage> BundleMeasure::parentBundleImage() {
    return m_parentBundleImage;
  }


  /**
   * Accesses the parent BundleObservation for this bundle measure
   *
   * @return @b QSharedPointer<BundleObservation> Returns a pointer to the parent BundleObservation
   */
  QSharedPointer<BundleObservation> BundleMeasure::parentBundleObservation() {
    return m_parentObservation;
  }


  /**
   * Accesses the parent observation's solve settings
   *
   * @see BundleObservation::solveSettings()
   *
   * @return @b const QSharedPointer<BundleObservationSolveSettings> Returns a const pointer to
   *     the BundleObservationSolveSettings for the parent BundleObservation
   *
   * @throws IException::Programmer "In BundleMeasure::observationSolveSettings:
   *                                 parent observation has not been set."
   */
  const QSharedPointer<BundleObservationSolveSettings> BundleMeasure::observationSolveSettings() {
    if (!m_parentObservation) {
      std::string msg = "In BundleMeasure::observationSolveSettings: "
                    "parent observation has not been set.\n";
      throw IException(IException::Programmer, msg, _FILEINFO_);
    }
    return m_parentObservation->solveSettings();
  }


  /**
   * Accesses the current sample measurement for this control measure
   *
   * @see ControlMeasure::GetSample()
   *
   * @return @b double Returns the sample measurement for this control measure
   */
  double BundleMeasure::sample() const {
    return m_controlMeasure->GetSample();
  }


  /**
   * Accesses the current line measurement for this control measure
   *
   * @see ControlMeasure::GetLine()
   *
   * @return double Returns the line measurement for this control measure
   */
  double BundleMeasure::line() const {
    return m_controlMeasure->GetLine();
  }


  /**
   * Accesses the sample residual for this control measure
   *
   * @see ControlMeasure::GetSampleResidual()
   *
   * @return @b double Returns the sample residual
   */
  double BundleMeasure::sampleResidual() const {
    return m_controlMeasure->GetSampleResidual();
  }


  /**
   * Accesses the line residual for this control measure
   *
   * @see ControlMeasure::GetLineResidual()
   *
   * @return @b double Returns the line residual
   */
  double BundleMeasure::lineResidual() const {
    return m_controlMeasure->GetLineResidual();
  }


  /**
   * Accesses the focal plane x-coordinate residual in millimeters
   *
   * @return double Returns the focal plane x-coordinate residual in millimeters
   */
  double BundleMeasure::xFocalPlaneResidual() const {
    return m_xFocalPlaneResidual;
  }


  /**
   * Accesses the focal plane y-coordinate residual in millimeters
   *
   * @return double Returns the focal plane y-coordinate residual in millimeters
   */
  double BundleMeasure::yFocalPlaneResidual() const {
    return m_yFocalPlaneResidual;
  }


  /**
   * Accesses the measure sigma
   *
   * @return double measure sigma
   */
  double BundleMeasure::sigma() const {
    return m_sigma;
  }


  /**
   * Accesses sqrt of measure weight for bundle
   *
   * @return double sqrt of measure weight
   */
  double BundleMeasure::weightSqrt() const {
    return m_weightSqrt;
  }


  /**
   * Accesses measure weight for bundle
   *
   * @return double measure weight
   */
  double BundleMeasure::weight() const {
    return m_weightSqrt*m_weightSqrt;
  }


  /**
   * Accesses the residual magnitude for this control measure
   *
   * @see ControlMeasure::GetResidualMagnitude()
   *
   * @return @b double Returns the residual magnitude
   */
  double BundleMeasure::residualMagnitude() const {
    return m_controlMeasure->GetResidualMagnitude();
  }


  /**
   * Accesses the serial number of the cube containing this control measure
   *
   * @see ControlMeasure::GetCubeSerialNumber()
   *
   * @return @b QString Returns the serial number of the cube that contains this control measure
   */
  QString BundleMeasure::cubeSerialNumber() const {
    return m_controlMeasure->GetCubeSerialNumber();
  }


  /**
   * Accesses the computed focal plane x value for this control measure
   *
   * @see ControlMeasure::GetFocalPlaneComputedX()
   *
   * @return @b double Returns the computed focal plane x value
   */
  double BundleMeasure::focalPlaneComputedX() const {
    return m_controlMeasure->GetFocalPlaneComputedX();
  }


  /**
   * Accesses the computed focal plane y value for this control measure
   *
   * @see ControlMeasure::GetFocalPlaneComputedY()
   *
   * @return @b double Returns the computed focal plane y value
   */
   double BundleMeasure::focalPlaneComputedY() const {
     return m_controlMeasure->GetFocalPlaneComputedY();
   }


  /**
   * Accesses the measured focal plane x value for this control measure //TODO verify?
   *
   * @see ControlMeasure::GetFocalPlaneMeasuredX()
   *
   * @return @b double Returns the measured focal plane x value
   */
  double BundleMeasure::focalPlaneMeasuredX() const {
    return m_controlMeasure->GetFocalPlaneMeasuredX();
  }


  /**
   * Accesses the measured focal plane y value for this control measure  //TODO verify?
   *
   * @see ControlMeasure::GetFocalPlaneMeasuredY()
   *
   * @return @b double Returns the measured focal plane y value
   */
  double BundleMeasure::focalPlaneMeasuredY() const {
    return m_controlMeasure->GetFocalPlaneMeasuredY();
  }


  /**
   * Computes and sets measure focal plane residuals in millimeters.
   *
   */
  void BundleMeasure::setFocalPlaneResidualsMillimeters() {
    m_xFocalPlaneResidual = m_controlMeasure->GetFocalPlaneMeasuredX() -
                            m_controlMeasure->GetFocalPlaneComputedX();

    m_yFocalPlaneResidual = m_controlMeasure->GetFocalPlaneMeasuredY() -
                            m_controlMeasure->GetFocalPlaneComputedY();
  }


  /**
   * Sets sigma (i.e. standard deviation or uncertainty) of raw measure in mm and sqrt of weight for bundle
   *
   * @param double sigma
   *
   * TODO: what if camera has been subsampled, is pixel pitch computation still valid?
   *
   */
  void BundleMeasure::setSigma(double sigmaMultiplier) {
    // TODO fix for CSM
    m_sigma = sigmaMultiplier * m_controlMeasure->Camera()->PixelPitch();

    if (m_sigma <= 0.0) {
      std::string msg = "In BundleMeasure::setMeasureSigma(): m_measureSigma must be positive\n";
      throw IException(IException::Programmer, msg, _FILEINFO_);
    }

    m_weightSqrt = 1.0/m_sigma;
  }


  /**
   * Accesses the observation index for the parent observation
   *
   * @see BundleObservation::index()
   *
   * @return @b int Returns the observation index of the parent observation
   *
   * @throws IException::Programmer "In BundleMeasure::observationIndex:
   *                                 parent observation has not been set."
   */
  int BundleMeasure::observationIndex() const {
    if (!m_parentObservation) {
      std::string msg = "In BundleMeasure::observationIndex: "
                    "parent observation has not been set.\n";
      throw IException(IException::Programmer, msg, _FILEINFO_);
    }
    return m_parentObservation->index();
  }

}
