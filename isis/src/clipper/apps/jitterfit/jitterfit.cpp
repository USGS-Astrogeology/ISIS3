#include <iostream>

#include <QList>
#include <QString>

#include "AutoReg.h"
#include "AutoRegFactory.h"
#include "BasisFunction.h"
#include "Chip.h"
#include "Cube.h"
#include "CSVReader.h"
#include "FileName.h"
#include "jitterfit.h"
#include "LeastSquares.h"
#include "NthOrderPolynomial.h"
#include "Pvl.h"
#include "Table.h"
#include "UserInterface.h"

using namespace std;

namespace Isis{
  struct RegistrationData {
    int checkLine; //0
    int checkSample; //1
    double checkTime; //2
    double matchedLine; //3
    double matchedSample; //4
    double matchedTime; //5
    double deltaLine; //6
    double deltaSample; //7
    double goodness; //8 Goodness of fit
    int success; //9
  };

  void jitterfit(UserInterface &ui) {

    bool registrationFileSpecified = false;

    Cube jitterCube;
    jitterCube.open(ui.GetCubeName("FROM"), "rw");

    Cube checkCube;
    checkCube.open(ui.GetCubeName("FROM2"), "r");

    Pvl defFile;
    defFile.read(ui.GetFileName("DEFFILE").toStdString());
    AutoReg *ar = AutoRegFactory::Create(defFile);

    double scale = ui.GetDouble("SCALE");

    int pointSpacing = jitterCube.sampleCount();

    // Setup the registration results file
    ofstream outputFile;
    if (ui.WasEntered("TO")) {
      registrationFileSpecified = true;
      QString to(FileName(ui.GetCubeName("TO")).expanded());
      outputFile.open(to.toLatin1().data());
      outputFile << "# checkline line, checkline sample, checkline time taken, "
                    "matched jittered image line, matched jittered image "
                    "sample, matched jittered image time taken, delta line, "
                    "delta sample, goodness of fit, registration success " << endl;
    }

    // ???? Question: Why use a file name here? Can't Table/blob read from an open Cube?
    Table mainReadouts("Normalized Main Readout Line Times", jitterCube.fileName());
    Table checklineReadouts("Normalized Checkline Readout Line Times", checkCube.fileName());

    // Register each check line to the area near the corresponding main image line using the
    // registration definition file
    QList<RegistrationData> registrationData;
    for (int k = 0; k < checkCube.lineCount(); k++) {

      int checklineLine = checklineReadouts[k][0];
      int mainLine = mainReadouts[checklineLine][0];

      int sample = (int)(pointSpacing / 2.0 + 0.5);

      ar->PatternChip()->TackCube(sample, k + 1);
      ar->PatternChip()->Load(checkCube);

      // The checkline will correspond to the line number that the checkCube was taken at
      ar->SearchChip()->TackCube(sample, checklineLine * scale);
      ar->SearchChip()->Load(jitterCube);

      ar->Register();

      if (registrationFileSpecified) {
        outputFile << checklineLine << "," << sample/scale << "," <<
                      std::setprecision(14) << double(checklineReadouts[k][1]) << "," <<
                      ar->CubeLine()/scale << "," << ar->CubeSample()/scale << ","  <<
                      double(mainReadouts[mainLine][1]) << "," <<
                      checklineLine - ar->CubeLine()/scale << "," <<
                      sample/scale - ar->CubeSample()/scale << "," <<
                      ar->GoodnessOfFit() << "," << ar->Success() << endl;
      }

      RegistrationData checkLineRegistration;
      checkLineRegistration.checkLine = checklineLine;
      checkLineRegistration.checkSample = sample/scale;
      checkLineRegistration.checkTime = double(checklineReadouts[k][1]);
      checkLineRegistration.matchedLine = ar->CubeLine()/scale;
      checkLineRegistration.matchedSample = ar->CubeSample()/scale;
      checkLineRegistration.matchedTime = double(mainReadouts[mainLine][1]);
      checkLineRegistration.deltaLine = checklineLine - ar->CubeLine()/scale;
      checkLineRegistration.deltaSample = sample/scale - ar->CubeSample()/scale;
      checkLineRegistration.goodness = ar->GoodnessOfFit();
      checkLineRegistration.success = ar->Success();
      registrationData.append(checkLineRegistration);
    }

    //!!!!!!!!FOR INTERNAL STORAGE BETWEEN THE REGISTRATION STEP AND THE FITTING STEP !!!!!!!
    //!!!!!!!! We are going to use a QList of registrationData structs !!!!!!!

    if (ui.WasEntered("TO2")) {
      ofstream regStatsFile;
      QString to(FileName(ui.GetFileName("TO2")).expanded());
      regStatsFile.open(to.toLatin1().data());
      Pvl regStats = ar->RegistrationStatistics();
      regStatsFile << regStats << endl;
      regStatsFile << endl;
      regStatsFile.close();
    }

    if (registrationFileSpecified) {
      outputFile.close();
    }

    // Solve for the coefficients of the Nth order polynomial
    double tolerance = ui.GetDouble("TOLERANCE");
    int degree = ui.GetInteger("DEGREE");
    double maxTime = ui.GetDouble("MAXTIME");

    BasisFunction *lineFunction = new NthOrderPolynomial(degree);
    BasisFunction *sampleFunction = new NthOrderPolynomial(degree);

    LeastSquares lsqLine(*lineFunction, false, false, false, false);
    LeastSquares lsqSample(*sampleFunction, false, false, false, false);

    std::vector<double> known(2);

    for (int i = 0; i < registrationData.size(); i++) {
      RegistrationData checkLineRow = registrationData[i];

      if (checkLineRow.goodness >= tolerance) {

        /* Normalization Equation
         *
         * a = min of scale
         * b = max of scale
         *
         * ((b - a)(x - min(x)) / (max(x) - min(x))) + a
         *
         * We're normalizing from -1 to 1 so the equation below is simplified
         */

        known[0] = ((2 * checkLineRow.matchedTime) / maxTime) - 1;
        known[1] = ((2 * checkLineRow.checkTime) / maxTime) - 1;

        lsqLine.AddKnown(known, checkLineRow.deltaLine);
        lsqSample.AddKnown(known, checkLineRow.deltaSample);
      }
    }

    lsqLine.Solve();
    lsqSample.Solve();


    // Write the coefficients to COEFFICIENTTO file and the main cube label
    ofstream outputCoefficientFile;
    QString coefficientTo(FileName(ui.GetFileName("COEFFICIENTTO")).expanded());
    outputCoefficientFile.open(coefficientTo.toLatin1().data());
    outputCoefficientFile << "# Line, Sample" << endl;

    PvlKeyword &jitterLineCoefficients =
        jitterCube.label()->findKeyword("JitterLineCoefficients", PvlObject::Traverse);
    PvlKeyword &jitterSampleCoefficients =
        jitterCube.label()->findKeyword("JitterSampleCoefficients", PvlObject::Traverse);

    for (int i = 0; i < degree; i++) {
      outputCoefficientFile << std::setprecision(14) << lineFunction->Coefficient(i) << "," <<
                               std::setprecision(14) << sampleFunction->Coefficient(i) << endl;
      if (i == 0) {
        jitterLineCoefficients.setValue(std::to_string(lineFunction->Coefficient(i)));
        jitterSampleCoefficients.setValue(std::to_string(sampleFunction->Coefficient(i)));
      }
      else {
        jitterLineCoefficients += std::to_string(lineFunction->Coefficient(i));
        jitterSampleCoefficients += std::to_string(sampleFunction->Coefficient(i));
      }
    }

    outputCoefficientFile.close();


    // Write the registered line/samp, solved line/samp, residual line/samp, time
    if (ui.WasEntered("RESIDUALTO")) {
      ofstream outputResidualFile;
      QString residualTo(FileName(ui.GetFileName("RESIDUALTO")).expanded());
      outputResidualFile.open(residualTo.toLatin1().data());

      outputResidualFile << "# Registered Line, Solved Line, Registered Line Residual, "
                            "Registered Sample, Solved Sample, Sample Residual, Time Taken" << endl;

      for (unsigned int i = 0; i < lsqLine.Residuals().size(); i++) {
        RegistrationData checkLineRow = registrationData[i];

        double solvedLine = 0;
        double solvedSample = 0;

        for (int k = 0; k < degree; k++) {
          solvedLine = solvedLine + lineFunction->Coefficient(k) * pow(checkLineRow.matchedTime, k+1);
          solvedSample = solvedSample + sampleFunction->Coefficient(k) * pow(checkLineRow.matchedTime, k+1);
        }

        outputResidualFile << std::setprecision(14) << checkLineRow.matchedLine << "," <<
                              std::setprecision(14) << checkLineRow.checkLine - solvedLine << "," <<
                              std::setprecision(14) << lsqLine.Residual(i) << "," <<
                              std::setprecision(14) << checkLineRow.matchedSample << "," <<
                              std::setprecision(14) << checkLineRow.checkSample - solvedSample << "," <<
                              std::setprecision(14) << lsqSample.Residual(i) << "," <<
                              std::setprecision(14) << checkLineRow.matchedTime << endl;
      }

      outputResidualFile.close();
    }

    delete lineFunction;
    delete sampleFunction;

  }
}
