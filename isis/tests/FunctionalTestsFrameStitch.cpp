#include <exception>
#include <memory>

#include "framestitch.h"
#include "Brick.h"
#include "IString.h"
#include "LineManager.h"
#include "SpecialPixel.h"
#include "Statistics.h"
#include "Table.h"

#include "TestUtilities.h"
#include "CameraFixtures.h"
#include "gmock/gmock.h"

#include "gmock/gmock.h"

using namespace Isis;
using namespace testing;


static QString APP_XML = QString::fromStdString(FileName("$ISISROOT/bin/xml/framestitch.xml").expanded());

TEST_F(PushFramePair, FunctionalTestFramestitchManualHeight) {
  QString outCubePath = tempDir.path() + "/stitched.cub";
  QVector<QString> args = {
        "EVEN="+evenCube->fileName(),
        "ODD="+oddCube->fileName(),
        "FRAMEHEIGHT="+QString::number(frameHeight),
        "TO="+outCubePath};

  UserInterface ui(APP_XML, args);

  framestitch(ui);

  Cube outCube(outCubePath.toStdString());
  std::shared_ptr<Statistics> bandStats(outCube.statistics());
  EXPECT_EQ(bandStats->Minimum(), 1);
  EXPECT_EQ(bandStats->Maximum(), numFrames);
  EXPECT_DOUBLE_EQ(bandStats->Average(), (numFrames + 1) / 2.0);
  EXPECT_EQ(bandStats->NullPixels(), 0);

  // Check the order on the first two and last two frames
  Brick outBrick(numSamps, frameHeight, numBands, outCube.pixelType());
  outBrick.SetBasePosition(1,1,1);
  outCube.read(outBrick);
  Statistics frameStats;
  frameStats.AddData(outBrick.DoubleBuffer(), outBrick.size());
  EXPECT_EQ(frameStats.Minimum(), 1);
  EXPECT_EQ(frameStats.Maximum(), 1);
  frameStats.Reset();

  outBrick.SetBasePosition(1,frameHeight + 1,1);
  outCube.read(outBrick);
  frameStats.AddData(outBrick.DoubleBuffer(), outBrick.size());
  EXPECT_EQ(frameStats.Minimum(), 2);
  EXPECT_EQ(frameStats.Maximum(), 2);
  frameStats.Reset();

  outBrick.SetBasePosition(1,8*frameHeight + 1,1);
  outCube.read(outBrick);
  frameStats.AddData(outBrick.DoubleBuffer(), outBrick.size());
  EXPECT_EQ(frameStats.Minimum(), 9);
  EXPECT_EQ(frameStats.Maximum(), 9);
  frameStats.Reset();

  outBrick.SetBasePosition(1,9*frameHeight + 1,1);
  outCube.read(outBrick);
  frameStats.AddData(outBrick.DoubleBuffer(), outBrick.size());
  EXPECT_EQ(frameStats.Minimum(), 10);
  EXPECT_EQ(frameStats.Maximum(), 10);
  frameStats.Reset();
}

TEST_F(PushFramePair, FunctionalTestFramestitchAutoHeight) {
  QString manualCubePath = tempDir.path() + "/manual.cub";
  QVector<QString> manualArgs = {
        "EVEN="+evenCube->fileName(),
        "ODD="+oddCube->fileName(),
        "FRAMEHEIGHT="+QString::number(frameHeight),
        "TO="+manualCubePath};

  UserInterface manualUI(APP_XML, manualArgs);

  framestitch(manualUI);

  QString autoCubePath = tempDir.path() + "/auto.cub";
  QVector<QString> autoArgs = {
        "EVEN="+evenCube->fileName(),
        "ODD="+oddCube->fileName(),
        "TO="+autoCubePath};

  UserInterface autoUI(APP_XML, autoArgs);

  framestitch(autoUI);

  Cube manualCube(manualCubePath.toStdString());
  Cube autoCube(autoCubePath.toStdString());

  std::shared_ptr<Statistics> manualStats(manualCube.statistics());
  std::shared_ptr<Statistics> autoStats(autoCube.statistics());
  EXPECT_EQ(manualStats->Minimum(), autoStats->Minimum());
  EXPECT_EQ(manualStats->Maximum(), autoStats->Maximum());
  EXPECT_EQ(manualStats->Average(), autoStats->Average());
  EXPECT_EQ(manualStats->NullPixels(), autoStats->NullPixels());

  Brick outBrick(numSamps, frameHeight, numBands, manualCube.pixelType());
  outBrick.SetBasePosition(1,1,1);
  Statistics manualFrameStats, autoFrameStats;
  manualCube.read(outBrick);
  manualFrameStats.AddData(outBrick.DoubleBuffer(), outBrick.size());
  autoCube.read(outBrick);
  autoFrameStats.AddData(outBrick.DoubleBuffer(), outBrick.size());
  EXPECT_EQ(manualFrameStats.Minimum(), autoFrameStats.Minimum());
  EXPECT_EQ(manualFrameStats.Maximum(), autoFrameStats.Maximum());
}

TEST_F(PushFramePair, FunctionalTestFramestitchFlip) {
  QString outCubePath = tempDir.path() + "/stitched.cub";
  QVector<QString> args = {
        "EVEN="+evenCube->fileName(),
        "ODD="+oddCube->fileName(),
        "FRAMEHEIGHT="+QString::number(frameHeight),
        "TO="+outCubePath,
        "FLIP=YES"};

  UserInterface ui(APP_XML, args);

  framestitch(ui);

  Cube outCube(outCubePath.toStdString());

  // Check the order on the first two and last two frames
  Brick outBrick(numSamps, frameHeight, numBands, outCube.pixelType());
  outBrick.SetBasePosition(1,1,1);
  outCube.read(outBrick);
  Statistics frameStats;
  frameStats.AddData(outBrick.DoubleBuffer(), outBrick.size());
  EXPECT_EQ(frameStats.Minimum(), 10);
  EXPECT_EQ(frameStats.Maximum(), 10);
  frameStats.Reset();

  outBrick.SetBasePosition(1,frameHeight + 1,1);
  outCube.read(outBrick);
  frameStats.AddData(outBrick.DoubleBuffer(), outBrick.size());
  EXPECT_EQ(frameStats.Minimum(), 9);
  EXPECT_EQ(frameStats.Maximum(), 9);
  frameStats.Reset();

  outBrick.SetBasePosition(1,8*frameHeight + 1,1);
  outCube.read(outBrick);
  frameStats.AddData(outBrick.DoubleBuffer(), outBrick.size());
  EXPECT_EQ(frameStats.Minimum(), 2);
  EXPECT_EQ(frameStats.Maximum(), 2);
  frameStats.Reset();

  outBrick.SetBasePosition(1,9*frameHeight + 1,1);
  outCube.read(outBrick);
  frameStats.AddData(outBrick.DoubleBuffer(), outBrick.size());
  EXPECT_EQ(frameStats.Minimum(), 1);
  EXPECT_EQ(frameStats.Maximum(), 1);
  frameStats.Reset();

  ASSERT_TRUE(outCube.hasGroup("Instrument"));
  ASSERT_TRUE(outCube.group("Instrument").hasKeyword("DataFlipped"));
  EXPECT_TRUE(std::stoi(outCube.group("Instrument")["DataFlipped"]));
}

TEST_F(FlippedPushFramePair, FunctionalTestFramestitchNoFlip) {
  QString outCubePath = tempDir.path() + "/stitched.cub";
  QVector<QString> args = {
        "EVEN="+evenCube->fileName(),
        "ODD="+oddCube->fileName(),
        "FRAMEHEIGHT="+QString::number(frameHeight),
        "TO="+outCubePath};

  UserInterface ui(APP_XML, args);

  framestitch(ui);

  Cube outCube(outCubePath.toStdString());

  // Check the order on the first two and last two frames
  Brick outBrick(numSamps, frameHeight, numBands, outCube.pixelType());
  outBrick.SetBasePosition(1,1,1);
  outCube.read(outBrick);
  Statistics frameStats;
  frameStats.AddData(outBrick.DoubleBuffer(), outBrick.size());
  EXPECT_EQ(frameStats.Minimum(), 10);
  EXPECT_EQ(frameStats.Maximum(), 10);
  frameStats.Reset();

  outBrick.SetBasePosition(1,frameHeight + 1,1);
  outCube.read(outBrick);
  frameStats.AddData(outBrick.DoubleBuffer(), outBrick.size());
  EXPECT_EQ(frameStats.Minimum(), 9);
  EXPECT_EQ(frameStats.Maximum(), 9);
  frameStats.Reset();

  outBrick.SetBasePosition(1,8*frameHeight + 1,1);
  outCube.read(outBrick);
  frameStats.AddData(outBrick.DoubleBuffer(), outBrick.size());
  EXPECT_EQ(frameStats.Minimum(), 2);
  EXPECT_EQ(frameStats.Maximum(), 2);
  frameStats.Reset();

  outBrick.SetBasePosition(1,9*frameHeight + 1,1);
  outCube.read(outBrick);
  frameStats.AddData(outBrick.DoubleBuffer(), outBrick.size());
  EXPECT_EQ(frameStats.Minimum(), 1);
  EXPECT_EQ(frameStats.Maximum(), 1);
  frameStats.Reset();

  ASSERT_TRUE(outCube.hasGroup("Instrument"));
  ASSERT_TRUE(outCube.group("Instrument").hasKeyword("DataFlipped"));
  EXPECT_TRUE(std::stoi(outCube.group("Instrument")["DataFlipped"]));
}

TEST_F(FlippedPushFramePair, FunctionalTestFramestitchFlip) {
  QString outCubePath = tempDir.path() + "/stitched.cub";
  QVector<QString> args = {
        "EVEN="+evenCube->fileName(),
        "ODD="+oddCube->fileName(),
        "FRAMEHEIGHT="+QString::number(frameHeight),
        "TO="+outCubePath,
        "FLIP=YES"};

  UserInterface ui(APP_XML, args);

  framestitch(ui);

  Cube outCube(outCubePath.toStdString());

  // Check the order on the first two and last two frames
  Brick outBrick(numSamps, frameHeight, numBands, outCube.pixelType());
  outBrick.SetBasePosition(1,1,1);
  outCube.read(outBrick);
  Statistics frameStats;
  frameStats.AddData(outBrick.DoubleBuffer(), outBrick.size());
  EXPECT_EQ(frameStats.Minimum(), 1);
  EXPECT_EQ(frameStats.Maximum(), 1);
  frameStats.Reset();

  outBrick.SetBasePosition(1,frameHeight + 1,1);
  outCube.read(outBrick);
  frameStats.AddData(outBrick.DoubleBuffer(), outBrick.size());
  EXPECT_EQ(frameStats.Minimum(), 2);
  EXPECT_EQ(frameStats.Maximum(), 2);
  frameStats.Reset();

  outBrick.SetBasePosition(1,8*frameHeight + 1,1);
  outCube.read(outBrick);
  frameStats.AddData(outBrick.DoubleBuffer(), outBrick.size());
  EXPECT_EQ(frameStats.Minimum(), 9);
  EXPECT_EQ(frameStats.Maximum(), 9);
  frameStats.Reset();

  outBrick.SetBasePosition(1,9*frameHeight + 1,1);
  outCube.read(outBrick);
  frameStats.AddData(outBrick.DoubleBuffer(), outBrick.size());
  EXPECT_EQ(frameStats.Minimum(), 10);
  EXPECT_EQ(frameStats.Maximum(), 10);
  frameStats.Reset();

  ASSERT_TRUE(outCube.hasGroup("Instrument"));
  ASSERT_TRUE(outCube.group("Instrument").hasKeyword("DataFlipped"));
  EXPECT_FALSE(std::stoi(outCube.group("Instrument")["DataFlipped"]));
}

TEST_F(PushFramePair, FunctionalTestFramestitchDifferentObservations) {
  evenCube->group("Instrument")["StartTime"] = "2008-06-14T13:45:12.865707";
  evenCube->reopen("rw");

  QVector<QString> args = {
        "EVEN="+evenCube->fileName(),
        "ODD="+oddCube->fileName(),
        "FRAMEHEIGHT="+QString::number(frameHeight),
        "TO="+tempDir.path() + "/stitched.cub"};

  UserInterface ui(APP_XML, args);

  try {
    framestitch(ui);
    FAIL() << "Expected an IException";
  }
  catch(IException &e) {
    EXPECT_THAT(e.what(), testing::HasSubstr("Even and odd cube start times must match"));
  }
  catch(std::exception &e) {
    FAIL() << "Expected an IException, got exception with error " << e.what() << std::endl;
  }
}

TEST_F(FlippedPushFramePair, FunctionalTestFramestitchDifferentFlipping) {
  evenCube->group("Instrument")["DataFlipped"] = "False";
  evenCube->reopen("rw");

  QVector<QString> args = {
        "EVEN="+evenCube->fileName(),
        "ODD="+oddCube->fileName(),
        "FRAMEHEIGHT="+QString::number(frameHeight),
        "TO="+tempDir.path() + "/stitched.cub"};

  UserInterface ui(APP_XML, args);

  try {
    framestitch(ui);
    FAIL() << "Expected an IException";
  }
  catch(IException &e) {
    EXPECT_THAT(e.what(), testing::HasSubstr("Both input cubes must be flipped or not flipped"));
  }
  catch(std::exception &e) {
    FAIL() << "Expected an IException, got exception with error " << e.what() << std::endl;
  }
}

TEST_F(PushFramePair, FunctionalTestFramestitchAutoNoNulls) {
  LineManager lineWriter(*evenCube);
  for (int i = 0; i < lineWriter.size(); i++) {
    lineWriter[i] = 0;
  }
  for (int bandIndex = 1; bandIndex <= evenCube->bandCount(); bandIndex++) {
    for (int lineIndex = 1; lineIndex <= evenCube->lineCount(); lineIndex++) {
      lineWriter.SetLine(lineIndex, bandIndex);
      evenCube->write(lineWriter);
    }
  }
  evenCube->reopen("rw");

  QVector<QString> args = {
        "EVEN="+evenCube->fileName(),
        "ODD="+oddCube->fileName(),
        "TO="+tempDir.path() + "/stitched.cub"};

  UserInterface ui(APP_XML, args);

  try {
    framestitch(ui);
    FAIL() << "Expected an IException";
  }
  catch(IException &e) {
    EXPECT_THAT(e.what(), testing::HasSubstr("Failed to find any NULL frames in cube"));
    EXPECT_THAT(e.what(), testing::HasSubstr("Please manually enter the frame height"));
  }
  catch(std::exception &e) {
    FAIL() << "Expected an IException, got exception with error " << e.what() << std::endl;
  }
}

TEST_F(PushFramePair, FunctionalTestFramestitchAutoMismatchedHeights) {
  LineManager lineWriter(*evenCube);
  for (int i = 0; i < lineWriter.size(); i++) {
    lineWriter[i] = Null;
  }
  for (int bandIndex = 1; bandIndex <= evenCube->bandCount(); bandIndex++) {
    for (int lineIndex = 1; lineIndex <= evenCube->lineCount(); lineIndex++) {
      lineWriter.SetLine(lineIndex, bandIndex);
      evenCube->write(lineWriter);
    }
  }

  Brick frameBrick(numSamps, 2*frameHeight, numBands, evenCube->pixelType());
  for (int frameIndex = 1; frameIndex < numFrames; frameIndex+=2) {
    for (int brickIndex = 0; brickIndex < frameBrick.size(); brickIndex++) {
      frameBrick[brickIndex] = numFrames - frameIndex;
    }
    frameBrick.SetBasePosition(1,2 * frameIndex * frameHeight + 1,1);
    evenCube->write(frameBrick);
  }
  evenCube->reopen("rw");

  QVector<QString> args = {
        "EVEN="+evenCube->fileName(),
        "ODD="+oddCube->fileName(),
        "TO="+tempDir.path() + "/stitched.cub"};

  UserInterface ui(APP_XML, args);

  try {
    framestitch(ui);
    FAIL() << "Expected an IException";
  }
  catch(IException &e) {
    EXPECT_THAT(e.what(), testing::HasSubstr("Computed frame heights for even cube "
                                             "[24] and odd cube [12] do not match"));
  }
  catch(std::exception &e) {
    FAIL() << "Expected an IException, got exception with error " << e.what() << std::endl;
  }
}

TEST_F(PushFramePair, FunctionalTestFramestitchAutoDifferentHeights) {
  // Make frame 7 NULL so that there is a 36 line null frame
  Brick frameBrick(numSamps, frameHeight, numBands, oddCube->pixelType());
  for (int brickIndex = 0; brickIndex < frameBrick.size(); brickIndex++) {
    frameBrick[brickIndex] = Null;
  }
  frameBrick.SetBasePosition(1,6 * frameHeight + 1,1);
  oddCube->write(frameBrick);
  oddCube->reopen("rw");

  QVector<QString> args = {
        "EVEN="+evenCube->fileName(),
        "ODD="+oddCube->fileName(),
        "TO="+tempDir.path() + "/stitched.cub"};

  UserInterface ui(APP_XML, args);

  try {
    framestitch(ui);
    FAIL() << "Expected an IException";
  }
  catch(IException &e) {
    EXPECT_THAT(e.what(), testing::HasSubstr("Found different frame heights between [12] "
                                             "and [36] lines in cube"));
    EXPECT_THAT(e.what(), testing::HasSubstr("Please manually enter the frame height"));
  }
  catch(std::exception &e) {
    FAIL() << "Expected an IException, got exception with error " << e.what() << std::endl;
  }
}

// Test removing a total of this many lines from each framelet
// (half at the top of each framelet and half at the bottom).
TEST_F(PushFramePair, FunctionalTestFramestitchRemoveOverlap) {
  QString outCubePath = tempDir.path() + "/stitched.cub";
  int numLinesOverlap = 2;
  QVector<QString> args = {
        "EVEN="+evenCube->fileName(),
        "ODD="+oddCube->fileName(),
        "FRAMEHEIGHT="+QString::number(frameHeight),
        "NUM_LINES_OVERLAP=" + QString::number(numLinesOverlap),
        "TO="+outCubePath};

  UserInterface ui(APP_XML, args);

  framestitch(ui);

  Cube outCube(outCubePath.toStdString());
  std::shared_ptr<Statistics> bandStats(outCube.statistics());
  EXPECT_EQ(bandStats->Minimum(), 1);
  EXPECT_EQ(bandStats->Maximum(), numFrames);
  EXPECT_DOUBLE_EQ(bandStats->Average(), (numFrames + 1) / 2.0);
  EXPECT_EQ(bandStats->NullPixels(), 0);

  // The output cube should have fewer lines
  EXPECT_EQ(evenCube->lineCount(), outCube.lineCount() + numFrames * numLinesOverlap);
}
