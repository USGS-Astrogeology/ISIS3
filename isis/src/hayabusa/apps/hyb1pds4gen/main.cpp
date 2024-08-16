/** This is free and unencumbered software released into the public domain.

The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */

#include "Isis.h"

#include <QDomDocument>
#include <QDomElement>
#include <QString>

#include "Application.h"
#include "Cube.h"
#include "ExportDescription.h"
#include "FileName.h"
#include "Process.h"
#include "ProcessExportPds.h"
#include "ProcessExportPds4.h"
#include "Pvl.h"
#include "PvlKeyword.h"
#include "PvlToXmlTranslationManager.h"

using namespace std;
using namespace Isis;

void generateCSVOutput(Pvl &inputCubeLabel);

void IsisMain() {
  UserInterface &ui = Application::GetUserInterface();

  // Setup the process and set the input cube
  ProcessExportPds4 process;
  Cube *inputCube = process.SetInputCube("FROM");
  Pvl *inputLabel = inputCube->label();
  generateCSVOutput(*inputLabel);
  QString logicalId = ui.GetString("PDS4LOGICALIDENTIFIER");
  process.setLogicalId(logicalId);

  QString translationFile = "$ISISROOT/appdata/translations/";
  PvlGroup instGroup = inputLabel->findObject("IsisCube").findGroup("Instrument");
  if (QString::fromStdString(instGroup["InstrumentId"][0]).compare("NIRS", Qt::CaseInsensitive) == 0) {

    process.setImageType(ProcessExportPds4::BinSetSpectrum);
    QDomDocument &pdsLabel = process.StandardPds4Label();

    translationFile += "HayabusaNirsPds4Export.trn";
    PvlToXmlTranslationManager xlator(*(inputLabel), translationFile);
    xlator.Auto(pdsLabel);

    ProcessExportPds4::translateUnits(pdsLabel);

  }
  else { // AMICA

    QDomDocument &pdsLabel = process.StandardPds4Label();

    translationFile += "HayabusaAmicaPds4Export.trn";
    PvlToXmlTranslationManager xlator(*(inputLabel), translationFile);
    xlator.Auto(pdsLabel);

    /*
     * Add additional pds label data here
     */
    QStringList xmlPath;
    xmlPath << "Product_Observational"
            << "Observation_Area"
            << "Discipline_Area"
            << "img:Imaging"
            << "img:Subframe_Parameters";
    if (instGroup.hasKeyword("FirstLine") && instGroup.hasKeyword("LastLine")) {

      int lines = (int) instGroup["LastLine"] - (int) instGroup["FirstLine"];
      QDomElement baseElement = pdsLabel.documentElement();
      QDomElement subframeParametersElement = process.getElement(xmlPath, baseElement);

      QDomElement linesElement = pdsLabel.createElement("img:lines");
      PvlToXmlTranslationManager::setElementValue(linesElement, toString(lines));
      subframeParametersElement.appendChild(linesElement);

    }

    if (instGroup.hasKeyword("FirstSample") && instGroup.hasKeyword("LastSample")) {
      int samples = (int) instGroup["LastSample"] - (int) instGroup["FirstSample"];
      QDomElement baseElement = pdsLabel.documentElement();
      QDomElement subframeParametersElement = process.getElement(xmlPath, baseElement);

      QDomElement samplesElement = pdsLabel.createElement("img:samples");
      PvlToXmlTranslationManager::setElementValue(samplesElement, toString(samples));
      subframeParametersElement.appendChild(samplesElement);
    }

    double radianceScalingFactor = 1.0;
    if (instGroup.hasKeyword("RadianceScaleFactor")) {
      radianceScalingFactor *= (double)instGroup["RadianceScaleFactor"];
    }
    if (instGroup.hasKeyword("RadianceStandard")) {
      radianceScalingFactor *= (double)instGroup["RadianceStandard"];
    }
    xmlPath[4] = "img:Radiometric_Correction_Parameters";
    QDomElement baseElement = pdsLabel.documentElement();
    QDomElement radiometricParametersElement = process.getElement(xmlPath, baseElement);
    QDomElement radianceFactorElement = pdsLabel.createElement("img:radiance_scaling_factor_WO_units");
    PvlToXmlTranslationManager::setElementValue(radianceFactorElement, toString(radianceScalingFactor));
    radiometricParametersElement.appendChild(radianceFactorElement);

    ProcessExportPds4::translateUnits(pdsLabel);
  }

  QString outFile = ui.GetFileName("TO");
  process.WritePds4(outFile);

  return;
}


/**
 * Write extra values to an output CSV formatted file.
 */
void generateCSVOutput(Pvl &inputCubeLabel) {
  if (inputCubeLabel.findObject("IsisCube").hasGroup("Instrument")) {
    UserInterface &ui = Application::GetUserInterface();
    // Create the vars for holding the info
    QString keys;
    QString values;
    const QString delimeter = ",";

    // Output the result
    fstream outFile;//QFile???
    QString outputImage = ui.GetAsString("TO");
    FileName imgOutput(outputImage);
    FileName csvOutput = imgOutput.removeExtension().setExtension("csv");

    PvlGroup instGroup = inputCubeLabel.findObject("IsisCube").findGroup("Instrument");
    if (QString::fromStdString(instGroup["InstrumentId"][0]).compare("amica", Qt::CaseInsensitive) == 0) {
      if (inputCubeLabel.findObject("IsisCube").hasGroup("RadiometricCalibration")) {
        outFile.open(csvOutput.expanded().toLatin1().data(), std::ios::out);
        PvlGroup radiometricGroup = inputCubeLabel.findObject("IsisCube").findGroup("RadiometricCalibration");
        outFile << "RadiometricCalibrationUnits"
                << delimeter
                << "RadianceStandard"
                << delimeter
                << "RadianceScaleFactor"
                << endl;
        outFile << radiometricGroup["Units"][0]
                << delimeter
                << radiometricGroup["RadianceStandard"][0]
                << delimeter
                << radiometricGroup["RadianceScaleFactor"][0]
                << endl;
        outFile.close();
      }
    }
    else { // NIRS
      outFile.open(csvOutput.expanded().toLatin1().data(), std::ios::out);
      outFile << "IntegrationTime" << endl;
      outFile << instGroup["IntegrationTime"][0] << endl;
      outFile.close();
    }
  }
}
