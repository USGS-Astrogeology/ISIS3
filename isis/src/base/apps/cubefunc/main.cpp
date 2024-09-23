#include <cmath>
#include "Isis.h"
#include "ProcessByLine.h"
#include "SpecialPixel.h"
#include "IException.h"

using namespace std;
using namespace Isis;

void cubefunc(Buffer &in,
              Buffer &out);

double bad = 0;
double y;

enum function {
  COS,
  SIN,
  TAN,
  ACOS,
  ASIN,
  ATAN,
  INV,
  SQRT,
  POW10,
  EXP,
  XTOY,
  LOG10,
  LN,
  ABS
} Function;


void IsisMain() {
  // We will be processing by line
  ProcessByLine p;

  // Setup the input and output cubes
  p.SetInputCube("FROM");
  p.SetOutputCube("TO");

  UserInterface &ui = Application::GetUserInterface();

  // Which function is it to be?
  QString func = ui.GetString("FUNCTION");
  if(func == "COS") Function = COS;
  if(func == "SIN") Function = SIN;
  if(func == "TAN") Function = TAN;
  if(func == "ACOS") Function = ACOS;
  if(func == "ASIN") Function = ASIN;
  if(func == "ATAN") Function = ATAN;
  if(func == "INV") Function = INV;
  if(func == "SQRT") Function = SQRT;
  if(func == "POW10") Function = POW10;
  if(func == "EXP") Function = EXP;
  if(func == "XTOY") Function = XTOY;
  if(func == "LOG10") Function = LOG10;
  if(func == "LN") Function = LN;
  if(func == "ABS") Function = ABS;

  if(Function == XTOY) {
    if(ui.WasEntered("Y")) {
      y = ui.GetDouble("Y");
    }
    else {
      string message = "For the XTOY function, you must enter a value for y";
      throw IException(IException::User, message, _FILEINFO_);
    }
  }

  // Start the processing
  p.StartProcess(cubefunc);

  if(bad != 0) {
    PvlGroup results("Results");
    std::string message = "Invalid input pixels converted to Isis NULL values";
    results += PvlKeyword("Error", message);
    results += PvlKeyword("Count", toString(bad));
    Application::Log(results);
  }
  p.EndProcess();
}

// Line processing routine
void cubefunc(Buffer &in,
              Buffer &out) {

  // Loop for each pixel in the line.
  for(int i = 0; i < in.size(); i++) {

    if(IsSpecial(in[i])) {
      out[i] = in[i];
    }
    else {
      switch(Function) {
        case COS:
          if(in[i] < (-(2 * PI)) || in[i] > (2 * PI)) {
            out[i] = NULL8;
            bad++;
          }
          out[i] = cos(in[i]);
          break;
        case SIN:
          if(in[i] < (-(2 * PI)) || in[i] > (2 * PI)) {
            out[i] = NULL8;
            bad++;
          }
          out[i] = sin(in[i]);
          break;
        case TAN:
          //  Check for invalid input values.  Check within a certain
          //  tolerance since the radiance value will probably never be
          //  exactly 90, 270, -90 or -270 degrees due to round off.
          //  First convert input value from radians to degrees.
          if(abs(abs(in[i]) - 90.0) <= .0001 ||
              abs(abs(in[i]) - 270.0) <= .0001) {
            out[i] = NULL8;
            bad++;
          }
          else {
            out[i] = tan(in[i]);
          }
          break;
        case ACOS:
          if(in[i] < -1.0 || in[i] > 1.0) {
            out[i] = NULL8;
            bad++;
          }
          else {
            out[i] = acos(in[i]);
          }
          break;
        case ASIN:
          if(in[i] < -1.0 || in[i] > 1.0) {
            out[i] = NULL8;
            bad++;
          }
          else {
            out[i] = asin(in[i]);
          }
          break;
        case ATAN:
          out[i] = atan(in[i]);
          break;
        case INV:
          if(in[i] == 0) {
            out[i] = NULL8;
            bad++;
          }
          else {
            out[i] = 1 / in[i];
          }
          break;
        case SQRT:
          if(in[i] <= 0) {
            out[i] = NULL8;
            bad++;
          }
          else {
            out[i] = sqrt(in[i]);
          }
          break;
        case POW10:
          out[i] = pow(10, in[i]);
          break;
        case EXP:
          out[i] = exp(in[i]);
          break;
        case XTOY:
          out[i] = pow(in[i], y);
          break;
        case LOG10:
          if(in[i] <= 0) {
            out[i] = NULL8;
            bad++;
          }
          else {
            out[i] = log10(in[i]);
          }
          break;
        case LN:
          if(in[i] <= 0) {
            out[i] = NULL8;
            bad++;
          }
          else {
            out[i] = log(in[i]);
          }
          break;
        case ABS:
          out[i] = abs(in[i]);
          break;
      }
    }
  }
}
