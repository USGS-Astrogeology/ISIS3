#ifndef ZeroReverse_h
#define ZeroReverse_h

/** This is free and unencumbered software released into the public domain.

The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */
                                                                      
#include <cmath>
#include <string>
#include <vector>
#include <iostream>
#include <sstream>


#include "IString.h"
#include "HiCalTypes.h"
#include "HiCalUtil.h"
#include "HiCalConf.h"
#include "Module.h"
#include "SplineFill.h"
#include "Statistics.h"
#include "SpecialPixel.h"
#include "IException.h"

namespace Isis {

  /**
   * @brief Processes Reverse Clock calibration data (ZeroReverse Module)
   *
   * This class loads and processes the Reverse Clock data from a  HiRISE image
   * for offset correction purposes.   Additional processing may occur in
   * subsequent modules.
   *
   * @ingroup Utility
   *
   * @author 2008-06-13 Kris Becker
   * @internal
   *   @history 2010-04-16 Kris Becker Renamed from Zf to ZeroReverse
   *   @history 2010-10-28 Kris Becker Renamed parameters replacing "Zz" with
   *            "ZeroReverse".
   */
  class ZeroReverse : public Module {

    public:
      //  Constructors and Destructor
      ZeroReverse() : Module("ZeroReverse") { }
      ZeroReverse(HiCalData &cal, const HiCalConf &conf) :
                  Module("ZeroReverse") {
        init(cal, conf);
      }

      /** Destructor */
      virtual ~ZeroReverse() { }

      /**
       * @brief Return statistics for raw Reverse Clock buffer
       *
       * @return const Statistics&  Statistics class with all stats
       */
      const Statistics &Stats() const { return (_stats); }

      /**
       * @brief Specifies if the input trigger conditions were met
       *
       * If trigger conditions where met, the reverse clock correction becomes a
       * constant as opposed to processed reverse clock pixels.
       *
       * @return bool True if triggered, false otherwise
       */
      bool wasTriggered() const { return (_triggered); }

    private:
      HiVector   _revClock;
      Statistics _stats;
      bool       _triggered;


      /**
       * @brief Initialize and compute data solution
       *
       *
       * @author Kris Becker - 4/16/2010
       *
       * @param cal  HiRISE calibration data provided
       * @param conf Configuration data provider
       */
      void init(HiCalData &cal, const HiCalConf &conf) {
        DbProfile prof = conf.getMatrixProfile();
        _history.clear();
        _history.add("Profile["+ prof.Name()+"]");

        int line0 = ConfKey(prof,"ZeroReverseFirstLine",QString("0")).toInt();
        int lineN = ConfKey(prof,"ZeroReverseLastLine",QString("19")).toInt();
        QString tfile= conf.getMatrixSource("ReverseClockStatistics",prof);

        HiMatrix revclk = cropLines(cal.getReverseClock(), line0, lineN);
        _stats.Reset();
        _stats.AddData(revclk[0], revclk.dim1()*revclk.dim2());

        _revClock = averageLines(revclk);
       _history.add("RevClock(CropLines["+ToString(line0)+","+ToString(lineN) +
                    "],Mean["+ToString(_stats.Average()) +
                     "],StdDev["+ToString(_stats.StandardDeviation()) +
                     "],LisPixels["+ToString(_stats.LisPixels())+
                     "],HisPixels["+ToString(_stats.HisPixels()) +
                     "],NulPixels["+ToString(_stats.NullPixels())+ "])");

       DbAccess triggers(Pvl(tfile.toStdString()).findObject("ReverseClockStatistics"));
       QString tprofName = conf.resolve("{FILTER}{CCD}_{CHANNEL}_{BIN}",prof);
       _history.add("ReverseClockStatistics(File["+tfile+
                    "],Profile["+tprofName+"])");

       _triggered= false;
       if (triggers.profileExists(tprofName)) {
         DbProfile tprof(prof, triggers.getProfile(tprofName), tprofName);
         double revmean = ConfKey(tprof,"RevMeanTrigger", QString::fromStdString(toString(_stats.Average()))).toDouble();
         double revstddev = ConfKey(tprof,"RevStdDevTrigger", QString::fromStdString(toString(DBL_MAX))).toDouble();
         int lisTol = ConfKey(tprof, "RevLisTolerance", QString::fromStdString(toString(1))).toInt();
         int hisTol = ConfKey(tprof, "RevHisTolerance", QString::fromStdString(toString(1))).toInt();
         int nulTol = ConfKey(tprof, "RevNulTolerance", QString::fromStdString(toString(1))).toInt();

         _history.add("TriggerLimits(RevMeanTrigger["+ToString(revmean) +
                      "],RevStdDevTrigger["+ToString(revstddev)+
                      "],RevLisTolerance["+ToString(lisTol)+
                      "],RevHisTolerance["+ToString(hisTol)+
                      "],RevNulTolerance["+ToString(nulTol)+ "])");

         if ((_stats.LisPixels() > lisTol) || (_stats.HisPixels() > hisTol) ||
             (_stats.NullPixels() > nulTol) ||
             (_stats.StandardDeviation() > revstddev)) {
           _triggered = true;
           _data = HiVector(_revClock.dim1(), revmean);
           _history.add("Trigger(True - Reverse Clock set to constant,"
                        "ReverseClock["+ToString(revmean)+"])");
         }
         else {
           _history.add("Trigger(False - Reverse Clock processing invoked)");
           _triggered = false;
         }
       }
       else {
         _history.add("Trigger(Profile["+tprofName+"],NotFound!)");
         _triggered = false;
       }

       if (!_triggered) {
        SplineFill spline(_revClock, _history);
        _data = spline.ref();
        _history = spline.History();
       }

        return;
      }

      /** Virtual dump of data processing vectors */
      virtual void printOn(std::ostream &o) const {
        o << "#  History = " << _history << std::endl;
        //  Write out the header
        o << std::setw(_fmtWidth)   << "RevClock"
          << std::setw(_fmtWidth+1) << "Applied\n";

        for (int i = 0 ; i < _data.dim() ; i++) {
          o << formatDbl(_revClock[i]).toStdString() << " "
            << formatDbl(_data[i]).toStdString() << std::endl;
        }
        return;
      }

  };

}     // namespace Isis
#endif
