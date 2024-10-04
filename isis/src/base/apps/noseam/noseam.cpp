/** This is free and unencumbered software released into the public domain.

The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */

#include "noseam.h"

#include <iostream>
#include <fstream>

//#include "automos.h"
#include "FileList.h"
#include "FileName.h"
#include "Cube.h"
#include "Preference.h"
#include "ProgramLauncher.h"

using namespace std;

namespace Isis {

  /**
   * Noseam creates a mosaic from a list of input cubes using an
   * algorithm that minimizes seams.
   *
   * @param ui UserInterface object containing parameters
   */
  void noseam(UserInterface &ui) {

    // Get Filename with list of cubes to mosaic
    FileName cubeListFileName(ui.GetFileName("FROMLIST"));
    std::cout << "***going to run 2nd noseam method\n";
    return noseam(cubeListFileName, ui);
  }


  /**
   * Noseam creates a mosaic from a list of input cubes using an
   * algorithm that minimizes seams.
   *
   * @param cubeListFileName Filename with list of cubes to mosaic
   * @param ui UserInterface object containing parameters
   */
  void noseam(FileName &cubeListFileName, UserInterface &ui) {

    // Boxcar samples and lines must be odd and 1 or greater
    int samples;
    if (ui.WasEntered("SAMPLES")) {
      samples = ui.GetInteger("SAMPLES");
      
      if (samples < 1 || samples % 2 == 0) {
        string msg = "Value for [SAMPLES] must be odd and greater or equal to 1.";
        throw IException(IException::User, msg, _FILEINFO_);
      }
    }
    else {
      string msg = "Parameter [SAMPLES] must be entered.";
      throw IException(IException::User, msg, _FILEINFO_);
    }

    int lines;
    if (ui.WasEntered("LINES")) {
      lines = ui.GetInteger("LINES");

      if (lines < 1 || lines % 2 == 0) {
        string msg = "Value for [LINES] must be odd and greater or equal to 1.";
        throw IException(IException::User, msg, _FILEINFO_);
      }
    }
    else {
      string msg = "Parameter [LINES] must be entered.";
      throw IException(IException::User, msg, _FILEINFO_);      
    }
    // Get user parameters
    FileList cubes;
    cubes.read(cubeListFileName);

    QString match = ui.GetAsString("MATCHBANDBIN");

    // Sets up the pathName to be used for most application calls
    FileName inFile = cubes[0];

    QString pathName = FileName("$TEMPORARY/").expanded();

    /**
     * Creates a mosaic from the original images.  It is placed here
     * so that the failure MATCHBANDBIN causes does not leave
     * highpasses cubes lying around!
    */
    QString parameters = "FROMLIST=" + cubeListFileName.original() + 
                        " MOSAIC=" + pathName + "OriginalMosaic.cub" +
                        " MATCHBANDBIN=" + match;
    ProgramLauncher::RunIsisProgram("automos", parameters);

    // Creates the highpass cubes from the cubes FileList
    ofstream highPassList;
    highPassList.open("HighPassList.lis");
    for(int i = 0; i < cubes.size(); i++) {
      inFile = cubes[i];
      QString outParam = pathName + inFile.baseName() + "_highpass.cub";
      parameters = "FROM=" + inFile.expanded() +
                   " TO=" + outParam
                   + " SAMPLES=" + toString(samples) + " LINES=" + toString(lines);
      ProgramLauncher::RunIsisProgram("highpass", parameters);
      // Reads the just created highpass cube into a list file for automos
      highPassList << outParam << endl;
    }
    highPassList.close();

    // Makes a mosaic out of the highpass cube filelist
    parameters = "FROMLIST=HighPassList.lis MOSAIC=" + pathName + "HighpassMosaic.cub"
                 + " MATCHBANDBIN=" + match;
    ProgramLauncher::RunIsisProgram("automos", parameters);

    // Does a lowpass on the original mosaic
    parameters = "FROM=" + pathName + "OriginalMosaic.cub"
                 + " TO=" + pathName + "LowpassMosaic.cub"
                 + " SAMPLES=" + toString(samples) + " LINES=" + toString(lines);
    ProgramLauncher::RunIsisProgram("lowpass", parameters);

    // Finally combines the highpass and lowpass mosaics
    parameters = "FROM=" + pathName + "HighpassMosaic.cub" +
                 " FROM2=" + pathName + "LowpassMosaic.cub" +
                 " TO=" + ui.GetCubeName("TO") +
                 " OPERATOR= add";
    ProgramLauncher::RunIsisProgram("algebra", parameters);

    // Will remove all of the temp files by default
    if(ui.GetBoolean("REMOVETEMP")) {
      QString file("HighPassList.lis");
      remove(file.toLatin1().data());
      file = pathName + "HighpassMosaic.cub";
      remove(file.toLatin1().data());
      file = pathName + "LowpassMosaic.cub";
      remove(file.toLatin1().data());
      file = pathName + "OriginalMosaic.cub";
      remove(file.toLatin1().data());

      for(int i = 0; i < cubes.size(); i++) {
        inFile = cubes[i];
        file = pathName + inFile.baseName() + "_highpass.cub";
        remove(file.toLatin1().data());
      }
    }
  }
}
