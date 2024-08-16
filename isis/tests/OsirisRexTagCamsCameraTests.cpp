#include "OsirisRexTagcamsCamera.h"
#include "IException.h"
#include "TestUtilities.h"
#include "iTime.h"

#include "CameraFixtures.h"
#include <gtest/gtest.h>

using namespace Isis;
using namespace std;

/**
   * Osiris-REx TagCams NAVCam test.
   * 
   * Tests ...
   *   1) instrument rotation frame
   *   2) kernel IDs
   *   3) spacecraft names; instrument name and id
   *   4) exposure duration, start time, and shutter times
   *   5) back and forth between sample/line and Universal Lat/Lon
   * 
   */
TEST_F(OsirisRexTagcamsNAVCamCube, NavigationCam) {
  setInstrument("-64081", "NAVCam");

  OsirisRexTagcamsCamera *cam = (OsirisRexTagcamsCamera *)testCube->camera();

  EXPECT_EQ(cam->instrumentRotation()->Frame(), -64081);

  // Test kernel IDs
  EXPECT_EQ(cam->CkFrameId(), -64000);
  EXPECT_EQ(cam->CkReferenceId(), 1);
  EXPECT_EQ(cam->SpkTargetId(), -64);
  EXPECT_EQ(cam->SpkReferenceId(), 1);

  EXPECT_PRED_FORMAT2(AssertQStringsEqual, cam->spacecraftNameLong(), "OSIRIS-REx");
  EXPECT_PRED_FORMAT2(AssertQStringsEqual, cam->spacecraftNameShort(), "OSIRIS-REx");
  EXPECT_PRED_FORMAT2(AssertQStringsEqual, cam->instrumentNameLong(), "Primary Optical Navigation (NCM) Camera");
  EXPECT_PRED_FORMAT2(AssertQStringsEqual, cam->instrumentNameShort(), "NAVCam");
  EXPECT_PRED_FORMAT2(AssertQStringsEqual, cam->instrumentId(), "NAVCam");

  const PvlGroup &inst = testCube->label()->findGroup("Instrument", Pvl::Traverse);
  double exposureDuration = ((double) inst["ExposureDuration"])/1000;
  QString stime = QString::fromStdString(inst["StartTime"]);
  double et; // StartTime keyword is the center exposure time
  str2et_c(stime.toLatin1().data(), &et);
  pair <iTime, iTime> shuttertimes = cam->ShutterOpenCloseTimes(et, exposureDuration);
  EXPECT_NEAR(shuttertimes.first.Et(), 636543100.32342994, 6E-14);
  EXPECT_NEAR(shuttertimes.second.Et(), 636543100.32343423, 6E-14);

  EXPECT_TRUE(cam->SetImage(5, 5));
  EXPECT_NEAR(cam->UniversalLatitude(), 18.576402476976771, 0.0001);
  EXPECT_NEAR(cam->UniversalLongitude(), 30.430429999416273, 0.0001);

  EXPECT_TRUE(cam->SetUniversalGround(cam->UniversalLatitude(), cam->UniversalLongitude()));
  EXPECT_NEAR(cam->Line(), 5, 0.01);
  EXPECT_NEAR(cam->Sample(), 5, 0.01);
}


/**
   * Osiris-REx TagCams NFTCam test.
   * 
   * Tests ...
   *   1) instrument rotation frame
   *   2) kernel IDs
   *   3) spacecraft names; instrument name and id
   *   4) exposure duration, start time, and shutter times
   *   5) back and forth between sample/line and Universal Lat/Lon
   * 
   */
TEST_F(OsirisRexTagcamsNFTCamCube, NaturalFeatureTrackingCam) {
  setInstrument("-64082", "NftCam");

  OsirisRexTagcamsCamera *cam = (OsirisRexTagcamsCamera *)testCube->camera();

  EXPECT_EQ(cam->instrumentRotation()->Frame(), -64082);

  // Test kernel IDs
  EXPECT_EQ(cam->CkFrameId(), -64000);
  EXPECT_EQ(cam->CkReferenceId(), 1);
  EXPECT_EQ(cam->SpkTargetId(), -64);
  EXPECT_EQ(cam->SpkReferenceId(), 1);

  EXPECT_PRED_FORMAT2(AssertQStringsEqual, cam->spacecraftNameLong(), "OSIRIS-REx");
  EXPECT_PRED_FORMAT2(AssertQStringsEqual, cam->spacecraftNameShort(), "OSIRIS-REx");
  EXPECT_PRED_FORMAT2(AssertQStringsEqual, cam->instrumentNameLong(), "Natural Feature Tracking (NFT) Camera");
  EXPECT_PRED_FORMAT2(AssertQStringsEqual, cam->instrumentNameShort(), "NFTCam");
  EXPECT_PRED_FORMAT2(AssertQStringsEqual, cam->instrumentId(), "NFTCam");

  const PvlGroup &inst = testCube->label()->findGroup("Instrument", Pvl::Traverse);
  double exposureDuration = ((double) inst["ExposureDuration"])/1000;
  QString stime = QString::fromStdString(inst["StartTime"]);
  double et; // StartTime keyword is the center exposure time
  str2et_c(stime.toLatin1().data(), &et);
  pair <iTime, iTime> shuttertimes = cam->ShutterOpenCloseTimes(et, exposureDuration);
  EXPECT_NEAR(shuttertimes.first.Et(), 656502230.15640402, 6E-14);
  EXPECT_NEAR(shuttertimes.second.Et(), 656502230.15640986, 6E-14);

  EXPECT_TRUE(cam->SetImage(5, 5));
  EXPECT_NEAR(cam->UniversalLatitude(), 53.749944508818018, 0.0001);
  EXPECT_NEAR(cam->UniversalLongitude(), 44.879563021627902, 0.0001);

  EXPECT_TRUE(cam->SetUniversalGround(cam->UniversalLatitude(), cam->UniversalLongitude()));
  EXPECT_NEAR(cam->Line(), 5, 0.01);
  EXPECT_NEAR(cam->Sample(), 5, 0.01);
}