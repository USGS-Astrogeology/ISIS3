/** This is free and unencumbered software released into the public domain.
The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */
#include "ReseauDistortionMap.h"
#include "LeastSquares.h"
#include "BasisFunction.h"
#include "CameraFocalPlaneMap.h"
#include "Pvl.h"
#include "Statistics.h"
#include <float.h>

using namespace std;
namespace Isis {
  /**
   * Creates a ReseauDistortionMap object
   *
   * @param parent The parent camera model
   * @param labels The pvl labels to get the Reference Reseau location from
   * @param fname The filename containing master reseau location for the
   *              particular camera
   *
   * @throws Isis::IException::User - There are not the same amount of master and
   *                                  refined reseaus
   */
  ReseauDistortionMap::ReseauDistortionMap(Camera *parent, Pvl &labels, const QString &fname) :
    CameraDistortionMap(parent, 1.0) {
    // Set up distortion coefficients
    Pvl mast(fname.toStdString());
    PvlGroup dim = mast.findGroup("Dimensions");
    p_distortedLines = dim.findKeyword("DistortedLines");
    p_distortedSamps = dim.findKeyword("DistortedSamples");
    p_undistortedLines = dim.findKeyword("UndistortedLines");
    p_undistortedSamps = dim.findKeyword("UndistortedSamples");
    PvlGroup mastRes = mast.findGroup("MasterReseaus");
    PvlKeyword mline = mastRes.findKeyword("Line");
    PvlKeyword msamp = mastRes.findKeyword("Sample");
    p_numRes = mline.size();
    if(mline.size() != msamp.size()) {
      string msg = "The number of lines and samples for the master reseaus are";
      msg += "not equal, the data file may be bad";
      throw IException(IException::User, msg, _FILEINFO_);
    }
    for(int i = 0; i < p_numRes; i++) {
      p_mlines.push_back(std::stod(mline[i]));
      p_msamps.push_back(std::stod(msamp[i]));
    }
    p_pixelPitch = parent->PixelPitch();

    PvlGroup refRes = labels.findGroup("Reseaus", Pvl::Traverse);
    PvlKeyword rline = refRes.findKeyword("Line");
    PvlKeyword rsamp = refRes.findKeyword("Sample");
    if(rline.size() != rsamp.size()) {
      string msg = "The number of lines and samples for the refined reseaus are";
      msg += "not equal, the data file may be bad";
      throw IException(IException::User, msg, _FILEINFO_);
    }
    for(int i = 0; i < p_numRes; i++) {
      p_rlines.push_back(std::stod(rline[i]));
      p_rsamps.push_back(std::stod(rsamp[i]));
    }
    if(p_mlines.size() != p_rlines.size()) {
      string msg = "The number of master reseaus and refined reseaus";
      msg += "do not appear to be equal";
      throw IException(IException::User, msg, _FILEINFO_);
    }
  }

  /**
   * Finds the undistorted x/y position of the given distorted point
   *
   * @param dx The distorted x position of the point
   * @param dy The distorted y position of the point
   *
   * @return bool Returns true if the undistortion was completed successful, and
   *              false if it was not
   */
  bool ReseauDistortionMap::SetFocalPlane(const double dx,
                                          const double dy) {
    p_focalPlaneX = dx;
    p_focalPlaneY = dy;

    // Convert distorted x,y position to a sample, line position
    double focalSamp = dx / p_pixelPitch +
                       p_camera->FocalPlaneMap()->DetectorSampleOrigin();
    double focalLine = dy / p_pixelPitch +
                       p_camera->FocalPlaneMap()->DetectorLineOrigin();

    // Find distance from input point to all nominal reseaus
    double distances[p_numRes], wt[5];
    int closepts[5];
    double ldiffsq, sdiffsq;
    for(int i = 0; i < p_numRes; i++) {
      sdiffsq = (focalSamp - p_rsamps[i]) * (focalSamp - p_rsamps[i]);
      ldiffsq = (focalLine - p_rlines[i]) * (focalLine - p_rlines[i]);
      distances[i]  =  ldiffsq + sdiffsq;
    }

    // Get 5 closest reseaus to the input point
    for(int ifpt = 0;  ifpt < 5;  ifpt++)  {
      int imin = 0;
      for(int i = 0;  i < p_numRes;  i++)  {
        if(distances[i] < distances[imin])  imin = i;
      }
      closepts[ifpt] = imin;
      wt[ifpt] = distances[imin];
      distances[imin] = DBL_MAX;  /* Arbitrary big number */
    }

    // Make sure 5 closest points are not colinear
    Statistics lstats, sstats;
    double line[5], samp[5];
    for(int k = 0; k < 5; k++) {
      line[k] = p_rlines[closepts[k]];
      samp[k] = p_rsamps[closepts[k]];
    }
    lstats.AddData(line, 5);
    sstats.AddData(samp, 5);

    // If they are colinear, return false
    if(lstats.StandardDeviation() < 10.0 || sstats.StandardDeviation() < 10.0) {
      return false;
    }

    // Weight each of the 5 closest points, and solve the system of equations
    if(wt[0] > 0.0)   {
      double scale = wt[0];
      double rfitlines[5], rfitsamps[5], mfitlines[5], mfitsamps[5];
      for(int ifpt = 0;  ifpt < 5; ifpt++)   {
        int index = closepts[ifpt];
        rfitlines[ifpt] = p_rlines[index];
        rfitsamps[ifpt] = p_rsamps[index];
        mfitlines[ifpt] = p_mlines[index];
        mfitsamps[ifpt] = p_msamps[index];
        wt[ifpt] = scale / wt[ifpt];
      }
      BasisFunction bsX("bilinearInterpX", 3, 3);
      BasisFunction bsY("bilinearInterpY", 3, 3);
      LeastSquares lsqX(bsX);
      LeastSquares lsqY(bsY);

      vector<double> known;
      known.resize(3);
      for(int i = 0; i < 5; i++) {
        known[0] = 1.0;
        known[1] = rfitsamps[i];
        known[2] = rfitlines[i];
        lsqX.AddKnown(known, mfitsamps[i], wt[i]);
        lsqY.AddKnown(known, mfitlines[i], wt[i]);
      }
      lsqX.Solve();
      lsqY.Solve();

      known[1] = focalSamp;
      known[2] = focalLine;

      // Test to make sure the point is inside of the image
      double undistortedFocalSamp = lsqX.Evaluate(known);
      double undistortedFocalLine = lsqY.Evaluate(known);
      if(undistortedFocalSamp < 0.5) return false;
      if(undistortedFocalLine < 0.5) return false;
      if(undistortedFocalSamp > p_undistortedSamps + 0.5) return false;
      if(undistortedFocalLine > p_undistortedLines + 0.5) return false;

      // Convert undistorted sample, line position to an x,y position
      p_undistortedFocalPlaneX = (undistortedFocalSamp - p_undistortedSamps
                                  / 2.0) * p_pixelPitch;
      p_undistortedFocalPlaneY = (undistortedFocalLine - p_undistortedLines
                                  / 2.0) * p_pixelPitch;
    }
    else   { // If the point passed in is a reseau...
      int index = closepts[0];
      p_undistortedFocalPlaneX = (p_msamps[index] - p_undistortedSamps / 2.0)
                                 * p_pixelPitch;
      p_undistortedFocalPlaneY = (p_mlines[index] - p_undistortedLines / 2.0)
                                 * p_pixelPitch;
    }
    return true;
  }

  /**
   * Finds the distorted x/y position of the given undistorted point
   *
   * @param ux The undistorted x position of the point
   * @param uy The undistorted y position of the point
   *
   * @return bool Returns true if the distortion was completed successful, and
   *              false if it was not
   */
  bool ReseauDistortionMap::SetUndistortedFocalPlane(const double ux,
      const double uy) {
    p_undistortedFocalPlaneX = ux;
    p_undistortedFocalPlaneY = uy;

    // Convert undistorted values to sample, line positions
    double undistortedFocalSamp = ux / p_pixelPitch + p_undistortedSamps / 2.0;
    double undistortedFocalLine = uy / p_pixelPitch + p_undistortedLines / 2.0;

    // Find distance from input point to all nominal reseaus
    double distances[p_numRes], wt[5];
    int closepts[5];
    double ldiffsq, sdiffsq;
    for(int i = 0; i < p_numRes; i++) {
      sdiffsq = (undistortedFocalSamp - p_msamps[i]) *
                (undistortedFocalSamp - p_msamps[i]);
      ldiffsq = (undistortedFocalLine - p_mlines[i]) *
                (undistortedFocalLine - p_mlines[i]);
      distances[i]  =  ldiffsq + sdiffsq;
    }

    // Get 5 closest reseaus to the input point
    for(int ifpt = 0;  ifpt < 5;  ifpt++)  {
      int imin = 0;
      for(int i = 0;  i < p_numRes;  i++)  {
        if(distances[i] < distances[imin])  imin = i;
      }
      closepts[ifpt] = imin;
      wt[ifpt] = distances[imin];
      distances[imin] = DBL_MAX;  /* Arbitrary big number */
    }

    // Make sure 5 closest points are not colinear
    Statistics lstats, sstats;
    double line[5], samp[5];
    for(int k = 0; k < 5; k++) {
      line[k] = p_rlines[closepts[k]];
      samp[k] = p_rsamps[closepts[k]];
    }
    lstats.AddData(line, 5);
    sstats.AddData(samp, 5);
    // If they are colinear, return false
    if(lstats.StandardDeviation() < 10.0 || sstats.StandardDeviation() < 10.0) {
      return false;
    }

    // Weight each of the 5 closest points and solve the system of equations
    if(wt[0] > 0.0)   {
      double scale = wt[0];
      double rfitlines[5], rfitsamps[5], mfitlines[5], mfitsamps[5];
      for(int ifpt = 0;  ifpt < 5; ifpt++)   {
        int index = closepts[ifpt];
        mfitlines[ifpt] = p_mlines[index];
        mfitsamps[ifpt] = p_msamps[index];
        rfitlines[ifpt] = p_rlines[index];
        rfitsamps[ifpt] = p_rsamps[index];
        wt[ifpt] = scale / wt[ifpt];
      }
      BasisFunction bsX("bilinearInterpX", 3, 3);
      BasisFunction bsY("bilinearInterpY", 3, 3);
      LeastSquares lsqX(bsX);
      LeastSquares lsqY(bsY);

      vector<double> known;
      known.resize(3);
      for(int i = 0; i < 5; i++) {
        known[0] = 1.0;
        known[1] = mfitsamps[i];
        known[2] = mfitlines[i];
        lsqX.AddKnown(known, rfitsamps[i], wt[i]);
        lsqY.AddKnown(known, rfitlines[i], wt[i]);
      }
      lsqX.Solve();
      lsqY.Solve();

      known[1] = undistortedFocalSamp;
      known[2] = undistortedFocalLine;

      // Test points to make sure they are in the image
      double distortedFocalSamp = lsqX.Evaluate(known);
      double distortedFocalLine = lsqY.Evaluate(known);
      if(distortedFocalSamp < 0.5) return false;
      if(distortedFocalLine < 0.5) return false;
      if(distortedFocalSamp > p_undistortedSamps + 0.5) return false;
      if(distortedFocalLine > p_undistortedLines + 0.5) return false;

      // Convert distorted sample, line position back to an x,y position
      p_focalPlaneX = (distortedFocalSamp -
                       p_camera->FocalPlaneMap()->DetectorSampleOrigin()) * p_pixelPitch;
      p_focalPlaneY = (distortedFocalLine -
                       p_camera->FocalPlaneMap()->DetectorLineOrigin()) * p_pixelPitch;
    }

    else   { // If the point passed in is a reseau...
      int index = closepts[0];
      p_focalPlaneX = (p_rsamps[index] -
                       p_camera->FocalPlaneMap()->DetectorSampleOrigin()) * p_pixelPitch;
      p_focalPlaneY = (p_rlines[index] -
                       p_camera->FocalPlaneMap()->DetectorLineOrigin()) * p_pixelPitch;
    }
    return true;
  }

} // end namespace isis
