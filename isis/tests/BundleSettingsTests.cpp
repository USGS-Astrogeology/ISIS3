#include "BundleSettings.h"

#include <algorithm>

#include <QBuffer>
#include <QByteArray>
#include <QString>
#include <QList>
#include <QPair>
#include <QDomDocument>
#include <QXmlStreamWriter>

#include "BundleObservationSolveSettings.h"
#include "BundleTargetBody.h"
#include "IException.h"
#include "MaximumLikelihoodWFunctions.h"

#include "gmock/gmock.h"

bool observationSettingsComparison(
      BundleObservationSolveSettings m,
      BundleObservationSolveSettings n
) {
  return (m.instrumentId() == n.instrumentId());
}

::testing::AssertionResult assertObservationSettingsEqual(
      const char* m_expr,
      const char* n_expr,
      BundleObservationSolveSettings m,
      BundleObservationSolveSettings n
) {
  if (observationSettingsComparison(m, n)) return ::testing::AssertionSuccess();

  return ::testing::AssertionFailure() << m_expr << " and " << n_expr
        << " are different because they have different InstrumentIds ("
        << m.instrumentId().toStdString() << " and "
        << n.instrumentId().toStdString() << ")";
}

QDomDocument saveToQDomDocument(BundleSettings &settings) {

  QString outputString;
  QXmlStreamWriter outputWriter(&outputString);
  settings.save(outputWriter, NULL);
  QDomDocument settingsDoc("settings_doc");
  settingsDoc.setContent(outputString);

  return settingsDoc;
}

class BundleSettings_ObservationTest : public ::testing::Test {
  protected:
    BundleSettings testSettings;
    QList<BundleObservationSolveSettings> optionsList;
    QString firstInstrument;
    QString secondInstrument;
    QString firstObservationNumber;
    QString secondObservationNumber;

    void SetUp() override {
      firstInstrument = "First Instrument";
      secondInstrument = "Second Instrument";
      firstObservationNumber = "First Observation";
      secondObservationNumber = "Second Observation";
      BundleObservationSolveSettings firstObsSettings;
      BundleObservationSolveSettings secondObsSettings;
      firstObsSettings.setInstrumentId(firstInstrument);
      secondObsSettings.setInstrumentId(secondInstrument);
      firstObsSettings.addObservationNumber(firstObservationNumber);
      secondObsSettings.addObservationNumber(secondObservationNumber);
      optionsList = {firstObsSettings, secondObsSettings};
      testSettings.setObservationSolveOptions(optionsList);
   }
};

class BundleSettings_NotDefault : public ::testing::Test {
  protected:
    BundleSettings testSettings;

    void SetUp() override {
      BundleSettings testSettings;
      testSettings.setSolveOptions(
            true, true, true, true,
            SurfacePoint::Rectangular, SurfacePoint::Rectangular,
            0.1,
            0.25,
            0.3);
      testSettings.setValidateNetwork(true);
      testSettings.setOutlierRejection(true, 5.0);
      testSettings.setCreateInverseMatrix(true);
      QList<BundleObservationSolveSettings> emptySolveSettings;
      testSettings.setObservationSolveOptions(emptySolveSettings);
      testSettings.setConvergenceCriteria(
            BundleSettings::ParameterCorrections,
            10,
            5);
      testSettings.addMaximumLikelihoodEstimatorModel(
            MaximumLikelihoodWFunctions::Huber,
            75.0);
      BundleTargetBodyQsp testTarget(new BundleTargetBody);
      testSettings.setBundleTargetBody(testTarget);
      testSettings.setOutputFilePrefix("test/path");
   }
};

class MockBundleTargetBody : public BundleTargetBody {
  public:
    MOCK_METHOD0(numberParameters, int());
    MOCK_METHOD0(solvePoleRA, bool());
    MOCK_METHOD0(solvePoleRAVelocity, bool());
    MOCK_METHOD0(solvePoleDec, bool());
    MOCK_METHOD0(solvePoleDecVelocity, bool());
    MOCK_METHOD0(solvePM, bool());
    MOCK_METHOD0(solvePMVelocity, bool());
    MOCK_METHOD0(solvePMAcceleration, bool());
    MOCK_METHOD0(solveTriaxialRadii, bool());
    MOCK_METHOD0(solveMeanRadius, bool());
};

class BoolTest : public ::testing::TestWithParam<bool> {
  // Intentionally empty
};

class CoordinateTypeTest : public ::testing::TestWithParam<SurfacePoint::CoordinateType> {
  // Intentionally empty
};

class ConvergenceCriteriaTest : public ::testing::TestWithParam<BundleSettings::ConvergenceCriteria> {
  // Intentionally empty
};

TEST(BundleSettings, DefaultConstructor) {
  BundleSettings testSettings;

  EXPECT_TRUE(testSettings.validateNetwork());

  EXPECT_FALSE(testSettings.createInverseMatrix());
  EXPECT_FALSE(testSettings.solveObservationMode());
  EXPECT_FALSE(testSettings.solveRadius());
  EXPECT_FALSE(testSettings.updateCubeLabel());
  EXPECT_FALSE(testSettings.errorPropagation());
  EXPECT_FALSE(testSettings.outlierRejection());

  EXPECT_EQ(3.0, testSettings.outlierRejectionMultiplier());

  EXPECT_EQ(Isis::Null, testSettings.globalPointCoord1AprioriSigma());
  EXPECT_EQ(Isis::Null, testSettings.globalPointCoord2AprioriSigma());
  EXPECT_EQ(Isis::Null, testSettings.globalPointCoord3AprioriSigma());

  EXPECT_EQ(BundleSettings::Sigma0, testSettings.convergenceCriteria());
  EXPECT_EQ(1.0e-10, testSettings.convergenceCriteriaThreshold());
  EXPECT_EQ(50, testSettings.convergenceCriteriaMaximumIterations());

  EXPECT_TRUE(testSettings.maximumLikelihoodEstimatorModels().isEmpty());

  EXPECT_FALSE(testSettings.solveTargetBody());
  EXPECT_FALSE(testSettings.solvePoleRA());
  EXPECT_FALSE(testSettings.solvePoleRAVelocity());
  EXPECT_FALSE(testSettings.solvePoleDec());
  EXPECT_FALSE(testSettings.solvePoleDecVelocity());
  EXPECT_FALSE(testSettings.solvePM());
  EXPECT_FALSE(testSettings.solvePMVelocity());
  EXPECT_FALSE(testSettings.solvePMAcceleration());
  EXPECT_FALSE(testSettings.solveTargetBody());
  EXPECT_FALSE(testSettings.solveMeanRadius());
  EXPECT_EQ(0, testSettings.numberTargetBodyParameters());

  EXPECT_EQ(1, testSettings.numberSolveSettings());

  EXPECT_EQ(SurfacePoint::Latitudinal, testSettings.controlPointCoordTypeReports());
  EXPECT_EQ(SurfacePoint::Latitudinal, testSettings.controlPointCoordTypeBundle());

  EXPECT_EQ("", testSettings.outputFilePrefix());
}

TEST_F(BundleSettings_NotDefault, CopyConstructor) {
  BundleSettings copySettings(testSettings);

  EXPECT_EQ(testSettings.validateNetwork(), copySettings.validateNetwork());

  EXPECT_EQ(testSettings.createInverseMatrix(), copySettings.createInverseMatrix());
  EXPECT_EQ(testSettings.solveObservationMode(), copySettings.solveObservationMode());
  EXPECT_EQ(testSettings.solveRadius(), copySettings.solveRadius());
  EXPECT_EQ(testSettings.updateCubeLabel(), copySettings.updateCubeLabel());
  EXPECT_EQ(testSettings.errorPropagation(), copySettings.errorPropagation());
  EXPECT_EQ(testSettings.outlierRejection(), copySettings.outlierRejection());

  EXPECT_EQ(testSettings.outlierRejectionMultiplier(), copySettings.outlierRejectionMultiplier());

  EXPECT_EQ(testSettings.globalPointCoord1AprioriSigma(), copySettings.globalPointCoord1AprioriSigma());
  EXPECT_EQ(testSettings.globalPointCoord2AprioriSigma(), copySettings.globalPointCoord2AprioriSigma());
  EXPECT_EQ(testSettings.globalPointCoord3AprioriSigma(), copySettings.globalPointCoord3AprioriSigma());

  EXPECT_EQ(testSettings.convergenceCriteria(), copySettings.convergenceCriteria());
  EXPECT_EQ(testSettings.convergenceCriteriaThreshold(), copySettings.convergenceCriteriaThreshold());
  EXPECT_EQ(testSettings.convergenceCriteriaMaximumIterations(), copySettings.convergenceCriteriaMaximumIterations());

  EXPECT_EQ(testSettings.maximumLikelihoodEstimatorModels(), copySettings.maximumLikelihoodEstimatorModels());

  EXPECT_EQ(testSettings.numberSolveSettings(), copySettings.numberSolveSettings());

  EXPECT_EQ(testSettings.controlPointCoordTypeReports(), copySettings.controlPointCoordTypeReports());
  EXPECT_EQ(testSettings.controlPointCoordTypeBundle(), copySettings.controlPointCoordTypeBundle());

  EXPECT_EQ(testSettings.outputFilePrefix(), copySettings.outputFilePrefix());

  EXPECT_EQ(testSettings.bundleTargetBody(), copySettings.bundleTargetBody());
}

TEST_F(BundleSettings_NotDefault, Assignment) {
  BundleSettings assignedSettings;
  assignedSettings = testSettings;

  EXPECT_EQ(testSettings.validateNetwork(), assignedSettings.validateNetwork());

  EXPECT_EQ(testSettings.createInverseMatrix(), assignedSettings.createInverseMatrix());
  EXPECT_EQ(testSettings.solveObservationMode(), assignedSettings.solveObservationMode());
  EXPECT_EQ(testSettings.solveRadius(), assignedSettings.solveRadius());
  EXPECT_EQ(testSettings.updateCubeLabel(), assignedSettings.updateCubeLabel());
  EXPECT_EQ(testSettings.errorPropagation(), assignedSettings.errorPropagation());
  EXPECT_EQ(testSettings.outlierRejection(), assignedSettings.outlierRejection());

  EXPECT_EQ(testSettings.outlierRejectionMultiplier(), assignedSettings.outlierRejectionMultiplier());

  EXPECT_EQ(testSettings.globalPointCoord1AprioriSigma(), assignedSettings.globalPointCoord1AprioriSigma());
  EXPECT_EQ(testSettings.globalPointCoord2AprioriSigma(), assignedSettings.globalPointCoord2AprioriSigma());
  EXPECT_EQ(testSettings.globalPointCoord3AprioriSigma(), assignedSettings.globalPointCoord3AprioriSigma());

  EXPECT_EQ(testSettings.convergenceCriteria(), assignedSettings.convergenceCriteria());
  EXPECT_EQ(testSettings.convergenceCriteriaThreshold(), assignedSettings.convergenceCriteriaThreshold());
  EXPECT_EQ(testSettings.convergenceCriteriaMaximumIterations(), assignedSettings.convergenceCriteriaMaximumIterations());

  EXPECT_EQ(testSettings.maximumLikelihoodEstimatorModels(), assignedSettings.maximumLikelihoodEstimatorModels());

  EXPECT_EQ(testSettings.numberSolveSettings(), assignedSettings.numberSolveSettings());

  EXPECT_EQ(testSettings.controlPointCoordTypeReports(), assignedSettings.controlPointCoordTypeReports());
  EXPECT_EQ(testSettings.controlPointCoordTypeBundle(), assignedSettings.controlPointCoordTypeBundle());

  EXPECT_EQ(testSettings.outputFilePrefix(), assignedSettings.outputFilePrefix());

  EXPECT_EQ(testSettings.bundleTargetBody(), assignedSettings.bundleTargetBody());
}

TEST_P(BoolTest, validateNetwork) {
  BundleSettings testSettings;
  testSettings.setValidateNetwork(GetParam());
  EXPECT_EQ(GetParam(), testSettings.validateNetwork());
}

TEST_P(BoolTest, saveValidateNetwork) {
  BundleSettings testSettings;
  testSettings.setValidateNetwork(GetParam());

  QDomDocument settingsDoc = saveToQDomDocument(testSettings);
  QDomElement root = settingsDoc.documentElement();

  QDomElement globalSettings = root.firstChildElement("globalSettings");
  ASSERT_FALSE(globalSettings.isNull());

  QDomElement validateNetwork = globalSettings.firstChildElement("validateNetwork");
  ASSERT_FALSE(validateNetwork.isNull());
  EXPECT_EQ("validateNetwork", validateNetwork.tagName());
  EXPECT_EQ(QString::number(testSettings.validateNetwork()), validateNetwork.text());
}

TEST_P(BoolTest, outlierRejection) {
  BundleSettings testSettings;
  testSettings.setOutlierRejection(GetParam());
  EXPECT_EQ(GetParam(), testSettings.outlierRejection());
}

TEST_P(BoolTest, saveOutlierRejection) {
  BundleSettings testSettings;
  testSettings.setOutlierRejection(GetParam());

  QDomDocument settingsDoc = saveToQDomDocument(testSettings);
  QDomElement root = settingsDoc.documentElement();

  QDomElement globalSettings = root.firstChildElement("globalSettings");
  ASSERT_FALSE(globalSettings.isNull());

  QDomElement outlierRejectionOptions = globalSettings.firstChildElement("outlierRejectionOptions");
  ASSERT_FALSE(outlierRejectionOptions.isNull());
  QDomNamedNodeMap outlierRejectionOptionsAtts = outlierRejectionOptions.attributes();
  EXPECT_EQ(
        QString::number(testSettings.outlierRejection()),
        outlierRejectionOptionsAtts.namedItem("rejection").nodeValue()
  );
  EXPECT_EQ(
        (testSettings.outlierRejection() ?
            QString::number(testSettings.outlierRejectionMultiplier()) : "N/A"),
        outlierRejectionOptionsAtts.namedItem("multiplier").nodeValue()
  );
}

TEST_P(BoolTest, inverseMatrix) {
  BundleSettings testSettings;
  testSettings.setSolveOptions(
        testSettings.solveObservationMode(),
        testSettings.updateCubeLabel(),
        true,
        testSettings.solveRadius()
  );
  testSettings.setCreateInverseMatrix(GetParam());
  EXPECT_EQ(GetParam(), testSettings.createInverseMatrix());
}

TEST_P(BoolTest, setBoolSolveOptions) {
  BundleSettings testSettings;
  testSettings.setSolveOptions(
        GetParam(),
        GetParam(),
        GetParam(),
        GetParam()
  );
  EXPECT_EQ(GetParam(), testSettings.solveObservationMode());
  EXPECT_EQ(GetParam(), testSettings.updateCubeLabel());
  EXPECT_EQ(GetParam(), testSettings.errorPropagation());
  EXPECT_EQ(GetParam(), testSettings.solveRadius());
}

TEST_P(BoolTest, saveSolveOptions) {
  BundleSettings testSettings;
  testSettings.setSolveOptions(
        GetParam(),
        GetParam(),
        GetParam(),
        GetParam()
  );
  testSettings.setCreateInverseMatrix(GetParam());

  QDomDocument settingsDoc = saveToQDomDocument(testSettings);
  QDomElement root = settingsDoc.documentElement();

  QDomElement globalSettings = root.firstChildElement("globalSettings");
  ASSERT_FALSE(globalSettings.isNull());

  QDomElement solveOptions = globalSettings.firstChildElement("solveOptions");
  ASSERT_FALSE(solveOptions.isNull());
  QDomNamedNodeMap solveOptionAtts = solveOptions.attributes();
  EXPECT_EQ(
        QString::number(testSettings.solveObservationMode()),
        solveOptionAtts.namedItem("solveObservationMode").nodeValue()
  );
  EXPECT_EQ(
        QString::number(testSettings.solveRadius()),
        solveOptionAtts.namedItem("solveRadius").nodeValue()
  );
  EXPECT_EQ(
        QString::number(testSettings.updateCubeLabel()),
        solveOptionAtts.namedItem("updateCubeLabel").nodeValue()
  );
  EXPECT_EQ(
        QString::number(testSettings.errorPropagation()),
        solveOptionAtts.namedItem("errorPropagation").nodeValue()
  );
  EXPECT_EQ(
        QString::number(testSettings.createInverseMatrix()),
        solveOptionAtts.namedItem("createInverseMatrix").nodeValue()
  );
}

INSTANTIATE_TEST_SUITE_P(
      BundleSettings,
      BoolTest,
      ::testing::Bool()
);

TEST_P(CoordinateTypeTest, setCoordinateTypeSolveOptions) {
  BundleSettings testSettings;
  testSettings.setSolveOptions(
        testSettings.solveObservationMode(),
        testSettings.updateCubeLabel(),
        testSettings.errorPropagation(),
        testSettings.solveRadius(),
        GetParam(),
        GetParam()
  );
  EXPECT_EQ(GetParam(), testSettings.controlPointCoordTypeReports());
  EXPECT_EQ(GetParam(), testSettings.controlPointCoordTypeBundle());
}

TEST_P(BoolTest, saveCoordinateTypes) {
  BundleSettings testSettings;
  testSettings.setSolveOptions(
        GetParam(),
        GetParam(),
        GetParam(),
        GetParam()
  );
  testSettings.setCreateInverseMatrix(GetParam());

  QDomDocument settingsDoc = saveToQDomDocument(testSettings);
  QDomElement root = settingsDoc.documentElement();

  QDomElement globalSettings = root.firstChildElement("globalSettings");
  ASSERT_FALSE(globalSettings.isNull());

  QDomElement solveOptions = globalSettings.firstChildElement("solveOptions");
  ASSERT_FALSE(solveOptions.isNull());
  QDomNamedNodeMap solveOptionAtts = solveOptions.attributes();
  EXPECT_EQ(
        QString::number(testSettings.controlPointCoordTypeReports()),
        solveOptionAtts.namedItem("controlPointCoordTypeReports").nodeValue()
  );
  EXPECT_EQ(
        QString::number(testSettings.controlPointCoordTypeBundle()),
        solveOptionAtts.namedItem("controlPointCoordTypeBundle").nodeValue()
  );
}

INSTANTIATE_TEST_SUITE_P(
      BundleSettings,
      CoordinateTypeTest,
      ::testing::Values(SurfacePoint::Latitudinal, SurfacePoint::Rectangular)
);

TEST(BundleSettings, setGlobalSigmas) {
  BundleSettings testSettings;
  testSettings.setSolveOptions(
        testSettings.solveObservationMode(),
        testSettings.updateCubeLabel(),
        testSettings.errorPropagation(),
        true,
        testSettings.controlPointCoordTypeBundle(),
        testSettings.controlPointCoordTypeReports(),
        2.0,
        8.0,
        32.0
  );

  EXPECT_EQ(2.0, testSettings.globalPointCoord1AprioriSigma());
  EXPECT_EQ(8.0, testSettings.globalPointCoord2AprioriSigma());
  EXPECT_EQ(32.0, testSettings.globalPointCoord3AprioriSigma());
}

TEST(BundleSettings, setBadGlobalSigmas) {
  BundleSettings testSettings;
  testSettings.setSolveOptions(
        testSettings.solveObservationMode(),
        testSettings.updateCubeLabel(),
        testSettings.errorPropagation(),
        true,
        testSettings.controlPointCoordTypeBundle(),
        testSettings.controlPointCoordTypeReports(),
        -2.0,
        -8.0,
        -32.0
  );

  EXPECT_EQ(Isis::Null, testSettings.globalPointCoord1AprioriSigma());
  EXPECT_EQ(Isis::Null, testSettings.globalPointCoord2AprioriSigma());
  EXPECT_EQ(Isis::Null, testSettings.globalPointCoord3AprioriSigma());
}

TEST(BundleSettings, setGlobalSigmasNoRadius) {
  BundleSettings testSettings;
  testSettings.setSolveOptions(
        testSettings.solveObservationMode(),
        testSettings.updateCubeLabel(),
        testSettings.errorPropagation(),
        false,
        testSettings.controlPointCoordTypeBundle(),
        testSettings.controlPointCoordTypeReports(),
        Isis::Null,
        Isis::Null,
        32.0
  );

  EXPECT_EQ(Isis::Null, testSettings.globalPointCoord3AprioriSigma());
}

TEST(BundleSettings, saveGlobalSigmas) {
  BundleSettings testSettings;
  testSettings.setSolveOptions(
        testSettings.solveObservationMode(),
        testSettings.updateCubeLabel(),
        testSettings.errorPropagation(),
        true,
        testSettings.controlPointCoordTypeBundle(),
        testSettings.controlPointCoordTypeReports(),
        2.0,
        8.0,
        32.0
  );

  QDomDocument settingsDoc = saveToQDomDocument(testSettings);
  QDomElement root = settingsDoc.documentElement();

  QDomElement globalSettings = root.firstChildElement("globalSettings");
  ASSERT_FALSE(globalSettings.isNull());

  QDomElement aprioriSigmas = globalSettings.firstChildElement("aprioriSigmas");
  ASSERT_FALSE(aprioriSigmas.isNull());
  QDomNamedNodeMap aprioriSigmasAtts = aprioriSigmas.attributes();
  EXPECT_EQ(
        QString::number(testSettings.globalPointCoord1AprioriSigma()),
        aprioriSigmasAtts.namedItem("pointCoord1").nodeValue()
  );
  EXPECT_EQ(
        QString::number(testSettings.globalPointCoord2AprioriSigma()),
        aprioriSigmasAtts.namedItem("pointCoord2").nodeValue()
  );
  EXPECT_EQ(
        QString::number(testSettings.globalPointCoord3AprioriSigma()),
        aprioriSigmasAtts.namedItem("pointCoord3").nodeValue()
  );
}

TEST(BundleSettings, saveBadGlobalSigmas) {
  BundleSettings testSettings;
  testSettings.setSolveOptions(
        testSettings.solveObservationMode(),
        testSettings.updateCubeLabel(),
        testSettings.errorPropagation(),
        true,
        testSettings.controlPointCoordTypeBundle(),
        testSettings.controlPointCoordTypeReports(),
        -2.0,
        -8.0,
        -32.0
  );

  QDomDocument settingsDoc = saveToQDomDocument(testSettings);
  QDomElement root = settingsDoc.documentElement();

  QDomElement globalSettings = root.firstChildElement("globalSettings");
  ASSERT_FALSE(globalSettings.isNull());

  QDomElement aprioriSigmas = globalSettings.firstChildElement("aprioriSigmas");
  ASSERT_FALSE(aprioriSigmas.isNull());
  QDomNamedNodeMap aprioriSigmasAtts = aprioriSigmas.attributes();
  EXPECT_EQ(
        "N/A",
        aprioriSigmasAtts.namedItem("pointCoord1").nodeValue()
  );
  EXPECT_EQ(
        "N/A",
        aprioriSigmasAtts.namedItem("pointCoord2").nodeValue()
  );
  EXPECT_EQ(
        "N/A",
        aprioriSigmasAtts.namedItem("pointCoord3").nodeValue()
  );
}

TEST(BundleSettings, outlierRejectionMultiplier) {
  BundleSettings testSettings;
  testSettings.setOutlierRejection(true, 8.0);
  EXPECT_EQ(8.0, testSettings.outlierRejectionMultiplier());
}

TEST_F(BundleSettings_ObservationTest, observationSolveSettings) {
  EXPECT_EQ(testSettings.numberSolveSettings(), optionsList.size());
  EXPECT_TRUE(std::equal(
        optionsList.begin(),
        optionsList.end(),
        testSettings.observationSolveSettings().begin(),
        observationSettingsComparison)
  );
  EXPECT_PRED_FORMAT2(
        assertObservationSettingsEqual,
        testSettings.observationSolveSettings(secondObservationNumber),
        optionsList[1]
  );
  EXPECT_PRED_FORMAT2(
        assertObservationSettingsEqual,
        testSettings.observationSolveSettings(1),
        optionsList[1]
  );
}

TEST_F(BundleSettings_ObservationTest, saveObservationSolveSettings) {
  QDomDocument settingsDoc = saveToQDomDocument(testSettings);
  QDomElement root = settingsDoc.documentElement();
  QDomElement observationSolveSettingsList = root.firstChildElement("observationSolveSettingsList");
  ASSERT_FALSE(observationSolveSettingsList.isNull());
  EXPECT_EQ(testSettings.numberSolveSettings(), observationSolveSettingsList.childNodes().size());
}

TEST_P(ConvergenceCriteriaTest, convergenceCriteriaStrings) {
  QString criteriaString = BundleSettings::convergenceCriteriaToString(GetParam());
  BundleSettings::ConvergenceCriteria criteria =
        BundleSettings::stringToConvergenceCriteria(criteriaString);
  EXPECT_EQ(GetParam(), criteria);
}

TEST_P(ConvergenceCriteriaTest, convergenceCriteria) {
  BundleSettings testSettings;
  testSettings.setConvergenceCriteria(
        GetParam(),
        2.0,
        50
  );
  EXPECT_EQ(GetParam(), testSettings.convergenceCriteria());
  EXPECT_EQ(2.0, testSettings.convergenceCriteriaThreshold());
  EXPECT_EQ(50, testSettings.convergenceCriteriaMaximumIterations());
}

TEST_P(ConvergenceCriteriaTest, saveConvergenceCriteria) {
  BundleSettings testSettings;
  testSettings.setConvergenceCriteria(
        GetParam(),
        2.0,
        50
  );
  testSettings.setCreateInverseMatrix(GetParam());

  QDomDocument settingsDoc = saveToQDomDocument(testSettings);
  QDomElement root = settingsDoc.documentElement();

  QDomElement globalSettings = root.firstChildElement("globalSettings");
  ASSERT_FALSE(globalSettings.isNull());

  QDomElement convergenceCriteriaOptions = globalSettings.firstChildElement("convergenceCriteriaOptions");
  ASSERT_FALSE(convergenceCriteriaOptions.isNull());
  QDomNamedNodeMap convergenceCriteriaOptionsAtts = convergenceCriteriaOptions.attributes();
  EXPECT_EQ(
        BundleSettings::convergenceCriteriaToString(testSettings.convergenceCriteria()),
        convergenceCriteriaOptionsAtts.namedItem("convergenceCriteria").nodeValue()
  );
  EXPECT_EQ(
        QString::number(testSettings.convergenceCriteriaThreshold()),
        convergenceCriteriaOptionsAtts.namedItem("threshold").nodeValue()
  );
  EXPECT_EQ(
        QString::number(testSettings.convergenceCriteriaMaximumIterations()),
        convergenceCriteriaOptionsAtts.namedItem("maximumIterations").nodeValue()
  );
}

INSTANTIATE_TEST_SUITE_P(
      BundleSettings,
      ConvergenceCriteriaTest,
      ::testing::Values(BundleSettings::Sigma0, BundleSettings::ParameterCorrections)
);

TEST(BundleSettings, maximumLikelihoodHuber) {
  BundleSettings testSettings;
  testSettings.addMaximumLikelihoodEstimatorModel(
        MaximumLikelihoodWFunctions::Huber,
        64.0
  );
  QList< QPair< MaximumLikelihoodWFunctions::Model, double > > functions =
      testSettings.maximumLikelihoodEstimatorModels();
  ASSERT_EQ(1, functions.size());
  EXPECT_EQ(MaximumLikelihoodWFunctions::Huber, functions.front().first);
  EXPECT_EQ(64.0, functions.front().second);
}

TEST(BundleSettings, maximumLikelihoodHuberModified) {
  BundleSettings testSettings;
  testSettings.addMaximumLikelihoodEstimatorModel(
        MaximumLikelihoodWFunctions::HuberModified,
        64.0
  );
  QList< QPair< MaximumLikelihoodWFunctions::Model, double > > functions =
      testSettings.maximumLikelihoodEstimatorModels();
  ASSERT_EQ(1, functions.size());
  EXPECT_EQ(MaximumLikelihoodWFunctions::HuberModified, functions.front().first);
  EXPECT_EQ(64.0, functions.front().second);
}

// I am not sure why this throws an exceptions. Unforunately, the people who wrote
// this are now gone and no one knows if they should throw or not without doing
// more research. For now, we are testing existing functionality - JAM 2018/11/13
TEST(BundleSettings, maximumLikelihoodWelsch) {
  BundleSettings testSettings;
  try {
    testSettings.addMaximumLikelihoodEstimatorModel(
          MaximumLikelihoodWFunctions::Welsch,
          64.0
    );
    FAIL() << "Expected an exception";
  }
  catch (IException &e) {
    EXPECT_THAT(
           e.toString(),
          ::testing::HasSubstr("the first model must be of type HUBER or HUBER_MODIFIED")
    );
  }
  catch (...) {
    FAIL() << "Expected an ISIS exception";
  }
}

// I am not sure why this throws an exceptions. Unforunately, the people who wrote
// this are now gone and no one knows if they should throw or not without doing
// more research. For now, we are testing existing functionality - JAM 2018/11/13
TEST(BundleSettings, maximumLikelihoodChen) {
  BundleSettings testSettings;
  try {
    testSettings.addMaximumLikelihoodEstimatorModel(
          MaximumLikelihoodWFunctions::Chen,
          64.0
    );
    FAIL() << "Expected an exception";
  }
  catch (IException &e) {
    EXPECT_THAT(
           e.toString(),
          ::testing::HasSubstr("the first model must be of type HUBER or HUBER_MODIFIED")
    );
  }
  catch (...) {
    FAIL() << "Expected an ISIS exception";
  }
}

TEST(BundleSettings, multipleMaximumLikelihoodModels) {
  BundleSettings testSettings;
  testSettings.addMaximumLikelihoodEstimatorModel(
        MaximumLikelihoodWFunctions::Huber,
        64.0
  );
  testSettings.addMaximumLikelihoodEstimatorModel(
        MaximumLikelihoodWFunctions::HuberModified,
        32.0
  );
  testSettings.addMaximumLikelihoodEstimatorModel(
        MaximumLikelihoodWFunctions::Welsch,
        16.0
  );
  testSettings.addMaximumLikelihoodEstimatorModel(
        MaximumLikelihoodWFunctions::Chen,
        8.0
  );
  QList< QPair< MaximumLikelihoodWFunctions::Model, double > > functions =
      testSettings.maximumLikelihoodEstimatorModels();
  ASSERT_EQ(4, functions.size());
  EXPECT_EQ(MaximumLikelihoodWFunctions::Huber, functions[0].first);
  EXPECT_EQ(64.0, functions[0].second);
  EXPECT_EQ(MaximumLikelihoodWFunctions::HuberModified, functions[1].first);
  EXPECT_EQ(32.0, functions[1].second);
  EXPECT_EQ(MaximumLikelihoodWFunctions::Welsch, functions[2].first);
  EXPECT_EQ(16.0, functions[2].second);
  EXPECT_EQ(MaximumLikelihoodWFunctions::Chen, functions[3].first);
  EXPECT_EQ(8.0, functions[3].second);
}

TEST(BundleSettings, saveMaximumLikelyhoodModels) {
  BundleSettings testSettings;
  testSettings.addMaximumLikelihoodEstimatorModel(
        MaximumLikelihoodWFunctions::Huber,
        64.0
  );
  testSettings.addMaximumLikelihoodEstimatorModel(
        MaximumLikelihoodWFunctions::HuberModified,
        32.0
  );
  testSettings.addMaximumLikelihoodEstimatorModel(
        MaximumLikelihoodWFunctions::Welsch,
        16.0
  );
  testSettings.addMaximumLikelihoodEstimatorModel(
        MaximumLikelihoodWFunctions::Chen,
        8.0
  );
  QList< QPair< MaximumLikelihoodWFunctions::Model, double > > functions =
      testSettings.maximumLikelihoodEstimatorModels();

  QDomDocument settingsDoc = saveToQDomDocument(testSettings);
  QDomElement root = settingsDoc.documentElement();

  QDomElement globalSettings = root.firstChildElement("globalSettings");
  ASSERT_FALSE(globalSettings.isNull());

  QDomElement maximumLikelihoodEstimation = globalSettings.firstChildElement("maximumLikelihoodEstimation");
  ASSERT_FALSE(maximumLikelihoodEstimation.isNull());

  QDomNodeList modelNodes = maximumLikelihoodEstimation.childNodes();
  ASSERT_EQ(functions.size(), modelNodes.size());
  for(int modelIndex = 0; modelIndex < functions.size(); modelIndex++) {
    QDomNode modelElement = modelNodes.at(modelIndex);
    EXPECT_EQ("model", modelElement.nodeName());
    QDomNamedNodeMap modelAtts = modelElement.attributes();
    EXPECT_EQ(
          MaximumLikelihoodWFunctions::modelToString(functions[modelIndex].first),
          modelAtts.namedItem("type").nodeValue()
    );
    EXPECT_EQ(
          QString::number(functions[modelIndex].second),
          modelAtts.namedItem("quantile").nodeValue()
    );
  }
}

TEST(BundleSettings, outputFilePrefix) {
  BundleSettings testSettings;
  QString testPrefix("test/file/prefix");
  testSettings.setOutputFilePrefix(testPrefix);
  EXPECT_EQ(testPrefix, testSettings.outputFilePrefix());
}

TEST(BundleSettings, SaveOutputFilePrefix) {
  BundleSettings testSettings;
  QString testPrefix("test/file/prefix");
  testSettings.setOutputFilePrefix(testPrefix);

  QDomDocument settingsDoc = saveToQDomDocument(testSettings);
  QDomElement root = settingsDoc.documentElement();

  QDomElement globalSettings = root.firstChildElement("globalSettings");
  ASSERT_FALSE(globalSettings.isNull());

  QDomElement outputFileOptions = globalSettings.firstChildElement("outputFileOptions");
  ASSERT_FALSE(outputFileOptions.isNull());
  QDomNamedNodeMap outputFileOptionsAtts = outputFileOptions.attributes();
  EXPECT_EQ(
        testSettings.outputFilePrefix(),
        outputFileOptionsAtts.namedItem("fileNamePrefix").nodeValue()
  );
}

TEST(BundleSettings, setBundleTargetBody) {
  BundleTargetBodyQsp testTarget(new BundleTargetBody);
  BundleSettings testSettings;
  testSettings.setBundleTargetBody(testTarget);
  EXPECT_EQ(testTarget, testSettings.bundleTargetBody());
}

TEST(BundleSettings, BundleTargetBodyAccesors) {
  MockBundleTargetBody *mockBody = new MockBundleTargetBody;
  EXPECT_CALL(*mockBody, numberParameters())
        .Times(3)
        .WillOnce(::testing::Return(0))
        .WillRepeatedly(::testing::Return(5));
  EXPECT_CALL(*mockBody, solvePoleRA())
        .Times(1)
        .WillOnce(::testing::Return(true));
  EXPECT_CALL(*mockBody, solvePoleRAVelocity())
        .Times(1)
        .WillOnce(::testing::Return(true));
  EXPECT_CALL(*mockBody, solvePoleDec())
        .Times(1)
        .WillOnce(::testing::Return(true));
  EXPECT_CALL(*mockBody, solvePoleDecVelocity())
        .Times(1)
        .WillOnce(::testing::Return(true));
  EXPECT_CALL(*mockBody, solvePM())
        .Times(1)
        .WillOnce(::testing::Return(true));
  EXPECT_CALL(*mockBody, solvePMVelocity())
        .Times(1)
        .WillOnce(::testing::Return(true));
  EXPECT_CALL(*mockBody, solvePMAcceleration())
        .Times(1)
        .WillOnce(::testing::Return(true));
  EXPECT_CALL(*mockBody, solveTriaxialRadii())
        .Times(1)
        .WillOnce(::testing::Return(true));
  EXPECT_CALL(*mockBody, solveMeanRadius())
        .Times(1)
        .WillOnce(::testing::Return(true));

  BundleSettings testSettings;
  BundleTargetBodyQsp testTarget(mockBody);
  testSettings.setBundleTargetBody(testTarget);
  EXPECT_FALSE(testSettings.solveTargetBody());
  EXPECT_TRUE(testSettings.solveTargetBody());
  EXPECT_EQ(5, testSettings.numberTargetBodyParameters());
  EXPECT_TRUE(testSettings.solvePoleRA());
  EXPECT_TRUE(testSettings.solvePoleRAVelocity());
  EXPECT_TRUE(testSettings.solvePoleDec());
  EXPECT_TRUE(testSettings.solvePoleDecVelocity());
  EXPECT_TRUE(testSettings.solvePM());
  EXPECT_TRUE(testSettings.solvePMVelocity());
  EXPECT_TRUE(testSettings.solvePMAcceleration());
  EXPECT_TRUE(testSettings.solveTriaxialRadii());
  EXPECT_TRUE(testSettings.solveMeanRadius());
}
