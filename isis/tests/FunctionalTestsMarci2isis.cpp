#include <QTemporaryDir>

#include "marci2isis.h"
#include "Pvl.h"
#include "PvlGroup.h"
#include "TestUtilities.h"

#include "gtest/gtest.h"

using namespace Isis;

static QString APP_XML = QString::fromStdString(FileName("$ISISROOT/bin/xml/marci2isis.xml").expanded());

TEST(Marci2Isis, Marci2isisTestDefault) {
  Pvl appLog;
  QTemporaryDir prefix;
  QString cubeFileName = prefix.path() + "/marci2isis_out.cub";
  QVector<QString> args = {"from=data/marci2isis/MOI_000009_0294_MU_00N044W_cropped.IMG", "to=" + cubeFileName };
  UserInterface options(APP_XML, args);
  marci2isis(options, &appLog);

  QString cubeFileNameEven = prefix.path() + "/marci2isis_out.even.cub";
  QString cubeFileNameOdd = prefix.path() + "/marci2isis_out.odd.cub";

  Cube cubeEven(cubeFileNameEven.toStdString());
  Cube cubeOdd(cubeFileNameOdd.toStdString());
  Pvl *evenLabel = cubeEven.label();
  Pvl *oddLabel = cubeOdd.label();

  ASSERT_EQ((int)evenLabel->findObject("OriginalLabel").findKeyword("Bytes"), (int)oddLabel->findObject("OriginalLabel").findKeyword("Bytes"));

  // Dimensions Group
  PvlGroup &evenDimensions = evenLabel->findGroup("Dimensions", Pvl::Traverse);
  PvlGroup &oddDimensions = oddLabel->findGroup("Dimensions", Pvl::Traverse);

  ASSERT_EQ((int)evenDimensions["Samples"], 128);
  ASSERT_EQ((int)evenDimensions["Lines"], 74);
  ASSERT_EQ((int)evenDimensions["Bands"], 2);
  ASSERT_EQ(evenDimensions["Samples"], oddDimensions["Samples"]);
  ASSERT_EQ(evenDimensions["Lines"], oddDimensions["Lines"]);
  ASSERT_EQ(evenDimensions["Bands"], oddDimensions["Bands"]);

  // Pixels Group
  PvlGroup &evenPixels = evenLabel->findGroup("Pixels", Pvl::Traverse);
  PvlGroup &oddPixels = oddLabel->findGroup("Pixels", Pvl::Traverse);

  EXPECT_PRED_FORMAT2(AssertStringsEqual, evenPixels["Type"][0], oddPixels["Type"][0]);
  EXPECT_PRED_FORMAT2(AssertStringsEqual, evenPixels["ByteOrder"][0], oddPixels["ByteOrder"][0]);
  ASSERT_EQ((double)evenPixels["Base"], (double)oddPixels["Base"]);
  ASSERT_EQ((double)evenPixels["Multiplier"], (double)oddPixels["Multiplier"]);

  // Instrument Group
  PvlGroup &evenInst = evenLabel->findGroup("Instrument", Pvl::Traverse);
  PvlGroup &oddInst = oddLabel->findGroup("Instrument", Pvl::Traverse);

  ASSERT_EQ(evenInst["Framelets"][0], "Even");
  ASSERT_EQ(oddInst["Framelets"][0], "Odd");

  ASSERT_EQ(evenInst["SpacecraftName"][0], "MARS RECONNAISSANCE ORBITER");
  ASSERT_EQ(evenInst["InstrumentId"][0], "Marci" );
  ASSERT_EQ(evenInst["TargetName"][0], "MARS" );
  ASSERT_EQ((int)evenInst["SummingMode"], 8);
  ASSERT_EQ(evenInst["StartTime"][0], "2006-03-24T04:25:53.096000" );
  ASSERT_EQ(evenInst["StopTime"][0], "2006-03-24T04:55:48.296000" );
  ASSERT_EQ(evenInst["SpacecraftClockCount"][0], "827641567:30" );
  ASSERT_EQ((int)evenInst["DataFlipped"], 1 );
  ASSERT_EQ((int)evenInst["ColorOffset"], 2 );
  ASSERT_DOUBLE_EQ(double(evenInst["InterframeDelay"]), 3.2);
  ASSERT_DOUBLE_EQ(double(evenInst["ExposureDuration"]), 3.112237);
  ASSERT_EQ((int)evenInst["FrameNumber"], 0);
  ASSERT_DOUBLE_EQ(double(evenInst["VariableExposureDuration"]), 3112.24);

  ASSERT_EQ(evenInst["SpacecraftName"][0], oddInst["SpacecraftName"][0]);
  ASSERT_EQ(evenInst["InstrumentId"][0], oddInst["InstrumentId"][0]);
  ASSERT_EQ(evenInst["TargetName"][0], oddInst["TargetName"][0]);
  ASSERT_EQ((int)evenInst["SummingMode"], (int)oddInst["SummingMode"]);
  ASSERT_EQ(evenInst["StartTime"][0], oddInst["StartTime"][0]);
  ASSERT_EQ(evenInst["StopTime"][0], oddInst["StopTime"][0]);
  ASSERT_EQ(evenInst["SpacecraftClockCount"][0], oddInst["SpacecraftClockCount"][0]);
  ASSERT_EQ((int)evenInst["DataFlipped"], (int)oddInst["DataFlipped"]);
  ASSERT_EQ((int)evenInst["ColorOffset"], (int)oddInst["ColorOffset"]);
  ASSERT_DOUBLE_EQ(double(evenInst["InterframeDelay"]), double(oddInst["InterframeDelay"]));
  ASSERT_DOUBLE_EQ(double(evenInst["ExposureDuration"]), double(oddInst["ExposureDuration"]));
  ASSERT_EQ(evenInst["StartTime"][0], oddInst["StartTime"][0]);
  ASSERT_EQ(evenInst["StopTime"][0], oddInst["StopTime"][0]);
  ASSERT_EQ((int)evenInst["DataFlipped"], (int)oddInst["DataFlipped"]);

  // Archive Group
  PvlGroup &evenArchive = evenLabel->findGroup("Archive", Pvl::Traverse);
  PvlGroup &oddArchive = oddLabel->findGroup("Archive", Pvl::Traverse);
  QString ratDesc = "Post-MOI image of Argyre and Mare Erythraeum region";

  ASSERT_EQ(evenArchive["ProductId"][0], "MOI_000009_0294_MU_00N044W" );
  ASSERT_EQ(evenArchive["OriginalProductId"][0], "4A_05_0001000200" );
  ASSERT_EQ((int)evenArchive["OrbitNumber"], 9);
  ASSERT_EQ(evenArchive["SampleBitModeId"][0], "SQROOT" );
  ASSERT_DOUBLE_EQ(double(evenArchive["FocalPlaneTemperature"]), 240.9 );
  EXPECT_PRED_FORMAT2(AssertStringsEqual, evenArchive["RationaleDesc"][0], ratDesc.toStdString());

  ASSERT_EQ(evenArchive["ProductId"][0], oddArchive["ProductId"][0]);
  ASSERT_EQ(evenArchive["OriginalProductId"][0], oddArchive["OriginalProductId"][0]);
  ASSERT_EQ((int)evenArchive["OrbitNumber"], (int)oddArchive["OrbitNumber"]);
  ASSERT_EQ(evenArchive["SampleBitModeId"][0], oddArchive["SampleBitModeId"][0]);
  ASSERT_DOUBLE_EQ(double(evenArchive["FocalPlaneTemperature"]), double(oddArchive["FocalPlaneTemperature"]));
  ASSERT_EQ(evenArchive["RationaleDesc"][0], oddArchive["RationaleDesc"][0]);


  // BandBin Group
  PvlGroup &evenBandBin = evenLabel->findGroup("BandBin", Pvl::Traverse);
  PvlGroup &oddBandBin = oddLabel->findGroup("BandBin", Pvl::Traverse);

  ASSERT_EQ(evenBandBin["FilterName"][0], "SHORT_UV");
  ASSERT_EQ(evenBandBin["FilterName"][1], "LONG_UV");
  ASSERT_EQ(evenBandBin["OriginalBand"][0], "1");
  ASSERT_EQ(evenBandBin["OriginalBand"][1], "2");

  ASSERT_EQ(evenBandBin["FilterName"][0], oddBandBin["FilterName"][0]);
  ASSERT_EQ(evenBandBin["FilterName"][1], oddBandBin["FilterName"][1]);
  ASSERT_EQ(evenBandBin["OriginalBand"][0], oddBandBin["OriginalBand"][0]);
  ASSERT_EQ(evenBandBin["OriginalBand"][1], oddBandBin["OriginalBand"][1]);


  // Kernels Group
  PvlGroup &evenKernels = evenLabel->findGroup("Kernels", Pvl::Traverse);
  PvlGroup &oddKernels = oddLabel->findGroup("Kernels", Pvl::Traverse);

  ASSERT_EQ((int)evenKernels["NaifIkCode"], -74420);
  ASSERT_EQ((int)evenKernels["NaifIkCode"], (int)oddKernels["NaifIkCode"]);


  // Label Object
  ASSERT_EQ((int)evenLabel->findObject("Label").findKeyword("Bytes"), 65536);
  ASSERT_EQ((int)oddLabel->findObject("Label").findKeyword("Bytes"), 65536);


  // OriginalLabel Object
  PvlObject evenOgLbl = evenLabel->findObject("OriginalLabel");
  PvlObject oddOgLbl = oddLabel->findObject("OriginalLabel");

  ASSERT_EQ(evenOgLbl["Name"][0], "IsisCube");
  ASSERT_EQ((int)evenOgLbl["StartByte"], 141313);
  ASSERT_EQ(evenOgLbl["Name"][0], oddOgLbl["Name"][0]);
  ASSERT_EQ((int)evenOgLbl["StartByte"], (int)oddOgLbl["StartByte"]);
}

TEST(Marci2Isis, Marci2isisTestDefaultUnFlipped) {
  Pvl appLog;
  QTemporaryDir prefix;
  QString cubeFileName = prefix.path() + "/marci2isis_out.cub";
  QVector<QString> args = {"from=data/marci2isis/MOI_000009_0294_MU_00N044W_cropped.IMG",
                           "flip=no","to=" + cubeFileName };
  UserInterface options(APP_XML, args);
  marci2isis(options, &appLog);

  QString cubeFileNameEven = prefix.path() + "/marci2isis_out.even.cub";
  Cube cubeEven(cubeFileNameEven.toStdString());
  Pvl *evenLabel = cubeEven.label();

  PvlGroup &evenInst = evenLabel->findGroup("Instrument", Pvl::Traverse);
  ASSERT_EQ((int)evenInst["DataFlipped"], 0);
}

TEST(Marci2Isis, Marci2isisTestColorOffset) {
  Pvl appLog;
  QTemporaryDir prefix;
  QString cubeFileName = prefix.path() + "/marci2isis_out.cub";
  QVector<QString> args = {"from=data/marci2isis/T02_001251_1292_MU_00N237W_cropped.IMG",
                           "to=" + cubeFileName, "coloroffset_size=3" };
  UserInterface options(APP_XML, args);
  marci2isis(options, &appLog);

  QString cubeFileNameEven = prefix.path() + "/marci2isis_out.even.cub";
  Cube cubeEven(cubeFileNameEven.toStdString());
  Pvl *evenLabel = cubeEven.label();

  ASSERT_EQ((int)evenLabel->findKeyword("TileLines", Pvl::Traverse), 86);

  PvlGroup dimensions = evenLabel->findGroup("Dimensions", Pvl::Traverse);
  ASSERT_EQ((int)dimensions.findKeyword("Lines"), 86);

  PvlGroup instrument = evenLabel->findGroup("Instrument", Pvl::Traverse);
  ASSERT_EQ((int)instrument.findKeyword("ColorOffset"), 3);

  PvlObject ogLabel = evenLabel->findObject("OriginalLabel");
  ASSERT_EQ((int)ogLabel["StartByte"], 153601);
}


TEST(Marci2Isis, Marci2isisTestVarExp) {
  Pvl appLog;
  QTemporaryDir prefix;
  // run marci2isis on an image with varexp with flip=yes and flip=no
  QString cubeFileNameFlipped = prefix.path() + "/marci2isis_out.cub";
  QVector<QString> args = {"from=data/marci2isis/P07_003640_2331_MA_00N288W_cropped.IMG",
                           "to=" + cubeFileNameFlipped, "flip=yes" };
  UserInterface options(APP_XML, args);
  marci2isis(options, &appLog);

  QString cubeFileNameUnflipped = prefix.path() + "/marci2isis_out_unflipped.cub";
  args = {"from=data/marci2isis/P07_003640_2331_MA_00N288W_cropped.IMG",
          "to=" + cubeFileNameUnflipped, "flip=no" };
  UserInterface options_unflipped(APP_XML, args);
  marci2isis(options_unflipped, &appLog);

  // get output from each call
  QString outFileNameFlipped = prefix.path() + "/marci2isis_out.even.cub";
  Cube cubeFlipped(outFileNameFlipped.toStdString());
  Pvl *labelFlipped = cubeFlipped.label();
  PvlGroup instFlipped = labelFlipped->findGroup("Instrument", Pvl::Traverse);

  QString outFileNameUnflipped = prefix.path() + "/marci2isis_out_unflipped.even.cub";
  Cube cubeUnflipped(outFileNameUnflipped.toStdString());
  Pvl *labelUnflipped = cubeUnflipped.label();
  PvlGroup instUnflipped = labelUnflipped->findGroup("Instrument", Pvl::Traverse);

  // check and compare output
  ASSERT_EQ((int)instFlipped["DataFlipped"], 1);
  ASSERT_EQ(instFlipped["FrameNumber"][0], "400");
  ASSERT_EQ(instFlipped["FrameNumber"][1], "64");
  ASSERT_EQ(instFlipped["FrameNumber"][2], "0");
  ASSERT_EQ(instFlipped["VariableExposureDuration"][0], "17.5");
  ASSERT_EQ(instFlipped["VariableExposureDuration"][1], "15");
  ASSERT_EQ(instFlipped["VariableExposureDuration"][2], "17.5");

  ASSERT_EQ((int)instUnflipped["DataFlipped"], 0);
  ASSERT_EQ(instUnflipped["FrameNumber"][0], "0");
  ASSERT_EQ(instUnflipped["FrameNumber"][1], "64");
  ASSERT_EQ(instUnflipped["FrameNumber"][2], "400");
  ASSERT_EQ(instUnflipped["VariableExposureDuration"][0], "17.5");
  ASSERT_EQ(instUnflipped["VariableExposureDuration"][1], "15");
  ASSERT_EQ(instUnflipped["VariableExposureDuration"][2], "17.5");
}
