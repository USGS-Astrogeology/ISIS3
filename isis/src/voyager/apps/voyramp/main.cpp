/** This is free and unencumbered software released into the public domain.

The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */

#include "Isis.h"

#include "Buffer.h"
#include "Cube.h"
#include "IException.h"
#include "iTime.h"
#include "ProcessByLine.h"
#include "Pvl.h"
#include "PvlGroup.h"
#include "PvlKeyword.h"
#include "SpecialPixel.h"
#include "UserInterface.h"

#include <QVector4D>

using namespace std;
using namespace Isis;

static const QVector4D table[17] = {
  //QVector4D (top, middle, bottom, hour),
  QVector4D( 0.0,  0.0,  0.0,  1.0),
  QVector4D( 0.1,  0.1,  0.1,  2.0),
  QVector4D( 0.2,  0.3,  0.4,  3.0),
  QVector4D( 0.4,  0.6,  0.8,  4.0),
  QVector4D( 0.7,  1.0,  1.4,  5.0),
  QVector4D( 1.1,  1.9,  2.6,  6.0),
  QVector4D( 2.0,  2.8,  4.2,  7.0),
  QVector4D( 3.0,  4.0,  6.1,  8.0),
  QVector4D( 4.6,  6.6, 10.0,  9.0),
  QVector4D( 7.5, 11.8, 17.9, 10.0),
  QVector4D(11.2, 16.0, 25.0, 11.0),
  QVector4D(12.0, 17.8, 27.2, 12.0),
  QVector4D( 9.5, 14.6, 22.9, 13.0),
  QVector4D( 6.0,  8.9, 14.5, 14.0),
  QVector4D( 2.5,  4.7,  8.0, 15.0),
  QVector4D( 1.2,  1.7,  2.2, 16.0),
  QVector4D( 0.0,  0.0,  0.0, 17.0)
};

void ioCorrection(Buffer &in,
                  Buffer &out);

double plasmaOffset;
double plasmaA;
double plasmaB;
double plasmaC;
int line;

static double interp(double x1, double y1, double x2, double y2, double x);

void IsisMain() {
  // Processing by line
  ProcessByLine p;
  line = 1;

  UserInterface &ui = Application::GetUserInterface();

  p.SetInputCube("FROM");
  Cube * outCube = p.SetOutputCube("TO");

  // FYI, labels are copied on SetOutputCube call, so In and Out labels match
  PvlObject & isiscube = outCube->label()->findObject("IsisCube");
  PvlGroup & inst = isiscube.findGroup("Instrument");

  // Verify Voyager1 spacecraft
  if (inst["SpacecraftName"][0] != "VOYAGER_1") {
    QString msg = "The cube [" + ui.GetCubeName("FROM") + "] does not appear" +
                  " to be a Voyager1 image";
    throw IException(IException::User, msg, _FILEINFO_);
  }

  // Verify has been radiometrically calibrated
  if (!isiscube.hasGroup("Radiometry")) {
    QString msg = "The cube [" + ui.GetCubeName("FROM") + "] has not been" +
                  "radiometrically corrected, run voycal first";
  }

  // Image time
  iTime time(QString::fromStdString(inst["StartTime"][0]));
  // Day 64, hour 1
  iTime min("1979-03-05T01:00:00.000");
  // Day 64, hour 17
  iTime max("1979-03-05T17:00:00.000");

  // From Isis2, the time range is day 64, hours 1-16, inclusive.
  if (time < min || time >= max) {
    QString message = "The cube [" + ui.GetCubeName("FROM") + "] has image" +
                      " time [" + time.UTC() + "] outside of allowable" +
                      "range [" + min.UTC() + "] to [" + max.UTC() + "]";
    throw IException(IException::User, message, _FILEINFO_);
  }

  PvlGroup & radio = isiscube.findGroup("Radiometry");
  double gain = radio["GainCorrection"];
  double off = radio["OffsetCorrection"];
  double XMLT = radio["XMLT"];

  int startHour = time.Hour() - 1;
  int endHour = time.Hour();

  double event_hr;
  double rawtop, rawmid, rawbot;
  double y1, y2, y3;
  double x1, x2, x3;
  double x1sq, x2sq, x3sq;
  double deta, detb1, detb2, detb3;

  /*
   * You get what you see: This is copied from Isis2's voyramp.
   * Documentation was sparse...
   * I cannot explain what occuring from an understand of the code, only from
   * and understanding of the problem. The paper referenced in the XML
   * provides a graph with 3 curves, offsets for top, middle, and bottom of
   * the frame. These lines are DN offsets by hour. From these values we have
   * to interpolate. So that at any given time with in the range and at any
   * location in the image we may find the correct DN offset. The code below
   * should be accomplishing this.
   */
  event_hr = ((double)time.Hour()) + ((double)time.Minute())/60.0 + time.Second()/3600.0;

  rawtop = interp(table[startHour].w(), table[startHour].x(),
                  table[endHour].w(),   table[endHour].x(), event_hr);
  rawmid = interp(table[startHour].w(), table[startHour].y(),
                  table[endHour].w(),  table[endHour].y(), event_hr);
  rawbot = interp(table[startHour].w(), table[startHour].z(),
                  table[endHour].w(),   table[endHour].z(),event_hr);

  y1 = XMLT * (gain * rawtop + off);
  y2 = XMLT * (gain * rawmid + off);
  y3 = XMLT * (gain * rawbot + off);

  x1 = 1.0;
  x2 = 400.0;
  x3 = 800.0;
  x1sq = x1 * x1;
  x2sq = x2 * x2;
  x3sq = x3 * x3;

  deta = x1sq*(x2-x3) - x1*(x2sq-x3sq) + (x2sq*x3-x2*x3sq);
  detb1=y1*(x2-x3)-x1*(y2-y3)+(y2*x3-x2*y3);
  detb2=x1sq*(y2-y3)-y1*(x2sq-x3sq)+(x2sq*y3-y2*x3sq);
  detb3=x1sq*(x2*y3-y2*x3)-x1*(x2sq*y3-y2*x3sq)+ y1*(x2sq*x3-x2*x3sq);

  // Corrective output DN for ...
  plasmaA = detb1/deta;
  plasmaB = detb2/deta;
  plasmaC = detb3/deta;

  // Create data to go in Radiometry group
  PvlKeyword top = PvlKeyword("TopCorrectiveDN", std::to_string(y1));
  top.addComment("Voyramp plasma torus corrective DN values:");
  PvlKeyword mid = PvlKeyword("MiddleCorrectiveDN", std::to_string(y2));
  PvlKeyword bot = PvlKeyword("BottomCorrectiveDN", std::to_string(y3));

  // Add it
  radio += top;
  radio += mid;
  radio += bot;

  p.StartProcess(ioCorrection);
  p.EndProcess();
}

// Line process

// In Isis2 a mistake was made where instead of line, another value was used
// in calculating the plasmaOffset, after talking to Kris Becker, the
// original author, we determine line should be correct, however, this means
// there is no truth data to verify this.
void ioCorrection(Buffer &in, Buffer &out) {
  plasmaOffset = plasmaA * line * line +
                 plasmaB * line +
                 plasmaC;
  for (int i = 0; i < in.size(); i++) {
    if (IsSpecial(in[i])) {
      out[i] = in[i];
    }
    else {
      out[i] = in[i] - plasmaOffset;
    }
  }
  // Increment line
  line++;
}

// Interpolate between two sets of values at two times.
static double interp(double x1, double y1, double x2, double y2, double x) {
  if (x1 == x2) {
    throw IException(IException::Programmer,
                     "First dependent variables are equal", _FILEINFO_);
  }
  return (y2 + (x - x2) * (y1 - y2) / (x1 - x2));
}
