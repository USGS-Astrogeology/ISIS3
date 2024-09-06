/** This is free and unencumbered software released into the public domain.

The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */

#include "Isis.h"

#include "Pipeline.h"

using namespace std;
using namespace Isis;

void IsisMain() {
  UserInterface &ui = Application::GetUserInterface();
  bool rmv = ui.GetBoolean("REMOVE");

  Pipeline pipeline("viknobutter");
  pipeline.SetInputFile("FROM");
  pipeline.SetOutputFile("TO");
  pipeline.KeepTemporaryFiles(!rmv);

  // Figure out which masking cube to use
  Pvl p(ui.GetCubeName("FROM").toStdString());
  PvlGroup &inst = p.findGroup("Instrument", Pvl::Traverse);
  int spn;
  QString scn = QString::fromStdString(inst["SpacecraftName"]);
  if(scn == "VIKING_ORBITER_1") spn = 1;
  else if(scn == "VIKING_ORBITER_2") spn = 2;
  else {
    std::string msg = "Invalid spacecraftname [" + scn + "]";
    throw IException(IException::Programmer, msg, _FILEINFO_);
  }
  // determine if # of cols is even or odd
  bool even = true;
  PvlGroup &arch = p.findGroup("Archive", Pvl::Traverse);
  QString id = QString::fromStdString(arch["ProductId"]);
  int num = toInt(id.mid(5, 1));
  if(num == 1 || num == 3 || num == 5 || num == 7 || num == 9) even = false;

  // Run a standard deviation filter on the cube
  pipeline.AddToPipeline("noisefilter");
  pipeline.Application("noisefilter").SetInputParameter("FROM", false);
  pipeline.Application("noisefilter").SetOutputParameter("TO", "step1");
  pipeline.Application("noisefilter").AddConstParameter("toldef", "stddev");
  pipeline.Application("noisefilter").AddConstParameter("flattol", "10");
  pipeline.Application("noisefilter").AddConstParameter("samp", "3");
  pipeline.Application("noisefilter").AddConstParameter("line", "3");
  pipeline.Application("noisefilter").AddConstParameter("minimum", "5");
  pipeline.Application("noisefilter").AddConstParameter("tolmin", "2.5");
  pipeline.Application("noisefilter").AddConstParameter("tolmax", "2.5");
  pipeline.Application("noisefilter").AddConstParameter("replace", "null");

  // Run a lowpass filter on the cube
  pipeline.AddToPipeline("lowpass", "lowpass1");
  pipeline.Application("lowpass1").SetInputParameter("FROM", false);
  pipeline.Application("lowpass1").SetOutputParameter("TO", "step2");
  pipeline.Application("lowpass1").AddConstParameter("samp", "3");
  pipeline.Application("lowpass1").AddConstParameter("line", "3");
  pipeline.Application("lowpass1").AddConstParameter("minimum", "5");
  pipeline.Application("lowpass1").AddConstParameter("filter", "outside");

  // Run mask on the cube with the correct masking cube
  pipeline.AddToPipeline("mask");
  pipeline.Application("mask").SetInputParameter("FROM", false);
  pipeline.Application("mask").SetOutputParameter("TO", "step3");

  QString maskParameter = "$ISISDATA/viking" + toString(spn) +
    "/calibration/vik" + toString(spn);
  if(even) {
    maskParameter += "evenMask.cub";
  }
  else {
    maskParameter += "oddMask.cub";
  }
  pipeline.Application("mask").AddConstParameter("mask", maskParameter);

  // Run a low pass filter on the invalid data in the cube
  pipeline.AddToPipeline("lowpass", "lowpass2");
  pipeline.Application("lowpass2").SetInputParameter("FROM", false);
  pipeline.Application("lowpass2").SetOutputParameter("TO", "step4");
  pipeline.Application("lowpass2").AddConstParameter("samp", "3");
  pipeline.Application("lowpass2").AddConstParameter("line", "3");
  pipeline.Application("lowpass2").AddConstParameter("filter", "outside");
  pipeline.Application("lowpass2").AddConstParameter("replace", "null");

  // Run a lowpass filter on the cube
  pipeline.AddToPipeline("lowpass", "lowpass3");
  pipeline.Application("lowpass3").SetInputParameter("FROM", false);
  pipeline.Application("lowpass3").SetOutputParameter("TO", "step5");
  pipeline.Application("lowpass3").AddConstParameter("samp", "7");
  pipeline.Application("lowpass3").AddConstParameter("line", "7");
  pipeline.Application("lowpass3").AddConstParameter("filter", "outside");
  pipeline.Application("lowpass3").AddConstParameter("replace", "null");

  // Run a lowpass filter on the cube
  pipeline.AddToPipeline("lowpass", "lowpass4");
  pipeline.Application("lowpass4").SetInputParameter("FROM", false);
  pipeline.Application("lowpass4").SetOutputParameter("TO", "step6");
  pipeline.Application("lowpass4").AddConstParameter("samp", "11");
  pipeline.Application("lowpass4").AddConstParameter("line", "11");
  pipeline.Application("lowpass4").AddConstParameter("filter", "outside");
  pipeline.Application("lowpass4").AddConstParameter("replace", "null");

  // Trim the cube
  pipeline.AddToPipeline("trim");
  pipeline.Application("trim").SetInputParameter("FROM", false);
  pipeline.Application("trim").SetOutputParameter("TO", "");
  pipeline.Application("trim").AddConstParameter("bottom", "20");
  pipeline.Application("trim").AddConstParameter("top", "25");
  pipeline.Application("trim").AddConstParameter("left", "30");
  pipeline.Application("trim").AddConstParameter("right", "30");

  pipeline.Run();
}
