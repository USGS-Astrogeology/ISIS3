/** This is free and unencumbered software released into the public domain.
The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */
#include "Isis.h"
#include "Cube.h"
#include "ProcessByLine.h"
#include "Pvl.h"
#include "SubArea.h"

#include <sstream>
#include <iostream>
#include <cmath>

using namespace std;
using namespace Isis;

void IsisMain() {
  Isis::Preference::Preferences(true);
  UserInterface &ui = Application::GetUserInterface();
  ProcessByLine p1;
  Pvl inlabel;
  Cube cube;

  int sl, ss;
  int el, es;
  double linc, sinc;
  int inl, ins;
  int onl, ons;

  p1.SetInputCube("FROM1");
  QString from1 = ui.GetCubeName("FROM1");
  Cube inomapcube;
  inomapcube.open(from1);
  inl = inomapcube.lineCount();
  ins = inomapcube.sampleCount();
  sl = 1;
  ss = 1;
  el = inl;
  es = ins;
  linc = 1.0;
  sinc = 1.0;
  onl = (int)ceil((double)(el - sl + 1) / linc);
  ons = (int)ceil((double)(es - ss + 1) / sinc);
  Cube *onomapcube = p1.SetOutputCube("TO", ons, onl, 1);

  PvlGroup results("Results");
  results += PvlKeyword("InputLines", std::to_string(inl));
  results += PvlKeyword("InputSamples", std::to_string(ins));
  results += PvlKeyword("StartingLine", std::to_string(sl));
  results += PvlKeyword("StartingSample", std::to_string(ss));
  results += PvlKeyword("EndingLine", std::to_string(el));
  results += PvlKeyword("EndingSample", std::to_string(es));
  results += PvlKeyword("LineIncrement", std::to_string(linc));
  results += PvlKeyword("SampleIncrement", std::to_string(sinc));
  results += PvlKeyword("OutputLines", std::to_string(onl));
  results += PvlKeyword("OutputSamples", std::to_string(ons));

  SubArea s;
  s.SetSubArea(inl, ins, sl, ss, el, es, linc, sinc);
  s.UpdateLabel(&inomapcube, onomapcube, results);

  cout << "Input unprojected cube label: " << endl << endl;
  inlabel = *inomapcube.label();
  cout << inlabel.findObject("IsisCube").findObject("Core").findGroup("Dimensions") << endl << endl;
  if(inlabel.findObject("IsisCube").hasGroup("Instrument")) {
    cout << inlabel.findObject("IsisCube").findGroup("Instrument") << endl << endl;
  }
  if(inlabel.findObject("IsisCube").hasGroup("Mapping")) {
    cout << inlabel.findObject("IsisCube").findGroup("Mapping") << endl << endl;
  }
  if(inlabel.findObject("IsisCube").hasGroup("AlphaCube")) {
    cout << inlabel.findObject("IsisCube").findGroup("AlphaCube") << endl << endl;
  }

  cout << "Testing no change in image area for unprojected cube..." << endl << endl;
  Isis::Application::Log(results);
  cout << endl;
  p1.EndProcess();

  cout << "Output cube label: " << endl << endl;
  QString file = ui.GetCubeName("TO");
  Pvl label(file.toStdString());
  cout << label.findObject("IsisCube").findObject("Core").findGroup("Dimensions") << endl << endl;
  if(label.findObject("IsisCube").hasGroup("Instrument")) {
    cout << label.findObject("IsisCube").findGroup("Instrument") << endl << endl;
  }
  if(label.findObject("IsisCube").hasGroup("Mapping")) {
    cout << label.findObject("IsisCube").findGroup("Mapping") << endl << endl;
  }
  if(label.findObject("IsisCube").hasGroup("AlphaCube")) {
    cout << label.findObject("IsisCube").findGroup("AlphaCube") << endl << endl;
  }

  cube.open(file);
  cube.close(true);

  results.clear();

  ProcessByLine p2;
  p2.SetInputCube("FROM1");
  linc = 2.0;
  sinc = 2.0;
  onl = (int)ceil((double)(el - sl + 1) / linc);
  ons = (int)ceil((double)(es - ss + 1) / sinc);
  onomapcube = p2.SetOutputCube("TO", ons, onl, 1);

  results += PvlKeyword("InputLines", std::to_string(inl));
  results += PvlKeyword("InputSamples", std::to_string(ins));
  results += PvlKeyword("StartingLine", std::to_string(sl));
  results += PvlKeyword("StartingSample", std::to_string(ss));
  results += PvlKeyword("EndingLine", std::to_string(el));
  results += PvlKeyword("EndingSample", std::to_string(es));
  results += PvlKeyword("LineIncrement", std::to_string(linc));
  results += PvlKeyword("SampleIncrement", std::to_string(sinc));
  results += PvlKeyword("OutputLines", std::to_string(onl));
  results += PvlKeyword("OutputSamples", std::to_string(ons));

  s.SetSubArea(inl, ins, sl, ss, el, es, linc, sinc);
  s.UpdateLabel(&inomapcube, onomapcube, results);

  cout << "Testing full image area with linc=2, sinc=2 ";
  cout << "for unprojected cube..." << endl;

  Isis::Application::Log(results);
  cout << endl;
  p2.EndProcess();

  cout << "Output cube label: " << endl << endl;
  cube.open(file);
  label = *cube.label();
  cube.close(true);
  cout << label.findObject("IsisCube").findObject("Core").findGroup("Dimensions") << endl << endl;
  if(label.findObject("IsisCube").hasGroup("Instrument")) {
    cout << label.findObject("IsisCube").findGroup("Instrument") << endl << endl;
  }
  if(label.findObject("IsisCube").hasGroup("Mapping")) {
    cout << label.findObject("IsisCube").findGroup("Mapping") << endl << endl;
  }
  if(label.findObject("IsisCube").hasGroup("AlphaCube")) {
    cout << label.findObject("IsisCube").findGroup("AlphaCube") << endl << endl;
  }

  results.clear();

  ProcessByLine p3;
  p3.SetInputCube("FROM1");
  linc = 2.0;
  sinc = 3.0;
  onl = (int)ceil((double)(el - sl + 1) / linc);
  ons = (int)ceil((double)(es - ss + 1) / sinc);
  onomapcube = p3.SetOutputCube("TO", ons, onl, 1);

  results += PvlKeyword("InputLines", std::to_string(inl));
  results += PvlKeyword("InputSamples", std::to_string(ins));
  results += PvlKeyword("StartingLine", std::to_string(sl));
  results += PvlKeyword("StartingSample", std::to_string(ss));
  results += PvlKeyword("EndingLine", std::to_string(el));
  results += PvlKeyword("EndingSample", std::to_string(es));
  results += PvlKeyword("LineIncrement", std::to_string(linc));
  results += PvlKeyword("SampleIncrement", std::to_string(sinc));
  results += PvlKeyword("OutputLines", std::to_string(onl));
  results += PvlKeyword("OutputSamples", std::to_string(ons));

  s.SetSubArea(inl, ins, sl, ss, el, es, linc, sinc);
  s.UpdateLabel(&inomapcube, onomapcube, results);

  cout << "Testing full image area with linc=2, sinc=3 ";
  cout << "for unprojected cube..." << endl;

  Isis::Application::Log(results);
  cout << endl;
  p3.EndProcess();

  cout << "Output cube label: " << endl << endl;
  cube.open(file);
  label = *cube.label();
  cube.close(true);
  cout << label.findObject("IsisCube").findObject("Core").findGroup("Dimensions") << endl << endl;
  if(label.findObject("IsisCube").hasGroup("Instrument")) {
    cout << label.findObject("IsisCube").findGroup("Instrument") << endl << endl;
  }
  if(label.findObject("IsisCube").hasGroup("Mapping")) {
    cout << label.findObject("IsisCube").findGroup("Mapping") << endl << endl;
  }
  if(label.findObject("IsisCube").hasGroup("AlphaCube")) {
    cout << label.findObject("IsisCube").findGroup("AlphaCube") << endl << endl;
  }

  results.clear();

  ProcessByLine p4;
  p4.SetInputCube("FROM1");
  sl = 25;
  ss = 10;
  el = inl - 33;
  es = ins - 18;
  linc = .5;
  sinc = .5;
  onl = (int)ceil((double)(el - sl + 1) / linc);
  ons = (int)ceil((double)(es - ss + 1) / sinc);
  onomapcube = p4.SetOutputCube("TO", ons, onl, 1);

  results += PvlKeyword("InputLines", std::to_string(inl));
  results += PvlKeyword("InputSamples", std::to_string(ins));
  results += PvlKeyword("StartingLine", std::to_string(sl));
  results += PvlKeyword("StartingSample", std::to_string(ss));
  results += PvlKeyword("EndingLine", std::to_string(el));
  results += PvlKeyword("EndingSample", std::to_string(es));
  results += PvlKeyword("LineIncrement", std::to_string(linc));
  results += PvlKeyword("SampleIncrement", std::to_string(sinc));
  results += PvlKeyword("OutputLines", std::to_string(onl));
  results += PvlKeyword("OutputSamples", std::to_string(ons));

  s.SetSubArea(inl, ins, sl, ss, el, es, linc, sinc);
  s.UpdateLabel(&inomapcube, onomapcube, results);

  cout << "Testing sub image area with linc=.5, sinc=.5 ";
  cout << "for unprojected cube..." << endl;

  Isis::Application::Log(results);
  cout << endl;
  p4.EndProcess();

  cout << "Output cube label: " << endl << endl;
  cube.open(file);
  label = *cube.label();
  cube.close(true);
  cout << label.findObject("IsisCube").findObject("Core").findGroup("Dimensions") << endl << endl;
  if(label.findObject("IsisCube").hasGroup("Instrument")) {
    cout << label.findObject("IsisCube").findGroup("Instrument") << endl << endl;
  }
  if(label.findObject("IsisCube").hasGroup("Mapping")) {
    cout << label.findObject("IsisCube").findGroup("Mapping") << endl << endl;
  }
  if(label.findObject("IsisCube").hasGroup("AlphaCube")) {
    cout << label.findObject("IsisCube").findGroup("AlphaCube") << endl << endl;
  }

  results.clear();

  ProcessByLine p5;
  p5.SetInputCube("FROM1");
  linc = 1.0;
  sinc = 2.5;
  onl = (int)ceil((double)(el - sl + 1) / linc);
  ons = (int)ceil((double)(es - ss + 1) / sinc);
  onomapcube = p5.SetOutputCube("TO", ons, onl, 1);

  results += PvlKeyword("InputLines", std::to_string(inl));
  results += PvlKeyword("InputSamples", std::to_string(ins));
  results += PvlKeyword("StartingLine", std::to_string(sl));
  results += PvlKeyword("StartingSample", std::to_string(ss));
  results += PvlKeyword("EndingLine", std::to_string(el));
  results += PvlKeyword("EndingSample", std::to_string(es));
  results += PvlKeyword("LineIncrement", std::to_string(linc));
  results += PvlKeyword("SampleIncrement", std::to_string(sinc));
  results += PvlKeyword("OutputLines", std::to_string(onl));
  results += PvlKeyword("OutputSamples", std::to_string(ons));

  s.SetSubArea(inl, ins, sl, ss, el, es, linc, sinc);
  s.UpdateLabel(&inomapcube, onomapcube, results);

  cout << "Testing sub image area with linc=1.0, sinc=2.5 ";
  cout << "for unprojected cube..." << endl;

  Isis::Application::Log(results);
  cout << endl;
  p5.EndProcess();

  cout << "Output cube label: " << endl << endl;
  cube.open(file);
  label = *cube.label();
  cube.close(true);
  cout << label.findObject("IsisCube").findObject("Core").findGroup("Dimensions") << endl << endl;
  if(label.findObject("IsisCube").hasGroup("Instrument")) {
    cout << label.findObject("IsisCube").findGroup("Instrument") << endl << endl;
  }
  if(label.findObject("IsisCube").hasGroup("Mapping")) {
    cout << label.findObject("IsisCube").findGroup("Mapping") << endl << endl;
  }
  if(label.findObject("IsisCube").hasGroup("AlphaCube")) {
    cout << label.findObject("IsisCube").findGroup("AlphaCube") << endl << endl;
  }

  inomapcube.close();

  results.clear();

  ProcessByLine p6;
  p6.SetInputCube("FROM2");
  QString from2 = ui.GetCubeName("FROM2");
  Cube imapcube;
  imapcube.open(from2);
  inl = imapcube.lineCount();
  ins = imapcube.sampleCount();
  sl = 1;
  ss = 1;
  el = inl;
  es = ins;
  linc = 1.0;
  sinc = 1.0;
  onl = (int)ceil((double)(el - sl + 1) / linc);
  ons = (int)ceil((double)(es - ss + 1) / sinc);
  Cube *omapcube = p6.SetOutputCube("TO", ons, onl, 1);

  results += PvlKeyword("InputLines", std::to_string(inl));
  results += PvlKeyword("InputSamples", std::to_string(ins));
  results += PvlKeyword("StartingLine", std::to_string(sl));
  results += PvlKeyword("StartingSample", std::to_string(ss));
  results += PvlKeyword("EndingLine", std::to_string(el));
  results += PvlKeyword("EndingSample", std::to_string(es));
  results += PvlKeyword("LineIncrement", std::to_string(linc));
  results += PvlKeyword("SampleIncrement", std::to_string(sinc));
  results += PvlKeyword("OutputLines", std::to_string(onl));
  results += PvlKeyword("OutputSamples", std::to_string(ons));

  s.SetSubArea(inl, ins, sl, ss, el, es, linc, sinc);
  s.UpdateLabel(&imapcube, omapcube, results);

  cout << "Input projected cube label: " << endl << endl;
  inlabel = *imapcube.label();
  cout << inlabel.findObject("IsisCube").findObject("Core").findGroup("Dimensions") << endl << endl;
  if(inlabel.findObject("IsisCube").hasGroup("Instrument")) {
    cout << inlabel.findObject("IsisCube").findGroup("Instrument") << endl << endl;
  }
  if(inlabel.findObject("IsisCube").hasGroup("Mapping")) {
    cout << inlabel.findObject("IsisCube").findGroup("Mapping") << endl << endl;
  }
  if(inlabel.findObject("IsisCube").hasGroup("AlphaCube")) {
    cout << inlabel.findObject("IsisCube").findGroup("AlphaCube") << endl << endl;
  }

  cout << "Testing no change in image area for projected cube..." << endl;
  Isis::Application::Log(results);
  cout << endl;
  p6.EndProcess();

  cout << "Output cube label: " << endl << endl;
  cube.open(file);
  label = *cube.label();
  cube.close(true);
  cout << label.findObject("IsisCube").findObject("Core").findGroup("Dimensions") << endl << endl;
  if(label.findObject("IsisCube").hasGroup("Instrument")) {
    cout << label.findObject("IsisCube").findGroup("Instrument") << endl << endl;
  }
  if(label.findObject("IsisCube").hasGroup("Mapping")) {
    cout << label.findObject("IsisCube").findGroup("Mapping") << endl << endl;
  }
  if(label.findObject("IsisCube").hasGroup("AlphaCube")) {
    cout << label.findObject("IsisCube").findGroup("AlphaCube") << endl << endl;
  }

  results.clear();

  ProcessByLine p7;
  p7.SetInputCube("FROM2");
  linc = 2.0;
  sinc = 2.0;
  onl = (int)ceil((double)(el - sl + 1) / linc);
  ons = (int)ceil((double)(es - ss + 1) / sinc);
  omapcube = p7.SetOutputCube("TO", ons, onl, 1);

  results += PvlKeyword("InputLines", std::to_string(inl));
  results += PvlKeyword("InputSamples", std::to_string(ins));
  results += PvlKeyword("StartingLine", std::to_string(sl));
  results += PvlKeyword("StartingSample", std::to_string(ss));
  results += PvlKeyword("EndingLine", std::to_string(el));
  results += PvlKeyword("EndingSample", std::to_string(es));
  results += PvlKeyword("LineIncrement", std::to_string(linc));
  results += PvlKeyword("SampleIncrement", std::to_string(sinc));
  results += PvlKeyword("OutputLines", std::to_string(onl));
  results += PvlKeyword("OutputSamples", std::to_string(ons));

  s.SetSubArea(inl, ins, sl, ss, el, es, linc, sinc);
  s.UpdateLabel(&imapcube, omapcube, results);

  cout << "Testing full image area with linc=2, sinc=2 ";
  cout << "for projected cube..." << endl;

  Isis::Application::Log(results);
  cout << endl;
  p7.EndProcess();

  cout << "Output cube label: " << endl << endl;
  cube.open(file);
  label = *cube.label();
  cube.close(true);
  cout << label.findObject("IsisCube").findObject("Core").findGroup("Dimensions") << endl << endl;
  if(label.findObject("IsisCube").hasGroup("Instrument")) {
    cout << label.findObject("IsisCube").findGroup("Instrument") << endl << endl;
  }
  if(label.findObject("IsisCube").hasGroup("Mapping")) {
    cout << label.findObject("IsisCube").findGroup("Mapping") << endl << endl;
  }
  if(label.findObject("IsisCube").hasGroup("AlphaCube")) {
    cout << label.findObject("IsisCube").findGroup("AlphaCube") << endl << endl;
  }

  results.clear();

  ProcessByLine p8;
  p8.SetInputCube("FROM2");
  linc = 2.0;
  sinc = 3.0;
  onl = (int)ceil((double)(el - sl + 1) / linc);
  ons = (int)ceil((double)(es - ss + 1) / sinc);
  omapcube = p8.SetOutputCube("TO", ons, onl, 1);

  results += PvlKeyword("InputLines", std::to_string(inl));
  results += PvlKeyword("InputSamples", std::to_string(ins));
  results += PvlKeyword("StartingLine", std::to_string(sl));
  results += PvlKeyword("StartingSample", std::to_string(ss));
  results += PvlKeyword("EndingLine", std::to_string(el));
  results += PvlKeyword("EndingSample", std::to_string(es));
  results += PvlKeyword("LineIncrement", std::to_string(linc));
  results += PvlKeyword("SampleIncrement", std::to_string(sinc));
  results += PvlKeyword("OutputLines", std::to_string(onl));
  results += PvlKeyword("OutputSamples", std::to_string(ons));

  s.SetSubArea(inl, ins, sl, ss, el, es, linc, sinc);
  s.UpdateLabel(&imapcube, omapcube, results);

  cout << "Testing full image area with linc=2, sinc=3 ";
  cout << "for projected cube..." << endl;

  Isis::Application::Log(results);
  cout << endl;
  p8.EndProcess();

  cout << "Output cube label: " << endl << endl;
  cube.open(file);
  label = *cube.label();
  cube.close(true);
  cout << label.findObject("IsisCube").findObject("Core").findGroup("Dimensions") << endl << endl;
  if(label.findObject("IsisCube").hasGroup("Instrument")) {
    cout << label.findObject("IsisCube").findGroup("Instrument") << endl << endl;
  }
  if(label.findObject("IsisCube").hasGroup("Mapping")) {
    cout << label.findObject("IsisCube").findGroup("Mapping") << endl << endl;
  }
  if(label.findObject("IsisCube").hasGroup("AlphaCube")) {
    cout << label.findObject("IsisCube").findGroup("AlphaCube") << endl << endl;
  }

  results.clear();

  ProcessByLine p9;
  p9.SetInputCube("FROM2");
  sl = 25;
  ss = 10;
  el = inl - 33;
  es = ins - 18;
  linc = .5;
  sinc = .5;
  onl = (int)ceil((double)(el - sl + 1) / linc);
  ons = (int)ceil((double)(es - ss + 1) / sinc);
  omapcube = p9.SetOutputCube("TO", ons, onl, 1);

  results += PvlKeyword("InputLines", std::to_string(inl));
  results += PvlKeyword("InputSamples", std::to_string(ins));
  results += PvlKeyword("StartingLine", std::to_string(sl));
  results += PvlKeyword("StartingSample", std::to_string(ss));
  results += PvlKeyword("EndingLine", std::to_string(el));
  results += PvlKeyword("EndingSample", std::to_string(es));
  results += PvlKeyword("LineIncrement", std::to_string(linc));
  results += PvlKeyword("SampleIncrement", std::to_string(sinc));
  results += PvlKeyword("OutputLines", std::to_string(onl));
  results += PvlKeyword("OutputSamples", std::to_string(ons));

  s.SetSubArea(inl, ins, sl, ss, el, es, linc, sinc);
  s.UpdateLabel(&imapcube, omapcube, results);

  cout << "Testing sub image area with linc=.5, sinc=.5 ";
  cout << "for projected cube..." << endl;

  Isis::Application::Log(results);
  cout << endl;
  p9.EndProcess();

  cout << "Output cube label: " << endl << endl;
  cube.open(file);
  label = *cube.label();
  cube.close(true);
  cout << label.findObject("IsisCube").findObject("Core").findGroup("Dimensions") << endl << endl;
  if(label.findObject("IsisCube").hasGroup("Instrument")) {
    cout << label.findObject("IsisCube").findGroup("Instrument") << endl << endl;
  }
  if(label.findObject("IsisCube").hasGroup("Mapping")) {
    cout << label.findObject("IsisCube").findGroup("Mapping") << endl << endl;
  }
  if(label.findObject("IsisCube").hasGroup("AlphaCube")) {
    cout << label.findObject("IsisCube").findGroup("AlphaCube") << endl << endl;
  }

  results.clear();

  ProcessByLine p10;
  p10.SetInputCube("FROM2");
  linc = 1.0;
  sinc = 2.5;
  onl = (int)ceil((double)(el - sl + 1) / linc);
  ons = (int)ceil((double)(es - ss + 1) / sinc);
  omapcube = p10.SetOutputCube("TO", ons, onl, 1);

  results += PvlKeyword("InputLines", std::to_string(inl));
  results += PvlKeyword("InputSamples", std::to_string(ins));
  results += PvlKeyword("StartingLine", std::to_string(sl));
  results += PvlKeyword("StartingSample", std::to_string(ss));
  results += PvlKeyword("EndingLine", std::to_string(el));
  results += PvlKeyword("EndingSample", std::to_string(es));
  results += PvlKeyword("LineIncrement", std::to_string(linc));
  results += PvlKeyword("SampleIncrement", std::to_string(sinc));
  results += PvlKeyword("OutputLines", std::to_string(onl));
  results += PvlKeyword("OutputSamples", std::to_string(ons));

  s.SetSubArea(inl, ins, sl, ss, el, es, linc, sinc);
  s.UpdateLabel(&imapcube, omapcube, results);

  cout << "Testing sub image area with linc=1.0, sinc=2.5 ";
  cout << "for projected cube..." << endl;

  Isis::Application::Log(results);
  cout << endl;
  p10.EndProcess();

  cout << "Output cube label: " << endl << endl;
  cube.open(file);
  label = *cube.label();
  cube.close(true);
  cout << label.findObject("IsisCube").findObject("Core").findGroup("Dimensions") << endl << endl;
  if(label.findObject("IsisCube").hasGroup("Instrument")) {
    cout << label.findObject("IsisCube").findGroup("Instrument") << endl << endl;
  }
  if(label.findObject("IsisCube").hasGroup("Mapping")) {
    cout << label.findObject("IsisCube").findGroup("Mapping") << endl << endl;
  }
  if(label.findObject("IsisCube").hasGroup("AlphaCube")) {
    cout << label.findObject("IsisCube").findGroup("AlphaCube") << endl << endl;
  }

  imapcube.close();

  results.clear();

  Cube smapcube;
  QString from3 = ui.GetCubeName("FROM3");
  smapcube.open(from3);
  inl = smapcube.lineCount();
  ins = smapcube.sampleCount();
  ProcessByLine p11;
  p11.SetInputCube("FROM3");
  sl = 2070;
  ss = 818;
  el = 2270;
  es = 1018;
  linc = 1.0;
  sinc = 1.0;
  onl = (int)ceil((double)(el - sl + 1) / linc);
  ons = (int)ceil((double)(es - ss + 1) / sinc);
  omapcube = p11.SetOutputCube("TO", ons, onl, 1);

  results += PvlKeyword("InputLines", std::to_string(inl));
  results += PvlKeyword("InputSamples", std::to_string(ins));
  results += PvlKeyword("StartingLine", std::to_string(sl));
  results += PvlKeyword("StartingSample", std::to_string(ss));
  results += PvlKeyword("EndingLine", std::to_string(el));
  results += PvlKeyword("EndingSample", std::to_string(es));
  results += PvlKeyword("LineIncrement", std::to_string(linc));
  results += PvlKeyword("SampleIncrement", std::to_string(sinc));
  results += PvlKeyword("OutputLines", std::to_string(onl));
  results += PvlKeyword("OutputSamples", std::to_string(ons));

  s.SetSubArea(inl, ins, sl, ss, el, es, linc, sinc);
  s.UpdateLabel(&smapcube, omapcube, results);

  cout << "Input Simple Cylindrical projected cube label: " << endl << endl;
  inlabel = *smapcube.label();
  cout << inlabel.findObject("IsisCube").findObject("Core").findGroup("Dimensions") << endl << endl;
  if(inlabel.findObject("IsisCube").hasGroup("Instrument")) {
    cout << inlabel.findObject("IsisCube").findGroup("Instrument") << endl << endl;
  }
  if(inlabel.findObject("IsisCube").hasGroup("Mapping")) {
    cout << inlabel.findObject("IsisCube").findGroup("Mapping") << endl << endl;
  }
  if(inlabel.findObject("IsisCube").hasGroup("AlphaCube")) {
    cout << inlabel.findObject("IsisCube").findGroup("AlphaCube") << endl << endl;
  }

  cout << "Testing sub image area with linc=1.0, sinc=1.0 ";
  cout << "for Simple Cylindrical projected cube..." << endl;

  Isis::Application::Log(results);
  cout << endl;
  p11.EndProcess();

  cout << "Output cube label: " << endl << endl;
  cube.open(file);
  label = *cube.label();
  cube.close(true);
  cout << label.findObject("IsisCube").findObject("Core").findGroup("Dimensions") << endl << endl;
  if(label.findObject("IsisCube").hasGroup("Instrument")) {
    cout << label.findObject("IsisCube").findGroup("Instrument") << endl << endl;
  }
  if(label.findObject("IsisCube").hasGroup("Mapping")) {
    cout << label.findObject("IsisCube").findGroup("Mapping") << endl << endl;
  }
  if(label.findObject("IsisCube").hasGroup("AlphaCube")) {
    cout << label.findObject("IsisCube").findGroup("AlphaCube") << endl << endl;
  }

  results.clear();
}
