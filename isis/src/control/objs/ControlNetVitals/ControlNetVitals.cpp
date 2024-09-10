/** This is free and unencumbered software released into the public domain.

The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */

#include "ControlNetVitals.h"

#include <QDateTime>
#include <QList>
#include <QPair>
#include <QVariant>

#include "IException.h"
#include "IString.h"
#include "ControlNet.h"
#include "ControlPoint.h"
#include "ControlMeasure.h"

namespace Isis {


  /**
   *  Constructs a ControlNetVitals object from a ControlNet.
   *  once complete, it calls the validate() method to evaluate the current status
   *  of the newly ingested Control Network.
   *
   *  @param cnet The Control Network that we will be tracking vitals for.
   */
  ControlNetVitals::ControlNetVitals(ControlNet *cnet) {
    m_controlNet = cnet;

    initializeVitals();

    connect(cnet, SIGNAL(networkModified(ControlNet::ModType)),
            this, SLOT(validateNetwork(ControlNet::ModType)));

    connect(cnet, SIGNAL(newPoint(ControlPoint*)),
            this, SLOT(addPoint(ControlPoint*)));
    connect(cnet, SIGNAL(pointModified(ControlPoint *, ControlPoint::ModType, QVariant, QVariant)),
            this, SLOT(pointModified(ControlPoint *, ControlPoint::ModType, QVariant, QVariant)));
    connect(cnet, SIGNAL(pointDeleted(ControlPoint*)),
            this, SLOT(deletePoint(ControlPoint*)));

    connect(cnet, SIGNAL(newMeasure(ControlMeasure*)),
            this, SLOT(addMeasure(ControlMeasure*)));
    connect(cnet, SIGNAL(measureModified(ControlMeasure *, ControlMeasure::ModType, QVariant, QVariant)),
            this, SLOT(measureModified(ControlMeasure *, ControlMeasure::ModType, QVariant, QVariant)));
    connect(cnet, SIGNAL(measureRemoved(ControlMeasure*)),
            this, SLOT(deleteMeasure(ControlMeasure*)));

    validate();
  }


  /**
   *  This will initialize all necessary values and set up the point measure and
   *  image measure QMaps appropriately.
   *
   */
  void ControlNetVitals::initializeVitals() {

    m_islandList = m_controlNet->GetSerialConnections();

    m_numPoints = 0;
    m_numPointsIgnored = 0;
    m_numPointsLocked = 0;
    m_numMeasures = m_controlNet->GetNumMeasures();

    m_pointMeasureCounts.clear();
    m_imageMeasureCounts.clear();
    m_pointTypeCounts.clear();

    m_pointTypeCounts.insert(ControlPoint::Free, 0);
    m_pointTypeCounts.insert(ControlPoint::Constrained, 0);
    m_pointTypeCounts.insert(ControlPoint::Fixed, 0);

    foreach(ControlPoint* point, m_controlNet->GetPoints()) {
      addPoint(point);
    }

    foreach(QString serial, m_controlNet->GetCubeSerials()) {
      int numValidMeasures = m_controlNet->GetNumberOfValidMeasuresInImage(serial);
      if ( !m_imageMeasureCounts.contains(numValidMeasures) ) {
        m_imageMeasureCounts.insert(numValidMeasures, 1);
      }
      else {
        m_imageMeasureCounts[numValidMeasures]++;
      }
    }
  }


  /**
   *  This method is designed to be called whenever a modification is made to the network,
   *  or any of it's control points or measures. It receives all of the components that
   *  make up the history entry (the comment, the ID of what was modified, oldValue, newValue)
   *  and emits them along with a timestamp of when the modification was made.
   *
   *  The historyEntry() will pass these values on to the listening SLOT in the health monitor
   *  widget so that it can be displayed in the history table.
   *
   *  @param entry The history comment that includes what modification was made.
   *  @param id The ID of the object modified. This can be a point id, measure serial, or net id.
   *  @param oldValue The oldValue the object had before the modification.
   *  @param newValue The newValue the object had after its modification.
   */
  void ControlNetVitals::emitHistoryEntry(QString entry, QString id, QVariant oldValue, QVariant newValue) {
    emit historyEntry(entry, id, oldValue, newValue, QDateTime::currentDateTime().toString());
  }


  /**
   *  This SLOT is designed to intercept the newPoint() signal emitted from a ControlNetwork
   *  whenever a new point has been added. It observes the Control Point and increments the
   *  appropriate internal counters to reflect the addition of this new point.
   *
   *  Once complete, we then call the validate() method to re-validate the status and
   *  details of the Control Network.
   *
   *  Unlike deletePoint(), this method does modify counters based on measures
   *  because the ControlNet does not emit separate measureAdded signals for
   *  efficiency reasons.
   *
   *  @param point The ControlPoint being added to the network.
   */
  void ControlNetVitals::addPoint(ControlPoint *point) {

    emitHistoryEntry("Control Point Added", point->GetId(), "", "");
    m_numPoints++;

    if (point->IsIgnored()) {
      m_numPointsIgnored++;
      return;
    }

    if (point->IsEditLocked()) {
      m_numPointsLocked++;
    }

    m_pointTypeCounts[point->GetType()]++;

    int numValidMeasures = point->GetNumValidMeasures();
    if ( !m_pointMeasureCounts.contains(numValidMeasures) ) {
      m_pointMeasureCounts.insert(numValidMeasures, 1);
    }

    else {
      m_pointMeasureCounts[numValidMeasures]++;
    }

    m_numMeasures = m_controlNet->GetNumMeasures();
    validate();
  }


  /**
   *  This SLOT is designed to receive a signal emitted from the Control Network
   *  whenever a modification is made to a Control Point. This SLOT receives the
   *  ControlPoint that was modified, as well with the ControlPoint::ModType enum
   *  indicating what type of modification was made to the Control Point.
   *
   *  We then increment or decrement the appropriate internal counters based on which type
   *  of modification was made to the Control Point.
   *
   *  Once complete, we then call the validate() method to re-validate the status and
   *  details of the Control Network.
   *
   *  @param point The Control Point that was modified in the observed Control Network.
   *  @param type The type of modification that was made to the Control Point.
   *  @param oldValue The old value (if any) of whatever modification was made to the Control Point.
   *  @param newValue The new value (if any) of whatever modification was made to the Control Point.
   */
  void ControlNetVitals::pointModified(ControlPoint *point, ControlPoint::ModType type,
                                       QVariant oldValue, QVariant newValue) {

    QString historyEntry;

    switch(type) {
      case ControlPoint::EditLockModified:

        historyEntry = "Point Edit Lock Modified";

        if (oldValue.toBool()) {
          m_numPointsLocked--;
        }

        if (newValue.toBool()) {
          m_numPointsLocked++;
        }

        emitHistoryEntry( historyEntry, point->GetId(),
                          oldValue, newValue );

        break;

      case ControlPoint::IgnoredModified:

        historyEntry = "Point Ignored Modified";

        if (oldValue.toBool()) {
          m_numPointsIgnored--;
          if (point->IsEditLocked()) {
            m_numPointsLocked++;
          }
          m_pointTypeCounts[point->GetType()]++;
          int numValidMeasures = point->GetNumValidMeasures();
          if ( !m_pointMeasureCounts.contains(numValidMeasures) ) {
            m_pointMeasureCounts.insert(numValidMeasures, 1);
          }
          else {
            m_pointMeasureCounts[numValidMeasures]++;
          }
        }

        if (newValue.toBool()) {
          m_numPointsIgnored++;
          if (point->IsEditLocked()) {
            m_numPointsLocked--;
          }
          m_pointTypeCounts[point->GetType()]--;
          int numValidMeasures = point->GetNumValidMeasures();
          if ( --m_pointMeasureCounts[numValidMeasures] < 1 ) {
            m_pointMeasureCounts.remove(numValidMeasures);
          }
        }

        emitHistoryEntry( historyEntry, point->GetId(),
                          oldValue, newValue );

        break;

      case ControlPoint::TypeModified:

        historyEntry = "Point Type Modified";

        m_pointTypeCounts[ControlPoint::PointType(oldValue.toInt())]--;
        m_pointTypeCounts[ControlPoint::PointType(newValue.toInt())]++;

        emitHistoryEntry( historyEntry, point->GetId(),
                          ControlPoint::PointTypeToString(ControlPoint::PointType(oldValue.toInt())),
                          ControlPoint::PointTypeToString(ControlPoint::PointType(newValue.toInt())) );

        break;

      default:
        // no operation
        break;
    }

    validate();

  }


  /**
   *  This method is designed to return the Control Point with the associated point id
   *  from the Control Network.
   *
   *  @param id The Point ID of the control point to be fetched.
   *  @return The Control Point with the associated point id.
   */
  ControlPoint* ControlNetVitals::getPoint(QString id) {
    return m_controlNet->GetPoint(id);
  }


  /**
   *  This SLOT is designed to intercept the removePoint() signal emitted by a Control Network
   *  whenever a point is deleted. It observes the to-be deleted point and decrements the
   *  appropriate internal counters to reflect the removal of this point.
   *
   *  This does not modify any counters based on the measures in the point
   *  because separate measureDeleted signals will be emitted by the ControlNet.
   *  addPoint does add modify counters based on measures because ControlNet does
   *  not emit separate measureAdded signals for efficiency reasons.
   *
   *  Once complete, we then call the validate() method to re-validate the status and
   *  details of the Control Network.
   *
   *  @param point The Control Point being deleted from the Control Network.
   */
  void ControlNetVitals::deletePoint(ControlPoint *point) {

    emitHistoryEntry("Control Point Deleted", point->GetId(), "", "");
    m_numPoints--;

    if (point->IsIgnored()) {
      m_numPointsIgnored--;
      validate();
      return;
    }
    if (point->IsEditLocked()) {
      m_numPointsLocked--;
    }

    m_pointTypeCounts[point->GetType()]--;

    int numValidMeasures= point->GetNumValidMeasures();
    if ( --m_pointMeasureCounts[numValidMeasures] < 1 ) {
      m_pointMeasureCounts.remove(numValidMeasures);
    }

    validate();
  }


  /**
   *  This SLOT is designed to intercept the newMeasure() signal emitted by a Control Network
   *  whenever a measure is added to one of it's Control Points.
   *  It grabs the parent Control Point of the Measure and decrements the pointMeasureCount QMap
   *  at the old measure count for the Control Point and increments the pointMeasureCount at the
   *  new measure count for the Control Point to reflect the addition of this measure.
   *
   *  This does not modify any counters based on the measures in the point
   *  because separate measureDeleted signals will be emitted by the ControlNet.
   *  addPoint does add modify counters based on measures because ControlNet does
   *  not emit separate measureAdded signals for efficiency reasons.
   *
   *  Once complete, we then call the validate() method to re-validate the status and
   *  details of the Control Network.
   *
   *  @param measure The Control Measure being added to a Control Point in the network.
   */
  void ControlNetVitals::addMeasure(ControlMeasure *measure) {
    emitHistoryEntry("Control Measure Added", measure->GetCubeSerialNumber(), "", "");

    m_numMeasures++;

    addMeasureToCounts(measure);

    validate();
  }


  /**
   *  This SLOT is designed to intercept the measureModified() signal emitted by a Control Network
   *  whenever a measure is modified in one of it's Control Points. This SLOT receives the
   *  ControlMeasure that was modified, as well with the ControlMeasure::ModType enum
   *  indicating what type of modification was made to the Control Measure. The appropriate methods
   *  are called depending on which modification was made.
   *
   *  Once complete, we then call the validate() method to re-validate the status and
   *  details of the Control Network.
   *
   *  @param point The Control Measure that was modified in the observed Control Network.
   *  @param type The type of modification that was made to the Control Measure.
   *  @param oldValue The old value (if any) of whatever modification was made to the Control Measure.
   *  @param newValue The new value (if any) of whatever modification was made to the Control Measure.
   */
  void ControlNetVitals::measureModified(ControlMeasure *measure, ControlMeasure::ModType type, QVariant oldValue, QVariant newValue) {

    QString historyEntry;

    switch (type) {
      case ControlMeasure::IgnoredModified:

        historyEntry = "Measure Ignored Modified";

        if ( !oldValue.toBool() && newValue.toBool() ) {
          return removeMeasureFromCounts(measure);
        }
        else if ( oldValue.toBool() && !newValue.toBool() ) {
          return addMeasureToCounts(measure);
        }
        break;

      default:
        // No operation.
        break;
    }

    ControlNetVitals::emitHistoryEntry(historyEntry, measure->GetCubeSerialNumber(), "", "");
    validate();
  }


  /**
   *  This SLOT is designed to intercept the measureRemoved() signal emitted by a Control Network
   *  whenever a Control Measure is deleted. It observes the to-be deleted point and decrements the
   *  appropriate internal counters to reflect the removal of this Control Measure.
   *
   *  Once complete, we then call the validate() method to re-validate the status and
   *  details of the Control Network.
   *
   *  @param point The Control Measure being deleted from a Control Point in the Control Network.
   */
  void ControlNetVitals::deleteMeasure(ControlMeasure *measure) {

    emitHistoryEntry("Control Measure Deleted", measure->GetCubeSerialNumber(), "", "");

    m_numMeasures--;

    removeMeasureFromCounts(measure);

    validate();
  }


  /**
   * Add a measure to the internal counters
   *
   * @param measure The measure to add
   */
   void ControlNetVitals::addMeasureToCounts(ControlMeasure *measure) {
     ControlPoint *point = measure->Parent();
     if (point) {
       // By this time, the measure has been added to its parent point, so the
       // old count is the current count minus one.
       int numValidMeasures = point->GetNumValidMeasures();
       if ( --m_pointMeasureCounts[numValidMeasures] < 1 ) {
         m_pointMeasureCounts.remove(numValidMeasures);
       }
       if ( !m_pointMeasureCounts.contains(numValidMeasures + 1) ) {
         m_pointMeasureCounts.insert(numValidMeasures + 1, 1);
       }
       else {
         m_pointMeasureCounts[numValidMeasures + 1]++;
       }
     }

     QString serial = measure->GetCubeSerialNumber();
     int numValidMeasures = m_controlNet->GetNumberOfValidMeasuresInImage(serial);
     if ( --m_imageMeasureCounts[numValidMeasures - 1] < 1 ) {
       m_imageMeasureCounts.remove(numValidMeasures);
     }
     if ( !m_imageMeasureCounts.contains(numValidMeasures) ) {
       m_imageMeasureCounts.insert(numValidMeasures, 1);
     }
     else {
       m_imageMeasureCounts[numValidMeasures]++;
     }
   }


   /**
    * Remove a measure from the internal counters
    *
    * @param measure The measure to remove
    */
    void ControlNetVitals::removeMeasureFromCounts(ControlMeasure *measure) {
      ControlPoint *point = measure->Parent();
      if (point) {
        // By this time, the measure is still a valid measure in the parent control point.
        int numValidMeasures = point->GetNumValidMeasures();

        if ( --m_pointMeasureCounts[numValidMeasures] < 1 ) {
          m_pointMeasureCounts.remove(numValidMeasures);
        }
        if ( !m_pointMeasureCounts.contains(numValidMeasures - 1) ) {
          m_pointMeasureCounts.insert(numValidMeasures - 1, 1);
        }
        else {
          m_pointMeasureCounts[numValidMeasures - 1]++;
        }
      }

      QString serial = measure->GetCubeSerialNumber();
      int numValidMeasures = m_controlNet->GetNumberOfValidMeasuresInImage(serial);

      if ( --m_imageMeasureCounts[numValidMeasures] < 1 ) {
        m_imageMeasureCounts.remove(numValidMeasures);
      }

      if ( !m_imageMeasureCounts.contains(numValidMeasures - 1) ) {
        m_imageMeasureCounts.insert(numValidMeasures - 1, 1);
      }
      else {
        m_imageMeasureCounts[numValidMeasures - 1]++;
      }
    }


  /**
   *  This SLOT is designed to intercept the networkModified() signal emitted by a Control Network
   *  whenever a modification is made to the network. This SLOT receives the ControlNet::ModType
   *  enum indicating what type of modication was made to the Control Network. It then acts
   *  based on what type of change was made.
   *
   *  Once complete, we then call the validate() method to re-validate the status and
   *  details of the Control Network.
   *
   *  @param point The type of modification that was made to the observed Control Network.
   */
  void ControlNetVitals::validateNetwork(ControlNet::ModType type) {

    QString historyEntry;

    switch (type) {
      case ControlNet::Swapped:
        emitHistoryEntry("Control Net Swapped", m_controlNet->GetNetworkId(), "", "");
        initializeVitals();
        break;
      case ControlNet::GraphModified:
        emitHistoryEntry("Control Net Graph Modified", m_controlNet->GetNetworkId(), "", "");
        m_islandList = m_controlNet->GetSerialConnections();
        break;
      default:
        // No operation.
        break;
    }
    validate();
  }


  /**
   *  De-constructor
   */
  ControlNetVitals::~ControlNetVitals() {
  }


  /**
   *  This method is designed to return true if islands exist in the ControlNet Graph
   *  and False otherwise.
   *
   *  @return True if islands exist, False otherwise.
   */
  bool ControlNetVitals::hasIslands() {
    return numIslands() > 1;
  }


  /**
   *  This method is designed to return the number of islands that exist in the
   *  ControlNet Graph.
   *
   *  @return The number of islands present in the ControlNet Graph.
   */
  int ControlNetVitals::numIslands() {
    return m_islandList.size();
  }


  /**
   *  This method is designed to return a QList containing each island present in the ControlNet.
   *
   *  Each island is composed of another QList containing the cube serials for all cubes in that island.
   *
   *  @return A QList containing a QList of cube serials for each island present in the Control Net.
   */
  const QList< QList<QString> > &ControlNetVitals::getIslands() {
    return m_islandList;
  }


  /**
   *  This method is designed to return the number of points in the Control Network.
   *
   *  @return The number of points in the Control Network.
   */
  int ControlNetVitals::numPoints() {
    return m_numPoints;
  }


  /**
   *  This method is designed to return the number of ignored points in the Control Network.
   *
   *  @return The number of ignored points in the Control Network.
   */
  int ControlNetVitals::numIgnoredPoints() {
    return m_numPointsIgnored;
  }


  /**
   *  This method is designed to return the number of edit locked points in the Control Network.
   *
   *  @return The number of edit locked points in the Control Network.
   */
  int ControlNetVitals::numLockedPoints() {
    return m_numPointsLocked;
  }


  /**
   *  This method is designed to return the number of fixed points in the Control Network.
   *
   *  @return The number of fixed points in the Control Network.
   */
  int ControlNetVitals::numFixedPoints() {
    return m_pointTypeCounts[ControlPoint::Fixed];
  }


  /**
   *  This method is designed to return the number of constrained points in the Control Network.
   *
   *  @return The number of constrained points in the Control Network.
   */
  int ControlNetVitals::numConstrainedPoints() {
    return m_pointTypeCounts[ControlPoint::Constrained];
  }


  /**
   *  This method is designed to return the number of free points in the Control Network.
   *
   *  @return The number of free points in the Control Network.
   */
  int ControlNetVitals::numFreePoints() {
    return m_pointTypeCounts[ControlPoint::Free];
  }


  /**
   *  This method is designed to return the number of points that fall below a measure threshold.
   *
   *  For instance, a measure threshold of 3 would return all points with less than 3 measures.
   *
   *  @param num The number of measures a point needs to have to meet the threshold.
   *  @return The number of points with number of measures less than the threshold.
   */
  int ControlNetVitals::numPointsBelowMeasureThreshold(int num) {
    int count = 0;

    QMap<int, int>::const_iterator i = m_pointMeasureCounts.constBegin();
    while (i != m_pointMeasureCounts.constEnd()) {
      if (i.key() >= num ) {
        break;
      }
      count += i.value();
      ++i;
    }

    return count;
  }


  /**
   *  This method is designed to return the number of images in the Control Network.
   *
   *  @return The number of images in the Control Network.
   */
  int ControlNetVitals::numImages() {
    return m_controlNet->GetCubeSerials().size();
  }


  /**
   *  This method is designed to return the number of measures in the Control Network.
   *
   *  @return The number of measures in the Control Network.
   */
  int ControlNetVitals::numMeasures() {
    return m_numMeasures;
  }


  /**
   *  This method is designed to return the number of images that fall below a measure threshold.
   *
   *  For instance, a measure threshold of 3 would return all images with less than 3 measures.
   *
   *  @param num The number of measures an image needs to have to meet the threshold.
   *  @return The number of images with number of measures less than the threshold.
   */
  int ControlNetVitals::numImagesBelowMeasureThreshold(int num) {
    int count = 0;

    QMap<int, int>::const_iterator i = m_imageMeasureCounts.constBegin();
    while (i != m_imageMeasureCounts.constEnd()) {
      if (i.key() >= num ) {
        break;
      }
      count += i.value();
      ++i;
    }
    return count;
  }


  // REFACTOR
  /**
   *  This method is designed to return the number of images that fall below a hull tolerance.
   *
   *  For instance, a tolerance of .75 would return all images with a hull tolerance of < 75%.
   *
   *  @param num The number of measures an image needs to have to meet the threshold.
   *  @return The number of images with number of measures less than the threshold.
   */
  int ControlNetVitals::numImagesBelowHullTolerance(int tolerance) {
    return 0;
  }


  /**
   *  This method is designed to return all cube serials present in the Control Network.
   *
   *  @return A QList<QString> containing all cube serials in the Control Network.
   */
  QList<QString> ControlNetVitals::getCubeSerials() {
    return m_controlNet->GetCubeSerials();
  }


  /**
   *  This method is designed to return all points in the Control Network.
   *
   *  @return A QList<ControlPoint*> containing all points in the Control Network.
   */
  QList<ControlPoint*> ControlNetVitals::getAllPoints() {
    return m_controlNet->GetPoints();
  }


  /**
   *  This method is designed to return all ignored points in the Control Network.
   *
   *  @return A QList<ControlPoint*> containing all ignored points in the Control Network.
   */
  QList<ControlPoint*> ControlNetVitals::getIgnoredPoints() {
    QList<ControlPoint*> ignoredPoints;
    foreach(ControlPoint* point, m_controlNet->GetPoints()) {
      if (point->IsIgnored()) ignoredPoints.append(point);
    }
    return ignoredPoints;
  }


  /**
   *  This method is designed to return all edit locked points in the Control Network.
   *
   *  @return A QList<ControlPoint*> containing all edit locked points in the Control Network.
   */
  QList<ControlPoint*> ControlNetVitals::getLockedPoints() {
    QList<ControlPoint*> lockedPoints;
    foreach(ControlPoint* point, m_controlNet->GetPoints()) {
      if (!point->IsIgnored() && point->IsEditLocked()) lockedPoints.append(point);
    }
    return lockedPoints;
  }


  /**
   *  This method is designed to return all fixed points in the Control Network.
   *
   *  @return A QList<ControlPoint*> containing all fixed points in the Control Network.
   */
  QList<ControlPoint*> ControlNetVitals::getFixedPoints() {
    QList<ControlPoint*> fixedPoints;
    foreach(ControlPoint* point, m_controlNet->GetPoints()) {
      if (!point->IsIgnored() && point->GetType() == ControlPoint::Fixed) fixedPoints.append(point);
    }
    return fixedPoints;
  }


  /**
   *  This method is designed to return all constrained points in the Control Network.
   *
   *  @return A QList<ControlPoint*> containing all constrained points in the Control Network.
   */
  QList<ControlPoint*> ControlNetVitals::getConstrainedPoints() {
    QList<ControlPoint*> constrainedPoints;
    foreach(ControlPoint* point, m_controlNet->GetPoints()) {
      if (!point->IsIgnored() && point->GetType() == ControlPoint::Constrained) constrainedPoints.append(point);
    }
    return constrainedPoints;
  }


  /**
   *  This method is designed to return all free points in the Control Network.
   *
   *  @return A QList<ControlPoint*> containing all free points in the Control Network.
   */
  QList<ControlPoint*> ControlNetVitals::getFreePoints() {
    QList<ControlPoint*> freePoints;
    foreach(ControlPoint* point, m_controlNet->GetPoints()) {
      if (!point->IsIgnored() && point->GetType() == ControlPoint::Free) freePoints.append(point);
    }
    return freePoints;
  }


  /**
   *  This method is designed to return all points that fall below a measure threshold.
   *
   *  For instance, a measure threshold of 3 would return all points with less than 3 measures.
   *
   *  @param num The number of measures a point needs to have to meet the threshold.
   *  @return All of points with number of measures less than the threshold.
   */
  QList<ControlPoint*> ControlNetVitals::getPointsBelowMeasureThreshold(int num) {
    QList<ControlPoint*> belowThreshold;
    foreach(ControlPoint* point, m_controlNet->GetPoints()) {
      if (!point->IsIgnored() && point->GetNumMeasures() < num) belowThreshold.append(point);
    }
    return belowThreshold;
  }


  /**
   *  This method is designed to return a QList containing cube serials for all images
   *  that fall below a measure threshold.
   *
   *  For instance, a measure threshold of 3 would return all images with less than 3 measures.
   *
   *  @param num The number of measures an image needs to have to meet the threshold.
   *  @return A QList<QString> containing all images with number of measures less than the threshold.
   */
  QList<QString> ControlNetVitals::getImagesBelowMeasureThreshold(int num) {
    QList<QString> imagesBelowThreshold;
    foreach(QString serial, m_controlNet->GetCubeSerials()) {
      if (m_controlNet->GetValidMeasuresInCube(serial).size() < num) {
        imagesBelowThreshold.append(serial);
      }
    }
    return imagesBelowThreshold;
  }


  /**
   *  This method is designed to return a QList containing cube serials for all images
   *  that fall below a convex hull tolerance threshold.
   *
   *  For instance, a tolerance of .75 would return all images with a hull tolerance less than 75%.
   *
   *  @param num The hull tolerance (decimal percent) an image needs to meet the threshold.
   *  @return A QList<QString> containing all images with a hull tolerance below the threshold.
   */
  QList<QString> ControlNetVitals::getImagesBelowHullTolerance(int num) {
    QList<QString> list;
    return list;
  }


  /**
   *  This method is designed to return the current status of the network.
   *
   *  The possible values are "Healthy!", "Weak!", "Broken!"
   *
   *  @return A QString indicating the status of the Control Network.
   */
  QString ControlNetVitals::getStatus() {
    return m_status;
  }


  /**
   *  This method is designed to return details for the status of the network.
   *
   *  This QString could contain several details if the status is weak, if more than one
   *  factor contribute to the weakness of the network. Details are separated by newline '\n'
   *  in the QString.
   *
   *  @return A QString containing details for the status of the Control Network.
   */
  QString ControlNetVitals::getStatusDetails() {
    return m_statusDetails;
  }


  /**
   *  This method is designed to return networkId of the observed Control Network.
   *
   *  It is a wrapper for the ControlNet::GetNetworkId() call of the observed Control Network.
   *
   *  @return A QString containing the NetworkId of the Control Network.
   */
  QString ControlNetVitals::getNetworkId() {
    return m_controlNet->GetNetworkId();
  }


  /**
   *  This method is designed to evaluate the current vitals of the network to determine
   *  if any weaknesses are present and update the status of the network.
   *
   *  The network status is split into 3 states: Healthy, Weak, and Broken.
   *
   *  Healthy - A network is healthy if there are no weaknesses found and it is not broken.
   *  Weak    - A network is weak if it has points that fall below the measure threshold,
   *            A network is weak if it has images that fall below the measure threshold,
   *            A network is weak if it has images that fall below a Convex Hull tolerance.
   *  Broken  - A network is broken if it has more than 1 island.
   */
  void ControlNetVitals::validate() {

    QString status = "";
    QString details = "";
    if (hasIslands()) {
      status = "Broken!";
      details = "This network has " + QString::number(numIslands()) + " islands.";
    }
    else {

      if (numPointsBelowMeasureThreshold() > 0) {
        status = "Weak!";
        details += "This network has " + QString::number(numPointsBelowMeasureThreshold()) + " point(s) with less than 3 measures\n";
      }

      if (numImagesBelowMeasureThreshold() > 0) {
        status = "Weak!";
        details += "This network has " + QString::number(numImagesBelowMeasureThreshold()) + " image(s) with less than 3 measures\n";
      }

      if (numImagesBelowHullTolerance() > 0) {
        status = "Weak!";
        details += "This network has " + QString::number(numImagesBelowHullTolerance()) + " image(s) below the Convex Hull Tolerance of 75%\n";
      }

      if (status.isEmpty()) {
        status = "Healthy!";
        details = "This network is healthy.";
      }
    }
    m_status = status;
    m_statusDetails = details;
    emit networkChanged();
  }

}
