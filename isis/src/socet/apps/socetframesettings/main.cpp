/** This is free and unencumbered software released into the public domain.

The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */

#include "Isis.h"

#include <cstdlib>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <math.h>
#include <sstream>
#include <stdlib.h>

#include <QFileInfo>

#include "Camera.h"
#include "CameraDetectorMap.h"
#include "CameraDistortionMap.h"
#include "CameraFocalPlaneMap.h"
#include "CameraGroundMap.h"
#include "Constants.h"
#include "FileName.h"
#include "IException.h"
#include "IString.h"
#include "iTime.h"
#include "NaifStatus.h"
#include "Process.h"
#include "Projection.h"
#include "Pvl.h"
#include "PvlObject.h"
#include "Spice.h"

using namespace std;
using namespace Isis;

void getCamPosOPK(Spice &spice, QString spacecraftName, SpiceDouble et, Camera *cam,
                  SpiceDouble ographicCamPos[3], SpiceDouble omegaPhiKappa[3],
                  SpiceDouble isisFocalPlane2SocetPlateTranspose[3][3]);

void IsisMain() {

  // Use a regular Process
  Process p;

  UserInterface &ui = Application::GetUserInterface();
  QString from = ui.GetCubeName("FROM");
  QString to = FileName(ui.GetFileName("TO")).expanded();
  QString socetProject = ui.GetString("SS_PROJECT");
  QString socetImageLocation = ui.GetString("SS_IMG_LOC");
  QString socetInputDataPath = ui.GetString("SS_INPUT_PATH");
  QString socetCameraCalibrationPath = ui.GetString("SS_CAM_CALIB_PATH");

  // Open input cube and make sure this is a lev1 image (ie, not map projected)
  Cube cube;
  cube.open(from);

  if (cube.isProjected()) {
    QString msg = QString("You can only create a SOCET Set Framing Camera or FrameOffAxis settings "
                          "file for level 1 images. The input image [%1] is a map projected, level "
                          "2, cube.").arg(from);
    throw IException(IException::User, msg, _FILEINFO_);
  }

  // Initialize the camera
  Cube *input = p.SetInputCube("FROM");
  Camera *cam = input->camera();
  CameraDetectorMap *detectorMap = cam->DetectorMap();
  CameraFocalPlaneMap *focalMap = cam->FocalPlaneMap();

  // Make sure the image contains the SPICE blobs/tables
  PvlGroup test = cube.label()->findGroup("Kernels", Pvl::Traverse);
  QString instrumentPointing = QString::fromStdString(test["InstrumentPointing"]);
  if (instrumentPointing != "Table") {
    QString msg = QString("Input image [%1] does not contain needed SPICE blobs.  Please run "
                          "spiceinit on the image with attach=yes.").arg(from);
    throw IException(IException::User, msg, _FILEINFO_);
  }

  // Set the image at the boresight pixel to get the ephemeris time and SPICE data at that image
  // location
  double detectorSampleOrigin = focalMap->DetectorSampleOrigin();
  double detectorLineOrigin = focalMap->DetectorLineOrigin();
  cam->SetImage(detectorSampleOrigin, detectorLineOrigin);
  SpiceDouble et = cam->time().Et();

  Spice spice(*input);
  spice.setTime(et);

  // Get required keywords from instrument and band groups
  PvlGroup inst = cube.label()->findGroup("Instrument", Pvl::Traverse);
  QString instrumentId = QString::fromStdString(inst["InstrumentId"]);
  QString spacecraftName = QString::fromStdString(inst["SpacecraftName"]);

  // Compensate for noproj altering cube labels
  if (instrumentId == "IdealCamera") {
    PvlGroup orig = cube.label()->findGroup("OriginalInstrument", Pvl::Traverse);
    instrumentId = QString::fromStdString(orig["InstrumentId"]);
    spacecraftName = QString::fromStdString(orig["SpacecraftName"]);
  }

  // Get sensor position and orientation (opk) angles
  SpiceDouble ographicCamPos[3] = { 0.0, 0.0, 0.0 };
  SpiceDouble omegaPhiKappa[3] = { 0.0, 0.0, 0.0 };
  SpiceDouble isisFocalPlane2SocetPlateTranspose[3][3] = { { 0.0, 0.0, 0.0 },
                                                           { 0.0, 0.0, 0.0 },
                                                           { 0.0, 0.0, 0.0 } };

  getCamPosOPK(spice, spacecraftName, et, cam, ographicCamPos,
               omegaPhiKappa,isisFocalPlane2SocetPlateTranspose);

  // Determine the SOCET Set camera calibration file
  QString socetCamFile = socetCameraCalibrationPath;

  if (spacecraftName == "VIKING_ORBITER_1") {
    if (instrumentId == "VISUAL_IMAGING_SUBSYSTEM_CAMERA_A") {
      socetCamFile += "VIK1A.cam";
    }
    else {
      socetCamFile += "VIK1B.cam";
    }
  }
  else if (spacecraftName == "VIKING_ORBITER_2") {
    if (instrumentId == "VISUAL_IMAGING_SUBSYSTEM_CAMERA_A") {
      socetCamFile += "VIK2A.cam";
    }
    else {
      socetCamFile += "VIK2B.cam";
    }
  }

  //----------------------------------------.-------------
  //TO DO: Uncomment these lines when MEX SRC is supported
  //----------------------------------------.-------------
  //  // Mars Express
  //  else if (spacecraftName == "MARS_EXPRESS") {
  //    socetCamFile += "SRC.cam";
  //  }
  //-----------------------------------------------------
  //TO DO: Uncomment these lines when Themis is supported
  //-----------------------------------------------------
  //  // THEMIS VIS images (MARS Odyssey)
  //  else if (spacecraftName == "MARS_ODYSSEY") {
  //    socetCamFile += "THEMIS_VIS_F3.cam";
  //  }
  //-----------------------------------------------------
  //TO DO: Uncomment these lines when Apollo is supported
  //-----------------------------------------------------
  //  else if (spacecraftName == "APOLLO 15") {
  //    socetCamFile += "Apollo15_M_ASU.cam";
  //  }
  //  else if (spacecraftName == "APOLLO 16") {
  //    socetCamFile += "Apollo16_M_ASU.cam";
  //  }
  //  else if (spacecraftName == "APOLLO 17") {
  //    socetCamFile += "Apollo17_M_ASU.cam";
  //  }
  else if (spacecraftName == "Galileo Orbiter") {
    //Check if this image was aquired with the cover on or off
    iTime removeCoverDate("1994/04/01 00:00:00");
    iTime imageDate(QString::fromStdString(inst["StartTime"]));

    if (imageDate < removeCoverDate) {
      socetCamFile += "Galileo_SSI_Cover.cam";
    }
    else {
      socetCamFile += "Galileo_SSI.cam";
    }
  }
  else if (spacecraftName == "Cassini-Huygens") {
    // Get the image filter and replace "/" with "_"
    PvlGroup bandBin = cube.label()->findGroup("BandBin", Pvl::Traverse);
    QString filter = QString::fromStdString(bandBin["FilterName"]);
    filter.replace("/", "_");

    socetCamFile += "Cassini_ISSNA_";
    socetCamFile += filter;
    socetCamFile += ".cam";
  }
  else if (spacecraftName == "Messenger") {
    if (instrumentId == "MDIS-NAC") {
      socetCamFile += "MDIS_NAC.cam";
    }
    else {
      socetCamFile += "MDIS_WAC.cam";
    }
  }
  else if (spacecraftName == "CLEMENTINE 1") {
    if (instrumentId == "UVVIS") {
      socetCamFile += "ClemUVVIS.cam";
    }
  }
  else if (spacecraftName == "OSIRIS-REX") {
    if (instrumentId == "MapCam") {
      socetCamFile += "OCAMS_MapCam.cam";
    }
    else if (instrumentId == "PolyCam") {
      socetCamFile += "OCAMS_PolyCam.cam";
    }
    else {
      QString msg = QString("The ISIS to SOCET Set translation of input image "
                            "[%1] is currently not supported for OSIRIS-REX "
                            "instrument [%2].").arg(from).arg(instrumentId);
      throw IException(IException::User, msg, _FILEINFO_);
    }
  }
  // Throw exception for unsupported camera
  else {
    QString msg = QString("The ISIS to SOCET Set translation of input image [%1] is currently "
                          "not supported for instrument [%2].").arg(from).arg(instrumentId);
    throw IException(IException::User, msg, _FILEINFO_);
  }

  // For THEMIS VIS, Galileo SSI, Cassini ISS get the image summation mode
  int summation = 1;
  //-----------------------------------------------------
  //TO DO: Uncomment these lines when Themis is supported
  //-----------------------------------------------------
  //  if (spacecraftName == "MARS_ODYSSEY") {
  //    try {
  //      summation = (int) detectorMap->SampleScaleFactor();
  //    }
  //    catch (IException &e) {
  //      QString msg = "Error reading SpatialSumming from Instrument label";
  //      throw IException(IException::User, msg, _FILEINFO_);
  //    }
  //  }

  if (spacecraftName == "Galileo Orbiter") {
    try {
      summation = (int) detectorMap->SampleScaleFactor();
    }
    catch (IException &e) {
      QString msg = "Error reading Summing from Instrument label";
      throw IException(IException::User, msg, _FILEINFO_);
    }
  }

  if (spacecraftName == "Cassini-Huygens") {
    try {
      summation = (int) detectorMap->SampleScaleFactor();
    }
    catch (IException &e) {
      QString msg = "Error reading Summing from Instrument label";
      throw IException(IException::User, msg, _FILEINFO_);
    }
  }

  // Get NL/NS of image and calculate the size in x/y dimensions, in mm
  // Note: for THEMIS VIS, Galileo SSI and Cassini ISS summed images, calculate the size of the full
  // resolution image because our "isis2socet" scripts will enlarge the summed image for import into
  // Socet Set
  double pixelSize = 1.0 / cam->PixelPitch();
  int numLines = cube.lineCount();
  int numSamples = cube.sampleCount();
  if (summation > 1) {
    // For Themis VIS, Galileo SSI, Cassini ISS:
    numLines *= summation;
    numSamples *= summation;
  }
  double sizeX = numSamples / pixelSize;
  double sizeY = numLines / pixelSize;

  // Make sure the Socet Set project name has the .prj extension
  if (socetProject.endsWith(".prj", Qt::CaseInsensitive) == false)  socetProject += ".prj";

  // Find cube base name w/o extensions & establish the Socet Set support file name
  // Note: I'm using the QFileInfo class because the baseName method in the ISIS
  // FileName class only strips the last extension, and we need the core name
  // of the file without any extensions, or path
  QString baseName = QFileInfo(from).baseName();
  QString socetSupFile = baseName + ".sup";

  //  Open and write to the SOCET Set Framing Camera settings file keywords and values
  //  If this is a Messenger image, add the temperature-dependent focal length so as to overrride
  //  the nominal focal lenghth stored in the SOCET Set camera calibration files
  ofstream toStrm;

  toStrm.open (to.toLatin1().data(), ios::trunc);
  if (toStrm.bad()) {
    QString msg = "Unable to open output settings file";
    throw IException(IException::User, msg, _FILEINFO_);
  }

  toStrm << "setting_file                        1.1\n";
  toStrm << "multi_frame.project                 " << socetProject << endl;
  toStrm << "multi_frame.cam_calib_filename      " << socetCamFile << endl;
  toStrm << "multi_frame.create_files            IMAGE_AND_SUPPORT\n";
  toStrm << "multi_frame.atmos_ref               0\n";
  toStrm << "multi_frame.auto_min                YES\n";
  toStrm << "multi_frame.digital_cam             NO\n";
  toStrm << "multi_frame.input_image_filename    " << socetInputDataPath + baseName +
                                                 ".raw" << endl;
  toStrm << "multi_frame.output_format           img_type_vitec\n";
  toStrm << "multi_frame.output_name             " << socetSupFile << endl;
  toStrm << "multi_frame.output_location         " << socetImageLocation << endl;
  toStrm << "multi_frame.cam_loc_ang_sys         OPK\n";
  toStrm << "multi_frame.cam_loc_ang_units       UNIT_DEGREES\n";
  toStrm << "multi_frame.cam_loc_xy_units        UNIT_DEGREES\n";

  if (spacecraftName == "Messenger") {
    // Overide the nominal focal length in the SOCET SET camera calibration file with the
    // Temperature Dependent Focal Length used in ISIS
    double focalLength = cam->FocalLength();
    toStrm << "multi_frame.cam_loc_focal           " << setprecision(17) << focalLength
                                                                 << endl;
  }
  toStrm << "multi_frame.cam_loc_y_or_lat        " << setprecision(17) <<
                                                       ographicCamPos[0] << endl;
  toStrm << "multi_frame.cam_loc_x_or_lon        " << ographicCamPos[1] << endl;
  toStrm << "multi_frame.cam_loc_elev            " << ographicCamPos[2] << endl;
  toStrm << "multi_frame.cam_loc_omega           " << omegaPhiKappa[0] << endl;
  toStrm << "multi_frame.cam_loc_phi             " << omegaPhiKappa[1] << endl;
  toStrm << "multi_frame.cam_loc_kappa           " << omegaPhiKappa[2] << endl;
  toStrm << "multi_frame.img_size_lines          " << numLines << endl;
  toStrm << "multi_frame.img_size_samps          " << numSamples << endl;
  toStrm << "multi_frame.sizex                   " << setprecision(6) << sizeX << endl;
  toStrm << "multi_frame.sizey                   " << sizeY << endl;
  toStrm << "multi_frame.orientation             1\n";

  // Furthermore, if this is a Messenger image, get the needed keywords values needed for the
  // USGSAstro FrameOffAxis *support* file, and add them to the output settings file.
  // During frame import in SOCET Set, these values will be ignored, but then later accessed by
  // the USGSAstro import_frame SOCET Set program.
  //
  // Note: Summed Messenger images are handled in the FrameOffAxis sensor model, so no need to
  // account for enlarging Messenger images in the "socet2isis" scripts

  if (spacecraftName == "Messenger") {
    double originalHalfLines = numLines / 2.0;
    double originalHalfSamples = numSamples / 2.0;

    // Set the lens distortion coefficients
    // Note: These values were calculated for SOCET Set by Orrin Thomas in an MSExcel spreadsheet,
    // and are hardcoded here
    QString lenscoX;
    QString lenscoY;
    if (instrumentId == "MDIS-WAC") {
      lenscoX = QString("1.0913499678359500E-06 1.0000181809155400E+00 5.2705094712778700E-06 "
                        "7.3086112844249500E-05 -2.1503011755973800E-06 -3.5311655893430800E-08 "
                        "-5.3312743384716000E-06 -1.4642661005550900E-07 -5.4770856997706100E-06 "
                        "-1.2364567692453900E-07 0.0000000000000000E+00 0.0000000000000000E+00 "
                        "0.0000000000000000E+00 0.0000000000000000E+00 0.0000000000000000E+00");

      lenscoY = QString("-4.8524316760252900E-08 -5.2704844291112000E-06 1.0000181808487100E+00 "
                        "2.4702140905559800E-09 7.3084305868732200E-05 -2.1478354889239300E-06 "
                        "1.2364567791040000E-07 -5.4663905009059100E-06 1.4516772126792600E-07 "
                        "-5.3419626374895400E-06 0.0000000000000000E+00 0.0000000000000000E+00 "
                        "0.0000000000000000E+00 0.0000000000000000E+00 0.0000000000000000E+00");
    }
    else {
      //MDIS-NAC lens distortion coefficients:
      lenscoX = QString("-0.000000000000005 0.997948053760188 0.000000000000000 0.000000000000000 "
                        "0.000542184519158 0.000000000000000 -0.000007008182254 0.000000000000000 "
                        "-0.000006526474815 0.000000000000000 0.000000000000000 0.000000000000000 "
                        "0.000000000000000 0.000000000000000 0.000000000000000");

      lenscoY = QString("-0.000003746900328 0.000000000000000 0.999999575428613 -0.000880501428960 "
                        "0.000000000000000 -0.000332760373453 0.000000000000000 -0.000008067196812 "
                        "0.000000000000000 -0.000007553955548  0.000000000000000  0.000000000000000 "
                        "0.000000000000000  0.000000000000000  0.000000000000000");
    }

    // Get the image summation
    double sampleSumming = (int) detectorMap->SampleScaleFactor();
    double lineSumming = (int) detectorMap->LineScaleFactor();

    // Get the Starting Detector Line/Sample
    double startingSample = detectorMap->AdjustedStartingSample();
    double startingLine = detectorMap->AdjustedStartingLine();

    // Get the image plane corrdinates to pixel coordinates transformation matrices
    const double *iTransS = focalMap->TransS();
    const double *iTransL = focalMap->TransL();

    // Because of the options for applying light-time correction, capture the pertinent
    // ISIS keywords as a record to be stored in the settingsfile
    // Note: these values will not go into the Socet Set support file)
    QString ikCode;
    QString swapObserverTarget;
    QString lightTimeCorrection;
    QString ltSurfaceCorrect;
    PvlObject naifKeywordsObject = cube.label()->findObject("NaifKeywords");
    if (instrumentId == "MDIS-NAC") {
      ikCode = "236820";
      swapObserverTarget = QString::fromStdString(naifKeywordsObject["INS-236820_SWAP_OBSERVER_TARGET"]);
      lightTimeCorrection = QString::fromStdString(naifKeywordsObject["INS-236820_LIGHTTIME_CORRECTION"]);
      ltSurfaceCorrect = QString::fromStdString(naifKeywordsObject["INS-236820_LT_SURFACE_CORRECT"]);
    }
    else {
      ikCode = "236800";
      swapObserverTarget = QString::fromStdString(naifKeywordsObject["INS-236800_SWAP_OBSERVER_TARGET"]);
      lightTimeCorrection = QString::fromStdString(naifKeywordsObject["INS-236800_LIGHTTIME_CORRECTION"]);
      ltSurfaceCorrect = QString::fromStdString(naifKeywordsObject["INS-236800_LT_SURFACE_CORRECT"]);
    }

    toStrm << "\nSENSOR_TYPE FrameOffAxis" << endl;
    toStrm << "USE_LENS_DISTORTION 1" << endl;
    toStrm << "ORIGINAL_HALF_LINES " <<  originalHalfLines << endl;
    toStrm << "ORIGINAL_HALF_SAMPLES " << originalHalfSamples << endl;
    toStrm << "LENSCOX " << lenscoX << endl;
    toStrm << "LENSCOY " << lenscoY << endl;
    toStrm << "SAMPLE_SUMMING  " << sampleSumming << endl;
    toStrm << "LINE_SUMMING  " << lineSumming << endl;
    toStrm << "STARTING_DETECTOR_SAMPLE " << setprecision(17) << startingSample << endl;
    toStrm << "STARTING_DETECTOR_LINE " << startingLine << endl;
    toStrm << "SAMPLE_BORESIGHT " << detectorSampleOrigin << endl;
    toStrm << "LINE_BORESIGHT " << detectorLineOrigin << endl;
    toStrm << "INS_ITRANSS";
    for (int i = 0; i < 3; i++)
      toStrm << " " << setprecision(14) << iTransS[i];
    toStrm << endl;
    toStrm << "INS_ITRANSL";
    for (int i = 0; i < 3; i++)
      toStrm << " " << iTransL[i];
    toStrm << endl;
    toStrm << "M_SOCET2ISIS_FOCALPLANE " << setprecision(2) <<
              isisFocalPlane2SocetPlateTranspose[0][0] << " " <<
              isisFocalPlane2SocetPlateTranspose[0][1] << " " <<
              isisFocalPlane2SocetPlateTranspose[0][2] << endl;
    toStrm << "                         " <<
              isisFocalPlane2SocetPlateTranspose[1][0] << " " <<
              isisFocalPlane2SocetPlateTranspose[1][1] << " " <<
              isisFocalPlane2SocetPlateTranspose[1][2] << endl;
    toStrm << "                         " <<
              isisFocalPlane2SocetPlateTranspose[2][0] << " " <<
              isisFocalPlane2SocetPlateTranspose[2][1] << " " <<
              isisFocalPlane2SocetPlateTranspose[2][2] << endl;
    toStrm << "INS-" << ikCode << "_SWAP_OBSERVER_TARGET = '" << swapObserverTarget << "'\n";
    toStrm << "INS-" << ikCode << "_LIGHTTIME_CORRECTION = '" << lightTimeCorrection << "'\n";
    toStrm << "INS-" << ikCode << "_LT_SURFACE_CORRECT = '" << ltSurfaceCorrect <<"'\n";
  }

} //End IsisMain


////////////////////////////////////////////////////////////////////////

// OVERVIEW

// getCamPosOPK converts the geometry contained in ISIS Cube labels, and
// passed via Spice and Cam object arguments, into camera position
// (lat,Elon,height) and camera attitude Euler angles (omega,phi,kappa; OPK)
// sensor model parameters understood by SOCET SET.  The conversion is
// dependent on spacecraft- and instrument-dependent parameters, which may
// not be contained in any ISIS Cube label, and so have been hard-coded here.


// DETAILS

// Camera position is returned in the planetographic(Note 1) [latitude,
// East-positive longitude, height] coordinate system, with longitudes
// in the (-180:+180] degree range.
//
// The OPK angles are Euler angles(Note 2), in degrees, representing a
// 3x3 matrix that converts
//
//   vectors expressed in the target body-fixed LSR(Note 3) frame, as
//   defined for SOCET SET,
//
// to
//
//   vectors expressed in the SOCET SET (SS) plate (SS camera focal plane)
//   frame (Note 0).
//
// For the USGSAstro FrameOffAxis sensor model, the transpose of the rotation
// matrix from ISIS (camera) focal plane to SOCET SET focal plane coordinates
// is also returned.
//
// The matrix represented by the OPK Euler angles is calculated by chaining
// together four known matrices (or their transposes):
//
//   i) LSR to Planetocentric body fixed   - spice.instrumentPosition()
//  ii) Planetocentric body fixed to J2000 - cam->bodyRotation()
// iii) J2000 to ISIS camera               - cam->instrumentRotation()
//  iv) ISIS camera to SS plate            - per-instrument, hard-coded
//
// The conversion from ISIS plate frame to SOCET plate frame is dependent on
// the mission- or instrument-specific conventions used in ISIS and SS:
//
// a) ISIS image data layout
//
//    The definitions of ISIS samples and lines are the fastest- and
//    slowest-moving indices, respectively, of the 2D ISIS Cube image data,
//    starting at the beginning of the image data in the file.  The number
//    of samples per line, and lines per image, is specified in the ISIS
//    Cube label.
//
// b) ISIS reference frame definition wrt image data layout
//
//    The ISIS boresight is either +Z or -Z, so X and Y are in the ISIS image
//    focal plane, and are defined in either the mission Frame-Kernel (FK),
//    or in the ISIS Instrument Addendum Kernel (IAK).  The sample and line
//    scales and directions with respect to ISIS +X and +Y are stored
//    within SPICE kernel pool variables (KPVS) INS-#####_ITRANSS and
//    INS-####_ITRANSL, respectively, in units of pixel/mm.
//
//    The Instrument Addendum Kernels (IAKs), which are ISIS-specific SPICE
//    Instrument Kernels (IKs), exist for each instrument, and contain those
//    _ITRANSS and _ITRANSL parameters, e.g.
//
//      INS-98903_ITRANSS = ( 0.0,  -76.9,   0.0  )
//      INS-98903_ITRANSL = ( 0.0,    0.0,  76.9 )
//
//    _ITRANSS is a triplet:  the 2nd value is the sample pixel/mm wrt +X;
//                            the 3rd value is the sample pixel/mm wrt +Y.
//
//    _ITRANSL is a triplet:  the 2nd value is the line pixel/mm wrt +X;
//                            the 3rd value is the line pixel/mm wrt +Y.
//
//
//    The first values in _ITRANSS and _ITRANSL may be ignored as long as
//    they are zero. If these values are non-zero, then the transformation is
//    an affine transformation where the values are the offset.
//
// c) ISIS display
//
//    ISIS display software always displays the first pixel (sample) at the
//    upper left, the next line-worth of (sample) pixels to the right of
//    that, and the second line below the first line, and so on.  This may,
//    in some cases, display a mirrored image from the actual scene, and
//    projecting the boresight (+/- Z), +X and +Y against the display will
//    ***IN THAT CASE*** dipslay a left-handed frame i.e. a mirrored
//    right-handed frame.
//
// The conventions in SOCET SET (SS) are assumed to be as follows (see also
// Note 0 below):
//
// a) SS image data layout
//
//    Identical to ISIS definitions
//
// b) SS reference frame wrt image layout
//
//    The SS +sample direction is along the SS +X axis (+Xss displays to the
//    right), and the SS -line direction is along the +Y axis (+Yss displays
//    up).  +Z is perpendicular to the SS focal plane and points away from
//    the imaged scene, so the SS frame displays a right-handed frame.
//
// c) SS display
//
//    The SS pixel display convention is similar to that for ISIS (start at
//    at top-left, then right with increasing sample - fastest index, then
//    down with increasing line - slowest index), but it does ***NOT***
//    allow for mirrored images.
//
// The instrument-specific pixel storage convention used in ISIS Cubes,
// coupled with the _ITRANSS/_ITRANSL KPVs and the mapping of the pixel
// storage from ISIS to SS determines calculation of the matrix.  Copying
// ISIS Cube image pixel data to SS raw images is a process that is external
// to this application, and therefore must be coordinated with the output of
// this application.


// NOTES

// 0) The definition of the SOCET SET (SS) plate reference frame is virtually
//    undocumented in the public domain.  From what is available, it appears
//    that
//
//    i) +zSS is the anti-boresight (normal to the SS focal plane away from the
//       direction of the imaged scene)
//
//    ii) +xSS is typically displayed to the right, and +ySS is displayed up
//        in the right-handed system, HOWEVER ...
//
//    iii) It may be that the orientations of +xSS and +ySS are dependent on
//         the SOCET cam file used; that file is external to this appication,
//         and the per-spacecraft and/or per-instrument if-else clauses in
//         this application make assumptions about the contents of that
//         external file.

// (1) Planetographic coordinates are referred to as geodetic coordinates
//     in SPICE.  Planetographic latitude and height are normal and
//     relative, repsectively, to the surface of a target modeled as a
//     spheroid that is a volume of revolution.  Only the radius at the
//     intersection of the prime meridian and the equator, and the polar
//     radius, are used; the intermediate radius, at the intersection of
//     meridians +/-90degrees and the equator, of the tri-axial ellipsoid
//     model is not used.
//
//     N.B. Planetographic coordinates are used ***ONLY*** for calculating
//          the camera position; they are ***NOT*** used in not used in
//          the calculation of the camera attitude OPK angles.

// (2) Specifically, LsrToSsMatrix = [kappa]  [phi]  [omega]
//                                          3      2        1
//     where
//
//       [angleN]
//               axisN
//
//     represents a *FRAME* rotation by angleN about the coordinate axis
//     indexed by axisN.  N.B. the result of one
//
//       [angleN]
//               axisN
//
//     frame rotation is a matrix, which rotates vectors by -angleN radians
//     about the axisN coordinate axis cf. SPICE RECGEO documentation
//
//       https://naif.jpl.nasa.gov/pub/naif/toolkit_docs/C/cspice/recgeo_c.html

// (3)  LSR frame - Local Space Rectangular frame
//      - A.k.a. ENU frame, East-North-Up (What's up?  East cross North)
//      The LSR frame is based on the position of the instrument in the
//      target body-fixed (BF) frame, where
//
//      +zLSR => Vector from target body cent to camera position. N.B. this
//               vector is planetocentric and different from the
//               plantographic (geodetic) coordinates described in (1) above.
//
//      +xLSR => Direction of east longitude at sub-camera point, equal to
//               cross product of planetocentric North polar axis (+zBF)
//               vector with +zLSR vector (above).
//
//      +yLSR => In same half-plane of xzLSR plane as +zBF (north, or
//               positive rotation, pole)


void getCamPosOPK(Spice &spice, QString spacecraftName, SpiceDouble et, Camera *cam,
                  SpiceDouble ographicCamPos[3], SpiceDouble omegaPhiKappa[3],
                  SpiceDouble isisFocalPlane2SocetPlateTranspose[3][3]) {

  // Unit vectors along positive and negative principal axes:
  //
  //                                       [<=== -Z ===>]
  //                                       |    [<=== -Y ===>]
  //                                       |    |    [<=== -X ====>]
  SpiceDouble uvData[8] = { 0.0, 0.0, 1.0, 0.0, 0.0, -1.0, 0.0, 0.0 };
  //                        [<=== +Z ===>]   |    |
  //                             [<=== +Y ==>]    |
  //                                  [<=== +X ==>]
  SpiceDouble* uvPlusZ = uvData + 0;
  //Currently not used:
  //SpiceDouble* uvPlusY = uvData + 1;
  //SpiceDouble* uvPlusX = uvData + 2;
  //SpiceDouble* uvMinusZ = uvData + 3;
  //SpiceDouble* uvMinusY = uvData + 4;
  //SpiceDouble* uvMinusX = uvData + 5;

  // Initialize the isisFocalPlane2SocetPlate matrix based on mission
  //  and/or instrument.
  //
  // isisCam2SocetPlate is the Rotation matrix from ISIS focal plane coordinates
  // to Socet Set plate/focal plane coordinates
  // For Socet, we need  +Xss = +SAMPLEss
  //                     +Yss = -LINEss
  //                     +Zss = anti-boresight
  //
  // N.B. +X and +Y are dependent on how pixels are stored in SOCET SET
  //      .raw files, and that process is external to this application,
  //      so the hard-coded per-instrument choices made and parameters
  //      set here make assumptions about that process.  For the most
  //      part, the SS storage order is the same as in ISIS CUBs; the
  //      only exceptions, as of 2017-11-16, are OSIRIS-REx MapCam and
  //      PolyCam.

  SpiceDouble isisFocalPlane2SocetPlate[3][3] = { { 0.0, 0.0, 0.0 },
                                                  { 0.0, 0.0, 0.0 },
                                                  { 0.0, 0.0, 0.0 } };
  //-----------------------------------------------------
  //TO DO: Uncomment these lines when Apollo is supported
  //-----------------------------------------------------
  //  if (spacecraftName == "APOLLO 15" || spacecraftName == "APOLLO 16") {
  //    isisFocalPlane2SocetPlate[0][0] = 1.0;
  //    isisFocalPlane2SocetPlate[1][1] = -1.0;
  //    isisFocalPlane2SocetPlate[2][2] = -1.0;
  //  }
  //  else if (spacecraftName == "APOLLO 17") {
  //    isisFocalPlane2SocetPlate[0][0] = -1.0;
  //    isisFocalPlane2SocetPlate[1][1] = 1.0;
  //    isisFocalPlane2SocetPlate[2][2] = -1.0;
  //  }
  //-----------------------------------------------------
  //TO DO: Uncomment these lines when MEX-SRC is supported
  //-----------------------------------------------------
  //  if (spacecraftName == "VIKING_ORBITER_1" || spacecraftName == "VIKING_ORBITER_2" ||
  //      spacecraftName == "CLEMENTINE 1"     || spacecraftName == "MARS_EXPRESS") {
  //-----------------------------------------------------
  //TO DO: Delete the next two lines when MEX-SRC is supported
  //-----------------------------------------------------
  if (spacecraftName == "VIKING_ORBITER_1" || spacecraftName == "VIKING_ORBITER_2" ||
      spacecraftName == "CLEMENTINE 1") {
    isisFocalPlane2SocetPlate[1][0] = -1.0;
    isisFocalPlane2SocetPlate[0][1] = -1.0;
    isisFocalPlane2SocetPlate[2][2] = -1.0;
  }

  //-----------------------------------------------------
  //TO DO: Uncomment these lines when Themis-VIS is supported
  //-----------------------------------------------------
  //  if (spacecraftName == "MARS_ODYSSEY"    || spacecraftName == "Galileo Orbiter" ||
  //      spacecraftName == "Cassini-Huygens" || spacecraftName == "Messenger" ) {
  //-----------------------------------------------------
  //TO DO: Delete this next line when Themis-VIS is supported
  //-----------------------------------------------------
  else if (spacecraftName == "Galileo Orbiter" ||
           spacecraftName == "Cassini-Huygens" ||
           spacecraftName == "Messenger") {
    isisFocalPlane2SocetPlate[0][0] = 1.0;
    isisFocalPlane2SocetPlate[1][1] = -1.0;
    isisFocalPlane2SocetPlate[2][2] = -1.0;
  }

  /*********************************************************************

 OSIRIS-REx (ORX) spacecraft, MapCam and PolyCam instrument conventions
 ======================================================================

 _______________________________________________________________________
 - MapCam and PolyCam FITS

   - Pixels displayed left-to-right (+NAXIS1) and up (+NAXIS2)
     - Yields image as seen on sky

   - From IK orx_ocams_v06.ti (-64361 and -64360 are Map and Poly):

      INS-64361_BORESIGHT         = ( 0 0 1 )
      INS-64361_SPOC_FITS_NAXIS1  = (  0.0,  1.0, 0.0 )
      INS-64361_SPOC_FITS_NAXIS2  = (  1.0,  0.0, 0.0 )

      INS-64360_BORESIGHT         = ( 0 0 1 )
      INS-64360_SPOC_FITS_NAXIS1  = (  0.0,  1.0, 0.0 )
      INS-64360_SPOC_FITS_NAXIS2  = (  1.0,  0.0, 0.0 )

   - Boresight is -Zfits (instrument frame)
   - +NAXIS1 == RIGHT == +Yfits (instrument frame)
   - +NAXIS2 == UP    == +Xfits (instrument frame)


   - Pixels displayed left-to-right (+NAXIS1) and up (+NAXIS2)
     - Yields image as seen on sky

 _______________________________________________________________________
 - MapCam and PolyCam ISIS

   - Pixels are displayed left-to-right (+SAMPLEisis) and down (+LINEisis)

   - Pixels are stored in the same order in OREX ISIS Cubes as they are
     in FITS files
     - +SAMPLEisis == +NAXIS1(fits)
     - +LINEisis == +NAXIS2(fits)

   - N.B. So OREX ISIS image display is mirrored about horizontal axis
          w.r.t.  as seen on sky

   - MapCam and PolyCam ISIS use FITS reference frame

     - +Xisis == +Xfits
     - +Yisis == +Yfits
     - +Zisis == +Zfits
     - BORESIGHTisis == -Zisis

   - From ORX ISIS IAK (extracted from ORX CUB labels):

                             dSample    dSample          dSample
                             -------    -------          -------
                              dBand?      dX               dY

       INS-64360_ITRANSS = (     0.0,    0.0,            117.64705882353 )
       INS-64361_ITRANSS = (     0.0,    0.0,            117.64705882353 )


                              dLine    dLine              dLine
                             -------   -----              -----
                              dBand?    dX                 dY

       INS-64360_ITRANSL = (     0.0,  117.64705882353,    0.0           )
       INS-64361_ITRANSL = (     0.0,  117.64705882353,    0.0           )

   - dSample/dXisis = 0, dSample/dYisis > 0: +SAMPLEisis = +Yisis = right
   -   dLine/dXisis > 0,   dLine/dYisis = 0: +LINEisis   = +Xisis = down

   - N.B. since BORESIGHT == -Zisis, ISIS displays a left-handed frame

 _______________________________________________________________________
 - MapCam and PolyCam SOCET SET (SS)

   - Pixels are displayed left-to-right (+SAMPLEss) and down (+LINEss)

   - SS conventions

     - +Xss = right = +SAMPLEss
     - +Yss = up = -LINEss
     - +Zss = anti-boresight
     - -Zss = boresight

     - Displaying a left-handed frame is not allowed in SS

   - So SS image pixel storage must be mirrored wrt ISIS image pixel storage

   - Make assumption here that a process, external to this application
     (socetframesettings), will mirror ISIS image pixels about horizontal
     (line) axis when writing SS raw image pixels e.g. use ISIS [flip]
     application.

     - So +LINEss = -LINEisis

   - Final relationship between ISIS focal plane frame and SS plate frame:

     - +Xss = +SAMPLEss      = +SAMPLEisis = +Yisis
     - +Yss = -LINEss        = +LINEisis   = +Xisis
     - +Zss = anti-boresight               = -Zisis


 _______________________________________________________________________
 - Visual summary of conventions and choices above:

   - Characters "fits" and "isis" represent faux image data displayed as
     they would be in SS, assuming they are legible when they are displayed
     according to native FITS and ISIS display conventions, respectively.


       +Yss  ^
             |
             |
             |
             |+Zss (out of screen)
           --O-----------------------------------> +SAMPLEss
             |                                     +Xss
             |
             |      X--------------------------------------------------+
             |      |(1,1)ss                                           |
             |      |                                                  |
    +LINEss  V      |       _                                          |
                    |      | \                                         |
                    |      |    *                                      |
                    |      |           |     ___                       |
                    |      |    |    --+--  /   \                      |
                    |    --+--  |      |    \___                       |
                    |      |    |      |        \                      |
                    |      |    |_/    |_/  \___/                      |
                    |                                                  |
                    |                                                  |
                    |       _    ___    _    ___                       |
                    |      | \  /   \  | \  /   \                      |
                    |      |     ___/  |     ___/                      |
                    |      |    /      |    /                          |
                    |      |    \___/  |    \___/                      |
                    |                                                  |
 +NAXIS2fits ^      |      *           *                               |
 +LINEisis   |      |                                                  |
 +Xfits      |      |                                                  |
 +Xisis      |      |(1,1)isis == (0,0)fits                            |
             |      X--------------------------------------------------+
             |
             |
             |+Zfits = +Zisis (into screen)
          ---X---------------------------------> +NAXIS1fits
             |                                   +SAMPLEisis
             |                                   +Yfits
             |                                   +Yisis

 +Xss == +Yisis == +Yfits
 +Yss == +Xisis == +Xfits
 +Zss == -Zisis == -Zfits

   ********************************************************************/

  else if (spacecraftName == "OSIRIS-REX") {
    /* MapCam and PolyCam ISIS-to-SS Matrix swaps X and Y, inverts Z */
    isisFocalPlane2SocetPlate[1][0] =  1.0;  // +Xisis => +Yss
    isisFocalPlane2SocetPlate[0][1] =  1.0;  // +Yisis => +Xss
    isisFocalPlane2SocetPlate[2][2] = -1.0;  // +Zisis => -Zss
  }

  // Confirm that matrix is now a rotation matrix
  else {
    QString msg = QString("The ISIS to SOCET Set translation of input image is currently "
                          "not supported for instrument [%1].").arg(spacecraftName);
    throw IException(IException::User, msg, _FILEINFO_);
  }

  //////////////////////////////////////////////////////////////////////
  //////////////////////////////////////////////////////////////////////
  // End of section setting isisFocalPlane2SocetPlate
  //////////////////////////////////////////////////////////////////////
  //////////////////////////////////////////////////////////////////////

  //____________________________________________________________________
  // From Camera object, fetch rotation matrices that convert vectors from
  // J2000 inertial frame to Ocentric frame and to ISIS Focal Plane frame.
  // - (Planet-)Ocentric => target body-fixed [+X = PMxEq.; +Z = TPRP]
  // - PMxEq. => intersection of target Prime Merdian and Equator
  // - TPRP => Target Positive Rotation Pole, typically = North
  vector<double>  j2000ToOcentricMatrixVector = cam->bodyRotation()->Matrix();
  vector<double>  j2000ToIsisFocalPlaneMatrixVector = cam->instrumentRotation()->Matrix();

  // Reformat rotation matrices from 9-element vector<double>'s to 3x3 arrays
  SpiceDouble j2000ToOcentricRotationMatrix[3][3] = { { 0.0, 0.0, 0.0 },
                                                      { 0.0, 0.0, 0.0 },
                                                      { 0.0, 0.0, 0.0 } };
  SpiceDouble j2000ToIsisFocalPlaneMatrix[3][3] = { { 0.0, 0.0, 0.0 },
                                                    { 0.0, 0.0, 0.0 },
                                                    { 0.0, 0.0, 0.0 } };

  for (int j = 0; j < 3; j++) {
    for (int k = 0; k < 3; k++) {
      j2000ToOcentricRotationMatrix[j][k] = j2000ToOcentricMatrixVector[3 * j + k];
      j2000ToIsisFocalPlaneMatrix[j][k] = j2000ToIsisFocalPlaneMatrixVector[3 * j + k];
    }
  }

  // Compute rotation matrix from ISIS Focal Plane frame to Ocentric frame
  SpiceDouble isisFocalPlaneToOcentricRotationMatrix[3][3] = { { 0.0, 0.0, 0.0 },
                                                               { 0.0, 0.0, 0.0 },
                                                               { 0.0, 0.0, 0.0 } };
  mxmt_c(j2000ToOcentricRotationMatrix, j2000ToIsisFocalPlaneMatrix,
         isisFocalPlaneToOcentricRotationMatrix);

  // Get instrumemt position vector, convert to meters
  SpiceDouble instrumentPosition[3] = { 0.0, 0.0, 0.0 };
  spice.instrumentPosition(instrumentPosition);
  vscl_c(1000.0, instrumentPosition, instrumentPosition);

  // Get planet radii
  Distance dRadii[3];
  spice.radii(dRadii);

  // Calculate ographic coordinates of spacecraft position vector, in meters
  SpiceDouble equatorialRadiusMeters = dRadii[0].meters();
  SpiceDouble flattening = ( equatorialRadiusMeters - dRadii[2].meters() ) /
                           equatorialRadiusMeters;
  SpiceDouble lon = 0.0;
  SpiceDouble lat = 0.0;
  SpiceDouble height = 0.0;
  recgeo_c (instrumentPosition, equatorialRadiusMeters, flattening,
            &lon, &lat, &height);

  // Calculate rotation matrix from Socet Set plate to ocentric ground coordinates
  SpiceDouble ocentricGroundToSocetPlateRotationMatrix[3][3] = { { 0.0, 0.0, 0.0 },
                                                                 { 0.0, 0.0, 0.0 },
                                                                 { 0.0, 0.0, 0.0 } };

  mxmt_c (isisFocalPlane2SocetPlate, isisFocalPlaneToOcentricRotationMatrix,
          ocentricGroundToSocetPlateRotationMatrix);

  // Populate the ocentric-to-LSR rotation matrix; it is a function of
  // camera position only
  SpiceDouble lsrToOcentricRotationMatrix[3][3] = { { 0.0, 0.0, 0.0 },
                                                    { 0.0, 0.0, 0.0 },
                                                    { 0.0, 0.0, 0.0 } };
  SpiceDouble ocentricToLsrRotationMatrix[3][3] = { { 0.0, 0.0, 0.0 },
                                                    { 0.0, 0.0, 0.0 },
                                                    { 0.0, 0.0, 0.0 } };

  twovec_c(instrumentPosition, 3, uvPlusZ, 2, ocentricToLsrRotationMatrix);
  xpose_c(ocentricToLsrRotationMatrix, lsrToOcentricRotationMatrix);

  // Compute the Rotation matrix from LSR frame to Socet Set Plate frame,
  // and extract the euler angles to get omega-phi-kappa attidude angles
  SpiceDouble lsrGroundToSocetPlateRotationMatrix[3][3] = { { 0.0, 0.0, 0.0 },
                                                            { 0.0, 0.0, 0.0 },
                                                            { 0.0, 0.0, 0.0 } };

  mxmt_c (ocentricGroundToSocetPlateRotationMatrix, ocentricToLsrRotationMatrix,
          lsrGroundToSocetPlateRotationMatrix);

  SpiceDouble omega = 0.0;
  SpiceDouble phi = 0.0;
  SpiceDouble kappa = 0.0;
  m2eul_c (lsrGroundToSocetPlateRotationMatrix, 3, 2, 1, &kappa, &phi, &omega);

  // Return resulting geographic lat, lon, omega, phi, kappa in decimal degrees
  // height in meters
  ographicCamPos[0] = lat * RAD2DEG;
  ographicCamPos[1] = lon * RAD2DEG;
  ographicCamPos[2] = height;

  omegaPhiKappa[0] = omega * RAD2DEG;
  omegaPhiKappa[1] = phi * RAD2DEG;
  omegaPhiKappa[2] = kappa * RAD2DEG;

  if (spacecraftName == "VIKING_ORBITER_1" || spacecraftName == "VIKING_ORBITER_2" ||
      spacecraftName == "MARS_EXPRESS" || spacecraftName == "CLEMENTINE 1") {
    omegaPhiKappa[2] -= 90.0;
  }

  // Return the transpose of the isisFocalPlane2SocetPlate matrix for the FrameOffAxis sensor model
  xpose_c(isisFocalPlane2SocetPlate, isisFocalPlane2SocetPlateTranspose);

  return;
}
