/** This is free and unencumbered software released into the public domain.
The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */
#include "Isis.h"

#include <iostream>
#include <string>
#include <cmath>

#include <QFile>

#include "Cube.h"
#include "CubeAttribute.h"
#include "IString.h"
#include "ProcessByLine.h"
#include "PvlGroup.h"
#include "Reduce.h"
#include "UserInterface.h"

using namespace std;
using namespace Isis;

void IsisMain() {
  Preference::Preferences(true);
  ProcessByLine p;
  UserInterface &ui = Application::GetUserInterface();
  vector<QString> bands;

  p.SetInputCube("FROM");
  Cube icube;

  CubeAttributeInput cai(ui.GetAsString("FROM").toStdString());
  bands = cai.bands();

  icube.setVirtualBands(bands);
  icube.open(ui.GetCubeName("FROM"));

  double sscale = 3;
  double lscale = 4;
  int ons = (int)ceil((double)icube.sampleCount() / sscale);
  int onl = (int)ceil((double)icube.lineCount() / lscale);

  // Reduce by "Near"
  Cube *ocube = p.SetOutputCube("TO", ons, onl, icube.bandCount());
  Nearest near(&icube, sscale, lscale);
  p.ClearInputCubes();
  cout << "Reduce by Near\n";
  p.ProcessCubeInPlace(near, false);
  PvlGroup results = near.UpdateOutputLabel(ocube);
  p.Finalize();
  cout << results << endl;

  // Reduce by "Average"
  p.SetInputCube("FROM");
  ocube=p.SetOutputCube("TO2", ons, onl, icube.bandCount());
  p.ClearInputCubes();
  Average avg(&icube, sscale, lscale, 0.5, "scale");
  cout << "\nReduce by Average\n";
  p.ProcessCubeInPlace(avg, false);
  results = avg.UpdateOutputLabel(ocube);
  cout << results << endl;

  p.Finalize();
  icube.close();
  QFile::remove(ui.GetCubeName("TO"));
  QFile::remove(ui.GetCubeName("TO2"));
}
