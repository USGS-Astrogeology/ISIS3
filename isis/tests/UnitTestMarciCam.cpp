#include "MarciCamera.h"
#include "PushFrameCameraDetectorMap.h"

#include "TempFixtures.h"
#include "gmock/gmock.h"

using namespace Isis;

TEST_F(TempTestingFiles, UnitTestMarciCameraPhocubeBandChange) {

  QString cubeFileName = "data/marcical/P12_005901_3391_MA_00N096W_cropped.cub";

  // simulate phocube
  std::istringstream bbin_stream(R"(
     Group = BandBin
       FilterName   = BLUE
       OriginalBand = (1, 1, 1, 1, 1)
       Name         = ("Phase Angle", "Emission Angle", "Incidence Angle",
                       Latitude, Longitude)
       Center       = (1.0, 1.0, 1.0, 1.0, 1.0)
       Width        = (1.0, 1.0, 1.0, 1.0, 1.0)
     End_Group
  )");

  PvlGroup newbbin;
  bbin_stream >> newbbin;

  Cube cube(cubeFileName.toStdString());
  Pvl *isisLabel = cube.label();
  PvlGroup &ogbbin = isisLabel->findGroup("BandBin", Pvl::Traverse);
  ogbbin = newbbin;

  Camera *cam = cube.camera();
  PushFrameCameraDetectorMap *dmap = (PushFrameCameraDetectorMap*) cam->DetectorMap();

  // properties should BLUE filter's properties after band change
  cam->SetBand(1);
  EXPECT_EQ(dmap->GetBandFirstDetectorLine(), 709);
  cam->SetBand(4);
  EXPECT_EQ(dmap->GetBandFirstDetectorLine(), 709);
}