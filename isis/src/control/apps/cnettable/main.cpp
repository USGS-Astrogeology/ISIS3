/** This is free and unencumbered software released into the public domain.

The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */

#include "Isis.h"

#include <string>

#include <QList>
#include <QString>
#include <QStringList>

#include "CameraPointInfo.h"
#include "ControlMeasure.h"
#include "ControlMeasureLogData.h"
#include "ControlNet.h"
#include "ControlPoint.h"
#include "Displacement.h"
#include "FileName.h"
#include "IException.h"
#include "IString.h"
#include "Latitude.h"
#include "Longitude.h"
#include "Progress.h"
#include "PvlGroup.h"
#include "PvlKeyword.h"
#include "PvlObject.h"
#include "SerialNumberList.h"
#include "Statistics.h"
#include "SurfacePoint.h"
#include "TextFile.h"
#include "UserInterface.h"

using namespace std;
using namespace Isis;

QString CheckValue(double value);
QString CheckValue(QString value);
void Write(PvlGroup *point, const ControlMeasure &cm);

// Allows for column names to be written on the first pass
bool isFirst;
bool append;

TextFile *txt = NULL;

// For control measure related data and labels
QString measureInfo;
QString measureLabels;

void IsisMain() {
  isFirst = true;
  append = false;
  bool outside = false;
  bool errors = false;
  measureInfo = "";
  measureLabels = "";

  Progress prog;

  // Get user entered information
  UserInterface & ui = Application::GetUserInterface();
  ControlNet cnet(ui.GetFileName("CNET"));
  SerialNumberList serials(ui.GetFileName("FROMLIST"));
  append = ui.GetBoolean("APPEND");

  if (cnet.GetNumMeasures() == 0) {
    string msg = "Your control network must contain at least one point";
    throw IException(IException::User, msg, _FILEINFO_);
  }

  prog.SetMaximumSteps(cnet.GetNumMeasures());

  // If append is true, output will be appended or a new file created
  if (append) {
    // Check to see if its a new file or we open an existing file
    FileName file(ui.GetFileName("FLATFILE").toStdString());
    if (!file.fileExists()) {
      // It is new, so we aren't appending
      // Set this because it is used elsewhere
      append = false;
    }
    txt = new TextFile(ui.GetFileName("FLATFILE"), "append");
  }
  // Without append, if the files exists it will be overwritten
  else {
    txt = new TextFile(ui.GetFileName("FLATFILE"), "overwrite");
  }

  PvlGroup * grp = NULL;

  CameraPointInfo camPoint;
  camPoint.SetCSVOutput(true);

  outside = ui.GetBoolean("ALLOWOUTSIDE");
  errors = ui.GetBoolean("ALLOWERRORS");

  // Loop through all points in controlnet
  for (int i = 0; i < cnet.GetNumPoints(); i++) {
    const ControlPoint * cpoint = cnet[i];

    if (isFirst && !append) {
      measureLabels += "ControlPointId,";
      measureLabels += "PointType,";
      measureLabels += "ChooserName,";
      measureLabels += "DateTime,";
      measureLabels += "EditLock,";
      measureLabels += "Ignored,";

      measureLabels += "AprioriSurfacePointSource,";
      measureLabels += "AprioriSurfacePointSourceFile,";

      measureLabels += "AprioriRadiusSource,";
      measureLabels += "AprioriRadiusSourceFile,";

      measureLabels += "AprioriX,";
      measureLabels += "AprioriY,";
      measureLabels += "AprioriZ,";
      measureLabels += "AprioriXSigma,";
      measureLabels += "AprioriYSigma,";
      measureLabels += "AprioriZSigma,";
      measureLabels += "AprioriLatitude,";
      measureLabels += "AprioriLongitude,";
      measureLabels += "AprioriLocalRadius,";
      measureLabels += "AprioriLatitudeSigma,";
      measureLabels += "AprioriLongitudeSigma,";
      measureLabels += "AprioriLocalRadiusSigma,";
      measureLabels += "AprioriLatitudeSigmaDistance,";
      measureLabels += "AprioriLongitudeSigmaDistance,";

      measureLabels += "AdjustedX,";
      measureLabels += "AdjustedY,";
      measureLabels += "AdjustedZ,";
      measureLabels += "AdjustedXSigma,";
      measureLabels += "AdjustedYSigma,";
      measureLabels += "AdjustedZSigma,";
      measureLabels += "AdjustedLatitude,";
      measureLabels += "AdjustedLongitude,";
      measureLabels += "AdjustedLocalRadius,";
      measureLabels += "AdjustedLatitudeSigma,";
      measureLabels += "AdjustedLongitudeSigma,";
      measureLabels += "AdjustedLocalRadiusSigma,";
      measureLabels += "AdjustedLatitudeSigmaDistance,";
      measureLabels += "AdjustedLongitudeSigmaDistance,";

      measureLabels += "MinimumResidual,";
      measureLabels += "MaximumResidual,";
      measureLabels += "AverageResidual,";
      measureLabels += "MinimumSampleResidual,";
      measureLabels += "MaximumSamlpeResidual,";
      measureLabels += "MinimumLineResidual,";
      measureLabels += "MaximumLineResidual,";

      measureLabels += "MaximumSampleShift,";
      measureLabels += "MinimumSampleShift,";
      measureLabels += "AverageSampleShift,";
      measureLabels += "MaximumLineShift,";
      measureLabels += "MinimumLineShift,";
      measureLabels += "AverageLineShift,";
      measureLabels += "MaximumPixelShift,";
      measureLabels += "MinimumPixelShift,";
      measureLabels += "AveragePixelShift,";
      measureLabels += "MinimumPixelZScore,";
      measureLabels += "AveragePixelZScore,";
      measureLabels += "MaximumGoodnessOfFit,";
      measureLabels += "MinimumGoodnessOfFit,";
      measureLabels += "AverageGoodnessOfFit,";
    }

    // Always add data
    measureInfo.clear();
    measureInfo += cpoint->GetId() + ",";
    measureInfo += cpoint->GetPointTypeString() + ",";
    measureInfo += QString(cpoint->GetChooserName()) + ",";
    measureInfo += QString(cpoint->GetDateTime()) + ",";
    measureInfo += QString::number((int)(cpoint->IsEditLocked())) + ",";
    measureInfo += QString::number((int)(cpoint->IsIgnored())) + ",";

    measureInfo += QString(cpoint->GetSurfacePointSourceString()) + ",";
    measureInfo += QString(cpoint->GetAprioriSurfacePointSourceFile()) + ",";
    measureInfo += QString(cpoint->GetRadiusSourceString()) + ",";
    measureInfo += QString(cpoint->GetAprioriRadiusSourceFile()) + ",";

    SurfacePoint Asp = cpoint->GetAprioriSurfacePoint();
    measureInfo += QString(CheckValue(Asp.GetX().kilometers())) + ",";
    measureInfo += QString(CheckValue(Asp.GetY().kilometers())) + ",";
    measureInfo += QString(CheckValue(Asp.GetZ().kilometers())) + ",";
    measureInfo += QString(CheckValue(Asp.GetXSigma().kilometers())) + ",";
    measureInfo += QString(CheckValue(Asp.GetYSigma().kilometers())) + ",";
    measureInfo += QString(CheckValue(Asp.GetZSigma().kilometers())) + ",";
    measureInfo += QString(CheckValue(Asp.GetLatitude().planetocentric(Angle::Degrees))) + ",";
    measureInfo += QString(CheckValue(Asp.GetLongitude().positiveEast(Angle::Degrees))) + ",";
    measureInfo += QString(CheckValue(Asp.GetLocalRadius().kilometers())) + ",";
    measureInfo += QString(CheckValue(Asp.GetLatSigma().degrees())) + ",";
    measureInfo += QString(CheckValue(Asp.GetLonSigma().degrees())) + ",";
    measureInfo += QString(CheckValue(Asp.GetLocalRadiusSigma().kilometers())) + ",";
    try {
      measureInfo += QString(CheckValue(Asp.GetLatSigmaDistance().kilometers())) + ",";
    }
    catch (IException &) {
      measureInfo += ",";
    }
    try {
      measureInfo += QString(CheckValue(Asp.GetLonSigmaDistance().kilometers())) + ",";
    }
    catch (IException &) {
      measureInfo += ",";
    }

    SurfacePoint sp = cpoint->GetAdjustedSurfacePoint();
    measureInfo += QString(CheckValue(sp.GetX().kilometers())) + ",";
    measureInfo += QString(CheckValue(sp.GetY().kilometers())) + ",";
    measureInfo += QString(CheckValue(sp.GetZ().kilometers())) + ",";
    measureInfo += QString(CheckValue(sp.GetXSigma().kilometers())) + ",";
    measureInfo += QString(CheckValue(sp.GetYSigma().kilometers())) + ",";
    measureInfo += QString(CheckValue(sp.GetZSigma().kilometers())) + ",";
    measureInfo += QString(CheckValue(sp.GetLatitude().planetocentric(Angle::Degrees))) + ",";
    measureInfo += QString(CheckValue(sp.GetLongitude().positiveEast(Angle::Degrees))) + ",";
    measureInfo += QString(CheckValue(sp.GetLocalRadius().kilometers())) + ",";
    measureInfo += QString(CheckValue(sp.GetLatSigma().degrees())) + ",";
    measureInfo += QString(CheckValue(sp.GetLonSigma().degrees())) + ",";
    measureInfo += QString(CheckValue(sp.GetLocalRadiusSigma().kilometers())) + ",";
    try {
      measureInfo += QString(CheckValue(sp.GetLatSigmaDistance().kilometers())) + ",";
    }
    catch (IException &e) {
      measureInfo += ",";
    }
    try {
      measureInfo += QString(CheckValue(sp.GetLonSigmaDistance().kilometers())) + ",";
    }
    catch (IException &e) {
      measureInfo += ",";
    }

    measureInfo += QString(CheckValue(cpoint->GetStatistic(
        &ControlMeasure::GetResidualMagnitude).Minimum())) + ",";
    measureInfo += QString(CheckValue(cpoint->GetStatistic(
        &ControlMeasure::GetResidualMagnitude).Maximum())) + ",";
    measureInfo += QString(CheckValue(cpoint->GetStatistic(
        &ControlMeasure::GetResidualMagnitude).Average())) + ",";
    measureInfo += QString(CheckValue(cpoint->GetStatistic(
        &ControlMeasure::GetSampleResidual).Minimum())) + ",";
    measureInfo += QString(CheckValue(cpoint->GetStatistic(
        &ControlMeasure::GetSampleResidual).Maximum())) + ",";
    measureInfo += QString(CheckValue(cpoint->GetStatistic(
        &ControlMeasure::GetLineResidual).Minimum())) + ",";
    measureInfo += QString(CheckValue(cpoint->GetStatistic(
        &ControlMeasure::GetLineResidual).Maximum())) + ",";


    measureInfo += QString(CheckValue(cpoint->GetStatistic(
        &ControlMeasure::GetSampleShift).Maximum())) + ",";
    measureInfo += QString(CheckValue(cpoint->GetStatistic(
        &ControlMeasure::GetSampleShift).Minimum())) + ",";
    measureInfo += QString(CheckValue(cpoint->GetStatistic(
        &ControlMeasure::GetSampleShift).Average())) + ",";
    measureInfo += QString(CheckValue(cpoint->GetStatistic(
        &ControlMeasure::GetLineShift).Maximum())) + ",";
    measureInfo += QString(CheckValue(cpoint->GetStatistic(
        &ControlMeasure::GetLineShift).Minimum())) + ",";
    measureInfo += QString(CheckValue(cpoint->GetStatistic(
        &ControlMeasure::GetLineShift).Average())) + ",";

    measureInfo += QString(CheckValue(cpoint->GetStatistic(
        &ControlMeasure::GetPixelShift).Maximum())) + ",";
    measureInfo += QString(CheckValue(cpoint->GetStatistic(
        &ControlMeasure::GetPixelShift).Minimum())) + ",";
    measureInfo += QString(CheckValue(cpoint->GetStatistic(
        &ControlMeasure::GetPixelShift).Average())) + ",";

    measureInfo += QString(CheckValue(cpoint->GetStatistic(
        ControlMeasureLogData::MaximumPixelZScore).Maximum())) + ",";
    measureInfo += QString(CheckValue(cpoint->GetStatistic(
        ControlMeasureLogData::MinimumPixelZScore).Minimum())) + ",";
    measureInfo += QString(CheckValue(cpoint->GetStatistic(
        ControlMeasureLogData::GoodnessOfFit).Maximum())) + ",";
    measureInfo += QString(CheckValue(cpoint->GetStatistic(
        ControlMeasureLogData::GoodnessOfFit).Minimum())) + ",";
    measureInfo += QString(CheckValue(cpoint->GetStatistic(
        ControlMeasureLogData::GoodnessOfFit).Average())) + ",";

    // Loop through all measures in controlpoint
    for (int j = 0; j < cpoint->GetNumMeasures(); j++) {

      const ControlMeasure * cmeasure = (*cpoint)[j];

      // Set and then get CameraPointInfo information
      camPoint.SetCube(serials.fileName(cmeasure->GetCubeSerialNumber()));

      grp = camPoint.SetImage(cmeasure->GetSample(), cmeasure->GetLine(), outside, errors);
      // Shouldn't ever happen, but, being safe...
      if (grp == NULL) {
        string msg = "You shouldn't have gotten here. Errors in CameraPointInfo class";
        throw IException(IException::Programmer, msg, _FILEINFO_);
      }
      Write(grp, *cmeasure);
      delete grp;
      grp = NULL;
    }

    // Making progress!
    prog.CheckStatus();
  }

  // All done, clean up
  prog.CheckStatus();
  if (txt != NULL) {
    delete txt;
    txt = NULL;
  }
}

// This function is meant to check a value, and, if it is a special pixel
// return " " instead of the special pixel value.
QString CheckValue(double value) {
  if (IsSpecial(value)) {
    return QString("");
  }
  else {
    return CheckValue(QString::number(value));
  }
}

QString CheckValue(QString value) {
  if (value == QString::number(Isis::Null) ||
      value == QString::number(Isis::Hrs) ||
      value == QString::number(Isis::His) ||
      value == QString::number(Isis::Lrs) ||
      value == QString::number(Isis::Lis) ||
      value == QString::number(Isis::Null)) {
    return QString("");
  }
  else {
    return value;
  }
}

// Write each PvlGroup out to file
void Write(PvlGroup *point, const ControlMeasure &cm) {

  // QStrings are used QString here because of ControlMeasure returning
  // QStrings. There is some monkey motion involving ingesting doubles,
  // this is necessary as IString will accept doubles but QString won't.

  QString output = "";
  QVector < IString > dataNames;

  // Do we have errors?
  int maxCount = 0;
  bool errors = point->hasKeyword("Error");
  if (errors) {
    maxCount = point->keywords() - 1;
  }
  else {
    maxCount = point->keywords();
  }

  // If its first and not appending, write the column labels
  if (isFirst && !append) {

    QList< QStringList > printableMeasureData = cm.PrintableClassData();
    QStringList nameValuePair;
    foreach (nameValuePair, printableMeasureData) {
      output += nameValuePair.at(0) + ",";
    }

    // point information
    for (int i = 0; i < maxCount; i++) {
      if ((*point)[i].size() == 3) {
        output += QString::fromStdString((*point)[i].name()) + "X,";
        output += QString::fromStdString((*point)[i].name()) + "Y,";
        output += QString::fromStdString((*point)[i].name()) + "Z,";
      }
      else {
        output += QString::fromStdString((*point)[i].name()) + ",";
      }
    }

    if (errors) output += QString::fromStdString((*point)[maxCount].name());
    isFirst = false;
    measureLabels += output;
    // In some cases, we need to trim a trailing comma:
    if (measureLabels.endsWith(",")) {
      measureLabels.chop(1);
    }
    txt->PutLine(measureLabels);
  }
  output.clear();
  measureLabels.clear();

  QList< QStringList > printableMeasureData = cm.PrintableClassData();
  QStringList nameValuePair;
  foreach (nameValuePair, printableMeasureData) {
    output += CheckValue(nameValuePair.at(1)) + ",";
  }

  // Write out date values
  // point information
  for (int i = 0; i < maxCount; i++) {
    if ((*point)[i].size() == 3) {
      output += QString(CheckValue(QString::fromStdString((*point)[i][0]))) + ",";
      output += QString(CheckValue(QString::fromStdString((*point)[i][1]))) + ",";
      output += QString(CheckValue(QString::fromStdString((*point)[i][2]))) + ",";
    }
    else {
      output += QString(CheckValue(QString::fromStdString((*point)[i][0]))) + ",";
    }
  }

  if (errors) output += QString::fromStdString((*point)[maxCount][0]);

  // Measure info comes first
  QString pri = "";
  pri += measureInfo;
  pri += output;

  // In some cases, we need to trim a trailing comma:
  if (pri.endsWith(",")) {
    pri.chop(1);
  }
  txt->PutLine(pri);

  output.clear();
  pri.clear();
}
