#ifndef ZeroBufferSmooth_h
#define ZeroBufferSmooth_h

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
#include "LowPassFilter.h"
#include "Statistics.h"
#include "SpecialPixel.h"
#include "IException.h"

namespace Isis {

  /**
   * @brief Processes Buffer calibration data (ZeroBufferSmooth Module)
   *
   * This class loads and processes the Buffer data from a HiRISE image for
   * drift correction purposes.  The config file contains parameter
   * (ZfFirstSample, ZfLastSample) that indicate which regions of the
   * calibration buffer to extract/use.  This region is averaged across the line
   * axis for each line resulting in a single value for each line.  The
   * resulting vector is then filtered with a lowpass filter. The filter width
   * (ZfFilterWidth) and number of interations (ZfFilterIterations) are
   * contained within the config file. A spline fit is applied if any missing
   * data remain after filtering.
   *
   * @ingroup Utility
   *
   * @author 2008-06-10 Kris Becker
   * @internal
   *   @history 2010-04-16 Kris Becker Completed documentation
   *   @history 2010-10-28 Kris Becker Renamed parameters removing the "Zf"
   *            prefix and replacing with "ZeroBufferSmooth".
   */
  class ZeroBufferSmooth : public Module {

    public:
      //  Constructors and Destructor
      ZeroBufferSmooth() : Module("ZeroBufferSmooth") { }
      /**
       * @brief Construct with data parameters
       *
       * This constructor completely computes drift from data collected in a
       * HiRISE image
       *
       * @param cal   Calibration data collection
       * @param conf  All necessary parameters for computations
       */
      ZeroBufferSmooth(HiCalData &cal, const HiCalConf &conf) :
                  Module("ZeroBufferSmooth") {
        init(cal, conf);
      }

      /** Destructor */
      virtual ~ZeroBufferSmooth() { }

      /**
       * @brief Return statistics for filtered - raw Buffer
       *
       * @return const Statistics&  Statistics class with all stats
       */
      const Statistics &Stats() const { return (_stats); }

    private:
      HiVector   _buffer;
      Statistics _stats;

      /**
       * @brief Workhorse of the zero buffer computation
       *
       * The default module, assumed to be the Zf module, is retrieved to
       * provide parameter necessary to compute the drift correction for a
       * HiRISE image.
       *
       *
       * @param cal  Calibration data container/provider
       * @param conf Configuration parameter provider
       */
      void init(HiCalData &cal, const HiCalConf &conf) {
        DbProfile prof = conf.getMatrixProfile();
        _history.clear();
        _history.add("Profile["+ prof.Name()+"]");

        int samp0 = ConfKey(prof,"ZeroBufferSmoothFirstSample",QString("0")).toInt();
        int sampN = ConfKey(prof,"ZeroBufferSmoothLastSample",QString("11")).toInt();
        _buffer = averageSamples(cal.getBuffer(), samp0, sampN);
        _history.add("AveCols(Buffer["+ToString(samp0)+","+ToString(sampN)+"])");

        //  Smooth/filter the averages
        LowPassFilter bufter(_buffer, _history,
                          ConfKey(prof,"ZeroBufferSmoothFilterWidth",QString("201")).toInt(),
                          ConfKey(prof,"ZeroBufferSmoothFilterIterations",QString("2")).toInt());
        //  If need be, fill the data with a cubic spline
        SplineFill spline(bufter);
        _data = spline.ref();
        _history = spline.History();

        //  Compute statistics and record to history
        _stats.Reset();
        for ( int i = 0 ; i < _data.dim() ; i++ ) {
          // Spline guarantees _data is non-null!
          if ( !IsSpecial(_buffer[i]) ) {
            _stats.AddData(_data[i] - _buffer[i]);
          }
        }
        _history.add("Statistics(Average["+ToString(_stats.Average())+
                     "],StdDev["+ToString(_stats.StandardDeviation())+"])");
        return;
      }

      /**
       * @brief Virtualized parameter reporting method
       *
       * This method is invoked when dumping the drift correction parameters.
       *
       * @param o Output stream to write results to
       */
      virtual void printOn(std::ostream &o) const {
        o << "#  History = " << _history << std::endl;
        //  Write out the header
        o << std::setw(_fmtWidth)   << "RawBuffer"
          << std::setw(_fmtWidth+1) << "Filtered\n";

        for (int i = 0 ; i < _data.dim() ; i++) {
          o << formatDbl(_buffer[i]).toStdString() << " "
            << formatDbl(_data[i]).toStdString() << std::endl;
        }
        return;
      }

  };

}     // namespace Isis
#endif
