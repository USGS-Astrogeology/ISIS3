#include "Isis.h"

#include <QDebug>

#include "ProcessByLine.h"
#include "SpecialPixel.h"
#include "IException.h"
#include "SessionLog.h"
#include "Pvl.h"

using namespace std;
using namespace Isis;

void mask(vector<Buffer *> &in,
          vector<Buffer *> &out);

enum which_special {NONE, NULLP, ALL} spixels;
enum range_preserve {INSIDE, OUTSIDE} preserve;

double g_minimum, g_maximum;
bool g_masked;
double g_pixelsMasked;

void IsisMain() {
  // We will be processing by line
  ProcessByLine p;

  g_masked = false;
  g_pixelsMasked = 0;

  // Setup the input and output cubes
  UserInterface &ui = Application::GetUserInterface();

  p.SetInputCube("FROM");
  if(!ui.WasEntered("MASK")) {
    p.SetInputCube("FROM");
  }
  else {
    try {
      p.SetInputCube("MASK", OneBand);
    }
    catch (IException &e) {
      std::string msg = "The MASK input must be a single band.";
      throw IException(e, IException::User, msg, _FILEINFO_);
    }
  }
  p.SetOutputCube("TO");

  //  Get min/max info
  g_minimum = VALID_MIN8;
  g_maximum = VALID_MAX8;
  if(ui.WasEntered("MINIMUM")) g_minimum = ui.GetDouble("MINIMUM");
  if(ui.WasEntered("MAXIMUM")) g_maximum = ui.GetDouble("MAXIMUM");

  //  Will we preserve inside or outside of min/max range
  preserve = INSIDE;
  QString Preserve;
  if(ui.WasEntered("PRESERVE")) Preserve = ui.GetString("PRESERVE");
  if(Preserve == "OUTSIDE") preserve = OUTSIDE;

  //  How are special pixels handled?
  spixels = NULLP;
  QString Spixels;
  if(ui.WasEntered("SPIXELS")) Spixels = ui.GetString("SPIXELS");
  if(Spixels == "NONE") spixels = NONE;
  if(Spixels == "ALL") spixels = ALL;

  // Start the processing
  p.StartProcess(mask);
  p.EndProcess();
  
  // Add an entry to print.prt to indicate whether this file was masked.
  // Pvl maskedPvl;
  PvlGroup results("Results");

  if(g_masked == true) {
      results += PvlKeyword("PixelsMasked", Isis::toString(g_pixelsMasked));
  }
  else {
      PvlKeyword pm("PixelsMasked", Isis::toString(g_pixelsMasked));
      pm.addComment( "No pixels were masked for this image");
      results += pm;
  }

  // maskedPvl.addGroup(results);

  Application::Log(results);

}

// Line processing routine
void mask(vector<Buffer *> &in,
          vector<Buffer *> &out) {

  Buffer &inp = *in[0];
  Buffer &mask = *in[1];
  Buffer &outp = *out[0];

  // Loop for each pixel in the line.
  for(int i = 0; i < inp.size(); i++) {
    if(IsSpecial(mask[i])) {
      if(spixels == ALL) {
        outp[i] = NULL8;
        g_masked = true;
        g_pixelsMasked++;
      }
      else if(spixels == NULLP && mask[i] == NULL8) {
        outp[i] = NULL8;
        g_masked = true;
        g_pixelsMasked++;
      }
      else {
        outp[i] = inp[i];
      }
    }

    else {
      if(preserve == INSIDE && (mask[i] >= g_minimum && mask[i] <= g_maximum))
        outp[i] = inp[i];
      else if(preserve == OUTSIDE && (mask[i] < g_minimum || mask[i] > g_maximum))
        outp[i] = inp[i];
      else {
        outp[i] = NULL8;
        g_masked = true;
        g_pixelsMasked++;
      }
    }
  }

}
