/** This is free and unencumbered software released into the public domain.

The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */

#include "ControlPoint.h"

#include <boost/numeric/ublas/symmetric.hpp>
#include <boost/numeric/ublas/io.hpp>

#include <QDebug>
#include <QHash>
#include <QString>
#include <QStringList>

#include "Application.h"
#include "CameraDetectorMap.h"
#include "CameraDistortionMap.h"
#include "CameraFocalPlaneMap.h"
#include "CameraGroundMap.h"
#include "ControlMeasure.h"
#include "ControlMeasureLogData.h"
#include "ControlNet.h"
#include "Cube.h"
#include "IString.h"
#include "Latitude.h"
#include "Longitude.h"
#include "PvlObject.h"
#include "SerialNumberList.h"
#include "SpecialPixel.h"
#include "Statistics.h"

using boost::numeric::ublas::symmetric_matrix;
using boost::numeric::ublas::upper;
using namespace std;

namespace Isis {
  /**
   * Construct a control point
   *
   * @author tsucharski (5/5/2010)
   *
   */
  ControlPoint::ControlPoint() : invalid(false) {
    measures = NULL;
    cubeSerials = NULL;

    measures = new QHash< QString, ControlMeasure * >;
    cubeSerials = new QStringList;

    type = Free;
    dateTime = "";
    editLock = false;
    ignore = false;
    jigsawRejected = false;
    referenceExplicitlySet = false;
    aprioriSurfacePointSource = SurfacePointSource::None;
    aprioriRadiusSource = RadiusSource::None;
    parentNetwork = NULL;
    referenceMeasure = NULL;
    numberOfRejectedMeasures = 0;
    constraintStatus.reset();
  }


  /**
   * Copy the given control point into this instance.
   *
   * @param other The control point to duplicate
   */
  ControlPoint::ControlPoint(const ControlPoint &other) {
    measures = NULL;
    cubeSerials = NULL;
    referenceMeasure = NULL;
    parentNetwork = NULL;

    editLock = false;

    measures = new QHash< QString, ControlMeasure * >;
    cubeSerials = new QStringList;

    QListIterator<QString> i(*other.cubeSerials);
    while (i.hasNext()) {
      QString sn = i.next();

      const ControlMeasure *otherCm = other.GetMeasure(sn);
      ControlMeasure *newMeasure = new ControlMeasure(*otherCm);
      AddMeasure(newMeasure);

      if (other.referenceMeasure == otherCm) {
        SetRefMeasure(newMeasure);
      }
    }

    id = other.id;
    chooserName = other.chooserName;
    dateTime = other.dateTime;
    type = other.type;
    invalid = other.invalid;
    editLock = other.editLock;
    jigsawRejected = other.jigsawRejected;
    referenceExplicitlySet = other.referenceExplicitlySet;
    ignore = other.ignore;
    aprioriSurfacePointSource = other.aprioriSurfacePointSource;
    aprioriSurfacePointSourceFile = other.aprioriSurfacePointSourceFile;
    aprioriRadiusSource = other.aprioriRadiusSource;
    aprioriRadiusSourceFile = other.aprioriRadiusSourceFile;
    aprioriSurfacePoint = other.aprioriSurfacePoint;
    adjustedSurfacePoint = other.adjustedSurfacePoint;
    numberOfRejectedMeasures = other.numberOfRejectedMeasures;
    constraintStatus = other.constraintStatus;
  }


  /**
   * Construct a control point with given Id
   *
   * @param id Control Point Id
   */
  ControlPoint::ControlPoint(const QString &newId) : invalid(false) {
    parentNetwork = NULL;
    measures = NULL;
    referenceMeasure = NULL;
    numberOfRejectedMeasures = 0;
    measures = new QHash< QString, ControlMeasure * >;
    cubeSerials = new QStringList;

    id = newId;
    type = Free;
    editLock = false;
    jigsawRejected = false;
    referenceExplicitlySet = false;
    ignore = false;
    aprioriSurfacePointSource = SurfacePointSource::None;
    aprioriRadiusSource = RadiusSource::None;
    constraintStatus.reset();
  }

  /**
   * This destroys the current instance and cleans up any and all allocated
   *    memory.
   */
  ControlPoint::~ControlPoint() {
    if (measures != NULL) {
      QList< QString > keys = measures->keys();
      for (int i = 0; i < keys.size(); i++) {
        delete(*measures)[keys[i]];
        (*measures)[keys[i]] = NULL;
      }

      delete measures;
      measures = NULL;
    }

    if (cubeSerials) {
      delete cubeSerials;
      cubeSerials = NULL;
    }

    referenceMeasure = NULL;
  }

  /**
  * Loads the PvlObject into a ControlPoint
  *
  * @param p PvlObject containing ControlPoint information
  * @param forceBuild Allows invalid Control Measures to be added to this
  *                   Control Point
  *
  * @throws Isis::iException::User - Invalid Point Type
  * @throws Isis::iException::User - Unable to add ControlMeasure to Control
  *                                  Point
  *
  * @history 2008-06-18  Tracie Sucharski/Jeannie Walldren, Fixed bug with
  *                         checking for "True" vs "true", change to
  *                         lower case for comparison.
  * @history 2009-12-29  Tracie Sucharski - Added new ControlPoint information.
  * @history 2010-01-13  Tracie Sucharski - Changed from Set methods to simply
  *                         setting private variables to increase speed?
  * @history 2010-07-30  Tracie Sucharski, Updated for changes made after
  *                         additional working sessions for Control network
  *                         design.
  * @history 2010-09-01  Tracie Sucharski, Add checks for AprioriLatLonSource
  *                         AprioriLatLonSourceFile.  If there are
  *                         AprioriSigmas,but no AprioriXYZ, use the XYZ values.
  * @history 2010-09-15 Tracie Sucharski, It was decided after mtg with
  *                         Debbie, Stuart, Ken and Tracie that ControlPoint
  *                         will only function with x/y/z, not lat/lon/radius.
  *                         It will be the responsibility of the application
  *                         or class using ControlPoint to set up a
  *                         SurfacePoint object to do conversions between x/y/z
  *                         and lat/lon/radius.
  *                         So... remove all conversion methods from this
  *                         class.
  *                         It was also decided that when importing old
  *                         networks that contain Sigmas, the sigmas will not
  *                         be imported , due to conflicts with the units of
  *                         the sigmas,we cannot get accurate x,y,z sigmas from
  *                         the lat,lon,radius sigmas without the covariance
  *                         matrix.
  * @history 2010-09-28 Tracie Sucharski, Added back the conversion methods
  *                         from lat,lon,radius to x,y,z only for the point,
  *                         since that is what most applications need.
  * @history 2010-12-02 Debbie A. Cook, Added units to
  *                         SurfacePoint.SetSpherical calls.
  * @history 2011-03-12 Debbie A. Cook, Added targetRadius to do conversions
  */
  void ControlPoint::Load(PvlObject &p) {

  }


  /**
   * Add a measurement to the control point, taking ownership of the measure in
   * the process.
   *
   * @param measure The ControlMeasure to add
   */
  void ControlPoint::Add(ControlMeasure *measure) {
    PointModified();
    AddMeasure(measure);
  }


  /**
   * Do the actual work of adding a measure to this point, without changing
   *   any extra data.
   */
  void ControlPoint::AddMeasure(ControlMeasure *measure) {
    // Make sure measure is unique
    foreach(ControlMeasure * m, measures->values()) {
      if (m->GetCubeSerialNumber() == measure->GetCubeSerialNumber()) {
        std::string msg = "The SerialNumber is not unique. A measure with "
            "serial number [" + measure->GetCubeSerialNumber().toStdString() + "] already "
            "exists for ControlPoint [" + GetId().toStdString() + "]";
        throw IException(IException::Programmer, msg, _FILEINFO_);
      }
    }

    if (!measures->size()) {
      referenceMeasure = measure;
    }
    else if (referenceMeasure->IsIgnored() && !measure->IsIgnored() &&
        !IsReferenceExplicit() && !IsEditLocked()) {
      // The current "implicit" reference is ignored, but this new measure
      // isn't, and the point is not edit locked, so make this measure the new
      // reference
      referenceMeasure = measure;
    }

    measure->parentPoint = this;
    QString newSerial = measure->GetCubeSerialNumber();
    measures->insert(newSerial, measure);
    cubeSerials->append(newSerial);

    // notify parent network if we have one
    if (parentNetwork) {
      parentNetwork->measureAdded(measure);
      parentNetwork->emitNetworkStructureModified();
    }
  }


  /**
   * Throws an exception if none of the point's measures have the given serial
   * number.  It is common to ensure that a measure exists before taking some
   * action.
   *
   * @param sn The serial number of the measure to validate
   */
  void ControlPoint::ValidateMeasure(QString serialNumber) const {
    if (!measures->contains(serialNumber)) {
      std::string msg = "No measure with serial number [" + serialNumber.toStdString() +
          "] is owned by this point";
      throw IException(IException::Programmer, msg, _FILEINFO_);
    }
  }


  /**
   * Remove a measurement from the control point, deleting reference measure
   * is allowed.
   *
   * @param serialNumber The serial number of the measure to delete
   */
  int ControlPoint::Delete(QString serialNumber) {
    ValidateMeasure(serialNumber);
    ControlMeasure *cm = (*measures)[serialNumber];

    // notify parent network of the change
    if (parentNetwork) {
      parentNetwork->measureDeleted(cm);

      if (!IsIgnored() && !cm->IsIgnored()) {
        parentNetwork->emitNetworkStructureModified();
      }
    }

    if (cm->IsEditLocked()) {
      return ControlMeasure::MeasureLocked;
    }

    // remove measure from the point's data structures
    measures->remove(serialNumber);
    cubeSerials->removeAt(cubeSerials->indexOf(serialNumber));

    // update the reference measure
    if (cubeSerials->size()) {
      if (referenceMeasure == cm) {
        referenceMeasure = (*measures)[cubeSerials->at(0)];
        referenceExplicitlySet = false;
      }
    }
    else {
      referenceMeasure = NULL;
    }

    delete cm;
    cm = NULL;

    PointModified();

    return ControlMeasure::Success;
  }


  /**
   * This method is a wrapper to emit the measureModified() signal in the parent network
   * is called whenever a change is made to a Control Measure.
   *
   * @param measure The ControlMeasure* that was modified.
   * @param type The ControlMeasure::ModType indicating which modification occured.
   * @param oldValue The oldValue before the change.
   * @param newValue The new value that the change incorporated.
   */
  void ControlPoint::emitMeasureModified(ControlMeasure *measure, ControlMeasure::ModType modType, QVariant oldValue, QVariant newValue) {
    if (parentNetwork) {
      parentNetwork->emitMeasureModified(measure, modType, oldValue, newValue);
    }
  }


  /**
   * Remove a measurement from the control point, deleting reference measure
   * is allowed.
   *
   * @param measure The measure to delete
   */
  int ControlPoint::Delete(ControlMeasure *measure) {
    return Delete(measure->GetCubeSerialNumber());
  }


  /**
   * Remove a measurement from the control point, deleting reference measure
   * is allowed.
   *
   * @param index The index of the control measure to delete
   */
  int ControlPoint::Delete(int index) {
    if (index < 0 || index >= cubeSerials->size()) {
      std::string msg = "index [" + toString(index) + "] out of bounds";
      throw IException(IException::Programmer, msg, _FILEINFO_);
    }

    return Delete(cubeSerials->at(index));
  }


  /**
   * Reset all the Apriori info to defaults
   *
   * @author Sharmila Prasad (10/22/2010)
   */
  ControlPoint::Status ControlPoint::ResetApriori() {
    if (IsEditLocked()) {
      return PointLocked;
    }

    aprioriSurfacePointSource = SurfacePointSource::None;
    aprioriSurfacePointSourceFile    = "";
    aprioriRadiusSource     = RadiusSource::None;
    aprioriRadiusSourceFile = "";

    aprioriSurfacePoint = SurfacePoint();
    constraintStatus.reset();

    return Success;
  }


  /**
   * Get a control measure based on its cube's serial number.
   *
   * @param serialNumber serial number of measure to get
   * @returns control measure with matching serial number
   */
  ControlMeasure *ControlPoint::GetMeasure(QString serialNumber) {
    ValidateMeasure(serialNumber);
    return (*measures)[serialNumber];
  }


  /**
   * Get a control measure based on its cube's serial number.
   *
   * @param serialNumber serial number of measure to get
   * @returns const control measure with matching serial number
   */
  const ControlMeasure *ControlPoint::GetMeasure(QString serialNumber) const {
    ValidateMeasure(serialNumber);
    return measures->value(serialNumber);
  }


  const ControlMeasure *ControlPoint::GetMeasure(int index) const {
    if (index < 0 || index >= cubeSerials->size()) {
      std::string msg = "Index [" + toString(index) + "] out of range";
      throw IException(IException::Programmer, msg, _FILEINFO_);
    }

    return GetMeasure(cubeSerials->at(index));
  }


  ControlMeasure *ControlPoint::GetMeasure(int index) {
    if (index < 0 || index >= cubeSerials->size()) {
      std::string msg = "Index [" + toString(index) + "] out of range";
      throw IException(IException::Programmer, msg, _FILEINFO_);
    }

    return GetMeasure(cubeSerials->at(index));
  }


  /**
   * Checks to see if a reference measure is set.
   *
   * @returns bool True if a reference measure is set.
   */
  bool ControlPoint::HasRefMeasure() const {
    return !(referenceMeasure == NULL);
  }


  /**
   * Get the reference control measure.
   *
   * @returns const reference measure for this point
   */
  const ControlMeasure *ControlPoint::GetRefMeasure() const {
    if (!HasRefMeasure()) {
      std::string msg = "Control point [" + GetId().toStdString() + "] has no reference measure!";
      throw IException(IException::Programmer, msg, _FILEINFO_);
    }

    return referenceMeasure;
  }


  /**
   * Get the measure that is the reference directly.
   */
  ControlMeasure *ControlPoint::GetRefMeasure() {
    if (!HasRefMeasure()) {
      std::string msg = "Control point [" + GetId().toStdString() + "] has no reference measure!";
      throw IException(IException::Programmer, msg, _FILEINFO_);
    }

    return referenceMeasure;
  }


  /**
   * Set the point's chooser name. This will be lost if any attributes relating
   *   to this point is later changed and the current user will be set. This is
   *   one of the 'last modified attributes' referred to in other comments.
   *
   * @param name The username of the person who last modified this control point
   */
  ControlPoint::Status ControlPoint::SetChooserName(QString name) {
    if (editLock) {
      return PointLocked;
    }
    chooserName = name;
    return Success;
  }


  /**
   * Set the point's last modified time. This will be lost if any attributes
   *   relating to this point are later changed and the current time will be
   *   set. This is one of the 'last modified attributes' referred to in other
   *   comments.
   *
   * @param newDateTime The date and time this control point was last modified
   */
  ControlPoint::Status ControlPoint::SetDateTime(QString newDateTime) {
    if (editLock) {
      return PointLocked;
    }
    dateTime = newDateTime;
    return Success;
  }


  /**
   * Set the EditLock state. If edit lock is on, then most attributes relating
   *   to this point are not modifiable. Edit lock is like "Don't modify my
   *   attributes, but you can still modify my measures' attributes". The
   *   reference measure is implicitely edit locked if the point is edit locked.
   *
   * @param lock True to enable edit lock, false to disable it and allow the
   *   point to be modified.
   */
  ControlPoint::Status ControlPoint::SetEditLock(bool lock) {
    if (parentNetwork) {
      parentNetwork->emitPointModified(this, ControlPoint::EditLockModified, editLock, lock);
    }
    editLock = lock;
    return Success;
  }


  /**
   * Set the jigsawRejected state. If IsRejected is true, then this point should be
   *   ignored until the next iteration in the bundle adjustement.  BundleAdjust
   *   decides when to reject or accept a point. The initial IsRejected state of
   *   a measure is false.
   *
   * @param reject True to reject a measure, false to include it in the adjustment
   */
  ControlPoint::Status ControlPoint::SetRejected(bool reject) {
    jigsawRejected = reject;
    return Success;
  }


  /**
   * Sets the Id of the control point
   *
   * @param id Control Point Id
   *
   * @return  (int) status Success or PointLocked
   */
  ControlPoint::Status ControlPoint::SetId(QString newId) {
    if (editLock) {
      return PointLocked;
    }
    QString oldId = id;
    id = newId;
    if (parentNetwork) {
      parentNetwork->UpdatePointReference(this, oldId);
    }
    return Success;
  }


  /**
   * Set the point's reference measure
   *
   * @param cm The new reference measure
   */
  ControlPoint::Status ControlPoint::SetRefMeasure(ControlMeasure *cm) {
    if (editLock) {
      return PointLocked;
    }

    SetExplicitReference(cm);
    return Success;
  }


  /**
   * Set the point's reference measure
   *
   * @param index The index of the new reference measure
   */
  ControlPoint::Status ControlPoint::SetRefMeasure(int index) {
    if (editLock) {
      return PointLocked;
    }

    if (index < 0 || index >= cubeSerials->size()) {
      std::string msg = "Index [";
      msg += toString(index) + "] out of range";
      throw IException(IException::Programmer, msg, _FILEINFO_);
    }

    SetExplicitReference((*measures)[cubeSerials->at(index)]);
    return Success;
  }


  /**
   * Set the points reference measure
   *
   * @param sn The serial number of the new reference measure
   */
  ControlPoint::Status ControlPoint::SetRefMeasure(QString sn) {
    if (editLock) {
      return PointLocked;
    }

    if (!cubeSerials->contains(sn)) {
      std::string msg = "Point [" + id.toStdString() + "] has no measure with serial number [" +
          sn.toStdString() + "]";
      throw IException(IException::Programmer, msg, _FILEINFO_);
    }

    SetExplicitReference((*measures)[sn]);
    return Success;
  }


  /**
   * Explicitly defines a new reference measure by pointer.  This assumes the
   * point already has ownership over this pointer.  As part of the explicit
   * definition process, the reference will attempt to be made ignored if the
   * measure will allow it.
   *
   * In the past, setting an explicit reference would also attempt to set the
   * new reference to un-ignored (this would only fail if the measure was "Edit
   * Locked").  This blanket rule was removed, however, because the bundle
   * adjustment processing phase could often intentionally set references to
   * ignored, and in some instances (e.g., merging a partial network back into
   * the base network) this rule would mistakenly set those properly ignored
   * references back to un-ignored.  While this rule made sense for the
   * registration phase of processing, it clearly caused problems during bundle
   * adjustment and merging.
   *
   * @param measure The new reference measure
   */
  void ControlPoint::SetExplicitReference(ControlMeasure *measure) {
    referenceExplicitlySet = true;
    referenceMeasure = measure;
  }


  /**
   * Set whether to ignore or use control point
   *
   * @param newIgnoreStatus True to ignore this Control Point, False to
   *                        un-ignore
   */
  ControlPoint::Status ControlPoint::SetIgnored(bool newIgnoreStatus) {
    if (editLock) {
      return PointLocked;
    }

    bool oldStatus = ignore;
    ignore = newIgnoreStatus;

    // only update if there was a change in status
    if (oldStatus != ignore) {
      PointModified();
      if (parentNetwork) {
        if (ignore) {
          parentNetwork->pointIgnored(this);
        }
        else {
          parentNetwork->pointUnIgnored(this);
        }
        parentNetwork->emitPointModified(this, ControlPoint::IgnoredModified, oldStatus, ignore);
      }
    }

    return Success;
  }


  /**
   * Set or update the surface point relating to this control point. This is
   *   the point on the surface of the planet that the measures are tied to.
   *   This updates the last modified attributes of this point.
   *     *** Warning:  Only BundleAdjust and its applications should be
   *                   using this method.
   *
   * @param newSurfacePoint The point on the target's surface the measures are
   *                        tied to
   *
   * @internal
   *   @history 2011-07-01 Debbie A. Cook  Removed editLock check
   */
  ControlPoint::Status ControlPoint::SetAdjustedSurfacePoint(
    SurfacePoint newSurfacePoint) {

    PointModified();
    adjustedSurfacePoint = newSurfacePoint;
    return Success;
  }


  /**
   * Updates the control point's type. This updates the last modified attributes
   *   of this point.
   *
   * @see PointType
   *
   * @param newType The new type this control point should be
   */
  ControlPoint::Status ControlPoint::SetType(PointType newType) {
    if (type != Fixed && type != Free && type != Constrained) {
      std::string msg = "Invalid Point Enumeration, [" + toString(type) + "], for "
          "Control Point [" + GetId().toStdString() + "]";
      throw IException(IException::Programmer, msg, _FILEINFO_);
    }

    if (editLock) {
      return PointLocked;
    }
    if (parentNetwork) {
      parentNetwork->emitPointModified(this, ControlPoint::TypeModified, type, newType);
    }

    PointModified();
    type = newType;
    return Success;
  }


  /**
   * This updates the source of the radius of the apriori surface point.
   *
   * @see RadiusSource::Source
   *
   * @param source Where the radius came from
   */
  ControlPoint::Status ControlPoint::SetAprioriRadiusSource(
    RadiusSource::Source source) {
    if (editLock) {
      return PointLocked;
    }
    PointModified();
    aprioriRadiusSource = source;
    return Success;
  }


  /**
   * This updates the filename of the DEM that the apriori radius came from. It
   *   doesn't really make sense to call this unless the RadiusSource is DEM.
   *
   * @see RadiusSource::Source
   *
   * @param source Where the radius came from
   */
  ControlPoint::Status ControlPoint::SetAprioriRadiusSourceFile(
    QString sourceFile) {
    if (editLock) {
      return PointLocked;
    }
    PointModified();
    aprioriRadiusSourceFile = sourceFile;
    return Success;
  }


  /**
   * This updates the apriori surface point.
   *
   * @see SetAprioriRadiusSource
   * @see SetAprioriRadiusSourceFile
   * @see SetAprioriPointSource
   * @see SetAprioriPointSourceFile
   * @see aprioriSurfacePoint
   *
   * @param aprioriSP The apriori surface point to remember
   * @todo This method needs to be revisited.  It will set the constraint
   *       status based on the sigmas and override the existing status.
   */
  ControlPoint::Status ControlPoint::SetAprioriSurfacePoint(
    SurfacePoint aprioriSP) {
    SurfacePoint::CoordinateType coordType = SurfacePoint::Latitudinal;
    if (parentNetwork) {
      coordType = parentNetwork->GetCoordType();
    }
    if (editLock) {
      return PointLocked;
    }
      // ***TBD*** Does it make sense to try to do a generic check here? The
      // data types are different (angles vs distance) so for now do a switch.
    switch (coordType) {
      case SurfacePoint::Latitudinal:
        if (aprioriSP.GetLatSigma().isValid())
          constraintStatus.set(Coord1Constrained);
        if (aprioriSP.GetLonSigma().isValid())
          constraintStatus.set(Coord2Constrained);
        if (aprioriSP.GetLocalRadiusSigma().isValid())
          constraintStatus.set(Coord3Constrained);
        break;
      case SurfacePoint::Rectangular:
        if (aprioriSP.GetXSigma().isValid())
          constraintStatus.set(Coord1Constrained);
        if (aprioriSP.GetYSigma().isValid())
          constraintStatus.set(Coord2Constrained);
        if (aprioriSP.GetZSigma().isValid())
          constraintStatus.set(Coord3Constrained);
      }

    PointModified();
    aprioriSurfacePoint = aprioriSP;
    return Success;
  }


  /**
   * This updates the source of the surface point
   *
   * @see SurfacePointSource::Source
   *
   * @param source Where the surface point came from
   */
  ControlPoint::Status ControlPoint::SetAprioriSurfacePointSource(
      SurfacePointSource::Source source) {
    if (editLock) {
      return PointLocked;
    }
    PointModified();
    aprioriSurfacePointSource = source;
    return Success;
  }


  /**
   * This updates the filename of where the apriori surface point came from.
   *
   * @see RadiusSource::Source
   *
   * @param sourceFile Where the surface point came from
   */
  ControlPoint::Status ControlPoint::SetAprioriSurfacePointSourceFile(
      QString sourceFile) {
    if (editLock) {
      return PointLocked;
    }
    PointModified();
    aprioriSurfacePointSourceFile = sourceFile;
    return Success;
  }


  /**
   * Computes a priori lat/lon/radius point coordinates by determining the average lat/lon/radius of
   * all measures. Note that this does not change ignored, fixed or constrained points.
   *
   * Also, it does not use unmeasured or ignored measures when computing lat/lon/radius.
   *
   * (KLE) Note this not a rigorous triangulation considering outliers. A better way would be to...
   *         a) use e.g. a closest approach algorithm to find intersection of all rays, regardless
   *            of whether the intersection lies on the surface in question, then;
   *         b) perform a rigorous triangulation with some sort outlier detection approach, a robust
   *            estimation technique (perhaps RANSAC)
   *
   * @internal
   *   @history 2008-06-18  Tracie Sucharski/Jeannie Walldren,
   *                               Changed error messages for
   *                               Held/Ground points.
   *   @history 2009-10-13 Jeannie Walldren - Added detail to
   *                               error message.
   *   @history 2010-11-29 Tracie Sucharski - Remove call to ControlMeasure::
   *                               SetMeasuredEphemerisTime, the values were
   *                               never used. so these methods were removed
   *                               from ControlMeasure and the call was removed
   *                               here.
   *   @history 2010-12-02 Debbie A. Cook - Added units to SetRectangular
   *                               calls since default is meters and units
   *                               are km.
   *   @history 2011-03-17 Debbie A. Cook - Added initialization of
   *                               adjustedSurfacePoint to aprioriSurfacePoint
   *                               and set test for empty covariance matrix
   *                               to use 0. instead of nulls.
   *   @history 2011-03-24 Debbie A. Cook - Removed IsMeasured check since it
   *                               was really checking for Candidate measures.
   *   @history 2011-07-12 Debbie A. Cook - Removed editLock test.  Users agreed
   *                               editLock was only for fixed and constrained
   *                               points, which are already left unchanged by
   *                               ComputeApriori. If a free point is editLocked
   *                               the editLock will be ignored by this method.
   *  @history 2017-04-25 Debbie A. Cook - change constraint status calls
   *                               to use generic coordinate names (Coord1, Coord2,
   *                               and Coord3).
   *  @history 2019-03-10 Ken Edmundson - Fixed bug where focal plane measured x,y coordinates were
   *                               not set if the cam->SetImage call failed. Setting the measured
   *                               focal plane coordinates should not depend upon the success of the
   *                               SetImage call (References #2591). Improved error messages.
   *                               Cleaned up code. Added comments above to suggest a more rigorous
   *                               approach to computing a priori point coordinates.
   *
   * @return Status Success or PointLocked
   */
  ControlPoint::Status ControlPoint::ComputeApriori() {
    // TODO (KLE): where should call this go? Also, what's the point? The method has no description.
    PointModified();

    // if point is fixed or constrained, ensure valid a priori point coordinates exist
    if ( (IsFixed() || IsConstrained()) &&  !aprioriSurfacePoint.Valid() ) {
      std::string msg = "In method ControlPoint::ComputeApriori(). ControlPoint [" + GetId().toStdString() + "] is ";
      msg += "fixed or constrained and requires a priori coordinates";
      throw IException(IException::User, msg, _FILEINFO_);
    }

    double xB = 0.0;  // body-fixed x
    double yB = 0.0;  // body-fixed y
    double zB = 0.0;  // body-fixed z
    double r2B = 0.0; // radius squared in body-fixed
    int goodMeasures = 0;
    double pB[3];

    // loop over measures to ...
    // 1) set focal plane x,y coordinates for all unignored measures;
    // 2) sum latitude, longitude, and radius coordinates in preparation for computing a priori
    //    coordinates by averaging.
    for (int i = 0; i < cubeSerials->size(); i++) {
      ControlMeasure *m = GetMeasure(i);
      if (m->IsIgnored()) {
        continue;
      }

      Camera *cam = m->Camera();
      if (cam == NULL) {
        QString cubeSN = m->GetCubeSerialNumber();
        std::string msg = "in method ControlPoint::ComputeApriori(). Camera has not been set in ";
        msg += "measure for cube serial number [" + cubeSN.toStdString() + "], Control Point id ";
        msg += "[" + GetId().toStdString() + "]. Camera must be set prior to calculating a priori coordinates";
        throw IException(IException::Programmer, msg, _FILEINFO_);
      }

      bool setImageSuccess = cam->SetImage(m->GetSample(), m->GetLine());
      // CSM cameras do not have focal planes so use sample and line instead
      if (cam->GetCameraType() == Camera::Csm) {
        m->SetFocalPlaneMeasured(m->GetSample(),
                                 m->GetLine());
      }
      else {
        m->SetFocalPlaneMeasured(cam->DistortionMap()->UndistortedFocalPlaneX(),
                                 cam->DistortionMap()->UndistortedFocalPlaneY());
      }

      // TODO: Seems like we should be able to skip this computation if point is fixed or
      // constrained in any coordinate. Currently we are always summing coordinates here. We could
      // save time by not doing this for fixed or constrained points.
      if (setImageSuccess) {
        goodMeasures++;
        cam->Coordinate(pB);
        xB += pB[0];
        yB += pB[1];
        zB += pB[2];
        r2B += pB[0]*pB[0] + pB[1]*pB[1] + pB[2]*pB[2];
      }
    }

    // if point is Fixed or Constrained in any number of coordinates, initialize adjusted surface
    // point to a priori coordinates (set in e.g. qnet or cneteditor) and exit
    if( IsFixed() || IsConstrained() || id.contains("Lidar")) {
      adjustedSurfacePoint = aprioriSurfacePoint;
      return Success;
    }

    // if point is Free, we continue to compute a priori coordinates

    // if no good measures, we're done
    // TODO: is the message true/meaningful?
    if (goodMeasures == 0) {
      std::string msg = "in method ControlPoint::ComputeApriori(). ControlPoint [" + GetId().toStdString() + "] has ";
      msg += "no measures which project to the body";
      throw IException(IException::User, msg, _FILEINFO_);
    }

    // Compute the averages if all coordinates are free
    // TODO: confirm if this "if" statement is necessary
    if (GetType() == Free || NumberOfConstrainedCoordinates() == 0) {
      double avgX = xB / goodMeasures;
      double avgY = yB / goodMeasures;
      double avgZ = zB / goodMeasures;
      double avgR2 = r2B / goodMeasures;
      double scale = sqrt(avgR2/(avgX*avgX+avgY*avgY+avgZ*avgZ));

      aprioriSurfacePoint.SetRectangular(
        Displacement((avgX*scale), Displacement::Kilometers),
        Displacement((avgY*scale), Displacement::Kilometers),
        Displacement((avgZ*scale), Displacement::Kilometers));
    }

    adjustedSurfacePoint = aprioriSurfacePoint;
    SetAprioriSurfacePointSource(SurfacePointSource::AverageOfMeasures);
    SetAprioriRadiusSource(RadiusSource::AverageOfMeasures);

    return Success;
  }


  /**
   * This method computes the BundleAdjust residuals for a point.
   *     *** Warning:  Only BundleAdjust and its applications should be
   *                   using this method.
   *
   * @history 2008-07-17 Tracie Sucharski,  Added ptid and measure serial
   *                            number to the unable to map to surface error.
   * @history 2009-12-06 Tracie Sucharski, Renamed from ComputeErrors
   * @history 2010-08-05 Tracie Sucharski, Changed lat/lon/radius to x/y/z
   * @history 2010-12-10 Debbie A. Cook, Revised error calculation for radar
   *                            because it was always reporting line errors=0.
   * @history 2011-03-17 Debbie A. Cook, Fixed typo in radar call to get
   *                            longitude
   * @history 2011-03-24 Debbie A. Cook, Removed IsMeasured check since it
   *                            was really checking for Candidate measures.
   * @history 2011-07-01 Debbie A. Cook, Removed editLock check to allow
   *                            BundleAdjust to compute residuals for
   *                            editLocked points
   * @history 2012-01-18 Debbie A. Cook, Revised to call
   *                            ComputeResidualsMillimeters() to avoid duplication of code.
   * @history 2019-05-16 Debbie A. Cook, The calls to CameraGroundMap::GetXY
   *                           were changed to allow not testing for points on the back side of the
   *                           planet during bundle adjustment.  Now, the instrument coordinates
   *                           will be calculated and returned always to this method. In the future,
   *                           a separate diagnostic tool may be helpful to check for non-visable
   *                           points in a control net AFTER bundle adjustment. References #2591.
   *
   */
  ControlPoint::Status ControlPoint::ComputeResiduals() {
    if (IsIgnored()) {
      return Failure;
    }

    PointModified();

    // Loop for each measure to compute the error
    QList<QString> keys = measures->keys();

    for (int j = 0; j < keys.size(); j++) {
      ControlMeasure *m = (*measures)[keys[j]];
      if (m->IsIgnored()) {
        continue;
      }

      Camera *cam = m->Camera();

      double cuSamp;
      double cuLine;
      CameraFocalPlaneMap *fpmap = m->Camera()->FocalPlaneMap();

      // Map the coordinates of the control point through the Spice of the
      // measurement sample/line to get the computed sample/line.  This must be
      // done manually because the camera will compute a new time for line scanners,
      // instead of using the measured time.
      ComputeResiduals_Millimeters();

      // Convert the residuals in millimeters to undistorted pixels
      if (cam->GetCameraType()  ==  Isis::Camera::Radar) {
        // For radar line is calculated from time in the camera.  Use the
        // closest line to scale the focal plane y (doppler shift) to image line
        // for computing the line residual.  Get a local ratio
        //     measureLine    =   adjacentLine
        //     ------------       --------------  in both cases, doppler shift
        //     dopplerMLine       dopplerAdjLine  is calculated using SPICE
        //                                        at the time of the measurement
        //
        // 1.  Get the surface point mapped to by an adjacent pixel above (if
        //     doppler is < 0) or below (if doppler is > 0) the measured pixel
        // 2.  Set image to the measured sample/line to load the SPICE for the
        //     time of the measurement.
        // 3.  Map the surface point from the adjacent pixel through the SPICE
        //     into the image plane to get a scale for mapping from doppler
        //     shift to line.  Apply the scale to get the line residual
        double sample = m->GetSample();
        double computedY = m->GetFocalPlaneComputedY();
        double computedX = m->GetFocalPlaneComputedX();
        double adjLine;

        // Step 1. What happens if measured line is 1???  TODO
        if (computedY < 0) {
          adjLine = m->GetLine() - 1.;
        }
        else {
          adjLine = m->GetLine() + 1.;
        }

        cam->SetImage(sample, adjLine);
        SurfacePoint sp = cam->GetSurfacePoint();

        // Step 2.
        cam->SetImage(sample, m->GetLine());
        double focalplaneX;
        double scalingY;

        // Step 3.
        // The default bool value will come from CameraGroundMap instead of
        // RadarGroundMap so be explicit to turn off back-of-planet test.
        cam->GroundMap()->GetXY(sp, &focalplaneX, &scalingY, false);
        double deltaLine;

        if (computedY < 0) {
          deltaLine = -computedY/scalingY;
        }
        else {
          deltaLine = computedY/scalingY;
        }

        // Now map through the camera steps to take X from slant range to ground
        // range to pixels.  Y just tracks through as 0.
        if (cam->DistortionMap()->SetUndistortedFocalPlane(computedX,
                                                           computedY)){
          double focalPlaneX = cam->DistortionMap()->FocalPlaneX();
          double focalPlaneY = cam->DistortionMap()->FocalPlaneY();
          fpmap->SetFocalPlane(focalPlaneX,focalPlaneY);
        }
        cuSamp = fpmap->DetectorSample();
        cuLine = m->GetLine() + deltaLine;
      }
      else if (cam->GetCameraType()  ==  Isis::Camera::Csm) {
        //
        cuSamp = m->GetFocalPlaneComputedX();
        cuLine = m->GetFocalPlaneComputedY();
      }
      else {
        // Now things get tricky.  We want to produce errors in pixels not mm
        // but some of the camera maps could fail.  One that won't is the
        // FocalPlaneMap which takes x/y to detector s/l.  We will bypass the
        // distortion map and have residuals in undistorted pixels.
        if (!fpmap->SetFocalPlane(m->GetFocalPlaneComputedX(), m->GetFocalPlaneComputedY())) {
          std::string msg = "Sanity check #1 for ControlPoint [" + GetId().toStdString() +
              "], ControlMeasure [" + m->GetCubeSerialNumber().toStdString() + "]";
          throw IException(IException::Programmer, msg, _FILEINFO_);
          // This error shouldn't happen but check anyways
        }

        cuSamp = fpmap->DetectorSample();
        cuLine = fpmap->DetectorLine();
      }

      // Compute the measures sample and line

      double muSamp;
      double muLine;

      if (cam->GetCameraType()  ==  Isis::Camera::Radar ||
          cam->GetCameraType()  ==  Isis::Camera::Csm) {
        // For CSM and Radar we use distorted pixels
        muSamp = m->GetSample();
        muLine = m->GetLine();
      }
      else {
        // For other sensors conver to undistorted pixels
        // Again we will bypass the distortion map and have residuals in undistorted pixels.
        if (!fpmap->SetFocalPlane(m->GetFocalPlaneMeasuredX(), m->GetFocalPlaneMeasuredY())) {
          std::string msg = "Sanity check #2 for ControlPoint [" + GetId().toStdString() +
              "], ControlMeasure [" + m->GetCubeSerialNumber().toStdString() + "]";
          throw IException(IException::Programmer, msg, _FILEINFO_);
          // This error shouldn't happen but check anyways
        }
        muSamp = fpmap->DetectorSample();
        muLine = fpmap->DetectorLine();
      }

      // The units are in detector sample/lines.  We will apply the instrument
      // summing mode to get close to real pixels.  Note however we are in
      // undistorted pixels except for radar and CSM instruments.
      double sampResidual = muSamp - cuSamp;
      double lineResidual = muLine - cuLine;
      m->SetResidual(sampResidual, lineResidual);
    }

    return Success;
  }

  /**
   * This method computes the residuals for a point.
   *
   * @history 2008-07-17  Tracie Sucharski -  Added ptid and measure serial
   *                            number to the unable to map to surface error.
   * @history 2010-12-06  Tracie Sucharski - Renamed from ComputeErrors
   * @history 2011-03-19  Debbie A. Cook - Changed to use the Camera classes
   *                            like ComputeResiduals and get the correct
   *                            calculations for each camera type.
   * @history 2011-03-24 Debbie A. Cook - Removed IsMeasured check since it
   *                            was really checking for Candidate measures.
   * @history 2012-01-18 Debbie A. Cook - Made radar case the same as
   *                            other instruments and removed incorrect
   *                            call to SetResidual, which was setting
   *                            focal plane residuals in x and y instead
   *                            of image residuals in sample and line.
   */

  ControlPoint::Status ControlPoint::ComputeResiduals_Millimeters() {
    if (IsIgnored()) {
      return Failure;
    }

    PointModified();

    // Loop for each measure to compute the error
    QList<QString> keys = measures->keys();

    for (int j = 0; j < keys.size(); j++) {
      ControlMeasure *m = (*measures)[keys[j]];
      if (m->IsIgnored()) {
        continue;
      }

      Camera *cam = m->Camera();
      double cudx, cudy;

      // Map the coordinates of the control point through the Spice of the
      // measurement sample/line to get the computed undistorted focal plane
      // coordinates (mm if not radar).
      // This works for radar too because in the undistorted focal plane,
      // y has not been set to 0 (set to 0 when going to distorted focal plane
      // or ground range in this case), so we can hold the Spice to calculate
      // residuals in undistorted focal plane coordinates.
      // This does not work with CSM as it does not have a focal plane so
      // just use the sample and line
      if (cam->GetCameraType() == Camera::Csm) {
        cam->SetGround(GetAdjustedSurfacePoint());
        cudx = cam->Sample();
        cudy = cam->Line();
        // Reset to measure
        cam->SetImage(m->GetSample(), m->GetLine());
      }
      else {
        // no need to call setimage for framing camera
        if (cam->GetCameraType() != 0) {
          cam->SetImage(m->GetSample(), m->GetLine());
        }
        // The default bool value is true.  Turn back-of-planet test off for bundle adjustment.
        cam->GroundMap()->GetXY(GetAdjustedSurfacePoint(), &cudx, &cudy, false);
      }

      m->SetFocalPlaneComputed(cudx, cudy);
    }

    return Success;
  }

  QString ControlPoint::GetChooserName() const {
    if (chooserName != "") {
      return chooserName;
    }
    else {
      return QString::fromStdString(FileName(Application::Name().toStdString()).name());
    }
  }

  //! Returns true if the choosername is not empty.
  bool ControlPoint::HasChooserName() const {
    return !chooserName.isEmpty();
  }

  //! Returns true if the datetime is not empty.
  bool ControlPoint::HasDateTime() const {
    return !dateTime.isEmpty();
  }


  QString ControlPoint::GetDateTime() const {
    if (dateTime != "") {
      return dateTime;
    }
    else {
      return Application::DateTime();
    }
  }


  bool ControlPoint::IsEditLocked() const {
    return editLock;
  }


  bool ControlPoint::IsRejected() const {
    return jigsawRejected;
  }


  SurfacePoint ControlPoint::GetAdjustedSurfacePoint() const {
    return adjustedSurfacePoint;
  }


  /**
   * Returns the adjusted surface point if it exists, otherwise returns
   * the a priori surface point.
   */
  SurfacePoint ControlPoint::GetBestSurfacePoint() const {
    if (adjustedSurfacePoint.Valid()) {
      return adjustedSurfacePoint;
    }
    else {
      return aprioriSurfacePoint;
    }
  }


  /**
   * Return the Id of the control point
   *
   * @return Control Point Id
   */
  QString ControlPoint::GetId() const {
    return id;
  }


  bool ControlPoint::IsIgnored() const {
    return ignore;
  }


  bool ControlPoint::IsValid() const {
    return !invalid;
  }


  bool ControlPoint::IsInvalid() const {
    return invalid;
  }


  /**
   *  Obtain a string representation of a given PointType
   *
   *  @param type PointType to convert to a string
   *
   *  @returns A string representation of type
   */
  QString ControlPoint::PointTypeToString(PointType pointType) {
    QString str;

    switch (pointType) {
      case Fixed:
        str = "Fixed";
        break;
      case Constrained:
        str = "Constrained";
        break;
      case Free:
        str = "Free";
        break;
    }

    return str;
  }


  /**
   *  Obtain a PointType given a string representation of it.
   *
   *  @param pointTypeString for the requested PointType
   *
   *  @returns the PointType for the given string
   */
  ControlPoint::PointType ControlPoint::StringToPointType(
      QString pointTypeString) {

    //  On failure assume Free
    ControlPoint::PointType type = ControlPoint::Free;

    std::string errMsg  = "There is no PointType that has a string representation"
                      " of \"";
            errMsg += pointTypeString.toStdString();
            errMsg += "\".";

    if (pointTypeString == "Fixed") {
      type = ControlPoint::Fixed;
    }
    else if (pointTypeString == "Constrained") {
      type = ControlPoint::Constrained;
    }
    else if (pointTypeString == "Free") {
      type = ControlPoint::Free;
    }
    else {
      throw IException(IException::Programmer, errMsg, _FILEINFO_);
    }

    return type;
  }


  /**
   * Obtain a string representation of the PointType
   *
   * @return A string representation of the PointType
   */
  QString ControlPoint::GetPointTypeString() const {
    return PointTypeToString(GetType());
  }


  /**
   * @returns this point't type
   *
   */
  ControlPoint::PointType ControlPoint::GetType() const {
    return type;
  }


  /**
   *  Obtain a string representation of a given RadiusSource
   *
   *  @param source RadiusSource to convert to string
   *
   *  @returns A string representation of RadiusSource
   */
  QString ControlPoint::RadiusSourceToString(RadiusSource::Source source) {
    QString str;

    switch (source) {
      case RadiusSource::None:
        str = "None";
        break;
      case RadiusSource::User:
        str = "User";
        break;
      case RadiusSource::AverageOfMeasures:
        str = "AverageOfMeasures";
        break;
      case RadiusSource::Ellipsoid:
        str = "Ellipsoid";
        break;
      case RadiusSource::DEM:
        str = "DEM";
        break;
      case RadiusSource::BundleSolution:
        str = "BundleSolution";
        break;
    }

    return str;
  }


  /**
   *  Obtain a RadiusSource::Source from a string
   *
   *  @param str string to get a RadiusSource::Source from
   *
   *  @returns The RadiusSource::Source matching the given string
   */
  ControlPoint::RadiusSource::Source ControlPoint::StringToRadiusSource(
    QString str) {

    str = str.toLower();
    RadiusSource::Source source = RadiusSource::None;

    if (str == "user") {
      source = RadiusSource::User;
    }
    else if (str == "averageofmeasures") {
      source = RadiusSource::AverageOfMeasures;
    }
    else if (str == "ellipsoid") {
      source = RadiusSource::Ellipsoid;
    }
    else if (str == "dem") {
      source = RadiusSource::DEM;
    }
    else if (str == "bundlesolution") {
      source = RadiusSource::BundleSolution;
    }

    return source;
  }



  /**
   * Obtain a string representation of the RadiusSource
   *
   * @return A string representation of the RadiusSource
   */
  QString ControlPoint::GetRadiusSourceString() const {
    return RadiusSourceToString(aprioriRadiusSource);
  }


  /**
   *  Obtain a string representation of a given SurfacePointSource
   *
   *  @param souce SurfacePointSource to get a string representation of
   *
   *  @returns A string representation of SurfacePointSource
   */
  QString ControlPoint::SurfacePointSourceToString(
    SurfacePointSource::Source source) {

    QString str;

    switch (source) {
      case SurfacePointSource::None:
        str = "None";
        break;
      case SurfacePointSource::User:
        str = "User";
        break;
      case SurfacePointSource::AverageOfMeasures:
        str = "AverageOfMeasures";
        break;
      case SurfacePointSource::Reference:
        str = "Reference";
        break;
      case SurfacePointSource::Basemap:
        str = "Basemap";
        break;
      case SurfacePointSource::BundleSolution:
        str = "BundleSolution";
        break;
    }

    return str;
  }


  /**
   *  Obtain a SurfacePoint::Source from a string
   *
   *  @param str string to get a SurfacePoint::Source from
   *
   *  @returns The SurfacePint::Source matching the given string
   */
  ControlPoint::SurfacePointSource::Source
  ControlPoint::StringToSurfacePointSource(
    QString str) {

    str = str.toLower();
    SurfacePointSource::Source source = SurfacePointSource::None;

    if (str == "user") {
      source = SurfacePointSource::User;
    }
    else if (str == "averageofmeasures") {
      source = SurfacePointSource::AverageOfMeasures;
    }
    else if (str == "reference") {
      source = SurfacePointSource::Reference;
    }
    else if (str == "basemap") {
      source = SurfacePointSource::Basemap;
    }
    else if (str == "bundlesolution") {
      source = SurfacePointSource::BundleSolution;
    }

    return source;
  }


  /**
   * Obtain a string representation of the SurfacePointSource
   *
   * @return A string representation of the SurfacePointSource
   */
  QString ControlPoint::GetSurfacePointSourceString() const {
    return SurfacePointSourceToString(aprioriSurfacePointSource);
  }


  SurfacePoint ControlPoint::GetAprioriSurfacePoint() const {
    return aprioriSurfacePoint;
  }


   ControlPoint::RadiusSource::Source ControlPoint::GetAprioriRadiusSource()
       const {
     return aprioriRadiusSource;
   }


   bool ControlPoint::HasAprioriCoordinates() {
     if (aprioriSurfacePoint.GetX().isValid() &&
         aprioriSurfacePoint.GetY().isValid() &&
         aprioriSurfacePoint.GetZ().isValid()) {
       return true;
     }

     return false;
     // return aprioriSurfacePoint.Valid(); ???
   }


   /**
    * Return bool indicating if point is Free or not.
    *
    * @return bool indicating if point is Free or not.
    */
   bool ControlPoint::IsFree() const {
     return (type != Fixed && type != Constrained);
   }


   /**
    * Return bool indicating if point is Fixed or not.
    *
    * @return bool indicating if point is Fixed or not.
    */
   bool ControlPoint::IsFixed() const {
     return (type == Fixed);
   }


   /**
    * Return bool indicating if point is Constrained or not.
    *
    * @return bool indicating if point is Constrained or not.
    */
  bool ControlPoint::IsConstrained() {
    // if point type is Free, we ignore any a priori sigmas on the coordinates
    if (type == Free) {
      return false;
    }

    return constraintStatus.any();
  }


  /**
   * Return bool indicating if 1st coordinate is Constrained or not. This corresponds to Latitude
   *   for a Latitudinal solution or X for a Rectangular solution.
   *
   * @return bool indicating if 1st coordinate is Constrained or not.
   */
  bool ControlPoint::IsCoord1Constrained() {
    return constraintStatus[Coord1Constrained];
  }


  /**
   * Return bool indicating if 2nd coordinate is Constrained or not. This corresponds to Longitude
   *   for a Latitudinal solution or Y for a Rectangular solution.
   *
   * @return bool indicating if 2nd coordinate is Constrained or not.
   */
  bool ControlPoint::IsCoord2Constrained() {
    return constraintStatus[Coord2Constrained];
  }

  /**
   * Return bool indicating if 3rd coordinate is Constrained or not. This corresponds to Radius
   *   for a Latitudinal solution or Z for a Rectangular solution.
   *
   * @return bool indicating if 3rd coordinate is Constrained or not.
   */
  bool ControlPoint::IsCoord3Constrained() {
    return constraintStatus[Coord3Constrained];
  }


  /**
   * Return bool indicating if point is Constrained or not.
   *
   * @return bool indicating if point is Constrained or not.
   */
  int ControlPoint::NumberOfConstrainedCoordinates() {
    return constraintStatus.count();
  }


 /**
  * Checks to see if the radius source file has been set.
  *
  * @return bool True if the radius source file has been set.
  */
  bool ControlPoint::HasAprioriRadiusSourceFile() const {
    return !( aprioriRadiusSourceFile.isEmpty() || aprioriRadiusSourceFile.isNull() );
  }


  QString ControlPoint::GetAprioriRadiusSourceFile() const {
    return aprioriRadiusSourceFile;
  }


  ControlPoint::SurfacePointSource::Source
  ControlPoint::GetAprioriSurfacePointSource() const {
    return aprioriSurfacePointSource;
  }


 /**
  * Checks to see if the surface point source file has been set.
  *
  * @return bool True if the surface point source file has been set.
  */
  bool ControlPoint::HasAprioriSurfacePointSourceFile() const {
    return !( aprioriSurfacePointSourceFile.isEmpty() || aprioriSurfacePointSourceFile.isNull() );
  }


  QString ControlPoint::GetAprioriSurfacePointSourceFile() const {
    return aprioriSurfacePointSourceFile;
  }


  int ControlPoint::GetNumMeasures() const {
    return measures->size();
  }


  /**
   *
   * @return Number of valid control measures
   */
  int ControlPoint::GetNumValidMeasures() const {
    int size = 0;
    QList<QString> keys = measures->keys();
    for (int cm = 0; cm < keys.size(); cm++) {
      if (!(*measures)[keys[cm]]->IsIgnored()) {
        size++;
      }
    }
    return size;
  }


  /**
   * Returns the number of locked control measures
   *
   * @return Number of locked control measures
   */
  int ControlPoint::GetNumLockedMeasures() const {
    int size = 0;
    QList<QString> keys = measures->keys();
    for (int cm = 0; cm < keys.size(); cm++) {
      if ((*measures)[keys[cm]]->IsEditLocked()) {
        size++;
      }
    }
    return size;
  }


  /**
   *  Return true if given serial number exists in point
   *
   *  @param serialNumber  The serial number
   *  @return True if point contains serial number, false if not
   */
  bool ControlPoint::HasSerialNumber(QString serialNumber) const {
    return cubeSerials->contains(serialNumber);
  }


  /**
   * @returns true Returns true if SetRefMeasure has ever been set on this
   *               point.
   */
  bool ControlPoint::IsReferenceExplicit() const {
    return referenceExplicitlySet;
  }


  /**
   * @returns The cube serial number of the reference measure
   */
  QString ControlPoint::GetReferenceSN() const {
    if (!HasRefMeasure()) {
      std::string msg = "There is no reference measure set in the ControlPoint [" +
          GetId().toStdString() + "]";
      throw IException(IException::Programmer, msg, _FILEINFO_);
    }

    return referenceMeasure->GetCubeSerialNumber();
  }


  /**
   * @param cm The control measure to find the index of
   * @param throws Throws an exception on failure instead of returning -1.
   *               Be aware that by default this is true!
   *
   * @returns The index of the passed in measure, or -1 on failure if throws
   *          is false.
   */
  int ControlPoint::IndexOf(ControlMeasure *cm, bool throws) const {
    return IndexOf(cm->GetCubeSerialNumber(), throws);
  }


  /**
   * @param sn The serial number of the control measure to find the index of
   * @param throws Throws an exception on failure instead of returning -1.
   *               Be aware that by default this is true!
   *
   * @returns The index of the measure with serial number matching sn,
   *          or -1 on failure if throws is false.
   */
  int ControlPoint::IndexOf(QString sn, bool throws) const {
    int index = cubeSerials->indexOf(sn);

    if (throws && index == -1) {
      std::string msg = "ControlMeasure [" + sn.toStdString() + "] does not exist in point [" +
          id.toStdString() + "]";
      throw IException(IException::Programmer, msg, _FILEINFO_);
    }

    return index;
  }


  /**
   * @param throws Throws an exception on failure instead of returning -1.
   *               Be aware that by default this is true!
   *
   * @returns The index of the reference measure, or -1 if no measures exist
   * in the point (A point with at least one measure ALWAYS has a reference
   * measure.
   */
  int ControlPoint::IndexOfRefMeasure() const {
    if (!HasRefMeasure()) {
      std::string msg = "There is no reference measure for point [" + id.toStdString() + "]."
          "  This also means of course that the point is empty!";
      throw IException(IException::Programmer, msg, _FILEINFO_);
    }

    int index = cubeSerials->indexOf(referenceMeasure->GetCubeSerialNumber());

    return index;
  }


  /**
   * This function will call a given method on every control measure that
   * this point has.
   *
   * @param statFunc The function to use for data collection
   *
   * @returns The gathered statistic
   *
   * @history 2011-03-08  Debbie A. Cook - Changed to get statistics for all
   *                       point types and not just Candidate.
   */
  Statistics ControlPoint::GetStatistic(
    double(ControlMeasure::*statFunc)() const) const {
    Statistics stats;
    foreach(ControlMeasure * cm, *measures) {
      if (!cm->IsIgnored()) {
        stats.AddData((cm->*statFunc)());
      }
    }

    return stats;
  }


  Statistics ControlPoint::GetStatistic(long dataType) const {
    Statistics stats;
    foreach(ControlMeasure * cm, *measures) {
      if (!cm->IsIgnored()) {
        stats.AddData(cm->GetLogData(dataType).GetNumericalValue());
      }
    }

    return stats;
  }


  /**
   * @param excludeIgnored Ignored measures are excluded if this is true.  It
   *                       is false by default.
   *
   * @returns A list of this points measures
   */
  QList< ControlMeasure * > ControlPoint::getMeasures(
    bool excludeIgnored) const {
    QList< ControlMeasure * > orderedMeasures;
    for (int i = 0; i < cubeSerials->size(); i++) {
      ControlMeasure *measure = measures->value((*cubeSerials)[i]);
      if (!excludeIgnored || !measure->IsIgnored()) {
        orderedMeasures.append(measures->value((*cubeSerials)[i]));
      }
    }
    return orderedMeasures;
  }


  /**
   * @returns A list of cube serial numbers
   */
  QList< QString > ControlPoint::getCubeSerialNumbers() const {
    return *cubeSerials;
  }


  /**
   *  Same as GetMeasure (provided for convenience)
   *
   *  @param serialNumber Cube serial number of desired control measure
   *
   *  @returns const version of the measure which has the provided serial number
   */
  const ControlMeasure *ControlPoint::operator[](QString serialNumber) const {
    return GetMeasure(serialNumber);
  }


  /**
   *  Same as GetMeasure (provided for convenience)
   *
   *  @param serialNumber Cube serial number of desired control measure
   *
   *  @returns The measure which has the provided serial number
   */
  ControlMeasure *ControlPoint::operator[](QString serialNumber) {
    return GetMeasure(serialNumber);
  }


  /**
   *  Same as GetMeasure (provided for convenience)
   *
   *  @param index If there are n measures, the measure returned will be the
   *               ith measure added to the point
   *
   *  @returns const version of the measure which has the provided serial number
   */
  const ControlMeasure *ControlPoint::operator[](int index) const {
    return GetMeasure(index);
  }


  /**
   *  Same as GetMeasure (provided for convenience)
   *
   *  @param index If there are n measures, the measure returned will be the
   *               ith measure added to the point
   *
   *  @returns The measure which has the provided serial number
   */
  ControlMeasure *ControlPoint::operator[](int index) {
    return GetMeasure(index);
  }


  /**
   * Compare two Control Points for inequality
   *
   * @param other The other point to compare this one to
   *
   * @returns true if the two points are not equal, and false otherwise
   */
  bool ControlPoint::operator!=(const ControlPoint &other) const {
    return !(*this == other);
  }


  /**
   * Compare two Control Points for equality
   *
   * @param other The other point to compare to
   *
   * @returns true if the two points are equal, and false otherwise
   *
   */
  bool ControlPoint::operator==(const ControlPoint &other) const {
    return other.GetNumMeasures() == GetNumMeasures() &&
        other.id == id &&
        other.type == type &&
        other.chooserName == chooserName &&
        other.editLock == editLock &&
        other.ignore == ignore &&
        other.aprioriSurfacePointSource  == aprioriSurfacePointSource &&
        other.aprioriSurfacePointSourceFile == aprioriSurfacePointSourceFile &&
        other.aprioriRadiusSource  == aprioriRadiusSource &&
        other.aprioriRadiusSourceFile  == aprioriRadiusSourceFile &&
        other.aprioriSurfacePoint == aprioriSurfacePoint &&
        other.adjustedSurfacePoint == adjustedSurfacePoint &&
        other.invalid == invalid &&
        other.measures == measures &&
        other.dateTime == dateTime &&
        other.jigsawRejected == jigsawRejected &&
        other.constraintStatus == constraintStatus &&
        other.referenceExplicitlySet == referenceExplicitlySet &&
        other.numberOfRejectedMeasures == numberOfRejectedMeasures &&
        other.cubeSerials == cubeSerials &&
        other.referenceMeasure == referenceMeasure;
  }


  /**
   *
   * @param pPoint
   *
   * @return ControlPoint&
   *
   * @internal
   *   @history 2011-09-13 Eric Hyer,Tracie Sucharski - Changed input parameter
   *                          to const &.  Re-wrote using Delete and AddMeasure
   *                          methods, so that the ControlGraphNode is updated
   *                          correctly.
   *   @history 2011-09-30 Tracie Sucharski - Fixed some memory leaks and
   *                          deleted some calls that were already handled in
   *                          AddMeasure.
   *   @history 2011-10-03 Tracie Sucharski - Unlock measures before Deleting
   */
  const ControlPoint &ControlPoint::operator=(const ControlPoint &other) {

    if (this != &other) {
      bool oldLock = editLock;
      editLock = false;
      for (int i = cubeSerials->size() - 1; i >= 0; i--) {
        (*measures)[cubeSerials->at(i)]->SetEditLock(false);
        Delete(cubeSerials->at(i));
      }

      //measures->clear(); = new QHash< QString, ControlMeasure * >;

      QHashIterator< QString, ControlMeasure * > i(*other.measures);
      while (i.hasNext()) {
        i.next();
        ControlMeasure *newMeasure = new ControlMeasure;
        *newMeasure = *i.value();
        AddMeasure(newMeasure);
        if (other.referenceMeasure == i.value()) {
          SetRefMeasure(newMeasure);
        }
      }

      invalid = other.invalid;
      referenceExplicitlySet   = other.referenceExplicitlySet;
      numberOfRejectedMeasures = other.numberOfRejectedMeasures;
      constraintStatus         = other.constraintStatus;

      SetId(other.id);
      SetChooserName(other.chooserName);
      SetDateTime(other.dateTime);
      SetType(other.type);
      SetRejected(other.jigsawRejected);
      SetIgnored(other.ignore);
      SetAprioriSurfacePointSource(other.aprioriSurfacePointSource);
      SetAprioriSurfacePointSourceFile(other.aprioriSurfacePointSourceFile);
      SetAprioriRadiusSource(other.aprioriRadiusSource);
      SetAprioriRadiusSourceFile(other.aprioriRadiusSourceFile);
      SetAprioriSurfacePoint(other.aprioriSurfacePoint);
      SetAdjustedSurfacePoint(other.adjustedSurfacePoint);

      // Set edit lock last so the it doesn't interfere with copying the other fields over.
      editLock = oldLock;
      SetEditLock(other.editLock);
    }

    return *this;
  }


  /**
   * Signal to indicate the point has been modified. Resets the last modified dateTime to null.
   */
  void ControlPoint::PointModified() {
    dateTime = "";
  }


  //! Initialize the number of rejected measures to 0
  void ControlPoint::ZeroNumberOfRejectedMeasures()

  {
    numberOfRejectedMeasures = 0;
  }


  /**
   * Set (update) the number of rejected measures for the control point
   *
   * @param numRejected    The number of rejected measures
   *
   */
  void ControlPoint::SetNumberOfRejectedMeasures(int numRejected) {
    numberOfRejectedMeasures = numRejected;
  }


  /**
   * Get the number of rejected measures on the control point
   *
   * @return The number of rejected measures on this control point
   *
   */
  int ControlPoint::GetNumberOfRejectedMeasures() const {
    return numberOfRejectedMeasures;
  }

  /**
   * Get rms of sample residuals
   *
   * @return The rms of sample residuals
   *
   */
  double ControlPoint::GetSampleResidualRms() const {
      int nmeasures = measures->size();
      if ( nmeasures <= 0 ) {
          return 0.0;
      }

      Statistics stats;

      for( int i = 0; i < nmeasures; i++) {
          const ControlMeasure* m = GetMeasure(i);
          if ( !m ) {
              continue;
          }

          if ( !m->IsIgnored() || m->IsRejected() ) {
              continue;
          }

          stats.AddData(m->GetSampleResidual());
      }

      return stats.Rms();
  }


  /**
   * Get rms of line residuals
   *
   * @return The rms of line residuals
   *
   */
  double ControlPoint::GetLineResidualRms() const {
      int nmeasures = measures->size();
      if ( nmeasures <= 0 ) {
          return 0.0;
      }

      Statistics stats;

      for( int i = 0; i < nmeasures; i++) {
          const ControlMeasure* m = GetMeasure(i);
          if ( !m ) {
              continue;
          }

          if ( !m->IsIgnored() || m->IsRejected() ) {
              continue;
          }

          stats.AddData(m->GetLineResidual());
      }

      return stats.Rms();
  }


  /**
   * Get rms of residuals
   *
   * @return The rms of residuals
   *
   */
  double ControlPoint::GetResidualRms() const {
      int nmeasures = measures->size();
      if ( nmeasures <= 0 ) {
          return 0.0;
      }

      Statistics stats;

      for( int i = 0; i < nmeasures; i++) {
          const ControlMeasure* m = GetMeasure(i);
          if ( !m ) {
              continue;
          }

          if ( m->IsIgnored() || m->IsRejected() ) {
              continue;
          }

          stats.AddData(m->GetSampleResidual());
          stats.AddData(m->GetLineResidual());
      }

      return stats.Rms();
  }

  /**
   * Set jigsaw rejected flag for all measures to false
   * and set the jigsaw rejected flag for the point itself to false
   *
   */
  void ControlPoint::ClearJigsawRejected() {
    int nmeasures = measures->size();
    if ( nmeasures <= 0 ) {
        return;
    }

    for( int i = 0; i < nmeasures; i++) {
      ControlMeasure* m = GetMeasure(i);
      if ( !m ) {
        continue;
      }

      m->SetRejected(false);
    }

    SetRejected(false);
  }

}
