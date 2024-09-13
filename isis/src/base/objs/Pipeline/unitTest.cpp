/** This is free and unencumbered software released into the public domain.
The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */
#include "Isis.h"
#include "Pipeline.h"
#include "Preference.h"
#include "SpecialPixel.h"
#include "UserInterface.h"

using namespace Isis;
using namespace std;

void PipeBranched();
void PipeMultiBranched();
void PipeBranchDisabled();
void PipeSimple();
void PipeListed();
void PipeContinue();

void IsisMain() {
  Preference::Preferences(true);

  UserInterface &ui = Application::GetUserInterface();
  ui.PutFileName("FROM",  "$ISISTESTDATA/isis/src/odyssey/unitTestData/I00831002RDR.even.cub");
  ui.PutFileName("FROM2", "$ISISTESTDATA/isis/src/odyssey/unitTestData/I00831002RDR.odd.cub");
  ui.PutFileName("TO",    "/work1/out.cub");
  ui.PutString  ("SHAPE", "ELLIPSOID");

  ui.Clear("MAPPING");
  ui.PutBoolean("MAPPING", true);
  std::cerr << "Simple Pipe" << std::endl;
  PipeSimple();
  std::cerr << "Simple Pipe 2" << std::endl;
  ui.Clear("MAPPING");
  ui.PutBoolean("MAPPING", false);
  ui.PutString("BANDS", "2,4-5");
  PipeSimple();

  ui.Clear("MAPPING");
  ui.PutBoolean("MAPPING", true);
  std::cerr << "Non-Merging Branching Pipe" << std::endl;
  PipeBranched();
  std::cerr << "Standard Branching Pipe" << std::endl;
  ui.Clear("MAPPING");
  ui.PutBoolean("MAPPING", false);
  PipeBranched();

  std::cerr << "Complicated Branching Pipe" << std::endl;
  PipeMultiBranched();

  ui.Clear("FROM");
  ui.Clear("TO");
  ui.PutFileName("FROM", "unitTest.lis");
  std::cerr << "Testing listing methods" << std::endl;
  PipeListed();

  ui.Clear("FROM");
  ui.Clear("TO");
  ui.PutAsString("FROM", "$ISISTESTDATA/isis/src/odyssey/unitTestData/I00831002RDR.cub");
  ui.PutFileName("TO", "./out.cub");
  std::cerr << "*** Branching Pipe with a branch disabled ***" << std::endl;
  PipeBranchDisabled();

  ui.Clear("TO");
  ui.PutFileName("TO",   "./out.cub");
  std::cerr << "\n*** Continue option ***" << endl;
  cerr << "input=" << ui.GetAsString("FROM").toStdString() << endl;
  PipeContinue();
}

void PipeBranched() {
  UserInterface &ui = Application::GetUserInterface();
  Pipeline p("unitTest1");

  p.SetInputFile("FROM", "BANDS");
  p.SetOutputFile("TO");
  p.KeepTemporaryFiles(!ui.GetBoolean("REMOVE"));

  p.AddToPipeline("thm2isis");
  p.Application("thm2isis").SetInputParameter("FROM", false);
  p.Application("thm2isis").SetOutputParameter("TO", "lev1");
  p.Application("thm2isis").AddBranch("even", PipelineApplication::ConstantStrings);
  p.Application("thm2isis").AddBranch("odd", PipelineApplication::ConstantStrings);

  cerr << p << endl;

  p.AddToPipeline("spiceinit");
  p.Application("spiceinit").SetInputParameter("FROM", false);
  p.Application("spiceinit").AddParameter("PCK", "PCK");
  p.Application("spiceinit").AddParameter("CK", "CK");
  p.Application("spiceinit").AddParameter("SPK", "SPK");
  p.Application("spiceinit").AddParameter("SHAPE", "SHAPE");
  p.Application("spiceinit").AddParameter("MODEL", "MODEL");
  p.Application("spiceinit").AddParameter("CKNADIR", "CKNADIR");

  cerr << p << endl;

  p.AddToPipeline("thmvisflat");
  p.Application("thmvisflat").SetInputParameter("FROM", true);
  p.Application("thmvisflat").SetOutputParameter("TO", "flat");

  cerr << p << endl;

  p.AddToPipeline("thmvistrim");
  p.Application("thmvistrim").SetInputParameter("FROM", true);
  p.Application("thmvistrim").SetOutputParameter("TO", "cal");

  if(!ui.GetBoolean("VISCLEANUP")) {
    p.Application("thmvisflat").Disable();
    p.Application("thmvistrim").Disable();
  }

  cerr << p << endl;

  p.AddToPipeline("cam2map");
  p.Application("cam2map").SetInputParameter("FROM", true);
  p.Application("cam2map").SetOutputParameter("TO", "lev2");

  p.Application("cam2map").AddParameter("even", "MAP", "MAP");
  p.Application("cam2map").AddParameter("even", "PIXRES", "RESOLUTION");

  if(ui.WasEntered("PIXRES")) {
    p.Application("cam2map").AddConstParameter("even", "PIXRES", "MPP");
  }

  cerr << p << endl;

  p.Application("cam2map").AddParameter("odd", "MAP", PipelineApplication::LastOutput);
  p.Application("cam2map").AddConstParameter("odd", "PIXRES", "MAP");
  p.Application("cam2map").AddConstParameter("odd", "DEFAULTRANGE", "MAP");

  cerr << p << endl;

  p.AddToPipeline("automos");
  p.Application("automos").SetInputParameter("FROMLIST", PipelineApplication::LastAppOutputList, false);
  p.Application("automos").SetOutputParameter("TO", "mos");

  cerr << p << endl;

  if(ui.GetBoolean("INGESTION")) {
    p.SetFirstApplication("thm2isis");
  }
  else {
    p.SetFirstApplication("spiceinit");
  }

  cerr << p << endl;

  if(ui.GetBoolean("MAPPING")) {
    p.SetLastApplication("automos");
  }
  else {
    p.SetLastApplication("thmvistrim");
  }

  cerr << p << endl;
}

void PipeSimple() {
  UserInterface &ui = Application::GetUserInterface();
  Pipeline p("unitTest2");

  p.SetInputFile("FROM", "BANDS");
  p.SetOutputFile("TO");

  p.KeepTemporaryFiles(!ui.GetBoolean("REMOVE"));

  p.AddToPipeline("thm2isis");
  p.Application("thm2isis").SetInputParameter("FROM", false);
  p.Application("thm2isis").SetOutputParameter("TO", "lev1");

  cerr << p << endl;

  p.AddToPipeline("spiceinit");
  p.Application("spiceinit").SetInputParameter("FROM", false);
  p.Application("spiceinit").AddParameter("PCK", "PCK");
  p.Application("spiceinit").AddParameter("CK", "CK");
  p.Application("spiceinit").AddParameter("SPK", "SPK");
  p.Application("spiceinit").AddParameter("SHAPE", "SHAPE");
  p.Application("spiceinit").AddParameter("MODEL", "MODEL");
  p.Application("spiceinit").AddParameter("CKNADIR", "CKNADIR");

  cerr << p << endl;

  p.AddToPipeline("cam2map");
  p.Application("cam2map").SetInputParameter("FROM", true);
  p.Application("cam2map").SetOutputParameter("TO", "lev2");
  p.Application("cam2map").AddParameter("MAP", "MAP");
  p.Application("cam2map").AddParameter("PIXRES", "RESOLUTION");

  cerr << p << endl;

  if(ui.WasEntered("PIXRES")) {
    p.Application("cam2map").AddConstParameter("PIXRES", "MPP");
  }

  cerr << p << endl;

  if(ui.GetBoolean("INGESTION")) {
    p.SetFirstApplication("thm2isis");
  }
  else {
    p.SetFirstApplication("spiceinit");
  }

  cerr << p << endl;

  if(ui.GetBoolean("MAPPING")) {
    p.SetLastApplication("cam2map");
  }
  else {
    p.SetLastApplication("spiceinit");
  }

  cerr << p << endl;
}

void PipeMultiBranched() {
  Pipeline p("unitTest3");

  p.SetInputFile("FROM", "BANDS");
  p.SetInputFile("FROM2", "BANDS");
  p.SetOutputFile("TO");

  p.KeepTemporaryFiles(false);

  p.AddToPipeline("fft");
  p.Application("fft").SetInputParameter("FROM", true);
  p.Application("fft").AddBranch("mag", PipelineApplication::ConstantStrings);
  p.Application("fft").AddBranch("phase", PipelineApplication::ConstantStrings);
  p.Application("fft").SetOutputParameter("FROM.mag", "MAGNITUDE", "fft", "cub");
  p.Application("fft").SetOutputParameter("FROM.phase", "PHASE", "fft", "cub");
  p.Application("fft").SetOutputParameter("FROM2.mag", "MAGNITUDE", "fft", "cub");
  p.Application("fft").SetOutputParameter("FROM2.phase", "PHASE", "fft", "cub");

  cerr << p << endl;

  p.AddToPipeline("fx");
  p.Application("fx").SetInputParameter("FILELIST", PipelineApplication::LastAppOutputListNoMerge, false);
  p.Application("fx").SetOutputParameter("FROM.mag", "TO", "fx2", "cub");
  p.Application("fx").SetOutputParameter("FROM2.phase", "TO", "fx2", "cub");
  p.Application("fx").AddConstParameter("FROM.mag", "equation", "1+2");
  p.Application("fx").AddConstParameter("MODE", "list");
  p.Application("fx").AddConstParameter("FROM2.phase", "equation", "1+3");

  cerr << p << endl;

  p.AddToPipeline("ifft");
  p.Application("ifft").SetInputParameter("MAGNITUDE", true);
  p.Application("ifft").AddParameter("PHASE", PipelineApplication::LastOutput);
  p.Application("ifft").SetOutputParameter("FROM.mag", "TO", "untranslated", "cub");

  cerr << p << endl;

  p.AddToPipeline("translate");
  p.Application("translate").SetInputParameter("FROM", true);
  p.Application("translate").AddConstParameter("STRANS", "-1");
  p.Application("translate").AddConstParameter("LTRANS", "-1");
  p.Application("translate").AddConstParameter("INTERP", "near");
  p.Application("translate").SetOutputParameter("FROM.mag", "TO", "final", "cub");

  cerr << p << endl;
}

void PipeListed() {
  Pipeline p("unitTest4");

  p.SetInputListFile("FROM");
  p.SetOutputListFile("TO");

  p.KeepTemporaryFiles(false);

  p.AddToPipeline("cubeatt");
  p.Application("cubeatt").SetInputParameter("FROM", true);
  p.Application("cubeatt").SetOutputParameter("TO", "copy");

  p.AddToPipeline("spiceinit");
  p.Application("spiceinit").SetInputParameter("FROM", false);
  p.Application("spiceinit").AddConstParameter("ATTACH", "NO");

  p.AddToPipeline("appjit");
  p.Application("appjit").SetInputParameter("FROMLIST", PipelineApplication::LastAppOutputListNoMerge, false);

  p.Application("appjit").AddConstParameter("MASTER", "MASTER.cub");
  p.Application("appjit").AddConstParameter("DEGREE", "1");

  p.AddToPipeline("noproj");
  p.Application("noproj").SetInputParameter("FROM", true);
  p.Application("noproj").AddConstParameter("MATCH", "MATCH.cub");
  p.Application("noproj").SetOutputParameter("TO", "noproj");
  p.Application("noproj").SetOutputParameter("TO", "jitter");

  std::cerr << p << std::endl;
}

/**
 * Unittest pipeline with branch disabled
 *
 * @author sprasad (12/20/2010)
 */
void PipeBranchDisabled(void)
{
  Pipeline p("unitTest5");

  p.SetInputFile("FROM", "BANDS");
  p.SetOutputFile("TO");
  p.AddOriginalBranch("lpf");
  p.AddOriginalBranch("hpf");
  p.KeepTemporaryFiles(false);

  p.AddToPipeline("lowpass");
  p.Application("lowpass").SetInputParameter("FROM", false);
  p.Application("lowpass").SetOutputParameter("TO", "lowpass");
  p.Application("lowpass").AddConstParameter("lpf", "SAMPLES", "3");
  p.Application("lowpass").AddConstParameter("lpf", "LINES", "3");
  p.Application("lowpass").EnableBranch("lpf", true);
  p.Application("lowpass").EnableBranch("hpf", false);

  //std::cerr << p;

  p.AddToPipeline("highpass");
  p.Application("highpass").SetInputParameter("FROM", false);
  p.Application("highpass").SetOutputParameter("TO", "highpass");
  p.Application("highpass").AddConstParameter ("hpf", "SAMPLES", "3");
  p.Application("highpass").AddConstParameter("hpf", "LINES", "3");
  p.Application("highpass").EnableBranch("lpf", false);
  p.Application("highpass").EnableBranch("hpf", true);

  //std::cerr << p;

  p.AddToPipeline("fx");
  p.Application("fx").SetInputParameter("FROMLIST", PipelineApplication::LastAppOutputList, false);
  p.Application("fx").SetOutputParameter("TO", "add");
  p.Application("fx").AddConstParameter("MODE", "LIST");
  p.Application("fx").AddConstParameter("EQUATION", "f1+f2");

  std::cerr << p;
}

void PipeContinue(void)
{
  // Pipeline level "continue"
  Pipeline pc1("unitTest6");

  pc1.SetInputFile(FileName("$ISISTESTDATA/isis/src/mro/unitTestData/PSP_001446_1790_BG12_0.cub"));
  pc1.SetOutputFile("TO");
  pc1.SetContinue(true);
  pc1.KeepTemporaryFiles(false);

  pc1.AddToPipeline("noisefilter");
  // Intentionally broken parameters
  pc1.Application("noisefilter").SetInputParameter("FROM", false);
  pc1.Application("noisefilter").SetOutputParameter("TO", "");
  pc1.Application("noisefilter").AddConstParameter("SAMPLES", "0");
  pc1.Application("noisefilter").AddConstParameter("LINES", "0");

  pc1.AddToPipeline("lowpass");
  pc1.Application("lowpass").SetInputParameter ("FROM", false);
  pc1.Application("lowpass").SetOutputParameter("TO", "lowpass");
  pc1.Application("lowpass").AddConstParameter ("SAMPLES", "3");
  pc1.Application("lowpass").AddConstParameter ("LINES", "3");
  pc1.Application("lowpass").AddConstParameter("-PREFERENCE", "$ISISROOT/TestPreferences");

  cerr << pc1 << endl;

  pc1.Run();

  cerr << "\n*** Application level continue option ***\n";
  Pipeline pc2("unitTest7");

  pc2.SetInputFile(FileName("$ISISTESTDATA/isis/src/mro/unitTestData/PSP_001446_1790_BG12_0.cub"));
  pc2.SetOutputFile("TO");
  pc2.KeepTemporaryFiles(false);

  pc2.AddToPipeline("noisefilter", "nf1");
  pc2.Application("nf1").SetContinue(true);
  // Intentionally broken parameters
  pc2.Application("nf1").SetInputParameter("FROM",     false);
  pc2.Application("nf1").SetOutputParameter("TO",      "");
  pc2.Application("nf1").AddConstParameter("SAMPLES",  "0");
  pc2.Application("nf1").AddConstParameter("LINES",    "0");

  pc2.AddToPipeline("lowpass", "lpf1");
  pc2.Application("lpf1").SetInputParameter ("FROM", false);
  pc2.Application("lpf1").SetOutputParameter("TO", "lpf1");
  pc2.Application("lpf1").AddConstParameter ("SAMPLES", "3");
  pc2.Application("lpf1").AddConstParameter ("LINES", "3");
  pc2.Application("lpf1").AddConstParameter("-PREFERENCE", "$ISISROOT/TestPreferences");

  cerr << pc2 << endl;

  pc2.Run();
  remove("./out.cub");
}
