/** This is free and unencumbered software released into the public domain.

The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */
#include "lrowac2pds.h"

#include <cstdio>
#include <QString>
#include <QRegExp>
#include "FileList.h"
#include "Brick.h"
#include "OriginalLabel.h"
#include "Brick.h"
#include "History.h"
#include "Stretch.h"
#include "iTime.h"
#include "ProcessExport.h"
#include "PvlToPvlTranslationManager.h"
#include "PvlFormatPds.h"
#include "md5.h"
#include "md5wrapper.h"
#include "OriginalLabel.h"
#include <sstream>

#define COLOR_SAMPLES 704
#define VIS_SAMPLES 704
#define UV_SAMPLES 128
#define BW_SAMPLES 1024
#define VIS_LINES 14
#define UV_LINES 4

using namespace std;

namespace Isis {
  static vector<int> frameletLines;

  static void ResetGlobals ();
  static void mergeFramelets ();
  static QString MD5Checksum ( QString filename );
  static void OutputLabel ( std::ofstream &fout, Cube* cube, Pvl &pdsLab );
  static void CopyData ( std::ifstream &fin, std::ofstream &fout );

  static std::vector<int> padding;
  static int colorOffset = 0;

  // Output UV Files
  static Cube *uveven = NULL;
  static Cube *uvodd = NULL;

  // Output VIS Files
  static Cube *viseven = NULL;
  static Cube *visodd = NULL;

  static Cube* out = NULL;

  static QString instrumentModeId = "";
  static QString productId = "";
  static QString g_productVersionId = "N/A";

  static QString g_md5Checksum;

  static int numFramelets = 0;
  static int numSamples = 0;
  static int numLines = 0;
  static int numUVFilters = 0;
  static int numVisFilters = 0;

  static bool g_isIoF;

  void lrowac2pds(UserInterface &ui) {
      ResetGlobals();
      Pvl pdsLab;

      FileList list;
      list.read(ui.GetFileName("FROMLIST").toStdString());

      if (list.size() < 1) {
          std::string msg = "The list file [" + ui.GetFileName("FROMLIST").toStdString() + "does not contain any data";
          throw IException(IException::User, msg, _FILEINFO_);
      }

      g_productVersionId = ui.GetString("VERSIONIDSTRING");

      for (int i = 0; i < list.size(); i++) {

          Pvl tempPvl;
          tempPvl.read(list[i].toString());

          OriginalLabel origLab(QString::fromStdString(list[i].toString()));
          pdsLab = origLab.ReturnLabels();

          QString prodId = QString::fromStdString(pdsLab["PRODUCT_ID"][0]);
          if (productId == "")
              productId = prodId;

          if (productId != prodId) {
              std::string msg = "This program is intended for use on a single LROC WAC images only.";
              msg += "The ProductIds do not match.";
              throw IException(IException::User, msg, _FILEINFO_);
          }

          Isis::PvlGroup &inst = tempPvl.findGroup("Instrument", Pvl::Traverse);
          QString instId = QString::fromStdString(inst["InstrumentId"]);
          QString framelets = QString::fromStdString(inst["Framelets"]);
          QString numFrames = QString::fromStdString(inst["NumFramelets"]);

          if (instId != "WAC-VIS" && instId != "WAC-UV") {
              std::string msg = "This program is intended for use on LROC WAC images only. [";
              msg += list[i].toString() + "] does not appear to be a WAC image.";
              throw IException(IException::User, msg, _FILEINFO_);
          }

          QString instModeId = QString::fromStdString(inst["InstrumentModeId"]);
          if (instrumentModeId == "")
              instrumentModeId = instModeId;
          if (numFramelets == 0)
              numFramelets = numFrames.toInt();
          g_isIoF = QString::fromStdString(tempPvl.findGroup("Radiometry", Pvl::Traverse).findKeyword("RadiometricType")[0]).toUpper() == "IOF";

          if (instId == "WAC-VIS" && framelets == "Even") {
              viseven = new Cube();
              viseven->open(QString::fromStdString(list[i].toString()));
          }
          else if (instId == "WAC-VIS" && framelets == "Odd") {
              visodd = new Cube();
              visodd->open(QString::fromStdString(list[i].toString()));
          }
          if (instId == "WAC-UV" && framelets == "Even") {
              uveven = new Cube();
              uveven->open(QString::fromStdString(list[i].toString()));
          }
          else if (instId == "WAC-UV" && framelets == "Odd") {
              uvodd = new Cube();
              uvodd->open(QString::fromStdString(list[i].toString()));
          }
      }

      // Determine our band information based on
      // INSTRUMENT_MODE_ID - FILTER_NUMBER is
      // only going to be used for BW images
      if (instrumentModeId == "COLOR") {
          numUVFilters = 2;
          numVisFilters = 5;

          numSamples = COLOR_SAMPLES;
      }
      else if (instrumentModeId == "VIS") {
          numUVFilters = 0;
          numVisFilters = 5;

          numSamples = VIS_SAMPLES;
      }
      else if (instrumentModeId == "UV") {
          numUVFilters = 2;
          numVisFilters = 0;

          numSamples = UV_SAMPLES;
      }
      else if (instrumentModeId == "BW") {
          numUVFilters = 0;
          numVisFilters = 1;

          numSamples = BW_SAMPLES;
      }

      numLines = numFramelets * (UV_LINES * numUVFilters + VIS_LINES * numVisFilters);

      out = new Cube();
      out->setDimensions(numSamples, numLines, 1);
      out->setPixelType(Isis::Real);

      FileName mergedCube = FileName::createTempFile(
          "$TEMPORARY/" + FileName(ui.GetFileName("TO").toStdString()).baseName() + ".cub");
      out->create(QString::fromStdString(mergedCube.expanded()));

      mergeFramelets();

      /*

       FileName outFile(ui.GetFileName("TO", "img"));
       QString outFileName(outFile.expanded());
       ofstream oCube(outFileName.c_str());
       p.OutputLabel(oCube);
       p.StartProcess(oCube);
       oCube.close();
       p.EndProcess();

       */

      out->close();
      delete out;
      out = NULL;

      if (uveven) {
          uveven->close();
          delete uveven;
          uveven = NULL;
      }

      if (uvodd) {
          uvodd->close();
          delete uvodd;
          uvodd = NULL;
      }

      if (viseven) {
          viseven->close();
          delete viseven;
          viseven = NULL;
      }

      if (visodd) {
          visodd->close();
          delete visodd;
          visodd = NULL;
      }

      // Export data

      ProcessExport pe;

      // Setup the input cube
      Cube *inCube = pe.SetInputCube(QString::fromStdString(mergedCube.expanded()), CubeAttributeInput());

      pe.SetOutputType(Isis::Real);
      pe.SetOutputEndian(Isis::Lsb);

      pe.SetOutputRange(Isis::VALID_MIN4, Isis::VALID_MAX4);

      pe.SetOutputNull(Isis::NULL4);
      pe.SetOutputLrs(Isis::LOW_REPR_SAT4);
      pe.SetOutputLis(Isis::LOW_INSTR_SAT4);
      pe.SetOutputHis(Isis::HIGH_INSTR_SAT4);
      pe.SetOutputHrs(Isis::HIGH_REPR_SAT4);

      FileName tempFile = FileName::createTempFile(
          "$TEMPORARY/" + FileName(ui.GetFileName("TO").toStdString()).baseName() + ".temp");
      QString tempFileName(QString::fromStdString(tempFile.expanded()));
      ofstream temporaryFile(tempFileName.toLatin1().data());

      pe.StartProcess(temporaryFile);
      temporaryFile.close();

      // Calculate MD5 Checksum
      g_md5Checksum = MD5Checksum(tempFileName);

      FileName outFile(ui.GetFileName("TO").toStdString());
      QString outFileName(QString::fromStdString(outFile.expanded()));
      ifstream inFile(tempFileName.toLatin1().data());
      ofstream pdsFile(outFileName.toLatin1().data());

      // Output the label
      OutputLabel(pdsFile, inCube, pdsLab);

      // Then copy the image data
      CopyData(inFile, pdsFile);

      pdsFile.close();

      pe.EndProcess();

      remove((mergedCube.expanded()).c_str());
      remove(tempFileName.toLatin1().data());
      return;
  }

  void ResetGlobals () {
      colorOffset = 0;
      frameletLines.clear();

      uveven = NULL;
      uvodd = NULL;
      viseven = NULL;
      visodd = NULL;

      out = NULL;

      instrumentModeId = "";
      productId = "";
      g_md5Checksum = "";

      numFramelets = 0;
      numSamples = 0;
      numLines = 0;
      numUVFilters = 0;
      numVisFilters = 0;

      g_isIoF = false;
  }

  //! Merges each of the individual WAC framelets into the right place
  void mergeFramelets () {
      Brick *uvevenManager = NULL, *uvoddManager = NULL, *visevenManager = NULL, *visoddManager = NULL;

      if (numUVFilters > 0) {
          uvevenManager = new Brick(*uveven, UV_SAMPLES, UV_LINES, numUVFilters);
          uvoddManager = new Brick(*uvodd, UV_SAMPLES, UV_LINES, numUVFilters);

          uvevenManager->begin();
          uvoddManager->begin();
      }
      if (numVisFilters > 0) {
          visevenManager = new Brick(*viseven, numSamples, VIS_LINES, numVisFilters);
          visoddManager = new Brick(*visodd, numSamples, VIS_LINES, numVisFilters);

          visevenManager->begin();
          visoddManager->begin();
      }

      Brick outManager(*out, numSamples, UV_LINES * numUVFilters + VIS_LINES * numVisFilters, 1);
      outManager.begin();

      // For each frame
      for (int f = 0; f < numFramelets; f++) {
          // write out the UV first
          if (numUVFilters > 0) {
              uveven->read(*uvevenManager);
              uvodd->read(*uvoddManager);

              int pad = (numSamples - UV_SAMPLES) / 2;
              for (int line = 0; line < numUVFilters * UV_LINES; line++) {
                  int offset = numSamples * line;
                  // left padding
                  for (int i = 0; i < pad; i++)
                      outManager[offset + i] = Isis::Null;
                  for (int i = 0; i < UV_SAMPLES; i++) {
                      int index = line * UV_SAMPLES + i;
                      if (f % 2 == 0)
                          outManager[i + offset + pad] = uvoddManager->at(index);
                      else
                          outManager[i + offset + pad] = uvevenManager->at(index);
                  }
                  // right padding
                  for (int i = 0; i < pad; i++)
                      outManager[i + offset + pad + UV_SAMPLES] = Isis::Null;
              }
              uvevenManager->next();
              uvoddManager->next();
          }
          // then the vis
          if (numVisFilters > 0) {
              viseven->read(*visevenManager);
              visodd->read(*visoddManager);

              int offset = numUVFilters * UV_LINES * numSamples;
              for (int i = 0; i < numVisFilters * VIS_LINES * numSamples; i++) {
                  if (f % 2 == 0)
                      outManager[i + offset] = visoddManager->at(i);
                  else
                      outManager[i + offset] = visevenManager->at(i);
              }

              visevenManager->next();
              visoddManager->next();
          }

          out->write(outManager);
          outManager.next();
      }
  }

  QString MD5Checksum ( QString filename ) {
      md5wrapper md5;
      QString checkSum = md5.getHashFromFile(filename);
      return checkSum;
  }

  void OutputLabel ( std::ofstream &fout, Cube* cube, Pvl &labelPvl ) {
      //Pvl to store the labels
      Pvl outLabel;
      PvlFormatPds *p_formatter = new PvlFormatPds(
                                  "$ISISROOT/appdata/translations/LroNacPdsExportRootGen.typ");
      labelPvl.setFormat(p_formatter);
      labelPvl.setTerminator("END");

      stringstream stream;
      QString pdsLabel = "";

      //Translate the Original Pds Label
      FileName transFile("$ISISROOT/appdata/translations/LroWacPdsLabelExport.trn");
      PvlToPvlTranslationManager labelXlator(labelPvl, QString::fromStdString(transFile.expanded()));
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
      QString prod_id = QString::fromStdString(labelPvl["PRODUCT_ID"][0]);
      labelPvl["PRODUCT_ID"][0].replace((prod_id.length()-1), 1, "C");

      // Update the product creation time
      labelPvl["PRODUCT_CREATION_TIME"].setValue(iTime::CurrentGMT().toStdString());

      labelPvl["PRODUCT_VERSION_ID"].setValue(g_productVersionId.toStdString());

      // Update the "IMAGE" Object
      PvlObject &imageObject = labelPvl.findObject("IMAGE");
      imageObject.clear();
      imageObject += PvlKeyword("LINES", Isis::toString(cube->lineCount()));
      imageObject += PvlKeyword("LINE_SAMPLES", Isis::toString(cube->sampleCount()));
      imageObject += PvlKeyword("SAMPLE_BITS", Isis::toString(32));
      imageObject += PvlKeyword("SAMPLE_TYPE", "PC_REAL");
      imageObject += PvlKeyword("VALID_MINIMUM", "16#FF7FFFFA#");
      imageObject += PvlKeyword("NULL", "16#FF7FFFFB#");
      imageObject += PvlKeyword("LOW_REPR_SATURATION", "16#FF7FFFFC#");
      imageObject += PvlKeyword("LOW_INSTR_SATURATION", "16#FF7FFFFD#");
      imageObject += PvlKeyword("HIGH_INSTR_SATURATION", "16#FF7FFFFE#");
      imageObject += PvlKeyword("HIGH_REPR_SATURATION", "16#FF7FFFFF#");
      if (g_isIoF == true)
          imageObject += PvlKeyword("UNIT", "\"I/F\"");
      else
      imageObject += PvlKeyword("UNIT", "W / (m**2 micrometer sr)");
      imageObject += PvlKeyword("MD5_CHECKSUM", g_md5Checksum.toStdString());

      stream << labelPvl;

      int recordBytes = cube->sampleCount();
      int labelRecords = (int) ((stream.str().length()) / recordBytes) + 1;

      labelPvl["RECORD_BYTES"] = Isis::toString(recordBytes);
      labelPvl["FILE_RECORDS"] = Isis::toString((int) (cube->lineCount() * 4 + labelRecords));
      labelPvl["LABEL_RECORDS"] = Isis::toString(labelRecords);
      labelPvl["^IMAGE"] = Isis::toString((int) (labelRecords + 1));

      stream.str(std::string());

      stream << labelPvl;
      pdsLabel += stream.str().c_str();

      /* Ensure that we have enough room for the actual label content, plus at
       * least two bytes for a carriage return and a linefeed, so the end of the
       * label looks pretty */
      while ((int)pdsLabel.length() + 2 > (int)(labelRecords * recordBytes)) {
          labelRecords++;
          // Refresh the label content
          labelPvl["FILE_RECORDS"] = Isis::toString((int) (cube->lineCount() * 4 + labelRecords));
          labelPvl["LABEL_RECORDS"] = Isis::toString(labelRecords);
          labelPvl["^IMAGE"] = Isis::toString((int) (labelRecords + 1));
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

      fout << pdsLabel.toStdString();
      return;
  }

  void CopyData ( std::ifstream &fin, std::ofstream &fout ) {
      char line[704];
      while (!fin.eof()) {
          fin.read(line, 704);
          fout.write(line, fin.gcount());
      }
  }
}
