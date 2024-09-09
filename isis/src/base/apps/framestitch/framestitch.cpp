#include "framestitch.h"

#include <QString>
#include <iostream>

#include "Brick.h"
#include "Buffer.h"
#include "Cube.h"
#include "CubeAttribute.h"
#include "Camera.h"
#include "IException.h"
#include "IString.h"
#include "LineManager.h"
#include "ProcessByBrick.h"
#include "PvlGroup.h"
#include "PvlKeyword.h"
#include "Statistics.h"
#include "UserInterface.h"

using namespace std;
using namespace Isis;

namespace Isis {

  /**
   * Helper function to count the height of frames in a cube.
   *
   * Iterates over the lines in the cube and counts sequential all NULL lines
   * assuming these are frames that have been removed from the cube.
   *
   * If different frame heights are computed within the cube or there are
   * no all null lines in the cube an error is raised.
   */
  int computeFrameHeight(Cube *cube) {
    // Use a Statistics object to track min/max height count
    Statistics frameHeights;
    LineManager cubeLine(*cube);
    int currentFrameHeight = 0;
    for (int line = 1; line <= cube->lineCount(); line++) {
      cubeLine.SetLine(line);
      cube->read(cubeLine);
      Statistics lineStats;
      lineStats.AddData(cubeLine.DoubleBuffer(), cubeLine.size());
      // If the line is all NULL add it to the current frame count
      if (lineStats.TotalPixels() == lineStats.NullPixels()) {
        ++currentFrameHeight;
      }
      // If the line has non-NULL pixels and we have previously counted
      // some all null lines add the count to our histogram and reset
      else if (currentFrameHeight > 0) {
        frameHeights.AddData(currentFrameHeight);
        currentFrameHeight = 0;
      }
    }

    // If the last line is part of a NULL frame handle it now
    if (currentFrameHeight > 0) {
      frameHeights.AddData(currentFrameHeight);
    }

    if (frameHeights.TotalPixels() == 0) {
      std::string msg = "Failed to find any NULL frames in cube [" + cube->fileName().toStdString() + "]."
                    "Please manually enter the frame height.";
      throw IException(IException::User, msg, _FILEINFO_);
    }
    if (frameHeights.Minimum() != frameHeights.Maximum()) {
      std::string msg = "Found different frame heights between [" + toString((int)frameHeights.Minimum())
                  + "] and [" + toString((int)frameHeights.Maximum()) + "] lines in cube ["
                  + cube->fileName().toStdString() + "]. Please manually enter the frame height.";
      throw IException(IException::User, msg, _FILEINFO_);
    }

    // The statistics object contains only a single value at this point
    // so minimum=average=maximum
    return frameHeights.Average();
  }

  /**
   * Combine even and odd cubes from a push frame image into a single cube.
   *
   * @param ui The User Interface to parse the parameters from
   */
  void framestitch(UserInterface &ui) {
    ProcessByBrick process;
    process.PropagatePolygons(false);

    // It is very important that the odd cube gets added first as later on
    // we'll use frameNumber % 2 to index the input cube vector
    QString oddCubeFile = ui.GetCubeName("ODD");
    Cube* oddCube = process.SetInputCube(oddCubeFile,
                                         CubeAttributeInput(oddCubeFile.toStdString()));
    QString evenCubeFile = ui.GetCubeName("EVEN");
    Cube* evenCube = process.SetInputCube(evenCubeFile,
                                          CubeAttributeInput(evenCubeFile.toStdString()));


    // Check that all the inputs are valid
    if (evenCube->sampleCount() != oddCube->sampleCount() ||
        evenCube->lineCount() != oddCube->lineCount() ||
        evenCube->bandCount() != oddCube->bandCount()) {
      std::string msg = "Even and odd cube dimensions must match.";
      throw IException(IException::User, msg, _FILEINFO_);
    }

    bool inputFlipped = false;

    if (evenCube->hasGroup("Instrument") && oddCube->hasGroup("Instrument")) {
      const PvlGroup &evenInst = evenCube->group("Instrument");
      const PvlGroup &oddInst = oddCube->group("Instrument");
      // Use the start time as an indicator of being the same original image
      if (QString::compare(QString::fromStdString(evenInst["StartTime"]), QString::fromStdString(oddInst["StartTime"])) != 0) {
        std::string msg = "Even and odd cube start times must match.";
        throw IException(IException::User, msg, _FILEINFO_);
      }

      if (evenInst.hasKeyword("DataFlipped") && oddInst.hasKeyword("DataFlipped")) {
        if (toBool(evenInst["DataFlipped"]) != toBool(oddInst["DataFlipped"])) {
          std::string msg = "Both input cubes must be flipped or not flipped. Cannot combine "
                        "a flipped and unflipped cube.";
          throw IException(IException::User, msg, _FILEINFO_);
        }
        inputFlipped = toBool(evenInst["DataFlipped"]);
      }
    }

    int frameHeight = 1;
    if (ui.WasEntered("frameHeight")) {
      frameHeight = ui.GetInteger("FRAMEHEIGHT");
    } else {
      // User didn't pass the size of the frames so attempt to infer it from
      // the cubes
      int evenFrameHeight = computeFrameHeight(evenCube);
      int oddFrameHeight = computeFrameHeight(oddCube);

      if (evenFrameHeight != oddFrameHeight) {
        std::string msg = "Computed frame heights for even cube [" + toString(evenFrameHeight)
                    + "] and odd cube [" + toString(oddFrameHeight) + "] do not match.";
        throw IException(IException::User, msg, _FILEINFO_);
      }

      frameHeight = evenFrameHeight;
    }
    int numFrames = evenCube->lineCount() / frameHeight;

    int numLinesOverlap = 0; // The default value
    if (ui.WasEntered("NUM_LINES_OVERLAP"))
      numLinesOverlap = ui.GetInteger("NUM_LINES_OVERLAP");

    if (numLinesOverlap % 2 != 0 || numLinesOverlap < 0) {
      std::string msg = "Expecting a non-negative and even value for NUM_LINES_OVERLAP.";
      throw IException(IException::User, msg, _FILEINFO_);
    }

    int reducedFrameHeight = frameHeight - numLinesOverlap;

    QString outCubeFile = ui.GetCubeName("TO");
    Cube *outCube = process.SetOutputCube(
          outCubeFile, CubeAttributeOutput(outCubeFile.toStdString()),
          evenCube->sampleCount(), numFrames * reducedFrameHeight, evenCube->bandCount());

    // If there's an even number of frames and the inputs are flipped, we have to
    // swap even and odd because the first frame in the even cube is valid and the
    // first frame in the odd cube is now all NULL.
    //
    //  Before   --Flip-->  After
    // Even  Odd          Even  Odd
    // 0000  ####         ####  0000
    // ####  0000         0000  ####
    // 0000  ####         ####  0000
    // ####  0000         0000  ####
    //
    bool swapInputCubes = inputFlipped && numFrames % 2 == 0;

    // Processing setup
    process.SetBrickSize(evenCube->sampleCount(), frameHeight, evenCube->bandCount());

    // Put together the frames from the two input cubes. Note that we
    // wipe a total of numLinesOverlap lines from each frame as we do
    // so, by simply not reading in the lines we don't want to keep.
    for (int frame = 0; frame < numFrames; frame++) {

      Brick buff(evenCube->sampleCount(), reducedFrameHeight, evenCube->bandCount(),
                   evenCube->pixelType());

      // Set reading position
      buff.SetBasePosition(1, frame * frameHeight + numLinesOverlap/2 + 1, 1);

      int inIndex = frame % 2;
      if (swapInputCubes)
        inIndex = 1 - inIndex;

      if (inIndex == 0)
        oddCube->read(buff);
      else
        evenCube->read(buff);

      // Set writing position
      buff.SetBasePosition(1, frame * reducedFrameHeight + 1, 1);

      outCube->write(buff);
    }

    // Update the output label
    if (!outCube->hasGroup("Instrument")) {
      outCube->putGroup(PvlGroup("Instrument"));
    }
    PvlGroup &outInst = outCube->group("Instrument");
    if (!outInst.hasKeyword("Framelets")) {
      outInst.addKeyword(PvlKeyword("Framelets"));
    }
    outInst["Framelets"].setValue("All");

    // Flip the output cube if requested
    if (ui.GetBoolean("FLIP")) {
      // Need a temporary buffer to help with the swapping of top and
      // bottom portions of outCube.
      Brick topBuff(outCube->sampleCount(), reducedFrameHeight, outCube->bandCount(), outCube->pixelType());
      Brick botBuff(outCube->sampleCount(), reducedFrameHeight, outCube->bandCount(), outCube->pixelType());
      Brick tmpBuff(outCube->sampleCount(), reducedFrameHeight, outCube->bandCount(), outCube->pixelType());

      for (int frame = 0; frame < numFrames / 2; frame++) {
        topBuff.SetBasePosition(1,frame * reducedFrameHeight + 1,1);
        outCube->read(topBuff);
        botBuff.SetBasePosition(1,outCube->lineCount() - (frame + 1) * reducedFrameHeight + 1,1);
        outCube->read(botBuff);

        tmpBuff.Copy(topBuff);
        topBuff.Copy(botBuff);
        outCube->write(topBuff);
        botBuff.Copy(tmpBuff);
        outCube->write(botBuff);
      }

      if (!outInst.hasKeyword("DataFlipped")) {
        outInst.addKeyword(PvlKeyword("DataFlipped"));
      }
      outInst["DataFlipped"].setValue(std::to_string(!inputFlipped));
    }

    // Record numLinesOverlap
    if (!outInst.hasKeyword("NumLinesOverlap"))
      outInst.addKeyword(PvlKeyword("NumLinesOverlap"));
    outInst["numLinesOverlap"].setValue(std::to_string(numLinesOverlap));

    process.EndProcess();

    return;
  }

} // end namespace Isis
