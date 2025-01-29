#include <QTemporaryDir>

#include "Fixtures.h"
#include "CSVReader.h"
#include "Pvl.h"
#include "PvlGroup.h"
#include "TestUtilities.h"
#include "Histogram.h"

#include "algebra.h"
#include "automos.h"
#include "autoseed.h"
#include "bandtrim.h"
#include "cam2map.h"
#include "caminfo.h"
#include "campt.h"
#include "camstats.h"
#include "cubeit.h"
#include "explode.h"
#include "findimageoverlaps.h"
#include "footprintinit.h"
#include "getsn.h"
#include "mosrange.h"
#include "noseam.h"
#include "ocams2isis.h"
#include "overlapstats.h"
#include "phocube.h"
#include "pointreg.h"
#include "spiceinit.h"
#include "stats.h"

#include "gtest/gtest.h"

using namespace std;
using namespace Isis;

static QString OCAMS2ISIS_XML = FileName("$ISISROOT/bin/xml/ocams2isis.xml").expanded();
static QString SPICEINIT_XML = FileName("$ISISROOT/bin/xml/spiceinit.xml").expanded();
static QString PHOCUBE_XML = FileName("$ISISROOT/bin/xml/phocube.xml").expanded();
static QString EXPLODE_XML = FileName("$ISISROOT/bin/xml/explode.xml").expanded();
static QString CUBEIT_XML = FileName("$ISISROOT/bin/xml/cubeit.xml").expanded();
static QString ALGEBRA_XML = FileName("$ISISROOT/bin/xml/algebra.xml").expanded();
static QString CAM2MAP_XML = FileName("$ISISROOT/bin/xml/cam2map.xml").expanded();
static QString CAMINFO_XML = FileName("$ISISROOT/bin/xml/caminfo.xml").expanded();
static QString CAMSTATS_XML = FileName("$ISISROOT/bin/xml/camstats.xml").expanded();
static QString STATS_XML = FileName("$ISISROOT/bin/xml/stats.xml").expanded();
static QString FOOTPRINTINIT_XML = FileName("$ISISROOT/bin/xml/footprintinit.xml").expanded();
static QString CAMPT_XML = FileName("$ISISROOT/bin/xml/campt.xml").expanded();
static QString MOSRANGE_XML = FileName("$ISISROOT/bin/xml/mosrange.xml").expanded();
static QString AUTOMOS_XML = FileName("$ISISROOT/bin/xml/automos.xml").expanded();
static QString NOSEAM_XML = FileName("$ISISROOT/bin/xml/noseam.xml").expanded();
static QString BANDTRIM_XML = FileName("$ISISROOT/bin/xml/bandtrim.xml").expanded();
static QString GETSN_XML = FileName("$ISISROOT/bin/xml/getsn.xml").expanded();
static QString FINDIMAGEOVERLAPS_XML = FileName("$ISISROOT/bin/xml/findimageoverlaps.xml").expanded();
static QString OVERLAPSTATS_XML = FileName("$ISISROOT/bin/xml/overlapstats.xml").expanded();
static QString AUTOSEED_XML = FileName("$ISISROOT/bin/xml/autoseed.xml").expanded();
static QString POINTREG_XML = FileName("$ISISROOT/bin/xml/pointreg.xml").expanded();

/**
 * @internal
 *   @history 2016-09-09 Jeannie Backer
 *            - Original Makefile category test versions.
 *            - The programs originally chosen for these tests were copied from the
 *              OSIRIS-REx team's "testIsis" program.
 *            - Original output file names matched those in the "testIsis" program
 *              to make comparison easier.
 *   @history 2025-01-14 Ken Edmundson
 *            - Converted tests from Makefile to ctest module format.
 *            - Using different calibrated PolyCam images from OSIRIS-REx Detailed Survey
 *              (https://sbnarchive.psi.edu/pds4/orex/orex.ocams/data_calibrated/detailed_survey):
 *              20190328T200344S309_pol_iofL2pan.fits
 *              20190328T200717S769_pol_iofL2pan.fits
 *            - Removed original call to coreg and photomet apps as OSIRIS-REx IPWG determined
 *              these are not necessary.
 */


/**
   * PolyCamModuleExplodeReuniteTest
   * 
   * 1) PolyCam image 20190328T200344S309_pol_iofL2pan.fits is ingested with ocams2isis and
   *    spiceinited.
   * 2) phocube is used to create latitude, longitude, phase angle, emission angle,
   *    incidence angle, and pixel resolution backplanes.
   * 3) explode outputs the image backplanes into 7 separate cubes.
   * 4) cubeit reunites the exploded cubes.
   * 5) The test then compares resulting phocube and cubeit cube labels and histograms,
   *    which should be identical.
   * 
   * INPUT: isis/tests/data/osirisRexImages/ocams/20190328T200344S309_pol_iofL2pan.fits
   * 
   * OUTPUT: 1) ocams2isis
   *            - 20190328T200344S309_pol_iofL2pan.cub
   *         2) phocube
   *            - 20190328T200344S309_pol_iofL2pan_bp.cub
   *         3) explode (produces 7 cubes, one for each band, e.g.)
   *            - 20190328T200344S309_pol_iofL2pan_V001.band0001.cub (band0001-band0007)
   *         4) cubeit
   *            - 20190328T200344S309_pol_iofL2pan_V001_cubeit.cub
   */
TEST(OsirisRexPolyCamModules, PolyCamModuleExplodeReuniteTest) {
  QTemporaryDir tempDir;

  // ingest polycam fits format image with ocams2isis
  QString polyCamFileName = tempDir.path() + "/20190328T200344S309_pol_iofL2pan.cub";
  QVector<QString> ocams2isisArgs = {"from=data/osirisRexImages/ocams/20190328T200344S309_pol_iofL2pan.fits",
                                     "to=" + polyCamFileName};

  UserInterface ocams2isisPolyCam(OCAMS2ISIS_XML, ocams2isisArgs);
  try {
    ocams2isis(ocams2isisPolyCam);
  }
  catch (IException &e) {
    FAIL() << "Unable to run ocams2isis on polycam fits file 20190328T200344S309_pol_iofL2pan.fits: "
           << e.what() << std::endl;
  }

  // spiceinit ingested cube
  QVector<QString> spiceinitArgs = {"from=" + polyCamFileName};
  UserInterface spiceinitPolyCam(SPICEINIT_XML, spiceinitArgs);
  try {
    spiceinit(spiceinitPolyCam);
  }
  catch (IException &e) {
    FAIL() << "Unable to run spiceinit on polycam cube 20190328T200344S309_pol_iofL2pan.cub: "
           << e.what() << std::endl;
  }

  // phocube polycam image to create latitude, longitude, phase angle, emission angle,
  // incidence angle, and pixel resolution backplanes;
	// dn=y propagates input pixel to output file
  QString phocubeFileName = tempDir.path() + "/20190328T200344S309_pol_iofL2pan_bp.cub";
  QVector<QString> phocubeArgs = {"from=" + polyCamFileName,
                                  "to=" + phocubeFileName,
                                  "dn=y",
                                  "phase=y",
                                  "emission=y",
                                  "incidence=y",
                                  "pixelresolution=y"};
  UserInterface phocubeUI(PHOCUBE_XML, phocubeArgs);
  try {
    phocube(phocubeUI);
  }
  catch (IException &e) {
    FAIL() << "Unable to run phocube on polycam cube 20190328T200344S309_pol_iofL2pan.cub: "
           << e.what() << std::endl;
  }

  // explode image backplanes into 7 cubes
  QVector<QString> explodeArgs = {"from=" + phocubeFileName,
                                  "to=" + tempDir.path() + "/20190328T200344S309_pol_iofL2pan_V001"};
  UserInterface explodeUI(EXPLODE_XML, explodeArgs);
  try {
    explode(explodeUI);
  }
  catch (IException &e) {
    FAIL() << "Unable to run explode on phocubed polycam cube 20190328T200344S309_pol_iofL2pan_bp.cub: "
           << e.what() << std::endl;
  }

  // reunite exploded cubes
	// output should be identical to phocube bp output above
  FileList *cubeList = new FileList(); // list of exploded cubes
  cubeList->append(tempDir.path() + "/20190328T200344S309_pol_iofL2pan_V001.band0001.cub");
  cubeList->append(tempDir.path() + "/20190328T200344S309_pol_iofL2pan_V001.band0002.cub");
  cubeList->append(tempDir.path() + "/20190328T200344S309_pol_iofL2pan_V001.band0003.cub");
  cubeList->append(tempDir.path() + "/20190328T200344S309_pol_iofL2pan_V001.band0004.cub");
  cubeList->append(tempDir.path() + "/20190328T200344S309_pol_iofL2pan_V001.band0005.cub");
  cubeList->append(tempDir.path() + "/20190328T200344S309_pol_iofL2pan_V001.band0006.cub");
  cubeList->append(tempDir.path() + "/20190328T200344S309_pol_iofL2pan_V001.band0007.cub");
  
  QString cubeListFile = tempDir.path() + "/phocubeBandList.lis";
  cubeList->write(cubeListFile);

  QString cubeitFileName = tempDir.path() + "/20190328T200344S309_pol_iofL2pan_V001_cubeit.cub";

  QVector<QString> cubeitArgs = {"from=" + cubeListFile,
                                  "to=" + cubeitFileName};
  UserInterface cubeitUI(CUBEIT_XML, cubeitArgs);
  try {
    cubeit(cubeitUI);
  }
  catch (IException &e) {
    FAIL() << "Unable to run cubeit on exploded polycam cubes: " << e.what() << std::endl;
  }

  // compare original phocube and cubeit result cubes
  Cube phocubeCube(phocubeFileName);
  Pvl *phocubeLabel = phocubeCube.label();

  Cube cubeitCube(cubeitFileName);
  Pvl *cubeitLabel = cubeitCube.label();

  // Dimensions group
  EXPECT_EQ(phocubeCube.sampleCount(), cubeitCube.sampleCount());
  EXPECT_EQ(phocubeCube.lineCount(), cubeitCube.lineCount());
  EXPECT_EQ(phocubeCube.bandCount(), cubeitCube.bandCount());

  // Instrument Group
  PvlGroup &phocubeInst = phocubeLabel->findGroup("Instrument", Pvl::Traverse);
  PvlGroup &cubeitInst = cubeitLabel->findGroup("Instrument", Pvl::Traverse);
  EXPECT_EQ(phocubeInst["MissionName"], cubeitInst["MissionName"]);
  EXPECT_EQ(phocubeInst["SpacecraftName"], cubeitInst["SpacecraftName"]);
  EXPECT_EQ(phocubeInst["InstrumentId"], cubeitInst["InstrumentId"]);
  EXPECT_EQ(phocubeInst["TargetName"], cubeitInst["TargetName"]);
  EXPECT_EQ(phocubeInst["StartTime"], cubeitInst["StartTime"]);
  EXPECT_EQ(phocubeInst["ExposureDuration"], cubeitInst["ExposureDuration"]);
  EXPECT_EQ(phocubeInst["SpaceCraftClockStartCount"], cubeitInst["SpaceCraftClockStartCount"]);
  EXPECT_EQ(phocubeInst["FocusPosition"], cubeitInst["FocusPosition"]);
  EXPECT_EQ(phocubeInst["PolyCamFocusPositionNaifId"], cubeitInst["PolyCamFocusPositionNaifId"]);

  // Bandbin Group
  PvlGroup &phocubeBand = phocubeLabel->findGroup("BandBin", Pvl::Traverse);
  PvlGroup &cubeitBand = cubeitLabel->findGroup("BandBin", Pvl::Traverse);
  EXPECT_EQ(phocubeBand["FilterName"], cubeitBand["FilterName"]);
  EXPECT_EQ(phocubeBand["Center"], cubeitBand["Center"]);
  EXPECT_EQ(phocubeBand["Name"], cubeitBand["Name"]);
  EXPECT_EQ(phocubeBand["Width"], cubeitBand["Width"]);

  std::unique_ptr<Histogram> phocubeHist (phocubeCube.histogram());
  std::unique_ptr<Histogram> cubeitHist (cubeitCube.histogram());

  EXPECT_NEAR(phocubeHist->Average(), cubeitHist->Average(), 0.0001);
  EXPECT_EQ(phocubeHist->Sum(), cubeitHist->Sum());
  EXPECT_EQ(phocubeHist->ValidPixels(), cubeitHist->ValidPixels());
  EXPECT_EQ(phocubeHist->StandardDeviation(), cubeitHist->StandardDeviation());
}


/**
   * PolyCamModuleAlgebraTest
   * 
   * 1) Polycam image 20190328T200344S309_pol_iofL2pan.fits is ingested with ocams2isis.
   * 2) algebra is used modify the dn values by doubling and adding 1.
   * 3) The test compares the histograms from the original and algebra'd cubes.
   * 
   * INPUT: isis/tests/data/osirisRexImages/ocams/20190328T200344S309_pol_iofL2pan.fits
   * 
   * OUTPUT: 1) ocams2isis
   *            - 20190328T200344S309_pol_iofL2pan.cub
   *         2) algebra
   *            - 20190328T200344S309_pol_iofL2pan_algebra.cub
   */
  TEST(OsirisRexPolyCamModules, PolyCamModuleAlgebraTest) {
  QTemporaryDir tempDir;

  // ingest polycam fits format image with ocams2isis
  QString polyCamFileName = tempDir.path() + "/20190328T200344S309_pol_iofL2pan.cub";
  QVector<QString> ocams2isisArgs = {"from=data/osirisRexImages/ocams/20190328T200344S309_pol_iofL2pan.fits",
                                     "to=" + polyCamFileName};

  UserInterface ocams2isisUI(OCAMS2ISIS_XML, ocams2isisArgs);
  try {
    ocams2isis(ocams2isisUI);
  }
  catch (IException &e) {
    FAIL() << "Unable to run ocams2isis on polycam fits file 20190328T200344S309_pol_iofL2pan.fits: "
           << e.what() << std::endl;
  }

  // modify the dn values with algebra app by doubling and adding 1
  QString algebraFileName = tempDir.path() + "/20190328T200344S309_pol_iofL2pan_algebra.cub";

  QVector<QString> algebraArgs = {"from=" + polyCamFileName,
                                  "to=" + algebraFileName,
                                  "operator=unary",
                                  "a=2",
                                  "c=1"};
  UserInterface algebraUI(ALGEBRA_XML, algebraArgs);
  try {
    algebra(algebraUI);
  }
  catch (IException &e) {
    FAIL() << "Unable to run algebra on polycam cube 20190328T200344S309_pol_iofL2pan.cub: "
           << e.what() << std::endl;
  }

  // compare histograms of original and algebra'd cube
  Cube originalCube(polyCamFileName);
  Cube algebraCube(algebraFileName);
  
  std::unique_ptr<Histogram> originalHist (originalCube.histogram());
  std::unique_ptr<Histogram> algebraHist (algebraCube.histogram());

  // pixels multiplied by two and add one
  EXPECT_NEAR(algebraHist->ValidPixels(), originalHist->ValidPixels(), 0.0001);
  EXPECT_NEAR(algebraHist->Average(), (originalHist->Average()*2.0)+1.0, 0.0001);
  EXPECT_NEAR(algebraHist->Sum(), (originalHist->Sum()*2)+originalHist->ValidPixels(), 0.0001);
  EXPECT_NEAR(algebraHist->StandardDeviation(), (originalHist->StandardDeviation()*2), 0.0001);
}


/**
   * PolyCamModuleInfoAndStatsTest
   * 
   * 1) Polycam image 20190328T200344S309_pol_iofL2pan.fits is ingested with ocams2isis
   *    and spiceinited.
   * 2) getsn generates the cube serial number.
   * 3) caminfo outputs various spacecraft and instrument-related details (e.g.
   *    geometric, polygon, and mapping info).
   * 4) camstats generates camera statistics.
   * 5) stats generates pixel statistics.
   * 6) The test validates the output serial number, info, and statistics files.
   * 
   * INPUT: isis/tests/data/osirisRexImages/ocams/20190328T200344S309_pol_iofL2pan.fits
   * 
   * OUTPUT: 1) ocams2isis
   *            - 20190328T200344S309_pol_iofL2pan.cub
   *         2) getsn
   *            - polycam_getsn.pvl
   *         3) caminfo
   *            - 20190328T200344S309_pol_iofL2pan_caminfo.pvl
   *         4) camstats
   *            - 20190328T200344S309_pol_iofL2pan_camstats.pvl
   *         5) stats
   *            - 20190328T200344S309_pol_iofL2pan_dnstats.pvl
   */ 
  TEST(OsirisRexPolyCamModules, PolyCamModuleInfoAndStatsTest) {
  QTemporaryDir tempDir;

  // ingest polycam fits format image with ocams2isis
  QString polyCamFileName = tempDir.path() + "/20190328T200344S309_pol_iofL2pan.cub";
  QVector<QString> ocams2isisArgs = {"from=data/osirisRexImages/ocams/20190328T200344S309_pol_iofL2pan.fits",
                                     "to=" + polyCamFileName};

  UserInterface ocams2isisUI(OCAMS2ISIS_XML, ocams2isisArgs);
  try {
    ocams2isis(ocams2isisUI);
  }
  catch (IException &e) {
    FAIL() << "Unable to run ocams2isis on polycam fits file 20190328T200344S309_pol_iofL2pan.fits: "
           << e.what() << std::endl;
  }

  // spiceinit
  QVector<QString> spiceinitArgs = {"from=" + polyCamFileName};
  UserInterface spiceinitUI(SPICEINIT_XML, spiceinitArgs);
  try {
    spiceinit(spiceinitUI);
  }
  catch (IException &e) {
    FAIL() << "Unable to run spiceinit on polycam cube 20190328T200344S309_pol_iofL2pan.cub: "
           << e.what() << std::endl;
  }

  // getsn from polycam image
  QVector<QString> getsnArgs = {"from=" + polyCamFileName,
                                 "to=" + tempDir.path() + "/polycam_getsn.pvl",
                                 "append=yes"};
  UserInterface getsnUI(GETSN_XML, getsnArgs);
  Pvl appLog;
  try {
    getsn(getsnUI, &appLog);
  }
  catch (IException &e) {
    FAIL() << "Unable to run getsn on polycam cube 20190328T200344S309_pol_iofL2pan.cub: "
           << e.what() << std::endl;
  }

  // validate polycam image serial number
  Pvl getsnPvl;
  getsnPvl.read(tempDir.path() + "/polycam_getsn.pvl");

  // Results Group
  PvlGroup &getSNResultsGroup = getsnPvl.findGroup("Results", Pvl::Traverse);
  EXPECT_EQ(getSNResultsGroup["SerialNumber"][0].toStdString(), "OsirisRex/PolyCam/3/0607075384.53799" );

  // get camera information
  QVector<QString> caminfoArgs = {"from=" + polyCamFileName,
                                  "to=" + tempDir.path() + "/20190328T200344S309_pol_iofL2pan_caminfo.pvl"};
  UserInterface caminfoUI(CAMINFO_XML, caminfoArgs);
  try {
    caminfo(caminfoUI);
  }
  catch (IException &e) {
    FAIL() << "Unable to run caminfo on polycam cube 20190328T200344S309_pol_iofL2pan.cub: "
           << e.what() << std::endl;
  }

  // validate caminfo file
  Pvl caminfoPvl;
  caminfoPvl.read(tempDir.path() + "/20190328T200344S309_pol_iofL2pan_caminfo.pvl");

  // Parameters Object
  PvlObject &parametersObject = caminfoPvl.findObject("Parameters", Pvl::Traverse);
  EXPECT_EQ(parametersObject["Program"][0].toStdString(), "caminfo");
  EXPECT_EQ(parametersObject["IsisId"][0].toStdString(), "OsirisRex/PolyCam/3/0607075384.53799");
  EXPECT_EQ(parametersObject["From"][0].toStdString(), "20190328T200344S309_pol_iofL2pan.cub" );
  EXPECT_EQ(int(parametersObject["Lines"]), 1024);
  EXPECT_EQ(int(parametersObject["Samples"]), 1024);
  EXPECT_EQ(int(parametersObject["Bands"]), 1);

  // Geometry Object
  PvlObject &geometryObject = caminfoPvl.findObject("Geometry", Pvl::Traverse);
  EXPECT_EQ(int(geometryObject["BandsUsed"]), 1);
  EXPECT_EQ(int(geometryObject["ReferenceBand"]), 1);
  EXPECT_EQ(int(geometryObject["OriginalBand"]), 1);
  EXPECT_EQ(geometryObject["Target"][0].toStdString(), "Bennu");
  EXPECT_EQ(geometryObject["StartTime"][0].toStdString(), "2019-03-28T20:03:44.2740743");
  EXPECT_EQ(geometryObject["EndTime"][0].toStdString(), "2019-03-28T20:03:44.2740743" );
  EXPECT_DOUBLE_EQ(double(geometryObject["CenterLine"]), 512.0);
  EXPECT_DOUBLE_EQ(double(geometryObject["CenterSample"]), 512.0);
  EXPECT_NEAR(double(geometryObject.findKeyword("CenterLatitude")), -2.186073429909099, 1e-8);
  EXPECT_NEAR(double(geometryObject.findKeyword("CenterLongitude")), 33.611533123577999, 1e-8);
  EXPECT_NEAR(double(geometryObject.findKeyword("CenterRadius")), 279.21917057972001, 1e-8);
  EXPECT_NEAR(double(geometryObject.findKeyword("RightAscension")), 159.36649322592001, 1e-8);
  EXPECT_NEAR(double(geometryObject.findKeyword("Declination")), -37.119791880648997, 1e-8);
  EXPECT_NEAR(double(geometryObject.findKeyword("UpperLeftLongitude")), 27.484944488052999, 1e-8);
  EXPECT_NEAR(double(geometryObject.findKeyword("UpperLeftLatitude")), -6.0772752213547996, 1e-8);
  EXPECT_NEAR(double(geometryObject.findKeyword("LowerLeftLongitude")), 30.195774258998, 1e-8);
  EXPECT_NEAR(double(geometryObject.findKeyword("LowerLeftLatitude")), 6.0970614203915998, 1e-8);
  EXPECT_NEAR(double(geometryObject.findKeyword("LowerRightLongitude")), 39.797105432195998, 1e-8);
  EXPECT_NEAR(double(geometryObject.findKeyword("LowerRightLatitude")), 2.6310940923814998, 1e-8);
  EXPECT_NEAR(double(geometryObject.findKeyword("UpperRightLongitude")), 37.020066086413003, 1e-8);
  EXPECT_NEAR(double(geometryObject.findKeyword("UpperRightLatitude")), -9.2686382652325996, 1e-8);
  EXPECT_NEAR(double(geometryObject.findKeyword("PhaseAngle")), 49.397794786010998, 1e-8);
  EXPECT_NEAR(double(geometryObject.findKeyword("EmissionAngle")), 36.623634095367997, 1e-8);
  EXPECT_NEAR(double(geometryObject.findKeyword("IncidenceAngle")), 28.204225429331998, 1e-8);
  EXPECT_NEAR(double(geometryObject.findKeyword("NorthAzimuth")), 106.15937800693, 1e-8);
  EXPECT_NEAR(double(geometryObject.findKeyword("OffNadir")), 2.0738688480295, 1e-8);
  EXPECT_NEAR(double(geometryObject.findKeyword("SolarLongitude")), 62.759024139381999, 1e-8);
  EXPECT_NEAR(double(geometryObject.findKeyword("LocalTime")), 9.9949365498485996, 1e-8);
  EXPECT_NEAR(double(geometryObject.findKeyword("TargetCenterDistance")), 3.7078996302574998, 1e-8);
  EXPECT_NEAR(double(geometryObject.findKeyword("SlantDistance")), 3.4815282829207002, 1e-8);
  EXPECT_NEAR(double(geometryObject.findKeyword("SampleResolution")), 0.047107151100130001, 1e-8);
  EXPECT_NEAR(double(geometryObject.findKeyword("LineResolution")), 0.047107151100130001, 1e-8);
  EXPECT_NEAR(double(geometryObject.findKeyword("PixelResolution")), 0.047107151100130001, 1e-8);
  EXPECT_NEAR(double(geometryObject.findKeyword("MeanGroundResolution")), 0.047172966686957002, 1e-8);
  EXPECT_NEAR(double(geometryObject.findKeyword("SubSolarAzimuth")), 23.144034265352001, 1e-8);
  EXPECT_NEAR(double(geometryObject.findKeyword("SubSolarGroundAzimuth")), 81.921732948274993, 1e-8);
  EXPECT_NEAR(double(geometryObject.findKeyword("SubSolarLatitude")), 2.1846905951308999, 1e-8);
  EXPECT_NEAR(double(geometryObject.findKeyword("SubSolarLongitude")), 63.687484875849997, 1e-8);
  EXPECT_NEAR(double(geometryObject.findKeyword("SubSpacecraftAzimuth")), 287.63411245930001, 1e-8);
  EXPECT_NEAR(double(geometryObject.findKeyword("SubSpacecraftGroundAzimuth")), 178.82451866506, 1e-8);
  EXPECT_NEAR(double(geometryObject.findKeyword("SubSpacecraftLatitude")), -36.727312596628998, 1e-8);
  EXPECT_NEAR(double(geometryObject.findKeyword("SubSpacecraftLongitude")), 34.443255565085998, 1e-8);
  EXPECT_NEAR(double(geometryObject.findKeyword("ParallaxX")), 0.74314931590302002, 1e-8);
  EXPECT_NEAR(double(geometryObject.findKeyword("ParallaxY")), 0.015248605398993, 1e-8);
  EXPECT_NEAR(double(geometryObject.findKeyword("ShadowX")), -0.075362559767212997, 1e-8);
  EXPECT_NEAR(double(geometryObject.findKeyword("ShadowY")), 0.53096865870444998, 1e-8);
  EXPECT_EQ(geometryObject["HasLongitudeBoundary"][0].toStdString(), "FALSE");
  EXPECT_EQ(geometryObject["HasNorthPole"][0].toStdString(), "FALSE");
  EXPECT_EQ(geometryObject["HasSouthPole"][0].toStdString(), "FALSE");
  EXPECT_NEAR(double(geometryObject.findKeyword("ObliqueSampleResolution")), 0.058695272573753003, 1e-8);
  EXPECT_NEAR(double(geometryObject.findKeyword("ObliqueLineResolution")), 0.058695272573753003, 1e-8);
  EXPECT_NEAR(double(geometryObject.findKeyword("ObliquePixelResolution")), 0.058695272573753003, 1e-8);
  EXPECT_NEAR(double(geometryObject.findKeyword("ObliqueDetectorResolution")), 0.058695272573753003, 1e-8);

  // get camera statistics
  QVector<QString> camstatsArgs = {"from=" + polyCamFileName,
                                  "to=" + tempDir.path() + "/20190328T200344S309_pol_iofL2pan_camstats.pvl"};
  UserInterface camstatsUI(CAMSTATS_XML, camstatsArgs);

  try {
    camstats(camstatsUI, &appLog);
  }
  catch (IException &e) {
    FAIL() << "Unable to run camstats on polycam cube 20190328T200344S309_pol_iofL2pan.cub: "
           << e.what() << std::endl;
  }

  // validate camstats file
  Pvl camstatsPvl;
  camstatsPvl.read(tempDir.path() + "/20190328T200344S309_pol_iofL2pan_camstats.pvl");

  // User Parameters Group
  PvlGroup &userParametersGroup = camstatsPvl.findGroup("User Parameters", Pvl::Traverse);
  EXPECT_EQ(int(userParametersGroup["Linc"]), 1);
  EXPECT_EQ(int(userParametersGroup["Sinc"]), 1);

  // Latitude Group
  PvlGroup &latitudeGroup = camstatsPvl.findGroup("Latitude", Pvl::Traverse);
  EXPECT_NEAR((double)latitudeGroup.findKeyword("LatitudeMinimum"), -9.2686382652326, 1e-8);
  EXPECT_NEAR((double)latitudeGroup.findKeyword("LatitudeMaximum"), 6.0970614203916, 1e-8);
  EXPECT_NEAR((double)latitudeGroup.findKeyword("LatitudeAverage"), -2.0088179692259, 1e-8);
  EXPECT_NEAR((double)latitudeGroup.findKeyword("LatitudeStandardDeviation"), 3.5853384406358, 1e-8);

  // Longitude Group
  PvlGroup &longitudeGroup = camstatsPvl.findGroup("Longitude", Pvl::Traverse);
  EXPECT_NEAR((double)longitudeGroup.findKeyword("LongitudeMinimum"), 27.484944488053, 1e-8);
  EXPECT_NEAR((double)longitudeGroup.findKeyword("LongitudeMaximum"), 39.797105432196, 1e-8);
  EXPECT_NEAR((double)longitudeGroup.findKeyword("LongitudeAverage"), 33.620095384077, 1e-8);
  EXPECT_NEAR((double)longitudeGroup.findKeyword("LongitudeStandardDeviation"), 2.8651178515503, 1e-8);

  // SampleResolution Group
  PvlGroup &sampleResolutionGroup = camstatsPvl.findGroup("SampleResolution", Pvl::Traverse);
  EXPECT_NEAR((double)sampleResolutionGroup.findKeyword("SampleResolutionMinimum"), 0.046871054125116, 1e-8);
  EXPECT_NEAR((double)sampleResolutionGroup.findKeyword("SampleResolutionMaximum"), 0.047474615131164, 1e-8);
  EXPECT_NEAR((double)sampleResolutionGroup.findKeyword("SampleResolutionAverage"), 0.047126052255919, 1e-8);
  EXPECT_NEAR((double)sampleResolutionGroup.findKeyword("SampleResolutionStandardDeviation"), 1.42417470676804e-04, 1e-8);

  // LineResolution Group
  PvlGroup &lineResolutionGroup = camstatsPvl.findGroup("LineResolution", Pvl::Traverse);
  EXPECT_NEAR((double)lineResolutionGroup.findKeyword("LineResolutionMinimum"), 0.046871054125116, 1e-8);
  EXPECT_NEAR((double)lineResolutionGroup.findKeyword("LineResolutionMaximum"), 0.047474615131164, 1e-8);
  EXPECT_NEAR((double)lineResolutionGroup.findKeyword("LineResolutionAverage"), 0.047126052255919, 1e-8);
  EXPECT_NEAR((double)lineResolutionGroup.findKeyword("LineResolutionStandardDeviation"), 1.42417470676804e-04, 1e-8);

  // Resolution Group
  PvlGroup &resolutionGroup = camstatsPvl.findGroup("Resolution", Pvl::Traverse);
  EXPECT_NEAR((double)resolutionGroup.findKeyword("ResolutionMinimum"), 0.046871054125116, 1e-8);
  EXPECT_NEAR((double)resolutionGroup.findKeyword("ResolutionMaximum"), 0.047474615131164, 1e-8);
  EXPECT_NEAR((double)resolutionGroup.findKeyword("ResolutionAverage"), 0.047126052255919, 1e-8);
  EXPECT_NEAR((double)resolutionGroup.findKeyword("ResolutionStandardDeviation"), 1.42417470676804e-04, 1e-8);

  // ObliqueSampleResolution Group
  PvlGroup &obliqeSampleResolutionGroup = camstatsPvl.findGroup("ObliqueSampleResolution", Pvl::Traverse);
  EXPECT_NEAR((double)obliqeSampleResolutionGroup.findKeyword("ObliqueSampleResolutionMinimum"), 0.052968729623914, 1e-8);
  EXPECT_NEAR((double)obliqeSampleResolutionGroup.findKeyword("ObliqueSampleResolutionMaximum"), 0.070296807694853, 1e-8);
  EXPECT_NEAR((double)obliqeSampleResolutionGroup.findKeyword("ObliqueSampleResolutionAverage"), 0.059426418458696, 1e-8);
  EXPECT_NEAR((double)obliqeSampleResolutionGroup.findKeyword("ObliqueSampleResolutionStandardDeviation"), 0.0039440642620506, 1e-8);

  // ObliqueLineResolution Group
  PvlGroup &obliqueLineResolutionGroup = camstatsPvl.findGroup("ObliqueLineResolution", Pvl::Traverse);
  EXPECT_NEAR((double)obliqueLineResolutionGroup.findKeyword("ObliqueLineResolutionMinimum"), 0.052968729623914, 1e-8);
  EXPECT_NEAR((double)obliqueLineResolutionGroup.findKeyword("ObliqueLineResolutionMaximum"), 0.070296807694853, 1e-8);
  EXPECT_NEAR((double)obliqueLineResolutionGroup.findKeyword("ObliqueLineResolutionAverage"), 0.059426418458696, 1e-8);
  EXPECT_NEAR((double)obliqueLineResolutionGroup.findKeyword("ObliqueLineResolutionStandardDeviation"), 0.0039440642620506, 1e-8);

  // ObliqueResolution Group
  PvlGroup &obliqueResolutionGroup = camstatsPvl.findGroup("ObliqueResolution", Pvl::Traverse);
  EXPECT_NEAR((double)obliqueResolutionGroup.findKeyword("ObliqueResolutionMinimum"), 0.052968729623914, 1e-8);
  EXPECT_NEAR((double)obliqueResolutionGroup.findKeyword("ObliqueResolutionMaximum"), 0.070296807694853, 1e-8);
  EXPECT_NEAR((double)obliqueResolutionGroup.findKeyword("ObliqueResolutionAverage"), 0.059426418458696, 1e-8);
  EXPECT_NEAR((double)obliqueResolutionGroup.findKeyword("ObliqueResolutionStandardDeviation"), 0.0039440642620506, 1e-8);

  // AspectRatio Group
  PvlGroup &aspectRatioGroup = camstatsPvl.findGroup("AspectRatio", Pvl::Traverse);
  EXPECT_DOUBLE_EQ(double(aspectRatioGroup["AspectRatioMinimum"]), 1.0);
  EXPECT_DOUBLE_EQ(double(aspectRatioGroup["AspectRatioMaximun"]), 1.0);
  EXPECT_DOUBLE_EQ(double(aspectRatioGroup["AspectRatioAverage"]), 1.0);
  EXPECT_DOUBLE_EQ(double(aspectRatioGroup["AspectRatioStandardDeviation"]), 0.0);

  // PhaseAngle Group
  PvlGroup &phaseAngleGroup = camstatsPvl.findGroup("PhaseAngle", Pvl::Traverse);
  EXPECT_NEAR((double)phaseAngleGroup.findKeyword("PhaseMinimum"), 48.876692859867, 1e-8);
  EXPECT_NEAR((double)phaseAngleGroup.findKeyword("PhaseMaximum"), 49.920477301323, 1e-8);
  EXPECT_NEAR((double)phaseAngleGroup.findKeyword("PhaseAverage"), 49.398689481725, 1e-8);
  EXPECT_NEAR((double)phaseAngleGroup.findKeyword("PhaseStandardDeviation"), 0.22897617571212, 1e-8);

  // EmissionAngle Group
  PvlGroup &emissionAngleGroup = camstatsPvl.findGroup("EmissionAngle", Pvl::Traverse);
  EXPECT_NEAR((double)emissionAngleGroup.findKeyword("EmissionMinimum"), 27.763045742095, 1e-8);
  EXPECT_NEAR((double)emissionAngleGroup.findKeyword("EmissionMaximum"), 47.519033449668, 1e-8);
  EXPECT_NEAR((double)emissionAngleGroup.findKeyword("EmissionAverage"), 36.969119352261, 1e-8);
  EXPECT_NEAR((double)emissionAngleGroup.findKeyword("EmissionStandardDeviation"), 4.6918046818664, 1e-8);

  // IncidenceAngle Group
  PvlGroup &incidenceAngleGroup = camstatsPvl.findGroup("IncidenceAngle", Pvl::Traverse);
  EXPECT_NEAR((double)incidenceAngleGroup.findKeyword("IncidenceMinimum"), 21.466631120805, 1e-8);
  EXPECT_NEAR((double)incidenceAngleGroup.findKeyword("IncidenceMaximum"), 35.454339217982, 1e-8);
  EXPECT_NEAR((double)incidenceAngleGroup.findKeyword("IncidenceAverage"), 28.488976658239, 1e-8);
  EXPECT_NEAR((double)incidenceAngleGroup.findKeyword("IncidenceStandardDeviation"), 2.9992183214965, 1e-8);

  // LocalSolarTime Group
  PvlGroup &localSolarTimeGroup = camstatsPvl.findGroup("LocalSolarTime", Pvl::Traverse);
  EXPECT_NEAR((double)localSolarTimeGroup.findKeyword("LocalSolarTimeMinimum"), 9.5864973074802, 1e-8);
  EXPECT_NEAR((double)localSolarTimeGroup.findKeyword("LocalSolarTimeMaximum"), 10.40730803709, 1e-8);
  EXPECT_NEAR((double)localSolarTimeGroup.findKeyword("LocalSolarTimeAverage"), 9.9955073672148, 1e-8);
  EXPECT_NEAR((double)localSolarTimeGroup.findKeyword("LocalSolarTimeStandardDeviation"), 0.191007856794, 1e-8);

  // LocalRadius Group
  PvlGroup &localRadiusGroup = camstatsPvl.findGroup("LocalRadius", Pvl::Traverse);
  EXPECT_NEAR((double)localRadiusGroup.findKeyword("LocalRadiusMinimum"), 277.71213791062, 1e-8);
  EXPECT_NEAR((double)localRadiusGroup.findKeyword("LocalRadiusMaximum"), 280.18647579334, 1e-8);
  EXPECT_NEAR((double)localRadiusGroup.findKeyword("LocalRadiusAverage"), 279.07933370087, 1e-8);
  EXPECT_NEAR((double)localRadiusGroup.findKeyword("LocalRadiusStandardDeviation"), 0.58398919536395, 1e-8);

  // NorthAzimuth Group
  PvlGroup &northAzimuthGroup = camstatsPvl.findGroup("NorthAzimuth", Pvl::Traverse);
  EXPECT_NEAR((double)northAzimuthGroup.findKeyword("NorthAzimuthMinimum"), 105.24656669879, 1e-8);
  EXPECT_NEAR((double)northAzimuthGroup.findKeyword("NorthAzimuthMaximum"), 106.8169239958, 1e-8);
  EXPECT_NEAR((double)northAzimuthGroup.findKeyword("NorthAzimuthAverage"), 106.1554396739, 1e-8);
  EXPECT_NEAR((double)northAzimuthGroup.findKeyword("NorthAzimuthStandardDeviation"), 0.22228377516807, 1e-8);

  // get DN statistics
  QVector<QString> statsArgs = {"from=" + polyCamFileName,
                                "to=" + tempDir.path() + "/20190328T200344S309_pol_iofL2pan_dnstats.pvl"};
  UserInterface statsUI(STATS_XML, statsArgs);
  try {
    stats(statsUI);
  }
  catch (IException &e) {
    FAIL() << "Unable to run stats on polycam cube 20190328T200344S309_pol_iofL2pan.cub: "
           << e.what() << std::endl;
  }

  // validate dnstats file
  Pvl dnstatsPvl;
  dnstatsPvl.read(tempDir.path() + "/20190328T200344S309_pol_iofL2pan_dnstats.pvl");

  // Results Group
  PvlGroup &resultsGroup = dnstatsPvl.findGroup("Results", Pvl::Traverse);
  EXPECT_EQ(int(resultsGroup["Band"]), 1);
  EXPECT_NEAR((double)resultsGroup.findKeyword("Average"), 0.013450318437004, 1e-8);
  EXPECT_NEAR((double)resultsGroup.findKeyword("StandardDeviation"), 0.0070799520212006996, 1e-8);
  EXPECT_NEAR((double)resultsGroup.findKeyword("Variance"), 5.0125720622503901e-05, 1e-4);
  EXPECT_DOUBLE_EQ(double(resultsGroup["Median"]), 0.013795619090943);
  EXPECT_DOUBLE_EQ(double(resultsGroup["Mode"]), 0.013747211930541001);
  EXPECT_NEAR((double)resultsGroup.findKeyword("Skew"), -0.14631482794157, 1e-8);
  EXPECT_DOUBLE_EQ(double(resultsGroup["Minimum"]), -4.8828784201759798e-05);
  EXPECT_DOUBLE_EQ(double(resultsGroup["Maximum"]), 0.055606666952372);
  EXPECT_DOUBLE_EQ(double(resultsGroup["Sum"]), 14103.681105400001);
  EXPECT_EQ(int(resultsGroup["TotalPixels"]), 1048576);
  EXPECT_EQ(int(resultsGroup["ValidPixels"]), 1048576);
  EXPECT_EQ(int(resultsGroup["OverValidMaximumPixels"]), 0);
  EXPECT_EQ(int(resultsGroup["UnderValidMinimumPixels"]), 0);
  EXPECT_EQ(int(resultsGroup["NullPixels"]), 0);
  EXPECT_EQ(int(resultsGroup["LisPixels"]), 0);
  EXPECT_EQ(int(resultsGroup["LrsPixels"]), 0);
  EXPECT_EQ(int(resultsGroup["HisPixels"]), 0);
  EXPECT_EQ(int(resultsGroup["HrsPixels"]), 0);
}  


/**
   * PolyCamModuleCamptTest
   * 
   * 1) Polycam image 20190328T200344S309_pol_iofL2pan.fits is ingested with ocams2isis
   *    and spiceinited.
   * 2) campt is used to compute geometric and photometric info at the cube corner and
   *    center pixels.
   * 3) The test validates the campt output.
   * 
   * INPUT: isis/tests/data/osirisRexImages/ocams/20190328T200344S309_pol_iofL2pan.fits
   * 
   * OUTPUT: 1) ocams2isis
   *            - 20190328T200344S309_pol_iofL2pan.cub
   *         2) campt
   *            - 20190328T200344S309_pol_iofL2pan.cub_campt.pvl
   */
  TEST(OsirisRexPolyCamModules, PolyCamModuleCamptTest) {
  QTemporaryDir tempDir;

  // ingest polycam fits format image with ocams2isis
  QString polyCamFileName = tempDir.path() + "/20190328T200344S309_pol_iofL2pan.cub";
  QVector<QString> ocams2isisArgs = {"from=data/osirisRexImages/ocams/20190328T200344S309_pol_iofL2pan.fits",
                                     "to=" + polyCamFileName};

  UserInterface ocams2isisUI(OCAMS2ISIS_XML, ocams2isisArgs);
  try {
    ocams2isis(ocams2isisUI);
  }
  catch (IException &e) {
    FAIL() << "Unable to run ocams2isis on polycam fits file 20190328T200344S309_pol_iofL2pan.fits: "
           << e.what() << std::endl;
  }

  // spiceinit
  QVector<QString> spiceinitArgs = {"from=" + polyCamFileName};
  UserInterface spiceinitUI(SPICEINIT_XML, spiceinitArgs);
  try {
    spiceinit(spiceinitUI);
  }
  catch (IException &e) {
    FAIL() << "Unable to run spiceinit on polycam cube 20190328T200344S309_pol_iofL2pan.cub: "
           << e.what() << std::endl;
  }

  // create flatfile with corner and center coordinates
  ofstream of1;
  of1.open(tempDir.path().toStdString() + "/campt_input_coords.lis");
  of1 << "1,1\n1024,1\n1024,1024\n1,1024\n512,512";
  of1.close();

  Pvl appLog;

  // campt all image coordinates in campt_input_coords.lis file
  QVector<QString> camptArgs = {"from=" + polyCamFileName,
                                "to=" + tempDir.path() + "/20190328T200344S309_pol_iofL2pan.cub_campt.pvl",
                                "USECOORDLIST=TRUE",
                                "COORDLIST=" + tempDir.path() + "/campt_input_coords.lis",
                                "COORDTYPE=IMAGE",
                                "ALLOWOUTSIDE=no"};
  UserInterface camptULUI(CAMPT_XML, camptArgs);
  try {
    campt(camptULUI, &appLog);
  }
  catch (IException &e) {
    FAIL() << "Unable to run campt on polycam cube 20190328T200344S309_pol_iofL2pan.cub: "
           << e.what() << std::endl;
  }

  // read campt output pvl file
  Pvl camptPvl;
  camptPvl.read(tempDir.path() + "/20190328T200344S309_pol_iofL2pan.cub_campt.pvl");
 
  // validate upper left corner
  // GroundPoint Group
  PvlGroup &gpULGroup = camptPvl.findGroup("GroundPoint", Pvl::Traverse);
  EXPECT_DOUBLE_EQ(double(gpULGroup["Sample"]), 1.0);
  EXPECT_DOUBLE_EQ(double(gpULGroup["Line"]), 1.0);
  EXPECT_NEAR(double(gpULGroup["PixelValue"]), 0.018175066, 1e-8);
  EXPECT_NEAR((double)gpULGroup.findKeyword("RightAscension"), 159.09179809191, 1e-8);
  EXPECT_NEAR((double)gpULGroup.findKeyword("Declination"), -36.605401539008, 1e-8);
  EXPECT_NEAR((double)gpULGroup.findKeyword("PlanetocentricLatitude"), -6.0772752213548, 1e-8);
  EXPECT_NEAR((double)gpULGroup.findKeyword("PlanetographicLatitude"), -7.7896893724835, 1e-8);  
  EXPECT_NEAR((double)gpULGroup.findKeyword("PositiveEast360Longitude"), 27.484944488053, 1e-8);
  EXPECT_NEAR((double)gpULGroup.findKeyword("PositiveEast180Longitude"), 27.484944488053, 1e-8);
  EXPECT_NEAR((double)gpULGroup.findKeyword("PositiveWest360Longitude"), 332.51505551195, 1e-8);
  EXPECT_NEAR((double)gpULGroup.findKeyword("PositiveWest180Longitude"), -27.484944488053, 1e-8);
  EXPECT_NEAR(toDouble(gpULGroup.findKeyword("BodyFixedCoordinate")[0]), 0.24700263929967, 1e-8);
  EXPECT_NEAR(toDouble(gpULGroup.findKeyword("BodyFixedCoordinate")[1]), 0.12849895378606, 1e-8);
  EXPECT_NEAR(toDouble(gpULGroup.findKeyword("BodyFixedCoordinate")[2]),  -0.029643707760652, 1e-8);
  EXPECT_NEAR((double)gpULGroup.findKeyword("LocalRadius"), 280.00184705625, 1e-8);
  EXPECT_NEAR((double)gpULGroup.findKeyword("SampleResolution"), 0.04697427687785, 1e-8);
  EXPECT_NEAR((double)gpULGroup.findKeyword("LineResolution"), 0.04697427687785, 1e-8);
  EXPECT_NEAR((double)gpULGroup.findKeyword("ObliqueDetectorResolution"), 0.05528552259422, 1e-8);
  EXPECT_NEAR((double)gpULGroup.findKeyword("ObliquePixelResolution"), 0.05528552259422, 1e-8);
  EXPECT_NEAR((double)gpULGroup.findKeyword("ObliqueLineResolution"), 0.05528552259422, 1e-8);
  EXPECT_NEAR((double)gpULGroup.findKeyword("ObliqueSampleResolution"), 0.05528552259422, 1e-8);
  EXPECT_NEAR(toDouble(gpULGroup.findKeyword("SpacecraftPosition")[0]), 2.4508427760226, 1e-8);
  EXPECT_NEAR(toDouble(gpULGroup.findKeyword("SpacecraftPosition")[1]), 1.6808462152269, 1e-8);
  EXPECT_NEAR(toDouble(gpULGroup.findKeyword("SpacecraftPosition")[2]),  -2.2173509771884, 1e-8);
  EXPECT_NEAR((double)gpULGroup.findKeyword("SpacecraftAzimuth"), 299.5244684475, 1e-8);
  EXPECT_NEAR((double)gpULGroup.findKeyword("SlantDistance"), 3.4717080040005, 1e-8);
  EXPECT_NEAR((double)gpULGroup.findKeyword("TargetCenterDistance"), 3.7078996302575, 1e-8);
  EXPECT_NEAR((double)gpULGroup.findKeyword("SubSpacecraftLatitude"), -36.727312596629, 1e-8);
  EXPECT_NEAR((double)gpULGroup.findKeyword("SubSpacecraftLongitude"), 34.443255565086, 1e-8);
  EXPECT_NEAR((double)gpULGroup.findKeyword("SpacecraftAltitude"), 3.4404557273817, 1e-8);
  EXPECT_NEAR((double)gpULGroup.findKeyword("OffNadirAngle"), 0.52103410880642, 1e-8);
  EXPECT_NEAR((double)gpULGroup.findKeyword("SubSpacecraftGroundAzimuth"), 169.22915287862, 1e-8);
  EXPECT_NEAR(toDouble(gpULGroup.findKeyword("SunPosition")[0]), 70633378.107976, 1e-8);
  EXPECT_NEAR(toDouble(gpULGroup.findKeyword("SunPosition")[1]), 142837223.6272, 1e-8);
  EXPECT_NEAR(toDouble(gpULGroup.findKeyword("SunPosition")[2]),  6078864.0202888, 1e-8);
  EXPECT_NEAR((double)gpULGroup.findKeyword("SubSolarAzimuth"), 30.453539473229, 1e-8);
  EXPECT_NEAR((double)gpULGroup.findKeyword("SolarDistance"), 1.0659453790817, 1e-8);
  EXPECT_NEAR((double)gpULGroup.findKeyword("SubSolarLatitude"), 2.1846905951309, 1e-8);
  EXPECT_NEAR((double)gpULGroup.findKeyword("SubSolarLongitude"), 63.68748487585, 1e-8);
  EXPECT_NEAR((double)gpULGroup.findKeyword("SubSolarGroundAzimuth"), 78.202549107569, 1e-8);
  EXPECT_NEAR((double)gpULGroup.findKeyword("Phase"), 48.876692859867, 1e-8);
  EXPECT_NEAR((double)gpULGroup.findKeyword("Incidence"), 35.454339217982, 1e-8);
  EXPECT_NEAR((double)gpULGroup.findKeyword("Emission"), 31.824545321485, 1e-8);
  EXPECT_NEAR((double)gpULGroup.findKeyword("NorthAzimuth"), 106.61854749499, 1e-8);
  EXPECT_NEAR((double)gpULGroup.findKeyword("EphemerisTime"), 607075493.45972, 1e-8);
  EXPECT_EQ(gpULGroup["UTC"][0].toStdString(), "2019-03-28T20:03:44.2740743");
  EXPECT_NEAR((double)gpULGroup.findKeyword("LocalSolarTime"), 9.5864973074802, 1e-8);
  EXPECT_NEAR((double)gpULGroup.findKeyword("SolarLongitude"), 62.759024139382, 1e-8);

  // Look Direction Unit Vectors in Body Fixed, J2000, and Camera Coordinate Systems.
  EXPECT_NEAR(toDouble(gpULGroup.findKeyword("LookDirectionBodyFixed")[0]), -0.63479996998118, 1e-8);
  EXPECT_NEAR(toDouble(gpULGroup.findKeyword("LookDirectionBodyFixed")[1]), -0.44714223075561, 1e-8);
  EXPECT_NEAR(toDouble(gpULGroup.findKeyword("LookDirectionBodyFixed")[2]),  0.63015301601023, 1e-8);

  EXPECT_NEAR(toDouble(gpULGroup.findKeyword("LookDirectionJ2000")[0]), -0.74990216104615, 1e-8);
  EXPECT_NEAR(toDouble(gpULGroup.findKeyword("LookDirectionJ2000")[1]), 0.28648279845554, 1e-8);
  EXPECT_NEAR(toDouble(gpULGroup.findKeyword("LookDirectionJ2000")[2]),  -0.59630055764472, 1e-8);

  EXPECT_NEAR(toDouble(gpULGroup.findKeyword("LookDirectionCamera")[0]), -0.0069169353640599, 1e-8);
  EXPECT_NEAR(toDouble(gpULGroup.findKeyword("LookDirectionCamera")[1]), -0.0069167399410006, 1e-8);
  EXPECT_NEAR(toDouble(gpULGroup.findKeyword("LookDirectionCamera")[2]),  0.99995215621236, 1e-8);

  // remove upper left corner pvl group
  camptPvl.deleteGroup("GroundPoint");

  // validate upper right pixel
  // GroundPoint Group
  PvlGroup &gpURGroup = camptPvl.findGroup("GroundPoint", Pvl::Traverse);
  EXPECT_DOUBLE_EQ(double(gpURGroup["Sample"]), 1024.0);
  EXPECT_DOUBLE_EQ(double(gpURGroup["Line"]), 1.0);
  EXPECT_NEAR(double(gpURGroup["PixelValue"]), 0.010986163, 1e-8);
  EXPECT_NEAR((double)gpURGroup.findKeyword("RightAscension"), 160.01020817303, 1e-8);
  EXPECT_NEAR((double)gpURGroup.findKeyword("Declination"), -36.897805846293, 1e-8);
  EXPECT_NEAR((double)gpURGroup.findKeyword("PlanetocentricLatitude"), -9.2686382652326, 1e-8);
  EXPECT_NEAR((double)gpURGroup.findKeyword("PlanetographicLatitude"), -11.842579108518, 1e-8);  
  EXPECT_NEAR((double)gpURGroup.findKeyword("PositiveEast360Longitude"), 37.020066086413, 1e-8);
  EXPECT_NEAR((double)gpURGroup.findKeyword("PositiveEast180Longitude"), 37.020066086413, 1e-8);
  EXPECT_NEAR((double)gpURGroup.findKeyword("PositiveWest360Longitude"), 322.97993391359, 1e-8);
  EXPECT_NEAR((double)gpURGroup.findKeyword("PositiveWest180Longitude"), -37.020066086413, 1e-8);
  EXPECT_NEAR(toDouble(gpURGroup.findKeyword("BodyFixedCoordinate")[0]), 0.21883729823996, 1e-8);
  EXPECT_NEAR(toDouble(gpURGroup.findKeyword("BodyFixedCoordinate")[1]), 0.16502592529961, 1e-8);
  EXPECT_NEAR(toDouble(gpURGroup.findKeyword("BodyFixedCoordinate")[2]),  -0.044729323948947, 1e-8);
  EXPECT_NEAR((double)gpURGroup.findKeyword("LocalRadius"), 277.71213791062, 1e-8);
  EXPECT_NEAR((double)gpURGroup.findKeyword("SampleResolution"), 0.046871054125116, 1e-8);
  EXPECT_NEAR((double)gpURGroup.findKeyword("LineResolution"), 0.046871054125116, 1e-8);
  EXPECT_NEAR((double)gpURGroup.findKeyword("ObliqueDetectorResolution"), 0.052968729623914, 1e-8);
  EXPECT_NEAR((double)gpURGroup.findKeyword("ObliquePixelResolution"), 0.052968729623914, 1e-8);
  EXPECT_NEAR((double)gpURGroup.findKeyword("ObliqueLineResolution"), 0.052968729623914, 1e-8);
  EXPECT_NEAR((double)gpURGroup.findKeyword("ObliqueSampleResolution"), 0.052968729623914, 1e-8);
  EXPECT_NEAR(toDouble(gpURGroup.findKeyword("SpacecraftPosition")[0]), 2.4508427760226, 1e-8);
  EXPECT_NEAR(toDouble(gpURGroup.findKeyword("SpacecraftPosition")[1]), 1.6808462152269, 1e-8);
  EXPECT_NEAR(toDouble(gpURGroup.findKeyword("SpacecraftPosition")[2]), -2.2173509771884, 1e-8);
  EXPECT_NEAR((double)gpURGroup.findKeyword("SpacecraftAzimuth"), 280.74511665267, 1e-8);
  EXPECT_NEAR((double)gpURGroup.findKeyword("SlantDistance"), 3.464079163693, 1e-8);
  EXPECT_NEAR((double)gpURGroup.findKeyword("TargetCenterDistance"), 3.7078996302575, 1e-8);
  EXPECT_NEAR((double)gpURGroup.findKeyword("SubSpacecraftLatitude"), -36.727312596629, 1e-8);
  EXPECT_NEAR((double)gpURGroup.findKeyword("SubSpacecraftLongitude"), 34.443255565086, 1e-8);
  EXPECT_NEAR((double)gpURGroup.findKeyword("SpacecraftAltitude"), 3.4404557273817, 1e-8);
  EXPECT_NEAR((double)gpURGroup.findKeyword("OffNadirAngle"), 0.20515032549028, 1e-8);
  EXPECT_NEAR((double)gpURGroup.findKeyword("SubSpacecraftGroundAzimuth"), 184.46711420289, 1e-8);
  EXPECT_NEAR(toDouble(gpURGroup.findKeyword("SunPosition")[0]), 70633378.107976, 1e-8);
  EXPECT_NEAR(toDouble(gpURGroup.findKeyword("SunPosition")[1]), 142837223.6272, 1e-8);
  EXPECT_NEAR(toDouble(gpURGroup.findKeyword("SunPosition")[2]), 6078864.0202888, 1e-8);
  EXPECT_NEAR((double)gpURGroup.findKeyword("SubSolarAzimuth"), 34.053699817326, 1e-8);
  EXPECT_NEAR((double)gpURGroup.findKeyword("SolarDistance"), 1.0659453789502, 1e-8);
  EXPECT_NEAR((double)gpURGroup.findKeyword("SubSolarLatitude"), 2.1846905951309, 1e-8);
  EXPECT_NEAR((double)gpURGroup.findKeyword("SubSolarLongitude"), 63.68748487585, 1e-8);
  EXPECT_NEAR((double)gpURGroup.findKeyword("SubSolarGroundAzimuth"), 67.972584718604, 1e-8);
  EXPECT_NEAR((double)gpURGroup.findKeyword("Phase"), 49.197587828554, 1e-8);
  EXPECT_NEAR((double)gpURGroup.findKeyword("Incidence"), 27.740961628999, 1e-8);
  EXPECT_NEAR((double)gpURGroup.findKeyword("Emission"), 27.763045742095, 1e-8);
  EXPECT_NEAR((double)gpURGroup.findKeyword("NorthAzimuth"), 105.87742660106, 1e-8);
  EXPECT_NEAR((double)gpURGroup.findKeyword("EphemerisTime"), 607075493.45972, 1e-8);
  EXPECT_EQ(gpURGroup["UTC"][0].toStdString(), "2019-03-28T20:03:44.2740743");
  EXPECT_NEAR((double)gpURGroup.findKeyword("LocalSolarTime"), 10.222172080704, 1e-8);
  EXPECT_NEAR((double)gpURGroup.findKeyword("SolarLongitude"), 62.759024139382, 1e-8);

  // Look Direction Unit Vectors in Body Fixed, J2000, and Camera Coordinate Systems.
  EXPECT_NEAR(toDouble(gpURGroup.findKeyword("LookDirectionBodyFixed")[0]), -0.6443286577213, 1e-8);
  EXPECT_NEAR(toDouble(gpURGroup.findKeyword("LookDirectionBodyFixed")[1]), -0.43758246226432, 1e-8);
  EXPECT_NEAR(toDouble(gpURGroup.findKeyword("LookDirectionBodyFixed")[2]),  0.62718591307344, 1e-8);

  EXPECT_NEAR(toDouble(gpURGroup.findKeyword("LookDirectionJ2000")[0]), -0.75152809795036, 1e-8);
  EXPECT_NEAR(toDouble(gpURGroup.findKeyword("LookDirectionJ2000")[1]), 0.27338223286548, 1e-8);
  EXPECT_NEAR(toDouble(gpURGroup.findKeyword("LookDirectionJ2000")[2]),  -0.60038960079651, 1e-8);

  EXPECT_NEAR(toDouble(gpURGroup.findKeyword("LookDirectionCamera")[0]), -0.0069168979740324, 1e-8);
  EXPECT_NEAR(toDouble(gpURGroup.findKeyword("LookDirectionCamera")[1]), 0.0069031284911851, 1e-8);
  EXPECT_NEAR(toDouble(gpURGroup.findKeyword("LookDirectionCamera")[2]),  0.99995225052972, 1e-8);

  // remove upper right corner pvl group
  camptPvl.deleteGroup("GroundPoint");

  // validate lower right pixel
  // GroundPoint Group
  PvlGroup &gpLRGroup = camptPvl.findGroup("GroundPoint", Pvl::Traverse);
  EXPECT_DOUBLE_EQ(double(gpLRGroup["Sample"]), 1024.0);
  EXPECT_DOUBLE_EQ(double(gpLRGroup["Line"]), 1024.0);
  EXPECT_NEAR(double(gpLRGroup["PixelValue"]), 0.00048807812999999999, 1e-8);
  EXPECT_NEAR((double)gpLRGroup.findKeyword("RightAscension"), 159.64548559593, 1e-8);
  EXPECT_NEAR((double)gpLRGroup.findKeyword("Declination"), -37.63448992705, 1e-8);
  EXPECT_NEAR((double)gpLRGroup.findKeyword("PlanetocentricLatitude"), 2.6310940923815, 1e-8);
  EXPECT_NEAR((double)gpLRGroup.findKeyword("PlanetographicLatitude"), 3.3791195781359, 1e-8);  
  EXPECT_NEAR((double)gpLRGroup.findKeyword("PositiveEast360Longitude"), 39.797105432196, 1e-8);
  EXPECT_NEAR((double)gpLRGroup.findKeyword("PositiveEast180Longitude"), 39.797105432196, 1e-8);
  EXPECT_NEAR((double)gpLRGroup.findKeyword("PositiveWest360Longitude"), 320.2028945678, 1e-8);
  EXPECT_NEAR((double)gpLRGroup.findKeyword("PositiveWest180Longitude"), -39.797105432196, 1e-8);
  EXPECT_NEAR(toDouble(gpLRGroup.findKeyword("BodyFixedCoordinate")[0]), 0.21333215814442, 1e-8);
  EXPECT_NEAR(toDouble(gpLRGroup.findKeyword("BodyFixedCoordinate")[1]), 0.17772339511772, 1e-8);
  EXPECT_NEAR(toDouble(gpLRGroup.findKeyword("BodyFixedCoordinate")[2]), 0.01275956004721, 1e-8);
  EXPECT_NEAR((double)gpLRGroup.findKeyword("LocalRadius"), 277.95507054796, 1e-8);
  EXPECT_NEAR((double)gpLRGroup.findKeyword("SampleResolution"), 0.047336252371142, 1e-8);
  EXPECT_NEAR((double)gpLRGroup.findKeyword("LineResolution"), 0.047336252371142, 1e-8);
  EXPECT_NEAR((double)gpLRGroup.findKeyword("ObliqueDetectorResolution"), 0.065299690744535, 1e-8);
  EXPECT_NEAR((double)gpLRGroup.findKeyword("ObliquePixelResolution"), 0.065299690744535, 1e-8);
  EXPECT_NEAR((double)gpLRGroup.findKeyword("ObliqueLineResolution"), 0.065299690744535, 1e-8);
  EXPECT_NEAR((double)gpLRGroup.findKeyword("ObliqueSampleResolution"), 0.065299690744535, 1e-8);
  EXPECT_NEAR(toDouble(gpLRGroup.findKeyword("SpacecraftPosition")[0]), 2.4508427760226, 1e-8);
  EXPECT_NEAR(toDouble(gpLRGroup.findKeyword("SpacecraftPosition")[1]), 1.6808462152269, 1e-8);
  EXPECT_NEAR(toDouble(gpLRGroup.findKeyword("SpacecraftPosition")[2]), -2.2173509771884, 1e-8);
  EXPECT_NEAR((double)gpLRGroup.findKeyword("SpacecraftAzimuth"), 277.7051891273, 1e-8);
  EXPECT_NEAR((double)gpLRGroup.findKeyword("SlantDistance"), 3.498460373613, 1e-8);
  EXPECT_NEAR((double)gpLRGroup.findKeyword("TargetCenterDistance"), 3.7078996302575, 1e-8);
  EXPECT_NEAR((double)gpLRGroup.findKeyword("SubSpacecraftLatitude"), -36.727312596629, 1e-8);
  EXPECT_NEAR((double)gpLRGroup.findKeyword("SubSpacecraftLongitude"), 34.443255565086, 1e-8);
  EXPECT_NEAR((double)gpLRGroup.findKeyword("SpacecraftAltitude"), 3.4404557273817, 1e-8);
  EXPECT_NEAR((double)gpLRGroup.findKeyword("OffNadirAngle"), 3.8658970998287, 1e-8);
  EXPECT_NEAR((double)gpLRGroup.findKeyword("SubSpacecraftGroundAzimuth"), 186.72721055134, 1e-8);
  EXPECT_NEAR(toDouble(gpLRGroup.findKeyword("SunPosition")[0]), 70633378.107976, 1e-8);
  EXPECT_NEAR(toDouble(gpLRGroup.findKeyword("SunPosition")[1]), 142837223.6272, 1e-8);
  EXPECT_NEAR(toDouble(gpLRGroup.findKeyword("SunPosition")[2]), 6078864.0202888, 1e-8);
  EXPECT_NEAR((double)gpLRGroup.findKeyword("SubSolarAzimuth"), 12.417211310715, 1e-8);
  EXPECT_NEAR((double)gpLRGroup.findKeyword("SolarDistance"), 1.0659453788758, 1e-8);
  EXPECT_NEAR((double)gpLRGroup.findKeyword("SubSolarLatitude"), 2.1846905951309, 1e-8);
  EXPECT_NEAR((double)gpLRGroup.findKeyword("SubSolarLongitude"), 63.68748487585, 1e-8);
  EXPECT_NEAR((double)gpLRGroup.findKeyword("SubSolarGroundAzimuth"), 90.546606801213, 1e-8);
  EXPECT_NEAR((double)gpLRGroup.findKeyword("Phase"), 49.920477301323, 1e-8);
  EXPECT_NEAR((double)gpLRGroup.findKeyword("Incidence"), 21.466631120805, 1e-8);
  EXPECT_NEAR((double)gpLRGroup.findKeyword("Emission"), 43.538825129456, 1e-8);
  EXPECT_NEAR((double)gpLRGroup.findKeyword("NorthAzimuth"), 106.8169239958, 1e-8);
  EXPECT_NEAR((double)gpLRGroup.findKeyword("EphemerisTime"), 607075493.45972, 1e-8);
  EXPECT_EQ(gpLRGroup["UTC"][0].toStdString(), "2019-03-28T20:03:44.2740743");
  EXPECT_NEAR((double)gpLRGroup.findKeyword("LocalSolarTime"), 10.40730803709, 1e-8);
  EXPECT_NEAR((double)gpLRGroup.findKeyword("SolarLongitude"), 62.759024139382, 1e-8);

  // Look Direction Unit Vectors in Body Fixed, J2000, and Camera Coordinate Systems.
  EXPECT_NEAR(toDouble(gpLRGroup.findKeyword("LookDirectionBodyFixed")[0]), -0.63957009053312, 1e-8);
  EXPECT_NEAR(toDouble(gpLRGroup.findKeyword("LookDirectionBodyFixed")[1]), -0.42965266419665, 1e-8);
  EXPECT_NEAR(toDouble(gpLRGroup.findKeyword("LookDirectionBodyFixed")[2]), 0.63745485129864, 1e-8);

  EXPECT_NEAR(toDouble(gpLRGroup.findKeyword("LookDirectionJ2000")[0]), -0.74247333777921, 1e-8);
  EXPECT_NEAR(toDouble(gpLRGroup.findKeyword("LookDirectionJ2000")[1]), 0.27545260487424, 1e-8);
  EXPECT_NEAR(toDouble(gpLRGroup.findKeyword("LookDirectionJ2000")[2]), -0.61062198220749, 1e-8);

  EXPECT_NEAR(toDouble(gpLRGroup.findKeyword("LookDirectionCamera")[0]), 0.0069025358490113, 1e-8);
  EXPECT_NEAR(toDouble(gpLRGroup.findKeyword("LookDirectionCamera")[1]), 0.0069027391530857, 1e-8);
  EXPECT_NEAR(toDouble(gpLRGroup.findKeyword("LookDirectionCamera")[2]), 0.99995235246037, 1e-8);

  // remove lower right corner pvl group
  camptPvl.deleteGroup("GroundPoint");

  // validate lower left pixel
  // GroundPoint Group
  PvlGroup &gpLLGroup = camptPvl.findGroup("GroundPoint", Pvl::Traverse);
  EXPECT_DOUBLE_EQ(double(gpLLGroup["Sample"]), 1.0);
  EXPECT_DOUBLE_EQ(double(gpLLGroup["Line"]), 1024.0);
  EXPECT_NEAR(double(gpLLGroup["PixelValue"]), 0.029314211, 1e-8);
  EXPECT_NEAR((double)gpLLGroup.findKeyword("RightAscension"), 158.7195989993, 1e-8);
  EXPECT_NEAR((double)gpLLGroup.findKeyword("Declination"), -37.339252514878, 1e-8);
  EXPECT_NEAR((double)gpLLGroup.findKeyword("PlanetocentricLatitude"), 6.0970614203916, 1e-8);
  EXPECT_NEAR((double)gpLLGroup.findKeyword("PlanetographicLatitude"), 7.8149277102229, 1e-8);  
  EXPECT_NEAR((double)gpLLGroup.findKeyword("PositiveEast360Longitude"), 30.195774258998, 1e-8);
  EXPECT_NEAR((double)gpLLGroup.findKeyword("PositiveEast180Longitude"), 30.195774258998, 1e-8);
  EXPECT_NEAR((double)gpLLGroup.findKeyword("PositiveWest360Longitude"), 329.804225741, 1e-8);
  EXPECT_NEAR((double)gpLLGroup.findKeyword("PositiveWest180Longitude"), -30.195774258998, 1e-8);
  EXPECT_NEAR(toDouble(gpLLGroup.findKeyword("BodyFixedCoordinate")[0]), 0.24022335485545, 1e-8);
  EXPECT_NEAR(toDouble(gpLLGroup.findKeyword("BodyFixedCoordinate")[1]), 0.13978961347585, 1e-8);
  EXPECT_NEAR(toDouble(gpLLGroup.findKeyword("BodyFixedCoordinate")[2]), 0.029688367728872, 1e-8);
  EXPECT_NEAR((double)gpLLGroup.findKeyword("LocalRadius"), 279.51707538563, 1e-8);
  EXPECT_NEAR((double)gpLLGroup.findKeyword("SampleResolution"), 0.047474615131164, 1e-8);
  EXPECT_NEAR((double)gpLLGroup.findKeyword("LineResolution"), 0.047474615131164, 1e-8);
  EXPECT_NEAR((double)gpLLGroup.findKeyword("ObliqueDetectorResolution"), 0.070296807694853, 1e-8);
  EXPECT_NEAR((double)gpLLGroup.findKeyword("ObliquePixelResolution"), 0.070296807694853, 1e-8);
  EXPECT_NEAR((double)gpLLGroup.findKeyword("ObliqueLineResolution"), 0.070296807694853, 1e-8);
  EXPECT_NEAR((double)gpLLGroup.findKeyword("ObliqueSampleResolution"), 0.070296807694853, 1e-8);
  EXPECT_NEAR(toDouble(gpLLGroup.findKeyword("SpacecraftPosition")[0]), 2.4508427760226, 1e-8);
  EXPECT_NEAR(toDouble(gpLLGroup.findKeyword("SpacecraftPosition")[1]), 1.6808462152269, 1e-8);
  EXPECT_NEAR(toDouble(gpLLGroup.findKeyword("SpacecraftPosition")[2]), -2.2173509771884, 1e-8);
  EXPECT_NEAR((double)gpLLGroup.findKeyword("SpacecraftAzimuth"), 292.41771613968, 1e-8);
  EXPECT_NEAR((double)gpLLGroup.findKeyword("SlantDistance"), 3.5086862915696, 1e-8);
  EXPECT_NEAR((double)gpLLGroup.findKeyword("TargetCenterDistance"), 3.7078996302575, 1e-8);
  EXPECT_NEAR((double)gpLLGroup.findKeyword("SubSpacecraftLatitude"), -36.727312596629, 1e-8);
  EXPECT_NEAR((double)gpLLGroup.findKeyword("SubSpacecraftLongitude"), 34.443255565086, 1e-8);
  EXPECT_NEAR((double)gpLLGroup.findKeyword("SpacecraftAltitude"), 3.4404557273817, 1e-8);
  EXPECT_NEAR((double)gpLLGroup.findKeyword("OffNadirAngle"), 4.5104791444882, 1e-8);
  EXPECT_NEAR((double)gpLLGroup.findKeyword("SubSpacecraftGroundAzimuth"), 175.00737471983, 1e-8);
  EXPECT_NEAR(toDouble(gpLLGroup.findKeyword("SunPosition")[0]), 70633378.107976, 1e-8);
  EXPECT_NEAR(toDouble(gpLLGroup.findKeyword("SunPosition")[1]), 142837223.6272, 1e-8);
  EXPECT_NEAR(toDouble(gpLLGroup.findKeyword("SunPosition")[2]), 6078864.0202888, 1e-8);
  EXPECT_NEAR((double)gpLLGroup.findKeyword("SubSolarAzimuth"), 15.155246555599, 1e-8);
  EXPECT_NEAR((double)gpLLGroup.findKeyword("SolarDistance"), 1.065945379019, 1e-8);
  EXPECT_NEAR((double)gpLLGroup.findKeyword("SubSolarLatitude"), 2.1846905951309, 1e-8);
  EXPECT_NEAR((double)gpLLGroup.findKeyword("SubSolarLongitude"), 63.68748487585, 1e-8);
  EXPECT_NEAR((double)gpLLGroup.findKeyword("SubSolarGroundAzimuth"), 95.243875347485, 1e-8);
  EXPECT_NEAR((double)gpLLGroup.findKeyword("Phase"), 49.603064015649, 1e-8);
  EXPECT_NEAR((double)gpLLGroup.findKeyword("Incidence"), 31.661119314817, 1e-8);
  EXPECT_NEAR((double)gpLLGroup.findKeyword("Emission"), 47.519033449668, 1e-8);
  EXPECT_NEAR((double)gpLLGroup.findKeyword("NorthAzimuth"), 105.24656669879, 1e-8);
  EXPECT_NEAR((double)gpLLGroup.findKeyword("EphemerisTime"), 607075493.45972, 1e-8);
  EXPECT_EQ(gpLLGroup["UTC"][0].toStdString(), "2019-03-28T20:03:44.2740743");
  EXPECT_NEAR((double)gpLLGroup.findKeyword("LocalSolarTime"), 9.7672192922099, 1e-8);
  EXPECT_NEAR((double)gpLLGroup.findKeyword("SolarLongitude"), 62.759024139382, 1e-8);

  // Look Direction Unit Vectors in Body Fixed, J2000, and Camera Coordinate Systems.
  EXPECT_NEAR(toDouble(gpLLGroup.findKeyword("LookDirectionBodyFixed")[0]), -0.63004191240427, 1e-8);
  EXPECT_NEAR(toDouble(gpLLGroup.findKeyword("LookDirectionBodyFixed")[1]), -0.43921185129996, 1e-8);
  EXPECT_NEAR(toDouble(gpLLGroup.findKeyword("LookDirectionBodyFixed")[2]), 0.64042184401504, 1e-8);

  EXPECT_NEAR(toDouble(gpLLGroup.findKeyword("LookDirectionJ2000")[0]), -0.74084744234127, 1e-8);
  EXPECT_NEAR(toDouble(gpLLGroup.findKeyword("LookDirectionJ2000")[1]), 0.28855244512542, 1e-8);
  EXPECT_NEAR(toDouble(gpLLGroup.findKeyword("LookDirectionJ2000")[2]), -0.60653322546135, 1e-8);

  EXPECT_NEAR(toDouble(gpLLGroup.findKeyword("LookDirectionCamera")[0]), 0.0069025747085589, 1e-8);
  EXPECT_NEAR(toDouble(gpLLGroup.findKeyword("LookDirectionCamera")[1]), -0.006916352072465, 1e-8);
  EXPECT_NEAR(toDouble(gpLLGroup.findKeyword("LookDirectionCamera")[2]), 0.99995225812856, 1e-8);

  // remove lower left corner pvl group
  camptPvl.deleteGroup("GroundPoint");

  // validate center pixel
  // GroundPoint Group
  PvlGroup &gpCTRGroup = camptPvl.findGroup("GroundPoint", Pvl::Traverse);
  EXPECT_DOUBLE_EQ(double(gpCTRGroup["Sample"]), 512.0);
  EXPECT_DOUBLE_EQ(double(gpCTRGroup["Line"]), 512.0);
  EXPECT_NEAR(double(gpCTRGroup["PixelValue"]), 0.014208083999999999, 1e-8);
  EXPECT_NEAR((double)gpCTRGroup.findKeyword("RightAscension"), 159.36649322592, 1e-8);
  EXPECT_NEAR((double)gpCTRGroup.findKeyword("Declination"), -37.119791880649, 1e-8);
  EXPECT_NEAR((double)gpCTRGroup.findKeyword("PlanetocentricLatitude"), -2.1860734299091, 1e-8);
  EXPECT_NEAR((double)gpCTRGroup.findKeyword("PlanetographicLatitude"), -2.8079757801673, 1e-8);  
  EXPECT_NEAR((double)gpCTRGroup.findKeyword("PositiveEast360Longitude"), 33.611533123578, 1e-8);
  EXPECT_NEAR((double)gpCTRGroup.findKeyword("PositiveEast180Longitude"), 33.611533123578, 1e-8);
  EXPECT_NEAR((double)gpCTRGroup.findKeyword("PositiveWest360Longitude"), 326.38846687642, 1e-8);
  EXPECT_NEAR((double)gpCTRGroup.findKeyword("PositiveWest180Longitude"), -33.611533123578, 1e-8);
  EXPECT_NEAR(toDouble(gpCTRGroup.findKeyword("BodyFixedCoordinate")[0]), 0.232367234344, 1e-8);
  EXPECT_NEAR(toDouble(gpCTRGroup.findKeyword("BodyFixedCoordinate")[1]), 0.154451850805, 1e-8);
  EXPECT_NEAR(toDouble(gpCTRGroup.findKeyword("BodyFixedCoordinate")[2]), -0.010650793653882, 1e-8);
  EXPECT_NEAR((double)gpCTRGroup.findKeyword("LocalRadius"), 279.21917057972, 1e-8);
  EXPECT_NEAR((double)gpCTRGroup.findKeyword("SampleResolution"), 0.04710715110013, 1e-8);
  EXPECT_NEAR((double)gpCTRGroup.findKeyword("LineResolution"), 0.04710715110013, 1e-8);
  EXPECT_NEAR((double)gpCTRGroup.findKeyword("ObliqueDetectorResolution"), 0.058695272573753, 1e-8);
  EXPECT_NEAR((double)gpCTRGroup.findKeyword("ObliquePixelResolution"), 0.058695272573753, 1e-8);
  EXPECT_NEAR((double)gpCTRGroup.findKeyword("ObliqueLineResolution"), 0.058695272573753, 1e-8);
  EXPECT_NEAR((double)gpCTRGroup.findKeyword("ObliqueSampleResolution"), 0.058695272573753, 1e-8);
  EXPECT_NEAR(toDouble(gpCTRGroup.findKeyword("SpacecraftPosition")[0]), 2.4508427760226, 1e-8);
  EXPECT_NEAR(toDouble(gpCTRGroup.findKeyword("SpacecraftPosition")[1]), 1.6808462152269, 1e-8);
  EXPECT_NEAR(toDouble(gpCTRGroup.findKeyword("SpacecraftPosition")[2]), -2.2173509771884, 1e-8);
  EXPECT_NEAR((double)gpCTRGroup.findKeyword("SpacecraftAzimuth"), 287.6341124593, 1e-8);
  EXPECT_NEAR((double)gpCTRGroup.findKeyword("SlantDistance"), 3.4815282829207, 1e-8);
  EXPECT_NEAR((double)gpCTRGroup.findKeyword("TargetCenterDistance"), 3.7078996302575, 1e-8);
  EXPECT_NEAR((double)gpCTRGroup.findKeyword("SubSpacecraftLatitude"), -36.727312596629, 1e-8);
  EXPECT_NEAR((double)gpCTRGroup.findKeyword("SubSpacecraftLongitude"), 34.443255565086, 1e-8);
  EXPECT_NEAR((double)gpCTRGroup.findKeyword("SpacecraftAltitude"), 3.4404557273817, 1e-8);
  EXPECT_NEAR((double)gpCTRGroup.findKeyword("OffNadirAngle"), 2.0738688480295, 1e-8);
  EXPECT_NEAR((double)gpCTRGroup.findKeyword("SubSpacecraftGroundAzimuth"), 178.82451866506, 1e-8);
  EXPECT_NEAR(toDouble(gpCTRGroup.findKeyword("SunPosition")[0]), 70633378.107976, 1e-8);
  EXPECT_NEAR(toDouble(gpCTRGroup.findKeyword("SunPosition")[1]), 142837223.6272, 1e-8);
  EXPECT_NEAR(toDouble(gpCTRGroup.findKeyword("SunPosition")[2]), 6078864.0202888, 1e-8);
  EXPECT_NEAR((double)gpCTRGroup.findKeyword("SubSolarAzimuth"), 23.144034265352, 1e-8);
  EXPECT_NEAR((double)gpCTRGroup.findKeyword("SolarDistance"), 1.0659453789648, 1e-8);
  EXPECT_NEAR((double)gpCTRGroup.findKeyword("SubSolarLatitude"), 2.1846905951309, 1e-8);
  EXPECT_NEAR((double)gpCTRGroup.findKeyword("SubSolarLongitude"), 63.68748487585, 1e-8);
  EXPECT_NEAR((double)gpCTRGroup.findKeyword("SubSolarGroundAzimuth"), 81.921732948275, 1e-8);
  EXPECT_NEAR((double)gpCTRGroup.findKeyword("Phase"), 49.397794786011, 1e-8);
  EXPECT_NEAR((double)gpCTRGroup.findKeyword("Incidence"), 28.204225429332, 1e-8);
  EXPECT_NEAR((double)gpCTRGroup.findKeyword("Emission"), 36.623634095368, 1e-8);
  EXPECT_NEAR((double)gpCTRGroup.findKeyword("NorthAzimuth"), 106.15937800693, 1e-8);
  EXPECT_NEAR((double)gpCTRGroup.findKeyword("EphemerisTime"), 607075493.45972, 1e-8);
  EXPECT_EQ(gpCTRGroup["UTC"][0].toStdString(), "2019-03-28T20:03:44.2740743");
  EXPECT_NEAR((double)gpCTRGroup.findKeyword("LocalSolarTime"), 9.9949365498486, 1e-8);
  EXPECT_NEAR((double)gpCTRGroup.findKeyword("SolarLongitude"), 62.759024139382, 1e-8);

  // Look Direction Unit Vectors in Body Fixed, J2000, and Camera Coordinate Systems.
  EXPECT_NEAR(toDouble(gpCTRGroup.findKeyword("LookDirectionBodyFixed")[0]), -0.63721313210689, 1e-8);
  EXPECT_NEAR(toDouble(gpCTRGroup.findKeyword("LookDirectionBodyFixed")[1]), -0.43842653007013, 1e-8);
  EXPECT_NEAR(toDouble(gpCTRGroup.findKeyword("LookDirectionBodyFixed")[2]), 0.63383089385198, 1e-8);

  EXPECT_NEAR(toDouble(gpCTRGroup.findKeyword("LookDirectionJ2000")[0]), -0.74622675831374, 1e-8);
  EXPECT_NEAR(toDouble(gpCTRGroup.findKeyword("LookDirectionJ2000")[1]), 0.28098635895935, 1e-8);
  EXPECT_NEAR(toDouble(gpCTRGroup.findKeyword("LookDirectionJ2000")[2]), -0.60348346394523, 1e-8);

  EXPECT_NEAR(toDouble(gpCTRGroup.findKeyword("LookDirectionCamera")[0]), -1.35313080827926e-05, 1e-8);
  EXPECT_NEAR(toDouble(gpCTRGroup.findKeyword("LookDirectionCamera")[1]), -1.35306343099495e-05, 1e-8);
  EXPECT_NEAR(toDouble(gpCTRGroup.findKeyword("LookDirectionCamera")[2]), 0.99999999981691, 1e-8);
}


/**
   * PolyCamModuleTwoImageTest
   * 
   *  1) Two overlapping PolyCam images (fits format) are ingested with ocams2isis. Each is
   *     processed with spiceinit and footprintinit.
   *  2) mosrange computes the lat/lon range of the two images to prep for mosaicking.
   *  3) cam2map projects the two images using the map file output from mosrange.
   *  4) automos creates an uncontrolled mosaic of the two projected images.
   *  5) bandtrim is run on the uncontrolled mosaic to propagate NULL pixels in any band to
   *     all other bands.
   *  6) noseam is used to create an uncontrolled mosaic with minimal seams.
   *  7) findimageoverlaps is run on both images to prep for autoseed and pointreg.
   *  8) overlapstats is run on the output from findimageoverlaps.
   *  9) autoseed is run on both images.
   * 10) The images are registered using pointreg.
   * 11) The test validates output from mosrange, automos, bandtrim, noseam, overlapstats,
   *     autoseed, and pointreg.
   * 
   * INPUT: two polycam images in fits format
   *          - isis/tests/data/osirisRexImages/ocams/20190328T200344S309_pol_iofL2pan.fits
   *          - isis/tests/data/osirisRexImages/ocams/20190328T200717S769_pol_iofL2pan.fits
   * 
   * OUTPUT: 1) ocams2isis
   *            - 20190328T200344S309_pol_iofL2pan.cub
   *            - 20190328T200717S769_pol_iofL2pan.cub
   *         2) mosrange
   *            - polycam_equi.map
   *         3) cam2map
   *            - 20190328T200344S309_pol_iofL2pan_map.cub
   *            - 20190328T200717S769_pol_iofL2pan_map.cub
   *         4) automos
   *            - mosaicUncontrolled.cub
   *         5) bandtrim
   *            - mosaicTrimmedUncontrolled.cub
   *         6) noseam
   *            - noseamUncontrolledMosaic.cub
   *         7) findimageoverlaps
   *            - polycam_findimageoverlaps.txt
   *         8) autoseed
   *            - polycam_autoseed.net
   *         9) pointreg
   *            - polycam_pointreg.net
   */
  TEST(OsirisRexPolyCamModules, PolyCamModuleTwoImageTest) {
  QTemporaryDir tempDir;

  // ingest 1st polycam fits format image 
  QString polyCam1FileName = tempDir.path() + "/20190328T200344S309_pol_iofL2pan.cub";
  QVector<QString> ocams2isisArgs1 = {"from=data/osirisRexImages/ocams/20190328T200344S309_pol_iofL2pan.fits",
                                      "to=" + polyCam1FileName};

  UserInterface ocams2isisPolyCam1UI(OCAMS2ISIS_XML, ocams2isisArgs1);
  try {
    ocams2isis(ocams2isisPolyCam1UI);
  }
  catch (IException &e) {
    FAIL() << "Unable to run ocams2isis on first polycam image 20190328T200344S309_pol_iofL2pan.fits: "
           << e.what() << std::endl;
  }

  // spiceinit
  QVector<QString> spiceinitArgs1 = {"from=" + polyCam1FileName};
  UserInterface spiceinitPolyCam1UI(SPICEINIT_XML, spiceinitArgs1);
  try {
    spiceinit(spiceinitPolyCam1UI);
  }
  catch (IException &e) {
    FAIL() << "Unable to run spiceinit on first polycam cube 20190328T200344S309_pol_iofL2pan.cub: "
           << e.what() << std::endl;
  }

  // footprintinit
  QVector<QString> footprintinitArgs1 = {"from=" + polyCam1FileName};
  UserInterface footprintinitPolyCam1UI(FOOTPRINTINIT_XML, footprintinitArgs1);
  try {
    footprintinit(footprintinitPolyCam1UI);
  }
  catch (IException &e) {
    FAIL() << "Unable to run footprintinit on first polycam cube 20190328T200344S309_pol_iofL2pan.cub: "
           << e.what() << std::endl;
  }

  // ingest 2nd polycam fits format image 
  QString polyCam2FileName = tempDir.path() + "/20190328T200717S769_pol_iofL2pan.cub";
  QVector<QString> ocams2isisArgs2 = {"from=data/osirisRexImages/ocams/20190328T200717S769_pol_iofL2pan.fits",
                                      "to=" + polyCam2FileName};

  UserInterface ocams2isisPolyCam2UI(OCAMS2ISIS_XML, ocams2isisArgs2);
  try {
    ocams2isis(ocams2isisPolyCam2UI);
  }
  catch (IException &e) {
    FAIL() << "Unable to run ocams2isis on second polycam image 20190328T200717S769_pol_iofL2pan.fits: "
           << e.what() << std::endl;
  }

  // spiceinit
  QVector<QString> spiceinitArgs2 = {"from=" + polyCam2FileName};
  UserInterface spiceinitPolyCam2(SPICEINIT_XML, spiceinitArgs2);
  try {
    spiceinit(spiceinitPolyCam2);
  }
  catch (IException &e) {
    FAIL() << "Unable to run spiceinit on second polycam cube 20190328T200717S769_pol_iofL2pan.cub: "
           << e.what() << std::endl;
  }

  // footprintinit
  QVector<QString> footprintinitArgs2 = {"from=" + polyCam2FileName};
  UserInterface footprintinitPolyCam2UI(FOOTPRINTINIT_XML, footprintinitArgs2);
  try {
    footprintinit(footprintinitPolyCam2UI);
  }
  catch (IException &e) {
    FAIL() << "Unable to run footprintinit on second polycam cube 20190328T200717S769_pol_iofL2pan.cub: "
           << e.what() << std::endl;
  }

  // compute lat/lon range of the two polycam cubes for mosaicking with mosrange
  FileList *cubeList = new FileList(); 
  cubeList->append(polyCam1FileName);
  cubeList->append(polyCam2FileName);
  
  QString cubeListFile = tempDir.path() + "/unprojectedSpiceinitCubeList.lis";
  cubeList->write(cubeListFile);

  QString equiMapFile = tempDir.path() + "/polycam_equi.map";

  QVector<QString> mosrangeArgs = {"fromlist=" + cubeListFile,
                                   "to=" + equiMapFile,
                                   "precision=6",
                                   "projection=Equirectangular"};
  UserInterface mosrangeUI(MOSRANGE_XML, mosrangeArgs);
  try {
    mosrange(mosrangeUI);
  }
  catch (IException &e) {
    FAIL() << "Unable to run mosrange on polycam cubes: " << e.what() << std::endl;
  }

  // validate equirectangular map file from mosrange
  Pvl mosrangeMapPvl;
  mosrangeMapPvl.read(equiMapFile);

  // Mapping Group
  PvlGroup &mappingGroup = mosrangeMapPvl.findGroup("Mapping", Pvl::Traverse);
  EXPECT_EQ(mappingGroup["ProjectionName"][0].toStdString(), "Equirectangular");
  EXPECT_EQ(mappingGroup["TargetName"][0].toStdString(), "Bennu");
  EXPECT_DOUBLE_EQ(double(mappingGroup["EquatorialRadius"]), 283.065);
  EXPECT_DOUBLE_EQ(double(mappingGroup["PolarRadius"]), 249.72);  
  EXPECT_EQ(mappingGroup["LatitudeType"][0].toStdString(), "Planetocentric");
  EXPECT_EQ(mappingGroup["LongitudeDirection"][0].toStdString(), "PositiveEast");
  EXPECT_EQ(int(mappingGroup["LongitudeDomain"]), 360);
  EXPECT_NEAR((double)mappingGroup.findKeyword("PixelResolution"), 0.04718, 1e-5);
  EXPECT_NEAR((double)mappingGroup.findKeyword("Scale"), 104.713817, 1e-5);
  EXPECT_NEAR((double)mappingGroup.findKeyword("MinPixelResolution"), 0.046851605470021, 1e-8);
  EXPECT_NEAR((double)mappingGroup.findKeyword("MaxPixelResolution"), 0.047508745368827, 1e-8);
  EXPECT_NEAR((double)mappingGroup.findKeyword("MinObliquePixelResolution"), 0.05296474800839, 1e-8);
  EXPECT_NEAR((double)mappingGroup.findKeyword("MaxObliquePixelResolution"), 0.074068376061469, 1e-8);
  EXPECT_NEAR((double)mappingGroup.findKeyword("CenterLongitude"), 31.168954, 1e-5);
  EXPECT_NEAR((double)mappingGroup.findKeyword("CenterLatitude"), -0.557973, 1e-5);
  EXPECT_NEAR((double)mappingGroup.findKeyword("MinimumLatitude"), -9.275169, 1e-5);
  EXPECT_NEAR((double)mappingGroup.findKeyword("MaximumLatitude"), 8.159223, 1e-5);
  EXPECT_NEAR((double)mappingGroup.findKeyword("MinimumLongitude"), 22.534669, 1e-5);
  EXPECT_NEAR((double)mappingGroup.findKeyword("MaximumLongitude"), 39.80324, 1e-5);
  EXPECT_NEAR((double)mappingGroup.findKeyword("PreciseCenterLongitude"), 31.168954451306, 1e-8);
  EXPECT_NEAR((double)mappingGroup.findKeyword("PreciseCenterLatitude"), -0.55797299698864, 1e-8);
  EXPECT_NEAR((double)mappingGroup.findKeyword("PreciseMinimumLatitude"), -9.2751681116112, 1e-8);
  EXPECT_NEAR((double)mappingGroup.findKeyword("PreciseMaximumLatitude"), 8.1592221176339, 1e-8);
  EXPECT_NEAR((double)mappingGroup.findKeyword("PreciseMinimumLongitude"), 22.534669508607, 1e-8);
  EXPECT_NEAR((double)mappingGroup.findKeyword("PreciseMaximumLongitude"), 39.803239394005, 1e-8);

  // project the two images using the equirectangular map from mosrange
  QVector<QString> cam2mapArgs1 = {"from=" + polyCam1FileName,
                                   "to=" + tempDir.path() + "/20190328T200344S309_pol_iofL2pan_map.cub",
                                   "map=" + tempDir.path() + "/polycam_equi.map",
                                   "pixres=map"};
  UserInterface cam2map1UI(CAM2MAP_XML, cam2mapArgs1);
  try {
    cam2map(cam2map1UI);
  }
  catch (IException &e) {
    FAIL() << "Unable to run cam2map on first polycam cube 20190328T200344S309_pol_iofL2pan.cub: "
           << e.what() << std::endl;
  }

  QVector<QString> cam2mapArgs2 = {"from=" + polyCam2FileName,
                                   "to=" + tempDir.path() + "/20190328T200717S769_pol_iofL2pan_map.cub",
                                   "map=" + tempDir.path() + "/polycam_equi.map",
                                   "pixres=map"};
  UserInterface cam2map2UI(CAM2MAP_XML, cam2mapArgs2);
  try {
    cam2map(cam2map2UI);
  }
  catch (IException &e) {
    FAIL() << "Unable to run cam2map on second polycam cube 20190328T200717S769_pol_iofL2pan.cub: "
           << e.what() << std::endl;
  }

  // create uncontrolled mosaic of the two equirectangular projected polycam images
  FileList *projectedCubeList = new FileList(); 
  projectedCubeList->append(tempDir.path() + "/20190328T200344S309_pol_iofL2pan_map.cub");
  projectedCubeList->append(tempDir.path() + "/20190328T200717S769_pol_iofL2pan_map.cub");
  
  QString projectedCubeListFile = tempDir.path() + "/projectedCubeList.lis";
  projectedCubeList->write(projectedCubeListFile);

  QVector<QString> automosArgs = {"fromlist=" + projectedCubeListFile,
                                  "mosaic=" + tempDir.path() + "/mosaicUncontrolled.cub"};
  UserInterface automosUI(AUTOMOS_XML, automosArgs);
  try {
    automos(automosUI);
  }
  catch (IException &e) {
    FAIL() << "Unable to run automos on polycam cubes: " << e.what() << std::endl;
  }

  // validate mapping label and histogram of uncontrolled mosaic cube
  Cube mosaic(tempDir.path() + "/mosaicUncontrolled.cub");

  PvlGroup &mosaicMapGroup = (mosaic.label())->findObject("IsisCube").findGroup("Mapping");
  EXPECT_PRED_FORMAT2(AssertQStringsEqual, (QString)mosaicMapGroup.findKeyword("ProjectionName"), "Equirectangular");
  EXPECT_NEAR((double)mosaicMapGroup.findKeyword("CenterLongitude"), 31.168954, 1e-5);
  EXPECT_PRED_FORMAT2(AssertQStringsEqual, (QString)mosaicMapGroup.findKeyword("TargetName"), "Bennu");
  EXPECT_DOUBLE_EQ((double)mosaicMapGroup.findKeyword("EquatorialRadius"), 283.065);
  EXPECT_DOUBLE_EQ((double)mosaicMapGroup.findKeyword("PolarRadius"), 249.72);
  EXPECT_EQ(mosaicMapGroup.findKeyword("LatitudeType")[0], "Planetocentric");
  EXPECT_EQ(mosaicMapGroup.findKeyword("LongitudeDirection")[0], "PositiveEast");
  EXPECT_EQ((int)mosaicMapGroup.findKeyword("LongitudeDomain"), 360);
  EXPECT_NEAR((double)mosaicMapGroup.findKeyword("MinimumLatitude"), -9.2751681116112, 1e-8);
  EXPECT_NEAR((double)mosaicMapGroup.findKeyword("MaximumLatitude"), 8.1592221176339, 1e-8);
  EXPECT_NEAR((double)mosaicMapGroup.findKeyword("MinimumLongitude"), 22.534669508607, 1e-8);
  EXPECT_NEAR((double)mosaicMapGroup.findKeyword("MaximumLongitude"), 39.803239394005, 1e-8);
  EXPECT_NEAR((double)mosaicMapGroup.findKeyword("UpperLeftCornerX"), -42.6979, 1e-5);
  EXPECT_NEAR((double)mosaicMapGroup.findKeyword("UpperLeftCornerY"), 40.3389, 1e-5);
  EXPECT_NEAR((double)mosaicMapGroup.findKeyword("PixelResolution"), 0.04718, 1e-5);
  EXPECT_NEAR((double)mosaicMapGroup.findKeyword("Scale"), 104.71279164747, 1e-8);
  EXPECT_NEAR((double)mosaicMapGroup.findKeyword("CenterLatitude"), -0.557973, 1e-5);
  EXPECT_NEAR((double)mosaicMapGroup.findKeyword("CenterLatitudeRadius"), 283.06117623837, 1e-8);

  std::unique_ptr<Histogram> mosaicHist(mosaic.histogram());
  EXPECT_NEAR(mosaicHist->Average(), 0.013594288512311557, 1e-8);
  EXPECT_NEAR(mosaicHist->Sum(), 28517.174029508347, 1e-8);
  EXPECT_EQ(mosaicHist->ValidPixels(), 2097732);
  EXPECT_NEAR(mosaicHist->StandardDeviation(), 0.00734382104638924, 1e-8);

  // band trim automos mosaic
  QVector<QString> bandtrimArgs = {"from=" + tempDir.path() + "/mosaicUncontrolled.cub",
                                 "to=" + tempDir.path() + "/mosaicTrimmedUncontrolled.cub"};
  UserInterface bandtrimUI(BANDTRIM_XML, bandtrimArgs);
  try {
    bandtrim(bandtrimUI);
  }
  catch (IException &e) {
    FAIL() << "Unable to run bandtrim on polycam two image mosaic mosaicUncontrolled.cub: "
           << e.what() << std::endl;
  }

  // validate histogram of trimmed, uncontrolled noseam mosaic cube
  Cube trimmedUncontrolledMosaic(tempDir.path() + "/mosaicTrimmedUncontrolled.cub");

  std::unique_ptr<Histogram> trimmedUncontrolledMosaicHist(trimmedUncontrolledMosaic.histogram());
  EXPECT_NEAR(trimmedUncontrolledMosaicHist->Average(), 0.013594288512311557, 1e-8);
  EXPECT_NEAR(trimmedUncontrolledMosaicHist->Sum(), 28517.174029508347, 1e-8);
  EXPECT_EQ(trimmedUncontrolledMosaicHist->ValidPixels(), 2097732);
  EXPECT_NEAR(trimmedUncontrolledMosaicHist->StandardDeviation(), 0.00734382104638924, 1e-8);

  // create uncontrolled noseam mosaic
  QVector<QString> noseamArgs = {"fromlist=" + projectedCubeListFile,
                                 "to=" + tempDir.path() + "/noseamUncontrolledMosaic.cub",
                                 "samples=5",
                                 "lines=5"};
  UserInterface noseamUI(NOSEAM_XML, noseamArgs);
  try {
    noseam(noseamUI);
  }
  catch (IException &e) {
    FAIL() << "Unable to run noseam on polycam images: " << e.what() << std::endl;
  }

  // validate histogram of uncontrolled noseam mosaic cube
  Cube noseamMosaic(tempDir.path() + "/noseamUncontrolledMosaic.cub");

  std::unique_ptr<Histogram> noseamMosaicHist(noseamMosaic.histogram());
  EXPECT_NEAR(noseamMosaicHist->Average(), 0.013594249872813711, 1e-8);
  EXPECT_NEAR(noseamMosaicHist->Sum(), 28517.09297419725, 1e-8);
  EXPECT_EQ(noseamMosaicHist->ValidPixels(), 2097732);
  EXPECT_NEAR(noseamMosaicHist->StandardDeviation(), 0.0073422521190466645, 1e-8);

  // find overlaps of the two images
  QVector<QString> findimageoverlapsArgs =
      {"fromlist=" + tempDir.path() + "/unprojectedSpiceinitCubeList.lis",
       "overlaplist=" + tempDir.path() + "/polycam_findimageoverlaps.txt"};
  UserInterface findimageoverlapsUI(FINDIMAGEOVERLAPS_XML, findimageoverlapsArgs);
  try {
    findimageoverlaps(findimageoverlapsUI);
  }
  catch (IException &e) {
    FAIL() << "Unable to run findimageoverlaps on polycam images: " << e.what() << std::endl;
  }

  Pvl overlapstatsLog;

  // generate overlap stats for the two images
  QVector<QString> overlapstatsArgs =
      {"fromlist=" + tempDir.path() + "/unprojectedSpiceinitCubeList.lis",
       "overlaplist=" + tempDir.path() + "/polycam_findimageoverlaps.txt",
       "to=" + tempDir.path() + "/polycam_overlapstats.csv"};
  UserInterface overlapstatsUI(OVERLAPSTATS_XML, overlapstatsArgs);
  try {
    overlapstats(overlapstatsUI, &overlapstatsLog);
  }
  catch (IException &e) {
    FAIL() << "Unable to run overlapstats on polycam images: " << e.what() << std::endl;
  }

  // validate overlapstatsLog app log file

  PvlGroup &resultsGroup = overlapstatsLog.findGroup("Results");
  EXPECT_NEAR((double)resultsGroup.findKeyword("ThicknessMinimum"), 0.31325186797657001, 1e-8);
  EXPECT_NEAR((double)resultsGroup.findKeyword("ThicknessMaximum"), 0.31325186797657001, 1e-8);
  EXPECT_NEAR((double)resultsGroup.findKeyword("ThicknessAverage"), 0.31325186797657001, 1e-8);
  EXPECT_NEAR((double)resultsGroup.findKeyword("AreaMinimum"), 1437.1823616088, 1e-8);
  EXPECT_NEAR((double)resultsGroup.findKeyword("AreaMaximum"), 1437.1823616088, 1e-8);
  EXPECT_NEAR((double)resultsGroup.findKeyword("AreaAverage"), 1437.1823616088, 1e-8);
  EXPECT_DOUBLE_EQ((double)resultsGroup.findKeyword("ImageStackMinimum"), 2.0);
  EXPECT_DOUBLE_EQ((double)resultsGroup.findKeyword("ImageStackMaximum"), 2.0);
  EXPECT_DOUBLE_EQ((double)resultsGroup.findKeyword("ImageStackAverage"), 2.0);
  EXPECT_EQ(int(resultsGroup["PolygonCount"]), 3);

  // create autoseed.def file
  ofstream of;
  of.open(tempDir.path().toStdString() + "/autoseed.def");
  of << "Object = AutoSeed\n";
  of << "  Group = PolygonSeederAlgorithm\n";
  of << "    Name = Grid\n";
  of << "    MinimumThickness = 0.0\n";
  of << "    MinimumArea = 1\n";
  of << "    XSpacing = 5\n";
  of << "    YSpacing = 5\n";
  of << "  EndGroup\n";
  of << "EndObject\n";
  of.close();

  // autoseed the two polycam cubes
  QVector<QString> autoseedArgs =
      {"fromlist=" + tempDir.path() + "/unprojectedSpiceinitCubeList.lis",
       "overlaplist=" + tempDir.path() + "/polycam_findimageoverlaps.txt",
       "onet=" + tempDir.path() + "/polycam_autoseed.net",
       "deffile=" + tempDir.path() + "/autoseed.def",
       "networkid=OREx",
       "pointid=Bennu????",
       "description=auto"};
  UserInterface autoseedUI(AUTOSEED_XML, autoseedArgs);
  try {
    autoseed(autoseedUI);
  }
  catch (IException &e) {
    FAIL() << "Unable to run autoseed on polycam images: " << e.what() << std::endl;
  }

  // validate output control network
  ControlNet autoseedNet(tempDir.path() + "/polycam_autoseed.net");
  ASSERT_EQ(autoseedNet.GetNumPoints(), 58);
  ASSERT_EQ(autoseedNet.GetNumValidPoints(), 58);
  EXPECT_EQ(autoseedNet.GetNumMeasures(), 116);
  ASSERT_EQ(autoseedNet.GetNumValidMeasures(), 116);
  EXPECT_EQ(autoseedNet.GetNumIgnoredMeasures(), 0);

  // create pointreg.def file
  of.open(tempDir.path().toStdString() + "/pointreg.def");
  of << "Object = AutoRegistration\n";
  of << "  Group = Algorithm\n";
  of << "    Name         = MaximumCorrelation\n";
  of << "    Tolerance    = 0.7\n";
  of << "    SubPixelAccuracy = True\n";
  of << "  EndGroup\n";
  of << "  Group = PatternChip\n";
  of << "    Samples = 25\n";
  of << "    Lines   = 25\n";
  of << "  EndGroup\n";
  of << "  Group = SearchChip\n";
  of << "    Samples = 100\n";
  of << "    Lines   = 100\n";
  of << "  EndGroup\n";
  of << "EndObject\n";
  of.close();

  // register the two images with pointreg
  Pvl pointregLog;
  QVector<QString> pointregArgs =
      {"fromlist=" + tempDir.path() + "/unprojectedSpiceinitCubeList.lis",
       "cnet=" + tempDir.path() + "/polycam_autoseed.net",
       "deffile=" + tempDir.path() + "/pointreg.def",
       "onet=" + tempDir.path() + "/polycam_pointreg.net"};
  UserInterface pointregUI(POINTREG_XML, pointregArgs);
  try {
    pointreg(pointregUI, &pointregLog);
  }
  catch (IException &e) {
    FAIL() << "Unable to run pointreg on polycam images: " << e.what() << std::endl;
  }

  // validate output control network
  ControlNet pointregNet(tempDir.path() + "/polycam_pointreg.net");
  ASSERT_EQ(pointregNet.GetNumPoints(), 58);
  ASSERT_EQ(pointregNet.GetNumValidPoints(), 58);
  EXPECT_EQ(pointregNet.GetNumMeasures(), 116);
  ASSERT_EQ(pointregNet.GetNumValidMeasures(), 116);
  EXPECT_EQ(pointregNet.GetNumIgnoredMeasures(), 0);
}

