/** This is free and unencumbered software released into the public domain.

The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */

#include "LoCameraFiducialMap.h"

#include "Affine.h"
#include "CameraGroundMap.h"
#include "CameraSkyMap.h"
#include "IString.h"

using namespace std;

namespace Isis {
  /** Constructs mapping between Lunar Orbiter detectors and focal plane x/y.
   *  This method sets the x-axis direction to -1 if the NAIF IK code is even
   *  and to 1 if the code is odd.
   *
   * @param inst  Instrument group from the Pvl labels
   * @param naifIkCode  Naif code of the Lunar Orbiter instrument for reading coefficients
   *
   */
  LoCameraFiducialMap::LoCameraFiducialMap(PvlGroup &inst, const int naifIkCode) {
    // Get the Instrument label information needed to define the fiducial map for this frame
    p_naifIkCode = naifIkCode;
    ReadFiducials(inst);

    // Set the x-axis direction.  The medium camera is reversed.
    int xdir;
    if(naifIkCode % 2 == 0) {
      xdir = -1;
    }
    else {
      xdir = 1;
    }
    CreateTrans(xdir);
  }


  /**
   *
   * Reads the fiducials from the instrument group of the labels
   * @param inst Instrument group from the Pvl labels
   *
   * @throws IException::User - "Unable to read fiducial mapping from cube
   *             labels - Input cube must be processed in Isis 2 through
   *             lofixlabel and converted to Isis with pds2isis"
   */
  void LoCameraFiducialMap::ReadFiducials(PvlGroup &inst) {

    // Try to read the fiducials from the labels
    try {
      // Fiducial mapping to define the Focal Plane map
      PvlKeyword &fSamps = inst["FiducialSamples"];
      PvlKeyword &fLines = inst["FiducialLines"];
      PvlKeyword &fXs = inst["FiducialXCoordinates"];
      PvlKeyword &fYs = inst["FiducialYCoordinates"];

      for(int i = 0; i < fSamps.size(); i++) {
        p_fidSamples.push_back(IString::ToDouble(fSamps[i]));
        p_fidLines.push_back(IString::ToDouble(fLines[i]));
        p_fidXCoords.push_back(IString::ToDouble(fXs[i]));
        p_fidYCoords.push_back(IString::ToDouble(fYs[i]));
      }
    }
    catch(IException &e) {
      string msg = "Unable to read fiducial mapping from cube labels - ";
      msg += "Input cube must be processed in Isis 2 through lofixlabel ";
      msg += "and converted to Isis with pds2isis";
      throw IException(e, IException::User, msg, _FILEINFO_);
    }
  }


  /**
   * Creates focal plane affine transform.
   * @param xdir The x-axis direction.
   *
   *
   * @throws IException::User - "Unable to create fiducial map."
   */
  void LoCameraFiducialMap::CreateTrans(int xdir) {
    // Setup focal plane map
    Affine *fptrans = new Affine();

    try {
      fptrans->Solve(&p_fidSamples[0], (double *) &p_fidLines[0],
                     (double *) &p_fidXCoords[0], (double *) &p_fidYCoords[0],
                     p_fidSamples.size());

    }
    catch(IException &e) {
      string msg = "Unable to create fiducial map.";
      throw IException(IException::User, msg, _FILEINFO_);
    }

    // Get the coefficients
    vector<double> transx;
    vector<double> transy;
    transx = fptrans->Coefficients(1);
    transy = fptrans->Coefficients(2);

    // Medium camera has a reversed x-axis
    for(int icoef = 0; icoef < 3; icoef++) {
      transx[icoef] *= xdir;
    };

    // Correct the Affine order - move the constant to the front
    transx.insert(transx.begin(), transx[2]);
    transx.pop_back();
    transy.insert(transy.begin(), transy[2]);
    transy.pop_back();

    string icode = "INS" + IString(p_naifIkCode);
    string icodex = icode + "_TRANSX";
    string icodey = icode + "_TRANSY";
    pdpool_c(icodex.c_str(), 3, (double( *)) &transx[0]);
    pdpool_c(icodey.c_str(), 3, (double( *)) &transy[0]);

    vector<double> transs;
    vector<double> transl;
    transs = fptrans->InverseCoefficients(1);
    transl = fptrans->InverseCoefficients(2);

    // Correct the Affine order - move the constant to the front
    transs.insert(transs.begin(), transs[2]);
    transs.pop_back();
    transl.insert(transl.begin(), transl[2]);
    transl.pop_back();

    // Medium camera has a reversed x-axis
    transs[1] *= xdir;
    transl[1] *= xdir;

    string icodes = icode + "_ITRANSS";
    string icodel = icode + "_ITRANSL";
    pdpool_c(icodes.c_str(), 3, (double( *)) &transs[0]);
    pdpool_c(icodel.c_str(), 3, (double( *)) &transl[0]);
  }
}
