#include <iostream>

#include <QFile>
#include <QHash>
#include <QSet>
#include <QString>
#include <QStringList>
#include <QTextStream>

#include "cnetcombinept.h"
#include "Angle.h"
#include "ControlNet.h"
#include "ControlPoint.h"
#include "ControlMeasure.h"
#include "Distance.h"
#include "FileName.h"
#include "TempFixtures.h"
#include "Latitude.h"
#include "Longitude.h"
#include "SurfacePoint.h"
#include "TestUtilities.h"
#include "gmock/gmock.h"
#include "UserInterface.h"

using namespace Isis;

static QString APP_XML = QString::fromStdString(FileName("$ISISROOT/bin/xml/cnetcombinept.xml").expanded());

class CombineNetworks : public TempTestingFiles {
  protected:
    QString firstNetFile;
    QString secondNetFile;
    QString thirdNetFile;
    QString listFile;

    void SetUp() override {
      TempTestingFiles::SetUp();

      firstNetFile = tempDir.path() + "/first.net";
      secondNetFile = tempDir.path() + "/second.net";
      thirdNetFile = tempDir.path() + "/third.net";
      listFile = tempDir.path() + "/secondthird.lis";

      ControlNet firstNet;

      ControlPoint *point1a = new ControlPoint("1a");
      ControlMeasure *measure1aFoo = new ControlMeasure();
      measure1aFoo->SetCubeSerialNumber("foo");
      measure1aFoo->SetCoordinate(350.0, 350.0);
      point1a->Add(measure1aFoo);
      ControlMeasure *measure1aBar = new ControlMeasure();
      measure1aBar->SetCubeSerialNumber("bar");
      measure1aBar->SetCoordinate(50.0, 250.0);
      point1a->Add(measure1aBar);
      firstNet.AddPoint(point1a);

      ControlPoint *point1b = new ControlPoint("1b");
      ControlMeasure *measure1bFoo = new ControlMeasure();
      measure1bFoo->SetCubeSerialNumber("foo");
      measure1bFoo->SetCoordinate(400.0, 400.0);
      point1b->Add(measure1bFoo);
      ControlMeasure *measure1bBaz = new ControlMeasure();
      measure1bBaz->SetCubeSerialNumber("baz");
      measure1bBaz->SetCoordinate(300.0, 100.0);
      point1b->Add(measure1bBaz);
      firstNet.AddPoint(point1b);

      firstNet.Write(firstNetFile);


      ControlNet secondNet;

      ControlPoint *point2a = new ControlPoint("2a");
      ControlMeasure *measure2aBar = new ControlMeasure();
      measure2aBar->SetCubeSerialNumber("bar");
      measure2aBar->SetCoordinate(50.0, 250.0);
      point2a->Add(measure2aBar);
      ControlMeasure *measure2aBaz = new ControlMeasure();
      measure2aBaz->SetCubeSerialNumber("baz");
      measure2aBaz->SetCoordinate(250.0, 50.0);
      point2a->Add(measure2aBaz);
      secondNet.AddPoint(point2a);

      ControlPoint *point2b = new ControlPoint("2b");
      ControlMeasure *measure2bBar = new ControlMeasure();
      measure2bBar->SetCubeSerialNumber("bar");
      measure2bBar->SetCoordinate(150.0, 300.0);
      point2b->Add(measure2bBar);
      ControlMeasure *measure2bFoo = new ControlMeasure();
      measure2bFoo->SetCubeSerialNumber("foo");
      measure2bFoo->SetCoordinate(450.0, 400.0);
      point2b->Add(measure2bFoo);
      secondNet.AddPoint(point2b);

      secondNet.Write(secondNetFile);


      ControlNet thirdNet;

      ControlPoint *point3a = new ControlPoint("3a");
      ControlMeasure *measure3aBaz = new ControlMeasure();
      measure3aBaz->SetCubeSerialNumber("baz");
      measure3aBaz->SetCoordinate(300.0, 100.0);
      point3a->Add(measure3aBaz);
      ControlMeasure *measure3aBar = new ControlMeasure();
      measure3aBar->SetCubeSerialNumber("bar");
      measure3aBar->SetCoordinate(100.0, 300.0);
      point3a->Add(measure3aBar);
      thirdNet.AddPoint(point3a);

      ControlPoint *point3b = new ControlPoint("3b");
      ControlMeasure *measure3bBaz = new ControlMeasure();
      measure3bBaz->SetCubeSerialNumber("baz");
      measure3bBaz->SetCoordinate(350.0, 100.0);
      point3b->Add(measure3bBaz);
      ControlMeasure *measure3bFoo = new ControlMeasure();
      measure3bFoo->SetCubeSerialNumber("foo");
      measure3bFoo->SetCoordinate(450.0, 400.0);
      point3b->Add(measure3bFoo);
      thirdNet.AddPoint(point3b);

      thirdNet.Write(thirdNetFile);

      QFile listFileHandle(listFile);
      listFileHandle.open(QIODevice::WriteOnly);
      QTextStream listFileStream(&listFileHandle);
      listFileStream << secondNetFile << "\n";
      listFileStream << thirdNetFile << "\n";
      listFileHandle.close();
    }
};

TEST_F(CombineNetworks, FunctionalTestCnetcombineptDistance) {
  QString distance1NetFile = tempDir.path()+"/distance1.net";
  QVector<QString> args1 = {"cnetbase="+firstNetFile,
                            "cnetlist="+listFile,
                            "imagetol=1",
                            "onet="+distance1NetFile};
  UserInterface ui1(APP_XML, args1);

  cnetcombinept(ui1);

  ControlNet network1(distance1NetFile);
  EXPECT_EQ(network1.GetNumPoints(), 3);
  EXPECT_EQ(network1.GetNumMeasures(), 9);

  ControlPoint *point1aMerged1 = network1.GetPoint("1a");
  EXPECT_EQ(point1aMerged1->GetNumMeasures(), 3);
  EXPECT_EQ(point1aMerged1->GetMeasure("foo")->GetSample(), 350.0);
  EXPECT_EQ(point1aMerged1->GetMeasure("foo")->GetLine(), 350.0);
  EXPECT_EQ(point1aMerged1->GetMeasure("bar")->GetSample(), 50.0);
  EXPECT_EQ(point1aMerged1->GetMeasure("bar")->GetLine(), 250.0);
  EXPECT_EQ(point1aMerged1->GetMeasure("baz")->GetSample(), 250.0);
  EXPECT_EQ(point1aMerged1->GetMeasure("baz")->GetLine(), 50.0);

  ControlPoint *point1bMerged1 = network1.GetPoint("1b");
  EXPECT_EQ(point1bMerged1->GetNumMeasures(), 3);
  EXPECT_EQ(point1bMerged1->GetMeasure("foo")->GetSample(), 400.0);
  EXPECT_EQ(point1bMerged1->GetMeasure("foo")->GetLine(), 400.0);
  EXPECT_EQ(point1bMerged1->GetMeasure("bar")->GetSample(), 100.0);
  EXPECT_EQ(point1bMerged1->GetMeasure("bar")->GetLine(), 300.0);
  EXPECT_EQ(point1bMerged1->GetMeasure("baz")->GetSample(), 300.0);
  EXPECT_EQ(point1bMerged1->GetMeasure("baz")->GetLine(), 100.0);

  ControlPoint *point2bMerged1 = network1.GetPoint("2b");
  EXPECT_EQ(point2bMerged1->GetNumMeasures(), 3);
  EXPECT_EQ(point2bMerged1->GetMeasure("foo")->GetSample(), 450.0);
  EXPECT_EQ(point2bMerged1->GetMeasure("foo")->GetLine(), 400.0);
  EXPECT_EQ(point2bMerged1->GetMeasure("bar")->GetSample(), 150.0);
  EXPECT_EQ(point2bMerged1->GetMeasure("bar")->GetLine(), 300.0);
  EXPECT_EQ(point2bMerged1->GetMeasure("baz")->GetSample(), 350.0);
  EXPECT_EQ(point2bMerged1->GetMeasure("baz")->GetLine(), 100.0);


  QString distance55NetFile = tempDir.path()+"/distance55.net";
  QVector<QString> args2 = {"cnetbase="+firstNetFile,
                            "cnetlist="+listFile,
                            "imagetol=55",
                            "onet="+distance55NetFile};
  UserInterface ui2(APP_XML, args2);

  cnetcombinept(ui2);

  ControlNet network2(distance55NetFile);
  EXPECT_EQ(network2.GetNumPoints(), 2);
  EXPECT_EQ(network2.GetNumMeasures(), 6);

  ControlPoint *point1aMerged55 = network2.GetPoint("1a");
  EXPECT_EQ(point1aMerged55->GetNumMeasures(), 3);
  EXPECT_EQ(point1aMerged55->GetMeasure("foo")->GetSample(), 350.0);
  EXPECT_EQ(point1aMerged55->GetMeasure("foo")->GetLine(), 350.0);
  EXPECT_EQ(point1aMerged55->GetMeasure("bar")->GetSample(), 50.0);
  EXPECT_EQ(point1aMerged55->GetMeasure("bar")->GetLine(), 250.0);
  EXPECT_EQ(point1aMerged55->GetMeasure("baz")->GetSample(), 250.0);
  EXPECT_EQ(point1aMerged55->GetMeasure("baz")->GetLine(), 50.0);

  ControlPoint *point1bMerged55 = network2.GetPoint("1b");
  EXPECT_EQ(point1bMerged55->GetNumMeasures(), 3);
  EXPECT_EQ(point1bMerged55->GetMeasure("foo")->GetSample(), 400.0);
  EXPECT_EQ(point1bMerged55->GetMeasure("foo")->GetLine(), 400.0);
  EXPECT_EQ(point1bMerged55->GetMeasure("bar")->GetSample(), 150.0);
  EXPECT_EQ(point1bMerged55->GetMeasure("bar")->GetLine(), 300.0);
  EXPECT_EQ(point1bMerged55->GetMeasure("baz")->GetSample(), 300.0);
  EXPECT_EQ(point1bMerged55->GetMeasure("baz")->GetLine(), 100.0);


  QString distance200NetFile = tempDir.path()+"/distance200.net";
  QVector<QString> args3 = {"cnetbase="+firstNetFile,
                            "cnetlist="+listFile,
                            "imagetol=200",
                            "onet="+distance200NetFile};
  UserInterface ui3(APP_XML, args3);

  cnetcombinept(ui3);

  ControlNet network3(distance200NetFile);
  EXPECT_EQ(network3.GetNumPoints(), 1);
  EXPECT_EQ(network3.GetNumMeasures(), 3);

  ControlPoint *point1aMerged200 = network3.GetPoint("1a");
  EXPECT_EQ(point1aMerged200->GetNumMeasures(), 3);
  EXPECT_EQ(point1aMerged200->GetMeasure("foo")->GetSample(), 350.0);
  EXPECT_EQ(point1aMerged200->GetMeasure("foo")->GetLine(), 350.0);
  EXPECT_EQ(point1aMerged200->GetMeasure("bar")->GetSample(), 50.0);
  EXPECT_EQ(point1aMerged200->GetMeasure("bar")->GetLine(), 250.0);
  EXPECT_EQ(point1aMerged200->GetMeasure("baz")->GetSample(), 300.0);
  EXPECT_EQ(point1aMerged200->GetMeasure("baz")->GetLine(), 100.0);
}

TEST_F(CombineNetworks, FunctionalTestCnetcombineptLog) {
  QString logFileName = tempDir.path()+"/merged.log";
  QString mergedNetFile = tempDir.path()+"/merged.net";
  QVector<QString> args = {"cnetbase="+firstNetFile,
                           "cnetlist="+listFile,
                           "imagetol=55",
                           "onet="+mergedNetFile,
                           "logfile="+logFileName};
  UserInterface ui(APP_XML, args);

  cnetcombinept(ui);

  QFile logFileHandle(logFileName);
  logFileHandle.open(QIODevice::ReadOnly | QIODevice::Text);
  QString headerLine = logFileHandle.readLine();
  QStringList headerColumns = headerLine.trimmed().split(",");
  ASSERT_EQ(headerColumns.size(), 4);
  EXPECT_PRED_FORMAT2(AssertQStringsEqual, headerColumns[0], "pointID");
  EXPECT_PRED_FORMAT2(AssertQStringsEqual, headerColumns[1], "startNumMeasures");
  EXPECT_PRED_FORMAT2(AssertQStringsEqual, headerColumns[2], "endNumMeasures");
  EXPECT_PRED_FORMAT2(AssertQStringsEqual, headerColumns[3], "mergedIDs");
  QHash<QString, QSet<QString>> merges;
  QHash<QString, int> startSizes;
  QHash<QString, int> endSizes;
  short nLines = 0;
  while (!logFileHandle.atEnd()) {
    nLines++;
    QStringList splitLine = QString(logFileHandle.readLine()).trimmed().split(',');
    ASSERT_EQ(splitLine.size(), 4);
    startSizes.insert(splitLine[0], splitLine[1].toInt());
    endSizes.insert(splitLine[0], splitLine[2].toInt());
    QSet<QString> pointMerges;
    foreach(QString pointId, splitLine[3].split(' ')) {
      pointMerges.insert(pointId);
    }
    EXPECT_EQ(pointMerges.size(), splitLine[3].split(' ').size());
    merges.insert(splitLine[0], pointMerges);
  }
  EXPECT_EQ(startSizes.size(), 2);
  ASSERT_TRUE(startSizes.contains("1a"));
  EXPECT_EQ(startSizes["1a"], 2);
  ASSERT_TRUE(startSizes.contains("1b"));
  EXPECT_EQ(startSizes["1b"], 2);
  EXPECT_EQ(endSizes.size(), 2);
  ASSERT_TRUE(endSizes.contains("1a"));
  EXPECT_EQ(endSizes["1a"], 3);
  ASSERT_TRUE(endSizes.contains("1b"));
  EXPECT_EQ(endSizes["1b"], 3);
  EXPECT_EQ(merges.size(), 2);
  ASSERT_TRUE(merges.contains("1a"));
  EXPECT_EQ(merges["1a"].size(), 1);
  EXPECT_TRUE(merges["1a"].contains("2a"));
  ASSERT_TRUE(merges.contains("1b"));
  EXPECT_EQ(merges["1b"].size(), 3);
  EXPECT_TRUE(merges["1b"].contains("3a"));
  EXPECT_TRUE(merges["1b"].contains("2b"));
  EXPECT_TRUE(merges["1b"].contains("3b"));
  EXPECT_EQ(nLines, 2);
}

TEST_F(CombineNetworks, FunctionalTestCnetcombineptList) {
  QString specifiedNetFile = tempDir.path()+"/specified.net";
  QVector<QString> args1 = {"cnetbase="+firstNetFile,
                            "cnetfrom="+secondNetFile,
                            "onet="+specifiedNetFile};
  UserInterface ui1(APP_XML, args1);

  cnetcombinept(ui1);

  QString shortListFile = tempDir.path()+"/second.lis";
  QFile shortListFileHandle(shortListFile);
  shortListFileHandle.open(QIODevice::WriteOnly);
  QTextStream shortListFileStream(&shortListFileHandle);
  shortListFileStream << secondNetFile << "\n";
  shortListFileHandle.close();
  QString listNetFile = tempDir.path()+"/list.net";
  QVector<QString> args2 = {"cnetbase="+firstNetFile,
                            "cnetlist="+shortListFile,
                            "onet="+listNetFile};
  UserInterface ui2(APP_XML, args2);

  cnetcombinept(ui2);

  ControlNet specifiedNet(specifiedNetFile);
  ControlNet listNet(listNetFile);

  EXPECT_EQ(specifiedNet.GetNumPoints(), listNet.GetNumPoints());
  EXPECT_EQ(specifiedNet.GetNumMeasures(), listNet.GetNumMeasures());
  foreach(QString pointId, specifiedNet.GetPointIds()) {
    ControlPoint *specifiedPoint = specifiedNet.GetPoint(pointId);
    ControlPoint *listPoint = listNet.GetPoint(pointId);
    EXPECT_EQ(specifiedPoint->GetNumMeasures(), listPoint->GetNumMeasures())
          << pointId.toStdString();
    foreach(QString serial, specifiedPoint->getCubeSerialNumbers()) {
      EXPECT_EQ(*(specifiedPoint->GetMeasure(serial)), *(listPoint->GetMeasure(serial)))
            << pointId.toStdString() << ", " << serial.toStdString();
    }
  }
}

TEST_F(CombineNetworks, FunctionalTestCnetcombineptNetworkArgs) {
  QString outNetFile = tempDir.path()+"/combined.net";
  QString networkId = "Test1234";
  QString networkDescription = "This is a test network for the cnetcombinept application.";
  QVector<QString> args = {"cnetbase="+firstNetFile,
                           "onet="+outNetFile,
                           "networkid="+networkId,
                           "description="+networkDescription};
  UserInterface ui(APP_XML, args);

  cnetcombinept(ui);

  ControlNet outNet(outNetFile);
  EXPECT_PRED_FORMAT2(AssertQStringsEqual, outNet.GetNetworkId(), networkId);
  EXPECT_PRED_FORMAT2(AssertQStringsEqual, outNet.Description(), networkDescription);

}

TEST_F(CombineNetworks, FunctionalTestCnetcombineptSNList) {
  QString outNetFile = tempDir.path()+"/combined.net";
  QString outSNFile = tempDir.path()+"/serialnumbers.lis";
  QVector<QString> args = {"cnetbase="+firstNetFile,
                           "onet="+outNetFile,
                           "tosn="+outSNFile};
  UserInterface ui(APP_XML, args);

  cnetcombinept(ui);

  QFile outSNFileHandle(outSNFile);
  outSNFileHandle.open(QIODevice::ReadOnly | QIODevice::Text);
  QSet<QString> serials;
  int numLines = 0;
  while (!outSNFileHandle.atEnd()) {
    QString snLine = outSNFileHandle.readLine();
    serials.insert(snLine.trimmed());
    numLines++;
  }

  EXPECT_EQ(numLines, 3);
  EXPECT_EQ(serials.size(), 3);
  EXPECT_TRUE(serials.contains("foo"));
  EXPECT_TRUE(serials.contains("bar"));
  EXPECT_TRUE(serials.contains("baz"));
}

TEST_F(CombineNetworks, FunctionalTestCnetcombineptCleanNet) {
  ControlNet firstNet(firstNetFile);
  ControlPoint *ignoredPoint = new ControlPoint("ignored");
  ignoredPoint->SetIgnored(true);
  firstNet.AddPoint(ignoredPoint);
  ControlPoint *rejectedPoint = new ControlPoint("rejected");
  rejectedPoint->SetRejected(true);
  firstNet.AddPoint(rejectedPoint);
  firstNet.Write(firstNetFile);

  QString cleanNetFile = tempDir.path()+"/cleanCombined.net";
  QVector<QString> args1 = {"cnetbase="+firstNetFile,
                            "imagetol=1",
                            "onet="+cleanNetFile,
                            "cleannet=true"};
  UserInterface ui1(APP_XML, args1);

  cnetcombinept(ui1);

  ControlNet cleanNet(cleanNetFile);
  EXPECT_FALSE(cleanNet.ContainsPoint("ignored"));
  EXPECT_FALSE(cleanNet.ContainsPoint("rejected"));

  QString dirtyNetFile = tempDir.path()+"/dirtyCombined.net";
  QVector<QString> args2 = {"cnetbase="+firstNetFile,
                            "imagetol=1",
                            "onet="+dirtyNetFile,
                            "cleannet=false"};
  UserInterface ui2(APP_XML, args2);

  cnetcombinept(ui2);

  ControlNet dirtyNet(dirtyNetFile);
  EXPECT_TRUE(dirtyNet.ContainsPoint("ignored"));
  EXPECT_TRUE(dirtyNet.ContainsPoint("rejected"));
}

TEST_F(CombineNetworks, FunctionalTestCnetcombineptCleanMeasures) {
  ControlNet firstNet(firstNetFile);
  ControlPoint *ignoredPoint = new ControlPoint("ignored");
  ControlMeasure *ignoredMeasure = new ControlMeasure();
  ignoredMeasure->SetCoordinate(100, 100);
  ignoredMeasure->SetCubeSerialNumber("foo");
  ignoredMeasure->SetIgnored(true);
  ignoredPoint->Add(ignoredMeasure);
  ControlMeasure *goodMeasure = new ControlMeasure();
  goodMeasure->SetCoordinate(200, 200);
  goodMeasure->SetCubeSerialNumber("bar");
  ignoredPoint->Add(goodMeasure);
  firstNet.AddPoint(ignoredPoint);
  ControlPoint *rejectedPoint = new ControlPoint("rejected");
  ControlMeasure *rejectedMeasure = new ControlMeasure();
  rejectedMeasure->SetCoordinate(150, 150);
  rejectedMeasure->SetCubeSerialNumber("foo");
  rejectedMeasure->SetRejected(true);
  rejectedPoint->Add(rejectedMeasure);
  ControlMeasure *anotherGoodMeasure = new ControlMeasure();
  anotherGoodMeasure->SetCoordinate(250, 250);
  anotherGoodMeasure->SetCubeSerialNumber("bar");
  rejectedPoint->Add(anotherGoodMeasure);
  firstNet.AddPoint(rejectedPoint);
  firstNet.Write(firstNetFile);

  QString cleanNetFile = tempDir.path()+"/cleanCombined.net";
  QVector<QString> args1 = {"cnetbase="+firstNetFile,
                            "imagetol=1",
                            "onet="+cleanNetFile,
                            "cleanmeasures=true"};
  UserInterface ui1(APP_XML, args1);

  cnetcombinept(ui1);

  ControlNet cleanNet(cleanNetFile);
  EXPECT_FALSE(cleanNet.GetPoint("ignored")->HasSerialNumber("foo"));
  EXPECT_TRUE(cleanNet.GetPoint("ignored")->HasSerialNumber("bar"));
  EXPECT_FALSE(cleanNet.GetPoint("rejected")->HasSerialNumber("foo"));
  EXPECT_TRUE(cleanNet.GetPoint("rejected")->HasSerialNumber("bar"));

  QString dirtyNetFile = tempDir.path()+"/dirtyCombined.net";
  QVector<QString> args2 = {"cnetbase="+firstNetFile,
                            "imagetol=1",
                            "onet="+dirtyNetFile,
                            "cleanmeasures=false"};
  UserInterface ui2(APP_XML, args2);

  cnetcombinept(ui2);

  ControlNet dirtyNet(dirtyNetFile);
  EXPECT_TRUE(dirtyNet.GetPoint("ignored")->HasSerialNumber("foo"));
  EXPECT_TRUE(dirtyNet.GetPoint("ignored")->HasSerialNumber("bar"));
  EXPECT_TRUE(dirtyNet.GetPoint("rejected")->HasSerialNumber("foo"));
  EXPECT_TRUE(dirtyNet.GetPoint("rejected")->HasSerialNumber("bar"));
}

TEST_F(CombineNetworks, FunctionalTestCnetcombineptSetApriori) {
  ControlNet firstNet(firstNetFile);
  SurfacePoint aprioriPoint(Latitude(30.0, Angle::Degrees),
                            Longitude(0.0, Angle::Degrees),
                            Distance(1000.0, Distance::Kilometers));
  SurfacePoint adjustedPoint(Latitude(45.0, Angle::Degrees),
                             Longitude(10.0, Angle::Degrees),
                             Distance(1010.0, Distance::Kilometers));
  ControlPoint *newPoint = new ControlPoint("surface");
  newPoint->SetAprioriSurfacePoint(aprioriPoint);
  newPoint->SetAdjustedSurfacePoint(adjustedPoint);
  ControlMeasure *goodMeasure = new ControlMeasure();
  goodMeasure->SetCoordinate(200, 200);
  goodMeasure->SetCubeSerialNumber("bar");
  newPoint->Add(goodMeasure);
  firstNet.AddPoint(newPoint);
  firstNet.Write(firstNetFile);

  QString keepAprioriNetFile = tempDir.path()+"/keepapriori.net";
  QVector<QString> args1 = {"cnetbase="+firstNetFile,
                            "imagetol=1",
                            "onet="+keepAprioriNetFile,
                            "setaprioribest=false"};
  UserInterface ui1(APP_XML, args1);

  cnetcombinept(ui1);

  ControlNet keepAprioriNet(keepAprioriNetFile);
  EXPECT_EQ(keepAprioriNet.GetPoint("surface")->GetAprioriSurfacePoint(), aprioriPoint);

  QString setAprioriNetFile = tempDir.path()+"/setapriori.net";
  QVector<QString> args2 = {"cnetbase="+firstNetFile,
                            "imagetol=1",
                            "onet="+setAprioriNetFile,
                            "setaprioribest=true"};
  UserInterface ui2(APP_XML, args2);

  cnetcombinept(ui2);

  ControlNet setAprioriNet(setAprioriNetFile);
  EXPECT_EQ(setAprioriNet.GetPoint("surface")->GetAprioriSurfacePoint(), adjustedPoint);
}