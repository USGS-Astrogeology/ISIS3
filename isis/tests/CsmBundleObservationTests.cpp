#include <QString>
#include <iostream>
#include <math.h>

#include "CsmBundleObservation.h"
#include "CSMCamera.h"
#include "CsmFixtures.h"
#include "MockCsmPlugin.h"
#include "Mocks.h"
#include "TestUtilities.h"
#include "SerialNumber.h"
#include "BundleControlPoint.h"
#include "BundleImage.h"
#include "BundleTargetBody.h"


#include "gmock/gmock.h"


using namespace Isis;

TEST_F(CSMCameraFixture, CsmBundleOutputString) {
  EXPECT_CALL(mockModel, getNumParameters())
      .WillRepeatedly(::testing::Return(3));
  EXPECT_CALL(mockModel, getParameterType(0))
      .WillRepeatedly(::testing::Return(csm::param::REAL));
  EXPECT_CALL(mockModel, getParameterType(1))
      .WillRepeatedly(::testing::Return(csm::param::REAL));
  EXPECT_CALL(mockModel, getParameterType(2))
      .WillRepeatedly(::testing::Return(csm::param::REAL));
  EXPECT_CALL(mockModel, getParameterName(0))
      .WillRepeatedly(::testing::Return("Parameter 1"));
  EXPECT_CALL(mockModel, getParameterName(1))
      .WillRepeatedly(::testing::Return("Parameter 2"));
  EXPECT_CALL(mockModel, getParameterName(2))
      .WillRepeatedly(::testing::Return("Parameter 3"));
  EXPECT_CALL(mockModel, getParameterUnits(0))
      .WillRepeatedly(::testing::Return("m"));
  EXPECT_CALL(mockModel, getParameterUnits(1))
      .WillRepeatedly(::testing::Return("sec"));
  EXPECT_CALL(mockModel, getParameterUnits(2))
      .WillRepeatedly(::testing::Return("cm"));
  EXPECT_CALL(mockModel, getParameterValue(0))
      .WillRepeatedly(::testing::Return(234.2));
  EXPECT_CALL(mockModel, getParameterValue(1))
      .WillRepeatedly(::testing::Return(0.0));
  EXPECT_CALL(mockModel, getParameterValue(2))
      .WillRepeatedly(::testing::Return(M_PI));
  EXPECT_CALL(mockModel, getParameterCovariance(0, 0))
      .WillRepeatedly(::testing::Return(0.112));
  EXPECT_CALL(mockModel, getParameterCovariance(1, 1))
      .WillRepeatedly(::testing::Return(0.0123));
  EXPECT_CALL(mockModel, getParameterCovariance(2, 2))
      .WillRepeatedly(::testing::Return(0.342));

  std::stringstream fpOut;

  QString sn = SerialNumber::Compose(*testCube);

  BundleImageQsp bi = BundleImageQsp(new BundleImage(testCam, sn, testCube->fileName()));
  BundleObservationSolveSettings bundleSolSetting;

  bundleSolSetting.setCSMSolveSet(csm::param::ADJUSTABLE);

  CsmBundleObservation observation(bi,
                                   "ObservationNumber",
                                   "InstrumentId",
                                   nullptr);

  EXPECT_TRUE(observation.setSolveSettings(bundleSolSetting));
  observation.bundleOutputString(fpOut, false);

  QStringList lines = QString::fromStdString(fpOut.str()).split("\n");
  EXPECT_EQ(lines[0].toStdString(),
            "Parameter 1      234.20000000            0.00000000             234.20000000               0.112            N/A        m");
  EXPECT_EQ(lines[1].toStdString(),
            "Parameter 2        0.00000000            0.00000000               0.00000000              0.0123            N/A        sec");
  EXPECT_EQ(lines[2].toStdString(),
            "Parameter 3        3.14159265            0.00000000               3.14159265               0.342            N/A        cm");
}


TEST_F(CSMCameraFixture, CsmBundleOutputCSVString) {
  EXPECT_CALL(mockModel, getNumParameters())
      .WillRepeatedly(::testing::Return(3));
  EXPECT_CALL(mockModel, getParameterType(0))
      .WillRepeatedly(::testing::Return(csm::param::REAL));
  EXPECT_CALL(mockModel, getParameterType(1))
      .WillRepeatedly(::testing::Return(csm::param::REAL));
  EXPECT_CALL(mockModel, getParameterType(2))
      .WillRepeatedly(::testing::Return(csm::param::REAL));
  EXPECT_CALL(mockModel, getParameterName(0))
      .WillRepeatedly(::testing::Return("Parameter 1"));
  EXPECT_CALL(mockModel, getParameterName(1))
      .WillRepeatedly(::testing::Return("Parameter 2"));
  EXPECT_CALL(mockModel, getParameterName(2))
      .WillRepeatedly(::testing::Return("Parameter 3"));
  EXPECT_CALL(mockModel, getParameterUnits(0))
      .WillRepeatedly(::testing::Return("m"));
  EXPECT_CALL(mockModel, getParameterUnits(1))
      .WillRepeatedly(::testing::Return("sec"));
  EXPECT_CALL(mockModel, getParameterUnits(2))
      .WillRepeatedly(::testing::Return("cm"));
  EXPECT_CALL(mockModel, getParameterValue(0))
      .WillRepeatedly(::testing::Return(234.2));
  EXPECT_CALL(mockModel, getParameterValue(1))
      .WillRepeatedly(::testing::Return(0.0));
  EXPECT_CALL(mockModel, getParameterValue(2))
      .WillRepeatedly(::testing::Return(100.0));
  EXPECT_CALL(mockModel, getParameterCovariance(0, 0))
      .WillRepeatedly(::testing::Return(0.112));
  EXPECT_CALL(mockModel, getParameterCovariance(1, 1))
      .WillRepeatedly(::testing::Return(0.0123));
  EXPECT_CALL(mockModel, getParameterCovariance(2, 2))
      .WillRepeatedly(::testing::Return(0.342));

  QString sn = SerialNumber::Compose(*testCube);

  BundleImageQsp bi = BundleImageQsp(new BundleImage(testCam, sn, testCube->fileName()));
  BundleObservationSolveSettings bundleSolSetting;

  bundleSolSetting.setCSMSolveSet(csm::param::ADJUSTABLE);

  CsmBundleObservation observation(bi,
                                   "ObservationNumber",
                                   "InstrumentId",
                                   nullptr);

  EXPECT_TRUE(observation.setSolveSettings(bundleSolSetting));
  QString csvString = observation.bundleOutputCSV(false);
  EXPECT_EQ(csvString.toStdString(),
            "234.2,0,234.2,0.112,N/A,"
            "0,0,0,0.0123,N/A,"
            "100,0,100,0.342,N/A,");

  csvString = observation.bundleOutputCSV(true);
  EXPECT_FALSE(csvString.contains("N/A"));
}


TEST_F(CSMCameraFixture, CsmBundleSetSolveSettings) {
  EXPECT_CALL(mockModel, getNumParameters())
      .WillRepeatedly(::testing::Return(3));
  EXPECT_CALL(mockModel, getParameterType(0))
      .WillRepeatedly(::testing::Return(csm::param::FICTITIOUS));
  EXPECT_CALL(mockModel, getParameterType(1))
      .WillRepeatedly(::testing::Return(csm::param::FIXED));
  EXPECT_CALL(mockModel, getParameterType(2))
      .WillRepeatedly(::testing::Return(csm::param::REAL));
  EXPECT_CALL(mockModel, getParameterName(0))
      .WillRepeatedly(::testing::Return("Parameter 1"));
  EXPECT_CALL(mockModel, getParameterName(1))
      .WillRepeatedly(::testing::Return("Parameter 2"));
  EXPECT_CALL(mockModel, getParameterName(2))
      .WillRepeatedly(::testing::Return("Parameter 3"));
  EXPECT_CALL(mockModel, getParameterCovariance(0, 0))
      .WillRepeatedly(::testing::Return(0.112));
  EXPECT_CALL(mockModel, getParameterCovariance(1, 1))
      .WillRepeatedly(::testing::Return(0.0123));
  EXPECT_CALL(mockModel, getParameterCovariance(2, 2))
      .WillRepeatedly(::testing::Return(0.342));


  QString sn = SerialNumber::Compose(*testCube);

  BundleImageQsp bi = BundleImageQsp(new BundleImage(testCam, sn, testCube->fileName()));
  BundleObservationSolveSettings bundleSolSetting;

  CsmBundleObservation observation(bi,
                                   "ObservationNumber",
                                   "InstrumentId",
                                   nullptr);

  QStringList paramList;

  bundleSolSetting.setCSMSolveSet(csm::param::ADJUSTABLE);

  ASSERT_TRUE(observation.setSolveSettings(bundleSolSetting));
  ASSERT_EQ(observation.numberParameters(), 2);
  paramList = observation.parameterList();
  ASSERT_EQ(paramList.size(), 2);
  EXPECT_EQ(paramList[0].toStdString(), "Parameter 1");
  EXPECT_EQ(paramList[1].toStdString(), "Parameter 3");

  bundleSolSetting.setCSMSolveType(csm::param::FIXED);

  ASSERT_TRUE(observation.setSolveSettings(bundleSolSetting));
  ASSERT_EQ(observation.numberParameters(), 1);
  paramList = observation.parameterList();
  ASSERT_EQ(paramList.size(), 1);
  EXPECT_EQ(paramList[0].toStdString(), "Parameter 2");

  paramList.clear();
  paramList.push_back("Parameter 2");
  paramList.push_back("Parameter 3");
  bundleSolSetting.setCSMSolveParameterList(paramList);

  ASSERT_TRUE(observation.setSolveSettings(bundleSolSetting));
  ASSERT_EQ(observation.numberParameters(), 2);
  paramList = observation.parameterList();
  ASSERT_EQ(paramList.size(), 2);
  EXPECT_EQ(paramList[0].toStdString(), "Parameter 2");
  EXPECT_EQ(paramList[1].toStdString(), "Parameter 3");
}


TEST_F(CSMCameraFixture, CsmBundleApplyParameterCorrections) {
  EXPECT_CALL(mockModel, getNumParameters())
      .WillRepeatedly(::testing::Return(3));
  EXPECT_CALL(mockModel, getParameterType(0))
      .WillRepeatedly(::testing::Return(csm::param::FICTITIOUS));
  EXPECT_CALL(mockModel, getParameterType(1))
      .WillRepeatedly(::testing::Return(csm::param::FIXED));
  EXPECT_CALL(mockModel, getParameterType(2))
      .WillRepeatedly(::testing::Return(csm::param::REAL));
  EXPECT_CALL(mockModel, getParameterName(0))
      .WillRepeatedly(::testing::Return("Parameter 1"));
  EXPECT_CALL(mockModel, getParameterName(1))
      .WillRepeatedly(::testing::Return("Parameter 2"));
  EXPECT_CALL(mockModel, getParameterName(2))
      .WillRepeatedly(::testing::Return("Parameter 3"));
  EXPECT_CALL(mockModel, getParameterCovariance(0, 0))
      .WillRepeatedly(::testing::Return(0.112));
  EXPECT_CALL(mockModel, getParameterCovariance(1, 1))
      .WillRepeatedly(::testing::Return(0.0123));
  EXPECT_CALL(mockModel, getParameterCovariance(2, 2))
      .WillRepeatedly(::testing::Return(0.342));
  EXPECT_CALL(mockModel, getParameterValue(0))
      .WillRepeatedly(::testing::Return(234.2));
  EXPECT_CALL(mockModel, getParameterValue(1))
      .WillRepeatedly(::testing::Return(0.0));
  EXPECT_CALL(mockModel, getParameterValue(2))
      .WillRepeatedly(::testing::Return(100.0));
  EXPECT_CALL(mockModel, setParameterValue(0, 235.2))
      .Times(1);
  EXPECT_CALL(mockModel, setParameterValue(2, 110.0))
      .Times(1);


  QString sn = SerialNumber::Compose(*testCube);

  BundleImageQsp bi = BundleImageQsp(new BundleImage(testCam, sn, testCube->fileName()));
  BundleObservationSolveSettings bundleSolSetting;

  CsmBundleObservation observation(bi,
                                   "ObservationNumber",
                                   "InstrumentId",
                                   nullptr);

  QStringList paramList;

  bundleSolSetting.setCSMSolveSet(csm::param::ADJUSTABLE);

  ASSERT_TRUE(observation.setSolveSettings(bundleSolSetting));

  LinearAlgebra::Vector corrections(2);
  corrections[0] = 1.0;
  corrections[1] = 10.0;

  ASSERT_TRUE(observation.applyParameterCorrections(corrections));
}


TEST_F(CSMCameraFixture, CsmBundleComputePoint3DPartials) {
  EXPECT_CALL(mockModel, getNumParameters())
      .WillRepeatedly(::testing::Return(3));
  EXPECT_CALL(mockModel, getParameterType(0))
      .WillRepeatedly(::testing::Return(csm::param::FICTITIOUS));
  EXPECT_CALL(mockModel, getParameterType(1))
      .WillRepeatedly(::testing::Return(csm::param::FIXED));
  EXPECT_CALL(mockModel, getParameterType(2))
      .WillRepeatedly(::testing::Return(csm::param::REAL));
  EXPECT_CALL(mockModel, getParameterName(0))
      .WillRepeatedly(::testing::Return("Parameter 1"));
  EXPECT_CALL(mockModel, getParameterName(1))
      .WillRepeatedly(::testing::Return("Parameter 2"));
  EXPECT_CALL(mockModel, getParameterName(2))
      .WillRepeatedly(::testing::Return("Parameter 3"));
  EXPECT_CALL(mockModel, getParameterCovariance(0, 0))
      .WillRepeatedly(::testing::Return(0.112));
  EXPECT_CALL(mockModel, getParameterCovariance(1, 1))
      .WillRepeatedly(::testing::Return(0.0123));
  EXPECT_CALL(mockModel, getParameterCovariance(2, 2))
      .WillRepeatedly(::testing::Return(0.342));
  EXPECT_CALL(mockModel, computeGroundPartials)
      .WillRepeatedly(::testing::Return(std::vector<double>{1, 2, 3, 4, 5, 6}));


  QString sn = SerialNumber::Compose(*testCube);

  BundleImageQsp bi = BundleImageQsp(new BundleImage(testCam, sn, testCube->fileName()));
  BundleObservationSolveSettings bundleSolSetting;

  CsmBundleObservation observation(bi,
                                   "ObservationNumber",
                                   "InstrumentId",
                                   nullptr);

  QStringList paramList;

  bundleSolSetting.setCSMSolveSet(csm::param::ADJUSTABLE);

  ASSERT_TRUE(observation.setSolveSettings(bundleSolSetting));

  BundleSettingsQsp testBundleSettings(new BundleSettings());

  SurfacePoint testSurfacePoint(Displacement(1000.0, Displacement::Kilometers),
                                Displacement(0.0, Displacement::Kilometers),
                                Displacement(0.0, Displacement::Kilometers));

  ControlPoint testPoint("testPoint");
  testPoint.SetAdjustedSurfacePoint(testSurfacePoint);

  ControlMeasure *testMeasure = new ControlMeasure();
  testMeasure->SetCubeSerialNumber(sn);
  testMeasure->SetCamera(testCam);
  testPoint.Add(testMeasure);

  BundleControlPointQsp testBundlePoint(new BundleControlPoint(testBundleSettings, &testPoint));
  BundleMeasureQsp testBundleMeasure = testBundlePoint->front();

  LinearAlgebra::Matrix coeffPoint3D(2, 3);

  ASSERT_TRUE(observation.computePoint3DPartials(coeffPoint3D, *testBundleMeasure, SurfacePoint::Rectangular));

  EXPECT_EQ(coeffPoint3D(0,0), 4000);
  EXPECT_EQ(coeffPoint3D(0,1), 5000);
  EXPECT_EQ(coeffPoint3D(0,2), 6000);
  EXPECT_EQ(coeffPoint3D(1,0), 1000);
  EXPECT_EQ(coeffPoint3D(1,1), 2000);
  EXPECT_EQ(coeffPoint3D(1,2), 3000);

  ASSERT_TRUE(observation.computePoint3DPartials(coeffPoint3D, *testBundleMeasure, SurfacePoint::Latitudinal));

  EXPECT_EQ(coeffPoint3D(0,0), 6000000);
  EXPECT_EQ(coeffPoint3D(0,1), 5000000);
  EXPECT_EQ(coeffPoint3D(0,2), 4000);
  EXPECT_EQ(coeffPoint3D(1,0), 3000000);
  EXPECT_EQ(coeffPoint3D(1,1), 2000000);
  EXPECT_EQ(coeffPoint3D(1,2), 1000);
}