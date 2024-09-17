/** This is free and unencumbered software released into the public domain.

The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */

#include "Isis.h"

#include <QFile>
#include <QString>
#include <QStringList>
#include <QTextStream>

#include "FileName.h"
#include "IException.h"
#include "iTime.h"
#include "LineManager.h"
#include "OriginalLabel.h"
#include "Pipeline.h"
#include "ProcessImportPds.h"
#include "Progress.h"
#include "Pvl.h"
#include "PvlKeyword.h"
#include "PvlGroup.h"
#include "PvlObject.h"
#include "PvlToPvlTranslationManager.h"
#include "PushFrameCameraCcdLayout.h"
#include "UserInterface.h"

using namespace std;
using namespace Isis;

void translateLabel(Pvl &inputLabel, Pvl &outputLabel);
void processFramelets(Buffer &in);
void processFullFrames(Buffer &in);
void openNextCube(int index);

QList<Cube *> g_outputCubes;
QList<QString> g_outputCubeFileNames;
int g_frameletLines = 0;
QStringList g_filterList;
QList<int> g_filterOffsetList;
int g_fullFrameLines = 0;

void IsisMain() {
  ProcessImportPds importPds;
  g_outputCubes.clear();

  UserInterface &ui = Application::GetUserInterface();
  FileName inputFile = ui.GetFileName("FROM").toStdString();

  Pvl inputLabel;
  importPds.SetPdsFile(QString::fromStdString(inputFile.expanded()), "", inputLabel);
  OriginalLabel origLabels(inputLabel);

  Pvl outputLabel;
  translateLabel(inputLabel, outputLabel);

  bool doFullCcd = ui.GetBoolean("FULLCCD");
  int spacecraftCode = -61500;

  if (doFullCcd) {
    PushFrameCameraCcdLayout ccdLayout(spacecraftCode);
    try {
      ccdLayout.addKernel("$juno/kernels/ik/juno_junocam_v??.ti");
    }
    catch (IException &e) {
      std::string msg = "Failed to load the JunoCam Instrument Kernel required for "
                    "full ccd output.";
      throw IException(e, IException::Io, msg, _FILEINFO_);
    }
    try {
      ccdLayout.addKernel("$juno/kernels/iak/junoAddendum???.ti");
    }
    catch (IException &e) {
      std::string msg = "Failed to load the JunoCam Instrument Addendum Kernel "
                    "required for full ccd output.";
      throw IException(e, IException::Io, msg, _FILEINFO_);
    }

    //int blueOffset = ccdLayout.getFrameInfo(-61501).m_startLine;
    int blueLines = ccdLayout.getFrameInfo(-61501).m_lines;
    int greenOffset = ccdLayout.getFrameInfo(-61502).m_startLine;
    int greenLines = ccdLayout.getFrameInfo(-61502).m_lines;
    int redOffset = ccdLayout.getFrameInfo(-61503).m_startLine;

    // Determine which filters are contained in the input label and set the fullFrameLines.
    for (int i=0; i < g_filterList.size(); i++) {
      if (g_filterList[i] == "BLUE") {
        g_fullFrameLines += ccdLayout.getFrameInfo(-61501).m_lines;
        g_filterOffsetList.append(ccdLayout.getFrameInfo(-61501).m_startLine);
      }
      if (g_filterList[i] == "GREEN") {
        g_fullFrameLines += ccdLayout.getFrameInfo(-61502).m_lines;
        g_filterOffsetList.append(greenOffset-blueLines);
      }
      if (g_filterList[i] == "RED") {
        g_fullFrameLines += ccdLayout.getFrameInfo(-61503).m_lines;
        g_filterOffsetList.append(redOffset-greenLines-blueLines);
      }
      if (g_filterList[i] == "METHANE") {
        g_fullFrameLines += ccdLayout.getFrameInfo(-61504).m_lines;
        g_filterOffsetList.append(ccdLayout.getFrameInfo(-61504).m_startLine);
      }
    }

    int numFullFrames = importPds.Lines() / g_fullFrameLines;

    // Allocate this number of total cubes of the correct size
    FileName outputFileName(ui.GetCubeName("TO").toStdString());
    QString outputBaseName = QString::fromStdString(outputFileName.removeExtension().expanded());

    // Now this will be a list of output Fullframes 1-N.cub
    QFile allCubesListFile(outputBaseName + ".lis");
    if (!allCubesListFile.open(QFile::WriteOnly | QFile::Text)) {
      std::string msg = "Unable to write file [" + allCubesListFile.fileName().toStdString() + "]";
      throw IException(IException::User, msg, _FILEINFO_);
    }
    QTextStream allCubesListWriter(&allCubesListFile);

    Progress progress;
    progress.SetText("Setting up output fullframe cubes.");

    progress.SetMaximumSteps(numFullFrames);
    for (int i = 0; i < numFullFrames; i++) {
      progress.CheckStatus();
      Cube *fullFrameCube = new Cube();
      QString fullFrameNumString = QString("%1").arg(i+1, 4, 10, QChar('0'));
      fullFrameCube->setDimensions(ccdLayout.ccdSamples(), ccdLayout.ccdLines(), 1);
      fullFrameCube->setPixelType(Isis::SignedWord);
      FileName fullFrameCubeFileName(outputBaseName.toStdString()
                                     + "_" + fullFrameNumString.toStdString()
                                     + ".cub");
      fullFrameCube->create(QString::fromStdString(fullFrameCubeFileName.expanded()));
      g_outputCubes.append(fullFrameCube);
      fullFrameCube->close();
      g_outputCubeFileNames.append(QString::fromStdString(fullFrameCubeFileName.expanded()));
      allCubesListWriter << QString::fromStdString(fullFrameCubeFileName.baseName()) << ".cub\n";
    }
    progress.CheckStatus();
    allCubesListFile.close();

    // Figure out where each framelet belongs as we go through and process them.
    importPds.Progress()->SetText("Processing FullCCDFrame output cubes.");
    importPds.StartProcess(processFullFrames);
    importPds.EndProcess();

    // Write stuff
    progress.SetText("Updating labels of output cubes.");
    progress.SetMaximumSteps(numFullFrames);
    for (int i = 0; i < numFullFrames; i++) {
      progress.CheckStatus();
      for (int j = 0; j < outputLabel.findObject("IsisCube").groups(); j++) {
        if (!g_outputCubes[i]->isOpen()) {
          g_outputCubes[i]->open(g_outputCubeFileNames[i], "rw");
        }
        g_outputCubes[i]->putGroup(outputLabel.findObject("IsisCube").group(j));
      }
      // Update the labels
      Pvl *fullFrameLabel = g_outputCubes[i]->label();
      fullFrameLabel->findGroup("Instrument", PvlObject::Traverse)
                              .addKeyword(PvlKeyword("FrameNumber", std::to_string(i+1)));

      PvlGroup &bandBin = fullFrameLabel->findGroup("BandBin", PvlObject::Traverse);
      bandBin.addKeyword(PvlKeyword("FilterName", "FULLCCD"),
                         PvlObject::Replace);

      // Add filter-specific code to band bin Group
      bandBin.addKeyword(PvlKeyword("NaifIkCode", std::to_string(spacecraftCode)));

      importPds.WriteHistory(*g_outputCubes[i]);
      g_outputCubes[i]->write(origLabels);
      g_outputCubes[i]->close();
      delete g_outputCubes[i];
    }
    progress.CheckStatus();
  }
  else {
    // Process individual framelets: For now, keep processing the "old" way.
    int numSubimages = importPds.Lines() / g_frameletLines;
    int frameletsPerFilter = numSubimages / g_filterList.size();
    outputLabel.findGroup("Instrument", PvlObject::Traverse)
                         .addKeyword(PvlKeyword("NumberFramelets", std::to_string(frameletsPerFilter)));

    // get output file name and remove cube extension, if entered
    FileName outputFileName(ui.GetCubeName("TO").toStdString());
    QString outputBaseName = QString::fromStdString(outputFileName.removeExtension().expanded());

    QFile allCubesListFile(outputBaseName + ".lis");
    if (!allCubesListFile.open(QFile::WriteOnly | QFile::Text)) {
      std::string msg = "Unable to write file [" + allCubesListFile.fileName().toStdString() + "]";
      throw IException(IException::User, msg, _FILEINFO_);
    }
    QTextStream allCubesListWriter(&allCubesListFile);

    Progress progress;
    progress.SetText("Setting up output framelet cubes.");
    progress.SetMaximumSteps(numSubimages);

    for (int i = 0; i < numSubimages; i++) {
      progress.CheckStatus();
      Cube *frameletCube = new Cube();
      int frameletNumber = (i / g_filterList.size()) + 1;
      QString frameletNumString = QString("%1").arg(frameletNumber, 4, 10, QChar('0'));
      frameletCube->setDimensions(importPds.Samples(), g_frameletLines, 1);
      frameletCube->setPixelType(Isis::SignedWord);
      int filterIndex = i % g_filterList.size();
      FileName frameletCubeFileName(outputBaseName.toStdString()
                                    + "_" + g_filterList[filterIndex].toStdString()
                                    + "_" + frameletNumString.toStdString()
                                    + ".cub");

      frameletCube->create(QString::fromStdString(frameletCubeFileName.expanded()));
      g_outputCubes.append(frameletCube);
      frameletCube->close();
      g_outputCubeFileNames.append(QString::fromStdString(frameletCubeFileName.expanded()));

      QFile filterListFile(outputBaseName + "_" + g_filterList[filterIndex] + ".lis");
      if ( (frameletNumber == 1 && !filterListFile.open(QFile::WriteOnly | QFile::Text))
           || (frameletNumber > 1 && !filterListFile.open(QFile::Append | QFile::Text)) ) {
        std::string msg = "Unable to write to file [" + filterListFile.fileName().toStdString() + "]";
        throw IException(IException::User, msg, _FILEINFO_);
      }
      QTextStream filterListWriter(&filterListFile);

      allCubesListWriter << QString::fromStdString(frameletCubeFileName.baseName()) << ".cub\n";
      filterListWriter << QString::fromStdString(frameletCubeFileName.baseName()) << ".cub\n";
      filterListFile.close();
    }

    progress.CheckStatus();
    allCubesListFile.close();

    // Export the cube
    importPds.Progress()->SetText("Processing output cubes.");
    importPds.StartProcess(processFramelets);
    importPds.EndProcess();

    // Write stuff
    progress.SetText("Updating labels of output cubes.");
    progress.SetMaximumSteps(numSubimages);
    for (int i = 0; i < numSubimages; i++) {
      // re-open cube
      QString cubeFileName = g_outputCubes[i]->fileName();
      if ( !g_outputCubes[i]->isOpen() ) {
        g_outputCubes[i]->open(g_outputCubeFileNames[i], "rw");
      }
      // fromeix labels
      progress.CheckStatus();
      for (int j = 0; j < outputLabel.findObject("IsisCube").groups(); j++) {
        g_outputCubes[i]->putGroup(outputLabel.findObject("IsisCube").group(j));
      }
      int frameNumber = (i / g_filterList.size()) + 1;
      Pvl *frameletLabel = g_outputCubes[i]->label();
      frameletLabel->findGroup("Instrument", PvlObject::Traverse)
                              .addKeyword(PvlKeyword("FrameNumber", std::to_string(frameNumber)));

      int filterIndex = i % g_filterList.size();
      QString filterName = g_filterList[filterIndex];
      PvlGroup &bandBin = frameletLabel->findGroup("BandBin", PvlObject::Traverse);
      bandBin.addKeyword(PvlKeyword("FilterName", filterName.toStdString()),
                         PvlObject::Replace);

      if (filterName.compare("BLUE", Qt::CaseInsensitive) == 0) {
        spacecraftCode = -61501;
      }
      if (filterName.compare("GREEN", Qt::CaseInsensitive) == 0) {
        spacecraftCode = -61502;
      }
      if (filterName.compare("RED", Qt::CaseInsensitive) == 0) {
        spacecraftCode = -61503;
      }
      if (filterName.compare("METHANE", Qt::CaseInsensitive) == 0) {
        spacecraftCode = -61504;
      }
      // Add filter-specific code to band bin Group
      bandBin.addKeyword(PvlKeyword("NaifIkCode", std::to_string(spacecraftCode)));

      importPds.WriteHistory(*g_outputCubes[i]);
      g_outputCubes[i]->write(origLabels);
      g_outputCubes[i]->close();
      delete g_outputCubes[i];
    }
  progress.CheckStatus();
  }
  g_outputCubes.clear();

  return;
}

/**
 * Translate labels from PDS3 input to generic ISIS output. Note: Some values
 * will be updated for the individual output cubes.
 *
 * @param inputLabel The input PDS3 label.
 * @param outputLabel A reference to the output ISIS label to be updated.
 */
void translateLabel(Pvl &inputLabel, Pvl &outputLabel) {
  // Get the directory where the Juno translation tables are
  std::string transDir = "$ISISROOT/appdata/translations/";

  // Translate the Instrument group
  FileName instTransFile(transDir + "JunoJunoCamInstrument.trn");
  PvlToPvlTranslationManager instrumentXlater(inputLabel, QString::fromStdString(instTransFile.expanded()));
  instrumentXlater.Auto(outputLabel);
  PvlGroup &inst = outputLabel.findGroup("Instrument", PvlObject::Traverse);
  inst["StartTime"].addComment("Start time for the entire observation, "
                               "i.e. start time for FrameNumber 1.");
  inst["SpacecraftClockStartCount"].addComment("Start count for the entire observation, "
                                               "i.e. start count for FrameNumber 1.");
  QString instId  = QString::fromStdString(inst.findKeyword("InstrumentId"));
  QString spcName = QString::fromStdString(inst.findKeyword("SpacecraftName"));
  if (spcName.compare("JUNO", Qt::CaseInsensitive) != 0
      || instId.compare("JNC", Qt::CaseInsensitive) != 0) {

    std::string msg = "Unrecognized Spacecraft name ["
                  + spcName.toStdString()
                  + "] and instrument ID ["
                  + instId.toStdString()
                  + "]";
    throw IException(IException::User, msg, _FILEINFO_);
  }

  // Translate the BandBin group
  FileName bandBinTransFile(transDir + "JunoJunoCamBandBin.trn");
  PvlToPvlTranslationManager bandBinXlater(inputLabel, QString::fromStdString(bandBinTransFile.expanded()));
  bandBinXlater.Auto(outputLabel);
  PvlGroup &bandBin = outputLabel.findGroup("BandBin", PvlObject::Traverse);
  QString filter  = QString::fromStdString(bandBin.findKeyword("FilterName"));

  // COMPUTE FRAMELET SIZE
  QString summingKey = QString::fromStdString(outputLabel.findKeyword("SummingMode", PvlObject::Traverse)[0]);
  if (summingKey.compare("1") != 0 &&
      summingKey.compare("2") != 0) {
    std::string msg = "Invalid summing mode [" + summingKey.toStdString() + "], expected [1] or [2].";
    throw IException(IException::Unknown, msg, _FILEINFO_);
  }
  int summingMode = summingKey.toInt();
  g_frameletLines = 128 / summingMode;

  // Determine the filters
  g_filterList.clear();
  PvlKeyword filterKey = outputLabel.findKeyword("FilterName", PvlObject::Traverse);
  for (int i = 0; i < filterKey.size(); i++) {
    g_filterList.append(QString::fromStdString(filterKey[i]));
  }

  // Translate the Archive group
  FileName archiveTransFile(transDir + "JunoJunoCamArchive.trn");
  PvlToPvlTranslationManager archiveXlater(inputLabel, QString::fromStdString(archiveTransFile.expanded()));
  archiveXlater.Auto(outputLabel);
  PvlGroup &archive = outputLabel.findGroup("Archive", PvlObject::Traverse);
  iTime startTime(QString::fromStdString(inst["StartTime"][0]));
  PvlKeyword yeardoy("YearDoy", std::to_string(startTime.Year()*1000 + startTime.DayOfYear()));
  archive.addKeyword(yeardoy);
  UserInterface &ui = Application::GetUserInterface();

  //  NOTE - This needs to be the complete base name of the output filter file, not as it
  // is here, which is just the base name of the input file. It should be moved the place
  // where the file is created with the full label in it.
  PvlKeyword sourceProductId("SourceProductId", FileName(ui.GetFileName("FROM").toStdString()).baseName());
  archive.addKeyword(sourceProductId);

  // Setup the kernel group
  PvlGroup kern("Kernels");
  int spacecraftCode = -61500;
  kern += PvlKeyword("NaifFrameCode", std::to_string(spacecraftCode));
  outputLabel.findObject("IsisCube").addGroup(kern);

}

/**
 * Opens cube from g_outputCubes at provided index, closes cube at index-1 (last cube)
 */
void openNextCube(int nextCubeIndex) {
  if (nextCubeIndex >= 1) {
    if (g_outputCubes[nextCubeIndex-1]->isOpen()) {
      g_outputCubes[nextCubeIndex - 1]->close();
    }
  }
  if (!g_outputCubes[nextCubeIndex]->isOpen()) {
    g_outputCubes[nextCubeIndex]->open(g_outputCubeFileNames[nextCubeIndex], "rw");
  }
}


/**
 * Separates each of the individual frames into their own file.
 *
 * @param in A reference to the input Buffer to process.
 */
void processFramelets(Buffer &in) {
  // get the index for the correct output cube
  int outputCube = (in.Line() - 1) / g_frameletLines % g_outputCubes.size();

  // When we move to a new framlet, close the old cube and open the next one to avoid
  // having too many cubes open and hitting the open file limit.
  if( ((in.Line() - 1) % g_frameletLines) == 0 ) {
    openNextCube(outputCube);
  }

  LineManager mgr(*g_outputCubes[outputCube]);
  int outputCubeLineNumber = (in.Line()-1) % g_frameletLines + 1;
  mgr.SetLine(outputCubeLineNumber, 1);

  for (int i = 0; i < mgr.size(); i++) {
    mgr[i] = in[i];
  }
  g_outputCubes[outputCube]->write(mgr);
}


/**
 * Separates each into separate "fullframe" ccd images
 *
 * @param in A reference to the input Buffer to process.
 */
void processFullFrames(Buffer &in) {
  // get the index for the correct output cube
  int outputCube = (in.Line() - 1) / g_fullFrameLines % g_outputCubes.size();

  // When we move to a new framlet, close the old cube and open the next one to avoid
  // having too many cubes open and hitting the open file limit.
  if( ((in.Line() - 1) % g_fullFrameLines) == 0 ) {
    openNextCube(outputCube);
  }

  LineManager mgr(*g_outputCubes[outputCube]);

  int outputCubeLineNumber = ((in.Line()-1) % g_fullFrameLines);
  int filterIndex = (outputCubeLineNumber / g_frameletLines) % g_filterList.size();
  outputCubeLineNumber += g_filterOffsetList[filterIndex];

  mgr.SetLine(outputCubeLineNumber, 1);

  for (int i = 0; i < mgr.size(); i++) {
    mgr[i] = in[i];
  }
  g_outputCubes[outputCube]->write(mgr);
}
