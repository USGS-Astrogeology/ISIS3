#include <QFile>
#include <QString>
#include <QTemporaryDir>

#include "cnetedit.h"
#include "cnetdiff.h"
#include "cnetthinner.h"
#include "ControlNet.h"
#include "TempFixtures.h"

#include "gtest/gtest.h"

using namespace Isis;

static QString APP_XML = QString::fromStdString(FileName("$ISISROOT/bin/xml/cnetthinner.xml").expanded());
static QString APP1_XML = QString::fromStdString(FileName("$ISISROOT/bin/xml/cnetdiff.xml").expanded());
static QString APP2_XML = QString::fromStdString(FileName("$ISISROOT/bin/xml/cnetedit.xml").expanded());

class Cnetthinner : public TempTestingFiles {
  protected:
    QString cnetFile;

    void SetUp() override {
      TempTestingFiles::SetUp();

      cnetFile = "data/cnetthinner/customPointsTruth.pvl";
    }
};


/**
   * CnetthinnerEmptyInputControlNet
   * 
   * Cnetthinner test given a empty input ControlNet.
   */
TEST_F(Cnetthinner, FunctionalTestCnetthinnerEmptyControlNet) {

  // create empty control net
  ControlNet emptyNet;
  
  QVector<QString> args = {"cnet=emptyNet",
                           "onet=" + tempDir.path() + "/failNet",
                           "maxpoints=20",
                           };

  UserInterface options(APP_XML, args);

  try {
    cnetthinner(options);
    FAIL() << "Expected Exception for invalid, empty control network file";
  }
  catch(IException &e) {
    EXPECT_TRUE(e.toString().find("Invalid control network") != std::string::npos)
      <<  e.toString();
  }
}


/**
   * CnetthinnerDefault
   * 
   * Input ...
   *   1) ControlNet with 481 points  (data/cnetthinner/customPointsTruth.pvl)
   *   2) maxpoints=20000
   *   3) suppressed=ignore (suppressed points are to be ignored in output net)
   *   4) networkid=testID
   * 
   * Output ...
   *    1) thinned ControlNet (10 points should have been ignored)
   *    2) Pvl log file.
   */
TEST_F(Cnetthinner, FunctionalTestCnetthinnerDefault) {
  
  QVector<QString> args = {"cnet=" + cnetFile,
                           "onet=" + tempDir.path() + "/out.net",
                           "maxpoints=20000",
                           "suppressed=ignore",
                           "networkid=testID"
                           };

  UserInterface ui(APP_XML, args);

  try {
    cnetthinner(ui);
  }
  catch(IException &e) {
    FAIL() <<  e.toString().c_str() << std::endl;
  }

  // check output ControlNetwork
  ControlNet outNet(tempDir.path() + "/out.net");
  EXPECT_EQ(outNet.GetNetworkId().toStdString(), "testID");
  EXPECT_EQ(outNet.GetNumPoints(), 481);
  EXPECT_EQ(outNet.GetNumValidPoints(), 471); // 481 - 471 = 10 ignored points
  EXPECT_EQ(outNet.GetNumEditLockPoints(), 0);
  EXPECT_EQ(outNet.GetNumMeasures(), 962);
  EXPECT_EQ(outNet.GetNumIgnoredMeasures(), 0);
  EXPECT_EQ(outNet.GetNumEditLockMeasures(), 0);

  // confirm id's of 10 ignored points in output ControlNetwork
  EXPECT_TRUE(outNet.GetPoint("ff_test_339")->IsIgnored());
  EXPECT_TRUE(outNet.GetPoint("ff_test_363")->IsIgnored());
  EXPECT_TRUE(outNet.GetPoint("ff_test_387")->IsIgnored());
  EXPECT_TRUE(outNet.GetPoint("ff_test_398")->IsIgnored());
  EXPECT_TRUE(outNet.GetPoint("ff_test_413")->IsIgnored());
  EXPECT_TRUE(outNet.GetPoint("ff_test_466")->IsIgnored());
  EXPECT_TRUE(outNet.GetPoint("ff_test_606")->IsIgnored());
  EXPECT_TRUE(outNet.GetPoint("ff_test_717")->IsIgnored());
  EXPECT_TRUE(outNet.GetPoint("ff_test_727")->IsIgnored());
  EXPECT_TRUE(outNet.GetPoint("ff_test_754")->IsIgnored());
}


/**
   * CnetthinnerIgnoreOrRemove
   * 
   * 1) Cnetthinner is executed twice with max points of 200. In the
   *    first output ControlNet (ignored.net) the suppressed points
   *    are ignored. In the second (removed.net) the suppressed points
   *    are removed.
   * 
   * Input run 1...
   *   1) ControlNet with 481 points  (data/cnetthinner/customPointsTruth.pvl)
   *   2) maxpoints=200
   *   3) suppressed=ignore (suppressed points are to be ignored in output net)

   * Input run 2...
   *   1) ControlNet with 481 points  (data/cnetthinner/customPointsTruth.pvl)
   *   2) maxpoints=200
   *   3) suppressed=remove (suppressed points are to be removed in output net)
   *
   * 2) Cnetedit is used to delete the ignored points in ignored.net.
   *    The result is in ignored_removed.net.
   * 
   * 3) Cnetdiff is then used to compare ignored_removed.net and
   *    removed.net. Result in cnetdiff.txt. They should be identical.
   */
TEST_F(Cnetthinner, FunctionalTestCnetthinnerIgnoreOrRemove) {
  
  // Use cnetthinner to suppress points by ignoring in customPointsTruth.net.
  // Result in ignored.net.
  QVector<QString> args = {"cnet=" + cnetFile,
                           "onet=" + tempDir.path() + "/ignored.net",
                           "maxpoints=200",
                           "suppressed=ignore"
                           };
  
  UserInterface ui1(APP_XML, args);

  try {
    cnetthinner(ui1);
  }
  catch(IException &e) {
    FAIL() << "cnetthinner ignore failure: " <<  e.toString().c_str() << std::endl;
  }
  
  // Use cnetthinner to suppress points by removing in customPointsTruth.net.
  // Result in removed.net.
  args = {"cnet=" + cnetFile,
          "onet=" + tempDir.path() + "/removed.net",
          "maxpoints=200",
          "suppressed=remove"
          };
  UserInterface ui2(APP_XML, args);

  try {
    cnetthinner(ui2);
  }
  catch(IException &e) {
    FAIL() << "cnetthinner remove failure: " <<  e.toString().c_str() << std::endl;
  }

  // confirm 64 points are ignored in both runs
  ControlNet ignored(tempDir.path()+ "/ignored.net");
  EXPECT_EQ(ignored.GetNumPoints(), 481);
  EXPECT_EQ(ignored.GetNumValidPoints(), 417);

  ControlNet removed(tempDir.path()+ "/removed.net");
  EXPECT_EQ(removed.GetNumPoints(), 417);
  EXPECT_EQ(removed.GetNumValidPoints(), 417);

  // use cnetedit to delete ignored points in removed.net. Result in ignored_removed.net.
  args = {"cnet=" + tempDir.path() + "/ignored.net",
          "onet=" + tempDir.path() + "/ignored_removed.net",
          "ignore=no",
          "delete=yes"
          };

  UserInterface ui3(APP2_XML, args);

  try {
    cnetedit(ui3);
  }
  catch(IException &e) {
    FAIL() << "cnetedit failure to delete removed points: " <<  e.toString().c_str() << std::endl;
  }

   // Use cnetdiff to compare ignored_removed.net and removed.net.
   // Result in cnetdiff.txt. They should be identical.
  args = {"from=" + tempDir.path() + "/ignored_removed.net",
          "from2=" + tempDir.path() + "/removed.net",
          "to=" + tempDir.path() + "/compareIgnored_IgnoredRemoved.txt",
          "report=full"
          };

  UserInterface ui4(APP1_XML, args);

  try {
    cnetdiff(ui4);
  }
  catch(IException &e) {
    FAIL() << "cnetdiff failure to compare networks: " <<  e.toString().c_str() << std::endl;
  }

  // read back compareIgnored_IgnoredRemoved log file
  Pvl compareIgnored_IgnoredRemoved;
  try {
    compareIgnored_IgnoredRemoved.read(tempDir.path().toStdString() + "/compareIgnored_IgnoredRemoved.txt");
  }
  catch (IException &e) {
    FAIL() << "Unable to open error log pvl file: " << e.what() << std::endl;
  }

  // confirm output filenames and that there are no groups or objects
  // in the difference object confirming cnets are identical 
  ASSERT_TRUE(compareIgnored_IgnoredRemoved.hasObject("Differences"));
  PvlObject differences = compareIgnored_IgnoredRemoved.findObject("Differences");
  EXPECT_EQ(differences["Filename"][0], "ignored_removed.net");
  EXPECT_EQ(differences["Filename"][1], "removed.net");  
  EXPECT_EQ(differences.groups(), 0);
  EXPECT_EQ(differences.objects(), 0);
}


/**
   * CnetthinnerMinMaxPoints
   * 
   * Runs cnetthinner three times with different maxpoints (50, 200,
   * 400) and verifies the number of points in the output ControlNetworks.
   * 
   * Input...
   *   1) ControlNet with 481 points  (data/cnetthinner/customPointsTruth.pvl)
   *   2) maxpoints=50 (200, 400)
   *   3) suppressed=remove (default)
   *                                  expected
   * maxpoints = 50  =>   50.net with 101 points
   *           = 200 =>  200.net with 417 points
   *           = 400 =>  400.net with 471 points
   */
TEST_F(Cnetthinner, FunctionalTestCnetthinnerMinMaxPoints) {
  
  QVector<QString> args = {"cnet=" + cnetFile,
                           "onet=" + tempDir.path() + "/50.net",
                           "maxpoints=50",
                           };
  UserInterface ui1(APP_XML, args);

  try {
    cnetthinner(ui1);
  }
  catch(IException &e) {
    FAIL() <<  e.toString().c_str() << std::endl;
  }

  ControlNet outNet50(tempDir.path()+ "/50.net");
  EXPECT_EQ(outNet50.GetNumPoints(), 101);

  args = {"cnet=" + cnetFile,
          "onet=" + tempDir.path() + "/200.net",
          "maxpoints=200",
          };
  UserInterface ui2(APP_XML, args);

  try {
    cnetthinner(ui2);
  }
  catch(IException &e) {
    FAIL() <<  e.toString().c_str() << std::endl;
  }
	
  ControlNet outNet200(tempDir.path()+ "/200.net");
  EXPECT_EQ(outNet200.GetNumPoints(), 417);

  args = {"cnet=" + cnetFile,
          "onet=" + tempDir.path() + "/400.net",
          "maxpoints=400",
         };
  UserInterface ui3(APP_XML, args);

  try {
    cnetthinner(ui3);
  }
  catch(IException &e) {
    FAIL() <<  e.toString().c_str() << std::endl;
  }

  ControlNet outNet400(tempDir.path()+"/400.net");
  EXPECT_EQ(outNet400.GetNumPoints(), 471);
}


/**
   * CnetthinnerTolerance
   * 
   * Runs cnetthinner three times with maxpoints = 200 and different
   * tolerances (0.0, 0.5, 1.0) and verifies number of points in
   * output ControlNetworks.
   * 
   * Input...
   *   1) ControlNet with 481 points  (data/cnetthinner/customPointsTruth.pvl)
   *   2) maxpoints=200
   *   3) tolerance=0.0 (0.5, 1.0)
   *   4) suppressed=remove (default)
   * 
   *                                       expected
   * tolerance = 0.0 =>  tolSmall.net with 427 points
   * tolerance = 0.5 => tolMedium.net with 418 points
   * tolerance = 1.0 =>  tolLarge.net with   1 point
   */
TEST_F(Cnetthinner, FunctionalTestCnetthinnerTolerance) {
  
  QVector<QString> args = {"cnet=" + cnetFile,
          "onet=" + tempDir.path() + "/tolSmall.net",
          "maxpoints=200",
          "tolerance=0.0"
          };
  UserInterface ui1(APP_XML, args);

  try {
    cnetthinner(ui1);
  }
  catch(IException &e) {
    FAIL() <<  e.toString().c_str() << std::endl;
  }

  ControlNet smallTol(tempDir.path()+"/tolSmall.net");
  EXPECT_EQ(smallTol.GetNumPoints(), 427);

  args = {"cnet=" + cnetFile,
          "onet=" + tempDir.path() + "/tolMedium.net",
          "maxpoints=200",
          "tolerance=0.5"
          };
  UserInterface ui2(APP_XML, args);

  try {
    cnetthinner(ui2);
  }
  catch(IException &e) {
    FAIL() <<  e.toString().c_str() << std::endl;
  }
	
  ControlNet mediumTol(tempDir.path()+"/tolMedium.net");
  EXPECT_EQ(mediumTol.GetNumPoints(), 418);

  args = {"cnet=" + cnetFile,
          "onet=" + tempDir.path() + "/tolLarge.net",
          "maxpoints=200",
          "tolerance=1.0"
         };
  UserInterface ui3(APP_XML, args);

  try {
    cnetthinner(ui3);
  }
  catch(IException &e) {
    FAIL() <<  e.toString().c_str() << std::endl;
  }

  ControlNet largeTol(tempDir.path()+"/tolLarge.net");
  EXPECT_EQ(largeTol.GetNumPoints(), 1);
}


/**
   * CnetthinnerWeight
   * 
   * Runs cnetthinner four times with maxpoints=200 and different weights
   * (0.0, 0.5, 1.0, 10,000.0) and verifies number of points in output
   * control networks. Note all output nets contain 417 points.
   *
   * Input...
   *   1) ControlNet with 481 points  (data/cnetthinner/customPointsTruth.pvl)
   *   2) maxpoints=200
   *   3) weight=0.0 (0.5, 1.0, 10,000.0)
   *   4) suppressed=remove (default)
   *                                      expected
   * weight =      0.0 =>  Small.net with 417 points
   * weight =      0.5 => Medium.net with 417 points
   * weight =      1.0 =>  Large.net with 417 points
   * weight = 10,000.0 => XLarge.net with 417 points
   */
TEST_F(Cnetthinner, FunctionalTestCnetthinnerWeight) {
  
  QVector<QString> args = {"cnet=" + cnetFile,
          "onet=" + tempDir.path() + "/Small.net",
          "maxpoints=200",
          "weight=0.0"
          };
  UserInterface ui1(APP_XML, args);

  try {
    cnetthinner(ui1);
  }
  catch(IException &e) {
    FAIL() <<  e.toString().c_str() << std::endl;
  }

  ControlNet smallWeight(tempDir.path()+"/Small.net");
  EXPECT_EQ(smallWeight.GetNumPoints(), 417);

  args = {"cnet=" + cnetFile,
          "onet=" + tempDir.path() + "/Medium.net",
          "maxpoints=200",
          "weight=0.5"
          };
  UserInterface ui2(APP_XML, args);

  try {
    cnetthinner(ui2);
  }
  catch(IException &e) {
    FAIL() <<  e.toString().c_str() << std::endl;
  }
	
  ControlNet mediumWeight(tempDir.path()+"/Medium.net");
  EXPECT_EQ(mediumWeight.GetNumPoints(), 417);

  args = {"cnet=" + cnetFile,
          "onet=" + tempDir.path() + "/Large.net",
          "maxpoints=200",
          "weight=1.0"
         };
  UserInterface ui3(APP_XML, args);

  try {
    cnetthinner(ui3);
  }
  catch(IException &e) {
    FAIL() <<  e.toString().c_str() << std::endl;
  }

  ControlNet largeWeight(tempDir.path() + "/Large.net");
  EXPECT_EQ(largeWeight.GetNumPoints(), 417);

  args = {"cnet=" + cnetFile,
          "onet=" + tempDir.path() + "/XLarge.net",
          "maxpoints=200",
          "weight=10000.0"
         };
  UserInterface ui4(APP_XML, args);

  try {
    cnetthinner(ui4);
  }
  catch(IException &e) {
    FAIL() <<  e.toString().c_str() << std::endl;
  }

  ControlNet xLargeWeight(tempDir.path() + "/XLarge.net");
  EXPECT_EQ(xLargeWeight.GetNumPoints(), 417);
}
