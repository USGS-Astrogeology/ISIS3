#include "findfeatures.h"

#include <QTemporaryFile>
#include <QTextStream>
#include <QStringList>

#include "NetworkFixtures.h"
#include "PvlFlatMap.h"
#include "PvlGroup.h"
#include "TestUtilities.h"
#include "SurfacePoint.h"
#include "ControlPoint.h"
#include "Latitude.h"
#include "Longitude.h"
#include "SerialNumber.h"
#include "TextFile.h"

#include "gmock/gmock.h"

using namespace Isis;
using ::testing::HasSubstr;

static QString APP_XML = QString::fromStdString(FileName("$ISISROOT/bin/xml/findfeatures.xml").expanded());

// All FastGeom Keys expected in logs for algorithms
static const QStringList fastgeom_generic_keywords = { "FastGeomAlgorithm",
                                                       "FastGeomPoints",
                                                       "FastGeomTolerance",
                                                       "FastGeomQuerySampleTolerance",
                                                       "FastGeomQueryLineTolerance",
                                                       "FastGeomTrainSampleTolerance",
                                                       "FastGeomTrainLineTolerance" };
static const QStringList fastgeom_radial_keywords =  { "FastGeomMaximumRadius",
                                                       "FastGeomRadialSegmentLength",
                                                       "FastGeomRadialPointCount",
                                                       "FastGeomRadialPointFactor",
                                                       "FastGeomRadialSegments" };
static const QStringList fastgeom_grid_keywords =    { "FastGeomGridStartIteration",
                                                       "FastGeomGridStopIteration",
                                                       "FastGeomGridIterationStep",
                                                       "FastGeomGridSaveAllPoints",
                                                       "FastGeomPointIncrement",
                                                       "FastGeomTotalGridIterations" };



/** Helper function to load findfeatures debug log file */
inline QStringList filter_strings( const std::vector<QString> &strlist, const QString &pattern ) {
  QStringList found;
  for ( auto const &line : strlist ) {
    if ( line.contains( pattern ) ) found += line;
  }
  return ( found );
}

TEST_F(ThreeImageNetwork, FunctionalTestFindfeaturesDefault) {
  // Setup output file
  QVector<QString> args = {"algorithm=brisk/brisk",
                           "match=" + tempDir.path() + "/cube3.cub",
                           "fromlist=" + twoCubeListFile,
                           "tolist=" + tempDir.path() + "/toList.txt",
                           "tonotmatched=" + tempDir.path() + "/unmatched.txt",
                           "maxpoints=5000",
                           "epitolerance=1.0",
                           "ratio=.65",
                           "hmgtolerance=3.0",
                           "onet=" + tempDir.path() + "/network.net",
                           "networkid=new",
                           "pointid=test_network_????",
                           "target=MARS",
                           "description=new",
                           "debug=false"};
  UserInterface options(APP_XML, args);
  findfeatures(options);
  ControlNet network(options.GetFileName("ONET"));

  ASSERT_EQ(network.GetNetworkId(), "new");
  ASSERT_EQ(network.Description().toStdString(), "brisk/brisk/BFMatcher@NormType:NORM_HAMMING@CrossCheck:false");
  ASSERT_EQ(network.GetNumPoints(), 50);
}


TEST_F(ThreeImageNetwork, FunctionalTestFindfeaturesGeomMatch) {
  // Setup output file
  QVector<QString> args = {"algorithm=brisk/brisk",
                           "match=" + tempDir.path() + "/cube3.cub",
                           "fromlist=" + twoCubeListFile,
                           "tolist=" + tempDir.path() + "/toList.txt",
                           "tonotmatched=" + tempDir.path() + "/unmatched.txt",
                           "maxpoints=5000",
                           "epitolerance=1.0",
                           "ratio=.65",
                           "hmgtolerance=3.0",
                           "onet=" + tempDir.path() + "/network.net",
                           "networkid=new",
                           "pointid=test_network_????",
                           "description=new",
                           "geomsource=match",
                           "target=MARS",
                           "nettype=ground",
                           "debug=false"};
  UserInterface options(APP_XML, args);

  findfeatures(options);
  ControlNet network(options.GetFileName("ONET"));

  //Control point with a single measure
  ControlPoint *pt = network.GetPoint("test_network_0001");
  ControlMeasure *cm = pt->GetMeasure(SerialNumber::Compose(*cube2));
  EXPECT_NEAR(cm->GetSample(), 60.719512939453125, 1e-6);
  EXPECT_NEAR(cm->GetLine(), 31.866861343383789, 1e-6);
  SurfacePoint sp = pt->GetAprioriSurfacePoint();
  Latitude lat = sp.GetLatitude();
  Longitude lon = sp.GetLongitude();
  EXPECT_NEAR(lat.planetocentric(), 0.025811899541941467, 1e-6);
  EXPECT_NEAR(lon.positiveEast(), 0.0012615634743558179, 1e-6);


  // Control point with two measures
  pt = network.GetPoint("test_network_0018");
  cm = pt->GetMeasure(SerialNumber::Compose(*cube2));
  EXPECT_NEAR(cm->GetSample(), 143.62646484375, 1e-6);
  EXPECT_NEAR(cm->GetLine(), 69.777481079101562, 1e-6);

  cm = pt->GetMeasure(SerialNumber::Compose(*cube1));
  EXPECT_NEAR(cm->GetSample(), 383.62646484375, 1e-6);
  EXPECT_NEAR(cm->GetLine(), 81.777481079101562, 1e-6);

  sp = pt->GetAprioriSurfacePoint();
  lat = sp.GetLatitude();
  lon = sp.GetLongitude();
  EXPECT_NEAR(lat.planetocentric(), 0.028914626048514001, 1e-6);
  EXPECT_NEAR(lon.positiveEast(), 0.0071459947198023819, 1e-6);
}


TEST_F(ThreeImageNetwork, FunctionalTestFindfeaturesMultiAlgo) {
  // Setup output file
  QVector<QString> args = {"algorithm=brisk/brisk|orb@hessianThreshold:100/orb",
                           "match=" + tempDir.path() + "/cube3.cub",
                           "fromlist=" + twoCubeListFile,
                           "tolist=" + tempDir.path() + "/toList.txt",
                           "tonotmatched=" + tempDir.path() + "/unmatched.txt",
                           "maxpoints=5000",
                           "epitolerance=1.0",
                           "ratio=.65",
                           "hmgtolerance=3.0",
                           "onet=" + tempDir.path() + "/network.net",
                           "networkid=new",
                           "pointid=test_network_????",
                           "description=new",
                           "target=MARS",
                           "debug=false"};
  UserInterface options(APP_XML, args);
  findfeatures(options);
  ControlNet network(options.GetFileName("ONET"));

  ASSERT_EQ(network.GetNetworkId(), "new");
  ASSERT_EQ(network.Description().toStdString(), "brisk/brisk/BFMatcher@NormType:NORM_HAMMING@CrossCheck:false");
  ASSERT_EQ(network.GetNumPoints(), 50);
}

TEST_F(ThreeImageNetwork, FunctionalTestFindfeaturesMaxPoints) {
  // Tests that decreasing maxpoints decreases output points + tests pointindex
  QVector<QString> args1 = { "algorithm=brisk/brisk",
                           "match=" + tempDir.path() + "/cube3.cub",
                           "fromlist=" + twoCubeListFile,
                           "tolist=" + tempDir.path() + "/toList.txt",
                           "tonotmatched=" + tempDir.path() + "/unmatched.txt",
                           "maxpoints=5000",
                           "epitolerance=1.0",
                           "ratio=.65",
                           "hmgtolerance=3.0",
                           "onet=" + tempDir.path() + "/network.net",
                           "networkid=new",
                           "pointid=test_network_????",
                           "pointindex=100",
                           "description=new",
                           "target=MARS",
                           "debug=false"};


  QVector<QString> args2 = { "algorithm=brisk/brisk",
                           "match=" + tempDir.path() + "/cube3.cub",
                           "fromlist=" + twoCubeListFile,
                           "tolist=" + tempDir.path() + "/toList.txt",
                           "tonotmatched=" + tempDir.path() + "/unmatched.txt",
                           "maxpoints=1000",
                           "epitolerance=1.0",
                           "ratio=.65",
                           "hmgtolerance=3.0",
                           "onet=" + tempDir.path() + "/network2.net",
                           "networkid=new",
                           "pointid=test_network_????",
                           "pointindex=100",
                           "description=new",
                           "target=MARS",
                           "debug=false"};

  UserInterface options1(APP_XML, args1);
  UserInterface options2(APP_XML, args2);
  findfeatures(options1);
  findfeatures(options2);
  ControlNet network1(options1.GetFileName("ONET"));
  ControlNet network2(options2.GetFileName("ONET"));

  ASSERT_EQ(network1.GetNetworkId(), "new");
  ASSERT_EQ(network1.Description().toStdString(), "brisk/brisk/BFMatcher@NormType:NORM_HAMMING@CrossCheck:false");

  ASSERT_TRUE(network1.ContainsPoint("test_network_0100"));
  ASSERT_GT(network1.GetNumPoints(), network2.GetNumPoints());
}


TEST_F(ThreeImageNetwork, FunctionalTestFindfeaturesErrorListspecNoAlg) {
  QVector<QString> args = {"listspec=yes"};
  UserInterface options(APP_XML, args);
  try {
    findfeatures(options);
    FAIL() << "Should throw an exception" << std::endl;
  }
  catch (IException &e) {
    EXPECT_THAT(e.what(), HasSubstr("**USER ERROR** No feature matcher algorithms provided!"));
  }
}


TEST_F(ThreeImageNetwork, FunctionalTestFindfeaturesErrorInputNoAlg) {
  QVector<QString> args = {"match=" + tempDir.path() + "/cube3.cub",
                           "from=" + tempDir.path() + "/cube2.cub"};
  UserInterface options(APP_XML, args);
  try {
    findfeatures(options);
    FAIL() << "Should throw an exception" << std::endl;
  }
  catch (IException &e) {
    EXPECT_THAT(e.what(), HasSubstr("**USER ERROR** No feature matcher algorithms provided!"));
  }
}


TEST_F(ThreeImageNetwork, FunctionalTestFindfeaturesErrorNoInput) {
  QVector<QString> args = {"match=" + tempDir.path() + "/cube3.cub",
                           "algorithm=sift/sift"};
  UserInterface options(APP_XML, args);
  try {
    findfeatures(options);
    FAIL() << "Should throw an exception" << std::endl;
  }
  catch (IException &e) {
    EXPECT_THAT(e.what(), HasSubstr("**USER ERROR** Input cubes (0) failed to load. Must provide valid FROM/FROMLIST and MATCH cube or image filenames"));
  }
}



TEST_F(ThreeImageNetwork, FunctionalTestFindfeaturesErrorNoMatch) {
  // Tests that decreasing maxpoints decreases output points + tests pointindex
  QVector<QString> args = { "algorithm=brisk/brisk",
                           "match=" + tempDir.path() + "/cube3.cub",
                           "fromlist=" + twoCubeListFile,
                           "tolist=" + tempDir.path() + "/toList.txt",
                           "tonotmatched=" + tempDir.path() + "/unmatched.txt",
                           "maxpoints=1",
                           "epitolerance=1.0",
                           "ratio=.65",
                           "hmgtolerance=3.0",
                           "onet=" + tempDir.path() + "/network.net",
                           "networkid=new",
                           "pointid=test_network_????",
                           "pointindex=100",
                           "description=new",
                           "debug=false"};

  UserInterface options(APP_XML, args);
  try {
    findfeatures(options);
    FAIL() << "Should throw an exception" << std::endl;
  }
  catch (IException &e) {
    EXPECT_THAT(e.what(), HasSubstr("**USER ERROR** No control points found!"));
  }
}

TEST_F(ThreeImageNetwork, FunctionalTestFindfeaturesFastGeomDefault) {
  // Setup output file
  const QString debuglogfile = tempDir.path() + "/default_fastgeom_algorithm.log";

  // Needs no additional parameters to test the default case - just add log params
  QVector<QString> args = {"algorithm=orb@hessianThreshold:100/orb",
                           "match=" + tempDir.path() + "/cube3.cub",
                           "from=" + tempDir.path() + "/cube2.cub",
                           "tolist=" + tempDir.path() + "/toList.txt",
                           "tonotmatched=" + tempDir.path() + "/unmatched.txt",
                           "maxpoints=5000",
                           "fastgeom=true",
                           "epitolerance=3.0",
                           "ratio=.9",
                           "hmgtolerance=3.0",
                           "onet=" + tempDir.path() + "/default_fastgeom_network.net",
                           "networkid=default_fastgeom",
                           "pointid=test_network_????",
                           "description=default_fastgeom",
                           "target=MARS",
                           "debug=true",
                           "debuglog=" + debuglogfile };
  UserInterface options(APP_XML, args);
  findfeatures(options);
  ControlNet network(options.GetFileName("ONET"));

  // Tests are based upon these condtions
  ASSERT_EQ(network.GetNetworkId(), "default_fastgeom");
  ASSERT_EQ(network.Description().toStdString(), "orb@hessianThreshold:100/orb/BFMatcher@NormType:NORM_HAMMING@CrossCheck:false");
  ASSERT_EQ(network.GetNumPoints(), 30);

  // Load the log file and parse it looking for FastGeom signatures
  std::vector<QString> logdata;
  TextFile(debuglogfile, "input", logdata );
  QStringList fgeomkeys = filter_strings( logdata, "FastGeom" );

  QStringList expected = fastgeom_generic_keywords + fastgeom_radial_keywords;
  PvlFlatMap keyvalues;

  for ( auto const &key : expected ) {
    QStringList parsed = fgeomkeys.filter( key );
    ASSERT_EQ( parsed.size(), 1 );
    if ( parsed.size() == 1 ) {
      QStringList fgline = parsed[0].simplified().split(':');
      ASSERT_EQ( fgline.size(), 2 );
      ASSERT_EQ( fgline[0], key );
      if ( fgline.size() == 2 ) keyvalues.add(key, fgline[1].simplified());
    }
  }

  // Check for expected values here
  ASSERT_EQ( keyvalues.get( "FastGeomAlgorithm",            "null"), "radial");
  ASSERT_EQ( keyvalues.get( "FastGeomPoints",               "null"), "25");
  ASSERT_EQ( keyvalues.get( "FastGeomTolerance",            "null"), "3");
  ASSERT_EQ( keyvalues.get( "FastGeomQuerySampleTolerance", "null"), "0");
  ASSERT_EQ( keyvalues.get( "FastGeomQueryLineTolerance",   "null"), "0");
  ASSERT_EQ( keyvalues.get( "FastGeomTrainSampleTolerance", "null"), "0");
  ASSERT_EQ( keyvalues.get( "FastGeomTrainLineTolerance",   "null"), "0");
  ASSERT_EQ( keyvalues.get( "FastGeomRadialSegmentLength",  "null"), "25");
  ASSERT_EQ( keyvalues.get( "FastGeomRadialPointCount",     "null"), "5");
  ASSERT_EQ( keyvalues.get( "FastGeomRadialPointFactor",    "null"), "1");
  ASSERT_EQ( keyvalues.get( "FastGeomRadialSegments",       "null"), "14");

  // Do the floating point special
  EXPECT_NEAR( keyvalues.get( "FastGeomMaximumRadius",        "-1").toDouble(), 339.411, 1.0E-4);
}

TEST_F(ThreeImageNetwork, FunctionalTestFindfeaturesFastGeomRadialConfig) {
  // Setup output file
  const QString debuglogfile = tempDir.path() + "/radial_config_fastgeom_algorithm.log";

  // Needs no additional parameters to test the default case - just add log params
  QVector<QString> args = {"algorithm=orb@hessianThreshold:100/orb",
                           "match=" + tempDir.path() + "/cube3.cub",
                           "from=" + tempDir.path() + "/cube2.cub",
                           "tolist=" + tempDir.path() + "/toList.txt",
                           "tonotmatched=" + tempDir.path() + "/unmatched.txt",
                           "parameters=" + radial_fastgeom_config,
                           "maxpoints=5000",
                           "fastgeom=true",
                           "epitolerance=3.0",
                           "ratio=.9",
                           "hmgtolerance=3.0",
                           "onet=" + tempDir.path() + "/radial_config_fastgeom_network.net",
                           "networkid=radial_config_fastgeom",
                           "pointid=test_network_????",
                           "description=radial_config_fastgeom",
                           "target=MARS",
                           "debug=true",
                           "debuglog=" + debuglogfile };
  UserInterface options(APP_XML, args);
  findfeatures(options);
  ControlNet network(options.GetFileName("ONET"));

  // Tests are based upon these condtions
  ASSERT_EQ(network.GetNetworkId(), "radial_config_fastgeom");
  ASSERT_EQ(network.Description().toStdString(), "orb@hessianThreshold:100/orb/BFMatcher@NormType:NORM_HAMMING@CrossCheck:false");
  ASSERT_EQ(network.GetNumPoints(), 35);

  // Load the log file and parse it looking for FastGeom signatures
  std::vector<QString> logdata;
  TextFile(debuglogfile, "input", logdata );
  QStringList fgeomkeys = filter_strings( logdata, "FastGeom" );

  QStringList expected = fastgeom_generic_keywords + fastgeom_radial_keywords;
  PvlFlatMap keyvalues;

  for ( auto const &key : expected ) {
    QStringList parsed = fgeomkeys.filter( key );
    ASSERT_EQ( parsed.size(), 1 );
    if ( parsed.size() == 1 ) {
      QStringList fgline = parsed[0].simplified().split(':');
      ASSERT_EQ( fgline.size(), 2 );
      ASSERT_EQ( fgline[0], key );
      if ( fgline.size() == 2 ) keyvalues.add(key, fgline[1].simplified());
    }
  }

  // Check for expected values here
  ASSERT_EQ( keyvalues.get( "FastGeomAlgorithm",            "null"), "radial");
  ASSERT_EQ( keyvalues.get( "FastGeomPoints",               "null"), "25");
  ASSERT_EQ( keyvalues.get( "FastGeomTolerance",            "null"), "3");
  ASSERT_EQ( keyvalues.get( "FastGeomQuerySampleTolerance", "null"), "15");
  ASSERT_EQ( keyvalues.get( "FastGeomQueryLineTolerance",   "null"), "15");
  ASSERT_EQ( keyvalues.get( "FastGeomTrainSampleTolerance", "null"), "0");
  ASSERT_EQ( keyvalues.get( "FastGeomTrainLineTolerance",   "null"), "0");
  ASSERT_EQ( keyvalues.get( "FastGeomRadialSegmentLength",  "null"), "10");
  ASSERT_EQ( keyvalues.get( "FastGeomRadialPointCount",     "null"), "7");
  ASSERT_EQ( keyvalues.get( "FastGeomRadialPointFactor",    "null"), "0.5");
  ASSERT_EQ( keyvalues.get( "FastGeomRadialSegments",       "null"), "37");

  // Do the floating point special
  EXPECT_NEAR( keyvalues.get( "FastGeomMaximumRadius",        "-1").toDouble(), 360.624, 1.0E-4);
}

TEST_F(ThreeImageNetwork, FunctionalTestFindfeaturesFastGeomGridDefault) {
  // Setup output file
  const QString debuglogfile = tempDir.path() + "/grid_default_fastgeom_algorithm.log";

  // Needs no additional parameters to test the default case - just add log params
  QVector<QString> args = {"algorithm=orb@hessianThreshold:100/orb",
                           "match=" + tempDir.path() + "/cube3.cub",
                           "from=" + tempDir.path() + "/cube2.cub",
                           "tolist=" + tempDir.path() + "/toList.txt",
                           "tonotmatched=" + tempDir.path() + "/unmatched.txt",
                           "globals=FastGeomAlgorithm:grid",
                           "maxpoints=5000",
                           "fastgeom=true",
                           "epitolerance=3.0",
                           "ratio=.9",
                           "hmgtolerance=3.0",
                           "onet=" + tempDir.path() + "/grid_default_fastgeom_network.net",
                           "networkid=grid_default_fastgeom",
                           "pointid=test_network_????",
                           "description=grid_default_fastgeom",
                           "target=MARS",
                           "debug=true",
                           "debuglog=" + debuglogfile };
  UserInterface options(APP_XML, args);
  findfeatures(options);
  ControlNet network(options.GetFileName("ONET"));

  // Tests are based upon these condtions
  ASSERT_EQ(network.GetNetworkId(), "grid_default_fastgeom");
  ASSERT_EQ(network.Description().toStdString(), "orb@hessianThreshold:100/orb/BFMatcher@NormType:NORM_HAMMING@CrossCheck:false");
  ASSERT_EQ(network.GetNumPoints(), 38);

  // Load the log file and parse it looking for FastGeom signatures
  std::vector<QString> logdata;
  TextFile(debuglogfile, "input", logdata );
  QStringList fgeomkeys = filter_strings( logdata, "FastGeom" );

  QStringList expected = fastgeom_generic_keywords + fastgeom_grid_keywords;
  PvlFlatMap keyvalues;

  for ( auto const &key : expected ) {
    QStringList parsed = fgeomkeys.filter( key );
    ASSERT_EQ( parsed.size(), 1 );
    if ( parsed.size() == 1 ) {
      QStringList fgline = parsed[0].simplified().split(':');
      ASSERT_EQ( fgline.size(), 2 );
      ASSERT_EQ( fgline[0], key );
      if ( fgline.size() == 2 ) keyvalues.add(key, fgline[1].simplified());
    }
  }

  // Check for expected values here
  ASSERT_EQ( keyvalues.get( "FastGeomAlgorithm",            "null"), "grid");
  ASSERT_EQ( keyvalues.get( "FastGeomPoints",               "null"), "25");
  ASSERT_EQ( keyvalues.get( "FastGeomTolerance",            "null"), "3");
  ASSERT_EQ( keyvalues.get( "FastGeomQuerySampleTolerance", "null"), "0");
  ASSERT_EQ( keyvalues.get( "FastGeomQueryLineTolerance",   "null"), "0");
  ASSERT_EQ( keyvalues.get( "FastGeomTrainSampleTolerance", "null"), "0");
  ASSERT_EQ( keyvalues.get( "FastGeomTrainLineTolerance",   "null"), "0");
  ASSERT_EQ( keyvalues.get( "FastGeomGridStartIteration",   "null"), "0");
  ASSERT_EQ( keyvalues.get( "FastGeomGridStopIteration",    "null"), "239");
  ASSERT_EQ( keyvalues.get( "FastGeomGridIterationStep",    "null"), "1");
  ASSERT_EQ( keyvalues.get( "FastGeomGridSaveAllPoints",    "null"), "0");
  ASSERT_EQ( keyvalues.get( "FastGeomPointIncrement",       "null"), "5");
  ASSERT_EQ( keyvalues.get( "FastGeomTotalGridIterations",  "null"), "2");
}

TEST_F(ThreeImageNetwork, FunctionalTestFindfeaturesFastGeomGridConfig) {
  // Setup output file
  const QString debuglogfile = tempDir.path() + "/grid_config_fastgeom_algorithm.log";

  // Needs no additional parameters to test the default case - just add log params
  QVector<QString> args = {"algorithm=orb@hessianThreshold:100/orb",
                           "match=" + tempDir.path() + "/cube3.cub",
                           "from=" + tempDir.path() + "/cube2.cub",
                           "tolist=" + tempDir.path() + "/toList.txt",
                           "tonotmatched=" + tempDir.path() + "/unmatched.txt",
                           "parameters=" + grid_fastgeom_config,
                           "maxpoints=5000",
                           "fastgeom=true",
                           "epitolerance=3.0",
                           "ratio=.9",
                           "hmgtolerance=3.0",
                           "onet=" + tempDir.path() + "/grid_default_fastgeom_network.net",
                           "networkid=grid_config_fastgeom",
                           "pointid=test_network_????",
                           "description=grid_config_fastgeom",
                           "target=MARS",
                           "debug=true",
                           "debuglog=" + debuglogfile };
  UserInterface options(APP_XML, args);
  findfeatures(options);
  ControlNet network(options.GetFileName("ONET"));

  // Tests are based upon these condtions
  ASSERT_EQ(network.GetNetworkId(), "grid_config_fastgeom");
  ASSERT_EQ(network.Description().toStdString(), "orb@hessianThreshold:100/orb/BFMatcher@NormType:NORM_HAMMING@CrossCheck:false");
  ASSERT_EQ(network.GetNumPoints(), 31);

  // Load the log file and parse it looking for FastGeom signatures
  std::vector<QString> logdata;
  TextFile(debuglogfile, "input", logdata );
  QStringList fgeomkeys = filter_strings( logdata, "FastGeom" );

  QStringList expected = fastgeom_generic_keywords + fastgeom_grid_keywords;
  PvlFlatMap keyvalues;

  for ( auto const &key : expected ) {
    QStringList parsed = fgeomkeys.filter( key );
    ASSERT_EQ( parsed.size(), 1 );
    if ( parsed.size() == 1 ) {
      QStringList fgline = parsed[0].simplified().split(':');
      ASSERT_EQ( fgline.size(), 2 );
      ASSERT_EQ( fgline[0], key );
      if ( fgline.size() == 2 ) keyvalues.add(key, fgline[1].simplified());
    }
  }

  // Check for expected values here
  ASSERT_EQ( keyvalues.get( "FastGeomAlgorithm",            "null"), "grid");
  ASSERT_EQ( keyvalues.get( "FastGeomPoints",               "null"), "25");
  ASSERT_EQ( keyvalues.get( "FastGeomTolerance",            "null"), "3");
  ASSERT_EQ( keyvalues.get( "FastGeomQuerySampleTolerance", "null"), "0");
  ASSERT_EQ( keyvalues.get( "FastGeomQueryLineTolerance",   "null"), "0");
  ASSERT_EQ( keyvalues.get( "FastGeomTrainSampleTolerance", "null"), "15");
  ASSERT_EQ( keyvalues.get( "FastGeomTrainLineTolerance",   "null"), "15");
  ASSERT_EQ( keyvalues.get( "FastGeomGridStartIteration",   "null"), "5");
  ASSERT_EQ( keyvalues.get( "FastGeomGridStopIteration",    "null"), "10");
  ASSERT_EQ( keyvalues.get( "FastGeomGridIterationStep",    "null"), "2");
  ASSERT_EQ( keyvalues.get( "FastGeomGridSaveAllPoints",    "null"), "0");
  ASSERT_EQ( keyvalues.get( "FastGeomPointIncrement",       "null"), "5");
  ASSERT_EQ( keyvalues.get( "FastGeomTotalGridIterations",  "null"), "1");
}


