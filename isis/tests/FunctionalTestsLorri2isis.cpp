#include <QTemporaryDir>

#include "Pvl.h"
#include "PvlGroup.h"
#include "TestUtilities.h"
#include "Histogram.h"
#include "lorri2isis.h"

#include "gtest/gtest.h"

using namespace Isis;

static QString APP_XML = QString::fromStdString(FileName("$ISISROOT/bin/xml/lorri2isis.xml").expanded());

TEST(Lorri2Isis, Lorri2IsisTestDefault) {
  QTemporaryDir prefix;
  QString cubeFileName = prefix.path() + "/lorri2isisTEMP.cub";
  QVector<QString> args = {"from=data/lorri2isis/lor_0034974380_0x630_sci_1_cropped.fit",
                           "to="+ cubeFileName};

  UserInterface options(APP_XML, args);
  try {
    lorri2isis(options);
  }
  catch (IException &e) {
    FAIL() << "Unable to ingest LORRI image: " <<  e.toString().c_str() << std::endl;
  }
  Cube cube(cubeFileName.toStdString());
  Pvl *isisLabel = cube.label();

  // Dimensions Group
  PvlGroup &dimensions = isisLabel->findGroup("Dimensions", Pvl::Traverse);
  EXPECT_EQ(int(dimensions["Samples"]), 25);
  EXPECT_EQ(int(dimensions["Lines"]), 3);
  EXPECT_EQ(int(dimensions["Bands"]), 1);

  // Pixels Group
  PvlGroup &pixels = isisLabel->findGroup("Pixels", Pvl::Traverse);
  EXPECT_EQ(pixels["Type"][0], "Real");
  EXPECT_EQ(pixels["ByteOrder"][0], "Lsb");
  EXPECT_EQ(double(pixels["Base"]), 0.0);
  EXPECT_EQ(double(pixels["Multiplier"]), 1.0);

  // Instrument Group
  PvlGroup &inst = isisLabel->findGroup("Instrument", Pvl::Traverse);
  EXPECT_EQ(inst["SpacecraftName"][0], "NEW HORIZONS");
  EXPECT_EQ(inst["InstrumentId"][0], "LORRI" );
  EXPECT_EQ(inst["TargetName"][0], "IO" );
  EXPECT_EQ(inst["StartTime"][0], "2007-02-28T13:14:22.331");
  EXPECT_EQ(inst["SpacecraftClockStartCount"][0], "1/0034974379:47125");
  EXPECT_DOUBLE_EQ(double(inst["ExposureDuration"]), 0.075);

  // Archive Group
  PvlGroup &archive = isisLabel->findGroup("Archive", Pvl::Traverse);
  EXPECT_EQ(archive["HighSpeedCompressionMode"][0], "LOSSLESS");
  EXPECT_EQ(archive["ObservationCompletionStatus"][0], "COMPLETE");
  EXPECT_EQ(archive["SequenceDescription"][0], "Jupiter shine");

  // BandBin Group
  PvlGroup &bandbin = isisLabel->findGroup("BandBin", Pvl::Traverse);
  EXPECT_DOUBLE_EQ(IString::ToDouble(bandbin["Center"][0]), 600);
  EXPECT_DOUBLE_EQ(IString::ToDouble(bandbin["Width"][0]), 500);

  // Kernels Group
  PvlGroup &kernel = isisLabel->findGroup("Kernels", Pvl::Traverse);
  EXPECT_EQ(int(kernel["NaifFrameCode"]), -98301);

  Histogram *hist = cube.histogram();

  EXPECT_DOUBLE_EQ(hist->Average(), 0.57074409094328682);
  EXPECT_DOUBLE_EQ(hist->Sum(), 42.805806820746511);
  EXPECT_EQ(hist->ValidPixels(), 75);
  EXPECT_DOUBLE_EQ(hist->StandardDeviation(), 1.234004896087934);

  delete hist;
}

TEST(Lorri2Isis, Lorri2IsisTestRaw) {
  QTemporaryDir prefix;
  QString cubeFileName = prefix.path() + "/lorri2isisTEMP.cub";
  QVector<QString> args = {"from=data/lorri2isis/lor_0035140199_0x630_eng_1_cropped.fit",
                           "to="+ cubeFileName};

   UserInterface options(APP_XML, args);

   try {
     lorri2isis(options);
   }
   catch (IException &e) {
     FAIL() << "Unable to ingest LORRI image: " <<  e.toString().c_str() << std::endl;
   }

   Cube cube(cubeFileName.toStdString());
   Pvl *isisLabel = cube.label();

      // Dimensions Group
   PvlGroup &dimensions = isisLabel->findGroup("Dimensions", Pvl::Traverse);
   EXPECT_EQ(int(dimensions["Samples"]), 25);
   EXPECT_EQ(int(dimensions["Lines"]), 3);
   EXPECT_EQ(int(dimensions["Bands"]), 1);

   // Pixels Group
   PvlGroup &pixels = isisLabel->findGroup("Pixels", Pvl::Traverse);
   EXPECT_EQ(pixels["Type"][0], "SignedWord");
   EXPECT_EQ(pixels["ByteOrder"][0], "Lsb");
   EXPECT_EQ(double(pixels["Base"]), 0.0);
   EXPECT_EQ(double(pixels["Multiplier"]), 1.0);

   // Instrument Group
   PvlGroup &inst = isisLabel->findGroup("Instrument", Pvl::Traverse);
   EXPECT_EQ(inst["SpacecraftName"][0], "NEW HORIZONS");
   EXPECT_EQ(inst["InstrumentId"][0], "LORRI" );
   EXPECT_EQ(inst["TargetName"][0], "IO" );
   EXPECT_EQ(inst["StartTime"][0], "2007-03-02T11:18:01.329" );
   EXPECT_EQ(inst["SpacecraftClockStartCount"][0], "1/0035140198:47025" );
   EXPECT_DOUBLE_EQ(double(inst["ExposureDuration"]), 0.079);

   // Archive Group
   PvlGroup &archive = isisLabel->findGroup("Archive", Pvl::Traverse);
   EXPECT_EQ(archive["HighSpeedCompressionMode"][0], "LOSSLESS");
   EXPECT_EQ(archive["ObservationCompletionStatus"][0], "COMPLETE" );
   EXPECT_EQ(archive["SequenceDescription"][0], "High phase monitoring" );

   // BandBin Group
   PvlGroup &bandbin = isisLabel->findGroup("BandBin", Pvl::Traverse);
   EXPECT_DOUBLE_EQ(IString::ToDouble(bandbin["Center"][0]), 600);
   EXPECT_DOUBLE_EQ(IString::ToDouble(bandbin["Width"][0]), 500);

   // Kernels Group
   PvlGroup &kernel = isisLabel->findGroup("Kernels", Pvl::Traverse);
   EXPECT_EQ(int(kernel["NaifFrameCode"]), -98301);
 }

 TEST(Lorri2Isis, Lorri2IsisTestErrorQuality) {
    QTemporaryDir prefix;
    QString cubeFileName = prefix.path() + "/lorri2isisTEMP.cub";
    QString errFileName = prefix.path() + "/lorri2isiserrTEMP.cub";
    QString qualityFileName = prefix.path() + "/lorri2isisqualTEMP.cub";
    QVector<QString> args = {"from=data/lorri2isis/lor_0034974377_0x630_sci_1_cropped.fit",
                             "to=" + cubeFileName,
                             "quality=" + qualityFileName,
                             "error=" + errFileName};

   UserInterface options(APP_XML, args);
   try {
    lorri2isis(options);
   }
   catch (IException &e) {
     FAIL() << "Unable to ingest LORRI image: " <<  e.toString().c_str() << std::endl;
   }

   Cube errCube(errFileName.toStdString());
   Cube qualityCube(qualityFileName.toStdString());
   Pvl *isisErrLabel = errCube.label();
   Pvl *isisQualityLabel = qualityCube.label();

   // Quality file Dimensions Group:
   PvlGroup &qualDimensions = isisQualityLabel->findGroup("Dimensions", Pvl::Traverse);
   EXPECT_EQ(int(qualDimensions["Samples"]), 25);
   EXPECT_EQ(int(qualDimensions["Lines"]), 3);
   EXPECT_EQ(int(qualDimensions["Bands"]), 1);

   // Quality File Pixels Group
   PvlGroup &qualPixels = isisQualityLabel->findGroup("Pixels", Pvl::Traverse);
   EXPECT_EQ(qualPixels["Type"][0], "SignedWord");
   EXPECT_EQ(qualPixels["ByteOrder"][0], "Lsb");
   EXPECT_EQ(double(qualPixels["Base"]), 32768.0);
   EXPECT_EQ(double(qualPixels["Multiplier"]), 1.0);

   // Error file Dimensions Group:
   PvlGroup &errDimensions = isisErrLabel->findGroup("Dimensions", Pvl::Traverse);
   EXPECT_EQ(int(errDimensions["Samples"]), 25);
   EXPECT_EQ(int(errDimensions["Lines"]), 3);
   EXPECT_EQ(int(errDimensions["Bands"]), 1);

   // Error File Pixels Group
   PvlGroup &errPixels = isisErrLabel->findGroup("Pixels", Pvl::Traverse);
   EXPECT_EQ(errPixels["Type"][0], "Real");
   EXPECT_EQ(errPixels["ByteOrder"][0], "Lsb");
   EXPECT_EQ(double(errPixels["Base"]), 0.0);
   EXPECT_EQ(double(errPixels["Multiplier"]), 1.0);

   Histogram *errHist = errCube.histogram();

   EXPECT_DOUBLE_EQ(errHist->Average(), 1.0110764837265014);
   EXPECT_DOUBLE_EQ(errHist->Sum(), 75.83073627948761);
   EXPECT_EQ(errHist->ValidPixels(), 75);
   EXPECT_DOUBLE_EQ(errHist->StandardDeviation(), 0.61401264476475648);

   delete errHist;

   Histogram *qualHist = qualityCube.histogram();

   EXPECT_DOUBLE_EQ(qualHist->Average(),  32);
   EXPECT_DOUBLE_EQ(qualHist->Sum(), 800);
   EXPECT_EQ(qualHist->ValidPixels(), 25);
   EXPECT_DOUBLE_EQ(qualHist->StandardDeviation(), 0);

   delete qualHist;
 }

 TEST(Lorri2Isis, Lorri2IsisTestBadErrorFile) {
    QTemporaryDir prefix;
    QString cubeFileName = prefix.path() + "/lorri2isisTEMP.cub";
    QString errFileName = prefix.path() + "/lorri2isisTEMPerr.cub";
    QVector<QString> args = {"from=data/lorri2isis/lor_0035140199_0x630_eng_1_cropped.fit",
                             "to=" + cubeFileName,
                             "error=" + errFileName};

    UserInterface options(APP_XML, args);

    ASSERT_ANY_THROW(lorri2isis(options));
  }

  TEST(Lorri2Isis, Lorri2IsisTestBadQualityFile) {
     QTemporaryDir prefix;
     QString cubeFileName = prefix.path() + "/lorri2isisTEMP.cub";
     QString qualityFileName = prefix.path() + "/lorri2isisTEMPqual.cub";
     QVector<QString> args = {"from=data/lorri2isis/lor_0035140199_0x630_eng_1_cropped.fit",
                              "to=" + cubeFileName,
                              "quality=" + qualityFileName};

     UserInterface options(APP_XML, args);

     ASSERT_ANY_THROW(lorri2isis(options));
  }

  TEST(Lorri2Isis, Lorri2IsisTestBadInstrument) {
     QTemporaryDir prefix;
     QString cubeFileName = prefix.path() + "/lorri2isisTEMP.cub";
     QString qualityFileName = prefix.path() + "/lorri2isisTEMPqual.cub";
     QVector<QString> args = {"from=data/lorri2isis/badimageinstr_cropped.fit",
                              "to=" + cubeFileName,
                              "quality=" + qualityFileName};

     UserInterface options(APP_XML, args);

     ASSERT_ANY_THROW(lorri2isis(options));
  }

  TEST(Lorri2Isis, Lorri2IsisTestBadImage) {
     QTemporaryDir prefix;
     QString cubeFileName = prefix.path() + "/lorri2isisTEMP.cub";
     QString qualityFileName = prefix.path() + "/lorri2isisTEMPqual.cub";
     QVector<QString> args = {"from=data/lorri2isis/badimage_cropped.fit",
                              "to=" + cubeFileName,
                              "quality=" + qualityFileName};

     UserInterface options(APP_XML, args);

     ASSERT_ANY_THROW(lorri2isis(options));
  }
