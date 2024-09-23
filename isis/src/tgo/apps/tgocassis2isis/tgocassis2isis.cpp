/** This is free and unencumbered software released into the public domain.

The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */

#include "tgocassis2isis.h"

#include <QString>
#include <QtMath>
#include <QFile>

#include "AlphaCube.h"
#include "Cube.h"
#include "FileName.h"
#include "IException.h"
#include "iTime.h"
#include "LineManager.h"
#include "OriginalXmlLabel.h"
#include "Preference.h"
#include "ProcessImport.h"
#include "PvlTranslationTable.h"
#include "XmlToPvlTranslationManager.h"

using namespace std;

namespace Isis {
  static void translateCoreInfo(FileName &inputLabel, ProcessImport &importer);
  static void translateCoreInfo(XmlToPvlTranslationManager labelXlater, ProcessImport &importer);
  static bool translateMappingLabel(FileName inputLabel, Cube *outputCube);
  static bool translateMosaicLabel(FileName inputLabel, Cube *outputCube);
  static void translateLabels(FileName &inputLabel, Cube *outputCube, QString transFile);
  static QString convertUniqueIdToObservationId(Pvl &outputLabel);

  void tgocassis2isis(UserInterface &ui) {
    FileName xmlFileName = ui.GetFileName("FROM").toStdString();

    if (!xmlFileName.removeExtension().addExtension("dat").fileExists() && !xmlFileName.removeExtension().addExtension("img").fileExists()) {
        std::string msg = "Cannot find image file for [" + xmlFileName.name() + "]. Confirm that the "
        ".dat or .img file for this XML exists and is located in the same directory.";
      throw IException(IException::User, msg, _FILEINFO_);
    }
    if(QString::fromStdString(xmlFileName.name()).contains("cas_raw_sc")){
        std::string msg = "tgocassis2isis is unable to process cas_raw_sc data. Please use cas_cal_sc data instead.";
      throw IException(IException::User, msg, _FILEINFO_);
    }

    try {
      ProcessImport importer;
      translateCoreInfo(xmlFileName, importer);

      if(xmlFileName.removeExtension().addExtension("dat").fileExists()){
        importer.SetInputFile(QString::fromStdString(xmlFileName.removeExtension().addExtension("dat").expanded()));
      }
      else if (xmlFileName.removeExtension().addExtension("img").fileExists()) {
        importer.SetInputFile(QString::fromStdString(xmlFileName.removeExtension().addExtension("img").expanded()));
      }
      else {
        std::string msg = "Cannot find image file for [" + xmlFileName.name() + "]. Confirm that the "
          ".dat or .img file for this XML exists and is located in the same directory.";
        throw IException(IException::User, msg, _FILEINFO_);
      }

      CubeAttributeOutput &att = ui.GetOutputAttribute("TO");
      Cube *outputCube = importer.SetOutputCube(ui.GetCubeName("TO"), att);

      QString transRawFile = "TgoCassisInstrument.trn";
      QFile xmlFile(QString::fromStdString(xmlFileName.expanded()));
      QDomDocument xmlDoc;
      xmlDoc.setContent(&xmlFile, true);
      // If any instances of "Optical_Filter" or "Mission_Area" exist, use PSA .trn file
      QString transExportFile;
      if (!xmlDoc.elementsByTagName("Optical_Filter").isEmpty() &&
          !xmlDoc.elementsByTagName("Cassis_Data").isEmpty()) {
        transExportFile = "TgoCassisExportedInstrument_PSA_Optical_Filter.trn";
      }
      else if (!xmlDoc.elementsByTagName("Cassis_Data").isEmpty()) {
        transExportFile = "TgoCassisExportedInstrument_PSA.trn";
      }
      else {
        transExportFile = "TgoCassisExportedInstrument.trn";
      }
      // first assume lev1b image
      Pvl *outputLabel = outputCube->label();
      QString target = "";
      try {
        translateLabels(xmlFileName, outputCube, transRawFile);
      }
      catch (IException &e) {
        if (translateMappingLabel(xmlFileName, outputCube)) {
          if (!translateMosaicLabel(xmlFileName, outputCube)) {
            translateLabels(xmlFileName, outputCube, transExportFile);
          }
          else {
            if(outputLabel->findObject("IsisCube").hasGroup("Instrument")) {
              outputLabel->findObject("IsisCube").deleteGroup("Instrument");
            }
          }
        }
        else {
          if(outputLabel->findObject("IsisCube").hasGroup("Mapping")) {
            outputLabel->findObject("IsisCube").deleteGroup("Mapping");
          }
          translateLabels(xmlFileName, outputCube, transExportFile);
        }
      }

      if (!outputCube->group("Archive").hasKeyword("ObservationId")){
        convertUniqueIdToObservationId(*outputLabel);
      }

      FileName outputCubeFileName(ui.GetCubeName("TO").toStdString());

      OriginalXmlLabel xmlLabel;
      xmlLabel.readFromXmlFile(xmlFileName);

      importer.StartProcess();

      // Write out original label before closing the cube
      outputCube->write(xmlLabel);

      importer.EndProcess();
    }
    catch (IException &e) {

      std::string msg = "Given file [" + xmlFileName.expanded() + "] does not appear to be a valid TGO CaSSIS label.";
        throw IException(e, IException::User, msg, _FILEINFO_);
    }

    return;
  }


  /**
   * Translate core info from labels and set ProcessImport object with
   * these values.
   *
   * @param inputLabel Reference to the xml label file name from the input image.
   * @param importer Reference to the ProcessImport object to which core info will
   *                 be set.
   *
   * @internal
   *   @history 2017-01-20 Jeannie Backer - Original Version
   */
  void translateCoreInfo(FileName &inputLabel, ProcessImport &importer) {
    // Get the directory where the Tgo translation tables are
    QString missionDir = "$ISISROOT/appdata/translations/";

    // Get the translation manager ready
    FileName transFile;
    try {
      transFile = FileName(missionDir.toStdString() + "TgoCassis.trn");
      XmlToPvlTranslationManager labelXlater(inputLabel, QString::fromStdString(transFile.expanded()));
      translateCoreInfo(labelXlater, importer);
    }
    catch (IException &e) {
      // if exported, use this!
      transFile = FileName(missionDir.toStdString() + "TgoCassisRdr.trn");
      XmlToPvlTranslationManager labelXlater(inputLabel, QString::fromStdString(transFile.expanded()));
      translateCoreInfo(labelXlater, importer);
    }
  }


  /**
   * Translate core info from labels and set ProcessImport object with
   * these values.
   *
   * @param labelXlater Reference to the XmlToPvlTranslationManager objcet to use for the translation.
   * @param importer Reference to the ProcessImport object to which core info will
   *                 be set.
   *
   * @internal
   *   @history 2017-01-20 Jeannie Backer - Original Version
   *   @history 2017-01-21 Krisitn Berry - Flipped ns & nl. They're flipped in the CaSSIS header.
   */
  void translateCoreInfo(XmlToPvlTranslationManager labelXlater, ProcessImport &importer) {
    // Set up the ProcessImport
    QString str;
    str = labelXlater.Translate("CoreSamples");
    int ns = str.toInt();
    str = labelXlater.Translate("CoreLines");
    int nl = str.toInt();
    str = labelXlater.Translate("CoreBands");
    int nb = str.toInt();
    importer.SetDimensions(ns, nl, nb);

    str = labelXlater.Translate("CoreType");
    importer.SetPixelType(PixelTypeEnumeration(str));

    str = labelXlater.Translate("CoreByteOrder");
    importer.SetByteOrder(ByteOrderEnumeration(str.toStdString()));

    importer.SetFileHeaderBytes(0);

    str = labelXlater.Translate("CoreBase");
    importer.SetBase(str.toDouble());
    str = labelXlater.Translate("CoreMultiplier");
    importer.SetMultiplier(str.toDouble());
  }


  /**
   * Translate the cartographic info from the xml.
   *
   * @param xmlFileName The xml label file name for the input image.
   * @param outputCube Pointer to output cube where ISIS labels will be added and
   *                   updated.
   */
  bool translateMappingLabel(FileName xmlFileName, Cube *outputCube) {
    //Translate the Mapping Group
    try {
      QString missionDir = "$ISISROOT/appdata/translations/";
      QDomDocument xmlDoc;
      QFile xmlFile(QString::fromStdString(xmlFileName.expanded()));
      xmlDoc.setContent(&xmlFile, true);
      // If any instances of "Observing_System_Component" exist, use PSA .trn file
      FileName mapTransFile;
      if (xmlDoc.elementsByTagName("cart:a_axis_radius").size()){
        mapTransFile = FileName(missionDir.toStdString() + "TgoCassisMapping_PSA.trn");
      } else {
        mapTransFile = FileName(missionDir.toStdString() + "TgoCassisMapping.trn");
      }
      // Get the translation manager ready for translating the mapping label

      XmlToPvlTranslationManager labelXMappinglater(xmlFileName, QString::fromStdString(mapTransFile.expanded()));

      // Pvl output label
      Pvl *outputLabel = outputCube->label();
      labelXMappinglater.Auto(*(outputLabel));
    }
    catch (IException &e) {
      Pvl *outputLabel = outputCube->label();
      if(outputLabel->hasGroup("Mapping")) {
        outputLabel->deleteGroup("Mapping");
      }
      return false;
    }
    return true;
  }


  /**
   * Translate the Mosaic group info from the xml.
   *
   * @param xmlFileName The xml label file name for the input image.
   * @param outputCube Pointer to output cube where ISIS labels will be added and
   *                   updated.
   */
  bool translateMosaicLabel(FileName xmlFileName, Cube *outputCube) {
    QDomDocument xmlDoc;

    QFile xmlFile(QString::fromStdString(xmlFileName.expanded()));
    if ( !xmlFile.open(QIODevice::ReadOnly) ) {
      std::string msg = "Could not open label file [" + xmlFileName.expanded() +
                    "].";
      throw IException(IException::Unknown, msg, _FILEINFO_);
    }

    QString errmsg;
    int errline, errcol;
    if ( !xmlDoc.setContent(&xmlFile, false, &errmsg, &errline, &errcol) ) {
      xmlFile.close();
      std::string msg = "XML read/parse error in file [" + xmlFileName.expanded()
          + "] at line [" + toString(errline) + "], column [" + toString(errcol)
          + "], message: " + errmsg.toStdString();
      throw IException(IException::Unknown, msg, _FILEINFO_);
    }

    xmlFile.close();

    QDomElement inputParentElement = xmlDoc.documentElement();
    if (!inputParentElement.isNull()) {
      inputParentElement = inputParentElement.firstChildElement("Identification_Area");
      if (!inputParentElement.isNull()) {
        QDomElement logicalId = inputParentElement.firstChildElement("logical_identifier");
        if (!logicalId.isNull()) {
          QString logicalIdText = logicalId.text();
          QStringList logicalIdStringList = logicalIdText.split(":");
          if (logicalIdStringList.contains("data_mosaic")) {
            try {
              QString missionDir = "$ISISROOT/appdata/translations";
              FileName bandBinTransFile(missionDir.toStdString() + "/translations/TgoCassisMosaicBandBin.trn");
              // Get the translation manager ready for translating the band bin label
              XmlToPvlTranslationManager labelXBandBinlater(xmlFileName, QString::fromStdString(bandBinTransFile.expanded()));
              // Pvl output label
              Pvl *outputLabel = outputCube->label();
              labelXBandBinlater.Auto(*(outputLabel));
              FileName mosaicTransFile(missionDir.toStdString() + "/translations/TgoCassisMosaic.trn");

              // Get the translation manager ready for translating the mosaic label
              XmlToPvlTranslationManager labelXMosaiclater(xmlFileName, QString::fromStdString(mosaicTransFile.expanded()));
              labelXMosaiclater.Auto(*(outputLabel));
              return true;
            }
            catch (IException &e) {
              Pvl *outputLabel = outputCube->label();
              if(outputLabel->hasGroup("Mosaic")) {
                outputLabel->deleteGroup("Mosaic");
              }
              if(outputLabel->hasGroup("BandBin")) {
                outputLabel->deleteGroup("BandBin");
              }
              return false;
            }
          }
        }
      }
    }
    return false;
  }


  /**
   * Translate instrument, bandbin, and archive info from xml label into ISIS
   * label and add kernels group.
   *
   * @param inputLabel Reference to the xml label file name for the input image.
   * @param outputCube Pointer to output cube where ISIS labels will be added and
   *                   updated.
   *
   * @internal
   *   @history 2017-01-20 Jeannie Backer - Original Version
   *   @history 2017-01-23 Kristin Berry - Added support for bandBin group and archive group
   */
  void translateLabels(FileName &inputLabel, Cube *outputCube, QString instTransFile) {
    // Get the directory where the Tgo translation tables are
    QString missionDir = "$ISISROOT/appdata/translations/";
    FileName transFile(missionDir.toStdString() + instTransFile.toStdString());

    // Get the translation manager ready for translating the instrument label
    XmlToPvlTranslationManager labelXlater(inputLabel, QString::fromStdString(transFile.expanded()));

    // Pvl output label
    Pvl *outputLabel = outputCube->label();
    labelXlater.Auto(*(outputLabel));

    // Add needed keywords that are not in the translation table
    PvlGroup &inst = outputLabel->findGroup("Instrument", Pvl::Traverse);

    // Add units of measurement to keywords from translation table
    if (inst.hasKeyword("ExposureDuration")){
      inst.findKeyword("ExposureDuration").setUnits("seconds");
    }
    // Translate BandBin group
    FileName bandBinTransFile(missionDir.toStdString() + "TgoCassisBandBin.trn");
    XmlToPvlTranslationManager bandBinXlater(inputLabel, QString::fromStdString(bandBinTransFile.expanded()));

    // Pvl output label
    outputLabel = outputCube->label();
    bandBinXlater.Auto(*(outputLabel));

    PvlGroup &bandBin = outputLabel->findGroup("BandBin", Pvl::Traverse);
    bandBin.findKeyword("Center").setUnits("nm");
    bandBin.findKeyword("Width").setUnits("nm");

    // Create the Archive Group
    FileName archiveTransFile(missionDir.toStdString() + "TgoCassisArchive.trn");
    XmlToPvlTranslationManager archiveXlater(inputLabel, QString::fromStdString(archiveTransFile.expanded()));

    FileName subTransFile(missionDir.toStdString() + "TgoCassisSubWindow.trn");
    XmlToPvlTranslationManager subXlater(inputLabel, QString::fromStdString(subTransFile.expanded()));

    // Pvl output label
    outputLabel = outputCube->label();
    archiveXlater.Auto(*(outputLabel));
    subXlater.Auto(*(outputLabel));

    // Remove trailing "Z" from PDS4 .xml (on re-ingestion) and create YearDoy keyword in Archive group
    PvlKeyword *startTime = &outputLabel->findGroup("Instrument", Pvl::Traverse)["StartTime"];
    QString startTimeString = QString::fromStdString(startTime[0]);
    if (startTimeString.endsWith("Z", Qt::CaseInsensitive)) {
      startTimeString.chop(1);
      startTime->setValue(startTimeString.toStdString());
    }

    if (outputLabel->hasGroup("StopTime")) {
      PvlKeyword *stopTime = &outputLabel->findGroup("Instrument", Pvl::Traverse)["StopTime"];
      QString stopTimeString = QString::fromStdString(stopTime[0]);
      if (stopTimeString.endsWith("Z", Qt::CaseInsensitive)){
        stopTimeString.chop(1);
        stopTime->setValue(stopTimeString.toStdString());
      }
    }

    iTime stime(startTimeString);

    PvlGroup &archive = outputLabel->findGroup("Archive", Pvl::Traverse);

    // Calculate SummingMode keyword and add to label
    QString sumMode;
    if (inst.hasKeyword("Expanded") && (int)inst.findKeyword("Expanded") == 1) {
      sumMode = "0";
    }
    else {
      sumMode = QString::fromStdString(archive["Window" + (std::string)archive["WindowCount"] + "Binning"]);
    }
    PvlKeyword summingMode("SummingMode", sumMode.toStdString());
    outputLabel->findGroup("Instrument", Pvl::Traverse).addKeyword(summingMode);

    PvlKeyword yeardoy("YearDoy", Isis::toString(stime.Year()*1000 + stime.DayOfYear()));
    archive.addKeyword(yeardoy);

    // Set units on optional archived keywords
    if (archive.hasKeyword("PredictMaximumExposureTime")) {
      archive.findKeyword("PredictMaximumExposureTime").setUnits("ms");
    }
    if (archive.hasKeyword("CassisOffNadirAngle")) {
      archive.findKeyword("CassisOffNadirAngle").setUnits("deg");
    }
    if (archive.hasKeyword("PredictedRepetitionFrequency")) {
      archive.findKeyword("PredictedRepetitionFrequency").setUnits("ms");
    }
    if (archive.hasKeyword("GroundTrackVelocity")) {
      archive.findKeyword("GroundTrackVelocity").setUnits("km/s");
    }
    if (archive.hasKeyword("ForwardRotationAngle")) {
      archive.findKeyword("ForwardRotationAngle").setUnits("deg");
    }
    if (archive.hasKeyword("SpiceMisalignment")) {
      archive.findKeyword("SpiceMisalignment").setUnits("deg");
    }
    if (archive.hasKeyword("FocalLength")) {
      archive.findKeyword("FocalLength").setUnits("m");
    }
    if (archive.hasKeyword("ImageFrequency")) {
      archive.findKeyword("ImageFrequency").setUnits("ms");
    }
    if (archive.hasKeyword("ExposureTimePEHK")) {
      archive.findKeyword("ExposureTimePEHK").setUnits("ms");
    }

    // Setup the kernel group
    PvlGroup kern("Kernels");
    QString spacecraftNumber;
    QString instId  = QString::fromStdString(inst.findKeyword("InstrumentId"));
    QString spcName = QString::fromStdString(inst.findKeyword("SpacecraftName"));
    QString filter  = QString::fromStdString(bandBin.findKeyword("FilterName"));

    if(spcName.compare("TRACE GAS ORBITER", Qt::CaseInsensitive) == 0
       && instId.compare("CaSSIS", Qt::CaseInsensitive) == 0) {

      int spacecraftCode = -143400;

      kern += PvlKeyword("NaifFrameCode", Isis::toString(spacecraftCode));

      if (filter.compare("PAN", Qt::CaseInsensitive) == 0) {
        spacecraftCode = -143421;
      }
      else if (filter.compare("RED", Qt::CaseInsensitive) == 0) {
        spacecraftCode = -143422;
      }
      else if (filter.compare("NIR", Qt::CaseInsensitive) == 0) {
        spacecraftCode = -143423;
      }
      else if (filter.compare("BLU", Qt::CaseInsensitive) == 0) {
        spacecraftCode = -143424;
      }
      else {
        std::string msg = "Unrecognized filter name ["
          + filter.toStdString()
          + "].";
          throw IException(IException::User, msg, _FILEINFO_);

      }
      // Add Kernel to BandBin Group
      bandBin.addKeyword(PvlKeyword("NaifIkCode", Isis::toString(spacecraftCode)));
    }
    else {
      std::string msg = "Unrecognized Spacecraft name ["
        + spcName.toStdString()
        + "] and instrument ID ["
        + instId.toStdString()
        + "]";
        throw IException(IException::User, msg, _FILEINFO_);
    }
    outputCube->putGroup(kern);

    // Add an alpha cube group based on the subwindowing
    if (archive.hasKeyword("Window_Count")) {
      int windowNumber = (int)archive["Window_Count"] + 1;
      QString windowString = "Window_" + QString::number(windowNumber);
      int frameletStartSample = (int) archive[windowString.toStdString() + "_Start_Sample"] + 1;
      int frameletEndSample   = (int) archive[windowString.toStdString() + "_End_Sample"] + 1;
      int frameletStartLine   = (int) archive[windowString.toStdString() + "_Start_Line"] + 1;
      int frameletEndLine     = (int) archive[windowString.toStdString() + "_End_Line"] + 1;
      AlphaCube frameletArea(2048, 2048,
                             frameletEndSample - frameletStartSample + 1,
                             frameletEndLine - frameletStartLine + 1,
                             frameletStartSample - 0.5, frameletStartLine - 0.5,
                             frameletEndSample + 0.5, frameletEndLine + 0.5);
      frameletArea.UpdateGroup(*outputCube);
    }
  }


  QString convertUniqueIdToObservationId(Pvl &outputLabel) {
    if (outputLabel.findObject("IsisCube").hasGroup("Mosaic")) {
      return ""; // translation file should auto translate this case to Mosaic group.
                 // For any other product, this ID goes in the Archive group.
    }

    QString target = "";
    if (outputLabel.findObject("IsisCube").hasGroup("Instrument")) {
      target = QString::fromStdString(outputLabel.findGroup("Instrument", Pvl::Traverse)
                          .findKeyword("TargetName")[0]);
    }
    else {
      target = QString::fromStdString(outputLabel.findGroup("Mapping", Pvl::Traverse)
                          .findKeyword("TargetName")[0]);
    }

    PvlGroup &archiveGroup = outputLabel.findGroup("Archive", Pvl::Traverse);
    QString uniqueId = QString::fromStdString(archiveGroup.findKeyword("UniqueIdentifier")[0]);

    QString observationId = "";
    BigInt uniqueIdDecimalValue = uniqueId.toLongLong();
    BigInt operationPeriod = (uniqueIdDecimalValue & 1879048192);
    operationPeriod /= qPow(2,28);
    FileName transFile("$ISISROOT/appdata/translations/TgoCassisOperationPeriod.trn");
    PvlTranslationTable transTable(transFile);
    observationId = transTable.Translate("OperationPeriod", QString::number(operationPeriod));
    BigInt orbitNumber = (uniqueIdDecimalValue & 268433408);
    orbitNumber /= qPow(2,11);
    observationId += "_";
    observationId += QString("%1").arg(orbitNumber, 6, 10, QChar('0'));

    int orbitPhase = (uniqueIdDecimalValue & 2044);
    if (target.compare("mars", Qt::CaseInsensitive) == 0) {
      orbitPhase /= qPow(2,2);
    }
    else {
      orbitPhase = 900;
    }
    observationId += "_";
    observationId += QString("%1").arg(orbitPhase, 3, 10, QChar('0'));

    int imageType = (uniqueIdDecimalValue & 3);
    observationId += "_";
    observationId += QString::number(imageType);

    archiveGroup += PvlKeyword("ObservationId", observationId.toStdString());

    return observationId;

  }
}
