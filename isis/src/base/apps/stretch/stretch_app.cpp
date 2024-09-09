#include "Application.h"
#include "TextFile.h"
#include "Statistics.h"
#include "ProcessByLine.h"
#include "SpecialPixel.h"
#include "Stretch.h"
#include "PvlGroup.h"
#include "PvlKeyword.h"

#include "stretch_app.h"

namespace Isis {
  void stretchProcess(Buffer &in, Buffer &out);
  Stretch str;
  Statistics stats;

  void stretch(UserInterface &ui, Pvl *log) {
    Cube *cubeFile = new Cube();
    CubeAttributeInput inAtt = ui.GetInputAttribute("FROM");
    if (inAtt.bands().size() != 0) {
      cubeFile->setVirtualBands(inAtt.bands());
    }
    cubeFile->open(ui.GetCubeName("FROM"), "r");

    QString pairs;

    // first just get the pairs from where ever and worry about
    // whether they are dn values or %'s later
    if(ui.GetBoolean("READFILE")) {
      FileName pairsFileName = ui.GetFileName("INPUTFILE").toStdString();
      TextFile pairsFile;
      pairsFile.SetComment("#");
      pairsFile.Open(QString::fromStdString(pairsFileName.expanded()));

      // concat all non-comment lines into one string (pairs)
      QString line = "";
      while(pairsFile.GetLine(line, true)) {
        pairs += " " + line;
      }
      pairs += line;
    }
    else {
      if(ui.WasEntered("PAIRS"))
        pairs = ui.GetString("PAIRS");
    }

    stretch(cubeFile, pairs, ui, log);
  }


  void stretch(Cube *inCube, QString &pairs, UserInterface &ui, Pvl *log) {
    ProcessByLine p;
    p.SetInputCube(inCube);

    if(ui.GetBoolean("USEPERCENTAGES")) {
      str.Parse(pairs, inCube->histogram());
    }
    else {
      str.Parse(pairs);
    }

    // Setup new mappings for special pixels if necessary
    if(ui.WasEntered("NULL"))
      str.SetNull(StringToPixel(ui.GetString("NULL").toStdString()));
    if(ui.WasEntered("LIS"))
      str.SetLis(StringToPixel(ui.GetString("LIS").toStdString()));
    if(ui.WasEntered("LRS"))
      str.SetLrs(StringToPixel(ui.GetString("LRS").toStdString()));
    if(ui.WasEntered("HIS"))
      str.SetHis(StringToPixel(ui.GetString("HIS").toStdString()));
    if(ui.WasEntered("HRS"))
      str.SetHrs(StringToPixel(ui.GetString("HRS").toStdString()));

    p.SetOutputCubeStretch("TO", &ui);

    // Start the processing
    p.StartProcess(stretchProcess);
    p.EndProcess();

    PvlKeyword dnPairs = PvlKeyword("StretchPairs");
    dnPairs.addValue(str.Text().toStdString());

    PvlGroup results = PvlGroup("Results");
    results.addKeyword(dnPairs);

    Application::Log(results);
  }

  // Line processing routine
  void stretchProcess(Buffer &in, Buffer &out) {
    for(int i = 0; i < in.size(); i++) {
      out[i] = str.Map(in[i]);
    }
  }
}
