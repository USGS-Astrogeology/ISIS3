#include <QTemporaryDir>

#include "leisa2isis.h"
#include "Pvl.h"
#include "PvlGroup.h"
#include "TestUtilities.h"
#include "Histogram.h"

#include "gtest/gtest.h"

using namespace Isis;

static QString APP_XML = QString::fromStdString(FileName("$ISISROOT/bin/xml/leisa2isis.xml").expanded());

TEST(Leisa2Isis, Leisa2IsisTestDefault) {
   Pvl appLog;
   QTemporaryDir prefix;
   QString cubeFileName = prefix.path() + "/leisa2isisTEMP.cub";
   QVector<QString> args = {"from=data/leisa2isis/lsb_0034933739_0x53c_sci_1_cropped.fit", "to=" + cubeFileName };

  UserInterface options(APP_XML, args);
  try {
   leisa2isis(options, &appLog);
  }
  catch (IException &e) {
    FAIL() << "Unable to ingest LEISA image: " <<  e.toString().c_str() << std::endl;
  }
  Cube cube(cubeFileName.toStdString());
  Pvl *isisLabel = cube.label();

  // Dimensions Group
  PvlGroup &dimensions = isisLabel->findGroup("Dimensions", Pvl::Traverse);
  ASSERT_EQ(int(dimensions["Samples"]), 256);
  ASSERT_EQ(int(dimensions["Lines"]), 3);
  ASSERT_EQ(int(dimensions["Bands"]), 25);

  // Pixels Group
  PvlGroup &pixels = isisLabel->findGroup("Pixels", Pvl::Traverse);
  ASSERT_EQ(pixels["Type"][0], "Real");
  ASSERT_EQ(pixels["ByteOrder"][0], "Lsb");
  ASSERT_EQ(double(pixels["Base"]), 0.0);
  ASSERT_EQ(double(pixels["Multiplier"]), 1.0);

  // Instrument Group
  PvlGroup &inst = isisLabel->findGroup("Instrument", Pvl::Traverse);
  ASSERT_EQ(inst["SpacecraftName"][0], "NEW HORIZONS");
  ASSERT_EQ(inst["InstrumentId"][0], "LEISA" );
  ASSERT_EQ(inst["TargetName"][0], "EUROPA" );
  ASSERT_EQ(inst["SpacecraftClockStartCount"][0], "1/0034931099:00000" );
  ASSERT_DOUBLE_EQ(double(inst["ExposureDuration"]), 0.676);
  ASSERT_EQ(inst["StartTime"][0], "2007-02-28T01:13:01.3882781" );
  ASSERT_EQ(inst["StopTime"][0], "2007-02-28T01:17:12.388278" );
  ASSERT_NEAR(double(inst["FrameRate"]), 1.47929, 0.0001);
  ASSERT_EQ(inst["FrameRate"].unit(), "Hz");

  // Archive Group
  PvlGroup &archive = isisLabel->findGroup("Archive", Pvl::Traverse);
  ASSERT_DOUBLE_EQ(double(archive["MidObservationTime"]), 225897372.0736388);
  ASSERT_EQ(archive["MidObservationTime"].unit(), "s past J2000");
  ASSERT_DOUBLE_EQ(double(archive["ObservationDuration"]), 251.0);
  ASSERT_EQ(archive["Detector"][0], "LEISA" );
  ASSERT_EQ(archive["ScanType"][0], "LEISA" );

  // BandBin Group
  // Check size, first, 2 middle, and last values? Enough?
  PvlGroup &bandbin = isisLabel->findGroup("BandBin", Pvl::Traverse);
  ASSERT_EQ(bandbin["Center"].size(), 256);
  ASSERT_EQ(bandbin["Width"].size(), 256);
  ASSERT_EQ(bandbin["OriginalBand"].size(), 256);

  ASSERT_DOUBLE_EQ(IString::ToDouble(bandbin["Center"][0]), 2.4892);
  ASSERT_DOUBLE_EQ(IString::ToDouble(bandbin["Center"][64]), 1.9784);
  ASSERT_DOUBLE_EQ(IString::ToDouble(bandbin["Center"][128]), 1.572);
  ASSERT_DOUBLE_EQ(IString::ToDouble(bandbin["Center"][255]), 2.0898);

  ASSERT_DOUBLE_EQ(IString::ToDouble(bandbin["Width"][0]), 0.011228);
  ASSERT_DOUBLE_EQ(IString::ToDouble(bandbin["Width"][64]), 0.008924);
  ASSERT_DOUBLE_EQ(IString::ToDouble(bandbin["Width"][128]), 0.007091);
  ASSERT_DOUBLE_EQ(IString::ToDouble(bandbin["Width"][255]), 0.004915);

  ASSERT_DOUBLE_EQ(IString::ToDouble(bandbin["OriginalBand"][0]), 1);
  ASSERT_DOUBLE_EQ(IString::ToDouble(bandbin["OriginalBand"][64]), 65);
  ASSERT_DOUBLE_EQ(IString::ToDouble(bandbin["OriginalBand"][128]), 129);
  ASSERT_DOUBLE_EQ(IString::ToDouble(bandbin["OriginalBand"][255]), 256);

  // Kernels Group
  PvlGroup &kernel = isisLabel->findGroup("Kernels", Pvl::Traverse);
  ASSERT_EQ(int(kernel["NaifFrameCode"]), -98901);
  ASSERT_EQ(kernel["NaifFrameCode"].unit(), "SPICE ID");

  Histogram *hist = cube.histogram();

  ASSERT_DOUBLE_EQ(hist->Average(), 9178159546910.166);
  ASSERT_DOUBLE_EQ(hist->Sum(), 7048826532027008);
  ASSERT_EQ(hist->ValidPixels(), 768);
  ASSERT_DOUBLE_EQ(hist->StandardDeviation(), 16153319724110.654);

  delete hist;
}


TEST(Leisa2Isis, Leisa2IsisTestJan2015Format) {
   Pvl appLog;
   QTemporaryDir prefix;
   QString dAPP_XML = QString::fromStdString(FileName("$ISISROOT/bin/xml/leisa2isis.xml").expanded());
   QString cubeFileName = prefix.path() + "/leisa2isisTEMP.cub";
   QString errFileName = prefix.path() + "/leisa2isisTEMPerr.cub";
   QString qualityFileName = prefix.path() + "/leisa2isisTEMPqual.cub";
   QVector<QString> args = {"from=data/leisa2isis/jan2015_format.fit",
                            "to=" + cubeFileName,
                            "quality=" + qualityFileName,
                            "errormap=" + errFileName};

  UserInterface options(dAPP_XML, args);
  try {
   leisa2isis(options, &appLog);
  }
  catch (IException &e) {
    FAIL() << "Unable to ingest LEISA image: " <<  e.toString().c_str() << std::endl;
  }

  Cube errCube(errFileName.toStdString());
  Cube qualityCube(qualityFileName.toStdString());
  Pvl *isisErrLabel = errCube.label();
  Pvl *isisQualityLabel = qualityCube.label();

  // Quality file dimensions:
  PvlGroup &qualDimensions = isisQualityLabel->findGroup("Dimensions", Pvl::Traverse);
  ASSERT_EQ(int(qualDimensions["Samples"]), 256);
  ASSERT_EQ(int(qualDimensions["Lines"]), 3);
  ASSERT_EQ(int(qualDimensions["Bands"]), 25);

  // Error file dimensions:
  PvlGroup &errDimensions = isisErrLabel->findGroup("Dimensions", Pvl::Traverse);
  ASSERT_EQ(int(errDimensions["Samples"]), 256);
  ASSERT_EQ(int(errDimensions["Lines"]), 3);
  ASSERT_EQ(int(errDimensions["Bands"]), 25);

  // Quality File Pixels Group
  PvlGroup &qualPixels = isisQualityLabel->findGroup("Pixels", Pvl::Traverse);
  ASSERT_EQ(qualPixels["Type"][0], "SignedWord");
  ASSERT_EQ(qualPixels["ByteOrder"][0], "Lsb");
  ASSERT_EQ(double(qualPixels["Base"]), 0.0);
  ASSERT_EQ(double(qualPixels["Multiplier"]), 1.0);


  // Quality File Pixels Group
  PvlGroup &errPixels = isisErrLabel->findGroup("Pixels", Pvl::Traverse);
  ASSERT_EQ(errPixels["Type"][0], "Real");
  ASSERT_EQ(errPixels["ByteOrder"][0], "Lsb");
  ASSERT_EQ(double(errPixels["Base"]), 0.0);
  ASSERT_EQ(double(errPixels["Multiplier"]), 1.0);


  Histogram *err_hist = errCube.histogram();

  ASSERT_DOUBLE_EQ(err_hist->Average(), 32.577196389219189);
  ASSERT_DOUBLE_EQ(err_hist->Sum(), 17852.303621292114);
  ASSERT_EQ(err_hist->ValidPixels(), 548);
  ASSERT_DOUBLE_EQ(err_hist->StandardDeviation(), 77.113949800307125);

  delete err_hist;

  Histogram *qual_hist = qualityCube.histogram();

  ASSERT_DOUBLE_EQ(qual_hist->Average(), 0.0078125);
  ASSERT_DOUBLE_EQ(qual_hist->Sum(), 6);
  ASSERT_EQ(qual_hist->ValidPixels(), 768);
  ASSERT_DOUBLE_EQ(qual_hist->StandardDeviation(), 0.088099778978511525);

  delete qual_hist;
}


TEST(Leisa2Isis, Leisa2IsisTestCalib) {
   Pvl appLog;
   QTemporaryDir prefix;
   QString dAPP_XML = QString::fromStdString(FileName("$ISISROOT/bin/xml/leisa2isis.xml").expanded());
   QString cubeFileName = prefix.path() + "/leisa2isisTEMP.cub";
   QString errFileName = prefix.path() + "/leisa2isisTEMPerr.cub";
   QString qualityFileName = prefix.path() + "/leisa2isisTEMPqual.cub";
   QVector<QString> args = {"from=data/leisa2isis/calib.fit",
                            "to=" + cubeFileName,
                            "quality=" + qualityFileName,
                            "errormap=" + errFileName};

  UserInterface options(dAPP_XML, args);
  try {
   leisa2isis(options, &appLog);
  }
  catch (IException &e) {
    FAIL() << "Unable to ingest LEISA image: " <<  e.toString().c_str() << std::endl;
  }

  Cube errCube(errFileName.toStdString());
  Cube qualityCube(qualityFileName.toStdString());
  Pvl *isisErrLabel = errCube.label();
  Pvl *isisQualityLabel = qualityCube.label();

  // Quality file dimensions:
  PvlGroup &qualDimensions = isisQualityLabel->findGroup("Dimensions", Pvl::Traverse);
  ASSERT_EQ(int(qualDimensions["Samples"]), 25);
  ASSERT_EQ(int(qualDimensions["Lines"]), 1);
  ASSERT_EQ(int(qualDimensions["Bands"]), 3);

  // Error file dimensions:
  PvlGroup &errDimensions = isisErrLabel->findGroup("Dimensions", Pvl::Traverse);
  ASSERT_EQ(int(errDimensions["Samples"]), 25);
  ASSERT_EQ(int(errDimensions["Lines"]), 1);
  ASSERT_EQ(int(errDimensions["Bands"]), 3);

  // Quality File Pixels Group
  PvlGroup &qualPixels = isisQualityLabel->findGroup("Pixels", Pvl::Traverse);
  ASSERT_EQ(qualPixels["Type"][0], "SignedWord");
  ASSERT_EQ(qualPixels["ByteOrder"][0], "Lsb");
  ASSERT_EQ(double(qualPixels["Base"]), 0.0);
  ASSERT_EQ(double(qualPixels["Multiplier"]), 1.0);


  // Quality File Pixels Group
  PvlGroup &errPixels = isisErrLabel->findGroup("Pixels", Pvl::Traverse);
  ASSERT_EQ(errPixels["Type"][0], "Real");
  ASSERT_EQ(errPixels["ByteOrder"][0], "Lsb");
  ASSERT_EQ(double(errPixels["Base"]), 0.0);
  ASSERT_EQ(double(errPixels["Multiplier"]), 1.0);

  Histogram *err_hist = errCube.histogram();

  ASSERT_DOUBLE_EQ(err_hist->Average(), -2.9127522277832032);
  ASSERT_DOUBLE_EQ(err_hist->Sum(), -72.818805694580078);
  ASSERT_EQ(err_hist->ValidPixels(), 25);
  ASSERT_DOUBLE_EQ(err_hist->StandardDeviation(), 51.130458077495909);

  delete err_hist;

  Histogram *qual_hist = qualityCube.histogram();

  ASSERT_DOUBLE_EQ(qual_hist->Average(), .12);
  ASSERT_DOUBLE_EQ(qual_hist->Sum(), 3);
  ASSERT_EQ(qual_hist->ValidPixels(), 25);
  ASSERT_DOUBLE_EQ(qual_hist->StandardDeviation(), .6);

  delete qual_hist;
}


TEST(Leisa2Isis, Leisa2IsisTestRaw) {
   Pvl appLog;
   QTemporaryDir prefix;
   QString dAPP_XML = QString::fromStdString(FileName("$ISISROOT/bin/xml/leisa2isis.xml").expanded());
   QString cubeFileName = prefix.path() + "/leisa2isisTEMP.cub";

   QVector<QString> args = {"from=data/leisa2isis/raw.fit",
                            "to=" + cubeFileName};

   UserInterface options(dAPP_XML, args);

   try {
     leisa2isis(options, &appLog);
   }
   catch (IException &e) {
     FAIL() << "Unable to ingest LEISA image: " <<  e.toString().c_str() << std::endl;
   }

   Cube cube(cubeFileName.toStdString());
   Pvl *isisLabel = cube.label();

      // Dimensions Group
   PvlGroup &dimensions = isisLabel->findGroup("Dimensions", Pvl::Traverse);
   ASSERT_EQ(int(dimensions["Samples"]), 256);
   ASSERT_EQ(int(dimensions["Lines"]), 3);
   ASSERT_EQ(int(dimensions["Bands"]), 25);

   // Pixels Group
   PvlGroup &pixels = isisLabel->findGroup("Pixels", Pvl::Traverse);
   ASSERT_EQ(pixels["Type"][0], "SignedWord");
   ASSERT_EQ(pixels["ByteOrder"][0], "Lsb");
   ASSERT_EQ(double(pixels["Base"]), 0.0);
   ASSERT_EQ(double(pixels["Multiplier"]), 1.0);

   // Instrument Group
   PvlGroup &inst = isisLabel->findGroup("Instrument", Pvl::Traverse);
   ASSERT_EQ(inst["SpacecraftName"][0], "NEW HORIZONS");
   ASSERT_EQ(inst["InstrumentId"][0], "LEISA" );
   ASSERT_EQ(inst["TargetName"][0], "CALLISTO" );
   ASSERT_EQ(inst["SpacecraftClockStartCount"][0], "1/0030594839:00000" );
   ASSERT_DOUBLE_EQ(double(inst["ExposureDuration"]), 0.131);
   ASSERT_EQ(inst["StartTime"][0], "2007-01-08T20:42:01.3824425" );
   ASSERT_EQ(inst["StopTime"][0], "2007-01-08T20:42:42.3824425" );
   ASSERT_NEAR(double(inst["FrameRate"]), 7.63359, 0.0001);
   ASSERT_EQ(inst["FrameRate"].unit(), "Hz");

   // Archive Group
   PvlGroup &archive = isisLabel->findGroup("Archive", Pvl::Traverse);
   ASSERT_DOUBLE_EQ(double(archive["MidObservationTime"]), 221561007.0665882);
   ASSERT_EQ(archive["MidObservationTime"].unit(), "s past J2000");
   ASSERT_DOUBLE_EQ(double(archive["ObservationDuration"]), 41.0);
   ASSERT_EQ(archive["Detector"][0], "LEISA" );
   ASSERT_EQ(archive["ScanType"][0], "LEISA" );

   // BandBin Group
   PvlGroup &bandbin = isisLabel->findGroup("BandBin", Pvl::Traverse);
   ASSERT_EQ(bandbin["Center"].size(), 256);
   ASSERT_EQ(bandbin["Width"].size(), 256);
   ASSERT_EQ(bandbin["OriginalBand"].size(), 256);

   ASSERT_DOUBLE_EQ(IString::ToDouble(bandbin["Center"][0]), 2.4892);
   ASSERT_DOUBLE_EQ(IString::ToDouble(bandbin["Center"][64]), 1.9784);
   ASSERT_DOUBLE_EQ(IString::ToDouble(bandbin["Center"][128]), 1.572);
   ASSERT_DOUBLE_EQ(IString::ToDouble(bandbin["Center"][255]), 2.0898);

   ASSERT_DOUBLE_EQ(IString::ToDouble(bandbin["Width"][0]), 0.011228);
   ASSERT_DOUBLE_EQ(IString::ToDouble(bandbin["Width"][64]), 0.008924);
   ASSERT_DOUBLE_EQ(IString::ToDouble(bandbin["Width"][128]), 0.007091);
   ASSERT_DOUBLE_EQ(IString::ToDouble(bandbin["Width"][255]), 0.004915);

   ASSERT_DOUBLE_EQ(IString::ToDouble(bandbin["OriginalBand"][0]), 1);
   ASSERT_DOUBLE_EQ(IString::ToDouble(bandbin["OriginalBand"][64]), 65);
   ASSERT_DOUBLE_EQ(IString::ToDouble(bandbin["OriginalBand"][128]), 129);
   ASSERT_DOUBLE_EQ(IString::ToDouble(bandbin["OriginalBand"][255]), 256);
 }


TEST(Leisa2Isis, Leisa2IsisTestRawErrormapFail) {
   Pvl appLog;
   QTemporaryDir prefix;
   QString dAPP_XML = QString::fromStdString(FileName("$ISISROOT/bin/xml/leisa2isis.xml").expanded());
   QString cubeFileName = prefix.path() + "/leisa2isisTEMP.cub";
   QString errFileName = prefix.path() + "/leisa2isisTEMPerr.cub";

   QVector<QString> args = {"from=data/leisa2isis/raw.fit",
                            "to=" + cubeFileName,
                            "errormap=" + errFileName};

   UserInterface options(dAPP_XML, args);
   // Raw files with an errormap specification should fail
   ASSERT_ANY_THROW(leisa2isis(options, &appLog));
 }


TEST(Leisa2Isis, Leisa2IsisTestRawQualityFail) {
   Pvl appLog;
   QTemporaryDir prefix;
   QString dAPP_XML = QString::fromStdString(FileName("$ISISROOT/bin/xml/leisa2isis.xml").expanded());
   QString cubeFileName = prefix.path() + "/leisa2isisTEMP.cub";
   QString qualityFileName = prefix.path() + "/leisa2isisTEMPqual.cub";

   QVector<QString> args = {"from=data/leisa2isis/raw.fit",
                            "to=" + cubeFileName,
                            "quality=" + qualityFileName};

   UserInterface options(dAPP_XML, args);

   // Raw files with a quality specification should fail
   ASSERT_ANY_THROW(leisa2isis(options, &appLog));
}

TEST(Leisa2Isis, Leisa2IsisTestQualityReplacement) {
   Pvl appLog;
   QTemporaryDir prefix;
   QString cubeFileName = prefix.path() + "/leisa2isisTEMP.cub";
   QVector<QString> args = {"from=data/leisa2isis/lsb_0034933739_0x53c_sci_1_cropped.fit",
                            "to=" + cubeFileName,
                            "replace=true"};

  UserInterface options(APP_XML, args);
  try {
   leisa2isis(options, &appLog);
  }
  catch (IException &e) {
    FAIL() << "Unable to ingest LEISA image: " <<  e.toString().c_str() << std::endl;
  }
  Cube cube(cubeFileName.toStdString());

  Histogram *hist = cube.histogram();

  ASSERT_DOUBLE_EQ(hist->Average(), 9264735084251.8848);
  ASSERT_DOUBLE_EQ(hist->Sum(), 7059728134199936);
  ASSERT_EQ(hist->ValidPixels(), 762);
  ASSERT_DOUBLE_EQ(hist->StandardDeviation(), 16184539944722.791);

  delete hist;
}
