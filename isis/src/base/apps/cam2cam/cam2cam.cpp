/** This is free and unencumbered software released into the public domain.
The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */

#include <cmath>

#include "cam2cam.h"

#include "Application.h"
#include "Camera.h"
#include "CameraFactory.h"
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
 
    // Check for propagation of off-body (based upon RA/Dec) pixels as well
    // but allow for trimming of off target intersections in FROM file.
    bool offbody = ui.GetBoolean("OFFBODY");
    bool trim = ui.GetBoolean("OFFBODYTRIM");

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
                                            outcam,
                                            offbody,
                                            trim);

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


}
