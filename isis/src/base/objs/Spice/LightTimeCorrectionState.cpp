/** This is free and unencumbered software released into the public domain.
The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */
#include "LightTimeCorrectionState.h"

#include <cfloat>
#include <iomanip>

#include <string>
#include <vector>

#include <QString>

#include <SpiceZdf.h>

#include "IException.h"
#include "IString.h"
#include "Kernels.h"
#include "Pvl.h"
#include "PvlGroup.h"
#include "NaifStatus.h"
#include "Spice.h"

namespace Isis {

/**
 * @brief Default constructor sets default state of light time corrections 
 *  
 * The default state of the stellar aberration correction, swap of 
 * observer/target and correction of light time to the surface of a target 
 * body are set here.  The current state of these conditions are set to 
 * preserve preexisting behavior. 
 *  
 * Initial state of these conditions are "LT+S" for stellar aberration, false 
 * for observer/target swap and no light time to surface correction. 
 * 
 * @author 2012-10-28 Kris Becker
 */
  LightTimeCorrectionState::LightTimeCorrectionState() {
    setDefaultState();
  }


 /**
  * @brief Constructor that gathers state of light time correction
  *  
  * This constructor checks observer/target swap and light time correction 
  * states for an instrument indicated by the ikCode parameter (assumed to be a 
  * valid NAIF instrument code).  The Spice object is required so these values 
  * are properly recorded (in the label) for subsequent use. 
  *  
  * @author 2012-10-11 Kris Becker
  * 
  * @param ikCode NAIF code for instrument
  * @param spice  Spice object associated with geometry
  */
  LightTimeCorrectionState::LightTimeCorrectionState(int ikCode,
                                                     Spice *spice) {
    setDefaultState();
    checkObserverTargetSwap(ikCode, spice);
    checkAberrationCorrection(ikCode, spice);
    checkLightTimeToSurfaceCorrect(ikCode, spice);
  }


/** 
 * @brief Compare two instances of the LightTimeCorrectionState objects
 * 
 * @author Kris Becker - 10/28/2012
 * 
 * @param state Other state to compare to
 * 
 * @return bool false if all values aren't the same, true otherwise.
 */
  bool LightTimeCorrectionState::operator==(const LightTimeCorrectionState &state) 
                                            const {
    if (getAberrationCorrection() != state.getAberrationCorrection()) {
      return (false);
    }
    if (isObserverTargetSwapped() != state.isObserverTargetSwapped()) {
      return (false);
    }
    if (isLightTimeToSurfaceCorrected() != state.isLightTimeToSurfaceCorrected()) {
      return (false);
    }
    return (true);
  }

/**
 * @brief Apply instrument (team) specific light time/stellar aborration option
 *
 * This method checks for the value of the INS-XXXXXX_LIGHTTIME_CORRECTION
 * kernel pool keyword to determine the value (if specified) of the light time
 * and stellar aborration correction parameter provided to NAIF routines.
 *
 * @author 2012-10-09 Kris Becker
 *
 * @param ikCode Instrument code to check for keyword specification of
 *               correction
 * @param spice Spice class is required in order to check kernel pool for 
 *              keywords  
 * @return bool True if a value was found and applied
 */
  bool LightTimeCorrectionState::checkAberrationCorrection(int ikCode, 
                                                         Spice *spice) {
    try {
      QString ikernKey = "INS" + QString::number(ikCode) + "_LIGHTTIME_CORRECTION";
      QString abcorr = spice->getString(ikernKey);
      m_abcorr = abcorr;
      return (true);
    }
    catch (IException &ie) {
      // Keyword not found or error encountered - ignore
    }
    return (false);
  }


/**
 * @brief Sets the aberration correction directly 
 *  
 * Provides programmer direct setting of this value.  This is typically 
 * required by camera models that fix this value for a specific reason. 
 * 
 * @author 2012-10-28 Kris Becker
 * 
 * @param correction User specified abcorr correction option as defined by the 
 *                   NAIF routine found at
 *                   http://naif.jpl.nasa.gov/pub/naif/toolkit_docs/C/cspice/spkez_c.html.
 */
  void LightTimeCorrectionState::setAberrationCorrection(const QString &correction) { 
    m_abcorr = correction;
    return;
  }

 
/**
 * @brief Returns the value of the current stellar aberration state
 * 
 * @author Kris Becker - 10/28/2012
 * 
 * @return QString Value of correction
 */
  QString LightTimeCorrectionState::getAberrationCorrection() const {
    return (m_abcorr);
  }


 /**
 * @brief Check for light time/stellar aberration tag in SPK comments
 *  
 * This function will search through all SPK kernel file comments section 
 * looking for a specified tag called "ID:USGS_SPK_ABCORR".  If the tag is 
 * found in any of the loaded SPK files, then true is returned.  This is 
 * intended to indicate that the light time and stellar aberration correction 
 * option needs to be overridden when querying NAIF position states. 
 *  
 * This tag is generated by the spkwriter application when recording new SPK 
 * kernels from ISIS corrected SPICE tables.  The SPKs generated from that 
 * application has had all the light time correction states incorporated so we 
 * must ensure that if such kernels are loaded, no corrections are applied. 
 *  
 * Note that this routine is reentrant and can be called multiple times from 
 * multiple sources to make this determination. 
 * 
 * @return bool  Returns true if the tag is found anywhere in the comments 
 *               section of the SPK, otherwise false if it is not found.  When
 *               false, the existing value is retained.
 */
  bool LightTimeCorrectionState::checkSpkKernelsForAberrationCorrection() {
    //  Determine loaded-only kernels.  Our search is restricted to only 
    //  kernels that are loaded and, currently, only of SPK type is of 
    //  interest.
    Kernels kernels;
    kernels.Discover();

    //  Init the tag to Qt QString for effective searching
    QString qtag("ID:USGS_SPK_ABCORR");
    QString abcorr("");

    //  Retrieve list of loaded SPKs from Kernel object
    QStringList spks = kernels.getKernelList("SPK");
    NaifStatus::CheckErrors(); 
    for ( int k = 0 ; k < spks.size() ; k++ ) {
      QString spkFile = spks[k];
      SpiceChar ktype[32];
      SpiceChar source[128];
      SpiceInt  handle;
      SpiceBoolean found;
      //  Get info on SPK kernel mainly the NAIF handle for comment parsing
      (void) kinfo_c(spkFile.toLatin1().data(), sizeof(ktype), sizeof(source), ktype,
                     source, &handle, &found);
      if (found == SPICETRUE) {
        // SPK is open so read and parse all the comments.
        SpiceChar commnt[1001];
        SpiceBoolean done(SPICEFALSE);
        SpiceInt n;

        // NOTE it is specially important to read all comments so this routine
        // is reentrant!  NAIF will automatically reset the pointer to the
        // first comment line when and only when the last comment line is
        // read.  This is not apparent in the NAIF documentation.
        while ( !done ) {
          dafec_c(handle, 1, sizeof(commnt), &n, commnt, &done);
          QString cmmt(commnt);
          int pos = 0;
          if ( (pos = cmmt.indexOf(qtag, pos, Qt::CaseInsensitive)) != -1 ) {
            //  We can put more effort into this when the need arises and
            //  we have a better handle on options.
             abcorr = "NONE";
          }
        }
        // Don't need to read any more kernel comments if we found one with
        // the tag in it.
        if ( !abcorr.isEmpty() )  break;
      }
    }
    NaifStatus::CheckErrors();
  
    // Set internal state only if it was found in the kernels, otherwise the 
    // existing state is preserved.
    if (!abcorr.isEmpty()) {  m_abcorr = abcorr; }
    return (!abcorr.isEmpty());
  }
        
  /** Is light time to target corrected? */
  bool LightTimeCorrectionState::isLightTimeCorrected() const {
    return ("NONE" != m_abcorr);
  }


/**
 * @brief Check status of target/observer swap specification
 *
 * This method checks for the value of the INS-XXXXXX_SWAP_OBSERVER_TARGET
 * kernel pool keyword to determine if a swap of the observer/target order in
 * the SpicePosition class is requested/needed by the instrument specified by 
 * the ikCode parameter (this is assumed to be a valid NAIF instrument code). 
 *
 * A values of TRUE will result in the switch of the order of the target and
 * observer codes in the SpicePosition constructor.
 *
 * @author 2012-10-05 Kris Becker
 *
 * @param ikCode Instrument code to check for 
 * @param spice Spice class is required in order to check kernel pool for 
 *              keywords
 * @return true if swap of observer/target is requested via kernel pool 
 *         variables
 */
  bool LightTimeCorrectionState::checkObserverTargetSwap(int ikCode, 
                                                         Spice *spice) {

    try {
      QString ikernKey = "INS" + QString::number(ikCode) + "_SWAP_OBSERVER_TARGET";
      QString value = spice->getString(ikernKey).toUpper();
      m_swapObserverTarget = ("TRUE" == value);
    }
    catch (IException &ie) {
      // Not there is a false condition
      m_swapObserverTarget = false;
    }

    return (m_swapObserverTarget);
  }


  /** Returns state swap observer/target  */
  bool LightTimeCorrectionState::isObserverTargetSwapped() const {
    return (m_swapObserverTarget);
  }

  /** Turns on swapping of observer/target  */
  void LightTimeCorrectionState::setSwapObserverTarget() {
      m_swapObserverTarget = true;
  }

  /** Turns off swapping of observer/target (default) */
  void LightTimeCorrectionState::setNoSwapObserverTarget() { 
    m_swapObserverTarget = false;
  }


/**
 * @brief Determines state of surface to s/c light time correction 
 *  
 * This state specifies the radius of the target is to be taken into 
 * consideraton when correcting for the time it takes light to travel from 
 * surface to center of target body. 
 *  
 * This method checks for the value of the INS-XXXXXX_LT_SURFACE_CORRECT kernel
 * pool keyword to determine if correction adjustments for light time from 
 * surface to center body are to be applied in the SpicePosition class. 
 *
 * @author 2012-10-28 Kris Becker
 * 
 * @param ikCode Code of instrument to check for
 * @param spice Spice instance to interface virtual NAIF kernel pool
 * 
 * @return bool State of light time from surface to center body correction as 
 *         determined.
 */
  bool LightTimeCorrectionState::checkLightTimeToSurfaceCorrect(int ikCode, 
                                                                Spice *spice) {

    try {
      QString ikernKey = "INS" + QString::number(ikCode) + "_LT_SURFACE_CORRECT";
      QString value = spice->getString(ikernKey).toUpper();
      m_sc_to_surf_ltcorr = ("TRUE" == value);
    }
    catch (IException &ie) {
      // Not there is a false condition
      m_sc_to_surf_ltcorr = false;
    }
    
    return (m_sc_to_surf_ltcorr);
  }

  /** Returns state of light time from surface to center body correction */
  bool LightTimeCorrectionState::isLightTimeToSurfaceCorrected() const {
    return (m_sc_to_surf_ltcorr);
  }

  /**  Sets state of light time from surface to center body for orrection */
  void LightTimeCorrectionState::setCorrectLightTimeToSurface()  {
      m_sc_to_surf_ltcorr = true;
  }

  /**  Disables state of light time from surface to center body for
   *   correction */
  void LightTimeCorrectionState::setNoCorrectLightTimeToSurface() {
      m_sc_to_surf_ltcorr = false;
  }


/**
 * @brief Set default conditions for light time correction state 
 *  
 * This method sets the default conditions for light time correction. 
 * See the default constructor for full description. 
 *  
 * Default preserves existing behavior at the time of this implementation. 
 * 
 * @author 2012-10-28 Kris Becker
 */
  void LightTimeCorrectionState::setDefaultState() {
    m_abcorr = "LT+S";
    m_swapObserverTarget = false;
    m_sc_to_surf_ltcorr = false;
    return;  
  }


}  // namespace Isis
