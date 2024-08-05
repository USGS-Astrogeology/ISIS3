/** This is free and unencumbered software released into the public domain.

The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */

#include "bandtrim.h"

#include "Cube.h"
#include "ProcessByBrick.h"
#include "SpecialPixel.h"

namespace Isis {

  // Process to trim spectral pixels if any are null
  void BandTrimSpectral(Buffer &in, Buffer &out);

  /**
   * Bandtrim searches for NULL pixels in all bands of a cube.
   * When a NULL pixel is found the corresponding pixel is set
   * to NULL in all other bands.
   *
   * @param ui User Interface with application parameters
   */
  void bandtrim(UserInterface &ui) {
  
    // open cube
    Cube icube;
    icube.open(ui.GetCubeName("FROM"));

    bandtrim(&icube, ui);
  }


  /**
   * Bandtrim searches for NULL pixels in all bands of a cube.
   * When a NULL pixel is found the corresponding pixel is set
   * to NULL in all other bands.
   *
   * @param icube Input cube
   * @param ui User Interface with application parameters
   */
  void bandtrim(Cube *icube, UserInterface &ui) {
    ProcessByBrick p;
    p.SetInputCube(icube);
    p.SetBrickSize(1, 1, icube->bandCount());

    QString fname = ui.GetCubeName("TO");
    CubeAttributeOutput &atts = ui.GetOutputAttribute("TO");
    p.SetOutputCube(fname, atts);

    p.StartProcess(BandTrimSpectral);
    p.EndProcess();
  }

  // Process to trim spectral pixels if any are null
  void BandTrimSpectral(Buffer &in, Buffer &out) {
    // Copy input to output and check to see if we should null
    bool nullPixels = false;
    for(int i = 0; i < in.size(); i++) {
      out[i] = in[i];
      if(in[i] == Isis::Null) nullPixels = true;
    }

    // Null all pixels in the spectra if necessary
    if(nullPixels) {
      for(int i = 0; i < in.size(); i++) {
        out[i] = Isis::Null;
      }
    }
  }
}