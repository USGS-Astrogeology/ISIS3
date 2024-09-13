/** This is free and unencumbered software released into the public domain.
The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */
#include <QVector>

#include "CubeCalculator.h"
#include "ProcessByLine.h"
#include "CubeAttribute.h"
#include "LineManager.h"
#include "Preference.h"

using namespace Isis;

int main(int argc, char *argv[]) {
  Isis::Preference::Preferences(true);
  Isis::CubeCalculator c;
  Isis::ProcessByLine p;

  Isis::CubeAttributeInput att;

  Isis::Cube *icube = p.SetInputCube("$ISISTESTDATA/isis/src/base/unitTestData/CubeCalculator/unitTest.cub", att);
  std::cout << "CubeCalculator unit test" << std::endl;
  std::cout << "------------------------" << std::endl << std::endl;

  QString postfix = "sample line + -- band *";
  QVector<Isis::Cube *> iCubes;
  iCubes.push_back(icube);
  c.prepareCalculations(postfix, iCubes, icube);

  std::cout << "EQUATION: " << postfix.toStdString() << std::endl;

  Isis::LineManager mgr(*icube);
  mgr.SetLine(1);
  QVector<Buffer *> cubeData;
  cubeData.push_back(&mgr);

  while(!mgr.end()) {
    QVector<double> res = c.runCalculations(cubeData, mgr.Line(), mgr.Band());

    std::cout << "Line " << mgr.Line() << " Band " << mgr.Band() << std::endl;
    for(int i = 0; i < (int)res.size(); i++) std::cout << res[i] << std::endl;
    mgr ++;
  }
}
