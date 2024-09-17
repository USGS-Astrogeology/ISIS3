#ifndef GainChannelNormalize_h
#define GainChannelNormalize_h

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

#include "IException.h"

namespace Isis {

  /**
   * @brief Computes a gain correction for each sample GainChannelNormalize
   *
   * This class computes the HiRISE gain component correction for each sample.
   *
   * @ingroup Utility
   *
   * @author 2010-04-19 Kris Becker
   * @internal
   */
  class GainChannelNormalize : public Module {

    public:
      //  Constructors and Destructor
      GainChannelNormalize() : Module("GainChannelNormalize") { }
      GainChannelNormalize(const HiCalConf &conf) :
                           Module("GainChannelNormalize") {
        init(conf);
      }

      /** Destructor */
      virtual ~GainChannelNormalize() { }

    private:

      void init(const HiCalConf &conf) {
        _history.clear();
        DbProfile prof = conf.getMatrixProfile();
        _history.add("Profile["+ prof.Name()+"]");

        double bin = ToDouble(prof("Summing"));
        double tdi = ToDouble(prof("Tdi"));
        double _normalizer = 128.0 / tdi / (bin*bin);
        _history.add("ModeNormalizer["+ToString(_normalizer)+"]");

        HiVector z = loadCsv("Gains", conf, prof, 0);
        int nsamps = ToInteger(prof("Samples"));
        _data = HiVector(nsamps);
        // Support a single value being read or equal to the number of samples
        if ( z.dim() == 1 ) {
          _data = z[0];
        }
        else if ( z.dim() == nsamps) {
          _data = z;
        }
        else {
          std::ostringstream mess;
          mess << "Expected 1 or " << nsamps << " values from CSV file "
               << getcsvFile().toStdString() << " but got " << z.dim() << " instead!";
          throw IException(IException::User, mess.str(), _FILEINFO_);
        }

        //  Apply the factor to the data
        for ( int i = 0 ; i < _data.dim() ; i++ ) { _data[i] *= _normalizer; }
        return;
      }

  };

}     // namespace Isis
#endif
