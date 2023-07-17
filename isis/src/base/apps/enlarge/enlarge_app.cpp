#include "enlarge_app.h"

#include "Application.h"
#include "CubeAttribute.h"
#include "Enlarge.h"
#include "IException.h"
#include "ProcessRubberSheet.h"

using namespace std;
using namespace Isis;
namespace Isis{

  void enlarge(UserInterface &ui, Pvl *log) {
    Cube icube;
    CubeAttributeInput inAtt = ui.GetInputAttribute("FROM");
    if (inAtt.bands().size() != 0) {
      icube.setVirtualBands(inAtt.bands());
    }
    icube.open(ui.GetCubeName("FROM"));
    enlarge(&icube, ui, log);
  }

  void enlarge(Cube *icube, UserInterface &ui, Pvl *log) {
    ProcessRubberSheet p;
    p.SetInputCube(icube);

    // Input number of samples, lines, and bands
    int ins = icube->sampleCount();
    int inl = icube->lineCount();
    int inb = icube->bandCount();
    
    // Output samples and lines
    int ons, onl;

    // Scaling factors
    double sampleScale, lineScale;

    if(ui.GetString("MODE") == "SCALE") {
      // Retrieve the provided scaling factors
      sampleScale = ui.GetDouble("SSCALE");
      lineScale   = ui.GetDouble("LSCALE");

      // Calculate the output size. If there is a fractional pixel, round up
      ons = (int)ceil(ins * sampleScale);
      onl = (int)ceil(inl * lineScale);
    }
    else {
      // Retrieve the provided sample/line dimensions in the output
      ons = ui.GetInteger("ONS");
      onl = ui.GetInteger("ONL");

      // Calculate the scaling factors
      sampleScale = (double)ons / (double)ins;
      lineScale   = (double)onl / (double)inl;
    }

    // Ensure that the calculated number of output samples and lines is greater
    // than the input
    if(ons < ins || onl < inl) {
      string msg = "Number of output samples/lines must be greater than or equal";
      msg = msg + " to the input samples/lines.";
      throw IException(IException::User, msg, _FILEINFO_);
    }

    // Set up the interpolator
    Interpolator *interp;
    if(ui.GetString("INTERP") == "NEARESTNEIGHBOR") {
      interp = new Interpolator(Interpolator::NearestNeighborType);
    }
    else if(ui.GetString("INTERP") == "BILINEAR") {
      interp = new Interpolator(Interpolator::BiLinearType);
    }
    else {  // CUBICCONVOLUTION
      interp = new Interpolator(Interpolator::CubicConvolutionType);
    }
    
    // Allocate the output file, the number of bands does not change in the output
    QString outputFileName = ui.GetCubeName("TO");
    CubeAttributeOutput &att = ui.GetOutputAttribute("TO");
    Cube *ocube = p.SetOutputCube(outputFileName, att, ons, onl, inb);
    
    // Set up the transform object with the calculated scale and number of
    // output pixels
    //Transform *transform = new Enlarge(icube, sampleScale, lineScale);
    Enlarge *transform = new Enlarge(icube, sampleScale, lineScale);
    p.StartProcess(*transform, *interp);
    PvlGroup resultsGrp = transform->UpdateOutputLabel(ocube);

    // Cleanup
    icube->close();
    ocube->close();
    p.EndProcess();

    delete transform;
    delete interp;

    // Write the results to the log
    Application::Log(resultsGrp);
  }
}

