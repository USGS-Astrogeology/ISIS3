/** This is free and unencumbered software released into the public domain.

The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */

#include "ControlPointV0002.h"

#include <QString>

#include "ControlPointV0001.h"
#include "IException.h"
#include "Pvl.h"
#include "PvlContainer.h"

using namespace std;

namespace Isis {

  /**
   * Create a ControlPointV0002 object from a protobuf version 1 control point message.
   *
   * @param pointData The protobuf message from a control net file.
   * @param logData The accompanying protobuf control measure log data for the point.
   */
  ControlPointV0002::ControlPointV0002(
          QSharedPointer<ControlNetFileProtoV0001_PBControlPoint> pointData,
          QSharedPointer<ControlNetLogDataProtoV0001_Point> logData)
      : m_pointData(pointData), m_logData(logData) {

  }


  /**
   * Create a ControlPointV0002 object from a version 2 control point Pvl object.
   *
   * @param pointObject The control point and its measures in a Pvl object.
   */
  ControlPointV0002::ControlPointV0002(PvlObject &pointObject)
      : m_pointData(new ControlNetFileProtoV0001_PBControlPoint),
        m_logData(new ControlNetLogDataProtoV0001_Point) {

    // Copy over strings, doubles, and bools
    copy(pointObject, "PointId",
         m_pointData, &ControlNetFileProtoV0001_PBControlPoint::set_id);
    copy(pointObject, "ChooserName",
         m_pointData, &ControlNetFileProtoV0001_PBControlPoint::set_choosername);
    copy(pointObject, "DateTime",
         m_pointData, &ControlNetFileProtoV0001_PBControlPoint::set_datetime);
    copy(pointObject, "AprioriXYZSourceFile",
         m_pointData, &ControlNetFileProtoV0001_PBControlPoint::set_apriorisurfpointsourcefile);
    copy(pointObject, "AprioriRadiusSourceFile",
         m_pointData, &ControlNetFileProtoV0001_PBControlPoint::set_aprioriradiussourcefile);
    copy(pointObject, "JigsawRejected",
         m_pointData, &ControlNetFileProtoV0001_PBControlPoint::set_jigsawrejected);
    copy(pointObject, "EditLock",
         m_pointData, &ControlNetFileProtoV0001_PBControlPoint::set_editlock);
    copy(pointObject, "Ignore",
         m_pointData, &ControlNetFileProtoV0001_PBControlPoint::set_ignore);
    copy(pointObject, "AprioriX",
         m_pointData, &ControlNetFileProtoV0001_PBControlPoint::set_apriorix);
    copy(pointObject, "AprioriY",
         m_pointData, &ControlNetFileProtoV0001_PBControlPoint::set_aprioriy);
    copy(pointObject, "AprioriZ",
         m_pointData, &ControlNetFileProtoV0001_PBControlPoint::set_aprioriz);
    copy(pointObject, "AdjustedX",
         m_pointData, &ControlNetFileProtoV0001_PBControlPoint::set_adjustedx);
    copy(pointObject, "AdjustedY",
         m_pointData, &ControlNetFileProtoV0001_PBControlPoint::set_adjustedy);
    copy(pointObject, "AdjustedZ",
         m_pointData, &ControlNetFileProtoV0001_PBControlPoint::set_adjustedz);
    copy(pointObject, "LatitudeConstrained",
         m_pointData, &ControlNetFileProtoV0001_PBControlPoint::set_latitudeconstrained);
    copy(pointObject, "LongitudeConstrained",
         m_pointData, &ControlNetFileProtoV0001_PBControlPoint::set_longitudeconstrained);
    copy(pointObject, "RadiusConstrained",
         m_pointData, &ControlNetFileProtoV0001_PBControlPoint::set_radiusconstrained);

    // Copy enumerated values

    if (pointObject["PointType"][0] == "Ground") {
      m_pointData->set_type(ControlNetFileProtoV0001_PBControlPoint::Ground);
    }
    else if (pointObject["PointType"][0] == "Tie") {
      m_pointData->set_type(ControlNetFileProtoV0001_PBControlPoint::Tie);
    }
    else {
      std::string msg = "Invalid ControlPoint type [" + pointObject["PointType"][0] + "].";
      throw IException(IException::User, msg, _FILEINFO_);
    }

    if (pointObject.hasKeyword("AprioriXYZSource")) {
      QString source = QString::fromStdString(pointObject["AprioriXYZSource"][0]);

      if (source == "None") {
        m_pointData->set_apriorisurfpointsource(ControlNetFileProtoV0001_PBControlPoint::None);
      }
      else if (source == "User") {
        m_pointData->set_apriorisurfpointsource(ControlNetFileProtoV0001_PBControlPoint::User);
      }
      else if (source == "AverageOfMeasures") {
        m_pointData->set_apriorisurfpointsource(
                           ControlNetFileProtoV0001_PBControlPoint::AverageOfMeasures);
      }
      else if (source == "Reference") {
        m_pointData->set_apriorisurfpointsource(
                           ControlNetFileProtoV0001_PBControlPoint::Reference);
      }
      else if (source == "Basemap") {
        m_pointData->set_apriorisurfpointsource(ControlNetFileProtoV0001_PBControlPoint::Basemap);
      }
      else if (source == "BundleSolution") {
        m_pointData->set_apriorisurfpointsource(
                           ControlNetFileProtoV0001_PBControlPoint::BundleSolution);
      }
      else {
        std::string msg = "Invalid AprioriXYZSource [" + source.toStdString() + "]";
        throw IException(IException::User, msg, _FILEINFO_);
      }
    }

    if (pointObject.hasKeyword("AprioriRadiusSource")) {
      QString source = QString::fromStdString(pointObject["AprioriRadiusSource"][0]);

      if (source == "None") {
        m_pointData->set_aprioriradiussource(ControlNetFileProtoV0001_PBControlPoint::None);
      }
      else if (source == "User") {
        m_pointData->set_aprioriradiussource(ControlNetFileProtoV0001_PBControlPoint::User);
      }
      else if (source == "AverageOfMeasures") {
        m_pointData->set_aprioriradiussource(
                           ControlNetFileProtoV0001_PBControlPoint::AverageOfMeasures);
      }
      else if (source == "Ellipsoid") {
        m_pointData->set_aprioriradiussource(ControlNetFileProtoV0001_PBControlPoint::Ellipsoid);
      }
      else if (source == "DEM") {
        m_pointData->set_aprioriradiussource(ControlNetFileProtoV0001_PBControlPoint::DEM);
      }
      else if (source == "BundleSolution") {
        m_pointData->set_aprioriradiussource(
                           ControlNetFileProtoV0001_PBControlPoint::BundleSolution);
      }
      else {
        std::string msg = "Invalid AprioriRadiusSource, [" + source.toStdString() + "]";
        throw IException(IException::User, msg, _FILEINFO_);
      }
    }

    // Copy array values
    if (pointObject.hasKeyword("AprioriCovarianceMatrix")) {
      PvlKeyword &matrix = pointObject["AprioriCovarianceMatrix"];

      m_pointData->add_aprioricovar(Isis::toDouble(matrix[0]));
      m_pointData->add_aprioricovar(Isis::toDouble(matrix[1]));
      m_pointData->add_aprioricovar(Isis::toDouble(matrix[2]));
      m_pointData->add_aprioricovar(Isis::toDouble(matrix[3]));
      m_pointData->add_aprioricovar(Isis::toDouble(matrix[4]));
      m_pointData->add_aprioricovar(Isis::toDouble(matrix[5]));
    }

    if (pointObject.hasKeyword("AdjustedCovarianceMatrix")) {
      PvlKeyword &matrix = pointObject["AdjustedCovarianceMatrix"];

      m_pointData->add_adjustedcovar(Isis::toDouble(matrix[0]));
      m_pointData->add_adjustedcovar(Isis::toDouble(matrix[1]));
      m_pointData->add_adjustedcovar(Isis::toDouble(matrix[2]));
      m_pointData->add_adjustedcovar(Isis::toDouble(matrix[3]));
      m_pointData->add_adjustedcovar(Isis::toDouble(matrix[4]));
      m_pointData->add_adjustedcovar(Isis::toDouble(matrix[5]));
    }

    //  Process Measures
    for (int groupIndex = 0; groupIndex < pointObject.groups(); groupIndex ++) {
      PvlGroup &group = pointObject.group(groupIndex);
      ControlNetFileProtoV0001_PBControlPoint_PBControlMeasure measure;

      // Copy strings, booleans, and doubles
      copy(group, "SerialNumber",
           measure, &ControlNetFileProtoV0001_PBControlPoint_PBControlMeasure::set_serialnumber);
      copy(group, "ChooserName",
           measure, &ControlNetFileProtoV0001_PBControlPoint_PBControlMeasure::set_choosername);
      copy(group, "DateTime",
           measure, &ControlNetFileProtoV0001_PBControlPoint_PBControlMeasure::set_datetime);
      copy(group, "Diameter",
           measure, &ControlNetFileProtoV0001_PBControlPoint_PBControlMeasure::set_diameter);
      copy(group, "EditLock",
           measure, &ControlNetFileProtoV0001_PBControlPoint_PBControlMeasure::set_editlock);
      copy(group, "Ignore",
           measure, &ControlNetFileProtoV0001_PBControlPoint_PBControlMeasure::set_ignore);
      copy(group, "JigsawRejected",
           measure, &ControlNetFileProtoV0001_PBControlPoint_PBControlMeasure::set_jigsawrejected);
      copy(group, "AprioriSample",
           measure, &ControlNetFileProtoV0001_PBControlPoint_PBControlMeasure::set_apriorisample);
      copy(group, "AprioriLine",
           measure, &ControlNetFileProtoV0001_PBControlPoint_PBControlMeasure::set_aprioriline);
      copy(group, "SampleSigma",
           measure, &ControlNetFileProtoV0001_PBControlPoint_PBControlMeasure::set_samplesigma);
      copy(group, "LineSigma",
           measure, &ControlNetFileProtoV0001_PBControlPoint_PBControlMeasure::set_linesigma);

      // The sample, line, sample residual, and line residual are nested in another structure
      // inside the measure, so they cannot be copied with the conenience methods.
      if (group.hasKeyword("Sample")) {
        double value = Isis::toDouble(group["Sample"][0]);
        measure.mutable_measurement()->set_sample(value);
        group.deleteKeyword("Sample");
      }
      if (group.hasKeyword("Line")) {
        double value = Isis::toDouble(group["Line"][0]);
        measure.mutable_measurement()->set_line(value);
        group.deleteKeyword("Line");
      }
      if (group.hasKeyword("SampleResidual")) {
        double value = Isis::toDouble(group["SampleResidual"][0]);
        measure.mutable_measurement()->set_sampleresidual(value);
        group.deleteKeyword("SampleResidual");
      }
      if (group.hasKeyword("LineResidual")) {
        double value = Isis::toDouble(group["LineResidual"][0]);
        measure.mutable_measurement()->set_lineresidual(value);
        group.deleteKeyword("LineResidual");
      }
      if (group.hasKeyword("Reference")) {
        if (QString::fromStdString(group["Reference"][0]).toLower() == "true") {
          m_pointData->set_referenceindex(groupIndex);
        }
        group.deleteKeyword("Reference");
      }

      QString type = QString::fromStdString(group["MeasureType"][0]).toLower();
      if (type == "candidate") {
        measure.set_type(ControlNetFileProtoV0001_PBControlPoint_PBControlMeasure::Candidate);
      }
      else if (type == "manual") {
        measure.set_type(ControlNetFileProtoV0001_PBControlPoint_PBControlMeasure::Manual);
      }
      else if (type == "registeredpixel") {
        measure.set_type(
              ControlNetFileProtoV0001_PBControlPoint_PBControlMeasure::RegisteredPixel);
      }
      else if (type == "registeredsubpixel") {
        measure.set_type(
              ControlNetFileProtoV0001_PBControlPoint_PBControlMeasure::RegisteredSubPixel);
      }
      else {
        throw IException(IException::Io,
                         "Unknown measure type [" + type.toStdString() + "]",
                         _FILEINFO_);
      }
      group.deleteKeyword("MeasureType");

      // Clean up the remaining keywords
      // This also removes obsolete log data entries
      for (int cmKeyIndex = 0; cmKeyIndex < group.keywords(); cmKeyIndex ++) {
        if (group[cmKeyIndex][0] == ""
            || group[cmKeyIndex].name() == "ZScore"
            || group[cmKeyIndex].name() == "ErrorMagnitude") {
          group.deleteKeyword(cmKeyIndex);
        }
      }

      // Create the log data for the measure
      ControlNetLogDataProtoV0001_Point_Measure measureLogData;

      for (int keyIndex = 0; keyIndex < group.keywords(); keyIndex++) {
        PvlKeyword dataKeyword = group[keyIndex];
        QString name = QString::fromStdString(dataKeyword.name());
        int dataType = 0;
        double value = 0.0;

        if (name == "Obsolete_Eccentricity") {
          dataType = 1;
        }
        else if (name == "GoodnessOfFit") {
          dataType = 2;
        }
        else if (name ==  "MinimumPixelZScore") {
          dataType = 3;
        }
        else if (name ==  "MaximumPixelZScore") {
          dataType = 4;
        }
        else if (name == "PixelShift") {
          dataType = 5;
        }
        else if (name == "WholePixelCorrelation") {
          dataType = 6;
        }
        else if (name == "SubPixelCorrelation") {
          dataType = 7;
        }
        else if (name == "Obsolete_AverageResidual") {
          dataType = 8;
        }
        else {
          std::string msg = "Invalid control measure log data name [" + name.toStdString() + "]";
          throw IException(IException::Programmer, msg, _FILEINFO_);
        }

        try {
          value = Isis::toDouble(dataKeyword[0]);
        }
        catch (IException &e) {
          std::string msg = "Invalid control measure log data value [" + dataKeyword[0] + "]";
          throw IException(e, IException::Io, msg, _FILEINFO_);
        }

        ControlNetLogDataProtoV0001_Point_Measure_DataEntry logEntry;
        logEntry.set_datatype(dataType);
        logEntry.set_datavalue(value);
        *measureLogData.add_loggedmeasuredata() = logEntry;
      }

      // Store the measure and its log data
      *m_pointData->add_measures() = measure;
      *m_logData->add_measures() = measureLogData;
    }

    if (!m_pointData->IsInitialized()) {
      std::string msg = "There is missing required information in the control "
                    "points or measures";
      throw IException(IException::Io, msg, _FILEINFO_);
    }
  }


  /**
   * Create a version 2 control point from a version 1 control point. The two versions internally
   * store the same protobuf message, so all this does is copy the pointer to the internal protobuf
   * object.
   *
   * @note Because the two points share the same container, modifications to one will affect the
   *       other.
   *
   * @param oldPoint The old version 1 control point.
   */
  ControlPointV0002::ControlPointV0002(ControlPointV0001 &oldPoint)
      : m_pointData(oldPoint.pointData()), m_logData(oldPoint.logData()) {

  }


  /**
   * Access the protobuf control point data.
   *
   * @return @b QSharedPointer<ControlNetFileProtoV0001_PBControlPoint> A pointer to the internal
   *                                                                    point data.
   */
  QSharedPointer<ControlNetFileProtoV0001_PBControlPoint> ControlPointV0002::pointData() {
    return m_pointData;
  }


  /**
   * Access the protobuf log data for the control measures in the point.
   *
   * @return @b QSharedPointer<ControlNetLogDataProtoV0001_Point> A pointer to the internal
   *                                                              measure log data.
   */
  QSharedPointer<ControlNetLogDataProtoV0001_Point> ControlPointV0002::logData() {
    return m_logData;
  }


  /**
   * This convenience method takes a boolean value from a PvlKeyword and copies it into a version 1
   * protobuf field. Once copied, the PvlKeyword is deleted.
   *
   * If the keyword doesn't exist, this does nothing.
   *
   * @param container The PvlContainer representation of the control point that contains the
   *                  PvlKeyword.
   * @param keyName The name of the keyword to be copied.
   * @param[out] point The version 1 protobuf representation of the control point that the value
   *                   will be copied into.
   * @param setter The protobuf mutator method that sets the value of the field in the protobuf
   *               representation.
   */
  void ControlPointV0002::copy(PvlContainer &container,
                               QString keyName,
                               QSharedPointer<ControlNetFileProtoV0001_PBControlPoint> point,
                               void (ControlNetFileProtoV0001_PBControlPoint::*setter)(bool)) {

    if (!container.hasKeyword(keyName.toStdString())) {
      return;
    }

    QString value = QString::fromStdString(container[keyName.toStdString()][0]);
    container.deleteKeyword(keyName.toStdString());
    value = value.toLower();

    if (value == "true" || value == "yes") {
      (point.data()->*setter)(true);
    }
  }


  /**
   * This convenience method takes a double value from a PvlKeyword and copies it into a version 1
   * protobuf field. Once copied, the PvlKeyword is deleted.
   *
   * If the keyword doesn't exist, this does nothing.
   *
   * @param container The PvlContainer representation of the control point that contains the
   *                  PvlKeyword.
   * @param keyName The name of the keyword to be copied.
   * @param[out] point The version 1 protobuf representation of the control point that the value
   *                   will be copied into.
   * @param setter The protobuf mutator method that sets the value of the field in the protobuf
   *               representation.
   */
  void ControlPointV0002::copy(PvlContainer &container,
                               QString keyName,
                               QSharedPointer<ControlNetFileProtoV0001_PBControlPoint> point,
                               void (ControlNetFileProtoV0001_PBControlPoint::*setter)(double)) {

    if (!container.hasKeyword(keyName.toStdString())) {
      return;
    }

    double value = Isis::toDouble(container[keyName.toStdString()][0]);
    container.deleteKeyword(keyName.toStdString());
    (point.data()->*setter)(value);
  }


  /**
   * This convenience method takes a string value from a PvlKeyword and copies it into a version 1
   * protobuf field. Once copied, the PvlKeyword is deleted.
   *
   * If the keyword doesn't exist, this does nothing.
   *
   * @param container The PvlContainer representation of the control point that contains the
   *                  PvlKeyword.
   * @param keyName The name of the keyword to be copied.
   * @param[out] point The version 1 protobuf representation of the control point that the value
   *                   will be copied into.
   * @param setter The protobuf mutator method that sets the value of the field in the protobuf
   *               representation.
   */
  void ControlPointV0002::copy(PvlContainer &container,
                               QString keyName,
                               QSharedPointer<ControlNetFileProtoV0001_PBControlPoint> point,
                               void (ControlNetFileProtoV0001_PBControlPoint::*setter)(const std::string&)) {

    if (!container.hasKeyword(keyName.toStdString())) {
      return;
    }

    QString value = QString::fromStdString(container[keyName.toStdString()][0]);
    container.deleteKeyword(keyName.toStdString());
    (point.data()->*setter)(value.toLatin1().data());
  }


  /**
   * This convenience method takes a boolean value from a PvlKeyword and copies it into a version 1
   * protobuf field. Once copied, the PvlKeyword is deleted.
   *
   * If the keyword doesn't exist, this does nothing.
   *
   * @param container The PvlContainer representation of the control measure that contains the
   *                  PvlKeyword.
   * @param keyName The name of the keyword to be copied.
   * @param[out] measure The version 1 protobuf representation of the control measure that the
   *                     value will be copied into.
   * @param setter The protobuf mutator method that sets the value of the field in the protobuf
   *               representation.
   */
  void ControlPointV0002::copy(PvlContainer &container,
                               QString keyName,
                               ControlNetFileProtoV0001_PBControlPoint_PBControlMeasure &measure,
                               void (ControlNetFileProtoV0001_PBControlPoint_PBControlMeasure::*setter)(bool)) {

    if (!container.hasKeyword(keyName.toStdString())) {
      return;
    }

    QString value = QString::fromStdString(container[keyName.toStdString()][0]);
    container.deleteKeyword(keyName.toStdString());
    value = value.toLower();

    if (value == "true" || value == "yes") {
      (measure.*setter)(true);
    }
  }


  /**
   * This convenience method takes a double value from a PvlKeyword and copies it into a version 1
   * protobuf field. Once copied, the PvlKeyword is deleted.
   *
   * If the keyword doesn't exist, this does nothing.
   *
   * @param container The PvlContainer representation of the control measure that contains the
   *                  PvlKeyword.
   * @param keyName The name of the keyword to be copied.
   * @param[out] measure The version 1 protobuf representation of the control measure that the
   *                     value will be copied into.
   * @param setter The protobuf mutator method that sets the value of the field in the protobuf
   *               representation.
   */
  void ControlPointV0002::copy(PvlContainer &container,
                               QString keyName,
                               ControlNetFileProtoV0001_PBControlPoint_PBControlMeasure &measure,
                               void (ControlNetFileProtoV0001_PBControlPoint_PBControlMeasure::*setter)(double)) {

    if (!container.hasKeyword(keyName.toStdString())) {
      return;
    }

    double value = Isis::toDouble(container[keyName.toStdString()][0]);
    container.deleteKeyword(keyName.toStdString());
    (measure.*setter)(value);
  }


  /**
   * This convenience method takes a string value from a PvlKeyword and copies it into a version 1
   * protobuf field. Once copied, the PvlKeyword is deleted.
   *
   * If the keyword doesn't exist, this does nothing.
   *
   * @param container The PvlContainer representation of the control measure that contains the
   *                  PvlKeyword.
   * @param keyName The name of the keyword to be copied.
   * @param[out] measure The version 1 protobuf representation of the control measure that the
   *                     value will be into.
   * @param setter The protobuf mutator method that sets the value of the field in the protobuf
   *               representation.
   */
  void ControlPointV0002::copy(PvlContainer &container,
                               QString keyName,
                               ControlNetFileProtoV0001_PBControlPoint_PBControlMeasure &measure,
                               void (ControlNetFileProtoV0001_PBControlPoint_PBControlMeasure::*setter)
                                      (const std::string &)) {

    if (!container.hasKeyword(keyName.toStdString())) {
      return;
    }

    QString value = QString::fromStdString(container[keyName.toStdString()][0]);
    container.deleteKeyword(keyName.toStdString());
    (measure.*setter)(value.toLatin1().data());
  }
}
