#include "Camera.h"
#include "ProcessRubberSheet.h"
#include "TProjection.h"
#include "UserInterface.h"

#include "map2cam.h"

using namespace std;

namespace Isis{

  // Global variables
  Cube *mcube;
  Camera *outcam;

  void map2cam_f(UserInterface &ui) {

    // Open the input camera cube that we will be matching and create
    // the camera object
    FileName match = FileName(ui.GetCubeName("MATCH").toStdString());

    Process p;
    QString fname = ui.GetCubeName("MATCH");
    Isis::CubeAttributeInput &inputAtt = ui.GetInputAttribute("MATCH");
    mcube = p.SetInputCube(fname, inputAtt);
    outcam = mcube->camera();

    // Open the input projection cube and get the projection information
    ProcessRubberSheet rub;
    fname = ui.GetCubeName("FROM");
    inputAtt = ui.GetInputAttribute("FROM");
    Cube *icube = rub.SetInputCube(fname, inputAtt);
    TProjection *inmap = (TProjection *) icube->projection();

    // Set up for rubbersheeting
    Transform *transform = new map2cam(icube->sampleCount(),
                                       icube->lineCount(),
                                       inmap,
                                       mcube->sampleCount(),
                                       mcube->lineCount(),
                                       outcam);

   
    // Allocate the output cube but don't propagate any labels from the map
    // file. Instead propagate from the camera file
    rub.PropagateLabels(false);
    fname = ui.GetCubeName("TO");
    Isis::CubeAttributeOutput &outputAtt = ui.GetOutputAttribute("TO");
    rub.SetOutputCube(fname, outputAtt,
                      transform->OutputSamples(),
                      transform->OutputLines(),
                      mcube->bandCount());
    rub.PropagateLabels(QString::fromStdString(match.expanded()));
    rub.PropagateTables(QString::fromStdString(match.expanded()));

    // Set up the interpolator
    Interpolator *interp = NULL;
    if(ui.GetString("INTERP") == "NEARESTNEIGHBOR") {
      interp = new Interpolator(Interpolator::NearestNeighborType);
    }
    else if(ui.GetString("INTERP") == "BILINEAR") {
      interp = new Interpolator(Interpolator::BiLinearType);
    }
    else if(ui.GetString("INTERP") == "CUBICCONVOLUTION") {
      interp = new Interpolator(Interpolator::CubicConvolutionType);
    }

    // See if the camera is band dependent and account for it
    if(!outcam->IsBandIndependent()) {
      rub.BandChange(BandChange);
    }
    
    // Warp the cube
    rub.StartProcess(*transform, *interp);
    rub.EndProcess();

    // Cleanup
    delete transform;
    delete interp;

    p.EndProcess();
  }

  // Transform object constructor
  map2cam::map2cam(const int inputSamples, const int inputLines,
                   TProjection *inmap,
                   const int outputSamples, const int outputLines,
                   Camera *outcam) {
    p_inputSamples = inputSamples;
    p_inputLines = inputLines;
    p_inmap = inmap;

    p_outputSamples = outputSamples;
    p_outputLines = outputLines;
    p_outcam = outcam;
  }

  // Transform method mapping output line/samps to lat/lons to input line/samps
  bool map2cam::Xform(double &inSample, double &inLine,
                      const double outSample, const double outLine) {
    // See if the output image coordinate converts to lat/lon
    if(!p_outcam->SetImage(outSample, outLine)) return false;

    // Get the universal lat/lon and see if it can be converted to input line/samp
    double lat = p_outcam->UniversalLatitude();
    double lon = p_outcam->UniversalLongitude();
    if(!p_inmap->SetUniversalGround(lat, lon)) return false;

    // Make sure the point is inside the input image
    if(p_inmap->WorldX() < 0.5) return false;
    if(p_inmap->WorldY() < 0.5) return false;
    if(p_inmap->WorldX() > p_inputSamples + 0.5) return false;
    if(p_inmap->WorldY() > p_inputLines + 0.5) return false;

    // Everything is good
    inSample = p_inmap->WorldX();
    inLine = p_inmap->WorldY();
    return true;
  }

  int map2cam::OutputSamples() const {
    return p_outputSamples;
  }

  int map2cam::OutputLines() const {
    return p_outputLines;
  }

  void BandChange(const int band) {
    outcam->SetBand(band);
  }
}
