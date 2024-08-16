/** This is free and unencumbered software released into the public domain.
The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */
#include "Spice.h"

#include <cfloat>
#include <iomanip>

#include <QDebug>
#include <QVector>

#include <getSpkAbCorrState.hpp>

#include <ale/Load.h>

#include <nlohmann/json.hpp>
using json = nlohmann::json;


#include "Constants.h"
#include "Distance.h"
#include "EllipsoidShape.h"
#include "EndianSwapper.h"
#include "FileName.h"
#include "IException.h"
#include "IString.h"
#include "iTime.h"
#include "Longitude.h"
#include "LightTimeCorrectionState.h"
#include "NaifStatus.h"
#include "ShapeModel.h"
#include "SpacecraftPosition.h"
#include "Target.h"
#include "Blob.h"

using namespace std;

namespace Isis {
  /**
   * Constructs a Spice object and loads SPICE kernels using information from the
   * label object. The constructor expects an Instrument and Kernels group to be
   * in the labels.
   *
   * @param lab Label containing Instrument and Kernels groups.
   *
   * @internal
   * @history 2005-10-07 Jim Torson  -   Modified the constructor so it can
   *                                      handle multiple SpacecraftPosition and
   *                                      multiple SpacecraftPointing kernel files
   * @history 2005-11-29 Debbie A. Cook - Added loop to allow multiple frames
   *                                      kernels and code to initialize Naif
   *                                      codes when no platform is used (landers)
   * @history 2006-01-03 Debbie A. Cook - Added loop to allow multiple spacecraft
   *                                      clock kernels (for Viking)
   * @history 2006-02-21 Jeff Anderson/Debbie Cook - Refactor to use SpicePosition
   *                                                 and SpiceRotation classes
   * @history 2009-03-18 Tracie Sucharski - Remove code for old keywords.
   */

  // TODO: DOCUMENT EVERYTHING
  Spice::Spice(Cube &cube) {
    Pvl &lab = *cube.label();
    if (cube.hasBlob("CSMState", "String")) {
      csmInit(cube, lab);
    }
    else {
      PvlGroup kernels = lab.findGroup("Kernels", Pvl::Traverse);
      bool hasTables = (kernels["TargetPosition"][0] == "Table");
      // BONUS TODO: update to pull out separate init methods
      init(lab, !hasTables);
    }
  }


  /**
   * Constructs a Spice Object
   *
   * @param lab Isis Cube Pvl Lavel
   * @param isd ALE Json ISD
   */
  Spice::Spice(Pvl &lab, json isd) {
    init(lab, true, isd);
  }


  /**
   * Initialize the Spice object for a CSMCamera.
   * This sets up the Spice/Sensor/Camera object to not have any SPICE dependent
   * members initialized.
   *
   * @param cube The Cube containing image data for the camera
   * @param label The label containing information for the camera
   */
  void Spice::csmInit(Cube &cube, Pvl label) {
    defaultInit();
    m_target = new Target;
    NaifStatus::CheckErrors();
  }


  /**
   * Default initialize the members of the SPICE object.
   */
  void Spice::defaultInit() {
    m_solarLongitude = new Longitude;

    m_et = nullptr;
    m_kernels = new QVector<QString>;

    m_startTime = new iTime;
    m_endTime = new iTime;
    m_cacheSize = new SpiceDouble;
    *m_cacheSize = 0;

    m_startTimePadding = new SpiceDouble;
    *m_startTimePadding = 0;
    m_endTimePadding = new SpiceDouble;
    *m_endTimePadding = 0;

    m_instrumentPosition = nullptr;
    m_instrumentRotation = nullptr;

    m_sunPosition = nullptr;
    m_bodyRotation = nullptr;

    m_allowDownsizing = false;

    m_spkCode = nullptr;
    m_ckCode = nullptr;
    m_ikCode = nullptr;
    m_sclkCode = nullptr;
    m_spkBodyCode = nullptr;
    m_bodyFrameCode = nullptr;
    m_target = nullptr;
  }


  /**
   * Initialization of Spice object.
   *
   * @param lab  Pvl labels
   * @param noTables Indicates the use of tables.
   *
   * @throw Isis::IException::Io - "Can not find NAIF code for NAIF target"
   * @throw Isis::IException::Camera - "No camera pointing available"
   * @throw Isis::IException::Camera - "No instrument position available"
   *
   * @internal
   *   @history 2011-02-08 Jeannie Walldren - Initialize pointers to null.
   */
  void Spice::init(Pvl &lab, bool noTables, json isd) {
    NaifStatus::CheckErrors();
    // Initialize members
    defaultInit();

    m_spkCode = new SpiceInt;
    m_ckCode = new SpiceInt;
    m_ikCode = new SpiceInt;
    m_sclkCode = new SpiceInt;
    m_spkBodyCode = new SpiceInt;
    m_bodyFrameCode = new SpiceInt;

    m_naifKeywords = new PvlObject("NaifKeywords");
    // m_sky = false;

    // Get the kernel group and load main kernels
    PvlGroup kernels = lab.findGroup("Kernels", Pvl::Traverse);

    // Get the time padding first
    if (kernels.hasKeyword("StartPadding")) {
      *m_startTimePadding = std::stod(kernels["StartPadding"][0]);
    }
    else {
      *m_startTimePadding = 0.0;
    }

    if (kernels.hasKeyword("EndPadding")) {
      *m_endTimePadding  = std::stod(kernels["EndPadding"][0]);
    }
    else {
      *m_endTimePadding = 0.0;
    }

    // We should remove this completely in the near future
    m_usingNaif = !lab.hasObject("NaifKeywords") || noTables;
    m_usingAle = false;


    //  Modified  to load planetary ephemeris SPKs before s/c SPKs since some
    //  missions (e.g., MESSENGER) may augment the s/c SPK with new planet
    //  ephemerides. (2008-02-27 (KJB))
    if (m_usingNaif) {
      try {
        if (isd == NULL) {
          // try using ALE
          std::ostringstream kernel_pvl;
          kernel_pvl << kernels;

          json props;
          if (QString::fromStdString(kernels["InstrumentPointing"][0]).toUpper() == "NADIR") {
            props["nadir"] = true;
          }

          props["kernels"] = kernel_pvl.str();

          isd = ale::load(lab.fileName(), props.dump(), "ale", false, false, true);
        }

        json aleNaifKeywords = isd["naif_keywords"];
        m_naifKeywords = new PvlObject("NaifKeywords", aleNaifKeywords);

        // Still need to load clock kernels for now
        load(kernels["LeapSecond"], noTables);
        if ( kernels.hasKeyword("SpacecraftClock")) {
          load(kernels["SpacecraftClock"], noTables);
        }
        m_usingAle = true;
      }
      catch(...) {
        // Backup to standard ISIS implementation
        if (noTables) {
          load(kernels["TargetPosition"], noTables);
          load(kernels["InstrumentPosition"], noTables);
          load(kernels["InstrumentPointing"], noTables);
        }

        if (kernels.hasKeyword("Frame")) {
          load(kernels["Frame"], noTables);
        }

        load(kernels["TargetAttitudeShape"], noTables);
        if (kernels.hasKeyword("Instrument")) {
          load(kernels["Instrument"], noTables);
        }
        // Always load after instrument
        if (kernels.hasKeyword("InstrumentAddendum")) {
          load(kernels["InstrumentAddendum"], noTables);
        }

        // Still need to load clock kernels for now
        load(kernels["LeapSecond"], noTables);
        if ( kernels.hasKeyword("SpacecraftClock")) {
          load(kernels["SpacecraftClock"], noTables);
        }

        // Modified to load extra kernels last to allow overriding default values
        // (2010-04-07) (DAC)
        if (kernels.hasKeyword("Extra")) {
          load(kernels["Extra"], noTables);
        }
      }

      // Moved the construction of the Target after the NAIF kenels have been loaded or the
      // NAIF keywords have been pulled from the cube labels, so we can find target body codes
      // that are defined in kernels and not just body codes build into spicelib
      // TODO: Move this below the else once the rings code below has been refactored
      m_target = new Target(this, lab);

      // This should not be here. Consider having spiceinit add the necessary rings kernels to the
      // Extra parameter if the user has set the shape model to RingPlane.
      // If Target is Saturn and ShapeModel is RingPlane, load the extra rings pck file
      //  which changes the prime meridian values to report longitudes with respect to
      // the ascending node of the ringplane.
      if (m_target->name().toUpper() == "SATURN" && m_target->shape()->name().toUpper() == "PLANE") {
        PvlKeyword ringPck = PvlKeyword("RingPCK","$cassini/kernels/pck/saturnRings_v001.tpc");
        load(ringPck, noTables);
      }
    }
    else {
      *m_naifKeywords = lab.findObject("NaifKeywords");

      // Moved the construction of the Target after the NAIF kenels have been loaded or the
      // NAIF keywords have been pulled from the cube labels, so we can find target body codes
      // that are defined in kernels and not just body codes build into spicelib
      // TODO: Move this below the else once the rings code above has been refactored

      m_target = new Target(this, lab);
    }

    // Get NAIF ik, spk, sclk, and ck codes
    //
    //    Use ikcode to get parameters from instrument kernel such as focal
    //    length, distortions, focal plane maps, etc
    //
    //    Use spkcode to get spacecraft position from spk file
    //
    //    Use sclkcode to transform times from et to tics
    //
    //    Use ckcode to transform between frames
    //
    //    Use bodycode to obtain radii and attitude (pole position/omega0)
    //
    //    Use spkbodycode to read body position from spk

    QString trykey = "NaifIkCode";
    if (kernels.hasKeyword("NaifFrameCode")) trykey = "NaifFrameCode";
    *m_ikCode = std::stoi(kernels[trykey.toStdString()][0]);

    *m_spkCode  = *m_ikCode / 1000;
    *m_sclkCode = *m_spkCode;
    *m_ckCode   = *m_ikCode;

    if (!m_target->isSky()) {
      // Get target body code and radii and store them in the Naif group
      // DAC modified to look for and store body code so that the radii keyword name
      // will be able to be constructed even for new bodies not in the standard PCK yet.
      QString radiiKey = "BODY" + Isis::toString(m_target->naifBodyCode()) + "_RADII";
      QVariant result = m_target->naifBodyCode();
      storeValue("BODY_CODE", 0, SpiceIntType, result);
      std::vector<Distance> radii(3,Distance());
      radii[0] = Distance(getDouble(radiiKey, 0), Distance::Kilometers);
      radii[1] = Distance(getDouble(radiiKey, 1), Distance::Kilometers);
      radii[2] = Distance(getDouble(radiiKey, 2), Distance::Kilometers);
      // m_target doesn't have the getDouble method so Spice gets the radii for it
      m_target->setRadii(radii);
    }

    *m_spkBodyCode = m_target->naifBodyCode();

    // Override them if they exist in the labels
    if (kernels.hasKeyword("NaifSpkCode")) {
      *m_spkCode = (int) kernels["NaifSpkCode"];
    }

    if (kernels.hasKeyword("NaifCkCode")) {
      *m_ckCode = (int) kernels["NaifCkCode"];
    }

    if (kernels.hasKeyword("NaifSclkCode")) {
      *m_sclkCode = (int) kernels["NaifSclkCode"];
    }

    if (!m_target->isSky()) {
      if (kernels.hasKeyword("NaifSpkBodyCode")) {
        *m_spkBodyCode = (int) kernels["NaifSpkBodyCode"];
      }
    }

    if (m_target->isSky()) {
      // Create the identity rotation for sky targets
      // Everything in bodyfixed will really be J2000
      m_bodyRotation = new SpiceRotation(1);
    }
    else {
      // JAA - Modified to store and look for the frame body code in the cube labels
      SpiceInt frameCode;
      if (((m_usingNaif) || (!m_naifKeywords->hasKeyword("BODY_FRAME_CODE"))) && !isUsingAle()) {
        char frameName[32];
        SpiceBoolean found;
        cidfrm_c(*m_spkBodyCode, sizeof(frameName), &frameCode, frameName, &found);

        if (!found) {
          QString naifTarget = "IAU_" + m_target->name().toUpper();
          namfrm_c(naifTarget.toLatin1().data(), &frameCode);
          if (frameCode == 0) {
            QString msg = "Can not find NAIF BODY_FRAME_CODE for target ["
                         + m_target->name() + "]";
            throw IException(IException::Io, msg, _FILEINFO_);
          }
        }

        QVariant result = (int)frameCode;
        storeValue("BODY_FRAME_CODE", 0, SpiceIntType, result);
      }
      else {
        try {
          frameCode = getInteger("BODY_FRAME_CODE", 0);
        }
        catch(IException &e) {
          std::string msg = "Unable to read BODY_FRAME_CODE from naifkeywords group";
          throw IException(IException::Io, msg, _FILEINFO_);
        }
      }

      m_bodyRotation = new SpiceRotation(frameCode);
      *m_bodyFrameCode = frameCode;
    }

    m_instrumentRotation = new SpiceRotation(*m_ckCode);

    //  Set up for observer/target and light time correction to between s/c
    // and target body.
    LightTimeCorrectionState ltState(*m_ikCode, this);
    ltState.checkSpkKernelsForAberrationCorrection();

    vector<Distance> radius = m_target->radii();
    Distance targetRadius((radius[0] + radius[2])/2.0);
    m_instrumentPosition = new SpacecraftPosition(*m_spkCode, *m_spkBodyCode,
                                                  ltState, targetRadius);

    m_sunPosition = new SpicePosition(10, m_target->naifBodyCode());


    // Check to see if we have nadir pointing that needs to be computed &
    // See if we have table blobs to load
    if (m_usingAle) {
      m_sunPosition->LoadCache(isd["sun_position"]);
      if (m_sunPosition->cacheSize() > 3) {
        m_sunPosition->Memcache2HermiteCache(0.01);
      }
      m_bodyRotation->LoadCache(isd["body_rotation"]);
      m_bodyRotation->MinimizeCache(SpiceRotation::DownsizeStatus::Yes);
      if (m_bodyRotation->cacheSize() > 5) {
        m_bodyRotation->LoadTimeCache();
      }
      solarLongitude();
    }
    else if (QString::fromStdString(kernels["TargetPosition"][0]).toUpper() == "TABLE") {
      Table t("SunPosition", QString::fromStdString(lab.fileName()), lab);
      m_sunPosition->LoadCache(t);

      Table t2("BodyRotation", QString::fromStdString(lab.fileName()), lab);
      m_bodyRotation->LoadCache(t2);
      if (t2.Label().hasKeyword("SolarLongitude")) {
        *m_solarLongitude = Longitude(t2.Label()["SolarLongitude"],
            Angle::Degrees);
      }
      else {
        solarLongitude();
      }
    }

    //  We can't assume InstrumentPointing & InstrumentPosition exist, old
    //  files may be around with the old keywords, SpacecraftPointing &
    //  SpacecraftPosition.  The old keywords were in existance before the
    //  Table option, so we don't need to check for Table under the old
    //  keywords.

    if (kernels["InstrumentPointing"].size() == 0) {
      throw IException(IException::Unknown,
                       "No camera pointing available",
                       _FILEINFO_);
    }

    //  2009-03-18  Tracie Sucharski - Removed test for old keywords, any files
    // with the old keywords should be re-run through spiceinit.
    if (QString::fromStdString(kernels["InstrumentPointing"][0]).toUpper() == "NADIR" && !isUsingAle()) {
      if (m_instrumentRotation) {
        delete m_instrumentRotation;
        m_instrumentRotation = NULL;
      }

      m_instrumentRotation = new SpiceRotation(*m_ikCode, *m_spkBodyCode);
    }
    else if (m_usingAle) {
     m_instrumentRotation->LoadCache(isd["instrument_pointing"]);
     m_instrumentRotation->MinimizeCache(SpiceRotation::DownsizeStatus::Yes);
     if (m_instrumentRotation->cacheSize() > 5) {
       m_instrumentRotation->LoadTimeCache();
     }
    }
    else if (QString::fromStdString(kernels["InstrumentPointing"][0]).toUpper() == "TABLE") {
      Table t("InstrumentPointing", QString::fromStdString(lab.fileName()), lab);
      m_instrumentRotation->LoadCache(t);
    }


    if (kernels["InstrumentPosition"].size() == 0) {
      throw IException(IException::Unknown,
                       "No instrument position available",
                       _FILEINFO_);
    }

    if (m_usingAle) {
      m_instrumentPosition->LoadCache(isd["instrument_position"]);
    }
    else if (QString::fromStdString(kernels["InstrumentPosition"][0]).toUpper() == "TABLE") {
      Table t("InstrumentPosition", QString::fromStdString(lab.fileName()), lab);
      m_instrumentPosition->LoadCache(t);
    }
    NaifStatus::CheckErrors();
  }


  /**
   * Loads/furnishes NAIF kernel(s)
   *
   * @param key PvlKeyword
   * @param noTables Indicates the use of tables.
   *
   * @throw Isis::IException::Io - "Spice file does not exist."
   */
  void Spice::load(PvlKeyword &key, bool noTables) {
    NaifStatus::CheckErrors();

    for (int i = 0; i < key.size(); i++) {
      if (key[i] == "") continue;
      if (QString::fromStdString(key[i]).toUpper() == "NULL") break;
      if (QString::fromStdString(key[i]).toUpper() == "NADIR") break;
      if (QString::fromStdString(key[i]).toUpper() == "TABLE" && !noTables) break;
      if (QString::fromStdString(key[i]).toUpper() == "TABLE" && noTables) continue;
      FileName file(QString::fromStdString(key[i]));
      if (!file.fileExists()) {
        QString msg = "Spice file does not exist [" + file.expanded() + "]";
        throw IException(IException::Io, msg, _FILEINFO_);
      }
      QString fileName = file.expanded();
      furnsh_c(fileName.toLatin1().data());
      m_kernels->push_back(QString::fromStdString(key[i]));
    }

    NaifStatus::CheckErrors();
  }

  /**
   * Destroys the Spice object
   */
  Spice::~Spice() {
    NaifStatus::CheckErrors();

    if (m_solarLongitude != NULL) {
      delete m_solarLongitude;
      m_solarLongitude = NULL;
    }

    if (m_et != NULL) {
      delete m_et;
      m_et = NULL;
    }

    if (m_startTime != NULL) {
      delete m_startTime;
      m_startTime = NULL;
    }

    if (m_endTime != NULL) {
      delete m_endTime;
      m_endTime = NULL;
    }

    if (m_cacheSize != NULL) {
      delete m_cacheSize;
      m_cacheSize = NULL;
    }

    if (m_startTimePadding != NULL) {
      delete m_startTimePadding;
      m_startTimePadding = NULL;
    }

    if (m_endTimePadding != NULL) {
      delete m_endTimePadding;
      m_endTimePadding = NULL;
    }

    if (m_instrumentPosition != NULL) {
      delete m_instrumentPosition;
      m_instrumentPosition = NULL;
    }

    if (m_instrumentRotation != NULL) {
      delete m_instrumentRotation;
      m_instrumentRotation = NULL;
    }

    if (m_sunPosition != NULL) {
      delete m_sunPosition;
      m_sunPosition = NULL;
    }

    if (m_bodyRotation != NULL) {
      delete m_bodyRotation;
      m_bodyRotation = NULL;
    }

    if (m_spkCode != NULL) {
      delete m_spkCode;
      m_spkCode = NULL;
    }

    if (m_ckCode != NULL) {
      delete m_ckCode;
      m_ckCode = NULL;
    }

    if (m_ikCode != NULL) {
      delete m_ikCode;
      m_ikCode = NULL;
    }

    if (m_sclkCode != NULL) {
      delete m_sclkCode;
      m_sclkCode = NULL;
    }

    if (m_spkBodyCode != NULL) {
      delete m_spkBodyCode;
      m_spkBodyCode = NULL;
    }

    if (m_bodyFrameCode != NULL) {
      delete m_bodyFrameCode;
      m_bodyFrameCode = NULL;
    }

    if (m_target != NULL) {
      delete m_target;
      m_target = NULL;
    }

    // Unload the kernels (TODO: Can this be done faster)
    for (int i = 0; m_kernels && i < m_kernels->size(); i++) {
      FileName file(m_kernels->at(i));
      QString fileName = file.expanded();
      unload_c(fileName.toLatin1().data());
    }

    if (m_kernels != NULL) {
      delete m_kernels;
      m_kernels = NULL;
    }
    NaifStatus::CheckErrors();
  }

  /**
   * This method creates an internal cache of spacecraft and sun positions over a
   * specified time range. The SPICE kernels are then immediately unloaded. This
   * allows multiple instances of the Spice object to be created as the NAIF
   * toolkit can clash if multiple sets of SPICE kernels are loaded. Note that
   * the cache size is specified as an argument. Therefore, times requested via
   * setTime() which are not directly loaded in the cache will be interpolated.
   * If the instrument position is not cached and cacheSize is greater than 3,
   * the tolerance is passed to the SpicePosition Memcache2HermiteCache()
   * method.
   *
   * @b Note:  Before this method is called, the private variables m_cacheSize,
   * m_startTime and m_endTime must be set.  This is done in the Camera classes
   * using the methods SetCacheSize() and SetStartEndEphemerisTime().
   *
   * @param startTime Starting ephemeris time to cache
   * @param endTime Ending ephemeris time to cache
   * @param size Size of the cache.
   * @param tol Tolerance.
   *
   * @throw Isis::IException::Programmer - "Argument cacheSize must be greater
   *        than zero"
   * @throw Isis::IException::Programmer - "Argument startTime must be less than
   *        or equal to endTime"
   * @throw Isis::IException::User - "This instrument does not support time
   *             padding"
   *
   * @internal
   * @history 2011-04-10 Debbie A. Cook - Updated to only create cache for
   *          instrumentPosition if type is Spice.
   */
  void Spice::createCache(iTime startTime, iTime endTime,
      int cacheSize, double tol) {
    NaifStatus::CheckErrors();

    // Check for errors
    if (cacheSize <= 0) {
      std::string msg = "Argument cacheSize must be greater than zero";
      throw IException(IException::Programmer, msg, _FILEINFO_);
    }

    if (startTime > endTime) {
      std::string msg = "Argument startTime must be less than or equal to endTime";
      throw IException(IException::Programmer, msg, _FILEINFO_);
    }

    if (*m_cacheSize > 0) {
      std::string msg = "A cache has already been created";
      throw IException(IException::Programmer, msg, _FILEINFO_);
    }

    if (cacheSize == 1 && (*m_startTimePadding != 0 || *m_endTimePadding != 0)) {
      std::string msg = "This instrument does not support time padding";
      throw IException(IException::User, msg, _FILEINFO_);
    }

    string abcorr;
    if (getSpkAbCorrState(abcorr)) {
      instrumentPosition()->SetAberrationCorrection("NONE");
    }

    iTime avgTime((startTime.Et() + endTime.Et()) / 2.0);
    computeSolarLongitude(avgTime);

    // Cache everything
    if (!m_bodyRotation->IsCached()) {
      int bodyRotationCacheSize = cacheSize;
      if (cacheSize > 2) bodyRotationCacheSize = 2;
      m_bodyRotation->LoadCache(
          startTime.Et() - *m_startTimePadding,
          endTime.Et() + *m_endTimePadding,
          bodyRotationCacheSize);
    }

    if (m_instrumentRotation->GetSource() < SpiceRotation::Memcache) {
      if (cacheSize > 3) m_instrumentRotation->MinimizeCache(SpiceRotation::Yes);
      m_instrumentRotation->LoadCache(
          startTime.Et() - *m_startTimePadding,
          endTime.Et() + *m_endTimePadding,
          cacheSize);
    }

    if (m_instrumentPosition->GetSource() < SpicePosition::Memcache) {
      m_instrumentPosition->LoadCache(
          startTime.Et() - *m_startTimePadding,
          endTime.Et() + *m_endTimePadding,
          cacheSize);
      if (cacheSize > 3) m_instrumentPosition->Memcache2HermiteCache(tol);
    }
    else if (m_instrumentPosition->GetSource() == SpicePosition::Memcache && m_instrumentPosition->HasVelocity() && isUsingAle()) {
      int aleCacheSize = m_instrumentPosition->cacheSize();
      if (aleCacheSize > 3) {
        m_instrumentPosition->Memcache2HermiteCache(tol);
      }
    }
    

    if (!m_sunPosition->IsCached()) {
      int sunPositionCacheSize = cacheSize;
      if (cacheSize > 2) sunPositionCacheSize = 2;
      m_sunPosition->LoadCache(
          startTime.Et() - *m_startTimePadding,
          endTime.Et() + *m_endTimePadding,
          sunPositionCacheSize);
    }

    // Save the time and cache size
    *m_startTime = startTime;
    *m_endTime = endTime;
    *m_cacheSize = cacheSize;
    m_et = NULL;

    // Unload the kernels (TODO: Can this be done faster)
    for (int i = 0; i < m_kernels->size(); i++) {
      FileName file(m_kernels->at(i));
      QString fileName = file.expanded();
      unload_c(fileName.toLatin1().data());
    }

    m_kernels->clear();

    NaifStatus::CheckErrors();
  }


  /**
   * Accessor method for the cache start time.
   * @return @b iTime Start time for the image.
   * @author Steven Lambright
   * @internal
   *   @history 2011-02-09 Steven Lambright - Original version.
   */
  iTime Spice::cacheStartTime() const {
    if (m_startTime) {
      return *m_startTime;
    }

    return iTime();
  }

  /**
   * Accessor method for the cache end time.
   * @return @b iTime End time for the image.
   * @author Steven Lambright
   * @internal
   *   @history 2011-02-09 Steven Lambright - Original version.
   */
  iTime Spice::cacheEndTime() const {
    if (m_endTime) {
      return *m_endTime;
    }

    return iTime();
  }

  /**
   * Sets the ephemeris time and reads the spacecraft and sun position from the
   * kernels at that instant in time.
   *
   * @param et Ephemeris time (read NAIF documentation for a detailed
   *           description)
   *
   * @see http://naif.jpl.nasa.gov/naif/
   * @internal
   *   @history 2005-11-29 Debbie A. Cook - Added alternate code for processing
   *                                        instruments without a platform
   *   @history 2011-02-09 Steven Lambright - Changed name from
   *                                        SetEphemerisTime()
   */
  void Spice::setTime(const iTime &et) {

    if (m_et == NULL) {
      m_et = new iTime();

      // Before the Spice is cached, but after the camera aberration correction
      // is set, check to see if the instrument position kernel was created
      // by spkwriter.  If so turn off aberration corrections because the camera
      // set aberration corrections are included in the spk already.
      string abcorr;
      if (*m_cacheSize == 0) {
        if (m_startTime->Et() == 0.0 && m_endTime->Et() == 0.0
            && getSpkAbCorrState(abcorr)) {
          instrumentPosition()->SetAberrationCorrection("NONE");
        }
      }
    }

    *m_et = et;

    m_bodyRotation->SetEphemerisTime(et.Et());
    m_instrumentRotation->SetEphemerisTime(et.Et());
    m_instrumentPosition->SetEphemerisTime(et.Et());
    m_sunPosition->SetEphemerisTime(et.Et());

    std::vector<double> uB = m_bodyRotation->ReferenceVector(m_sunPosition->Coordinate());
    m_uB[0] = uB[0];
    m_uB[1] = uB[1];
    m_uB[2] = uB[2];

    computeSolarLongitude(*m_et);
  }

  /**
   * Returns the spacecraft position in body-fixed frame km units.
   *
   * @param p[] Spacecraft position
   *
   * @see setTime()
   *
   * @throw Isis::iException::Programmer - "You must call SetTime first"
   */
  void Spice::instrumentPosition(double p[3]) const {
    instrumentBodyFixedPosition(p);
  }

  /**
   * Returns the spacecraft position in body-fixed frame km units.
   *
   * @param p[] Spacecraft position
   *
   * @see setTime()
   *
   * @throw Isis::iException::Programmer - "You must call SetTime first"
   */
  void Spice::instrumentBodyFixedPosition(double p[3]) const {
    if (m_et == NULL) {
      std::string msg = "Unable to retrieve instrument's body fixed position."
                    " Spice::SetTime must be called first.";
      throw IException(IException::Programmer, msg, _FILEINFO_);
    }

    std::vector<double> sB = m_bodyRotation->ReferenceVector(m_instrumentPosition->Coordinate());
    p[0] = sB[0];
    p[1] = sB[1];
    p[2] = sB[2];
  }

  /**
   * Returns the spacecraft velocity in body-fixed frame km/sec units.
   *
   * @param v[] Spacecraft velocity
   */
  void Spice::instrumentBodyFixedVelocity(double v[3]) const {
    if (m_et == NULL) {
      std::string msg = "Unable to retrieve instrument's body fixed velocity."
                    " Spice::SetTime must be called first.";
      throw IException(IException::Programmer, msg, _FILEINFO_);
    }

    std::vector<double> state;
    state.push_back(m_instrumentPosition->Coordinate()[0]);
    state.push_back(m_instrumentPosition->Coordinate()[1]);
    state.push_back(m_instrumentPosition->Coordinate()[2]);
    state.push_back(m_instrumentPosition->Velocity()[0]);
    state.push_back(m_instrumentPosition->Velocity()[1]);
    state.push_back(m_instrumentPosition->Velocity()[2]);

    std::vector<double> vB = m_bodyRotation->ReferenceVector(state);
    v[0] = vB[3];
    v[1] = vB[4];
    v[2] = vB[5];
  }


  /**
    * Returns the ephemeris time in seconds which was used to obtain the
    * spacecraft and sun positions.
    *
    * @return @b iTime the currently set ephemeris time
    *
    * @throws IException::Programmer "Unable to retrieve the time Spice::setTime must be called
    *         first."
    */
  iTime Spice::time() const {
    if (m_et == NULL) {
      std::string msg = "Unable to retrieve the time."
                    " Spice::SetTime must be called first.";
      throw IException(IException::Programmer, msg, _FILEINFO_);
    }
    return *m_et;
  }


  /**
   * Fills the input vector with sun position information, in either body-fixed
   * or J2000 reference frame and km units.
   *
   * @param p[] Sun position
   *
   * @see setTime()
   */
  void Spice::sunPosition(double p[3]) const {
    if (m_et == NULL) {
      std::string msg = "Unable to retrieve sun's position."
                    " Spice::SetTime must be called first.";
      throw IException(IException::Programmer, msg, _FILEINFO_);
    }
    p[0] = m_uB[0];
    p[1] = m_uB[1];
    p[2] = m_uB[2];
  }

  /**
   * Calculates and returns the distance from the spacecraft to the target center
   *
   * @return double Distance to the center of the target from the spacecraft
   */
  double Spice::targetCenterDistance() const {
    std::vector<double> sB = m_bodyRotation->ReferenceVector(m_instrumentPosition->Coordinate());
    return sqrt(pow(sB[0], 2) + pow(sB[1], 2) + pow(sB[2], 2));
  }

  /**
   * Returns the radii of the body in km. The radii are obtained from the
   * appropriate SPICE kernel for the body specified by TargetName in the
   * Instrument group of the labels.
   *
   * @param r[] Radii of the target in kilometers
   */
  void Spice::radii(Distance r[3]) const {
    for (int i = 0; i < 3; i++)
       r[i] =m_target->radii()[i];
  }

  /**
   * This returns the NAIF body code of the target indicated in the labels.
   *
   * @return @b SpiceInt NAIF body code
   *
   */
  SpiceInt Spice::naifBodyCode() const {
    return (int) m_target->naifBodyCode();
  }

  /**
   * This returns the NAIF SPK code to use when reading from SPK kernels.
   *
   * @return @b SpiceInt NAIF SPK code
   */
  SpiceInt Spice::naifSpkCode() const {
    return *m_spkCode;
  }

  /**
   * This returns the NAIF CK code to use when reading from CK kernels.
   *
   * @return @b SpiceInt NAIF CK code
   */
  SpiceInt Spice::naifCkCode() const {
    return *m_ckCode;
  }

  /**
   * This returns the NAIF IK code to use when reading from instrument kernels.
   *
   * @return @b SpiceInt NAIF IK code
   */
  SpiceInt Spice::naifIkCode() const {
    return *m_ikCode;
  }

  /**
   * This returns the NAIF SCLK code to use when reading from instrument
   * kernels.
   *
   * @return @b SpiceInt NAIF SCLK code
   */
  SpiceInt Spice::naifSclkCode() const {
    return *m_sclkCode;
  }

  /**
   * This returns the NAIF body frame code. It is read from the labels, if it
   * exists. Otherwise, it's calculated by the init() method.
   *
   * @return @b SpiceInt NAIF body frame code
   *
   */
  SpiceInt Spice::naifBodyFrameCode() const {
    return *m_bodyFrameCode;
  }


  /**
   * This returns the PvlObject that stores all of the requested Naif data
   * and can be a replacement for furnishing text kernels.
   */
  PvlObject Spice::getStoredNaifKeywords() const {
    return *m_naifKeywords;
  }


  /**
   * Virtual method that returns the pixel resolution of the sensor in
   * meters/pix.
   *
   * @return @b double Resolution value of 1.0
   */
  double Spice::resolution() {
    return 1.0;
  };


  /**
   * This returns a value from the NAIF text pool. It is a static convience
   *
   * @param key Name of NAIF keyword to obtain from the pool
   * @param index If the keyword is an array, the element to obtain.
   *              Defaults to 0
   *
   * @return @b SpiceInt Spice integer from NAIF text pool
   *
   * @throw Isis::iException::Io - "Can not find key in instrument kernels
   */
  SpiceInt Spice::getInteger(const QString &key, int index) {
    return readValue(key, SpiceIntType, index).toInt();
  }

  /**
   * This returns a value from the NAIF text pool. It is a static convience method
   *
   * @param key Name of NAIF keyword to obtain from the pool
   * @param index If the keyword is an array, the element to obtain. Defaults to 0
   *
   * @return @b SpiceDouble Spice double from NAIF text pool
   *
   * @throw Isis::iException::Io - "Can not find key in instrument kernels."
   */
  SpiceDouble Spice::getDouble(const QString &key, int index) {
    return readValue(key, SpiceDoubleType, index).toDouble();
  }


  /**
   * This converts the spacecraft clock ticks value (clockValue) to an iTime.
   *
   * If the clock ticks value is provided directly, rather than the spacecraft
   * clock string, set clockTicks=true.
   *
   * Use this when possible because naif calls (such as scs2e_c) cannot be
   *   called when not using naif.
   */
  iTime Spice::getClockTime(QString clockValue, int sclkCode, bool clockTicks) {
    if (sclkCode == -1) {
      sclkCode = naifSclkCode();
    }

    iTime result;

    QString key = "CLOCK_ET_" + Isis::toString(sclkCode) + "_" + clockValue;
    QVariant storedClockTime = getStoredResult(key, SpiceDoubleType);

    if (storedClockTime.isNull()) {
      SpiceDouble timeOutput;
      NaifStatus::CheckErrors();
      if (clockTicks) {
        sct2e_c(sclkCode, (SpiceDouble) clockValue.toDouble(), &timeOutput);
      }
      else {
        scs2e_c(sclkCode, clockValue.toLatin1().data(), &timeOutput);
      }
      NaifStatus::CheckErrors();
      storedClockTime = timeOutput;
      storeResult(key, SpiceDoubleType, timeOutput);
    }

    result = storedClockTime.toDouble();

    return result;
  }


  /**
   * This should be used for reading ALL text naif kernel values. This will
   *   read it from Naif if we're using naif/not attached kernels. If we have
   *   attached kernels and a NaifKeywords label object we will grab it from
   *   there instead. This allows us to not furnish kernels after spiceinit.
   *
   * @param key The naif keyword,value name
   * @param type The naif value's primitive type
   * @param index The index into the naif keyword array to read
   */
  QVariant Spice::readValue(QString key, SpiceValueType type, int index) {
    QVariant result;

    if (m_usingNaif && !m_usingAle) {
      NaifStatus::CheckErrors();

      // This is the success status of the naif call
      SpiceBoolean found = false;

      // Naif tells us how many values were read, but we always just read one.
      //   Use this variable to make naif happy.
      SpiceInt numValuesRead;

      if (type == SpiceDoubleType) {
        SpiceDouble kernelValue;
        gdpool_c(key.toLatin1().data(), (SpiceInt)index, 1,
                 &numValuesRead, &kernelValue, &found);

        if (found && numValuesRead > 0)
          result = kernelValue;
      }
      else if (type == SpiceStringType) {
        char kernelValue[512];
        gcpool_c(key.toLatin1().data(), (SpiceInt)index, 1, sizeof(kernelValue),
                 &numValuesRead, kernelValue, &found);

        if (found && numValuesRead > 0)
          result = kernelValue;
      }
      else if (type == SpiceIntType) {
        SpiceInt kernelValue;
        gipool_c(key.toLatin1().data(), (SpiceInt)index, 1, &numValuesRead,
                 &kernelValue, &found);

        if (found && numValuesRead > 0)
          result = (int)kernelValue;
      }

      if (!found) {
        QString msg = "Can not find [" + key + "] in text kernels";
        throw IException(IException::Io, msg, _FILEINFO_);
      }
      else if (numValuesRead == 0){
        QString msg = "Found " + key + "] in text kernels, but no values were identified and read.";
        throw IException(IException::Io, msg, _FILEINFO_);
      }

      storeValue(key, index, type, result);
      NaifStatus::CheckErrors();
    }
    else {
      // Read from PvlObject that is our naif keywords
      result = readStoredValue(key, type, index);

      if (result.isNull()) {
        QString msg = "The camera is requesting spice data [" + key + "] that "
                      "was not attached, please re-run spiceinit";
        throw IException(IException::Unknown, msg, _FILEINFO_);
      }
    }

    return result;
  }


  void Spice::storeResult(QString name, SpiceValueType type, QVariant value) {
    if (type == SpiceDoubleType) {
      EndianSwapper swapper("LSB");

      double doubleVal = value.toDouble();
      doubleVal = swapper.Double(&doubleVal);
      QByteArray byteCode((char *) &doubleVal, sizeof(double));
      value = byteCode;
      type = SpiceByteCodeType;
    }

    storeValue(name + "_COMPUTED", 0, type, value);
  }


  QVariant Spice::getStoredResult(QString name, SpiceValueType type) {
    bool wasDouble = false;

    if (type == SpiceDoubleType) {
      wasDouble = true;
      type = SpiceByteCodeType;
    }

    QVariant stored = readStoredValue(name + "_COMPUTED", type, 0);

    if (wasDouble && !stored.isNull()) {
      EndianSwapper swapper("LSB");
      double doubleVal = swapper.Double((void *)QByteArray::fromHex(
          stored.toByteArray()).data());
      stored = doubleVal;
    }

    return stored;
  }


  void Spice::storeValue(QString key, int index, SpiceValueType type,
                         QVariant value) {
    if (!m_naifKeywords->hasKeyword(key.toStdString())) {
      m_naifKeywords->addKeyword(PvlKeyword(key.toStdString()));
    }

    PvlKeyword &storedKey = m_naifKeywords->findKeyword(key.toStdString());

    while(index >= storedKey.size()) {
      storedKey.addValue("");
    }

    if (type == SpiceByteCodeType) {
      storedKey[index] = value.toByteArray().toHex().data();
    }
    else if (type == SpiceStringType) {
      storedKey[index] = value.toString().toStdString();
    }
    else if (type == SpiceDoubleType) {
      storedKey[index] = std::to_string(value.toDouble());
    }
    else if (type == SpiceIntType) {
      storedKey[index] = std::to_string(value.toInt());
    }
    else {
      QString msg = "Unable to store variant in labels for key [" + key + "]";
      throw IException(IException::Unknown, msg, _FILEINFO_);
    }
  }


  QVariant Spice::readStoredValue(QString key, SpiceValueType type,
                                  int index) {
    // Read from PvlObject that is our naif keywords
    QVariant result;

    if (m_naifKeywords->hasKeyword(key.toStdString()) && (!m_usingNaif || m_usingAle)) {
      PvlKeyword &storedKeyword = m_naifKeywords->findKeyword(key.toStdString());

      try {
        if (type == SpiceDoubleType) {
          result = std::stod(storedKeyword[index]);
        }
        else if (type == SpiceStringType) {
          result = QString::fromStdString(storedKeyword[index]);
        }
        else if (type == SpiceByteCodeType || SpiceStringType) {
          result = QString::fromStdString(storedKeyword[index]);
        }
        else if (type == SpiceIntType) {
          result = std::stoi(storedKeyword[index]);
        }
      }
      catch(IException &e) {
        e.print();
      }
    }

    return result;
  }

  /**
   * This returns a value from the NAIF text pool. It is a static convience
   * method
   *
   * @param key Name of NAIF keyword to obtain from the pool
   * @param index If the keyword is an array, the element to obtain. Defaults to 0
   *
   * @return @b QString Value from the NAIF text pool
   *
   * @throw Isis::IException::Io - "Can not find key in instrument kernels."
   */
  QString Spice::getString(const QString &key, int index) {
    return readValue(key, SpiceStringType, index).toString();
  }


  /**
   * Returns the sub-spacecraft latitude/longitude in universal coordinates
   * (0-360 positive east, ocentric)
   *
   * @param lat Sub-spacecraft latitude
   *
   * @param lon Sub-spacecraft longitude
   *
   * @see setTime()
   * @throw Isis::IException::Programmer - "You must call SetTime
   *             first."
   */
  void Spice::subSpacecraftPoint(double &lat, double &lon) {
    NaifStatus::CheckErrors();

    if (m_et == NULL) {
      std::string msg = "Unable to retrieve subspacecraft position."
                    " Spice::SetTime must be called first.";
      throw IException(IException::Programmer, msg, _FILEINFO_);
    }

    SpiceDouble usB[3], dist;
    std::vector<double> vsB = m_bodyRotation->ReferenceVector(m_instrumentPosition->Coordinate());
    SpiceDouble sB[3];
    sB[0] = vsB[0];
    sB[1] = vsB[1];
    sB[2] = vsB[2];
    unorm_c(sB, usB, &dist);

    std::vector<Distance> radii = target()->radii();
    SpiceDouble a = radii[0].kilometers();
    SpiceDouble b = radii[1].kilometers();
    SpiceDouble c = radii[2].kilometers();

    SpiceDouble originB[3];
    originB[0] = originB[1] = originB[2] = 0.0;

    SpiceBoolean found;
    SpiceDouble subB[3];

    surfpt_c(originB, usB, a, b, c, subB, &found);

    SpiceDouble mylon, mylat;
    reclat_c(subB, &a, &mylon, &mylat);
    lat = mylat * 180.0 / PI;
    lon = mylon * 180.0 / PI;
    if (lon < 0.0) lon += 360.0;

    NaifStatus::CheckErrors();
  }


  /**
   * Returns the sub-solar latitude/longitude in universal coordinates (0-360
   * positive east, ocentric)
   *
   * @param lat Sub-solar latitude
   * @param lon Sub-solar longitude
   *
   * @see setTime()
   * @throw Isis::IException::Programmer - "You must call SetTime
   *             first."
   */
  void Spice::subSolarPoint(double &lat, double &lon) {
    NaifStatus::CheckErrors();

    if (m_et == NULL) {
      std::string msg = "Unable to retrieve subsolar point."
                    " Spice::SetTime must be called first.";
      throw IException(IException::Programmer, msg, _FILEINFO_);
    }

    SpiceDouble uuB[3], dist;
    unorm_c(m_uB, uuB, &dist);
    std::vector<Distance> radii = target()->radii();

    SpiceDouble a = radii[0].kilometers();
    SpiceDouble b = radii[1].kilometers();
    SpiceDouble c = radii[2].kilometers();

    SpiceDouble originB[3];
    originB[0] = originB[1] = originB[2] = 0.0;

    SpiceBoolean found;
    SpiceDouble subB[3];
    surfpt_c(originB, uuB, a, b, c, subB, &found);

    SpiceDouble mylon, mylat;
    reclat_c(subB, &a, &mylon, &mylat);

    lat = mylat * 180.0 / PI;
    lon = mylon * 180.0 / PI;
    if (lon < 0.0) lon += 360.0;
    NaifStatus::CheckErrors();
  }


  /**
    * Returns a pointer to the target object
    *
    * @return string
    */
  Target *Spice::target() const {
    return m_target;
  }


  /**
    * Returns the QString name of the target
    *
    * @return QString
    */
  QString Spice::targetName() const {
    return m_target->name();
  }


  double Spice::sunToBodyDist() const {
    std::vector<double> sunPosition = m_sunPosition->Coordinate();
    std::vector<double> bodyRotation = m_bodyRotation->Matrix();

    double sunPosFromTarget[3];
    mxv_c(&bodyRotation[0], &sunPosition[0], sunPosFromTarget);

    return vnorm_c(sunPosFromTarget);
  }


  /**
   * Computes the solar longitude for the given ephemeris time.  If the target
   * is sky, the longitude is set to -999.0.
   *
   * @param et Ephemeris time
   */
  void Spice::computeSolarLongitude(iTime et) {
    NaifStatus::CheckErrors();

    if (m_target->isSky()) {
      *m_solarLongitude = Longitude();
      return;
    }

    if (m_usingAle || !m_usingNaif) {
      double og_time = m_bodyRotation->EphemerisTime();
      m_bodyRotation->SetEphemerisTime(et.Et());
      m_sunPosition->SetEphemerisTime(et.Et());

      std::vector<double> bodyRotMat = m_bodyRotation->Matrix();
      std::vector<double> sunPos = m_sunPosition->Coordinate();
      std::vector<double> sunVel = m_sunPosition->Velocity();
      double sunAv[3];

      ucrss_c(&sunPos[0], &sunVel[0], sunAv);

      double npole[3];
      for (int i = 0; i < 3; i++) {
        npole[i] = bodyRotMat[6+i];
      }

      double x[3], y[3], z[3];
      vequ_c(sunAv, z);
      ucrss_c(npole, z, x);
      ucrss_c(z, x, y);

      double trans[3][3];
      for (int i = 0; i < 3; i++) {
        trans[0][i] = x[i];
        trans[1][i] = y[i];
        trans[2][i] = z[i];
      }

      double pos[3];
      mxv_c(trans, &sunPos[0], pos);

      double radius, ls, lat;
      reclat_c(pos, &radius, &ls, &lat);

      *m_solarLongitude = Longitude(ls, Angle::Radians).force360Domain();

      NaifStatus::CheckErrors();
      m_bodyRotation->SetEphemerisTime(og_time);
      m_sunPosition->SetEphemerisTime(og_time);
      return;
    }

    if (m_bodyRotation->IsCached()) return;

    double tipm[3][3], npole[3];
    char frameName[32];
    SpiceInt frameCode;
    SpiceBoolean found;

    cidfrm_c(*m_spkBodyCode, sizeof(frameName), &frameCode, frameName, &found);

    if (found) {
      pxform_c("J2000", frameName, et.Et(), tipm);
    }
    else {
      tipbod_c("J2000", *m_spkBodyCode, et.Et(), tipm);
    }

    for (int i = 0; i < 3; i++) {
      npole[i] = tipm[2][i];
    }

    double state[6], lt;
    spkez_c(*m_spkBodyCode, et.Et(), "J2000", "NONE", 10, state, &lt);

    double uavel[3];
    ucrss_c(state, &state[3], uavel);

    double x[3], y[3], z[3];
    vequ_c(uavel, z);
    ucrss_c(npole, z, x);
    ucrss_c(z, x, y);

    double trans[3][3];
    for (int i = 0; i < 3; i++) {
      trans[0][i] = x[i];
      trans[1][i] = y[i];
      trans[2][i] = z[i];
    }

    spkez_c(10, et.Et(), "J2000", "LT+S", *m_spkBodyCode, state, &lt);

    double pos[3];
    mxv_c(trans, state, pos);

    double radius, ls, lat;
    reclat_c(pos, &radius, &ls, &lat);

    *m_solarLongitude = Longitude(ls, Angle::Radians).force360Domain();

    NaifStatus::CheckErrors();

  }


  /**
   * Returns the solar longitude
   *
   * @return @b double The Solar Longitude
   */
  Longitude Spice::solarLongitude() {
    if (m_et) {
      computeSolarLongitude(*m_et);
      return *m_solarLongitude;
    }

    return Longitude();
  }


  /**
   * Returns true if the kernel group has kernel files
   *
   * @param lab Label containing Instrument and Kernels groups.
   *
   * @return @b bool status of kernel files in the kernel group
   */
  bool Spice::hasKernels(Pvl &lab) {

    // Get the kernel group and check main kernels
    PvlGroup kernels = lab.findGroup("Kernels", Pvl::Traverse);
    std::vector<string> keywords;
    keywords.push_back("TargetPosition");

    if (kernels.hasKeyword("SpacecraftPosition")) {
      keywords.push_back("SpacecraftPosition");
    }
    else {
      keywords.push_back("InstrumentPosition");
    }

    if (kernels.hasKeyword("SpacecraftPointing")) {
      keywords.push_back("SpacecraftPointing");
    }
    else {
      keywords.push_back("InstrumentPointing");
    }

    if (kernels.hasKeyword("Frame")) {
      keywords.push_back("Frame");
    }

    if (kernels.hasKeyword("Extra")) {
      keywords.push_back("Extra");
    }

    PvlKeyword key;
    for (int ikey = 0; ikey < (int) keywords.size(); ikey++) {
      key = kernels[ikey];

      for (int i = 0; i < key.size(); i++) {
        if (key[i] == "") return false;
        if (QString::fromStdString(key[i]).toUpper() == "NULL") return false;
        if (QString::fromStdString(key[i]).toUpper() == "NADIR") return false;
        if (QString::fromStdString(key[i]).toUpper() == "TABLE") return false;
      }
    }
    return true;
  }


  /**
   * Returns true if time has been initialized.
   *
   * @author 2016-10-19 Kristin Berry
   *
   * @return @b bool true if time has been set
   */
  bool Spice::isTimeSet(){
    return !(m_et == NULL);
  }


  /**
   * Accessor method for the sun position.
   * @return @b iTime Sun position for the image.
   * @author Steven Lambright
   * @internal
   *   @history 2011-02-09 Steven Lambright - Original version.
   */
  SpicePosition *Spice::sunPosition() const {
    return m_sunPosition;
  }

  /**
   * Accessor method for the instrument position.
   * @return @b iTime Instrument position for the image.
   * @author Steven Lambright
   * @internal
   *   @history 2011-02-09 Steven Lambright - Original version.
   */
  SpicePosition *Spice::instrumentPosition() const {
    return m_instrumentPosition;
  }

  /**
   * Accessor method for the body rotation.
   * @return @b iTime Body rotation for the image.
   * @author Steven Lambright
   * @internal
   *   @history 2011-02-09 Steven Lambright - Original version.
   */
  SpiceRotation *Spice::bodyRotation() const {
    return m_bodyRotation;
  }

  /**
   * Accessor method for the instrument rotation.
   * @return @b iTime Instrument rotation for the image.
   * @author Steven Lambright
   * @internal
   *   @history 2011-02-09 Steven Lambright - Original version.
   */
  SpiceRotation *Spice::instrumentRotation() const {
    return m_instrumentRotation;
  }

  bool Spice::isUsingAle(){
    return m_usingAle;
  }
}
