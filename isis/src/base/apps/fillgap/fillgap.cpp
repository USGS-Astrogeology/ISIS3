#include <QString>

#include "Application.h"
#include "ProcessBySample.h"
#include "ProcessByLine.h"
#include "ProcessBySpectra.h"
#include "NumericalApproximation.h"
#include "SpecialPixel.h"
#include "IException.h"

#include "fillgap.h"

using namespace std;

namespace Isis {
  static NumericalApproximation::InterpType iType(NumericalApproximation::CubicNatural);

  void fill(Buffer  &in, Buffer &out);

  //Global variables
  int numSpecPixKept;

  void fillgap(UserInterface &ui, Pvl *log) {

    Cube *cubeFile = new Cube();
    CubeAttributeInput inAtt = ui.GetInputAttribute("FROM");
    if (inAtt.bands().size() != 0) {
      cubeFile->setVirtualBands(inAtt.bands());
    }
    cubeFile->open(ui.GetCubeName("FROM"), "r");

    fillgap(cubeFile, ui, log);
  }

  void fillgap(Cube *inCube, UserInterface &ui, Pvl *log) {
    // initialize global variables
    numSpecPixKept = 0;
    CubeAttributeOutput &att = ui.GetOutputAttribute("TO");


    // set spline interpolation to user requested type
    QString splineType = ui.GetString("INTERP");
    if(splineType == "LINEAR") {
      iType = NumericalApproximation::Linear;
    }
    else if(splineType == "AKIMA") {
      iType = NumericalApproximation::Akima;
    }
    else {
      iType = NumericalApproximation::CubicNatural;
    }

    //Set null direction to the user defined direction
    QString  Dir = ui.GetString("DIRECTION");
    if(Dir == "SAMPLE") {
      ProcessBySample p;
      p.SetInputCube(inCube);
      p.SetOutputCube(ui.GetCubeName("TO"), att);
      p.StartProcess(fill);
      p.EndProcess();
    }
    else if(Dir == "LINE") {
      ProcessByLine p;
      p.SetInputCube(inCube);
      p.SetOutputCube(ui.GetCubeName("TO"), att);
      p.StartProcess(fill);
      p.EndProcess();
    }
    else if(Dir == "BAND") {
      ProcessBySpectra p;
      p.SetInputCube(inCube);
      p.SetOutputCube(ui.GetCubeName("TO"), att);
      p.StartProcess(fill);
      p.EndProcess();
    }

    // if any pixels were not filled, let user know by adding message to the log
    if(numSpecPixKept > 0) {
      PvlGroup mLog("Messages");
      mLog += PvlKeyword("Warning",
                        "Unable to fill " + toString(numSpecPixKept) + " special pixels.");
      Application::Log(mLog);
    }
    return;
  }

  /**
   * @brief Fill in gaps of image using an interpolation on the DN
   *        values.
   *
   * @param in Input Buffer
   * @param out Output Buffer
   * @internal
   *   @history 2009-04-21 Jeannie Walldren - Added a try/catch
   *            statement to the Evaluate() method.  The only
   *            reason for Evaluate to throw an error should be
   *            when (j+1) is outside the spline's domain.  This
   *            happens when there is at least one special pixel
   *            at an endpoint of the buffer.  Rather than
   *            extrapolating, we keep the original value.  If
   *            these pixels are not in a corner, the user can
   *            fill these by running the app again in a different
   *            direction. Otherwise, a box filter may be used.
   *            The global variable numSpecPixKept is incremented so we
   *            can keep a count of how many pixels were not
   *            filled by this app.
   */
  void fill(Buffer &in, Buffer &out) {

    // Fill the data set with valid pixel values
    NumericalApproximation spline(iType);
    for(int i = 0; i < in.size(); i++) {
      if(!IsSpecial(in[i])) {
        spline.AddData((double)(i + 1), in[i]);
      }
    }

    // loop through output buffer
    for(int j = 0 ; j < out.size() ; j++) {
      // if the input pixel is valid, copy it
      if(!IsSpecial(in[j])) {
        out[j] = in[j];
      }
      // otherwise, try to interpolate from the valid values
      else {
        try {
          out[j] = spline.Evaluate((double)(j + 1));
        }
        // if Evaluate() fails, copy the input value and increment numSpecPixKept
        catch(IException &e) {
          out[j] = in[j];
          numSpecPixKept++;
        }
      }
    }
    return;
  }
}