#include "CsmFixtures.h"

#include "csm/csm.h"

#include <QString>
#include <QUuid>
#include <QVector>

#include "csminit.h"

#include "Blob.h"
#include "FileName.h"
#include "MockCsmPlugin.h"
#include "PvlGroup.h"
#include "PvlKeyword.h"

namespace Isis {

  // Matches a CSM Image Coord for gMock
  ::testing::Matcher<const csm::ImageCoord&> MatchImageCoord(const csm::ImageCoord &expected) {
    return ::testing::AllOf(
        ::testing::Field(&csm::ImageCoord::line, ::testing::DoubleNear(expected.line, 0.0001)),
        ::testing::Field(&csm::ImageCoord::samp, ::testing::DoubleNear(expected.samp, 0.0001))
    );
}


  // Matches a CSM ECEF Coord for gMock
  ::testing::Matcher<const csm::EcefCoord&> MatchEcefCoord(const csm::EcefCoord &expected) {
    return ::testing::AllOf(
        ::testing::Field(&csm::EcefCoord::x, ::testing::DoubleNear(expected.x, 0.0001)),
        ::testing::Field(&csm::EcefCoord::y, ::testing::DoubleNear(expected.y, 0.0001)),
        ::testing::Field(&csm::EcefCoord::z, ::testing::DoubleNear(expected.z, 0.0001))
    );
  }

  void CSMCubeFixture::SetUp() {
    SmallCube::SetUp();

    // Instrument group
    // Just need a target name
    PvlGroup instGroup("Instrument");
    instGroup += PvlKeyword("TargetName", "TestTarget");
    instGroup += PvlKeyword("InstrumentId", "TestId");
    testCube->putGroup(instGroup);

    // Kernels group
    // Just need a shapemodel specified
    PvlGroup kernGroup("Kernels");
    kernGroup += PvlKeyword("ShapeModel", "Null");
    testCube->putGroup(kernGroup);

    // CSMInfo group
    // This just has to exist, but fill it out for completeness and incase it
    // ever does matter
    PvlGroup infoGroup("CsmInfo");
    infoGroup += PvlKeyword("CSMPlatformID", "TestPlatform");
    infoGroup += PvlKeyword("CSMInstrumentId", "TestInstrument");
    infoGroup += PvlKeyword("ReferenceTime", "2000-01-01T11:58:55.816"); // J2000 epoch

    PvlKeyword paramNames("ModelParameterNames");
    paramNames += "TestNoneParam";
    paramNames += "TestFictitiousParam";
    paramNames += "TestRealParam";
    paramNames += "TestFixedParam";
    PvlKeyword paramUnits("ModelParameterUnits");
    paramUnits += "unitless";
    paramUnits += "m";
    paramUnits += "rad";
    paramUnits += "lines/sec";
    PvlKeyword paramTypes("ModelParameterTypes");
    paramTypes += "NONE";
    paramTypes += "FICTITIOUS";
    paramTypes += "REAL";
    paramTypes += "FIXED";

    infoGroup += paramNames;
    infoGroup += paramUnits;
    infoGroup += paramTypes;

    testCube->putGroup(infoGroup);

    // Register the mock with our plugin
    std::string mockModelName = QUuid().toString().toStdString();
    MockCsmPlugin loadablePlugin;
    loadablePlugin.registerModel(mockModelName, &mockModel);

    // CSMState BLOB
    Blob csmStateBlob("CSMState", "String");
    csmStateBlob.setData(mockModelName.c_str(), mockModelName.size());
    csmStateBlob.Label() += PvlKeyword("ModelName", mockModelName);
    csmStateBlob.Label() += PvlKeyword("PluginName", loadablePlugin.getPluginName());
    testCube->write(csmStateBlob);
    filename = testCube->fileName();
    testCube->close();
    testCube->open(filename, "rw");
  }

  void CSMCameraFixture::SetUp() {
    CSMCubeFixture::SetUp();

    // Account for calls that happen while making a CSMCamera
    EXPECT_CALL(mockModel, getSensorIdentifier())
        .Times(2)
        .WillRepeatedly(::testing::Return("MockSensorID"));
    EXPECT_CALL(mockModel, getPlatformIdentifier())
        .Times(2)
        .WillRepeatedly(::testing::Return("MockPlatformID"));
    EXPECT_CALL(mockModel, getReferenceDateAndTime())
        .Times(1)
        .WillRepeatedly(::testing::Return("2000-01-01T11:58:55.816"));

    testCam = testCube->camera();
  }

  void CSMCameraSetFixture::SetUp() {
    CSMCameraFixture::SetUp();

    imagePt = csm::ImageCoord(4.5, 4.5);
    groundPt = csm::EcefCoord(wgs84.getSemiMajorRadius(), 0, 0);
    imageLocus = csm::EcefLocus(wgs84.getSemiMajorRadius() + 50000, 0, 0, -1, 0, 0);

    // Setup the mock for setImage and ensure it succeeds
    EXPECT_CALL(mockModel, imageToRemoteImagingLocus(MatchImageCoord(imagePt), ::testing::_, ::testing::_, ::testing::_))
        .Times(1)
        .WillOnce(::testing::Return(imageLocus));
    EXPECT_CALL(mockModel, getImageTime)
        .Times(1)
        .WillOnce(::testing::Return(10.0));

    ASSERT_TRUE(testCam->SetImage(5, 5)); // Assert here so that the test code doesn't run if the camera isn't set
  }

  void CSMCameraDemFixture::SetUp() {
    CSMCubeFixture::SetUp();

    // Record the demRadius at 0 lat, 0 lon
    demRadius = 3394200.43980104;

    // Update the shapemodel on the cube
    PvlGroup &kernGroup = testCube->group("Kernels");
    kernGroup.addKeyword(PvlKeyword("ShapeModel", "data/CSMCamera/mola_compressed_prep.cub"), Pvl::Replace);

    // Close and re-open the cube, then save off the new camera
    testCube->close();
    testCube->open(filename, "rw");

    // Account for calls that happen while making a CSMCamera
    EXPECT_CALL(mockModel, getSensorIdentifier())
        .Times(2)
        .WillRepeatedly(::testing::Return("MockSensorID"));
    EXPECT_CALL(mockModel, getPlatformIdentifier())
        .Times(2)
        .WillRepeatedly(::testing::Return("MockPlatformID"));
    EXPECT_CALL(mockModel, getReferenceDateAndTime())
        .Times(1)
        .WillRepeatedly(::testing::Return("2000-01-01T11:58:55.816"));

    testCam = testCube->camera();
  }

  void CSMNetwork::SetUp(){
    QString APP_XML = QString::fromStdString(FileName("$ISISROOT/bin/xml/csminit.xml").expanded());
    QVector<QString> fNames = {"/Test_A", "/Test_B",
                               "/Test_C", "/Test_D",
                               "/Test_E", "/Test_F",
                               "/Test_G", "/Test_H",
                               "/Test_I", "/Test_J"
                              };

    cubes.fill(nullptr, 10);

    cubeList = new FileList();
    cubeListFile = tempDir.path() + "/cubes.lis";
    // Create CSMInit-ed cubes
    for (int i = 0; i < cubes.size() ; i++){
      cubes[i] = new Cube();
      cubes[i]->setDimensions(1024,1024,1);
      FileName cubName = FileName(tempDir.path().toStdString()+fNames[i].toStdString()+".cub");
      cubes[i]->create(QString::fromStdString(cubName.expanded()));
      cubeList->append(cubes[i]->fileName().toStdString());
      QVector<QString> args = {"from="+QString::fromStdString(cubName.expanded()),
                               "state=data/CSMNetwork/"+fNames[i]+".json",
                               "modelname=TestCsmModel",
                               "pluginname=TestCsmPlugin"
                              };
      UserInterface ui(APP_XML, args);
      csminit(ui);
    }
    cubeList->write(cubeListFile.toStdString());
  }

  void CSMNetwork::TearDown() {
    for(int i = 0; i < cubes.size(); i++) {
      if(cubes[i] && cubes[i]->isOpen()) {
        delete cubes[i];
      }
    }

    if (cubeList) {
      delete cubeList;
    }
  }


}