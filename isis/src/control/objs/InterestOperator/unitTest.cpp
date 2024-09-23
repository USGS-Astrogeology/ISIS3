/** This is free and unencumbered software released into the public domain.

The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */

#include <iostream>
#include <cstdlib>
#include "InterestOperator.h"
#include "InterestOperatorFactory.h"
#include "StandardDeviationOperator.h"
#include "Chip.h"
#include "Cube.h"
#include "Pvl.h"
#include "PvlGroup.h"
#include "Preference.h"
#include "UniversalGroundMap.h"
#include "ControlNetValidMeasure.h"

using namespace Isis;

int main() {
  Isis::Preference::Preferences(true);
  try {
    PvlGroup op("Operator");
    op += PvlKeyword("Name", "StandardDeviation");
    op += PvlKeyword("DeltaLine", toString(100));
    op += PvlKeyword("DeltaSamp", toString(100));
    op += PvlKeyword("Samples", toString(15));
    op += PvlKeyword("Lines", toString(15));
    op += PvlKeyword("MinimumInterest", toString(0.01));

    PvlGroup opv("ValidMeasure");
    opv += PvlKeyword("MinDN", toString(0.0));
    opv += PvlKeyword("MaxDN", toString(1.0));
    opv += PvlKeyword("MinEmission", toString(15.0));
    opv += PvlKeyword("MaxEmission", toString(25.0));
    opv += PvlKeyword("MinIncidence", toString(0.0));
    opv += PvlKeyword("MaxIncidence", toString(135.0));
    //op += PvlKeyword("MinResolution", 100);
    //op += PvlKeyword("MaxResolution", 300);

    PvlObject o("InterestOperator");
    o.addGroup(op);
    o.addGroup(opv);

    Pvl pvl;
    pvl.addObject(o);
    std::cout << pvl << std::endl;

    InterestOperator *iop = InterestOperatorFactory::Create(pvl);

    Cube c;
    c.open("$ISISTESTDATA/isis/src/mgs/unitTestData/ab102401.cub");

    UniversalGroundMap univGrndMap(c);
    iop->Operate(c,  univGrndMap, 100, 350);

    std::cout << "Sample: " << iop->CubeSample() << std::endl
              << "Line : " << iop->CubeLine() << std::endl
              << "Interest: " << iop->InterestAmount() << std::endl;
  }
  catch (IException &e) {
    e.print();
  }



  return 0;
}
