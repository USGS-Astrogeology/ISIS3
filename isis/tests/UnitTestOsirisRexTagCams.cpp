#include "OsirisRexTagcamsCamera.h"

#include "iTime.h"
#include "TempFixtures.h"
#include "TestUtilities.h"

#include <gtest/gtest.h>

using namespace Isis;
using namespace std;

/**
   * Osiris-REx TagCams NAVCam unit test.
   * 
   * Tests ...
   *   - back and forth between sample/line and Universal Lat/Lon
   * 
   */
TEST_F(TempTestingFiles, UnitTestOsirisRexTagCamsNAVCam) {

  QString cubeFileName = "data/osirisRexImages/20200303T213031S138_ncm_L0-reduced.cub";
  Cube navCube(cubeFileName);
  
  OsirisRexTagcamsCamera *cam = (OsirisRexTagcamsCamera *)navCube.camera();

  // Test kernel IDs
  EXPECT_EQ(cam->CkFrameId(), -64000);
  EXPECT_EQ(cam->CkReferenceId(), 1);
  EXPECT_EQ(cam->SpkTargetId(), -64);
  EXPECT_EQ(cam->SpkReferenceId(), 1);

  // Test names and instrument id
  EXPECT_PRED_FORMAT2(AssertQStringsEqual, cam->spacecraftNameLong(), "OSIRIS-REx");
  EXPECT_PRED_FORMAT2(AssertQStringsEqual, cam->spacecraftNameShort(), "OSIRIS-REx");
  EXPECT_PRED_FORMAT2(AssertQStringsEqual, cam->instrumentNameLong(), "Primary Optical Navigation (NCM) Camera");
  EXPECT_PRED_FORMAT2(AssertQStringsEqual, cam->instrumentNameShort(), "NAVCam");
  EXPECT_PRED_FORMAT2(AssertQStringsEqual, cam->instrumentId(), "NAVCam");

  // Test Shutter Open/Close
  const PvlGroup &inst = navCube.label()->findGroup("Instrument", Pvl::Traverse);
  double exposureDuration = ((double) inst["ExposureDuration"])/1000;
  QString stime = QString::fromStdString(inst["StartTime"]);
  double et; // StartTime keyword is the center exposure time
  str2et_c(stime.toLatin1().data(), &et);
  pair <iTime, iTime> shuttertimes = cam->ShutterOpenCloseTimes(et, exposureDuration);
  EXPECT_NEAR(shuttertimes.first.Et(), 636543100.32342994, 6E-14);
  EXPECT_NEAR(shuttertimes.second.Et(), 636543100.32343423, 6E-14);

  // test at center of format
  EXPECT_TRUE(cam->SetImage(129.5, 97.0));
  EXPECT_NEAR(cam->UniversalLatitude(), 49.7487786981275, 0.0001);
  EXPECT_NEAR(cam->UniversalLongitude(), 43.7549667753273, 0.0001);

  EXPECT_TRUE(cam->SetUniversalGround(cam->UniversalLatitude(), cam->UniversalLongitude()));
  EXPECT_NEAR(cam->Sample(), 129.5, 0.01);
  EXPECT_NEAR(cam->Line(), 97.0, 0.01);

  // test at upper left corner
  EXPECT_TRUE(cam->SetImage(1.0, 1.0));
  EXPECT_NEAR(cam->UniversalLatitude(), 18.614472228664749, 0.0001);
  EXPECT_NEAR(cam->UniversalLongitude(), 30.4388285538537, 0.0001);

  EXPECT_TRUE(cam->SetUniversalGround(cam->UniversalLatitude(), cam->UniversalLongitude()));
  EXPECT_NEAR(cam->Sample(), 1.0, 0.01);
  EXPECT_NEAR(cam->Line(), 1.0, 0.01);

  // test at upper right corner
  EXPECT_TRUE(cam->SetImage(259.0, 1.0));
  EXPECT_NEAR(cam->UniversalLatitude(), 36.692323846663946, 0.0001);
  EXPECT_NEAR(cam->UniversalLongitude(), 81.774178147101267, 0.0001);

  EXPECT_TRUE(cam->SetUniversalGround(cam->UniversalLatitude(), cam->UniversalLongitude()));
  EXPECT_NEAR(cam->Sample(), 259.0, 0.01);
  EXPECT_NEAR(cam->Line(), 1.0, 0.01);

  // test at lower left corner
  EXPECT_TRUE(cam->SetImage(1.0, 194.0));
  EXPECT_NEAR(cam->UniversalLatitude(), 44.70914449416866, 0.0001);
  EXPECT_NEAR(cam->UniversalLongitude(), 1.6058653718226457, 0.0001);

  EXPECT_TRUE(cam->SetUniversalGround(cam->UniversalLatitude(), cam->UniversalLongitude()));
  EXPECT_NEAR(cam->Sample(), 1.0, 0.01);
  EXPECT_NEAR(cam->Line(), 194.0, 0.01);

    // test at lower right corner
  EXPECT_TRUE(cam->SetImage(259.0, 194.0));
  EXPECT_NEAR(cam->UniversalLatitude(), 71.210706457717208, 0.0001);
  EXPECT_NEAR(cam->UniversalLongitude(), 82.189907756214126, 0.0001);

  EXPECT_TRUE(cam->SetUniversalGround(cam->UniversalLatitude(), cam->UniversalLongitude()));
  EXPECT_NEAR(cam->Sample(), 259.0, 0.01);
  EXPECT_NEAR(cam->Line(), 194.0, 0.01);
}


/**
   * Osiris-REx TagCams NAVCam unit test.
   * 
   * Tests ...
   *   - back and forth between sample/line and Universal Lat/Lon
   * 
   */
TEST_F(TempTestingFiles, UnitTestOsirisRexTagCamsNFTCam) {

  QString cubeFileName = "data/osirisRexImages/20201020T214241S004_nft_L0-reduced.cub";
  Cube cube(cubeFileName);
  
  OsirisRexTagcamsCamera *cam = (OsirisRexTagcamsCamera *)cube.camera();

  // Test kernel IDs
  EXPECT_EQ(cam->CkFrameId(), -64000);
  EXPECT_EQ(cam->CkReferenceId(), 1);
  EXPECT_EQ(cam->SpkTargetId(), -64);
  EXPECT_EQ(cam->SpkReferenceId(), 1);
  
  // Test names and instrument id
  EXPECT_PRED_FORMAT2(AssertQStringsEqual, cam->spacecraftNameLong(), "OSIRIS-REx");
  EXPECT_PRED_FORMAT2(AssertQStringsEqual, cam->spacecraftNameShort(), "OSIRIS-REx");
  EXPECT_PRED_FORMAT2(AssertQStringsEqual, cam->instrumentNameLong(), "Natural Feature Tracking (NFT) Camera");
  EXPECT_PRED_FORMAT2(AssertQStringsEqual, cam->instrumentNameShort(), "NFTCam");
  EXPECT_PRED_FORMAT2(AssertQStringsEqual, cam->instrumentId(), "NFTCam");

  // checking at center of format of NftCam
  EXPECT_TRUE(cam->SetImage(129.5, 97.0));
  EXPECT_NEAR(cam->UniversalLatitude(), 53.7314045659365, 0.0001);
  EXPECT_NEAR(cam->UniversalLongitude(), 45.4736806050086, 0.0001);

  EXPECT_TRUE(cam->SetUniversalGround(cam->UniversalLatitude(), cam->UniversalLongitude()));
  EXPECT_NEAR(cam->Sample(), 129.5, 0.01);
  EXPECT_NEAR(cam->Line(), 97.0, 0.01);

    // test at upper left corner
  EXPECT_TRUE(cam->SetImage(1.0, 1.0));
  EXPECT_NEAR(cam->UniversalLatitude(), 53.74996008837919, 0.0001);
  EXPECT_NEAR(cam->UniversalLongitude(), 44.87991851142592, 0.0001);

  EXPECT_TRUE(cam->SetUniversalGround(cam->UniversalLatitude(), cam->UniversalLongitude()));
  EXPECT_NEAR(cam->Sample(), 1.0, 0.01);
  EXPECT_NEAR(cam->Line(), 1.0, 0.01);

  // test at upper right corner
  EXPECT_TRUE(cam->SetImage(259.0, 1.0));
  EXPECT_NEAR(cam->UniversalLatitude(), 53.45070519515528, 0.0001);
  EXPECT_NEAR(cam->UniversalLongitude(), 45.595734037697831, 0.0001);

  EXPECT_TRUE(cam->SetUniversalGround(cam->UniversalLatitude(), cam->UniversalLongitude()));
  EXPECT_NEAR(cam->Sample(), 259.0, 0.01);
  EXPECT_NEAR(cam->Line(), 1.0, 0.01);

  // test at lower left corner
  EXPECT_TRUE(cam->SetImage(1.0, 194.0));
  EXPECT_NEAR(cam->UniversalLatitude(), 54.251546951663194, 0.0001);
  EXPECT_NEAR(cam->UniversalLongitude(), 45.249110941406045, 0.0001);

  EXPECT_TRUE(cam->SetUniversalGround(cam->UniversalLatitude(), cam->UniversalLongitude()));
  EXPECT_NEAR(cam->Sample(), 1.0, 0.01);
  EXPECT_NEAR(cam->Line(), 194.0, 0.01);

    // test at lower right corner
  EXPECT_TRUE(cam->SetImage(259.0, 194.0));
  EXPECT_NEAR(cam->UniversalLatitude(), 53.714109526681277, 0.0001);
  EXPECT_NEAR(cam->UniversalLongitude(), 46.006871961761462, 0.0001);

  EXPECT_TRUE(cam->SetUniversalGround(cam->UniversalLatitude(), cam->UniversalLongitude()));
  EXPECT_NEAR(cam->Sample(), 259.0, 0.01);
  EXPECT_NEAR(cam->Line(), 194.0, 0.01);
}


/**
   * Osiris-REx TagCams StowCam unit test.
   * 
   * TODO: COMPLETE IF/WHEN NAIF HAS PROVIDED KERNELS FOR STOWCAM.
   *       CURRENT IK IS LABELED AS PLACEHOLDER ONLY.
   * 
   * Tests ...
   *   - back and forth between sample/line and Universal Lat/Lon
   * 
   */