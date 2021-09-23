/** This is free and unencumbered software released into the public domain.

The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */

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
#include "LROCEmpirical.h"
#include "PvlObject.h"

using namespace std;

namespace Isis {

 /**
  * Create an LROCEmpirical photometric object
  *
  * @author 2016-08-15 Victor Silva
  *
  * @internal
  *   @history 2016-08-15 - Code adapted from lrowacpho written by Kris Becker
  */
  LROCEmpirical::LROCEmpirical(PvlObject &pvl, Cube &cube, bool useCamera) :
                 PhotometricFunction(pvl, cube, useCamera) {
    init(pvl, cube);
  }


  /**
   * Destructor
   */
  LROCEmpirical::~LROCEmpirical() {};


  /**
   * @brief Initialize class from input PVL and Cube files
   *
   * This method is typically called at the class instantiation
   * time but is reentrant. It reads the parameter PVL file and
   * extracts Photometric model and Normalization models from it.
   * The cube is needed to match all potential profiles for each
   * band.
   *
   * @param pvl Input PVL parameter files
   * @param cube Input cube file to correct
   *
   * @throws IException::User "Errors in the input PVL file."
   * @throws IException::User "Band with wavelength Center does not have
   *                           PhotometricModel Algorithm group/profile."
   *
   * @author Victor Silva 2016-08-15
   *
   * @internal
   *   @history 2016-08-15 Victor Silva - Adapted from the lrowacpho application
   *                           written by Kris Becker.
   */
  void LROCEmpirical::init( PvlObject &pvl, Cube &cube ) {

    //  Make it reentrant
    m_profiles.clear();
    m_bandpho.clear();

    //  Interate over all Photometric groups
    m_normProf = DbProfile(pvl.findObject("NormalizationModel").findGroup("Algorithm", Pvl::Traverse));
    m_iRef = toDouble(ConfKey(m_normProf, "IncRef", toString(30.0)));
    m_eRef = toDouble(ConfKey(m_normProf, "EmaRef", toString(0.0)));
    m_gRef = toDouble(ConfKey(m_normProf, "PhaRef", toString(m_iRef)));

    PvlObject &phoObj = pvl.findObject("PhotometricModel");
    DbProfile phoProf = DbProfile(phoObj);
    PvlObject::PvlGroupIterator algo = phoObj.beginGroup();

    while (algo != phoObj.endGroup()) {

      if (algo->name().toLower() == "algorithm") {
        m_profiles.push_back(DbProfile(phoProf, DbProfile(*algo)));
      }
      ++algo;
    }

    Pvl *label = cube.label();
    PvlKeyword center = label->findGroup("BandBin", Pvl::Traverse)["Center"];
    QString errs("");

    for (int i = 0; i < cube.bandCount(); i++) {
      Parameters parms = findParameters(toDouble(center[i]));
      if (parms.IsValid()) {
        parms.band = i + 1;
        parms.phoStd = photometry(parms, m_iRef, m_eRef, m_gRef);
        m_bandpho.push_back(parms);
      }
      else { // Appropriate photometric parameters not found
        ostringstream mess;
        mess << "Band [" << i + 1 << "] with wavelength Center = [" << center[i] <<
        "] does not have PhotometricModel Algorithm group/profile";
        IException e(IException::User, mess.str(), _FILEINFO_);
        errs += e.toString() + "\n";
      }
    }

    // Check for errors and throw them all at the same time
    if (!errs.isEmpty()) {
      errs += " --> Errors in the input PVL file [" + pvl.fileName() + "]";
      throw IException(IException::User, errs, _FILEINFO_);
    }

    return;
  }


  /**
   * @brief Method to get photometric property given angles
   *
   * This method computes photometric property at the given cube location after
   * proper parameter container is found for the specific band.
   *
   * @param i    Incidence angle in degrees
   * @param e    Emission angle in degrees
   * @param g    Phase angle in degrees
   * @param band cube band to be tested
   *
   *
   * @throws IException::Programmer "Provided band out of range."
   *
   * @return @b double The photometric property
   *
   * @author 2016-08-15 Victor Silva
   *
   * @internal
   *   @history 2016-08-15 Victor Silva - Adapted code from lrowacpho application
   *                         written by Kris Becker
   */
  double LROCEmpirical::photometry( double i, double e, double g, int band ) const {

    if ((band <= 0) || (band > (int) m_bandpho.size())) {
      std::string mess = "Provided band " + IString(band) + " out of range.";
      throw IException(IException::Programmer, mess, _FILEINFO_);
    }

    double ph = photometry(m_bandpho[band - 1], i, e, g);
    return (m_bandpho[band - 1].phoStd / ph);
  }


  /**
   * @brief Performs actual photometric correction calculations
   *
   * This routine computes photometric correction using parameters for the
   * LROC Emperical equation.
   *
   * @param parms Container of band-specific exponential parameters
   * @param i     Incidence angle in degrees
   * @param e     Emission angle in degrees
   * @param g     Phase angle in degrees
   *
   * @return @b double Photometric correction parameter
   *
   * @author 2016-08-15 Victor Silva
   *
   * @internal
   *   @history 2016-08-15 Victor Silva - Adapted code from lrowacpho application
   *                         written by Kris Becker
   *   @history 2021-03-12 Victor Silva - Added b parameters for 2019 version of
   *            LROC Empirical algorithm
   */
  double LROCEmpirical::photometry( const Parameters &parms, double i, double e, double g ) const {
    //  Ensure problematic values are adjusted
    if (i == 0.0) {
      i = 10.E-12;
    }
    if (e == 0.0) {
      e = 10.E-12;
    }

    // Convert to radians
    i *= rpd_c();
    e *= rpd_c();
    g *= parms.phaUnit; //  Apply unit normalizer

    // Compute Lommel-Seeliger components
    double mu = cos(e);
    double mu0 = cos(i);
    double alpha = g;
    double rcal;

    if (parms.algoVersion == 2014 || parms.algoVersion == 0)
      rcal =  exp(parms.aTerms[0] + parms.aTerms[1] * alpha + parms.aTerms[2] * mu +  parms.aTerms[3] * mu0);
    else if (parms.algoVersion == 2019)
      rcal = mu0 / (mu + mu0) * exp(parms.bTerms[0] + parms.bTerms[1] * (alpha * alpha) + parms.bTerms[2] * alpha + parms.bTerms[3] * sqrt(alpha) + parms.bTerms[4] * mu + parms.bTerms[5] * mu0 + parms.bTerms[6] * (mu0 * mu0) );
    else {
      std::string mess = "Algorithm version in PVL file not recognized [" + IString(parms.algoVersion) + "]. ";
      throw IException(IException::Programmer, mess, _FILEINFO_);
    }

    return (rcal);
  }


  /**
   * @brief Return parameters used for all bands
   *
   * Method creates keyword vector of band-specific parameters
   * used in the photometric correction.
   *
   * @param pvl Output PVL container for keywords written
   *
   * @author 2016-08-15 Victor Silva
   *
   * @internal
   *   @history 2016-08-15 Victor Silva - Adapted code from lrowacpho application
   *                         written by Kris Becker
   *   @history 2021-03-12 Victor Silva - Added b parameters for 2019 version of
   *            LROC Empirical algorithm
   */
  void LROCEmpirical::report( PvlContainer &pvl ) {

    pvl.addComment("I/F = F(mu, mu0,phase)");
    pvl.addComment(" where:");
    pvl.addComment("  mu0 = cos(incidence)");
    pvl.addComment("  mu = cos(emission)");

    if (m_bandpho[0].algoVersion == 2019 )
      pvl.addComment("  F(mu, mu0, phase) = mu0 / (mu + mu0) * exp(B0 + B1 * (alpha * alpha) + B2 * alpha + B3 * sqrt(alpha) + B4 * mu + B5 * mu0 + B6 * (mu0 * mu0) )");
    else if (m_bandpho[0].algoVersion == 2014 || m_bandpho[0].algoVersion == 0)
      pvl.addComment("  F(mu, mu0, phase) = exp (A0 + A1 * phase + A2 * mu + A3 * mu0 ");
    else {
      std::string mess = "Could not file the correction algorithm name.";
      throw IException(IException::Programmer, mess, _FILEINFO_);
    }

    pvl += PvlKeyword("Algorithm", "LROC_Empirical");
    pvl += PvlKeyword("AlgorithmVersion", toString(m_bandpho[0].algoVersion), "" );
    pvl += PvlKeyword("IncRef", toString(m_iRef), "degrees");
    pvl += PvlKeyword("EmaRef", toString(m_eRef), "degrees");
    pvl += PvlKeyword("PhaRef", toString(m_gRef), "degrees");

    PvlKeyword units("FunctionUnits");
    PvlKeyword phostd("PhotometricStandard");
    PvlKeyword bbc("BandBinCenter");
    PvlKeyword bbct("BandBinCenterTolerance");
    PvlKeyword bbn("BandNumber");

    std::vector<PvlKeyword> aTermKeywords;
    std::vector<PvlKeyword> bTermKeywords;
    for (unsigned int i = 0; i < m_bandpho[0].aTerms.size(); i++)
        aTermKeywords.push_back(PvlKeyword("A" + toString((int) i)));
    for (unsigned int i = 0; i < m_bandpho[0].bTerms.size(); i++)
        bTermKeywords.push_back(PvlKeyword("B" + toString((int) i)));

    for (unsigned int i = 0; i < m_bandpho.size(); i++) {
        Parameters &p = m_bandpho[i];
        units.addValue(p.units);
        phostd.addValue(toString(p.phoStd));
        bbc.addValue(toString(p.wavelength));
        bbct.addValue(toString(p.tolerance));
        bbn.addValue(toString(p.band));
        for (unsigned int j = 0; j < aTermKeywords.size(); j++)
          aTermKeywords[j].addValue(toString(p.aTerms[j]));
        for (unsigned int j = 0; j < bTermKeywords.size(); j++)
          bTermKeywords[j].addValue(toString(p.bTerms[j]));
    }

    pvl += units;
    pvl += phostd;
    pvl += bbc;
    pvl += bbct;
    pvl += bbn;

    for (unsigned int i = 0; i < aTermKeywords.size(); i++)
        pvl += aTermKeywords[i];
    for (unsigned int i = 0; i < bTermKeywords.size(); i++)
        pvl += bTermKeywords[i];

    return;
  }


  /**
   * @brief Determine LROC Empirical parameters given a wavewlength
   *
   * This method determines the set of LROCEmpirical parameters to
   * use for a given wavelength. It iterates through all band profiles
   * as read from the PVL file and computes the difference between
   * wavelength parameter and the BandBinCenter keyword. The absolute
   * value of this value is check against the BandBinCenterTolerance
   * parameter and if it is less than or equal to it, a Parameter
   * container is returned.
   *
   * @param wavelength The wavelength to find parameters for
   *
   * @return @b LROEmpirical::parameters The parameters used for the specified
   *                                     wavelength. If a BandBinCenter is not
   *                                     found for the wavelength then a
   *                                     default parameters object is returned.
   *
   * @author 2016-08-15 Victor Silva
   *
   * @internal
   *   @history 2016-08-15 Victor Silva - Adapted from the lrowacpho application
   *                           written by Kris Becker.
   */
  LROCEmpirical::Parameters LROCEmpirical::findParameters( const double wavelength ) const {

    for (unsigned int i = 0; i < m_profiles.size(); i++) {
      const DbProfile &profile = m_profiles[i];

      if (profile.exists("BandBinCenter")) {
        double p_center = toDouble(ConfKey(profile, "BandBinCenter", toString(Null)));
        double tolerance = toDouble(ConfKey(profile, "BandBinCenterTolerance", toString(1.0E-6)));

        if (fabs(wavelength - p_center) <= fabs(tolerance)) {
          Parameters pars = extract(profile);
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



  /*
   * @brief Extracts necessary LROCEmprical parameters from profile
   *
   * Given a profile read from the input PVL file, this method extracts
   * needed parameters (from keywords) in the PVL profile and creates
   * a container of the converted values
   *
   * @param profile The profile to extract parameters for
   *
   * @return @b LROCEmpirical::parameters
   *
   * @author Victor Silva 2016-08-15
   *
   * @internal
   *   @history 2016-08-15 Victor Silva - Adapted from the lrowacpho application
   *            written by Kris Becker.
   *
   *   @history 2021-03-12 Victor Silva - Added b parameters for 2019 version of
   *            LROC Empirical algorithm
   *
   */
  LROCEmpirical::Parameters LROCEmpirical::extract( const DbProfile &profile) const {
    Parameters pars;

    for (int i=0; i<4; i++)
        pars.aTerms.push_back(toDouble(ConfKey(profile, "A" + toString(i), toString(0.0))));
    for (int i=0; i<7; i++)
        pars.bTerms.push_back(toDouble(ConfKey(profile, "B" + toString(i), toString(0.0))));

    pars.wavelength = toDouble(ConfKey(profile, "BandBinCenter", toString(Null)));
    pars.tolerance = toDouble(ConfKey(profile, "BandBinCenterTolerance", toString(Null)));
    //  Determine equation units - defaults to Radians
    pars.units = ConfKey(profile, "Units", QString("Radians"));
    pars.phaUnit = (pars.units.toLower() == "degrees") ? 1.0 : rpd_c();
    pars.algoVersion = toInt(ConfKey(profile, "AlgorithmVersion", QString("0")));

    return (pars);
  }
}
