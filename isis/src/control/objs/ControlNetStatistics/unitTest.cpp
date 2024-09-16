/** This is free and unencumbered software released into the public domain.

The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */

#include "Isis.h"
#include "Application.h"
#include "ControlNet.h"
#include "ControlPoint.h"
#include "ControlMeasure.h"
#include "ControlNetStatistics.h"
#include "IException.h"
#include "Preference.h"
#include "PvlGroup.h"
#include "PvlKeyword.h"
#include "SpecialPixel.h"
#include "TextFile.h"

#include <string>
#include <iostream>
#include <stdio.h>
#include <stdlib.h>

using namespace std;
using namespace Isis;

void IsisMain() {
  Preference::Preferences(true);
  cout << "UnitTest for ControlNetStatistics ...." << endl << endl;

  UserInterface &ui = Application::GetUserInterface();

  cout << "CNET=" << ui.GetAsString("CNET").toStdString() << endl;
  cout << "Serial File=" << ui.GetAsString("FROMLIST").toStdString() << endl;

  ControlNet cnetOrig(ui.GetFileName("CNET"));
  ControlNet cnet = cnetOrig;

  QString sSerialFile = ui.GetFileName("FROMLIST");
  ControlNetStatistics cnetStats(&cnet, sSerialFile);

  PvlGroup statsGrp;
  cnetStats.GenerateControlNetStats(statsGrp);

  cout << statsGrp << endl;

}
