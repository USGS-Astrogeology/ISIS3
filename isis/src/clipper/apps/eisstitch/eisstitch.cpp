/** This is free and unencumbered software released into the public domain.

The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */

#include "Application.h"
#include "Cube.h"

#include "FileName.h"
#include "FileList.h"
#include "IException.h"
#include "iTime.h"

#include "CubeAttribute.h"
#include "EisStitchFunctor.h"
#include "ProcessByBrick.h"
#include "Pvl.h"
#include "PvlKeyword.h"
#include "Table.h"
#include "TableField.h"
#include "TableRecord.h"
#include "UserInterface.h"

#include <iostream>
#include <fstream>
#include <istream>
#include <cmath>

#include "eisstitch.h"
#include <QDebug>
#include <QPair>
#include <QString>
#include <QVector>

using namespace std;

// This will eventually change to account for the pushbroom
// framelets but is just a linescanner for now
struct eisTiming {
  double start;
  int line_start;
  int lines;
  double exposureDuration;
  // Should be the start + (lineCounts * exposureTimes)
  double stop;

  eisTiming(double start, int line_start, int lines, double exposureDuration) : 
            start(start), line_start(line_start), lines(lines), exposureDuration(exposureDuration) 
  {
    stop = start + (lines * exposureDuration);
  }
};

bool compareByStartTime(const eisTiming &time1, const eisTiming &time2) {
  return time1.start < time2.start;
}

namespace Isis {

  void eisstitch(UserInterface &ui) {
    // Get the list of names of input cubes to stitch together
    FileList fileList;
    ui = Application::GetUserInterface();
    try {
      fileList.read(ui.GetFileName("FROMLIST"));
    }
    catch(IException &e) {
      QString msg = "Unable to read [" + ui.GetFileName("FROMLIST") + "]";
      IException(e, IException::User, msg, _FILEINFO_);
    }
    if (fileList.size() < 1) {
      QString msg = "The list file[" + ui.GetFileName("FROMLIST") +
                    " does not contain any filenames";
      throw IException(IException::User, msg, _FILEINFO_);
    }

    std::vector<struct eisTiming> eisTimes = {};
    QString filterName = "";
    int naifFrameCode = 0;

    for (FileName file : fileList) {
      Pvl inputCubeLabel = Pvl(file.expanded());

      PvlGroup instGroup;
      try {
        instGroup = inputCubeLabel.findGroup("Instrument", PvlObject::FindOptions::Traverse);
      }
      catch(IException &e) {
        QString msg = "Unable to find instrument group in [" + file.name() + "]";
        throw IException(e, IException::User, msg, _FILEINFO_); 
      }

      PvlGroup kernelGroup;
      try {
        kernelGroup = inputCubeLabel.findGroup("Kernels", PvlObject::FindOptions::Traverse);
      }
      catch(IException &e) {
        QString msg = "Unable to find kernels group in [" + file.name() + "]";
        throw IException(e, IException::User, msg, _FILEINFO_); 
      }

      if (naifFrameCode == 0) {
        naifFrameCode = kernelGroup.findKeyword("NaifFrameCode");
      }
      else {
        if (naifFrameCode != toInt(kernelGroup.findKeyword("NaifFrameCode"))) {
          QString msg = "NaifFrameCode [" + kernelGroup.findKeyword("NaifFrameCode")[0] + "] from image [" + file.name() + "] does "
                        "not match recorded frame code " + toString(naifFrameCode);
          throw IException(IException::User, msg, _FILEINFO_); 
        }
      }

      PvlGroup bandBinGroup;
      try {
        bandBinGroup = inputCubeLabel.findGroup("BandBin", PvlObject::FindOptions::Traverse);
      }
      catch(IException &e) {
        QString msg = "Unable to find kernels group in [" + file.name() + "]";
        throw IException(e, IException::User, msg, _FILEINFO_); 
      }
      
      if (filterName == "") {
        filterName = bandBinGroup.findKeyword("FilterName")[0];
      }
      else {
        if (filterName != bandBinGroup.findKeyword("FilterName")[0]) {
          QString msg = "FilterName [" + kernelGroup.findKeyword("FilterName")[0] + "] from image [" + file.name() + "] does "
                        "not match recorded filter name " + filterName;
          throw IException(IException::User, msg, _FILEINFO_); 
        }
      }

      PvlGroup dimGroup;
      try {
        dimGroup = inputCubeLabel.findGroup("Dimensions", PvlObject::FindOptions::Traverse);
      }
      catch(IException &e) {
        QString msg = "Unable to find instrument group in [" + file.name() + "]";
        throw IException(e, IException::User, msg, _FILEINFO_); 
      }
      Table timesTable("LineScanTimes", file.expanded());

      if(timesTable.Records() <= 0) {
        QString msg = "Table [LineScanTimes] in [";
        msg += file.expanded() + "] must not be empty";
        throw IException(IException::Unknown, msg, _FILEINFO_);
      }

      for(int i = 0; i < timesTable.Records(); i++) {
        double startTime = (double)timesTable[i][0];
        int line_start = timesTable[i][2];
        int lines = int(dimGroup.findKeyword("Lines")) - (line_start - 1);
        double exposureDuration = timesTable[i][1];
        eisTimes.push_back(eisTiming(startTime, line_start, lines, exposureDuration));
      }
    }

    // Likely need to re-order the cubes in the fileList if the eisTimes
    // are moved around
    // sort(eisTimes.begin(), eisTimes.end(), compareByStartTime);

    // Check for overlap
    for (int i = 0; i < eisTimes.size() - 1; i++) {
      if (eisTimes[i].stop > eisTimes[i + 1].start) {
        QString msg = "Image " + QString(i + 1) + " and " + QString(i + 2) + " in the image list have " + 
        "overlapping times.";
        throw IException(IException::User, msg, _FILEINFO_);
      }
    }

    // Assume the images don't know about each other
    TableField ephTimeField("EphemerisTime", TableField::Double);
    TableField expTimeField("ExposureTime", TableField::Double);
    TableField lineStartField("LineStart", TableField::Integer);

    TableRecord timesRecord;
    timesRecord += ephTimeField;
    timesRecord += expTimeField;
    timesRecord += lineStartField;

    Table timesTable("LineScanTimes", timesRecord);

    int lineCount = 1;
    for (int i = 0; i < eisTimes.size(); i++) {
      timesRecord[0] = eisTimes[i].start;
      timesRecord[1] = eisTimes[i].exposureDuration;
      timesRecord[2] = lineCount;
      lineCount += eisTimes[i].lines;
      timesTable += timesRecord;

      if (i < eisTimes.size() - 1) {
        double imageTotalTime = eisTimes[i].start + (eisTimes[i].exposureDuration * eisTimes[i].lines);
        if ( std::abs(imageTotalTime - eisTimes[i + 1].start) > std::numeric_limits<double>::epsilon()) {
          double remainingTime = eisTimes[i + 1].start - imageTotalTime;
          int blankLines = int(remainingTime / eisTimes[i].exposureDuration);
          lineCount += blankLines;
        }
      }
    }

    Pvl firstCubeLabel(fileList[0].expanded());
    PvlGroup dimGroup;
    try {
      dimGroup = firstCubeLabel.findGroup("Dimensions", PvlObject::FindOptions::Traverse);
    }
    catch(IException &e) {
      QString msg = "Unable to find instrument group in [" + fileList[0].name() + "]";
      throw IException(e, IException::User, msg, _FILEINFO_); 
    }
    int outLines = lineCount - 1;
    int outSamples = dimGroup["Samples"];

    ProcessByBrick process;
    process.PropagateLabels(false);
    process.PropagateTables(false);
    process.PropagatePolygons(false);
    process.PropagateOriginalLabel(false);
    process.SetInputCube(fileList[0].expanded(),  CubeAttributeInput());

    QString outCubeFile = ui.GetCubeName("TO");
    Cube *outCube = process.SetOutputCube(outCubeFile, CubeAttributeOutput(outCubeFile),
                          outSamples, outLines, 1);
    process.ClearInputCubes();
    Brick cubeBrick = Brick(outCube->sampleCount(), 1, 1, outCube->pixelType());

    int writeLine = 1;

    for (int i = 0; i < fileList.size(); i++) {

      Cube *inputCube = process.SetInputCube(fileList[i].expanded(), CubeAttributeInput());

      for (int line = 1; line < inputCube->lineCount() + 1; line++) {
        cubeBrick.SetBasePosition(1, line, 1);
        inputCube->read(cubeBrick);
        cubeBrick.SetBasePosition(1, writeLine, 1);
        outCube->write(cubeBrick);
        writeLine++;
      }

      if (i < fileList.size() - 1) {
        int expectedLines = (int)timesTable[i + 1][2] - 1;
        while(writeLine != expectedLines) {
          writeLine++;
        }
      }

      process.ClearInputCubes();
    }

    // Still need to write the Time table and other label data
    outCube->write(timesTable);


    // Group = Instrument
    //   SpacecraftName   = Clipper
    //   InstrumentId     = WAC-PUSHBROOM
    //   TargetName       = Mars
    //   StartTime        = "2020 APR 25 23:50:28.1879999637604"
    //   ExposureDuration = 27.547366730736318 <s>
    // End_Group

    Pvl *outLabel = outCube->label();
    PvlObject &outputIsisObject = outLabel->findObject("IsisCube");

    Pvl inputCubeLabel = Pvl(fileList[0].expanded());
    PvlGroup inputInstGroup;
    try {
      inputInstGroup = inputCubeLabel.findGroup("Instrument", PvlObject::FindOptions::Traverse);
    }
    catch(IException &e) {
      QString msg = "Unable to find instrument group in [" + fileList[0].name() + "]";
      throw IException(e, IException::User, msg, _FILEINFO_); 
    }

    PvlGroup instGroup("Instrument");
    instGroup.addKeyword(PvlKeyword("SpacecraftName", inputInstGroup["SpacecraftName"]));
    instGroup.addKeyword(PvlKeyword("InstrumentId", inputInstGroup["InstrumentId"]));
    instGroup.addKeyword(PvlKeyword("TargetName", inputInstGroup["TargetName"]));
    instGroup.addKeyword(PvlKeyword("StartTime", inputInstGroup["StartTime"]));
    outputIsisObject.addGroup(instGroup);


    PvlGroup inputKernelsGroup;
    try {
      inputKernelsGroup = inputCubeLabel.findGroup("Kernels", PvlObject::FindOptions::Traverse);
    }
    catch(IException &e) {
      QString msg = "Unable to find instrument group in [" + fileList[0].name() + "]";
      throw IException(e, IException::User, msg, _FILEINFO_); 
    }

    PvlGroup kernelsGroup("Kernels");
    kernelsGroup.addKeyword(PvlKeyword("NaifFrameCode", inputKernelsGroup["NaifFrameCode"]));
    outputIsisObject.addGroup(kernelsGroup);

    PvlGroup bandBinGroup;
    try {
      bandBinGroup = inputCubeLabel.findGroup("BandBin", PvlObject::FindOptions::Traverse);
    }
    catch(IException &e) {
      QString msg = "Unable to find instrument group in [" + fileList[0].name() + "]";
      throw IException(e, IException::User, msg, _FILEINFO_); 
    }
    outputIsisObject.addGroup(bandBinGroup);

    process.EndProcess();
  }
}