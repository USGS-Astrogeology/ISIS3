#include "CameraFixtures.h"
#include "Pvl.h"
#include "PvlGroup.h"
#include "TestUtilities.h"
#include "LineManager.h"
#include "Histogram.h"

#include "camdev.h"

#include "gtest/gtest.h"

using namespace Isis;

static QString APP_XML = FileName("$ISISROOT/bin/xml/camdev.xml").expanded();

TEST_F(LineScannerCube, FunctionalTestCamdevDefault) {
  LineManager line(*testCube);
  double pixelValue = 0.0;
  for(line.begin(); !line.end(); line++) {
    for(int i = 0; i < line.size(); i++) {
      line[i] = (double) pixelValue++;
    }
    testCube->write(line);
  }

  testCube->reopen("r");

  QString outCubeFileName = tempDir.path() + "/outTEMP.cub";

  QVector<QString> args = {"to="+outCubeFileName,
  	"dn=yes",
	  "radec=yes",
	  "planetographiclatitude=yes",
	  "positiveeast360longitude=yes",
	  "positiveeast180longitude=yes",
	  "positivewest360longitude=yes",
	  "positivewest180longitude=yes",
	  "bodyfixed=yes",
	  "localradius=yes",
	  "pixelresolution=yes",
	  "lineresolution=yes",
	  "sampleresolution=yes",
	  "detectorresolution=yes",
	  "spacecraftposition=yes",
	  "spacecraftazimuth=yes",
	  "slantdistance=yes",
	  "targetcenterdistance=yes",
	  "subspacecraftlatitude=yes",
	  "subspacecraftlongitude=yes",
	  "spacecraftaltitude=yes",
	  "offnadirangle=yes",
	  "subspacecraftgroundazimuth=y", \
	  "sunposition=yes",
	  "sunazimuth=yes",
	  "solardistance=yes",
	  "subsolarlatitude=yes",
	  "subsolarlongitude=yes",
	  "subsolargroundazimuth=yes",
	  "phase=yes",
	  "emission=yes",
	  "incidence=yes",
	  "localemission=yes",
	  "localincidence=yes",
	  "northazimuth=yes",
	  "distortedfocalplane=yes",
	  "undistortedfocalplane=yes",
	  "ephemeristime=yes",
	  "utc=yes",
	  "localsolartime=yes",
	  "solarlongitude=yes",
	  "morphology=yes",
	  "albedo=y",
  };

  UserInterface options(APP_XML, args);
  try {
    camdev(testCube, options);
  }
  catch (IException &e) {
    FAIL() << "Unable to process image: " << e.what() << std::endl;
  }

  Cube oCube(outCubeFileName);
  std::unique_ptr<Histogram> oCubeStats;

  double average = 0;
  double sum = 0;
  double stddev = 0;
  double validPixels = 0;

  for (int i = 1; i <= oCube.bandCount(); i++) {
    oCubeStats.reset(oCube.histogram(i));
    average += oCubeStats->Average();
    sum += oCubeStats->Sum();
    validPixels += oCubeStats->ValidPixels();
    stddev += oCubeStats->StandardDeviation();
  }

	EXPECT_NEAR(average / oCube.bandCount(), 9183553.1942694616, 0.0000001);
	EXPECT_NEAR(sum/oCube.bandCount(), 112847454091.27431, 0.0000001);
  EXPECT_NEAR(stddev/oCube.bandCount(), 11.294379992444282, 0.0000001);
  EXPECT_NEAR(validPixels/oCube.bandCount(), 12064.188679245282, 0.0000001);
}
