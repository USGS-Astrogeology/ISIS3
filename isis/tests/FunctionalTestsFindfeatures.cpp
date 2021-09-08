#include "findfeatures.h"

#include <QTemporaryFile>
#include <QTextStream>
#include <QStringList>

#include "Fixtures.h"
#include "PvlGroup.h"
#include "TestUtilities.h"
#include "SurfacePoint.h"
#include "ControlPoint.h"
#include "Latitude.h"
#include "Longitude.h"
#include "SerialNumber.h"

#include "gmock/gmock.h"

using namespace Isis;
using ::testing::HasSubstr;

static QString APP_XML = FileName("$ISISROOT/bin/xml/findfeatures.xml").expanded();

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
  EXPECT_DOUBLE_EQ(cm->GetSample(), 60.719512939453125);
  EXPECT_DOUBLE_EQ(cm->GetLine(), 31.866861343383789);
  SurfacePoint sp = pt->GetAprioriSurfacePoint();
  Latitude lat = sp.GetLatitude();
  Longitude lon = sp.GetLongitude();
  EXPECT_DOUBLE_EQ(lat.planetocentric(), 0.02581190071853327);
  EXPECT_DOUBLE_EQ(lon.positiveEast(), 0.0012615643288328212);


  // Control point with two measures
  pt = network.GetPoint("test_network_0018");
  cm = pt->GetMeasure(SerialNumber::Compose(*cube2));
  EXPECT_DOUBLE_EQ(cm->GetSample(), 143.62646484375);
  EXPECT_DOUBLE_EQ(cm->GetLine(), 69.777481079101562);

  cm = pt->GetMeasure(SerialNumber::Compose(*cube1));
  EXPECT_DOUBLE_EQ(cm->GetSample(), 383.62646484375);
  EXPECT_DOUBLE_EQ(cm->GetLine(), 81.777481079101562);

  sp = pt->GetAprioriSurfacePoint();
  lat = sp.GetLatitude();
  lon = sp.GetLongitude();
  EXPECT_DOUBLE_EQ(lat.planetocentric(), 0.028914626182060754);
  EXPECT_DOUBLE_EQ(lon.positiveEast(), 0.0071459825362942221);
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
    EXPECT_THAT(e.what(), HasSubstr("**USER ERROR** Must provide both a FROM/FROMLIST and MATCH cube or image filename"));
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
