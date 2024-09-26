#ifndef GainTemperature_h
#define GainTemperature_h

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
#include "Statistics.h"
#include "IException.h"

namespace Isis {
  /**
   * @brief GaingTemperature Module - Applies temperature-dependant gain
   *        correction (column)
   *
   * This class computes the temperature dependant gain correction.  It has a
   * comma separated value (CSV) file in the config file parameter
   * "FpaTemperatureFactorFile".  This file is assumed to contain three columns
   * of data:  column 0: CCD identifier (Ex:  RED0), column 1: FPA factor for
   * channel 0, and column 2: FPA factor for channel 1.  It should have 14 rows,
   * 1 for each HiRISE CCD.
   *
   * @ingroup Utility
   *
   * @author 2009-09-14 Kris Becker
   * @internal
   *   @history 2010-04-16 Kris Becker Utilize standardized CSV reader to get
   *            FpaFactor
   *   @history 2010-05-26 Kris Becker Corrected sign for CSV factor
   *            (_fpaFactor)
   *   @history 2016-08-28 Kelvin Rodriguez - Removed usused private member variables to
   *            eliminate unused member variables warnings in clang. Part of porting to OS X 10.11.
   */
  class GainTemperature : public Module {

    public:
      //  Constructors and Destructor
      GainTemperature() : Module("GainTemperature") { }
      GainTemperature(const HiCalConf &conf) : Module("GainTemperature") {
        init(conf);
      }

      /** Destructor */
      virtual ~GainTemperature() { }

    private:
      std::string _fpaFile;
      double _refTemp;        // Reference temperature
      double _fpaFactor;      // Temperature factor

      void init(const HiCalConf &conf) {
        _history.clear();
        DbProfile prof = conf.getMatrixProfile();
        _history.add("Profile["+ prof.Name()+"]");

        //  Get temperature factor
        HiVector factor = loadCsv("FpaGain", conf, prof, 1);
        _fpaFactor = factor[0];

        //  Get temperature parameters
        _refTemp = ConfKey(prof, "FpaReferenceTemperature", QString::fromStdString(toString(21.0))).toDouble();

        double fpa_py_temp = ToDouble(prof("FpaPositiveYTemperature"));
        double fpa_my_temp = ToDouble(prof("FpaNegativeYTemperature"));


        double FPA_temp = (fpa_py_temp+fpa_my_temp) / 2.0;
        double _baseT = 1.0 - (_fpaFactor * (FPA_temp - _refTemp));

        //  Create data
        int nsamps = ToInteger(prof("Samples"));
        _data = HiVector(nsamps, _baseT);

        // History
        _history.add("FpaTemperatureFactor[" + ToString(_fpaFactor) + "]");
        _history.add("FpaAverageTemperature[" + ToString(FPA_temp) + "]");
        _history.add("FpaReferenceTemperature[" + ToString(_refTemp) + "]");
        _history.add("Correction[" + ToString(_baseT) + "]");
        return;
      }

  };

}     // namespace Isis
#endif
