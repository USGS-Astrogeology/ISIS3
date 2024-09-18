#include "TempFixtures.h"
#include "Pvl.h"
#include "PvlGroup.h"
#include "Histogram.h"
#include "TestUtilities.h"

#include "thm2isis.h"

#include "gtest/gtest.h"

using namespace Isis;

static QString APP_XML = QString::fromStdString(FileName("$ISISROOT/bin/xml/thm2isis.xml").expanded());

TEST_F(TempTestingFiles, FunctionalTestThm2isisVis) {
  // tempDir exists if the fixture subclasses TempTestingFiles, which most do
  QString outCubeFileName = tempDir.path() + "/test.cub";
  QVector<QString> args = {"from=data/thm2isis/V00821003RDR_cropped.QUB",  "to="+outCubeFileName};

  UserInterface options(APP_XML, args);
  try {
    thm2isis(options);
  }
  catch (IException &e) {
    FAIL() << "Unable to open image: " << e.what() << std::endl;
  }

  // open even cube
  Cube evenCube( tempDir.path().toStdString() + "/test.even.cub");
  Pvl *isisLabel = evenCube.label();

  // Dimensions Group
  EXPECT_EQ(evenCube.sampleCount(), 1024);
  EXPECT_EQ(evenCube.lineCount(), 200);
  EXPECT_EQ(evenCube.bandCount(), 5);

  // Pixels Group
  EXPECT_EQ(PixelTypeName(evenCube.pixelType()), "Real");
  EXPECT_EQ(ByteOrderName(evenCube.byteOrder()), "Lsb");
  EXPECT_DOUBLE_EQ(evenCube.base(), 0.0);
  EXPECT_DOUBLE_EQ(evenCube.multiplier(), 1.0);

  // Instrument Group
  PvlGroup &inst = isisLabel->findGroup("Instrument", Pvl::Traverse);
  EXPECT_EQ(inst["SpacecraftName"][0], "MARS_ODYSSEY");
  EXPECT_EQ(inst["InstrumentId"][0], "THEMIS_VIS" );
  EXPECT_EQ(inst["TargetName"][0], "MARS" );
  EXPECT_EQ(inst["SpacecraftClockCount"][0], "698642092.025" );
  EXPECT_EQ(inst["ExposureDuration"][0], "6.0" );
  EXPECT_EQ(inst["StartTime"][0], "2002-02-20T03:14:02.471000" );
  EXPECT_EQ(inst["StopTime"][0], "2002-02-20T03:14:09.471000" );
  EXPECT_EQ(inst["Framelets"][0], "Even" );
  EXPECT_EQ(inst["InterframeDelay"][0], "1.0" );
  EXPECT_EQ(int(inst["NumFramelets"]), 1);

  // Archive Group
  PvlGroup &archive = isisLabel->findGroup("Archive", Pvl::Traverse);
  EXPECT_EQ(archive["DataSetId"][0], "ODY-M-THM-3-VISRDR-V1.0" );
  EXPECT_EQ(archive["ProductId"][0], "V00821003RDR" );
  EXPECT_EQ(archive["ProductCreationTime"][0], "2003-07-08T03:07:17" );
  EXPECT_EQ(double(archive["ProductVersionId"]), 1.3);

  // BandBin Group
  PvlGroup &bandbin = isisLabel->findGroup("BandBin", Pvl::Traverse);
  EXPECT_EQ(bandbin["OriginalBand"].size(), 5);
  EXPECT_EQ(bandbin["Center"].size(), 5);
  EXPECT_EQ(bandbin["Width"].size(), 5);
  EXPECT_EQ(bandbin["FilterNumber"].size(), 5);

  // Kernels Group
  PvlGroup &kernel = isisLabel->findGroup("Kernels", Pvl::Traverse);
  EXPECT_EQ(int(kernel["NaifFrameCode"]), -53032);

  std::unique_ptr<Histogram> hist (evenCube.histogram());

  EXPECT_NEAR(hist->Average(), 0.0012095900723426705, 0.0001);
  EXPECT_NEAR(hist->Sum(), 9.5743556655943394, .00001);
  EXPECT_EQ(hist->ValidPixels(), 7920);
  EXPECT_NEAR(hist->StandardDeviation(), 2.241887e-05, .00001);

  // open odd cube
  Cube oddCube( tempDir.path().toStdString() + "/test.odd.cub");
  isisLabel = oddCube.label();

  // Dimensions Group
  EXPECT_EQ(oddCube.sampleCount(), 1024);
  EXPECT_EQ(oddCube.lineCount(), 200);
  EXPECT_EQ(oddCube.bandCount(), 5);

  // Pixels Group
  EXPECT_EQ(PixelTypeName(oddCube.pixelType()), "Real");
  EXPECT_EQ(ByteOrderName(oddCube.byteOrder()), "Lsb");
  EXPECT_DOUBLE_EQ(oddCube.base(), 0.0);
  EXPECT_DOUBLE_EQ(oddCube.multiplier(), 1.0);

  // Instrument Group
  inst = isisLabel->findGroup("Instrument", Pvl::Traverse);
  EXPECT_EQ(inst["SpacecraftName"][0], "MARS_ODYSSEY");
  EXPECT_EQ(inst["InstrumentId"][0], "THEMIS_VIS" );
  EXPECT_EQ(inst["TargetName"][0], "MARS" );
  EXPECT_EQ(inst["SpacecraftClockCount"][0], "698642092.025" );
  EXPECT_EQ(inst["ExposureDuration"][0], "6.0" );
  EXPECT_EQ(inst["StartTime"][0], "2002-02-20T03:14:02.471000" );
  EXPECT_EQ(inst["StopTime"][0], "2002-02-20T03:14:09.471000" );
  EXPECT_EQ(inst["Framelets"][0], "Odd" );
  EXPECT_EQ(inst["InterframeDelay"][0], "1.0" );
  EXPECT_EQ(int(inst["NumFramelets"]), 1);

  // Archive Group
  archive = isisLabel->findGroup("Archive", Pvl::Traverse);
  EXPECT_EQ(archive["DataSetId"][0], "ODY-M-THM-3-VISRDR-V1.0" );
  EXPECT_EQ(archive["ProductId"][0], "V00821003RDR" );
  EXPECT_EQ(archive["ProductCreationTime"][0], "2003-07-08T03:07:17" );
  EXPECT_EQ(double(archive["ProductVersionId"]), 1.3);

  // BandBin Group
  bandbin = isisLabel->findGroup("BandBin", Pvl::Traverse);
  EXPECT_EQ(bandbin["OriginalBand"].size(), 5);
  EXPECT_EQ(bandbin["Center"].size(), 5);
  EXPECT_EQ(bandbin["Width"].size(), 5);
  EXPECT_EQ(bandbin["FilterNumber"].size(), 5);

  // Kernels Group
  kernel = isisLabel->findGroup("Kernels", Pvl::Traverse);
  EXPECT_EQ(int(kernel["NaifFrameCode"]), -53032);

  hist.reset(oddCube.histogram());

  EXPECT_NEAR(hist->Average(), 0.0012095900723426705, 0.0001);
  EXPECT_NEAR(hist->Sum(), 228.48262293543667, .00001);
  EXPECT_EQ(hist->ValidPixels(), 188100);
  EXPECT_NEAR(hist->StandardDeviation(), 2.241887e-05, .00001);
}


TEST_F(TempTestingFiles, FunctionalTestThm2isisIr) {
  QString newLabelPath = "data/thm2isis/I00831002RDR_cropped.QUB" ;

  // tempDir exists if the fixture subclasses TempTestingFiles, which most do
  QString outCubeFileName = tempDir.path() + "/test.cub";
  QVector<QString> args = {"from=" + newLabelPath,  "to="+outCubeFileName};

  UserInterface options(APP_XML, args);
  try {
    thm2isis(options);
  }
  catch (IException &e) {
    FAIL() << "Unable to open image: " << e.what() << std::endl;
  }

  Cube oCube(outCubeFileName.toStdString());
  Pvl *isisLabel = oCube.label();

  // Dimensions Group
  EXPECT_EQ(oCube.sampleCount(), 10);
  EXPECT_EQ(oCube.lineCount(), 5);
  EXPECT_EQ(oCube.bandCount(), 10);

  // Pixels Group
  EXPECT_EQ(PixelTypeName(oCube.pixelType()), "Real");
  EXPECT_EQ(ByteOrderName(oCube.byteOrder()), "Lsb");
  EXPECT_DOUBLE_EQ(oCube.base(), 0.0);
  EXPECT_DOUBLE_EQ(oCube.multiplier(), 1.0);

  // Instrument Group
  PvlGroup &inst = isisLabel->findGroup("Instrument", Pvl::Traverse);

  EXPECT_EQ(inst["SpacecraftName"][0], "MARS_ODYSSEY");
  EXPECT_EQ(inst["InstrumentId"][0], "THEMIS_IR" );
  EXPECT_EQ(inst["TargetName"][0], "MARS" );
  EXPECT_EQ(inst["SpacecraftClockCount"][0], "698713127.000" );
  EXPECT_EQ(inst["StartTime"][0], "2002-02-20T22:57:57.253000" );
  EXPECT_EQ(inst["StopTime"][0], "2002-02-20T23:00:56.983000" );
  EXPECT_EQ((int)inst["GainNumber"], 16);

  // Archive Group
  PvlGroup &archive = isisLabel->findGroup("Archive", Pvl::Traverse);
  EXPECT_EQ(archive["DataSetId"][0], "ODY-M-THM-3-IRRDR-V1.0" );
  EXPECT_EQ(archive["ProductId"][0], "I00831002RDR" );
  EXPECT_EQ(archive["ProductCreationTime"][0], "2003-03-12T12:59:33" );
  EXPECT_EQ(double(archive["ProductVersionId"]), 1.4);

  // BandBin Group
  PvlGroup &bandbin = isisLabel->findGroup("BandBin", Pvl::Traverse);
  EXPECT_EQ(bandbin["OriginalBand"].size(), 10);
  EXPECT_EQ(bandbin["Center"].size(), 10);
  EXPECT_EQ(bandbin["Width"].size(), 10);
  EXPECT_EQ(bandbin["FilterNumber"].size(), 10);

  std::unique_ptr<Histogram> hist (oCube.histogram());

  EXPECT_NEAR(hist->Minimum(), 0.00029065093258395791, 0.0001);
  EXPECT_NEAR(hist->Maximum(), 0.00064912717789411545, 0.0001);
  EXPECT_NEAR(hist->Average(), 0.00047608536842744795, 0.0001);
  EXPECT_NEAR(hist->Sum(), 0.023804268421372399, .00001);
  EXPECT_EQ(hist->ValidPixels(), 50);
  EXPECT_NEAR(hist->StandardDeviation(), 0.00011232993701816659, .00001);
}


TEST_F(TempTestingFiles, FunctionalTestThm2isisOutAttributes) {
  // tempDir exists if the fixture subclasses TempTestingFiles, which most do
  QString outCubeFileName = tempDir.path() + "/test.cub+msb+8bit+0.0012:0.0013";
  QVector<QString> args = {"from=data/thm2isis/V00821003RDR_cropped.QUB",  "to="+outCubeFileName};

  UserInterface options(APP_XML, args);
  try {
    thm2isis(options);
  }
  catch (IException &e) {
    FAIL() << "Unable to open image: " << e.what() << std::endl;
  }

  // open even cube
  Cube evenCube( tempDir.path().toStdString() + "/test.even.cub");

  // Pixels Group
  EXPECT_EQ(PixelTypeName(evenCube.pixelType()), "UnsignedByte");
  EXPECT_EQ(ByteOrderName(evenCube.byteOrder()), "Msb");
  EXPECT_DOUBLE_EQ(evenCube.base(), 0.001199604743083);
  EXPECT_DOUBLE_EQ(evenCube.multiplier(), 3.95256916996048e-07);

  std::unique_ptr<Histogram> hist (evenCube.histogram());

  EXPECT_NEAR(hist->Minimum(), 0.0012, 0.0001);
  EXPECT_NEAR(hist->Maximum(), 0.0013, 0.0001);
  EXPECT_NEAR(hist->Average(), 0.00122, 0.0001);
  EXPECT_NEAR(hist->Sum(), 6.6702830039524876, .00001);
  EXPECT_EQ(hist->ValidPixels(), 5472);
  EXPECT_NEAR(hist->StandardDeviation(), 1.5069986471567319e-05, .00001);
}

