/** This is free and unencumbered software released into the public domain.

The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */

#include "LroWideAngleCamera.h"
#include "LroWideAngleCameraFocalPlaneMap.h"
#include "LroWideAngleCameraDistortionMap.h"

#include <sstream>
#include <iomanip>

#include <QString>
#include <QVector>

#include "CameraFocalPlaneMap.h"
#include "CameraSkyMap.h"
#include "CollectorMap.h"
#include "IException.h"
#include "IString.h"
#include "iTime.h"
#include "NaifStatus.h"
#include "PushFrameCameraDetectorMap.h"
#include "PushFrameCameraGroundMap.h"

using namespace std;
namespace Isis {
  /**
   * Constructor for the LRO WAC Camera Model
   *
   * @param lab Pvl Label to create the camera model from
   *
   * @throws Isis::iException::User - The image does not appear to be a Lunar
   *             Reconaissance Orbiter Wide Angle Camera image
   * @internal
   *   @history 2011-05-03 Jeannie Walldren - Added NAIF error check.
   */
  LroWideAngleCamera::LroWideAngleCamera(Cube &cube) :
    PushFrameCamera(cube) {

    NaifStatus::CheckErrors();

    m_spacecraftNameLong = "Lunar Reconnaissance Orbiter";
    m_spacecraftNameShort = "LRO";

    // Set up the camera characteristics
    instrumentRotation()->SetFrame(naifIkCode());
    SetFocalLength();
    SetPixelPitch();

    Pvl &lab = *cube.label();

    // Get the ephemeris time from the labels
    double et;
    PvlGroup &inst = lab.findGroup("Instrument", Pvl::Traverse);
    QString stime = QString::fromStdString(inst["SpacecraftClockStartCount"]);
    et = getClockTime(stime).Et();

    p_exposureDur = std::stod(inst["ExposureDuration"]);
    // TODO:  Changed et - exposure to et + exposure.
    //   Think about if this is correct
    p_etStart = et + ((p_exposureDur / 1000.0) / 2.0);

    // Compute the framelet size and number of framelets
    QString instId = QString::fromStdString(inst["InstrumentId"][0]).toUpper();

    int frameletSize = 14;
    int sumMode = 1;
    int filterIKBase = 10;

    if (instId == "WAC-UV") {
      sumMode = 4;
      frameletSize = 16;
      filterIKBase = 15 - 1; //  New UV IK code = filterIKBase + BANDID
      m_instrumentNameLong = "Wide Angle Camera Ultra Violet";
      m_instrumentNameShort = "WAC-UV";
    }
    else if (instId == "WAC-VIS") {
      sumMode = 1;
      frameletSize = 14;
      filterIKBase = 10 - 3; //  New VIS IK code = filterIKBase + BANDID
      m_instrumentNameLong = "Wide Angle Camera Visual";
      m_instrumentNameShort = "WAC-VIS";
    }
    else {
      std::string msg = "Invalid value [" + instId
                    + "] for keyword [InstrumentId]";
      throw IException(IException::User, msg, _FILEINFO_);
    }

    p_nframelets = (int) (ParentLines() / (frameletSize / sumMode));

    // Setup the line detector offset map for each filter
    int nbands = (int) lab.findKeyword("Bands", PvlObject::Traverse);
    const PvlGroup &bandBin = lab.findGroup("BandBin", Pvl::Traverse);
    const PvlKeyword &filtNames = bandBin["Center"];

    // Sanity check
    if (nbands != filtNames.size()) {
      ostringstream mess;
      mess << "Number bands in (file) label (" << nbands
           << ") do not match number of values in BandBin/Center keyword ("
           << filtNames.size() << ") - required for band-dependent geoemtry";
      throw IException(IException::User, mess.str(), _FILEINFO_);
    }

    // Is the data flipped?
    bool dataflipped = (QString::fromStdString(inst["DataFlipped"][0]).toUpper() == "YES");

    //  Now create detector offsets
    QString instCode = "INS" + QString::number(naifIkCode());

    QString ikernKey = instCode + "_FILTER_BANDCENTER";
    IntParameterList fbc = GetVector(ikernKey);

    ikernKey = instCode + "_FILTER_OFFSET";
    IntParameterList foffset = GetVector(ikernKey);

    //  Get band ID to determine new filter dependent IK codes
    ikernKey = instCode + "_FILTER_BANDID";
    IntParameterList fbandid = GetVector(ikernKey);


    // Create a map of filter wavelength to offset.  Also needs a reverse
    // lookup to order the offset into the CCD (ascending sort provided
    // automagically be CollectorMap).
    CollectorMap<int, int> filterToDetectorOffset, wavel,filterIKCode;
    for (int i = 0 ; i < foffset.size() ; i++) {
      filterToDetectorOffset.add(fbc[i], foffset[i]);
      wavel.add(foffset[i], fbc[i]);
      filterIKCode.add(fbc[i], naifIkCode() - (filterIKBase + fbandid[i]));  // New IK code
    }

    // Construct special format for framelet offsets into CCD.  Uses the above
    // reverse map.  Need only get the value (wavelength) of the map as the
    // key (offset) is sorted above.
    int frameletOffsetFactor = inst["ColorOffset"];
    if ( dataflipped ) frameletOffsetFactor *= -1;
    CollectorMap<int, int> filterToFrameletOffset;
    for (int j = 0 ; j < wavel.size() ; j++) {
      int wavelen = wavel.getNth(j);
      filterToFrameletOffset.add(wavelen, j * frameletOffsetFactor);
    }

    //  Now map the actual filters that exist in cube to camera components or
    // storage vectors for later band selection (see SetBand(vband))
    for (int i = 0; i < filtNames.size(); i++) {
      if (!filterToDetectorOffset.exists(std::stoi(filtNames[i]))) {
        std::string msg = "Unrecognized filter name [" + filtNames[i] + "]";
        throw IException(IException::Programmer, msg, _FILEINFO_);
      }

      p_detectorStartLines.push_back(filterToDetectorOffset.get(std::stoi(filtNames[i])));
      p_frameletOffsets.push_back(filterToFrameletOffset.get(std::stoi(filtNames[i])));

      QString kBase = "INS" + QString::number(filterIKCode.get(std::stoi(filtNames[i])));
      p_focalLength.push_back(getDouble(kBase+"_FOCAL_LENGTH"));
      p_boreSightSample.push_back(getDouble(kBase+"_BORESIGHT_SAMPLE"));
      p_boreSightLine.push_back(getDouble(kBase+"_BORESIGHT_LINE"));
    }

    // Setup detector map
    double frameletRate = (double) inst["InterframeDelay"] / 1000.0;
    PushFrameCameraDetectorMap *dmap = new PushFrameCameraDetectorMap(this,
        p_etStart, frameletRate, frameletSize);
    dmap->SetDetectorSampleSumming(sumMode);
    dmap->SetDetectorLineSumming(sumMode);

    // flipping disabled if already flipped
    bool flippedFramelets = dataflipped;
    dmap->SetFrameletOrderReversed(flippedFramelets, p_nframelets);
    dmap->SetFrameletsGeometricallyFlipped(false);

    //  get instrument-specific sample offset
    QString instModeId = QString::fromStdString(inst["InstrumentModeId"][0]).toUpper();
    // For BW mode, add the mode (0,1 (non-polar) or 2,3 (polar)) used to
    // acquire image
    if (instModeId == "BW") {
      instModeId += QString::fromStdString(inst["Mode"][0]);
      // There are no offsets for BW mode.. there can only be 1 filter
      //   and there must be 1 filter.
      p_frameletOffsets[0] = 0;
    }

    ikernKey = instCode + "_" + instModeId + "_SAMPLE_OFFSET";
    int sampOffset = getInteger(ikernKey);
    dmap->SetStartingDetectorSample(sampOffset+1);

    // Setup focal plane and distortion maps
    LroWideAngleCameraFocalPlaneMap *fplane = new LroWideAngleCameraFocalPlaneMap(this, naifIkCode());
    LroWideAngleCameraDistortionMap *distort = new LroWideAngleCameraDistortionMap(this, naifIkCode());

    for ( int i = 0 ; i < filtNames.size() ; i++ ) {
      fplane->addFilter(filterIKCode.get(std::stoi(filtNames[i])));
      distort->addFilter(filterIKCode.get(std::stoi(filtNames[i])));
    }

    // Setup the ground and sky map
    bool evenFramelets = (QString::fromStdString(inst["Framelets"][0]).toUpper() == "EVEN");
    new PushFrameCameraGroundMap(this, evenFramelets);
    new CameraSkyMap(this);

    SetBand(1);
    LoadCache();
    NaifStatus::CheckErrors();

    if(instId == "WAC-UV") {
      // geometric tiling is not worth trying for 4-line framelets
      SetGeometricTilingHint(2, 2);
    }
    else {
      SetGeometricTilingHint(8, 4);
    }
  }



  //! Destroys the LroWideAngleCamera object
  LroWideAngleCamera::~LroWideAngleCamera() {
  }



  /**
   * Sets the band in the camera model
   *
   * @param vband The band number to set
   */
  void LroWideAngleCamera::SetBand(const int vband) {

    // Sanity check on requested band
    int maxVirtualBands = min(p_detectorStartLines.size(), p_frameletOffsets.size());
    if (((vband <= 0) || (vband > maxVirtualBands)) && (vband > Bands())) {
      ostringstream mess;
      mess << "Requested virtual band (" << vband
           << ") outside valid (BandBin/Center) limits (1 - " << maxVirtualBands
           <<  ")";
      throw IException(IException::Programmer, mess.str(), _FILEINFO_);
    }

    //  Set up valid band access
    Camera::SetBand(vband);
    if ((vband > maxVirtualBands) && (vband <= Bands())) {
      // probably switching to a band from phocube or similar
      // instead of a different filter band, so just re-use the 
      // properties from the current band. 
      return;
    }

    PushFrameCameraDetectorMap *dmap = NULL;
    dmap = (PushFrameCameraDetectorMap *) DetectorMap();
    dmap->SetBandFirstDetectorLine(p_detectorStartLines.at(vband - 1));
    dmap->SetFrameletOffset(p_frameletOffsets.at(vband - 1));

    SetFocalLength(p_focalLength.at(vband-1));

    LroWideAngleCameraFocalPlaneMap *fplane = (LroWideAngleCameraFocalPlaneMap *) FocalPlaneMap();
    fplane->setBand(vband);
    fplane->SetDetectorOrigin(p_boreSightSample.at(vband-1) + 1.0,
                              p_boreSightLine.at(vband-1)   + 1.0);

    LroWideAngleCameraDistortionMap *distort = (LroWideAngleCameraDistortionMap *) DistortionMap();
    distort->setBand(vband);
    return;
  }



  /**
   * @param key
   * @return @b int Pool key size
   */
  int LroWideAngleCamera::PoolKeySize(const QString &key) const {
    SpiceBoolean found;
    SpiceInt n;
    SpiceChar ctype[1];
    dtpool_c(key.toLatin1().data(), &found, &n, ctype);
    if (!found) n = 0;
    return (n);
  }



  /**
   * @param key
   * @return @b vector < @b int >
   */
  LroWideAngleCamera::IntParameterList LroWideAngleCamera::GetVector(const QString &key) {
    QVariant poolKeySize = getStoredResult(key + "_SIZE", SpiceIntType);

    int nvals = poolKeySize.toInt();

    if (nvals == 0) {
      nvals = PoolKeySize(key);
      storeResult(key + "_SIZE", SpiceIntType, nvals);
    }

    if (nvals <= 0) {
      PvlObject NaifKeywords = getStoredNaifKeywords();
      if (!NaifKeywords.hasKeyword(key.toStdString())){
        std::string mess = "Kernel pool keyword " + key + " not found!";
        throw IException(IException::Programmer, mess, _FILEINFO_);
      }
      PvlKeyword kw = NaifKeywords[key.toStdString()];
      IntParameterList parms;
      for (int i = 0; i<kw.size(); i++){
        parms.push_back(std::stoi(kw[i]));
      }
      return parms;
    }

    IntParameterList parms;
    for (int i = 0 ; i < nvals ; i++) {
      parms.push_back(getInteger(key, i));
    }

    return (parms);
  }



  /**
   * The camera model is band dependent, so this method returns false
   *
   * @return bool False
   */
  bool LroWideAngleCamera::IsBandIndependent() {
    // only band independant if there is one filter 
    // band on the image
    return p_frameletOffsets.size() == 1;
  }



  /**
   * CK frame ID -  - Instrument Code from spacit run on CK
   *
   * @return @b int The appropriate instrument code for the "Camera-matrix"
   *         Kernel Frame ID
   */
  int LroWideAngleCamera::CkFrameId() const {
    return (-85000);
  }



  /**
   * CK Reference ID - J2000
   *
   * @return @b int The appropriate instrument code for the "Camera-matrix"
   *         Kernel Reference ID
   */
  int LroWideAngleCamera::CkReferenceId() const {
    return (1);
  }



  /**
   *  SPK Reference ID - J2000
   *
   * @return @b int The appropriate instrument code for the Spacecraft
   *         Kernel Reference ID
   */
  int LroWideAngleCamera::SpkReferenceId() const {
    return (1);
  }
}


// Plugin
/**
 * This is the function that is called in order to instantiate a
 * LroWideAngleCamera object.
 *
 * @param lab Cube labels
 *
 * @return Isis::Camera* LroWideAngleCamera
 * @internal
 *   @history 2011-05-03 Jeannie Walldren - Removed Lro namespace.
 */
extern "C" Isis::Camera *LroWideAngleCameraPlugin(Isis::Cube &cube) {
  return new Isis::LroWideAngleCamera(cube);
}
