#include <QTextStream>
#include <QStringList>
#include <QTemporaryFile>

#include "cnetextract.h"
#include "NetworkFixtures.h"
#include "Pvl.h"
#include "PvlGroup.h"
#include "TestUtilities.h"

#include "gmock/gmock.h"

using namespace Isis;
using testing::HasSubstr;

static QString APP_XML = QString::fromStdString(FileName("$ISISROOT/bin/xml/cnetextract.xml").expanded());

TEST_F(ThreeImageNetwork, FunctionalTestCnetextractExclusiveNoFromlist) {
  QVector<QString> args = {"prefix=" + tempDir.path() + "/",
                           "tolist=" + tempDir.path() + "/newList.lis",
                           "onet=" + tempDir.path() + "/newNet.net",
                           "noignore=true"};
  UserInterface options(APP_XML, args);
  Pvl appLog;

  try {
    cnetextract( *network, options, &appLog );
    FAIL() << "Should not have been able to extract a new network with no fromlist set" << std::endl;
  } catch(IException &e) {
    EXPECT_THAT(e.what(), HasSubstr("To create a [TOLIST] the [FROMLIST] parameter must be provided"));
  }
}

TEST_F(ThreeImageNetwork, FunctionalTestCnetextractExclusiveNoOnet) {
  QVector<QString> args = {"fromlist=" + cubeListFile,
                           "prefix=" + tempDir.path() + "/",
                           "tolist=" + tempDir.path() + "/newList.lis",
                           "noignore=true"};
  UserInterface options(APP_XML, args);
  Pvl appLog;

  try {
    cnetextract( *network, options, &appLog );
    FAIL() << "Should not have been able to extract a new network with no onet set" << std::endl;
  } catch(IException &e) {
    EXPECT_THAT(e.what(), HasSubstr("Parameter [ONET] has no value"));
  }
}

TEST_F(ThreeImageNetwork, FunctionalTestCnetextractExclusiveNoFilter) {
  QVector<QString> args = {"fromlist=" + cubeListFile,
                           "prefix=" + tempDir.path() + "/",
                           "tolist=" + tempDir.path() + "/newList.lis",
                           "onet=" + tempDir.path() + "/newNet.net"};
  UserInterface options(APP_XML, args);
  Pvl appLog;

  try {
    cnetextract( *network, options, &appLog );
    FAIL() << "Should not have been able to extract a new network with no filter" << std::endl;
  } catch(IException &e) {
    EXPECT_THAT(e.what(), HasSubstr("At least one filter must be selected"));
  }
}

TEST_F(ThreeImageNetwork, FunctionalTestCnetextractExclusiveNoIgnore) {
  QVector<QString> args = {"fromlist=" + cubeListFile,
                           "prefix=" + tempDir.path() + "/",
                           "tolist=" + tempDir.path() + "/newList.lis",
                           "onet=" + tempDir.path() + "/newNet.net",
                           "noignore=true",
                           "networkid=new",
                           "description=new"};
  UserInterface options(APP_XML, args);
  Pvl appLog;

  QList<ControlPoint *> points = network->GetPoints();
  points[0]->SetIgnored(true);

  cnetextract( *network, options, &appLog );

  ControlNet outputNet(options.GetFileName("ONET"));

  ASSERT_EQ(outputNet.GetNetworkId(), "new");
  ASSERT_EQ(outputNet.Description(), "new");

  ASSERT_EQ(int(appLog.findGroup("ResultSummary")["IgnoredPoints"]), 1);
  ASSERT_EQ(int(appLog.findGroup("ResultSummary")["IgnoredMeasures"]), 0);

  ASSERT_EQ(network->GetNumPoints() - outputNet.GetNumPoints(), 1);
  ASSERT_EQ(network->GetNumMeasures() - outputNet.GetNumMeasures(), 2);

  FileName ignoredPointsFile(options.GetAsString("prefix").toStdString() + "IgnoredPoints.txt");
  EXPECT_TRUE(ignoredPointsFile.fileExists());
}

TEST_F(ThreeImageNetwork, FunctionalTestCnetextractExclusiveNoSingleMeasure) {
  QVector<QString> args = {"fromlist=" + cubeListFile,
                           "prefix=" + tempDir.path() + "/",
                           "tolist=" + tempDir.path() + "/newList.lis",
                           "onet=" + tempDir.path() + "/newNet.net",
                           "nosinglemeasure=true"};
  UserInterface options(APP_XML, args);
  Pvl appLog;

  QList<ControlPoint *> points = network->GetPoints();

  // Deletes one of the two meausres that would have been removed
  for (auto measure : points[0]->getMeasures()) {
    points[0]->Delete(measure);
    break;
  }

  cnetextract( *network, options, &appLog );

  ControlNet outputNet(options.GetFileName("ONET"));

  ASSERT_EQ(int(appLog.findGroup("ResultSummary")["SingleMeasurePoints"]), 1);

  ASSERT_EQ(network->GetNumPoints() - outputNet.GetNumPoints(), 1);
  ASSERT_EQ(network->GetNumMeasures() - outputNet.GetNumMeasures(), 1);

  FileName measurePointsFile(options.GetAsString("prefix").toStdString() + "SingleMeasurePoints.txt");
  EXPECT_TRUE(measurePointsFile.fileExists());
}

TEST_F(ThreeImageNetwork, FunctionalTestCnetextractExclusiveNoMeasureless) {
  QVector<QString> args = {"fromlist=" + cubeListFile,
                           "prefix=" + tempDir.path() + "/",
                           "tolist=" + tempDir.path() + "/newList.lis",
                           "onet=" + tempDir.path() + "/newNet.net",
                           "nomeasureless=true"};
  UserInterface options(APP_XML, args);
  Pvl appLog;

  QList<ControlPoint *> points = network->GetPoints();

  // Deletes both measures that would have been removed
  for (auto measure : points[0]->getMeasures()) {
    points[0]->Delete(measure);
  }

  cnetextract( *network, options, &appLog );

  ControlNet outputNet(options.GetFileName("ONET"));

  ASSERT_EQ(int(appLog.findGroup("ResultSummary")["MeasurelessPoints"]), 1);

  ASSERT_EQ(network->GetNumPoints() - outputNet.GetNumPoints(), 1);
  ASSERT_EQ(network->GetNumMeasures() - outputNet.GetNumMeasures(), 0);

  FileName measurelessPointsFile(options.GetAsString("prefix").toStdString() + "MeasurelessPoints.txt");
  EXPECT_TRUE(measurelessPointsFile.fileExists());
}

TEST_F(ThreeImageNetwork, FunctionalTestCnetextractInclusiveReference) {
  QVector<QString> args = {"fromlist=" + cubeListFile,
                           "prefix=" + tempDir.path() + "/",
                           "tolist=" + tempDir.path() + "/newList.lis",
                           "onet=" + tempDir.path() + "/newNet.net",
                           "reference=true"};
  UserInterface options(APP_XML, args);
  Pvl appLog;

  cnetextract( *network, options, &appLog );

  ControlNet outputNet(options.GetFileName("ONET"));

  ASSERT_EQ(int(appLog.findGroup("ResultSummary")["NonReferenceMeasures"]), 25);

  ASSERT_EQ(network->GetNumPoints() - outputNet.GetNumPoints(), 0);
  ASSERT_EQ(network->GetNumMeasures() - outputNet.GetNumMeasures(), 25);

  FileName referencePointsFile(options.GetAsString("prefix").toStdString() + "NonReferenceMeasures.txt");
  EXPECT_TRUE(referencePointsFile.fileExists());
}

TEST_F(ThreeImageNetwork, FunctionalTestCnetextractInclusiveFixed) {
  QVector<QString> args = {"fromlist=" + cubeListFile,
                           "prefix=" + tempDir.path() + "/",
                           "tolist=" + tempDir.path() + "/newList.lis",
                           "onet=" + tempDir.path() + "/newNet.net",
                           "fixed=true"};
  UserInterface options(APP_XML, args);
  Pvl appLog;

  QList<ControlPoint *> points = network->GetPoints();

  points[0]->SetType(ControlPoint::PointType::Fixed);
  points[1]->SetType(ControlPoint::PointType::Fixed);
  points[2]->SetType(ControlPoint::PointType::Fixed);

  cnetextract( *network, options, &appLog );

  ControlNet outputNet(options.GetFileName("ONET"));

  ASSERT_EQ(int(appLog.findGroup("ResultSummary")["NonFixedPoints"]), 13);

  ASSERT_EQ(network->GetNumPoints() - outputNet.GetNumPoints(), 13);
  ASSERT_EQ(network->GetNumMeasures() - outputNet.GetNumMeasures(), 35);

  FileName fixedPointsFile(options.GetAsString("prefix").toStdString() + "NonFixedPoints.txt");
  EXPECT_TRUE(fixedPointsFile.fileExists());
}

TEST_F(ThreeImageNetwork, FunctionalTestCnetextractInclusiveContrained) {
  QVector<QString> args = {"fromlist=" + cubeListFile,
                           "prefix=" + tempDir.path() + "/pre",
                           "tolist=" + tempDir.path() + "/newList.lis",
                           "onet=" + tempDir.path() + "/newNet.net",
                           "constrained=true"};
  UserInterface options(APP_XML, args);
  Pvl appLog;

  QList<ControlPoint *> points = network->GetPoints();

  points[0]->SetType(ControlPoint::PointType::Constrained);
  points[1]->SetType(ControlPoint::PointType::Constrained);
  points[2]->SetType(ControlPoint::PointType::Constrained);

  cnetextract( *network, options, &appLog );

  ControlNet outputNet(options.GetFileName("ONET"));

  ASSERT_EQ(network->GetNumPoints() - outputNet.GetNumPoints(), 13);
  ASSERT_EQ(network->GetNumMeasures() - outputNet.GetNumMeasures(), 35);
}

TEST_F(ThreeImageNetwork, FunctionalTestCnetextractInclusiveEditlock) {
  QVector<QString> args = {"fromlist=" + cubeListFile,
                           "prefix="  + tempDir.path() + "/",
                           "tolist=" + tempDir.path() + "/newList.lis",
                           "onet=" + tempDir.path() + "/newNet.net",
                           "editlock=true"};
  UserInterface options(APP_XML, args);
  Pvl appLog;

  QList<ControlPoint *> points = network->GetPoints();

  points[0]->SetEditLock(true);
  points[1]->SetEditLock(true);
  points[2]->SetEditLock(true);

  cnetextract( *network, options, &appLog );

  ControlNet outputNet(options.GetFileName("ONET"));

  ASSERT_EQ(network->GetNumPoints() - outputNet.GetNumPoints(), 13);
  ASSERT_EQ(network->GetNumMeasures() - outputNet.GetNumMeasures(), 35);
}

TEST_F(ThreeImageNetwork, FunctionalTestCnetextractInclusivePixeltolerence) {
  QVector<QString> args = {"fromlist=" + cubeListFile,
                           "prefix=" + tempDir.path() + "/",
                           "tolist=" + tempDir.path() + "/newList.lis",
                           "onet=" + tempDir.path() + "/newNet.net",
                           "tolerance=true",
                           "pixeltolerance=9.0"};
  UserInterface options(APP_XML, args);
  Pvl appLog;

  QList<ControlPoint *> points = network->GetPoints();

  for (int i = 0; i < points.size(); i++) {
    points[i]->getMeasures()[0]->SetResidual(i, i);
  }

  cnetextract( *network, options, &appLog );

  ControlNet outputNet(options.GetFileName("ONET"));

  ASSERT_EQ(int(appLog.findGroup("ResultSummary")["TolerancePoints"]), 9);

  ASSERT_EQ(network->GetNumPoints() - outputNet.GetNumPoints(), 9);
  ASSERT_EQ(network->GetNumMeasures() - outputNet.GetNumMeasures(), 23);

  FileName tolerancePointsFile(options.GetAsString("prefix").toStdString() + "TolerancePoints.txt");
  EXPECT_TRUE(tolerancePointsFile.fileExists());
}

TEST_F(ThreeImageNetwork, FunctionalTestCnetextractInclusivePointlist) {
  QString pointListFile = tempDir.path() + "/pointList.lis";
  QVector<QString> args = {"fromlist=" + cubeListFile,
                           "prefix=" + tempDir.path() + "/",
                           "tolist=" + tempDir.path() + "/newList.lis",
                           "onet=" + tempDir.path() + "/newNet.net",
                           "pointlist=" + pointListFile};

  FileList pointList;
  pointList.append("test0001");
  pointList.append("test0002");
  pointList.append("test0003");
  pointList.append("test0004");
  pointList.append("test0005");
  pointList.write(pointListFile.toStdString());

  UserInterface options(APP_XML, args);
  Pvl appLog;

  cnetextract( *network, options, &appLog );

  ControlNet outputNet(options.GetFileName("ONET"));

  ASSERT_EQ(int(appLog.findGroup("ResultSummary")["NonListedPoints"]), 11);

  ASSERT_EQ(network->GetNumPoints() - outputNet.GetNumPoints(), 11);
  ASSERT_EQ(network->GetNumMeasures() - outputNet.GetNumMeasures(), 30);

  FileName nonListedPointsFile(options.GetAsString("prefix").toStdString() + "NonListedPoints.txt");
  EXPECT_TRUE(nonListedPointsFile.fileExists());
}

TEST_F(ThreeImageNetwork, FunctionalTestCnetextractCubeCubelist) {
  QString reducedCubeList = tempDir.path() + "/reducedCubes.lis";
  cubeList->pop_back();
  cubeList->pop_back();
  cubeList->write(reducedCubeList.toStdString());

  QVector<QString> args = {"fromlist=" + cubeListFile,
                           "prefix=" + tempDir.path() + "/",
                           "tolist=" + tempDir.path() + "/newList.lis",
                           "onet=" + tempDir.path() + "/newNet.net",
                           "cubes=true",
                           "cubelist=" + reducedCubeList};

  UserInterface options(APP_XML, args);
  Pvl appLog;

  cnetextract( *network, options, &appLog );

  ControlNet outputNet(options.GetFileName("ONET"));

  ASSERT_EQ(int(appLog.findGroup("ResultSummary")["NonCubePoints"]), 3);

  ASSERT_EQ(network->GetNumPoints() - outputNet.GetNumPoints(), 3);
  ASSERT_EQ(network->GetNumMeasures() - outputNet.GetNumMeasures(), 6);

  FileName nonCubePointsFile(options.GetAsString("prefix").toStdString() + "NonCubePoints.txt");
  EXPECT_TRUE(nonCubePointsFile.fileExists());
}

TEST_F(ThreeImageNetwork, FunctionalTestCnetextractCubeCubemeasures) {
  QString reducedCubeList = tempDir.path() + "/reducedCubes.lis";
  cubeList->pop_back();
  cubeList->pop_back();
  cubeList->write(reducedCubeList.toStdString());

  QVector<QString> args = {"fromlist=" + cubeListFile,
                           "prefix=" + tempDir.path() + "/",
                           "tolist=" + tempDir.path() + "/newList.lis",
                           "onet=" + tempDir.path() + "/newNet.net",
                           "cubes=true",
                           "cubelist=" + reducedCubeList,
                           "cubemeasures=true"};

  UserInterface options(APP_XML, args);
  Pvl appLog;

  cnetextract( *network, options, &appLog );

  ControlNet outputNet(options.GetFileName("ONET"));

  ASSERT_EQ(int(appLog.findGroup("ResultSummary")["NonCubePoints"]), 3);
  ASSERT_EQ(int(appLog.findGroup("ResultSummary")["NonCubeMeasures"]), 28);

  ASSERT_EQ(network->GetNumPoints() - outputNet.GetNumPoints(), 3);
  ASSERT_EQ(network->GetNumMeasures() - outputNet.GetNumMeasures(), 28);

  FileName nonCubePointsFile(options.GetAsString("prefix").toStdString() + "NonCubePoints.txt");
  EXPECT_TRUE(nonCubePointsFile.fileExists());

  FileName nonCubeMeasuresFile(options.GetAsString("prefix").toStdString() + "NonCubeMeasures.txt");
  EXPECT_TRUE(nonCubeMeasuresFile.fileExists());
}

TEST_F(ThreeImageNetwork, FunctionalTestCnetextractCubeRetainreference) {
  QString reducedCubeList = tempDir.path() + "/reducedCubes.lis";
  cubeList->pop_back();
  cubeList->pop_back();
  cubeList->write(reducedCubeList.toStdString());

  QVector<QString> args = {"fromlist=" + cubeListFile,
                           "prefix=" + tempDir.path() + "/",
                           "tolist=" + tempDir.path() + "/newList.lis",
                           "onet=" + tempDir.path() + "/newNet.net",
                           "cubes=true",
                           "cubelist=" + reducedCubeList,
                           "cubemeasures=true",
                           "retain_reference=true"};

  UserInterface options(APP_XML, args);
  Pvl appLog;

  QList<ControlPoint *> points = network->GetPoints();

  points[0]->SetRefMeasure(points[0]->getMeasures()[1]);

  cnetextract( *network, options, &appLog );

  ControlNet outputNet(options.GetFileName("ONET"));

  ASSERT_EQ(int(appLog.findGroup("ResultSummary")["NonCubePoints"]), 3);
  ASSERT_EQ(int(appLog.findGroup("ResultSummary")["NonCubeMeasures"]), 28);

  ASSERT_EQ(network->GetNumPoints() - outputNet.GetNumPoints(), 3);
  ASSERT_EQ(network->GetNumMeasures() - outputNet.GetNumMeasures(), 27);

  FileName nonCubePointsFile(options.GetAsString("prefix").toStdString() + "NonCubePoints.txt");
  EXPECT_TRUE(nonCubePointsFile.fileExists());

  FileName nonCubeMeasuresFile(options.GetAsString("prefix").toStdString() + "NonCubeMeasures.txt");
  EXPECT_TRUE(nonCubeMeasuresFile.fileExists());
}

TEST_F(ThreeImageNetwork, FunctionalTestCnetextractLatlon) {
  QVector<QString> args = {"fromlist=" + cubeListFile,
                           "prefix=" + tempDir.path() + "/",
                           "tolist=" + tempDir.path() + "/newList.lis",
                           "onet=" + tempDir.path() + "/newNet.net",
                           "latlon=true",
                           "minlat=0", "maxlat=2",
                           "minlon=0", "maxlon=1"};

  UserInterface options(APP_XML, args);
  Pvl appLog;

  cnetextract( *network, options, &appLog );

  ControlNet outputNet(options.GetFileName("ONET"));

  ASSERT_EQ(int(appLog.findGroup("ResultSummary")["LatLonOutOfRange"]), 10);
  ASSERT_EQ(int(appLog.findGroup("ResultSummary")["NoLatLonPoints"]), 0);

  ASSERT_EQ(network->GetNumPoints() - outputNet.GetNumPoints(), 10);
  ASSERT_EQ(network->GetNumMeasures() - outputNet.GetNumMeasures(), 25);

  FileName outOfRangeFile(options.GetAsString("prefix").toStdString() + "LatLonOutOfRange.txt");
  EXPECT_TRUE(outOfRangeFile.fileExists());

  FileName noLatLonPointsFile(options.GetAsString("prefix").toStdString() + "NoLatLonPoints.txt");
  EXPECT_FALSE(noLatLonPointsFile.fileExists());
}
