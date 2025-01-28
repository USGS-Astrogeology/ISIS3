/** This is free and unencumbered software released into the public domain.

The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */

#include "photrim.h"

#include "Camera.h"
#include "ProcessByLine.h"
#include "SpecialPixel.h"

namespace Isis {

  /*
   * The photrim program trims pixels outside of the phase, incidence,
   * and emission angles by setting them to "null" within all bands of
   * the cube. A user can either trim using the program's default method
   * or the USEDEM method.
   * 
   * @param ui UserInterface object containing parameters
   */
  void photrim(UserInterface &ui) {

    // open input cube
    Cube icube;
    icube.open(ui.GetCubeName("FROM"));

    photrim(&icube, ui);
  }


  /*
   * The photrim program trims pixels outside of the phase, incidence,
   * and emission angles by setting them to "null" within all bands of
   * the cube. A user can either trim using the program's default method
   * or the USEDEM method.
   * 
   * @param iCube Input cube
   * @param ui UserInterface object containing parameters
   */
  void photrim(Cube *icube, UserInterface &ui) {
    // processing by line
    ProcessByLine p;

    // Setup input cube and get the camera model
    p.SetInputCube(icube);

    Camera* cam = icube->camera();

    // Create the output cube
    QString fname = ui.GetCubeName("TO");
    CubeAttributeOutput &atts = ui.GetOutputAttribute("TO");
    p.SetOutputCube(fname, atts);

    // Get the trim angles
    double minPhase = ui.GetDouble("MINPHASE");
    double maxPhase = ui.GetDouble("MAXPHASE");
    double minEmission = ui.GetDouble("MINEMISSION");
    double maxEmission = ui.GetDouble("MAXEMISSION");
    double minIncidence = ui.GetDouble("MININCIDENCE");
    double maxIncidence = ui.GetDouble("MAXINCIDENCE");
    
    bool usedem = ui.GetBoolean("USEDEM");
    
    if (!usedem) {
      cam->IgnoreElevationModel(true);
    }

    // Start the processing
    int lastBand = 0;

    // lambda function with captures to process by line
    auto photrimLineProcess = [&](Buffer &in, Buffer &out)->void {
      // See if there is a change in band which would change the camera model
      if (in.Band() != lastBand) {
        lastBand = in.Band();
        cam->SetBand(icube->physicalBand(lastBand));
      }

      // Loop for each pixel in the line.
      double samp, phase, emission, incidence;
      double line = in.Line();
      for (int i = 0; i < in.size(); i++) {
        samp = in.Sample(i);
        cam->SetImage(samp, line);
        if (cam->HasSurfaceIntersection()) {
          if (((phase = cam->PhaseAngle()) < minPhase) || (phase > maxPhase)) {
            out[i] = Isis::NULL8;
          }
          else if (((emission = cam->EmissionAngle()) < minEmission) ||
                  (emission > maxEmission)) {
            out[i] = Isis::NULL8;
          }
          else if (((incidence = cam->IncidenceAngle()) < minIncidence) ||
                  (incidence > maxIncidence)) {
            out[i] = Isis::NULL8;
          }
          else {
            out[i] = in[i];
          }
        }
        // Trim outerspace
        else {
          out[i] = Isis::NULL8;
        }
      }
    };

    p.StartProcess(photrimLineProcess);
    p.EndProcess();
  }
}
