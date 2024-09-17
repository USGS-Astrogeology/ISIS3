/** This is free and unencumbered software released into the public domain.

The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */

#include "MdisGeometry.h"

#include <cmath>
#include <iomanip>
#include <iostream>
#include <numeric>
#include <sstream>
#include <string>
#include <vector>

#include <SpiceUsr.h>

#include "CameraFactory.h"
#include "IException.h"
#include "IString.h"
#include "NaifStatus.h"
#include "OriginalLabel.h"
#include "Pvl.h"
#include "SpecialPixel.h"
#include "SpiceManager.h"
#include "Target.h"
#include "TProjection.h"

using namespace std;

namespace Isis {

  /**
   * @brief Constructor using an ISIS cube file name
   * @param filename Name of ISIS cube file
   */
  MdisGeometry::MdisGeometry(const QString &filename) {
    Cube cube;
    cube.open(filename);
    init(cube);
  }

  /**
   * @brief Construct using an ISIS Cube class
   * @param cube ISIS cube class
   */
  MdisGeometry::MdisGeometry(Cube &cube) {
    init(cube);
  }

  /**
   * @brief Initialize class with an ISIS file
   *
   * This method is reentrant in that it can be used repeatedly in computing
   * MDIS geometry.  If a file has already been processed, it will be cleared to
   * make way for the specified file.
   *
   * @param filename Name of ISIS cube file
   */
  void MdisGeometry::setCube(const QString &filename) {
    Cube cube;
    cube.open(filename);
    delete _camera;
    init(cube);
    return;
  }


  /**
   * @brief Checks for the TargetName keyword for validity
   *
   * This static method will check the value of the Targetname keyword for a
   * valid NAIF target code.  If it is not valid, ISIS cannot spiceinit the
   * image and we will not be able to get any geometric data.
   *
   * The value of the TargetName keyword is extracted and is checked for a NAIF
   * body code, indicating it is a valid target.  This result is returned to the
   * caller.
   *
   * If the target is not a recognized NAIF code and the makeValid parameter is
   * true, the TargetName keyword values is changed to "Sky".  This will allow
   * for the basic values to be computed.
   *
   * @param label Label to validate target
   * @param makeValid True if the caller wants a valid Sky target if NAIF knows
   *                  nothing about it
   *
   * @return bool True if the target is a recognized target, false if not.
   */
  bool MdisGeometry::validateTarget(Pvl &label, bool makeValid) {
    // Add the planetary constants kernel
    SpiceManager naif;
    naif.add("$base/kernels/pck/pck?????.tpc");

    //  Get the target and check for validity
    PvlKeyword &target = label.findKeyword("TargetName", PvlObject::Traverse);

    try {
      Target::lookupNaifBodyCode(QString::fromStdString(target));
      return (true);
    }
    catch (...) {
      if (makeValid) {
        target.setValue("Sky");
      }
      return (false);
    }

  }

  /**
   * @brief Returns the center line and sample coordinate of the image
   *
   * This method returns the reference line and sample coordinate at the center
   * of the image.
   *
   * @param sample Sample coordinate at the center
   * @param line   Line coordinate at the center
   */
  void MdisGeometry::refCenterCoord(double &sample, double &line) const {
    // Ensure there is a camera model instantiated!
    if (!_camera) {
      string mess = "No image (camera model) established for reference pixel!";
      throw IException(IException::Programmer, mess.c_str(),
                       _FILEINFO_);
    }

    //  Compute point at center
    sample = _camera->Samples() / 2.0;
    line   = _camera->Lines() / 2.0;
    return;
  }

  /**
   * @brief Returns the upper left line and sample coordinate of the image
   *
   * This method returns the reference line and sample coordinate at the upper
   * left corner of the image.
   *
   * @param sample Sample coordinate at the upper left
   * @param line   Line coordinate at the upper left
   */
  void MdisGeometry::refUpperLeftCoord(double &sample, double &line) const {
    //  Upper left point is a constant
    sample = 1.0;
    line   = 1.0;
    return;
  }

  /**
   * @brief Returns the upper right line and sample coordinate of the image
   *
   * This method returns the reference line and sample coordinate at the upper
   * right corner of the image.
   *
   * @param sample Sample coordinate at the upper right
   * @param line   Line coordinate at the upper right
   */
  void MdisGeometry::refUpperRightCoord(double &sample, double &line) const {
    // Ensure there is a camera model instantiated!
    if (!_camera) {
      string mess = "No image (camera model) established for reference pixel!";
      throw IException(IException::Programmer, mess.c_str(),
                       _FILEINFO_);
    }

    //  Upper right corner point
    sample = _camera->Samples();
    line   = 1.0;
    return;
  }

  /**
   * @brief Returns the lower left line and sample coordinate of the image
   *
   * This method returns the reference line and sample coordinate at the lower
   * left corner of the image.
   *
   * @param sample Sample coordinate at the lower left
   * @param line   Line coordinate at the lower left
   */
  void MdisGeometry::refLowerLeftCoord(double &sample, double &line) const {
    // Ensure there is a camera model instantiated!
    if (!_camera) {
      string mess = "No image (camera model) established for reference pixel!";
      throw IException(IException::Programmer, mess.c_str(),
                       _FILEINFO_);
    }

    //  Lower left corner point
    sample = 1.0;
    line   = _camera->Lines();
    return;
  }

  /**
   * @brief Returns the lower right line and sample coordinate of the image
   *
   * This method returns the reference line and sample coordinate at the lower
   * right corner of the image.
   *
   * @param sample Sample coordinate at the lower right
   * @param line   Line coordinate at the lower right
   */
  void MdisGeometry::refLowerRightCoord(double &sample, double &line) const {
    // Ensure there is a camera model instantiated!
    if (!_camera) {
      string mess = "No image (camera model) established for reference pixel!";
      throw IException(IException::Programmer, mess.c_str(),
                       _FILEINFO_);
    }

    //  Lower right corner point
    sample = _camera->Samples();
    line   = _camera->Lines();
    return;
  }

  /**
   * @brief Compute and retrieve geometric parameters for given file
   *
   * This routine invokes a series of methods that compute geometric parameters
   * for MESSENGER MDIS camera observations.  These parameters are expressly for
   * population of PDS keyword values in the original EDRs.
   *
   * A camera model is required when computing these values and is provided
   * through several constructor options and a method.
   *
   * The filename argument is only for recording in the FILENAME keyword as the
   * source of the actual ISIS file is likely from a PDS EDR file.  This
   * satisifies a need to associate the data with the original EDR. The camera
   * model is invoked from the ISIS verision of the cube.
   *
   * @param filename File to record as orginal source of data
   *
   * @return Pvl Contains PvlKeywords of all computed parameters
   */
  Pvl MdisGeometry::getGeometry(const QString &filename) {
    Pvl geom;

    // Set initial keywords
    geom += PvlKeyword("FILENAME", filename.toStdString());
    geom += format("SOURCE_PRODUCT_ID", _spice.getList(true));

    //  Invoke routines to compute associated keys
    GeometryKeys(geom);
    TargetKeys(geom);
    SpacecraftKeys(geom);
    ViewingAndLightingKeys(geom);
    return (geom);
  }

  /**
   * @brief Initialize the class parameter with the ISIS cube source
   *
   * This method is the main initialization routine.  If is mostly reentrant but
   * the caller must, at a minimum, decide to free any existing camera model.
   *
   * It is intended to be used in the various constructors, but can also be used
   * to intialize a new cube as needed in this object.  It does reset everything
   * to defaults.
   *
   * It is assumed that the incoming ISIS cube has been initialize with SPICE
   * kernels (typically via spiceinit) or this initialization will fail.
   *
   * @param cube ISIS Cube to initialize
   */
  void MdisGeometry::init(Cube &cube) {
    _label = *cube.label();
    _orglabel = OriginalLabel(cube.fileName()).ReturnLabels();
    _nSubframes = (int) _orglabel.findKeyword("MESS:SUBFRAME",
                  PvlObject::Traverse);
    _camera = CameraFactory::Create(cube);
    _digitsPrecision = _defaultDigits;
    _NullDefault = "\"N/A\"";
    _doUpdate = true;
    _spice.Load(_label);
    return;
  }


  /**
   * @brief Compute values related to camera attitude
   *
   * This method determines keywords that provides camera FOV data.  It computes
   * RA and DEC coordinates of the boresight and its 4 corners.  It generates
   * the follwing keywords:  RA_DEC_REF_PIXEL, RIGHT_ASCENSION, DECLINATION,
   * TWIST_ANGLE, RETICLE_POINT_RA and RETICLE_POINT_DECLINATION.
   *
   * @param geom Pvl container for generated keyword/value data
   */
  void MdisGeometry::GeometryKeys(Pvl &geom) {
    // Ensure there is a camera model instantiated!
    if (!_camera) {
      string mess = "No image (camera model) established for Geometry keys!";
      throw IException(IException::Programmer, mess.c_str(),
                       _FILEINFO_);
    }

// Get the center ra/dec
    double refSamp, refLine;
    refCenterCoord(refSamp, refLine);

    std::vector<double> refPixel;
    refPixel.push_back(refSamp);
    refPixel.push_back(refLine);
    geom += format("RA_DEC_REF_PIXEL", refPixel);

    _camera->SetImage(refSamp, refLine);
    double centerRa  = _camera->RightAscension();
    double centerDec = _camera->Declination();
    geom += format("RIGHT_ASCENSION", centerRa,  "DEG");
    geom += format("DECLINATION",    centerDec, "DEG");

// Compute the celestial north clocking angle for TWIST_ANGLE
    double res = _camera->RaDecResolution();
    _camera->SetRightAscensionDeclination(centerRa, centerDec + 2.0 * res);
    double x = _camera->Sample() - refSamp;
    double y = _camera->Line() - refLine;
    double rot = atan2(-y, x) * 180.0 / Isis::PI;
    rot = 90.0 - rot;
    if (rot < 0.0) rot += 360.0;
    // Above completes celestial north, below is twist angle
    double twist_angle = (180.0 - rot);
    twist_angle = TProjection::To360Domain(fmod(twist_angle, 360.0));
    geom += format("TWIST_ANGLE", twist_angle, "DEG");

//  Now compute the RA/DEC reticle points
    std::vector<double> retRa, retDec;

    refUpperLeftCoord(refSamp, refLine);
    _camera->SetImage(refSamp, refLine);
    retRa.push_back(_camera->RightAscension());
    retDec.push_back(_camera->Declination());

    refUpperRightCoord(refSamp, refLine);
    _camera->SetImage(refSamp, refLine);
    retRa.push_back(_camera->RightAscension());
    retDec.push_back(_camera->Declination());

    refLowerLeftCoord(refSamp, refLine);
    _camera->SetImage(refSamp, refLine);
    retRa.push_back(_camera->RightAscension());
    retDec.push_back(_camera->Declination());

    refLowerRightCoord(refSamp, refLine);
    _camera->SetImage(refSamp, refLine);
    retRa.push_back(_camera->RightAscension());
    retDec.push_back(_camera->Declination());

    geom += format("RETICLE_POINT_RA", retRa, "DEG");
    geom += format("RETICLE_POINT_DECLINATION", retDec, "DEG");

    return;
  }

  /**
   * @brief Computes geometric values related to a target
   *
   * This method computes geometric values related to an observed target, such
   * as Earth, Moon, Mercury, etc...
   *
   * Not all data will be of a planetary body. Some are taken of deep space (we
   * call it Sky) or other undefined bodies.  These types of observations are
   * not defined for the values computed herein.  The criteria used here is the
   * center reference pixel must intersect the target body.  It is undefined for
   * Sky images and will result in a null string ("N/A", probably) for all
   * keyword values.
   *
   * If the center reference pixel does intersect the target body, the values
   * should be complete for most cases. One exception is the
   * RETICLE_POINT_LATITUDE and RETICLE_POINT_LONGITUDE values, which are taken
   * at the 4 corners of the image.  It is not uncommon for flyby images to have
   * the target body centered in the camera FOV and not all corner pixels
   * intersect it.  For these cases, the values will be the null string.  The
   * opposite may also be true in that the center pixel does not intersect the
   * target body but one or more of the reticle points do.  This method will
   * resolve as many of these parameters as are defined.
   *
   * The following keyword values are computed in this method:
   * SC_TARGET_POSITION_VECTOR, TARGET_CENTER_DISTANCE, SLANT_DISTANCE,
   * CENTER_LATITUDE, CENTER_LONGITUDE, HORIZONTAL_PIXEL_SCALE,
   * VERTICAL_PIXEL_SCALE, SMEAR_MAGNITUDE, SMEAR_AZIMUTH, NORTH_AZIMUTH,
   * RETICLE_POINT_LATITUDE and RETICLE_POINT_LONGITUDE.
   *
   * @param geom Pvl container for generated keyword/value data
   */
  void MdisGeometry::TargetKeys(Pvl &geom) {
    // Ensure there is a camera model instantiated!
    if (!_camera) {
      string mess = "No image (camera model) established for Target keys!";
      throw IException(IException::Programmer, mess.c_str(),
                       _FILEINFO_);
    }

    // Get sc_target_position_vector and target_center_distance for all targets
    // except Sky
    if (!_camera->target()->isSky()) {
      SpicePosition *scpos = _camera->instrumentPosition();
      std::vector<double> jVec;
      jVec = scpos->Coordinate();
      geom += format("SC_TARGET_POSITION_VECTOR", jVec, "KM");

      //  Compute distances
      geom += format("TARGET_CENTER_DISTANCE", _camera->targetCenterDistance(),
                     "KM");
    }
    else if (_doUpdate) {
      geom += format("SC_TARGET_POSITION_VECTOR", Null);
      geom += format("TARGET_CENTER_DISTANCE", Null);
    }

    // Get reference pixel coordinate
    double refSamp, refLine;
    refCenterCoord(refSamp, refLine);

    //  Set point at center
    _camera->SetImage(refSamp, refLine);
    if (_camera->HasSurfaceIntersection()) {

      geom += format("SLANT_DISTANCE", _camera->SlantDistance(), "KM");

      //  Geometric coordinages
      geom += format("CENTER_LATITUDE", _camera->UniversalLatitude(), "DEG");
      geom += format("CENTER_LONGITUDE", _camera->UniversalLongitude(), "DEG");

      // Resolution
      geom += format("HORIZONTAL_PIXEL_SCALE", _camera->SampleResolution(), "M");
      geom += format("VERTICAL_PIXEL_SCALE", _camera->LineResolution(), "M");

//  COMPUTE SMEAR MAGNITUDE AND AZIMUTH

      double smear_magnitude, smear_azimuth;
      if (SmearComponents(smear_magnitude, smear_azimuth)) {
        geom += format("SMEAR_MAGNITUDE", smear_magnitude, "PIXELS");
        geom += format("SMEAR_AZIMUTH", smear_azimuth, "DEG");
      }
      else if (_doUpdate) {
        geom += format("SMEAR_MAGNITUDE", Null);
        geom += format("SMEAR_AZIMUTH", Null);
      }

      //  Other angles
      geom += format("NORTH_AZIMUTH", _camera->NorthAzimuth(), "DEG");
    }
    else if (_doUpdate) {
      geom += format("SLANT_DISTANCE", Null);
      geom += format("CENTER_LATITUDE", Null);
      geom += format("CENTER_LONGITUDE", Null);
      geom += format("HORIZONTAL_PIXEL_SCALE", Null);
      geom += format("VERTICAL_PIXEL_SCALE", Null);
      geom += format("SMEAR_MAGNITUDE", Null);
      geom += format("SMEAR_AZIMUTH", Null);
      geom += format("NORTH_AZIMUTH", Null);
    }

    //  Now compute the reticle points
    std::vector<double> retLat, retLon;
    int nGood(0);

    refUpperLeftCoord(refSamp, refLine);
    _camera->SetImage(refSamp, refLine);
    if (_camera->HasSurfaceIntersection()) {
      retLat.push_back(_camera->UniversalLatitude());
      retLon.push_back(_camera->UniversalLongitude());
      nGood++;
    }
    else {
      retLat.push_back(Null);
      retLon.push_back(Null);
    }

    refUpperRightCoord(refSamp, refLine);
    _camera->SetImage(refSamp, refLine);
    if (_camera->HasSurfaceIntersection()) {
      retLat.push_back(_camera->UniversalLatitude());
      retLon.push_back(_camera->UniversalLongitude());
      nGood++;
    }
    else {
      retLat.push_back(Null);
      retLon.push_back(Null);
    }

    refLowerLeftCoord(refSamp, refLine);
    _camera->SetImage(refSamp, refLine);
    if (_camera->HasSurfaceIntersection()) {
      retLat.push_back(_camera->UniversalLatitude());
      retLon.push_back(_camera->UniversalLongitude());
      nGood++;
    }
    else {
      retLat.push_back(Null);
      retLon.push_back(Null);
    }

    refLowerRightCoord(refSamp, refLine);
    _camera->SetImage(refSamp, refLine);
    if (_camera->HasSurfaceIntersection()) {
      retLat.push_back(_camera->UniversalLatitude());
      retLon.push_back(_camera->UniversalLongitude());
      nGood++;
    }
    else {
      retLat.push_back(Null);
      retLon.push_back(Null);
    }

    if (nGood > 0) {
      geom += format("RETICLE_POINT_LATITUDE", retLat, "DEG");
      geom += format("RETICLE_POINT_LONGITUDE", retLon, "DEG");
    }
    else if (_doUpdate) {
      geom += format("RETICLE_POINT_LATITUDE", retLat);
      geom += format("RETICLE_POINT_LONGITUDE", retLon);
    }

    //  Do subframe targets
    SubframeTargetKeys(geom);

    return;
  }

  /**
   * @brief Computes geometric values related to a target for subframe images
   *
   * This method computes geometric values related to an observed target, such
   * as Earth, Moon, Mercury, etc..., for subframe images.  This
   *
   * @param geom Pvl container for generated keyword/value data
   */
  void MdisGeometry::SubframeTargetKeys(Pvl &geom) {
    // Ensure there is a camera model instantiated!
    if (!_camera) {
      string mess = "No image (camera model) established for Target keys!";
      throw IException(IException::Programmer, mess.c_str(),
                       _FILEINFO_);
    }

    //  Now compute the reticle points of all subframes if they exist
    for (int i = 1 ; i <= 5 ; i++) {
      QString n = QString::number(i);
      QString object = "SUBFRAME" + n + "_PARAMETERS/";

      double sample, line, width, height;
      if (!getSubframeCoordinates(i, sample, line, width, height)) {
        //  Subframe does not exist
        std::vector<double> retLat(4, Null), retLon(4, Null);
        geom += format(object + "RETICLE_POINT_LATITUDE", retLat);
        geom += format(object + "RETICLE_POINT_LONGITUDE", retLon);
      }
      else {
        //  Subframe exists in this frame
        std::vector<double> retLat, retLon;
        int nGood(0);

        double refSamp(sample), refLine(line);
        _camera->SetImage(refSamp, refLine);
        if (_camera->HasSurfaceIntersection()) {
          retLat.push_back(_camera->UniversalLatitude());
          retLon.push_back(_camera->UniversalLongitude());
          nGood++;
        }
        else {
          retLat.push_back(Null);
          retLon.push_back(Null);
        }

        refSamp = sample + width - 1.0;
        refLine = line;
        _camera->SetImage(refSamp, refLine);
        if (_camera->HasSurfaceIntersection()) {
          retLat.push_back(_camera->UniversalLatitude());
          retLon.push_back(_camera->UniversalLongitude());
          nGood++;
        }
        else {
          retLat.push_back(Null);
          retLon.push_back(Null);
        }

        refSamp = sample;
        refLine = line + height - 1.0;
        _camera->SetImage(refSamp, refLine);
        if (_camera->HasSurfaceIntersection()) {
          retLat.push_back(_camera->UniversalLatitude());
          retLon.push_back(_camera->UniversalLongitude());
          nGood++;
        }
        else {
          retLat.push_back(Null);
          retLon.push_back(Null);
        }

        refSamp = sample + width - 1.0;
        refLine = line + height - 1.0;
        _camera->SetImage(refSamp, refLine);
        if (_camera->HasSurfaceIntersection()) {
          retLat.push_back(_camera->UniversalLatitude());
          retLon.push_back(_camera->UniversalLongitude());
          nGood++;
        }
        else {
          retLat.push_back(Null);
          retLon.push_back(Null);
        }

        if (nGood > 0) {
          geom += format(object + "RETICLE_POINT_LATITUDE", retLat, "DEG");
          geom += format(object + "RETICLE_POINT_LONGITUDE", retLon, "DEG");
        }
        else if (_doUpdate) {
          geom += format(object + "RETICLE_POINT_LATITUDE", retLat);
          geom += format(object + "RETICLE_POINT_LONGITUDE", retLon);
        }
      }
    }
    return;
  }

  /**
   * @brief Determine if the specified subframe exists and return its data
   *
   * This method looks at the contents of the orginal PDS EDR label to determine
   * if the requested subframe exists within the full frame.  If it does, the
   * starting sample and line, as well as the width and height are return.  The
   * method returns a true condition when a subframe exists, otherwise it
   * returns false (with zeros for coordinates).
   *
   * @param frameno  Specified frame number (valid range: 1-5)
   * @param sample   Returns the starting sample coordinate of the subframe
   * @param line     Returns the starting line coordinate of the subframe
   * @param width    Returns the width in pixels of the subframe
   * @param height   Returns the heigth in pixels of the subframe
   *
   * @return bool    Returns true if the subframe exists in this image, false
   *                 otherwise.
   */
  bool MdisGeometry::getSubframeCoordinates(int frameno,
      double &sample, double &line,
      double &width, double &height) {

    //  Does the subframe exist?
    if ((frameno < 1) || (frameno > _nSubframes)) {
      //  No
      sample = line = width = height = 0.0;
      return (false);
    }
    else {
      //  It does exist, extract coordinates from original image label
      QString n = QString::number(frameno);
      sample  = (double) _orglabel.findKeyword("MESS:SUBF_X" + n.toStdString(),
                PvlObject::Traverse);
      line    = (double) _orglabel.findKeyword("MESS:SUBF_Y" + n.toStdString(),
                PvlObject::Traverse);
      width   = (double) _orglabel.findKeyword("MESS:SUBF_DX" + n.toStdString(),
                PvlObject::Traverse);
      height  = (double) _orglabel.findKeyword("MESS:SUBF_DY" + n.toStdString(),
                PvlObject::Traverse);

    }
    return (true);
  }


  /**
   * @brief Compute smear components of the reference pixel
   *
   * This method computes the SMEAR_MAGNITUDE and SMEAR_AZIMUTH keyword values
   * if possible.  There are cases where the values cannot be computed, such as
   * the boresight is on the limb and the magnitude becomes infinity.
   *
   * IMPORTANT:  It is assumed that the pixel location to compute these values
   * for are already set in the _camera object and that this pixel intersections
   * the target surface. All computations will be derived from this intersection
   * point.
   *
   * Note that at the time of this development, ISIS did not provide all the
   * necessary NAIF SPICE components though the API (namely, velecity vectors).
   * Because of this, all the SPICE kernels must be loaded as the values cannnot
   * come from what is internally cached in the Spice class.  This is an
   * unfortunate consquence as efficiency is severely compromised by this issue.
   * This object provides this requirement via the SpiceManager class.
   *
   * @param smear_magnitude  Returns the smear magnitude compoment in pixel
   *                         units.
   * @param smear_azimuth Returns the smear velocity vector in km/s units.
   *
   * @return bool True if the components can be computed, false if it fails.
   */
  bool MdisGeometry::SmearComponents(double &smear_magnitude,
                                     double &smear_azimuth) {
    smear_magnitude = Null;
    smear_azimuth = Null;

    // Ensure there is a camera model instantiated!
    if (!_camera) {
      string mess = "No image/camera model established for smear components!";
      throw IException(IException::Programmer, mess.c_str(), _FILEINFO_);
    }

    // Get NAIF body codes
    SpiceInt scCode(-236), targCode(0);
    SpiceBoolean found;
    QString target = _camera->target()->name();

    try {
      scCode = Target::lookupNaifBodyCode("MESSENGER");
      targCode = Target::lookupNaifBodyCode(target.toLatin1().data());
    }
    catch (...) {
      found = false;
      return found;
    }

    //  Get the target state (starg)
    SpiceRotation *rotate = _camera->instrumentRotation();
    SpiceDouble starg[6];  // Position and velocity vector in J2000
    SpiceDouble lt;
    spkez_c(targCode, rotate->EphemerisTime(), "J2000", "LT+S", scCode,
            starg, &lt);

    //  Get surfarce intersection vector in body-fixed coordinates (surfx)
    double surfx[3];
    _camera->Coordinate(surfx);

    //  Get camera transform (ticam)
    std::vector<double> ticam = rotate->Matrix();

    //  Get angular velocity vector of camera (av)
    SpiceDouble sclkdp;
    (void) sce2c_c(scCode, rotate->EphemerisTime(), &sclkdp);

    //  Determine instrument ID (inst)
    PvlKeyword &key = _label.findKeyword("NaifIkCode", PvlObject::Traverse);
    QString iCode = key[0];
    key = _label.findKeyword("Number", PvlObject::Traverse);

    // Get CK time tolerance (tol)
    SpiceDouble tol;
    SpiceInt tmp;
    gdpool_c(QString("INS" + iCode + "_CK_TIME_TOLERANCE").toLatin1().data(),
             0, 1, &tmp, &tol, &found);

    // Finally get av
    SpiceDouble cmat[3][3], av[3], clkout;
    (void) ckgpav_c((scCode * 1000), sclkdp, tol, "J2000", cmat, av, &clkout,
                    &found);
    if (!found) {
#if defined(DEBUG)
      cout << "Cannot get angular camera velocity for time "
           << setprecision(12) << sclkdp << "!\n";
#endif
      return (false);
    }

    //  Get the state transformation matrix (tsipm)
    SpiceChar frname[40];
    SpiceInt frcode;
    (void) cidfrm_c(targCode, sizeof(frname), &frcode, frname, &found);
    if (!found) {
      return (false);
    }

    SpiceDouble tsipm[6][6];
    (void) sxform_c("J2000", frname, rotate->EphemerisTime(), tsipm);

    //  Get focal length
    double foclen = _camera->FocalLength();

    // Get pixel scale (pix/mm) from camera pixel pitch (mm/pix)
    double pxlscl = 1.0 / _camera->PixelPitch();

//--  Now implement the SMEAR routine (smrimg)
    SpiceDouble tipm[3][3], dtipm[3][3];
    for (int i = 0 ; i < 3 ; i++) {
      for (int j = 0 ; j < 3 ; j++) {
        tipm[i][j] = tsipm[i][j];
        dtipm[i][j] = tsipm[i][j+3];
      }
    }

    //  rav2dr stuff here
    SpiceDouble omega[3][3];
    omega[0][0] = 0.0;
    omega[1][1] = 0.0;
    omega[2][2] = 0.0;

    omega[0][1] = -av[2];
    omega[0][2] =  av[1];
    omega[1][2] = -av[0];

    omega[1][0] =  av[2];
    omega[2][0] = -av[1];
    omega[2][1] =  av[0];

    SpiceDouble dticam[3][3];
    mxmt_c(&ticam[0], omega, dticam);

    //--  Done with rav2dr

    // Complete the rest of smrimg
    SpiceDouble surfxi[3], vi[3];
    mtxv_c(tipm, surfx, surfxi);
    vadd_c(starg, surfxi, vi);

    SpiceDouble dvb[3], dvi[3];
    mtxv_c(dtipm, surfx, dvb);
    vadd_c(&starg[3], dvb, dvi);

    SpiceDouble vc[3], dvc1[3], dvc2[3], dvc[3];
    mxv_c(&ticam[0], vi, vc);

    mxv_c(&ticam[0], dvi, dvc1);
    mxv_c(dticam, vi, dvc2);
    vadd_c(dvc1, dvc2, dvc);

    // Make sure we Vf can be computed
    if (vc[2] == 0.0) {
      return (false);
    }

    // Compute derivative of Vf which is dvf
    SpiceDouble dvf[2];
    vlcomg_c(2, -foclen * dvc[2] / (vc[2]*vc[2]), vc,
             foclen        / vc[2],         dvc, dvf);

    // dvf has units for mm/sec.  Scale by pixel pitch and multiply
    // by exposure length to obtain smear
    key = _label.findKeyword("ExposureDuration", PvlObject::Traverse);
    double explen = (double) key;   // in milliseconds

    SpiceDouble smear[2];
    vsclg_c(pxlscl * (explen / 1000.0), dvf, 2, smear); //convert explen to seconds

    //  Compute the norm and azimuth angle
    smear_magnitude = vnormg_c(smear, 2);
    NaifStatus::CheckErrors();

    if (smear_magnitude == 0.0) {
      smear_azimuth = 0.0;
    }
    else {
      smear_azimuth = atan2(smear[1], smear[0]) * dpr_c();
      if (smear_azimuth  < 0.0) smear_azimuth += 360.0;
    }

    return (true);
  }


  /**
   * @brief Computes spacecraft attitude geomtric values
   *
   * This method computes geometric values related to spacecraft location and
   * attitude. These will typically always be defined as long as SPICE is valid
   * at the image epoch.
   *
   * The following keyword values are computed: SUB_SPACECRAFT_LATITUDE,
   * SUB_SPACECRAFT_LONGITUDE, SPACECRAFT_ALTITUDE, SUB_SPACECRAFT_AZIMUTH,
   * SPACECRAFT_SOLAR_DISTANCE, SC_SUN_POSITION_VECTOR and
   * SC_SUN_VELOCITY_VECTOR.
   *
   * The SUB_SPACECRAFT_LATITUDE, SUB_SPACECRAFT_LONGITUDE, SPACECRAFT_ALTITUDE,
   * and SUB_SPACECRAFT_AZIMUTH are subject to center reference pixel
   * intersection with the target body and are also not relevent to Sky images.
   *
   * @param geom Pvl container for generated keyword/value data
   */
  void MdisGeometry::SpacecraftKeys(Pvl &geom) {
    // Ensure there is a camera model instantiated!
    if (!_camera) {
      string mess = "No image (camera model) established for Spacecraft keys!";
      throw IException(IException::Programmer, mess.c_str(), _FILEINFO_);
    }

    // Get reference pixel coordinate
    double refSamp, refLine;
    refCenterCoord(refSamp, refLine);

    // Get the center ra/dec
    _camera->SetImage(refSamp, refLine);
    if (!_camera->target()->isSky()) {
      double lat, lon;
      _camera->subSpacecraftPoint(lat, lon);
      geom += format("SUB_SPACECRAFT_LATITUDE", lat, "DEG");
      geom += format("SUB_SPACECRAFT_LONGITUDE", lon, "DEG");
      geom += format("SPACECRAFT_ALTITUDE", _camera->SpacecraftAltitude(), "KM");

      if (_camera->HasSurfaceIntersection()) {
        geom += format("SUB_SPACECRAFT_AZIMUTH", _camera->SpacecraftAzimuth(),
                       "DEG");
      }
      else if (_doUpdate) {
        geom += format("SUB_SPACECRAFT_AZIMUTH", Null);
      }
    }
    else if (_doUpdate) {
      geom += format("SUB_SPACECRAFT_LATITUDE", Null);
      geom += format("SUB_SPACECRAFT_LONGITUDE", Null);
      geom += format("SPACECRAFT_ALTITUDE", Null);
      geom += format("SUB_SPACECRAFT_AZIMUTH", Null);
    }

    // Compute distance and position from spacecraft to sun
    // This is the J2000 target to sun reference
    SpicePosition *sunpos = _camera->sunPosition();
    std::vector<double> jVec = sunpos->Coordinate();

    //  J2000 spacecraft to sun reference
    SpicePosition *campos = _camera->instrumentPosition();
    std::vector<double> sVec = campos->Coordinate();

    //  Subtract target-sun vector from sc-sun vector and normalize to get
    //  distance from observer to sun
    double scPos[3];
    vsub_c(&sVec[0], &jVec[0], scPos);
    double sc_sun_dist = vnorm_c(scPos);
    geom += format("SPACECRAFT_SOLAR_DISTANCE", sc_sun_dist, "KM");

    //  Record position vector
    std::vector<double> scVec;
    scVec.push_back(scPos[0]);
    scVec.push_back(scPos[1]);
    scVec.push_back(scPos[2]);
    geom += format("SC_SUN_POSITION_VECTOR", scVec, "KM");

    geom += format("SC_SUN_VELOCITY_VECTOR", ScVelocityVector(), "KM/S");
    return;
  }

  /**
   * @brief Computes the sun velocity vector relative to observer (spacecraft)
   *
   * This method computes the x, y, z component of the velocity vector of sun
   * relative to the observer, expressed in J2000 coordinates, and corrected for
   * light time, evaluated at epoch at which image was taken. Units are
   * kilometers per second.  This routine provides the SC_SUN_VELOCITY_VECTOR
   * keyword.
   *
   * @return std::vector<double>  x, y, z velocity components in km/s units
   */
  std::vector<double> MdisGeometry::ScVelocityVector() {
    // Ensure there is a camera model instantiated!
    if (!_camera) {
      string mess = "No image/camera model established for Spacecraft Velocity keys!";
      throw IException(IException::Programmer, mess.c_str(), _FILEINFO_);
    }

    // Get NAIF body codes
    SpiceInt sc(-236), sun(10);
    sc = Target::lookupNaifBodyCode("MESSENGER");
    sun = Target::lookupNaifBodyCode("SUN");

    //  Get the Sun to Messenger state matrix
    SpiceRotation *rotate = _camera->bodyRotation();
    SpiceDouble stateJ[6];  // Position and velocity vector in J2000
    SpiceDouble lt;
    spkez_c(sc , rotate->EphemerisTime(), "J2000", "LT+S", sun, stateJ, &lt);
    NaifStatus::CheckErrors();

    // Stage result and negate as it needs to be relative to Messenger
    vector<double> scvel;
    scvel.push_back(stateJ[3]);
    scvel.push_back(stateJ[4]);
    scvel.push_back(stateJ[5]);
    vminus_c(&scvel[0], &scvel[0]);

    return (scvel);
  }

  /**
   * @brief Compute viewing and lighting geometric components
   *
   * This method computes the viewing and lighting angles and distance values in
   * relation to the target and the sun.  If the center reference pixel
   * coordinate does not intersect the surface these values are undefined and
   * will result in null strings for each keyword.
   *
   * The following keywords are computed:  SOLAR_DISTANCE, SUB_SOLAR_AZIMUTH,
   * SUB_SOLAR_LATITUDE, SUB_SOLAR_LONGITUDE, INCIDENCE_ANGLE, EMISSION_ANGLE,
   * and PHASE_ANGLE.
   *
   * @param geom Pvl container for generated keyword/value data
   */
  void MdisGeometry::ViewingAndLightingKeys(Pvl &geom) {
    // Ensure there is a camera model instantiated!
    if (!_camera) {
      string mess = "No image (camera model) established for Viewing & Lighting keys!";
      throw IException(IException::Programmer, mess.c_str(), _FILEINFO_);
    }

// Get reference pixel coordinate
    double refSamp, refLine;
    refCenterCoord(refSamp, refLine);

// Get the center ra/dec
    _camera->SetImage(refSamp, refLine);

    //  These parameters only require a target other than the Sky
    if (!_camera->target()->isSky()) {
      double sslat, sslon;
      _camera->subSolarPoint(sslat, sslon);
      geom += format("SUB_SOLAR_LATITUDE", sslat, "DEG");
      geom += format("SUB_SOLAR_LONGITUDE", sslon, "DEG");

      SpicePosition *sunpos = _camera->sunPosition();
      std::vector<double> jVec = sunpos->Coordinate();
      double solar_dist = vnorm_c(&jVec[0]);

      geom += format("SOLAR_DISTANCE", solar_dist, "KM");

    }
    else if (_doUpdate) {
      geom += format("SUB_SOLAR_LATITUDE", Null);
      geom += format("SUB_SOLAR_LONGITUDE", Null);
      geom += format("SOLAR_DISTANCE", Null);
    }

    //  These require surface intersections
    if (_camera->HasSurfaceIntersection()) {
// Solar information
      geom += format("SUB_SOLAR_AZIMUTH", _camera->SunAzimuth(), "DEG");
      geom += format("INCIDENCE_ANGLE", _camera->IncidenceAngle(), "DEG");
      geom += format("PHASE_ANGLE", _camera->PhaseAngle(), "DEG");
      geom += format("EMISSION_ANGLE", _camera->EmissionAngle(), "DEG");
      geom += format("LOCAL_HOUR_ANGLE", _camera->LocalSolarTime() * 15.0, "DEG");
    }
    else if (_doUpdate) {
      geom += format("SUB_SOLAR_AZIMUTH", Null);
      geom += format("INCIDENCE_ANGLE", Null);
      geom += format("PHASE_ANGLE", Null);
      geom += format("EMISSION_ANGLE", Null);
      geom += format("LOCAL_HOUR_ANGLE", Null);
    }
    return;

  }

  /**
   * @brief Format a single double value according to specifications
   *
   * This method formats the actual double value according to specifications of
   * the PDS and mission definitions. If the passed values is a an ISIS special
   * pixel (namely a Null value), the special null string is subsitituted.
   *
   * The double precision value is subject to formatted digits of precision of a
   * predetermined magnitude.  It is manageable at the caller level.
   *
   * @param name   Name of the keyword to create
   * @param value  Double precision value to format.  If its a special pixel,
   *               the null string will be substituted.
   * @param unit Optional unit for the value.  Pass an empty string to exclude
   *             unit from format.
   *
   * @return PvlKeyword  Returns the formatted PVL keyword
   */
  PvlKeyword MdisGeometry::format(const QString &name, const double &value,
                                  const QString &unit) const {
    if (IsSpecial(value)) {
      return (PvlKeyword(name.toStdString(), _NullDefault.toStdString()));
    }
    else  {
      return (PvlKeyword(name.toStdString(), DoubleToString(value).toStdString(), unit.toStdString()));
    }
  }

  /**
   * @brief Format a vector of double precision values
   *
   * This method formats double valued according to specifications of the PDS
   * and mission definitions. If the passed values is a an ISIS special pixel
   * (namely a Null value), the special null string is subsitituted.
   *
   * The double precision value is subject to formatted digits of precision of a
   * predetermined magnitude.  It is manageable at the caller level.
   *
   * @param name   Name of the keyword to create
   * @param values Vector of double precision values to format.  If its a
   *               special pixel, the null string will be substituted.
   * @param unit Optional unit for the value.  Pass an empty string to exclude
   *             unit from format.
   *
   * @return PvlKeyword  Returns the formatted PVL keyword
   */
  PvlKeyword MdisGeometry::format(const QString &name,
                                  const std::vector<double> &values,
                                  const QString &unit) const {
    PvlKeyword key(name.toStdString());
    for (unsigned int i = 0 ; i < values.size() ; i++) {
      if (IsSpecial(values[i])) {
        key.addValue(_NullDefault.toStdString());
      }
      else {
        key.addValue(DoubleToString(values[i]).toStdString(), unit.toStdString());
      }
    }
    return (key);
  }

  /**
   * @brief Create a PvlKeyword from a vector of string values
   *
   * This method formats a vector of string values according to specifications
   * of the PDS and mission definitions. If any one of the passed string values
   * is a an empty string, the special null string is subsitituted.
   *
   * @param name   Name of the keyword to create
   * @param values Vector of string values to format.  Empty strings are
   *               substituted by the null string value.
   * @param unit Optional unit for the value.  Pass an empty string to exclude
   *             unit from format.
   *
   * @return PvlKeyword  Returns the formatted PVL keyword
   */
  PvlKeyword MdisGeometry::format(const QString &name,
                                  const std::vector<std::string> &values,
                                  const QString &unit) const {
    PvlKeyword key(name.toStdString());
    for (unsigned int i = 0 ; i < values.size() ; i++) {
      if (values[i].empty()) {
        key.addValue(_NullDefault.toStdString());
      }
      else {
        key.addValue(values[i].c_str(), unit.toStdString());
      }
    }
    return (key);
  }

  /**
   * @brief Create a PvlKeyword from a vector of string values
   *
   * This method formats a vector of string values according to specifications
   * of the PDS and mission definitions. If any one of the passed string values
   * is a an empty string, the special null string is subsitituted.
   *
   * @param name   Name of the keyword to create
   * @param values Vector of string values to format.  Empty strings are
   *               substituted by the null string value.
   * @param unit Optional unit for the value.  Pass an empty string to exclude
   *             unit from format.
   *
   * @return PvlKeyword  Returns the formatted PVL keyword
   */
  PvlKeyword MdisGeometry::format(const QString &name,
                                  const std::vector<QString> &values,
                                  const QString &unit) const {
    PvlKeyword key(name.toStdString());
    for (unsigned int i = 0 ; i < values.size() ; i++) {
      if (values[i].isEmpty()) {
        key.addValue(_NullDefault.toStdString());
      }
      else {
        key.addValue(values[i].toStdString(), unit.toStdString());
      }
    }
    return (key);
  }

  /**
   * @brief Convert a double value to a string subject to precision specs
   *
   * This method converts a double value to a string that has a prefined digitis
   * of precision.  Fixed float form is used with the specified number of digits
   * of precision.
   *
   * @param value Double value to convert to string
   *
   * @return QString Returns the converted string
   */
  QString MdisGeometry::DoubleToString(const double &value) const {
    if (IsSpecial(value)) {
      return (_NullDefault);
    }

    //  Format the string to specs
    ostringstream strcnv;
    strcnv.setf(std::ios::fixed);
    strcnv << setprecision(_digitsPrecision) << value;
    return (QString::fromStdString(strcnv.str().c_str()));
  }
}  // namespace Isis
