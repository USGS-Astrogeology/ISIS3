#include <gtest/gtest.h>

#include "BundleObservationSolveSettings.h"
#include "TestUtilities.h"
#include "PvlGroup.h"
#include "PvlKeyword.h"
#include <QPair>
#include <QDomDocument>
#include <QDomElement>
#include <QXmlStreamWriter>
#include "SpecialPixel.h"

using namespace Isis;

QDomDocument saveToQDomDocument(BundleObservationSolveSettings &boss) {

  QString outputString;
  QXmlStreamWriter outputWriter(&outputString);
  boss.save(outputWriter, NULL);
  QDomDocument settingsDoc("settings_doc");
  settingsDoc.setContent(outputString);

  return settingsDoc;
}


class CSMSolveOptionStrings : public testing::TestWithParam<QPair<
  BundleObservationSolveSettings::CSMSolveOption, QString>> {
};
class CSMSolveSetStrings : public testing::TestWithParam<QPair<
  csm::param::Set, QString>> {
};
class CSMSolveTypeStrings : public testing::TestWithParam<QPair<
  csm::param::Type, QString>> {
};
class PointingSolveOptionStrings : public testing::TestWithParam<QPair<
  BundleObservationSolveSettings::InstrumentPointingSolveOption, QString>> {
};
class PositionSolveOptionStrings : public testing::TestWithParam<QPair<
  BundleObservationSolveSettings::InstrumentPositionSolveOption, QString>> {
};

TEST(BundleObservationSolveSettings, DefaultConstructor) {
  BundleObservationSolveSettings boss;

  EXPECT_PRED_FORMAT2(AssertQStringsEqual, boss.instrumentId(), "");
  EXPECT_EQ(boss.csmSolveOption(), BundleObservationSolveSettings::NoCSMParameters);
  EXPECT_EQ(boss.csmParameterSet(), csm::param::ADJUSTABLE);
  EXPECT_EQ(boss.csmParameterType(), csm::param::REAL);
  EXPECT_TRUE(boss.csmParameterList().empty());
  EXPECT_EQ(boss.instrumentPointingSolveOption(), BundleObservationSolveSettings::AnglesOnly);
  EXPECT_EQ(boss.ckDegree(), 2);
  EXPECT_EQ(boss.ckSolveDegree(), 2);
  EXPECT_EQ(boss.solveTwist(), true);
  EXPECT_EQ(boss.solvePolyOverPointing(), false);
  EXPECT_EQ(boss.numberCameraAngleCoefficientsSolved(), 1);
  EXPECT_EQ(boss.aprioriPointingSigmas().at(0), Isis::Null);
  EXPECT_EQ(boss.pointingInterpolationType(), SpiceRotation::PolyFunction);
  EXPECT_EQ(boss.instrumentPositionSolveOption(), BundleObservationSolveSettings::NoPositionFactors);
  EXPECT_EQ(boss.spkDegree(), 2);
  EXPECT_EQ(boss.spkSolveDegree(), 2);
  EXPECT_EQ(boss.numberCameraPositionCoefficientsSolved(), 0);
  EXPECT_EQ(boss.positionInterpolationType(), SpiceRotation::PolyFunction);
  EXPECT_TRUE(boss.aprioriPositionSigmas().isEmpty());
}

TEST(BundleObservationSolveSettings, CopyConstructor) {
  BundleObservationSolveSettings boss;
  BundleObservationSolveSettings copied(boss);

  EXPECT_PRED_FORMAT2(AssertQStringsEqual, copied.instrumentId(), "");
  EXPECT_EQ(copied.csmSolveOption(), BundleObservationSolveSettings::NoCSMParameters);
  EXPECT_EQ(copied.csmParameterSet(), csm::param::ADJUSTABLE);
  EXPECT_EQ(copied.csmParameterType(), csm::param::REAL);
  EXPECT_TRUE(copied.csmParameterList().empty());
  EXPECT_EQ(copied.instrumentPointingSolveOption(), BundleObservationSolveSettings::AnglesOnly);
  EXPECT_EQ(copied.ckDegree(), 2);
  EXPECT_EQ(copied.ckSolveDegree(), 2);
  EXPECT_EQ(copied.solveTwist(), true);
  EXPECT_EQ(copied.solvePolyOverPointing(), false);
  EXPECT_EQ(copied.numberCameraAngleCoefficientsSolved(), 1);
  EXPECT_EQ(copied.aprioriPointingSigmas().at(0), Isis::Null);
  EXPECT_EQ(copied.pointingInterpolationType(), SpiceRotation::PolyFunction);
  EXPECT_EQ(copied.instrumentPositionSolveOption(), BundleObservationSolveSettings::NoPositionFactors);
  EXPECT_EQ(copied.spkDegree(), 2);
  EXPECT_EQ(copied.spkSolveDegree(), 2);
  EXPECT_EQ(copied.numberCameraPositionCoefficientsSolved(), 0);
  EXPECT_EQ(copied.positionInterpolationType(), SpiceRotation::PolyFunction);
  EXPECT_TRUE(copied.aprioriPositionSigmas().isEmpty());
}

TEST(BundleObservationSolveSettings, PvlGroupConstructor) {
  PvlGroup settingsGroup("VO1/VISA");
  PvlKeyword camsolve("CamSolve");
  PvlKeyword twist("Twist");
  PvlKeyword spsolve("SPSolve");
  PvlKeyword csmsolveset("CSMSOLVESET");
  camsolve = "Angles";
  twist = "yes";
  spsolve = "None";
  csmsolveset = "ADJUSTABLE";
  settingsGroup += camsolve;
  settingsGroup += twist;
  settingsGroup += spsolve;
  settingsGroup += csmsolveset;

  BundleObservationSolveSettings boss(settingsGroup);

  EXPECT_PRED_FORMAT2(AssertQStringsEqual, boss.instrumentId(), "VO1/VISA");
  EXPECT_EQ(boss.ckSolveDegree(), 2);
  EXPECT_EQ(boss.instrumentPointingSolveOption(), BundleObservationSolveSettings::AnglesOnly);
  EXPECT_EQ(boss.numberCameraAngleCoefficientsSolved(), 1);
  EXPECT_FALSE(boss.solvePolyOverPointing());
  EXPECT_EQ(boss.pointingInterpolationType(), SpiceRotation::PolyFunction);
  EXPECT_EQ(boss.spkDegree(), 2);
  EXPECT_EQ(boss.spkSolveDegree(), 2);
  EXPECT_EQ(boss.instrumentPositionSolveOption(), BundleObservationSolveSettings::NoPositionFactors);
  EXPECT_EQ(boss.numberCameraPositionCoefficientsSolved(), 0);
  EXPECT_TRUE(boss.solveTwist());
  EXPECT_FALSE(boss.solvePositionOverHermite());
  EXPECT_EQ(boss.positionInterpolationType(), SpiceRotation::PolyFunction);
  EXPECT_TRUE(boss.aprioriPositionSigmas().isEmpty());
  EXPECT_EQ(boss.csmSolveOption(), BundleObservationSolveSettings::Set);
  EXPECT_EQ(boss.csmParameterSet(), csm::param::ADJUSTABLE);
}

TEST(BundleObservationSolveSettings, PvlGroupCSMTypeConstructor) {
  PvlGroup settingsGroup("VO1/VISA");
  PvlKeyword camsolve("CamSolve");
  PvlKeyword spsolve("SPSolve");
  PvlKeyword csmsolvetype("CSMSOLVETYPE");
  camsolve = "None";
  spsolve = "None";
  csmsolvetype = "REAL";
  settingsGroup += camsolve;
  settingsGroup += spsolve;
  settingsGroup += csmsolvetype;

  BundleObservationSolveSettings boss(settingsGroup);

  EXPECT_EQ(boss.csmSolveOption(), BundleObservationSolveSettings::Type);
  EXPECT_EQ(boss.csmParameterType(), csm::param::REAL);
}

TEST(BundleObservationSolveSettings, PvlGroupCSMListConstructor) {
  PvlGroup settingsGroup("VO1/VISA");
  PvlKeyword camsolve("CamSolve");
  PvlKeyword spsolve("SPSolve");
  PvlKeyword csmsolvelist("CSMSOLVELIST");
  camsolve = "None";
  spsolve = "None";
  csmsolvelist += "Param 1";
  csmsolvelist += "Param 2";
  settingsGroup += camsolve;
  settingsGroup += spsolve;
  settingsGroup += csmsolvelist;

  BundleObservationSolveSettings boss(settingsGroup);

  EXPECT_EQ(boss.csmSolveOption(), BundleObservationSolveSettings::List);
  QStringList csmParamList = boss.csmParameterList();
  ASSERT_EQ(csmParamList.size(), 2);
  EXPECT_EQ(csmParamList[0].toStdString(), "Param 1");
  EXPECT_EQ(csmParamList[1].toStdString(), "Param 2");
}

TEST(BundleObservationSolveSettings, AssignmentOperator) {
  BundleObservationSolveSettings boss;
  BundleObservationSolveSettings copied = boss;

  EXPECT_PRED_FORMAT2(AssertQStringsEqual, copied.instrumentId(), "");
  EXPECT_EQ(copied.instrumentPointingSolveOption(), BundleObservationSolveSettings::AnglesOnly);
  EXPECT_EQ(copied.ckDegree(), 2);
  EXPECT_EQ(copied.ckSolveDegree(), 2);
  EXPECT_TRUE(copied.solveTwist());
  EXPECT_FALSE(copied.solvePolyOverPointing());
  EXPECT_EQ(copied.numberCameraAngleCoefficientsSolved(), 1);
  EXPECT_EQ(copied.aprioriPointingSigmas().at(0), Isis::Null);
  EXPECT_EQ(copied.pointingInterpolationType(), SpiceRotation::PolyFunction);
  EXPECT_EQ(copied.instrumentPositionSolveOption(), BundleObservationSolveSettings::NoPositionFactors);
  EXPECT_EQ(copied.spkDegree(), 2);
  EXPECT_EQ(copied.spkSolveDegree(), 2);
  EXPECT_EQ(copied.numberCameraPositionCoefficientsSolved(), 0);
  EXPECT_EQ(copied.positionInterpolationType(), SpiceRotation::PolyFunction);
  EXPECT_TRUE(copied.aprioriPositionSigmas().isEmpty());
}

TEST(BundleObservationSolveSettings, InstrumentId) {
  BundleObservationSolveSettings boss;

  boss.setInstrumentId("MRO/CTX");
  EXPECT_PRED_FORMAT2(AssertQStringsEqual, boss.instrumentId(), "MRO/CTX");
}

TEST(BundleObservationSolveSettings, ObservationNumbers) {
  BundleObservationSolveSettings boss;
  EXPECT_TRUE(boss.observationNumbers().isEmpty());

  boss.addObservationNumber("123");
  boss.addObservationNumber("456");
  EXPECT_EQ(boss.observationNumbers().size(), 2);
  EXPECT_TRUE(boss.observationNumbers().contains("123"));
  EXPECT_TRUE(boss.observationNumbers().contains("456"));

  boss.removeObservationNumber("123");
  EXPECT_EQ(boss.observationNumbers().size(), 1);
  EXPECT_FALSE(boss.observationNumbers().contains("123"));
  EXPECT_TRUE(boss.observationNumbers().contains("456"));
}

TEST(BundleObservationSolveSettings, SetInstrumentPointingSettingsAllCoeffs) {
  BundleObservationSolveSettings boss;
  QList<double> additionalPointingSigmas = {4};
  boss.setInstrumentPointingSettings(BundleObservationSolveSettings::AllPointingCoefficients,
                                     true, 3, 3, false, 1, 2, 3, &additionalPointingSigmas);

  EXPECT_EQ(boss.numberCameraAngleCoefficientsSolved(), 4);
  ASSERT_EQ(boss.aprioriPointingSigmas().size(), 4);
  EXPECT_EQ(boss.aprioriPointingSigmas().at(0), 1);
  EXPECT_EQ(boss.aprioriPointingSigmas().at(1), 2);
  EXPECT_EQ(boss.aprioriPointingSigmas().at(2), 3);
  EXPECT_EQ(boss.aprioriPointingSigmas().at(3), 4);
  EXPECT_EQ(boss.pointingInterpolationType(), SpiceRotation::PolyFunction);
}

TEST(BundleObservationSolveSettings, SetInstrumentPointingSettingsNoFactors) {
  BundleObservationSolveSettings boss;
  boss.setInstrumentPointingSettings(BundleObservationSolveSettings::NoPointingFactors,
                                     true, 2, 2, true);

  EXPECT_EQ(boss.numberCameraAngleCoefficientsSolved(), 0);
  EXPECT_TRUE(boss.aprioriPointingSigmas().isEmpty());
  EXPECT_EQ(boss.pointingInterpolationType(), SpiceRotation::PolyFunctionOverSpice);
}

TEST(BundleObservationSolveSettings, SetInstrumentPointingSettingsAnglesOnly) {
  BundleObservationSolveSettings boss;
  boss.setInstrumentPointingSettings(BundleObservationSolveSettings::AnglesOnly, true);

  EXPECT_EQ(boss.numberCameraAngleCoefficientsSolved(), 1);
  ASSERT_EQ(boss.aprioriPointingSigmas().size(), 1);
  EXPECT_EQ(boss.aprioriPointingSigmas().at(0), Isis::Null);
}

TEST(BundleObservationSolveSettings, SetInstrumentPointingSettingsAnglesVelocity) {
  BundleObservationSolveSettings boss;
  boss.setInstrumentPointingSettings(BundleObservationSolveSettings::AnglesVelocity,
                                     true);

  EXPECT_EQ(boss.numberCameraAngleCoefficientsSolved(), 2);
  ASSERT_EQ(boss.aprioriPointingSigmas().size(), 2);
  EXPECT_EQ(boss.aprioriPointingSigmas().at(0), Isis::Null);
  EXPECT_EQ(boss.aprioriPointingSigmas().at(1), Isis::Null);
}

TEST(BundleObservationSolveSettings, SetInstrumentPointingSettingsAnglesVelocityAcceleration) {
  BundleObservationSolveSettings boss;
  boss.setInstrumentPointingSettings(BundleObservationSolveSettings::AnglesVelocityAcceleration,
                                     true);

  EXPECT_EQ(boss.numberCameraAngleCoefficientsSolved(), 3);
  ASSERT_EQ(boss.aprioriPointingSigmas().size(), 3);
  EXPECT_EQ(boss.aprioriPointingSigmas().at(0), Isis::Null);
  EXPECT_EQ(boss.aprioriPointingSigmas().at(1), Isis::Null);
  EXPECT_EQ(boss.aprioriPointingSigmas().at(2), Isis::Null);
}

TEST(BundleObservationSolveSettings, setInstrumentPositionSettingsAllCoefficients) {
  BundleObservationSolveSettings boss;
  QList<double> additionalPositionSigmas = {4};

  boss.setInstrumentPositionSettings(BundleObservationSolveSettings::AllPositionCoefficients,
                                     3, 3, false, 1, 2, 3, &additionalPositionSigmas);

  EXPECT_EQ(boss.numberCameraPositionCoefficientsSolved(), 4);
  ASSERT_EQ(boss.aprioriPositionSigmas().size(), 4);
  EXPECT_EQ(boss.aprioriPositionSigmas().at(0), 1);
  EXPECT_EQ(boss.aprioriPositionSigmas().at(1), 2);
  EXPECT_EQ(boss.aprioriPositionSigmas().at(2), 3);
  EXPECT_EQ(boss.aprioriPositionSigmas().at(3), 4);
  EXPECT_EQ(boss.positionInterpolationType(), SpiceRotation::PolyFunction);
}

TEST(BundleObservationSolveSettings, setInstrumentPositionSettingsNoFactors) {
  BundleObservationSolveSettings boss;
  boss.setInstrumentPositionSettings(BundleObservationSolveSettings::NoPositionFactors,
                                     2, 2, true);

  EXPECT_EQ(boss.numberCameraPositionCoefficientsSolved(), 0);
  EXPECT_TRUE(boss.aprioriPositionSigmas().isEmpty());
  EXPECT_EQ(boss.positionInterpolationType(), SpiceRotation::PolyFunctionOverSpice);
}

TEST(BundleObservationSolveSettings, setInstrumentPositionSettingsPositionOnly) {
  BundleObservationSolveSettings boss;
  boss.setInstrumentPositionSettings(BundleObservationSolveSettings::PositionOnly);

  EXPECT_EQ(boss.numberCameraPositionCoefficientsSolved(), 1);
  ASSERT_EQ(boss.aprioriPositionSigmas().size(), 1);
  EXPECT_EQ(boss.aprioriPositionSigmas().at(0), Isis::Null);
}

TEST(BundleObservationSolveSettings, setInstrumentPositionSettingsPositionVelocity) {
  BundleObservationSolveSettings boss;
  boss.setInstrumentPositionSettings(BundleObservationSolveSettings::PositionVelocity);

  EXPECT_EQ(boss.numberCameraPositionCoefficientsSolved(), 2);
  ASSERT_EQ(boss.aprioriPositionSigmas().size(), 2);
  EXPECT_EQ(boss.aprioriPositionSigmas().at(0), Isis::Null);
  EXPECT_EQ(boss.aprioriPositionSigmas().at(1), Isis::Null);
}

TEST(BundleObservationSolveSettings, setInstrumentPositionSettingsPositionVelocityAcceleration) {
  BundleObservationSolveSettings boss;
  boss.setInstrumentPositionSettings(BundleObservationSolveSettings::PositionVelocityAcceleration);

  EXPECT_EQ(boss.numberCameraPositionCoefficientsSolved(), 3);
  ASSERT_EQ(boss.aprioriPositionSigmas().size(), 3);
  EXPECT_EQ(boss.aprioriPositionSigmas().at(0), Isis::Null);
  EXPECT_EQ(boss.aprioriPositionSigmas().at(1), Isis::Null);
  EXPECT_EQ(boss.aprioriPositionSigmas().at(2), Isis::Null);
}

TEST(BundleObservationSolveSettings, setCsmSolveType) {
  BundleObservationSolveSettings boss;
  boss.setCSMSolveSet(csm::param::VALID);

  EXPECT_EQ(boss.csmSolveOption(), BundleObservationSolveSettings::Set);
  EXPECT_EQ(boss.csmParameterSet(), csm::param::VALID);
}

TEST(BundleObservationSolveSettings, setCSMSolveType) {
  BundleObservationSolveSettings boss;
  boss.setCSMSolveType(csm::param::FICTITIOUS);

  EXPECT_EQ(boss.csmSolveOption(), BundleObservationSolveSettings::Type);
  EXPECT_EQ(boss.csmParameterType(), csm::param::FICTITIOUS);
}

TEST(BundleObservationSolveSettings, setCSMSolveParameterList) {
  BundleObservationSolveSettings boss;
  boss.setCSMSolveParameterList({"param1", "param2"});

  QStringList parameterList = boss.csmParameterList();

  EXPECT_EQ(boss.csmSolveOption(), BundleObservationSolveSettings::List);
  ASSERT_EQ(parameterList.size(), 2);
  EXPECT_EQ(parameterList[0].toStdString(), "param1");
  EXPECT_EQ(parameterList[1].toStdString(), "param2");
}

TEST(BundleObservationSolveSettings, SaveSettings){
  BundleObservationSolveSettings boss;
  QDomDocument settingsDoc = saveToQDomDocument(boss);
  QDomElement root = settingsDoc.documentElement();

  QDomElement id = root.firstChildElement("id");
  EXPECT_FALSE(id.isNull());

  QDomElement instrumentId = root.firstChildElement("instrumentId");
  ASSERT_FALSE(instrumentId.isNull());
  EXPECT_PRED_FORMAT2(AssertQStringsEqual,
    instrumentId.nodeValue(), boss.instrumentId());

// begin testing instrument pointing options save
  QDomElement instrumentPointingOptions = root.firstChildElement("instrumentPointingOptions");
  ASSERT_FALSE(instrumentPointingOptions.isNull());

  QDomNamedNodeMap pointingOptionsAtts = instrumentPointingOptions.attributes();
  QString pointingSolveOption =
    BundleObservationSolveSettings::instrumentPointingSolveOptionToString(boss.instrumentPointingSolveOption());

  EXPECT_PRED_FORMAT2(AssertQStringsEqual, pointingOptionsAtts.namedItem(
    "solveOption").nodeValue(), pointingSolveOption);

  EXPECT_PRED_FORMAT2(AssertQStringsEqual,
    pointingOptionsAtts.namedItem("numberCoefSolved").nodeValue(),
    toString(boss.numberCameraAngleCoefficientsSolved()));

  EXPECT_PRED_FORMAT2(AssertQStringsEqual,
    pointingOptionsAtts.namedItem("degree").nodeValue(), toString(boss.ckDegree()));

  EXPECT_PRED_FORMAT2(AssertQStringsEqual,
    pointingOptionsAtts.namedItem("solveDegree").nodeValue(),
    toString(boss.ckSolveDegree()));

  EXPECT_PRED_FORMAT2(AssertQStringsEqual,
    pointingOptionsAtts.namedItem("solveTwist").nodeValue(),
    toString(boss.solveTwist()));

  EXPECT_PRED_FORMAT2(AssertQStringsEqual,
    pointingOptionsAtts.namedItem("solveOverExisting").nodeValue(),
    toString(boss.solvePolyOverPointing()));

  EXPECT_PRED_FORMAT2(AssertQStringsEqual,
    pointingOptionsAtts.namedItem("interpolationType").nodeValue(), "3");

  QDomElement aprioiPointingSigmas = instrumentPointingOptions.firstChildElement(
    "aprioriPointingSigmas");

  EXPECT_PRED_FORMAT2(AssertQStringsEqual,
    aprioiPointingSigmas.namedItem("sigma").nodeValue(), "");

// begin testing instrument position options save
  QDomElement instrumentPositionOptions = root.firstChildElement("instrumentPositionOptions");
  ASSERT_FALSE(instrumentPointingOptions.isNull());

  QDomNamedNodeMap positionOptionsAtts = instrumentPositionOptions.attributes();
  QString positionSolveOption =
    BundleObservationSolveSettings::instrumentPositionSolveOptionToString(
    boss.instrumentPositionSolveOption());

  EXPECT_PRED_FORMAT2(AssertQStringsEqual, positionOptionsAtts.namedItem(
    "solveOption").nodeValue(), positionSolveOption);

  EXPECT_PRED_FORMAT2(AssertQStringsEqual,
    positionOptionsAtts.namedItem("numberCoefSolved").nodeValue(),
    toString(boss.numberCameraPositionCoefficientsSolved()));

  EXPECT_PRED_FORMAT2(AssertQStringsEqual,
    positionOptionsAtts.namedItem("degree").nodeValue(),
    toString(boss.spkDegree()));

  EXPECT_PRED_FORMAT2(AssertQStringsEqual,
    positionOptionsAtts.namedItem("degree").nodeValue(),
    toString(boss.spkDegree()));

  EXPECT_PRED_FORMAT2(AssertQStringsEqual,
    positionOptionsAtts.namedItem("solveDegree").nodeValue(),
    toString(boss.spkSolveDegree()));

  EXPECT_PRED_FORMAT2(AssertQStringsEqual,
    positionOptionsAtts.namedItem("solveOverHermiteSpline").nodeValue(),
    toString(boss.solvePositionOverHermite()));

  EXPECT_PRED_FORMAT2(AssertQStringsEqual,
    positionOptionsAtts.namedItem("interpolationType").nodeValue(), "3");

  QDomElement aprioriPositionSigmas = instrumentPositionOptions.firstChildElement(
    "aprioriPositionSigmas");

  EXPECT_PRED_FORMAT2(AssertQStringsEqual,
    aprioriPositionSigmas.namedItem("sigma").nodeValue(), "");
}

TEST_P(CSMSolveOptionStrings, StringToOption) {
  EXPECT_EQ(GetParam().first,
    BundleObservationSolveSettings::stringToCSMSolveOption(GetParam().second));
}

TEST_P(CSMSolveOptionStrings, OptionToString) {
  EXPECT_PRED_FORMAT2(AssertQStringsEqual, GetParam().second,
    BundleObservationSolveSettings::csmSolveOptionToString(GetParam().first));
}

TEST_P(CSMSolveSetStrings, StringToOption) {
  EXPECT_EQ(GetParam().first,
    BundleObservationSolveSettings::stringToCSMSolveSet(GetParam().second));
}

TEST_P(CSMSolveSetStrings, OptionToString) {
  EXPECT_PRED_FORMAT2(AssertQStringsEqual, GetParam().second,
    BundleObservationSolveSettings::csmSolveSetToString(GetParam().first));
}

TEST_P(CSMSolveTypeStrings, StringToOption) {
  EXPECT_EQ(GetParam().first,
    BundleObservationSolveSettings::stringToCSMSolveType(GetParam().second));
}

TEST_P(CSMSolveTypeStrings, OptionToString) {
  EXPECT_PRED_FORMAT2(AssertQStringsEqual, GetParam().second,
    BundleObservationSolveSettings::csmSolveTypeToString(GetParam().first));
}

TEST_P(PointingSolveOptionStrings, StringToOption) {
  EXPECT_EQ(GetParam().first,
    BundleObservationSolveSettings::stringToInstrumentPointingSolveOption(GetParam().second));
}

TEST_P(PointingSolveOptionStrings, OptionToString) {
  EXPECT_PRED_FORMAT2(AssertQStringsEqual, GetParam().second,
    BundleObservationSolveSettings::instrumentPointingSolveOptionToString(GetParam().first));
}

TEST_P(PositionSolveOptionStrings, StringToOption) {
  EXPECT_EQ(GetParam().first,
    BundleObservationSolveSettings::stringToInstrumentPositionSolveOption(GetParam().second));
}

TEST_P(PositionSolveOptionStrings, OptionToString) {
  EXPECT_PRED_FORMAT2(AssertQStringsEqual, GetParam().second,
    BundleObservationSolveSettings::instrumentPositionSolveOptionToString(GetParam().first));
}

INSTANTIATE_TEST_SUITE_P(BundleObservationSolveSettings, CSMSolveOptionStrings, ::testing::Values(
  qMakePair(BundleObservationSolveSettings::NoCSMParameters, QString("NoCSMParameters")),
  qMakePair(BundleObservationSolveSettings::Set, QString("Set")),
  qMakePair(BundleObservationSolveSettings::Type, QString("Type")),
  qMakePair(BundleObservationSolveSettings::List, QString("List"))));

INSTANTIATE_TEST_SUITE_P(BundleObservationSolveSettings, CSMSolveSetStrings, ::testing::Values(
  qMakePair(csm::param::VALID, QString("VALID")),
  qMakePair(csm::param::ADJUSTABLE, QString("ADJUSTABLE")),
  qMakePair(csm::param::NON_ADJUSTABLE, QString("NON_ADJUSTABLE"))));

INSTANTIATE_TEST_SUITE_P(BundleObservationSolveSettings, CSMSolveTypeStrings, ::testing::Values(
  qMakePair(csm::param::NONE, QString("NONE")),
  qMakePair(csm::param::FICTITIOUS, QString("FICTITIOUS")),
  qMakePair(csm::param::REAL, QString("REAL")),
  qMakePair(csm::param::FIXED, QString("FIXED"))));

INSTANTIATE_TEST_SUITE_P(BundleObservationSolveSettings, PointingSolveOptionStrings, ::testing::Values(
  qMakePair(BundleObservationSolveSettings::NoPointingFactors, QString("None")),
  qMakePair(BundleObservationSolveSettings::AnglesOnly, QString("AnglesOnly")),
  qMakePair(BundleObservationSolveSettings::AnglesVelocity, QString("AnglesAndVelocity")),
  qMakePair(BundleObservationSolveSettings::AnglesVelocityAcceleration,
            QString("AnglesVelocityAndAcceleration")),
  qMakePair(BundleObservationSolveSettings::AllPointingCoefficients,
            QString("AllPolynomialCoefficients"))));

INSTANTIATE_TEST_SUITE_P(BundleObservationSolveSettings, PositionSolveOptionStrings, ::testing::Values(
  qMakePair(BundleObservationSolveSettings::NoPositionFactors, QString("None")),
  qMakePair(BundleObservationSolveSettings::PositionOnly, QString("PositionOnly")),
  qMakePair(BundleObservationSolveSettings::PositionVelocity, QString("PositionAndVelocity")),
  qMakePair(BundleObservationSolveSettings::PositionVelocityAcceleration,
            QString("PositionVelocityAndAcceleration")),
  qMakePair(BundleObservationSolveSettings::AllPositionCoefficients,
            QString("AllPolynomialCoefficients"))));

TEST(BundleObservationSolveSettings, GroupConstructorBadOverhermite) {
  std::string message = "The OVERHERMITE parameter must be set to TRUE or FALSE; YES or NO";

  PvlGroup settingsGroup("VO1/VISA");
  PvlKeyword camsolve("CamSolve");
  PvlKeyword spsolve("SPSolve");
  PvlKeyword overhermite("OVERHERMITE");
  overhermite = "MAYBE";
  camsolve = "Angles";
  spsolve = "None";
  settingsGroup += overhermite;
  settingsGroup += camsolve;
  settingsGroup += spsolve;

  try {
    BundleObservationSolveSettings boss(settingsGroup);
  }
  catch(Isis::IException &e) {
    EXPECT_PRED_FORMAT2(Isis::AssertIExceptionMessage, e, message);
  }
  catch(...) {
    FAIL() << "Expected an IException with message \""
    << message.toStdString() <<"\"";
  }
}

TEST(BundleObservationSolveSettings, GroupConstructorBadTwist) {
  std::string message = "The TWIST parameter must be set to TRUE or FALSE; YES or NO";

  PvlGroup settingsGroup("VO1/VISA");
  PvlKeyword camsolve("CamSolve");
  PvlKeyword twist("Twist");
  PvlKeyword spsolve("SPSolve");
  camsolve = "Angles";
  twist = "maybe";
  spsolve = "None";
  settingsGroup += camsolve;
  settingsGroup += twist;
  settingsGroup += spsolve;

  try {
    BundleObservationSolveSettings boss(settingsGroup);
  }
  catch(Isis::IException &e) {
    EXPECT_PRED_FORMAT2(Isis::AssertIExceptionMessage, e, message);
  }
  catch(...) {
    FAIL() << "Expected an IException with message \""
    << message.toStdString() <<"\"";
  }
}

TEST(BundleObservationSolveSettings, GroupConstructorBadOverExisting) {
  std::string message = "The OVEREXISTING parameter must be set to TRUE or FALSE; YES or NO";

  PvlGroup settingsGroup("VO1/VISA");
  PvlKeyword camsolve("CamSolve");
  PvlKeyword overexisting("OverExisting");
  PvlKeyword spsolve("SPSolve");
  camsolve = "Angles";
  overexisting = "maybe";
  spsolve = "None";
  settingsGroup += camsolve;
  settingsGroup += overexisting;
  settingsGroup += spsolve;

  try {
    BundleObservationSolveSettings boss(settingsGroup);
  }
  catch(Isis::IException &e) {
    EXPECT_PRED_FORMAT2(Isis::AssertIExceptionMessage, e, message);
  }
  catch(...) {
    FAIL() << "Expected an IException with message \""
    << message.toStdString() <<"\"";
  }
}

TEST(BundleObservationSolveSettings, PositionStringToOptionBadValue)
{
  std::string message = "Unknown bundle instrument position solve option foo.";

  try {
    BundleObservationSolveSettings::stringToInstrumentPositionSolveOption(
      "foo");
  }
  catch(Isis::IException &e) {
    EXPECT_PRED_FORMAT2(Isis::AssertIExceptionMessage, e, message);
  }
  catch(...) {
    FAIL() << "Expected an IException with message \""
    << message.toStdString() <<"\"";
  }
}

TEST(BundleObservationSolveSettings, PointingStringToOptionBadValue)
{
  std::string message = "Unknown bundle instrument pointing solve option foo.";

  try {
    BundleObservationSolveSettings::stringToInstrumentPointingSolveOption(
      "foo");
  }
  catch(Isis::IException &e) {
    EXPECT_PRED_FORMAT2(Isis::AssertIExceptionMessage, e, message);
  }
  catch(...) {
    FAIL() << "Expected an IException with message \""
    << message.toStdString() <<"\"";
  }
}
