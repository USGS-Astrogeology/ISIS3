#include <gtest/gtest.h>
#include "CameraFixtures.h"
#include "Camera.h"
#include "CameraFactory.h"
#include "Cube.h"
#include "ClipperWacFcCamera.h"
#include "IException.h"
#include "iTime.h"
#include "TestUtilities.h"

using namespace Isis;

void TestLineSamp(Camera *cam, double samp, double line) {
  bool success = cam->SetImage(samp, line);
  EXPECT_TRUE(success);

  if(success) {
    success = cam->SetUniversalGround(cam->UniversalLatitude(), cam->UniversalLongitude());
  }
  EXPECT_TRUE(success);

  if(success) {
    EXPECT_NEAR(samp, cam->Sample(), 1.1e-2);
    EXPECT_NEAR(line, cam->Line(), 1.0e-2);
  }
}

void TestImageToGroundToImage(Camera *cam, double samp, double line, double lat, double lon){
  ASSERT_TRUE(cam->SetImage(samp, line));
  EXPECT_DOUBLE_EQ(cam->UniversalLatitude(), lat);
  EXPECT_DOUBLE_EQ(cam->UniversalLongitude(), lon);
  ASSERT_TRUE(cam->SetUniversalGround(cam->UniversalLatitude(), cam->UniversalLongitude()));
  EXPECT_NEAR(cam->Sample(), samp, 0.001);
  EXPECT_NEAR(cam->Line(), line, 0.001);
}


TEST_F(ClipperWacFcCube, ClipperWacFcCameraUnitTest) {
  ClipperWacFcCamera *cam;
  try {
    cam = (ClipperWacFcCamera *) CameraFactory::Create(*wacFcCube);
  } catch(IException &e) {
    FAIL() << "Unable to create ClipperWacFcCamera" << std::endl;
  }

  // Camera info
  //EXPECT_EQ(cam->instrumentRotation()->Frame(), -27002);    // from the defaultcube isd
  EXPECT_EQ(cam->CkFrameId(), -159000);
  EXPECT_EQ(cam->CkReferenceId(), 1);
  EXPECT_EQ(cam->SpkTargetId(), -159);
  EXPECT_EQ(cam->SpkReferenceId(), 1);
  EXPECT_NEAR(cam->FocalLength(), 44.95757712, 1e-4);

  EXPECT_PRED_FORMAT2(AssertQStringsEqual, cam->spacecraftNameLong(), "Europa Clipper");
  EXPECT_PRED_FORMAT2(AssertQStringsEqual, cam->spacecraftNameShort(), "Clipper");
  EXPECT_PRED_FORMAT2(AssertQStringsEqual, cam->instrumentNameLong(), "Europa Imaging System Framing Wide Angle Camera");
  EXPECT_PRED_FORMAT2(AssertQStringsEqual, cam->instrumentNameShort(), "EIS-FWAC");

  // Check SetImage around the planet within the view port

  // Top
  TestLineSamp(cam, 2130, 30);
  // Bottom
  TestLineSamp(cam, 2130, 2030);
  // Right
  TestLineSamp(cam, 3058, 1024);
  // Left
  TestLineSamp(cam, 544, 1024);

  TestImageToGroundToImage(cam, 745, 261, 11.208058331735591, 94.210914418320556);
  TestImageToGroundToImage(cam, 3655, 157, -73.190005076221794, 89.189042140654522);
  TestImageToGroundToImage(cam, 489, 1767, 35.950887637869364, 60.291146636386621);
  TestImageToGroundToImage(cam, 3767, 1579, -50.121882340218569, 10.23020799909251);

  // Simple test for ClipperWacFcCamera::ShutterOpenCloseTimes
  PvlGroup &inst = wacFcCube->label()->findObject("IsisCube").findGroup("Instrument", Pvl::Traverse);
  QString startTime = QString::fromStdString(inst["StartTime"]);
  iTime etStart(startTime);

  std::pair<iTime, iTime> startStop;
  startStop = cam->ShutterOpenCloseTimes(etStart.Et(), 0.00005);  // dummy value for exposure duration
  EXPECT_TRUE(startStop.first.Et() < startStop.second.Et());
}
