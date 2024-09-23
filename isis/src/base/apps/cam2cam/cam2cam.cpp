#include <cmath>

#include "cam2cam.h"

#include "Application.h"
#include "Camera.h"
#include "CameraFactory.h"
#include "Distance.h"
#include "ProcessRubberSheet.h"

using namespace std;

namespace Isis {

  void BandChange(const int band);

  // Global variables
  namespace cam2camGlobal {
    Camera *incam;
  }

  /**
   * Convert the pixels of a camera image to the geometry of a different camera
   * image.
   *
   * @param ui The User Interface to parse the parameters from
   */
  void cam2cam(UserInterface &ui) {
    Cube *icube = new Cube();
    icube->open(ui.GetCubeName("FROM"));
    Cube *mcube = new Cube();
    mcube->open(ui.GetCubeName("MATCH"));
    return cam2cam(icube, mcube, ui);
  }


  /**
   * Convert the pixels of a camera image to the geometry of a different camera
   * image. This is the programmatic interface to the ISIS cam2cam
   * application.
   *
   * @param icube input cube to be transformed
   * @param mcube input cube to transform the icube to match
   * @param ui The User Interface to parse the parameters from
   */
  void cam2cam(Cube *icube, Cube *mcube, UserInterface &ui) {

    ProcessRubberSheet m;
    m.SetInputCube(mcube);
    Cube *ocube = m.SetOutputCube(ui.GetCubeName("TO"), ui.GetOutputAttribute("TO"),
                                  mcube->sampleCount(), mcube->lineCount(), mcube->bandCount());

    // Set up the default reference band to the middle of the cube
    // If we have even bands it will be close to the middle
    int referenceBand = ocube->bandCount();
    referenceBand += (referenceBand % 2);
    referenceBand /= 2;

   // See if the user wants to override the reference band
    if (ui.WasEntered("REFBAND")) {
      referenceBand = ui.GetInteger("REFBAND");
    }

    // Using the Camera method out of the object opack will not work, because the
    // filename required by the Camera is not passed by the process class in this
    // case.  Use the CameraFactory to create the Camera instead to get around this
    // problem.
    Camera *outcam = mcube->camera();

    // Set the reference band we want to match
    PvlGroup instgrp = mcube->group("Instrument");
    if (!outcam->IsBandIndependent()) {
      PvlKeyword rBand("ReferenceBand", toString(referenceBand));
      rBand.addComment("# All bands are aligned to reference band");
      instgrp += rBand;
      mcube->putGroup(instgrp);
      delete outcam;
      outcam = NULL;
    }

    // Only recreate the output camera if it was band dependent
    if (outcam == NULL) outcam = CameraFactory::Create(*mcube);

    // We might need the instrument group later, so get a copy before clearing the input
    //   cubes.
    m.ClearInputCubes();

    m.SetInputCube(icube);
    cam2camGlobal::incam = icube->camera();

    // Set up the transform object which will simply map
    // output line/samps -> output lat/lons -> input line/samps
    Transform *transform = new cam2camXform(icube->sampleCount(),
                                            icube->lineCount(),
                                            cam2camGlobal::incam,
                                            ocube->sampleCount(),
                                            ocube->lineCount(),
                                            outcam);

    // Add the reference band to the output if necessary
    ocube->putGroup(instgrp);

    // Set up the interpolator
    Interpolator *interp = NULL;
    if (ui.GetString("INTERP") == "NEARESTNEIGHBOR") {
      interp = new Interpolator(Interpolator::NearestNeighborType);
    }
    else if (ui.GetString("INTERP") == "BILINEAR") {
      interp = new Interpolator(Interpolator::BiLinearType);
    }
    else if (ui.GetString("INTERP") == "CUBICCONVOLUTION") {
      interp = new Interpolator(Interpolator::CubicConvolutionType);
    }

    // See if we need to deal with band dependent camera models
    if (!cam2camGlobal::incam->IsBandIndependent()) {
      m.BandChange(BandChange);
    }

    // Warp the cube
    m.StartProcess(*transform, *interp);
    m.EndProcess();

    // Cleanup
    delete transform;
    delete interp;
  }


  // Transform object constructor
  cam2camXform::cam2camXform(const int inputSamples, const int inputLines,
                             Camera *incam, const int outputSamples,
                             const int outputLines, Camera *outcam) {
    p_inputSamples = inputSamples;
    p_inputLines = inputLines;
    p_incam = incam;

    p_outputSamples = outputSamples;
    p_outputLines = outputLines;
    p_outcam = outcam;
  }

  // Transform method mapping output line/samps to lat/lons to input line/samps
  bool cam2camXform::Xform(double &inSample, double &inLine,
                           const double outSample, const double outLine) {
    // See if the output image coordinate converts to lat/lon
    if (!p_outcam->SetImage(outSample, outLine)) return false;

    // Get the universal lat/lon and see if it can be converted to input line/samp
    double lat = p_outcam->UniversalLatitude();
    double lon = p_outcam->UniversalLongitude();
    Distance rad = p_outcam->LocalRadius();
    if (rad.isValid()) {
      if (!p_incam->SetUniversalGround(lat, lon, rad.meters())) return false;
    }
    else {
      if (!p_incam->SetUniversalGround(lat, lon)) return false;
    }

    // Make sure the point is inside the input image
    if (p_incam->Sample() < 0.5) return false;
    if (p_incam->Line() < 0.5) return false;
    if (p_incam->Sample() > p_inputSamples + 0.5) return false;
    if (p_incam->Line() > p_inputLines + 0.5) return false;

    // Everything is good
    inSample = p_incam->Sample();
    inLine = p_incam->Line();
    return true;
  }


  int cam2camXform::OutputSamples() const {
    return p_outputSamples;
  }


  int cam2camXform::OutputLines() const {
    return p_outputLines;
  }

}
