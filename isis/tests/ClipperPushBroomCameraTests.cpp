#include "ClipperPushBroomCamera.h"
#include "CameraFixtures.h"
#include "TestUtilities.h"

#include <gtest/gtest.h>

using namespace Isis;

TEST_F(ClipperPbCube, ClipperPushBroomCameraNacTest) {
  setInstrument("EIS-NAC-PB");

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
  EXPECT_DOUBLE_EQ(cam->UniversalLatitude(), -12.117595283473364);
  EXPECT_DOUBLE_EQ(cam->UniversalLongitude(), 66.463853428869669);
  EXPECT_TRUE(cam->SetUniversalGround(cam->UniversalLatitude(), cam->UniversalLongitude()));
  EXPECT_NEAR(cam->Sample(), 1, 0.001);
  EXPECT_NEAR(cam->Line(), 1, 0.001);

  // Upper Right
  EXPECT_TRUE(cam->SetImage(1, 1000));
  EXPECT_DOUBLE_EQ(cam->UniversalLatitude(), -12.096927266599458);
  EXPECT_DOUBLE_EQ(cam->UniversalLongitude(), 66.374968500075056);
  EXPECT_TRUE(cam->SetUniversalGround(cam->UniversalLatitude(), cam->UniversalLongitude()));
  EXPECT_NEAR(cam->Sample(), 1, 0.001);
  EXPECT_NEAR(cam->Line(), 1000, 0.001);

  // Lower Right
  EXPECT_TRUE(cam->SetImage(4096, 1000));
  EXPECT_DOUBLE_EQ(cam->UniversalLatitude(), -15.707723103010919);
  EXPECT_DOUBLE_EQ(cam->UniversalLongitude(), 64.991443017841291);
  EXPECT_TRUE(cam->SetUniversalGround(cam->UniversalLatitude(), cam->UniversalLongitude()));
  EXPECT_NEAR(cam->Sample(), 4096, 0.001);
  EXPECT_NEAR(cam->Line(), 1000, 0.001);

  // Lower Left
  EXPECT_TRUE(cam->SetImage(4096, 1));
  EXPECT_DOUBLE_EQ(cam->UniversalLatitude(), -15.746353121956448);
  EXPECT_DOUBLE_EQ(cam->UniversalLongitude(), 65.074729620675669);
  EXPECT_TRUE(cam->SetUniversalGround(cam->UniversalLatitude(), cam->UniversalLongitude()));
  EXPECT_NEAR(cam->Sample(), 4096, 0.001);
  EXPECT_NEAR(cam->Line(), 1, 0.001);
}

TEST_F(ClipperPbCube, ClipperPushBroomCameraWacTest) {
  setInstrument("EIS-WAC-PB");

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
  EXPECT_DOUBLE_EQ(cam->UniversalLatitude(),  -22.133032614015832);
  EXPECT_DOUBLE_EQ(cam->UniversalLongitude(), 87.101796154127783);
  EXPECT_TRUE(cam->SetUniversalGround(cam->UniversalLatitude(), cam->UniversalLongitude()));
  EXPECT_NEAR(cam->Sample(), 2130, 0.001);
  EXPECT_NEAR(cam->Line(), 30, 0.0011);

  // Bottom
  EXPECT_TRUE(cam->SetImage(2130, 2030));
  EXPECT_DOUBLE_EQ(cam->UniversalLatitude(),  -22.016871728071468);
  EXPECT_DOUBLE_EQ(cam->UniversalLongitude(), 86.674027874092516);
  EXPECT_TRUE(cam->SetUniversalGround(cam->UniversalLatitude(), cam->UniversalLongitude()));
  EXPECT_NEAR(cam->Sample(), 2130, 0.001);
  EXPECT_NEAR(cam->Line(), 2030, 0.0017);

  // Right
  EXPECT_TRUE(cam->SetImage(3580, 1024));
  EXPECT_DOUBLE_EQ(cam->UniversalLatitude(), -72.765663853451784);
  EXPECT_DOUBLE_EQ(cam->UniversalLongitude(), 108.87646404992245);
  EXPECT_TRUE(cam->SetUniversalGround(cam->UniversalLatitude(), cam->UniversalLongitude()));
  EXPECT_NEAR(cam->Sample(), 3580, 0.001);
  EXPECT_NEAR(cam->Line(), 1024, 0.037);

  // Left
  EXPECT_TRUE(cam->SetImage(544, 1024));
  EXPECT_DOUBLE_EQ(cam->UniversalLatitude(), 20.853469071265028);
  EXPECT_DOUBLE_EQ(cam->UniversalLongitude(), 113.21575238420702);
  EXPECT_TRUE(cam->SetUniversalGround(cam->UniversalLatitude(), cam->UniversalLongitude()));
  EXPECT_NEAR(cam->Sample(), 544, 0.001);
  EXPECT_NEAR(cam->Line(), 1024, 0.034);
}
