/** This is free and unencumbered software released into the public domain.

The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */

#include "lronac2pds.h"

#include <sstream>

#include <QDebug>

#include "ProcessExport.h"
#include "ProcessByLine.h"
#include "Pvl.h"
#include "PvlToPvlTranslationManager.h"
#include "PvlFormatPds.h"
#include "OriginalLabel.h"
#include "iTime.h"
#include "md5.h"
#include "md5wrapper.h"

#define SCALING_FACTOR 32767

using namespace std;

namespace Isis {
  static void ResetGlobals ();
  static void ProcessImage ( Buffer &in, Buffer &out );
  static QString MD5Checksum ( QString filename );
  static void OutputLabel ( std::ofstream &fout, Cube* cube );
  static void CopyData ( std::ifstream &fin, std::ofstream &fout );

  QString g_md5Checksum;

  bool g_isIof;

  QString g_productVersionId = "N/A";

  void lronac2pds(UserInterface &ui) {
      ResetGlobals();

      g_productVersionId = ui.GetString("VERSIONIDSTRING");

      // Set the processing object

      ProcessByLine p;
      CubeAttributeInput &att = ui.GetInputAttribute("FROM");
      Cube *inCube = p.SetInputCube(ui.GetCubeName("FROM"), att);

      g_isIof = QString::fromStdString(inCube->label()->findGroup("Radiometry", Pvl::Traverse).findKeyword("RadiometricType")[0]).toUpper() == "IOF";

      FileName scaledCube("$TEMPORARY/" + FileName(ui.GetCubeName("FROM")).name());
      scaledCube.addExtension("cub");

      scaledCube = FileName::createTempFile(scaledCube);
      p.SetOutputCube(
          scaledCube.expanded(), CubeAttributeOutput(),
          inCube->sampleCount(), inCube->lineCount(),
          inCube->bandCount());

      // Scale image and calculate max and min values
      p.StartProcess(ProcessImage);
      p.EndProcess();

      ProcessExport pe;

      // Setup the input cube
      inCube = pe.SetInputCube(scaledCube.expanded(), CubeAttributeInput());

      if (g_isIof) {
          pe.SetOutputType(Isis::SignedWord);
          pe.SetOutputEndian(Isis::Lsb);

          pe.SetOutputRange(Isis::VALID_MIN2, Isis::VALID_MAX2);

          pe.SetOutputNull(Isis::NULL2);
          pe.SetOutputLrs(Isis::LOW_REPR_SAT2);
          pe.SetOutputLis(Isis::LOW_INSTR_SAT2);
          pe.SetOutputHis(Isis::HIGH_INSTR_SAT2);
          pe.SetOutputHrs(Isis::HIGH_REPR_SAT2);
      }
      else {
          pe.SetOutputType(Isis::Real);
          pe.SetOutputEndian(Isis::Lsb);

          pe.SetOutputRange(Isis::VALID_MIN4, Isis::VALID_MAX4);

          pe.SetOutputNull(Isis::NULL4);
          pe.SetOutputLrs(Isis::LOW_REPR_SAT4);
          pe.SetOutputLis(Isis::LOW_INSTR_SAT4);
          pe.SetOutputHis(Isis::HIGH_INSTR_SAT4);
          pe.SetOutputHrs(Isis::HIGH_REPR_SAT4);
      }

      FileName tempFile;
      tempFile = FileName::createTempFile("$TEMPORARY/" + FileName(ui.GetFileName("TO")).baseName() + ".temp");
      QString tempFileName(tempFile.expanded());
      ofstream temporaryFile(tempFileName.toLatin1().data());

      pe.StartProcess(temporaryFile);
      temporaryFile.close();

      // Calculate MD5 Checksum
      g_md5Checksum = MD5Checksum(tempFileName);

      FileName outFile(ui.GetFileName("TO"));
      QString outFileName(outFile.expanded());
      ifstream inFile(tempFileName.toLatin1().data());
      ofstream pdsFile(outFileName.toLatin1().data());

      // Output the label
      OutputLabel(pdsFile, inCube);

      // Then copy the image data
      CopyData(inFile, pdsFile);

      pdsFile.close();

      pe.EndProcess();

      remove((scaledCube.expanded()).toLatin1().data());
      remove(tempFileName.toLatin1().data());
      return;
  }

  void ResetGlobals () {
      g_md5Checksum = "";
      g_isIof = false;
  }

  void ProcessImage ( Buffer &in, Buffer &out ) {
      for (int i = 0; i < in.size(); i++) {
          if (IsSpecial(in[i]))
              out[i] = in[i];
          else if (g_isIof)
              out[i] = SCALING_FACTOR * in[i];
          else
              out[i] = in[i];
      }
  }

  QString MD5Checksum ( QString filename ) {
      md5wrapper md5;
      QString checkSum = md5.getHashFromFile(filename);
      return checkSum;
  }

  void OutputLabel ( std::ofstream &fout, Cube* cube ) {
      OriginalLabel origLab(cube->fileName());
      Pvl labelPvl = origLab.ReturnLabels();

      //Pvl to store the labels
      Pvl outLabel;
      PvlFormatPds *p_formatter = new PvlFormatPds("$ISISROOT/appdata/translations/LroNacPdsExportRootGen.typ");
      labelPvl.setFormat(p_formatter);
      labelPvl.setTerminator("END");

      stringstream stream;
      QString pdsLabel = "";

      //Translate the Original Pds Label
      FileName transFile("$ISISROOT/appdata/translations/LroNacPdsLabelExport.trn");
      PvlToPvlTranslationManager labelXlator(labelPvl, transFile.expanded());
      labelXlator.Auto(outLabel);

      // Copy any Translation changes over
      for (int i = 0; i < outLabel.keywords(); i++) {
          bool hasUnit = false;
          QString unit = "";
          if (labelPvl[outLabel[i].name()].unit() != "") {
              hasUnit = true;
              unit = QString::fromStdString(labelPvl[outLabel[i].name()].unit());
          }
          bool hasComment = false;
          QString comment = "";
          if (labelPvl[outLabel[i].name()].comments() > 0) {
              hasComment = true;
              comment = QString::fromStdString(labelPvl[outLabel[i].name()].comment(0));
          }
          labelPvl[outLabel[i].name()] = outLabel[i];

          if (hasUnit)
              labelPvl[outLabel[i].name()].setUnits(unit.toStdString());
          if (hasComment)
              labelPvl[outLabel[i].name()].addComment(comment.toStdString());
      }

      //Update the product ID
      //we switch the last char in the id from edr->cdr
      QString prod_id = QString::fromStdString(labelPvl["PRODUCT_ID"][0]);
      labelPvl["PRODUCT_ID"][0].replace((prod_id.length()-1), 1, "C");

      // Update the product creation time
      labelPvl["PRODUCT_CREATION_TIME"].setValue(iTime::CurrentGMT().toStdString());

      labelPvl["PRODUCT_VERSION_ID"].setValue(g_productVersionId.toStdString());

      // Update the "IMAGE" Object
      PvlObject &imageObject = labelPvl.findObject("IMAGE");
      imageObject.clear();
      imageObject += PvlKeyword("LINES", std::to_string(cube->lineCount()));
      imageObject += PvlKeyword("LINE_SAMPLES", std::to_string(cube->sampleCount()));
      if (g_isIof) {
          imageObject += PvlKeyword("SAMPLE_BITS", std::to_string(16));
          imageObject += PvlKeyword("SAMPLE_TYPE", "LSB_INTEGER");
          imageObject += PvlKeyword("SCALING_FACTOR", std::to_string(1.0 / SCALING_FACTOR));
          imageObject += PvlKeyword("VALID_MINIMUM", std::to_string(Isis::VALID_MIN2));
          imageObject += PvlKeyword("NULL", std::to_string(Isis::NULL2));
          imageObject += PvlKeyword("LOW_REPR_SATURATION", std::to_string(Isis::LOW_REPR_SAT2));
          imageObject += PvlKeyword("LOW_INSTR_SATURATION", std::to_string(Isis::LOW_INSTR_SAT2));
          imageObject += PvlKeyword("HIGH_INSTR_SATURATION", std::to_string(Isis::HIGH_INSTR_SAT2));
          imageObject += PvlKeyword("HIGH_REPR_SATURATION", std::to_string(Isis::HIGH_REPR_SAT2));
          imageObject += PvlKeyword("UNIT", "Scaled I/F");
      }
      else {
          imageObject += PvlKeyword("SAMPLE_BITS", "32");
          imageObject += PvlKeyword("SAMPLE_TYPE", "PC_REAL");
          imageObject += PvlKeyword("VALID_MINIMUM", "16#FF7FFFFA#");
          imageObject += PvlKeyword("NULL", "16#FF7FFFFB#");
          imageObject += PvlKeyword("LOW_REPR_SATURATION", "16#FF7FFFFC#");
          imageObject += PvlKeyword("LOW_INSTR_SATURATION", "16#FF7FFFFD#");
          imageObject += PvlKeyword("HIGH_INSTR_SATURATION", "16#FF7FFFFE#");
          imageObject += PvlKeyword("HIGH_REPR_SATURATION", "16#FF7FFFFF#");
          imageObject += PvlKeyword("UNIT", "W / (m**2 micrometer sr)");
      }
      imageObject += PvlKeyword("MD5_CHECKSUM", g_md5Checksum.toStdString());

      stream << labelPvl;

      int recordBytes = cube->sampleCount();
      int labelRecords = (int) ((stream.str().length()) / recordBytes) + 1;

      labelPvl["RECORD_BYTES"] = std::to_string(recordBytes);
      if (g_isIof)
          labelPvl["FILE_RECORDS"] = std::to_string((int) (cube->lineCount() * 2 + labelRecords));
      else
          labelPvl["FILE_RECORDS"] = std::to_string((int) (cube->lineCount() * 4 + labelRecords));
      labelPvl["LABEL_RECORDS"] = std::to_string(labelRecords);
      labelPvl["^IMAGE"] = std::to_string((int) (labelRecords + 1));

      stream.str(std::string());

      stream << labelPvl;
      pdsLabel += stream.str().c_str();
      /* Ensure that we have enough room for the actual label content, plus at
       * least two bytes for a carriage return and a linefeed, so the end of the
       * label looks pretty */
      while ((int)pdsLabel.length() + 2 > (int)(labelRecords * recordBytes)) {
          labelRecords++;
          // Refresh the label content
          labelPvl["FILE_RECORDS"] = std::to_string((int) (cube->lineCount() * 4 + labelRecords));
          labelPvl["LABEL_RECORDS"] = std::to_string(labelRecords);
          labelPvl["^IMAGE"] = std::to_string((int) (labelRecords + 1));
          stream.str(std::string());
          stream << labelPvl;
          pdsLabel = stream.str().c_str();
      }

      /* Now, add a carriage return and linefeed, and then pad the label with
       * spaces if necessary */
      pdsLabel += "\r\n";


      while ((int)pdsLabel.length() < (int) (labelRecords * recordBytes)) {
          pdsLabel += " ";
      }

      fout << pdsLabel;
      return;
  }

  void CopyData ( std::ifstream &fin, std::ofstream &fout ) {
      char line[2532];
      while (!fin.eof()) {
          fin.read(line, 2532);
          fout.write(line, fin.gcount());
      }
  }
}
