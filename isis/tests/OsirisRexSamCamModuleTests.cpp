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
 *   @history 2025-01-15 Ken Edmundson
 *            - Converted tests from Makefile to ctest module format.
 *            - Using calibrated SamCam images, from OSIRIS-REx sample collection (TAG; 
 *              https://sbnarchive.psi.edu/pds4/orex/orex.ocams/data_calibrated/sample_collection):
 *              20200811T221418S029_sam_iofL2pan5.fits
 *              20200811T221518S391_sam_iofL2pan5.fits
 *            - Removed original call to coreg and photomet app as OSIRIS-REx IPWG determined
 *              these are not necessary.
 */


/**
 * SamCamModuleExplodeReuniteTest
 *
 * 1) SamCam image 20200811T221418S029_sam_iofL2pan5.fits is ingested with ocams2isis and
 *    spiceinited.
 * 2) phocube is used to create latitude, longitude, phase angle, emission angle,
 *    incidence angle, and pixel resolution backplanes.
 * 3) explode outputs the image backplanes into 7 separate cubes.
 * 4) cubeit app reunites the exploded cubes.
 * 5) The test then compares resulting phocube and cubeit cube labels and histograms,
 *    which should be identical.
 *
 * INPUT: isis/tests/data/osirisRexImages/ocams/20200811T221418S029_sam_iofL2pan5.fits
 *
 * OUTPUT: 1) ocams2isis
 *            - 20200811T221418S029_sam_iofL2pan5.cub
 *         2) phocube
 *            - 20200811T221418S029_sam_iofL2pan5_bp.cub
 *         3) explode (produces 7 cubes, one for each band, e.g.)
 *            - 20190328T200344S309_pol_L0pan_V001.band0001.cub (band0001-band0007)
 *         4) cubeit
 *            - 20200811T221418S029_sam_iofL2pan5_V001_cubeit.cub
 */
TEST(OsirisRexSamCamModules, SamCamModuleExplodeReuniteTest)
{
  QTemporaryDir tempDir;

  // ingest samcam fits format image with ocams2isis
  QString samCamFileName = tempDir.path() + "/20200811T221418S029_sam_iofL2pan5.cub";
  QVector<QString> ocams2isisArgs = {"from=data/osirisRexImages/ocams/20200811T221418S029_sam_iofL2pan5.fits",
                                     "to=" + samCamFileName};

  UserInterface ocams2isisSamCam(OCAMS2ISIS_XML, ocams2isisArgs);
  try
  {
    ocams2isis(ocams2isisSamCam);
  }
  catch (IException &e)
  {
    FAIL() << "Unable to run ocams2isis on samcam fits file 20200811T221418S029_sam_iofL2pan5.fits: "
           << e.what() << std::endl;
  }

  // spiceinit
  QVector<QString> spiceinitArgs = {"from=" + samCamFileName};
  UserInterface spiceinitSamCam(SPICEINIT_XML, spiceinitArgs);
  try
  {
    spiceinit(spiceinitSamCam);
  }
  catch (IException &e)
  {
    FAIL() << "Unable to run spiceinit on samcam cube 20200811T221418S029_sam_iofL2pan5.cub: "
           << e.what() << std::endl;
  }

  // phocube samcam image to create backplanes for latitude, longitude, phase angle,
  // emission angle, incidence angle, and pixel resolution;
  // dn=y propagates input pixel to output file
  QString phocubeFileName = tempDir.path() + "/20200811T221418S029_sam_iofL2pan5_bp.cub";
  QVector<QString> phocubeArgs = {"from=" + samCamFileName,
                                  "to=" + phocubeFileName,
                                  "dn=y",
                                  "phase=y",
                                  "emission=y",
                                  "incidence=y",
                                  "pixelresolution=y"};
  UserInterface phocubeUI(PHOCUBE_XML, phocubeArgs);
  try
  {
    phocube(phocubeUI);
  }
  catch (IException &e)
  {
    FAIL() << "Unable to run phocube on samcam cube 20200811T221418S029_sam_iofL2pan5.cub: "
           << e.what() << std::endl;
  }

  // explode image backplanes into 7 cubes
  QVector<QString> explodeArgs = {"from=" + phocubeFileName,
                                  "to=" + tempDir.path() + "/20200811T221418S029_sam_iofL2pan5_V001"};
  UserInterface explodeUI(EXPLODE_XML, explodeArgs);
  try
  {
    explode(explodeUI);
  }
  catch (IException &e)
  {
    FAIL() << "Unable to run explode on phocubed samcam cube 20200811T221418S029_sam_iofL2pan5_bp.cub: "
           << e.what() << std::endl;
  }

  // reunite exploded cubes
  // output should be identical to phocube bp output above
  FileList *cubeList = new FileList(); // list of exploded cubes
  cubeList->append(tempDir.path() + "/20200811T221418S029_sam_iofL2pan5_V001.band0001.cub");
  cubeList->append(tempDir.path() + "/20200811T221418S029_sam_iofL2pan5_V001.band0002.cub");
  cubeList->append(tempDir.path() + "/20200811T221418S029_sam_iofL2pan5_V001.band0003.cub");
  cubeList->append(tempDir.path() + "/20200811T221418S029_sam_iofL2pan5_V001.band0004.cub");
  cubeList->append(tempDir.path() + "/20200811T221418S029_sam_iofL2pan5_V001.band0005.cub");
  cubeList->append(tempDir.path() + "/20200811T221418S029_sam_iofL2pan5_V001.band0006.cub");
  cubeList->append(tempDir.path() + "/20200811T221418S029_sam_iofL2pan5_V001.band0007.cub");

  QString cubeListFile = tempDir.path() + "/phocubeBandList.lis";
  cubeList->write(cubeListFile);

  QString cubeitFileName = tempDir.path() + "/20200811T221418S029_sam_iofL2pan5_V001_cubeit.cub";

  QVector<QString> cubeitArgs = {"from=" + cubeListFile,
                                 "to=" + cubeitFileName};
  UserInterface cubeitUI(CUBEIT_XML, cubeitArgs);
  try
  {
    cubeit(cubeitUI);
  }
  catch (IException &e)
  {
    FAIL() << "Unable to run cubeit on exploded samcam images: " << e.what() << std::endl;
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

  std::unique_ptr<Histogram> phocubeHist(phocubeCube.histogram());
  std::unique_ptr<Histogram> cubeitHist(cubeitCube.histogram());

  EXPECT_NEAR(phocubeHist->Average(), cubeitHist->Average(), 0.0001);
  EXPECT_EQ(phocubeHist->Sum(), cubeitHist->Sum());
  EXPECT_EQ(phocubeHist->ValidPixels(), cubeitHist->ValidPixels());
  EXPECT_EQ(phocubeHist->StandardDeviation(), cubeitHist->StandardDeviation());
}

/**
 * SamCamModuleAlgebraTest
 *
 * 1) Samcam image 20200811T221418S029_sam_iofL2pan5.fits is ingested with ocams2isis.
 * 2) algebra is used to modify the dn values by doubling and adding 1.
 * 3) The test compares the histograms from the original and algebra'd cubes.
 *
 * INPUT: isis/tests/data/osirisRexImages/ocams/20200811T221418S029_sam_iofL2pan5.fits
 *
 * OUTPUT: 1) ocams2isis
 *            - 20200811T221418S029_sam_iofL2pan5.cub
 *         2) algebra
 *            - 20200811T221418S029_sam_iofL2pan5_algebra.cub
 */
TEST(OsirisRexSamCamModules, SamCamModuleAlgebraTest)
{
  QTemporaryDir tempDir;

  // ingest samcam fits format image with ocams2isis
  QString samCamFileName = tempDir.path() + "/20200811T221418S029_sam_iofL2pan5.cub";
  QVector<QString> ocams2isisArgs = {"from=data/osirisRexImages/ocams/20200811T221418S029_sam_iofL2pan5.fits",
                                     "to=" + samCamFileName};

  UserInterface ocams2isisUI(OCAMS2ISIS_XML, ocams2isisArgs);
  try
  {
    ocams2isis(ocams2isisUI);
  }
  catch (IException &e)
  {
    FAIL() << "Unable to run ocams2isis on samcam fits file 20200811T221418S029_sam_iofL2pan5.fits: "
           << e.what() << std::endl;
  }

  QString algebraFileName = tempDir.path() + "/20200811T221418S029_sam_iofL2pan5_algebra.cub";

  QVector<QString> algebraArgs = {"from=" + samCamFileName,
                                  "to=" + algebraFileName,
                                  "operator=unary",
                                  "a=2",
                                  "c=1"};
  UserInterface algebraUI(ALGEBRA_XML, algebraArgs);
  try
  {
    algebra(algebraUI);
  }
  catch (IException &e)
  {
    FAIL() << "Unable to run algebra on samcam cube 20200811T221418S029_sam_iofL2pan5.cub: "
           << e.what() << std::endl;
  }

  // compare histograms of original and algebra'd cube
  Cube originalCube(samCamFileName);
  Cube algebraCube(algebraFileName);

  std::unique_ptr<Histogram> originalHist(originalCube.histogram());
  std::unique_ptr<Histogram> algebraHist(algebraCube.histogram());

  // pixels are multiplied by two and add one
  EXPECT_EQ(algebraHist->ValidPixels(), originalHist->ValidPixels());
  EXPECT_NEAR(algebraHist->Average(), (originalHist->Average() * 2.0) + 1.0, 0.0001);
  EXPECT_NEAR(algebraHist->Sum(), (originalHist->Sum() * 2) + originalHist->ValidPixels(), 0.0001);
  EXPECT_NEAR(algebraHist->StandardDeviation(), (originalHist->StandardDeviation() * 2), 0.0001);
}


/**
 * SamCamModuleInfoAndStatsTest
 *
 * 1) Samcam image 20200811T221418S029_sam_iofL2pan5.fits is ingested with ocams2isis
 *    and spiceinited.
 * 2) getsn is used to generate the cube serial number.
 * 3) caminfo outputs various spacecraft and instrument-related details (e.g. geometric,
 *    polygon, and mapping info).
 * 4) camstats generates camera statistics.
 * 5) stats generates pixel statistics.
 * 6) The test validates the output serial number, info, and statistics files.
 *
 * INPUT: isis/tests/data/osirisRexImages/ocams/20200811T221418S029_sam_iofL2pan5.fits
 *
 * OUTPUT: 1) ocams2isis
 *            - 20200811T221418S029_sam_iofL2pan5.cub
 *         2) getsn
 *            - polycam_getsn.pvl
 *         3) caminfo
 *            - 20200811T221418S029_sam_iofL2pan5_caminfo.pvl
 *         4) camstats
 *            - 20200811T221418S029_sam_iofL2pan5_camstats.pvl
 *         5) stats
 *            - 20200811T221418S029_sam_iofL2pan5_dnstats.pvl
 */
TEST(OsirisRexSamCamModules, SamCamModuleInfoAndStatsTest)
{
  QTemporaryDir tempDir;

  // ingest samcam fits format image with ocams2isis and then spiceinit
  QString samCamFileName = tempDir.path() + "/20200811T221418S029_sam_iofL2pan5.cub";
  QVector<QString> ocams2isisArgs = {"from=data/osirisRexImages/ocams/20200811T221418S029_sam_iofL2pan5.fits",
                                     "to=" + samCamFileName};

  UserInterface ocams2isisUI(OCAMS2ISIS_XML, ocams2isisArgs);
  try
  {
    ocams2isis(ocams2isisUI);
  }
  catch (IException &e)
  {
    FAIL() << "Unable to run ocams2isis on samcam fits file 20200811T221418S029_sam_iofL2pan5.fits: "
           << e.what() << std::endl;
  }

  QVector<QString> spiceinitArgs = {"from=" + samCamFileName};
  UserInterface spiceinitUI(SPICEINIT_XML, spiceinitArgs);
  try
  {
    spiceinit(spiceinitUI);
  }
  catch (IException &e)
  {
    FAIL() << "Unable to run spiceinit on samcam cube 20200811T221418S029_sam_iofL2pan5.cub: "
           << e.what() << std::endl;
  }

  // getsn from samcam image
  QVector<QString> getsnArgs = {"from=" + samCamFileName,
                                "to=" + tempDir.path() + "/samcam_getsn.pvl",
                                "append=yes"};
  UserInterface getsnUI(GETSN_XML, getsnArgs);
  Pvl appLog;
  try
  {
    getsn(getsnUI, &appLog);
  }
  catch (IException &e)
  {
    FAIL() << "Unable to run getsn on samcam cube 20200811T221418S029_sam_iofL2pan5.cub: "
           << e.what() << std::endl;
  }

  // validate samcam image serial number
  Pvl getsnPvl;
  getsnPvl.read(tempDir.path() + "/samcam_getsn.pvl");

  // Results Group
  PvlGroup &getSNResultsGroup = getsnPvl.findGroup("Results", Pvl::Traverse);
  EXPECT_EQ(getSNResultsGroup["SerialNumber"][0].toStdString(), "OsirisRex/SamCam/3/0650455990.36753");

  // get camera information
  QVector<QString> caminfoArgs = {"from=" + samCamFileName,
                                  "to=" + tempDir.path() + "/20200811T221418S029_sam_iofL2pan5_caminfo.pvl"};
  UserInterface caminfoUI(CAMINFO_XML, caminfoArgs);
  try
  {
    caminfo(caminfoUI);
  }
  catch (IException &e)
  {
    FAIL() << "Unable to run caminfo on samcam cube 20200811T221418S029_sam_iofL2pan5.cub: "
           << e.what() << std::endl;
  }

  // validate caminfo file
  Pvl caminfoPvl;
  caminfoPvl.read(tempDir.path() + "/20200811T221418S029_sam_iofL2pan5_caminfo.pvl");

  // Parameters Object
  PvlObject &parametersObject = caminfoPvl.findObject("Parameters", Pvl::Traverse);
  EXPECT_EQ(parametersObject["Program"][0].toStdString(), "caminfo");
  EXPECT_EQ(parametersObject["IsisId"][0].toStdString(), "OsirisRex/SamCam/3/0650455990.36753");
  EXPECT_EQ(parametersObject["From"][0].toStdString(), "20200811T221418S029_sam_iofL2pan5.cub");
  EXPECT_EQ(int(parametersObject["Lines"]), 1024);
  EXPECT_EQ(int(parametersObject["Samples"]), 1024);
  EXPECT_EQ(int(parametersObject["Bands"]), 1);

  // Geometry Object
  PvlObject &geometryObject = caminfoPvl.findObject("Geometry", Pvl::Traverse);
  EXPECT_EQ(int(geometryObject["BandsUsed"]), 1);
  EXPECT_EQ(int(geometryObject["ReferenceBand"]), 1);
  EXPECT_EQ(int(geometryObject["OriginalBand"]), 1);
  EXPECT_EQ(geometryObject["Target"][0].toStdString(), "Bennu");
  EXPECT_EQ(geometryObject["StartTime"][0].toStdString(), "2020-08-11T22:14:18.0166387");
  EXPECT_EQ(geometryObject["EndTime"][0].toStdString(), "2020-08-11T22:14:18.0166387");
  EXPECT_DOUBLE_EQ(double(geometryObject["CenterLine"]), 512.0);
  EXPECT_DOUBLE_EQ(double(geometryObject["CenterSample"]), 512.0);
  EXPECT_NEAR(double(geometryObject.findKeyword("CenterLatitude")), 50.146656833644997, 1e-8);
  EXPECT_NEAR(double(geometryObject.findKeyword("CenterLongitude")), 58.342021411841998, 1e-8);
  EXPECT_NEAR(double(geometryObject.findKeyword("CenterRadius")), 258.99974956113999, 1e-8);
  EXPECT_NEAR(double(geometryObject.findKeyword("RightAscension")), 191.71210958578999, 1e-8);
  EXPECT_NEAR(double(geometryObject.findKeyword("Declination")), 47.643033700765997, 1e-8);
  EXPECT_NEAR(double(geometryObject.findKeyword("UpperLeftLongitude")), 61.704087172386998, 1e-8);
  EXPECT_NEAR(double(geometryObject.findKeyword("UpperLeftLatitude")), 46.151998147838, 1e-8);
  EXPECT_NEAR(double(geometryObject.findKeyword("LowerLeftLongitude")), 52.206916399613, 1e-8);
  EXPECT_NEAR(double(geometryObject.findKeyword("LowerLeftLatitude")), 47.584454638079002, 1e-8);
  EXPECT_NEAR(double(geometryObject.findKeyword("LowerRightLongitude")), 53.826055067016, 1e-8);
  EXPECT_NEAR(double(geometryObject.findKeyword("LowerRightLatitude")), 54.435297799548003, 1e-8);
  EXPECT_NEAR(double(geometryObject.findKeyword("UpperRightLongitude")), 65.021639385019995, 1e-8);
  EXPECT_NEAR(double(geometryObject.findKeyword("UpperRightLatitude")), 52.367667037958, 1e-8);
  EXPECT_NEAR(double(geometryObject.findKeyword("PhaseAngle")), 59.768293345121002, 1e-8);
  EXPECT_NEAR(double(geometryObject.findKeyword("EmissionAngle")), 9.5393438584533001, 1e-8);
  EXPECT_NEAR(double(geometryObject.findKeyword("IncidenceAngle")), 60.890338972278997, 1e-8);
  EXPECT_NEAR(double(geometryObject.findKeyword("NorthAzimuth")), 14.652172109042001, 1e-8);
  EXPECT_NEAR(double(geometryObject.findKeyword("OffNadir")), 7.7063113038062996, 1e-8);
  EXPECT_NEAR(double(geometryObject.findKeyword("SolarLongitude")), 112.19349952257, 1e-8);
  EXPECT_NEAR(double(geometryObject.findKeyword("LocalTime")), 14.327224630888001, 1e-8);
  EXPECT_NEAR(double(geometryObject.findKeyword("TargetCenterDistance")), 0.34198263675243, 1e-8);
  EXPECT_NEAR(double(geometryObject.findKeyword("SlantDistance")), 0.083527290386371994, 1e-8);
  EXPECT_NEAR(double(geometryObject.findKeyword("SampleResolution")), 0.029097621650989999, 1e-8);
  EXPECT_NEAR(double(geometryObject.findKeyword("LineResolution")), 0.029097621650989999, 1e-8);
  EXPECT_NEAR(double(geometryObject.findKeyword("PixelResolution")), 0.029097621650989999, 1e-8);
  EXPECT_NEAR(double(geometryObject.findKeyword("MeanGroundResolution")), 0.030308189151408999, 1e-8);
  EXPECT_NEAR(double(geometryObject.findKeyword("SubSolarAzimuth")), 151.53939253928999, 1e-8);
  EXPECT_NEAR(double(geometryObject.findKeyword("SubSolarGroundAzimuth")), 223.44958120455999, 1e-8);
  EXPECT_NEAR(double(geometryObject.findKeyword("SubSolarLatitude")), 2.2742756940816, 1e-8);
  EXPECT_NEAR(double(geometryObject.findKeyword("SubSolarLongitude")), 23.433651948528, 1e-8);
  EXPECT_NEAR(double(geometryObject.findKeyword("SubSpacecraftAzimuth")), 267.08409761706002, 1e-8);
  EXPECT_NEAR(double(geometryObject.findKeyword("SubSpacecraftGroundAzimuth")), 107.71147351473, 1e-8);
  EXPECT_NEAR(double(geometryObject.findKeyword("SubSpacecraftLatitude")), 49.557593608158, 1e-8);
  EXPECT_NEAR(double(geometryObject.findKeyword("SubSpacecraftLongitude")), 61.034386896356, 1e-8);
  EXPECT_NEAR(double(geometryObject.findKeyword("ParallaxX")), 0.051124387921281003, 1e-8);
  EXPECT_NEAR(double(geometryObject.findKeyword("ParallaxY")), 0.16008319339659, 1e-8);
  EXPECT_NEAR(double(geometryObject.findKeyword("ShadowX")), 1.3038109153713999, 1e-8);
  EXPECT_NEAR(double(geometryObject.findKeyword("ShadowY")), -1.2350916593898, 1e-8);
  EXPECT_EQ(geometryObject["HasLongitudeBoundary"][0].toStdString(), "FALSE");
  EXPECT_EQ(geometryObject["HasNorthPole"][0].toStdString(), "FALSE");
  EXPECT_EQ(geometryObject["HasSouthPole"][0].toStdString(), "FALSE");
  EXPECT_NEAR(double(geometryObject.findKeyword("ObliqueSampleResolution")), 0.029505624417934001, 1e-8);
  EXPECT_NEAR(double(geometryObject.findKeyword("ObliqueLineResolution")), 0.029505624417934001, 1e-8);
  EXPECT_NEAR(double(geometryObject.findKeyword("ObliquePixelResolution")), 0.029505624417934001, 1e-8);
  EXPECT_NEAR(double(geometryObject.findKeyword("ObliqueDetectorResolution")), 0.029505624417934001, 1e-8);

  // get camera statistics
  QVector<QString> camstatsArgs = {"from=" + samCamFileName,
                                   "to=" + tempDir.path() + "/20200811T221418S029_sam_iofL2pan5_camstats.pvl"};
  UserInterface camstatsUI(CAMSTATS_XML, camstatsArgs);

  try
  {
    camstats(camstatsUI, &appLog);
  }
  catch (IException &e)
  {
    FAIL() << "Unable to run camstats on samcam cube 20200811T221418S029_sam_iofL2pan5.cub: "
           << e.what() << std::endl;
  }

  // validate camstats file
  Pvl camstatsPvl;
  camstatsPvl.read(tempDir.path() + "/20200811T221418S029_sam_iofL2pan5_camstats.pvl");

  // User Parameters Group
  PvlGroup &userParametersGroup = camstatsPvl.findGroup("User Parameters", Pvl::Traverse);
  EXPECT_EQ(int(userParametersGroup["Linc"]), 1);
  EXPECT_EQ(int(userParametersGroup["Sinc"]), 1);

  // Latitude Group
  PvlGroup &latitudeGroup = camstatsPvl.findGroup("Latitude", Pvl::Traverse);
  EXPECT_NEAR((double)latitudeGroup.findKeyword("LatitudeMinimum"), 46.151998147838, 1e-8);
  EXPECT_NEAR((double)latitudeGroup.findKeyword("LatitudeMaximum"), 54.435297799548003, 1e-8);
  EXPECT_NEAR((double)latitudeGroup.findKeyword("LatitudeAverage"), 50.145503771096998, 1e-8);
  EXPECT_NEAR((double)latitudeGroup.findKeyword("LatitudeStandardDeviation"), 1.9290840891526, 1e-8);

  // Longitude Group
  PvlGroup &longitudeGroup = camstatsPvl.findGroup("Longitude", Pvl::Traverse);
  EXPECT_NEAR((double)longitudeGroup.findKeyword("LongitudeMinimum"), 52.206916399613, 1e-8);
  EXPECT_NEAR((double)longitudeGroup.findKeyword("LongitudeMaximum"), 65.021639385019995, 1e-8);
  EXPECT_NEAR((double)longitudeGroup.findKeyword("LongitudeAverage"), 58.292126985129997, 1e-8);
  EXPECT_NEAR((double)longitudeGroup.findKeyword("LongitudeStandardDeviation"), 3.0231340867297001, 1e-8);

  // SampleResolution Group
  PvlGroup &sampleResolutionGroup = camstatsPvl.findGroup("SampleResolution", Pvl::Traverse);
  EXPECT_NEAR((double)sampleResolutionGroup.findKeyword("SampleResolutionMinimum"), 0.028788519652444, 1e-8);
  EXPECT_NEAR((double)sampleResolutionGroup.findKeyword("SampleResolutionMaximum"), 0.031730379572807998, 1e-8);
  EXPECT_NEAR((double)sampleResolutionGroup.findKeyword("SampleResolutionAverage"), 0.029512077743988001, 1e-8);
  EXPECT_NEAR((double)sampleResolutionGroup.findKeyword("SampleResolutionStandardDeviation"), 0.00058519158093344901, 1e-8);

  // LineResolution Group
  PvlGroup &lineResolutionGroup = camstatsPvl.findGroup("LineResolution", Pvl::Traverse);
  EXPECT_NEAR((double)lineResolutionGroup.findKeyword("LineResolutionMinimum"), 0.028788519652444, 1e-8);
  EXPECT_NEAR((double)lineResolutionGroup.findKeyword("LineResolutionMaximum"), 0.031730379572807998, 1e-8);
  EXPECT_NEAR((double)lineResolutionGroup.findKeyword("LineResolutionAverage"), 0.029512077743988001, 1e-8);
  EXPECT_NEAR((double)lineResolutionGroup.findKeyword("LineResolutionStandardDeviation"), 0.00058519158093344901, 1e-8);

  // Resolution Group
  PvlGroup &resolutionGroup = camstatsPvl.findGroup("Resolution", Pvl::Traverse);
  EXPECT_NEAR((double)resolutionGroup.findKeyword("ResolutionMinimum"), 0.028788519652444, 1e-8);
  EXPECT_NEAR((double)resolutionGroup.findKeyword("ResolutionMaximum"), 0.031730379572807998, 1e-8);
  EXPECT_NEAR((double)resolutionGroup.findKeyword("ResolutionAverage"), 0.029512077743988001, 1e-8);
  EXPECT_NEAR((double)resolutionGroup.findKeyword("ResolutionStandardDeviation"), 0.00058519158093344901, 1e-8);

  // ObliqueSampleResolution Group
  PvlGroup &obliqeSampleResolutionGroup = camstatsPvl.findGroup("ObliqueSampleResolution", Pvl::Traverse);
  EXPECT_NEAR((double)obliqeSampleResolutionGroup.findKeyword("ObliqueSampleResolutionMinimum"), 0.028788520065944002, 1e-8);
  EXPECT_NEAR((double)obliqeSampleResolutionGroup.findKeyword("ObliqueSampleResolutionMaximum"), 0.036133840581173998, 1e-8);
  EXPECT_NEAR((double)obliqeSampleResolutionGroup.findKeyword("ObliqueSampleResolutionAverage"), 0.030503267610016999, 1e-8);
  EXPECT_NEAR((double)obliqeSampleResolutionGroup.findKeyword("ObliqueSampleResolutionStandardDeviation"), 0.0014179533692269999, 1e-8);

  // ObliqueLineResolution Group
  PvlGroup &obliqueLineResolutionGroup = camstatsPvl.findGroup("ObliqueLineResolution", Pvl::Traverse);
  EXPECT_NEAR((double)obliqueLineResolutionGroup.findKeyword("ObliqueLineResolutionMinimum"), 0.028788520065944002, 1e-8);
  EXPECT_NEAR((double)obliqueLineResolutionGroup.findKeyword("ObliqueLineResolutionMaximum"), 0.036133840581173998, 1e-8);
  EXPECT_NEAR((double)obliqueLineResolutionGroup.findKeyword("ObliqueLineResolutionAverage"), 0.030503267610016999, 1e-8);
  EXPECT_NEAR((double)obliqueLineResolutionGroup.findKeyword("ObliqueLineResolutionStandardDeviation"), 0.0014179533692269999, 1e-8);

  // ObliqueResolution Group
  PvlGroup &obliqueResolutionGroup = camstatsPvl.findGroup("ObliqueResolution", Pvl::Traverse);
  EXPECT_NEAR((double)obliqueResolutionGroup.findKeyword("ObliqueResolutionMinimum"), 0.028788520065944002, 1e-8);
  EXPECT_NEAR((double)obliqueResolutionGroup.findKeyword("ObliqueResolutionMaximum"), 0.036133840581173998, 1e-8);
  EXPECT_NEAR((double)obliqueResolutionGroup.findKeyword("ObliqueResolutionAverage"), 0.030503267610016999, 1e-8);
  EXPECT_NEAR((double)obliqueResolutionGroup.findKeyword("ObliqueResolutionStandardDeviation"), 0.0014179533692269999, 1e-8);

  // AspectRatio Group
  PvlGroup &aspectRatioGroup = camstatsPvl.findGroup("AspectRatio", Pvl::Traverse);
  EXPECT_DOUBLE_EQ(double(aspectRatioGroup["AspectRatioMinimum"]), 1.0);
  EXPECT_DOUBLE_EQ(double(aspectRatioGroup["AspectRatioMaximun"]), 1.0);
  EXPECT_DOUBLE_EQ(double(aspectRatioGroup["AspectRatioAverage"]), 1.0);
  EXPECT_DOUBLE_EQ(double(aspectRatioGroup["AspectRatioStandardDeviation"]), 0.0);

  // PhaseAngle Group
  PvlGroup &phaseAngleGroup = camstatsPvl.findGroup("PhaseAngle", Pvl::Traverse);
  EXPECT_NEAR((double)phaseAngleGroup.findKeyword("PhaseMinimum"), 45.845109824169, 1e-8);
  EXPECT_NEAR((double)phaseAngleGroup.findKeyword("PhaseMaximum"), 73.807471406353002, 1e-8);
  EXPECT_NEAR((double)phaseAngleGroup.findKeyword("PhaseAverage"), 59.943381412362001, 1e-8);
  EXPECT_NEAR((double)phaseAngleGroup.findKeyword("PhaseStandardDeviation"), 5.8597501027209002, 1e-8);

  // EmissionAngle Group
  PvlGroup &emissionAngleGroup = camstatsPvl.findGroup("EmissionAngle", Pvl::Traverse);
  EXPECT_NEAR((double)emissionAngleGroup.findKeyword("EmissionMinimum"), 0.0097110433496234005, 1e-8);
  EXPECT_NEAR((double)emissionAngleGroup.findKeyword("EmissionMaximum"), 28.581833397705001, 1e-8);
  EXPECT_NEAR((double)emissionAngleGroup.findKeyword("EmissionAverage"), 13.024101463321999, 1e-8);
  EXPECT_NEAR((double)emissionAngleGroup.findKeyword("EmissionStandardDeviation"), 6.1573486114604998, 1e-8);

  // IncidenceAngle Group
  PvlGroup &incidenceAngleGroup = camstatsPvl.findGroup("IncidenceAngle", Pvl::Traverse);
  EXPECT_NEAR((double)incidenceAngleGroup.findKeyword("IncidenceMinimum"), 56.936857115134998, 1e-8);
  EXPECT_NEAR((double)incidenceAngleGroup.findKeyword("IncidenceMaximum"), 64.751791647299996, 1e-8);
  EXPECT_NEAR((double)incidenceAngleGroup.findKeyword("IncidenceAverage"), 60.906555072379, 1e-8);
  EXPECT_NEAR((double)incidenceAngleGroup.findKeyword("IncidenceStandardDeviation"), 1.6873078846595999, 1e-8);

  // LocalSolarTime Group
  PvlGroup &localSolarTimeGroup = camstatsPvl.findGroup("LocalSolarTime", Pvl::Traverse);
  EXPECT_NEAR((double)localSolarTimeGroup.findKeyword("LocalSolarTimeMinimum"), 13.918217630072, 1e-8);
  EXPECT_NEAR((double)localSolarTimeGroup.findKeyword("LocalSolarTimeMaximum"), 14.772532495766001, 1e-8);
  EXPECT_NEAR((double)localSolarTimeGroup.findKeyword("LocalSolarTimeAverage"), 14.323898335773, 1e-8);
  EXPECT_NEAR((double)localSolarTimeGroup.findKeyword("LocalSolarTimeStandardDeviation"), 0.20154227243987999, 1e-8);

  // LocalRadius Group
  PvlGroup &localRadiusGroup = camstatsPvl.findGroup("LocalRadius", Pvl::Traverse);
  EXPECT_NEAR((double)localRadiusGroup.findKeyword("LocalRadiusMinimum"), 257.52355100671002, 1e-8);
  EXPECT_NEAR((double)localRadiusGroup.findKeyword("LocalRadiusMaximum"), 260.51113341011001, 1e-8);
  EXPECT_NEAR((double)localRadiusGroup.findKeyword("LocalRadiusAverage"), 259.01661026276997, 1e-8);
  EXPECT_NEAR((double)localRadiusGroup.findKeyword("LocalRadiusStandardDeviation"), 0.80872878214788002, 1e-8);

  // NorthAzimuth Group
  PvlGroup &northAzimuthGroup = camstatsPvl.findGroup("NorthAzimuth", Pvl::Traverse);
  EXPECT_NEAR((double)northAzimuthGroup.findKeyword("NorthAzimuthMinimum"), 8.3819300079138994, 1e-8);
  EXPECT_NEAR((double)northAzimuthGroup.findKeyword("NorthAzimuthMaximum"), 21.711455490376, 1e-8);
  EXPECT_NEAR((double)northAzimuthGroup.findKeyword("NorthAzimuthAverage"), 14.644365477847, 1e-8);
  EXPECT_NEAR((double)northAzimuthGroup.findKeyword("NorthAzimuthStandardDeviation"), 2.6065812557906001, 1e-8);

  // get DN statistics
  QVector<QString> statsArgs = {"from=" + samCamFileName,
                                "to=" + tempDir.path() + "/20200811T221418S029_sam_iofL2pan5_dnstats.pvl"};
  UserInterface statsUI(STATS_XML, statsArgs);
  try
  {
    stats(statsUI);
  }
  catch (IException &e)
  {
    FAIL() << "Unable to run stats on samcam cube 20200811T221418S029_sam_iofL2pan5.cub: "
           << e.what() << std::endl;
  }

  // validate dnstats file
  Pvl dnstatsPvl;
  dnstatsPvl.read(tempDir.path() + "/20200811T221418S029_sam_iofL2pan5_dnstats.pvl");

  // Results Group
  PvlGroup &resultsGroup = dnstatsPvl.findGroup("Results", Pvl::Traverse);
  EXPECT_EQ(int(resultsGroup["Band"]), 1);
  EXPECT_NEAR((double)resultsGroup.findKeyword("Average"), 0.01072035323966, 1e-8);
  EXPECT_NEAR((double)resultsGroup.findKeyword("StandardDeviation"), 0.0108438322027, 1e-8);
  EXPECT_NEAR((double)resultsGroup.findKeyword("Variance"), 0.000117588696840307, 1e-8);
  EXPECT_NEAR(double(resultsGroup["Median"]), 0.0086525052318413998, 1e-8);
  EXPECT_NEAR(double(resultsGroup["Mode"]), 0.00062461889323983698, 1e-8);
  EXPECT_NEAR((double)resultsGroup.findKeyword("Skew"), 0.57208041469995996, 1e-8);
  EXPECT_NEAR(double(resultsGroup["Minimum"]), -0.00016337799024768201, 1e-8);
  EXPECT_NEAR(double(resultsGroup["Maximum"]), 0.058586765080690002, 1e-8);
  EXPECT_NEAR(double(resultsGroup["Sum"]), 11241.105118629999, 1e-8);
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
 * SamCamModuleCamptTest
 *
 * 1) SamCam image 20200811T221418S029_sam_iofL2pan5.cub is ingested with ocams2isis
 *    and spiceinited.
 * 2) campt is used to compute geometric and photometric info at the cube corner
 *    and center pixels.
 * 3) The test validates the campt output.
 *
 * INPUT: isis/tests/data/osirisRexImages/ocams/20200811T221418S029_sam_iofL2pan5.fits
 *
 * OUTPUT: 1) ocams2isis
 *            - 20200811T221418S029_sam_iofL2pan5.cub
 *         2) campt
 *            - 20200811T221418S029_sam_iofL2pan5.cub_campt.pvl
 */
  TEST(OsirisRexSamCamModules, SamCamModuleCamptTest) {
  QTemporaryDir tempDir;

  // ingest polycam fits format image with ocams2isis
  QString samCamFileName = tempDir.path() + "/20200811T221418S029_sam_iofL2pan5.cub";
  QVector<QString> ocams2isisArgs = {"from=data/osirisRexImages/ocams/20200811T221418S029_sam_iofL2pan5.fits",
                                         "to=" + samCamFileName};

  UserInterface ocams2isisUI(OCAMS2ISIS_XML, ocams2isisArgs);
  try {
    ocams2isis(ocams2isisUI);
  }
  catch (IException &e) {
    FAIL() << "Unable to run ocams2isis on samcam fits file 20200811T221418S029_sam_iofL2pan5.fits: "
           << e.what() << std::endl;
  }

  QVector<QString> spiceinitArgs = {"from=" + samCamFileName};
  UserInterface spiceinitUI(SPICEINIT_XML, spiceinitArgs);
  try {
    spiceinit(spiceinitUI);
  }
  catch (IException &e) {
    FAIL() << "Unable to run spiceinit on samcam cube 20200811T221418S029_sam_iofL2pan5.cub: "
           << e.what() << std::endl;
  }

  // create flatfile with corner and center coordinates
  ofstream of1;
  of1.open(tempDir.path().toStdString() + "/campt_input_coords.lis");
  of1 << "1,1\n1024,1\n1024,1024\n1,1024\n512,512";
  of1.close();

  Pvl appLog;

  // campt all image coordinates in campt_input_coords.lis file
  QVector<QString> camptArgs = {"from=" + samCamFileName,
                                "to=" + tempDir.path() + "/20200811T221418S029_sam_iofL2pan5.cub_campt.pvl",
                                "USECOORDLIST=TRUE",
                                "COORDLIST=" + tempDir.path() + "/campt_input_coords.lis",
                                "COORDTYPE=IMAGE",
                                "ALLOWOUTSIDE=no"};
  UserInterface camptULUI(CAMPT_XML, camptArgs);
  try {
    campt(camptULUI, &appLog);
  }
  catch (IException &e) {
    FAIL() << "Unable to run campt on samcam cube 20200811T221418S029_sam_iofL2pan5.cub: "
           << e.what() << std::endl;
  }

  // read campt output pvl file
  Pvl camptPvl;
  camptPvl.read(tempDir.path() + "/20200811T221418S029_sam_iofL2pan5.cub_campt.pvl");

  // validate upper left corner
  // GroundPoint Group
  PvlGroup &gpULGroup = camptPvl.findGroup("GroundPoint", Pvl::Traverse);
  EXPECT_DOUBLE_EQ(double(gpULGroup["Sample"]), 1.0);
  EXPECT_DOUBLE_EQ(double(gpULGroup["Line"]), 1.0);
  EXPECT_NEAR(double(gpULGroup["PixelValue"]), 0.00036952681999999999, 1e-8);
  EXPECT_NEAR((double)gpULGroup.findKeyword("RightAscension"), 213.06877557355, 1e-8);
  EXPECT_NEAR((double)gpULGroup.findKeyword("Declination"), 51.439520363050001, 1e-8);
  EXPECT_NEAR((double)gpULGroup.findKeyword("PlanetocentricLatitude"), 46.151998147838, 1e-8);
  EXPECT_NEAR((double)gpULGroup.findKeyword("PlanetographicLatitude"), 53.218443268698003, 1e-8);
  EXPECT_NEAR((double)gpULGroup.findKeyword("PositiveEast360Longitude"), 61.704087172386998, 1e-8);
  EXPECT_NEAR((double)gpULGroup.findKeyword("PositiveEast180Longitude"), 61.704087172386998, 1e-8);
  EXPECT_NEAR((double)gpULGroup.findKeyword("PositiveWest360Longitude"), 298.29591282760998, 1e-8);
  EXPECT_NEAR((double)gpULGroup.findKeyword("PositiveWest180Longitude"), -61.704087172388, 1e-8);
  EXPECT_NEAR(toDouble(gpULGroup.findKeyword("BodyFixedCoordinate")[0]), 0.085519550447066994, 1e-8);
  EXPECT_NEAR(toDouble(gpULGroup.findKeyword("BodyFixedCoordinate")[1]), 0.15885418701880999, 1e-8);
  EXPECT_NEAR(toDouble(gpULGroup.findKeyword("BodyFixedCoordinate")[2]),  0.18781595992137001, 1e-8);
  EXPECT_NEAR((double)gpULGroup.findKeyword("LocalRadius"), 260.42864866075001, 1e-8);
  EXPECT_NEAR((double)gpULGroup.findKeyword("SampleResolution"), 0.029085679101775001, 1e-8);
  EXPECT_NEAR((double)gpULGroup.findKeyword("LineResolution"), 0.029085679101775001, 1e-8);
  EXPECT_NEAR((double)gpULGroup.findKeyword("ObliqueDetectorResolution"), 0.029480374616900999, 1e-8);
  EXPECT_NEAR((double)gpULGroup.findKeyword("ObliquePixelResolution"), 0.029480374616900999, 1e-8);
  EXPECT_NEAR((double)gpULGroup.findKeyword("ObliqueLineResolution"), 0.029480374616900999, 1e-8);
  EXPECT_NEAR((double)gpULGroup.findKeyword("ObliqueSampleResolution"), 0.029480374616900999, 1e-8);
  EXPECT_NEAR(toDouble(gpULGroup.findKeyword("SpacecraftPosition")[0]), 0.10743294712690001, 1e-8);
  EXPECT_NEAR(toDouble(gpULGroup.findKeyword("SpacecraftPosition")[1]), 0.19408878947425001, 1e-8);
  EXPECT_NEAR(toDouble(gpULGroup.findKeyword("SpacecraftPosition")[2]),  0.26026876015417999, 1e-8);
  EXPECT_NEAR((double)gpULGroup.findKeyword("SpacecraftAzimuth"), 23.902626458669999, 1e-8);
  EXPECT_NEAR((double)gpULGroup.findKeyword("SlantDistance"), 0.083493008245096006, 1e-8);
  EXPECT_NEAR((double)gpULGroup.findKeyword("TargetCenterDistance"), 0.34198263675243001, 1e-8);
  EXPECT_NEAR((double)gpULGroup.findKeyword("SubSpacecraftLatitude"), 49.557593608158001, 1e-8);
  EXPECT_NEAR((double)gpULGroup.findKeyword("SubSpacecraftLongitude"), 61.034386896355997, 1e-8);
  EXPECT_NEAR((double)gpULGroup.findKeyword("SpacecraftAltitude"), 0.082907681748561002, 1e-8);
  EXPECT_NEAR((double)gpULGroup.findKeyword("OffNadirAngle"), 5.9510850664105002, 1e-8);
  EXPECT_NEAR((double)gpULGroup.findKeyword("SubSpacecraftGroundAzimuth"), 352.73031127557999, 1e-8);
  EXPECT_NEAR(toDouble(gpULGroup.findKeyword("SunPosition")[0]), 173381543.43966001, 1e-5);
  EXPECT_NEAR(toDouble(gpULGroup.findKeyword("SunPosition")[1]), 75149827.507274002, 1e-5);
  EXPECT_NEAR(toDouble(gpULGroup.findKeyword("SunPosition")[2]),  7504735.7285030996, 1e-5);
  EXPECT_NEAR((double)gpULGroup.findKeyword("SubSolarAzimuth"), 146.88037353876001, 1e-8);
  EXPECT_NEAR((double)gpULGroup.findKeyword("SolarDistance"), 1.2641644235414999, 1e-8);
  EXPECT_NEAR((double)gpULGroup.findKeyword("SubSolarLatitude"), 2.2742756940816, 1e-8);
  EXPECT_NEAR((double)gpULGroup.findKeyword("SubSolarLongitude"), 23.433651948527999, 1e-8);
  EXPECT_NEAR((double)gpULGroup.findKeyword("SubSolarGroundAzimuth"), 228.98577856626, 1e-8);
  EXPECT_NEAR((double)gpULGroup.findKeyword("Phase"), 63.720451308504003, 1e-8);
  EXPECT_NEAR((double)gpULGroup.findKeyword("Incidence"), 59.520921603882002, 1e-8);
  EXPECT_NEAR((double)gpULGroup.findKeyword("Emission"), 9.3861602300607991, 1e-8);
  EXPECT_NEAR((double)gpULGroup.findKeyword("NorthAzimuth"), 16.332787390147999, 1e-8);
  EXPECT_NEAR((double)gpULGroup.findKeyword("EphemerisTime"), 650456127.19964004, 1e-5);
  EXPECT_EQ(gpULGroup["UTC"][0].toStdString(), "2020-08-11T22:14:18.0166387");
  EXPECT_NEAR((double)gpULGroup.findKeyword("LocalSolarTime"), 14.551362348256999, 1e-8);
  EXPECT_NEAR((double)gpULGroup.findKeyword("SolarLongitude"), 112.19349952256999, 1e-8);

  // Look Direction Unit Vectors in Body Fixed, J2000, and Camera Coordinate Systems.
  EXPECT_NEAR(toDouble(gpULGroup.findKeyword("LookDirectionBodyFixed")[0]), -0.26245786492091, 1e-8);
  EXPECT_NEAR(toDouble(gpULGroup.findKeyword("LookDirectionBodyFixed")[1]), -0.4220066230217, 1e-8);
  EXPECT_NEAR(toDouble(gpULGroup.findKeyword("LookDirectionBodyFixed")[2]),  -0.86777086795247005, 1e-8);

  EXPECT_NEAR(toDouble(gpULGroup.findKeyword("LookDirectionJ2000")[0]), -0.52236934248141997, 1e-8);
  EXPECT_NEAR(toDouble(gpULGroup.findKeyword("LookDirectionJ2000")[1]), -0.34012278280211, 1e-8);
  EXPECT_NEAR(toDouble(gpULGroup.findKeyword("LookDirectionJ2000")[2]),  0.78195061394853005, 1e-8);

  EXPECT_NEAR(toDouble(gpULGroup.findKeyword("LookDirectionCamera")[0]), 0.17517470147392999, 1e-8);
  EXPECT_NEAR(toDouble(gpULGroup.findKeyword("LookDirectionCamera")[1]), -0.17508246846240999, 1e-8);
  EXPECT_NEAR(toDouble(gpULGroup.findKeyword("LookDirectionCamera")[2]),  0.96884464864118003, 1e-8);

  // remove upper left corner pvl group
  camptPvl.deleteGroup("GroundPoint");

  // validate upper right pixel
  // GroundPoint Group
  PvlGroup &gpURGroup = camptPvl.findGroup("GroundPoint", Pvl::Traverse);
  EXPECT_DOUBLE_EQ(double(gpURGroup["Sample"]), 1024.0);
  EXPECT_DOUBLE_EQ(double(gpURGroup["Line"]), 1.0);
  EXPECT_NEAR(double(gpURGroup["PixelValue"]), 0.019590021999999999, 1e-8);
  EXPECT_NEAR((double)gpURGroup.findKeyword("RightAscension"), 198.49295776143001, 1e-8);
  EXPECT_NEAR((double)gpURGroup.findKeyword("Declination"), 34.270334225111, 1e-8);
  EXPECT_NEAR((double)gpURGroup.findKeyword("PlanetocentricLatitude"), 52.367667037958, 1e-8);
  EXPECT_NEAR((double)gpURGroup.findKeyword("PlanetographicLatitude"), 59.033964585221, 1e-8);
  EXPECT_NEAR((double)gpURGroup.findKeyword("PositiveEast360Longitude"), 65.021639385019995, 1e-8);
  EXPECT_NEAR((double)gpURGroup.findKeyword("PositiveEast180Longitude"), 65.021639385019995, 1e-8);
  EXPECT_NEAR((double)gpURGroup.findKeyword("PositiveWest360Longitude"), 294.97836061497998, 1e-8);
  EXPECT_NEAR((double)gpURGroup.findKeyword("PositiveWest180Longitude"), -65.021639385019995, 1e-8);
  EXPECT_NEAR(toDouble(gpURGroup.findKeyword("BodyFixedCoordinate")[0]), 0.066459123254037994, 1e-8);
  EXPECT_NEAR(toDouble(gpURGroup.findKeyword("BodyFixedCoordinate")[1]), 0.14266269733567, 1e-8);
  EXPECT_NEAR(toDouble(gpURGroup.findKeyword("BodyFixedCoordinate")[2]),  0.20412780883011999, 1e-8);
  EXPECT_NEAR((double)gpURGroup.findKeyword("LocalRadius"), 257.75496622290001, 1e-8);
  EXPECT_NEAR((double)gpURGroup.findKeyword("SampleResolution"), 0.030119205908992999, 1e-8);
  EXPECT_NEAR((double)gpURGroup.findKeyword("LineResolution"), 0.030119205908992999, 1e-8);
  EXPECT_NEAR((double)gpURGroup.findKeyword("ObliqueDetectorResolution"), 0.031940014182285002, 1e-8);
  EXPECT_NEAR((double)gpURGroup.findKeyword("ObliquePixelResolution"), 0.031940014182285002, 1e-8);
  EXPECT_NEAR((double)gpURGroup.findKeyword("ObliqueLineResolution"), 0.031940014182285002, 1e-8);
  EXPECT_NEAR((double)gpURGroup.findKeyword("ObliqueSampleResolution"), 0.031940014182285002, 1e-8);
  EXPECT_NEAR(toDouble(gpURGroup.findKeyword("SpacecraftPosition")[0]), 0.10743294712690001, 1e-8);
  EXPECT_NEAR(toDouble(gpURGroup.findKeyword("SpacecraftPosition")[1]), 0.19408878947425001, 1e-8);
  EXPECT_NEAR(toDouble(gpURGroup.findKeyword("SpacecraftPosition")[2]), 0.26026876015417999, 1e-8);
  EXPECT_NEAR((double)gpURGroup.findKeyword("SpacecraftAzimuth"), 157.38320773039001, 1e-8);
  EXPECT_NEAR((double)gpURGroup.findKeyword("SlantDistance"), 0.086459838138757003, 1e-8);
  EXPECT_NEAR((double)gpURGroup.findKeyword("TargetCenterDistance"), 0.34198263675243001, 1e-8);
  EXPECT_NEAR((double)gpURGroup.findKeyword("SubSpacecraftLatitude"), 49.557593608158001, 1e-8);
  EXPECT_NEAR((double)gpURGroup.findKeyword("SubSpacecraftLongitude"), 61.034386896355997, 1e-8);
  EXPECT_NEAR((double)gpURGroup.findKeyword("SpacecraftAltitude"), 0.082907681748561002, 1e-8);
  EXPECT_NEAR((double)gpURGroup.findKeyword("OffNadirAngle"), 15.672041739717001, 1e-8);
  EXPECT_NEAR((double)gpURGroup.findKeyword("SubSpacecraftGroundAzimuth"), 223.34979952218001, 1e-8);
  EXPECT_NEAR(toDouble(gpURGroup.findKeyword("SunPosition")[0]), 173381543.43966001, 1e-5);
  EXPECT_NEAR(toDouble(gpURGroup.findKeyword("SunPosition")[1]), 75149827.507274002, 1e-5);
  EXPECT_NEAR(toDouble(gpURGroup.findKeyword("SunPosition")[2]), 7504735.7285030996, 1e-5);
  EXPECT_NEAR((double)gpURGroup.findKeyword("SubSolarAzimuth"), 151.02219421958, 1e-8);
  EXPECT_NEAR((double)gpURGroup.findKeyword("SolarDistance"), 1.264164423697, 1e-8);
  EXPECT_NEAR((double)gpURGroup.findKeyword("SubSolarLatitude"), 2.2742756940816, 1e-8);
  EXPECT_NEAR((double)gpURGroup.findKeyword("SubSolarLongitude"), 23.433651948527999, 1e-8);
  EXPECT_NEAR((double)gpURGroup.findKeyword("SubSolarGroundAzimuth"), 229.44198580279999, 1e-8);
  EXPECT_NEAR((double)gpURGroup.findKeyword("Phase"), 45.845109824169, 1e-8);
  EXPECT_NEAR((double)gpURGroup.findKeyword("Incidence"), 64.751791647299996, 1e-8);
  EXPECT_NEAR((double)gpURGroup.findKeyword("Emission"), 19.439596731908001, 1e-8);
  EXPECT_NEAR((double)gpURGroup.findKeyword("NorthAzimuth"), 21.711455490376, 1e-8);
  EXPECT_NEAR((double)gpURGroup.findKeyword("EphemerisTime"), 650456127.19964004, 1e-5);
  EXPECT_EQ(gpURGroup["UTC"][0].toStdString(), "2020-08-11T22:14:18.0166387");
  EXPECT_NEAR((double)gpURGroup.findKeyword("LocalSolarTime"), 14.772532495766001, 1e-8);
  EXPECT_NEAR((double)gpURGroup.findKeyword("SolarLongitude"), 112.19349952256999, 1e-8);

  // Look Direction Unit Vectors in Body Fixed, J2000, and Camera Coordinate Systems.
  EXPECT_NEAR(toDouble(gpURGroup.findKeyword("LookDirectionBodyFixed")[0]), -0.47390585912389999, 1e-8);
  EXPECT_NEAR(toDouble(gpURGroup.findKeyword("LookDirectionBodyFixed")[1]), -0.59479746025017, 1e-8);
  EXPECT_NEAR(toDouble(gpURGroup.findKeyword("LookDirectionBodyFixed")[2]),  -0.64932982217667001, 1e-8);

  EXPECT_NEAR(toDouble(gpURGroup.findKeyword("LookDirectionJ2000")[0]), -0.78371736894566002, 1e-8);
  EXPECT_NEAR(toDouble(gpURGroup.findKeyword("LookDirectionJ2000")[1]), -0.26212105678415998, 1e-8);
  EXPECT_NEAR(toDouble(gpURGroup.findKeyword("LookDirectionJ2000")[2]),  0.56309824826867005, 1e-8);

  EXPECT_NEAR(toDouble(gpURGroup.findKeyword("LookDirectionCamera")[0]), -0.17451569393964, 1e-8);
  EXPECT_NEAR(toDouble(gpURGroup.findKeyword("LookDirectionCamera")[1]), -0.17492772137842, 1e-8);
  EXPECT_NEAR(toDouble(gpURGroup.findKeyword("LookDirectionCamera")[2]),  0.96899151949960005, 1e-8);

  // remove upper right corner pvl group
  camptPvl.deleteGroup("GroundPoint");

  // validate lower right pixel
  // GroundPoint Group
  PvlGroup &gpLRGroup = camptPvl.findGroup("GroundPoint", Pvl::Traverse);
  EXPECT_DOUBLE_EQ(double(gpLRGroup["Sample"]), 1024.0);
  EXPECT_DOUBLE_EQ(double(gpLRGroup["Line"]), 1024.0);
  EXPECT_NEAR(double(gpLRGroup["PixelValue"]), 0.0014908130999999999, 1e-8);
  EXPECT_NEAR((double)gpLRGroup.findKeyword("RightAscension"), 174.31200488452001, 1e-8);
  EXPECT_NEAR((double)gpLRGroup.findKeyword("Declination"), 40.549363905867999, 1e-8);
  EXPECT_NEAR((double)gpLRGroup.findKeyword("PlanetocentricLatitude"), 54.435297799548003, 1e-8);
  EXPECT_NEAR((double)gpLRGroup.findKeyword("PlanetographicLatitude"), 60.905512734826999, 1e-8);
  EXPECT_NEAR((double)gpLRGroup.findKeyword("PositiveEast360Longitude"), 53.826055067016, 1e-8);
  EXPECT_NEAR((double)gpLRGroup.findKeyword("PositiveEast180Longitude"), 53.826055067016, 1e-8);
  EXPECT_NEAR((double)gpLRGroup.findKeyword("PositiveWest360Longitude"), 306.17394493298002, 1e-8);
  EXPECT_NEAR((double)gpLRGroup.findKeyword("PositiveWest180Longitude"), -53.826055067016, 1e-8);
  EXPECT_NEAR(toDouble(gpLRGroup.findKeyword("BodyFixedCoordinate")[0]), 0.088406739132171994, 1e-8);
  EXPECT_NEAR(toDouble(gpLRGroup.findKeyword("BodyFixedCoordinate")[1]), 0.12090781767354999, 1e-8);
  EXPECT_NEAR(toDouble(gpLRGroup.findKeyword("BodyFixedCoordinate")[2]), 0.20948490977763001, 1e-8);
  EXPECT_NEAR((double)gpLRGroup.findKeyword("LocalRadius"), 257.52355100671002, 1e-8);
  EXPECT_NEAR((double)gpLRGroup.findKeyword("SampleResolution"), 0.031730379572807998, 1e-8);
  EXPECT_NEAR((double)gpLRGroup.findKeyword("LineResolution"), 0.031730379572807998, 1e-8);
  EXPECT_NEAR((double)gpLRGroup.findKeyword("ObliqueDetectorResolution"), 0.036133840581173998, 1e-8);
  EXPECT_NEAR((double)gpLRGroup.findKeyword("ObliquePixelResolution"), 0.036133840581173998, 1e-8);
  EXPECT_NEAR((double)gpLRGroup.findKeyword("ObliqueLineResolution"), 0.036133840581173998, 1e-8);
  EXPECT_NEAR((double)gpLRGroup.findKeyword("ObliqueSampleResolution"), 0.036133840581173998, 1e-8);
  EXPECT_NEAR(toDouble(gpLRGroup.findKeyword("SpacecraftPosition")[0]), 0.10743294712690001, 1e-8);
  EXPECT_NEAR(toDouble(gpLRGroup.findKeyword("SpacecraftPosition")[1]), 0.19408878947425001, 1e-8);
  EXPECT_NEAR(toDouble(gpLRGroup.findKeyword("SpacecraftPosition")[2]), 0.26026876015417999, 1e-8);
  EXPECT_NEAR((double)gpLRGroup.findKeyword("SpacecraftAzimuth"), 236.9429080301, 1e-8);
  EXPECT_NEAR((double)gpLRGroup.findKeyword("SlantDistance"), 0.091084854303120993, 1e-8);
  EXPECT_NEAR((double)gpLRGroup.findKeyword("TargetCenterDistance"), 0.34198263675243001, 1e-8);
  EXPECT_NEAR((double)gpLRGroup.findKeyword("SubSpacecraftLatitude"), 49.557593608158001, 1e-8);
  EXPECT_NEAR((double)gpLRGroup.findKeyword("SubSpacecraftLongitude"), 61.034386896355997, 1e-8);
  EXPECT_NEAR((double)gpLRGroup.findKeyword("SpacecraftAltitude"), 0.082907681748561002, 1e-8);
  EXPECT_NEAR((double)gpLRGroup.findKeyword("OffNadirAngle"), 21.993698903517998, 1e-8);
  EXPECT_NEAR((double)gpLRGroup.findKeyword("SubSpacecraftGroundAzimuth"), 134.81055570525999, 1e-8);
  EXPECT_NEAR(toDouble(gpLRGroup.findKeyword("SunPosition")[0]), 173381543.43966001, 1e-5);
  EXPECT_NEAR(toDouble(gpLRGroup.findKeyword("SunPosition")[1]), 75149827.507274002, 1e-5);
  EXPECT_NEAR(toDouble(gpLRGroup.findKeyword("SunPosition")[2]), 7504735.7285030996, 1e-5);
  EXPECT_NEAR((double)gpLRGroup.findKeyword("SubSolarAzimuth"), 153.97800288957001, 1e-8);
  EXPECT_NEAR((double)gpLRGroup.findKeyword("SolarDistance"), 1.2641644236189, 1e-8);
  EXPECT_NEAR((double)gpLRGroup.findKeyword("SubSolarLatitude"), 2.2742756940816, 1e-8);
  EXPECT_NEAR((double)gpLRGroup.findKeyword("SubSolarLongitude"), 23.433651948527999, 1e-8);
  EXPECT_NEAR((double)gpLRGroup.findKeyword("SubSolarGroundAzimuth"), 216.70677474211999, 1e-8);
  EXPECT_NEAR((double)gpLRGroup.findKeyword("Phase"), 57.798742629012999, 1e-8);
  EXPECT_NEAR((double)gpLRGroup.findKeyword("Incidence"), 62.540336988627999, 1e-8);
  EXPECT_NEAR((double)gpLRGroup.findKeyword("Emission"), 28.581833397705001, 1e-8);
  EXPECT_NEAR((double)gpLRGroup.findKeyword("NorthAzimuth"), 8.3819300079138994, 1e-8);
  EXPECT_NEAR((double)gpLRGroup.findKeyword("EphemerisTime"), 650456127.19964004, 1e-5);
  EXPECT_EQ(gpLRGroup["UTC"][0].toStdString(), "2020-08-11T22:14:18.0166387");
  EXPECT_NEAR((double)gpLRGroup.findKeyword("LocalSolarTime"), 14.026160207899, 1e-8);
  EXPECT_NEAR((double)gpLRGroup.findKeyword("SolarLongitude"), 112.19349952256999, 1e-8);

  // Look Direction Unit Vectors in Body Fixed, J2000, and Camera Coordinate Systems.
  EXPECT_NEAR(toDouble(gpLRGroup.findKeyword("LookDirectionBodyFixed")[0]), -0.20888443133925999, 1e-8);
  EXPECT_NEAR(toDouble(gpLRGroup.findKeyword("LookDirectionBodyFixed")[1]), -0.80343732622283004, 1e-8);
  EXPECT_NEAR(toDouble(gpLRGroup.findKeyword("LookDirectionBodyFixed")[2]), -0.55754439928671995, 1e-8);

  EXPECT_NEAR(toDouble(gpLRGroup.findKeyword("LookDirectionJ2000")[0]), -0.75610493290724001, 1e-8);
  EXPECT_NEAR(toDouble(gpLRGroup.findKeyword("LookDirectionJ2000")[1]), 0.075309307739164, 1e-8);
  EXPECT_NEAR(toDouble(gpLRGroup.findKeyword("LookDirectionJ2000")[2]), 0.65010294461815998, 1e-8);

  EXPECT_NEAR(toDouble(gpLRGroup.findKeyword("LookDirectionCamera")[0]), -0.17453974693652999, 1e-8);
  EXPECT_NEAR(toDouble(gpLRGroup.findKeyword("LookDirectionCamera")[1]), 0.17463130328074, 1e-8);
  EXPECT_NEAR(toDouble(gpLRGroup.findKeyword("LookDirectionCamera")[2]), 0.96904065170343001, 1e-8);

  // remove lower right corner pvl group
  camptPvl.deleteGroup("GroundPoint");

  // validate lower left pixel
  // GroundPoint Group
  PvlGroup &gpLLGroup = camptPvl.findGroup("GroundPoint", Pvl::Traverse);
  EXPECT_DOUBLE_EQ(double(gpLLGroup["Sample"]), 1.0);
  EXPECT_DOUBLE_EQ(double(gpLLGroup["Line"]), 1024.0);
  EXPECT_NEAR(double(gpLLGroup["PixelValue"]), 0.0051967328999999998, 1e-8);
  EXPECT_NEAR((double)gpLLGroup.findKeyword("RightAscension"), 180.27848467792001, 1e-8);
  EXPECT_NEAR((double)gpLLGroup.findKeyword("Declination"), 60.349865085462, 1e-8);
  EXPECT_NEAR((double)gpLLGroup.findKeyword("PlanetocentricLatitude"), 47.584454638079002, 1e-8);
  EXPECT_NEAR((double)gpLLGroup.findKeyword("PlanetographicLatitude"), 54.585191891701001, 1e-8);
  EXPECT_NEAR((double)gpLLGroup.findKeyword("PositiveEast360Longitude"), 52.206916399613, 1e-8);
  EXPECT_NEAR((double)gpLLGroup.findKeyword("PositiveEast180Longitude"), 52.206916399613, 1e-8);
  EXPECT_NEAR((double)gpLLGroup.findKeyword("PositiveWest360Longitude"), 307.79308360038999, 1e-8);
  EXPECT_NEAR((double)gpLLGroup.findKeyword("PositiveWest180Longitude"), -52.206916399613, 1e-8);
  EXPECT_NEAR(toDouble(gpLLGroup.findKeyword("BodyFixedCoordinate")[0]), 0.10768048862273, 1e-8);
  EXPECT_NEAR(toDouble(gpLLGroup.findKeyword("BodyFixedCoordinate")[1]), 0.13885545715365999, 1e-8);
  EXPECT_NEAR(toDouble(gpLLGroup.findKeyword("BodyFixedCoordinate")[2]), 0.19232817011357001, 1e-8);
  EXPECT_NEAR((double)gpLLGroup.findKeyword("LocalRadius"), 260.51113341011001, 1e-8);
  EXPECT_NEAR((double)gpLLGroup.findKeyword("SampleResolution"), 0.030502364428718998, 1e-8);
  EXPECT_NEAR((double)gpLLGroup.findKeyword("LineResolution"), 0.030502364428718998, 1e-8);
  EXPECT_NEAR((double)gpLLGroup.findKeyword("ObliqueDetectorResolution"), 0.032867282057289997, 1e-8);
  EXPECT_NEAR((double)gpLLGroup.findKeyword("ObliquePixelResolution"),  0.032867282057289997, 1e-8);
  EXPECT_NEAR((double)gpLLGroup.findKeyword("ObliqueLineResolution"), 0.032867282057289997, 1e-8);
  EXPECT_NEAR((double)gpLLGroup.findKeyword("ObliqueSampleResolution"), 0.032867282057289997, 1e-8);
  EXPECT_NEAR(toDouble(gpLLGroup.findKeyword("SpacecraftPosition")[0]), 0.10743294712690001, 1e-8);
  EXPECT_NEAR(toDouble(gpLLGroup.findKeyword("SpacecraftPosition")[1]), 0.19408878947425001, 1e-8);
  EXPECT_NEAR(toDouble(gpLLGroup.findKeyword("SpacecraftPosition")[2]), 0.26026876015417999, 1e-8);
  EXPECT_NEAR((double)gpLLGroup.findKeyword("SpacecraftAzimuth"), 301.43101778467002, 1e-8);
  EXPECT_NEAR((double)gpLLGroup.findKeyword("SlantDistance"), 0.087559728477734994, 1e-8);
  EXPECT_NEAR((double)gpLLGroup.findKeyword("TargetCenterDistance"), 0.34198263675243001, 1e-8);
  EXPECT_NEAR((double)gpLLGroup.findKeyword("SubSpacecraftLatitude"), 49.557593608158001, 1e-8);
  EXPECT_NEAR((double)gpLLGroup.findKeyword("SubSpacecraftLongitude"), 61.034386896355997, 1e-8);
  EXPECT_NEAR((double)gpLLGroup.findKeyword("SpacecraftAltitude"), 0.082907681748561002, 1e-8);
  EXPECT_NEAR((double)gpLLGroup.findKeyword("OffNadirAngle"), 15.706768952824, 1e-8);
  EXPECT_NEAR((double)gpLLGroup.findKeyword("SubSpacecraftGroundAzimuth"), 68.057343513310002, 1e-8);
  EXPECT_NEAR(toDouble(gpLLGroup.findKeyword("SunPosition")[0]), 173381543.43966001, 1e-5);
  EXPECT_NEAR(toDouble(gpLLGroup.findKeyword("SunPosition")[1]), 75149827.507274002, 1e-5);
  EXPECT_NEAR(toDouble(gpLLGroup.findKeyword("SunPosition")[2]), 7504735.7285030996, 1e-5);
  EXPECT_NEAR((double)gpLLGroup.findKeyword("SubSolarAzimuth"), 154.57933226068999, 1e-8);
  EXPECT_NEAR((double)gpLLGroup.findKeyword("SolarDistance"), 1.2641644234575999, 1e-8);
  EXPECT_NEAR((double)gpLLGroup.findKeyword("SubSolarLatitude"), 2.2742756940816, 1e-8);
  EXPECT_NEAR((double)gpLLGroup.findKeyword("SubSolarLongitude"), 23.433651948527999, 1e-8);
  EXPECT_NEAR((double)gpLLGroup.findKeyword("SubSolarGroundAzimuth"), 217.80959281623001, 1e-8);
  EXPECT_NEAR((double)gpLLGroup.findKeyword("Phase"), 73.807471406353002, 1e-8);
  EXPECT_NEAR((double)gpLLGroup.findKeyword("Incidence"), 56.936857115134998, 1e-8);
  EXPECT_NEAR((double)gpLLGroup.findKeyword("Emission"), 21.867688503058002, 1e-8);
  EXPECT_NEAR((double)gpLLGroup.findKeyword("NorthAzimuth"), 11.918468304929, 1e-8);
  EXPECT_NEAR((double)gpLLGroup.findKeyword("EphemerisTime"), 650456127.19964004, 1e-5);
  EXPECT_EQ(gpLLGroup["UTC"][0].toStdString(), "2020-08-11T22:14:18.0166387");
  EXPECT_NEAR((double)gpLLGroup.findKeyword("LocalSolarTime"), 13.918217630072, 1e-8);
  EXPECT_NEAR((double)gpLLGroup.findKeyword("SolarLongitude"), 112.19349952256999, 1e-8);

  // Look Direction Unit Vectors in Body Fixed, J2000, and Camera Coordinate Systems.
  EXPECT_NEAR(toDouble(gpLLGroup.findKeyword("LookDirectionBodyFixed")[0]), 0.0028271158458317998, 1e-8);
  EXPECT_NEAR(toDouble(gpLLGroup.findKeyword("LookDirectionBodyFixed")[1]), -0.63080748742419002, 1e-8);
  EXPECT_NEAR(toDouble(gpLLGroup.findKeyword("LookDirectionBodyFixed")[2]), -0.77593422480619001, 1e-8);

  EXPECT_NEAR(toDouble(gpLLGroup.findKeyword("LookDirectionJ2000")[0]), -0.49469665879427999, 1e-8);
  EXPECT_NEAR(toDouble(gpLLGroup.findKeyword("LookDirectionJ2000")[1]), -0.0024044794527672002, 1e-8);
  EXPECT_NEAR(toDouble(gpLLGroup.findKeyword("LookDirectionJ2000")[2]), 0.86906238801155, 1e-8);

  EXPECT_NEAR(toDouble(gpLLGroup.findKeyword("LookDirectionCamera")[0]), 0.17519859939577001, 1e-8);
  EXPECT_NEAR(toDouble(gpLLGroup.findKeyword("LookDirectionCamera")[1]), 0.17478591825758, 1e-8);
  EXPECT_NEAR(toDouble(gpLLGroup.findKeyword("LookDirectionCamera")[2]), 0.96889387114823999, 1e-8);

  // remove lower left corner pvl group
  camptPvl.deleteGroup("GroundPoint");

  // validate center pixel
  // GroundPoint Group
  PvlGroup &gpCTRGroup = camptPvl.findGroup("GroundPoint", Pvl::Traverse);
  EXPECT_DOUBLE_EQ(double(gpCTRGroup["Sample"]), 512.0);
  EXPECT_DOUBLE_EQ(double(gpCTRGroup["Line"]), 512.0);
  EXPECT_NEAR(double(gpCTRGroup["PixelValue"]), 0.0043593375000000002, 1e-8);
  EXPECT_NEAR((double)gpCTRGroup.findKeyword("RightAscension"), 191.71210958578999, 1e-8);
  EXPECT_NEAR((double)gpCTRGroup.findKeyword("Declination"), 47.643033700765997, 1e-8);
  EXPECT_NEAR((double)gpCTRGroup.findKeyword("PlanetocentricLatitude"), 50.146656833644997, 1e-8);
  EXPECT_NEAR((double)gpCTRGroup.findKeyword("PlanetographicLatitude"), 56.989642244050003, 1e-8);
  EXPECT_NEAR((double)gpCTRGroup.findKeyword("PositiveEast360Longitude"), 58.342021411841998, 1e-8);
  EXPECT_NEAR((double)gpCTRGroup.findKeyword("PositiveEast180Longitude"), 58.342021411841998, 1e-8);
  EXPECT_NEAR((double)gpCTRGroup.findKeyword("PositiveWest360Longitude"), 301.65797858816001, 1e-8);
  EXPECT_NEAR((double)gpCTRGroup.findKeyword("PositiveWest180Longitude"), -58.342021411841998, 1e-8);
  EXPECT_NEAR(toDouble(gpCTRGroup.findKeyword("BodyFixedCoordinate")[0]), 0.087110746446919005, 1e-8);
  EXPECT_NEAR(toDouble(gpCTRGroup.findKeyword("BodyFixedCoordinate")[1]), 0.14127597097733, 1e-8);
  EXPECT_NEAR(toDouble(gpCTRGroup.findKeyword("BodyFixedCoordinate")[2]), 0.19883080282141, 1e-8);
  EXPECT_NEAR((double)gpCTRGroup.findKeyword("LocalRadius"), 258.99974956113999, 1e-8);
  EXPECT_NEAR((double)gpCTRGroup.findKeyword("SampleResolution"), 0.029097621650989999, 1e-8);
  EXPECT_NEAR((double)gpCTRGroup.findKeyword("LineResolution"), 0.029097621650989999, 1e-8);
  EXPECT_NEAR((double)gpCTRGroup.findKeyword("ObliqueDetectorResolution"), 0.029505624417934001, 1e-8);
  EXPECT_NEAR((double)gpCTRGroup.findKeyword("ObliquePixelResolution"), 0.029505624417934001, 1e-8);
  EXPECT_NEAR((double)gpCTRGroup.findKeyword("ObliqueLineResolution"), 0.029505624417934001, 1e-8);
  EXPECT_NEAR((double)gpCTRGroup.findKeyword("ObliqueSampleResolution"), 0.029505624417934001, 1e-8);
  EXPECT_NEAR(toDouble(gpCTRGroup.findKeyword("SpacecraftPosition")[0]), 0.10743294712690001, 1e-8);
  EXPECT_NEAR(toDouble(gpCTRGroup.findKeyword("SpacecraftPosition")[1]), 0.19408878947425001, 1e-8);
  EXPECT_NEAR(toDouble(gpCTRGroup.findKeyword("SpacecraftPosition")[2]), 0.26026876015417999, 1e-8);
  EXPECT_NEAR((double)gpCTRGroup.findKeyword("SpacecraftAzimuth"), 267.08409761706002, 1e-8);
  EXPECT_NEAR((double)gpCTRGroup.findKeyword("SlantDistance"), 0.083527290386371994, 1e-8);
  EXPECT_NEAR((double)gpCTRGroup.findKeyword("TargetCenterDistance"), 0.34198263675243001, 1e-8);
  EXPECT_NEAR((double)gpCTRGroup.findKeyword("SubSpacecraftLatitude"), 49.557593608158001, 1e-8);
  EXPECT_NEAR((double)gpCTRGroup.findKeyword("SubSpacecraftLongitude"), 61.034386896355997, 1e-8);
  EXPECT_NEAR((double)gpCTRGroup.findKeyword("SpacecraftAltitude"), 0.082907681748561002, 1e-8);
  EXPECT_NEAR((double)gpCTRGroup.findKeyword("OffNadirAngle"), 7.7063113038062996, 1e-8);
  EXPECT_NEAR((double)gpCTRGroup.findKeyword("SubSpacecraftGroundAzimuth"), 107.71147351473, 1e-8);
  EXPECT_NEAR(toDouble(gpCTRGroup.findKeyword("SunPosition")[0]), 173381543.43966001, 1e-5);
  EXPECT_NEAR(toDouble(gpCTRGroup.findKeyword("SunPosition")[1]), 75149827.507274002, 1e-5);
  EXPECT_NEAR(toDouble(gpCTRGroup.findKeyword("SunPosition")[2]), 7504735.7285030996, 1e-5);
  EXPECT_NEAR((double)gpCTRGroup.findKeyword("SubSolarAzimuth"), 151.53939253928999, 1e-8);
  EXPECT_NEAR((double)gpCTRGroup.findKeyword("SolarDistance"), 1.2641644235755001, 1e-8);
  EXPECT_NEAR((double)gpCTRGroup.findKeyword("SubSolarLatitude"), 2.2742756940816, 1e-8);
  EXPECT_NEAR((double)gpCTRGroup.findKeyword("SubSolarLongitude"), 23.433651948527999, 1e-8);
  EXPECT_NEAR((double)gpCTRGroup.findKeyword("SubSolarGroundAzimuth"), 223.44958120455999, 1e-8);
  EXPECT_NEAR((double)gpCTRGroup.findKeyword("Phase"), 59.768293345121002, 1e-8);
  EXPECT_NEAR((double)gpCTRGroup.findKeyword("Incidence"), 60.890338972278997, 1e-8);
  EXPECT_NEAR((double)gpCTRGroup.findKeyword("Emission"), 9.5393438584533001, 1e-8);
  EXPECT_NEAR((double)gpCTRGroup.findKeyword("NorthAzimuth"), 14.652172109042001, 1e-8);
  EXPECT_NEAR((double)gpCTRGroup.findKeyword("EphemerisTime"), 650456127.19964004, 1e-5);
  EXPECT_EQ(gpCTRGroup["UTC"][0].toStdString(), "2020-08-11T22:14:18.0166387");
  EXPECT_NEAR((double)gpCTRGroup.findKeyword("LocalSolarTime"), 14.327224630888001, 1e-8);
  EXPECT_NEAR((double)gpCTRGroup.findKeyword("SolarLongitude"), 112.19349952256999, 1e-8);

  // Look Direction Unit Vectors in Body Fixed, J2000, and Camera Coordinate Systems.
  EXPECT_NEAR(toDouble(gpCTRGroup.findKeyword("LookDirectionBodyFixed")[0]), -0.24330013084317001, 1e-8);
  EXPECT_NEAR(toDouble(gpCTRGroup.findKeyword("LookDirectionBodyFixed")[1]), -0.63228219486854997, 1e-8);
  EXPECT_NEAR(toDouble(gpCTRGroup.findKeyword("LookDirectionBodyFixed")[2]), -0.73554352174695004, 1e-8);

  EXPECT_NEAR(toDouble(gpCTRGroup.findKeyword("LookDirectionJ2000")[0]), -0.65972008671999005, 1e-8);
  EXPECT_NEAR(toDouble(gpCTRGroup.findKeyword("LookDirectionJ2000")[1]), -0.13676688146457, 1e-8);
  EXPECT_NEAR(toDouble(gpCTRGroup.findKeyword("LookDirectionJ2000")[2]), 0.73896158716988003, 1e-8);

  EXPECT_NEAR(toDouble(gpCTRGroup.findKeyword("LookDirectionCamera")[0]), 0.00034836119687148298, 1e-8);
  EXPECT_NEAR(toDouble(gpCTRGroup.findKeyword("LookDirectionCamera")[1]), -0.00034836084791695099, 1e-8);
  EXPECT_NEAR(toDouble(gpCTRGroup.findKeyword("LookDirectionCamera")[2]), 0.99999987864460005, 1e-8);
}


/**
 * SamCamModuleTwoImageTest
 *
 *  1) Two overlapping SamCam images (fits format) are ingested with ocams2isis. Each is
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
 * 11) The test is validated on output from mosrange, automos, bandtrim, noseam,
 *     overlapstats, autoseed, and pointreg.
 *
 * INPUT: two samcam images in fits format
 *          - isis/tests/data/osirisRexImages/ocams/20200811T221418S029_sam_iofL2pan5.fits
 *          - isis/tests/data/osirisRexImages/ocams/20200811T221518S391_sam_iofL2pan5.fits
 *
 * OUTPUT: 1) ocams2isis
 *            - 20200811T221418S029_sam_iofL2pan5.cub
 *            - 20200811T221518S391_sam_iofL2pan5.cub
 *         2) mosrange
 *            - polycam_equi.map
 *         3) cam2map
 *            - 20200811T221418S029_sam_iofL2pan5_map.cub
 *            - 20200811T221518S391_sam_iofL2pan5_map.cub
 *         4) automos
 *            - mosaicUncontrolled.cub
 *         5) bandtrim
 *            - mosaicTrimmedUncontrolled.cub
 *         6) noseam
 *            - noseamUncontrolledMosaic.cub
 *         7) findimageoverlaps
 *            - samcam_findimageoverlaps.txt
 *         8) autoseed
 *            - samcam_autoseed.net
 *         9) pointreg
 *            - samcam_pointreg.net
 */
  TEST(OsirisRexSamCamModules, SamCamModuleTwoImageTest) {
  QTemporaryDir tempDir;

  // ingest 1st samcam fits format image, then spiceinit and footprintinit
  QString samCam1FileName = tempDir.path() + "/20200811T221418S029_sam_iofL2pan5.cub";
  QVector<QString> ocams2isisArgs1 = {"from=data/osirisRexImages/ocams/20200811T221418S029_sam_iofL2pan5.fits",
                                         "to=" + samCam1FileName};

  UserInterface ocams2isisSamCam1UI(OCAMS2ISIS_XML, ocams2isisArgs1);
  try {
    ocams2isis(ocams2isisSamCam1UI);
  }
  catch (IException &e) {
    FAIL() << "Unable to run ocams2isis on first samcam fits file 20200811T221418S029_sam_iofL2pan5.fits: "
           << e.what() << std::endl;
  }

  QVector<QString> spiceinitArgs1 = {"from=" + samCam1FileName};
  UserInterface spiceinitSamCam1UI(SPICEINIT_XML, spiceinitArgs1);
  try {
    spiceinit(spiceinitSamCam1UI);
  }
  catch (IException &e) {
    FAIL() << "Unable to run spiceinit on first samcam cube 20200811T221418S029_sam_iofL2pan5.cub: "
           << e.what() << std::endl;
  }

  QVector<QString> footprintinitArgs1 = {"from=" + samCam1FileName};
  UserInterface footprintinitSamCam1UI(FOOTPRINTINIT_XML, footprintinitArgs1);
  try {
    footprintinit(footprintinitSamCam1UI);
  }
  catch (IException &e) {
    FAIL() << "Unable to run footprintinit on first samcam cube 20200811T221418S029_sam_iofL2pan5.cub: "
           << e.what() << std::endl;
  }

  // ingest 2nd samcam fits format image, then spiceinit and footprintinit
  QString samCam2FileName = tempDir.path() + "/20200811T221518S391_sam_iofL2pan5.cub";
  QVector<QString> ocams2isisArgs2 = {"from=data/osirisRexImages/ocams/20200811T221518S391_sam_iofL2pan5.fits",
                                         "to=" + samCam2FileName};

  UserInterface ocams2isisSamCam2UI(OCAMS2ISIS_XML, ocams2isisArgs2);
  try {
    ocams2isis(ocams2isisSamCam2UI);
  }
  catch (IException &e) {
    FAIL() << "Unable to run ocams2isis on second samcam fits file 20200811T221518S391_sam_iofL2pan5.fits: "
           << e.what() << std::endl;
  }

  QVector<QString> spiceinitArgs2 = {"from=" + samCam2FileName};
  UserInterface spiceinitSamCam2(SPICEINIT_XML, spiceinitArgs2);
  try {
    spiceinit(spiceinitSamCam2);
  }
  catch (IException &e) {
    FAIL() << "Unable to run spiceinit on second samcam cube 20200811T221518S391_sam_iofL2pan5.cub: "
           << e.what() << std::endl;
  }

  // footprintinit second samcam image
  QVector<QString> footprintinitArgs2 = {"from=" + samCam2FileName};
  UserInterface footprintinitSamCam2UI(FOOTPRINTINIT_XML, footprintinitArgs2);
  try {
    footprintinit(footprintinitSamCam2UI);
  }
  catch (IException &e) {
    FAIL() << "Unable to run footprintinit on second samcam cube 20200811T221518S391_sam_iofL2pan5.cub: "
           << e.what() << std::endl;
  }

  // compute the lat/lon range of the two images for mosaicking with mosrange
  FileList *cubeList = new FileList();
  cubeList->append(samCam1FileName);
  cubeList->append(samCam2FileName);

  QString cubeListFile = tempDir.path() + "/unprojectedSpiceinitCubeList.lis";
  cubeList->write(cubeListFile);

  QString equiMapFile = tempDir.path() + "/samcam_equi.map";

  QVector<QString> mosrangeArgs = {"fromlist=" + cubeListFile,
                                   "to=" + equiMapFile,
                                   "precision=6",
                                   "projection=Equirectangular"};
  UserInterface mosrangeUI(MOSRANGE_XML, mosrangeArgs);
  try {
    mosrange(mosrangeUI);
  }
  catch (IException &e) {
    FAIL() << "Unable to run mosrange on samcam images: " << e.what() << std::endl;
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
  EXPECT_NEAR((double)mappingGroup.findKeyword("PixelResolution"), 0.03049, 1e-5);
  EXPECT_NEAR((double)mappingGroup.findKeyword("Scale"), 162.03252900000001, 1e-8);
  EXPECT_NEAR((double)mappingGroup.findKeyword("MinPixelResolution"), 0.028881774379621999, 1e-8);
  EXPECT_NEAR((double)mappingGroup.findKeyword("MaxPixelResolution"), 0.032098773989488003, 1e-8);
  EXPECT_NEAR((double)mappingGroup.findKeyword("MinObliquePixelResolution"), 0.029003424829951999, 1e-8);
  EXPECT_NEAR((double)mappingGroup.findKeyword("MaxObliquePixelResolution"), 0.036540307813724998, 1e-8);
  EXPECT_NEAR((double)mappingGroup.findKeyword("CenterLongitude"), 59.111224, 1e-5);
  EXPECT_NEAR((double)mappingGroup.findKeyword("CenterLatitude"), 50.546586, 1e-5);
  EXPECT_NEAR((double)mappingGroup.findKeyword("MinimumLatitude"), 46.147973, 1e-5);
  EXPECT_NEAR((double)mappingGroup.findKeyword("MaximumLatitude"), 54.945199, 1e-5);
  EXPECT_NEAR((double)mappingGroup.findKeyword("MinimumLongitude"), 52.200801, 1e-5);
  EXPECT_NEAR((double)mappingGroup.findKeyword("MaximumLongitude"), 66.021648, 1e-5);
  EXPECT_NEAR((double)mappingGroup.findKeyword("PreciseCenterLongitude"), 59.111224337358003, 1e-8);
  EXPECT_NEAR((double)mappingGroup.findKeyword("PreciseCenterLatitude"), 50.546585896819003, 1e-8);
  EXPECT_NEAR((double)mappingGroup.findKeyword("PreciseMinimumLatitude"), 46.147973431243997, 1e-8);
  EXPECT_NEAR((double)mappingGroup.findKeyword("PreciseMaximumLatitude"), 54.945198362394997, 1e-8);
  EXPECT_NEAR((double)mappingGroup.findKeyword("PreciseMinimumLongitude"), 52.200801469285999, 1e-8);
  EXPECT_NEAR((double)mappingGroup.findKeyword("PreciseMaximumLongitude"), 66.02164720543, 1e-8);

  // project the two images using the equirectangular map from mosrange
  QVector<QString> cam2mapArgs1 = {"from=" + samCam1FileName,
                                   "to=" + tempDir.path() + "/20200811T221418S029_sam_iofL2pan5_map.cub",
                                   "map=" + tempDir.path() + "/samcam_equi.map",
                                   "pixres=map"};
  UserInterface cam2map1UI(CAM2MAP_XML, cam2mapArgs1);
  try {
    cam2map(cam2map1UI);
  }
  catch (IException &e) {
    FAIL() << "Unable to run cam2map on first samcam cube 20200811T221418S029_sam_iofL2pan5.cub: "
           << e.what() << std::endl;
  }

  QVector<QString> cam2mapArgs2 = {"from=" + samCam2FileName,
                                   "to=" + tempDir.path() + "/20200811T221518S391_sam_iofL2pan5_map.cub",
                                   "map=" + tempDir.path() + "/samcam_equi.map",
                                   "pixres=map"};
  UserInterface cam2map2UI(CAM2MAP_XML, cam2mapArgs2);
  try {
    cam2map(cam2map2UI);
  }
  catch (IException &e) {
    FAIL() << "Unable to run cam2map on second samcam cube 20200811T221518S391_sam_iofL2pan5.cub: "
           << e.what() << std::endl;
  }

  // create uncontrolled mosaic of the two equirectangular projected samcam images
  FileList *projectedCubeList = new FileList();
  projectedCubeList->append(tempDir.path() + "/20200811T221418S029_sam_iofL2pan5_map.cub");
  projectedCubeList->append(tempDir.path() + "/20200811T221518S391_sam_iofL2pan5_map.cub");

  QString projectedCubeListFile = tempDir.path() + "/projectedCubeList.lis";
  projectedCubeList->write(projectedCubeListFile);

  QVector<QString> automosArgs = {"fromlist=" + projectedCubeListFile,
                                  "mosaic=" + tempDir.path() + "/mosaicUncontrolled.cub"};
  UserInterface automosUI(AUTOMOS_XML, automosArgs);
  try {
    automos(automosUI);
  }
  catch (IException &e) {
    FAIL() << "Unable to run automos on polycam images: " << e.what() << std::endl;
  }

  // validate mapping label and histogram of uncontrolled mosaic cube
  Cube mosaic(tempDir.path() + "/mosaicUncontrolled.cub");

  PvlGroup &mosaicMapGroup = (mosaic.label())->findObject("IsisCube").findGroup("Mapping");
  EXPECT_PRED_FORMAT2(AssertQStringsEqual, (QString)mosaicMapGroup.findKeyword("ProjectionName"), "Equirectangular");
  EXPECT_NEAR((double)mosaicMapGroup.findKeyword("CenterLongitude"), 59.111224, 1e-5);
  EXPECT_PRED_FORMAT2(AssertQStringsEqual, (QString)mosaicMapGroup.findKeyword("TargetName"), "Bennu");
  EXPECT_DOUBLE_EQ((double)mosaicMapGroup.findKeyword("EquatorialRadius"), 283.065);
  EXPECT_DOUBLE_EQ((double)mosaicMapGroup.findKeyword("PolarRadius"), 249.72);
  EXPECT_EQ(mosaicMapGroup.findKeyword("LatitudeType")[0], "Planetocentric");
  EXPECT_EQ(mosaicMapGroup.findKeyword("LongitudeDirection")[0], "PositiveEast");
  EXPECT_EQ((int)mosaicMapGroup.findKeyword("LongitudeDomain"), 360);
  EXPECT_NEAR((double)mosaicMapGroup.findKeyword("MinimumLatitude"), 46.147973431243997, 1e-8);
  EXPECT_NEAR((double)mosaicMapGroup.findKeyword("MaximumLatitude"), 54.945198362394997, 1e-8);
  EXPECT_NEAR((double)mosaicMapGroup.findKeyword("MinimumLongitude"), 52.200801469285999, 1e-8);
  EXPECT_NEAR((double)mosaicMapGroup.findKeyword("MaximumLongitude"), 66.02164720543, 1e-8);
  EXPECT_NEAR((double)mosaicMapGroup.findKeyword("UpperLeftCornerX"), -20.062420, 1e-5);
  EXPECT_NEAR((double)mosaicMapGroup.findKeyword("UpperLeftCornerY"), 250.99368, 1e-5);
  EXPECT_NEAR((double)mosaicMapGroup.findKeyword("PixelResolution"), 0.03049, 1e-5);
  EXPECT_NEAR((double)mosaicMapGroup.findKeyword("Scale"), 149.80996909744999, 1e-8);
  EXPECT_NEAR((double)mosaicMapGroup.findKeyword("CenterLatitude"), 50.546586, 1e-5);
  EXPECT_NEAR((double)mosaicMapGroup.findKeyword("CenterLatitudeRadius"), 261.71027343763001, 1e-8);

  std::unique_ptr<Histogram> mosaicHist(mosaic.histogram());
  EXPECT_NEAR(mosaicHist->Average(), 0.010375299676988539, 1e-8);
  EXPECT_NEAR(mosaicHist->Sum(), 11976.426298441087, 1e-5);
  EXPECT_EQ(mosaicHist->ValidPixels(), 1154321);
  EXPECT_NEAR(mosaicHist->StandardDeviation(), 0.010383565236007088, 1e-8);

  // band trim automos mosaic
  QVector<QString> bandtrimArgs = {"from=" + tempDir.path() + "/mosaicUncontrolled.cub",
                                 "to=" + tempDir.path() + "/mosaicTrimmedUncontrolled.cub"};
  UserInterface bandtrimUI(BANDTRIM_XML, bandtrimArgs);
  try {
    bandtrim(bandtrimUI);
  }
  catch (IException &e) {
    FAIL() << "Unable to run bandtrim on samcam 2 image mosaic: " << e.what() << std::endl;
  }

  // validate histogram of trimmed, uncontrolled noseam mosaic cube
  Cube trimmedUncontrolledMosaic(tempDir.path() + "/mosaicTrimmedUncontrolled.cub");

  std::unique_ptr<Histogram> trimmedUncontrolledMosaicHist(trimmedUncontrolledMosaic.histogram());
  EXPECT_NEAR(trimmedUncontrolledMosaicHist->Average(), 0.010375299676988539, 1e-8);
  EXPECT_NEAR(trimmedUncontrolledMosaicHist->Sum(), 11976.426298441087, 1e-8);
  EXPECT_EQ(trimmedUncontrolledMosaicHist->ValidPixels(), 1154321);
  EXPECT_NEAR(trimmedUncontrolledMosaicHist->StandardDeviation(), 0.010383565236007088, 1e-8);

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
    FAIL() << "Unable to run noseam on samcam images: " << e.what() << std::endl;
  }

  // validate histogram of uncontrolled noseam mosaic cube
  Cube noseamMosaic(tempDir.path() + "/noseamUncontrolledMosaic.cub");

  std::unique_ptr<Histogram> noseamMosaicHist(noseamMosaic.histogram());
  EXPECT_NEAR(noseamMosaicHist->Average(), 0.010375753089644776, 1e-8);
  EXPECT_NEAR(noseamMosaicHist->Sum(), 11976.949682191847, 1e-8);
  EXPECT_EQ(noseamMosaicHist->ValidPixels(), 1154321);
  EXPECT_NEAR(noseamMosaicHist->StandardDeviation(), 0.010379350848041419, 1e-8);

  // find overlaps of the two images
  QVector<QString> findimageoverlapsArgs =
      {"fromlist=" + tempDir.path() + "/unprojectedSpiceinitCubeList.lis",
       "overlaplist=" + tempDir.path() + "/samcam_findimageoverlaps.txt"};
  UserInterface findimageoverlapsUI(FINDIMAGEOVERLAPS_XML, findimageoverlapsArgs);
  try {
    findimageoverlaps(findimageoverlapsUI);
  }
  catch (IException &e) {
    FAIL() << "Unable to run findimageoverlaps on samcam images: " << e.what() << std::endl;
  }

  Pvl overlapstatsLog;

  // generate overlap stats for the two images
  QVector<QString> overlapstatsArgs =
      {"fromlist=" + tempDir.path() + "/unprojectedSpiceinitCubeList.lis",
       "overlaplist=" + tempDir.path() + "/samcam_findimageoverlaps.txt",
       "to=" + tempDir.path() + "/samcam_overlapstats.csv"};
  UserInterface overlapstatsUI(OVERLAPSTATS_XML, overlapstatsArgs);
  try {
    overlapstats(overlapstatsUI, &overlapstatsLog);
  }
  catch (IException &e) {
    FAIL() << "Unable to run overlapstats on samcam images: " << e.what() << std::endl;
  }

  // validate overlapstatsLog app log file

  PvlGroup &resultsGroup = overlapstatsLog.findGroup("Results");
  EXPECT_NEAR((double)resultsGroup.findKeyword("ThicknessMinimum"), 0.34443265217544999, 1e-8);
  EXPECT_NEAR((double)resultsGroup.findKeyword("ThicknessMaximum"), 0.34443265217544999, 1e-8);
  EXPECT_NEAR((double)resultsGroup.findKeyword("ThicknessAverage"), 0.34443265217544999, 1e-8);
  EXPECT_NEAR((double)resultsGroup.findKeyword("AreaMinimum"), 979.88986613001998, 1e-8);
  EXPECT_NEAR((double)resultsGroup.findKeyword("AreaMaximum"), 979.88986613001998, 1e-8);
  EXPECT_NEAR((double)resultsGroup.findKeyword("AreaAverage"), 979.88986613001998, 1e-8);
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

  // autoseed the two images
  QVector<QString> autoseedArgs =
      {"fromlist=" + tempDir.path() + "/unprojectedSpiceinitCubeList.lis",
       "overlaplist=" + tempDir.path() + "/samcam_findimageoverlaps.txt",
       "onet=" + tempDir.path() + "/samcam_autoseed.net",
       "deffile=" + tempDir.path() + "/autoseed.def",
       "networkid=OREx",
       "pointid=Bennu????",
       "description=auto"};
  UserInterface autoseedUI(AUTOSEED_XML, autoseedArgs);
  try {
    autoseed(autoseedUI);
  }
  catch (IException &e) {
    FAIL() << "Unable to run autoseed on samcam images: " << e.what() << std::endl;
  }

  // validate output control network
  ControlNet autoseedNet(tempDir.path() + "/samcam_autoseed.net");
  ASSERT_EQ(autoseedNet.GetNumPoints(), 39);
  ASSERT_EQ(autoseedNet.GetNumValidPoints(), 39);
  EXPECT_EQ(autoseedNet.GetNumMeasures(), 78);
  ASSERT_EQ(autoseedNet.GetNumValidMeasures(), 78);
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
       "cnet=" + tempDir.path() + "/samcam_autoseed.net",
       "deffile=" + tempDir.path() + "/pointreg.def",
       "onet=" + tempDir.path() + "/samcam_pointreg.net"};
  UserInterface pointregUI(POINTREG_XML, pointregArgs);
  try {
    pointreg(pointregUI, &pointregLog);
  }
  catch (IException &e) {
    FAIL() << "Unable to run pointreg on samcam images: " << e.what() << std::endl;
  }

  // validate output control network
  ControlNet pointregNet(tempDir.path() + "/samcam_pointreg.net");
  ASSERT_EQ(pointregNet.GetNumPoints(), 39);
  ASSERT_EQ(pointregNet.GetNumValidPoints(), 33);
  EXPECT_EQ(pointregNet.GetNumMeasures(), 78);
  ASSERT_EQ(pointregNet.GetNumValidMeasures(), 66);
  EXPECT_EQ(pointregNet.GetNumIgnoredMeasures(), 6);
}
