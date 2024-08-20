#include <QTemporaryDir>
#include <QFile>

#include "eis2isis.h"
#include "Pvl.h"
#include "PvlGroup.h"
#include "TestUtilities.h"
#include "Histogram.h"

#include "gmock/gmock.h"
#include "gtest/gtest.h"

using namespace Isis;


static QString APP_XML = FileName("$ISISROOT/bin/xml/eis2isis.xml").expanded();

TEST(Eis2Isis, Eis2IsisTestNacDefault) {
  /*
  To test proper run with just the main image and an associated times file
	  $(APPNAME) $(TSTARGS) from=$(INPUT)/simulated_clipper_eis_nac_rolling_shutter.xml \
	  to=$(OUTPUT)/simulated_clipper_eis_nac_rolling_shutter2.cub \
	  mainreadout=$(INPUT)/simulated_clipper_eis_nac_rolling_shutter_times.csv;
  */

  QTemporaryDir prefix;
  QString cubeFileName = prefix.path() + "/eis2isisTEMP.cub";

  QVector<QString> args = { "from=data/eis2isis/nacRs/simulated_clipper_eis_nac_rolling_shutter_cropped.xml",
                            "to=" + cubeFileName,
                            "mainreadout=data/eis2isis/nacRs/simulated_clipper_eis_nac_rolling_shutter_times_cropped.csv" };

  UserInterface options(APP_XML, args);

  try {
    eis2isis(options);
  }
  catch (IException &e) {
    FAIL() << "Unable to ingest image: " << e.toString().toStdString().c_str() << std::endl;
  }

  Cube cube(cubeFileName);
  Pvl *isisLabel = cube.label();

  // Dimensions Group
  ASSERT_EQ(cube.sampleCount(), 3);
  ASSERT_EQ(cube.lineCount(), 60);
  ASSERT_EQ(cube.bandCount(), 1);

  // Instrument Group
  PvlGroup &inst = isisLabel->findGroup("Instrument", Pvl::Traverse);
  ASSERT_EQ(inst["SpacecraftName"][0], "Clipper");
  ASSERT_EQ(inst["InstrumentId"][0], "EIS-NAC-RS" );
  ASSERT_EQ(inst["TargetName"][0], "Europa" );
  ASSERT_EQ(inst["StartTime"][0], "2025-01-01T00:00:00.000" );
  ASSERT_EQ(inst["JitterLineCoefficients"][0], "0.0");
  ASSERT_EQ(inst["JitterLineCoefficients"][1], "0.0");
  ASSERT_EQ(inst["JitterLineCoefficients"][2], "0.0");
  ASSERT_EQ(inst["JitterSampleCoefficients"][0], "0.0");
  ASSERT_EQ(inst["JitterSampleCoefficients"][1], "0.0");
  ASSERT_EQ(inst["JitterSampleCoefficients"][2], "0.0");

  // Kernels Group
  PvlGroup &kernels = isisLabel->findGroup("Kernels", Pvl::Traverse);
  ASSERT_EQ(int(kernels["NaifFrameCode"]), -159101 );

  Histogram *hist = cube.histogram();

  ASSERT_NEAR(hist->Average(), 6.1489e+17, 1e+13);
  ASSERT_NEAR(hist->Sum(), 1.1068e+20, 1e+16);
  ASSERT_EQ(hist->ValidPixels(), 180);
  ASSERT_NEAR(hist->StandardDeviation(),  1.2004e+19, 1e+15);
}

TEST(Eis2Isis, Eis2IsisTestNacCheckline)
{
  /*
  	# To test proper run with a checkline image and a checkline times file added
	$(APPNAME) from=$(INPUT)/simulated_clipper_eis_nac_rolling_shutter.xml \
	  from2=$(INPUT)/simulated_clipper_eis_nac_rolling_shutter_checkline.xml \
	  to=$(OUTPUT)/simulated_clipper_eis_nac_rolling_shutter1.cub \
	  to2=$(OUTPUT)/simulated_clipper_eis_nac_rolling_shutter_checkline1.cub \
	  mainreadout=$(INPUT)/simulated_clipper_eis_nac_rolling_shutter_times.csv \
	  checklinereadout=$(INPUT)/simulated_clipper_eis_nac_rolling_shutter_checkline_times.csv;
  */

  QTemporaryDir prefix;
  QString cubeFileName = prefix.path() + "/eis2isisTEMP.cub";
  QString cubeFileName_checkline = prefix.path() + "/eis2isisTEMP_checkline.cub";

  QVector<QString> args = { "from=data/eis2isis/nacRs/simulated_clipper_eis_nac_rolling_shutter_cropped.xml",
                            "from2=data/eis2isis/nacRs/simulated_clipper_eis_nac_rolling_shutter_checkline_cropped.xml",
                            "to=" + cubeFileName,
                            "to2=" + cubeFileName_checkline,
                            "mainreadout=data/eis2isis/nacRs/simulated_clipper_eis_nac_rolling_shutter_times_cropped.csv",
                            "checklinereadout=data/eis2isis/nacRs/simulated_clipper_eis_nac_rolling_shutter_checkline_times_cropped.csv" };

  UserInterface options(APP_XML, args);

  try {
    eis2isis(options);
  }
  catch (IException &e) {
    FAIL() << "Unable to ingest image: " << e.toString().toStdString().c_str() << std::endl;
  }

  Cube cube(cubeFileName);
  Pvl *isisLabel = cube.label();

  // Dimensions Group
  ASSERT_EQ(cube.sampleCount(), 3);
  ASSERT_EQ(cube.lineCount(), 60);
  ASSERT_EQ(cube.bandCount(), 1);

  // Instrument Group
  PvlGroup &inst = isisLabel->findGroup("Instrument", Pvl::Traverse);
  ASSERT_EQ(inst["SpacecraftName"][0], "Clipper");
  ASSERT_EQ(inst["InstrumentId"][0], "EIS-NAC-RS" );
  ASSERT_EQ(inst["TargetName"][0], "Europa" );
  ASSERT_EQ(inst["StartTime"][0], "2025-01-01T00:00:00.000" );
  ASSERT_EQ(inst["JitterLineCoefficients"][0], "0.0");
  ASSERT_EQ(inst["JitterLineCoefficients"][1], "0.0");
  ASSERT_EQ(inst["JitterLineCoefficients"][2], "0.0");
  ASSERT_EQ(inst["JitterSampleCoefficients"][0], "0.0");
  ASSERT_EQ(inst["JitterSampleCoefficients"][1], "0.0");
  ASSERT_EQ(inst["JitterSampleCoefficients"][2], "0.0");


  // Kernels Group
  PvlGroup &kernels = isisLabel->findGroup("Kernels", Pvl::Traverse);
  ASSERT_EQ(int(kernels["NaifFrameCode"]), -159101 );

  Histogram *hist = cube.histogram();

  ASSERT_NEAR(hist->Average(), 6.1489e+17, 1e+13);
  ASSERT_NEAR(hist->Sum(), 1.1068e+20, 1e+16);
  ASSERT_EQ(hist->ValidPixels(), 180);
  ASSERT_NEAR(hist->StandardDeviation(),  1.2004e+19, 1e+15);

}


TEST(Eis2Isis, Eis2IsisTestNacChecklineError)
{
/*
# To test failed run with a checkline image but no checkline times file
	$(APPNAME) \
			from=$(INPUT)/simulated_clipper_eis_nac_rolling_shutter.xml \
			from2=$(INPUT)/simulated_clipper_eis_nac_rolling_shutter_checkline.xml \
			to=$(OUTPUT)/simulated_clipper_eis_nac_rolling_shutter.cub \
			to2=$(OUTPUT)/simulated_clipper_eis_nac_rolling_shutter_checkline.cub \
			mainreadout=$(INPUT)/simulated_clipper_eis_nac_rolling_shutter_times.csv
*/

  QTemporaryDir prefix;
  QString cubeFileName = prefix.path() + "/eis2isisTEMP.cub";
  QString cubeFileName_checkline = prefix.path() + "/eis2isisTEMP_checkline.cub";

  QVector<QString> args = { "from=data/eis2isis/nacRs/simulated_clipper_eis_nac_rolling_shutter_cropped.xml",
                            "from2=data/eis2isis/nacRs/simulated_clipper_eis_nac_rolling_shutter_checkline_cropped.xml",
                            "to=" + cubeFileName,
                            "to2=" + cubeFileName_checkline,
                            "mainreadout=data/eis2isis/nacRs/simulated_clipper_eis_nac_rolling_shutter_times_cropped.csv" };

  UserInterface options(APP_XML, args);

  try {
    eis2isis(options);
    FAIL();
  }
  catch (IException &e) {
    ASSERT_TRUE(e.toString().contains("as the [CHECKLINEREADOUT] parameter"));
  }

}


TEST(Eis2Isis, Eis2IsisTestWacDefault) {
  /*
	# To test proper run
	$(APPNAME) $(TSTARGS) from=$(INPUT)/simulated_clipper_eis_nac_rolling_shutter.xml \
	  to=$(OUTPUT)/simulated_clipper_eis_nac_rolling_shutter.cub > /dev/null;
  */

  QTemporaryDir prefix;
  QString cubeFileName = prefix.path() + "/eis2isisTEMP.cub";

  QVector<QString> args = { "from=data/eis2isis/wacFc/simulated_clipper_eis_wac_rolling_shutter_cropped.xml",
                            "to=" + cubeFileName };

  UserInterface options(APP_XML, args);

  try {
    eis2isis(options);
  }
  catch (IException &e) {
    FAIL() << "Unable to ingest image: " << e.toString().toStdString().c_str() << std::endl;
  }

  Cube cube(cubeFileName);
  Pvl *isisLabel = cube.label();

  // Dimensions Group
  ASSERT_EQ(cube.sampleCount(), 3);
  ASSERT_EQ(cube.lineCount(), 60);
  ASSERT_EQ(cube.bandCount(), 1);

  // Kernels Group
  PvlGroup &kernels = isisLabel->findGroup("Kernels", Pvl::Traverse);
  ASSERT_EQ(int(kernels["NaifFrameCode"]), -159102 );


  Histogram *hist = cube.histogram();

  ASSERT_NEAR(hist->Average(), -2.0496e+17, 1e+13);
  ASSERT_NEAR(hist->Sum(), -3.6893e+19, 1e+16);
  ASSERT_EQ(hist->ValidPixels(), 180);
  ASSERT_NEAR(hist->StandardDeviation(),  1.3223e+19, 1e+15);
}
