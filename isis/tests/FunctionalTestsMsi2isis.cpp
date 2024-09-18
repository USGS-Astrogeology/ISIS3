#include <QTemporaryDir>

#include "msi2isis.h"
#include "TempFixtures.h"
#include "LineManager.h"
#include "Pvl.h"
#include "PvlGroup.h"
#include "TestUtilities.h"
#include "Histogram.h"
#include "ImageHistogram.h"

#include "gtest/gtest.h"

using namespace Isis;

static QString APP_XML = QString::fromStdString(FileName("$ISISROOT/bin/xml/msi2isis.xml").expanded());

TEST_F(TempTestingFiles, Msi2isisTestGblIngestDefault) {
  Pvl appLog;
  QString cubeFileName = tempDir.path() + "/m0157063318f4_2p_iof_gbl.cub";
  QVector<QString> args = {"from=data/near/msi2isis/m0157063318f4_2p_iof_gbl.lbl",
                            "to=" + cubeFileName };

  UserInterface options(APP_XML, args);
  try {
    msi2isis(options, &appLog);
  }
  catch (IException &e) {
    FAIL() << "Unable to ingest NEAR/MSI GBL file: " <<  e.toString() << std::endl;
  }
  Cube cube(cubeFileName.toStdString());
  Pvl *isisLabel = cube.label();

  // Dimensions Group
  ASSERT_EQ(cube.sampleCount(), 537);
  ASSERT_EQ(cube.lineCount(), 412);
  ASSERT_EQ(cube.bandCount(), 1);

  // Instrument Group
  PvlGroup &inst = isisLabel->findGroup("Instrument", Pvl::Traverse);
  EXPECT_EQ(inst["SpacecraftName"][0], "NEAR EARTH ASTEROID RENDEZVOUS");
  EXPECT_EQ(inst["InstrumentId"][0], "MSI" );
  EXPECT_EQ(inst["TargetName"][0], "EROS" );
  EXPECT_EQ(inst["SpacecraftClockStartCount"][0], "157063318856" );
  EXPECT_EQ(inst["SpacecraftClockStopCount"][0], "157063318919" );
  EXPECT_EQ(inst["OriginalSpacecraftClockStartCount"][0], "157063318.856" );
  EXPECT_EQ(inst["OriginalSpacecraftClockStopCount"][0], "157063318.919" );
  ASSERT_DOUBLE_EQ(double(inst["ExposureDuration"]), 63.0);
  EXPECT_EQ(inst["StartTime"][0], "2001-02-08T17:24:42.217" );
  EXPECT_EQ(inst["StopTime"][0], "2001-02-08T17:24:42.280" );
  ASSERT_DOUBLE_EQ(double(inst["DpuDeckTemperature"]), 286.5);
  EXPECT_EQ(inst["DpuDeckTemperature"].unit(), "K");

  // Archive Group
  PvlGroup &archive = isisLabel->findGroup("Archive", Pvl::Traverse);
  EXPECT_EQ(archive["DataSetId"][0], "NEAR-A-MSI-3-EDR-EROS/ORBIT-V1.0");
  EXPECT_EQ(archive["ProductId"][0], "M0157063318F4_2P_IOF_GBL.FIT");
  EXPECT_EQ(archive["InstrumentHostName"][0], "NEAR EARTH ASTEROID RENDEZVOUS");
  EXPECT_EQ(archive["InstrumentName"][0], "MULTI-SPECTRAL IMAGER");
  EXPECT_EQ(archive["InstrumentId"][0], "MSI");
  EXPECT_EQ(archive["TargetName"][0], "EROS");

  // BandBin Group
  PvlGroup &bandbin = isisLabel->findGroup("BandBin", Pvl::Traverse);
  EXPECT_EQ(int(bandbin["FilterNumber"]), 4);
  EXPECT_EQ(int(bandbin["Center"]), 950);
  EXPECT_EQ(bandbin["Center"].unit(), "nm");

 // Kernels Group
  PvlGroup &kernel = isisLabel->findGroup("Kernels", Pvl::Traverse);
  EXPECT_EQ(int(kernel["NaifFrameCode"]), -93001);

  std::unique_ptr<Histogram> hist (cube.histogram());

  EXPECT_NEAR(hist->Average(), 0.042633522648316, .000001);
  EXPECT_NEAR(hist->Sum(), 6947.8146519054, .0001);
  EXPECT_EQ(hist->ValidPixels(), 162966);
  EXPECT_EQ(hist->NullPixels(), 58278);
  EXPECT_NEAR(hist->StandardDeviation(), 0.016649588425538, .0001);

  // This section tests that the edge was trimmed as expected
  // (TRIM=TRUE, the default)
  // Count the number of edge pixels
  int total_edge_pixels = 0;

  const int trimSize = 33;
  const int expectValid = cube.sampleCount() - ( 2 * trimSize);
  const int end_s = cube.sampleCount() - trimSize;
  const int end_l = cube.lineCount() - trimSize;

  // Data range 0-1 for I/F data!
  ImageHistogram trimEdge(0.0, 1.0);
  ImageHistogram innerImage(0.0, 1.0);

  LineManager line(cube);
  for ( line.begin() ; !line.end() ; line++ ) {
    cube.read(line);
    const double *lbuf = line.DoubleBuffer();
    if ( (line.Line() <= trimSize) || ( line.Line() > end_l ) ) {
      trimEdge.AddData(&lbuf[0], line.size());
      total_edge_pixels += line.size();
    }
    else {

      // This is past full line trimming and is on the edges only
      trimEdge.AddData(&lbuf[0], trimSize);
      trimEdge.AddData(&lbuf[end_s], trimSize);
      total_edge_pixels += (trimSize + trimSize);

      // Add valid data in untrimmed area should match image histogram data
      innerImage.AddData( &lbuf[trimSize], expectValid );
    }
  }

  // The whole image histogram should match the untrimmed section
  // in the this test
  EXPECT_EQ(innerImage.Average(), hist->Average());
  EXPECT_EQ(innerImage.Sum(), hist->Sum());
  EXPECT_EQ(innerImage.ValidPixels(), hist->ValidPixels());
  EXPECT_EQ(innerImage.StandardDeviation(), hist->StandardDeviation());

  EXPECT_EQ(trimEdge.ValidPixels(), 0);
  EXPECT_EQ(trimEdge.Sum(), 0.0);
  EXPECT_EQ(trimEdge.NullPixels(), total_edge_pixels);
}



TEST_F(TempTestingFiles, Msi2isisTestGblIngestNoTrim) {
  Pvl appLog;
  QString cubeFileName = tempDir.path() + "/m0157063318f4_2p_iof_gbl_notrim.cub";
  QVector<QString> args = {"from=data/near/msi2isis/m0157063318f4_2p_iof_gbl.lbl",
                            "to=" + cubeFileName,
                            "trim=false" };

  UserInterface options(APP_XML, args);
  try {
   msi2isis(options, &appLog);
  }
  catch (IException &e) {
    FAIL() << "Unable to ingest NEAR/MSI GBL file: " <<  e.toString() << std::endl;
  }
  Cube cube(cubeFileName.toStdString());

  // Dimensions Group
  ASSERT_EQ(cube.sampleCount(), 537);
  ASSERT_EQ(cube.lineCount(), 412);
  ASSERT_EQ(cube.bandCount(), 1);

  // Label content is unchanged from previous test

  std::unique_ptr<Histogram> hist (cube.histogram());

  EXPECT_NEAR(hist->Average(), 0.040669880452177, .000001);
  EXPECT_NEAR(hist->Sum(), 8997.9670307614, .0001);
  EXPECT_EQ(hist->ValidPixels(), 221244);
  EXPECT_EQ(hist->NullPixels(), 0);
  EXPECT_NEAR(hist->StandardDeviation(), 0.017675184374796, .0001);

  // This section tests that the edge was not trimmed (TRIM=FALSE)

  const int trimSize = 33;
  const int expectValid = cube.sampleCount() - ( 2 * trimSize);
  const int end_s = cube.sampleCount() - trimSize;
  const int end_l = cube.lineCount() - trimSize;

  // Data range 0-1 for I/F data!
  ImageHistogram trimEdge(0.0, 1.0);
  ImageHistogram innerImage(0.0, 1.0);

  LineManager line(cube);
  for ( line.begin() ; !line.end() ; line++ ) {
    cube.read(line);
    const double *lbuf = line.DoubleBuffer();
    if ( (line.Line() <= trimSize) || ( line.Line() > end_l ) ) {
      trimEdge.AddData(&lbuf[0], line.size());
    }
    else {

      // This is past full line trimming and is on the edges only
      trimEdge.AddData(&lbuf[0], trimSize);
      trimEdge.AddData(&lbuf[end_s], trimSize);

      // Add valid data in untrimmed area which should match the image
      // histogram data of whole trimmed image in the first test
      innerImage.AddData( &lbuf[trimSize], expectValid );
    }
  }

  // This innerImage area should be eqivalent to whole trimmed image
  // in the first test
  EXPECT_NEAR(innerImage.Average(), 0.042633522648316, .000001);
  EXPECT_NEAR(innerImage.Sum(), 6947.8146519054, .0001);
  EXPECT_EQ(innerImage.ValidPixels(), 162966);
  EXPECT_EQ(innerImage.NullPixels(), 0);
  EXPECT_NEAR(innerImage.StandardDeviation(), 0.016649588425538, .0001);

  // Just to be sure...
  EXPECT_EQ(trimEdge.NullPixels(), 0);
}

TEST_F(TempTestingFiles, Msi2isisTestCubicConvolution) {
  Pvl appLog;
  QString cubeFileName = tempDir.path() + "/m0126865998f4_2p_iof.cubic.cub";
  QVector<QString> args = {"from=data/near/msi2isis/m0126865998f4_2p_iof.lbl",
                            "to=" + cubeFileName,
                            "interp=cubicconvolution" };

  UserInterface options(APP_XML, args);
  try {
   msi2isis(options, &appLog);
  }
  catch (IException &e) {
    FAIL() << "Unable to ingest NEAR/MSI file: " <<  e.toString() << std::endl;
  }
  Cube cube(cubeFileName.toStdString());
  Pvl *isisLabel = cube.label();

  // Dimensions Group
  ASSERT_EQ(cube.sampleCount(), 537);
  ASSERT_EQ(cube.lineCount(), 412);
  ASSERT_EQ(cube.bandCount(), 1);

  // Instrument Group
  PvlGroup &inst = isisLabel->findGroup("Instrument", Pvl::Traverse);
  EXPECT_EQ(inst["SpacecraftName"][0], "NEAR EARTH ASTEROID RENDEZVOUS");
  EXPECT_EQ(inst["InstrumentId"][0], "MSI" );
  EXPECT_EQ(inst["TargetName"][0], "EROS" );
  EXPECT_EQ(inst["SpacecraftClockStartCount"][0], "126865998830" );
  EXPECT_EQ(inst["SpacecraftClockStopCount"][0], "126865998919" );
  EXPECT_EQ(inst["OriginalSpacecraftClockStartCount"][0], "126865998.830" );
  EXPECT_EQ(inst["OriginalSpacecraftClockStopCount"][0], "126865998.919" );
  EXPECT_DOUBLE_EQ(double(inst["ExposureDuration"]), 89.0);
  EXPECT_EQ(inst["StartTime"][0], "2000-02-25T05:16:12.656" );
  EXPECT_EQ(inst["StopTime"][0], "2000-02-25T05:16:12.745" );
  EXPECT_DOUBLE_EQ(double(inst["DpuDeckTemperature"]), 286.5);
  EXPECT_EQ(inst["DpuDeckTemperature"].unit(), "K");

  // Archive Group
  PvlGroup &archive = isisLabel->findGroup("Archive", Pvl::Traverse);
  EXPECT_EQ(archive["DataSetId"][0], "NEAR-A-MSI-3-EDR-EROS/ORBIT-V1.0");
  EXPECT_EQ(archive["ProductId"][0], "M0126865998F4_2P_IOF.FIT");
  EXPECT_EQ(archive["InstrumentHostName"][0], "NEAR EARTH ASTEROID RENDEZVOUS");
  EXPECT_EQ(archive["InstrumentName"][0], "MULTI-SPECTRAL IMAGER");
  EXPECT_EQ(archive["InstrumentId"][0], "MSI");
  EXPECT_EQ(archive["TargetName"][0], "EROS");

  // BandBin Group
  PvlGroup &bandbin = isisLabel->findGroup("BandBin", Pvl::Traverse);
  EXPECT_EQ(int(bandbin["FilterNumber"]), 4);
  EXPECT_EQ(int(bandbin["Center"]), 950);
  EXPECT_EQ(bandbin["Center"].unit(), "nm");

 // Kernels Group
  PvlGroup &kernel = isisLabel->findGroup("Kernels", Pvl::Traverse);
  EXPECT_EQ(int(kernel["NaifFrameCode"]), -93001);

  std::unique_ptr<Histogram> hist (cube.histogram());

  EXPECT_NEAR(hist->Average(), 0.00909104981804087, .000001);
  EXPECT_NEAR(hist->Sum(), 1481.5320246468, .0001);
  EXPECT_EQ(hist->ValidPixels(), 162966);
  EXPECT_EQ(hist->NullPixels(), 58278);
  EXPECT_NEAR(hist->StandardDeviation(), 0.013555951402431, .0001);
}

TEST_F(TempTestingFiles, Msi2isisTestNearestNeighbor) {
  Pvl appLog;
  QString cubeFileName = tempDir.path() + "/m0126865998f4_2p_iof.nearestneighbor.cub";
  QVector<QString> args = {"from=data/near/msi2isis/m0126865998f4_2p_iof.lbl",
                            "to=" + cubeFileName,
                            "interp=nearestneighbor" };

  UserInterface options(APP_XML, args);
  try {
   msi2isis(options, &appLog);
  }
  catch (IException &e) {
    FAIL() << "Unable to ingest NEAR/MSI file: " <<  e.toString() << std::endl;
  }
  Cube cube(cubeFileName.toStdString());

  // Dimensions Group
  ASSERT_EQ(cube.sampleCount(), 537);
  ASSERT_EQ(cube.lineCount(), 412);
  ASSERT_EQ(cube.bandCount(), 1);

  // No changes in labels from previous tests

  std::unique_ptr<Histogram> hist (cube.histogram());

  EXPECT_NEAR(hist->Average(), 0.0090912713562279774, .000001);
  EXPECT_NEAR(hist->Sum(), 1481.6802540164529, .0001);
  EXPECT_EQ(hist->ValidPixels(), 162966);
  EXPECT_EQ(hist->NullPixels(), 58278);
  EXPECT_NEAR(hist->StandardDeviation(), 0.013550997159207, .0001);
}

TEST_F(TempTestingFiles, Msi2isisTestBilinear) {
  Pvl appLog;
  QString cubeFileName = tempDir.path() + "/m0126865998f4_2p_iof.bilinear.cub";
  QVector<QString> args = {"from=data/near/msi2isis/m0126865998f4_2p_iof.lbl",
                            "to=" + cubeFileName,
                            "interp=bilinear" };

  UserInterface options(APP_XML, args);
  try {
   msi2isis(options, &appLog);
  }
  catch (IException &e) {
    FAIL() << "Unable to ingest NEAR/MSI file: " <<  e.toString() << std::endl;
  }
  Cube cube(cubeFileName.toStdString());

  // Dimensions Group
  ASSERT_EQ(cube.sampleCount(), 537);
  ASSERT_EQ(cube.lineCount(), 412);
  ASSERT_EQ(cube.bandCount(), 1);

  // No changes in labels from previous tests

  std::unique_ptr<Histogram> hist (cube.histogram());

  EXPECT_NEAR(hist->Average(), 0.0090912713562279774, .000001);
  EXPECT_NEAR(hist->Sum(), 1481.5681278390484, .0001);
  EXPECT_EQ(hist->ValidPixels(), 162966);
  EXPECT_EQ(hist->NullPixels(), 58278);
  EXPECT_NEAR(hist->StandardDeviation(), 0.013542099831273, .0001);
}


// Error tests
TEST_F(TempTestingFiles, Msi2isisTestAMissingLabelFile) {
   Pvl appLog;
   QString cubeFileName = tempDir.path() + "/image.cub";
   QVector<QString> args = {"from=data/near/msi2isis/image.fit",
                            "to=" + cubeFileName };

   UserInterface options(APP_XML, args);

   // TEST A: pass in a fits file without label
   try {
     msi2isis(options, &appLog);
     FAIL() << "Missing label should throw an exception." <<std::endl;
   }
   catch (IException &e ) {
     EXPECT_THAT(e.what(), testing::HasSubstr("**I/O ERROR** Unable to find PDS label file"));
   }
   catch ( ... ) {
     FAIL() << "Expected error: Unable to find PDS label...";
   }
}


TEST_F(TempTestingFiles, Msi2isisTestBInvalidInstrumentId) {
   Pvl appLog;
   QString cubeFileName = tempDir.path() + "/bad_msi.cub";
   QVector<QString> args = {"from=data/near/msi2isis/label1.lbl",
                            "to=" + cubeFileName };

   UserInterface options(APP_XML, args);

   // TEST B: Invalid Instrument ID
   try {
     msi2isis(options, &appLog);
     FAIL() << "Invalid Instrument should throw an exception." <<std::endl;
   }
   catch (IException &e ) {
     EXPECT_THAT(e.what(), testing::HasSubstr("has an invalid value for INSTRUMENT_ID"));
   }
   catch ( ... ) {
     FAIL() << "Expected error: ...has an invalid value for INSTRUMENT_ID...";
   }
}

TEST_F(TempTestingFiles, Msi2isisTestCInvalidSize) {
   Pvl appLog;
   QString cubeFileName = tempDir.path() + "/bad_msi.cub";
   QVector<QString> args = {"from=data/near/msi2isis/label2.lbl",
                            "to=" + cubeFileName };

   UserInterface options(APP_XML, args);

   // TEST C: Invalid size
   try {
     msi2isis(options, &appLog);
     FAIL() << "Invalid size should throw an exception." <<std::endl;
   }
   catch (IException &e ) {
     EXPECT_THAT(e.what(), testing::HasSubstr("does not contain a full MSI image"));
   }
   catch ( ... ) {
     FAIL() << "Expected error: ...does not contain a full MSI image...";
   }
}

TEST_F(TempTestingFiles, Msi2isisTestDInvalidSize2) {
   Pvl appLog;
   QString cubeFileName = tempDir.path() + "/bad_msi.cub";
   QVector<QString> args = {"from=data/near/msi2isis/label3.lbl",
                            "to=" + cubeFileName };

   UserInterface options(APP_XML, args);

   // TEST D: Invalid size
   try {
     msi2isis(options, &appLog);
     FAIL() << "Invalid size should throw an exception." <<std::endl;
   }
   catch (IException &e ) {
     EXPECT_THAT(e.what(), testing::HasSubstr("does not contain a full MSI image"));
   }
   catch ( ... ) {
     FAIL() << "Expected error: ...does not contain a full MSI image...";
   }
}

TEST_F(TempTestingFiles, Msi2isisTestEInvalidSampleDirection) {
   Pvl appLog;
   QString cubeFileName = tempDir.path() + "/bad_msi.cub";
   QVector<QString> args = {"from=data/near/msi2isis/label4.lbl",
                            "to=" + cubeFileName };

   UserInterface options(APP_XML, args);

   // TEST E: Invalid Sample Display Direction
   try {
     msi2isis(options, &appLog);
     FAIL() << "Invalid SAMPLE_DISPLAY_DIRECTION should throw an exception." <<std::endl;
   }
   catch (IException &e ) {
     EXPECT_THAT(e.what(), testing::HasSubstr("has an invalid value for SAMPLE_DISPLAY_DIRECTION"));
   }
   catch ( ... ) {
     FAIL() << "Expected error: ...has an invalid value for SAMPLE_DISPLAY_DIRECTION...";
   }
}

TEST_F(TempTestingFiles, Msi2isisTestFInvalidLineDirection) {
   Pvl appLog;
   QString cubeFileName = tempDir.path() + "/bad_msi.cub";
   QVector<QString> args = {"from=data/near/msi2isis/label5.lbl",
                            "to=" + cubeFileName };

   UserInterface options(APP_XML, args);

   // TEST F: Invalid Line Display Direction
   try {
     msi2isis(options, &appLog);
     FAIL() << "Invalid LINE_DISPLAY_DIRECTION should throw an exception." <<std::endl;
   }
   catch (IException &e ) {
     EXPECT_THAT(e.what(), testing::HasSubstr("has an invalid value for LINE_DISPLAY_DIRECTION"));
   }
   catch ( ... ) {
     FAIL() << "Expected error: ...has an invalid value for LINE_DISPLAY_DIRECTION...";
   }
}

TEST_F(TempTestingFiles, Msi2isisTestGInvalidProjectedImage) {
   Pvl appLog;
   QString cubeFileName = tempDir.path() + "/bad_msi.cub";
   QVector<QString> args = {"from=data/near/msi2isis/label6.lbl",
                            "to=" + cubeFileName };

   UserInterface options(APP_XML, args);

   // TEST G: Unable to import projected images
   try {
     msi2isis(options, &appLog);
     FAIL() << "Invalid projected file not support should throw an exception." <<std::endl;
   }
   catch (IException &e ) {
     EXPECT_THAT(e.what(), testing::HasSubstr("This program only imports images that have not been projected"));
   }
   catch ( ... ) {
     FAIL() << "Expected error: ...his program only imports images that have not been projected...";
   }
}

TEST_F(TempTestingFiles, Msi2isisTestHLabelTranslateError) {
   Pvl appLog;
   QString cubeFileName = tempDir.path() + "/bad_msi.cub";
   QVector<QString> args = {"from=data/near/msi2isis/label7.lbl",
                            "to=" + cubeFileName };

   UserInterface options(APP_XML, args);

   // TEST H: Unable to translate labels
   try {
     msi2isis(options, &appLog);
     FAIL() << "Invalid label translation should throw an exception." <<std::endl;
   }
   catch (IException &e ) {
     EXPECT_THAT(e.what(), testing::HasSubstr("Unable to translate the labels from "));
   }
   catch ( ... ) {
     FAIL() << "Expected error: ...Unable to translate the labels from ...";
   }
}