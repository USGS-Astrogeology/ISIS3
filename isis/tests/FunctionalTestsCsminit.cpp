#include "csminit.h"

#include <QString>
#include <iostream>

#include "AlternativeTestCsmModel.h"
#include "Blob.h"
#include "TestCsmPlugin.h"
#include "TempFixtures.h"
#include "CameraFixtures.h"
#include "Table.h"
#include "TestUtilities.h"
#include "TestCsmModel.h"
#include "FileName.h"

#include <nlohmann/json.hpp>
using json = nlohmann::json;

#include "gmock/gmock.h"

using namespace Isis;

static QString APP_XML = FileName("$ISISROOT/bin/xml/csminit.xml").expanded();

class CSMPluginFixture : public TempTestingFiles {
  protected:
    const csm::Plugin *plugin;
    Cube *testCube;
    Pvl label;
    QString isdPath;
    QString altIsdPath;
    QString bodyRotationIsdPath;
    QString filename;
    TestCsmModel model;
    AlternativeTestCsmModel altModel;

    void SetUp() override {
      TempTestingFiles::SetUp();

      // Create and populate test ISDs
      json isd;
      isd["reference_time"] = 0;
      isd["center_latitude"] = 3.03125;
      isd["center_longitude"] = -2.9375;
      isd["scale"] = 240;
      isd["center_longitude_sigma"] = 0.0645181963189456;
      isd["center_latitude_sigma"] = 0.0645181963189456;
      isd["scale_sigma"] = 8.25832912882503;
      isdPath = tempDir.path() + "/default.json";
      std::ofstream file(isdPath.toStdString());
      file << isd;
      file.flush();

      json altIsd;
      altIsd["test_param_one"] = 1.0;
      altIsd["test_param_two"] = 2.0;
      altIsd["test_param_three"] = 3.0;
      altIsd["test_param_four"] = 4.0;

      altIsdPath = tempDir.path() + "/alternate.json";
      std::ofstream altFile(altIsdPath.toStdString());
      altFile << altIsd;
      altFile.flush();
      std::ifstream cubeLabel("data/threeImageNetwork/cube1.pvl");
      cubeLabel >> label;
      testCube = new Cube();
      filename = tempDir.path() + "/csminitCube.cub";
      testCube->fromLabel(filename, label, "rw");
      testCube->close();

      plugin = csm::Plugin::findPlugin(TestCsmPlugin::PLUGIN_NAME);
    }

    void TearDown() override {
      if (testCube) {
        if (testCube->isOpen()) {
          testCube->close();
        }
        delete testCube;
        testCube = NULL;
      }
    }
};

TEST_F(CSMPluginFixture, CSMInitDefault) {
  // Run csminit with defaults for everything besides FROM and ISD
  QVector<QString> args = {
    "from="+filename,
    "isd="+isdPath
  };

  UserInterface options(APP_XML, args);
  csminit(options);

  testCube->open(filename);

  // Get a model and a state string
  Blob stateString("CSMState", "String");
  testCube->read(stateString);

  // Verify contents of the Blob's PVL label
  PvlObject blobPvl = stateString.Label();

  // Check that the plugin can create a model from the state string
  std::string modelName = blobPvl.findKeyword("ModelName");
  std::string modelState(stateString.getBuffer(), stateString.Size());
  EXPECT_TRUE(plugin->canModelBeConstructedFromState(modelName, modelState));

  // Check blob label ModelName and Plugin Name
  EXPECT_EQ(blobPvl.findKeyword("PluginName"), plugin->getPluginName());
  EXPECT_EQ(modelName, TestCsmModel::SENSOR_MODEL_NAME);

  // Check the Instrument group
  ASSERT_TRUE(testCube->hasGroup("Instrument"));
  PvlGroup &instGroup = testCube->group("Instrument");
  EXPECT_TRUE(instGroup.hasKeyword("TargetName"));

  // Check the CsmInfo group
  ASSERT_TRUE(testCube->hasGroup("CsmInfo"));
  PvlGroup &infoGroup = testCube->group("CsmInfo");
  ASSERT_TRUE(infoGroup.hasKeyword("CSMPlatformID"));
  EXPECT_EQ(infoGroup["CSMPlatformID"][0], model.getPlatformIdentifier());
  ASSERT_TRUE(infoGroup.hasKeyword("CSMInstrumentId"));
  EXPECT_EQ(infoGroup["CSMInstrumentId"][0], model.getSensorIdentifier());
  ASSERT_TRUE(infoGroup.hasKeyword("ReferenceTime"));
  EXPECT_EQ(infoGroup["ReferenceTime"][0], model.getReferenceDateAndTime());
  ASSERT_TRUE(infoGroup.hasKeyword("ModelParameterNames"));
  ASSERT_EQ(infoGroup["ModelParameterNames"].size(), 3);
  EXPECT_EQ(infoGroup["ModelParameterNames"][0], TestCsmModel::PARAM_NAMES[0]);
  EXPECT_EQ(infoGroup["ModelParameterNames"][1], TestCsmModel::PARAM_NAMES[1]);
  ASSERT_TRUE(infoGroup.hasKeyword("ModelParameterUnits"));
  ASSERT_EQ(infoGroup["ModelParameterUnits"].size(), 3);
  EXPECT_EQ(infoGroup["ModelParameterUnits"][0], TestCsmModel::PARAM_UNITS[0]);
  EXPECT_EQ(infoGroup["ModelParameterUnits"][1], TestCsmModel::PARAM_UNITS[1]);
  ASSERT_TRUE(infoGroup.hasKeyword("ModelParameterTypes"));
  ASSERT_EQ(infoGroup["ModelParameterTypes"].size(), 3);
  EXPECT_EQ(infoGroup["ModelParameterTypes"][0], "REAL");
  EXPECT_EQ(infoGroup["ModelParameterTypes"][1], "REAL");

  // Check the Kernels group
  ASSERT_TRUE(testCube->hasGroup("Kernels"));
  PvlGroup &kernGroup = testCube->group("Kernels");
  EXPECT_TRUE(kernGroup.hasKeyword("ShapeModel"));

  // Check that the BodyRotation table does not exist
  ASSERT_FALSE(testCube->hasTable("BodyRotation"));
}

TEST_F(CSMPluginFixture, CSMInitRunTwice) {
  // Run csminit twice in a row. Verify that end result is as if csminit were
  // only run once with the second arguments.
  QVector<QString> args = {
    "from="+filename,
    "isd="+isdPath};

  UserInterface options(APP_XML, args);

  csminit(options);

  QVector<QString> altArgs = {
    "from="+filename,
    "isd="+altIsdPath,
    "modelName="+QString::fromStdString(AlternativeTestCsmModel::SENSOR_MODEL_NAME)};

  UserInterface altOptions(APP_XML, altArgs);

  csminit(altOptions);

  testCube->open(filename);

  Blob stateString("CSMState", "String");
  testCube->read(stateString);
  PvlObject blobPvl = stateString.Label();

  // Check blob label ModelName and Plugin Name
  EXPECT_EQ(blobPvl.findKeyword("PluginName"), plugin->getPluginName());
  EXPECT_EQ(blobPvl.findKeyword("ModelName"), AlternativeTestCsmModel::SENSOR_MODEL_NAME);

  // Check that the plugin can create a model from the state string
  std::string modelName = blobPvl.findKeyword("ModelName");
  std::string modelState(stateString.getBuffer(), stateString.Size());
  EXPECT_TRUE(plugin->canModelBeConstructedFromState(modelName, modelState));

  // Make sure there is only one CSMState Blob on the label
  PvlObject *label = testCube->label();
  label->deleteObject("String");
  EXPECT_FALSE(label->hasObject("String"));

  // Check that there is only one CsmInfo group
  ASSERT_TRUE(testCube->hasGroup("CsmInfo"));
  testCube->deleteGroup("CsmInfo");
  ASSERT_FALSE(testCube->hasGroup("CsmInfo"));

  // Check that there is only one ShapeModel
  ASSERT_TRUE(testCube->hasGroup("Kernels"));
  PvlGroup &kernGroup = testCube->group("Kernels");
  EXPECT_TRUE(kernGroup.hasKeyword("ShapeModel"));
  kernGroup.deleteKeyword("ShapeModel");
  EXPECT_FALSE(kernGroup.hasKeyword("ShapeModel"));
}

TEST_F(CSMPluginFixture, CSMInitMultiplePossibleModels) {
  // Test csminit when multiple possible models can be created. First, verify that this will fail
  // without specifying the MODELNAME, then specify the MODELNAME and check the results.

  QVector<QString> args = {
    "from="+filename,
    "isd="+altIsdPath};

  UserInterface options(APP_XML, args);

  // If there are two possible models, csminit will fail and prompt the user to specify the model
  // and/or plugin name.
  try {
    csminit(options);
  }
  catch (IException &e) {
    EXPECT_THAT(e.what(), testing::HasSubstr("Multiple models can be created from the ISD"));
  }

  // Re-run with the model name specified
  args = {
    "from="+filename,
    "isd="+altIsdPath,
    "modelName="+QString::fromStdString(AlternativeTestCsmModel::SENSOR_MODEL_NAME)};

  UserInterface betterOptions(APP_XML, args);
  csminit(betterOptions);

  testCube->open(filename);
  Blob stateString("CSMState", "String");
  testCube->read(stateString);
  PvlObject blobPvl = stateString.Label();

  // Check that the plugin can create a model from the state string
  std::string modelName = blobPvl.findKeyword("ModelName");
  std::string modelState(stateString.getBuffer(), stateString.Size());
  EXPECT_TRUE(plugin->canModelBeConstructedFromState(modelName, modelState));

  // check blob label ModelName and Plugin Name
  EXPECT_EQ(blobPvl.findKeyword("PluginName"), plugin->getPluginName());
  EXPECT_EQ(blobPvl.findKeyword("ModelName"), AlternativeTestCsmModel::SENSOR_MODEL_NAME);

  // Check the CsmInfo group
  ASSERT_TRUE(testCube->hasGroup("CsmInfo"));
  PvlGroup &infoGroup = testCube->group("CsmInfo");
  ASSERT_TRUE(infoGroup.hasKeyword("CSMPlatformID"));
  EXPECT_EQ(infoGroup["CSMPlatformID"][0], altModel.getPlatformIdentifier());
  ASSERT_TRUE(infoGroup.hasKeyword("CSMInstrumentId"));
  EXPECT_EQ(infoGroup["CSMInstrumentId"][0], altModel.getSensorIdentifier());
  ASSERT_TRUE(infoGroup.hasKeyword("ReferenceTime"));
  EXPECT_EQ(infoGroup["ReferenceTime"][0], altModel.getReferenceDateAndTime());
  ASSERT_TRUE(infoGroup.hasKeyword("ModelParameterNames"));
  ASSERT_EQ(infoGroup["ModelParameterNames"].size(), 3);
  EXPECT_EQ(infoGroup["ModelParameterNames"][0], AlternativeTestCsmModel::PARAM_NAMES[0]);
  EXPECT_EQ(infoGroup["ModelParameterNames"][1], AlternativeTestCsmModel::PARAM_NAMES[1]);
  EXPECT_EQ(infoGroup["ModelParameterNames"][2], AlternativeTestCsmModel::PARAM_NAMES[2]);
  ASSERT_TRUE(infoGroup.hasKeyword("ModelParameterUnits"));
  ASSERT_EQ(infoGroup["ModelParameterUnits"].size(), 3);
  EXPECT_EQ(infoGroup["ModelParameterUnits"][0], AlternativeTestCsmModel::PARAM_UNITS[0]);
  EXPECT_EQ(infoGroup["ModelParameterUnits"][1], AlternativeTestCsmModel::PARAM_UNITS[1]);
  EXPECT_EQ(infoGroup["ModelParameterUnits"][2], AlternativeTestCsmModel::PARAM_UNITS[2]);
  ASSERT_TRUE(infoGroup.hasKeyword("ModelParameterTypes"));
  ASSERT_EQ(infoGroup["ModelParameterTypes"].size(), 3);
  EXPECT_EQ(infoGroup["ModelParameterTypes"][0], "FICTITIOUS");
  EXPECT_EQ(infoGroup["ModelParameterTypes"][1], "REAL");
  EXPECT_EQ(infoGroup["ModelParameterTypes"][2], "FIXED");
}


TEST_F(CSMPluginFixture, CSMInitFails) {
  // Create an ISD that will result in no successful models
  json isd;
  isd["name"] = "failing_isd";
  isd["test_param_one"] = "value_one";
  isd["test_param_does_not_exist"] = "failing_value";

  isdPath = tempDir.path() + "/failing.json";
  std::ofstream file(isdPath.toStdString());
  file << isd;
  file.flush();

  QVector<QString> args = {
    "from="+filename,
    "isd="+isdPath};

  UserInterface options(APP_XML, args);

  // Expect a failure due to being unable to construct any model from the isd
  try {
    csminit(options);
  }
  catch (IException &e) {
    EXPECT_THAT(e.what(), testing::HasSubstr("No loaded model could be created from the ISD"));
  }
}


// This test uses the DefaultCube fixture because it has already has attached
// spice data that csminit should remove.
TEST_F(DefaultCube, CSMInitSpiceCleanup) {
  // Create an ISD
  json isd;
  isd["reference_time"] = 0;
  isd["center_latitude"] = 3.03125;
  isd["center_longitude"] = -2.9375;
  isd["scale"] = 240;
  isd["center_longitude_sigma"] = 0.0645181963189456;
  isd["center_latitude_sigma"] = 0.0645181963189456;
  isd["scale_sigma"] = 8.25832912882503;
  QString isdPath = tempDir.path() + "/default.json";
  std::ofstream file(isdPath.toStdString());
  file << isd;
  file.flush();

  QVector<QString> args = {
    "from="+testCube->fileName(),
    "isd="+isdPath};
  QString cubeFile = testCube->fileName();

  UserInterface options(APP_XML, args);

  testCube->close();
  csminit(options);
  Cube outputCube(cubeFile);

  EXPECT_FALSE(outputCube.hasTable("InstrumentPointing"));
  EXPECT_FALSE(outputCube.hasTable("InstrumentPosition"));
  EXPECT_FALSE(outputCube.hasTable("BodyRotation"));
  EXPECT_FALSE(outputCube.hasTable("SunPosition"));
  EXPECT_FALSE(outputCube.hasTable("CameraStatistics"));
  ASSERT_TRUE(outputCube.hasGroup("Kernels"));
  EXPECT_EQ(outputCube.group("Kernels").keywords(), 2);
}


// This test uses the DefaultCube fixture because it has already been spiceinit'd
TEST_F(DefaultCube, CSMInitSpiceRestoredAfterFailure) {
  // Create an ISD
  json isd;
  isd["test_param_one"] = 1.0;
  isd["test_param_two"] = 2.0;

  QString isdPath = tempDir.path() + "/default.json";
  std::ofstream file(isdPath.toStdString());
  file << isd;
  file.flush();

  QVector<QString> args = {
    "from="+testCube->fileName(),
    "isd="+isdPath,
    "shape=fake.broken"};

  QString cubeFile = testCube->fileName();

  PvlGroup kernelsGroup = testCube->group("Kernels");
  PvlGroup instrumentGroup = testCube->group("Instrument");

  UserInterface options(APP_XML, args);

  testCube->close();

  ASSERT_ANY_THROW(csminit(options));

  Cube outputCube(cubeFile);
  ASSERT_NO_THROW(outputCube.camera());

  EXPECT_TRUE(outputCube.hasTable("InstrumentPointing"));
  EXPECT_TRUE(outputCube.hasTable("InstrumentPosition"));
  EXPECT_TRUE(outputCube.hasTable("BodyRotation"));
  EXPECT_TRUE(outputCube.hasTable("SunPosition"));
  ASSERT_TRUE(outputCube.hasGroup("Kernels"));
  ASSERT_TRUE(outputCube.hasGroup("Instrument"));
  EXPECT_PRED_FORMAT2(AssertPvlGroupEqual, kernelsGroup, outputCube.group("Kernels"));
  EXPECT_PRED_FORMAT2(AssertPvlGroupEqual, instrumentGroup, outputCube.group("Instrument"));
}


// This test uses the DefaultCube fixture because it has already has attached
// spice data that csminit should not remove on a failure.
TEST_F(DefaultCube, CSMInitSpiceNoCleanup) {
  // Create an ISD that will result in no successful models
  json isd;
  isd["test_param_one"] = "value_one";
  isd["test_param_does_not_exist"] = "failing_value";

  QString isdPath = tempDir.path() + "/default.json";
  std::ofstream file(isdPath.toStdString());
  file << isd;
  file.flush();

  QVector<QString> args = {
    "from="+testCube->fileName(),
    "isd="+isdPath};
  QString cubeFile = testCube->fileName();

  UserInterface options(APP_XML, args);

  testCube->close();
  // Expect a failure due to being unable to construct any model from the isd
  EXPECT_ANY_THROW(csminit(options));
  Cube outputCube(cubeFile);

  // The cube should still be intact and we should still be able to get a camera
  EXPECT_NO_THROW(outputCube.camera());
}

TEST_F(CSMPluginFixture, CSMInitStateStringFails) {
  // Create an state string that will result in no successful models
  json state;
  state["name"] = "failing_isd";
  state["test_param_one"] = "value_one";
  state["test_param_does_not_exist"] = "failing_value";

  QString statePath = tempDir.path() + "/failing.json";
  std::ofstream file(statePath.toStdString());
  file << state;
  file.flush();

  QVector<QString> args = {
    "from="+filename,
    "state="+statePath};

  UserInterface options(APP_XML, args);

  // Expect a failure due to missing modelname and pluginname
  try {
    csminit(options);
  }
  catch (IException &e) {
    EXPECT_THAT(e.what(), testing::HasSubstr("When using a State string, PLUGINNAME and MODELNAME must be specified"));
  }

  QVector<QString> argsWithModel = {
    "from="+filename,
    "state="+statePath,
    "modelname=TestCsmModel",
    "pluginname=TestCsmPlugin"};

  UserInterface optionsWithModel(APP_XML, argsWithModel);

  // Expect a failure due to a bad state string.
  try {
    csminit(optionsWithModel);
  }
  catch (IException &e) {
    EXPECT_THAT(e.what(), testing::HasSubstr("Could not construct sensor model using STATE string and MODELNAME"));
  }

  QVector<QString> argsWithIsdAndState = {
    "from="+filename,
    "isd=fakePath",
    "state="+statePath,
    "modelname=TestCsmModel",
    "pluginname=TestCsmPlugin"};

  UserInterface optionsWithIsdAndState(APP_XML, argsWithIsdAndState);

  // Expect a failure due to providing both an ISD and a STATE string
  try {
    csminit(optionsWithIsdAndState);
  }
  catch (IException &e) {
    EXPECT_THAT(e.what(), testing::HasSubstr("Cannot enter both [ISD] and [STATE]. Please enter either [ISD] or [STATE]"));
  }
}

TEST_F(CSMPluginFixture, CSMInitWithState) {
  // Run csminit with ISD
  QVector<QString> args = {
    "from="+filename,
    "isd="+isdPath
  };

  UserInterface options(APP_XML, args);
  csminit(options);

  testCube->open(filename);

  // Read blob off csminited cube, get state string and save off to compare
  Blob state("CSMState", "String");
  testCube->read(state);
  testCube->close();

  // Write the state out to a file
  std::string stateBefore(state.getBuffer(), state.Size());
  QString statePath = tempDir.path() + "/state.json";
  std::ofstream stateFile(statePath.toStdString());
  stateFile << stateBefore;
  stateFile.flush();

  // Run csminit with state just saved to file
  QVector<QString> argsState = {
    "from="+filename,
    "state="+statePath,
    "modelname=TestCsmModel",
    "pluginname=TestCsmPlugin"};

  UserInterface optionsState(APP_XML, argsState);
  csminit(optionsState);

  // Pull state string off. if ending state string = original state string these are functionally equiv.
  testCube->open(filename);

  Blob stateBlobAfter("CSMState", "String");
  testCube->read(stateBlobAfter);
  std::string stateAfter(stateBlobAfter.getBuffer(), stateBlobAfter.Size());
  EXPECT_EQ(stateBefore, stateAfter);
}
