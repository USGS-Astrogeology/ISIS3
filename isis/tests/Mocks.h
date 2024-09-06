#ifndef Mocks_h
#define Mocks_h

#include "gmock/gmock.h"

#include "csm/CorrelationModel.h"
#include "csm/csm.h"
#include "csm/RasterGM.h"

#include "Cube.h"
#include "Camera.h"
#include "Distance.h"
#include "Interpolator.h"
#include "iTime.h"
#include "Latitude.h"
#include "Longitude.h"
#include "Process.h"
#include "ProcessRubberSheet.h"
#include "ShapeModel.h"
#include "SpicePosition.h"
#include "SurfacePoint.h"
#include "TProjection.h"
#include "Transform.h"

using namespace Isis;

class MockSpicePosition : public SpicePosition {
  public:
    MockSpicePosition(int targetCode, int observerCode): SpicePosition(targetCode, observerCode) {}
    MOCK_METHOD(const std::vector<double>&, SetEphemerisTime, (double et));
    MOCK_METHOD(double, EphemerisTime, (), (const));
    MOCK_METHOD(const std::vector<double>&, Coordinate, ());
};

class MockCube : public Cube {
  public:
    MOCK_CONST_METHOD0(bandCount, int());
    MOCK_CONST_METHOD0(fileName, QString());
    MOCK_CONST_METHOD1(physicalBand, int(const int &virtualBand));
    MOCK_METHOD4(histogram, Histogram*(
          const int &band, const double &validMin,
          const double &validMax,
          std::string msg));
};

class MockCamera : public Camera {
  public:
    MockCamera(Cube &cube): Camera(cube) {}
    MOCK_METHOD(bool, SetImage, (const double sample, const double line), (override));
    MOCK_METHOD(double, Line, (), (const, override));
    MOCK_METHOD(double, Sample, (), (const, override));
    MOCK_METHOD(void, SetBand, (int band), (override));
    MOCK_METHOD(int, Band, (), (const, override));
    MOCK_METHOD(bool, SetGround, (const SurfacePoint &surfacePt), (override));
    MOCK_METHOD(SurfacePoint, GetSurfacePoint, (), (const, override));
    MOCK_METHOD(bool, IsBandIndependent, (), (override));
    MOCK_METHOD(double, UniversalLatitude, (), (const override));
    MOCK_METHOD(double, UniversalLongitude, (), (const override));
    MOCK_METHOD(bool, SetUniversalGround, (const double latitude, const double longitude), (override));
    MOCK_METHOD(bool, SetUniversalGround, (const double latitude, const double longitude,
                                           const double radius), (override));
    MOCK_METHOD(CameraType, GetCameraType, (), (const, override));
    MOCK_METHOD(int, CkFrameId, (), (const override));
    MOCK_METHOD(int, CkReferenceId, (), (const override));
    MOCK_METHOD(int, SpkReferenceId, (), (const override));
    MOCK_METHOD(std::vector<double>, lookDirectionBodyFixed, (), (const, override));
    MOCK_METHOD(std::vector<double>, lookDirectionJ2000, (), (const, override));
    MOCK_METHOD(void, instrumentBodyFixedPosition, (double p[3]), (const, override));
    MOCK_METHOD(iTime, time, (), (const, override));
};

class MockTProjection : public TProjection {
  public:
    MockTProjection(Pvl &label): TProjection(label) {}
    MOCK_METHOD(bool, SetWorld, (const double x, const double y), (override));
    MOCK_METHOD(bool, HasGroundRange, (), (const override));
    MOCK_METHOD(double, UniversalLatitude, (), (override));
    MOCK_METHOD(double, UniversalLongitude, (), (override));
    MOCK_METHOD(double, Latitude, (), (const override));
    MOCK_METHOD(double, MinimumLatitude, (), (const override));
    MOCK_METHOD(double, MaximumLatitude, (), (const override));
    MOCK_METHOD(double, Longitude, (), (const override));
    MOCK_METHOD(double, MinimumLongitude, (), (const override));
    MOCK_METHOD(double, MaximumLongitude, (), (const override));
    MOCK_METHOD(double, WorldX, (), (const override));
    MOCK_METHOD(double, WorldY, (), (const override));
    MOCK_METHOD(bool, SetUniversalGround, (const double lat, const double lon), (override));
    // MOCK_METHOD(CameraType, GetCameraType, (), (const, override));
    MOCK_METHOD(QString, Name, (), (const override));
    MOCK_METHOD(QString, Version, (), (const override));
    // MOCK_METHOD(int, SpkReferenceId, (), (const override));
};

class MockProcessRubberSheet : public ProcessRubberSheet {
public:
  MOCK_METHOD(void, StartProcess, (Transform &trans, Interpolator &interp), (override));
  MOCK_METHOD(Cube*, SetOutputCube, (const QString &fname,
                                     const Isis::CubeAttributeOutput &att,
                                     const int ns, const int nl,
                                     const int nb), (override));
  MOCK_METHOD(void, SetInputCube, (Cube *inCube, int requirements), (override));
  MOCK_METHOD(void, processPatchTransform, (Transform &trans, Interpolator &interp), (override));
  MOCK_METHOD(void, setPatchParameters, (int startSample, int startLine, int samples,
                                         int lines, int sampleIncrement, int lineIncrement), (override));
  MOCK_METHOD(void, forceTile, (double Samp, double Line));
  MOCK_METHOD(void, SetTiling, (long long start, long long end));
  MOCK_METHOD(void, EndProcess, (), (override));
  MOCK_METHOD(void, BandChange, (void (*funct)(const int band)), (override));
};

class MockShapeModel : public ShapeModel {
  public:
    MOCK_METHOD(bool, intersectSurface, (std::vector<double> observerPos, std::vector<double> lookDirection));
    MOCK_METHOD(SurfacePoint*, surfaceIntersection,(), (const));
    MOCK_METHOD(bool, isDEM, (), (const));
    MOCK_METHOD(void, calculateLocalNormal, (QVector<double *> neighborPoints));
    MOCK_METHOD(void, calculateSurfaceNormal, ());
    MOCK_METHOD(std::vector<double>, normal, ());
    MOCK_METHOD(void, calculateDefaultNormal, ());
    MOCK_METHOD(Distance, localRadius, (const Latitude &lat, const Longitude &lon));
};


// Mock CSM Model class
class MockRasterGM : public csm::RasterGM {
  public:
    // csm::Model
    MOCK_METHOD(csm::Version, getVersion, (), (const, override));
    MOCK_METHOD(std::string, getModelName, (), (const, override));
    MOCK_METHOD(std::string, getPedigree, (), (const, override));
    MOCK_METHOD(std::string, getImageIdentifier, (), (const, override));
    MOCK_METHOD(void, setImageIdentifier, (const std::string&, csm::WarningList*), (override));
    MOCK_METHOD(std::string, getSensorIdentifier, (), (const, override));
    MOCK_METHOD(std::string, getPlatformIdentifier, (), (const, override));
    MOCK_METHOD(std::string, getCollectionIdentifier, (), (const, override));
    MOCK_METHOD(std::string, getTrajectoryIdentifier, (), (const, override));
    MOCK_METHOD(std::string, getSensorType, (), (const, override));
    MOCK_METHOD(std::string, getSensorMode, (), (const, override));
    MOCK_METHOD(std::string, getReferenceDateAndTime, (), (const, override));
    MOCK_METHOD(std::string, getModelState, (), (const, override));
    MOCK_METHOD(void, replaceModelState, (const std::string&), (override));
    // csm::GeometricModel methods
    MOCK_METHOD(csm::EcefCoord, getReferencePoint, (), (const, override));
    MOCK_METHOD(void, setReferencePoint, (const csm::EcefCoord&), (override));
    MOCK_METHOD(int, getNumParameters, (), (const, override));
    MOCK_METHOD(std::string, getParameterName, (int), (const, override));
    MOCK_METHOD(std::string, getParameterUnits, (int), (const, override));
    MOCK_METHOD(bool, hasShareableParameters, (), (const, override));
    MOCK_METHOD(bool, isParameterShareable, (int), (const, override));
    MOCK_METHOD(csm::SharingCriteria, getParameterSharingCriteria, (int), (const, override));
    MOCK_METHOD(double, getParameterValue, (int), (const, override));
    MOCK_METHOD(void, setParameterValue, (int, double), (override));
    MOCK_METHOD(csm::param::Type, getParameterType, (int), (const, override));
    MOCK_METHOD(void, setParameterType, (int, csm::param::Type), (override));
    MOCK_METHOD(double, getParameterCovariance, (int, int), (const, override));
    MOCK_METHOD(void, setParameterCovariance, (int, int, double), (override));
    MOCK_METHOD(int, getNumGeometricCorrectionSwitches, (), (const, override));
    MOCK_METHOD(std::string, getGeometricCorrectionName, (int), (const, override));
    MOCK_METHOD(void, setGeometricCorrectionSwitch, (int, bool, csm::param::Type), (override));
    MOCK_METHOD(bool, getGeometricCorrectionSwitch, (int), (const, override));
    MOCK_METHOD(std::vector<double>,
                getCrossCovarianceMatrix,
                (const csm::GeometricModel&, csm::param::Set, const csm::GeometricModel::GeometricModelList&),
                (const, override));
    // RasterGM methods
    MOCK_METHOD(csm::ImageCoord, groundToImage, (const csm::EcefCoord&, double, double*, csm::WarningList*), (const, override));
    MOCK_METHOD(csm::ImageCoordCovar,
                groundToImage,
                (const csm::EcefCoordCovar&, double, double*, csm::WarningList*),
                (const, override));
    MOCK_METHOD(csm::EcefCoord,
                imageToGround,
                (const csm::ImageCoord&, double, double, double*, csm::WarningList*),
                (const, override));
    MOCK_METHOD(csm::EcefCoordCovar,
                imageToGround,
                (const csm::ImageCoordCovar&, double, double, double, double*, csm::WarningList*),
                (const, override));
    MOCK_METHOD(csm::EcefLocus,
                imageToProximateImagingLocus,
                (const csm::ImageCoord&, const csm::EcefCoord&, double, double*, csm::WarningList*),
                (const, override));
    MOCK_METHOD(csm::EcefLocus,
                imageToRemoteImagingLocus,
                (const csm::ImageCoord&, double, double*, csm::WarningList*),
                (const, override));
    MOCK_METHOD(csm::ImageCoord, getImageStart, (), (const, override));
    MOCK_METHOD(csm::ImageVector, getImageSize, (), (const, override));
    MOCK_METHOD((std::pair<csm::ImageCoord, csm::ImageCoord>), getValidImageRange, (), (const, override));
    MOCK_METHOD((std::pair<double,double>), getValidHeightRange, (), (const, override));
    MOCK_METHOD(csm::EcefVector, getIlluminationDirection, (const csm::EcefCoord&), (const, override));
    MOCK_METHOD(double, getImageTime, (const csm::ImageCoord&), (const, override));
    MOCK_METHOD(csm::EcefCoord, getSensorPosition, (const csm::ImageCoord&), (const, override));
    MOCK_METHOD(csm::EcefCoord, getSensorPosition, (double), (const, override));
    MOCK_METHOD(csm::EcefVector, getSensorVelocity, (const csm::ImageCoord&), (const, override));
    MOCK_METHOD(csm::EcefVector, getSensorVelocity, (double), (const, override));
    MOCK_METHOD(csm::RasterGM::SensorPartials,
                computeSensorPartials,
                (int, const csm::EcefCoord&, double, double*, csm::WarningList*),
                (const, override));
    MOCK_METHOD(csm::RasterGM::SensorPartials,
                computeSensorPartials,
                (int, const csm::ImageCoord&, const csm::EcefCoord&, double, double*, csm::WarningList*),
                (const, override));
    MOCK_METHOD(std::vector<double>, computeGroundPartials, (const csm::EcefCoord&), (const, override));
    MOCK_METHOD(const csm::CorrelationModel&, getCorrelationModel, (), (const, override));
    MOCK_METHOD(std::vector<double>,
                getUnmodeledCrossCovariance,
                (const csm::ImageCoord&, const csm::ImageCoord&),
                (const, override));
};


#endif
