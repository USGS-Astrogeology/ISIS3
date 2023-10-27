/** This is free and unencumbered software released into the public domain.
The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */
#include <iostream>
#include <cstdlib>

#include "AutoReg.h"
#include "Gruen.h"
#include "Chip.h"
#include "Cube.h"
#include "Pvl.h"
#include "PvlGroup.h"
#include "Preference.h"

using namespace Isis;

int main() {
  Isis::Preference::Preferences(true);

  try {
    PvlGroup alg("Algorithm");
    alg += PvlKeyword("Name", "Gruen");
    alg += PvlKeyword("Tolerance", std::to_string(100.0));
    alg += PvlKeyword("AffineTranslationTolerance", std::to_string(0.15));
    alg += PvlKeyword("AffineScaleTolerance", std::to_string(0.3));
    alg += PvlKeyword("MaximumIterations", std::to_string(30));

    PvlGroup pchip("PatternChip");
    pchip += PvlKeyword("Samples", std::to_string(19));
    pchip += PvlKeyword("Lines", std::to_string(19));

    PvlGroup schip("SearchChip");
    schip += PvlKeyword("Samples", std::to_string(25));
    schip += PvlKeyword("Lines", std::to_string(25));

    PvlObject o("AutoRegistration");
    o.addGroup(alg);
    o.addGroup(pchip);
    o.addGroup(schip);

    Pvl pvl;
    pvl.addObject(o);
    std::cout << pvl << std::endl;

    Gruen gruen(pvl);

    Cube p;
    p.open("$ISISTESTDATA/isis/src/messenger/unitTestData/EW0131770376G.equi.cub");

    Cube s;
    s.open("$ISISTESTDATA/isis/src/messenger/unitTestData/EW0131770381F.equi.cub");

    gruen.SearchChip()->TackCube(512.0, 512.0);
    gruen.SearchChip()->Load(s);
    gruen.PatternChip()->TackCube(512.0, 512.0);
    gruen.PatternChip()->Load(p);

    std::cout << "Register = " << gruen.Register() << std::endl;
    std::cout << "Position = " << gruen.CubeSample() << " " <<
              gruen.CubeLine() << std::endl;


#if defined(FULL_DISCLOSURE)
    Pvl pstat = gruen.RegistrationStatistics();
    std::cout << "\n\n" << pstat << std::endl;
#endif


  }
  catch(IException &e) {
    e.print();
  }

  return 0;
}
