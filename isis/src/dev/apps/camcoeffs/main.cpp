/** This is free and unencumbered software released into the public domain.

The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */

#include "Isis.h"

#include <string>
#include <cmath>

#include "IException.h"

using namespace std;
using namespace Isis;

void IsisMain() {
  UserInterface &ui = Application::GetUserInterface();

  double eq1[] = {
    ui.GetDouble("XCONSTCOEF"),
    ui.GetDouble("XSAMPLECOEF"),
    ui.GetDouble("XLINECOEF"),
  };
  double eq2[] = {
    ui.GetDouble("YCONSTCOEF"),
    ui.GetDouble("YSAMPLECOEF"),
    ui.GetDouble("YLINECOEF"),
  };

  double res1[3] = {0.0, 0.0, 0.0};
  double res2[3] = {0.0, 0.0, 0.0};

  bool solutionFound = false;

  /**
   * Do this loop in order to shorten the number of solutions we have
   *  to program in. This halves the number of cases by saying "What if equation 1
   *  is really equation 2, and equation 2 is really equation 1?" The solution
   *  will also be flipped, which is handled if order becomes 1.
   *
   *  order = 0 means the equations are in their original form, order = 1 means
   *  they were swapped.
   */
  for(int order = 0; !solutionFound && order < 2; order++) {
    double A = eq1[0];
    double B = eq1[1];
    double C = eq1[2];
    double D = eq2[0];
    double E = eq2[1];
    double F = eq2[2];

    if(order == 1) {
      A = eq2[0];
      B = eq2[1];
      C = eq2[2];
      D = eq1[0];
      E = eq1[1];
      F = eq1[2];
    }

    solutionFound = true;

    // These are used to test solution dependencies, they become zero if the
    //   equations are parallel and thus unsolvable
    double denomX = (F == 0) ? 0.0 : B - (E * C / F);
    double denomY = (E == 0) ? 0.0 : C - (F * B / E);
    if(B != 0 && E != 0 && F != 0 && denomX != 0 && denomY != 0) {
      /**
       * Input Equations:
       * X = A + BS + CL
       * Y = D + ES + FL
       *
       * Dependencies:
       *   B != 0, E != 0, F != 0, (B-EC/F) != 0, (C-FB/E) != 0
       *
       * Inverses:
       * S = ((DC/F-A)/(B-EC/F)) + (1/(B-EC/F))X + ((-C/F)/(B-EC/F))Y
       * L = (DB/E-A)/(C-FB/E) + (1/(C-FB/E))X + ((-B/E)/(C-FB/E))Y
       */
      res1[0] = (D * C / F - A) / denomX;
      res1[1] = 1.0 / denomX;
      res1[2] = -(C / F) / denomX;
      res2[0] = (D * B / E - A) / denomY;
      res2[1] = 1.0 / denomY;
      res2[2] = (-B / E) / denomY;
    }
    else if(C != 0 && E != 0 && B == 0) {
      /**
       * Input Equations:
       * X = A + CL
       * Y = D + ES + FL
       *
       * Dependencies:
       *   C != 0, E != 0, B == 0
       *
       * Inverses:
       * S = ((FA)/(CE) - D/E) + (-F/(CE))X + (1/E)Y
       * L = (-A/C) + (1/C)X + 0.0Y
       */
      res1[0] = (F * A) / (C * E) - D / E;
      res1[1] = -F / (C * E);
      res1[2] = 1.0 / E;
      res2[0] = -A / C;
      res2[1] = 1.0 / C;
      res2[2] = 0.0;
    }
    else {
      solutionFound = false;
    }

    // If we found a swapped sol'n, the x coefficient is really
    // the y coefficient at this point. The constants are correct.
    if(order == 1 && solutionFound) {
      double tmp = res1[1];
      res1[1] = res1[2];
      res1[2] = tmp;

      tmp = res2[1];
      res2[1] = res2[2];
      res2[2] = tmp;
    }
  }

  if(!solutionFound) {
    throw IException(IException::Unknown, "Not enough information", _FILEINFO_);
  }

  QString inEquationX = "X = " + QString::number(eq1[0]);
  inEquationX += " + " + QString::number(eq1[1]) + "S";
  inEquationX += " + " + QString::number(eq1[2]) + "L";
  QString inEquationY = "Y = " + QString::number(eq2[0]);
  inEquationY += " + " + QString::number(eq2[1]) + "S";
  inEquationY += " + " + QString::number(eq2[2]) + "L";
  QString outEquationS = "S = " + QString::number(res1[0]);
  outEquationS += " + " + QString::number(res1[1]) + "X";
  outEquationS += " + " + QString::number(res1[2]) + "Y";
  QString outEquationL = "L = " + QString::number(res2[0]);
  outEquationL += " + " + QString::number(res2[1]) + "X";
  outEquationL += " + " + QString::number(res2[2]) + "Y";

  // check....
  /*
  double rndS = 12;
  double rndL = 534;

  double x = eq1[0] + rndS*eq1[1] + rndL*eq1[2];
  double y = eq2[0] + rndS*eq2[1] + rndL*eq2[2];
  double s = res1[0] + x*res1[1] + y*res1[2];
  double l = res2[0] + x*res2[1] + y*res2[2];

  if(fabs(rndS - s) > 1E-12 || fabs(rndL - l) > 1E-12) {
    std::cerr << "Equation Fails!" << std::endl;
    std::cerr << "Differences: " << fabs(rndS - s) << "," << fabs(rndL - l) << std::endl;
  }
  */

  PvlGroup res("Results");

  if(ui.WasEntered("IAKCODE")) {
    PvlKeyword naifFormatX("INS" + ui.GetString("IAKCODE").toStdString() + "_TRANSX");
    naifFormatX += toString(eq1[0]);
    naifFormatX += toString(eq1[1]);
    naifFormatX += toString(eq1[2]);
    PvlKeyword naifFormatY("INS" + ui.GetString("IAKCODE").toStdString() + "_TRANSY");
    naifFormatY += toString(eq2[0]);
    naifFormatY += toString(eq2[1]);
    naifFormatY += toString(eq2[2]);
    PvlKeyword naifFormatS("INS" + ui.GetString("IAKCODE").toStdString() + "_ITRANSS");
    naifFormatS += toString(res1[0]);
    naifFormatS += toString(res1[1]);
    naifFormatS += toString(res1[2]);
    PvlKeyword naifFormatL("INS" + ui.GetString("IAKCODE").toStdString() + "_ITRANSL");
    naifFormatL += toString(res2[0]);
    naifFormatL += toString(res2[1]);
    naifFormatL += toString(res2[2]);

    res += naifFormatX;
    res += naifFormatY;
    res += naifFormatS;
    res += naifFormatL;
  }
  else {
    res += PvlKeyword("EquationX", inEquationX.toStdString());
    res += PvlKeyword("EquationY", inEquationY.toStdString());
    res += PvlKeyword("EquationS", outEquationS.toStdString());
    res += PvlKeyword("EquationL", outEquationL.toStdString());
  }

  Application::Log(res);
}
