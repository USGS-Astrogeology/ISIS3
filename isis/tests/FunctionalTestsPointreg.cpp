#include "pointreg.h"

#include "NetworkFixtures.h"
#include "TestUtilities.h"
#include "UserInterface.h"
#include "ControlNet.h"
#include "LineManager.h"

#include "gmock/gmock.h"

using namespace Isis;

static QString APP_XML = QString::fromStdString(FileName("$ISISROOT/bin/xml/pointreg.xml").expanded());

TEST_F(ThreeImageNetwork, FunctionalTestPointregDefault) {
  Pvl log;
  QTemporaryDir prefix;
  QString flatFilePath = prefix.path() + "/flatfile.csv";
  QString outNetPath = prefix.path() + "/outNet.net";
  QVector<QString> args = { "fromlist=" + cubeListFile,
                            "cnet=" + networkFile,
                            "deffile=data/threeImageNetwork/autoRegTemplate.def",
                            "flatfile=" + flatFilePath,
                            "onet=" + outNetPath };
  UserInterface options(APP_XML, args);

  try {
    pointreg(options, &log);
  }
  catch (IException &e) {
    FAIL() <<  e.toString().c_str() << std::endl;
  }

  // Check app log
  PvlGroup points = log.findGroup("Points");
  PvlGroup measures = log.findGroup("Measures");
  EXPECT_EQ(int(points["Total"]), 16);
  EXPECT_EQ(int(measures["Registered"]), 3);

  // Check flatFile
  QFile flatFile(flatFilePath);
  EXPECT_TRUE(flatFile.size() > 0) ;

  // Check output control network
  ControlNet outNet(outNetPath);
  EXPECT_EQ(int(outNet.GetNumPoints()), 16);
  EXPECT_EQ(int(outNet.GetNumMeasures()), 41);
}

TEST_F(ThreeImageNetwork, FunctionalTestPointregFailOptions) {
  Pvl log;
  QTemporaryDir prefix;
  QString flatFilePath = prefix.path() + "/flatfile.csv";
  QString outNetPath = prefix.path() + "/outNet.net";
  QVector<QString> args = { "fromlist=" + cubeListFile,
                            "cnet=" + networkFile,
                            "deffile=data/threeImageNetwork/autoRegTemplate.def",
                            "flatfile=" + flatFilePath,
                            "onet=" + outNetPath,
                            "outputignored=no",
                            "outputfailed=no",
                            "points=all",
                            "measures=all" };
  UserInterface options(APP_XML, args);

  try {
    pointreg(options, &log);
  }
  catch (IException &e) {
    FAIL() <<  e.toString().c_str() << std::endl;
  }

  // Check app log
  PvlGroup points = log.findGroup("Points");
  PvlGroup measures = log.findGroup("Measures");
  EXPECT_EQ(int(points["Total"]), 3);
  EXPECT_EQ(int(measures["Registered"]), 3);

  // Check flatFile
  QFile flatFile(flatFilePath);
  EXPECT_TRUE(flatFile.size() > 0) ;

  // Check output control network
  ControlNet outNet(outNetPath);
  EXPECT_EQ(int(outNet.GetNumPoints()), 3);
  EXPECT_EQ(int(outNet.GetNumMeasures()), 6);
}

TEST_F(ThreeImageNetwork, FunctionalTestPointregOutputOptionsA) {
  // Output ignored, register ignored
  Pvl log;
  QTemporaryDir prefix;
  QString flatFilePath = prefix.path() + "/flatfile.csv";
  QString outNetPath = prefix.path() + "/outNet.net";
  QVector<QString> args = { "fromlist=" + cubeListFile,
                            "cnet=" + networkFile,
                            "deffile=data/threeImageNetwork/autoRegTemplate.def",
                            "flatfile=" + flatFilePath,
                            "onet=" + outNetPath,
                            "outputfailed=no",
                            "points=ignored" };
  UserInterface options(APP_XML, args);

  try {
    pointreg(options, &log);
  }
  catch (IException &e) {
    FAIL() <<  e.toString().c_str() << std::endl;
  }

  // Check app log
  PvlGroup points = log.findGroup("Points");
  PvlGroup measures = log.findGroup("Measures");
  PvlGroup surface= log.findGroup("SurfaceModelFailures");
  EXPECT_EQ(int(points["Total"]), 16);
  EXPECT_EQ(int(measures["Registered"]), 0);
  EXPECT_EQ(int(surface["SurfaceModelNotEnoughValidData"]), 0);

  // Check flatFile
  QFile flatFile(flatFilePath);
  EXPECT_TRUE(flatFile.size() > 0) ;

  // Check output control network
  ControlNet outNet(outNetPath);
  EXPECT_EQ(int(outNet.GetNumPoints()), 16);
  EXPECT_EQ(int(outNet.GetNumMeasures()), 41);
}

TEST_F(ThreeImageNetwork, FunctionalTestPointregOutputOptionsB) {
  // Output ignored only
  Pvl log;
  QTemporaryDir prefix;
  QString flatFilePath = prefix.path() + "/flatfile.csv";
  QString outNetPath = prefix.path() + "/outNet.net";
  QVector<QString> args = { "fromlist=" + cubeListFile,
                            "cnet=" + networkFile,
                            "deffile=data/threeImageNetwork/autoRegTemplate.def",
                            "flatfile=" + flatFilePath,
                            "onet=" + outNetPath,
                            "outputfailed=no",
                            "points=nonignored" };
  UserInterface options(APP_XML, args);

  try {
    pointreg(options, &log);
  }
  catch (IException &e) {
    FAIL() <<  e.toString().c_str() << std::endl;
  }

  // Check app log
  PvlGroup points = log.findGroup("Points");
  PvlGroup measures = log.findGroup("Measures");
  PvlGroup surface= log.findGroup("SurfaceModelFailures");
  EXPECT_EQ(int(points["Total"]), 16);
  EXPECT_EQ(int(measures["Registered"]), 3);
  EXPECT_EQ(int(surface["SurfaceModelNotEnoughValidData"]), 15);

  // Check flatFile
  QFile flatFile(flatFilePath);
  EXPECT_TRUE(flatFile.size() > 0) ;

  // Check output control network
  ControlNet outNet(outNetPath);
  EXPECT_EQ(int(outNet.GetNumPoints()), 16);
  EXPECT_EQ(int(outNet.GetNumMeasures()), 19);
}

TEST_F(ThreeImageNetwork, FunctionalTestPointregOutputOptionsC) {
  // Output Unmeasured Only
  Pvl log;
  QTemporaryDir prefix;
  QString flatFilePath = prefix.path() + "/flatfile.csv";
  QString outNetPath = prefix.path() + "/outNet.net";
  QVector<QString> args = { "fromlist=" + cubeListFile,
                            "cnet=" + networkFile,
                            "deffile=data/threeImageNetwork/autoRegTemplate.def",
                            "flatfile=" + flatFilePath,
                            "onet=" + outNetPath,
                            "outputignored=no",
                            "points=nonignored" };
  UserInterface options(APP_XML, args);

  try {
    pointreg(options, &log);
  }
  catch (IException &e) {
    FAIL() <<  e.toString().c_str() << std::endl;
  }

  // Check app log
  PvlGroup points = log.findGroup("Points");
  PvlGroup measures = log.findGroup("Measures");
  PvlGroup surface= log.findGroup("SurfaceModelFailures");
  EXPECT_EQ(int(points["Total"]), 3);
  EXPECT_EQ(int(measures["Registered"]), 3);
  EXPECT_EQ(int(surface["SurfaceModelNotEnoughValidData"]), 15);

  // Check flatFile
  QFile flatFile(flatFilePath);
  EXPECT_TRUE(flatFile.size() > 0) ;

  // Check output control network
  ControlNet outNet(outNetPath);
  EXPECT_EQ(int(outNet.GetNumPoints()), 3);
  EXPECT_EQ(int(outNet.GetNumMeasures()), 9);
}

TEST_F(ThreeImageNetwork, FunctionalTestPointregRegisterOptionsIgnored) {
  QTemporaryDir prefix;
  QString flatFilePath = prefix.path() + "/flatfile.csv";
  QString outNetPath = prefix.path() + "/outNet.net";
  QVector<QString> args = { "fromlist=" + cubeListFile,
                            "cnet=" + networkFile,
                            "deffile=data/threeImageNetwork/autoRegTemplate.def",
                            "flatfile=" + flatFilePath,
                            "onet=" + outNetPath,
                            "outputfailed=no",
                            "outputignored=no",
                            "points=ignored" };
  UserInterface options(APP_XML, args);  // Register ignored only
  Pvl log;

  try {
    pointreg(options, &log);
  }
  catch (IException &e) {
    FAIL() <<  e.toString().c_str() << std::endl;
  }

  // Check app log
  PvlGroup points = log.findGroup("Points");
  PvlGroup measures = log.findGroup("Measures");

  EXPECT_EQ(int(points["Total"]), 16);
  EXPECT_EQ(int(measures["Registered"]), 0);
}

TEST_F(ThreeImageNetwork, FunctionalTestPointregRegisterOptionsValid) {
  QTemporaryDir prefix;
  QString flatFilePath = prefix.path() + "/flatfile.csv";
  QString outNetPath = prefix.path() + "/outNet.net";
  QVector<QString> args = { "fromlist=" + cubeListFile,
                            "cnet=" + networkFile,
                            "deffile=data/threeImageNetwork/autoRegTemplate.def",
                            "flatfile=" + flatFilePath,
                            "onet=" + outNetPath,
                            "outputfailed=no",
                            "outputignored=no" };
  UserInterface options(APP_XML, args);  // Register valid only
  Pvl log;

  try {
    pointreg(options, &log);
  }
  catch (IException &e) {
    FAIL() <<  e.toString().c_str() << std::endl;
  }

  // Check app log
  PvlGroup points = log.findGroup("Points");
  PvlGroup measures = log.findGroup("Measures");

  EXPECT_EQ(int(points["Total"]), 3);
  EXPECT_EQ(int(measures["Registered"]), 3);
}

TEST_F(ThreeImageNetwork, FunctionalTestPointregValidation) {
  QTemporaryDir prefix;
  QString flatFilePath = prefix.path() + "/flatfile.csv";
  QString outNetPath = prefix.path() + "/outNet.net";

  QString falsePosPath = prefix.path() + "/falsePos.csv";

  QVector<QString> args = { "fromlist=" + cubeListFile,
                            "cnet=" + networkFile,
                            "deffile=data/threeImageNetwork/autoRegTemplate.def",
                            "flatfile=" + flatFilePath,
                            "onet=" + outNetPath,
                            "validate=after",
                            "falsepositives=" + falsePosPath,
                            "revert=no", "shift=0.1",
                            "points=nonignored" };
  UserInterface options(APP_XML, args);  // Test the validation without reverting failed registration
  Pvl log;

  try {
    pointreg(options, &log);
  }
  catch (IException &e) {
    FAIL() <<  e.toString().c_str() << std::endl;
  }

  // Check app log
  PvlGroup points = log.findGroup("Points");
  PvlGroup measures = log.findGroup("Measures");
  PvlGroup valid = log.findGroup("ValidationStatistics");

  EXPECT_EQ(int(points["Total"]), 16);
  EXPECT_EQ(int(measures["Registered"]), 3);
  EXPECT_EQ(int(valid["Total"]), 3);

  QFile falsePos(falsePosPath);  // Should be empty

  EXPECT_TRUE(falsePos.size() > 140);  // 140 is the size of the empty table due to column names
}

TEST_F(ThreeImageNetwork, FunctionalTestPointregValidationRevert) {
  QTemporaryDir prefix;
  QString flatFilePath = prefix.path() + "/flatfile.csv";
  QString outNetPath = prefix.path() + "/outNet.net";

  QString falsePosPath = prefix.path() + "/falsePos.csv";

  QVector<QString> args = { "fromlist=" + cubeListFile,
                            "cnet=" + networkFile,
                            "deffile=data/threeImageNetwork/autoRegTemplate.def",
                            "flatfile=" + flatFilePath,
                            "onet=" + outNetPath,
                            "validate=only",
                            "falsepositives=" + falsePosPath,
                            "search=7", "shift=0.1",
                            "points=nonignored" };
  UserInterface options(APP_XML, args);  // Test the validation with reverting failed registration
  Pvl log;

  try {
    pointreg(options, &log);
  }
  catch (IException &e) {
    FAIL() <<  e.toString().c_str() << std::endl;
  }

  // Check app log
  PvlGroup points = log.findGroup("Points");
  PvlGroup measures = log.findGroup("Measures");
  PvlGroup valid = log.findGroup("ValidationStatistics");

  EXPECT_EQ(int(points["Total"]), 16);
  EXPECT_EQ(int(measures["Registered"]), 0);
  EXPECT_EQ(int(valid["Total"]), 0);

  QFile falsePos(falsePosPath);  // Should be empty

  EXPECT_TRUE(falsePos.size() == 140);  // 140 is the size of the empty table due to column names
}

TEST_F(ThreeImageNetwork, FunctionalTestPointregValidationSkipped) {
  QTemporaryDir prefix;
  QString flatFilePath = prefix.path() + "/flatfile.csv";
  QString outNetPath = prefix.path() + "/outNet.net";

  QString falsePosPath = prefix.path() + "/falsePos.csv";

  QVector<QString> args = { "fromlist=" + cubeListFile,
                            "cnet=" + networkFile,
                            "deffile=data/threeImageNetwork/autoRegTemplate.def",
                            "flatfile=" + flatFilePath,
                            "onet=" + outNetPath,
                            "validate=only",
                            "falsepositives=" + falsePosPath,
                            "search=7", "shift=0.1", "restolerance=0.0",
                            "points=nonignored" };
  // Test the validation with everything being skipped due restolerance=0.0
  UserInterface options(APP_XML, args);
  Pvl log;

  try {
    pointreg(options, &log);
  }
  catch (IException &e) {
    FAIL() <<  e.toString().c_str() << std::endl;
  }

  // Check app log
  PvlGroup points = log.findGroup("Points");
  PvlGroup measures = log.findGroup("Measures");
  PvlGroup valid = log.findGroup("ValidationStatistics");

  EXPECT_EQ(int(points["Total"]), 16);
  EXPECT_EQ(int(measures["Registered"]), 0);
  EXPECT_EQ(int(valid["Total"]), 0);

  QFile falsePos(falsePosPath);  // Should be empty

  EXPECT_TRUE(falsePos.size() == 140);  // 140 is the size of the empty table due to column names
}

