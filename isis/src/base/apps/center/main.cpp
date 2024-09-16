#include "Isis.h"

#include <QString>

#include "ProgramLauncher.h"
#include "ProcessByLine.h"
#include "SpecialPixel.h"
#include "Statistics.h"
#include "IException.h"

using namespace std; 
using namespace Isis;

// Line processing routine
void center(Buffer &in);

static Statistics sumall;
static double sumX, sumY;
static double dnMin, dnMax;


void IsisMain() {
  // We will be processing by line
  ProcessByLine p;

  // Setup the input cube
  Cube *icube = p.SetInputCube("FROM");
  if (icube->bandCount() != 1) {
    std::string msg = "center only works for single-band images.";
    throw IException(IException::User, msg, _FILEINFO_);
  }
  sumall.Reset();
  sumX = sumY = 0.0;
  double sMiddle = (double) (icube->sampleCount())/ 2.0;
  double lMiddle = (double) (icube->lineCount())/ 2.0;


  // Override the defaults if the user entered a value
  UserInterface &ui = Application::GetUserInterface();
  if (ui.WasEntered("MINIMUM")) dnMin = ui.GetDouble("MINIMUM");
  else dnMin = ValidMinimum;
  if (ui.WasEntered("MAXIMUM")) dnMax = ui.GetDouble("MAXIMUM");
  else dnMax = ValidMaximum;

  // Start the processing
  p.StartProcess(center);
  p.EndProcess();


  PvlGroup results("Result");
  double csamp(Null), cline(Null);
  if (sumall.ValidPixels() > 0) {
    csamp = sumX / sumall.Sum();
    cline = sumY / sumall.Sum();
    results += PvlKeyword("CentroidLine", std::to_string(cline));
    results += PvlKeyword("CentroidSample", std::to_string(csamp));
    results += PvlKeyword("LineOffset", std::to_string(lMiddle - cline));
    results += PvlKeyword("SampleOffset", std::to_string(sMiddle - csamp));
  }
  else {
    PvlKeyword badl = PvlKeyword("CentroidLine", "Null");
    badl.addComment("No valid pixels in image!");
    results += badl;
    badl.addComment("");
    results += PvlKeyword("CentroidSample", "Null");
    results += PvlKeyword("LineOffset", "Null");
    results += PvlKeyword("SampleOffset", "Null");
  }
  Application::Log(results);

  if (ui.WasEntered("TO")) {
    if (sumall.ValidPixels() == 0) {
      std::string mess = "No valid pixels so cannot compute center in " + icube->fileName().toStdString();
      throw IException(IException::User, mess, _FILEINFO_);
    }
    double sTrans = sMiddle - csamp;
    double lTrans = lMiddle - cline;
    QString params = "from=" + ui.GetCubeName("FROM") + 
                     " to=" + ui.GetCubeName("TO") + 
                     " strans=" + QString::number(sTrans) + 
                     " ltrans=" + QString::number(lTrans) + 
                     " interp=" + ui.GetString("INTERP");
    ProgramLauncher::RunIsisProgram("translate",params);
  }
  return;
}


//  Line processing routine
void center(Buffer &in) {
  double dline = in.Line() - 1;
  for (int i = 0 ; i < in.size() ; i++) {
    if (!IsSpecial(in[i])) {
      if ((in[i] > dnMin) && (in[i] < dnMax)) {
        double dsamp = i;
        sumX += in[i] * dsamp;
        sumY += in[i] * dline;
        sumall.AddData(in[i]);
      }
    }
  }
 
  return;
}


