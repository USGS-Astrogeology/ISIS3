/** This is free and unencumbered software released into the public domain.
The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */
#include "Isis.h"

#include <QFile>

#include "ProcessImportVicar.h"
#include "Application.h"
#include "Cube.h"
#include "FileName.h"
#include "Statistics.h"

using namespace Isis;
using namespace std;

void IsisMain() {

  Preference::Preferences(true);

  ProcessImportVicar p;
  Pvl vlab;
  p.SetVicarFile(QString::fromStdString(FileName("$ISISTESTDATA/isis/src/base/objs/ProcessImportVicar/unitTest.img").expanded()), vlab);
  p.SetOutputCube("TO");
  p.StartProcess();
  p.EndProcess();

  cout << vlab << endl;
  Process p2;
  CubeAttributeInput att;
  QString file = Application::GetUserInterface().GetCubeName("TO");
  Cube *icube = p2.SetInputCube(file, att);
  Statistics *stat = icube->statistics();
  cout << stat->Average() << endl;
  cout << stat->Variance() << endl;
  p2.EndProcess();
  QFile::remove(file);
}
