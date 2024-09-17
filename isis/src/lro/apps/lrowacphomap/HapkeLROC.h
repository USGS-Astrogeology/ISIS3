#ifndef hapkelroc_h
#define hapkelroc_h
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

#include "PhotometricFunction.h"

namespace Isis {
  class PvlObject;
  class Camera;
  class Brick;
  class Projection;


  /**
   * @brief An implementation of the bidirectional reflectance model by Hapke (2012).
   *
   * This class is based on the Hapke PhotoModel from Isis base code. Photometry method
   * was updated to match full Hapke (2012) model and some variable names were changed
   * to match reference formula.
   * 
   * Reference:
   *    Hapke, B. (2012) Theory of Reflectance and Emittance Spectroscopy, Cambridge Univ. Press.
   *
   * @author 2021-08-18 Cordell Michaud
   * @internal
   *   @history 2021-07-19 Cordell Michaud - Code adapted from HapkeLROC written by Dan Clarke
   *   @history 2021-07-19 Cordell Michaud - Code adapted from PhotometricFunction written by Kris Becker
   *
   */
  class HapkeLROC : public PhotometricFunction {
    public:
      HapkeLROC(PvlObject &pvl, Cube &cube, bool useCamera, Cube *paramMap);
      ~HapkeLROC();

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

      mutable double m_chiThetaP;
      mutable double m_invChiThetaP;
      mutable double m_oldPhase;
      mutable double m_oldIncidence;
      mutable double m_oldEmission;
      mutable double m_photoCot2t;
      mutable double m_photoCott;
      mutable double m_photoTant;
      mutable double m_photoThetaold;

      mutable double m_result;


      /**
        * Container for band photometric correction parameters
        * 
        * @author 2021-08-02 Cordell Michaud
        * 
        * @internal
        *   @history 2021-07-19 Cordell Michaud - Code adapted from PhotometricFunction written by Kris Becker
        */
      class Parameters {
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

            std::string msg = "No parameter band named '" + name.toStdString() + "'.";
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
      double photometry(const Parameters &parms, double i, double e, double g) const;
  };

};

#endif