#include "CSVReader.h"
#include "TempFixtures.h"
#include "jitterfit.h"
#include "TestUtilities.h"

#include "gmock/gmock.h"
#include <fstream>

using namespace Isis;

static QString APP_XML = QString::fromStdString(FileName("$ISISROOT/bin/xml/jitterfit.xml").expanded());
TEST_F(TempTestingFiles, DISABLED_FunctionalTestJitterfitDefault){
  QTemporaryDir prefix;
  QString outputCoeffs = prefix.path() + "/coef.csv";
  QString outputResiduals = prefix.path() + "/residuals.csv";
  QString outputRegistrationStats = prefix.path() + "/regStats.pvl";
  QString outputRegistrationResults = prefix.path() + "/regResults.csv";

  // Copy input data to tempdir because jitterfit writes to cube labels
  QString dataCube = prefix.path() + "/source.cub";
  std::ifstream dataSrc("data/jitterfit/simulated_clipper_eis_nac_rolling_shutter_1500_750.cub", std::ios::binary);
  std::ofstream dataDest(dataCube.toStdString(), std::ios::binary);
  dataDest << dataSrc.rdbuf();
  dataSrc.close();
  dataDest.close();

  QString checkline = prefix.path() + "/checkline.cub";
  std::ifstream checklineSrc("data/jitterfit/simulated_clipper_eis_nac_rolling_shutter_checkline_750.cub", std::ios::binary);;
  std::ofstream checklineDest(checkline.toStdString(), std::ios::binary);
  checklineDest << checklineSrc.rdbuf();
  checklineSrc.close();
  checklineDest.close();

  QVector<QString> args = {"from=" + dataCube,
                           "from2=" + checkline,
                           "scale=1.0",
                           "deffile=data/jitterfit/S046mos1400x2250.def",
                           "coefficientto=" + outputCoeffs,
                           "residualto=" + outputResiduals,
                           "to=" + outputRegistrationResults,
                           "to2=" + outputRegistrationStats};

  UserInterface options(APP_XML, args);

  try {
    jitterfit(options);
  }
  catch (IException &e) {
    FAIL() << "Jitterfit failed: " << e.what() << std::endl;
  }

  // Test coefficients that were written to csv
  CSVReader::CSVAxis csvLine;
  CSVReader  csv = CSVReader(outputCoeffs, false, 0, ',', true, true);

  // line 0, sample 0
  csvLine = csv.getRow(0);
  EXPECT_DOUBLE_EQ(csvLine[0].toDouble(), -2.7623177125388001e-09);
  EXPECT_DOUBLE_EQ(csvLine[1].toDouble(), -6.3185740156775997e-09);

  // line 1, sample 1
  csvLine = csv.getRow(1);
  EXPECT_DOUBLE_EQ(csvLine[0].toDouble(), 3.3727910440682999e-06);
  EXPECT_DOUBLE_EQ(csvLine[1].toDouble(), 2.4403471374750999e-06);
  // line 2, sample 2
  csvLine = csv.getRow(2);
  EXPECT_DOUBLE_EQ(csvLine[0].toDouble(), 0.0013484324974775001);
  EXPECT_DOUBLE_EQ(csvLine[1].toDouble(), 0.0032581267520383002);

  // Test coefficients that were written to cube
  Cube cube(dataCube.toStdString());
  Pvl *isisLabel = cube.label();
  PvlGroup &inst = isisLabel->findGroup("Instrument", Pvl::Traverse);

  EXPECT_DOUBLE_EQ(std::stod(inst["JitterSampleCoefficients"][0]), -6.3185740156776303e-09);
  EXPECT_DOUBLE_EQ(std::stod(inst["JitterSampleCoefficients"][1]), 2.44034713747508e-06);
  EXPECT_DOUBLE_EQ(std::stod(inst["JitterSampleCoefficients"][2]), 0.0032581267520383002);
  EXPECT_DOUBLE_EQ(std::stod(inst["JitterLineCoefficients"][0]), -2.7623177125388001e-09);
  EXPECT_DOUBLE_EQ(std::stod(inst["JitterLineCoefficients"][1]), 3.3727910440682698e-06);
  EXPECT_DOUBLE_EQ(std::stod(inst["JitterLineCoefficients"][2]), 0.0013484324974775001);

  // Test residuals
  CSVReader::CSVAxis residLine;
  CSVReader residCsv = CSVReader(outputResiduals, false, 0, ',', true, true);

  // first
  residLine = residCsv.getRow(0);
  EXPECT_DOUBLE_EQ(residLine[0].toDouble(), 472.05701420458); // Solved Line
  EXPECT_DOUBLE_EQ(residLine[1].toDouble(), 471.00019746177998); // Solved Line
  EXPECT_DOUBLE_EQ(residLine[2].toDouble(), -0.7481500070121499); // Registered Line Residual
  EXPECT_DOUBLE_EQ(residLine[3].toDouble(), 376.18491660661); // Solved Sample
  EXPECT_DOUBLE_EQ(residLine[4].toDouble(), 375.00047870498003); // Solved Sample
  EXPECT_DOUBLE_EQ(residLine[5].toDouble(), -0.81010042440526997); // Sample Residual
  EXPECT_DOUBLE_EQ(residLine[6].toDouble(), -0.52792617775619); // Time Taken

  // middle
  residLine = residCsv.getRow(29);
  EXPECT_DOUBLE_EQ(residLine[0].toDouble(),  1460);
  EXPECT_DOUBLE_EQ(residLine[1].toDouble(), 1460.9998655764);
  EXPECT_DOUBLE_EQ(residLine[2].toDouble(), -0.24153962899969);
  EXPECT_DOUBLE_EQ(residLine[3].toDouble(), 374);
  EXPECT_DOUBLE_EQ(residLine[4].toDouble(), 374.99967642426998);
  EXPECT_DOUBLE_EQ(residLine[5].toDouble(), 0.18615309567725999);
  EXPECT_DOUBLE_EQ(residLine[6].toDouble(), 0.46284604176784999);

  // last
  residLine = residCsv.getRow(59);
  EXPECT_DOUBLE_EQ(residLine[0].toDouble(), 1461.1743826668001);
  EXPECT_DOUBLE_EQ(residLine[1].toDouble(), 1460.9998655764);
  EXPECT_DOUBLE_EQ(residLine[2].toDouble(), -0.80403923092037999);
  EXPECT_DOUBLE_EQ(residLine[3].toDouble(), 374.47660613948);
  EXPECT_DOUBLE_EQ(residLine[4].toDouble(), 374.99967642426998);
  EXPECT_DOUBLE_EQ(residLine[5].toDouble(), -0.57941935501399);
  EXPECT_DOUBLE_EQ(residLine[6].toDouble(), 0.46284604176785);


  // Test results of registration
  CSVReader::CSVAxis regLine;
  CSVReader regCsv= CSVReader(outputRegistrationResults, false, 0, ',', true, true);

  // first
  regLine = regCsv.getRow(0);
  EXPECT_DOUBLE_EQ(regLine[0].toDouble(), 471); // Checkline Line
  EXPECT_DOUBLE_EQ(regLine[1].toDouble(), 375); // Checkline Sample
  EXPECT_DOUBLE_EQ(regLine[2].toDouble(), -1); // Checkline Time Taken
  EXPECT_NEAR(regLine[3].toDouble(), 472.05701420458, .00000001); // Matched Jittered Image Line
  EXPECT_NEAR(regLine[4].toDouble(), 376.18491660661, .00000001); // Matched Jittered Image Sample
  EXPECT_NEAR(regLine[5].toDouble(), -0.52792617775619, .00000001); // Matched Jittered image Time Taken
  EXPECT_NEAR(regLine[6].toDouble(), -1.057014204579, .00000001); // Delta Line
  EXPECT_NEAR(regLine[7].toDouble(), -1.1849166066075, .00000001); // Delta Sample
  EXPECT_NEAR(regLine[8].toDouble(), 0.98617538549134, .00000001); // Goodness Of Fit
  EXPECT_DOUBLE_EQ(regLine[9].toDouble(), 1); // Registration Success

  // middle
  regLine = regCsv.getRow(29);
  EXPECT_DOUBLE_EQ(regLine[0].toDouble(), 1461);
  EXPECT_DOUBLE_EQ(regLine[1].toDouble(), 375);
  EXPECT_NEAR(regLine[2].toDouble(), -0.016949152542373, .00000001);
  EXPECT_DOUBLE_EQ(regLine[3].toDouble(), 1460);
  EXPECT_DOUBLE_EQ(regLine[4].toDouble(), 374);
  EXPECT_NEAR(regLine[5].toDouble(), 0.46284604176785, .00000001);
  EXPECT_DOUBLE_EQ(regLine[6].toDouble(), 1);
  EXPECT_DOUBLE_EQ(regLine[7].toDouble(), 1);
  EXPECT_NEAR(regLine[8].toDouble(), 0.98756321875905, .00000001);
  EXPECT_DOUBLE_EQ(regLine[9].toDouble(), 0);

  // Last
  regLine = regCsv.getRow(59);
  EXPECT_DOUBLE_EQ(regLine[0].toDouble(), 1461);
  EXPECT_DOUBLE_EQ(regLine[1].toDouble(), 375);
  EXPECT_DOUBLE_EQ(regLine[2].toDouble(), 1);
  EXPECT_NEAR(regLine[3].toDouble(), 1461.1743826668001, .00000001);
  EXPECT_NEAR(regLine[4].toDouble(), 374.47660613948, .00000001);
  EXPECT_NEAR(regLine[5].toDouble(), 0.46284604176785, .00000001);
  EXPECT_NEAR(regLine[6].toDouble(), -0.17438266677323, .00000001);
  EXPECT_NEAR(regLine[7].toDouble(), 0.52339386051505998, .00000001);
  EXPECT_NEAR(regLine[8].toDouble(), 0.94265790827978002, .00000001);
  EXPECT_DOUBLE_EQ(regLine[9].toDouble(), 1);


  // Test statistics of registration
  Pvl stats = Pvl(outputRegistrationStats.toStdString());
  PvlGroup &autoRegStats = stats.findGroup("AutoRegStatistics");
  EXPECT_EQ((double) autoRegStats.findKeyword("Total"), 60);
  EXPECT_EQ((double) autoRegStats.findKeyword("Successful"), 42);
  EXPECT_EQ((double) autoRegStats.findKeyword("Failure"), 18);

  PvlGroup &success = stats.findGroup("Successes");
  EXPECT_EQ((double) success.findKeyword("SuccessPixel"), 0);
  EXPECT_EQ((double) success.findKeyword("SuccessSubPixel"), 42);

  PvlGroup &patternFailure = stats.findGroup("PatternChipFailures");
  EXPECT_EQ((double) patternFailure.findKeyword("PatternNotEnoughValidData"), 0);
  EXPECT_EQ((double) patternFailure.findKeyword("PatternZScoreNotMet"), 0);

  PvlGroup &fitFailure = stats.findGroup("FitChipFailures");
  EXPECT_EQ((double) fitFailure.findKeyword("FitChipNoData"), 0);
  EXPECT_EQ((double) fitFailure.findKeyword("FitChipToleranceNotMet"), 0);

  PvlGroup &surfaceModelFailures = stats.findGroup("SurfaceModelFailures");
  EXPECT_EQ((double) surfaceModelFailures.findKeyword("SurfaceModelNotEnoughValidData"), 18);
  EXPECT_EQ((double) surfaceModelFailures.findKeyword("SurfaceModelSolutionInvalid"), 0);
  EXPECT_EQ((double) surfaceModelFailures.findKeyword("SurfaceModelDistanceInvalid"), 0);
}
