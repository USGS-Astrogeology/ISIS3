/** This is free and unencumbered software released into the public domain.

The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */

#include "ControlPointV0003.h"

#include <QString>

#include "ControlMeasureLogData.h"
#include "ControlPointFileEntryV0002.pb.h"
#include "ControlPointV0002.h"
#include "IException.h"
#include "PvlObject.h"
#include "PvlContainer.h"

using namespace std;

namespace Isis {

  /**
   * Create a ControlPointV0003 object from a protobuf version 2 control point message.
   *
   * @param pointData The protobuf message from a control net file.
   */
  ControlPointV0003::ControlPointV0003(QSharedPointer<ControlPointFileEntryV0002> pointData)
      : m_pointData(pointData) {

  }


  /**
   * Create a ControlPointV0003 object from a version 3 or 4 control point Pvl object
   *
   * @param pointObject The control point and its measures in a Pvl object
   */
  ControlPointV0003::ControlPointV0003(PvlObject &pointObject)
      : m_pointData(new ControlPointFileEntryV0002) {

    // Copy over strings, doubles, and bools
    copy(pointObject, "PointId",
         m_pointData, &ControlPointFileEntryV0002::set_id);
    copy(pointObject, "ChooserName",
         m_pointData, &ControlPointFileEntryV0002::set_choosername);
    copy(pointObject, "DateTime",
         m_pointData, &ControlPointFileEntryV0002::set_datetime);
    copy(pointObject, "AprioriXYZSourceFile",
         m_pointData, &ControlPointFileEntryV0002::set_apriorisurfpointsourcefile);
    copy(pointObject, "AprioriRadiusSourceFile",
         m_pointData, &ControlPointFileEntryV0002::set_aprioriradiussourcefile);
    copy(pointObject, "JigsawRejected",
         m_pointData, &ControlPointFileEntryV0002::set_jigsawrejected);
    copy(pointObject, "EditLock",
         m_pointData, &ControlPointFileEntryV0002::set_editlock);
    copy(pointObject, "Ignore",
         m_pointData, &ControlPointFileEntryV0002::set_ignore);
    copy(pointObject, "AprioriX",
         m_pointData, &ControlPointFileEntryV0002::set_apriorix);
    copy(pointObject, "AprioriY",
         m_pointData, &ControlPointFileEntryV0002::set_aprioriy);
    copy(pointObject, "AprioriZ",
         m_pointData, &ControlPointFileEntryV0002::set_aprioriz);
    copy(pointObject, "AdjustedX",
         m_pointData, &ControlPointFileEntryV0002::set_adjustedx);
    copy(pointObject, "AdjustedY",
         m_pointData, &ControlPointFileEntryV0002::set_adjustedy);
    copy(pointObject, "AdjustedZ",
         m_pointData, &ControlPointFileEntryV0002::set_adjustedz);
    copy(pointObject, "LatitudeConstrained",
         m_pointData, &ControlPointFileEntryV0002::set_latitudeconstrained);
    copy(pointObject, "LongitudeConstrained",
         m_pointData, &ControlPointFileEntryV0002::set_longitudeconstrained);
    copy(pointObject, "RadiusConstrained",
         m_pointData, &ControlPointFileEntryV0002::set_radiusconstrained);

    // Copy enumerated values

    // The control point type names were changed between version 3 and version 4.
    // In version 3, the types are ground, tie, and constrained
    // In version 4, these were changed to fixed, free, and constrained respectively.
    // The protobuf file version was not changed, fixed and free were simply added to the
    // enumeration and the old names were flagged as obsolete.
    if (pointObject["PointType"][0] == "Fixed"
        || pointObject["PointType"][0] == "Ground") {
      m_pointData->set_type(ControlPointFileEntryV0002::Fixed);
    }
    else if (pointObject["PointType"][0] == "Constrained") {
      m_pointData->set_type(ControlPointFileEntryV0002::Constrained);
    }
    else if (pointObject["PointType"][0] == "Free"
             || pointObject["PointType"][0] == "Tie") {
      m_pointData->set_type(ControlPointFileEntryV0002::Free);
    }
    else {
      std::string msg = "Invalid ControlPoint type [" + pointObject["PointType"][0] + "].";
      throw IException(IException::User, msg, _FILEINFO_);
    }

    if (pointObject.hasKeyword("AprioriXYZSource")) {
      QString source = QString::fromStdString(pointObject["AprioriXYZSource"][0]);

      if (source == "None") {
        m_pointData->set_apriorisurfpointsource(ControlPointFileEntryV0002::None);
      }
      else if (source == "User") {
        m_pointData->set_apriorisurfpointsource(ControlPointFileEntryV0002::User);
      }
      else if (source == "AverageOfMeasures") {
        m_pointData->set_apriorisurfpointsource(ControlPointFileEntryV0002::AverageOfMeasures);
      }
      else if (source == "Reference") {
        m_pointData->set_apriorisurfpointsource(ControlPointFileEntryV0002::Reference);
      }
      else if (source == "Basemap") {
        m_pointData->set_apriorisurfpointsource(ControlPointFileEntryV0002::Basemap);
      }
      else if (source == "BundleSolution") {
        m_pointData->set_apriorisurfpointsource(ControlPointFileEntryV0002::BundleSolution);
      }
      else {
        std::string msg = "Invalid AprioriXYZSource [" + source.toStdString() + "]";
        throw IException(IException::User, msg, _FILEINFO_);
      }
    }

    if (pointObject.hasKeyword("AprioriRadiusSource")) {
      QString source = QString::fromStdString(pointObject["AprioriRadiusSource"][0]);

      if (source == "None") {
        m_pointData->set_aprioriradiussource(ControlPointFileEntryV0002::None);
      }
      else if (source == "User") {
        m_pointData->set_aprioriradiussource(ControlPointFileEntryV0002::User);
      }
      else if (source == "AverageOfMeasures") {
        m_pointData->set_aprioriradiussource(ControlPointFileEntryV0002::AverageOfMeasures);
      }
      else if (source == "Ellipsoid") {
        m_pointData->set_aprioriradiussource(ControlPointFileEntryV0002::Ellipsoid);
      }
      else if (source == "DEM") {
        m_pointData->set_aprioriradiussource(ControlPointFileEntryV0002::DEM);
      }
      else if (source == "BundleSolution") {
        m_pointData->set_aprioriradiussource(ControlPointFileEntryV0002::BundleSolution);
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
      ControlPointFileEntryV0002_Measure measure;

      // Copy strings, booleans, and doubles
      copy(group, "SerialNumber",
           measure, &ControlPointFileEntryV0002_Measure::set_serialnumber);
      copy(group, "ChooserName",
           measure, &ControlPointFileEntryV0002_Measure::set_choosername);
      copy(group, "Sample",
           measure, &ControlPointFileEntryV0002_Measure::set_sample);
      copy(group, "Line",
           measure, &ControlPointFileEntryV0002_Measure::set_line);
      copy(group, "SampleResidual",
           measure, &ControlPointFileEntryV0002_Measure::set_sampleresidual);
      copy(group, "LineResidual",
           measure, &ControlPointFileEntryV0002_Measure::set_lineresidual);
      copy(group, "DateTime",
           measure, &ControlPointFileEntryV0002_Measure::set_datetime);
      copy(group, "Diameter",
           measure, &ControlPointFileEntryV0002_Measure::set_diameter);
      copy(group, "EditLock",
           measure, &ControlPointFileEntryV0002_Measure::set_editlock);
      copy(group, "Ignore",
           measure, &ControlPointFileEntryV0002_Measure::set_ignore);
      copy(group, "JigsawRejected",
           measure, &ControlPointFileEntryV0002_Measure::set_jigsawrejected);
      copy(group, "AprioriSample",
           measure, &ControlPointFileEntryV0002_Measure::set_apriorisample);
      copy(group, "AprioriLine",
           measure, &ControlPointFileEntryV0002_Measure::set_aprioriline);
      copy(group, "SampleSigma",
           measure, &ControlPointFileEntryV0002_Measure::set_samplesigma);
      copy(group, "LineSigma",
           measure, &ControlPointFileEntryV0002_Measure::set_linesigma);

      if (group.hasKeyword("Reference")) {
        if (QString::fromStdString(group["Reference"][0]).toLower() == "true") {
          m_pointData->set_referenceindex(groupIndex);
        }
        group.deleteKeyword("Reference");
      }

      QString type = QString::fromStdString(group["MeasureType"][0]).toLower();
      if (type == "candidate") {
        measure.set_type(ControlPointFileEntryV0002_Measure::Candidate);
      }
      else if (type == "manual") {
        measure.set_type(ControlPointFileEntryV0002_Measure::Manual);
      }
      else if (type == "registeredpixel") {
        measure.set_type(ControlPointFileEntryV0002_Measure::RegisteredPixel);
      }
      else if (type == "registeredsubpixel") {
        measure.set_type(ControlPointFileEntryV0002_Measure::RegisteredSubPixel);
      }
      else {
        throw IException(IException::Io,
                         "Unknown measure type [" + type.toStdString() + "]",
                         _FILEINFO_);
      }
      group.deleteKeyword("MeasureType");

      for (int key = 0; key < group.keywords(); key++) {
        ControlMeasureLogData interpreter(group[key]);
        if (!interpreter.IsValid()) {
          std::string msg = "Unhandled or duplicate keywords in control measure ["
                        + group[key].name() + "]";
          throw IException(IException::Programmer, msg, _FILEINFO_);
        }
        else {
          ControlPointFileEntryV0002_Measure_MeasureLogData protoBufDataEntry;

          protoBufDataEntry.set_doubledatatype(interpreter.GetDataType());
          protoBufDataEntry.set_doubledatavalue(interpreter.GetNumericalValue());

          *measure.add_log() = protoBufDataEntry;
        }
      }

      *m_pointData->add_measures() = measure;
    }

    if (!m_pointData->IsInitialized()) {
      std::string msg = "There is missing required information in the control "
                    "points or measures";
      throw IException(IException::Io, msg, _FILEINFO_);
    }
  }


  /**
   * Create a ControlPointV0003 object from a ControlPointV0002 object
   *
   * @param oldPoint The PvlControlPointV0002 that will be upgraded to V0003.
   */
  ControlPointV0003::ControlPointV0003(ControlPointV0002 &oldPoint)
      : m_pointData(new ControlPointFileEntryV0002) {
    QSharedPointer<ControlNetFileProtoV0001_PBControlPoint> oldPointData = oldPoint.pointData();
    if (!oldPointData) {
      std::string msg = "Version 2 control point is missing point data.";
      throw IException(IException::User, msg, _FILEINFO_);
    }
    QSharedPointer<ControlNetLogDataProtoV0001_Point> oldLogData = oldPoint.logData();
    if (!oldLogData) {
      std::string msg = "Version 2 control point is missing measure log data.";
      throw IException(IException::User, msg, _FILEINFO_);
    }

    // Copy over POD values
    if ( oldPointData->has_id() ) {
      m_pointData->set_id( oldPointData->id() );
    }
    if ( oldPointData->has_choosername() ) {
      m_pointData->set_choosername( oldPointData->choosername() );
    }
    if ( oldPointData->has_datetime() ) {
      m_pointData->set_datetime( oldPointData->datetime() );
    }
    if ( oldPointData->has_apriorisurfpointsourcefile() ) {
      m_pointData->set_apriorisurfpointsourcefile( oldPointData->apriorisurfpointsourcefile() );
    }
    if ( oldPointData->has_aprioriradiussourcefile() ) {
      m_pointData->set_aprioriradiussourcefile( oldPointData->aprioriradiussourcefile() );
    }
    if ( oldPointData->has_jigsawrejected() ) {
      m_pointData->set_jigsawrejected( oldPointData->jigsawrejected() );
    }
    if ( oldPointData->has_editlock() ) {
      m_pointData->set_editlock( oldPointData->editlock() );
    }
    if ( oldPointData->has_ignore() ) {
      m_pointData->set_ignore( oldPointData->ignore() );
    }
    if ( oldPointData->has_apriorix() ) {
      m_pointData->set_apriorix( oldPointData->apriorix() );
    }
    if ( oldPointData->has_aprioriy() ) {
      m_pointData->set_aprioriy( oldPointData->aprioriy() );
    }
    if ( oldPointData->has_aprioriz() ) {
      m_pointData->set_aprioriz( oldPointData->aprioriz() );
    }
    if ( oldPointData->has_adjustedx() ) {
      m_pointData->set_adjustedx( oldPointData->adjustedx() );
    }
    if ( oldPointData->has_adjustedy() ) {
      m_pointData->set_adjustedy( oldPointData->adjustedy() );
    }
    if ( oldPointData->has_adjustedz() ) {
      m_pointData->set_adjustedz( oldPointData->adjustedz() );
    }
    if ( oldPointData->has_latitudeconstrained() ) {
      m_pointData->set_latitudeconstrained( oldPointData->latitudeconstrained() );
    }
    if ( oldPointData->has_longitudeconstrained() ) {
      m_pointData->set_longitudeconstrained( oldPointData->longitudeconstrained() );
    }
    if ( oldPointData->has_radiusconstrained() ) {
      m_pointData->set_radiusconstrained( oldPointData->radiusconstrained() );
    }
    if ( oldPointData->has_referenceindex() ) {
      m_pointData->set_referenceindex( oldPointData->referenceindex() );
    }

    // Copy over enumerated values

    // The only point types in V0002 are ground and tie.
    // So, convert ground and tie to their V0003 values, fixed and free respectively.
    // Later check if the point is constrained.
    if ( oldPointData->has_type() ) {
      ControlNetFileProtoV0001_PBControlPoint_PointType pointType = oldPointData->type();
      if (pointType == ControlNetFileProtoV0001_PBControlPoint::Ground) {
        m_pointData->set_type(ControlPointFileEntryV0002::Fixed);
      }
      else if (pointType == ControlNetFileProtoV0001_PBControlPoint::Tie) {
        m_pointData->set_type(ControlPointFileEntryV0002::Free);
      }
      else {
        std::string msg = "Invalid ControlPoint type.";
        throw IException(IException::User, msg, _FILEINFO_);
      }
    }

    if ( oldPointData->has_apriorisurfpointsource() ) {
      ControlNetFileProtoV0001_PBControlPoint_AprioriSource surfacePointSource;
      surfacePointSource = oldPointData->apriorisurfpointsource();
      if (surfacePointSource == ControlNetFileProtoV0001_PBControlPoint::None) {
        m_pointData->set_apriorisurfpointsource(ControlPointFileEntryV0002::None);
      }
      else if (surfacePointSource == ControlNetFileProtoV0001_PBControlPoint::User) {
        m_pointData->set_apriorisurfpointsource(ControlPointFileEntryV0002::User);
      }
      else if (surfacePointSource == ControlNetFileProtoV0001_PBControlPoint::AverageOfMeasures) {
        m_pointData->set_apriorisurfpointsource(ControlPointFileEntryV0002::AverageOfMeasures);
      }
      else if (surfacePointSource == ControlNetFileProtoV0001_PBControlPoint::Reference) {
        m_pointData->set_apriorisurfpointsource(ControlPointFileEntryV0002::Reference);
      }
      else if (surfacePointSource == ControlNetFileProtoV0001_PBControlPoint::Basemap) {
        m_pointData->set_apriorisurfpointsource(ControlPointFileEntryV0002::Basemap);
      }
      else if (surfacePointSource == ControlNetFileProtoV0001_PBControlPoint::BundleSolution) {
        m_pointData->set_apriorisurfpointsource(ControlPointFileEntryV0002::BundleSolution);
      }
      else {
        std::string msg = "Invalid ControlPoint apriori surface point source.";
        throw IException(IException::User, msg, _FILEINFO_);
      }
    }

    if ( oldPointData->has_aprioriradiussource() ) {
      ControlNetFileProtoV0001_PBControlPoint_AprioriSource radiusSource;
      radiusSource = oldPointData->aprioriradiussource();
      if (radiusSource == ControlNetFileProtoV0001_PBControlPoint::None) {
        m_pointData->set_aprioriradiussource(ControlPointFileEntryV0002::None);
      }
      else if (radiusSource == ControlNetFileProtoV0001_PBControlPoint::User) {
        m_pointData->set_aprioriradiussource(ControlPointFileEntryV0002::User);
      }
      else if (radiusSource == ControlNetFileProtoV0001_PBControlPoint::AverageOfMeasures) {
        m_pointData->set_aprioriradiussource(ControlPointFileEntryV0002::AverageOfMeasures);
      }
      else if (radiusSource == ControlNetFileProtoV0001_PBControlPoint::Ellipsoid) {
        m_pointData->set_aprioriradiussource(ControlPointFileEntryV0002::Ellipsoid);
      }
      else if (radiusSource == ControlNetFileProtoV0001_PBControlPoint::DEM) {
        m_pointData->set_aprioriradiussource(ControlPointFileEntryV0002::DEM);
      }
      else if (radiusSource == ControlNetFileProtoV0001_PBControlPoint::BundleSolution) {
        m_pointData->set_aprioriradiussource(ControlPointFileEntryV0002::BundleSolution);
      }
      else {
        std::string msg = "Invalid AprioriRadiusSource.";
        throw IException(IException::User, msg, _FILEINFO_);
      }
    }

    // Copy the array values

    // If there is a covariance matrix, then the point is constrained
    if (oldPointData->aprioricovar_size() > 0) {
      m_pointData->set_type(ControlPointFileEntryV0002::Constrained);
      m_pointData->add_aprioricovar( oldPointData->aprioricovar(0) );
      m_pointData->add_aprioricovar( oldPointData->aprioricovar(1) );
      m_pointData->add_aprioricovar( oldPointData->aprioricovar(2) );
      m_pointData->add_aprioricovar( oldPointData->aprioricovar(3) );
      m_pointData->add_aprioricovar( oldPointData->aprioricovar(4) );
      m_pointData->add_aprioricovar( oldPointData->aprioricovar(5) );
    }

    if (oldPointData->adjustedcovar_size() > 0) {
      m_pointData->add_adjustedcovar( oldPointData->adjustedcovar(0) );
      m_pointData->add_adjustedcovar( oldPointData->adjustedcovar(1) );
      m_pointData->add_adjustedcovar( oldPointData->adjustedcovar(2) );
      m_pointData->add_adjustedcovar( oldPointData->adjustedcovar(3) );
      m_pointData->add_adjustedcovar( oldPointData->adjustedcovar(4) );
      m_pointData->add_adjustedcovar( oldPointData->adjustedcovar(5) );
    }

    // Copy the measures
    for (int i = 0; i < oldPointData->measures_size(); i++) {
      ControlPointFileEntryV0002_Measure *newMeasure = m_pointData->add_measures();
      ControlNetFileProtoV0001_PBControlPoint_PBControlMeasure oldMeasure;
      oldMeasure = oldPointData->measures(i);

      // Copy over POD values
      if ( oldMeasure.has_serialnumber() ) {
        newMeasure->set_serialnumber( oldMeasure.serialnumber() );
      }
      if ( oldMeasure.has_choosername() ) {
        newMeasure->set_choosername( oldMeasure.choosername() );
      }
      if ( oldMeasure.has_datetime() ) {
        newMeasure->set_datetime( oldMeasure.datetime() );
      }
      if ( oldMeasure.has_diameter() ) {
        newMeasure->set_diameter( oldMeasure.diameter() );
      }
      if ( oldMeasure.has_editlock() ) {
        newMeasure->set_editlock( oldMeasure.editlock() );
      }
      if ( oldMeasure.has_ignore() ) {
        newMeasure->set_ignore( oldMeasure.ignore() );
      }
      if ( oldMeasure.has_jigsawrejected() ) {
        newMeasure->set_jigsawrejected( oldMeasure.jigsawrejected() );
      }
      if ( oldMeasure.has_apriorisample() ) {
        newMeasure->set_apriorisample( oldMeasure.apriorisample() );
      }
      if ( oldMeasure.has_aprioriline() ) {
        newMeasure->set_aprioriline( oldMeasure.aprioriline() );
      }
      if ( oldMeasure.has_samplesigma() ) {
        newMeasure->set_samplesigma( oldMeasure.samplesigma() );
      }
      if ( oldMeasure.has_linesigma() ) {
        newMeasure->set_linesigma( oldMeasure.linesigma() );
      }
      // In the version 1 protobuf format, the sample, line, sample residual, and line residual
      // values are stored in a nested message so we have to copy them differently.
      if ( oldMeasure.has_measurement() ) {
        ControlNetFileProtoV0001_PBControlPoint_PBControlMeasure_PBMeasure oldMeasurement;
        oldMeasurement = oldMeasure.measurement();
        if ( oldMeasurement.has_sample() ) {
          newMeasure->set_sample( oldMeasurement.sample() );
        }
        if ( oldMeasurement.has_line() ) {
          newMeasure->set_line( oldMeasurement.line() );
        }
        if ( oldMeasurement.has_sampleresidual() ) {
          newMeasure->set_sampleresidual( oldMeasurement.sampleresidual() );
        }
        if ( oldMeasurement.has_lineresidual() ) {
          newMeasure->set_lineresidual( oldMeasurement.lineresidual() );
        }
      }

      // Copy over the enumerated values
      if ( oldMeasure.has_type() ) {
        ControlNetFileProtoV0001_PBControlPoint_PBControlMeasure_MeasureType oldType;
        oldType = oldMeasure.type();
        if (oldType == ControlNetFileProtoV0001_PBControlPoint_PBControlMeasure_MeasureType_Candidate) {
          newMeasure->set_type(ControlPointFileEntryV0002_Measure_MeasureType_Candidate);
        }
        else if (oldType == ControlNetFileProtoV0001_PBControlPoint_PBControlMeasure_MeasureType_Manual) {
          newMeasure->set_type(ControlPointFileEntryV0002_Measure_MeasureType_Manual);
        }
        else if (oldType == ControlNetFileProtoV0001_PBControlPoint_PBControlMeasure_MeasureType_RegisteredPixel) {
          newMeasure->set_type(ControlPointFileEntryV0002_Measure_MeasureType_RegisteredPixel);
        }
        else if (oldType == ControlNetFileProtoV0001_PBControlPoint_PBControlMeasure_MeasureType_RegisteredSubPixel) {
          newMeasure->set_type(ControlPointFileEntryV0002_Measure_MeasureType_RegisteredSubPixel);
        }
        else {
          std::string msg = "Invalid measure type";
          throw IException(IException::User, msg, _FILEINFO_);
        }

        // Copy over any log data
        ControlNetLogDataProtoV0001_Point_Measure measureLogData = oldLogData->measures(i);
        for (int j = 0; j < measureLogData.loggedmeasuredata_size(); j++) {

          ControlNetLogDataProtoV0001_Point_Measure_DataEntry oldData =
              measureLogData.loggedmeasuredata(j);

          ControlPointFileEntryV0002_Measure_MeasureLogData newData;

          newData.set_doubledatatype( oldData.datatype() );
          newData.set_doubledatavalue( oldData.datavalue() );

          *newMeasure->add_log() = newData;
        }

        // Check that all the required fields in the measure are filled
        if ( !newMeasure->IsInitialized() ) {
          std::string msg = "Measure file entry at index [" + toString(i)
                        + "] is missing required fields.";
          throw IException(IException::User, msg, _FILEINFO_);
        }
      }
    }

    // Check that all of the required fields in the point are filled
    if ( !m_pointData->IsInitialized() ) {
      std::string msg = "Control point file entry is missing required fields.";
      throw IException(IException::User, msg, _FILEINFO_);
    }
  }


  /**
   * Access the protobuf control point data. If there is not internal point data then
   * default point data is returned. Note that default point data may be missing required
   * fields.
   *
   * @return @b const ControlPointFileEntryV0002& A constant reference to the internal control
   *                                              point data. There is no guarantee that the point
   *                                              data is fully initialized.
   */
  const ControlPointFileEntryV0002 &ControlPointV0003::pointData() {
    if (!m_pointData) {
      m_pointData.reset(new ControlPointFileEntryV0002);
    }

    return *m_pointData;
  }


  /**
   * This convenience method takes a boolean value from a PvlKeyword and copies it into a version 2
   * protobuf field. Once copied, the PvlKeyword is deleted.
   *
   * If the keyword doesn't exist, this does nothing.
   *
   * @param container The PvlContainer representation of the control point that contains the
   *                  PvlKeyword.
   * @param keyName The name of the keyword to be copied.
   * @param[out] point The version 2 protobuf representation of the control point that the value
   *                   will be copied into.
   * @param setter The protobuf mutator method that sets the value of the field in the protobuf
   *               representation.
   */
  void ControlPointV0003::copy(PvlContainer &container,
                               QString keyName,
                               QSharedPointer<ControlPointFileEntryV0002> point,
                               void (ControlPointFileEntryV0002::*setter)(bool)) {

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
   * This convenience method takes a double value from a PvlKeyword and copies it into a version 2
   * protobuf field. Once copied, the PvlKeyword is deleted.
   *
   * If the keyword doesn't exist, this does nothing.
   *
   * @param container The PvlContainer representation of the control point that contains the
   *                  PvlKeyword.
   * @param keyName The name of the keyword to be copied.
   * @param[out] point The version 2 protobuf representation of the control point that the value
   *                   will be copied into.
   * @param setter The protobuf mutator method that sets the value of the field in the protobuf
   *               representation.
   */
  void ControlPointV0003::copy(PvlContainer &container,
                               QString keyName,
                               QSharedPointer<ControlPointFileEntryV0002> point,
                               void (ControlPointFileEntryV0002::*setter)(double)) {

    if (!container.hasKeyword(keyName.toStdString())) {
      return;
    }

    double value = Isis::toDouble(container[keyName.toStdString()][0]);
    container.deleteKeyword(keyName.toStdString());
    (point.data()->*setter)(value);
  }


  /**
   * This convenience method takes a string value from a PvlKeyword and copies it into a version 2
   * protobuf field. Once copied, the PvlKeyword is deleted.
   *
   * If the keyword doesn't exist, this does nothing.
   *
   * @param container The PvlContainer representation of the control point that contains the
   *                  PvlKeyword.
   * @param keyName The name of the keyword to be copied.
   * @param[out] point The version 2 protobuf representation of the control point that the value
   *                   will be copied into.
   * @param setter The protobuf mutator method that sets the value of the field in the protobuf
   *               representation.
   */
  void ControlPointV0003::copy(PvlContainer &container,
                               QString keyName,
                               QSharedPointer<ControlPointFileEntryV0002> point,
                               void (ControlPointFileEntryV0002::*setter)(const std::string&)) {

    if (!container.hasKeyword(keyName.toStdString())) {
      return;
    }

    QString value = QString::fromStdString(container[keyName.toStdString()][0]);
    container.deleteKeyword(keyName.toStdString());
    (point.data()->*setter)(value.toLatin1().data());
  }


  /**
   * This convenience method takes a boolean value from a PvlKeyword and copies it into a version 2
   * protobuf field. Once copied, the PvlKeyword is deleted.
   *
   * If the keyword doesn't exist, this does nothing.
   *
   * @param container The PvlContainer representation of the control measure that contains the
   *                  PvlKeyword.
   * @param keyName The name of the keyword to be copied.
   * @param[out] measure The version 2 protobuf representation of the control measure that the
   *                     value will be copied into.
   * @param setter The protobuf mutator method that sets the value of the field in the protobuf
   *               representation.
   */
  void ControlPointV0003::copy(PvlContainer &container,
                               QString keyName,
                               ControlPointFileEntryV0002_Measure &measure,
                               void (ControlPointFileEntryV0002_Measure::*setter)(bool)) {

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
   * This convenience method takes a double value from a PvlKeyword and copies it into a version 2
   * protobuf field. Once copied, the PvlKeyword is deleted.
   *
   * If the keyword doesn't exist, this does nothing.
   *
   * @param container The PvlContainer representation of the control measure that contains the
   *                  PvlKeyword.
   * @param keyName The name of the keyword to be copied.
   * @param[out] measure The version 2 protobuf representation of the control measure that the
   *                     value will be copied into.
   * @param setter The protobuf mutator method that sets the value of the field in the protobuf
   *               representation.
   */
  void ControlPointV0003::copy(PvlContainer &container,
                               QString keyName,
                               ControlPointFileEntryV0002_Measure &measure,
                               void (ControlPointFileEntryV0002_Measure::*setter)(double)) {

    if (!container.hasKeyword(keyName.toStdString())) {
      return;
    }

    double value = Isis::toDouble(container[keyName.toStdString()][0]);
    container.deleteKeyword(keyName.toStdString());
    (measure.*setter)(value);
  }


  /**
   * This convenience method takes a string value from a PvlKeyword and copies it into a version 2
   * protobuf field. Once copied, the PvlKeyword is deleted.
   *
   * If the keyword doesn't exist, this does nothing.
   *
   * @param container The PvlContainer representation of the control measure that contains the
   *                  PvlKeyword.
   * @param keyName The name of the keyword to be copied.
   * @param[out] measure The version 2 protobuf representation of the control measure that the
   *                     value will be into.
   * @param setter The protobuf mutator method that sets the value of the field in the protobuf
   *               representation.
   */
  void ControlPointV0003::copy(PvlContainer &container,
                               QString keyName,
                               ControlPointFileEntryV0002_Measure &measure,
                               void (ControlPointFileEntryV0002_Measure::*setter)
                                      (const std::string &)) {

    if (!container.hasKeyword(keyName.toStdString())) {
      return;
    }

    QString value = QString::fromStdString(container[keyName.toStdString()][0]);
    container.deleteKeyword(keyName.toStdString());
    (measure.*setter)(value.toLatin1().data());
  }
}
