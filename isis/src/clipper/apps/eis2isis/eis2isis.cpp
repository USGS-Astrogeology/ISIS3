/** This is free and unencumbered software released into the public domain.

The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */


#include "CSVReader.h"
#include "Cube.h"

#include "FileName.h"
#include "IException.h"
#include "OriginalXmlLabel.h"
#include "ProcessImport.h"

#include "Pvl.h"
#include "PvlKeyword.h"
#include "Table.h"
#include "UserInterface.h"
#include "XmlToPvlTranslationManager.h"

#include <iostream>
#include <fstream>
#include <istream>

#include "eis2isis.h"
#include <QDebug>
#include <QPair>
#include <QString>
#include <QVector>

using namespace std;

namespace Isis {

  void translateCoreInfo(FileName &inputLabel, ProcessImport &importer);
  void translateCoreInfo(XmlToPvlTranslationManager labelXlater, ProcessImport &importer);
  void translateEISLabels(FileName &inputLabel, Pvl *outputLabel);
  void translateLabels(FileName &inputLabel, Pvl *outputLabel, FileName transFile);
  Table normalizeTimeTable(const FileName &file, const QString &tableName, int numLines);
  Table createTable(const FileName &file, const QString &tableName, int numLines);
  void modifyNacRollingShutterLabel(UserInterface &ui, Cube *outputCube, FileName xmlFileName, OriginalXmlLabel xmlLabel);

  void eis2isis(UserInterface &ui) {
    FileName xmlFileName = ui.GetFileName("FROM");

    try {
      ProcessImport p;
      translateCoreInfo(xmlFileName, p);

      if (xmlFileName.removeExtension().addExtension("dat").fileExists()) {
        p.SetInputFile(xmlFileName.removeExtension().addExtension("dat").expanded());
      }
      else {
        std::string msg = "Cannot find image file for [" + xmlFileName.name() + "]. Confirm the "
          ".dat file for this XML exists and is located in the same directory.";
        throw IException(IException::User, msg, _FILEINFO_);
      }

      Cube *outputCube = p.SetOutputCube("TO", ui);
      Pvl *outputLabel = outputCube->label();

      translateEISLabels(xmlFileName, outputLabel);

      FileName outputCubeFileName(ui.GetCubeName("TO"));

      OriginalXmlLabel xmlLabel;
      xmlLabel.readFromXmlFile(xmlFileName);

      p.StartProcess();

      // The ClipperNacRollingShutterCamera requires extra information for instantiating a camera.
      if (outputLabel->findKeyword("InstrumentId", PvlObject::Traverse)[0] == "EIS-NAC-RS") {
        modifyNacRollingShutterLabel(ui, outputCube, xmlFileName, xmlLabel);
      }

      // Write out original label before closing the cube
      outputCube->write(xmlLabel);

      // Pvl output label
      outputLabel = outputCube->label();

      // Remove trailing "Z" from StartTime for ISIS label
      PvlKeyword *startTime = &outputLabel->findGroup("Instrument", Pvl::Traverse)["StartTime"];
      QString startTimeString = QString::fromStdString(startTime[0]);
      if (startTimeString.endsWith("Z", Qt::CaseInsensitive)) {
        startTimeString.chop(1);
        startTime->setValue(startTimeString.toStdString());
      }

      PvlKeyword *instrumentName = &outputLabel->findGroup("Instrument", Pvl::Traverse)["InstrumentId"];
      QString instrumentNameString = QString::fromStdString(instrumentName[0]);

      PvlGroup kerns("Kernels");
      if (instrumentNameString == "EIS-NAC-RS") {
        // This ID will need to be updated. It is temporarily used for testing but is NOT the actual
        // NAC ID.
        kerns += PvlKeyword("NaifFrameCode", std::to_string(-159101));
      }
      else if (instrumentNameString == "EIS-WAC-FC") {
        kerns += PvlKeyword("NaifFrameCode", std::to_string(-159102));
      }
      else {
        std::string msg = "Input file [" + xmlFileName.expanded() + "] has an invalid " +
                  "InstrumentId.";
        throw IException(IException::Unknown, msg, _FILEINFO_);
      }

      outputCube->putGroup(kerns);

      p.EndProcess();
    }
    catch (IException &e) {

      std::string msg = "Given file [" + xmlFileName.expanded() + "] does not appear to be a valid "
                    "Clipper EIS label or associated line times files are not provided.";
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

    // Get the translation manager ready
    FileName transFile = "$ISISROOT/appdata/translations/ClipperEisCore.trn";
    XmlToPvlTranslationManager labelXlater(inputLabel, transFile.expanded());
    translateCoreInfo(labelXlater, importer);
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
    int ns = toInt(str);
    str = labelXlater.Translate("CoreLines");
    int nl = toInt(str);
    str = labelXlater.Translate("CoreBands");
    int nb = toInt(str);
    importer.SetDimensions(ns, nl, nb);

    str = labelXlater.Translate("CoreType");
    importer.SetPixelType(PixelTypeEnumeration(str));

    str = labelXlater.Translate("CoreByteOrder");
    importer.SetByteOrder(ByteOrderEnumeration(str));

    importer.SetFileHeaderBytes(0);

    str = labelXlater.Translate("CoreBase");
    importer.SetBase(toDouble(str));
    str = labelXlater.Translate("CoreMultiplier");
    importer.SetMultiplier(toDouble(str));

    // These are hard-coded to ISIS values, but the team may choose to set them
    // differently and include them in the imported xml file in the future
    importer.SetNull(Isis::NULL4, Isis::NULL4);
    importer.SetLRS(Isis::LOW_REPR_SAT4, Isis::LOW_REPR_SAT4);
    importer.SetLIS(Isis::LOW_INSTR_SAT4, Isis::LOW_INSTR_SAT4);
    importer.SetHRS(Isis::HIGH_REPR_SAT4, Isis::HIGH_REPR_SAT4);
    importer.SetHIS(Isis::HIGH_INSTR_SAT4, Isis::HIGH_INSTR_SAT4);
  }


  /**
   * @brief Translate the EIS labels using all necessary translation files.
   *
   * @param FileName inputLabel Reference to the xml label file name for the input image.
   * @param Pvl outputCube Pointer to the output cube where ISIS labels will be added and updated.
  */
  void translateEISLabels(FileName &inputLabel, Pvl *outputLabel) {

    // Translate labels for each translation file needed
    translateLabels(inputLabel, outputLabel, FileName("$ISISROOT/appdata/translations/ClipperEisInstrument.trn"));
  }


  /**
   * @brief Make use of the given translation file to fill the ISIS cube label with info from the xml label.
   *
   * @param FileName inputLabel Reference to the xml label file name for the input image.
   * @param Pvl outputLabel Pointer to the output cube label to be added to and/or updated.
   * @param FileName transFile The file name of the translation file to use.
   */
  void translateLabels(FileName &inputLabel, Pvl *outputLabel, FileName transFile) {
    // Get the translation manager ready for translating the label
    XmlToPvlTranslationManager labelXlater(inputLabel, transFile.expanded());

    // Translate the output label
    labelXlater.Auto(*(outputLabel));
  }


  /**
   * @brief Adds to the NacRs cube label as this camera model needs more/certain information.
   *
   * @param Cube outputCube Pointer to the output cube to modify
   * @param FileName xmlFileName File name of the original xml label
   */
  void modifyNacRollingShutterLabel(UserInterface &ui, Cube *outputCube, FileName xmlFileName, OriginalXmlLabel xmlLabel) {

    Pvl *outputLabel = outputCube->label();

    // Set a default value for the JitterSampleCoefficients and the JitterLineCoefficient keywords in the Instrument group.
    // These values are overwritten with a call to the jitterfit application.
    PvlKeyword jitterLineCoefficients = PvlKeyword("JitterLineCoefficients", (std::to_string(0.0)));
    jitterLineCoefficients += std::to_string(0.0);
    jitterLineCoefficients += std::to_string(0.0);
    outputLabel->findGroup("Instrument", PvlObject::Traverse).addKeyword(jitterLineCoefficients);

    PvlKeyword jitterSampleCoefficients = PvlKeyword("JitterSampleCoefficients", (std::to_string(0.0)));
    jitterSampleCoefficients += std::to_string(0.0);
    jitterSampleCoefficients += std::to_string(0.0);
    outputLabel->findGroup("Instrument", PvlObject::Traverse).addKeyword(jitterSampleCoefficients);


    // Write the line times tables to the main EIS cube
    if (ui.WasEntered("MAINREADOUT")) {

      // Create and write normalized time values in the range [-1,1] to the primary EIS cube
      Table normalizedReadout = normalizeTimeTable(FileName(ui.GetFileName("MAINREADOUT")),
                                  "Normalized Main Readout Line Times",
                                  outputCube->lineCount());

      outputCube->write(normalizedReadout);

    }
    // Since ClipperNacRolingShutterCamera requires the "Normailized Main Readout Line
    // Times" table, we are requiring that a file of lines and their times be
    // provided in the MAINREADOUT parameter. It may be possible to refactor
    // the camera object to be able to handle when a table does not exist and
    // this will be able to be refactored to not require a line times file.
    else {
      std::string msg = "This image appears to be a Narrow Angle Rolling Shutter Camera. "
                    "You must provide the line times file associated with [" + xmlFileName.name() +
                    "] as the [MAINREADOUT] parameter.";
      throw IException(IException::User, msg, _FILEINFO_);
    }

    // Handle an optional checkline cube.
    if (ui.WasEntered("FROM2")) {
      FileName checklineXmlFileName = ui.GetFileName("FROM2");

      if (ui.WasEntered("CHECKLINEREADOUT")) {
        // Process the checkline image to an ISIS cube and write the checkline tables
        ProcessImport p2;
        translateCoreInfo(checklineXmlFileName, p2);
        if (checklineXmlFileName.removeExtension()
                                .addExtension("dat")
                                .fileExists()) {
          p2.SetInputFile(checklineXmlFileName.removeExtension()
                                            .addExtension("dat")
                                            .expanded());
        }
        else {
          std::string msg = "Cannot find image file for [" + checklineXmlFileName.name() + "]. Confirm the "
          ".dat file for this XML exists and is located in the same directory.";
          throw IException(IException::User, msg, _FILEINFO_);
        }

        Cube *checklineCube = p2.SetOutputCube("TO2", ui);
        Pvl *checklineLabel = checklineCube->label();

        translateEISLabels(checklineXmlFileName, checklineLabel);

        OriginalXmlLabel checklineXmlLabel;
        xmlLabel.readFromXmlFile(checklineXmlFileName);
        p2.StartProcess();
        // Create and write regular checkline time values to the checkline cube
        Table checkLineReadout = createTable(FileName(ui.GetFileName("CHECKLINEREADOUT")),
                                            "Checkline Readout Line Times",
                                            60);

        // Create and write normalized checkline time values in the range
        // [-1,1] to the checkline cube
        Table checkLineNormalizedReadout = normalizeTimeTable(FileName(ui.GetFileName("CHECKLINEREADOUT")),
                                    "Normalized Checkline Readout Line Times",
                                    60);

        // Write these table to the main cube
        outputCube->write(checkLineReadout);
        outputCube->write(checkLineNormalizedReadout);

        checklineCube->write(checkLineReadout);
        checklineCube->write(checkLineNormalizedReadout);

        // Write out original label before closing the checkline cube
        checklineCube->write(checklineXmlLabel);
        p2.EndProcess();
      }
      // Since the checkline cube is not worth anything without the associated
      // times, require that the line times for the checkline cube be provided.
      else {
        std::string msg = "Must provide the line times file associated with [" + checklineXmlFileName.name() + "] as the [CHECKLINEREADOUT] parameter.";
        throw IException(IException::User, msg, _FILEINFO_);
      }

    }
  }


  /**
   * @brief Creates a table from <double line, double time> pairs stored in a CSV file.  The times
   * are normalized in the range [-1,1]
   * @param const FileName &file Name of the CSV file to read in
   * @param const QString &tableName Name of the table to create
   * @param int numLines Number of lines in the CSV file (without header)
   * @return  Table Returns the created table with normalized time values.
   */
  Table normalizeTimeTable(const FileName &file, const QString &tableName, int numLines) {
    double tmin =DBL_MAX;
    double tmax = DBL_MIN;
    CSVReader csv(file.expanded() );

    // Number of lines provided must match number of rows in csv file
    if (numLines != csv.rows()) {
      std::string msg = "Readout table [" + file.expanded() +
                    "] does not have the same number of lines as the image";
      throw IException(IException::User, msg, _FILEINFO_);
    }

    TableField lineField("line number", TableField::Integer);
    TableField timeField("time", TableField::Double);

    // Add the fields to a TableRecord to define the Table
    TableRecord record;
    record += lineField;
    record += timeField;

    Table table(tableName.toStdString(), record);

    QVector<QPair<int,double> > tvector;

    for (int i = 0; i < csv.rows(); i++ ) {
      CSVReader::CSVAxis row = csv.getRow(i);
      double t = row[1].toDouble();
      if ( t <= tmin) tmin = t;
      if (t >= tmax) tmax = t;
      QPair <int,double> tnode;
      tnode.first = row[0].toInt();
      tnode.second = t;
      tvector.push_back(tnode);
    }

    double trange = tmax - tmin;

    for (int i=0; i < tvector.size(); i++) {
      QPair<int,double> tnode = tvector.at(i);
      // Scale to [0,1] first
      double normalt = (tnode.second - tmin)/trange;
      // Now scale to [-1,1]
      double trange2=2.0;  //max([-1,1]) - min ([-1,1])
      normalt = normalt*trange2 + (-1);
      record[0] = tnode.first;

      record[1] = normalt;
      table += record;
    }
    return table;
  }


  /**
   * Creates a table from a CSV file.
   *
   * @param const FileName &file Name of the CSV file to read in
   * @param const QString &tableName Name of the table to create
   * @param int numLines Number of lines in the CSV file (without header)
   *
   * @return Table Returns the created table.
   */
  Table createTable(const FileName &file, const QString &tableName, int numLines) {
    CSVReader csv(file.expanded());

    // Number of lines provided must match number of rows in csv file
    if (numLines != csv.rows()) {
      std::string msg = "Readout table [" + file.expanded() +
                    "] does not have the same number of lines as the image";
      throw IException(IException::User, msg, _FILEINFO_);
    }

    // Define the fields and their types
    TableField lineField("line number", TableField::Integer);
    TableField timeField("time", TableField::Double);

    // Add the fields to a TableRecord to define the Table
    TableRecord record;
    record += lineField;
    record += timeField;

    Table table(tableName.toStdString(), record);

    // Grab the values from the csv and put them into the table row-by-row
    for (int i = 0; i < csv.rows(); i++) {
      CSVReader::CSVAxis row = csv.getRow(i);
      record[0] = row[0].toInt();
      record[1] = row[1].toDouble();
      table += record;
    }
    return table;
  }
}
