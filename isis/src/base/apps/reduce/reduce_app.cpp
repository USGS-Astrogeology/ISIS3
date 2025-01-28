#include "Application.h"
#include "IException.h"
#include "IString.h"
#include "ProcessByLine.h"
#include "reduce_app.h"
#include "Reduce.h"


#include <cmath>

using namespace std;
namespace Isis{

  void reduce(UserInterface &ui, Pvl *log) {
    try {
      // We will be processing by line
      ProcessByLine p;
      double sscale, lscale;
      int ins, inl, inb;
      int ons, onl;
      vector<QString> bands;
      Cube inCube;

      // To propogate labels, set input cube,
      // this cube will be cleared after output cube is set.
      CubeAttributeInput &inputAtt = ui.GetInputAttribute("FROM");
      p.SetInputCube(ui.GetCubeName("FROM"), inputAtt);

      // Setup the input and output cubes
      QString replaceMode = ui.GetAsString("VPER_REPLACE");
      CubeAttributeInput cai(ui.GetAsString("FROM"));
      bands = cai.bands();

      QString from = ui.GetCubeName("FROM");
      inCube.setVirtualBands(bands);
      inCube.open(from);

      ins = inCube.sampleCount();
      inl = inCube.lineCount();
      inb = inCube.bandCount();

      QString alg  = ui.GetString("ALGORITHM");
      double vper = ui.GetDouble("VALIDPER") / 100.;

      if(ui.GetString("MODE") == "TOTAL") {
        ons = ui.GetInteger("ONS");
        onl = ui.GetInteger("ONL");
        sscale = (double)ins / (double)ons;
        lscale = (double)inl / (double)onl;
      }
      else {
        sscale = ui.GetDouble("SSCALE");
        lscale = ui.GetDouble("LSCALE");
        ons = (int)((double)ins / sscale + 0.5);
        onl = (int)((double)inl / lscale + 0.5);
      }

      if(ons > ins || onl > inl) {
        QString msg = "Number of output samples/lines must be less than or equal";
        msg = msg + " to the input samples/lines.";
        throw IException(IException::User, msg, _FILEINFO_);
      }

      //  Allocate output file
      CubeAttributeOutput &att = ui.GetOutputAttribute("TO");
      Cube *ocube = p.SetOutputCube(ui.GetCubeName("TO"), att, ons, onl, inb);
      // Our processing routine only needs 1
      // the original set was for info about the cube only
      p.ClearInputCubes();

      // Start the processing
      PvlGroup results;
      if(alg == "AVERAGE"){
        Average average(&inCube, sscale, lscale, vper, replaceMode);
        p.ProcessCubeInPlace(average, false);
        results = average.UpdateOutputLabel(ocube);
      }
      else if(alg == "NEAREST") {
        Nearest near(&inCube, sscale, lscale);
        p.ProcessCubeInPlace(near, false);
        results = near.UpdateOutputLabel(ocube);
      }

      // Cleanup
      inCube.close();
      p.EndProcess();

      // Write the results to the log
      Application::Log(results);
    } // REFORMAT THESE ERRORS INTO ISIS TYPES AND RETHROW
    catch (IException &) {
      throw;
    }
    catch (std::exception const &se) {
      QString message = "std::exception: " + (QString)se.what();
      throw IException(IException::User, message, _FILEINFO_);
    }
    catch (...) {
      QString message = "Other Error";
      throw IException(IException::User, message, _FILEINFO_);
    }
  }
}
