#include "Isis.h"
#include "ProcessByLine.h"
#include "SpecialPixel.h"
#include "MultivariateStatistics.h"
#include "OverlapStatistics.h"
#include "UserInterface.h"
#include "IException.h"

#include <QList>

using namespace std;
using namespace Isis;

void getStats(vector<Buffer *> &in, vector<Buffer *> &out);
void toneMatch(Buffer &in, Buffer &out);

MultivariateStatistics stats;
double base, mult;

void IsisMain() {
  // We will be processing by line
  ProcessByLine p;

  // Get the user interface
  UserInterface &ui = Application::GetUserInterface();

  Cube from, match;
  from.open( ui.GetCubeName("FROM") );
  match.open( ui.GetCubeName("MATCH") );

  if( (from.bandCount() != 1) || (match.bandCount() != 1) ) {
    string msg = "tonematch only works for single band images.";
    throw IException(IException::User, msg, _FILEINFO_);
  }
  // If the user selected pOverlap, get the projected overlap statistics

  // Set up the overlap statistics object
  OverlapStatistics oStats(from, match);

  if( ui.GetBoolean("POVERLAP") ) {
    //Make sure the projections overlap
    if(!oStats.HasOverlap()) {
      string msg = "Input Cubes do not appear to overlap";
      throw IException(IException::User, msg, _FILEINFO_);
    }
    // Get mvstat data for the overlapping area
    stats = oStats.GetMStats(1);
  }

  // If the user didnt select pOverlap, get the stats of the entire cubes
  else {
    // Setup the input cubes to match
    p.SetInputCube("FROM", Isis::OneBand);
    p.SetInputCube("MATCH", Isis::OneBand);

    // Get the statistics from the cubes
    p.StartProcess(getStats);
  }

  // compute the linear regression fit of the mvstats data
  stats.LinearRegression(base, mult);

  PvlGroup results("Results");
  QString stringNum = "";
  results += PvlKeyword( "Offset", stringNum.setNum(base).toStdString() );
  results += PvlKeyword( "Gain", stringNum.setNum(mult).toStdString() );
  Pvl fileOutput;
  fileOutput += results;
  oStats.SetMincount( ui.GetInteger("MINCOUNT") );
  fileOutput += oStats.toPvl();
  Application::Log(results);

  if (ui.WasEntered("OUTSTATS")) {
    fileOutput.write(ui.GetFileName("OUTSTATS").toStdString());
  }

  // Apply the correction
  p.ClearInputCubes();
  p.SetInputCube("FROM");
  p.SetOutputCube("TO");
  p.StartProcess(toneMatch);
  p.EndProcess();
}



void getStats(vector<Buffer *> &in, vector<Buffer *> &out) {
  stats.AddData(in[0]->DoubleBuffer(), in[1]->DoubleBuffer(), in[0]->size());
}



void toneMatch(Buffer &in, Buffer &out) {
  for(int i = 0; i < in.size(); i++) {
    if(Isis::IsSpecial(in[i])) {
      out[i] = in[i];
    }
    else {
      out[i] = base + mult * in[i];
    }
  }
}
