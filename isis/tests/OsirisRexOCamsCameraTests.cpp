#include "OsirisRexOcamsCamera.h"
#include "IException.h"
#include "TestUtilities.h"
#include "iTime.h"

#include "CameraFixtures.h"
#include <gtest/gtest.h>

using namespace Isis;
using namespace std;

TEST_F(OsirisRexOcamsCube, PolyMath) {
  setInstrument("-64360", "PolyCam");

  OsirisRexOcamsCamera *cam = (OsirisRexOcamsCamera *)testCube->camera();

  EXPECT_EQ(cam->instrumentRotation()->Frame(), -27002);

  // Test kernel IDs
  EXPECT_EQ(cam->CkFrameId(), -64000);
  EXPECT_EQ(cam->CkReferenceId(), 1);
  EXPECT_EQ(cam->SpkTargetId(), -64);
  EXPECT_EQ(cam->SpkReferenceId(), 1);

  // Test name methods
  EXPECT_PRED_FORMAT2(AssertQStringsEqual, cam->spacecraftNameLong(), "OSIRIS-REx");
  EXPECT_PRED_FORMAT2(AssertQStringsEqual, cam->spacecraftNameShort(), "OSIRIS-REx");
  EXPECT_PRED_FORMAT2(AssertQStringsEqual, cam->instrumentNameLong(), "PolyMath Camera");
  EXPECT_PRED_FORMAT2(AssertQStringsEqual, cam->instrumentNameShort(), "PolyCam");
  EXPECT_PRED_FORMAT2(AssertQStringsEqual, cam->instrumentId(), "PolyCam");

  const PvlGroup &inst = testCube->label()->findGroup("Instrument", Pvl::Traverse);
  double exposureDuration = ((double) inst["ExposureDuration"])/1000;
  QString stime = QString::fromStdString(inst["StartTime"]);
  double et; // StartTime keyword is the center exposure time
  str2et_c(stime.toLatin1().data(), &et);
  pair <iTime, iTime> shuttertimes = cam->ShutterOpenCloseTimes(et, exposureDuration);
  EXPECT_NEAR(shuttertimes.first.Et(), 600694634.18428946, 6E-14);
  EXPECT_NEAR(shuttertimes.second.Et(), 600694634.28428948, 6E-14);

  EXPECT_TRUE(cam->SetImage(5, 5));
  EXPECT_NEAR(cam->UniversalLatitude(), 9.26486, 0.0001);
  EXPECT_NEAR(cam->UniversalLongitude(), 276.167, 0.0001);

  EXPECT_TRUE(cam->SetUniversalGround(cam->UniversalLatitude(), cam->UniversalLongitude()));
  EXPECT_NEAR(cam->Line(), 5, 0.01);
  EXPECT_NEAR(cam->Sample(), 5, 0.01);
}


TEST_F(OsirisRexOcamsCube, MappingCam) {
  setInstrument("-64361", "MapCam");

  OsirisRexOcamsCamera *cam = (OsirisRexOcamsCamera *)testCube->camera();
  EXPECT_PRED_FORMAT2(AssertQStringsEqual, cam->spacecraftNameLong(), "OSIRIS-REx");
  EXPECT_PRED_FORMAT2(AssertQStringsEqual, cam->spacecraftNameShort(), "OSIRIS-REx");
  EXPECT_PRED_FORMAT2(AssertQStringsEqual, cam->instrumentNameLong(), "Mapping Camera");
  EXPECT_PRED_FORMAT2(AssertQStringsEqual, cam->instrumentNameShort(), "MapCam");
  EXPECT_PRED_FORMAT2(AssertQStringsEqual, cam->instrumentId(), "MapCam");
}


TEST_F(OsirisRexOcamsCube, SamplingCam) {
  setInstrument("-64362", "SamCam");

  OsirisRexOcamsCamera *cam = (OsirisRexOcamsCamera *)testCube->camera();

  EXPECT_EQ(cam->instrumentRotation()->Frame(), -27002);

  // Test kernel IDs
  EXPECT_EQ(cam->CkFrameId(), -64000);
  EXPECT_EQ(cam->CkReferenceId(), 1);
  EXPECT_EQ(cam->SpkTargetId(), -64);
  EXPECT_EQ(cam->SpkReferenceId(), 1);

  EXPECT_PRED_FORMAT2(AssertQStringsEqual, cam->spacecraftNameLong(), "OSIRIS-REx");
  EXPECT_PRED_FORMAT2(AssertQStringsEqual, cam->spacecraftNameShort(), "OSIRIS-REx");
  EXPECT_PRED_FORMAT2(AssertQStringsEqual, cam->instrumentNameLong(), "Sampling Camera");
  EXPECT_PRED_FORMAT2(AssertQStringsEqual, cam->instrumentNameShort(), "SamCam");
  EXPECT_PRED_FORMAT2(AssertQStringsEqual, cam->instrumentId(), "SamCam");

  const PvlGroup &inst = testCube->label()->findGroup("Instrument", Pvl::Traverse);
  double exposureDuration = ((double) inst["ExposureDuration"])/1000;
  cam->SetImage (0.5, 0.5);
  double et = cam->time().Et();
  pair <iTime, iTime> shuttertimes = cam->ShutterOpenCloseTimes(et, exposureDuration);
  EXPECT_NEAR(shuttertimes.first.Et(), 502476937.73296136, 1e-14);
  EXPECT_NEAR(shuttertimes.second.Et(), 502476937.83296138, 1e-14);

  EXPECT_TRUE(cam->SetImage(5, 5));
  EXPECT_NEAR(cam->UniversalLatitude(), 9.26486, 0.0001);
  EXPECT_NEAR(cam->UniversalLongitude(), 276.167, 0.0001);

  EXPECT_TRUE(cam->SetUniversalGround(cam->UniversalLatitude(), cam->UniversalLongitude()));
  EXPECT_NEAR(cam->Line(), 5, 0.01);
  EXPECT_NEAR(cam->Sample(), 5, 0.01);
}

TEST_F(OsirisRexOcamsCube, PolyCamUpdatedIkCodes){
setInstrument("-64500", "PolyCam");

  OsirisRexOcamsCamera *cam = (OsirisRexOcamsCamera *)testCube->camera();

  EXPECT_EQ(cam->instrumentRotation()->Frame(), -27002);

  // Test kernel IDs
  EXPECT_EQ(cam->CkFrameId(), -64000);
  EXPECT_EQ(cam->CkReferenceId(), 1);
  EXPECT_EQ(cam->SpkTargetId(), -64);
  EXPECT_EQ(cam->SpkReferenceId(), 1);

  // Test name methods
  EXPECT_PRED_FORMAT2(AssertQStringsEqual, cam->spacecraftNameLong(), "OSIRIS-REx");
  EXPECT_PRED_FORMAT2(AssertQStringsEqual, cam->spacecraftNameShort(), "OSIRIS-REx");
  EXPECT_PRED_FORMAT2(AssertQStringsEqual, cam->instrumentNameLong(), "PolyMath Camera");
  EXPECT_PRED_FORMAT2(AssertQStringsEqual, cam->instrumentNameShort(), "PolyCam");
  EXPECT_PRED_FORMAT2(AssertQStringsEqual, cam->instrumentId(), "PolyCam");

  const PvlGroup &inst = testCube->label()->findGroup("Instrument", Pvl::Traverse);
  double exposureDuration = ((double) inst["ExposureDuration"])/1000;
  QString stime = QString::fromStdString(inst["StartTime"]);
  double et; // StartTime keyword is the center exposure time
  str2et_c(stime.toLatin1().data(), &et);
  pair <iTime, iTime> shuttertimes = cam->ShutterOpenCloseTimes(et, exposureDuration);
  EXPECT_NEAR(shuttertimes.first.Et(), 600694634.18428946, 6E-14);
  EXPECT_NEAR(shuttertimes.second.Et(), 600694634.28428948, 6E-14);

  EXPECT_TRUE(cam->SetImage(5, 5));
  EXPECT_NEAR(cam->UniversalLatitude(), 9.26486, 0.0001);
  EXPECT_NEAR(cam->UniversalLongitude(), 276.167, 0.0001);

  EXPECT_TRUE(cam->SetUniversalGround(cam->UniversalLatitude(), cam->UniversalLongitude()));
  EXPECT_NEAR(cam->Line(), 5, 0.01);
  EXPECT_NEAR(cam->Sample(), 5, 0.01);
}
