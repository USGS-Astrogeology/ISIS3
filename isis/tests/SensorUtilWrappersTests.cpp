#include "gmock/gmock.h"

#include "CameraFixtures.h"
#include "Mocks.h"

#include "csm.h"
#include <vector>

#include "CsmSensor.h"
#include "Cube.h"
#include "Displacement.h"
#include "IsisIlluminator.h"
#include "IsisSensor.h"
#include "IsisShape.h"
#include "IsisBody.h"
#include "SurfacePoint.h"

using namespace std;

class SpiceKernels : public ::testing::Test {
  protected:
    vector<QString> kernels;
    double startTime;
    double endTime;
    int frameCode;
    int targetCode;

  void SetUp() {
    startTime = -69382819.0;
    endTime = -69382512.0;
    frameCode = -94031;
    targetCode = 499;

    QString dir = QString::fromStdString(FileName("$ISISTESTDATA/isis/src/base/unitTestData/kernels").expanded() + "/");
    kernels.clear();
    kernels.push_back(dir + "naif0007.tls");
    kernels.push_back(dir + "MGS_SCLKSCET.00045.tsc");
    kernels.push_back(dir + "moc13.ti");
    kernels.push_back(dir + "moc.bc");
    kernels.push_back(dir + "moc.bsp");
    kernels.push_back(dir + "de405.bsp");
    kernels.push_back(dir + "pck00009.tpc");
    kernels.push_back(dir + "mocSpiceRotationUnitTest.ti");
    kernels.push_back(dir + "ROS_V29.TF");
    kernels.push_back(dir + "CATT_DV_145_02_______00216.BC");
    for (QString& kernel : kernels) {
      furnsh_c(kernel.toLatin1().data());
    }
  }

  void TearDown() {
    for (QString& kernel : kernels) {
      unload_c(kernel.toLatin1().data());
    }
  }
};

// Custom csm equality operators for mock matching
namespace csm {
  bool operator==(const ImageCoord &lhs, const ImageCoord &rhs) {
    return lhs.line == rhs.line && lhs.samp == rhs.samp;
  }


  bool operator==(const EcefCoord &lhs, const EcefCoord &rhs) {
    return lhs.x == rhs.x && lhs.y == rhs.y && lhs.z == rhs.z;
  }
}

TEST(IsisIlluminator, PositionNewTime) {
  double testTime = 10.0;
  double oldTime = testTime - 1;
  std::vector<double> testPosKm = {-1.0, 1.0, 2.0};
  SensorUtilities::Vec testPosM = {-1000.0, 1000.0, 2000.0};

  MockSpicePosition mockSpice(0, 1);
  EXPECT_CALL(mockSpice, EphemerisTime)
        .WillRepeatedly(::testing::Return(oldTime));

  EXPECT_CALL(mockSpice, SetEphemerisTime(testTime))
        .WillRepeatedly(::testing::ReturnRef(testPosKm));

  EXPECT_CALL(mockSpice, Coordinate)
        .WillRepeatedly(::testing::ReturnRef(testPosKm));

  EXPECT_CALL(mockSpice, SetEphemerisTime(oldTime))
        .WillRepeatedly(::testing::ReturnRef(testPosKm));

  IsisIlluminator testIlluminator(&mockSpice);

  EXPECT_EQ(testIlluminator.position(testTime), testPosM);
}

TEST(IsisIlluminator, PositionOldTime) {
  double testTime = 10.0;
  std::vector<double> testPosKm = {-1.0, 1.0, 2.0};
  SensorUtilities::Vec testPosM = {-1000.0, 1000.0, 2000.0};

  MockSpicePosition mockSpice(0, 1);
  EXPECT_CALL(mockSpice, EphemerisTime)
        .WillRepeatedly(::testing::Return(testTime));

  EXPECT_CALL(mockSpice, Coordinate)
        .WillRepeatedly(::testing::ReturnRef(testPosKm));

  IsisIlluminator testIlluminator(&mockSpice);

  EXPECT_EQ(testIlluminator.position(testTime), testPosM);
}

TEST(IsisIlluminator, Velocity) {
  double testTime = 10.0;
  SensorUtilities::Vec testVelM = {-1000.0, 1000.0, 2000.0};

  MockSpicePosition mockSpice(0, 1);

  const double state[6] = {0.0, 0,0, -1.0, 1.0, 2.0};
  mockSpice.setStateVector(state, true);

  EXPECT_CALL(mockSpice, EphemerisTime)
        .WillRepeatedly(::testing::Return(testTime));

  IsisIlluminator testIlluminator(&mockSpice);

  EXPECT_EQ(testIlluminator.velocity(testTime), testVelM);
}

TEST(IsisShape, IntersectStandardNormal) {
  SensorUtilities::Vec sensorPos = {1000.0, 0.0, 0.0};
  SensorUtilities::Vec lookVec = {-1.0, 0.0, 0.0};
  SensorUtilities::Vec groundPt = {100.0, 0.0, 0.0};
  SensorUtilities::Vec testNormal = {1.0, 0.0, 0.0};
  SurfacePoint testPoint(
        Displacement(groundPt.x, Displacement::Meters),
        Displacement(groundPt.y, Displacement::Meters),
        Displacement(groundPt.z, Displacement::Meters));

  MockShapeModel mockShape;
  EXPECT_CALL(mockShape, intersectSurface(std::vector<double>(sensorPos), std::vector<double>(lookVec)));
  EXPECT_CALL(mockShape, surfaceIntersection())
        .WillRepeatedly(::testing::Return(&testPoint));
  EXPECT_CALL(mockShape, calculateSurfaceNormal);
  EXPECT_CALL(mockShape, normal())
        .WillRepeatedly(::testing::Return(testNormal));

  IsisShape testShape(&mockShape);

  SensorUtilities::Intersection intersection = testShape.intersect(sensorPos, lookVec, false);

  EXPECT_EQ(intersection.groundPt, groundPt);
  EXPECT_EQ(intersection.normal, testNormal);
}

TEST(IsisShape, IntersectDEM) {
  SensorUtilities::Vec sensorPos = {1000.0, 0.0, 0.0};
  SensorUtilities::Vec lookVec = {-1.0, 0.0, 0.0};
  SensorUtilities::Vec groundPt = {100.0, 0.0, 0.0};
  SensorUtilities::Vec testNormal = {1.0, 0.0, 0.0};
  SurfacePoint testPoint(
        Displacement(groundPt.x, Displacement::Meters),
        Displacement(groundPt.y, Displacement::Meters),
        Displacement(groundPt.z, Displacement::Meters));

  MockShapeModel mockShape;
  EXPECT_CALL(mockShape, intersectSurface(std::vector<double>(sensorPos), std::vector<double>(lookVec)));
  EXPECT_CALL(mockShape, surfaceIntersection())
        .WillRepeatedly(::testing::Return(&testPoint));
  EXPECT_CALL(mockShape, isDEM())
        .WillRepeatedly(::testing::Return(true));
  EXPECT_CALL(mockShape, calculateSurfaceNormal);
  EXPECT_CALL(mockShape, normal())
        .WillRepeatedly(::testing::Return(testNormal));

  IsisShape testShape(&mockShape);

  SensorUtilities::Intersection intersection = testShape.intersect(sensorPos, lookVec, true);

  EXPECT_EQ(intersection.groundPt, groundPt);
  EXPECT_EQ(intersection.normal, testNormal);
}

TEST(IsisShape, IntersectLocalNormal) {
  SensorUtilities::Vec sensorPos = {1000.0, 0.0, 0.0};
  SensorUtilities::Vec lookVec = {-1.0, 0.0, 0.0};
  SensorUtilities::Vec groundPt = {100.0, 0.0, 0.0};
  SensorUtilities::Vec testNormal = {1.0, 0.0, 0.0};
  SurfacePoint testPoint(
        Displacement(groundPt.x, Displacement::Meters),
        Displacement(groundPt.y, Displacement::Meters),
        Displacement(groundPt.z, Displacement::Meters));

  MockShapeModel mockShape;
  EXPECT_CALL(mockShape, intersectSurface(std::vector<double>(sensorPos), std::vector<double>(lookVec)));
  EXPECT_CALL(mockShape, surfaceIntersection())
        .WillRepeatedly(::testing::Return(&testPoint));
  EXPECT_CALL(mockShape, isDEM())
        .WillRepeatedly(::testing::Return(false));
  EXPECT_CALL(mockShape, calculateLocalNormal);
  EXPECT_CALL(mockShape, normal())
        .WillRepeatedly(::testing::Return(testNormal));

  IsisShape testShape(&mockShape);

  SensorUtilities::Intersection intersection = testShape.intersect(sensorPos, lookVec, true);

  EXPECT_EQ(intersection.groundPt, groundPt);
  EXPECT_EQ(intersection.normal, testNormal);
}

TEST(CsmSensor, GetStateImage) {
  SensorUtilities::Vec sensorPos = {1000.0, 0.0, 0.0};
  SensorUtilities::Vec lookVec = {-1.0, 0.0, 0.0};
  SensorUtilities::ImagePt testImagePt = {10, 20};
  double testTime = 0.5;

  // Test orientation goes from no rotation at 0 to 180 degrees around x-axis at 1
  ale::Orientations testOrientations({{0, 0, 0, 0}, {0, 1, 0, 0}}, {0, 1});

  ale::Vec3d j2000AleLookVec = testOrientations.rotateVectorAt(
        testTime, ale::Vec3d(lookVec.x ,lookVec.y, lookVec.z));
  SensorUtilities::Vec j2000LookVec(j2000AleLookVec.x, j2000AleLookVec.y, j2000AleLookVec.z);

  csm::ImageCoord testCsmCoord(testImagePt.line, testImagePt.sample);
  csm::EcefLocus testCsmLocus(sensorPos.x, sensorPos.y, sensorPos.z, lookVec.x, lookVec.y, lookVec.z);

  MockRasterGM mockModel;
  EXPECT_CALL(mockModel, getImageTime(testCsmCoord))
        .WillRepeatedly(::testing::Return(testTime));
  EXPECT_CALL(mockModel, imageToRemoteImagingLocus(testCsmCoord, ::testing::_, ::testing::_, ::testing::_))
        .WillRepeatedly(::testing::Return(testCsmLocus));

  CsmSensor testSensor(&mockModel, &testOrientations);

  SensorUtilities::ObserverState obsState = testSensor.getState(testImagePt);

  EXPECT_EQ(obsState.lookVec, lookVec);
  EXPECT_EQ(obsState.j2000LookVec, j2000LookVec);
  EXPECT_EQ(obsState.sensorPos, sensorPos);
  EXPECT_EQ(obsState.time, testTime);
  EXPECT_EQ(obsState.imagePoint, testImagePt);
}

TEST(CsmSensor, GetStateGround) {
  SensorUtilities::GroundPt3D testGroundPt = {0.0, 0.0, 100.0};
  SensorUtilities::Vec sensorPos = {1000.0, 0.0, 0.0};
  SensorUtilities::Vec lookVec = {-1.0, 0.0, 0.0};
  SensorUtilities::ImagePt testImagePt = {10, 20};
  double testTime = 0.5;

  // Test orientation goes from no rotation at 0 to 180 degrees around z-axis at 1
  ale::Orientations testOrientations({{0, 0, 0, 0}, {0, 0, 0, 1}}, {0, 1});

  ale::Vec3d j2000AleLookVec = testOrientations.rotateVectorAt(
        testTime, ale::Vec3d(lookVec.x ,lookVec.y, lookVec.z));
  SensorUtilities::Vec j2000LookVec(j2000AleLookVec.x, j2000AleLookVec.y, j2000AleLookVec.z);

  SensorUtilities::Vec testGroundVec = SensorUtilities::sphericalToRect(testGroundPt);
  csm::EcefCoord testCsmGroundPt = {testGroundVec.x, testGroundVec.y, testGroundVec.z};
  csm::ImageCoord testCsmCoord(testImagePt.line, testImagePt.sample);
  csm::EcefLocus testCsmLocus(sensorPos.x, sensorPos.y, sensorPos.z, lookVec.x, lookVec.y, lookVec.z);

  MockRasterGM mockModel;
  EXPECT_CALL(mockModel, groundToImage(testCsmGroundPt, ::testing::_, ::testing::_, ::testing::_))
        .WillRepeatedly(::testing::Return(testCsmCoord));
  EXPECT_CALL(mockModel, getImageTime(testCsmCoord))
        .WillRepeatedly(::testing::Return(testTime));
  EXPECT_CALL(mockModel, imageToRemoteImagingLocus(testCsmCoord, ::testing::_, ::testing::_, ::testing::_))
        .WillRepeatedly(::testing::Return(testCsmLocus));

  CsmSensor testSensor(&mockModel, &testOrientations);

  SensorUtilities::ObserverState obsState = testSensor.getState(testGroundPt);

  EXPECT_EQ(obsState.lookVec, lookVec);
  EXPECT_EQ(obsState.j2000LookVec, j2000LookVec);
  EXPECT_EQ(obsState.sensorPos, sensorPos);
  EXPECT_EQ(obsState.time, testTime);
  EXPECT_EQ(obsState.imagePoint, testImagePt);
}

TEST_F(DefaultCube, IsisSensorGetStateImageNewPoint) {
  SensorUtilities::Vec sensorPos = {1000.0, 0.0, 0.0};
  SensorUtilities::Vec lookVec = {-1.0, 0.0, 0.0};
  SensorUtilities::Vec lookVecJ2000 = {0.0, -1.0, 0.0};
  SensorUtilities::ImagePt oldImagePt = {15, 25, 5};
  SensorUtilities::ImagePt testImagePt = {10, 20};
  double testTime = 0.5;

  std::vector<double> sensorPosArray = {sensorPos.x / 1000.0, sensorPos.y / 1000.0, sensorPos.z / 1000.0};
  std::vector<double> lookVecArray = {lookVec.x, lookVec.y, lookVec.z};
  std::vector<double> lookVecJ2000Array = {lookVecJ2000.x, lookVecJ2000.y, lookVecJ2000.z};

  MockCamera mockCam(*testCube);
  EXPECT_CALL(mockCam, Line())
        .WillRepeatedly(::testing::Return(oldImagePt.line));
  EXPECT_CALL(mockCam, Sample())
        .WillRepeatedly(::testing::Return(oldImagePt.sample));
  EXPECT_CALL(mockCam, Band())
        .WillRepeatedly(::testing::Return(oldImagePt.band));
  EXPECT_CALL(mockCam, IsBandIndependent())
        .WillRepeatedly(::testing::Return(true));
  EXPECT_CALL(mockCam, SetBand(testImagePt.band + 1));
  EXPECT_CALL(mockCam, SetImage(testImagePt.sample + 0.5, testImagePt.line + 0.5))
        .WillRepeatedly(::testing::Return(true));
  EXPECT_CALL(mockCam, lookDirectionBodyFixed())
        .WillRepeatedly(::testing::Return(lookVecArray));
  EXPECT_CALL(mockCam, lookDirectionJ2000())
        .WillRepeatedly(::testing::Return(lookVecJ2000Array));
  EXPECT_CALL(mockCam, instrumentBodyFixedPosition(::testing::_))
        .WillRepeatedly(::testing::SetArrayArgument<0>(sensorPosArray.begin(), sensorPosArray.end()));
  EXPECT_CALL(mockCam, time())
        .WillRepeatedly(::testing::Return(iTime(testTime)));
  EXPECT_CALL(mockCam, SetBand(oldImagePt.band));
  EXPECT_CALL(mockCam, SetImage(oldImagePt.sample, oldImagePt.line))
        .WillRepeatedly(::testing::Return(true));

  IsisSensor testSensor(&mockCam);

  SensorUtilities::ObserverState obsState = testSensor.getState(testImagePt);

  EXPECT_EQ(obsState.lookVec, lookVec);
  EXPECT_EQ(obsState.j2000LookVec, lookVecJ2000);
  EXPECT_EQ(obsState.sensorPos, sensorPos);
  EXPECT_EQ(obsState.time, testTime);
  EXPECT_EQ(obsState.imagePoint, testImagePt);
}

TEST_F(DefaultCube, IsisSensorGetStateImageOldPoint) {
  SensorUtilities::Vec sensorPos = {1000.0, 0.0, 0.0};
  SensorUtilities::Vec lookVec = {-1.0, 0.0, 0.0};
  SensorUtilities::Vec lookVecJ2000 = {0.0, -1.0, 0.0};
  SensorUtilities::ImagePt testImagePt = {10, 20};
  double testTime = 0.5;

  std::vector<double> sensorPosArray = {sensorPos.x / 1000.0, sensorPos.y / 1000.0, sensorPos.z / 1000.0};
  std::vector<double> lookVecArray = {lookVec.x, lookVec.y, lookVec.z};
  std::vector<double> lookVecJ2000Array = {lookVecJ2000.x, lookVecJ2000.y, lookVecJ2000.z};

  MockCamera mockCam(*testCube);
  EXPECT_CALL(mockCam, Line())
        .WillRepeatedly(::testing::Return(testImagePt.line + 0.5));
  EXPECT_CALL(mockCam, Sample())
        .WillRepeatedly(::testing::Return(testImagePt.sample + 0.5));
  EXPECT_CALL(mockCam, Band())
        .WillRepeatedly(::testing::Return(testImagePt.band + 1));
  EXPECT_CALL(mockCam, IsBandIndependent())
        .WillRepeatedly(::testing::Return(true));
  EXPECT_CALL(mockCam, lookDirectionBodyFixed())
        .WillRepeatedly(::testing::Return(lookVecArray));
  EXPECT_CALL(mockCam, lookDirectionJ2000())
        .WillRepeatedly(::testing::Return(lookVecJ2000Array));
  EXPECT_CALL(mockCam, instrumentBodyFixedPosition(::testing::_))
        .WillRepeatedly(::testing::SetArrayArgument<0>(sensorPosArray.begin(), sensorPosArray.end()));
  EXPECT_CALL(mockCam, time())
        .WillRepeatedly(::testing::Return(iTime(testTime)));

  IsisSensor testSensor(&mockCam);

  SensorUtilities::ObserverState obsState = testSensor.getState(testImagePt);

  EXPECT_EQ(obsState.lookVec, lookVec);
  EXPECT_EQ(obsState.j2000LookVec, lookVecJ2000);
  EXPECT_EQ(obsState.sensorPos, sensorPos);
  EXPECT_EQ(obsState.time, testTime);
  EXPECT_EQ(obsState.imagePoint, testImagePt);
}

TEST_F(DefaultCube, IsisSensorGetStateGroundNewPoint) {
  SensorUtilities::GroundPt3D testGroundPt = {0.0, 0.0, 100.0};
  SensorUtilities::Vec sensorPos = {1000.0, 0.0, 0.0};
  SensorUtilities::Vec lookVec = {-1.0, 0.0, 0.0};
  SensorUtilities::Vec lookVecJ2000 = {0.0, -1.0, 0.0};
  SensorUtilities::ImagePt oldImagePt = {15, 25, 5};
  SensorUtilities::ImagePt testImagePt = {10, 20};
  double testTime = 0.5;

  SensorUtilities::Vec testGroundVec = SensorUtilities::sphericalToRect(testGroundPt);
  SurfacePoint testPoint(
        Displacement(testGroundVec.x, Displacement::Meters),
        Displacement(testGroundVec.y, Displacement::Meters),
        Displacement(testGroundVec.z, Displacement::Meters));
  SurfacePoint oldPoint(
        Displacement(testGroundVec.x + 10.0, Displacement::Meters),
        Displacement(testGroundVec.y, Displacement::Meters),
        Displacement(testGroundVec.z, Displacement::Meters));
  std::vector<double> sensorPosArray = {sensorPos.x / 1000.0, sensorPos.y / 1000.0, sensorPos.z / 1000.0};
  std::vector<double> lookVecArray = {lookVec.x, lookVec.y, lookVec.z};
  std::vector<double> lookVecJ2000Array = {lookVecJ2000.x, lookVecJ2000.y, lookVecJ2000.z};

  MockCamera mockCam(*testCube);
  EXPECT_CALL(mockCam, GetSurfacePoint())
        .WillRepeatedly(::testing::Return(oldPoint));
  EXPECT_CALL(mockCam, Line())
        .WillOnce(::testing::Return(oldImagePt.line))
        .WillOnce(::testing::Return(testImagePt.line + 0.5));
  EXPECT_CALL(mockCam, Sample())
        .WillOnce(::testing::Return(oldImagePt.sample))
        .WillOnce(::testing::Return(testImagePt.sample + 0.5));
  EXPECT_CALL(mockCam, Band())
        .WillOnce(::testing::Return(oldImagePt.band))
        .WillOnce(::testing::Return(testImagePt.band + 1));
  EXPECT_CALL(mockCam, SetGround(testPoint))
        .WillRepeatedly(::testing::Return(true));
  EXPECT_CALL(mockCam, lookDirectionBodyFixed())
        .WillRepeatedly(::testing::Return(lookVecArray));
  EXPECT_CALL(mockCam, lookDirectionJ2000())
        .WillRepeatedly(::testing::Return(lookVecJ2000Array));
  EXPECT_CALL(mockCam, instrumentBodyFixedPosition(::testing::_))
        .WillRepeatedly(::testing::SetArrayArgument<0>(sensorPosArray.begin(), sensorPosArray.end()));
  EXPECT_CALL(mockCam, time())
        .WillRepeatedly(::testing::Return(iTime(testTime)));
  EXPECT_CALL(mockCam, SetBand(oldImagePt.band));
  EXPECT_CALL(mockCam, SetImage(oldImagePt.sample, oldImagePt.line))
        .WillRepeatedly(::testing::Return(true));

  IsisSensor testSensor(&mockCam);

  SensorUtilities::ObserverState obsState = testSensor.getState(testGroundPt);

  EXPECT_EQ(obsState.lookVec, lookVec);
  EXPECT_EQ(obsState.j2000LookVec, lookVecJ2000);
  EXPECT_EQ(obsState.sensorPos, sensorPos);
  EXPECT_EQ(obsState.time, testTime);
  EXPECT_EQ(obsState.imagePoint, testImagePt);
}

TEST_F(DefaultCube, IsisSensorGetStateGroundOldPoint) {
  SensorUtilities::GroundPt3D testGroundPt = {0.0, 0.0, 100.0};
  SensorUtilities::Vec sensorPos = {1000.0, 0.0, 0.0};
  SensorUtilities::Vec lookVec = {-1.0, 0.0, 0.0};
  SensorUtilities::Vec lookVecJ2000 = {0.0, -1.0, 0.0};
  SensorUtilities::ImagePt testImagePt = {10, 20};
  double testTime = 0.5;

  SensorUtilities::Vec testGroundVec = SensorUtilities::sphericalToRect(testGroundPt);
  SurfacePoint testPoint(
        Displacement(testGroundVec.x, Displacement::Meters),
        Displacement(testGroundVec.y, Displacement::Meters),
        Displacement(testGroundVec.z, Displacement::Meters));
  std::vector<double> sensorPosArray = {sensorPos.x / 1000.0, sensorPos.y / 1000.0, sensorPos.z / 1000.0};
  std::vector<double> lookVecArray = {lookVec.x, lookVec.y, lookVec.z};
  std::vector<double> lookVecJ2000Array = {lookVecJ2000.x, lookVecJ2000.y, lookVecJ2000.z};

  MockCamera mockCam(*testCube);
  EXPECT_CALL(mockCam, GetSurfacePoint())
        .WillRepeatedly(::testing::Return(testPoint));
  EXPECT_CALL(mockCam, Line())
        .WillRepeatedly(::testing::Return(testImagePt.line + 0.5));
  EXPECT_CALL(mockCam, Sample())
        .WillRepeatedly(::testing::Return(testImagePt.sample + 0.5));
  EXPECT_CALL(mockCam, Band())
        .WillRepeatedly(::testing::Return(testImagePt.band + 1));
  EXPECT_CALL(mockCam, lookDirectionBodyFixed())
        .WillRepeatedly(::testing::Return(lookVecArray));
  EXPECT_CALL(mockCam, lookDirectionJ2000())
        .WillRepeatedly(::testing::Return(lookVecJ2000Array));
  EXPECT_CALL(mockCam, instrumentBodyFixedPosition(::testing::_))
        .WillRepeatedly(::testing::SetArrayArgument<0>(sensorPosArray.begin(), sensorPosArray.end()));
  EXPECT_CALL(mockCam, time())
        .WillRepeatedly(::testing::Return(iTime(testTime)));

  IsisSensor testSensor(&mockCam);

  SensorUtilities::ObserverState obsState = testSensor.getState(testGroundPt);

  EXPECT_EQ(obsState.lookVec, lookVec);
  EXPECT_EQ(obsState.j2000LookVec, lookVecJ2000);
  EXPECT_EQ(obsState.sensorPos, sensorPos);
  EXPECT_EQ(obsState.time, testTime);
  EXPECT_EQ(obsState.imagePoint, testImagePt);
}

TEST_F(SpiceKernels, IsisBodyRotation) {
  std::vector<double> expectedResult = { -617.29587681050612, 406.01820331225525, -673.86572776172341, 
                                           10.22369321950767, 860.60644926850875, 509.16796401030905, 
                                           786.66465318438088, 307.41788831578776, -535.39981824218091 };
  
  SpiceRotation rot(frameCode);
  rot.SetEphemerisTime(startTime);

  IsisBody testBody(&rot);
  std::vector<double> rotation = testBody.rotation(-69382512);

  EXPECT_EQ(testBody.rotation(-69382512), expectedResult);
}

TEST_F(SpiceKernels, IsisBodyFixedVector) {
  SpiceRotation rot(frameCode);
  rot.SetEphemerisTime(startTime);
  IsisBody testBody(&rot);

  SensorUtilities::Vec pos = {-0.1, 0.0, 0.0};
  SensorUtilities::Vec expectResult = {87.506927366393171, -1.1442263253059641, -48.386241939308213};
  SensorUtilities::Vec result = testBody.fixedVector(pos);

  EXPECT_EQ(result.x, expectResult.x);
  EXPECT_EQ(result.y, expectResult.y);
  EXPECT_EQ(result.z, expectResult.z);
}
