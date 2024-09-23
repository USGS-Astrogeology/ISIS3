/** This is free and unencumbered software released into the public domain.

The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */

#include "ControlPointV0001.h"

#include <QString>

#include "ControlMeasureLogData.h"
#include "Distance.h"
#include "IException.h"
#include "Latitude.h"
#include "Longitude.h"
#include "NaifStatus.h"
#include "Pvl.h"
#include "PvlContainer.h"
#include "SpecialPixel.h"
#include "SurfacePoint.h"
#include "Target.h"

using namespace std;

namespace Isis {

  /**
   * Create a ControlPointV0001 object from a protobuf version 1 control point message.
   *
   * @param pointData The protobuf message from a control net file.
   * @param logData The accompanying protobuf control measure log data for the point.
   */
  ControlPointV0001::ControlPointV0001(
          QSharedPointer<ControlNetFileProtoV0001_PBControlPoint> pointData,
          QSharedPointer<ControlNetLogDataProtoV0001_Point> logData)
      : m_pointData(pointData), m_logData(logData) {

   }


  /**
   * Create a ControlPointV0001 object from a version 1 control point Pvl object
   *
   * @param pointObject The control point and its measures in a Pvl object
   * @param targetName The name of the target
   */
  ControlPointV0001::ControlPointV0001(PvlObject &pointObject, const QString targetName)
      : m_pointData(new ControlNetFileProtoV0001_PBControlPoint),
        m_logData(new ControlNetLogDataProtoV0001_Point) {
    // Clean up the Pvl control point
    // Anything that doesn't have a value is removed
    for (int cpKeyIndex = 0; cpKeyIndex < pointObject.keywords(); cpKeyIndex ++) {
      if (pointObject[cpKeyIndex][0] == "") {
        pointObject.deleteKeyword(cpKeyIndex);
      }
    }

    // Copy over POD values
    copy(pointObject, "PointId",
         m_pointData, &ControlNetFileProtoV0001_PBControlPoint::set_id);
    copy(pointObject, "ChooserName",
         m_pointData, &ControlNetFileProtoV0001_PBControlPoint::set_choosername);
    copy(pointObject, "DateTime",
         m_pointData, &ControlNetFileProtoV0001_PBControlPoint::set_datetime);
    copy(pointObject, "AprioriXYZSourceFile",
         m_pointData, &ControlNetFileProtoV0001_PBControlPoint::set_apriorisurfpointsourcefile);
    copy(pointObject, "AprioriLatLonSourceFile",
         m_pointData, &ControlNetFileProtoV0001_PBControlPoint::set_apriorisurfpointsourcefile);
    copy(pointObject, "AprioriRadiusSourceFile",
         m_pointData, &ControlNetFileProtoV0001_PBControlPoint::set_aprioriradiussourcefile);
    copy(pointObject, "JigsawRejected",
         m_pointData, &ControlNetFileProtoV0001_PBControlPoint::set_jigsawrejected);
    copy(pointObject, "EditLock",
         m_pointData, &ControlNetFileProtoV0001_PBControlPoint::set_editlock);
    copy(pointObject, "Ignore",
         m_pointData, &ControlNetFileProtoV0001_PBControlPoint::set_ignore);
    copy(pointObject, "LatitudeConstrained",
         m_pointData, &ControlNetFileProtoV0001_PBControlPoint::set_latitudeconstrained);
    copy(pointObject, "LongitudeConstrained",
         m_pointData, &ControlNetFileProtoV0001_PBControlPoint::set_longitudeconstrained);
    copy(pointObject, "RadiusConstrained",
         m_pointData, &ControlNetFileProtoV0001_PBControlPoint::set_radiusconstrained);
    // Copy over the adjusted surface point
    if ( pointObject.hasKeyword("Latitude")
         && pointObject.hasKeyword("Longitude")
         && pointObject.hasKeyword("Radius") ) {
      SurfacePoint adjustedPoint(
          Latitude(Isis::toDouble(pointObject["Latitude"][0]), Angle::Degrees),
          Longitude(Isis::toDouble(pointObject["Longitude"][0]), Angle::Degrees),
          Distance(Isis::toDouble(pointObject["Radius"][0]), Distance::Meters));

      m_pointData->set_adjustedx( adjustedPoint.GetX().meters() );
      m_pointData->set_adjustedy( adjustedPoint.GetY().meters() );
      m_pointData->set_adjustedz( adjustedPoint.GetZ().meters() );
    }
    else if ( pointObject.hasKeyword("X")
              && pointObject.hasKeyword("Y")
              && pointObject.hasKeyword("Z") ) {
      m_pointData->set_adjustedx( Isis::toDouble(pointObject["X"][0]) );
      m_pointData->set_adjustedy( Isis::toDouble(pointObject["Y"][0]) );
      m_pointData->set_adjustedz( Isis::toDouble(pointObject["Z"][0]) );
    }

    // copy over the apriori surface point
    if ( pointObject.hasKeyword("AprioriLatitude")
         && pointObject.hasKeyword("AprioriLongitude")
         && pointObject.hasKeyword("AprioriRadius") ) {
      SurfacePoint aprioriPoint(
          Latitude(Isis::toDouble(pointObject["AprioriLatitude"][0]), Angle::Degrees),
          Longitude(Isis::toDouble(pointObject["AprioriLongitude"][0]), Angle::Degrees),
          Distance(Isis::toDouble(pointObject["AprioriRadius"][0]), Distance::Meters));

      m_pointData->set_apriorix( aprioriPoint.GetX().meters() );
      m_pointData->set_aprioriy( aprioriPoint.GetY().meters() );
      m_pointData->set_aprioriz( aprioriPoint.GetZ().meters() );
    }
    else if ( pointObject.hasKeyword("AprioriX")
              && pointObject.hasKeyword("AprioriY")
              && pointObject.hasKeyword("AprioriZ") ) {
      m_pointData->set_apriorix( Isis::toDouble(pointObject["AprioriX"][0]) );
      m_pointData->set_aprioriy( Isis::toDouble(pointObject["AprioriY"][0]) );
      m_pointData->set_aprioriz( Isis::toDouble(pointObject["AprioriZ"][0]) );
    }
    // If the apriori values are missing, copy them from the adjusted.
    else if ( m_pointData->has_adjustedx()
              && m_pointData->has_adjustedy()
              && m_pointData->has_adjustedz() ) {
      m_pointData->set_apriorix( m_pointData->adjustedx() );
      m_pointData->set_aprioriy( m_pointData->adjustedy() );
      m_pointData->set_aprioriz( m_pointData->adjustedz() );
    }

    // Ground points were previously flagged by the Held keyword being true.
    if ( (pointObject.hasKeyword("Held") && pointObject["Held"][0] == "True")
         || (pointObject["PointType"][0] == "Ground") ) {
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

    if (pointObject.hasKeyword("AprioriLatLonSource")) {
      QString source = QString::fromStdString(pointObject["AprioriLatLonSource"][0]);

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
        std::string msg = "Invalid AprioriLatLonSource [" + source.toStdString() + "]";
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

    // Copy the covariance matrices
    // Sometimes they are not stored in version 1 Pvls so we compute them from the
    // surface point sigmas using the local radius to convert to/from angular units.

    // Add the Apriori Covariance Matrix
    if ( pointObject.hasKeyword("AprioriCovarianceMatrix") ) {
      PvlKeyword &matrix = pointObject["AprioriCovarianceMatrix"];

      m_pointData->add_aprioricovar(Isis::toDouble(matrix[0]));
      m_pointData->add_aprioricovar(Isis::toDouble(matrix[1]));
      m_pointData->add_aprioricovar(Isis::toDouble(matrix[2]));
      m_pointData->add_aprioricovar(Isis::toDouble(matrix[3]));
      m_pointData->add_aprioricovar(Isis::toDouble(matrix[4]));
      m_pointData->add_aprioricovar(Isis::toDouble(matrix[5]));

      m_pointData->set_latitudeconstrained(true);
      m_pointData->set_longitudeconstrained(true);
      m_pointData->set_radiusconstrained(true);
    }
    else if ( pointObject.hasKeyword("AprioriSigmaLatitude")
              || pointObject.hasKeyword("AprioriSigmaLongitude")
              || pointObject.hasKeyword("AprioriSigmaRadius") ) {
      // There may be missing or negative apriori sigmas so default to 10,000
      double sigmaLat = 10000.0;
      double sigmaLon = 10000.0;
      double sigmaRad = 10000.0;

      if ( pointObject.hasKeyword("AprioriSigmaLatitude") ) {
        if (Isis::toDouble(pointObject["AprioriSigmaLatitude"][0]) > 0
            && Isis::toDouble(pointObject["AprioriSigmaLatitude"][0]) < sigmaLat) {
          sigmaLat = Isis::toDouble(pointObject["AprioriSigmaLatitude"][0]);
        }
        m_pointData->set_latitudeconstrained(true);
      }

      if ( pointObject.hasKeyword("AprioriSigmaLongitude") ) {
        if (Isis::toDouble(pointObject["AprioriSigmaLongitude"][0]) > 0
            && Isis::toDouble(pointObject["AprioriSigmaLongitude"][0]) < sigmaLon) {
          sigmaLon = Isis::toDouble(pointObject["AprioriSigmaLongitude"][0]);
        }
        m_pointData->set_longitudeconstrained(true);
      }

      if ( pointObject.hasKeyword("AprioriSigmaRadius") ) {
        if (Isis::toDouble(pointObject["AprioriSigmaRadius"][0]) > 0
            && Isis::toDouble(pointObject["AprioriSigmaRadius"][0]) < sigmaRad) {
          sigmaRad = Isis::toDouble(pointObject["AprioriSigmaRadius"][0]);
        }
        m_pointData->set_radiusconstrained(true);
      }

      SurfacePoint aprioriPoint;
      aprioriPoint.SetRectangular( Displacement(m_pointData->apriorix(), Displacement::Meters),
                                   Displacement(m_pointData->aprioriy(), Displacement::Meters),
                                   Displacement(m_pointData->aprioriz(), Displacement::Meters) );
      aprioriPoint.SetSphericalSigmasDistance( Distance(sigmaLat, Distance::Meters),
                                               Distance(sigmaLon, Distance::Meters),
                                               Distance(sigmaRad, Distance::Meters) );
      m_pointData->add_aprioricovar( aprioriPoint.GetRectangularMatrix()(0, 0) );
      m_pointData->add_aprioricovar( aprioriPoint.GetRectangularMatrix()(0, 1) );
      m_pointData->add_aprioricovar( aprioriPoint.GetRectangularMatrix()(0, 2) );
      m_pointData->add_aprioricovar( aprioriPoint.GetRectangularMatrix()(1, 1) );
      m_pointData->add_aprioricovar( aprioriPoint.GetRectangularMatrix()(1, 2) );
      m_pointData->add_aprioricovar( aprioriPoint.GetRectangularMatrix()(2, 2) );
    }

    // Add the Adjusted (Apost) Covariance Matrix
    if ( pointObject.hasKeyword("ApostCovarianceMatrix") ) {
      PvlKeyword &matrix = pointObject["ApostCovarianceMatrix"];

      m_pointData->add_adjustedcovar(Isis::toDouble(matrix[0]));
      m_pointData->add_adjustedcovar(Isis::toDouble(matrix[1]));
      m_pointData->add_adjustedcovar(Isis::toDouble(matrix[2]));
      m_pointData->add_adjustedcovar(Isis::toDouble(matrix[3]));
      m_pointData->add_adjustedcovar(Isis::toDouble(matrix[4]));
      m_pointData->add_adjustedcovar(Isis::toDouble(matrix[5]));

      m_pointData->set_latitudeconstrained(true);
      m_pointData->set_longitudeconstrained(true);
      m_pointData->set_radiusconstrained(true);
    }
    else if ( pointObject.hasKeyword("AdjustedSigmaLatitude")
          || pointObject.hasKeyword("AdjustedSigmaLongitude")
          || pointObject.hasKeyword("AdjustedSigmaRadius") ) {
      // There may be missing or negative adjusted sigmas so default to 10,000
      double sigmaLat = 10000.0;
      double sigmaLon = 10000.0;
      double sigmaRad = 10000.0;

      if ( pointObject.hasKeyword("AdjustedSigmaLatitude") ) {
        if (Isis::toDouble(pointObject["AdjustedSigmaLatitude"][0]) > 0
            && Isis::toDouble(pointObject["AdjustedSigmaLatitude"][0]) < sigmaLat) {
          sigmaLat = Isis::toDouble(pointObject["AdjustedSigmaLatitude"][0]);
        }
      }

      if ( pointObject.hasKeyword("AdjustedSigmaLongitude") ) {
        if (Isis::toDouble(pointObject["AdjustedSigmaLongitude"][0]) > 0
            && Isis::toDouble(pointObject["AdjustedSigmaLongitude"][0]) < sigmaLon) {
          sigmaLon = Isis::toDouble(pointObject["AdjustedSigmaLongitude"][0]);
        }
      }

      if ( pointObject.hasKeyword("AdjustedSigmaRadius") ) {
        if (Isis::toDouble(pointObject["AdjustedSigmaRadius"][0]) > 0
            && Isis::toDouble(pointObject["AdjustedSigmaRadius"][0]) < sigmaRad) {
          sigmaRad = Isis::toDouble(pointObject["AdjustedSigmaRadius"][0]);
        }
      }

      SurfacePoint adjustedPoint;
      adjustedPoint.SetRectangular( Displacement(m_pointData->adjustedx(), Displacement::Meters),
                                    Displacement(m_pointData->adjustedy(), Displacement::Meters),
                                    Displacement(m_pointData->adjustedz(), Displacement::Meters) );
      adjustedPoint.SetSphericalSigmasDistance( Distance(sigmaLat, Distance::Meters),
                                                Distance(sigmaLon, Distance::Meters),
                                                Distance(sigmaRad, Distance::Meters) );
      m_pointData->add_adjustedcovar( adjustedPoint.GetRectangularMatrix()(0, 0) );
      m_pointData->add_adjustedcovar( adjustedPoint.GetRectangularMatrix()(0, 1) );
      m_pointData->add_adjustedcovar( adjustedPoint.GetRectangularMatrix()(0, 2) );
      m_pointData->add_adjustedcovar( adjustedPoint.GetRectangularMatrix()(1, 1) );
      m_pointData->add_adjustedcovar( adjustedPoint.GetRectangularMatrix()(1, 2) );
      m_pointData->add_adjustedcovar( adjustedPoint.GetRectangularMatrix()(2, 2) );
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
        // The sample may not be a numeric value
        // in this case set it to 0 and ignore the measure
        double value;
        try {
          value = Isis::toDouble(group["Sample"][0]);
        }
        catch (...) {
          value = 0;
          measure.set_ignore(true);
        }
        measure.mutable_measurement()->set_sample(value);
        group.deleteKeyword("Sample");
      }
      if (group.hasKeyword("Line")) {
        // The line may not be a numeric value
        // in this case set it to 0 and ignore the measure
        double value;
        try {
          value = Isis::toDouble(group["Line"][0]);
        }
        catch (...) {
          value = 0;
          measure.set_ignore(true);
        }
        measure.mutable_measurement()->set_line(value);
        group.deleteKeyword("Line");
      }

      // Some old networks use ErrorSample and ErrorLine,
      // others use SampleResidual and LineResidual so check for both
      if (group.hasKeyword("ErrorSample")) {
        double value = Isis::toDouble(group["ErrorSample"][0]);
        measure.mutable_measurement()->set_sampleresidual(value);
        group.deleteKeyword("ErrorSample");
      }
      if (group.hasKeyword("ErrorLine")) {
        double value = Isis::toDouble(group["ErrorLine"][0]);
        measure.mutable_measurement()->set_lineresidual(value);
        group.deleteKeyword("ErrorLine");
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

      // Copy the measure type
      if (group.hasKeyword("MeasureType")) {
        QString type = QString::fromStdString(group["MeasureType"][0]).toLower();
        if (type == "estimated"
            || type == "unmeasured"
            || type == "candidate") {
          measure.set_type(ControlNetFileProtoV0001_PBControlPoint_PBControlMeasure::Candidate);
        }
        else if (type == "manual") {
          measure.set_type(ControlNetFileProtoV0001_PBControlPoint_PBControlMeasure::Manual);
        }
        else if (type == "automatic"
                 || type == "validatedmanual"
                 || type == "automaticpixel") {
          measure.set_type(ControlNetFileProtoV0001_PBControlPoint_PBControlMeasure::RegisteredPixel);
        }
        else if (type == "validatedautomatic"
                 || type == "automaticsubpixel") {
          measure.set_type(ControlNetFileProtoV0001_PBControlPoint_PBControlMeasure::RegisteredSubPixel);
        }
        else {
          throw IException(IException::Io,
                           "Unknown measure type [" + type.toStdString() + "]",
                           _FILEINFO_);
        }
        group.deleteKeyword("MeasureType");
      }

      // Clean up the remaining keywords
      for (int cmKeyIndex = 0; cmKeyIndex < group.keywords(); cmKeyIndex++) {
        if (group[cmKeyIndex][0] == ""
            || group[cmKeyIndex].name() == "ZScore"
            || group[cmKeyIndex].name() == "ErrorMagnitude") {
          group.deleteKeyword(cmKeyIndex);
          cmKeyIndex--;
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
   * Access the protobuf control point data.
   *
   * @return @b QSharedPointer<ControlNetFileProtoV0001_PBControlPoint> A pointer to the internal
   *                                                                    control point data.
   */
  QSharedPointer<ControlNetFileProtoV0001_PBControlPoint> ControlPointV0001::pointData() {
    return m_pointData;
  }


  /**
   * Access the protobuf log data for the control measures in the point.
   *
   * @return @b QSharedPointer<ControlNetLogDataProtoV0001_Point> A pointer to the internal
   *                                                              measure log data.
   */
  QSharedPointer<ControlNetLogDataProtoV0001_Point> ControlPointV0001::logData() {
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
  void ControlPointV0001::copy(PvlContainer &container,
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
  void ControlPointV0001::copy(PvlContainer &container,
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
  void ControlPointV0001::copy(PvlContainer &container,
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
  void ControlPointV0001::copy(PvlContainer &container,
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
  void ControlPointV0001::copy(PvlContainer &container,
                               QString keyName,
                               ControlNetFileProtoV0001_PBControlPoint_PBControlMeasure &measure,
                               void (ControlNetFileProtoV0001_PBControlPoint_PBControlMeasure::*setter)(double)) {

    if (!container.hasKeyword(keyName.toStdString())) {
      return;
    }

    double value = Isis::Null;
    if ( container.hasKeyword(keyName.toStdString()) ) {
      value = Isis::toDouble(container[keyName.toStdString()][0]);
      container.deleteKeyword(keyName.toStdString());

    }

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
  void ControlPointV0001::copy(PvlContainer &container,
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
