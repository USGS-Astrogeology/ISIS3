#ifndef LROCEmpirical_h
#define LROCEmpirical_h

/** This is free and unencumbered software released into the public domain.

The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */

#include <iostream>
#include <iomanip>
#include <sstream>
#include <iomanip>

#include "Camera.h"
#include "DbProfile.h"
#include "IString.h"
#include "PhotometricFunction.h"
#include "SpecialPixel.h"

namespace Isis {

  class PvlObject;
  class Camera;

  /**
   * @brief An implementation of photometric equation used for LRO NAC Cameras
   *
   * This class implements the LROC Empirical photometric equation used for the LRO NAC
   * Cameras.
   *
   * @author 2016-08-15 Victor Silva
   *
   * @internal
   *   @history 2016-08-15 - Code adapted from lrowacpho written by Kris Becker
   *   @history 2021-03-12 Victor Silva - Added b parameters for 2019 version of
   *            LROC Empirical algorithm
   *
   */
  class LROCEmpirical : public PhotometricFunction {
    public:
      LROCEmpirical (PvlObject &pvl, Cube &cube, bool useCamera);
      virtual ~LROCEmpirical () ;

      double photometry(double i, double e, double g, int band = 1) const;
      void report( PvlContainer &pvl );

    private:
      /**
       * Container for band photometric correction parameters
       *
       * @author 2016-08-15 Victor Silva
       *
       * @internal
       *   @history 2016-08-05 - Code adapted from lrowacpho written by Kris Becker
       *   @history 2021-03-12 Victor Silva - Added b parameters for 2019 version of
       *            LROC Empirical algorithm
       */
      struct Parameters {
        Parameters() : aTerms(), bTerms(),
                       wavelength(0.0), tolerance(0.0),
                       units("Degrees"), phaUnit(1.0), band(0), phoStd(0.0),
                       algoVersion(2019),
                       iProfile(-1) { }
        ~Parameters() { }
        bool IsValid() const {
          return (iProfile != -1);
        }
        std::vector<double> aTerms; //<! a-terms for 2014 algorithm
        std::vector<double> bTerms; //<! b-terms for 2019 algorithm
        double wavelength; //<! Wavelength for correction
        double tolerance; //<! Wavelength Range/Tolerance
        QString units; //<! Phase units of equation
        double phaUnit; //<! 1 for degrees, Pi/180 for radians
        int band; //<! Cube band parameters
        double phoStd; //<! Computed photometric std.
        int algoVersion; //<! Algorithm version
        int iProfile; //<! Profile index of this data
      };

      void init(PvlObject &pvl, Cube &cube);
      double photometry(const Parameters &parms, double i, double e,double g) const;
      Parameters findParameters(const double wavelength) const;
      Parameters extract( const DbProfile &profile) const;

      std::vector<DbProfile> m_profiles; //<! Profiles for all possible wavelengths
      std::vector<Parameters> m_bandpho; //<! Parameters for each band
  };

};
#endif
