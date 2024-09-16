/** This is free and unencumbered software released into the public domain.

The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */

#include <QDataStream>
#include <QDebug>

#include <iostream>
#include <new>

#include "IException.h"
#include "MaximumLikelihoodWFunctions.h"
#include "Preference.h"
#include "Statistics.h"


using namespace std;
using namespace Isis;
/**
 * @author 2012-03-23 Orrin Thomas
 *
 * @internal
 *   @history 2012-03-23 Orrin Thomas - Original Version
 *   @history 2014-09-19 Jeannie Backer - Improved unitTest to greater than 90% scope, line,
 *                           and function test coverage.
 */

int main(int argc, char *argv[]) {
  Preference::Preferences(true);

  qDebug() << "Test for MaximumLikelihoodWFunctions";
  qDebug() << "";

  double temp;

  qDebug() << "Default constructor sets model to Huber and corresponding default TC:";
  MaximumLikelihoodWFunctions wFunc;
  qDebug() << "Model                    = "
           << MaximumLikelihoodWFunctions::modelToString(wFunc.model());
  qDebug() << "TweakingConstant         = " << QString::number(wFunc.tweakingConstant());
  qDebug() << "WeightedResidualCutoff   = " << wFunc.weightedResidualCutoff();
  qDebug() << "TweakingConstantQuantile = " << QString::number(wFunc.tweakingConstantQuantile());
  temp = wFunc.sqrtWeightScaler(-0.5);
  qDebug() << "wFunc->sqrtWeightScaler(-0.5): " << temp;
  temp = wFunc.sqrtWeightScaler(0.75);
  qDebug() << "wFunc->sqrtWeightScaler(0.75): " << temp;
  temp = wFunc.sqrtWeightScaler(-2.0);
  qDebug() << "wFunc->sqrtWeightScaler(-2):   " << temp;
  temp = wFunc.sqrtWeightScaler(2.5);
  qDebug() << "wFunc->sqrtWeightScaler(2.5):  " << temp;
  qDebug() << "";

  wFunc.setTweakingConstant(2.0);
  qDebug() << "TC constant re-set to 2.0:";
  qDebug() << "Model                    = "
           << MaximumLikelihoodWFunctions::modelToString(wFunc.model());
  qDebug() << "TweakingConstant         = " << QString::number(wFunc.tweakingConstant());
  qDebug() << "WeightedResidualCutoff   = " << wFunc.weightedResidualCutoff();
  qDebug() << "TweakingConstantQuantile = " << QString::number(wFunc.tweakingConstantQuantile());
  temp = wFunc.sqrtWeightScaler(-0.5);
  qDebug() << "wFunc->sqrtWeightScaler(-0.5): " << temp;
  temp = wFunc.sqrtWeightScaler(0.75);
  qDebug() << "wFunc->sqrtWeightScaler(0.75): " << temp;
  temp = wFunc.sqrtWeightScaler(-2.0);
  qDebug() << "wFunc->sqrtWeightScaler(-2):   " << temp;
  temp = wFunc.sqrtWeightScaler(2.5);
  qDebug() << "wFunc->sqrtWeightScaler(2.5):  " << temp;
  qDebug() << "";

  wFunc.setTweakingConstantDefault();
  qDebug() << "TC constant re-set to default:";
  qDebug() << "Model                    = "
           << MaximumLikelihoodWFunctions::modelToString(wFunc.model());
  qDebug() << "TweakingConstant         = " << QString::number(wFunc.tweakingConstant());
  qDebug() << "WeightedResidualCutoff   = " << wFunc.weightedResidualCutoff();
  qDebug() << "TweakingConstantQuantile = " << QString::number(wFunc.tweakingConstantQuantile());
  temp = wFunc.sqrtWeightScaler(-0.5);
  qDebug() << "wFunc->sqrtWeightScaler(-0.5): " << temp;
  temp = wFunc.sqrtWeightScaler(0.75);
  qDebug() << "wFunc->sqrtWeightScaler(0.75): " << temp;
  temp = wFunc.sqrtWeightScaler(-2.0);
  qDebug() << "wFunc->sqrtWeightScaler(-2):   " << temp;
  temp = wFunc.sqrtWeightScaler(2.5);
  qDebug() << "wFunc->sqrtWeightScaler(2.5):  " << temp;
  qDebug() << "";

  wFunc.setModel(MaximumLikelihoodWFunctions::Huber);
  qDebug() << "Model manually set with default TC:";
  qDebug() << "Model                    = "
           << MaximumLikelihoodWFunctions::modelToString(wFunc.model());
  qDebug() << "TweakingConstant         = " << QString::number(wFunc.tweakingConstant());
  qDebug() << "WeightedResidualCutoff   = " << wFunc.weightedResidualCutoff();
  qDebug() << "TweakingConstantQuantile = " << QString::number(wFunc.tweakingConstantQuantile());
  temp = wFunc.sqrtWeightScaler(-0.5);
  qDebug() << "wFunc->sqrtWeightScaler(-0.5): " << temp;
  temp = wFunc.sqrtWeightScaler(0.75);
  qDebug() << "wFunc->sqrtWeightScaler(0.75): " << temp;
  temp = wFunc.sqrtWeightScaler(-2.0);
  qDebug() << "wFunc->sqrtWeightScaler(-2):   " << temp;
  temp = wFunc.sqrtWeightScaler(2.5);
  qDebug() << "wFunc->sqrtWeightScaler(2.5):  " << temp;
  qDebug() << "";

  wFunc.setModel(MaximumLikelihoodWFunctions::Huber, 2.0);
  qDebug() << "Model manually set with TC = 2.0:";
  qDebug() << "Model                    = "
           << MaximumLikelihoodWFunctions::modelToString(wFunc.model());
  qDebug() << "TweakingConstant         = " << QString::number(wFunc.tweakingConstant());
  qDebug() << "WeightedResidualCutoff   = " << wFunc.weightedResidualCutoff();
  qDebug() << "TweakingConstantQuantile = " << QString::number(wFunc.tweakingConstantQuantile());
  temp = wFunc.sqrtWeightScaler(-0.5);
  qDebug() << "wFunc->sqrtWeightScaler(-0.5): " << temp;
  temp = wFunc.sqrtWeightScaler(0.75);
  qDebug() << "wFunc->sqrtWeightScaler(0.75): " << temp;
  temp = wFunc.sqrtWeightScaler(-2.0);
  qDebug() << "wFunc->sqrtWeightScaler(-2):   " << temp;
  temp = wFunc.sqrtWeightScaler(2.5);
  qDebug() << "wFunc->sqrtWeightScaler(2.5):  " << temp;
  qDebug() << "";

  MaximumLikelihoodWFunctions copyWFunc(wFunc);
  qDebug() << "Testing Copy constructor:";
  qDebug() << "Model                    = "
           << MaximumLikelihoodWFunctions::modelToString(copyWFunc.model());
  qDebug() << "TweakingConstant         = " << QString::number(copyWFunc.tweakingConstant());
  qDebug() << "WeightedResidualCutoff   = " << copyWFunc.weightedResidualCutoff();
  qDebug() << "TweakingConstantQuantile = " << QString::number(copyWFunc.tweakingConstantQuantile());
  temp = copyWFunc.sqrtWeightScaler(-0.5);
  qDebug() << "copyWFunc->sqrtWeightScaler(-0.5): " << temp;
  temp = copyWFunc.sqrtWeightScaler(0.75);
  qDebug() << "copyWFunc->sqrtWeightScaler(0.75): " << temp;
  temp = copyWFunc.sqrtWeightScaler(-2.0);
  qDebug() << "copyWFunc->sqrtWeightScaler(-2):   " << temp;
  temp = copyWFunc.sqrtWeightScaler(2.5);
  qDebug() << "copyWFunc->sqrtWeightScaler(2.5):  " << temp;
  qDebug() << "";
  qDebug() << "";



  qDebug() << "Reassign object using operator= and passing HuberModified to constructor with "
              "default TC:";
  wFunc = MaximumLikelihoodWFunctions(MaximumLikelihoodWFunctions::stringToModel("HuberModified"));
  qDebug() << "Model                    = "
           << MaximumLikelihoodWFunctions::modelToString(wFunc.model());
  qDebug() << "TweakingConstant         = " << QString::number(wFunc.tweakingConstant());
  qDebug() << "WeightedResidualCutoff   = " << wFunc.weightedResidualCutoff();
  qDebug() << "TweakingConstantQuantile = " << QString::number(wFunc.tweakingConstantQuantile());
  temp = wFunc.sqrtWeightScaler(-0.5);
  qDebug() << "wFunc->sqrtWeightScaler(-0.5): " << temp;
  temp = wFunc.sqrtWeightScaler(0.75);
  qDebug() << "wFunc->sqrtWeightScaler(0.75): " << temp;
  temp = wFunc.sqrtWeightScaler(-2.0);
  qDebug() << "wFunc->sqrtWeightScaler(-2):   " << temp;
  temp = wFunc.sqrtWeightScaler(2.5);
  qDebug() << "wFunc->sqrtWeightScaler(2.5):  " << temp;
  qDebug() << "";

  wFunc.setTweakingConstant(2.0);
  qDebug() << "TC constant re-set to 2.0:";
  qDebug() << "Model                    = "
           << MaximumLikelihoodWFunctions::modelToString(wFunc.model());
  qDebug() << "TweakingConstant         = " << QString::number(wFunc.tweakingConstant());
  qDebug() << "WeightedResidualCutoff   = " << wFunc.weightedResidualCutoff();
  qDebug() << "TweakingConstantQuantile = " << QString::number(wFunc.tweakingConstantQuantile());
  temp = wFunc.sqrtWeightScaler(-0.5);
  qDebug() << "wFunc->sqrtWeightScaler(-0.5): " << temp;
  temp = wFunc.sqrtWeightScaler(0.75);
  qDebug() << "wFunc->sqrtWeightScaler(0.75): " << temp;
  temp = wFunc.sqrtWeightScaler(-2.0);
  qDebug() << "wFunc->sqrtWeightScaler(-2):   " << temp;
  temp = wFunc.sqrtWeightScaler(2.5);
  qDebug() << "wFunc->sqrtWeightScaler(2.5):  " << temp;
  qDebug() << "";

  wFunc.setTweakingConstantDefault();
  qDebug() << "TC constant re-set to default:";
  qDebug() << "Model                    = "
           << MaximumLikelihoodWFunctions::modelToString(wFunc.model());
  qDebug() << "TweakingConstant         = " << QString::number(wFunc.tweakingConstant());
  qDebug() << "WeightedResidualCutoff   = " << wFunc.weightedResidualCutoff();
  qDebug() << "TweakingConstantQuantile = " << QString::number(wFunc.tweakingConstantQuantile());
  temp = wFunc.sqrtWeightScaler(-0.5);
  qDebug() << "wFunc->sqrtWeightScaler(-0.5): " << temp;
  temp = wFunc.sqrtWeightScaler(0.75);
  qDebug() << "wFunc->sqrtWeightScaler(0.75): " << temp;
  temp = wFunc.sqrtWeightScaler(-2.0);
  qDebug() << "wFunc->sqrtWeightScaler(-2):   " << temp;
  temp = wFunc.sqrtWeightScaler(2.5);
  qDebug() << "wFunc->sqrtWeightScaler(2.5):  " << temp;
  qDebug() << "";

  wFunc.setModel(MaximumLikelihoodWFunctions::stringToModel("Huber_Modified"));
  qDebug() << "Model manually set to Huber_Modified with default TC:";
  qDebug() << "Model                    = "
           << MaximumLikelihoodWFunctions::modelToString(wFunc.model());
  qDebug() << "TweakingConstant         = " << QString::number(wFunc.tweakingConstant());
  qDebug() << "WeightedResidualCutoff   = " << wFunc.weightedResidualCutoff();
  qDebug() << "TweakingConstantQuantile = " << QString::number(wFunc.tweakingConstantQuantile());
  temp = wFunc.sqrtWeightScaler(-0.5);
  qDebug() << "wFunc->sqrtWeightScaler(-0.5): " << temp;
  temp = wFunc.sqrtWeightScaler(0.75);
  qDebug() << "wFunc->sqrtWeightScaler(0.75): " << temp;
  temp = wFunc.sqrtWeightScaler(-2.0);
  qDebug() << "wFunc->sqrtWeightScaler(-2):   " << temp;
  temp = wFunc.sqrtWeightScaler(2.5);
  qDebug() << "wFunc->sqrtWeightScaler(2.5):  " << temp;
  qDebug() << "";

  wFunc.setModel(MaximumLikelihoodWFunctions::HuberModified, 2.0);
  qDebug() << "Model manually set with TC = 2.0:";
  qDebug() << "Model                    = "
           << MaximumLikelihoodWFunctions::modelToString(wFunc.model());
  qDebug() << "TweakingConstant         = " << QString::number(wFunc.tweakingConstant());
  qDebug() << "WeightedResidualCutoff   = " << wFunc.weightedResidualCutoff();
  qDebug() << "TweakingConstantQuantile = " << QString::number(wFunc.tweakingConstantQuantile());
  temp = wFunc.sqrtWeightScaler(-0.5);
  qDebug() << "wFunc->sqrtWeightScaler(-0.5): " << temp;
  temp = wFunc.sqrtWeightScaler(0.75);
  qDebug() << "wFunc->sqrtWeightScaler(0.75): " << temp;
  temp = wFunc.sqrtWeightScaler(-2.0);
  qDebug() << "wFunc->sqrtWeightScaler(-2):   " << temp;
  temp = wFunc.sqrtWeightScaler(2.5);
  qDebug() << "wFunc->sqrtWeightScaler(2.5):  " << temp;
  qDebug() << "";
  qDebug() << "";



  qDebug() << "Reassign object using operator= and passing Welsch with TC = 2.0:";
  wFunc = MaximumLikelihoodWFunctions(MaximumLikelihoodWFunctions::stringToModel("Welsch"), 2.0);
  qDebug() << "Model                    = "
           << MaximumLikelihoodWFunctions::modelToString(wFunc.model());
  qDebug() << "TweakingConstant         = " << QString::number(wFunc.tweakingConstant());
  qDebug() << "WeightedResidualCutoff   = " << wFunc.weightedResidualCutoff();
  qDebug() << "TweakingConstantQuantile = " << QString::number(wFunc.tweakingConstantQuantile());
  temp = wFunc.sqrtWeightScaler(-0.5);
  qDebug() << "wFunc->sqrtWeightScaler(-0.5): " << temp;
  temp = wFunc.sqrtWeightScaler(0.75);
  qDebug() << "wFunc->sqrtWeightScaler(0.75): " << temp;
  temp = wFunc.sqrtWeightScaler(-2.0);
  qDebug() << "wFunc->sqrtWeightScaler(-2):   " << temp;
  temp = wFunc.sqrtWeightScaler(2.5);
  qDebug() << "wFunc->sqrtWeightScaler(2.5):  " << temp;
  qDebug() << "";

  wFunc.setTweakingConstantDefault();
  qDebug() << "TC constant set to default:";
  qDebug() << "Model                    = "
           << MaximumLikelihoodWFunctions::modelToString(wFunc.model());
  qDebug() << "TweakingConstant         = " << QString::number(wFunc.tweakingConstant());
  qDebug() << "WeightedResidualCutoff   = " << wFunc.weightedResidualCutoff();
  qDebug() << "TweakingConstantQuantile = " << QString::number(wFunc.tweakingConstantQuantile());
  temp = wFunc.sqrtWeightScaler(-0.5);
  qDebug() << "wFunc->sqrtWeightScaler(-0.5): " << temp;
  temp = wFunc.sqrtWeightScaler(0.75);
  qDebug() << "wFunc->sqrtWeightScaler(0.75): " << temp;
  temp = wFunc.sqrtWeightScaler(-2.0);
  qDebug() << "wFunc->sqrtWeightScaler(-2):   " << temp;
  temp = wFunc.sqrtWeightScaler(2.5);
  qDebug() << "wFunc->sqrtWeightScaler(2.5):  " << temp;
  qDebug() << "";

  wFunc.setModel(MaximumLikelihoodWFunctions::Welsch);
  qDebug() << "Model manually set with default TC:";
  qDebug() << "Model                    = "
           << MaximumLikelihoodWFunctions::modelToString(wFunc.model());
  qDebug() << "TweakingConstant         = " << QString::number(wFunc.tweakingConstant());
  qDebug() << "WeightedResidualCutoff   = " << wFunc.weightedResidualCutoff();
  qDebug() << "TweakingConstantQuantile = " << QString::number(wFunc.tweakingConstantQuantile());
  temp = wFunc.sqrtWeightScaler(-0.5);
  qDebug() << "wFunc->sqrtWeightScaler(-0.5): " << temp;
  temp = wFunc.sqrtWeightScaler(0.75);
  qDebug() << "wFunc->sqrtWeightScaler(0.75): " << temp;
  temp = wFunc.sqrtWeightScaler(-2.0);
  qDebug() << "wFunc->sqrtWeightScaler(-2):   " << temp;
  temp = wFunc.sqrtWeightScaler(2.5);
  qDebug() << "wFunc->sqrtWeightScaler(2.5):  " << temp;
  qDebug() << "";

  wFunc.setModel(MaximumLikelihoodWFunctions::Welsch, 2.0);
  qDebug() << "Model manually set with TC = 2.0:";
  qDebug() << "Model                    = "
           << MaximumLikelihoodWFunctions::modelToString(wFunc.model());
  qDebug() << "TweakingConstant         = " << QString::number(wFunc.tweakingConstant());
  qDebug() << "WeightedResidualCutoff   = " << wFunc.weightedResidualCutoff();
  qDebug() << "TweakingConstantQuantile = " << QString::number(wFunc.tweakingConstantQuantile());
  temp = wFunc.sqrtWeightScaler(-0.5);
  qDebug() << "wFunc->sqrtWeightScaler(-0.5): " << temp;
  temp = wFunc.sqrtWeightScaler(0.75);
  qDebug() << "wFunc->sqrtWeightScaler(0.75): " << temp;
  temp = wFunc.sqrtWeightScaler(-2.0);
  qDebug() << "wFunc->sqrtWeightScaler(-2):   " << temp;
  temp = wFunc.sqrtWeightScaler(2.5);
  qDebug() << "wFunc->sqrtWeightScaler(2.5):  " << temp;
  qDebug() << "";
  qDebug() << "";




  qDebug() << "Reassign object using operator= and passing Chen with default TC:";
  wFunc = MaximumLikelihoodWFunctions(MaximumLikelihoodWFunctions::stringToModel("Chen"));
  qDebug() << "Model                    = "
           << MaximumLikelihoodWFunctions::modelToString(wFunc.model());
  qDebug() << "TweakingConstant         = " << QString::number(wFunc.tweakingConstant());
  qDebug() << "WeightedResidualCutoff   = " << wFunc.weightedResidualCutoff();
  qDebug() << "TweakingConstantQuantile = " << QString::number(wFunc.tweakingConstantQuantile());
  temp = wFunc.sqrtWeightScaler(-0.5);
  qDebug() << "wFunc->sqrtWeightScaler(-0.5): " << temp;
  temp = wFunc.sqrtWeightScaler(0.75);
  qDebug() << "wFunc->sqrtWeightScaler(0.75): " << temp;
  temp = wFunc.sqrtWeightScaler(-2.0);
  qDebug() << "wFunc->sqrtWeightScaler(-2):   " << temp;
  temp = wFunc.sqrtWeightScaler(2.5);
  qDebug() << "wFunc->sqrtWeightScaler(2.5):  " << temp;
  qDebug() << "";

  wFunc.setTweakingConstant(2.0);
  qDebug() << "TC constant re-set to 2.0:";
  qDebug() << "Model                    = "
           << MaximumLikelihoodWFunctions::modelToString(wFunc.model());
  qDebug() << "TweakingConstant         = " << QString::number(wFunc.tweakingConstant());
  qDebug() << "WeightedResidualCutoff   = " << wFunc.weightedResidualCutoff();
  qDebug() << "TweakingConstantQuantile = " << QString::number(wFunc.tweakingConstantQuantile());
  temp = wFunc.sqrtWeightScaler(-0.5);
  qDebug() << "wFunc->sqrtWeightScaler(-0.5): " << temp;
  temp = wFunc.sqrtWeightScaler(0.75);
  qDebug() << "wFunc->sqrtWeightScaler(0.75): " << temp;
  temp = wFunc.sqrtWeightScaler(-2.0);
  qDebug() << "wFunc->sqrtWeightScaler(-2):   " << temp;
  temp = wFunc.sqrtWeightScaler(2.5);
  qDebug() << "wFunc->sqrtWeightScaler(2.5):  " << temp;
  qDebug() << "";

  wFunc.setTweakingConstantDefault();
  qDebug() << "TC constant re-set to default:";
  qDebug() << "Model                    = "
           << MaximumLikelihoodWFunctions::modelToString(wFunc.model());
  qDebug() << "TweakingConstant         = " << QString::number(wFunc.tweakingConstant());
  qDebug() << "WeightedResidualCutoff   = " << wFunc.weightedResidualCutoff();
  qDebug() << "TweakingConstantQuantile = " << QString::number(wFunc.tweakingConstantQuantile());
  temp = wFunc.sqrtWeightScaler(-0.5);
  qDebug() << "wFunc->sqrtWeightScaler(-0.5): " << temp;
  temp = wFunc.sqrtWeightScaler(0.75);
  qDebug() << "wFunc->sqrtWeightScaler(0.75): " << temp;
  temp = wFunc.sqrtWeightScaler(-2.0);
  qDebug() << "wFunc->sqrtWeightScaler(-2):   " << temp;
  temp = wFunc.sqrtWeightScaler(2.5);
  qDebug() << "wFunc->sqrtWeightScaler(2.5):  " << temp;
  qDebug() << "";

  wFunc.setModel(MaximumLikelihoodWFunctions::Chen);
  qDebug() << "Model manually set with default TC:";
  qDebug() << "Model                    = "
           << MaximumLikelihoodWFunctions::modelToString(wFunc.model());
  qDebug() << "TweakingConstant         = " << QString::number(wFunc.tweakingConstant());
  qDebug() << "WeightedResidualCutoff   = " << wFunc.weightedResidualCutoff();
  qDebug() << "TweakingConstantQuantile = " << QString::number(wFunc.tweakingConstantQuantile());
  temp = wFunc.sqrtWeightScaler(-0.5);
  qDebug() << "wFunc->sqrtWeightScaler(-0.5): " << temp;
  temp = wFunc.sqrtWeightScaler(0.75);
  qDebug() << "wFunc->sqrtWeightScaler(0.75): " << temp;
  temp = wFunc.sqrtWeightScaler(-2.0);
  qDebug() << "wFunc->sqrtWeightScaler(-2):   " << temp;
  temp = wFunc.sqrtWeightScaler(2.5);
  qDebug() << "wFunc->sqrtWeightScaler(2.5):  " << temp;
  qDebug() << "";

  wFunc.setModel(MaximumLikelihoodWFunctions::Chen, 2.0);
  qDebug() << "Model manually set with TC = 2.0:";
  qDebug() << "Model                    = "
           << MaximumLikelihoodWFunctions::modelToString(wFunc.model());
  qDebug() << "TweakingConstant         = " << QString::number(wFunc.tweakingConstant());
  qDebug() << "WeightedResidualCutoff   = " << wFunc.weightedResidualCutoff();
  qDebug() << "TweakingConstantQuantile = " << QString::number(wFunc.tweakingConstantQuantile());
  temp = wFunc.sqrtWeightScaler(-0.5);
  qDebug() << "wFunc->sqrtWeightScaler(-0.5): " << temp;
  temp = wFunc.sqrtWeightScaler(0.75);
  qDebug() << "wFunc->sqrtWeightScaler(0.75): " << temp;
  temp = wFunc.sqrtWeightScaler(-2.0);
  qDebug() << "wFunc->sqrtWeightScaler(-2):   " << temp;
  temp = wFunc.sqrtWeightScaler(2.5);
  qDebug() << "wFunc->sqrtWeightScaler(2.5):  " << temp;
  qDebug() << "";
  qDebug() << "";


  qDebug() << "Testing serialization...";
  qDebug() << "Previous class written and read from QByteArray:";
  QByteArray byteArray;
  QDataStream outputData(&byteArray, QIODevice::WriteOnly);
  outputData << wFunc;
  QDataStream inputData(byteArray);
  MaximumLikelihoodWFunctions newWFunc;
  inputData >> newWFunc;
  qDebug() << "Model                    = "
           << MaximumLikelihoodWFunctions::modelToString(wFunc.model());
  qDebug() << "TweakingConstant         = " << QString::number(wFunc.tweakingConstant());
  qDebug() << "WeightedResidualCutoff   = " << wFunc.weightedResidualCutoff();
  qDebug() << "TweakingConstantQuantile = " << QString::number(wFunc.tweakingConstantQuantile());
  temp = wFunc.sqrtWeightScaler(-0.5);
  qDebug() << "wFunc->sqrtWeightScaler(-0.5): " << temp;
  temp = wFunc.sqrtWeightScaler(0.75);
  qDebug() << "wFunc->sqrtWeightScaler(0.75): " << temp;
  temp = wFunc.sqrtWeightScaler(-2.0);
  qDebug() << "wFunc->sqrtWeightScaler(-2):   " << temp;
  temp = wFunc.sqrtWeightScaler(2.5);
  qDebug() << "wFunc->sqrtWeightScaler(2.5):  " << temp;
  qDebug() << "";
  qDebug() << "Huber enum written to and read from QByteArray:";
  QByteArray enumByteArray;
  QDataStream outputEnumData(&enumByteArray, QIODevice::WriteOnly);
  outputEnumData << MaximumLikelihoodWFunctions::stringToModel("Huber");
  QDataStream inputEnumData(enumByteArray);
  MaximumLikelihoodWFunctions::Model model;
  inputEnumData >> model;
  qDebug() << "Model                    = " << MaximumLikelihoodWFunctions::modelToString(model);
  qDebug() << "";
  qDebug() << "";

  qDebug() << "Testing error throws...";
  try {
    wFunc.setTweakingConstant(-1.0);
  }
  catch (IException &e) {
    e.print();
  }
  try {
    MaximumLikelihoodWFunctions::modelToString((MaximumLikelihoodWFunctions::Model)4);
  }
  catch (IException &e) {
    e.print();
  }
  try {
    MaximumLikelihoodWFunctions::stringToModel("Nonsense");
  }
  catch (IException &e) {
    e.print();
  }

}
