#ifndef hapkelro_h
#define hapkelro_h
/**
 * @file
 * $Revision$
 * $Date$
 *
 *   Unless noted otherwise, the portions of Isis written by the USGS are
 *   public domain. See individual third-party library and package descriptions
 *   for intellectual property information, user agreements, and related
 *   information.
 *
 *   Although Isis has been used by the USGS, no warranty, expressed or
 *   implied, is made by the USGS as to the accuracy and functioning of such
 *   software and related material nor shall the fact of distribution
 *   constitute any such warranty, and no responsibility is assumed by the
 *   USGS in connection therewith.
 *
 *   For additional information, launch
 *   $ISISROOT/doc//documents/Disclaimers/Disclaimers.html
 *   in a browser or see the Privacy &amp; Disclaimers page on the Isis website,
 *   http://isis.astrogeology.usgs.gov, and the USGS privacy and disclaimers on
 *   http://www.usgs.gov/privacy.html.
 */

#include <iostream>
#include <sstream>
#include <iomanip>
#include <vector>

#include "PhotometricFunction.h"
#include "Camera.h"
#include "SpecialPixel.h"

namespace Isis {

  class PvlObject;
  class Camera;
  class Brick;
  class Projection;


  /**
   * @brief An implementation of the HapkeLRO photometric function
   *
   * This class implements the Exponential-Buratti-Hill photometric
   * equation as outline in thier paper "Multispectral Photometry
   * of the Moon and Absolute Calibration of the Clementine UV/VIS
   * Camera", published in Icaris v141, pg. 205-255 (1999).
   *
   * @author  2021-08-18 Cordell Michaud
   * 
   * @internal
   *   @history 2021-07-19 Cordell Michaud - Code adapted from PhotometricFunction and HapkeLRO written by Kris Becker
   *
   */
  class HapkeLRO : public PhotometricFunction {
    public:
      HapkeLRO(PvlObject &pvl, Cube &cube, bool useCamera, Cube *paramMap);
      ~HapkeLRO();
      bool normalized() const;
      double photometry(double i, double e, double g, int band = 1) const override;
      double photometry(double i, double e, double g, double lat, double lon, int band = 1) const;
      void report(PvlContainer &pvl) override;
      void setNormalized(bool normalized);

    private:
      QString m_hfunc;

      bool m_isDegrees;

      bool m_normalized;

      Cube *m_paramMap;
      mutable Projection *m_paramProj;
      mutable int m_currentMapSample, m_currentMapLine, m_currentMapIndex;

      mutable Brick **m_paramBricks;
      int m_paramBrickCount;


      /**
       * Container for band photometric correction parameters
       * 
       * @author 2021-07-30 Cordell Michaud
       * 
       * @internal
       *   @history 2021-07-19 Cordell Michaud - Code adapted from PhotoemtricFunction written by Kris Becker
       */
      class Parameters
      {
        public:
          Parameters() 
            : band(1), bandBinCenter(0.0), mapBands(), 
            names(), phoStd(0.0), values() {}

          ~Parameters() {}

          double operator[] (const QString &name) const {
            for (unsigned int i = 0; i < names.size(); i++) {
              if (names[i] == name) {
                return values[i];
              }
            }

            std::string msg = "No parameter band named '" + name + "'.";
            throw IException(Isis::IException::User, msg, _FILEINFO_);
          }

          int band; // band number
          double bandBinCenter; // center wavelength
          std::vector<int> mapBands; // parameter bands
          std::vector<QString> names; // parameter names
          double phoStd; // computed photometric std
          std::vector<double> values; // parameter values
      };

      mutable std::vector<Parameters> m_bandParameters; // Parameters for each band
      double photometry(Parameters &parms, double i, double e, double g) const;
  };
};

#endif