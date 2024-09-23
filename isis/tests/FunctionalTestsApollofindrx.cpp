#include "CubeFixtures.h"
#include "Pvl.h"
#include "PvlGroup.h"
#include "TestUtilities.h"

#include "apollofindrx.h"

#include "gtest/gtest.h"

using namespace Isis;

static QString APP_XML = QString::fromStdString(FileName("$ISISROOT/bin/xml/apollofindrx.xml").expanded());

TEST_F(LargeCube, FunctionalTestApollofindrxDefault) {

  PvlGroup reseaus("Reseaus");
  PvlKeyword samples = PvlKeyword("Sample", "200");
  samples += "400";
  samples += "600";

  PvlKeyword lines = PvlKeyword("Line", "200");
  lines += "400";
  lines += "600";

  PvlKeyword types = PvlKeyword("Type", "5");
  types += "5";
  types += "5";

  PvlKeyword valid = PvlKeyword("Valid", "1");
  valid += "1";
  valid += "1";

  reseaus += lines;
  reseaus += samples;
  reseaus += types;
  reseaus += valid;
  reseaus += PvlKeyword("Status", "Nominal");

  std::istringstream instStr (R"(
    Group = Instrument
        SpacecraftName = "APOLLO 15"
        InstrumentId   = METRIC
        TargetName     = MOON
        StartTime      = 1971-08-01T14:58:03.78
    End_Group
  )");

  PvlGroup instGroup;
  instStr >> instGroup;

  Pvl *lab = testCube->label();
  lab->findObject("IsisCube").addGroup(reseaus);
  lab->findObject("IsisCube").addGroup(instGroup);

  testCube->reopen("r");

  QTemporaryDir prefix;
  QVector<QString> args = {"tolerance=0.5", "patternsize=201"};

  UserInterface options(APP_XML, args);
  try {
    apollofindrx(testCube, options);
  }
  catch (IException &e) {
    FAIL() << "Call failed, Unable to process cube: " << e.what() << std::endl;
  }

  Pvl newLab = *testCube->label();

  PvlGroup newReseaus = newLab.findObject("IsisCube").findGroup("Reseaus");
  PvlKeyword testKeyword = newReseaus.findKeyword("Line");
  EXPECT_NEAR(Isis::toDouble(testKeyword[0]), 100.8141, 0.0001);
  EXPECT_NEAR(Isis::toDouble(testKeyword[1]), 192.8, 0.0001);
  EXPECT_NEAR(Isis::toDouble(testKeyword[2]), 275.8, 0.0001);

  testKeyword = newReseaus.findKeyword("Sample");
  EXPECT_NEAR(Isis::toDouble(testKeyword[0]), 100.8141, 0.0001);
  EXPECT_NEAR(Isis::toDouble(testKeyword[1]), 192.8, 0.0001);
  EXPECT_NEAR(Isis::toDouble(testKeyword[2]), 167.8, 0.0001);

  testKeyword = newReseaus.findKeyword("Valid");
  EXPECT_EQ(Isis::toInt(testKeyword[0]), 1);
  EXPECT_EQ(Isis::toInt(testKeyword[1]), 1);
  EXPECT_EQ(Isis::toInt(testKeyword[2]), 1);

  EXPECT_PRED_FORMAT2(AssertStringsEqual, newReseaus.findKeyword("Status"), "Refined");

  // Assert some stuff
}
