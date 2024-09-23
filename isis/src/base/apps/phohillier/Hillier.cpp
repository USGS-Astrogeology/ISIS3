/**
 * @file
 * $Revision: 1.3 $
 * $Date: 2010/02/25 18:39:05 $
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
#include <cmath>
#include <cstdlib>
#include <iomanip>
#include <iostream>
#include <memory>
#include <sstream>

#include <SpiceUsr.h>
#include <SpiceZfc.h>
#include <SpiceZmc.h>

#include "Camera.h"
#include "DbProfile.h"
#include "Hillier.h"
#include "PvlObject.h"

using namespace std;

namespace Isis {

  /**
   * @brief Construct from PVL and Cube file
   *
   * @author Kris Becker - 2/21/2010
   *
   * @param pvl Photometric parameter files
   * @param cube Input cube file
   */
  Hillier::Hillier(PvlObject &pvl, Cube &cube) {
    _camera = cube.camera();
    init(pvl, cube);
  }
  /**
   * @brief Compute photometric DN at given line/sample/band
   *
   * This routine applies the photometric angles to the equation
   * and returns the calibration coefficient at the given  cube
   * location.
   *
   * The return parameter is the photometric standard/photometric
   * correction coefficient at the given pixel location.
   *
   * @author Kris Becker - 2/21/2010
   *
   * @param line   Line of cube image to compute photometry
   * @param sample Sample of cube image to compute photometry
   * @param band   Band of cube image to compute photometry
   *
   * @return double Photometric correction at cube loation
   */
  double Hillier::Compute(const double &line, const double &sample, int band) {

    // Update band if necessary
    if(_camera->Band() != band) {
      _camera->SetBand(band);
    }
    if(!_camera->SetImage(sample, line))  return (Null);

    double i = _camera->IncidenceAngle();
    double e = _camera->EmissionAngle();
    double g = _camera->PhaseAngle();
    if(i >= 90.0) return (Null);

    return(photometry(i, e, g, band));
  }

  /**
   * @brief Method to get photometric property given angles
   *
   * This routine computes the photometric property at the given
   * cube location after ensuring a proper parameter container is
   * found for the specified band.
   *
   * @author Kris Becker - 2/21/2010
   *
   * @param i     Incidence angle at cube location
   * @param e     Emission angle at cube location
   * @param g     Phase angle at cube location
   * @param band  Band number in cube (actually is band index) for
   *              lookup purposes
   *
   * @return double Returns photometric correction using
   *         parameters
   */
  double Hillier::photometry(double i, double e, double g, int band) const {
    // Test for valid band
    if((band <= 0) || (band > (int) _bandpho.size())) {
      std::string mess = "Provided band " + toString(band) + " out of range.";
      throw IException(IException::Programmer, mess, _FILEINFO_);
    }
    double ph = photometry(_bandpho[band-1], i, e, g);
    return (_bandpho[band-1].phoStd / ph);
  }

  /**
   * @brief Performs actual photometric correction calculations
   *
   * This routine computes photometric correction using parameters
   * for the Hillier-Buratti-Hill equation.
   *
   * @author Kris Becker - 2/21/2010
   *
   * @param parms Container of band-specific Hillier parameters
   * @param i     Incidence angle in degrees
   * @param e     Emission angle in degrees
   * @param g     Phase angle in degrees
   *
   * @return double Photometric correction parameter
   */
  double Hillier::photometry(const Parameters &parms, double i, double e,
                             double g) const {

    //  Ensure problematic values are adjusted
    if(i == 0.0) i = 10.E-12;
    if(e == 0.0) e = 10.E-12;

    // Convert to radians
    i *= rpd_c();
    e *= rpd_c();
    g *= parms.phaUnit;  //  Apply unit normalizer

    // Compute Lommel-Seeliger components
    double mu = cos(e);
    double mu0 = cos(i);

    double alpha = g;
    double alpha2 = g * g;

    // Simple Hillier photometric polynomial equation with exponential opposition
    //  surge term.
    double rcal = (mu0 / (mu + mu0)) * (parms.b0 * exp(-parms.b1 * alpha) + parms.a0 +
                                        (parms.a1 * alpha) + (parms.a2 * alpha2) +
                                        (parms.a3 * alpha * alpha2) +
                                        (parms.a4 * alpha2 * alpha2));
    return (rcal);
  }


  /**
   * @brief Return parameters used for all bands
   *
   * Method creates keyword vectors of band specific parameters
   * used in the photometric correction.
   *
   * @author Kris Becker - 2/22/2010
   *
   * @param pvl Output PVL container write keywords
   */
  void Hillier::Report(PvlContainer &pvl) {
    pvl += PvlKeyword("Algorithm", "Hillier");
    pvl += PvlKeyword("IncRef", toString(_iRef), "degrees");
    pvl += PvlKeyword("EmaRef", toString(_eRef), "degrees");
    pvl += PvlKeyword("PhaRef", toString(_gRef), "degrees");
    PvlKeyword units("HillierUnits");
    PvlKeyword phostd("PhotometricStandard");
    PvlKeyword bbc("BandBinCenter");
    PvlKeyword bbct("BandBinCenterTolerance");
    PvlKeyword bbn("BandNumber");
    PvlKeyword b0("B0");
    PvlKeyword b1("B1");
    PvlKeyword a0("A0");
    PvlKeyword a1("A1");
    PvlKeyword a2("A2");
    PvlKeyword a3("A3");
    PvlKeyword a4("A4");
    for(unsigned int i = 0 ; i < _bandpho.size() ; i++) {
      Parameters &p = _bandpho[i];
      units.addValue(p.units.toStdString());
      phostd.addValue(toString(p.phoStd));
      bbc.addValue(toString(p.wavelength));
      bbct.addValue(toString(p.tolerance));
      bbn.addValue(toString(p.band));
      b0.addValue(toString(p.b0));
      b1.addValue(toString(p.b1));
      a0.addValue(toString(p.a0));
      a1.addValue(toString(p.a1));
      a2.addValue(toString(p.a2));
      a3.addValue(toString(p.a3));
      a4.addValue(toString(p.a4));
    }
    pvl += units;
    pvl += phostd;
    pvl += bbc;
    pvl += bbct;
    pvl += bbn;
    pvl += b0;
    pvl += b1;
    pvl += a0;
    pvl += a1;
    pvl += a2;
    pvl += a3;
    pvl += a4;
    return;
  }


  /**
   * @brief Determine Hillier parameters given a wavelength
   *
   * This method determines the set of Hillier parameters to use
   * for a given wavelength.  It iterates through all band
   * profiles as read from the PVL file and computes the
   * difference between the "wavelength" parameter and the
   * BandBinCenter keyword.  The absolute value of this value is
   * checked against the BandBinCenterTolerance paramter and if it
   * is less than or equal to it, a Parameter container is
   * returned.
   *
   * @author Kris Becker - 2/22/2010
   *
   * @param wavelength Wavelength used to find parameter set
   *
   * @return Hillier::Parameters Container of valid values.  If
   *         not found, a value of iProfile = -1 is returned.
   */
  Hillier::Parameters Hillier::findParameters(const double wavelength) const {
    for(unsigned int i = 0 ; i < _profiles.size() ; i++) {
      const DbProfile &p = _profiles[i];
      if(p.exists("BandBinCenter")) {
        double p_center = toDouble(ConfKey(p, "BandBinCenter", toString(Null)));
        double tolerance = toDouble(ConfKey(p, "BandBinCenterTolerance", toString(1.0E-6)));
        if(fabs(wavelength - p_center) <= fabs(tolerance)) {
          Parameters pars = extract(p);
          pars.iProfile = i;
          pars.wavelength = wavelength;
          pars.tolerance = tolerance;
          return (pars);
        }
      }
    }

    // Not found if we reach here
    return (Parameters());
  }

  /**
   * @brief Extracts necessary Hillier parameters from profile
   *
   * Given a profile read from the input PVL file, this method
   * extracts needed parameters (from Keywords) in the PVL profile
   * and creates a container of the converted values.
   *
   * @author Kris Becker - 2/22/2010
   *
   * @param p Profile to extract/convert
   *
   * @return Hillier::Parameters Container of extracted values
   */
  Hillier::Parameters Hillier::extract(const DbProfile &p) const {
    Parameters pars;
    pars.b0 = toDouble(ConfKey(p, "B0", toString(0.0)));
    pars.b1 = toDouble(ConfKey(p, "B1", toString(0.0)));
    pars.a0 = toDouble(ConfKey(p, "A0", toString(0.0)));
    pars.a1 = toDouble(ConfKey(p, "A1", toString(0.0)));
    pars.a2 = toDouble(ConfKey(p, "A2", toString(0.0)));
    pars.a3 = toDouble(ConfKey(p, "A3", toString(0.0)));
    pars.a4 = toDouble(ConfKey(p, "A4", toString(0.0)));
    pars.wavelength = toDouble(ConfKey(p, "BandBinCenter", toString(Null)));
    pars.tolerance = toDouble(ConfKey(p, "BandBinCenterTolerance", toString(Null)));
    //  Determine equation units - defaults to Radians
    pars.units = ConfKey(p, "HillierUnits", QString("Radians"));
    pars.phaUnit = (pars.units.toLower() == "degrees") ? 1.0 : rpd_c();
    return (pars);
  }

  /**
   * @brief Initialize class from input PVL and Cube files
   *
   * This method is typically called at class instantiation time,
   * but is reentrant.  It reads the parameter PVL file and
   * extracts Photometric model and Normalization models from it.
   * The cube is needed to match all potential profiles for each
   * band.
   *
   * @param pvl  Input PVL parameter files
   * @param cube Input cube file to correct
   *
   * @author Kris Becker - 2/22/2010
   * @history 2010-02-25 Kris Becker Added check for valid incidence angle
   */
  void Hillier::init(PvlObject &pvl, Cube &cube) {

    //  Make it reentrant
    _profiles.clear();
    _bandpho.clear();

    //  Interate over all Photometric groups
    _normProf = DbProfile(pvl.findObject("NormalizationModel").findGroup("Algorithm", Pvl::Traverse));
    _iRef = toDouble(ConfKey(_normProf, "IncRef", toString(30.0)));
    _eRef = toDouble(ConfKey(_normProf, "EmaRef", toString(0.0)));
    _gRef = toDouble(ConfKey(_normProf, "PhaRef", toString(_iRef)));

    // Check for valid incidence angle
    if(_iRef > fabs(90.0)) {
      ostringstream mess;
      mess << "Invalid incidence angle (" << _iRef
           << " >= 90.0) provided in PVL config file " << pvl.fileName();
      throw IException(IException::User, mess.str(), _FILEINFO_);
    }


    PvlObject &phoObj = pvl.findObject("PhotometricModel");
    DbProfile phoProf = DbProfile(phoObj);
    PvlObject::PvlGroupIterator algo = phoObj.beginGroup();
    while(algo != phoObj.endGroup()) {
      if(QString::fromStdString(algo->name()).toLower() == "algorithm") {
        _profiles.push_back(DbProfile(phoProf, DbProfile(*algo)));
      }
      ++algo;
    }

    Pvl *label = cube.label();
    PvlKeyword center = label->findGroup("BandBin", Pvl::Traverse)["Center"];
    std::string errs("");
    for(int i = 0; i < cube.bandCount() ; i++) {
      Parameters parms = findParameters(IString::ToDouble(center[i]));
      if(parms.IsValid()) {
        parms.band = i + 1;
        _camera->SetBand(i + 1);
        parms.phoStd = photometry(parms, _iRef, _eRef, _gRef);
        _bandpho.push_back(parms);
      }
      else {  // Appropriate photometric parameters not found
        ostringstream mess;
        mess << "Band " << i + 1 << " with wavelength Center = " << center[i]
             << " does not have PhotometricModel Algorithm group/profile";
        IException e(IException::User, mess.str(), _FILEINFO_);
        errs += e.toString() + "\n";
      }
    }

    // Check for errors and throw them all at the same time
    if(!errs.empty()) {
      errs += " --> Errors in the input PVL file \"" + pvl.fileName() + "\"";
      throw IException(IException::User, errs, _FILEINFO_);
    }

    return;
  }

} // namespace Isis


