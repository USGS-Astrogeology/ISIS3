#ifndef ZeroDarkRate_h
#define ZeroDarkRate_h

/** This is free and unencumbered software released into the public domain.

The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */

#include <cmath>
#include <string>
#include <vector>

#include "IString.h"
#include "HiCalTypes.h"
#include "HiCalUtil.h"
#include "HiCalConf.h"
#include "Module.h"
#include "FileName.h"
#include "LoadCSV.h"
#include "LowPassFilter.h"
#include "Statistics.h"
#include "IException.h"

namespace Isis {

  /**
   * @brief Computes a complex dark subtraction component (ZeroDarkRate module)
   *
   * This class computes the HiRISE dark correction component using a
   * combination of the B matrix, slope/intercept components and temperature
   * profiles.
   *
   * @ingroup Utility
   *
   * @author 2021-02-28 Moses Milazzo
   * @internal
   * @history 2021-02-28 Moses Milazzo Copied - "ZeroDarkRate.h" to new file
   *                         "ZeroDarkRate.h" to add new dark current correction model.
   *                         This model no longer uses the B matrices and instead uses
   *                         a temperature-dependent exponential model to calculate
   *                         the correction on a per-image basis.
   *                         dc_rate = a * exp(b*FPA_T) + c
   */
  class ZeroDarkRate : public Module {

    public:
      //  Constructors and Destructor
      ZeroDarkRate() : Module("ZeroDarkRate") { }
      ZeroDarkRate(const HiCalConf &conf) : Module("ZeroDarkRate") {
        init(conf);
      }

      /** Destructor */
      virtual ~ZeroDarkRate() { }

      /**
       * @brief Return statistics for filtered - raw Buffer
       *
       * @return const Statistics&  Statistics class with all stats
       */
      const Statistics &Stats() const { return (_stats); }

    private:
      int _tdi;
      int _bin;
      double _temp;

      /**
       * The coefficients are stored in a csv text file as a 3-column, 1024/bin row
       * matrix..
       */
      HiMatrix _coeffMat;

      Statistics _stats;

      void init(const HiCalConf &conf) {
        _history.clear();
        DbProfile prof = conf.getMatrixProfile();
        _history.add("Profile["+ prof.Name()+"]");
        _tdi = ToInteger(prof("Tdi"));
        _bin = ToInteger(prof("Summing"));
        int samples = ToInteger(prof("Samples"));

        // Load the coefficients
        // The CSV files for this module are named:
        // file: DarkRate_CCD_Ch_TDI${tdi}_BIN{$binning}_ADC{$adc}_hical_????.csv
        // Example:
        // DarkRate_RED1_1_TDI64_BIN2_54_hical_0002.csv
        // The format of the file is as follows:
        // There are three comment lines
        // # Number of files used to generate these values = 40
        // # exponential equation: DC_Rate = a * exp(b * FPA Temperature) + c
        // # a, b, c
        // Then the coefficients begin.
        // Three columns (a, b, c), and 1024/binning rows
        // 2.483618177203812394e+00,2.255885064806690821e-01,5.617339162650616345e+03
        _coeffMat = LoadCSV("DarkRate", conf, prof).getMatrix();
        if (_coeffMat.dim2() != 3) {
          std::string msg = "Zero Dark Rate coefficient CSV has [" + toString(_coeffMat.dim2()) +
                        "] columns, expected 3.";
          throw IException(IException::User, msg, _FILEINFO_);
        }
        if (_coeffMat.dim1() != samples) {
          std::string msg = "Zero Dark Rate coefficient CSV has [" + toString(_coeffMat.dim1()) +
                        "] rows, expected " + toString(samples) + ".";
          throw IException(IException::User, msg, _FILEINFO_);
        }

        //  Set average FPA temperature
        double fpa_py_temp = ToDouble(prof("FpaPositiveYTemperature"));
        double fpa_my_temp = ToDouble(prof("FpaNegativeYTemperature"));
        _temp = (fpa_py_temp+fpa_my_temp) / 2.0;
        _history.add("BaseTemperature[" + ToString(_temp) + "]");

        // Calculate the dark rate for each column.
        _data = HiVector(samples);
        for (int j = 0 ; j < samples ; j++) {
          _data[j] = _coeffMat[j][0] * exp(_coeffMat[j][1]*_temp) + _coeffMat[j][2];
        }

        //  Compute statistics and record to history
        _stats.Reset();
        for ( int i = 0 ; i < _data.dim() ; i++ ) {
          _stats.AddData(_data[i]);
        }
        _history.add("Statistics(Average["+ToString(_stats.Average())+
                     "],StdDev["+ToString(_stats.StandardDeviation())+"])");
        return;
      }


      /** Virtualized data dump method */
      virtual void printOn(std::ostream &o) const {
        o << "#  History = " << _history << std::endl;
        //  Write out the header
        o << std::setw(_fmtWidth+1) << "FPA_Temperature\n"
          << std::setw(_fmtWidth+1) << "ZeroDarkRate\n";

        for (int i = 0 ; i < _data.dim() ; i++) {
          o << formatDbl(_temp) << " "
            << formatDbl(_data[i]) << std::endl;
        }
        return;
      }

  };

}     // namespace Isis
#endif
