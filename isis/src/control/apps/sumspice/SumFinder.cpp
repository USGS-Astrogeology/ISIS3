/** This is free and unencumbered software released into the public domain.

The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */

#include "SumFinder.h"

#include <cmath>
#include <numeric>
#include <string>
#include <vector>

#include <QDebug>
#include <QFile>
#include <QStringList>
#include <QTextStream>
#include <QtGlobal>

// boost library
#include <boost/foreach.hpp>

#include "Camera.h"
#include "IException.h"
#include "IString.h"
#include "Kernels.h"
#include "NaifStatus.h"
#include "Progress.h"
#include "History.h"
#include "Application.h"


using namespace std;

namespace Isis {


  SumFinder::SumFinder() : m_cube(), m_kernels(), m_cubename(),
                           m_sumfile(), m_timestamp(Center), m_sumtime(),
                           m_cubeStartTime(), m_cubeCenterTime(), m_cubeStopTime(),
                           m_cubeExposureTime(0), m_exposureDelay(0.0),
                           m_timeDiff(0.0), m_closest(DBL_MAX) {
  }

  SumFinder::SumFinder(const QString &cubename, const TimeStamp &tstamp) :
                       m_cube(), m_kernels(), m_cubename(),
                       m_sumfile(), m_timestamp(tstamp), m_sumtime(),
                       m_cubeStartTime(), m_cubeCenterTime(), m_cubeStopTime(),
                       m_cubeExposureTime(0), m_exposureDelay(0.0),
                       m_timeDiff(0.0), m_closest(DBL_MAX) {

    setCube(cubename);
  }

  SumFinder::SumFinder(const QString &cubename, const SumFileList &sumlist,
                       const double &tolerance, const TimeStamp &tstamp) :
                       m_cube(), m_kernels(), m_cubename(cubename),
                       m_sumfile(), m_timestamp(tstamp), m_sumtime(),
                       m_cubeStartTime(), m_cubeCenterTime(), m_cubeStopTime(),
                       m_cubeExposureTime(0), m_exposureDelay(0.0),
                       m_timeDiff(0.0), m_closest(DBL_MAX) {

    setCube(cubename);
    seek(sumlist, tolerance);
  }

  SumFinder::SumFinder(const QString &cubename, const SharedSumFile &sumfile,
                       const TimeStamp &tstamp) :
                       m_cube(), m_kernels(), m_cubename(cubename),
                       m_sumfile(sumfile), m_timestamp(tstamp), m_sumtime(),
                       m_cubeStartTime(), m_cubeCenterTime(), m_cubeStopTime(),
                       m_cubeExposureTime(0), m_exposureDelay(0.0),
                       m_timeDiff(0.0), m_closest(DBL_MAX) {

    setCube(cubename);
    SumFileList sumlist;
    sumlist.append(sumfile);
    seek(sumlist, DBL_MAX);  // Should unconditionally succeed
    if ( !isFound() ) {  // Gut check
      QString mess = "Failed to unconditionally accept associated SUMFILE!";
      throw IException(IException::Programmer, mess, _FILEINFO_);
    }
  }

  SumFinder::~SumFinder() { }

  bool SumFinder::isValid() const {
    if ( m_cubename.isEmpty() ) { return ( false ); }
    return ( isFound() );
  }

  bool SumFinder::isFound() const {
    return ( !m_sumfile.isNull() );
  }

  void SumFinder::setTimeStamp(const SumFinder::TimeStamp &tstamp) {
    m_timestamp = tstamp;
    return;
  }

  SumFinder::TimeStamp SumFinder::getTimeStamp() const {
    return ( m_timestamp );
  }

  const iTime &SumFinder::cubeStartTime() const {
    return  ( m_cubeStartTime );
  }

  const iTime &SumFinder::cubeCenterTime() const {
    return  ( m_cubeCenterTime );
  }


  const iTime &SumFinder::cubeStopTime() const {
    return ( m_cubeStopTime );
  }

  double SumFinder::exposureTime() const {
    return ( m_cubeExposureTime );
  }


  double SumFinder::sumStartTime() const {
    if ( Start == getTimeStamp() ) {
      return ( m_sumtime.Et() );
    }
    else if ( Center == getTimeStamp() ) {
      return ( (m_sumtime.Et() - (exposureTime() / 2.0)) );
    }
    else {  // Stop == getTimeStamp()
      return ( (m_sumtime.Et() - exposureTime()) );
    }
  }


  double SumFinder::sumCenterTime() const {
    if ( Start == getTimeStamp() ) {
      return ( (m_sumtime.Et() + (exposureTime() / 2.0)) );
    }
    else if ( Center == getTimeStamp() ) {
      return ( m_sumtime.Et() );
    }
    else {  // Stop == getTimeStamp()
      return ( (m_sumtime.Et() - (exposureTime() / 2.0)) );
    }
  }


  double SumFinder::sumStopTime() const {
    if ( Start == getTimeStamp() ) {
      return ( (m_sumtime.Et() + exposureTime()) );
    }
    else if ( Center == getTimeStamp() ) {
      return ( (m_sumtime.Et() + (exposureTime() / 2.0)) );
    }
    else {  // Stop == getTimeStamp()
      return ( m_sumtime.Et()  );
    }
  }

  double SumFinder::exposureDelay() const {
    return ( m_exposureDelay );
  }

  const iTime &SumFinder::timeT() const {
    if ( Center == m_timestamp ) { return ( m_cubeCenterTime );  }
    if ( Start  == m_timestamp ) { return ( m_cubeStartTime );  }
    if ( Stop   == m_timestamp ) { return ( m_cubeStopTime );  }
    return ( m_cubeCenterTime );
  }


  double SumFinder::deltaT() const {
    return ( m_timeDiff );
  }

  double SumFinder::closest() const {
    return ( m_closest );
  }

  void SumFinder::resetCube() {
    // Always close out the kernels and cubes.
    m_kernels.reset();
    m_cube.reset();
  }

  void SumFinder::setCube(const QString &name) {
    resetCube();

    m_cubename =  name;
    m_cube.reset( new Cube(name, "rw") );

    // Ensure kernels are loaded for time conversions (mainly)
    m_kernels.reset( new Kernels(*m_cube) );
    m_kernels->Load();

    calculateTimes(*m_cube, m_cubeStartTime, m_cubeCenterTime, m_cubeStopTime,
                   m_cubeExposureTime, m_exposureDelay);
    return;
  }

  const Cube *SumFinder::cube() const {
    return ( m_cube.data() );
  }

  const QString &SumFinder::name() const {
    return ( m_cubename );
  }


  /**
   * @brief Find SUMFILE for the given cube file
    *
   * This method will find the appropriate SUMFILE associated with a Cube object
   * given a list of SumFile objects. The time the image was observed is compared
   * to the time of a SUMFILE. The SumFile that matches the cube is the one with
   * the time closest to the Cube observation time.
   *
   * @param Cube       Cube object to find a SumFile for.
   * @param sumFiles   List of SharedSumFile objects to search, sorted in ascending order.
   * @param deltaT     Maximum time difference in ISIS cube file time and
   *                   SumFile time.
   *
   * @return SharedSumFile Pointer to SumFile for ISIS cube
   */
  bool SumFinder::seek(const SumFileList &sumFiles, const double &tolerance ) {

    // Check to see if we got a cube to find
    confirmValidity(m_cube,"Must set a cube to find an associated SumFile!");

    // Disassociate any previous solution
    m_sumfile.clear();
    m_timeDiff = m_closest = DBL_MAX;

    // Just a reset if list is empty
    if ( !(sumFiles.size() > 0) )  return ( false );

    // Otherwise, find the sum file with start ET closest to the
    // cube's ET Note that the sum files are already sorted by ET...
    for (int index = 0 ; index < sumFiles.size() ; index++) {
      double tdiff = fabs( sumFiles[index]->et() - timeT().Et() );
      m_closest = qMin( m_closest, tdiff );  // Only done here
      if ( tdiff <= tolerance ) {
        if ( tdiff < fabs(m_timeDiff) ) {
          setSumFile(sumFiles[index]);
        }
      }
    }

    // Return results
    return ( isFound() );
  }


  bool SumFinder::setSumFile(const SharedSumFile &sumfile) {
    m_sumfile = sumfile;
    m_sumtime = sumfile->time();
    m_timeDiff = m_sumtime.Et() - timeT().Et();
    return ( true );
  }


 const SumFile *SumFinder::sumfile() const {
   return ( m_sumfile.data() );
 }


/**
 * @brief Calculate start, end and exposure time from an ISIS Cube file
 *
 * @author 2016-07-26 Kris Becker
 *
 * @param cube         Cube to extract times from
 * @param startTime    Start time of observation
 * @param stopTime     Stop time of observation
 * @param exposureTime Exposure time of observation
 *
 * @return @b bool True if successful, false if failed
 */
  bool SumFinder::calculateTimes(Cube &cube,
                                 iTime &startTime,
                                 iTime &centerTime,
                                 iTime &stopTime,
                                 double &exposureTime,
                                 double &exposureDelay) {

    // Use the mid exposure time and compute endtimes there. Note this assumes
    // a framing camera
    exposureDelay    = startExposureDelay(cube);
    double stopDelay = stopExposureDelay(cube);

    // Exposure and center time is determined. (Assumes its a framing camera!)
    exposureTime = getExposureTime(cube);
    centerTime = cube.camera()->time();


    // Get spacecraft clock times for starting/ending elements
    Pvl *cubeLabels = cube.label();
    PvlGroup &instGrp = cubeLabels->findGroup("Instrument", Pvl::Traverse);

    // Compute real start time. We are going to trust the SCLK values in the
    // label
    startTime = centerTime - (exposureTime / 2.0 );
    if ( instGrp.hasKeyword("SpacecraftClockStartCount") ) {
      PvlKeyword startSClock = instGrp["SpacecraftClockStartCount"];
      QString originalClock = QString::fromStdString(startSClock[0]);
      startTime = cube.camera()->getClockTime(originalClock) + exposureDelay;
    }

    // Determine end time where label values take precedence
    stopTime = centerTime + (exposureTime / 2.0);
    if ( instGrp.hasKeyword("SpacecraftClockStopCount") ) {
      PvlKeyword stopSClock = instGrp["SpacecraftClockStopCount"];
      QString originalClock = QString::fromStdString(stopSClock[0]);
      try {
        stopTime = cube.camera()->getClockTime(originalClock) - stopDelay;
      }
      catch(IException &e) {
        // The stop time is not required. So, if we cannot access it, move on.
      }
    }

    return ( true );
  }


/**
 * @brief Get the exposure time from the Cube label
 *
 * @author 2016-08-23 Kris Becker
 *
 * @param cube   Cube to extract expousure time from
 *
 * @return double Provides the expousure time in seconds
 */
  double SumFinder::getExposureTime(const Cube &cube) const {

    Pvl *cubeLabels = cube.label();
    PvlGroup &instGrp = cubeLabels->findGroup("Instrument", Pvl::Traverse);
    PvlKeyword exptime = instGrp["ExposureDuration"];
    QString units = QString::fromStdString(exptime.unit(0)).toLower();
    double etime = std::stod(exptime[0]);

    // Convert to seconds if indicated
    if ( "milliseconds" == units) etime /= 1000.0;
    if ( "millisecond" == units) etime /= 1000.0;
    if ( "msecs" == units) etime /= 1000.0;
    if ( "msec" == units) etime /= 1000.0;
    if ( "ms" == units) etime /= 1000.0;

    return (etime);
  }

/**
 * @brief Update requested items based upon bit mask of options
 *
 * @author 2016-07-26 Kris Becker
 *
 * @param options Bit mask of processing Options
 *
 * @return bool   True if all operations were successful, false
 *                if a failure occured
 */
  bool SumFinder::update(const unsigned int options)  {
    confirmValidity(m_cube,"Valid Cube (and SUMFILE) required for updates!");

    bool good = true;

    // Reset timing to original times just needs a cube file.
    if ( options & Reset) {
        good = ( good && resetTimes() );
        return ( good );
    }

    // All other options require a sumfile to be associated
    confirmValidity(m_sumfile,"Valid SUMFILE (got a Cube) required for updates!");

    if ( options & Times) {
        good = ( good && updateTimes() );
    }

    if ( options & Spice ) {
        good = ( good && m_sumfile->updateSpice(*m_cube) );
    }
    else {

      if ( options & Pointing ) {
          good = ( good && m_sumfile->updatePointing(*m_cube) );
      }

      if ( options & Position ) {
          good = ( good && m_sumfile->updatePosition(*m_cube) );
      }
    }

    return (good);
  }

/**
 * @brief Determine delay at start time to beginning of exposure
 *
 * This method determines the delay from the start time to the begining of the
 * exposure if it exists. This is not typically but is usually determined in the
 * camera model.
 *
 * @author 2016-09-13 Kris Becker
 *
 * @param cube  ISIS image cube to determine delay
 *
 * @return double Returns the exposure start time delay in seconds if exists
 */
  double SumFinder::startExposureDelay(const Cube &cube) const {

    Pvl *cubeLabel = cube.label();
    PvlGroup &instGrp = cubeLabel->findGroup("Instrument", Pvl::Traverse);

    QString scname = QString::fromStdString(instGrp["SpacecraftName"]);
    if ( scname.toLower() != "dawn" ) { return (0.0); }

    QString instname = QString::fromStdString(instGrp["InstrumentId"]);
    if ( instname.toLower() == "fc1") { return (0.193); }
    if ( instname.toLower() == "fc2") { return (0.193); }

    return (0.0);
  }


  double SumFinder::stopExposureDelay(const Cube &cube) const {
    return (0.0);
  }



/**
 * @brief Update start/end times in the label of an ISIS cube
 *
 * This method will update the start and end times in the label of an ISIS cube
 * file with the contents of the SUMSPICE. The contents of the
 * SpacecraftStartClockTime/SpacecraftEndClockTimes with the contents the lines
 * of the UTC contained in the SUMFILE
 *
 * @param cube    An intialized ISIS Cube object
 * @return bool   True if succesful, false if the operation fails
 */
  bool SumFinder::updateTimes() {

    // Check conditions
    confirmValidity(m_cube,"Must set a cube to update times with SUMFILE times!");
    confirmValidity(m_sumfile,"Must associated SumFile with a cube to update times!");

    // Acquire keywords
    Pvl *cubeLabel = m_cube->label();
    PvlGroup &instGrp = cubeLabel->findGroup("Instrument", Pvl::Traverse);
    PvlGroup sumtime("SumTimeHistory");
    PvlGroup *sumtPtr = &sumtime;
    bool addSumGroup = true;  // Assume the object doesn't exist!

    // Some monkey business for Sumtime history object (see below)
    PvlObject &isiscube = cubeLabel->findObject("IsisCube");
    if ( isiscube.hasGroup(sumtime.name()) ) {
      sumtPtr = &cubeLabel->findGroup(sumtime.name(), Pvl::Traverse);
      addSumGroup = false;
    }
    PvlGroup &sumtGrp = *sumtPtr;

    // Find relevant cube keywords
    PvlKeyword origStartClock = findKeyword("SpacecraftClockStartCount", instGrp);
    PvlKeyword origStopClock  = findKeyword("SpacecraftClockStopCount", instGrp);
    PvlKeyword origStartTime  = findKeyword("StartTime", instGrp);
    PvlKeyword origStopTime   = findKeyword("StopTime", instGrp);

    // Find relevant archive keywords
    PvlKeyword sumtStartClock = findKeyword("SpacecraftClockStartCount", sumtGrp);
    PvlKeyword sumtStopClock  = findKeyword("SpacecraftClockStopCount", sumtGrp);
    PvlKeyword sumtStartTime  = findKeyword("StartTime", sumtGrp);
    PvlKeyword sumtStopTime   = findKeyword("StopTime", sumtGrp);


    // Ok, first lets determine if we have any expected keywords in the label.
    // This will prevent any updates to the label if total failure occurs. This
    // will also throw an error if this situation is encountered.
    int nvalid = origStartClock.size() + origStopClock.size() +
                 origStartTime.size()  + origStopTime.size();

    // Now check to ensure we have at least one valid time that will be
    // modified and assume that is enough to get it done properly
    if ( 0 == nvalid ) {
      QString mess = "No expected timing keywords found on labels - "
                     "assuming non-standard, time update failed";
      throw IException(IException::User, mess, _FILEINFO_);
    }

    // Add the sum file name to the Archive group here. It just looks better to
    // put this keyword here as it nicely delineates from other keywords.
    PvlKeyword sumtFileKeyword = findKeyword("SUMFILE", sumtGrp);
    if (sumtFileKeyword.size() == 0) {
      sumtFileKeyword.addComment("SUMFILE(s) used to update the SCLK timing "
                                 "in the instrument group (SPC).");
    }
    sumtFileKeyword.addValue(m_sumfile->name().toStdString());
    setKeyword(sumtFileKeyword, sumtGrp);

    // Compute new values for existing keywords if we got them
    Camera *camera = m_cube->camera();

    iTime newStartClock(sumStartTime() - startExposureDelay(*m_cube));
    iTime newStopClock(sumStopTime()   + stopExposureDelay(*m_cube));

    // Compute start SCLK if present on labels
    if ( origStartClock.size() > 0 ) {
      NaifStatus::CheckErrors();
      char newSCLK[256];
      sce2s_c(camera->naifSclkCode(), newStartClock.Et(),
              sizeof(newSCLK), newSCLK);
      NaifStatus::CheckErrors();

      sumtStartClock.addValue(origStartClock[0], origStartClock.unit());
      origStartClock.setValue(QString(newSCLK).toStdString(), origStartClock.unit());

      setKeyword(origStartClock, instGrp);
      setKeyword(sumtStartClock, sumtGrp);
    }


    // Compute end SCLK if present on labels
    if ( origStopClock.size() > 0 ) {
      NaifStatus::CheckErrors();
      char newSCLK[256];
      sce2s_c(camera->naifSclkCode(), newStopClock.Et(),
              sizeof(newSCLK), newSCLK);
      NaifStatus::CheckErrors();

      sumtStopClock.addValue(origStopClock[0], origStopClock.unit());
      origStopClock.setValue(QString(newSCLK).toStdString(), origStopClock.unit());

      setKeyword(origStopClock, instGrp);
      setKeyword(sumtStopClock, sumtGrp);
    }


    // Now check for StartTime
    if ( origStartTime.size() > 0 ) {
      sumtStartTime.addValue(origStartTime[0], origStartTime.unit());
      origStartTime.setValue(newStartClock.UTC().toStdString(), origStartTime.unit());

      setKeyword(origStartTime, instGrp);
      setKeyword(sumtStartTime, sumtGrp);
    }

    // Now check for StopTime
    if ( origStopTime.size() > 0 ) {
      sumtStopTime.addValue(origStopTime[0], origStopTime.unit());
      origStopTime.setValue(newStopClock.UTC().toStdString(), origStopTime.unit());

      setKeyword(origStopTime, instGrp);
      setKeyword(sumtStopTime, sumtGrp);
    }

    // Now add the SUMFILE time group if needed (first time only)
    if ( addSumGroup  ) {
      isiscube.addGroup(sumtGrp);
    }

    // Disable current SPICE on label
    (void) disableSpice(*cubeLabel);
    return ( true );
  }

  bool SumFinder::resetTimes() {

    // Check conditions
    confirmValidity(m_cube,"Must set a cube to update times with SUMFILE times!");

    // Acquire keywords
    Pvl *cubeLabel = m_cube->label();
    PvlGroup &instGrp = cubeLabel->findGroup("Instrument", Pvl::Traverse);
    PvlObject &isiscube = cubeLabel->findObject("IsisCube");

    // See if the proper group exists - we're done if doesn't
    if ( !isiscube.hasGroup("SumTimeHistory") ) {
      return ( false );
    }

    // Pull the group from the label
    PvlGroup sumtGrp = isiscube.findGroup("SumTimeHistory");

    // Find relevant cube keywords
    PvlKeyword origStartClock = findKeyword("SpacecraftClockStartCount", instGrp);
    PvlKeyword origStopClock  = findKeyword("SpacecraftClockStopCount", instGrp);
    PvlKeyword origStartTime  = findKeyword("StartTime", instGrp);
    PvlKeyword origStopTime   = findKeyword("StopTime", instGrp);

    // Find relevant archive keywords
    PvlKeyword sumtStartClock = findKeyword("SpacecraftClockStartCount", sumtGrp);
    PvlKeyword sumtStopClock  = findKeyword("SpacecraftClockStopCount", sumtGrp);
    PvlKeyword sumtStartTime  = findKeyword("StartTime", sumtGrp);
    PvlKeyword sumtStopTime   = findKeyword("StopTime", sumtGrp);

    // Reset start SCLK if present on labels
    if ( (origStartClock.size() > 0) && (sumtStartClock.size() > 0) ) {
      origStartClock.setValue(sumtStartClock[0], origStartClock.unit());
      setKeyword(origStartClock, instGrp);
    }

    // Reset end SCLK if present on labels
    if ( (origStopClock.size() > 0) && (sumtStopClock.size() > 0) ) {
      origStopClock.setValue(sumtStopClock[0], origStopClock.unit());
      setKeyword(origStopClock, instGrp);
    }

    // Now check for StartTime
    if ( (origStartTime.size() > 0) && (sumtStartTime.size() > 0) ) {
      origStartTime.setValue(sumtStartTime[0], origStartTime.unit());
      setKeyword(origStartTime, instGrp);
    }

    // Now check for StopTime
    if ( (origStopTime.size() > 0) && (sumtStopTime.size() > 0)  ) {
      origStopTime.setValue(sumtStopTime[0], origStopTime.unit());
      setKeyword(origStopTime, instGrp);
    }

    // Now remove the sumtime group from the labels.
    isiscube.deleteGroup(sumtGrp.name());

    // Disable current SPICE on label
    (void) disableSpice(*cubeLabel);
    return ( true );
  }



  PvlKeyword SumFinder::findKeyword(const QString &name,
                                    const PvlContainer &keys) const {
    if ( keys.hasKeyword(name.toStdString()) ) {
      return ( keys.findKeyword(name.toStdString()) );
    }

    return (PvlKeyword(name.toStdString()));
  }


  void SumFinder::setKeyword(const PvlKeyword &keyword,
                             PvlContainer &keys) const {
    keys.addKeyword(keyword, PvlContainer::Replace);
    return;
  }

  bool SumFinder::deleteKeyword(const QString &keyword,
                                PvlContainer &keys) const {
    if ( keys.hasKeyword(keyword.toStdString()) ) {
      keys.deleteKeyword(keyword.toStdString());
      return ( true );
    }

    return ( false );
  }

  int SumFinder::disableSpice(Pvl &label) const {
    int ndeleted = 0;

    if ( label.hasObject("IsisCube") ) {
      PvlObject &iCube = label.findObject("IsisCube");
      if ( iCube.hasGroup("Kernels") ) {
        PvlGroup &kernGrp = iCube.findGroup("Kernels", Pvl::Traverse);

        // Known Kernels group keywords
        QStringList kernkeys;
        kernkeys << "LeapSecond" << "TargetAttitudeShape" << "TargetPosition"
                 << "InstrumentPointing" << "Instrument" << "SpacecraftClock"
                 << "InstrumentPosition" << "InstrumentAddendum" << "ShapeModel"
                 << "Extra" << "InstrumentPositionQuality"
                 << "InstrumentPointingQuality" << "SpacecraftPointing"
                 << "SpacecraftPosition" << "ElevationModel" << "Frame"
                 << "StartPadding" << "EndPadding" << "CameraVersion";

        // Force user to re-run spiceinit by removing every known keyword in the
        // Kernels Group that is created by spiceinit (as of 2016-09-15)
        BOOST_FOREACH ( QString keyword, kernkeys ) {
          if ( deleteKeyword(keyword, kernGrp) ) {
            ndeleted++;
          }
        }
      }
    }
    return (ndeleted);
  }
}
// namespace Isis
