#ifndef SumFinder_h
#define SumFinder_h

/** This is free and unencumbered software released into the public domain.

The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */

#include <ostream>
#include <cfloat>
#include <cmath>

#include <QMap>
#include <QSharedPointer>
#include <QString>
#include <QStringList>

#include "Cube.h"
#include "FileList.h"
#include "iTime.h"
#include "Kernels.h"
#include "Quaternion.h"
#include "SpecialPixel.h"
#include "SumFile.h"

namespace Isis {



  /**
   * Container class for the sums.
   *
   * @author 2016-09-14 Kris Becker
   *
   * @internal
   *   @history 2016-09-14 Kris Becker - Original Version
   *   @history 2017-02-09 Jesse Mapel - Updated to ignore invalid
   *                            SpacecraftClockStopCount values.
   *   @history 2018-04-12 Kaitlyn Lee - Added method writeHistory()
   *                           to add a sumspice entry to
   *                           m_cube's History blob. Fixes #4972.
   *
   */
  class SumFinder {
    public:
      enum Options   { None = 0, Times = 1, Spice = 2, Pointing = 4,
                       Position = 8, Reset = 16};
      enum TimeStamp { Start, Center, Stop };

      SumFinder();
      SumFinder(const QString &cubename, const TimeStamp &tstamp = Center);
      SumFinder(const QString &cubename, const SumFileList &sumlist,
                const double &tolerance = DBL_MAX,
                const TimeStamp &tstamp = Center);
      SumFinder(const QString &cubename, const SharedSumFile &sumfile,
                const TimeStamp &tstamp = Center);
      virtual ~SumFinder();

      bool isValid() const;
      bool isFound() const;

      void setTimeStamp(const TimeStamp &tstamp);
      TimeStamp getTimeStamp() const;

      const iTime &cubeStartTime() const;
      const iTime &cubeCenterTime() const;
      const iTime &cubeStopTime() const;
      double exposureTime() const;
      double exposureDelay() const;

      double sumStartTime() const;
      double sumCenterTime() const;
      double sumStopTime() const;

      const iTime &timeT() const;
      double deltaT() const;
      double closest() const;

      void resetCube();
      void setCube(const QString &name);
      const Cube *cube() const;
      const QString &name() const;

      bool  seek(const SumFileList &sumlist, const double &tolerance = DBL_MAX);
      bool  setSumFile(const SharedSumFile &sumfile);
      const SumFile *sumfile() const;

      bool update(const unsigned int options);

   protected:
      virtual bool calculateTimes(Cube &cube,
                                  iTime &startTime,
                                  iTime &centerTime,
                                  iTime &stopTime,
                                  double &exposureTime,
                                  double &exposureDelay);

      virtual double getExposureTime(const Cube &cube) const;
      virtual double startExposureDelay(const Cube &cube) const;
      virtual double stopExposureDelay(const Cube &cube) const;


    private:
      QScopedPointer<Cube>    m_cube;
      QScopedPointer<Kernels> m_kernels;
      QString                 m_cubename;

      SharedSumFile           m_sumfile;  ///!< Pointer to SumFile if found
      TimeStamp               m_timestamp;
      iTime                   m_sumtime;

      iTime                   m_cubeStartTime;
      iTime                   m_cubeCenterTime;
      iTime                   m_cubeStopTime;
      double                  m_cubeExposureTime;
      double                  m_exposureDelay;

      double                  m_timeDiff;
      double                  m_closest;

      bool updateTimes();
      bool resetTimes();

      PvlKeyword findKeyword(const QString &name, const PvlContainer &keys) const;
      void setKeyword(const PvlKeyword &keyword, PvlContainer &keys) const;
      bool deleteKeyword(const QString &keyword, PvlContainer &keys) const;
      int disableSpice(Pvl &label) const;

      template <class T>
        bool confirmValidity(const T &target, const QString &errmess,
                             const bool &throwIfInvalid = true)
                             const;
  };


/**
 * @brief Provides a check of a shared pointer and manage error
 *        condition
 *
 *  class T must define an isNull() method that returns true when the container
 *  possesses a valid (pointer) element.
 *
 *  This method is designed to take a QSharedPointer, QScopedPointer and
 *  QPointer objects and potentially others that have the isNull() method.
 *
 * @param target     Pointer container to check
 * @param errmess    Error message to throw when null and thow
 *                   is enabled
 * @param throwIfInvalid True if throwing an exception if
 *                       non-valid condition.
 *
 * @return bool True if container content is valid, false if
 *              error
 *
 * @author 2016-09-14 Kris Becker
 *
 * @internal
 *   @history 2016-09-14 Kris Becker - Original Version
 */
  template <class T>
     bool SumFinder::confirmValidity(const T &target, const QString &errmess,
                                     const bool &throwIfInvalid) const {

      if ( !target.isNull() ) {  return (true);  }

      if ( throwIfInvalid ) {
        throw IException(IException::Programmer, errmess.toStdString(), _FILEINFO_);
      }
      return (false);
    }



};  // namespace Isis
#endif
