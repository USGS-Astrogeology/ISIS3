/** This is free and unencumbered software released into the public domain.

The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */

#include "ControlNet.h"

#include <iostream>
#include <cmath>
#include <sstream>

#include <QDebug>
#include <QMutex>
#include <QMutexLocker>
#include <QPair>
#include <QScopedPointer>
#include <QSet>
#include <QStringList>
#include <QTime>
#include <QVariant>
#include <QVector>

#include <boost/numeric/ublas/symmetric.hpp>
#include <boost/numeric/ublas/io.hpp>

#include "Application.h"
#include "CameraFactory.h"
#include "ControlMeasure.h"
#include "ControlNetVersioner.h"
#include "ControlPoint.h"
#include "Distance.h"
#include "FileName.h"
#include "IException.h"
#include "iTime.h"
#include "Progress.h"
#include "SerialNumberList.h"
#include "SpecialPixel.h"
#include "Statistics.h"
#include "Target.h"

using namespace std;
using namespace boost::numeric::ublas;


namespace Isis {

  void ControlNet::nullify() {

    points = NULL;
    pointIds = NULL;
    m_mutex = NULL;
  }

  //!Creates an empty ControlNet object
  ControlNet::ControlNet(SurfacePoint::CoordinateType coordType) {

  //! Creates an empty ControlNet object
  // ControlNet::ControlNet() {
    nullify();

    points = new QHash< QString, ControlPoint * >;
    pointIds = new QStringList;

    m_ownPoints = true;
    p_created = Application::DateTime();
    p_modified = Application::DateTime();
    m_coordType = coordType;
  }

  ControlNet::ControlNet(const ControlNet &other) {

    nullify();

    points = new QHash< QString, ControlPoint * >;
    pointIds = new QStringList;

    for (int cpIndex = 0; cpIndex < other.GetNumPoints(); cpIndex++) {
      ControlPoint *newPoint = new ControlPoint(*other.GetPoint(cpIndex));
      AddPoint(newPoint);
    }

    m_ownPoints = true;

    p_targetName = other.p_targetName;
    p_networkId = other.p_networkId;
    p_created = other.p_created;
    p_modified = other.p_modified;
    p_description = other.p_description;
    p_userName = other.p_userName;
    p_cameraMap = other.p_cameraMap;
    p_cameraList = other.p_cameraList;
    m_coordType = other.m_coordType;
  }


  /**
   * Creates a ControlNet object from the given file
   *
   * @param ptfile Name of network file
   * @param progress A pointer to the progress of reading in the control points
   */
  ControlNet::ControlNet(const QString &ptfile, Progress *progress,
                         SurfacePoint::CoordinateType coordType) {

    nullify();

    points = new QHash< QString, ControlPoint * >;
    pointIds = new QStringList;

    m_ownPoints = true;
    m_coordType = coordType;

    try {
      ReadControl(ptfile, progress);
    }
    catch (IException &e) {
      std::string msg = "Invalid control network [" + ptfile + "]";
      throw IException(e, IException::Io, msg, _FILEINFO_);
    }
  }


 /**
  *  @brief Destructor removes allocated memory
  *
  * @author Kris Becker
  */
  ControlNet::~ControlNet() {

    clear();

    delete points;
    delete pointIds;

    nullify();
  }


  /**
   * This method is a wrapper to emit the measureModified() signal and is called whenever a change
   * is made to a Control Measure.
   *
   * @param measure The ControlMeasure* that was modified.
   * @param type The ControlMeasure::ModType indicating which modification occured.
   * @param oldValue The oldValue before the change.
   * @param newValue The new value that the change incorporated.
   */
  void ControlNet::emitMeasureModified(ControlMeasure *measure, ControlMeasure::ModType type, QVariant oldValue, QVariant newValue) {
    emit measureModified(measure, type, oldValue, newValue);
  }


  /**
   * This method is a wrapper to emit the pointModified() signal and is called whenever a change
   * is made to a Control Point.
   *
   * @param point The ControlPoint* that was modified.
   * @param type The ControlPoint::ModType indicating which modification occured.
   * @param oldValue The oldValue before the change.
   * @param newValue The new value that the change incorporated.
   */
  void ControlNet::emitPointModified(ControlPoint *point, ControlPoint::ModType type, QVariant oldValue, QVariant newValue) {
    emit pointModified(point, type, oldValue, newValue);
  }


 /**
  *  @brief Clear the contents of this object
  *
  *  The contents of the ControlNet object are deleted. The internal variables
  *  that hold the contents are not. See the destructor.
  *
  * @author Kris Becker
  */
  void ControlNet::clear() {

    // Now must also own points to delete them.
    if (points) {
      if (GetNumPoints() > 0) {
        if (m_ownPoints) {
          QHashIterator<QString, ControlPoint*> i(*points);
          while (i.hasNext()) {
            i.next();
            delete(*points)[i.key()];
            (*points)[i.key()] = NULL;
          }
        }
      }
      points->clear();
    }

    m_vertexMap.clear();
    m_controlGraph.clear();

    if (pointIds) {
      pointIds->clear();
    }

    return;
  }


  /**
   * @brief Transfer ownership of all points to caller
   *
   * This method is used to transfer ownership to the caller. This method is not
   * reintrant in the sense that if someone else has already made this call, it is
   * an error to attempt to take ownership again.
   *
   * Note that it now becomes the responsibility of the caller to delete all the
   * pointers to ControlPoints that are returned in the list.
   *
   * WARNING!!! This call alone can create a situation where the owner could
   * delete point memory after the point list is exported from this class creating
   * a problem. For this reason, the clear() method be called!!!
   *
   * @author Kris Becker
   *
   * @return QList<ControlPoint*> Returns the list of all control points to caller
   */
    QList< ControlPoint * > ControlNet::take() {

      // First check to see if someone else has taken ownership
      if (!m_ownPoints) {
        throw IException(IException::Programmer, "Ownership has already been taken",
                         _FILEINFO_);
      }

      QList<ControlPoint *> points = GetPoints();
      m_ownPoints = false;
      clear();

      // Disconnect the parent network reference
      for (int i = 0; i < points.size(); i++) {
        points[i]->parentNetwork = NULL;
      }
      return (points);
    }


  /**
   * Reads in the control points from the given file
   *
   * @param ptfile Name of file containing a Pvl list of control points
   * @param progress A pointer to the progress of reading in the control points
   *
   * @throws Isis::iException::User - "Invalid Network Type"
   * @throws Isis::iException::User - "Invalid Control Point"
   * @throws Isis::iException::User - "Invalid Format"
   *
   * @internal
   * @history 2009-04-07 Tracie Sucharski - Keep track of ignored measures.
   * @history 2010-08-06 Tracie Sucharski, Updated for changes made after
   *                           additional working sessions for Control network
   *                           design.
   * @history 2011-04-02 Debbie A. Cook - Added code to set the target radii
   *                           in the surface points of the control points as
   *                           they are read into memory instead of setting
   *                           parent prematurely to be able to set the radii
   *                           in ControlPoint.
   * @history 2017-12-21 Jesse Mapel - Modified to use the ControlNetVersioner.
   * @history 2018-04-05 Adam Goins - Added a check to the versionedReader targetRadii
   *                         group to set radii values to those ingested from the versioner
   *                         if they exist. Otherwise, we call SetTarget with the targetname.
   */
  void ControlNet::ReadControl(const QString &filename, Progress *progress) {

    FileName cnetFileName(filename.toStdString());
    ControlNetVersioner versionedReader(cnetFileName, progress);
    SetTarget( versionedReader.targetName() );
    p_networkId   = versionedReader.netId();
    p_userName    = versionedReader.userName();
    p_created     = versionedReader.creationDate();
    p_modified    = versionedReader.lastModificationDate();
    p_description = versionedReader.description();

    int numPoints = versionedReader.numPoints();

    if (progress) {
      progress->SetText("Adding Control Points to Network...");
      progress->SetMaximumSteps(numPoints);
      progress->CheckStatus();
    }

    for (int i = 0; i < numPoints; i++) {
      AddPoint( versionedReader.takeFirstPoint() );
      if (progress) {
        progress->CheckStatus();
      }
    }
  }


  /**
   * Writes out the control network
   *
   * @param ptfile Name of file containing a Pvl list of control points
   * @param pvl    Boolean indicating whether to write in pvl format
   *               (Default=false)
   *
   * @history 2010-10-05 Tracie Sucharski - Renamed old WRite method to WritePvl
   *                     and created this new method to determine format to
   *                     be written.
   * @history 2017-12-21 Jesse Mapel - Modified to use new ControlNetVersioner.
   */
  void ControlNet::Write(const QString &ptfile, bool pvl) {
    ControlNetVersioner versionedWriter(this);

    if (pvl) {
      Pvl network;
      try {
        network = versionedWriter.toPvl();
      }
      catch (IException &e) {
        std::string msg = "Failed to convert control network to Pvl format.";
        throw IException(e, IException::Programmer, msg, _FILEINFO_);
      }

      try {
        network.write(ptfile.toStdString());
      }
      catch (IException &e) {
        std::string msg = "Failed writing control network to file [" + ptfile + "]";
        throw IException(e, IException::Io, msg, _FILEINFO_);
      }
    }
    else {
      try {
        versionedWriter.write(QString::fromStdString((FileName(ptfile).toString())));
      }
      catch (IException &e) {
        std::string msg = "Failed writing control network to file [" + ptfile + "]";
        throw IException(e, IException::Io, msg, _FILEINFO_);
      }
    }
  }


  /**
   * Adds a ControlPoint to the ControlNet
   *
   * @param point Control point to be added
   *
   * @throws IException::Programmer "Null pointer passed to ControlNet::AddPoint!"
   * @throws IException::Programmer "ControlPoint must have unique Id"
   */
  void ControlNet::AddPoint(ControlPoint *point) {
    if (!point) {
      IString msg = "Null pointer passed to ControlNet::AddPoint!";
      throw IException(IException::Programmer, msg, _FILEINFO_);
    }

    if (ContainsPoint(point->GetId())) {
      IString msg = "ControlPoint must have unique Id";
      throw IException(IException::Programmer, msg, _FILEINFO_);
    }

    QString pointId = point->GetId();
    points->insert(pointId, point);
    pointIds->append(pointId);

    point->parentNetwork = this;

    // notify control network of new point
    pointAdded(point);

    emit networkStructureModified();
  }


 /**
   * Adds a whole point to the control net graph.
   *
   * @throws IException::Programmer "NULL measure passed to ControlNet::pointAdded!"
   * @throws IException::Programmer "Control measure with NULL parent passed to
   *     ControlNet::pointAdded"
   * @throws IException::Programmer "ControlNet does not contain the point."
   */
  void ControlNet::pointAdded(ControlPoint *point) {
    if (!point) {
      IString msg = "NULL point passed to "
          "ControlNet::pointAdded!";
      throw IException(IException::Programmer, msg, _FILEINFO_);
    }

    if (!ContainsPoint(point->GetId())) {
      std::string msg = "ControlNet does not contain the point [";
      msg += point->GetId() + "]";
      throw IException(IException::Programmer, msg, _FILEINFO_);
    }

    // Make sure there is a node for every measure
    for (int i = 0; i < point->GetNumMeasures(); i++) {
      QString sn = point->GetMeasure(i)->GetCubeSerialNumber();
      // If the graph doesn't have the sn, add a node for it
      if (!m_vertexMap.contains(sn)) {
        Image newImage;
        newImage.serial = sn;
        ImageVertex newVertex = boost::add_vertex(newImage, m_controlGraph);
        m_vertexMap.insert(sn, newVertex);
        emit networkModified(GraphModified);
      }
    }

    QList< ControlMeasure * > measures = point->getMeasures();
    for(int i = 0; i < measures.size(); i++) {
      ControlMeasure* measure = measures[i];
      // Add the measure to the corresponding node
      QString serial = measure->GetCubeSerialNumber();
      m_controlGraph[m_vertexMap[serial]].measures[measure->Parent()] = measure;

      // In this measure's node add connections to the other nodes reachable from
      // its point
      if (!point->IsIgnored() && !measure->IsIgnored()) {
        for (int j = i + 1; j < measures.size(); j++) {
          ControlMeasure *cm = measures[j];
          if (!cm->IsIgnored()) {
            QString sn = cm->GetCubeSerialNumber();
            addEdge(serial, sn);
          }
        }
      }
    }
    emit newPoint(point);
  }


  /**
   * In the ControlNet graph: adds an edge between the verticies associated with the two serial
   * numbers provided. Or, if the edge already exists, increments the strength of the edge.
   *
   * @param sourceSerial The first serial to be connected by the edge
   * @param targetSerial The second serial number to be connected by the edge
   *
   * @return bool true if a new edge was added, false otherwise.
  */
  bool ControlNet::addEdge(QString sourceSerial, QString targetSerial) {
    // If the edge doesn't already exist, this adds and returns the edge.
    // If the edge already exists, this just returns it. (The use of a set
    // forces the edges to be unique.)
    ImageConnection connection;
    bool edgeAdded;

    boost::tie(connection, edgeAdded) = boost::add_edge(m_vertexMap[sourceSerial],
                                                        m_vertexMap[targetSerial],
                                                        m_controlGraph);
    m_controlGraph[connection].strength++;
    if (edgeAdded) {
      emit networkModified(GraphModified);
    }
    return edgeAdded;
  }


  /**
   * In the ControlNet graph, decrements the strength on the edge between the two serial numbers.
   * This is called when the ControlMeasures that connect these images are deleted or ignored.
   * If it is the last measure connecting two verticies (serial numbers) the edge is removed.
   *
   * @param sourceSerial The first serial number defining the end of the edge to have its strength
   *                     decremented or be removed.
   * @param targetSerial The second serial number defining the other end of the edge to have its
   *                     strength decremented or be removed.
   * @return bool true if the edge is removed, otherwise false
   */
  bool ControlNet::removeEdge(QString sourceSerial, QString targetSerial) {
    ImageConnection connection;
    bool edgeExists;
    boost::tie(connection, edgeExists) = boost::edge(m_vertexMap[sourceSerial],
                                                     m_vertexMap[targetSerial],
                                                     m_controlGraph);
    if (edgeExists) {
      m_controlGraph[connection].strength--;
      if (m_controlGraph[connection].strength <= 0) {
        boost::remove_edge(m_vertexMap[sourceSerial],
                           m_vertexMap[targetSerial],
                           m_controlGraph);
        emit networkModified(GraphModified);

        return true;
      }
    }
    return false;
  }


  /**
   * Used for verifying graph intergrity
   *
   * @returns A string representation of the ControlNet graph
   */
  QString ControlNet::GraphToString() const {
    QString graphString;

    QStringList images = GetCubeSerials();
    images.sort();

    QHash<QString, QStringList> imagePointIds;

    foreach(QString imageSerial, images) {
      QList<ControlPoint *> imagePoints = m_controlGraph[m_vertexMap[imageSerial]].measures.keys();
      QStringList pointIds;
      foreach(ControlPoint *point, imagePoints) {
        if (!point->IsIgnored()) {
          pointIds.append(point->GetId());
        }
      }
      pointIds.sort();
      imagePointIds.insert(imageSerial, pointIds);
    }

    foreach(QString imageSerial, images) {
      QStringList adjacentImages = getAdjacentImages(imageSerial);
      adjacentImages.sort();
      foreach(QString adjacentSerial, adjacentImages) {
        if (QString::compare(adjacentSerial, imageSerial) < 0) {
          continue;
        }

        QStringList commonPoints;
        QList<QString>::const_iterator imageIt = imagePointIds[imageSerial].cbegin();
        QList<QString>::const_iterator adjacentIt = imagePointIds[adjacentSerial].cbegin();
        while (imageIt != imagePointIds[imageSerial].cend() &&
               adjacentIt != imagePointIds[adjacentSerial].cend()) {
          int stringDiff = QString::compare(*imageIt, *adjacentIt);
          if (stringDiff < 0) {
            imageIt++;
          }
          else if(stringDiff == 0) {
            commonPoints.append(*imageIt);
            imageIt++;
            adjacentIt++;
          }
          else {
            adjacentIt++;
          }
        }

        std::pair<ImageConnection, bool> result = boost::edge(m_vertexMap[imageSerial],
                                                              m_vertexMap[adjacentSerial],
                                                              m_controlGraph);
        QString edgeStrength = "UNKNOWN";
        if (result.second) {
          edgeStrength = QString::fromStdString(toString(m_controlGraph[result.first].strength));
        }

        graphString.append(imageSerial);
        graphString.append(" ----(");
        graphString.append(edgeStrength);
        graphString.append(") [");
        graphString.append(commonPoints.join(","));
        graphString.append("]---- ");
        graphString.append(adjacentSerial);
        graphString.append("\n");
      }
    }

    return graphString;
   }


   /**
   * Updates the ControlNet graph for the measure's serial number to
   * reflect the addition.  If there is currently no node for
   * this measure's serial, then a new node is created with
   * this measure as its first.
   *
   * @param measure The measure added to the network.
   *
   * @throws IException::Programmer "NULL measure passed to ControlNet::measureAdded!"
   * @throws IException::Programmer "Control measure with NULL parent passed to
   *     ControlNet::measureAdded!"
   * @throws IException::Programmer "ControlNet does not contain the point."
   */
  void ControlNet::measureAdded(ControlMeasure *measure) {
    if (!measure) {
      IString msg = "NULL measure passed to "
          "ControlNet::measureAdded!";
      throw IException(IException::Programmer, msg, _FILEINFO_);
    }

    ControlPoint *point = measure->Parent();
    if (!point) {
      IString msg = "Control measure with NULL parent passed to "
          "ControlNet::measureAdded!";
      throw IException(IException::Programmer, msg, _FILEINFO_);
    }

    if (!ContainsPoint(point->GetId())) {
      std::string msg = "ControlNet does not contain the point [";
      msg += point->GetId() + "]";
      throw IException(IException::Programmer, msg, _FILEINFO_);
    }
    // Add the measure to the corresponding node
    QString serial = measure->GetCubeSerialNumber();

    // If the graph doesn't have the sn, add a node for it
    if (!m_vertexMap.contains(serial)) {
      Image newImage;
      newImage.serial = serial;
      ImageVertex newVertex = boost::add_vertex(newImage, m_controlGraph);
      m_vertexMap.insert(serial, newVertex);
      emit networkModified(GraphModified);
    }

    m_controlGraph[m_vertexMap[serial]].measures[measure->Parent()] = measure;

    // in this measure's node add connections to the other nodes reachable from
    // its point
    if (!point->IsIgnored() && !measure->IsIgnored()) {
      for (int i = 0; i < point->GetNumMeasures(); i++) {
        ControlMeasure *cm = point->GetMeasure(i);
        if (!cm->IsIgnored()) {
          QString sn = cm->GetCubeSerialNumber();


          if (QString::compare(sn, serial) != 0) {
            addEdge(serial, sn);
          }
        }
      }
    }
    emit newMeasure(measure);
  }


  /**
   * Update the ControlNet's internal structure when a ControlPoint is un-ignored.
   *
   * @param point A pointer to the un-ignored point.
   */
  void ControlNet::pointUnIgnored(ControlPoint *point) {
    if (!point) {
      IString msg = "NULL point passed to "
          "ControlNet::pointUnIgnored!";
      throw IException(IException::Programmer, msg, _FILEINFO_);
    }

    QList< ControlMeasure * > validMeasures = point->getMeasures(true);

    for (int i = 0; i < validMeasures.size(); i++) {
      ControlMeasure *sourceMeasure = validMeasures[i];
      QString sourceSerial = sourceMeasure->GetCubeSerialNumber();

      if (!ValidateSerialNumber(sourceSerial)) {
        std::string msg = "Node does not exist for [";
        msg += sourceSerial + "]";
        throw IException(IException::Programmer, msg, _FILEINFO_);
      }

      for(int j = i+1; j < validMeasures.size(); j++) {
        ControlMeasure *targetMeasure = validMeasures[j];
        QString targetSerial = targetMeasure->GetCubeSerialNumber();

        if (!ValidateSerialNumber(targetSerial)) {
          std::string msg = "Node does not exist for [";
          msg += targetSerial + "]";
          throw IException(IException::Programmer, msg, _FILEINFO_);
        }
        addEdge(sourceSerial, targetSerial);
      }
    }
  }


  /**
   * Updates the connections for the ControlNet graph associated with the
   * measure's serial number to reflect the unignoration.
   *
   * @param measure The measure unignored from the network.
   *
   * @throws IException::Programmer "NULL measure passed to ControlNet::measureUnIgnored!"
   * @throws IException::Programmer "Control measure with NULL parent passed to
   *     ControlNet::measureUnIgnored!"
   * @throws IException::Programmer "ControlNet does not contain the point."
   * @throws IException::Programmer "Node does not exist for the cube serial number."
   */
  void ControlNet::measureUnIgnored(ControlMeasure *measure) {
    if (!measure) {
      IString msg = "NULL measure passed to "
          "ControlNet::measureUnIgnored!";
      throw IException(IException::Programmer, msg, _FILEINFO_);
    }

    ControlPoint *point = measure->Parent();
    if (!point) {
      IString msg = "Control measure with NULL parent passed to "
          "ControlNet::measureUnIgnored!";
      throw IException(IException::Programmer, msg, _FILEINFO_);
    }

    if (!ContainsPoint(point->GetId())) {
      std::string msg = "ControlNet does not contain the point [";
      msg += point->GetId() + "]";
      throw IException(IException::Programmer, msg, _FILEINFO_);
    }

    // Make sure there is a node for every measure in this measure's parent
    for (int i = 0; i < point->GetNumMeasures(); i++) {
      ControlMeasure *adjacentMeasure = point->GetMeasure(i);
      QString sn = adjacentMeasure->GetCubeSerialNumber();
      if (!adjacentMeasure->IsIgnored() && !m_vertexMap.contains(sn)) {
        std::string msg = "Node does not exist for [";
        msg += measure->GetCubeSerialNumber() + "]";
        throw IException(IException::Programmer, msg, _FILEINFO_);
      }
    }

    if (!point->IsIgnored()) {
      QString serial = measure->GetCubeSerialNumber();

      // In this measure's node add connections to the other nodes reachable
      // from its point
      for (int i = 0; i < point->GetNumMeasures(); i++) {
        ControlMeasure *cm = point->GetMeasure(i);
        if (!cm->IsIgnored()) {
          QString sn = cm->GetCubeSerialNumber();

          if (QString::compare(sn, serial) != 0) {
            addEdge(serial, sn);
          }
        }
      }
    }
  }


  /**
   * Updates the key reference (poind Id) from the old one to what the point
   * id was changet to. This method should only be called from ControlPoint's
   * SetId().
   *
   * @param point The point that needs to be updated.
   * @param oldId The pointId that the point had.
   */
  void ControlNet::UpdatePointReference(ControlPoint *point, QString oldId) {
    points->remove(oldId);
    (*points)[point->GetId()] = point;
    (*pointIds)[pointIds->indexOf((QString) oldId)] = (QString)point->GetId();
  }


  /**
   * Updates the node for this measure's serial number to
   * reflect the deletion.
   *
   * @param measure The measure removed from the network.
   */
  void ControlNet::measureDeleted(ControlMeasure *measure) {
    QString serial = measure->GetCubeSerialNumber();

    emit measureRemoved(measure);

    // Remove connections to and from this node
    if (!measure->IsIgnored() && !measure->Parent()->IsIgnored()) {
      // Break connections
      measureIgnored(measure);
    }

    // Remove the measure from the associated node.
    // Conceptually, I think this belongs in measureIgnored, but it isn't done
    // for the old graph.
    m_controlGraph[m_vertexMap[serial]].measures.remove(measure->Parent());
  }


  /**
   * Update the ControlNet's internal structure when a ControlPoint is ignored.
   *
   * @param point A pointer to the ignored point.
   */
  void ControlNet::pointIgnored(ControlPoint *point) {
    if (!point) {
      IString msg = "NULL point passed to "
          "ControlNet::pointIgnored!";
      throw IException(IException::Programmer, msg, _FILEINFO_);
    }

    QList< ControlMeasure * > validMeasures = point->getMeasures(true);

    for (int i = 0; i < validMeasures.size(); i++) {
      ControlMeasure *sourceMeasure = validMeasures[i];
      QString sourceSerial = sourceMeasure->GetCubeSerialNumber();

      if (!ValidateSerialNumber(sourceSerial)) {
        std::string msg = "Node does not exist for [";
        msg += sourceSerial + "]";
        throw IException(IException::Programmer, msg, _FILEINFO_);
      }

      for(int j = i+1; j < validMeasures.size(); j++) {
        ControlMeasure *targetMeasure = validMeasures[j];
        QString targetSerial = targetMeasure->GetCubeSerialNumber();

        if (!ValidateSerialNumber(targetSerial)) {
          std::string msg = "Node does not exist for [";
          msg += targetSerial + "]";
          throw IException(IException::Programmer, msg, _FILEINFO_);
        }
        removeEdge(sourceSerial, targetSerial);
      }
    }
  }



  /**
   * Updates the edges in the ControlNet graph to reflect the ignored
   * measure. If this was the last measure connecting one node to another,
   * then the edge is deleted as well.
   *
   * @param measure The measure set to ignored from the network.
   */
  void ControlNet::measureIgnored(ControlMeasure *measure) {
    if (!measure) {
      IString msg = "NULL measure passed to "
          "ControlNet::measureIgnored!";
      throw IException(IException::Programmer, msg, _FILEINFO_);
    }

    ControlPoint *point = measure->Parent();
    if (!point) {
      IString msg = "Control measure with NULL parent passed to "
          "ControlNet::measureIgnored!";
      throw IException(IException::Programmer, msg, _FILEINFO_);
    }

    QString serial = measure->GetCubeSerialNumber();
    if (!ValidateSerialNumber(serial)) {
      std::string msg = "Node does not exist for [";
      msg += serial + "]";
      throw IException(IException::Programmer, msg, _FILEINFO_);
    }

    // Decrement the edge strength for edges from this node
    // Remove edge if the strength becomes 0.
    for (int i = 0; i < point->GetNumMeasures(); i++) {
      ControlMeasure *adjacentMeasure = point->GetMeasure(i);
      QString sn = adjacentMeasure->GetCubeSerialNumber();
      if (!adjacentMeasure->IsIgnored() && m_vertexMap.contains(sn)) {
        if (QString::compare(serial, sn) !=0) {
          removeEdge(serial, sn);
        }
      }
    }
  }


  /**
   * Sets the control point coordinate type
   *
   * @param coordType Control point coordinate type
   */
  void ControlNet::SetCoordType(SurfacePoint::CoordinateType coordType)  {
    m_coordType = coordType;
  }


  /**
   * This method is a wrapper to emit the networkStructureModified() signal.
   */
  void ControlNet::emitNetworkStructureModified() {
    emit networkStructureModified();
  }


  /**
   * Delete a ControlPoint from the network by the point's address.
   *
   * @param point The point to delete
   */
  int ControlNet::DeletePoint(ControlPoint *point) {
    if (points->values().contains(point)) {
      return DeletePoint(point->GetId());
    }
    else {
      IString msg = "point [";
      msg += (long) point;
      msg += "] does not exist in the network";
      throw IException(IException::User, msg, _FILEINFO_);
    }
  }


  /**
   * Delete a ControlPoint from the network using the point's Id.
   *
   * @param pointId The Point Id of the ControlPoint to be deleted.
   *
   * @throw IException::User "the point Id does not exist in the network"
   */
  int ControlNet::DeletePoint(QString pointId) {
    if (!points->contains(pointId)) {
      std::string msg = "point Id [" + pointId.toStdString() + "] does not exist in the network";
      throw IException(IException::User, msg, _FILEINFO_);
    }

    ControlPoint *point = (*points)[pointId];

    if (point->IsEditLocked())
      return ControlPoint::PointLocked;

    bool wasIgnored = point->IsIgnored();

    // notify CubeSerialNumbers of the loss of this point
    foreach(ControlMeasure * measure, point->getMeasures()) {
      point->Delete(measure);
    }

    emit pointDeleted(point);

    // delete point
    points->remove(pointId);
    pointIds->removeAt(pointIds->indexOf(pointId));
    delete point;
    point = NULL;

    if (!wasIgnored)
      emit networkStructureModified();
    return ControlPoint::Success;
  }


  /**
   * Delete a ControlPoint from the network using the point's index.
   *
   * @param index The index of the Control Point to be deleted.
   */
  int ControlNet::DeletePoint(int index) {
    if (index < 0 || index >= pointIds->size()) {
      IString msg = "Index [" + IString(index) + "] out of range";
      throw IException(IException::Programmer, msg, _FILEINFO_);
    }

    return DeletePoint(pointIds->at(index));
  }


  /**
   * @param pointId the point Id to check for in the network.
   *
   * @returns True if the point is in the network, false otherwise.
   */
  bool ControlNet::ContainsPoint(QString pointId) const {
    return points->contains(pointId);
  }


  /**
   * This method searches through all the cube serial numbers in the network.
   * Serials which are connected to other serials through points are grouped
   * together in the same lists.  The list containing the lists of strings is
   * nothing more than a list of islands such that each island is a list of
   * serials which are connected to each other.  If the control network is
   * completely connected, then this list will only have one element (a list
   * of all the serials in the network).
   *
   * @returns A list of cube islands as serial numbers
   */
  QList< QList< QString > > ControlNet::GetSerialConnections() const {

    VertexIndexMap indexMap;
    VertexIndexMapAdaptor indexMapAdaptor(indexMap);

    // Needed to use connected_componenets
    QList< QString> serials = m_vertexMap.keys();
    for (int i = 0; i < serials.size(); i++) {
      boost::put(indexMapAdaptor, m_vertexMap[serials[i]], i);
    }

    VertexIndexMap componentMap;
    VertexIndexMapAdaptor componentAdaptor(componentMap);
    int numComponents = boost::connected_components(m_controlGraph, componentAdaptor,
                                                      boost::vertex_index_map(indexMapAdaptor));

    QList< QList< QString > > islandStrings;
    for (int i = 0; i < numComponents; i++) {
      QList<QString> tempList;
      islandStrings.append(tempList);
    }
    std::map<ImageVertex, size_t>::iterator it = componentMap.begin();
    while(it != componentMap.end())
    {
      QString serial = m_controlGraph[it->first].serial;
      int group = (int) it->second;
      islandStrings[group].append(serial);
      ++it;
    }
    return islandStrings;
  }


  /**
   * @returns The total number of edges in the bi-directional graph for images
   */
  int ControlNet::getEdgeCount() const {
    return boost::num_edges(m_controlGraph);
  }


  /**
   * Use this method to get a complete list of all the cube serial numbers in
   * the network.  Note that the order in which the serials are ordered in the
   * returned list is arbitrary and could change each time this method is
   * called (but the operation is done in constant time).
   *
   * @returns A list of the Cube Serial Numbers in the ControlNet.
   */
  QList< QString > ControlNet::GetCubeSerials() const {
    return m_vertexMap.keys();
  }


  /**
   * Does a check to ensure that the given serial number is contained within
   * the network.
   *
   * @param serialNumber the cube serial number to validate
   *
   * @return @b bool If the serial number is contained in the network.
   */
  bool ControlNet::ValidateSerialNumber(QString serialNumber) const {
    return m_vertexMap.contains(serialNumber);
  }


  /**
   * Get all images connected to a given image by common control points.
   *
   * @param serialNumber the serial number of the image to find images adjacent to.
   *
   * @returns @b QList<QString> The serial numbers of all adjacent images.
   */
  QList< QString > ControlNet::getAdjacentImages(QString serialNumber) const {
    if (!ValidateSerialNumber(serialNumber)) {
      std::string msg = "Cube Serial Number [" + serialNumber + "] not found in "
          "the network";
      throw IException(IException::Programmer, msg, _FILEINFO_);
    }

    QList< QString > adjacentSerials;

    AdjacencyIterator adjIt, adjEnd;
    boost::tie(adjIt, adjEnd) = boost::adjacent_vertices(m_vertexMap[serialNumber], m_controlGraph);
    for( ; adjIt != adjEnd; adjIt++) {
      adjacentSerials.append(m_controlGraph[*adjIt].serial);
    }

    return adjacentSerials;
  }


  /**
   * Get all the measures pertaining to a given cube serial number
   *
   * @returns A list of all measures which are in a given cube
   */
  QList< ControlMeasure * > ControlNet::GetMeasuresInCube(QString serialNumber) {
    if( !ValidateSerialNumber(serialNumber) ) {
      std::string msg = "Cube Serial Number [" + serialNumber.toStdString() + "] not found in "
          "the network";
      throw IException(IException::Programmer, msg, _FILEINFO_);

    }
    return m_controlGraph[m_vertexMap[serialNumber]].measures.values();
  }


  /**
   * Get all the valid measures pertaining to a given cube serial number
   *
   * @returns A list of all valid measures which are in a given cube
   */
  QList< ControlMeasure * > ControlNet::GetValidMeasuresInCube(QString serialNumber) {
    QList< ControlMeasure * > validMeasures;

    // Get measures in cube will validate this for us, so we don't need to re-check
    QList< ControlMeasure * > measureList = GetMeasuresInCube(serialNumber);

    foreach(ControlMeasure * measure, measureList) {
      if (!measure->IsIgnored())
        validMeasures.append(measure);
    }

    return validMeasures;
  }


  /**
   * Copies the content of the a ControlMeasureLessThanFunctor
   *
   * @param other, reference to the other ControlMeasureLessThanFunctor
   */
  ControlNet::ControlMeasureLessThanFunctor &
    ControlNet::ControlMeasureLessThanFunctor::operator=(ControlMeasureLessThanFunctor const &other) {

    if (this != &other) {
      this->m_accessor = other.m_accessor;
    }

    return *this;
  }


  /**
   * The () operator for the Control Measure less than functor
   *
   * @param a, ControlMeasure* pointer to the first control measure
   * @param b, ControlMeasure* pointer to the sencond control measure
   */
  bool ControlNet::ControlMeasureLessThanFunctor::operator()
    (ControlMeasure* const &a, ControlMeasure* const &b) {

    return (a->*this->m_accessor)() < (b->*this->m_accessor)();

  }


  /**
   * Get a sorted list of all the measures that have values in a given ragen
   *
   * @param statFunc A pointer to a control Measure acessor
   * @param min the minimum value of acessor return for list inclusion
   * @param max the maximum value of acessor return for list inclusion
   */
  QList< ControlMeasure * > ControlNet::sortedMeasureList(double(ControlMeasure::*statFunc)() const,
                                                          double min,double max) {

    QList< ControlMeasure * >measures;

    //build a list of all the pointers to the relevant measures
      //get the number of object points
    int nObjPts =  this->GetNumPoints();
    for (int i=0;i<nObjPts;i++) { //for each Object point
      ControlPoint *point = this->GetPoint(i);
      if (point->IsIgnored()) continue;  //if the point is ignored then continue

      // Get the number of measures
      int nObs = point->GetNumMeasures();
      for (int j=0;j<nObs;j++) {  //for every measure
        ControlMeasure *measure = point->GetMeasure(j);
        if (measure->IsIgnored())  continue;
        double temp = (measure->*statFunc)();
        if (temp > min)
        if (min <= temp && temp <= max) measures.push_back(measure);
      }
    }

    // Sort the measures
    ControlMeasureLessThanFunctor lessThan(statFunc);
    std::sort(measures.begin(),measures.end(),lessThan);

    return measures;
  }


  /**
   * Compute error for each point in the network
   *
   * @history 2010-01-11  Tracie Sucharski, Renamed from ComputeErrors
   */
  void ControlNet::ComputeResiduals() {
    // TODO:  Make sure the cameras have been initialized

    QHashIterator< QString, ControlPoint * > i(*points);
    while (i.hasNext()) {
      i.next();
      i.value()->ComputeResiduals();
    }
  }


  /**
   * Compute aprior values for each point in the network
   *
   * @history 2010-08-23  Ken Edmundson, skipping ignored points
   */
  void ControlNet::ComputeApriori() {
    // TODO:  Make sure the cameras have been initialized
    QHashIterator< QString, ControlPoint * > i(*points);
    while (i.hasNext()) {
      i.next();
      ControlPoint *point = i.value();
      if ( !point->IsIgnored() )
        point->ComputeApriori();
    }
  }


  /**
   * Compute the average error of all points in the network
   * @return <B>double</B> Average error of points
   *
   * @history 2010-01-12  Tracie Sucharski - Renamed from AverageError
   */
  double ControlNet::AverageResidual() {
    // TODO:  Make sure the cameras have been initialized
    double avgResidual = 0.0;
    int count = 0;
    QHashIterator< QString, ControlPoint * > i(*points);
    while (i.hasNext()) {
      i.next();
      ControlPoint *point = i.value();
      if (!point->IsIgnored()) {
        avgResidual += point->GetStatistic(
            &ControlMeasure::GetResidualMagnitude).Average();
        count++;
      }
    }

    if (count != 0)
      avgResidual /= count;

    return avgResidual;
  }


  /**
   * Returns the camera list from the given image number
   *
   * @param index The image number
   *
   * @return Isis::Camera* The pointer to the resultant camera list
   */
  Isis::Camera *ControlNet::Camera(int index) {
    return p_cameraList[index];
  }


  /**
   * Returns the camera list from the input serial number
   * 
   * @param serialNumber Serial number.
   * 
   * @return Isis::Camera* The pointer to the resultant camera.
   */
  Isis::Camera *ControlNet::Camera(QString serialNumber) {
    return p_cameraMap[serialNumber];
  } 


  /**
   * Return the Created Date
   *
   * @author Sharmila Prasad (10/6/2010)
   *
   * @return QString
   */
  QString ControlNet::CreatedDate() const {
    return p_created;
  }


  /**
   * Return the description of the network
   *
   * @return The description of this Control Network
   */
  QString ControlNet::Description() const {
    return p_description;
  }


  /**
   * Finds and returns a pointer to the closest ControlPoint to the
   * ControlMeasure with the given serial number and line sample location
   *
   * @param serialNumber The serial number of the the file the ControlMeasure is
   *                     on
   * @param sample The sample number of the ControlMeasure
   * @param line The line number of the ControlMeasure
   *
   * @return <B>ControlPoint*</B> Pointer to the ControlPoint
   *         closest to the given line, sample position
   */
  ControlPoint *ControlNet::FindClosest(QString serialNumber,
      double sample, double line) {

    if (!ValidateSerialNumber(serialNumber)) {
      std::string msg = "serialNumber [";
      msg += serialNumber;
      msg += "] not found in ControlNet";
      throw IException(IException::Programmer, msg, _FILEINFO_);
    }

    const double SEARCH_DISTANCE = 99999999.0;
    double minDist = SEARCH_DISTANCE;
    ControlPoint *closestPoint = NULL;

    QList < ControlMeasure * > measures = m_controlGraph[m_vertexMap[serialNumber]].measures.values();

    for (int i = 0; i < measures.size(); i++) {
      ControlMeasure *measureToCheck = measures[i];

      // Find closest line sample & return that controlpoint
      double dx = fabs(sample - measureToCheck->GetSample());
      double dy = fabs(line - measureToCheck->GetLine());

      double dist = sqrt((dx * dx) + (dy * dy));

      if (dist < minDist) {
        minDist = dist;
        closestPoint = measureToCheck->Parent();
      }
    }

    if (!closestPoint) {
      IString msg = "No point found within ";
      msg += IString(SEARCH_DISTANCE);
      msg += "pixels of sample/line [";
      msg += IString(sample);
      msg += ", ";
      msg += IString(line);
      msg += "]";
      throw IException(IException::Programmer, msg, _FILEINFO_);
    }

    return closestPoint;
  }


  /**
   * Determine the maximum error of all points in the network
   * @return <B>double</B> Max error of points
   *
   * @history 2010-01-12  Tracie Sucharski - Renamed from MaximumError
   */
  double ControlNet::GetMaximumResidual() {
    // TODO:  Make sure the cameras have been initialized

    double maxResidual = 0.0;
    foreach(ControlPoint * p, *points) {
      double residual = p->GetStatistic(
          &ControlMeasure::GetResidualMagnitude).Maximum();
      if (residual > maxResidual)
        maxResidual = residual;
    }

    return maxResidual;
  }


  QString ControlNet::GetNetworkId() const {
    return p_networkId;
  }


  /**
   * Return the total number of edit locked measures for all control points in the
   * network
   *
   * @return Number of edit locked measures
   */
  int ControlNet::GetNumEditLockMeasures() {
    int numLockedMeasures = 0;
    foreach(ControlPoint * p, *points) {
      numLockedMeasures += p->GetNumLockedMeasures();
    }

    return numLockedMeasures;
  }


  /**
   * Returns the number of edit locked control points
   *
   * @return Number of edit locked control points
   */
  int ControlNet::GetNumEditLockPoints() {
    int editLockPoints = 0;
    foreach(ControlPoint * p, *points) {
      if (p->IsEditLocked())
        editLockPoints++;
    }

    return editLockPoints;
  }


  /**
   * Return the total number of ignored measures for all control points in the
   * network
   *
   * @return Number of valid measures
   */
  int ControlNet::GetNumIgnoredMeasures() {
    int numIgnoredMeasures = 0;
    foreach(ControlPoint * p, *points) {
      numIgnoredMeasures += p->GetNumMeasures() - p->GetNumValidMeasures();
    }

    return numIgnoredMeasures;
  }


  /**
   * Return the number of measures in image specified by serialNumber
   *
   * @return Number of valid measures in image
   *
   * @history 2013-12-18 Tracie Sucharski - Renamed from GetNumberOfMeasuresInImage, it is
   *                         returning a count of only valid measures (Ignore=False).
   */
  int ControlNet::GetNumberOfValidMeasuresInImage(const QString &serialNumber) {
    // If SetImage was called, use the map that has been populated with valid measures
    if (p_cameraList.size() > 0) {
      return p_cameraValidMeasuresMap[serialNumber];
    }
    return GetValidMeasuresInCube(serialNumber).size();
  }


  /**
   * Return the number of jigsaw rejected measures in image specified by serialNumber
   *
   * @return Number of jigsaw rejected measures in image
   */
  int ControlNet::GetNumberOfJigsawRejectedMeasuresInImage(const QString &serialNumber) {
    return p_cameraRejectedMeasuresMap[serialNumber];
  }


  /**
   * Sets jigsaw rejected flag to false for all points and measures.
   * Called by BundleAdjust::Init method
   *
   */
  void ControlNet::ClearJigsawRejected() {
    foreach(ControlPoint * p, *points) {
      p->ClearJigsawRejected();
    }
  }


  /**
   * Increment number of jigsaw rejected measures in image specified by serialNumber
   *
   */
  void ControlNet::IncrementNumberOfRejectedMeasuresInImage(const QString &serialNumber) {
    p_cameraRejectedMeasuresMap[serialNumber]++;
  }


  /**
   * Decrement number of jigsaw rejected measures in image specified by serialNumber
   *
   */
  void ControlNet::DecrementNumberOfRejectedMeasuresInImage(const QString &serialNumber) {
    if (p_cameraRejectedMeasuresMap[serialNumber] > 0)
      p_cameraRejectedMeasuresMap[serialNumber]--;
  }


  /**
   * Returns the total number of measures for all control points in the network
   *
   * @return Number of control measures
   */
  int ControlNet::GetNumMeasures() const {
    int numMeasures = 0;
    foreach(ControlPoint * p, *points) {
      numMeasures += p->GetNumMeasures();
    }

    return numMeasures;
  }


  //! Return the number of control points in the network
  int ControlNet::GetNumPoints() const {
    return points->size();
  }


  /**
   * Return the number of valid (non-ignored) measures for all control points
   * in the network
   *
   * @return Number of valid measures
   *
   * @internal
   * @history 2011-03-17 Debbie A. Cook - Modified to not count ignored measures.
   */
  int ControlNet::GetNumValidMeasures() {
    int numValidMeasures = 0;
    foreach(ControlPoint * p, *points) {
      if (!p->IsIgnored())
        numValidMeasures += p->GetNumValidMeasures();
    }

    return numValidMeasures;
  }


  /**
   * Returns the number of non-ignored control points
   *
   * @return Number of valid control points
   */
  int ControlNet::GetNumValidPoints() {
    int validPoints = 0;
    foreach(ControlPoint * p, *points) {
      if (!p->IsIgnored())
        validPoints++;
    }

    return validPoints;
  }


  //! Return the target name
  QString ControlNet::GetTarget() const {
    return p_targetName;
  }


  //! Return the user name
  QString ControlNet::GetUserName() const {
    return p_userName;
  }

  //! Return the last modified date
  QString ControlNet::GetLastModified() const {
    return p_modified;
  }


  //! Return QList of all the ControlPoints in the network
  QList< ControlPoint * > ControlNet::GetPoints() {
    QList< ControlPoint * > pointsList;

    for (int i = 0; i < pointIds->size(); i++) {
      pointsList.append(GetPoint(i));
    }

    return pointsList;
  }


  //! Return QList of ControlPoint Ids used in hash, in order of addition
  QList< QString > ControlNet::GetPointIds() const {
    return *pointIds;
  }


  /**
   * Set the creation time
   *
   * @param date The date this Control Network was created
   */
  void ControlNet::SetCreatedDate(const QString &date) {
    p_created = date;
  }


  /**
    * Set the description of the network
    *
    * @param desc The description of this Control Network
    */
  void ControlNet::SetDescription(const QString &newDescription) {
    p_description = newDescription;
  }


  /**
   * Creates the ControlNet's image cameras based on an input file
   *
   * @param imageListFile The list of images
   */
  void ControlNet::SetImages(const QString &imageListFile) {
    SerialNumberList list(imageListFile);
    SetImages(list);
  }


  /**
   * Creates the ControlNet's image camera's based on the list of Serial Numbers
   *
   * @param list The list of Serial Numbers
   * @param progress A pointer to the progress of creating the cameras
   * @throws Isis::iException::System - "Unable to create camera
   *        for cube file"
   * @throws Isis::iException::User - "Control point measure does
   *        not have a cube with a matching serial number"
   * @internal
   *   @history 2009-01-06 Jeannie Walldren - Fixed typo in
   *            exception output.
   *   @history 2016-10-13 Ian Humphrey - Added initial check to see if cameras have already been
   *                           set, and immediately return if yes. References #4293.
   */
  void ControlNet::SetImages(SerialNumberList &list, Progress *progress) {
    // First check if cameras have already been setup via another SetImages call
    if (p_cameraList.size() > 0) {
      return;
    }
    // Prep for reporting progress
    if (progress != NULL) {
      progress->SetText("Setting input images...");
      progress->SetMaximumSteps(list.size());
      progress->CheckStatus();
    }
    // Open the camera for all the images in the serial number list
    for (int i = 0; i < list.size(); i++) {
      QString serialNumber = list.serialNumber(i);
      QString filename = list.fileName(i);
      Cube cube(filename.toStdString(), "r");

      try {
        Isis::Camera *cam = CameraFactory::Create(cube);
        p_cameraMap[serialNumber] = cam;
        p_cameraValidMeasuresMap[serialNumber] = 0;
        p_cameraRejectedMeasuresMap[serialNumber] = 0;
        p_cameraList.push_back(cam);
      }
      catch (IException &e) {
        std::string msg = "Unable to create camera for cube file ";
        msg += filename;
        throw IException(e, IException::Unknown, msg, _FILEINFO_);
      }

      if (progress != NULL)
        progress->CheckStatus();
    }

    // Loop through all measures and set the camera
    QHashIterator< QString, ControlPoint * > p(*points);
    while (p.hasNext()) {
      p.next();
      ControlPoint *curPoint = p.value();

      QList< QString > serialNums = curPoint->getCubeSerialNumbers();
      for (int m = 0; m < serialNums.size(); m++) {
        ControlMeasure *curMeasure = (*curPoint)[serialNums[m]];

        QString serialNumber = curMeasure->GetCubeSerialNumber();
        if (list.hasSerialNumber(serialNumber)) {
          curMeasure->SetCamera(p_cameraMap[serialNumber]);

          // increment number of measures for this image (camera)
          if (!curMeasure->IsIgnored()) p_cameraValidMeasuresMap[serialNumber]++;
        }
        else {
          IString msg = "Control point [" + curPoint->GetId() +
              "], measure [" + curMeasure->GetCubeSerialNumber() +
              "] does not have a cube with a matching serial number";
          throw IException(IException::User, msg, _FILEINFO_);
        }
      }
    }
  }


  /**
   * Set the last modified date
   *
   * @param date The last date this Control Network was modified
   */
  void ControlNet::SetModifiedDate(const QString &date) {
    p_modified = date;
  }


  /**
   * Set mutex to lock for making Naif calls
   *
   * @author 2012-09-11 Tracie Sucharski
   *
   * @param mutex
   */
  void ControlNet::SetMutex(QMutex *mutex) {
    m_mutex = mutex;
  }


  /**
   * Set the network id
   *
   * @param id The Id of this Control Network
   */
  void ControlNet::SetNetworkId(const QString &id) {
    p_networkId = id;
  }


  /**
   * Sets the target name and target radii, if available.
   *
   * Note: The target radii are found using NAIF target codes. If the given
   * target name is not recognized, the target radii vector will be filled with
   * Isis::Null values.
   *
   * @see Target::radiiGroup(QString)
   *
   * @param target The name of the target of this Control Network
   */
  void ControlNet::SetTarget(const QString &target) {
    p_targetName = target;
  }


  /**
   * Sets the target name and radii using values found in the mapping group of
   * the given label, if available. If this fails, calls SetTarget(QString).
   *
   * @param label A PVL Containing Target information (usually in a Mapping
   *              group or NaifKeywords object).
   */
  void ControlNet::SetTarget(Pvl label) {
    PvlGroup mapping;
    if ( (label.hasObject("IsisCube") && label.findObject("IsisCube").hasGroup("Mapping"))
         || label.hasGroup("Mapping") ) {
      mapping = label.findGroup("Mapping", Pvl::Traverse);
    }

    if (mapping.hasKeyword("TargetName")) {
      p_targetName = QString::fromStdString(mapping["TargetName"][0]);
    }
    else if (label.hasObject("IsisCube")
             && label.findObject("IsisCube").hasGroup("Instrument")
             && label.findObject("IsisCube").findGroup("Instrument").hasKeyword("TargetName")) {
      p_targetName = QString::fromStdString(label.findObject("IsisCube").findGroup("Instrument").findKeyword("TargetName")[0]);
    }
    else {
      p_targetName = "";
    }
  }


  /**
   * Set the user name of the control network.
   *
   * @param name The name of the user creating or modifying this ControlNet
   */
  void ControlNet::SetUserName(const QString &name) {
    p_userName = name;
  }


  /**
   * Swaps the member data with the given control net. This is an optimized form of:
   *   ControlNet a = ...
   *   ControlNet b = ...
   *
   *   Swap 'a' and 'b'
   *   ControlNet tmp = a;
   *   a = b;
   *   b = tmp;
   *
   * This is used primarily for the assignment operator in order to do copy-and-swap.
   *
   * @param other The control net to swap with.
   */
  void ControlNet::swap(ControlNet &other) {
    std::swap(points, other.points);
    std::swap(pointIds, other.pointIds);
    m_controlGraph.swap(other.m_controlGraph);
    std::swap(m_mutex, other.m_mutex);
    std::swap(p_targetName, other.p_targetName);
    std::swap(p_networkId, other.p_networkId);
    std::swap(p_created, other.p_created);
    std::swap(p_modified, other.p_modified);
    std::swap(p_description, other.p_description);
    std::swap(p_userName, other.p_userName);
    std::swap(p_cameraMap, other.p_cameraMap);
    std::swap(p_cameraValidMeasuresMap, other.p_cameraValidMeasuresMap);
    std::swap(p_cameraRejectedMeasuresMap, other.p_cameraRejectedMeasuresMap);
    std::swap(p_cameraList, other.p_cameraList);

    // points have parent pointers that need to be updated too...
    QHashIterator< QString, ControlPoint * > i(*points);
    while (i.hasNext()) {
      i.next().value()->parentNetwork = this;
    }

    QHashIterator< QString, ControlPoint * > i2(*other.points);
    while (i2.hasNext()) {
      i2.next().value()->parentNetwork = &other;
    }

    m_vertexMap.clear();
    VertexIterator v, vend;
    for (boost::tie(v, vend) = vertices(m_controlGraph); v != vend; ++v) {
      ImageVertex imVertex = *v;
      QString serialNum = m_controlGraph[*v].serial;
      m_vertexMap[serialNum] = imVertex;
    }

    other.m_vertexMap.clear();
    VertexIterator v2, vend2;
    for (boost::tie(v2, vend2) = vertices(other.m_controlGraph); v2 != vend2; ++v2) {
      ImageVertex imVertex = *v2;
      QString serialNum = other.m_controlGraph[*v2].serial;
      other.m_vertexMap[serialNum] = imVertex;
    }

    emit networkModified(ControlNet::Swapped);
  }


  /**
   * Assign other to this.
   *
   * This is an exception-safe assignment operator.
   *
   * @param other The control net to assign to this.
   */
  ControlNet &ControlNet::operator=(const ControlNet &other) {
    // Optimization: if this == other do nothing.
    if (this != &other) {
      // copy & swap
      ControlNet copy(other);
      swap(copy);
    }

    return *this;
  }


  const ControlPoint *ControlNet::GetPoint(QString id) const {
    if (!points->contains(id)) {
      std::string msg = "The control network has no control points with an ID "
          "equal to [" + id.toStdString() + "]";
      throw IException(IException::Programmer, msg, _FILEINFO_);
    }

    return points->value(id);
  }


  ControlPoint *ControlNet::GetPoint(QString id) {
    if (!points->contains(id)) {
      std::string msg = "The control network has no control points with an ID "
          "equal to [" + id.toStdString() + "]";
      throw IException(IException::Programmer, msg, _FILEINFO_);
    }

    return (*points)[id];
  }


  const ControlPoint *ControlNet::GetPoint(int index) const {
    if (index < 0 || index >= pointIds->size()) {
      IString msg = "Index [" + IString(index) + "] out of range";
      throw IException(IException::Programmer, msg, _FILEINFO_);
    }

    return GetPoint(pointIds->at(index));
  }


  ControlPoint *ControlNet::GetPoint(int index) {
    if (index < 0 || index >= pointIds->size()) {
      IString msg = "Index [" + IString(index) + "] out of range";
      throw IException(IException::Programmer, msg, _FILEINFO_);
    }

    return GetPoint(pointIds->at(index));
  }


  /**
   * Get the control point coordinate type (see the available types in SurfacePoint.h).
   *
   * @returns the control point coordinate type
   */
  SurfacePoint::CoordinateType ControlNet::GetCoordType() {
    return m_coordType;
  }

  const ControlPoint *ControlNet::operator[](QString id) const {
    return GetPoint(id);
  }


  ControlPoint *ControlNet::operator[](QString id) {
    return GetPoint(id);
  }


  const ControlPoint *ControlNet::operator[](int index) const {
    return GetPoint(index);
  }


  ControlPoint *ControlNet::operator[](int index) {
    return GetPoint(index);
  }
}
