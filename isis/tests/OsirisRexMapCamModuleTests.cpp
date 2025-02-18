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
#include "ProgramLauncher.h"
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
 *   @history 2017-08-29 Jeannie Backer
 *            - Updated to new test cube that has pre-processing flip applied.
 *   @history 2025-01-14 Ken Edmundson
 *            - Converted tests from Makefile to ctest module format.
 *            - Using different calibrated MapCam images, from OSIRIS-REx Detailed Survey
 *              (https://sbnarchive.psi.edu/pds4/orex/orex.ocams/data_calibrated/detailed_survey):
 *              20190223T023051S790_map_iofL2pan.fits
 *              20190223T024051S802_map_iofL2pan.fits
 *            - Removed original calls to coreg and photomet apps as OSIRIS-REx IPWG determined
 *              these were not necessary.
 */

/**
   * MapCamModuleExplodeReuniteTest
   * 
   * 1) Mapcam image 20190223T023051S790_map_iofL2pan.fits is ingested with ocams2isis and
   *    spiceinited.
   * 2) phocube is used to create latitude, longitude, phase angle, emission angle, incidence
   *    angle, and pixel resolution backplanes.
   * 3) explode app outputs the image backplanes into 7 separate cubes.
   * 4) cubeit reunites the exploded cubes.
   * 5) The test then compares resulting phocube and cubeit cube labels and histograms,
   *    which should be identical.
   * 
   * INPUT: isis/tests/data/osirisRexImages/ocams/20190223T023051S790_map_iofL2pan.fits
   * 
   * OUTPUT: 1) ocams2isis
   *            - 20190223T023051S790_map_iofL2pan.cub
   *         2) phocube
   *            - 20190223T023051S790_map_iofL2pan_bp.cub
   *         3) explode (produces 7 cubes, one for each band, e.g.)
   *            - 20190223T023051S790_map_iofL2pan_V001.band0001.cub (band0001-band0007)
   *         4) cubeit
   *            - 20190223T023051S790_map_iofL2pan_V001_cubeit.cub
   */
TEST(OsirisRexMapCamModules, MapCamModuleExplodeReuniteTest) {
  QTemporaryDir tempDir;

  // ingest mapcam fits format image with ocams2isis and then spiceinit
  QString mapCamFileName = tempDir.path() + "/20190223T023051S790_map_iofL2pan.cub";
  QVector<QString> ocams2isisArgs = {"from=data/osirisRexImages/ocams/20190223T023051S790_map_iofL2pan.fits",
                                     "to=" + mapCamFileName};

  UserInterface ocams2isisMapCam(OCAMS2ISIS_XML, ocams2isisArgs);
  try {
    ocams2isis(ocams2isisMapCam);
  }
  catch (IException &e) {
    FAIL() << "Unable to run ocams2isis on mapcam fits file 20190223T023051S790_map_iofL2pan.fits: "
           << e.what() << std::endl;
  }

  QVector<QString> spiceinitArgs = {"from=" + mapCamFileName};
  UserInterface spiceinitMapCam(SPICEINIT_XML, spiceinitArgs);
  try {
    spiceinit(spiceinitMapCam);
  }
  catch (IException &e) {
    FAIL() << "Unable to run spiceinit on mapcam image: " << e.what() << std::endl;
  }

  // phocube mapcam image to create backplanes for latitude, longitude, phase angle,
  // emission angle, incidence angle, and pixel resolution;
	// dn=y propagates input pixel to output file
  QString phocubeFileName = tempDir.path() + "/20190223T023051S790_map_iofL2pan_bp.cub";
  QVector<QString> phocubeArgs = {"from=" + mapCamFileName,
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
    FAIL() << "Unable to run phocube on mapcam image: " << e.what() << std::endl;
  }

  // explode image backplanes into 7 cubes
  QVector<QString> explodeArgs = {"from=" + phocubeFileName,
                                  "to=" + tempDir.path() + "/20190223T023051S790_map_iofL2pan_V001"};
  UserInterface explodeUI(EXPLODE_XML, explodeArgs);
  try {
    explode(explodeUI);
  }
  catch (IException &e) {
    FAIL() << "Unable to run explode on phocubed mapcam image: " << e.what() << std::endl;
  }

  // reunite exploded cubes
	// output should be identical to phocube bp output above
  FileList *cubeList = new FileList(); // list of exploded cubes
  cubeList->append(tempDir.path() + "/20190223T023051S790_map_iofL2pan_V001.band0001.cub");
  cubeList->append(tempDir.path() + "/20190223T023051S790_map_iofL2pan_V001.band0002.cub");
  cubeList->append(tempDir.path() + "/20190223T023051S790_map_iofL2pan_V001.band0003.cub");
  cubeList->append(tempDir.path() + "/20190223T023051S790_map_iofL2pan_V001.band0004.cub");
  cubeList->append(tempDir.path() + "/20190223T023051S790_map_iofL2pan_V001.band0005.cub");
  cubeList->append(tempDir.path() + "/20190223T023051S790_map_iofL2pan_V001.band0006.cub");
  cubeList->append(tempDir.path() + "/20190223T023051S790_map_iofL2pan_V001.band0007.cub");
  
  QString cubeListFile = tempDir.path() + "/phocubeBandList.lis";
  cubeList->write(cubeListFile);

  QString cubeitFileName = tempDir.path() + "/20190223T023051S790_map_iofL2pan_V001_cubeit.cub";

  QVector<QString> cubeitArgs = {"from=" + cubeListFile,
                                  "to=" + cubeitFileName};
  UserInterface cubeitUI(CUBEIT_XML, cubeitArgs);
  try {
    cubeit(cubeitUI);
  }
  catch (IException &e) {
    FAIL() << "Unable to run cubeit on exploded mapcam images: " << e.what() << std::endl;
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
   * MapCamModuleAlgebraTest
   * 
   * 1) Mapcam image 20190223T023051S790_map_iofL2pan.fits is ingested with ocams2isis (spiceinit
   *    isn't required).
   * 2) algebra is used modify the dn values by doubling and adding 1.
   * 3) The test compares the histograms from the original and algebra'd cubes.
   * 
   * INPUT: isis/tests/data/osirisRexImages/ocams/20190223T023051S790_map_iofL2pan.fits
   * 
   * OUTPUT: 1) ocams2isis
   *            - 20190223T023051S790_map_iofL2pan.cub
   *         2) algebra
   *            - 20190223T023051S790_map_iofL2pan_algebra.cub
   */
  TEST(OsirisRexMapCamModules, MapCamModuleAlgebraTest) {
  QTemporaryDir tempDir;

  // ingest mapcam fits format image with ocams2isis
  QString mapCamFileName = tempDir.path() + "/20190223T023051S790_map_iofL2pan.cub";
  QVector<QString> ocams2isisArgs = {"from=data/osirisRexImages/ocams/20190223T023051S790_map_iofL2pan.fits",
                                     "to=" + mapCamFileName};

  UserInterface ocams2isisUI(OCAMS2ISIS_XML, ocams2isisArgs);
  try {
    ocams2isis(ocams2isisUI);
  }
  catch (IException &e) {
    FAIL() << "Unable to run ocams2isis on mapcam image: " << e.what() << std::endl;
  }

  QString algebraFileName = tempDir.path() + "/20190223T023051S790_map_iofL2pan_algebra.cub";

  QVector<QString> algebraArgs = {"from=" + mapCamFileName,
                                  "to=" + algebraFileName,
                                  "operator=unary",
                                  "a=2",
                                  "c=1"};
  UserInterface algebraUI(ALGEBRA_XML, algebraArgs);
  try {
    algebra(algebraUI);
  }
  catch (IException &e) {
    FAIL() << "Unable to run algebra on mapcam image: " << e.what() << std::endl;
  }

  // compare histograms of original and algebra'd cube
  Cube originalCube(mapCamFileName);
  Cube algebraCube(algebraFileName);
  
  std::unique_ptr<Histogram> originalHist (originalCube.histogram());
  std::unique_ptr<Histogram> algebraHist (algebraCube.histogram());

  // pixels are multiplied by two and add one
  EXPECT_NEAR(algebraHist->ValidPixels(), originalHist->ValidPixels(), 0.0001);
  EXPECT_NEAR(algebraHist->Average(), (originalHist->Average()*2.0)+1.0, 0.0001);
  EXPECT_NEAR(algebraHist->Sum(), (originalHist->Sum()*2)+originalHist->ValidPixels(), 0.0001);
  EXPECT_NEAR(algebraHist->StandardDeviation(), (originalHist->StandardDeviation()*2), 0.0001);
}


/**
   * MapCamModuleInfoAndStatsTest
   * 
   * 1) Mapcam image 20190223T023051S790_map_iofL2pan.fits is ingested with ocams2isis
   *    and spiceinited.
   * 2) getsn generates the cube serial number.
   * 3) caminfo outputs various spacecraft and instrument-related details (e.g. geometric,
   *    polygon, and mapping info).
   * 4) camstats generates camera statistics.
   * 5) stats generates pixel statistics.
   * 6) The test validates the output serial number, info, and statistics files.
   * 
   * INPUT: isis/tests/data/osirisRexImages/ocams/20190223T023051S790_map_iofL2pan.fits
   * 
   * OUTPUT: 1) ocams2isis
   *            - 20190223T023051S790_map_iofL2pan.cub
   *         2) getsn
   *            - mapcam_getsn.pvl
   *         3) caminfo
   *            - 20190223T023051S790_map_iofL2pan_caminfo.pvl
   *         4) camstats
   *            - 20190223T023051S790_map_iofL2pan_camstats.pvl
   *         5) stats
   *            - 20190223T023051S790_map_iofL2pan_dnstats.pvl
   */
  TEST(OsirisRexMapCamModules, MapCamModuleInfoAndStatsTest) {
  QTemporaryDir tempDir;

  // ingest mapcam fits format image with ocams2isis and then spiceinit
  QString mapCamFileName = tempDir.path() + "/20190223T023051S790_map_iofL2pan.cub";
  QVector<QString> ocams2isisArgs = {"from=data/osirisRexImages/ocams/20190223T023051S790_map_iofL2pan.fits",
                                     "to=" + mapCamFileName};

  UserInterface ocams2isisUI(OCAMS2ISIS_XML, ocams2isisArgs);
  try {
    ocams2isis(ocams2isisUI);
  }
  catch (IException &e) {
    FAIL() << "Unable to run ocams2isis on mapcam image: " << e.what() << std::endl;
  }

  QVector<QString> spiceinitArgs = {"from=" + mapCamFileName};
  UserInterface spiceinitUI(SPICEINIT_XML, spiceinitArgs);
  try {
    spiceinit(spiceinitUI);
  }
  catch (IException &e) {
    FAIL() << "Unable to run spiceinit on mapcam image: " << e.what() << std::endl;
  }

  // getsn from mapcam image
  QVector<QString> getsnArgs = {"from=" + mapCamFileName,
                                 "to=" + tempDir.path() + "/mapcam_getsn.pvl",
                                 "append=yes"};
  UserInterface getsnUI(GETSN_XML, getsnArgs);
  Pvl appLog;
  try {
    getsn(getsnUI, &appLog);
  }
  catch (IException &e) {
    FAIL() << "Unable to run getsn on mapcam image: " << e.what() << std::endl;
  }

  // validate mapcam image serial number
  Pvl getsnPvl;
  getsnPvl.read(tempDir.path() + "/mapcam_getsn.pvl");

  // Results Group
  PvlGroup &getSNResultsGroup = getsnPvl.findGroup("Results", Pvl::Traverse);
  EXPECT_EQ(getSNResultsGroup["SerialNumber"][0].toStdString(), "OsirisRex/MapCam/3/0604161014.25874" );

  // get camera information
  QVector<QString> caminfoArgs = {"from=" + mapCamFileName,
                                  "to=" + tempDir.path() + "/20190223T023051S790_map_iofL2pan_caminfo.pvl"};
  UserInterface caminfoUI(CAMINFO_XML, caminfoArgs);
  try {
    caminfo(caminfoUI);
  }
  catch (IException &e) {
    FAIL() << "Unable to run caminfo on mapcam image: " << e.what() << std::endl;
  }

  // validate caminfo file
  Pvl caminfoPvl;
  caminfoPvl.read(tempDir.path() + "/20190223T023051S790_map_iofL2pan_caminfo.pvl");

  // Parameters Object
  PvlObject &parametersObject = caminfoPvl.findObject("Parameters", Pvl::Traverse);
  EXPECT_EQ(parametersObject["Program"][0].toStdString(), "caminfo");
  EXPECT_EQ(parametersObject["IsisId"][0].toStdString(), "OsirisRex/MapCam/3/0604161014.25874");
  EXPECT_EQ(parametersObject["From"][0].toStdString(), "20190223T023051S790_map_iofL2pan.cub" );
  EXPECT_EQ(int(parametersObject["Lines"]), 1024);
  EXPECT_EQ(int(parametersObject["Samples"]), 1024);
  EXPECT_EQ(int(parametersObject["Bands"]), 1);

  // Geometry Object
  PvlObject &geometryObject = caminfoPvl.findObject("Geometry", Pvl::Traverse);
  EXPECT_EQ(int(geometryObject["BandsUsed"]), 1);
  EXPECT_EQ(int(geometryObject["ReferenceBand"]), 1);
  EXPECT_EQ(int(geometryObject["OriginalBand"]), 1);
  EXPECT_EQ(geometryObject["Target"][0].toStdString(), "Bennu");
  EXPECT_EQ(geometryObject["StartTime"][0].toStdString(), "2019-02-23T02:30:51.765541");
  EXPECT_EQ(geometryObject["EndTime"][0].toStdString(), "2019-02-23T02:30:51.765541" );
  EXPECT_DOUBLE_EQ(double(geometryObject["CenterLine"]), 512.0);
  EXPECT_DOUBLE_EQ(double(geometryObject["CenterSample"]), 512.0);
  EXPECT_NEAR(double(geometryObject.findKeyword("CenterLatitude")), 63.939693821928, 1e-8);
  EXPECT_NEAR(double(geometryObject.findKeyword("CenterLongitude")), 254.37764186472, 1e-8);
  EXPECT_NEAR(double(geometryObject.findKeyword("CenterRadius")), 253.59833006535, 1e-8);
  EXPECT_NEAR(double(geometryObject.findKeyword("RightAscension")), 272.75907287018, 1e-8);
  EXPECT_NEAR(double(geometryObject.findKeyword("Declination")), 53.221788172495, 1e-8);
  EXPECT_NEAR(double(geometryObject.findKeyword("UpperLeftLongitude")), 307.4684697132, 1e-8);
  EXPECT_NEAR(double(geometryObject.findKeyword("UpperLeftLatitude")), 72.941762041119, 1e-8);
  EXPECT_NEAR(double(geometryObject.findKeyword("LowerLeftLongitude")), 274.97895219323, 1e-8);
  EXPECT_NEAR(double(geometryObject.findKeyword("LowerLeftLatitude")), 41.884620480871, 1e-8);
  EXPECT_NEAR(double(geometryObject.findKeyword("LowerRightLongitude")), 236.24864545458, 1e-8);
  EXPECT_NEAR(double(geometryObject.findKeyword("LowerRightLatitude")), 41.357364496984, 1e-8);
  EXPECT_NEAR(double(geometryObject.findKeyword("UpperRightLongitude")), 201.90727118408, 1e-8);
  EXPECT_NEAR(double(geometryObject.findKeyword("UpperRightLatitude")), 71.07703430006, 1e-8);
  EXPECT_NEAR(double(geometryObject.findKeyword("PhaseAngle")), 96.979536059037, 1e-8);
  EXPECT_NEAR(double(geometryObject.findKeyword("EmissionAngle")), 30.5649867207, 1e-8);
  EXPECT_NEAR(double(geometryObject.findKeyword("IncidenceAngle")), 66.414678374557, 1e-8);
  EXPECT_NEAR(double(geometryObject.findKeyword("NorthAzimuth")), 267.08994573458, 1e-8);
  EXPECT_NEAR(double(geometryObject.findKeyword("OffNadir")), 0.38858338848542, 1e-8);
  EXPECT_NEAR(double(geometryObject.findKeyword("SolarLongitude")), 28.894668237098, 1e-8);
  EXPECT_NEAR(double(geometryObject.findKeyword("LocalTime")), 11.845674610712, 1e-8);
  EXPECT_NEAR(double(geometryObject.findKeyword("TargetCenterDistance")), 2.0259586597458, 1e-8);
  EXPECT_NEAR(double(geometryObject.findKeyword("SlantDistance")), 1.8112189411342, 1e-8);
  EXPECT_NEAR(double(geometryObject.findKeyword("SampleResolution")), 0.12296614216965, 1e-8);
  EXPECT_NEAR(double(geometryObject.findKeyword("LineResolution")), 0.12296614216965, 1e-8);
  EXPECT_NEAR(double(geometryObject.findKeyword("PixelResolution")), 0.12296614216965, 1e-8);
  EXPECT_NEAR(double(geometryObject.findKeyword("MeanGroundResolution")), 0.1243209982227, 1e-8);
  EXPECT_NEAR(double(geometryObject.findKeyword("SubSolarAzimuth")), 90.237294274039, 1e-8);
  EXPECT_NEAR(double(geometryObject.findKeyword("SubSolarGroundAzimuth")), 177.39710616737, 1e-8);
  EXPECT_NEAR(double(geometryObject.findKeyword("SubSolarLatitude")), 1.1871474270835, 1e-8);
  EXPECT_NEAR(double(geometryObject.findKeyword("SubSolarLongitude")), 256.69252270405, 1e-8);
  EXPECT_NEAR(double(geometryObject.findKeyword("SubSpacecraftAzimuth")), 268.56958890645, 1e-8);
  EXPECT_NEAR(double(geometryObject.findKeyword("SubSpacecraftGroundAzimuth")), 358.7763197288, 1e-8);
  EXPECT_NEAR(double(geometryObject.findKeyword("SubSpacecraftLatitude")), 85.843896278413, 1e-8);
  EXPECT_NEAR(double(geometryObject.findKeyword("SubSpacecraftLongitude")), 82.895507575855, 1e-8);
  EXPECT_NEAR(double(geometryObject.findKeyword("ParallaxX")), -0.59043913514882, 1e-8);
  EXPECT_NEAR(double(geometryObject.findKeyword("ParallaxY")), -0.012612073705184, 1e-8);
  EXPECT_NEAR(double(geometryObject.findKeyword("ShadowX")), 2.288145703671, 1e-8);
  EXPECT_NEAR(double(geometryObject.findKeyword("ShadowY")), 0.10401989413454, 1e-8);
  EXPECT_EQ(geometryObject["HasLongitudeBoundary"][0].toStdString(), "FALSE");
  EXPECT_EQ(geometryObject["HasNorthPole"][0].toStdString(), "FALSE");
  EXPECT_EQ(geometryObject["HasSouthPole"][0].toStdString(), "FALSE");
  EXPECT_NEAR(double(geometryObject.findKeyword("ObliqueSampleResolution")), 0.14280903817193, 1e-8);
  EXPECT_NEAR(double(geometryObject.findKeyword("ObliqueLineResolution")), 0.14280903817193, 1e-8);
  EXPECT_NEAR(double(geometryObject.findKeyword("ObliquePixelResolution")), 0.14280903817193, 1e-8);
  EXPECT_NEAR(double(geometryObject.findKeyword("ObliqueDetectorResolution")), 0.14280903817193, 1e-8);

  // get camera statistics
  QVector<QString> camstatsArgs = {"from=" + mapCamFileName,
                                   "to=" + tempDir.path() + "/20190223T023051S790_map_iofL2pan_camstats.pvl"};
  UserInterface camstatsUI(CAMSTATS_XML, camstatsArgs);

  try {
    camstats(camstatsUI, &appLog);
  }
  catch (IException &e) {
    FAIL() << "Unable to run camstats on mapcam image: " << e.what() << std::endl;
  }

  // validate camstats file
  Pvl camstatsPvl;
  camstatsPvl.read(tempDir.path() + "/20190223T023051S790_map_iofL2pan_camstats.pvl");

  // User Parameters Group
  PvlGroup &userParametersGroup = camstatsPvl.findGroup("User Parameters", Pvl::Traverse);
  EXPECT_EQ(int(userParametersGroup["Linc"]), 1);
  EXPECT_EQ(int(userParametersGroup["Sinc"]), 1);

  // Latitude Group
  PvlGroup &latitudeGroup = camstatsPvl.findGroup("Latitude", Pvl::Traverse);
  EXPECT_NEAR((double)latitudeGroup.findKeyword("LatitudeMinimum"), 41.357364496984, 1e-8);
  EXPECT_NEAR((double)latitudeGroup.findKeyword("LatitudeMaximum"), 79.36749815371, 1e-8);
  EXPECT_NEAR((double)latitudeGroup.findKeyword("LatitudeAverage"), 61.805558141395, 1e-8);
  EXPECT_NEAR((double)latitudeGroup.findKeyword("LatitudeStandardDeviation"), 9.4536614657649, 1e-8);

  // Longitude Group
  PvlGroup &longitudeGroup = camstatsPvl.findGroup("Longitude", Pvl::Traverse);
  EXPECT_NEAR((double)longitudeGroup.findKeyword("LongitudeMinimum"), 201.90727118408, 1e-8);
  EXPECT_NEAR((double)longitudeGroup.findKeyword("LongitudeMaximum"), 307.46846971323, 1e-8);
  EXPECT_NEAR((double)longitudeGroup.findKeyword("LongitudeAverage"), 254.41109191841, 1e-8);
  EXPECT_NEAR((double)longitudeGroup.findKeyword("LongitudeStandardDeviation"), 20.270823914953, 1e-8);

  // SampleResolution Group
  PvlGroup &sampleResolutionGroup = camstatsPvl.findGroup("SampleResolution", Pvl::Traverse);
  EXPECT_NEAR((double)sampleResolutionGroup.findKeyword("SampleResolutionMinimum"), 0.12118431127729, 1e-8);
  EXPECT_NEAR((double)sampleResolutionGroup.findKeyword("SampleResolutionMaximum"), 0.1274515004168, 1e-8);
  EXPECT_NEAR((double)sampleResolutionGroup.findKeyword("SampleResolutionAverage"), 0.12347118408609, 1e-8);
  EXPECT_NEAR((double)sampleResolutionGroup.findKeyword("SampleResolutionStandardDeviation"), 0.0015863801042822, 1e-8);

  // LineResolution Group
  PvlGroup &lineResolutionGroup = camstatsPvl.findGroup("LineResolution", Pvl::Traverse);
  EXPECT_NEAR((double)lineResolutionGroup.findKeyword("LineResolutionMinimum"), 0.12118431127729, 1e-8);
  EXPECT_NEAR((double)lineResolutionGroup.findKeyword("LineResolutionMaximum"), 0.1274515004168, 1e-8);
  EXPECT_NEAR((double)lineResolutionGroup.findKeyword("LineResolutionAverage"), 0.12347118408609, 1e-8);
  EXPECT_NEAR((double)lineResolutionGroup.findKeyword("LineResolutionStandardDeviation"), 0.0015863801042822, 1e-8);

  // Resolution Group
  PvlGroup &resolutionGroup = camstatsPvl.findGroup("Resolution", Pvl::Traverse);
  EXPECT_NEAR((double)resolutionGroup.findKeyword("ResolutionMinimum"), 0.12118431127729, 1e-8);
  EXPECT_NEAR((double)resolutionGroup.findKeyword("ResolutionMaximum"), 0.1274515004168, 1e-8);
  EXPECT_NEAR((double)resolutionGroup.findKeyword("ResolutionAverage"), 0.12347118408609, 1e-8);
  EXPECT_NEAR((double)resolutionGroup.findKeyword("ResolutionStandardDeviation"), 0.0015863801042822, 1e-8);

  // ObliqueSampleResolution Group
  PvlGroup &obliqeSampleResolutionGroup = camstatsPvl.findGroup("ObliqueSampleResolution", Pvl::Traverse);
  EXPECT_NEAR((double)obliqeSampleResolutionGroup.findKeyword("ObliqueSampleResolutionMinimum"), 0.12556256961251, 1e-8);
  EXPECT_NEAR((double)obliqeSampleResolutionGroup.findKeyword("ObliqueSampleResolutionMaximum"), 0.21557326777525, 1e-8);
  EXPECT_NEAR((double)obliqeSampleResolutionGroup.findKeyword("ObliqueSampleResolutionAverage"), 0.15085829216293, 1e-8);
  EXPECT_NEAR((double)obliqeSampleResolutionGroup.findKeyword("ObliqueSampleResolutionStandardDeviation"), 0.020518680774251, 1e-8);

  // ObliqueLineResolution Group
  PvlGroup &obliqueLineResolutionGroup = camstatsPvl.findGroup("ObliqueLineResolution", Pvl::Traverse);
  EXPECT_NEAR((double)obliqueLineResolutionGroup.findKeyword("ObliqueLineResolutionMinimum"), 0.12556256961251, 1e-8);
  EXPECT_NEAR((double)obliqueLineResolutionGroup.findKeyword("ObliqueLineResolutionMaximum"), 0.21557326777525, 1e-8);
  EXPECT_NEAR((double)obliqueLineResolutionGroup.findKeyword("ObliqueLineResolutionAverage"), 0.15085829216293, 1e-8);
  EXPECT_NEAR((double)obliqueLineResolutionGroup.findKeyword("ObliqueLineResolutionStandardDeviation"), 0.020518680774251, 1e-8);

  // ObliqueResolution Group
  PvlGroup &obliqueResolutionGroup = camstatsPvl.findGroup("ObliqueResolution", Pvl::Traverse);
  EXPECT_NEAR((double)obliqueResolutionGroup.findKeyword("ObliqueResolutionMinimum"), 0.12556256961251, 1e-8);
  EXPECT_NEAR((double)obliqueResolutionGroup.findKeyword("ObliqueResolutionMaximum"), 0.21557326777525, 1e-8);
  EXPECT_NEAR((double)obliqueResolutionGroup.findKeyword("ObliqueResolutionAverage"), 0.15085829216293, 1e-8);
  EXPECT_NEAR((double)obliqueResolutionGroup.findKeyword("ObliqueResolutionStandardDeviation"), 0.020518680774251, 1e-8);

  // AspectRatio Group
  PvlGroup &aspectRatioGroup = camstatsPvl.findGroup("AspectRatio", Pvl::Traverse);
  EXPECT_DOUBLE_EQ(double(aspectRatioGroup["AspectRatioMinimum"]), 1.0);
  EXPECT_DOUBLE_EQ(double(aspectRatioGroup["AspectRatioMaximun"]), 1.0);
  EXPECT_DOUBLE_EQ(double(aspectRatioGroup["AspectRatioAverage"]), 1.0);
  EXPECT_DOUBLE_EQ(double(aspectRatioGroup["AspectRatioStandardDeviation"]), 0.0);

  // PhaseAngle Group
  PvlGroup &phaseAngleGroup = camstatsPvl.findGroup("PhaseAngle", Pvl::Traverse);
  EXPECT_NEAR((double)phaseAngleGroup.findKeyword("PhaseMinimum"), 94.985658717673, 1e-8);
  EXPECT_NEAR((double)phaseAngleGroup.findKeyword("PhaseMaximum"), 98.968876463472, 1e-8);
  EXPECT_NEAR((double)phaseAngleGroup.findKeyword("PhaseAverage"), 96.979691174085, 1e-8);
  EXPECT_NEAR((double)phaseAngleGroup.findKeyword("PhaseStandardDeviation"), 1.148268267812, 1e-8);

  // EmissionAngle Group
  PvlGroup &emissionAngleGroup = camstatsPvl.findGroup("EmissionAngle", Pvl::Traverse);
  EXPECT_NEAR((double)emissionAngleGroup.findKeyword("EmissionMinimum"), 15.174747758074, 1e-8);
  EXPECT_NEAR((double)emissionAngleGroup.findKeyword("EmissionMaximum"), 53.756279041623, 1e-8);
  EXPECT_NEAR((double)emissionAngleGroup.findKeyword("EmissionAverage"), 32.499702065855, 1e-8);
  EXPECT_NEAR((double)emissionAngleGroup.findKeyword("EmissionStandardDeviation"), 9.8114974299382, 1e-8);

  // IncidenceAngle Group
  PvlGroup &incidenceAngleGroup = camstatsPvl.findGroup("IncidenceAngle", Pvl::Traverse);
  EXPECT_NEAR((double)incidenceAngleGroup.findKeyword("IncidenceMinimum"), 48.047622408132, 1e-8);
  EXPECT_NEAR((double)incidenceAngleGroup.findKeyword("IncidenceMaximum"), 79.861344932839, 1e-8);
  EXPECT_NEAR((double)incidenceAngleGroup.findKeyword("IncidenceAverage"), 65.638256563522, 1e-8);
  EXPECT_NEAR((double)incidenceAngleGroup.findKeyword("IncidenceStandardDeviation"), 8.8203990436048, 1e-8);

  // LocalSolarTime Group
  PvlGroup &localSolarTimeGroup = camstatsPvl.findGroup("LocalSolarTime", Pvl::Traverse);
  EXPECT_NEAR((double)localSolarTimeGroup.findKeyword("LocalSolarTimeMinimum"), 8.3476498986687, 1e-8);
  EXPECT_NEAR((double)localSolarTimeGroup.findKeyword("LocalSolarTimeMaximum"), 15.385063133946, 1e-8);
  EXPECT_NEAR((double)localSolarTimeGroup.findKeyword("LocalSolarTimeAverage"), 11.847904614291, 1e-8);
  EXPECT_NEAR((double)localSolarTimeGroup.findKeyword("LocalSolarTimeStandardDeviation"), 1.3513882609952, 1e-8);

  // LocalRadius Group
  PvlGroup &localRadiusGroup = camstatsPvl.findGroup("LocalRadius", Pvl::Traverse);
  EXPECT_NEAR((double)localRadiusGroup.findKeyword("LocalRadiusMinimum"), 250.38045868107, 1e-8);
  EXPECT_NEAR((double)localRadiusGroup.findKeyword("LocalRadiusMaximum"), 262.91440961152, 1e-8);
  EXPECT_NEAR((double)localRadiusGroup.findKeyword("LocalRadiusAverage"), 254.72966213932, 1e-8);
  EXPECT_NEAR((double)localRadiusGroup.findKeyword("LocalRadiusStandardDeviation"), 2.8790013389839, 1e-8);

  // NorthAzimuth Group
  PvlGroup &northAzimuthGroup = camstatsPvl.findGroup("NorthAzimuth", Pvl::Traverse);
  EXPECT_NEAR((double)northAzimuthGroup.findKeyword("NorthAzimuthMinimum"), 213.46350578275, 1e-8);
  EXPECT_NEAR((double)northAzimuthGroup.findKeyword("NorthAzimuthMaximum"), 321.71460924195, 1e-8);
  EXPECT_NEAR((double)northAzimuthGroup.findKeyword("NorthAzimuthAverage"), 267.09225613417, 1e-8);
  EXPECT_NEAR((double)northAzimuthGroup.findKeyword("NorthAzimuthStandardDeviation"), 21.097773403588, 1e-8);

  // get DN statistics
  QVector<QString> statsArgs = {"from=" + mapCamFileName,
                                "to=" + tempDir.path() + "/20190223T023051S790_map_iofL2pan_dnstats.pvl"};
  UserInterface statsUI(STATS_XML, statsArgs);
  try {
    stats(statsUI);
  }
  catch (IException &e) {
    FAIL() << "Unable to run stats on mapcam image: " << e.what() << std::endl;
  }

  // validate dnstats file
  Pvl dnstatsPvl;
  dnstatsPvl.read(tempDir.path() + "/20190223T023051S790_map_iofL2pan_dnstats.pvl");

  // Results Group
  PvlGroup &resultsGroup = dnstatsPvl.findGroup("Results", Pvl::Traverse);
  EXPECT_EQ(int(resultsGroup["Band"]), 1);
  EXPECT_NEAR((double)resultsGroup.findKeyword("Average"), 0.0041934945340750002, 1e-8);
  EXPECT_NEAR((double)resultsGroup.findKeyword("StandardDeviation"), 0.0037911814114704998, 1e-8);
  EXPECT_NEAR((double)resultsGroup.findKeyword("Variance"), 1.43730564946797e-05, 1e-4);
  EXPECT_NEAR(double(resultsGroup["Median"]), 0.0033871533364112998, 1e-8);
  EXPECT_NEAR(double(resultsGroup["Mode"]), 9.0965273522258303e-05, 1e-8);
  EXPECT_NEAR((double)resultsGroup.findKeyword("Skew"), 0.63806590359199999, 1e-8);
  EXPECT_NEAR(double(resultsGroup["Minimum"]), -3.2972511689877097e-05, 1e-8);
  EXPECT_NEAR(double(resultsGroup["Maximum"]), 0.018219303339720001, 1e-8);
  EXPECT_NEAR(double(resultsGroup["Sum"]), 4397.1977245622002, 1e-8);
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
   * MapCamModuleCamptTest
   * 
   * 1) Mapcam image 20190223T023051S790_map_iofL2pan.fits is ingested with ocams2isis
   *    and spiceinited.
   * 2) campt is used to compute geometric and photometric info at the cube corner
   *    and center pixels.
   * 3) The test validates the campt output.
   * 
   * INPUT: isis/tests/data/osirisRexImages/ocams/20190223T023051S790_map_iofL2pan.fits
   * 
   * OUTPUT: 1) ocams2isis
   *            - 20190223T023051S790_map_iofL2pan.cub
   *         2) campt
   *            - 20190223T023051S790_map_iofL2pan.cub_campt.pvl
   */
  TEST(OsirisRexMapCamModules, MapCamModuleCamptTest) {
  QTemporaryDir tempDir;

  // ingest mapcam fits format image with ocams2isis and then spiceinit
  QString mapCamFileName = tempDir.path() + "/20190223T023051S790_map_iofL2pan.cub";
  QVector<QString> ocams2isisArgs = {"from=data/osirisRexImages/ocams/20190223T023051S790_map_iofL2pan.fits",
                                     "to=" + mapCamFileName};

  UserInterface ocams2isisUI(OCAMS2ISIS_XML, ocams2isisArgs);
  try {
    ocams2isis(ocams2isisUI);
  }
  catch (IException &e) {
    FAIL() << "Unable to run ocams2isis on mapcam image: " << e.what() << std::endl;
  }

  QVector<QString> spiceinitArgs = {"from=" + mapCamFileName};
  UserInterface spiceinitUI(SPICEINIT_XML, spiceinitArgs);
  try {
    spiceinit(spiceinitUI);
  }
  catch (IException &e) {
    FAIL() << "Unable to run spiceinit on mapcam image: " << e.what() << std::endl;
  }

  // create flatfile with corner and center coordinates
  ofstream of1;
  of1.open(tempDir.path().toStdString() + "/campt_input_coords.lis");
  of1 << "1,1\n1024,1\n1024,1024\n1,1024\n512,512";
  of1.close();

  Pvl appLog;

  // campt all image coordinates in campt_input_coords.lis file
  QVector<QString> camptArgs = {"from=" + mapCamFileName,
                                "to=" + tempDir.path() + "/20190223T023051S790_map_iofL2pan.cub_campt.pvl",
                                "USECOORDLIST=TRUE",
                                "COORDLIST=" + tempDir.path() + "/campt_input_coords.lis",
                                "COORDTYPE=IMAGE",
                                "ALLOWOUTSIDE=no"};
  UserInterface camptULUI(CAMPT_XML, camptArgs);
  try {
    campt(camptULUI, &appLog);
  }
  catch (IException &e) {
    FAIL() << "Unable to run campt on mapcam image: " << e.what() << std::endl;
  }

  // read campt output pvl file
  Pvl camptPvl;
  camptPvl.read(tempDir.path() + "/20190223T023051S790_map_iofL2pan.cub_campt.pvl");
  
  // validate upper left corner
  // GroundPoint Group
  PvlGroup &gpULGroup = camptPvl.findGroup("GroundPoint", Pvl::Traverse);
  EXPECT_DOUBLE_EQ(double(gpULGroup["Sample"]), 1.0);
  EXPECT_DOUBLE_EQ(double(gpULGroup["Line"]), 1.0);
  EXPECT_NEAR(double(gpULGroup["PixelValue"]), 5.7755544e-05, 1e-8);
  EXPECT_NEAR((double)gpULGroup.findKeyword("RightAscension"), 268.1874111106, 1e-8);
  EXPECT_NEAR((double)gpULGroup.findKeyword("Declination"), 53.928797788084, 1e-8);
  EXPECT_NEAR((double)gpULGroup.findKeyword("PlanetocentricLatitude"), 72.941762041119, 1e-8);
  EXPECT_NEAR((double)gpULGroup.findKeyword("PlanetographicLatitude"), 76.568833752848, 1e-8);  
  EXPECT_NEAR((double)gpULGroup.findKeyword("PositiveEast360Longitude"), 307.46846971323, 1e-8);
  EXPECT_NEAR((double)gpULGroup.findKeyword("PositiveEast180Longitude"), -52.531530286771, 1e-8);
  EXPECT_NEAR((double)gpULGroup.findKeyword("PositiveWest360Longitude"), 52.531530286771, 1e-8);
  EXPECT_NEAR((double)gpULGroup.findKeyword("PositiveWest180Longitude"), 52.531530286771, 1e-8);
  EXPECT_NEAR(toDouble(gpULGroup.findKeyword("BodyFixedCoordinate")[0]), 0.044907216488693, 1e-8);
  EXPECT_NEAR(toDouble(gpULGroup.findKeyword("BodyFixedCoordinate")[1]), -0.058590956666058, 1e-8);
  EXPECT_NEAR(toDouble(gpULGroup.findKeyword("BodyFixedCoordinate")[2]),  0.24058312817658, 1e-8);
  EXPECT_NEAR((double)gpULGroup.findKeyword("LocalRadius"), 251.65412744287, 1e-8);
  EXPECT_NEAR((double)gpULGroup.findKeyword("SampleResolution"), 0.12165674806909, 1e-8);
  EXPECT_NEAR((double)gpULGroup.findKeyword("LineResolution"), 0.12165674806909, 1e-8);
  EXPECT_NEAR((double)gpULGroup.findKeyword("ObliqueDetectorResolution"), 0.12966687426772, 1e-8);
  EXPECT_NEAR((double)gpULGroup.findKeyword("ObliquePixelResolution"), 0.12966687426772, 1e-8);
  EXPECT_NEAR((double)gpULGroup.findKeyword("ObliqueLineResolution"), 0.12966687426772, 1e-8);
  EXPECT_NEAR((double)gpULGroup.findKeyword("ObliqueSampleResolution"), 0.12966687426772, 1e-8);
  EXPECT_NEAR(toDouble(gpULGroup.findKeyword("SpacecraftPosition")[0]), 0.018159770661816, 1e-8);
  EXPECT_NEAR(toDouble(gpULGroup.findKeyword("SpacecraftPosition")[1]), 0.14570220629656, 1e-8);
  EXPECT_NEAR(toDouble(gpULGroup.findKeyword("SpacecraftPosition")[2]),  2.0206309858084, 1e-8);
  EXPECT_NEAR((double)gpULGroup.findKeyword("SpacecraftAzimuth"), 312.4941660997, 1e-8);
  EXPECT_NEAR((double)gpULGroup.findKeyword("SlantDistance"), 1.7919323362647, 1e-8);
  EXPECT_NEAR((double)gpULGroup.findKeyword("TargetCenterDistance"), 2.0259586597458, 1e-8);
  EXPECT_NEAR((double)gpULGroup.findKeyword("SubSpacecraftLatitude"), 85.843896278413, 1e-8);
  EXPECT_NEAR((double)gpULGroup.findKeyword("SubSpacecraftLongitude"), 82.895507575855, 1e-8);
  EXPECT_NEAR((double)gpULGroup.findKeyword("SpacecraftAltitude"), 1.7761380668912, 1e-8);
  EXPECT_NEAR((double)gpULGroup.findKeyword("OffNadirAngle"), 0.020399156170924, 1e-8);
  EXPECT_NEAR((double)gpULGroup.findKeyword("SubSpacecraftGroundAzimuth"), 8.4609884890703, 1e-8);
  EXPECT_NEAR(toDouble(gpULGroup.findKeyword("SunPosition")[0]), -33064987.253898, 1e-8);
  EXPECT_NEAR(toDouble(gpULGroup.findKeyword("SunPosition")[1]), -139793248.81236, 1e-8);
  EXPECT_NEAR(toDouble(gpULGroup.findKeyword("SunPosition")[2]),  2976809.9262304, 1e-8);
  EXPECT_NEAR((double)gpULGroup.findKeyword("SubSolarAzimuth"), 86.133151793804, 1e-8);
  EXPECT_NEAR((double)gpULGroup.findKeyword("SolarDistance"), 0.96044996324002, 1e-8);
  EXPECT_NEAR((double)gpULGroup.findKeyword("SubSolarLatitude"), 1.1871474270835, 1e-8);
  EXPECT_NEAR((double)gpULGroup.findKeyword("SubSolarLongitude"), 256.69252270405, 1e-8);
  EXPECT_NEAR((double)gpULGroup.findKeyword("SubSolarGroundAzimuth"), 232.31312590435, 1e-8);
  EXPECT_NEAR((double)gpULGroup.findKeyword("Phase"), 94.985658717673, 1e-8);
  EXPECT_NEAR((double)gpULGroup.findKeyword("Incidence"), 79.488071244421, 1e-8);
  EXPECT_NEAR((double)gpULGroup.findKeyword("Emission"), 20.244369144135, 1e-8);
  EXPECT_NEAR((double)gpULGroup.findKeyword("NorthAzimuth"), 321.71460924195, 1e-8);
  EXPECT_NEAR((double)gpULGroup.findKeyword("EphemerisTime"), 604161120.95081, 1e-8);
  EXPECT_EQ(gpULGroup["UTC"][0].toStdString(), "2019-02-23T02:30:51.765541");
  EXPECT_NEAR((double)gpULGroup.findKeyword("LocalSolarTime"), 15.385063133946, 1e-8);
  EXPECT_NEAR((double)gpULGroup.findKeyword("SolarLongitude"), 28.894668237098, 1e-8);

  // Look Direction Unit Vectors in Body Fixed, J2000, and Camera Coordinate Systems.
  EXPECT_NEAR(toDouble(gpULGroup.findKeyword("LookDirectionBodyFixed")[0]), 0.014926593647298, 1e-8);
  EXPECT_NEAR(toDouble(gpULGroup.findKeyword("LookDirectionBodyFixed")[1]), -0.11400718588988, 1e-8);
  EXPECT_NEAR(toDouble(gpULGroup.findKeyword("LookDirectionBodyFixed")[2]),  -0.99336778605286, 1e-8);

  EXPECT_NEAR(toDouble(gpULGroup.findKeyword("LookDirectionJ2000")[0]), -0.018623649570398, 1e-8);
  EXPECT_NEAR(toDouble(gpULGroup.findKeyword("LookDirectionJ2000")[1]), -0.58849556398131, 1e-8);
  EXPECT_NEAR(toDouble(gpULGroup.findKeyword("LookDirectionJ2000")[2]),  0.80828592147272, 1e-8);

  EXPECT_NEAR(toDouble(gpULGroup.findKeyword("LookDirectionCamera")[0]), -0.034670549658409, 1e-8);
  EXPECT_NEAR(toDouble(gpULGroup.findKeyword("LookDirectionCamera")[1]), -0.03466000039996, 1e-8);
  EXPECT_NEAR(toDouble(gpULGroup.findKeyword("LookDirectionCamera")[2]),  0.99879759579139, 1e-8);

  // remove upper left corner pvl group
  camptPvl.deleteGroup("GroundPoint");

  // validate upper right pixel
  // GroundPoint Group
  PvlGroup &gpURGroup = camptPvl.findGroup("GroundPoint", Pvl::Traverse);
  EXPECT_DOUBLE_EQ(double(gpURGroup["Sample"]), 1024.0);
  EXPECT_DOUBLE_EQ(double(gpURGroup["Line"]), 1.0);
  EXPECT_NEAR(double(gpURGroup["PixelValue"]), 0.00051495421, 1e-8);
  EXPECT_NEAR((double)gpURGroup.findKeyword("RightAscension"), 274.17773536265, 1e-8);
  EXPECT_NEAR((double)gpURGroup.findKeyword("Declination"), 55.905329569345, 1e-8);
  EXPECT_NEAR((double)gpURGroup.findKeyword("PlanetocentricLatitude"), 71.07703430006, 1e-8);
  EXPECT_NEAR((double)gpURGroup.findKeyword("PlanetographicLatitude"), 75.060786884513, 1e-8);  
  EXPECT_NEAR((double)gpURGroup.findKeyword("PositiveEast360Longitude"), 201.90727118408, 1e-8);
  EXPECT_NEAR((double)gpURGroup.findKeyword("PositiveEast180Longitude"), -158.09272881592, 1e-8);
  EXPECT_NEAR((double)gpURGroup.findKeyword("PositiveWest360Longitude"), 158.09272881592, 1e-8);
  EXPECT_NEAR((double)gpURGroup.findKeyword("PositiveWest180Longitude"), 158.09272881592, 1e-8);
  EXPECT_NEAR(toDouble(gpURGroup.findKeyword("BodyFixedCoordinate")[0]), -0.075987521229325, 1e-8);
  EXPECT_NEAR(toDouble(gpURGroup.findKeyword("BodyFixedCoordinate")[1]), -0.03055798990126, 1e-8);
  EXPECT_NEAR(toDouble(gpURGroup.findKeyword("BodyFixedCoordinate")[2]),  0.23890287450997, 1e-8);
  EXPECT_NEAR((double)gpURGroup.findKeyword("LocalRadius"), 252.55193045888, 1e-8);
  EXPECT_NEAR((double)gpURGroup.findKeyword("SampleResolution"), 0.12172236877096, 1e-8);
  EXPECT_NEAR((double)gpURGroup.findKeyword("LineResolution"), 0.12172236877096, 1e-8);
  EXPECT_NEAR((double)gpURGroup.findKeyword("ObliqueDetectorResolution"), 0.12997413460892, 1e-8);
  EXPECT_NEAR((double)gpURGroup.findKeyword("ObliquePixelResolution"), 0.12997413460892, 1e-8);
  EXPECT_NEAR((double)gpURGroup.findKeyword("ObliqueLineResolution"), 0.12997413460892, 1e-8);
  EXPECT_NEAR((double)gpURGroup.findKeyword("ObliqueSampleResolution"), 0.12997413460892, 1e-8);
  EXPECT_NEAR(toDouble(gpURGroup.findKeyword("SpacecraftPosition")[0]), 0.018159770661816, 1e-8);
  EXPECT_NEAR(toDouble(gpURGroup.findKeyword("SpacecraftPosition")[1]), 0.14570220629656, 1e-8);
  EXPECT_NEAR(toDouble(gpURGroup.findKeyword("SpacecraftPosition")[2]),  2.0206309858084, 1e-8);
  EXPECT_NEAR((double)gpURGroup.findKeyword("SpacecraftAzimuth"), 224.53326409564, 1e-8);
  EXPECT_NEAR((double)gpURGroup.findKeyword("SlantDistance"), 1.7928988906029, 1e-8);
  EXPECT_NEAR((double)gpURGroup.findKeyword("TargetCenterDistance"), 2.0259586597458, 1e-8);
  EXPECT_NEAR((double)gpURGroup.findKeyword("SubSpacecraftLatitude"), 85.843896278413, 1e-8);
  EXPECT_NEAR((double)gpURGroup.findKeyword("SubSpacecraftLongitude"), 82.895507575855, 1e-8);
  EXPECT_NEAR((double)gpURGroup.findKeyword("SpacecraftAltitude"), 1.7761380668912, 1e-8);
  EXPECT_NEAR((double)gpURGroup.findKeyword("OffNadirAngle"), -0.71416473325615, 1e-8);
  EXPECT_NEAR((double)gpURGroup.findKeyword("SubSpacecraftGroundAzimuth"), 349.92439869252, 1e-8);
  EXPECT_NEAR(toDouble(gpURGroup.findKeyword("SunPosition")[0]), -33064987.253898, 1e-8);
  EXPECT_NEAR(toDouble(gpURGroup.findKeyword("SunPosition")[1]), -139793248.81236, 1e-8);
  EXPECT_NEAR(toDouble(gpURGroup.findKeyword("SunPosition")[2]),  2976809.9262304, 1e-8);
  EXPECT_NEAR((double)gpURGroup.findKeyword("SubSolarAzimuth"), 93.836191513294, 1e-8);
  EXPECT_NEAR((double)gpURGroup.findKeyword("SolarDistance"), 0.96044996323659, 1e-8);
  EXPECT_NEAR((double)gpURGroup.findKeyword("SubSolarLatitude"), 1.1871474270835, 1e-8);
  EXPECT_NEAR((double)gpURGroup.findKeyword("SubSolarLongitude"), 256.69252270405, 1e-8);
  EXPECT_NEAR((double)gpURGroup.findKeyword("SubSolarGroundAzimuth"), 123.40234842066, 1e-8);
  EXPECT_NEAR((double)gpURGroup.findKeyword("Phase"), 94.999387735731, 1e-8);
  EXPECT_NEAR((double)gpURGroup.findKeyword("Incidence"), 79.813971733666, 1e-8);
  EXPECT_NEAR((double)gpURGroup.findKeyword("Emission"), 20.526151541045, 1e-8);
  EXPECT_NEAR((double)gpURGroup.findKeyword("NorthAzimuth"), 213.46350578275, 1e-8);
  EXPECT_NEAR((double)gpURGroup.findKeyword("EphemerisTime"), 604161120.95081, 1e-8);
  EXPECT_EQ(gpURGroup["UTC"][0].toStdString(), "2019-02-23T02:30:51.765541");
  EXPECT_NEAR((double)gpURGroup.findKeyword("LocalSolarTime"), 8.3476498986687, 1e-8);
  EXPECT_NEAR((double)gpURGroup.findKeyword("SolarLongitude"), 28.894668237098, 1e-8);

  // Look Direction Unit Vectors in Body Fixed, J2000, and Camera Coordinate Systems.
  EXPECT_NEAR(toDouble(gpURGroup.findKeyword("LookDirectionBodyFixed")[0]), -0.052511210969338, 1e-8);
  EXPECT_NEAR(toDouble(gpURGroup.findKeyword("LookDirectionBodyFixed")[1]), -0.098310170819812, 1e-8);
  EXPECT_NEAR(toDouble(gpURGroup.findKeyword("LookDirectionBodyFixed")[2]),  -0.9937694315262, 1e-8);

  EXPECT_NEAR(toDouble(gpURGroup.findKeyword("LookDirectionJ2000")[0]), 0.04083730028288, 1e-8);
  EXPECT_NEAR(toDouble(gpURGroup.findKeyword("LookDirectionJ2000")[1]), -0.55907247647769, 1e-8);
  EXPECT_NEAR(toDouble(gpURGroup.findKeyword("LookDirectionJ2000")[2]),  0.82811248085674, 1e-8);

  EXPECT_NEAR(toDouble(gpURGroup.findKeyword("LookDirectionCamera")[0]), -0.034665354946352, 1e-8);
  EXPECT_NEAR(toDouble(gpURGroup.findKeyword("LookDirectionCamera")[1]), 0.034581714319912, 1e-8);
  EXPECT_NEAR(toDouble(gpURGroup.findKeyword("LookDirectionCamera")[2]),  0.99880048968808, 1e-8);

  // remove upper right corner pvl group
  camptPvl.deleteGroup("GroundPoint");

  // validate lower right pixel
  // GroundPoint Group
  PvlGroup &gpLRGroup = camptPvl.findGroup("GroundPoint", Pvl::Traverse);
  EXPECT_DOUBLE_EQ(double(gpLRGroup["Sample"]), 1024.0);
  EXPECT_DOUBLE_EQ(double(gpLRGroup["Line"]), 1024.0);
  EXPECT_NEAR(double(gpLRGroup["PixelValue"]), 0.00092357588999999996, 1e-8);
  EXPECT_NEAR((double)gpLRGroup.findKeyword("RightAscension"), 277.16907447401, 1e-8);
  EXPECT_NEAR((double)gpLRGroup.findKeyword("Declination"), 52.345687021406, 1e-8);
  EXPECT_NEAR((double)gpLRGroup.findKeyword("PlanetocentricLatitude"), 41.357364496984, 1e-8);
  EXPECT_NEAR((double)gpLRGroup.findKeyword("PlanetographicLatitude"), 48.519858470138, 1e-8);  
  EXPECT_NEAR((double)gpLRGroup.findKeyword("PositiveEast360Longitude"), 236.24864545458, 1e-8);
  EXPECT_NEAR((double)gpLRGroup.findKeyword("PositiveEast180Longitude"), -123.75135454542, 1e-8);
  EXPECT_NEAR((double)gpLRGroup.findKeyword("PositiveWest360Longitude"), 123.75135454542, 1e-8);
  EXPECT_NEAR((double)gpLRGroup.findKeyword("PositiveWest180Longitude"), 123.75135454542, 1e-8);
  EXPECT_NEAR(toDouble(gpLRGroup.findKeyword("BodyFixedCoordinate")[0]), -0.10964251739938, 1e-8);
  EXPECT_NEAR(toDouble(gpLRGroup.findKeyword("BodyFixedCoordinate")[1]), -0.16408322574503, 1e-8);
  EXPECT_NEAR(toDouble(gpLRGroup.findKeyword("BodyFixedCoordinate")[2]),  0.17372161693009, 1e-8);
  EXPECT_NEAR((double)gpLRGroup.findKeyword("LocalRadius"), 262.91440961152, 1e-8);
  EXPECT_NEAR((double)gpLRGroup.findKeyword("SampleResolution"), 0.12743655124866, 1e-8);
  EXPECT_NEAR((double)gpLRGroup.findKeyword("LineResolution"), 0.12743655124866, 1e-8);
  EXPECT_NEAR((double)gpLRGroup.findKeyword("ObliqueDetectorResolution"), 0.21366000021655, 1e-8);
  EXPECT_NEAR((double)gpLRGroup.findKeyword("ObliquePixelResolution"), 0.21366000021655, 1e-8);
  EXPECT_NEAR((double)gpLRGroup.findKeyword("ObliqueLineResolution"), 0.21366000021655, 1e-8);
  EXPECT_NEAR((double)gpLRGroup.findKeyword("ObliqueSampleResolution"), 0.21366000021655, 1e-8);
  EXPECT_NEAR(toDouble(gpLRGroup.findKeyword("SpacecraftPosition")[0]), 0.018159770661816, 1e-8);
  EXPECT_NEAR(toDouble(gpLRGroup.findKeyword("SpacecraftPosition")[1]), 0.14570220629656, 1e-8);
  EXPECT_NEAR(toDouble(gpLRGroup.findKeyword("SpacecraftPosition")[2]), 2.0206309858084, 1e-8);
  EXPECT_NEAR((double)gpLRGroup.findKeyword("SpacecraftAzimuth"), 250.7975078041, 1e-8);
  EXPECT_NEAR((double)gpLRGroup.findKeyword("SlantDistance"), 1.8770654372156, 1e-8);
  EXPECT_NEAR((double)gpLRGroup.findKeyword("TargetCenterDistance"), 2.0259586597458, 1e-8);
  EXPECT_NEAR((double)gpLRGroup.findKeyword("SubSpacecraftLatitude"), 85.843896278413, 1e-8);
  EXPECT_NEAR((double)gpLRGroup.findKeyword("SubSpacecraftLongitude"), 82.895507575855, 1e-8);
  EXPECT_NEAR((double)gpLRGroup.findKeyword("SpacecraftAltitude"), 1.7761380668912, 1e-8);
  EXPECT_NEAR((double)gpLRGroup.findKeyword("OffNadirAngle"), 1.0022851768984, 1e-8);
  EXPECT_NEAR((double)gpLRGroup.findKeyword("SubSpacecraftGroundAzimuth"), 357.64819254932, 1e-8);
  EXPECT_NEAR(toDouble(gpLRGroup.findKeyword("SunPosition")[0]), -33064987.253898, 1e-8);
  EXPECT_NEAR(toDouble(gpLRGroup.findKeyword("SunPosition")[1]), -139793248.81236, 1e-8);
  EXPECT_NEAR(toDouble(gpLRGroup.findKeyword("SunPosition")[2]), 2976809.9262304, 1e-8);
  EXPECT_NEAR((double)gpLRGroup.findKeyword("SubSolarAzimuth"), 116.1714237726, 1e-8);
  EXPECT_NEAR((double)gpLRGroup.findKeyword("SolarDistance"), 0.96044996232544, 1e-8);
  EXPECT_NEAR((double)gpLRGroup.findKeyword("SubSolarLatitude"), 1.1871474270835, 1e-8);
  EXPECT_NEAR((double)gpLRGroup.findKeyword("SubSolarLongitude"), 256.69252270405, 1e-8);
  EXPECT_NEAR((double)gpLRGroup.findKeyword("SubSolarGroundAzimuth"), 149.94223339606, 1e-8);
  EXPECT_NEAR((double)gpLRGroup.findKeyword("Phase"), 98.966351307622, 1e-8);
  EXPECT_NEAR((double)gpLRGroup.findKeyword("Incidence"), 48.293759898172, 1e-8);
  EXPECT_NEAR((double)gpLRGroup.findKeyword("Emission"), 53.384250874811, 1e-8);
  EXPECT_NEAR((double)gpLRGroup.findKeyword("NorthAzimuth"), 246.29103083168, 1e-8);
  EXPECT_NEAR((double)gpLRGroup.findKeyword("EphemerisTime"), 604161120.95081, 1e-8);
  EXPECT_EQ(gpLRGroup["UTC"][0].toStdString(), "2019-02-23T02:30:51.765541");
  EXPECT_NEAR((double)gpLRGroup.findKeyword("LocalSolarTime"), 10.637074850036, 1e-8);
  EXPECT_NEAR((double)gpLRGroup.findKeyword("SolarLongitude"), 28.894668237098, 1e-8);

  // Look Direction Unit Vectors in Body Fixed, J2000, and Camera Coordinate Systems.
  EXPECT_NEAR(toDouble(gpLRGroup.findKeyword("LookDirectionBodyFixed")[0]), -0.068086218800543, 1e-8);
  EXPECT_NEAR(toDouble(gpLRGroup.findKeyword("LookDirectionBodyFixed")[1]), -0.16503709774824, 1e-8);
  EXPECT_NEAR(toDouble(gpLRGroup.findKeyword("LookDirectionBodyFixed")[2]), -0.98393446081346, 1e-8);

  EXPECT_NEAR(toDouble(gpLRGroup.findKeyword("LookDirectionJ2000")[0]), 0.076238419092995, 1e-8);
  EXPECT_NEAR(toDouble(gpLRGroup.findKeyword("LookDirectionJ2000")[1]), -0.6061200741886, 1e-8);
  EXPECT_NEAR(toDouble(gpLRGroup.findKeyword("LookDirectionJ2000")[2]), 0.79171090627817, 1e-8);

  EXPECT_NEAR(toDouble(gpLRGroup.findKeyword("LookDirectionCamera")[0]), 0.034557400552829, 1e-8);
  EXPECT_NEAR(toDouble(gpLRGroup.findKeyword("LookDirectionCamera")[1]), 0.034561798303565, 1e-8);
  EXPECT_NEAR(toDouble(gpLRGroup.findKeyword("LookDirectionCamera")[2]), 0.99880491997439, 1e-8);

  // remove lower right corner pvl group
  camptPvl.deleteGroup("GroundPoint");

  // validate lower left pixel
  // GroundPoint Group
  PvlGroup &gpLLGroup = camptPvl.findGroup("GroundPoint", Pvl::Traverse);
  EXPECT_DOUBLE_EQ(double(gpLLGroup["Sample"]), 1.0);
  EXPECT_DOUBLE_EQ(double(gpLLGroup["Line"]), 1024.0);
  EXPECT_NEAR(double(gpLLGroup["PixelValue"]), 0.0026517250000000002, 1e-8);
  EXPECT_NEAR((double)gpLLGroup.findKeyword("RightAscension"), 271.51576527218, 1e-8);
  EXPECT_NEAR((double)gpLLGroup.findKeyword("Declination"), 50.523945912677, 1e-8);
  EXPECT_NEAR((double)gpLLGroup.findKeyword("PlanetocentricLatitude"), 41.884620480871, 1e-8);
  EXPECT_NEAR((double)gpLLGroup.findKeyword("PlanetographicLatitude"), 49.04618089514, 1e-8);  
  EXPECT_NEAR((double)gpLLGroup.findKeyword("PositiveEast360Longitude"), 274.97895219323, 1e-8);
  EXPECT_NEAR((double)gpLLGroup.findKeyword("PositiveEast180Longitude"), -85.02104780677, 1e-8);
  EXPECT_NEAR((double)gpLLGroup.findKeyword("PositiveWest360Longitude"), 85.02104780677, 1e-8);
  EXPECT_NEAR((double)gpLLGroup.findKeyword("PositiveWest180Longitude"), 85.02104780677, 1e-8);
  EXPECT_NEAR(toDouble(gpLLGroup.findKeyword("BodyFixedCoordinate")[0]), 0.01686518741068, 1e-8);
  EXPECT_NEAR(toDouble(gpLLGroup.findKeyword("BodyFixedCoordinate")[1]), -0.19358902488031, 1e-8);
  EXPECT_NEAR(toDouble(gpLLGroup.findKeyword("BodyFixedCoordinate")[2]), 0.17426127205628, 1e-8);
  EXPECT_NEAR((double)gpLLGroup.findKeyword("LocalRadius"), 261.01367021514, 1e-8);
  EXPECT_NEAR((double)gpLLGroup.findKeyword("SampleResolution"), 0.1274515004168, 1e-8);
  EXPECT_NEAR((double)gpLLGroup.findKeyword("LineResolution"), 0.1274515004168, 1e-8);
  EXPECT_NEAR((double)gpLLGroup.findKeyword("ObliqueDetectorResolution"), 0.21557326777525, 1e-8);
  EXPECT_NEAR((double)gpLLGroup.findKeyword("ObliquePixelResolution"), 0.21557326777525, 1e-8);
  EXPECT_NEAR((double)gpLLGroup.findKeyword("ObliqueLineResolution"), 0.21557326777525, 1e-8);
  EXPECT_NEAR((double)gpLLGroup.findKeyword("ObliqueSampleResolution"), 0.21557326777525, 1e-8);
  EXPECT_NEAR(toDouble(gpLLGroup.findKeyword("SpacecraftPosition")[0]), 0.018159770661816, 1e-8);
  EXPECT_NEAR(toDouble(gpLLGroup.findKeyword("SpacecraftPosition")[1]), 0.14570220629656, 1e-8);
  EXPECT_NEAR(toDouble(gpLLGroup.findKeyword("SpacecraftPosition")[2]), 2.0206309858084, 1e-8);
  EXPECT_NEAR((double)gpLLGroup.findKeyword("SpacecraftAzimuth"), 287.43614020389, 1e-8);
  EXPECT_NEAR((double)gpLLGroup.findKeyword("SlantDistance"), 1.8772856296686, 1e-8);
  EXPECT_NEAR((double)gpLLGroup.findKeyword("TargetCenterDistance"), 2.0259586597458, 1e-8);
  EXPECT_NEAR((double)gpLLGroup.findKeyword("SubSpacecraftLatitude"), 85.843896278413, 1e-8);
  EXPECT_NEAR((double)gpLLGroup.findKeyword("SubSpacecraftLongitude"), 82.895507575855, 1e-8);
  EXPECT_NEAR((double)gpLLGroup.findKeyword("SpacecraftAltitude"), 1.7761380668912, 1e-8);
  EXPECT_NEAR((double)gpLLGroup.findKeyword("OffNadirAngle"), 1.5714486868052, 1e-8);
  EXPECT_NEAR((double)gpLLGroup.findKeyword("SubSpacecraftGroundAzimuth"), 1.1004062504143, 1e-8);
  EXPECT_NEAR(toDouble(gpLLGroup.findKeyword("SunPosition")[0]), -33064987.253898, 1e-8);
  EXPECT_NEAR(toDouble(gpLLGroup.findKeyword("SunPosition")[1]), -139793248.81236, 1e-8);
  EXPECT_NEAR(toDouble(gpLLGroup.findKeyword("SunPosition")[2]), 2976809.9262304, 1e-8);
  EXPECT_NEAR((double)gpLLGroup.findKeyword("SubSolarAzimuth"), 64.657513190501, 1e-8);
  EXPECT_NEAR((double)gpLLGroup.findKeyword("SolarDistance"), 0.96044996232808, 1e-8);
  EXPECT_NEAR((double)gpLLGroup.findKeyword("SubSolarLatitude"), 1.1871474270835, 1e-8);
  EXPECT_NEAR((double)gpLLGroup.findKeyword("SubSolarLongitude"), 256.69252270405, 1e-8);
  EXPECT_NEAR((double)gpLLGroup.findKeyword("SubSolarGroundAzimuth"), 206.8992633762, 1e-8);
  EXPECT_NEAR((double)gpLLGroup.findKeyword("Phase"), 98.953107167776, 1e-8);
  EXPECT_NEAR((double)gpLLGroup.findKeyword("Incidence"), 48.047622408132, 1e-8);
  EXPECT_NEAR((double)gpLLGroup.findKeyword("Emission"), 53.756279041623, 1e-8);
  EXPECT_NEAR((double)gpLLGroup.findKeyword("NorthAzimuth"), 289.53233199269, 1e-8);
  EXPECT_NEAR((double)gpLLGroup.findKeyword("EphemerisTime"), 604161120.95081, 1e-8);
  EXPECT_EQ(gpLLGroup["UTC"][0].toStdString(), "2019-02-23T02:30:51.765541");
  EXPECT_NEAR((double)gpLLGroup.findKeyword("LocalSolarTime"), 13.219095299279, 1e-8);
  EXPECT_NEAR((double)gpLLGroup.findKeyword("SolarLongitude"), 28.894668237098, 1e-8);

  // Look Direction Unit Vectors in Body Fixed, J2000, and Camera Coordinate Systems.
  EXPECT_NEAR(toDouble(gpLLGroup.findKeyword("LookDirectionBodyFixed")[0]), -6.89603771890868e-04, 1e-8);
  EXPECT_NEAR(toDouble(gpLLGroup.findKeyword("LookDirectionBodyFixed")[1]), -0.18073500687094, 1e-8);
  EXPECT_NEAR(toDouble(gpLLGroup.findKeyword("LookDirectionBodyFixed")[2]), -0.98353158654819, 1e-8);

  EXPECT_NEAR(toDouble(gpLLGroup.findKeyword("LookDirectionJ2000")[0]), 0.016817014814507, 1e-8);
  EXPECT_NEAR(toDouble(gpLLGroup.findKeyword("LookDirectionJ2000")[1]), -0.63553321496645, 1e-8);
  EXPECT_NEAR(toDouble(gpLLGroup.findKeyword("LookDirectionJ2000")[2]), 0.7718903553531, 1e-8);

  EXPECT_NEAR(toDouble(gpLLGroup.findKeyword("LookDirectionCamera")[0]), 0.03456252131293, 1e-8);
  EXPECT_NEAR(toDouble(gpLLGroup.findKeyword("LookDirectionCamera")[1]), -0.034640010496952, 1e-8);
  EXPECT_NEAR(toDouble(gpLLGroup.findKeyword("LookDirectionCamera")[2]), 0.99880203333457, 1e-8);

  // remove lower left corner pvl group
  camptPvl.deleteGroup("GroundPoint");

  // validate center pixel
  // GroundPoint Group
  PvlGroup &gpCTRGroup = camptPvl.findGroup("GroundPoint", Pvl::Traverse);
  EXPECT_DOUBLE_EQ(double(gpCTRGroup["Sample"]), 512.0);
  EXPECT_DOUBLE_EQ(double(gpCTRGroup["Line"]), 512.0);
  EXPECT_NEAR(double(gpCTRGroup["PixelValue"]), 0.0082613862999999996, 1e-8);
  EXPECT_NEAR((double)gpCTRGroup.findKeyword("RightAscension"), 272.75907287018, 1e-8);
  EXPECT_NEAR((double)gpCTRGroup.findKeyword("Declination"), 53.221788172495, 1e-8);
  EXPECT_NEAR((double)gpCTRGroup.findKeyword("PlanetocentricLatitude"), 63.939693821928, 1e-8);
  EXPECT_NEAR((double)gpCTRGroup.findKeyword("PlanetographicLatitude"), 69.162889596587, 1e-8);  
  EXPECT_NEAR((double)gpCTRGroup.findKeyword("PositiveEast360Longitude"), 254.37764186472, 1e-8);
  EXPECT_NEAR((double)gpCTRGroup.findKeyword("PositiveEast180Longitude"), -105.62235813528, 1e-8);
  EXPECT_NEAR((double)gpCTRGroup.findKeyword("PositiveWest360Longitude"), 105.62235813528, 1e-8);
  EXPECT_NEAR((double)gpCTRGroup.findKeyword("PositiveWest180Longitude"), 105.62235813528, 1e-8);
  EXPECT_NEAR(toDouble(gpCTRGroup.findKeyword("BodyFixedCoordinate")[0]), -0.030002238343273, 1e-8);
  EXPECT_NEAR(toDouble(gpCTRGroup.findKeyword("BodyFixedCoordinate")[1]), -0.10729427849513, 1e-8);
  EXPECT_NEAR(toDouble(gpCTRGroup.findKeyword("BodyFixedCoordinate")[2]), 0.22781553175439, 1e-8);
  EXPECT_NEAR((double)gpCTRGroup.findKeyword("LocalRadius"), 253.59833006535, 1e-8);
  EXPECT_NEAR((double)gpCTRGroup.findKeyword("SampleResolution"), 0.12296614216965, 1e-8);
  EXPECT_NEAR((double)gpCTRGroup.findKeyword("LineResolution"), 0.12296614216965, 1e-8);
  EXPECT_NEAR((double)gpCTRGroup.findKeyword("ObliqueDetectorResolution"), 0.14280903817193, 1e-8);
  EXPECT_NEAR((double)gpCTRGroup.findKeyword("ObliquePixelResolution"), 0.14280903817193, 1e-8);
  EXPECT_NEAR((double)gpCTRGroup.findKeyword("ObliqueLineResolution"), 0.14280903817193, 1e-8);
  EXPECT_NEAR((double)gpCTRGroup.findKeyword("ObliqueSampleResolution"), 0.14280903817193, 1e-8);
  EXPECT_NEAR(toDouble(gpCTRGroup.findKeyword("SpacecraftPosition")[0]), 0.018159770661816, 1e-8);
  EXPECT_NEAR(toDouble(gpCTRGroup.findKeyword("SpacecraftPosition")[1]), 0.14570220629656, 1e-8);
  EXPECT_NEAR(toDouble(gpCTRGroup.findKeyword("SpacecraftPosition")[2]), 2.0206309858084, 1e-8);
  EXPECT_NEAR((double)gpCTRGroup.findKeyword("SpacecraftAzimuth"), 268.56958890645, 1e-8);
  EXPECT_NEAR((double)gpCTRGroup.findKeyword("SlantDistance"), 1.8112189411342, 1e-8);
  EXPECT_NEAR((double)gpCTRGroup.findKeyword("TargetCenterDistance"), 2.0259586597458, 1e-8);
  EXPECT_NEAR((double)gpCTRGroup.findKeyword("SubSpacecraftLatitude"), 85.843896278413, 1e-8);
  EXPECT_NEAR((double)gpCTRGroup.findKeyword("SubSpacecraftLongitude"), 82.895507575855, 1e-8);
  EXPECT_NEAR((double)gpCTRGroup.findKeyword("SpacecraftAltitude"), 1.7761380668912, 1e-8);
  EXPECT_NEAR((double)gpCTRGroup.findKeyword("OffNadirAngle"), 0.38858338848542, 1e-8);
  EXPECT_NEAR((double)gpCTRGroup.findKeyword("SubSpacecraftGroundAzimuth"), 358.7763197288, 1e-8);
  EXPECT_NEAR(toDouble(gpCTRGroup.findKeyword("SunPosition")[0]), -33064987.253898, 1e-8);
  EXPECT_NEAR(toDouble(gpCTRGroup.findKeyword("SunPosition")[1]), -139793248.81236, 1e-8);
  EXPECT_NEAR(toDouble(gpCTRGroup.findKeyword("SunPosition")[2]), 2976809.9262304, 1e-8);
  EXPECT_NEAR((double)gpCTRGroup.findKeyword("SubSolarAzimuth"), 90.237294274039, 1e-8);
  EXPECT_NEAR((double)gpCTRGroup.findKeyword("SolarDistance"), 0.9604499628098, 1e-8);
  EXPECT_NEAR((double)gpCTRGroup.findKeyword("SubSolarLatitude"), 1.1871474270835, 1e-8);
  EXPECT_NEAR((double)gpCTRGroup.findKeyword("SubSolarLongitude"), 256.69252270405, 1e-8);
  EXPECT_NEAR((double)gpCTRGroup.findKeyword("SubSolarGroundAzimuth"), 177.39710616737, 1e-8);
  EXPECT_NEAR((double)gpCTRGroup.findKeyword("Phase"), 96.979536059037, 1e-8);
  EXPECT_NEAR((double)gpCTRGroup.findKeyword("Incidence"), 66.414678374557, 1e-8);
  EXPECT_NEAR((double)gpCTRGroup.findKeyword("Emission"), 30.5649867207, 1e-8);
  EXPECT_NEAR((double)gpCTRGroup.findKeyword("NorthAzimuth"), 267.08994573458, 1e-8);
  EXPECT_NEAR((double)gpCTRGroup.findKeyword("EphemerisTime"), 604161120.95081, 1e-8);
  EXPECT_EQ(gpCTRGroup["UTC"][0].toStdString(), "2019-02-23T02:30:51.765541");
  EXPECT_NEAR((double)gpCTRGroup.findKeyword("LocalSolarTime"), 11.845674610712, 1e-8);
  EXPECT_NEAR((double)gpCTRGroup.findKeyword("SolarLongitude"), 28.894668237098, 1e-8);

  // Look Direction Unit Vectors in Body Fixed, J2000, and Camera Coordinate Systems.
  EXPECT_NEAR(toDouble(gpCTRGroup.findKeyword("LookDirectionBodyFixed")[0]), -0.026590937137026, 1e-8);
  EXPECT_NEAR(toDouble(gpCTRGroup.findKeyword("LookDirectionBodyFixed")[1]), -0.13968299416815, 1e-8);
  EXPECT_NEAR(toDouble(gpCTRGroup.findKeyword("LookDirectionBodyFixed")[2]), -0.98983917037183, 1e-8);

  EXPECT_NEAR(toDouble(gpCTRGroup.findKeyword("LookDirectionJ2000")[0]), 0.028820118348176, 1e-8);
  EXPECT_NEAR(toDouble(gpCTRGroup.findKeyword("LookDirectionJ2000")[1]), -0.59802500764249, 1e-8);
  EXPECT_NEAR(toDouble(gpCTRGroup.findKeyword("LookDirectionJ2000")[2]), 0.80095910695403, 1e-8);

  EXPECT_NEAR(toDouble(gpCTRGroup.findKeyword("LookDirectionCamera")[0]), -6.78951614101475e-05, 1e-8);
  EXPECT_NEAR(toDouble(gpCTRGroup.findKeyword("LookDirectionCamera")[1]), -6.7889178477748e-05, 1e-8);
  EXPECT_NEAR(toDouble(gpCTRGroup.findKeyword("LookDirectionCamera")[2]), 0.99999999539065, 1e-8);  
}


/**
   * MapCamModuleTwoImageTest
   * 
   *  1) Two overlapping MapCam images (fits format) are ingested with ocams2isis. Each is
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
   * 11) The test validates output from mosrange, automos, bandtrim, noseam,
   *     overlapstats, autoseed, and pointreg.
   * 
   * INPUT: two mapcam images in fits format
   *          - isis/tests/data/osirisRexImages/ocams/20190223T023051S790_map_iofL2pan.fits
   *          - isis/tests/data/osirisRexImages/ocams/20190223T024051S802_map_iofL2pan.fits
   * 
   * OUTPUT: 1) ocams2isis
   *            - 20190223T023051S790_map_iofL2pan.cub
   *            - 20190223T024051S802_map_iofL2pan.cub
   *         2) mosrange
   *            - mapcam_equi.map
   *         3) cam2map
   *            - 20190223T023051S790_map_iofL2pan_map.cub
   *            - 20190223T024051S802_map_iofL2pan_map.cub
   *         4) automos
   *            - mosaicUncontrolled.cub
   *         5) bandtrim
   *            - mosaicTrimmedUncontrolled.cub
   *         6) noseam
   *            - noseamUncontrolledMosaic.cub
   *         7) findimageoverlaps
   *            - mapcam_findimageoverlaps.txt
   *         8) autoseed
   *            - mapcam_autoseed.net
   *         9) pointreg
   *            - mapcam_pointreg.net
   */
  TEST(OsirisRexMapCamModules, MapCamModuleTwoImageTest) {
  QTemporaryDir tempDir;

  // ingest 1st mapcam fits format image, then spiceinit and footprintinit 
  QString mapCam1FileName = tempDir.path() + "/20190223T023051S790_map_iofL2pan.cub";
  QVector<QString> ocams2isisArgs1 = {"from=data/osirisRexImages/ocams/20190223T023051S790_map_iofL2pan.fits",
                                      "to=" + mapCam1FileName};

  UserInterface ocams2isisMapCam1UI(OCAMS2ISIS_XML, ocams2isisArgs1);
  try {
    ocams2isis(ocams2isisMapCam1UI);
  }
  catch (IException &e) {
    FAIL() << "Unable to run ocams2isis on first mapcam image: " << e.what() << std::endl;
  }

  QVector<QString> spiceinitArgs1 = {"from=" + mapCam1FileName};
  UserInterface spiceinitMapCam1UI(SPICEINIT_XML, spiceinitArgs1);
  try {
    spiceinit(spiceinitMapCam1UI);
  }
  catch (IException &e) {
    FAIL() << "Unable to run spiceinit on first mapcam image: " << e.what() << std::endl;
  }

  QVector<QString> footprintinitArgs1 = {"from=" + mapCam1FileName};
  UserInterface footprintinitMapCam1UI(FOOTPRINTINIT_XML, footprintinitArgs1);
  try {
    footprintinit(footprintinitMapCam1UI);
  }
  catch (IException &e) {
    FAIL() << "Unable to run footprintinit on first mapcam image: " << e.what() << std::endl;
  }

  // ingest 2nd mapcam fits format image, then spiceinit and footprintinit 
  QString mapCam2FileName = tempDir.path() + "/20190223T024051S802_map_iofL2pan.cub";
  QVector<QString> ocams2isisArgs2 = {"from=data/osirisRexImages/ocams/20190223T024051S802_map_iofL2pan.fits",
                                      "to=" + mapCam2FileName};

  UserInterface ocams2isisMapCam2UI(OCAMS2ISIS_XML, ocams2isisArgs2);
  try {
    ocams2isis(ocams2isisMapCam2UI);
  }
  catch (IException &e) {
    FAIL() << "Unable to run ocams2isis on mapcam image 2: " << e.what() << std::endl;
  }

  QVector<QString> spiceinitArgs2 = {"from=" + mapCam2FileName};
  UserInterface spiceinitMapCam2(SPICEINIT_XML, spiceinitArgs2);
  try {
    spiceinit(spiceinitMapCam2);
  }
  catch (IException &e) {
    FAIL() << "Unable to run spiceinit on mapcam image 2: " << e.what() << std::endl;
  }

  // footprintinit second mapcam image
  QVector<QString> footprintinitArgs2 = {"from=" + mapCam2FileName};
  UserInterface footprintinitMapCam2UI(FOOTPRINTINIT_XML, footprintinitArgs2);
  try {
    footprintinit(footprintinitMapCam2UI);
  }
  catch (IException &e) {
    FAIL() << "Unable to run footprintinit on second mapcam image: " << e.what() << std::endl;
  }

  // compute the lat/lon range of the two images for mosaicking with mosrange
  FileList *cubeList = new FileList(); 
  cubeList->append(mapCam1FileName);
  cubeList->append(mapCam2FileName);
  
  QString cubeListFile = tempDir.path() + "/unprojectedSpiceinitCubeList.lis";
  cubeList->write(cubeListFile);

  QString equiMapFile = tempDir.path() + "/mapcam_equi.map";

  QVector<QString> mosrangeArgs = {"fromlist=" + cubeListFile,
                                   "to=" + equiMapFile,
                                   "precision=6",
                                   "projection=Equirectangular"};
  UserInterface mosrangeUI(MOSRANGE_XML, mosrangeArgs);
  try {
    mosrange(mosrangeUI);
  }
  catch (IException &e) {
    FAIL() << "Unable to run mosrange on mapcam images: " << e.what() << std::endl;
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
  EXPECT_NEAR((double)mappingGroup.findKeyword("PixelResolution"), 0.124332, 1e-5);
  EXPECT_NEAR((double)mappingGroup.findKeyword("Scale"), 39.735692, 1e-5);
  EXPECT_NEAR((double)mappingGroup.findKeyword("MinPixelResolution"), 0.1211832000401, 1e-8);
  EXPECT_NEAR((double)mappingGroup.findKeyword("MaxPixelResolution"), 0.12748070662235, 1e-8);
  EXPECT_NEAR((double)mappingGroup.findKeyword("MinObliquePixelResolution"), 0.1254770069828, 1e-8);
  EXPECT_NEAR((double)mappingGroup.findKeyword("MaxObliquePixelResolution"), 0.21574829316587, 1e-8);
  EXPECT_NEAR((double)mappingGroup.findKeyword("CenterLongitude"), 247.088654, 1e-5);
  EXPECT_NEAR((double)mappingGroup.findKeyword("CenterLatitude"), 60.355015, 1e-5);
  EXPECT_NEAR((double)mappingGroup.findKeyword("MinimumLatitude"), 41.328217, 1e-5);
  EXPECT_NEAR((double)mappingGroup.findKeyword("MaximumLatitude"), 79.381813, 1e-5);
  EXPECT_NEAR((double)mappingGroup.findKeyword("MinimumLongitude"), 186.64283, 1e-5);
  EXPECT_NEAR((double)mappingGroup.findKeyword("MaximumLongitude"), 307.534478, 1e-5);
  EXPECT_NEAR((double)mappingGroup.findKeyword("PreciseCenterLongitude"), 247.08865411184, 1e-8);
  EXPECT_NEAR((double)mappingGroup.findKeyword("PreciseCenterLatitude"), 60.35501461099, 1e-8);
  EXPECT_NEAR((double)mappingGroup.findKeyword("PreciseMinimumLatitude"), 41.328217072933, 1e-8);
  EXPECT_NEAR((double)mappingGroup.findKeyword("PreciseMaximumLatitude"), 79.381812149046, 1e-8);
  EXPECT_NEAR((double)mappingGroup.findKeyword("PreciseMinimumLongitude"), 186.64283095778, 1e-8);
  EXPECT_NEAR((double)mappingGroup.findKeyword("PreciseMaximumLongitude"), 307.53447726591, 1e-8);

  // project the two images using the equirectangular map from mosrange
  QVector<QString> cam2mapArgs1 = {"from=" + mapCam1FileName,
                                   "to=" + tempDir.path() + "/20190223T023051S790_map_iofL2pan_map.cub",
                                   "map=" + tempDir.path() + "/mapcam_equi.map",
                                   "pixres=map"};
  UserInterface cam2map1UI(CAM2MAP_XML, cam2mapArgs1);
  try {
    cam2map(cam2map1UI);
  }
  catch (IException &e) {
    FAIL() << "Unable to run cam2map on first mapcam image: " << e.what() << std::endl;
  }

  QVector<QString> cam2mapArgs2 = {"from=" + mapCam2FileName,
                                   "to=" + tempDir.path() + "/20190223T024051S802_map_iofL2pan_map.cub",
                                   "map=" + tempDir.path() + "/mapcam_equi.map",
                                   "pixres=map"};
  UserInterface cam2map2UI(CAM2MAP_XML, cam2mapArgs2);
  try {
    cam2map(cam2map2UI);
  }
  catch (IException &e) {
    FAIL() << "Unable to run cam2map on second mapcam image: " << e.what() << std::endl;
  }

  // create uncontrolled mosaic of the two equirectangular projected mapcam images
  FileList *projectedCubeList = new FileList(); 
  projectedCubeList->append(tempDir.path() + "/20190223T023051S790_map_iofL2pan_map.cub");
  projectedCubeList->append(tempDir.path() + "/20190223T024051S802_map_iofL2pan_map.cub");
  
  QString projectedCubeListFile = tempDir.path() + "/projectedCubeList.lis";
  projectedCubeList->write(projectedCubeListFile);

  QVector<QString> automosArgs = {"fromlist=" + projectedCubeListFile,
                                  "mosaic=" + tempDir.path() + "/mosaicUncontrolled.cub"};
  UserInterface automosUI(AUTOMOS_XML, automosArgs);
  try {
    automos(automosUI);
  }
  catch (IException &e) {
    FAIL() << "Unable to run automos on mapcam images: " << e.what() << std::endl;
  }

  // validate mapping label and histogram of uncontrolled mosaic cube
  Cube mosaic(tempDir.path() + "/mosaicUncontrolled.cub");

  PvlGroup &mosaicMapGroup = (mosaic.label())->findObject("IsisCube").findGroup("Mapping");
  EXPECT_PRED_FORMAT2(AssertQStringsEqual, (QString)mosaicMapGroup.findKeyword("ProjectionName"), "Equirectangular");
  EXPECT_NEAR((double)mosaicMapGroup.findKeyword("CenterLongitude"), 247.088654, 1e-5);
  EXPECT_PRED_FORMAT2(AssertQStringsEqual, (QString)mosaicMapGroup.findKeyword("TargetName"), "Bennu");
  EXPECT_DOUBLE_EQ((double)mosaicMapGroup.findKeyword("EquatorialRadius"), 283.065);
  EXPECT_DOUBLE_EQ((double)mosaicMapGroup.findKeyword("PolarRadius"), 249.72);
  EXPECT_EQ(mosaicMapGroup.findKeyword("LatitudeType")[0], "Planetocentric");
  EXPECT_EQ(mosaicMapGroup.findKeyword("LongitudeDirection")[0], "PositiveEast");
  EXPECT_EQ((int)mosaicMapGroup.findKeyword("LongitudeDomain"), 360);
  EXPECT_NEAR((double)mosaicMapGroup.findKeyword("MinimumLatitude"), 41.328217072933, 1e-8);
  EXPECT_NEAR((double)mosaicMapGroup.findKeyword("MaximumLatitude"), 79.381812149046, 1e-8);
  EXPECT_NEAR((double)mosaicMapGroup.findKeyword("MinimumLongitude"), 186.64283095778, 1e-8);
  EXPECT_NEAR((double)mosaicMapGroup.findKeyword("MaximumLongitude"), 307.53447726591, 1e-8);
  EXPECT_NEAR((double)mosaicMapGroup.findKeyword("UpperLeftCornerX"), -134.029896, 1e-5);
  EXPECT_NEAR((double)mosaicMapGroup.findKeyword("UpperLeftCornerY"), 355.838184, 1e-5);
  EXPECT_NEAR((double)mosaicMapGroup.findKeyword("PixelResolution"), 0.124332, 1e-5);
  EXPECT_NEAR((double)mosaicMapGroup.findKeyword("Scale"), 36.046117980109, 1e-8);
  EXPECT_NEAR((double)mosaicMapGroup.findKeyword("CenterLatitude"), 60.355015, 1e-5);
  EXPECT_NEAR((double)mosaicMapGroup.findKeyword("CenterLatitudeRadius"), 256.7816895054, 1e-8);

  std::unique_ptr<Histogram> mosaicHist(mosaic.histogram());
  EXPECT_NEAR(mosaicHist->Average(), 0.0040495621325216348, 1e-8);
  EXPECT_NEAR(mosaicHist->Sum(), 7338.5881496207794, 1e-8);
  EXPECT_EQ(mosaicHist->ValidPixels(), 1812193);
  EXPECT_NEAR(mosaicHist->StandardDeviation(), 0.0037577166560040049, 1e-8);

  // band trim automos mosaic
  QVector<QString> bandtrimArgs = {"from=" + tempDir.path() + "/mosaicUncontrolled.cub",
                                   "to=" + tempDir.path() + "/mosaicTrimmedUncontrolled.cub"};
  UserInterface bandtrimUI(BANDTRIM_XML, bandtrimArgs);
  try {
    bandtrim(bandtrimUI);
  }
  catch (IException &e) {
    FAIL() << "Unable to run bandtrim on mapcam 2 image mosaic: " << e.what() << std::endl;
  }

  // validate histogram of trimmed, uncontrolled noseam mosaic cube
  Cube trimmedUncontrolledMosaic(tempDir.path() + "/mosaicTrimmedUncontrolled.cub");

  std::unique_ptr<Histogram> trimmedUncontrolledMosaicHist(trimmedUncontrolledMosaic.histogram());
  EXPECT_NEAR(trimmedUncontrolledMosaicHist->Average(), 0.0040495621325216348, 1e-8);
  EXPECT_NEAR(trimmedUncontrolledMosaicHist->Sum(), 7338.5881496207794, 1e-8);
  EXPECT_EQ(trimmedUncontrolledMosaicHist->ValidPixels(), 1812193);
  EXPECT_NEAR(trimmedUncontrolledMosaicHist->StandardDeviation(), 0.0037577166560040049, 1e-8);

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
    FAIL() << "Unable to run noseam on mapcam images: " << e.what() << std::endl;
  }

  // validate histogram of uncontrolled noseam mosaic cube
  Cube noseamMosaic(tempDir.path() + "/noseamUncontrolledMosaic.cub");

  std::unique_ptr<Histogram> noseamMosaicHist(noseamMosaic.histogram());
  EXPECT_NEAR(noseamMosaicHist->Average(), 0.0040496894214538124, 1e-8);
  EXPECT_NEAR(noseamMosaicHist->Sum(), 7338.8188217326478, 1e-8);
  EXPECT_EQ(noseamMosaicHist->ValidPixels(), 1812193);
  EXPECT_NEAR(noseamMosaicHist->StandardDeviation(), 0.0037573821357031172, 1e-8);

  // find overlaps of the two images
  QVector<QString> findimageoverlapsArgs =
      {"fromlist=" + tempDir.path() + "/unprojectedSpiceinitCubeList.lis",
       "overlaplist=" + tempDir.path() + "/mapcam_findimageoverlaps.txt"};
  UserInterface findimageoverlapsUI(FINDIMAGEOVERLAPS_XML, findimageoverlapsArgs);
  try {
    findimageoverlaps(findimageoverlapsUI);
  }
  catch (IException &e) {
    FAIL() << "Unable to run findimageoverlaps on mapcam images: " << e.what() << std::endl;
  }

  Pvl overlapstatsLog;

  // generate overlap stats for the two images
  QVector<QString> overlapstatsArgs =
      {"fromlist=" + tempDir.path() + "/unprojectedSpiceinitCubeList.lis",
       "overlaplist=" + tempDir.path() + "/mapcam_findimageoverlaps.txt",
       "to=" + tempDir.path() + "/mapcam_overlapstats.csv"};
  UserInterface overlapstatsUI(OVERLAPSTATS_XML, overlapstatsArgs);
  try {
    overlapstats(overlapstatsUI, &overlapstatsLog);
  }
  catch (IException &e) {
    FAIL() << "Unable to run overlapstats on mapcam images: " << e.what() << std::endl;
  }

  // validate overlapstatsLog app log file

  PvlGroup &resultsGroup = overlapstatsLog.findGroup("Results");
  EXPECT_NEAR((double)resultsGroup.findKeyword("ThicknessMinimum"), 0.036885955217792, 1e-8);
  EXPECT_NEAR((double)resultsGroup.findKeyword("ThicknessMaximum"), 0.036885955217792, 1e-8);
  EXPECT_NEAR((double)resultsGroup.findKeyword("ThicknessAverage"), 0.036885955217792, 1e-8);
  EXPECT_NEAR((double)resultsGroup.findKeyword("AreaMinimum"), 17210.417187009, 1e-8);
  EXPECT_NEAR((double)resultsGroup.findKeyword("AreaMaximum"), 17210.417187009, 1e-8);
  EXPECT_NEAR((double)resultsGroup.findKeyword("AreaAverage"), 17210.417187009, 1e-8);
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
  of << "    XSpacing = 15\n";
  of << "    YSpacing = 15\n";
  of << "  EndGroup\n";
  of << "EndObject\n";
  of.close();

  // autoseed the two images
  QVector<QString> autoseedArgs =
      {"fromlist=" + tempDir.path() + "/unprojectedSpiceinitCubeList.lis",
       "overlaplist=" + tempDir.path() + "/mapcam_findimageoverlaps.txt",
       "onet=" + tempDir.path() + "/mapcam_autoseed.net",
       "deffile=" + tempDir.path() + "/autoseed.def",
       "networkid=OREx",
       "pointid=Bennu????",
       "description=auto"};
  UserInterface autoseedUI(AUTOSEED_XML, autoseedArgs);
  try {
    autoseed(autoseedUI);
  }
  catch (IException &e) {
    FAIL() << "Unable to run autoseed on mapcam images: " << e.what() << std::endl;
  }

  // validate output control network
  ControlNet autoseedNet(tempDir.path() + "/mapcam_autoseed.net");
  ASSERT_EQ(autoseedNet.GetNumPoints(), 77);
  ASSERT_EQ(autoseedNet.GetNumValidPoints(), 77);
  EXPECT_EQ(autoseedNet.GetNumMeasures(), 154);
  ASSERT_EQ(autoseedNet.GetNumValidMeasures(), 154);
  EXPECT_EQ(autoseedNet.GetNumIgnoredMeasures(), 0);

  // create pointreg.def file
  of.open(tempDir.path().toStdString() + "/pointreg.def");
  of << "Object = AutoRegistration\n";
  of << "  Group = Algorithm\n";
  of << "    Name         = MaximumCorrelation\n";
  of << "    Tolerance    = 0.7\n";
  of << "    # Valid gradient value is Sobel.\n";
  of << "    # Gradient = Sobel\n";
  of << "  EndGroup\n";
  of << "  Group = PatternChip\n";
  of << "    Samples = 25\n";
  of << "    Lines   = 25\n";
  of << "  EndGroup\n";
  of << "  Group = SearchChip\n";
  of << "    Samples = 200\n";
  of << "    Lines   = 200\n";
  of << "  EndGroup\n";
  of << "EndObject\n";
  of.close();

  // register the two images with pointreg
  Pvl pointregLog;
  QVector<QString> pointregArgs =
      {"fromlist=" + tempDir.path() + "/unprojectedSpiceinitCubeList.lis",
       "cnet=" + tempDir.path() + "/mapcam_autoseed.net",
       "deffile=" + tempDir.path() + "/pointreg.def",
       "onet=" + tempDir.path() + "/mapcam_pointreg.net"};
  UserInterface pointregUI(POINTREG_XML, pointregArgs);
  try {
    pointreg(pointregUI, &pointregLog);
  }
  catch (IException &e) {
    FAIL() << "Unable to run pointreg on mapcam images: " << e.what() << std::endl;
  }

  // validate output control network
  ControlNet pointregNet(tempDir.path() + "/mapcam_pointreg.net");
  ASSERT_EQ(pointregNet.GetNumPoints(), 77);
  ASSERT_EQ(pointregNet.GetNumValidPoints(), 74);
  EXPECT_EQ(pointregNet.GetNumMeasures(), 154);
  ASSERT_EQ(pointregNet.GetNumValidMeasures(), 148);
  EXPECT_EQ(pointregNet.GetNumIgnoredMeasures(), 3);
}

