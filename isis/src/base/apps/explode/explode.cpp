/** This is free and unencumbered software released into the public domain.

The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */

#include "explode.h"

#include "ProcessByLine.h"
#include "IException.h"
#include "FileName.h"

namespace Isis {
  
  // Line processing routine
  void CopyBand(Buffer &in, Buffer &out);

  /**
   * Extracts each band of the input cube into a separate one band cube file.
   * Given the output base name of "base", each output cube will be named
   * e.g. base.band#.cub. The appropiate BandBin group will be created.
   *
   * @param ui User Interface with application parameters
   */
  void explode(UserInterface &ui) {

    // open input cube
    Cube icube;
    icube.open(ui.GetCubeName("FROM"));

    explode(&icube, ui);
  }


  /**
   * Extracts each band of the input cube into a separate one band cube file.
   * Given the output base name of "base", each output cube will be named
   * e.g. base.band#.cub. The appropiate BandBin group will be created.
   *
   * @param ui User Interface with application parameters
   * @param icube Input cube
   */
  void explode(Cube *icube, UserInterface &ui) {
    
    Process p;
    p.SetInputCube(icube);
    int samps = icube->sampleCount();
    int lines = icube->lineCount();
    int bands = icube->bandCount();
    QString infile = icube->fileName();

    // We get the output filename so we can add attributes and extensions
    QString outbase = ui.GetCubeName("TO");
    CubeAttributeOutput &outatt = ui.GetOutputAttribute("TO");

    // Loop and extract each band
    for(int band = 1; band <= bands; band++) {
      int pband = icube->physicalBand(band);
      QString sband(toString(pband));

      ProcessByLine p2;
      Progress *prog = p2.Progress();
      prog->SetText("Exploding band " + sband);

      CubeAttributeInput inatt("+" + sband);
      p2.SetInputCube(infile, inatt);

      QString outfile = outbase + ".band";
      if(pband / 1000 == 0) {
        outfile += "0";
        if(pband / 100 == 0) {
          outfile += "0";
          if(pband / 10 == 0) {
            outfile += "0";
          }
        }
      }
      outfile += sband + ".cub";
      p2.SetOutputCube(outfile, outatt, samps, lines, 1);

      p2.StartProcess(CopyBand);
      p2.EndProcess();
    }

    // Cleanup
    p.EndProcess();
  }

  // Line processing routine
  void CopyBand(Buffer &in, Buffer &out) {
    for(int i = 0; i < in.size(); i++) {
      out[i] = in[i];
    }
  }
}
