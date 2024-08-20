#include <QFile>
#include <QRegularExpression>
#include <QString>
#include <QStringList>
#include <QTemporaryDir>
#include <QTextStream>

#include "hical.h"
#include "Pvl.h"
#include "PvlGroup.h"
#include "TestUtilities.h"
#include "Statistics.h"

#include "gtest/gtest.h"
#include "gmock/gmock.h"

using namespace Isis;

static QString APP_XML = FileName("$ISISROOT/bin/xml/hical.xml").expanded();

TEST(HicalTest, Default) {
  QTemporaryDir prefix;
  QString outFileName = prefix.path() + "/out.cub";
  QVector<QString> args = { "FROM=data/hical/mroHical.cub",
                            "TO=" + outFileName,
                            "OPATH=" + prefix.path() + "/" };
  UserInterface options(APP_XML, args);

  try {
    hical(options);
  }
  catch (IException &e) {
    FAIL() << e.toString().toStdString().c_str() << std::endl;
  }

  // Check calibrated cube
  Cube outCube(outFileName);

  ASSERT_TRUE(outCube.hasGroup("RadiometricCalibration"));

  PvlGroup calibration = outCube.group("RadiometricCalibration");
  ASSERT_TRUE(calibration.hasKeyword("Program"));
  EXPECT_EQ(calibration.findKeyword("Program")[0], "hical");

  std::unique_ptr<Statistics> stats (outCube.statistics());
  EXPECT_NEAR(stats->Average(), 0.066949089371337, .00001);
  EXPECT_NEAR(stats->StandardDeviation(), 0.0048735204823545, .00001);

  // Check log file
  QFile logFile(prefix.path() + "/ESP_044492_1605_RED3_0.hical.log");
  ASSERT_TRUE(logFile.open(QIODevice::ReadOnly)) << "Failed to open log file: " << logFile.fileName().toStdString();
  QTextStream logStream(&logFile);
  QStringList logText;
  while (!logStream.atEnd()) {
    logText.push_back(logStream.readLine());
  }
  ASSERT_EQ(logText.size(), 37);

  // Header
  EXPECT_EQ(logText[0].toStdString(), "Program:  hical");
  EXPECT_TRUE(logText[1].startsWith("RunTime")) << logText[1].toStdString();
  EXPECT_TRUE(logText[2].startsWith("Version")) << logText[2].toStdString();
  EXPECT_TRUE(logText[3].startsWith("Revision")) << logText[3].toStdString();

  // Arguments
  EXPECT_EQ(logText[5].toStdString(), "FROM:     data/hical/mroHical.cub");
  EXPECT_EQ(logText[6].toStdString(), "TO:       " + outFileName.toStdString());
  EXPECT_TRUE(logText[7].startsWith("CONF")) << logText[7].toStdString();

  // Parameter Generation

  // We need to parse some numbers out of the parameter generation strings
  // so we'll use the following regex to capture values between brackets.
  // The part of the parameter generation string we are checking is commented
  // above each match call.
  QRegularExpression valueBetweenBracketsRegex("(?<=\\[)[^]]*");
  QRegularExpressionMatchIterator patameterMatches;
  QRegularExpressionMatch parameterValues;
  QString paramString;

  // ZeroBufferSmooth parameters
  EXPECT_TRUE(logText[16].startsWith("ZeroBufferSmooth")) << logText[16].toStdString();
  QStringList zeroBufferSmoothParams = logText[16].split(";", Qt::SkipEmptyParts);
  ASSERT_EQ(zeroBufferSmoothParams.size(), 6);
  // AveCols(Buffer[ ])
  paramString = zeroBufferSmoothParams[1];
  patameterMatches = valueBetweenBracketsRegex.globalMatch(paramString);
  ASSERT_TRUE(patameterMatches.hasNext()) << "Failed to find bracketed value in " << paramString.toStdString();
  parameterValues = patameterMatches.next();
  EXPECT_EQ(parameterValues.captured(0).toStdString(), "5,11");
  // LowPassFilter(Width[ ],Iters[ ])
  paramString = zeroBufferSmoothParams[2];
  patameterMatches = valueBetweenBracketsRegex.globalMatch(paramString);
  ASSERT_TRUE(patameterMatches.hasNext()) << "Failed to find bracketed value in " << paramString.toStdString();
  parameterValues = patameterMatches.next();
  EXPECT_EQ(parameterValues.captured(0).toStdString(), "201");
  ASSERT_TRUE(patameterMatches.hasNext()) << "Failed to find second bracketed value in " << paramString.toStdString();
  parameterValues = patameterMatches.next();
  EXPECT_EQ(parameterValues.captured(0).toStdString(), "2");
  // SplineFill(Cubic,Filled[ ])
  paramString = zeroBufferSmoothParams[3];
  patameterMatches = valueBetweenBracketsRegex.globalMatch(paramString);
  ASSERT_TRUE(patameterMatches.hasNext()) << "Failed to find bracketed value in " << paramString.toStdString();
  parameterValues = patameterMatches.next();
  EXPECT_EQ(parameterValues.captured(0).toStdString(), "0");
  // Statistics(Average[ ],StdDev[ ])
  paramString = zeroBufferSmoothParams[4];
  patameterMatches = valueBetweenBracketsRegex.globalMatch(paramString);
  ASSERT_TRUE(patameterMatches.hasNext()) << "Failed to find bracketed value in " << paramString.toStdString();
  parameterValues = patameterMatches.next();
  EXPECT_NEAR(parameterValues.captured(0).toDouble(), -5.33791642580354e-05, 1e-10);
  ASSERT_TRUE(patameterMatches.hasNext()) << "Failed to find second bracketed value in " << paramString.toStdString();
  parameterValues = patameterMatches.next();
  EXPECT_NEAR(parameterValues.captured(0).toDouble(), 2.3166791031246, .00001);

  // ZeroBufferFit parameters
  EXPECT_TRUE(logText[18].startsWith("ZeroBufferFit")) << logText[18].toStdString();
  QStringList zeroBufferFitParams = logText[18].split(";", Qt::SkipEmptyParts);
  ASSERT_EQ(zeroBufferFitParams.size(), 5);
  // ZeroBufferFit(AbsErr[ ],RelErr[ ],MaxIter[ ])
  paramString = zeroBufferFitParams[1];
  patameterMatches = valueBetweenBracketsRegex.globalMatch(paramString);
  ASSERT_TRUE(patameterMatches.hasNext()) << "Failed to find bracketed value in " << paramString.toStdString();
  parameterValues = patameterMatches.next();
  EXPECT_EQ(parameterValues.captured(0).toDouble(), 1.0e-04);
  ASSERT_TRUE(patameterMatches.hasNext()) << "Failed to find second bracketed value in " << paramString.toStdString();
  parameterValues = patameterMatches.next();
  EXPECT_EQ(parameterValues.captured(0).toDouble(), 1.0e-04);
  ASSERT_TRUE(patameterMatches.hasNext()) << "Failed to find third bracketed value in " << paramString.toStdString();
  parameterValues = patameterMatches.next();
  EXPECT_EQ(parameterValues.captured(0).toDouble(), 100);
  // SkipFit(TRUE: Not using LMFit)
  EXPECT_EQ(zeroBufferFitParams[2].toStdString(), " SkipFit(TRUE: Not using LMFit)");
  // Normalize[ ]
  paramString = zeroBufferFitParams[3];
  patameterMatches = valueBetweenBracketsRegex.globalMatch(paramString);
  ASSERT_TRUE(patameterMatches.hasNext()) << "Failed to find bracketed value in " << paramString.toStdString();
  parameterValues = patameterMatches.next();
  EXPECT_NEAR(parameterValues.captured(0).toDouble(), 1213.5996244789, 0.00001);

  // ZeroReverse parameters
  EXPECT_TRUE(logText[20].startsWith("ZeroReverse")) << logText[20].toStdString();
  QStringList zeroReverseParams = logText[20].split(";", Qt::SkipEmptyParts);
  ASSERT_EQ(zeroReverseParams.size(), 7);
  // RevClock(CropLines[ ],Mean[ ],StdDev[ ],LisPixels[ ],HisPixels[ ],NulPixels[ ])
  paramString = zeroReverseParams[1];
  patameterMatches = valueBetweenBracketsRegex.globalMatch(paramString);
  ASSERT_TRUE(patameterMatches.hasNext()) << "Failed to find bracketed value in " << paramString.toStdString();
  parameterValues = patameterMatches.next();
  EXPECT_EQ(parameterValues.captured(0).toStdString(), "1,19");
  ASSERT_TRUE(patameterMatches.hasNext()) << "Failed to find second bracketed value in " << paramString.toStdString();
  parameterValues = patameterMatches.next();
  EXPECT_NEAR(parameterValues.captured(0).toDouble(), 1165.8540296053, 0.00001);
  ASSERT_TRUE(patameterMatches.hasNext()) << "Failed to find third bracketed value in " << paramString.toStdString();
  parameterValues = patameterMatches.next();
  EXPECT_NEAR(parameterValues.captured(0).toDouble(), 10.576682845308, 0.00001);
  ASSERT_TRUE(patameterMatches.hasNext()) << "Failed to find fourth bracketed value in " << paramString.toStdString();
  parameterValues = patameterMatches.next();
  EXPECT_EQ(parameterValues.captured(0).toInt(), 0);
  ASSERT_TRUE(patameterMatches.hasNext()) << "Failed to find fifth bracketed value in " << paramString.toStdString();
  parameterValues = patameterMatches.next();
  EXPECT_EQ(parameterValues.captured(0).toInt(), 0);
  ASSERT_TRUE(patameterMatches.hasNext()) << "Failed to find sixth bracketed value in " << paramString.toStdString();
  parameterValues = patameterMatches.next();
  EXPECT_EQ(parameterValues.captured(0).toInt(), 0);
  // TriggerLimits(RevMeanTrigger[ ],RevStdDevTrigger[ ],RevLisTolerance[ ],RevHisTolerance[ ],RevNulTolerance[ ])
  paramString = zeroReverseParams[3];
  patameterMatches = valueBetweenBracketsRegex.globalMatch(paramString);
  ASSERT_TRUE(patameterMatches.hasNext()) << "Failed to find bracketed value in " << paramString.toStdString();
  parameterValues = patameterMatches.next();
  EXPECT_EQ(parameterValues.captured(0).toStdString(), "1128.152");
  ASSERT_TRUE(patameterMatches.hasNext()) << "Failed to find second bracketed value in " << paramString.toStdString();
  parameterValues = patameterMatches.next();
  EXPECT_EQ(parameterValues.captured(0).toDouble(), 50.0);
  ASSERT_TRUE(patameterMatches.hasNext()) << "Failed to find third bracketed value in " << paramString.toStdString();
  parameterValues = patameterMatches.next();
  EXPECT_EQ(parameterValues.captured(0).toInt(), 1);
  ASSERT_TRUE(patameterMatches.hasNext()) << "Failed to find fourth bracketed value in " << paramString.toStdString();
  parameterValues = patameterMatches.next();
  EXPECT_EQ(parameterValues.captured(0).toInt(), 1);
  ASSERT_TRUE(patameterMatches.hasNext()) << "Failed to find fifth bracketed value in " << paramString.toStdString();
  parameterValues = patameterMatches.next();
  EXPECT_EQ(parameterValues.captured(0).toInt(), 1);

  // ZeroDark parameters
  EXPECT_TRUE(logText[22].startsWith("ZeroDark")) << logText[22].toStdString();
  QStringList zeroDarkParams = logText[22].split(";", Qt::SkipEmptyParts);
  ASSERT_EQ(zeroDarkParams.size(), 9);
  // Smooth(Width[ ],Iters[ ])
  paramString = zeroDarkParams[4];
  patameterMatches = valueBetweenBracketsRegex.globalMatch(paramString);
  ASSERT_TRUE(patameterMatches.hasNext()) << "Failed to find bracketed value in " << paramString.toStdString();
  parameterValues = patameterMatches.next();
  EXPECT_EQ(parameterValues.captured(0).toInt(), 3);
  ASSERT_TRUE(patameterMatches.hasNext()) << "Failed to find second bracketed value in " << paramString.toStdString();
  parameterValues = patameterMatches.next();
  EXPECT_EQ(parameterValues.captured(0).toInt(), 0);
  // BaseTemperature[ ]
  paramString = zeroDarkParams[5];
  patameterMatches = valueBetweenBracketsRegex.globalMatch(paramString);
  ASSERT_TRUE(patameterMatches.hasNext()) << "Failed to find bracketed value in " << paramString.toStdString();
  parameterValues = patameterMatches.next();
  EXPECT_NEAR(parameterValues.captured(0).toDouble(), 35.3167, 0.00001);
  // Statistics(Average[ ],StdDev[ ])
  paramString = zeroDarkParams[7];
  patameterMatches = valueBetweenBracketsRegex.globalMatch(paramString);
  ASSERT_TRUE(patameterMatches.hasNext()) << "Failed to find bracketed value in " << paramString.toStdString();
  parameterValues = patameterMatches.next();
  EXPECT_NEAR(parameterValues.captured(0).toDouble(), 218.63090957509, 0.00001);
  ASSERT_TRUE(patameterMatches.hasNext()) << "Failed to find second bracketed value in " << paramString.toStdString();
  parameterValues = patameterMatches.next();
  EXPECT_NEAR(parameterValues.captured(0).toDouble(), 9.229160895462, 0.00001);

  // ZeroDarkRate parameters
  EXPECT_TRUE(logText[24].startsWith("ZeroDarkRate")) << logText[24].toStdString();
  QStringList zeroDarkRateParams = logText[24].split(";", Qt::SkipEmptyParts);
  ASSERT_EQ(zeroDarkRateParams.size(), 3);
  // Skipped, module not in config file
  EXPECT_EQ(zeroDarkRateParams[1].toStdString(), " Skipped, module not in config file");

  // GainLineDrift parameters
  EXPECT_TRUE(logText[26].startsWith("GainLineDrift")) << logText[26].toStdString();
  QStringList gainLineDriftParams = logText[26].split(";", Qt::SkipEmptyParts);
  ASSERT_EQ(gainLineDriftParams.size(), 4);
  // Coefs[ , , , ]
  paramString = gainLineDriftParams[2];
  patameterMatches = valueBetweenBracketsRegex.globalMatch(paramString);
  ASSERT_TRUE(patameterMatches.hasNext()) << "Failed to find bracketed value in " << paramString.toStdString();
  parameterValues = patameterMatches.next();
  EXPECT_NEAR(parameterValues.captured(0).split(",")[0].toDouble(), 1.01578, 0.00001);
  EXPECT_NEAR(parameterValues.captured(0).split(",")[1].toDouble(), 6.6118e-04, 0.00001);
  EXPECT_NEAR(parameterValues.captured(0).split(",")[2].toDouble(), -0.0152593, 0.00001);
  EXPECT_NEAR(parameterValues.captured(0).split(",")[3].toDouble(), -1.25226, 0.00001);

  // GainNonLinearity parameters
  EXPECT_TRUE(logText[28].startsWith("GainNonLinearity")) << logText[28].toStdString();
  QStringList gainNonLinearityParams = logText[28].split(";", Qt::SkipEmptyParts);
  ASSERT_EQ(gainNonLinearityParams.size(), 4);
  // NonLinearityGainFactor[ ]
  paramString = gainNonLinearityParams[2];
  patameterMatches = valueBetweenBracketsRegex.globalMatch(paramString);
  ASSERT_TRUE(patameterMatches.hasNext()) << "Failed to find bracketed value in " << paramString.toStdString();
  parameterValues = patameterMatches.next();
  EXPECT_NEAR(parameterValues.captured(0).toDouble(), -1.43738e-07, 0.00001);

  // GainChannelNormalize parameters
  EXPECT_TRUE(logText[30].startsWith("GainChannelNormalize")) << logText[30].toStdString();
  QStringList gainChannelNormalizeParams = logText[30].split(";", Qt::SkipEmptyParts);
  ASSERT_EQ(gainChannelNormalizeParams.size(), 4);
  // ModeNormalizer[ ]
  paramString = gainChannelNormalizeParams[1];
  patameterMatches = valueBetweenBracketsRegex.globalMatch(paramString);
  ASSERT_TRUE(patameterMatches.hasNext()) << "Failed to find bracketed value in " << paramString.toStdString();
  parameterValues = patameterMatches.next();
  EXPECT_EQ(parameterValues.captured(0).toDouble(), 0.25);

  // GainFlatField parameters
  EXPECT_TRUE(logText[32].startsWith("GainFlatField")) << logText[32].toStdString();
  QStringList gainFlatFieldParams = logText[32].split(";", Qt::SkipEmptyParts);
  ASSERT_EQ(gainFlatFieldParams.size(), 4);
  // Statistics(Average[ ],StdDev[ ])
  paramString = gainFlatFieldParams[2];
  patameterMatches = valueBetweenBracketsRegex.globalMatch(paramString);
  ASSERT_TRUE(patameterMatches.hasNext()) << "Failed to find bracketed value in " << paramString.toStdString();
  parameterValues = patameterMatches.next();
  EXPECT_NEAR(parameterValues.captured(0).toDouble(), 1.0000375, 0.00001);
  ASSERT_TRUE(patameterMatches.hasNext()) << "Failed to find second bracketed value in " << paramString.toStdString();
  parameterValues = patameterMatches.next();
  EXPECT_NEAR(parameterValues.captured(0).toDouble(), 0.0063274662973697, 0.00001);

  // GainTemperature parameters
  EXPECT_TRUE(logText[34].startsWith("GainTemperature")) << logText[34].toStdString();
  QStringList gainTemperatureParams = logText[34].split(";", Qt::SkipEmptyParts);
  ASSERT_EQ(gainTemperatureParams.size(), 7);
  // FpaTemperatureFactor[ ]
  paramString = gainTemperatureParams[2];
  patameterMatches = valueBetweenBracketsRegex.globalMatch(paramString);
  ASSERT_TRUE(patameterMatches.hasNext()) << "Failed to find bracketed value in " << paramString.toStdString();
  parameterValues = patameterMatches.next();
  EXPECT_NEAR(parameterValues.captured(0).toDouble(), 0.0012, 0.00001);
  // FpaAverageTemperature[ ]
  paramString = gainTemperatureParams[3];
  patameterMatches = valueBetweenBracketsRegex.globalMatch(paramString);
  ASSERT_TRUE(patameterMatches.hasNext()) << "Failed to find bracketed value in " << paramString.toStdString();
  parameterValues = patameterMatches.next();
  EXPECT_NEAR(parameterValues.captured(0).toDouble(), 35.3167, 0.00001);
  // FpaReferenceTemperature[ ]
  paramString = gainTemperatureParams[4];
  patameterMatches = valueBetweenBracketsRegex.globalMatch(paramString);
  ASSERT_TRUE(patameterMatches.hasNext()) << "Failed to find bracketed value in " << paramString.toStdString();
  parameterValues = patameterMatches.next();
  EXPECT_NEAR(parameterValues.captured(0).toDouble(), 21.0, 0.00001);
  // Correction[ ]
  paramString = gainTemperatureParams[5];
  patameterMatches = valueBetweenBracketsRegex.globalMatch(paramString);
  ASSERT_TRUE(patameterMatches.hasNext()) << "Failed to find bracketed value in " << paramString.toStdString();
  parameterValues = patameterMatches.next();
  EXPECT_NEAR(parameterValues.captured(0).toDouble(), 0.98281996, 0.00001);

  // GainUnitConversion parameters
  EXPECT_TRUE(logText[36].startsWith("GainUnitConversion")) << logText[36].toStdString();
  QStringList gainUnitConversionParams = logText[36].split(";", Qt::SkipEmptyParts);
  ASSERT_EQ(gainUnitConversionParams.size(), 13);
  // SunDist[  (AU)]
  paramString = gainUnitConversionParams[1];
  patameterMatches = valueBetweenBracketsRegex.globalMatch(paramString);
  ASSERT_TRUE(patameterMatches.hasNext()) << "Failed to find bracketed value in " << paramString.toStdString();
  parameterValues = patameterMatches.next();
  EXPECT_NEAR(parameterValues.captured(0).split(" ")[0].toDouble(), 1.6464026610198, 0.00001);
  // GainUnitConversionBinFactor[ ]
  paramString = gainUnitConversionParams[2];
  patameterMatches = valueBetweenBracketsRegex.globalMatch(paramString);
  ASSERT_TRUE(patameterMatches.hasNext()) << "Failed to find bracketed value in " << paramString.toStdString();
  parameterValues = patameterMatches.next();
  EXPECT_EQ(parameterValues.captured(0).toDouble(), 1.0);
  // FilterGainCorrection[ ]
  paramString = gainUnitConversionParams[3];
  patameterMatches = valueBetweenBracketsRegex.globalMatch(paramString);
  ASSERT_TRUE(patameterMatches.hasNext()) << "Failed to find bracketed value in " << paramString.toStdString();
  parameterValues = patameterMatches.next();
  EXPECT_EQ(parameterValues.captured(0).toDouble(), 157702564.0);
  // T(AveFpa_YTemp)[ ]
  paramString = gainUnitConversionParams[4];
  patameterMatches = valueBetweenBracketsRegex.globalMatch(paramString);
  ASSERT_TRUE(patameterMatches.hasNext()) << "Failed to find bracketed value in " << paramString.toStdString();
  parameterValues = patameterMatches.next();
  EXPECT_NEAR(parameterValues.captured(0).toDouble(), 35.3167, 0.00001);
  // IoverFbasetemperature[ ]
  paramString = gainUnitConversionParams[5];
  patameterMatches = valueBetweenBracketsRegex.globalMatch(paramString);
  ASSERT_TRUE(patameterMatches.hasNext()) << "Failed to find bracketed value in " << paramString.toStdString();
  parameterValues = patameterMatches.next();
  EXPECT_EQ(parameterValues.captured(0).toDouble(), 18.9);
  // QEpercentincreaseperC[ ]
  paramString = gainUnitConversionParams[6];
  patameterMatches = valueBetweenBracketsRegex.globalMatch(paramString);
  ASSERT_TRUE(patameterMatches.hasNext()) << "Failed to find bracketed value in " << paramString.toStdString();
  parameterValues = patameterMatches.next();
  EXPECT_NEAR(parameterValues.captured(0).toDouble(), 5.704e-04, 0.00001);
  // AbsGain_TDI128[ ]
  paramString = gainUnitConversionParams[7];
  patameterMatches = valueBetweenBracketsRegex.globalMatch(paramString);
  ASSERT_TRUE(patameterMatches.hasNext()) << "Failed to find bracketed value in " << paramString.toStdString();
  parameterValues = patameterMatches.next();
  EXPECT_NEAR(parameterValues.captured(0).toDouble(), 6.376583, 0.00001);
  // CalFactQETempDep[ ]
  paramString = gainUnitConversionParams[8];
  patameterMatches = valueBetweenBracketsRegex.globalMatch(paramString);
  ASSERT_TRUE(patameterMatches.hasNext()) << "Failed to find bracketed value in " << paramString.toStdString();
  parameterValues = patameterMatches.next();
  EXPECT_NEAR(parameterValues.captured(0).toDouble(), 167119121.22791, 0.00001);
  // ScanExposureDuration[ ]
  paramString = gainUnitConversionParams[9];
  patameterMatches = valueBetweenBracketsRegex.globalMatch(paramString);
  ASSERT_TRUE(patameterMatches.hasNext()) << "Failed to find bracketed value in " << paramString.toStdString();
  parameterValues = patameterMatches.next();
  EXPECT_EQ(parameterValues.captured(0).toDouble(), 86.1875);
  // I/F_Factor[ ]
  paramString = gainUnitConversionParams[10];
  patameterMatches = valueBetweenBracketsRegex.globalMatch(paramString);
  ASSERT_TRUE(patameterMatches.hasNext()) << "Failed to find bracketed value in " << paramString.toStdString();
  parameterValues = patameterMatches.next();
  EXPECT_NEAR(parameterValues.captured(0).toDouble(), 11955.860146065, 0.00001);
  // Units[I/F]
  EXPECT_EQ(gainUnitConversionParams[11].toStdString(), " Units[I/F]");
}


TEST(HicalTest, Dns) {
  QTemporaryDir prefix;
  QString outFileName = prefix.path() + "/out.cub";
  QVector<QString> args = { "FROM=data/hical/mroHical.cub",
                            "TO=" + outFileName,
                            "UNITS=DN",
                            "OPATH=" + prefix.path() + "/" };
  UserInterface options(APP_XML, args);

  try {
    hical(options);
  }
  catch (IException &e) {
    FAIL() << e.toString().toStdString().c_str() << std::endl;
  }

  // Check calibrated cube
  Cube outCube(outFileName);

  std::unique_ptr<Statistics> stats (outCube.statistics());
  EXPECT_NEAR(stats->Average(), 800.43395004272, .00001);
  EXPECT_NEAR(stats->StandardDeviation(), 58.267128965098, .00001);

  // Check log file
  QFile logFile(prefix.path() + "/ESP_044492_1605_RED3_0.hical.log");
  ASSERT_TRUE(logFile.open(QIODevice::ReadOnly)) << "Failed to open log file: " << logFile.fileName().toStdString();
  QTextStream logStream(&logFile);
  QStringList logText;
  while (!logStream.atEnd()) {
    logText.push_back(logStream.readLine());
  }
  ASSERT_EQ(logText.size(), 37);

  // The only difference for DNs is the GainUnitConversion line
  EXPECT_TRUE(logText[36].startsWith("GainUnitConversion")) << logText[36].toStdString();
  QStringList gainUnitConversionParams = logText[36].split(";", Qt::SkipEmptyParts);
  ASSERT_EQ(gainUnitConversionParams.size(), 4);
  EXPECT_EQ(gainUnitConversionParams[1].toStdString(), " DN_Factor[1.0]");
  EXPECT_EQ(gainUnitConversionParams[2].toStdString(), " Units[DN]");
}


TEST(HicalTest, DnsPerMicrosecond) {
  QTemporaryDir prefix;
  QString outFileName = prefix.path() + "/out.cub";
  QVector<QString> args = { "FROM=data/hical/mroHical.cub",
                            "TO=" + outFileName,
                            "UNITS=DN/US",
                            "OPATH=" + prefix.path() + "/" };
  UserInterface options(APP_XML, args);

  try {
    hical(options);
  }
  catch (IException &e) {
    FAIL() << e.toString().toStdString().c_str() << std::endl;
  }

  // Check calibrated cube
  Cube outCube(outFileName);

  std::unique_ptr<Statistics> stats (outCube.statistics());
  EXPECT_NEAR(stats->Average(), 9.2871234171093, .00001);
  EXPECT_NEAR(stats->StandardDeviation(), 0.67605080377689, .00001);

  // Check log file
  QFile logFile(prefix.path() + "/ESP_044492_1605_RED3_0.hical.log");
  ASSERT_TRUE(logFile.open(QIODevice::ReadOnly)) << "Failed to open log file: " << logFile.fileName().toStdString();
  QTextStream logStream(&logFile);
  QStringList logText;
  while (!logStream.atEnd()) {
    logText.push_back(logStream.readLine());
  }
  ASSERT_EQ(logText.size(), 37);

  // We need to parse some numbers out of the parameter generation strings
  // so we'll use the following regex to capture values between brackets.
  // The part of the parameter generation string we are checking is commented
  // above each match call.
  QRegularExpression valueBetweenBracketsRegex("(?<=\\[)[^]]*");
  QRegularExpressionMatchIterator patameterMatches;
  QRegularExpressionMatch parameterValues;
  QString paramString;

  // The only difference for DNs per microsecond is the GainUnitConversion line
  EXPECT_TRUE(logText[36].startsWith("GainUnitConversion")) << logText[36].toStdString();
  QStringList gainUnitConversionParams = logText[36].split(";", Qt::SkipEmptyParts);
  ASSERT_EQ(gainUnitConversionParams.size(), 5);
  // ScanExposureDuration[ ]
  paramString = gainUnitConversionParams[1];
  patameterMatches = valueBetweenBracketsRegex.globalMatch(paramString);
  ASSERT_TRUE(patameterMatches.hasNext()) << "Failed to find bracketed value in " << paramString.toStdString();
  parameterValues = patameterMatches.next();
  EXPECT_NEAR(parameterValues.captured(0).toDouble(), 86.1875, 0.00001);
  // GainUnitConversionBinFactor[ ]
  paramString = gainUnitConversionParams[2];
  patameterMatches = valueBetweenBracketsRegex.globalMatch(paramString);
  ASSERT_TRUE(patameterMatches.hasNext()) << "Failed to find bracketed value in " << paramString.toStdString();
  parameterValues = patameterMatches.next();
  EXPECT_NEAR(parameterValues.captured(0).toDouble(), 86.1875, 0.00001);
  // Units[DNs/microsecond]
  EXPECT_EQ(gainUnitConversionParams[3].toStdString(), " Units[DNs/microsecond]");
}


TEST(HicalTest, DarkRate) {
  QTemporaryDir prefix;
  QString outFileName = prefix.path() + "/out.cub";
  QVector<QString> args = { "FROM=data/hical/mroHical.cub",
                            "TO=" + outFileName,
                            "CONF=data/hical/hical.0023_darkrate.conf",
                            "OPATH=" + prefix.path() + "/" };
  UserInterface options(APP_XML, args);

  try {
    hical(options);
  }
  catch (IException &e) {
    FAIL() << e.toString().toStdString().c_str() << std::endl;
  }

  // Check calibrated cube
  Cube outCube(outFileName);

  std::unique_ptr<Statistics> stats (outCube.statistics());
  EXPECT_NEAR(stats->Average(), 0.029009951796252, .00001);
  EXPECT_NEAR(stats->StandardDeviation(), 0.0045240528853485, .00001);

  // Check log file
  QFile logFile(prefix.path() + "/ESP_044492_1605_RED3_0.hical.log");
  ASSERT_TRUE(logFile.open(QIODevice::ReadOnly)) << "Failed to open log file: " << logFile.fileName().toStdString();
  QTextStream logStream(&logFile);
  QStringList logText;
  while (!logStream.atEnd()) {
    logText.push_back(logStream.readLine());
  }
  ASSERT_EQ(logText.size(), 37);

  // We need to parse some numbers out of the parameter generation strings
  // so we'll use the following regex to capture values between brackets.
  // The part of the parameter generation string we are checking is commented
  // above each match call.
  QRegularExpression valueBetweenBracketsRegex("(?<=\\[)[^]]*");
  QRegularExpressionMatchIterator patameterMatches;
  QRegularExpressionMatch parameterValues;
  QString paramString;

  // The only difference for ZeroDarkRate is the ZeroDarkRate line
  EXPECT_TRUE(logText[24].startsWith("ZeroDarkRate")) << logText[24].toStdString();
  QStringList zeroDarkRateParams = logText[24].split(";", Qt::SkipEmptyParts);
  ASSERT_EQ(zeroDarkRateParams.size(), 4);
  // BaseTemperature[ ]
  paramString = zeroDarkRateParams[1];
  patameterMatches = valueBetweenBracketsRegex.globalMatch(paramString);
  ASSERT_TRUE(patameterMatches.hasNext()) << "Failed to find bracketed value in " << paramString.toStdString();
  parameterValues = patameterMatches.next();
  EXPECT_NEAR(parameterValues.captured(0).toDouble(), 35.3167, 0.00001);
  // Statistics(Average[ ],StdDev[ ])
  paramString = zeroDarkRateParams[2];
  patameterMatches = valueBetweenBracketsRegex.globalMatch(paramString);
  ASSERT_TRUE(patameterMatches.hasNext()) << "Failed to find bracketed value in " << paramString.toStdString();
  parameterValues = patameterMatches.next();
  EXPECT_NEAR(parameterValues.captured(0).toDouble(), 2404.2733410374, 0.00001);
  ASSERT_TRUE(patameterMatches.hasNext()) << "Failed to find second bracketed value in " << paramString.toStdString();
  parameterValues = patameterMatches.next();
  EXPECT_NEAR(parameterValues.captured(0).toDouble(), 163.44824721503, 0.00001);
}

TEST(HicalTest, DarkRateFallback) {
  QTemporaryDir prefix;
  QString outFileName = prefix.path() + "/out.cub";
  QVector<QString> args = { "FROM=data/hical/mroHical.cub",
                            "TO=" + outFileName,
                            "CONF=data/hical/hical.0023_darkrate_missing.conf",
                            "OPATH=" + prefix.path() + "/" };
  UserInterface options(APP_XML, args);

  try {
    hical(options);
  }
  catch (IException &e) {
    FAIL() << e.toString().toStdString().c_str() << std::endl;
  }

  // Check calibrated cube
  Cube outCube(outFileName);

  std::unique_ptr<Statistics> stats (outCube.statistics());
  EXPECT_NEAR(stats->Average(), 0.066949089371337325, .00001);
  EXPECT_NEAR(stats->StandardDeviation(), 0.004873520482354521, .00001);
}