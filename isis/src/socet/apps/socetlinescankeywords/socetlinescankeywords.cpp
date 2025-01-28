#include "socetlinescankeywords.h"

#include <math.h>
#include <stdlib.h>
#include <QTemporaryDir>

#include <fstream>
#include <iomanip>
#include <iostream>
#include <memory>
#include <sstream>
#include <string>

#include "Camera.h"
#include "CameraDetectorMap.h"
#include "CameraDistortionMap.h"
#include "CameraFocalPlaneMap.h"
#include "CameraGroundMap.h"
#include "Constants.h"
#include "Distance.h"
#include "FileName.h"
#include "IException.h"
#include "IString.h"
#include "LineScanCameraDetectorMap.h"
#include "NaifStatus.h"
#include "Process.h"
#include "Pvl.h"
#include "Spice.h"
#include "TProjection.h"
#include "Table.h"
#include "VariableLineScanCameraDetectorMap.h"
#include "iTime.h"

using namespace std;

namespace Isis {
    
  int GetHRSCLineRates(Cube *cube, vector<LineRateChange> &lineRates,
                      int &totalLines, double &HRSCNadirCenterTime);

  double GetHRSCScanDuration(vector<LineRateChange> &lineRates, int &totalLines);

  void socetlinescankeywords(UserInterface &ui) {
  // Get user parameters and error check
  Cube input(ui.GetCubeName("FROM"), "rw");    
  socetlinescankeywords(&input, ui);
}

void socetlinescankeywords(Cube *input, UserInterface &ui) {
  // Use a regular Process
  Process p;

  QString to = FileName(ui.GetFileName("TO")).expanded();

  double HRSCNadirCenterTime = ui.GetDouble("HRSC_NADIRCENTERTIME");

  if (input->isProjected()) {
      QString msg =
          "Input images is a map projected cube ... not a level 1 image";
    throw IException(IException::User, msg, _FILEINFO_);
  }

  // Initialize the camera
  p.SetInputCube(input);
  Pvl *cubeHeader = input->label();
  
  Camera *cam = input->camera();
  
  CameraDetectorMap *detectorMap = cam->DetectorMap();
  CameraFocalPlaneMap *focalMap = cam->FocalPlaneMap();
  CameraDistortionMap *distortionMap = cam->DistortionMap();
  CameraGroundMap *groundMap = cam->GroundMap();

  // Make sure the image contains the InstrumentPointing (aka CK) blob/table
  PvlGroup test = input->label()->findGroup("Kernels", Pvl::Traverse);
  QString InstrumentPointing = (QString) test["InstrumentPointing"];
  if (InstrumentPointing != "Table") {
    QString msg = "Input image does not contain needed SPICE blobs...run spiceinit with attach=yes.";
    throw IException(IException::User, msg, _FILEINFO_);
  }

  // Open output line scanner keyword file
  ofstream toStrm;
  toStrm.open(to.toLatin1().data(), ios::trunc);
  if (toStrm.bad()) {
    QString msg = "Unable to open output TO file";
    throw IException(IException::User, msg, _FILEINFO_);
  }

  // Get required keywords from instrument and band groups
  PvlGroup inst =input->label()->findGroup("Instrument", Pvl::Traverse);
  QString instrumentId = (QString) inst["InstrumentId"];

  bool     isMocNA = false;
//TO DO: UNCOMMENT THIS LINES ONCE MOC IS WORKING IN SS
//  bool     isMocWARed = false;
  bool     isHiRise = false;
  bool     isCTX = false;
  bool     isLroNACL = false;
  bool     isLroNACR = false;
  bool     isHRSC = false;
//TO DO: UNCOMMENT THESE LINE ONCE MOC IS WORKING IN SS
//  if (instrumentId == "MOC") {
//    PvlGroup band = cube.label()->findGroup("BandBin", Pvl::Traverse);
//    QString filter = (QString) band["FilterName"];
//
//    if (strcmp(filter.toLatin1().data(), "BROAD_BAND") == 0)
//      isMocNA = true;
//    else if (strcmp(filter.toLatin1().data(), "RED") == 0)
//      isMocWARed = true;
//    else if (strcmp(filter.toLatin1().data(), "BLUE") == 0) {
//      QString msg = "MOC WA Blue filter images not supported for Socet Set mapping";
//      throw IException(IException::User, msg, _FILEINFO_);
//    }
//  }
//  else if (instrumentId == "IdealCamera") {
//TO DO: DELETE THIS LINE ONCE MOC IS WORKING IN SS
  if (instrumentId == "IdealCamera") {
    PvlGroup orig = input->label()->findGroup("OriginalInstrument",  Pvl::Traverse);
    QString origInstrumentId = (QString) orig["InstrumentId"];
    if (origInstrumentId == "HIRISE") {
      isHiRise = true;
    }
    else {
      QString msg = "Unsupported instrument: " + origInstrumentId;
      throw IException(IException::User, msg, _FILEINFO_);
    }
  }
  else if (instrumentId == "HIRISE") {
    isHiRise = true;
  }
  else if (instrumentId == "CTX") {
    isCTX = true;
  }
  else if (instrumentId == "NACL") {
    isLroNACL = true;
  }
  else if (instrumentId == "NACR") {
    isLroNACR = true;
  }
  else if (instrumentId == "HRSC") {
    isHRSC = true;
  } 
  else {
    QString msg = "Unsupported instrument: " + instrumentId;
    throw IException(IException::User, msg, _FILEINFO_);
  }

  int ikCode = cam->naifIkCode();

  // Get Focal Length.
  // NOTE:
  //   For MOC Wide Angle, cam->focal_length returns the focal length
  //      in pixels, so we must convert from pixels to mm using the PIXEL_SIZE
  //      of 0.007 mm gotten from $ISISDATA/mgs/kernels/ik/moc20.ti.  (The
  //      PIXEL_PITCH value gotten from cam->PixelPitch is 1.0 since the
  //      focal length used by ISIS in this case is in pixels)
  //      For reference: the MOC WA blue filter pixel size needs an adjustment
  //      of 1.000452 (see p_scale in MocWideAngleDistortionMap.cpp), so that
  //      the final blue filter pixel size = (0.007 / 1.000452)
  //
  //   For all other cameras, cam->focal_length returns the focal
  //      length in mm, as needed by Socet Set

  double focal = cam->FocalLength();  // focal length returned in mm

    // TO DO: UNCOMMENT THESE LINES ONCE MOC IS WORKING IN SS
    //   if (isMocWARed)
    //     focal = focal * 0.007;  // pixel to mm conversion
    if (isHRSC) {
      switch (ikCode) {
        case -41219:  // S1: fwd stereo
          focal = 184.88;
          break;
        case -41218:  // IR: infra-red
          focal = 181.57;
          break;
        case -41217:  // P1: fwd photo
          focal = 179.16;
          break;
        case -41216:  // GREEN
          focal = 175.31;
          break;
        case -41215:  // NADIR
          focal = 175.01;
          break;
        case -41214:  // BLUE
          focal = 175.53;
          break;
        case -41213:  // P2: aft photo
          focal = 179.19;
          break;
        case -41212:  // RED
          focal = 181.77;
          break;
        case -41211:  // S2: aft stereo
          focal = 184.88;
          break;
        default:
          break;
      }
    }

  // Get instrument summing modes
  int csum = (int) detectorMap->SampleScaleFactor();
  int dsum = (int) detectorMap->LineScaleFactor();

  if (isLroNACL || isLroNACR || isHRSC)
    dsum = csum;

  // Calculate location of boresight in image space, these are zero-based values
  //
  // Note: For MOC NA, the boresight is at the image center
  //       For MOC WA, MRO HiRISE, MRO CTX, LRO_NACL, LRO_NACR and HRSC the
  //       boresight is not at the detector center, but the boresight is at the
  //       center of a NOPROJ'ED MRO HIRISE image

  // Get line/samp of boresight pixel in detector space (summing == 1)
  focalMap->SetFocalPlane(0.0, 0.0);
  double detectorBoresightSample = focalMap->DetectorSample();
  double detectorBoresightLine = focalMap->DetectorLine();

  // Convert sample of boresight pixel in detector into image space
  // (summing, etc., is accounted for.)
  detectorMap->SetDetector(detectorBoresightSample, detectorBoresightLine);
  double boresightSample = detectorMap->ParentSample();

  // Set Atmospheric correction coefficients to 0
  double atmco[4] = {0.0, 0.0, 0.0, 0.0};

  // Get totalLines, totalSamples and account for summed images
  int totalLines = input->lineCount();
  int totalSamples = input->sampleCount();

  // Get the Interval Time in seconds and calculate
  // scan duration in seconds
  double scanDuration = 0.0;
  double intTime = 0.0;

    int numIntTimes = 0.0;
    vector<LineRateChange> lineRates;
    if (isHRSC) {
      numIntTimes = GetHRSCLineRates(input, lineRates, totalLines, HRSCNadirCenterTime);
      if (numIntTimes == 1) {
        LineRateChange lrc = lineRates.at(0);
        intTime = lrc.GetLineScanRate();
      }
      if (numIntTimes <= 0) {
        QString msg = "HRSC: Invalid number of scan times";
        throw IException(IException::Programmer, msg, _FILEINFO_);
      } else
        scanDuration = GetHRSCScanDuration(lineRates, totalLines);
    } else {
      intTime = detectorMap->LineRate();  // LineRate is in seconds
  scanDuration = intTime * totalLines;
    }

    // For reference, this is the code if calculating interval time
    // via LineExposureDuration keyword off image labels:
    //
    // if (isMocNA || isMocWARed)
    //   intTime = exposureDuration * (double) dsum / 1000.0;
    // else if (isHiRise)
    //   intTime = exposureDuration * (double) dsum / 1000000.0;

  // Get along and cross scan pixel size for NA and WA sensors.
  // NOTE:
  //     1) The MOC WA pixel size is gotten from moc20.ti and is 7 microns
  //         HRSC pixel size is from the Instrument Addendum file
  //     2) For others, cam->PixelPitch() returns the pixel pitch (size) in mm.
  double alongScanPxSize = 0.0;
  double crossScanPxSize = 0.0;
    // TO DO: UNCOMMENT THESE LINES ONCE MOC IS WORKING IN SS
    //   if (isMocWARed || isHRSC) {
    if (isHRSC) {
      alongScanPxSize = csum * 0.007;
      crossScanPxSize = dsum * 0.007;
    } 
    else {
  crossScanPxSize = dsum * cam->PixelPitch();

  // Get the ephemeris time, ground position and undistorted focal plane X
  // coordinate at the center line/samp of image
  cam->SetImage(input->sampleCount() / 2.0, input->lineCount() / 2.0);

  double tMid = cam->time().Et();

  const double latCenter = cam->UniversalLatitude();
  const double lonCenter = cam->UniversalLongitude();
  const double radiusCenter = cam->LocalRadius().meters();

  double uXCenter = distortionMap->UndistortedFocalPlaneX();

  // from the ground position at the image center, increment the ephemeris
  // time by the line rate and map the ground position into the sensor in
  // undistorted focal plane coordinates

  cam->setTime(iTime(tMid + intTime));
  double uX, uY;
  groundMap->GetXY(latCenter, lonCenter, radiusCenter, &uX, &uY);

  // the along scan pixel size is the difference in focal plane X coordinates
  alongScanPxSize = abs(uXCenter - uX);
    }

  // Now that we have totalLines, totalSamples, alongScanPxSize and
  // crossScanPxSize, fill the Interior Orientation Coefficient arrays
  double ioCoefLine[10];
  double ioCoefSample[10];
  for (int i = 0; i <= 9; i++) {
    ioCoefLine[i] = 0.0;
    ioCoefSample[i] = 0.0;
  }

  ioCoefLine[0] = totalLines / 2.0;
  ioCoefLine[1] = 1.0 / alongScanPxSize;

  ioCoefSample[0] = totalSamples / 2.0;
  ioCoefSample[2] = 1.0 / crossScanPxSize;

  // Update the Rectification Terms found in the base sensor class
  double rectificationTerms[6];
  rectificationTerms[0] = totalLines / 2.0;
  rectificationTerms[1] = 0.0;
  rectificationTerms[2] = 1.0;
  rectificationTerms[3] = totalSamples / 2.0;
  rectificationTerms[4] = 1.0;
  rectificationTerms[5] = 0.0;

  // Fill the triangulation parameters array
  double triParams[18];
  for (int i = 0; i <= 17; i++)
    triParams[i] = 0.0;

  triParams[15] = focal;

  // Set the Center Ground Point at the SOCET Set image, in radians
  double centerGp[3];
  double radii[3] = {0.0, 0.0, 0.0};
  Distance Dradii[3];

  cam->radii(Dradii);
  radii[0] = Dradii[0].kilometers();
  radii[1] = Dradii[1].kilometers();
  radii[2] = Dradii[2].kilometers();

  cam->SetImage(boresightSample, totalLines / 2.0);

  centerGp[0] = DEG2RAD *
                  TProjection::ToPlanetographic(cam->UniversalLatitude(), radii[0], radii[2]);
  centerGp[1] = DEG2RAD * TProjection::To180Domain(cam->UniversalLongitude());
  centerGp[2] = 0.0;
  //**** NOTE: in the import_pushbroom SOCET SET program, centerGp[2] will be set to the SS
  //**** project's gp_origin_z

  // Now get keyword values that depend on ephemeris data.

  // First get the ephemeris time and camera Lat Lon at image center line, boresight sample.
  double centerLine = double(totalLines) / 2.0;

  cam->SetImage(boresightSample, centerLine); //set to boresight of image
  double etCenter = cam->time().Et();

  // Get the sensor position at the image center in ographic lat,
  // +E lon domain 180 coordinates, radians, height in meters
  double sensorPosition[3] = {0.0, 0.0, 0.0};
  double ocentricLat, e360Lon;
  cam->subSpacecraftPoint(ocentricLat, e360Lon);
  sensorPosition[0] = DEG2RAD * TProjection::ToPlanetographic(ocentricLat, radii[0], radii[2]);
  sensorPosition[1] = DEG2RAD * TProjection::To180Domain(e360Lon);
  sensorPosition[2] = cam->SpacecraftAltitude() * 1000.0;

  // Build the ephem data.  If the image label contains the InstrumentPosition
  // table, use it as a guide for number and spacing of Ephem points.
  // Otherwise (i.e, for dejittered HiRISE images), the number and spacing of
  // ephem points based on hardcoded dtEphem value

  // Using the InstrumentPosition table as a guide build the ephem data
  QList< QList<double> > ephemPts;
  QList< QList<double> > ephemRates;

  PvlGroup kernels = input->label()->findGroup("Kernels", Pvl::Traverse);
  QString InstrumentPosition = (QString) kernels["InstrumentPosition"];

  int numEphem = 0;      // number of ephemeris points
  double dtEphem = 0.0;  // delta time of ephemeris points, seconds
  if (InstrumentPosition == "Table") {
    // Labels contain SPK blob
    // set up Ephem pts/rates number and spacing
    Table tablePosition("InstrumentPosition", cubeHeader->fileName());
    numEphem = tablePosition.Records();

    // increase the number of ephem nodes by 20%.  This is somewhat random but
    // generally intended to compensate for having equally time spaced nodes
    // instead of of the potentially more efficient placement used by spiceinit
    numEphem = int(double(numEphem) * 1.2);

    // if numEphem calcutated from SPICE blobs is too sparse for SOCET Set,
    // mulitiply it by a factor of 30
    // (30X was settled upon emperically.  In the future, make this an input parameter)
    if (numEphem <= 10) numEphem = tablePosition.Records() * 30;

    // make the number of nodes odd
    numEphem  = (numEphem % 2) == 1 ? numEphem : numEphem + 1;

    // SOCET has a max number of ephem pts of 10000, and we're going to add twenty...
    if (numEphem > 10000 - 20) numEphem = 9979;

    dtEphem = scanDuration / double(numEphem);

    //build the tables of values
    double et = etCenter - (((numEphem - 1) / 2) * dtEphem);
    for (int i = 0; i < numEphem; i++) {
      cam->setTime(iTime(et));
      SpiceRotation *bodyRot = cam->bodyRotation();
      vector<double> pos = bodyRot->ReferenceVector(cam->instrumentPosition()->Coordinate());
//TO DO: UNCOMMENT THE FOLLOWING LINE WHEN VELOCITY BLOBS ARE CORRECT IN ISIS
      //vector<double> vel = bodyRot->ReferenceVector(cam->instrumentPosition()->Velocity());

      //Add the ephemeris position and velocity to their respective lists, in meters and meters/sec
      QList<double> ephemPt;
      QList<double> ephemRate;
      ephemPts.append(ephemPt << pos[0] * 1000 << pos[1] * 1000 << pos[2] * 1000);
//TO DO: UNCOMMENT THE FOLLOWING LINE WHEN VELOCITY BLOBS ARE CORRECT IN ISIS
      //ephemRates.append(ephemRate << vel[0] * 1000 << vel[1] * 1000 << vel[2] * 1000);

      et += dtEphem;
    }

//TO DO: WHEN VELOCITY BLOBS ARE CORRECT IN ISIS, linearlly interpolate 10 nodes rather than 11
//       (need 11 now for computation of velocity at first and last ephemeris point)
    // linearlly interpolate 11 additional nodes before line 1 (SOCET requires this)
    for (int i = 0; i < 11; i++) {
      double vec[3] = {0.0, 0.0, 0.0};
      vec[0] = ephemPts[0][0] + (ephemPts[0][0] - ephemPts[1][0]);
      vec[1] = ephemPts[0][1] + (ephemPts[0][1] - ephemPts[1][1]);
      vec[2] = ephemPts[0][2] + (ephemPts[0][2] - ephemPts[1][2]);
      QList<double> ephemPt;
      ephemPts.prepend (ephemPt << vec[0] << vec[1] << vec[2]);

//TO DO: UNCOMMENT THE FOLLOWING LINES WHEN VELOCITY BLOBS ARE CORRECT IN ISIS
      //vec[0] = ephemRates[0][0] + (ephemRates[0][0] - ephemRates[1][0]);
      //vec[1] = ephemRates[0][1] + (ephemRates[0][1] - ephemRates[1][1]);
      //vec[2] = ephemRates[0][2] + (ephemRates[0][2] - ephemRates[1][2]);
      //QList<double> ephemRate;
      //ephemRates.prepend (ephemRate << vec[0] << vec[1] << vec[2]);
    }

//TO DO: WHEN VELOCITY BLOBS ARE CORRECT IN ISIS, linearlly interpolate 10 nodes rather than 11
//       (need 11 now for computation of velocity at first and last ephemeris point)
    // linearlly interpolate 11 additional nodes after the last line (SOCET requires this)
    for (int i = 0; i < 11; i++) {
      double vec[3] = {0.0, 0.0, 0.0};
      int index = ephemPts.size() - 1;
      vec[0] = ephemPts[index][0] + (ephemPts[index][0] - ephemPts[index - 1][0]);
      vec[1] = ephemPts[index][1] + (ephemPts[index][1] - ephemPts[index - 1][1]);
      vec[2] = ephemPts[index][2] + (ephemPts[index][2] - ephemPts[index - 1][2]);
      QList<double> ephemPt;
      ephemPts.append(ephemPt << vec[0] << vec[1] << vec[2]);

//TO DO: UNCOMMENT THE FOLLOWING LINES WHEN VELOCITY BLOBS ARE CORRECT IN ISIS
      //vec[0] = ephemRates[index][0] + (ephemRates[index][0] - ephemRates[index - 1][0]);
      //vec[1] = ephemRates[index][1] + (ephemRates[index][1] - ephemRates[index - 1][1]);
      //vec[2] = ephemRates[index][2] + (ephemRates[index][2] - ephemRates[index - 1][2]);
      //QList<double> ephemRate;
      //ephemRates.append(ephemRate << vec[0] << vec[1] << vec[2]);
    }

    numEphem += 20;

//TO DO: DELETE THE FOLLOWING LINES WHEN VELOCITY BLOBS ARE CORRECT IN ISIS
    // Compute the spacecraft velocity at each ephemeris point
    double deltaTime = 2.0 * dtEphem;
    for (int i = 0; i < numEphem; i++) {
      double vec[3] = {0.0, 0.0, 0.0};
      vec[0] = (ephemPts[i+2][0] - ephemPts[i][0]) / deltaTime;
      vec[1] = (ephemPts[i+2][1] - ephemPts[i][1]) / deltaTime;
      vec[2] = (ephemPts[i+2][2] - ephemPts[i][2]) / deltaTime;
      QList<double> ephemRate;
      ephemRates.append(ephemRate << vec[0] << vec[1] << vec[2]);
    }

  }
  else {
    // Calculate the number of ephemeris points that are needed, based on the
    // value of dtEphem (Delta-Time-Ephemeris).  SOCET SET needs the ephemeris
    // points to exceed the image range for interpolation.  For now, attempt a
    // padding of 10 ephemeris points on either side of the image.

    if (isMocNA || isHiRise || isCTX || isLroNACL || isLroNACR || isHRSC)
      // Try increment of every 300 image lines
      dtEphem = 300 * intTime;  // Make this a user definable increment?
    else // Set increment for WA images to one second
      dtEphem = 1.0;

    // Pad by 10 ephem pts on each side of the image
    numEphem = (int)(scanDuration / dtEphem) + 20;

    // if numEphem is even, make it odd so that the number of ephemeris points
    // is equal on either side of T_CENTER
    if ((numEphem % 2) == 0)
      numEphem++;

//TO DO: DELETE THE FOLLOWING LINE WHEN VELOCITY BLOBS ARE CORRECT IN ISIS
    numEphem = numEphem + 2; // Add two for calcuation of velocity vectors...

    // Find the ephemeris time for the first ephemeris point, and from that, get
    // to_ephem needed by SOCET (to_ephem is relative to etCenter)
    double et = etCenter - (((numEphem - 1) / 2) * dtEphem);
    for (int i = 0; i < numEphem; i++) {
      cam->setTime(iTime(et));
      SpiceRotation *bodyRot = cam->bodyRotation();
      vector<double> pos = bodyRot->ReferenceVector(cam->instrumentPosition()->Coordinate());
//TO DO: UNCOMMENT THE FOLLOWING LINE WHEN VELOCITY BLOBS ARE CORRECT IN ISIS
      //vector<double> vel = bodyRot->ReferenceVector(cam->instrumentPosition()->Velocity());

      //Add the ephemeris position and velocity to their respective lists, in meters and meters/sec
      QList<double> ephemPt;
      QList<double> ephemRate;
      ephemPts.append(ephemPt << pos[0] * 1000 << pos[1] * 1000 << pos[2] * 1000);
//TO DO: UNCOMMENT THE FOLLOWING LINE WHEN VELOCITY BLOBS ARE CORRECT IN ISIS
      //ephemRates.append(ephemRate << vel[0] * 1000 << vel[1] * 1000 << vel[2] * 1000);

      et += dtEphem;
    }
//TO DO: DELETE THE FOLLOWING LINES WHEN VELOCITY BLOBS ARE CORRECT IN ISIS
    // Compute the spacecraft velocity at each ephemeris point
    // (We must do this when blobs are not attached because the Spice Class
    // stores in memory the same data that would be in a blob...even when reading NAIF kernels)
    double deltaTime = 2.0 * dtEphem;
    numEphem = numEphem - 2; // set numEphem back to the number we need output
    for (int i = 0; i < numEphem; i++) {
      double vec[3] = {0.0, 0.0, 0.0};
      vec[0] = (ephemPts[i+2][0] - ephemPts[i][0]) / deltaTime;
      vec[1] = (ephemPts[i+2][1] - ephemPts[i][1]) / deltaTime;
      vec[2] = (ephemPts[i+2][2] - ephemPts[i][2]) / deltaTime;
      QList<double> ephemRate;
      ephemRates.append(ephemRate << vec[0] << vec[1] << vec[2]);
    }
  }

  //update ephem stats
  double etFirstEphem = etCenter - (((numEphem - 1) / 2) * dtEphem);
  double t0Ephem = etFirstEphem - etCenter;

  // Using the intrumentPointing table as a guide build the quarternions
  // for simplicity sake we'll leave the mountingAngles as identity
  // and store the complete rotation from body fixed to camera in the
  // quarternions

  //set up quaternions number and spacing
  Table tablePointing("InstrumentPointing", cubeHeader->fileName());

  //number of quaternions
  int numQuaternions = tablePointing.Records();

  // increase the number of quaternions nodes by 20%. This is somewhat random but
  // generally intended to compensate for having equally time spaced nodes
  // instead of of the potentially more efficient placement used by spiceinit
  numQuaternions = (int)(numQuaternions * 1.2);

  // if numQuaternions calcutated from SPICE blobs is too sparse for SOCET Set,
  // mulitiply it by a factor of 30
  // (30X was settled upon emperically.  In the future, make this an input parameter)
  if (numQuaternions <= 10) numQuaternions = tablePointing.Records() * 30;

  //make the number of nodes odd
  numQuaternions = (numQuaternions % 2) == 1 ? numQuaternions : numQuaternions + 1;

  // SOCET has a max number of quaternions of 20000, and we're going to add twenty...
  if (numQuaternions > 20000 - 20) numQuaternions = 19179;

  double dtQuat = scanDuration / double(numQuaternions);

  // build the tables of values
  QList< QList<double> > quaternions;
  double et = etCenter - (((numQuaternions - 1) / 2) * dtQuat);

  for (int i = 0; i < numQuaternions; i++) {
    cam->setTime(iTime(et));
    vector<double> j2000ToBodyFixedMatrixVector = cam->bodyRotation()->Matrix();
    vector<double> j2000ToCameraMatrixVector = cam->instrumentRotation()->Matrix();
    double quaternion[4] = {0.0, 0.0, 0.0, 0.0};

    double j2000ToBodyFixedRotationMatrix[3][3], //rotation from J2000 to target (aka body, planet)
           j2000ToCameraRotationMatrix[3][3], //rotation from J2000 to spacecraft
           cameraToBodyFixedRotationMatrix[3][3]; //rotation from camera to target

    // reformat vectors to 3x3 rotation matricies
    for (int j = 0; j < 3; j++) {
      for (int k = 0; k < 3; k++) {
        j2000ToBodyFixedRotationMatrix[j][k] = j2000ToBodyFixedMatrixVector[3 * j + k];
        j2000ToCameraRotationMatrix[j][k] = j2000ToCameraMatrixVector[3 * j + k];
      }
    }

    // get the quaternion
    mxmt_c(j2000ToBodyFixedRotationMatrix, j2000ToCameraRotationMatrix,
           cameraToBodyFixedRotationMatrix);
    m2q_c(cameraToBodyFixedRotationMatrix, quaternion);

    // add the quaternion to the list of quaternions
    QList<double> quat;
    quaternions.append(quat << quaternion[1] << quaternion[2] << quaternion[3] <<
                                quaternion[0]);
    //note also that the order is changed to match socet

    et += dtQuat;
  }

  // linearlly interpolate 10 additional nodes before the first quaternion (SOCET requires this)
  for (int i = 0; i < 10; i++) {
    double vec[4] = {0.0, 0.0, 0.0, 0.0};
    vec[0] = quaternions[0][0] + (quaternions[0][0] - quaternions[1][0]);
    vec[1] = quaternions[0][1] + (quaternions[0][1] - quaternions[1][1]);
    vec[2] = quaternions[0][2] + (quaternions[0][2] - quaternions[1][2]);
    vec[3] = quaternions[0][3] + (quaternions[0][3] - quaternions[1][3]);
    QList<double> quat;
    quaternions.prepend (quat << vec[0] << vec[1] << vec[2] << vec[3]);
  }

  // linearlly interpolate 10 additional nodes after the last quaternion (SOCET requires this)
  for (int i = 0; i < 10; i++) {
    double vec[4] = {0.0, 0.0, 0.0, 0.0};
    int index = quaternions.size() - 1;
    vec[0] = quaternions[index][0] + (quaternions[index][0] - quaternions[index - 1][0]);
    vec[1] = quaternions[index][1] + (quaternions[index][1] - quaternions[index - 1][1]);
    vec[2] = quaternions[index][2] + (quaternions[index][2] - quaternions[index - 1][2]);
    vec[3] = quaternions[index][3] + (quaternions[index][3] - quaternions[index - 1][3]);
    QList<double> quat;
    quaternions.append(quat << vec[0] << vec[1] << vec[2] << vec[3]);
  }

  //update quaternions stats
  numQuaternions += 20;

  //ephemeris time of the first quarternion
  double et0Quat = etCenter - (((numQuaternions - 1) / 2) * dtQuat);

  //quadrtic time of the first quarternion
  double qt0Quat = et0Quat - etCenter;

  //query remaing transformation parameters from Camera Classes
  //transformation to distortionless focal plane
  double zDirection = distortionMap->ZDirection();

  //transformation from DistortionlessFocalPlane to FocalPlane
  vector<double> opticalDistCoefs = distortionMap->OpticalDistortionCoefficients();

  // For instruments with less than 3 distortion coefficients, set the
  // unused ones to 0.0
  opticalDistCoefs.resize(3, 0);

  //transformation from focal plane to detector
  const double *iTransS = focalMap->TransS();
  const double *iTransL = focalMap->TransL();
  double detectorSampleOrigin = focalMap->DetectorSampleOrigin();
  double detectorLineOrigin = focalMap->DetectorLineOrigin();

  //transformation from dectector to cube
  double startingSample = detectorMap->AdjustedStartingSample();
  double startingLine = detectorMap->AdjustedStartingLine();
  double sampleSumming = detectorMap->SampleScaleFactor();
  double etStart = ((LineScanCameraDetectorMap *)detectorMap)->StartTime();
  double lineOffset = focalMap->DetectorLineOffset();

  // We are done with computing keyword values, so output the Line Scanner
  // Keyword file.

  // This is the SOCET SET base sensor class keywords portion of support file:
  toStrm.setf(ios::scientific);
  toStrm << "RECTIFICATION_TERMS" << endl;
  toStrm << "        " << setprecision(14) << rectificationTerms[0] << " " <<
              rectificationTerms[1] << " " << rectificationTerms[2] << endl;
  toStrm << "        " << rectificationTerms[3] << " " << rectificationTerms[4] <<
              " " << rectificationTerms[5] << endl;

  toStrm << "GROUND_ZERO ";
  toStrm << centerGp[0] << " " << centerGp[1] << " " << centerGp[2] << endl;

  toStrm << "LOAD_PT ";
  toStrm << centerGp[0] << " " << centerGp[1] << " " << centerGp[2] << endl;

  toStrm << "COORD_SYSTEM 1" << endl;

  toStrm << "IMAGE_MOTION 0" << endl;

  // This is the line scanner sensor model portion of support file:
  toStrm << "SENSOR_TYPE USGSAstroLineScanner" << endl;
  toStrm << "SENSOR_MODE UNKNOWN" << endl;

  Distance targetRadii[3];
  if (input->label()->hasGroup("Mapping")) {
    PvlGroup &mappingGroup = input->label()->findGroup("Mapping");
    targetRadii[0].setMeters(toDouble(mappingGroup["EquatorialRadius"][0]));
    targetRadii[2].setMeters(toDouble(mappingGroup["PolarRadius"][0]));
  }
  else {
    try {
      cam->radii(targetRadii);
    }
    catch (IException &e) {
      QString msg = "Failed to get target body radii from cube.";
      throw IException(e, IException::Programmer, msg, _FILEINFO_);
    }
  }
  double eccentricity = 1.0 - (targetRadii[2].meters() * targetRadii[2].meters())
                            / (targetRadii[0].meters() * targetRadii[0].meters());
  eccentricity = sqrt(eccentricity);
  toStrm << "SEMI_MAJOR_AXIS  " << targetRadii[0].meters() << "\n";
  toStrm << "ECCENTRICITY     " << eccentricity << "\n";

  toStrm << "FOCAL " << focal << endl;

  toStrm << "ATMCO";
  for (int i = 0; i < 4; i++) toStrm << " " << atmco[i];
  toStrm << endl;

  toStrm << "IOCOEF_LINE";
  for (int i = 0; i < 10; i++) toStrm << " " << ioCoefLine[i];
  toStrm << endl;

  toStrm << "IOCOEF_SAMPLE";
  for (int i = 0; i < 10; i++) toStrm << " " << ioCoefSample[i];
  toStrm << endl;

  toStrm << "ABERR    0" << endl;
  toStrm << "ATMREF   0" << endl;
  toStrm << "PLATFORM   1" << endl;
  toStrm << "SOURCE_FLAG  1" << endl;
  toStrm << "SINGLE_EPHEMERIDE  0" << endl;

  //Note, for TRI_PARAMETERS, we print the first element separate from the rest so that the array
  //starts in the first column.  Otherwise, SOCET Set will treat the array as a comment
  toStrm << "TRI_PARAMETERS" << endl;
  toStrm << triParams[0];
  for (int i = 1; i < 18; i++) toStrm << " " << triParams[i];
  toStrm << endl;

  toStrm << setprecision(25) << "T_CENTER  ";
  double tCenter = 0.0;

    if (isHRSC) {
      tCenter = etCenter - HRSCNadirCenterTime;
      toStrm << tCenter << endl;
    } 
    else {
    toStrm << tCenter << endl;
    }
  toStrm << "DT_EPHEM  " << dtEphem << endl;

  toStrm << "T0_EPHEM  ";

    if (isHRSC) {
      double t = tCenter + t0Ephem;
      toStrm << t << endl;
    } 
    else {
    toStrm << t0Ephem << endl;
    }
  toStrm << "NUMBER_OF_EPHEM   " << numEphem << endl;

  toStrm << "EPHEM_PTS" << endl;
//TO DO: DELETE THE FOLLOWING LINE WHEN VELOCITY BLOBS ARE CORRECT IN ISIS
  for (int i = 1; i <= numEphem; i++) {
//TO DO: UNCOMMENT THE FOLLOWING LINE WHEN VELOCITY BLOBS ARE CORRECT IN ISIS
  //for (int i = 0; i < numEphem; i++) {
    toStrm << " " << ephemPts[i][0];
    toStrm << " " << ephemPts[i][1];
    toStrm << " " << ephemPts[i][2] << endl;
  }

  toStrm  << "\n\nEPHEM_RATES" << endl;
  for (int i = 0; i < numEphem; i++) {
    toStrm << " " << ephemRates[i][0];
    toStrm << " " << ephemRates[i][1];
    toStrm << " " << ephemRates[i][2] << endl;
  }

  toStrm << "\n\nDT_QUAT " << dtQuat << endl;
  toStrm << "T0_QUAT " << qt0Quat << endl;
  toStrm << "NUMBER_OF_QUATERNIONS  " << numQuaternions << endl;
  toStrm << "QUATERNIONS" << endl;
  for (int i = 0; i < numQuaternions; i++) {
    toStrm << " " << quaternions[i][0];
    toStrm << " " << quaternions[i][1];
    toStrm << " " << quaternions[i][2];
    toStrm << " " << quaternions[i][3] << endl;
  }

  toStrm << "\n\nSCAN_DURATION " << scanDuration << endl;

  if (isHRSC) {
    toStrm << "INT_TIMES" << endl;
    for (int i = 0; i < numIntTimes; i++) {
      LineRateChange lr = lineRates.at(i);
      toStrm << " " << lr.GetStartEt();
      toStrm << " " << lr.GetLineScanRate();
      toStrm << " " << lr.GetStartLine() << endl;
    }
  } 
  else {
  toStrm << "INT_TIME " << intTime << endl;
  }

  toStrm << "\nALONG_SCAN_PIXEL_SIZE  " << alongScanPxSize << endl;
  toStrm << "CROSS_SCAN_PIXEL_SIZE  " << crossScanPxSize << endl;

  toStrm << "\nCENTER_GP";
  for (int i = 0; i < 3; i++) toStrm << " " << centerGp[i];
  toStrm << endl;

  toStrm << "SENSOR_POSITION";
  for (int i = 0; i < 3; i++) toStrm << " " << sensorPosition[i];
  toStrm << endl;

  toStrm << "MOUNTING_ANGLES";
  double mountingAngles[3] = {0.0, 0.0, 0.0};
  for (int i = 0; i < 3; i++) toStrm << " " << mountingAngles[i];
  toStrm << endl;

  toStrm << "\nTOTAL_LINES " << totalLines << endl;
  toStrm << "TOTAL_SAMPLES " << totalSamples << endl;
  toStrm << "\n\n\n" << endl;

  toStrm << "IKCODE  " << ikCode << endl;
  toStrm << "ISIS_Z_DIRECTION  " << zDirection << endl;

  toStrm << "OPTICAL_DIST_COEF";
  for (int i = 0; i < 3; i++) toStrm << " " << opticalDistCoefs[i];
  toStrm << endl;

  toStrm << "ITRANSS";
  for (int i = 0; i < 3; i++) toStrm << " " << iTransS[i];
  toStrm << endl;

  toStrm << "ITRANSL";
  for (int i = 0; i < 3; i++) toStrm << " " << iTransL[i];
  toStrm << endl;

  toStrm << "DETECTOR_SAMPLE_ORIGIN " << detectorSampleOrigin << endl;
  toStrm << "DETECTOR_LINE_ORIGIN " << detectorLineOrigin << endl;
  toStrm << "DETECTOR_LINE_OFFSET  " << lineOffset << endl;
  toStrm << "DETECTOR_SAMPLE_SUMMING  " << sampleSumming << endl;

  toStrm << "STARTING_SAMPLE " << startingSample << endl;
  toStrm << "STARTING_LINE " << startingLine << endl;
  toStrm << "STARTING_EPHEMERIS_TIME " << setprecision(25) << etStart << endl;
  toStrm << "CENTER_EPHEMERIS_TIME " << etCenter << endl;

} // end main

  int GetHRSCLineRates(Cube *cube, vector<LineRateChange> &lineRates,
                      int &dtotalLines, double &HRSCNadirCenterTime) {
    FileName cubefname = cube->fileName();
    QTemporaryDir prefix;
    FileName tablefname = FileName(prefix.path() + "/tabledump.txt");

    // system call to ISIS function tabledump to dump LineScanTimes
    char syscmd[1056];
    snprintf(syscmd, sizeof(syscmd), "tabledump from=%s to=%s name=LineScanTimes",
         cubefname.expanded().toLatin1().data(), tablefname.expanded().toLatin1().data());


    int n = system(syscmd);
    if (n != 0) return -1;

    // read table back in. This whole mess is necessary since
    // HrscCamera::ReadLineRates(IString filename) is private

    // open tabledump.txt for reading
    ifstream fpIn(tablefname.expanded().toLatin1().data(), ifstream::in);
    if (!fpIn) return -1;

    Camera *cam = cube->camera();

    char buf[512];

    // read and discard header line
    if (!fpIn.getline(buf, 512)) return -1;

    double ephemerisTime, exposureTime;
    int lineStart;

    // read and store all line rate data
    while (fpIn.getline(buf, 512)) {
      sscanf(buf, "%lf,%lf,%d", &ephemerisTime, &exposureTime, &lineStart);
      lineRates.push_back(LineRateChange(lineStart, ephemerisTime, exposureTime));
    }

    // close file pointer
    fpIn.close();

    // if cube has not been cropped, we're done (if AlphaCube group is
    // present, cube has been cropped)
    bool bIsCropped = cube->hasGroup("AlphaCube");

    if (!bIsCropped) return lineRates.size();

    // if cube is cropped, we need to trim the lineRates and map the line
    // numbers to match the cropped image. This is because the lineRates table
    // stores data for the original (alpha) image

    // get alpha cube start line
    PvlGroup alphacube = cube->group("AlphaCube");
    QString str = (QString)alphacube["AlphaStartingLine"];
    double dAlphaStartLine = atof(str.toLatin1().data());
    double dAlphaLastLine = dAlphaStartLine + dtotalLines;
    cam->SetImage(1, 0.5);
    double dStartETime = cam->time().Et();
    double dFirstLineTime = dStartETime - HRSCNadirCenterTime;

    if (lineRates.size() == 1) {
      LineRateChange lrc = lineRates.at(0);
      double lsr = lrc.GetLineScanRate();
      lineRates.erase(lineRates.begin());
      lineRates.push_back(LineRateChange(1, dFirstLineTime, lsr));
      return lineRates.size();
    }

    LineRateChange firstlrc(0, 0, 0);
    for (int i = 0; i < int(lineRates.size()); i++) {
      LineRateChange lrc = lineRates.at(i);

      int nRateStartLine = lrc.GetStartLine();

      // Line rate is within cropped boundaries, keep it and set the line
      // number to match the cropped image
      if (nRateStartLine >= dAlphaStartLine && nRateStartLine <= dAlphaLastLine) {
        double et = lrc.GetStartEt();
        double lsr = lrc.GetLineScanRate();
        int rsl = nRateStartLine - dAlphaStartLine + 1;
        lineRates.erase(lineRates.begin() + i);

        lineRates.insert(lineRates.begin() + i,
                        LineRateChange(rsl, et - HRSCNadirCenterTime, lsr));
        continue;
      }

      if (nRateStartLine < dAlphaStartLine) firstlrc = lrc;

      lineRates.erase(lineRates.begin() + i);
      i--;
    }

    // insert first line
    double secondtime = lineRates.at(0).GetStartEt();
    int secondstartline = lineRates.at(0).GetStartLine();
    double lsr = firstlrc.GetLineScanRate();
    double et = secondtime - (secondstartline - 1) * lsr;
    lineRates.insert(lineRates.begin(), LineRateChange(1, et, lsr));

    return lineRates.size();

  }  // end GetHRSCLineRates

  double GetHRSCScanDuration(vector<LineRateChange> &lineRates,
                            int &ntotalLines) {
    int nLineRates = lineRates.size();
    if (nLineRates == 1) return lineRates.at(0).GetLineScanRate() * ntotalLines;

    int nLines;
    double scanTime = 0.0;
    int i = 0;
    int nLastLine = lineRates.at(0).GetStartLine() + ntotalLines - 1;
    do {
      LineRateChange lrc1 = lineRates.at(i);
      LineRateChange lrc2 = lineRates.at(i + 1);

      nLines = lrc2.GetStartLine() - lrc1.GetStartLine();
      scanTime += lrc1.GetLineScanRate() * (double)nLines;

      i++;

    } while (i < (nLineRates - 1));

    // add in last interval
    LineRateChange lrc = lineRates.at(nLineRates - 1);
    nLines = nLastLine - lrc.GetStartLine() + 1;

    scanTime += lrc.GetLineScanRate() * (double)nLines;

    return scanTime;

  }  // end GetHRSCScanDuration
}  // namespace Isis
