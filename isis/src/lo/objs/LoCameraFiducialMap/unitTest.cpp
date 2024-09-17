/** This is free and unencumbered software released into the public domain.

The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */

#include <iomanip>
#include <iostream>

#include "Camera.h"
#include "CameraFactory.h"
#include "CameraFocalPlaneMap.h"
#include "FileName.h"
#include "IException.h"
#include "iTime.h"
#include "Preference.h"
#include "Pvl.h"

using namespace std;
using namespace Isis;

int main(void) {
  Preference::Preferences(true);

  cout << "Unit Test for LoCameraFiducialMap..." << endl;

  try {

    cout << "Testing medium fiducial fit on a 100 x 100 cropped frame" << endl;
    Cube cm("$ISISTESTDATA/isis/src/lo/unitTestData/3083_med_raw.cub", "r");
    Camera *camm = CameraFactory::Create(cm);

    cout << "FileName: " << FileName(cm.fileName().toStdString()).name() << endl;
    cout << "CK Frame: " << camm->instrumentRotation()->Frame() << endl << endl;
    cout.setf(std::ios::fixed);
    cout << setprecision(9);

    // Test all four corners to make sure the conversions are correct
    // Upper left pixel
    double samp = 1.0;
    double line = 1.0;
    double knownUpperLeftX = 37.38041454316298;
    double knownUpperLeftY = -31.62938549926507;

    // Now Convert from detector to distorted focal plane
    if(camm->FocalPlaneMap()->SetDetector(samp, line)) {
      double focalPlaneX = camm->FocalPlaneMap()->FocalPlaneX();
      double focalPlaneY = camm->FocalPlaneMap()->FocalPlaneY();
      if(fabs(focalPlaneX - knownUpperLeftX) < 1E-9 &&
          fabs(focalPlaneY - knownUpperLeftY) < 1E-9) {
        cout << "Upper left corner OK" << endl;
      }
      else {
        cout << setprecision(16) << "Upper left X off by: " << focalPlaneX - knownUpperLeftX << endl;
        cout << setprecision(16) << "Upper left Y off by: " << focalPlaneY - knownUpperLeftY << endl;
      }
    }

    // Upper right pixel
    samp = camm->Samples();
    double knownUpperRightX = 36.6926459512808;
    double knownUpperRightY = -31.62980917422247;

    // Now Convert from detector to distorted focal plane
    if(camm->FocalPlaneMap()->SetDetector(samp, line)) {
      double focalPlaneX = camm->FocalPlaneMap()->FocalPlaneX();
      double focalPlaneY = camm->FocalPlaneMap()->FocalPlaneY();

      if(fabs(focalPlaneX - knownUpperRightX) < 1E-9 &&
          fabs(focalPlaneY - knownUpperRightY) < 1E-9) {
        cout << "Upper right corner OK" << endl;
      }
      else {
        cout << setprecision(16) << "Upper right X off by: " << focalPlaneX - knownUpperRightX << endl;
        cout << setprecision(16) << "Upper right Y off by: " << focalPlaneY - knownUpperRightY << endl;
      }
    }

    // Lower left pixel
    samp = 1.0;
    line = camm->Lines();
    double knownLowerLeftX = 37.37931706369603;
    double knownLowerLeftY = -30.94212343336155;

    // Now Convert from detector to distorted focal plane
    if(camm->FocalPlaneMap()->SetDetector(samp, line)) {
      double focalPlaneX = camm->FocalPlaneMap()->FocalPlaneX();
      double focalPlaneY = camm->FocalPlaneMap()->FocalPlaneY();
      if(fabs(focalPlaneX - knownLowerLeftX) < 1E-9 &&
          fabs(focalPlaneY - knownLowerLeftY) < 1E-9) {
        cout << "Lower left corner OK" << endl;
      }
      else {
        cout << setprecision(16) << "Lower left X off by: " << focalPlaneX - knownLowerLeftX << endl;
        cout << setprecision(16) << "Lower left Y off by: " << focalPlaneY - knownLowerLeftY << endl;
      }
    }

    // Lower right pixel
    samp = camm->Samples();
    line = camm->Lines();
    double knownLowerRightX = 36.69154847181385;
    double knownLowerRightY = -30.94254710831895;

    // Now Convert from detector to distorted focal plane
    if(camm->FocalPlaneMap()->SetDetector(samp, line)) {
      double focalPlaneX = camm->FocalPlaneMap()->FocalPlaneX();
      double focalPlaneY = camm->FocalPlaneMap()->FocalPlaneY();
      if(fabs(focalPlaneX - knownLowerRightX) < 1E-9 &&
          fabs(focalPlaneY - knownLowerRightY) < 1E-9) {
        cout << "Lower right corner OK" << endl;
      }
      else {
        cout << setprecision(16) << "Lower right X off by: " << focalPlaneX - knownLowerRightX << endl;
        cout << setprecision(16) << "Lower right Y off by: " << focalPlaneY - knownLowerRightY << endl;
      }
      cout  << endl << "-------------------------------------" << endl;
    }

    cout << "Testing high fiducial fit on a full 34530 x 8750 frame" << endl;
    Cube ch("$ISISTESTDATA/isis/src/lo/unitTestData/4164H_Full_mirror.cub", "r");
    Camera *camh = CameraFactory::Create(ch);

    cout << "FileName: " << FileName(ch.fileName().toStdString()).name() << endl;
    cout << "CK Frame: " << camh->instrumentRotation()->Frame() << endl << endl;
    cout.setf(std::ios::fixed);
    cout << setprecision(9);

    // Test all four corners and center pixel to make sure the conversions are correct
    // Upper left pixel
    samp = 1.0;
    line = 1.0;
    knownUpperLeftX = 125.8894771096694;
    knownUpperLeftY = -31.23569821243375;

    // Now Convert from detector to distorted focal plane
    if(camh->FocalPlaneMap()->SetDetector(samp, line)) {
      double focalPlaneX = camh->FocalPlaneMap()->FocalPlaneX();
      double focalPlaneY = camh->FocalPlaneMap()->FocalPlaneY();
      if(fabs(focalPlaneX - knownUpperLeftX) < 1E-9 &&
          fabs(focalPlaneY - knownUpperLeftY) < 1E-9) {
        cout << "Upper left corner OK" << endl;
      }
      else {
        cout << setprecision(16) << "Upper left X off by: " << focalPlaneX - knownUpperLeftX << endl;
        cout << setprecision(16) << "Upper left Y off by: " << focalPlaneY - knownUpperLeftY << endl;
      }
    }

    // Upper right pixel
    samp = camh->Samples();
    knownUpperRightX = -114.13710918057000754;
    knownUpperRightY = -31.21483142132193;

    // Now Convert from detector to distorted focal plane
    if(camh->FocalPlaneMap()->SetDetector(samp, line)) {
      double focalPlaneX = camh->FocalPlaneMap()->FocalPlaneX();
      double focalPlaneY = camh->FocalPlaneMap()->FocalPlaneY();

      if(fabs(focalPlaneX - knownUpperRightX) < 1E-9 &&
          fabs(focalPlaneY - knownUpperRightY) < 1E-9) {
        cout << "Upper right corner OK" << endl;
      }
      else {
        cout << setprecision(18) << "Upper right X off by: " << focalPlaneX - knownUpperRightX << endl;
        cout << setprecision(18) << "Upper right Y off by: " << focalPlaneY - knownUpperRightY << endl;
      }
    }

    // Lower left pixel
    samp = 1.0;
    line = camh->Lines();
    knownLowerLeftX = 125.9405233275204;
    knownLowerLeftY = 29.52307388869981;

    // Now Convert from detector to distorted focal plane
    if(camh->FocalPlaneMap()->SetDetector(samp, line)) {
      double focalPlaneX = camh->FocalPlaneMap()->FocalPlaneX();
      double focalPlaneY = camh->FocalPlaneMap()->FocalPlaneY();
      if(fabs(focalPlaneX - knownLowerLeftX) < 1E-9 &&
          fabs(focalPlaneY - knownLowerLeftY) < 1E-9) {
        cout << "Lower left corner OK" << endl;
      }
      else {
        cout << setprecision(16) << "Lower left X off by: " << focalPlaneX - knownLowerLeftX << endl;
        cout << setprecision(16) << "Lower left Y off by: " << focalPlaneY - knownLowerLeftY << endl;
      }
    }

    // Lower right pixel
    samp = camh->Samples();
    line = camh->Lines();
    knownLowerRightX = -114.0860629627191;
    knownLowerRightY = 29.54394067981162;

    // Now Convert from detector to distorted focal plane
    if(camh->FocalPlaneMap()->SetDetector(samp, line)) {
      double focalPlaneX = camh->FocalPlaneMap()->FocalPlaneX();
      double focalPlaneY = camh->FocalPlaneMap()->FocalPlaneY();
      if(fabs(focalPlaneX - knownLowerRightX) < 1E-9 &&
          fabs(focalPlaneY - knownLowerRightY) < 1E-9) {
        cout << "Lower right corner OK" << endl;
      }
      else {
        cout << setprecision(16) << "Lower right X off by: " << focalPlaneX - knownLowerRightX << endl;
        cout << setprecision(16) << "Lower right Y off by: " << focalPlaneY - knownLowerRightY << endl;
      }
    }

    // Center pixel
    samp = camh->Samples() / 2;
    line = camh->Lines() / 2;
    double knownCenterX = 5.90517988076963;//119.9912428434887;
    double knownCenterY = -0.8493513951460194;//-30.39329207495764;

    // Now Convert from detector to distorted focal plane
    if(camh->FocalPlaneMap()->SetDetector(samp, line)) {
      double focalPlaneX = camh->FocalPlaneMap()->FocalPlaneX();
      double focalPlaneY = camh->FocalPlaneMap()->FocalPlaneY();
      if(fabs(focalPlaneX - knownCenterX) < 1E-9 &&
          fabs(focalPlaneY - knownCenterY) < 1E-9) {
        cout << "Center corner OK" << endl;
      }
      else {
        cout << setprecision(16) << "Center X off by: " << focalPlaneX - knownCenterX << endl;
        cout << setprecision(16) << "Center Y off by: " << focalPlaneY - knownCenterY << endl;
      }
    }

    delete camh;
  }

  catch(IException &e) {
    e.print();
  }
}
