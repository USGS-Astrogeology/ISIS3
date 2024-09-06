/** This is free and unencumbered software released into the public domain.

The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */

#include "Isis.h"

#include "Cube.h"
#include "IException.h"
#include "Pipeline.h"
#include "UserInterface.h"

using namespace std;
using namespace Isis;

void IsisMain() {
  UserInterface &ui = Application::GetUserInterface();

  // Check that it is a Mariner10 cube.
  Cube iCube;
  iCube.open(ui.GetCubeName("FROM"));
  Pvl * labels = iCube.label();
  if ("Mariner_10" !=  QString::fromStdString(labels->findKeyword("SpacecraftName", Pvl::Traverse))) {
    std::string msg = "The cube [" + ui.GetCubeName("FROM") + "] does not appear" +
        " to be a Mariner10 cube";
    throw IException(IException::User, msg, _FILEINFO_);
  }

  // Open the input cube
  Pipeline p("mar10nonoise");
  p.SetInputFile("FROM");
  p.SetOutputFile("TO");
  p.KeepTemporaryFiles(!ui.GetBoolean("REMOVE"));

  // Salt filter 1
  p.AddToPipeline("noisefilter", "saltRemoval1");
  p.Application("saltRemoval1").SetInputParameter("FROM", true);
  p.Application("saltRemoval1").SetOutputParameter("TO", "1salt1");
  p.Application("saltRemoval1").AddConstParameter("SAMP", "3");
  p.Application("saltRemoval1").AddConstParameter("LINE", "3");
  p.Application("saltRemoval1").AddConstParameter("MINIMUM", "4");
  p.Application("saltRemoval1").AddConstParameter("TOLMIN", "300");
  p.Application("saltRemoval1").AddConstParameter("TOLMAX", "35");
  p.Application("saltRemoval1").AddConstParameter("REPLACE", "null");

  // Pepper filter 1
  p.AddToPipeline("noisefilter", "pepperRemoval1");
  p.Application("pepperRemoval1").SetInputParameter("FROM", true);
  p.Application("pepperRemoval1").SetOutputParameter("TO", "2pepp1");
  p.Application("pepperRemoval1").AddConstParameter("SAMP", "3");
  p.Application("pepperRemoval1").AddConstParameter("LINE", "3");
  p.Application("pepperRemoval1").AddConstParameter("MINIMUM", "4");
  p.Application("pepperRemoval1").AddConstParameter("TOLMIN", "30");
  p.Application("pepperRemoval1").AddConstParameter("TOLMAX", "300");
  p.Application("pepperRemoval1").AddConstParameter("REPLACE", "null");

  // Salt filter 2
  p.AddToPipeline("noisefilter", "saltRemoval2");
  p.Application("saltRemoval2").SetInputParameter("FROM", true);
  p.Application("saltRemoval2").SetOutputParameter("TO", "3salt2");
  p.Application("saltRemoval2").AddConstParameter("SAMP", "3");
  p.Application("saltRemoval2").AddConstParameter("LINE", "3");
  p.Application("saltRemoval2").AddConstParameter("TOLDEF", "stddev");
  p.Application("saltRemoval2").AddConstParameter("FLATTOL", "15");
  p.Application("saltRemoval2").AddConstParameter("MINIMUM", "4");
  p.Application("saltRemoval2").AddConstParameter("TOLMIN", "300");
  p.Application("saltRemoval2").AddConstParameter("TOLMAX", "2");
  p.Application("saltRemoval2").AddConstParameter("REPLACE", "null");

  // Pepper filter 2
  p.AddToPipeline("noisefilter", "pepperRemoval2");
  p.Application("pepperRemoval2").SetInputParameter("FROM", true);
  p.Application("pepperRemoval2").SetOutputParameter("TO", "4pepp2");
  p.Application("pepperRemoval2").AddConstParameter("SAMP", "3");
  p.Application("pepperRemoval2").AddConstParameter("LINE", "3");
  p.Application("pepperRemoval2").AddConstParameter("TOLDEF", "stddev");
  p.Application("pepperRemoval2").AddConstParameter("FLATTOL", "15");
  p.Application("pepperRemoval2").AddConstParameter("MINIMUM", "4");
  p.Application("pepperRemoval2").AddConstParameter("TOLMIN", "2");
  p.Application("pepperRemoval2").AddConstParameter("TOLMAX", "300");
  p.Application("pepperRemoval2").AddConstParameter("REPLACE", "null");

  // Salt filter 3
  p.AddToPipeline("noisefilter", "saltRemoval3");
  p.Application("saltRemoval3").SetInputParameter("FROM", true);
  p.Application("saltRemoval3").SetOutputParameter("TO", "5salt3");
  p.Application("saltRemoval3").AddConstParameter("SAMP", "5");
  p.Application("saltRemoval3").AddConstParameter("LINE", "5");
  p.Application("saltRemoval3").AddConstParameter("TOLDEF", "stddev");
  p.Application("saltRemoval3").AddConstParameter("FLATTOL", "12");
  p.Application("saltRemoval3").AddConstParameter("MINIMUM", "15");
  p.Application("saltRemoval3").AddConstParameter("TOLMIN", "300");
  p.Application("saltRemoval3").AddConstParameter("TOLMAX", "1.9");
  p.Application("saltRemoval3").AddConstParameter("REPLACE", "null");

  // Pepper filter 3
  p.AddToPipeline("noisefilter", "pepperRemoval3");
  p.Application("pepperRemoval3").SetInputParameter("FROM", true);
  p.Application("pepperRemoval3").SetOutputParameter("TO", "6pepp3");
  p.Application("pepperRemoval3").AddConstParameter("SAMP", "5");
  p.Application("pepperRemoval3").AddConstParameter("LINE", "5");
  p.Application("pepperRemoval3").AddConstParameter("TOLDEF", "stddev");
  p.Application("pepperRemoval3").AddConstParameter("FLATTOL", "12");
  p.Application("pepperRemoval3").AddConstParameter("MINIMUM", "15");
  p.Application("pepperRemoval3").AddConstParameter("TOLMIN", "1.9");
  p.Application("pepperRemoval3").AddConstParameter("TOLMAX", "300");
  p.Application("pepperRemoval3").AddConstParameter("REPLACE", "null");

  p.Run();
}
