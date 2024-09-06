#include "LidarData.h"

#include <QByteArray>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonValue>
#include <QList>
#include <QMapIterator>
#include <QRegExp>
#include <QSharedPointer>
#include <QString>
#include <QStringList>
#include <QTextStream>

#include "Angle.h"
#include "CameraFactory.h"
#include "ControlNet.h"
#include "Distance.h"
#include "FileName.h"
#include "IException.h"
#include "iTime.h"
#include "Latitude.h"
#include "Longitude.h"
#include "Progress.h"
#include "SerialNumberList.h"
#include "SurfacePoint.h"

using namespace boost::numeric::ublas;

namespace Isis {


  /**
   * Default constructor.
   */
  LidarData::LidarData() {
    m_numSimultaneousMeasures = 0;
    m_numAsynchronousMeasures = 0;
  }


  /**
   * Adds a LidarControlPoint to the LidarData.
   *
   * @param QSharedPointer<LidarControlPoint> point LidarControlPoint to add.
   */
  void LidarData::insert(QSharedPointer<LidarControlPoint> point) {
    m_points.insert(point->GetId(), point);
  }

  /**
   * Gets a single LidarDataPoint by ID
   *
   * @param pointId The ID of the LidarDataPoint
   * @return QSharedPointer<LidarDataPoint> The LidarDataPoint matching the supplied ID
   */

  QSharedPointer<LidarControlPoint> LidarData::point(QString pointId) const{
    QSharedPointer<LidarControlPoint> point = m_points.value(pointId, 0);
    if (!point) {
      std::string msg = "Point " + pointId.toStdString() + " is not in the lidar data.";
      throw IException(IException::Programmer, msg, _FILEINFO_);
    }
    return point;
  }


  /**
   * Gets the list of Lidar data points optionally sorted .
   *
   * @param sort An option to sort the list.  The default is false (no sort).
   * @return @b QList<QSharedPointer<LidarControlPoint>> Returns list of Lidar control points.
   * @internal
   *   @history 2019-02-23 Debbie A Cook - Added optional argument to the points
   *                           method to sort the list.  See LidarControlPoint for the sorting
   *                           functor. The default behavior has not changed for backward
   *                           compatability. References #5343.
   */
  QList< QSharedPointer<LidarControlPoint> > LidarData::points(bool sort) const {
    if (!sort) {
      // This is the default behavior.  The list is coming from a QHash so the point order
      // will vary.
      return m_points.values();
    }
    else {
      // Sort the points as recommended by QT with std::sort since qsort is deprecated
      QList< QSharedPointer<LidarControlPoint> > pointlist = points();
      std::sort(pointlist.begin(), pointlist.end(),
                LidarControlPoint::LidarControlPointLessThanFunctor());
      return pointlist;
    }
  }


  /**
   * Returns number of Lidar data points.
   *
   * @return int Returns number of Lidar control points.
   */
  int LidarData::numberLidarPoints() {
    return m_points.size();
  }


  /**
   * Returns number of simultaneous lidar measures.
   *
   * @return int Returns number of simultaneous lidar measures.
   */
  int LidarData::numberSimultaneousMeasures() {
    return m_numSimultaneousMeasures;
  }

  /**
   * Returns number of non-simultaneous lidar measures.
   *
   * @return int Returns number of non-simultaneous lidar measures.
   */
  int LidarData::numberAsynchronousMeasures() {
    return m_numAsynchronousMeasures;
  }


  /**
   * Returns total number of lidar measures.
   *
   * @return int Returns total number of lidar measures.
   */
  int LidarData::numberMeasures() {
    return m_numSimultaneousMeasures + m_numAsynchronousMeasures;
  }


  /**
   * TODO: more detail below...
   *
   * Assigns Isis::Camera pointers to LidarControlPoint measures.
   *
   * @param controlNet Input ControlNet
   * @param progress A pointer to the progress of creating the cameras
   * @throws Isis::iException::User - "Lidar Control point measure does not have a cube with a
   *         matching serial number"
   * @internal
   *   @history 2019-02-06 Ken Edmundson - initial version.
   */
  void LidarData::SetImages(ControlNet &controlNet, Progress *progress) {

    // iterate over map between serial number and camera pointers
    QMapIterator< QString, Isis::Camera *> it(p_cameraMap);
    while (it.hasNext()) {
      it.next();
      QString serialNumber = it.key();

      // get corresponding camera pointer from controlNet
      Isis::Camera *cam = controlNet.Camera(serialNumber);
      p_cameraMap[serialNumber] = cam;
      p_cameraValidMeasuresMap[serialNumber] = 0;         // not sure if all this is needed
      p_cameraRejectedMeasuresMap[serialNumber] = 0;
      p_cameraList.push_back(cam);
    }

    // Loop through all measures and set the camera
    QHashIterator< QString, QSharedPointer<LidarControlPoint> > p(m_points);
    while (p.hasNext()) {
      p.next();
      LidarControlPointQsp curPoint = p.value();

      QList< QString > serialNums = curPoint->getCubeSerialNumbers();
      for (int m = 0; m < serialNums.size(); m++) {
        ControlMeasure *curMeasure = (*curPoint)[serialNums[m]];

        QString serialNumber = curMeasure->GetCubeSerialNumber();
        Isis::Camera *cam = p_cameraMap[serialNumber];

        if (cam == NULL) {
          IString msg = "Lidar Control point [" + curPoint->GetId().toStdString() +
              "], measure [" + curMeasure->GetCubeSerialNumber().toStdString() +
              "] does not have a cube in the ISIS control net with a matching serial number";
          throw IException(IException::User, msg, _FILEINFO_);
        }

        curMeasure->SetCamera(cam);

        // increment number of measures for this image (camera)
        if (!curMeasure->IsIgnored()) {
          p_cameraValidMeasuresMap[serialNumber]++;
        }
      }
    }
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
  void LidarData::SetImages(SerialNumberList &list, Progress *progress) {
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
        msg += filename.toStdString();
        throw IException(e, IException::Unknown, msg, _FILEINFO_);
      }

      if (progress != NULL)
        progress->CheckStatus();
    }

    // Loop through all measures and set the camera
    QHashIterator< QString, QSharedPointer<LidarControlPoint> > p(m_points);
    while (p.hasNext()) {
      p.next();
      LidarControlPointQsp curPoint = p.value();

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
          IString msg = "Control point [" + curPoint->GetId().toStdString() +
              "], measure [" + curMeasure->GetCubeSerialNumber().toStdString() +
              "] does not have a cube with a matching serial number";
          throw IException(IException::User, msg, _FILEINFO_);
        }
      }
    }
  }


  /**
   * @brief Unserialize LidarData.
   *
   * This method unserializes LidarData from a JSON or binary (QByteArray) file. It will
   * automatically determine if it is JSON or binary formatted data.
   *
   * @param FileName lidarFile Name of the serialized LidarData file to read.
   * @throws IException::User Throws User exception if it cannot open the file passed.
   */
  void LidarData::read(FileName lidarDataFile) {
    // Set up the input file
    QFile loadFile(QString::fromStdString(lidarDataFile.expanded()));
    // Make sure we can open the file successfully
    if (!loadFile.open(QIODevice::ReadOnly)) {
      std::string msg("Could not open " + loadFile.fileName().toStdString());
      throw IException(IException::User, msg, _FILEINFO_);
    }

    // Load file
    QByteArray saveData = loadFile.readAll();
    QCborValue loadData(saveData);
    QJsonDocument loadDoc(QJsonDocument::fromJson(loadData.toByteArray()));

    int totalMeasures = 0;

    // Unserialize LidarData
    QJsonObject lidarDataObject = loadDoc.object();
    if (lidarDataObject.contains("points") && lidarDataObject["points"].isArray()) {
      // Unserialize LidarControlPoints
      QJsonArray pointArray = lidarDataObject["points"].toArray();
      for (int pointIndex = 0; pointIndex < pointArray.size(); pointIndex++) {
        // Unserialize LidarControlPoint
        QJsonObject pointObject = pointArray[pointIndex].toObject();

        QString id;
        if (pointObject.contains("id") && pointObject["id"].isString()) {
          id = pointObject["id"].toString();
        }

        double range = 0.0;
        if (pointObject.contains("range") && pointObject["range"].isDouble()) {
          range = pointObject["range"].toDouble();
        }

        double sigmaRange = 0.0;
        if (pointObject.contains("sigmaRange") && pointObject["sigmaRange"].isDouble()) {
          sigmaRange = pointObject["sigmaRange"].toDouble();
        }

        double time = 0.0;
        if (pointObject.contains("time") && pointObject["time"].isDouble()) {
          time = pointObject["time"].toDouble();
        }

        double latitude = 0.0;
        if (pointObject.contains("latitude") && pointObject["latitude"].isDouble()) {
          latitude = pointObject["latitude"].toDouble();
        }

        double longitude = 0.0;
        if (pointObject.contains("longitude") && pointObject["longitude"].isDouble()) {
          longitude = pointObject["longitude"].toDouble();
        }

        double radius = 0.0;
        if (pointObject.contains("radius") && pointObject["radius"].isDouble()) {
          radius = pointObject["radius"].toDouble();
        }

        QSharedPointer<LidarControlPoint> lcp =
            QSharedPointer<LidarControlPoint>(new LidarControlPoint());
        lcp->SetId(id);
        lcp->setTime(iTime(time));
        lcp->setRange(range);
        lcp->setSigmaRange(sigmaRange);

        if (pointObject.contains("aprioriMatrix") &&
                                 pointObject["aprioriMatrix"].isArray()) {
          QJsonArray  aprioriMatrixArray = pointObject["aprioriMatrix"].toArray();
          boost::numeric::ublas::symmetric_matrix<double, upper> aprioriMatrix(3);
          aprioriMatrix.clear();
          aprioriMatrix(0, 0) = aprioriMatrixArray[0].toDouble();
          aprioriMatrix(0, 1) = aprioriMatrix(1, 0) = aprioriMatrixArray[1].toDouble();
          aprioriMatrix(0, 2) = aprioriMatrix(2, 0) = aprioriMatrixArray[2].toDouble();
          aprioriMatrix(1, 1) = aprioriMatrixArray[3].toDouble();
          aprioriMatrix(1, 2) = aprioriMatrix(2, 1) = aprioriMatrixArray[4].toDouble();
          aprioriMatrix(2, 2) = aprioriMatrixArray[5].toDouble();

          lcp->SetAprioriSurfacePoint(SurfacePoint(Latitude(latitude, Angle::Units::Degrees),
                                                 Longitude(longitude, Angle::Units::Degrees),
                                                 Distance(radius, Distance::Units::Kilometers),
                                                   aprioriMatrix));
          lcp->SetType(ControlPoint::Constrained);
        }
        else {
          lcp->SetAprioriSurfacePoint(SurfacePoint(Latitude(latitude, Angle::Units::Degrees),
                                                 Longitude(longitude, Angle::Units::Degrees),
                                                   Distance(radius, Distance::Units::Kilometers)));
        }

        // Set the adjusted surface point if it exists
        if (pointObject.contains("adjustedLatitude") &&
             pointObject["adjustedLatitude"].isDouble() &&
             pointObject.contains("adjustedLongitude") &&
             pointObject["adjustedLongitude"].isDouble() &&
             pointObject.contains("adjustedRadius") &&
             pointObject["adjustedRadius"].isDouble()) {

          double adjustedLatitude = 0.0;
          adjustedLatitude = pointObject["adjustedLatitude"].toDouble();

          double adjustedLongitude = 0.0;
          adjustedLongitude = pointObject["adjustedLongitude"].toDouble();

          double adjustedRadius = 0.0;
          adjustedRadius = pointObject["radius"].toDouble();

          if (pointObject.contains("adjustedMatrix") &&
              pointObject["adjustedMatrix"].isArray()) {
            QJsonArray  adjustedMatrixArray = pointObject["adjustedMatrix"].toArray();
            boost::numeric::ublas::symmetric_matrix<double, upper> adjustedMatrix(3);
            adjustedMatrix.clear();
            adjustedMatrix(0, 0) = adjustedMatrixArray[0].toDouble();
            adjustedMatrix(0, 1) = adjustedMatrix(1, 0) = adjustedMatrixArray[1].toDouble();
            adjustedMatrix(0, 2) = adjustedMatrix(2, 0) = adjustedMatrixArray[2].toDouble();
            adjustedMatrix(1, 1) = adjustedMatrixArray[3].toDouble();
            adjustedMatrix(1, 2) = adjustedMatrix(2, 1) = adjustedMatrixArray[4].toDouble();
            adjustedMatrix(2, 2) = adjustedMatrixArray[5].toDouble();

            lcp->SetAdjustedSurfacePoint(SurfacePoint(Latitude(adjustedLatitude, Angle::Units::Degrees),
                                                     Longitude(adjustedLongitude, Angle::Units::Degrees),
                                                     Distance(adjustedRadius, Distance::Units::Kilometers),
                                                     adjustedMatrix));
            lcp->SetType(ControlPoint::Constrained);
          }
          else {
            lcp->SetAdjustedSurfacePoint(SurfacePoint(Latitude(adjustedLatitude, Angle::Units::Degrees),
                                                     Longitude(adjustedLongitude, Angle::Units::Degrees),
                                                     Distance(adjustedRadius, Distance::Units::Kilometers)));
          }
        }

        if (pointObject.contains("simultaneousImages") &&
                                 pointObject["simultaneousImages"].isArray()) {
          QJsonArray simultaneousArray =
                pointObject["simultaneousImages"].toArray();

              for (int simIndex = 0; simIndex < simultaneousArray.size(); simIndex ++) {
                QString newSerial;
                // Unserialize each simultaneous image serial number
                newSerial =  simultaneousArray[simIndex].toString();
                lcp->addSimultaneous(newSerial);
                m_numSimultaneousMeasures++;
              }
        }

        // Unserialize ControlMeasures
        if (pointObject.contains("measures") && pointObject["measures"].isArray()) {
          QJsonArray measureArray = pointObject["measures"].toArray();
          for (int measureIndex = 0; measureIndex < measureArray.size(); measureIndex++) {
            // Unserialize ControlMeasure
            QJsonObject measureObject = measureArray[measureIndex].toObject();

            double line = 0.0;
            if (measureObject.contains("line") && measureObject["line"].toDouble()) {
              line = measureObject["line"].toDouble();
            }

            double sample = 0.0;
            if (measureObject.contains("sample") && measureObject["sample"].toDouble()) {
              sample = measureObject["sample"].toDouble();
            }

            QString serialNumber;
            if (measureObject.contains("serialNumber") && measureObject["serialNumber"].isString()) {
              serialNumber = measureObject["serialNumber"].toString();
            }

            if (!p_cameraMap.contains(serialNumber)) {
              p_cameraMap.insert(serialNumber, NULL);
            }

            // QString type;
            // if (measureObject.contains("type") && measureObject["type"].isString()) {
            //   type = measureObject["type"].toString();
            // }

            ControlMeasure *measure = new ControlMeasure();
            measure->SetCoordinate(sample, line);
            measure->SetCubeSerialNumber(serialNumber);
            // measure->SetType(measure->StringToMeasureType(type));
            lcp->Add(measure);
            totalMeasures++;
          }
        }

        insert(lcp);
      }
    }
    m_numAsynchronousMeasures = totalMeasures - m_numSimultaneousMeasures;
  }


  /**
   * @brief Serializes LidarData.
   *
   * This method serializes the LidarData to either a JSON or binary (QByteArray) file. If JSON,
   * the file extension will be .json; otherwise (if binary), the file extension will be .dat.
   *
   * @param FileName outputFile Name of the file to serialize to.
   * @param LidarData::Format format Format of the serialized file (Json or Binary).
   *
   * @throws IException::User Throws User exception if it cannot open the file for writing.
   */
  void LidarData::write(FileName outputFile, LidarData::Format format) {
    bool sort = false;  // Default behavior

    // Set up the output file
    if (format == Json) {
      outputFile = outputFile.setExtension("json");
    }
    else if (format == Test) {
      // Format is same as Json, but points are sorted instead of random so that
      // output can be compared to truth data
      outputFile = outputFile.setExtension("json");
      sort = true;
    }
    else {
      outputFile = outputFile.setExtension("dat");
    }
    QFile saveFile(QString::fromStdString(outputFile.expanded()));

    if (!saveFile.open(QIODevice::WriteOnly)) {
      std::string msg("Could not open " + saveFile.fileName().toStdString());
      throw IException(IException::User, msg, _FILEINFO_);
    }

    // Serialize LidarData
    QJsonObject lidarDataObject;
    QJsonArray pointArray;
    // Serialize the LidarControlPoints it contains
    foreach (QSharedPointer<LidarControlPoint> lcp, points(sort)) {
      // Serialize LidarControlPoint
      QJsonObject pointObject;
      pointObject["id"] = lcp->GetId();
      pointObject["range"] = lcp->range();
      pointObject["sigmaRange"] = lcp->sigmaRange();
      pointObject["time"] = lcp->time().Et();

      // Serialize the AprioriSurfacePoint
      SurfacePoint aprioriSurfacePoint = lcp->GetAprioriSurfacePoint();
      if (aprioriSurfacePoint.Valid()) {
        pointObject["latitude"] = aprioriSurfacePoint.GetLatitude().planetocentric(Angle::Units::Degrees);
        pointObject["longitude"] = aprioriSurfacePoint.GetLongitude().positiveEast(Angle::Units::Degrees);
        pointObject["radius"] = aprioriSurfacePoint.GetLocalRadius().kilometers();

        // Serialize the apriori matrix
        symmetric_matrix<double, upper> aprioriMatrix = aprioriSurfacePoint.GetSphericalMatrix();
        QJsonArray aprioriMatrixArray;
        aprioriMatrixArray += aprioriMatrix(0, 0);
        aprioriMatrixArray += aprioriMatrix(0, 1);
        aprioriMatrixArray += aprioriMatrix(0, 2);
        aprioriMatrixArray += aprioriMatrix(1, 1);
        aprioriMatrixArray += aprioriMatrix(1, 2);
        aprioriMatrixArray += aprioriMatrix(2, 2);

        // If the covariance matrix has a value, add it to the PVL point.
        if ( aprioriMatrix(0, 0) != 0.0
             || aprioriMatrix(0, 1) != 0.0
             || aprioriMatrix(0, 2) != 0.0
             || aprioriMatrix(1, 1) != 0.0
             || aprioriMatrix(1, 2) != 0.0
             || aprioriMatrix(2, 2) != 0.0 ) {
          pointObject["aprioriMatrix"] = aprioriMatrixArray;
        }
      }

      // Serialize the AdjustedSurfacePoint
      SurfacePoint adjustedSurfacePoint = lcp->GetAdjustedSurfacePoint();
      if (adjustedSurfacePoint.Valid()) {
        pointObject["adjustedLatitude"] =
          adjustedSurfacePoint.GetLatitude().planetocentric(Angle::Units::Degrees);
        pointObject["adjustedLongitude"] =
          adjustedSurfacePoint.GetLongitude().positiveEast(Angle::Units::Degrees);
        pointObject["adjustedRadius"] = adjustedSurfacePoint.GetLocalRadius().kilometers();

        // Serialize the adjusted matrix
        symmetric_matrix<double, upper> adjustedMatrix = adjustedSurfacePoint.GetSphericalMatrix();
        QJsonArray adjustedMatrixArray;
        adjustedMatrixArray += adjustedMatrix(0, 0);
        adjustedMatrixArray += adjustedMatrix(0, 1);
        adjustedMatrixArray += adjustedMatrix(0, 2);
        adjustedMatrixArray += adjustedMatrix(1, 1);
        adjustedMatrixArray += adjustedMatrix(1, 2);
        adjustedMatrixArray += adjustedMatrix(2, 2);

        // If the covariance matrix has a value, add it to the PVL point.
        if ( adjustedMatrix(0, 0) != 0.0
             || adjustedMatrix(0, 1) != 0.0
             || adjustedMatrix(0, 2) != 0.0
             || adjustedMatrix(1, 1) != 0.0
             || adjustedMatrix(1, 2) != 0.0
             || adjustedMatrix(2, 2) != 0.0 ) {
          pointObject["adjustedMatrix"] = adjustedMatrixArray;
        }
      }

      // Serialize the list of simultaneous images
      QJsonArray simultaneousArray;
      foreach (QString sn, lcp->snSimultaneous()) {
        simultaneousArray.append(sn);
      }
      pointObject["simultaneousImages"] = simultaneousArray;

      QJsonArray measureArray;
      // Serialize the ControlMeasures it contains
      foreach (ControlMeasure *measure, lcp->getMeasures()) {
        // Serialize ControlMeasure
        QJsonObject measureObject;
        measureObject["line"] = measure->GetLine();
        measureObject["sample"] = measure->GetSample();
        measureObject["serialNumber"] = measure->GetCubeSerialNumber();
        // measureObject["type"] = measure->GetMeasureTypeString();
        measureArray.append(measureObject);

      }
      // Add the ControlMeasures to the LidarControlPoint
      pointObject["measures"] = measureArray;

      pointArray.append(pointObject);

    }
    // Add the LidarControlPoints to the LidarData
    lidarDataObject["points"] = pointArray;

    // Write the JSON to the file
    QJsonDocument lidarDataDoc(lidarDataObject);
    if (format == Json || format == Test) {
      saveFile.write(lidarDataDoc.toJson());
    }
    else {
      QCborValue json(lidarDataDoc.toJson());
      saveFile.write(json.toByteArray());
    }
  }

  /**
   * Does a check to ensure that the given serial number is contained within
   * the network.
   *
   * @param serialNumber the cube serial number to validate
   *
   * @return @b bool If the serial number is contained in the network.
   */
  bool LidarData::ValidateSerialNumber(QString serialNumber) const {
    return p_cameraMap.contains(serialNumber);
  }


  /**
   * Return the number of measures in image specified by serialNumber
   *
   * @return Number of valid measures in image
   *
   * @history 2013-12-18 Tracie Sucharski - Renamed from GetNumberOfMeasuresInImage, it is
   *                         returning a count of only valid measures (Ignore=False).
   */
  int LidarData::GetNumberOfValidMeasuresInImage(const QString &serialNumber) {
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
  int LidarData::GetNumberOfJigsawRejectedMeasuresInImage(const QString &serialNumber) {
    return p_cameraRejectedMeasuresMap[serialNumber];
  }

  /**
   * Get all the valid measures pertaining to a given cube serial number
   *
   * @returns A list of all valid measures which are in a given cube
   */
  QList< ControlMeasure * > LidarData::GetValidMeasuresInCube(QString serialNumber) {
    QList< ControlMeasure * > validMeasures;

    // Get measures in cube will validate this for us, so we don't need to re-check
    QList< ControlMeasure * > measureList = GetMeasuresInCube(serialNumber);

    foreach(ControlMeasure * measure, measureList) {
      if (!measure->IsIgnored()) {
        validMeasures.append(measure);
      }
    }

    return validMeasures;
  }

  /**
   * Get all the measures pertaining to a given cube serial number
   *
   * @returns A list of all measures which are in a given cube
   */
  QList< ControlMeasure * > LidarData::GetMeasuresInCube(QString serialNumber) {
    if( !ValidateSerialNumber(serialNumber) ) {
      IString msg = "Cube Serial Number [" + serialNumber.toStdString() + "] not found in "
          "the network";
      throw IException(IException::Programmer, msg, _FILEINFO_);

    }
    QList< ControlMeasure * > measures;
    foreach(QSharedPointer <LidarControlPoint> point, m_points) {
      if (point->HasSerialNumber(serialNumber)) {
        measures.append(point->GetMeasure(serialNumber));
      }
    }
    return measures;
  }
}
