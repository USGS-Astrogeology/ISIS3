/** This is free and unencumbered software released into the public domain.

The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */

#include "ExportDescription.h"
#include "FileName.h"
#include "ImageExporter.h"
#include "UserInterface.h"


using namespace std;

namespace Isis  {

  int addChannel(UserInterface &ui, ExportDescription &desc, QString param, QString mode);
  void addResults(PvlGroup &results, ImageExporter *exporter, QString channel, int index);


  void isis2std(UserInterface &ui, Pvl *log) {
    QString format = ui.GetString("FORMAT");
    ImageExporter *exporter = ImageExporter::fromFormat(format);

    ExportDescription desc;
    if (ui.GetString("BITTYPE") == "8BIT") {
      desc.setPixelType(UnsignedByte);
    }
    else if (ui.GetString("BITTYPE") == "U16BIT") {
      desc.setPixelType(UnsignedWord);
    }
    else { //if (ui.GetString("BITTYPE") == "S16BIT")
      desc.setPixelType(SignedWord);
    }

    int redIndex = -1;
    int greenIndex = -1;
    int blueIndex = -1;
    int alphaIndex = -1;

    QString mode = ui.GetString("MODE");
    if (mode == "GRAYSCALE") {
      addChannel(ui, desc, "FROM", mode);
      exporter->setGrayscale(desc);
    }
    else {
      redIndex = addChannel(ui, desc, "RED", mode);
      greenIndex = addChannel(ui, desc, "GREEN", mode);
      blueIndex = addChannel(ui, desc, "BLUE", mode);

      if (mode == "ARGB") {
        alphaIndex = addChannel(ui, desc, "ALPHA", mode);
        exporter->setRgba(desc);
      }

      else {
        exporter->setRgb(desc);
      }
    }

    FileName outputName = ui.GetFileName("TO").toStdString();
    int quality = ui.GetInteger("QUALITY");


    QString compression;
    if (format == "TIFF") {
      compression = ui.GetString("COMPRESSION").toLower();

    }
    else {
      compression = "none";
    }

    exporter->write(outputName, quality, compression, &ui);

    if (mode != "GRAYSCALE" && ui.GetString("STRETCH") != "MANUAL") {
      ui.Clear("MINIMUM");
      ui.Clear("MAXIMUM");

      ui.PutDouble("RMIN", exporter->inputMinimum(redIndex));
      ui.PutDouble("RMAX", exporter->inputMaximum(redIndex));
      ui.PutDouble("GMIN", exporter->inputMinimum(greenIndex));
      ui.PutDouble("GMAX", exporter->inputMaximum(greenIndex));
      ui.PutDouble("BMIN", exporter->inputMinimum(blueIndex));
      ui.PutDouble("BMAX", exporter->inputMaximum(blueIndex));

      if (mode == "ARGB") {
        ui.PutDouble("AMIN", exporter->inputMinimum(alphaIndex));
        ui.PutDouble("AMAX", exporter->inputMaximum(alphaIndex));
      }
    }

    // Write out the results
    PvlGroup results("Results");
    results += PvlKeyword("OutputFileName", outputName.expanded());

    if (mode == "GRAYSCALE") {
      addResults(results, exporter, "", 0);
    }
    else {
      addResults(results, exporter, "Red", redIndex);
      addResults(results, exporter, "Green", greenIndex);
      addResults(results, exporter, "Blue", blueIndex);

      if (mode == "ARGB") addResults(results, exporter, "Alpha", alphaIndex);
    }

    if (log) {
      log->addGroup(results);
    }

    delete exporter;
  }


  int addChannel(UserInterface &ui, ExportDescription &desc, QString param, QString mode) {
    FileName name = ui.GetCubeName(param).toStdString();
    CubeAttributeInput &att = ui.GetInputAttribute(param);

    int index = -1;
    if (mode != "GRAYSCALE" && ui.GetString("STRETCH") == "MANUAL") {
      QString bandId = param.mid(0, 1);
      double min = ui.GetDouble(bandId + "MIN");
      double max = ui.GetDouble(bandId + "MAX");

      index = desc.addChannel(name, att, min, max);
    }
    else {
      index = desc.addChannel(name, att);
    }

    return index;
  }


  void addResults(PvlGroup &results, ImageExporter *exporter,
      QString channel, int index) {

    results += PvlKeyword(channel.toStdString() + "InputMinimum", toString(exporter->inputMinimum(index)));
    results += PvlKeyword(channel.toStdString() + "InputMaximum", toString(exporter->inputMaximum(index)));
  }

}