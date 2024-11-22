#include "ClipperPushBroomCamera.h"
#include "CameraFixtures.h"
#include "TestUtilities.h"

#include <gtest/gtest.h>

using namespace Isis;

TEST_F(ClipperPbCube, ClipperPushBroomCameraNacTest) {
  setInstrument("NAC-PUSHBROOM");

  ClipperPushBroomCamera *cam = (ClipperPushBroomCamera *)testCube->camera();

  EXPECT_EQ(cam->CkFrameId(), -159000);
  EXPECT_EQ(cam->CkReferenceId(), 1);
  EXPECT_EQ(cam->SpkTargetId(), -159);
  EXPECT_EQ(cam->SpkReferenceId(), 1);

  EXPECT_NEAR(cam->FocalLength(), 993.8834414, 0.0001);

  EXPECT_PRED_FORMAT2(AssertQStringsEqual, cam->spacecraftNameLong(), "Europa Clipper");
  EXPECT_PRED_FORMAT2(AssertQStringsEqual, cam->spacecraftNameShort(), "Clipper");
  EXPECT_PRED_FORMAT2(AssertQStringsEqual, cam->instrumentNameLong(), "Europa Imaging System Push Broom Narrow Angle Camera");
  EXPECT_PRED_FORMAT2(AssertQStringsEqual, cam->instrumentNameShort(), "EIS-PBNAC");
  EXPECT_PRED_FORMAT2(AssertQStringsEqual, cam->instrumentId(), "NAC-PUSHBROOM");

  // Upper Left
  EXPECT_TRUE(cam->SetImage(1, 1));
  EXPECT_DOUBLE_EQ(cam->UniversalLatitude(), -11.78444755310951);
  EXPECT_DOUBLE_EQ(cam->UniversalLongitude(), 65.533947871409154);
  EXPECT_TRUE(cam->SetUniversalGround(cam->UniversalLatitude(), cam->UniversalLongitude()));
  EXPECT_NEAR(cam->Sample(), 1, 0.001);
  EXPECT_NEAR(cam->Line(), 1, 0.001);

  // Upper Right
  EXPECT_TRUE(cam->SetImage(1, 1000));
  EXPECT_DOUBLE_EQ(cam->UniversalLatitude(), -11.765071602269636);
  EXPECT_DOUBLE_EQ(cam->UniversalLongitude(), 65.449731697384934);
  EXPECT_TRUE(cam->SetUniversalGround(cam->UniversalLatitude(), cam->UniversalLongitude()));
  EXPECT_NEAR(cam->Sample(), 1, 0.001);
  EXPECT_NEAR(cam->Line(), 1000, 0.001);

  // Lower Right
  EXPECT_TRUE(cam->SetImage(4096, 1000));
  EXPECT_DOUBLE_EQ(cam->UniversalLatitude(), -15.365515626386125);
  EXPECT_DOUBLE_EQ(cam->UniversalLongitude(), 64.055880485639165);
  EXPECT_TRUE(cam->SetUniversalGround(cam->UniversalLatitude(), cam->UniversalLongitude()));
  EXPECT_NEAR(cam->Sample(), 4096, 0.001);
  EXPECT_NEAR(cam->Line(), 1000, 0.001);

  // Lower Left
  EXPECT_TRUE(cam->SetImage(4096, 1));
  EXPECT_DOUBLE_EQ(cam->UniversalLatitude(), -15.402769964247618);
  EXPECT_DOUBLE_EQ(cam->UniversalLongitude(), 64.134350621554276);
  EXPECT_TRUE(cam->SetUniversalGround(cam->UniversalLatitude(), cam->UniversalLongitude()));
  EXPECT_NEAR(cam->Sample(), 4096, 0.001);
  EXPECT_NEAR(cam->Line(), 1, 0.001);
}

TEST_F(ClipperPbCube, ClipperPushBroomCameraWacTest) {
  setInstrument("WAC-PUSHBROOM");

  ClipperPushBroomCamera *cam = (ClipperPushBroomCamera *)testCube->camera();

  EXPECT_EQ(cam->CkFrameId(), -159000);
  EXPECT_EQ(cam->CkReferenceId(), 1);
  EXPECT_EQ(cam->SpkTargetId(), -159);
  EXPECT_EQ(cam->SpkReferenceId(), 1);

  EXPECT_NEAR(cam->FocalLength(), 44.95757712, 0.0001);

  EXPECT_PRED_FORMAT2(AssertQStringsEqual, cam->spacecraftNameLong(), "Europa Clipper");
  EXPECT_PRED_FORMAT2(AssertQStringsEqual, cam->spacecraftNameShort(), "Clipper");
  EXPECT_PRED_FORMAT2(AssertQStringsEqual, cam->instrumentNameLong(), "Europa Imaging System Push Broom Wide Angle Camera");
  EXPECT_PRED_FORMAT2(AssertQStringsEqual, cam->instrumentNameShort(), "EIS-PBWAC");
  EXPECT_PRED_FORMAT2(AssertQStringsEqual, cam->instrumentId(), "WAC-PUSHBROOM");

  // Top
  EXPECT_TRUE(cam->SetImage(2130, 30));
  EXPECT_DOUBLE_EQ(cam->UniversalLatitude(),  -15.171650787132243);
  EXPECT_DOUBLE_EQ(cam->UniversalLongitude(), 64.211656461414719);
  EXPECT_TRUE(cam->SetUniversalGround(cam->UniversalLatitude(), cam->UniversalLongitude()));
  EXPECT_NEAR(cam->Sample(), 2130, 0.001);
  EXPECT_NEAR(cam->Line(), 30, 0.0035);

  // Bottom
  EXPECT_TRUE(cam->SetImage(2130, 2030));
  EXPECT_DOUBLE_EQ(cam->UniversalLatitude(),  -15.098822267352618);
  EXPECT_DOUBLE_EQ(cam->UniversalLongitude(), 64.052574228370474);
  EXPECT_TRUE(cam->SetUniversalGround(cam->UniversalLatitude(), cam->UniversalLongitude()));
  EXPECT_NEAR(cam->Sample(), 2130, 0.001);
  EXPECT_NEAR(cam->Line(), 2030, 0.0022);

  // Right
  EXPECT_TRUE(cam->SetImage(3580, 1024));
  EXPECT_DOUBLE_EQ(cam->UniversalLatitude(), -48.907947401273326);
  EXPECT_DOUBLE_EQ(cam->UniversalLongitude(), 45.124328413917269);
  EXPECT_TRUE(cam->SetUniversalGround(cam->UniversalLatitude(), cam->UniversalLongitude()));
  EXPECT_NEAR(cam->Sample(), 3580, 0.001);
  EXPECT_NEAR(cam->Line(), 1024, 0.037);

  // Left
  EXPECT_TRUE(cam->SetImage(544, 1024));
  EXPECT_DOUBLE_EQ(cam->UniversalLatitude(), 21.737113033236419);
  EXPECT_DOUBLE_EQ(cam->UniversalLongitude(), 78.157998184264912);
  EXPECT_TRUE(cam->SetUniversalGround(cam->UniversalLatitude(), cam->UniversalLongitude()));
  EXPECT_NEAR(cam->Sample(), 544, 0.001);
  EXPECT_NEAR(cam->Line(), 1024, 0.034);
}
