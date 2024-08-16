/** This is free and unencumbered software released into the public domain.

The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */

#include "Isis.h"

#include "Chip.h"
#include "Cube.h"
#include "IException.h"
#include "Pipeline.h"
#include "Statistics.h"

using namespace std;
using namespace Isis;

void IsisMain() {
  UserInterface &ui = Application::GetUserInterface();

  // Check that it is a Mariner10 cube.
  Cube iCube;
  iCube.open(ui.GetCubeName("FROM"));
  Pvl * labels = iCube.label();
  if ("Mariner_10" !=  QString::fromStdString(labels->findKeyword("SpacecraftName", Pvl::Traverse))) {
    QString msg = "The cube [" + ui.GetCubeName("FROM") + "] does not appear" +
        " to be a Mariner10 cube";
    throw IException(IException::User, msg, _FILEINFO_);
  }

  // Check that the cube actually needs cleaning. This verifies that it
  // wasn't a "compressed" cube and that it hasn't been cleaned.
  Chip cp(5, 5);
  cp.TackCube(2.5, 2.5);
  cp.Load(iCube);
  Statistics *stats = NULL;
  stats = cp.Statistics();
  cout << "Valid pixels: "<< stats->ValidPixels() << endl;
  if (stats->ValidPixels() == 7) {
    QString msg = "The cube [" + ui.GetCubeName("FROM") + "] needs" +
      " reconstruction, try mar10restore instead";
    throw IException(IException::User, msg, _FILEINFO_);
  }
  else if (stats->ValidPixels() == 0) {
    QString msg = "The cube [" + ui.GetCubeName("FROM") + "]" +
      " appears to have already been cleaned";
    throw IException(IException::User, msg, _FILEINFO_);
  }
  if (stats != NULL) {
    delete stats;
    stats = NULL;
  }

  // Open the input cube
  Pipeline p("mar10clean");
  p.SetInputFile("FROM");
  p.SetOutputFile("TO");
  p.KeepTemporaryFiles(!ui.GetBoolean("REMOVE"));

  // Run mar10nonoise to remove noise
  p.AddToPipeline("mar10nonoise");
  p.Application("mar10nonoise").SetInputParameter("FROM", true);
  p.Application("mar10nonoise").SetOutputParameter("TO", "mar10nonoise");

  // Run findrx on the cube to find the actual position of the reseaus
  p.AddToPipeline("findrx");
  p.Application("findrx").SetInputParameter("FROM", false);

  // Run remrx on the cube to remove the reseaus
  p.AddToPipeline("remrx");
  p.Application("remrx").SetInputParameter("FROM", true);
  p.Application("remrx").SetOutputParameter("TO", "remrx");
  p.Application("remrx").AddParameter("SDIM", "SDIM");
  p.Application("remrx").AddParameter("LDIM", "LDIM");

  // Run a low pass filter on the null data in the cube
  p.AddToPipeline("lowpass", "pass1");
  p.Application("pass1").SetInputParameter("FROM", true);
  p.Application("pass1").SetOutputParameter("TO", "lowpass1");
  p.Application("pass1").AddConstParameter("SAMP", "3");
  p.Application("pass1").AddConstParameter("LINE", "3");
  p.Application("pass1").AddConstParameter("MINIMUM", "4");
  p.Application("pass1").AddConstParameter("FILTER", "outside");

  // Run a low pass filter on the null data in the cube
  p.AddToPipeline("lowpass", "pass2");
  p.Application("pass2").SetInputParameter("FROM", true);
  p.Application("pass2").SetOutputParameter("TO", "lowpass2");
  p.Application("pass2").AddConstParameter("SAMP", "3");
  p.Application("pass2").AddConstParameter("LINE", "3");
  p.Application("pass2").AddConstParameter("MINIMUM", "4");
  p.Application("pass2").AddConstParameter("FILTER", "outside");

  // Run trim to remove data outside of visual frame
  p.AddToPipeline("trim");
  p.Application("trim").SetInputParameter("FROM", true);
  p.Application("trim").SetOutputParameter("TO", "trim");
  p.Application("trim").AddConstParameter("TOP", "5");
  p.Application("trim").AddConstParameter("LEFT", "11");
  p.Application("trim").AddConstParameter("RIGHT", "8");

  cout << p;
  p.Run();
}
