#ifndef Hillier_h
#define Hillier_h
/**
 * @file
 * $Revision: 1.2 $
 * $Date: 2010/02/24 09:54:18 $
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

#include <iomanip>
#include <iostream>
#include <sstream>
#include <QString>

#include "Camera.h"
#include "DbProfile.h"
#include "IString.h"
#include "SpecialPixel.h"

namespace Isis {

  class PvlObject;
  class Camera;

  /**
   * @brief An implementation of the Hillier photometric function
   *
   * This class implements the Hillier-Buratti-Hill photometric
   * equation as outline in thier paper "Multispectral Photometry
   * of the Moon and Absolute Calibration of the Clementine UV/VIS
   * Camera", published in Icaris v141, pg. 205-255 (1999).
   *
   * @author  2010-02-15 Kris Becker
   *
   * @internal
   *   @history 2010-02-24 Kris Becker - Changed include paths by adding "naif" to NAIF
   *                       includes
   *
   */
  class Hillier {
    public:
      /**
       * @brief Create Hilier photometric object
       *
       */
      Hillier(PvlObject &pvl, Cube &cube);

      //! Destructor
      virtual ~Hillier() {};

      void setCamera(Camera *cam) {
        _camera = cam;
      }
      double Compute(const double &line, const double &sample, int band = 1);
      double photometry(double i, double e, double g, int band = 1) const;
      void Report(PvlContainer &pvl);

    private:
      /**
       * @brief Container for band photometric correction parameters
       *
       * @author Kris Becker - 2/21/2010
       */
      struct Parameters {
        Parameters() : b0(0.0), b1(0.0), a0(0.0), a1(0.0), a2(0.0), a3(0.0),
          a4(0.0), wavelength(0.0), tolerance(0.0),
          units("Degrees"), phaUnit(1.0), band(0), phoStd(0.0),
          iProfile(-1) { }
        ~Parameters() { }
        bool IsValid() const {
          return (iProfile != -1);
        }
        double b0, b1, a0, a1, a2, a3, a4;  //<! Hillier parameters
        double wavelength;                  //<! Wavelength for correction
        double tolerance;                   //<! Wavelenght Range/Tolerance
        QString units;                      //<! Phase units of Hiller eq.
        double phaUnit;  // 1 for degrees, Pi/180 for radians
        int band;                           //<! Cube band parameters
        double phoStd;                      //<! Computed photometric std.
        int iProfile;                       //<! Profile index of this data
      };

      DbProfile _normProf;
      std::vector<DbProfile>  _profiles;
      std::vector<Parameters> _bandpho;
      Camera *_camera;
      double _iRef;  //!<  Incidence refernce angle
      double _eRef;  //  Emission  reference angle
      double _gRef;  //  Phase     reference angle

      double photometry(const Parameters &parms, double i, double e,
                        double g) const;

      Parameters findParameters(const double wavelength) const;
      Parameters extract(const DbProfile &profile) const;
      void init(PvlObject &pvl, Cube &cube);

      /**
       * @brief Helper method to initialize parameters
       *
       * This method will check the existance of a keyword and extract the value
       * if it exists to the passed parameter (type).  If it doesn't exist, the
       * default values is returned.
       *
       * @param T Templated variable type
       * @param conf Parameter profile container
       * @param keyname Name of keyword to get a value from
       * @param defval Default value it keyword/value doesn't exist
       * @param index Optional index of the value for keyword arrays
       *
       * @return T Return type
       */
      template <typename T>
      T ConfKey(const DbProfile &conf, const QString &keyname,
                const T &defval, int index = 0) const {
        if (!conf.exists(keyname)) {
            return defval;
        }
        if (conf.count(keyname) <= index) {
            return defval;
        }

        QString qValue = conf.value(keyname, index);
        std::string iValue = qValue.toStdString();

        // Handle different types of T
        if constexpr (std::is_same_v<T, std::string>) {
            return iValue;
        } else if constexpr (std::is_same_v<T, QString>) {
            return qValue;
        } else if constexpr (std::is_same_v<T, int>) {
            try {
                return Isis::toInt(iValue);
            } catch (...) {
                return defval;
            }
        } else if constexpr (std::is_same_v<T, double>) {
            try {
                return Isis::toDouble(iValue);
            } catch (...) {
                return defval;
            }
        } else {
            static_assert(std::is_same_v<T, void>, "Unsupported type for ConfKey");
        }
    }
  };

};

#endif

