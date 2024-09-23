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
#include "HapkeExponential.h"
#include "PvlObject.h"

using namespace std;

namespace Isis {

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
    double HapkeExponential::photometry ( double i, double e, double g, int band ) const {
        // Test for valid band
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
     * This routine computes photometric correction using parameters
     * for the Exponential-Buratti-Hill equation.
     *
     * @author Kris Becker - 2/21/2010
     *
     * @param parms Container of band-specific HapkeExponential parameters
     * @param i     Incidence angle in degrees
     * @param e     Emission angle in degrees
     * @param g     Phase angle in degrees
     *
     * @return double Photometric correction parameter
     */
    double HapkeExponential::photometry ( const Parameters &parms, double i, double e, double g ) const {
        //  Ensure problematic values are adjusted
        if (i == 0.0)
            i = 10.E-12;
        if (e == 0.0)
            e = 10.E-12;

        // Convert to radians
        i *= rpd_c();
        e *= rpd_c();
        g *= parms.phaUnit; //  Apply unit normalizer

        // Compute Lommel-Seeliger components
        double mu = cos(e);
        double mu0 = cos(i);

        double alpha = g;

        // Simple HapkeExponential photometric polynomial equation with exponential opposition
        // surge term.

        // I   µ0 + µ
        // _ * ______  = A1 exp(B1 phase) + A2 ( µ0 + µ )exp(B2 phase) + A3 ( µ0 + µ ) + A4
        // F    µ0


        double rcal = parms.aTerms[0]*exp(parms.bTerms[0] * alpha);
        rcal += parms.aTerms[1]*(mu0 + mu)*exp(parms.bTerms[1] * alpha);
        rcal += parms.aTerms[2]*(mu0 + mu);
        rcal += parms.aTerms[3];

        return rcal*mu0/(mu0+mu);
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
    void HapkeExponential::report ( PvlContainer &pvl ) {
        pvl.addComment("I/F = mu0/(mu0+mu) * F(mu,mu0,phase)");
        pvl.addComment("  where:");
        pvl.addComment("    mu0 = cos(incidence)");
        pvl.addComment("    mu = cos(emission)");
        pvl.addComment("    F(mu,mu0,phase) = A1*exp(B1*phase) + A2*( mu0 + mu )exp(B2*phase) + A3*( mu0 + mu ) + A4");

        pvl += PvlKeyword("Algorithm", "HapkeExponential");
        pvl += PvlKeyword("IncRef", Isis::toString(m_iRef), "degrees");
        pvl += PvlKeyword("EmaRef", Isis::toString(m_eRef), "degrees");
        pvl += PvlKeyword("PhaRef", Isis::toString(m_gRef), "degrees");
        PvlKeyword units("HapkeExponentialUnits");
        PvlKeyword phostd("PhotometricStandard");
        PvlKeyword bbc("BandBinCenter");
        PvlKeyword bbct("BandBinCenterTolerance");
        PvlKeyword bbn("BandNumber");

        std::vector<PvlKeyword> aTermKeywords;
        std::vector<PvlKeyword> bTermKeywords;
        for (unsigned int i = 0; i < m_bandpho[0].aTerms.size(); i++)
            aTermKeywords.push_back(PvlKeyword("A" + Isis::toString((int) i+1)));
        for (unsigned int i = 0; i < m_bandpho[0].bTerms.size(); i++)
            bTermKeywords.push_back(PvlKeyword("B" + Isis::toString((int) i+1)));

        for (unsigned int i = 0; i < m_bandpho.size(); i++) {
            Parameters &p = m_bandpho[i];
            units.addValue(p.units.toStdString());
            phostd.addValue(Isis::toString(p.phoStd));
            bbc.addValue(Isis::toString(p.wavelength));
            bbct.addValue(Isis::toString(p.tolerance));
            bbn.addValue(Isis::toString(p.band));
            for (unsigned int j = 0; j < aTermKeywords.size(); j++)
                aTermKeywords[j].addValue(Isis::toString(p.aTerms[j]));
            for (unsigned int j = 0; j < bTermKeywords.size(); j++)
                bTermKeywords[j].addValue(Isis::toString(p.bTerms[j]));
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
     * @brief Determine HapkeExponential parameters given a wavelength
     *
     * This method determines the set of HapkeExponential parameters to use
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
     * @return HapkeExponential::Parameters Container of valid values.  If
     *         not found, a value of iProfile = -1 is returned.
     */
    HapkeExponential::Parameters HapkeExponential::findParameters ( const double wavelength ) const {
        for (unsigned int i = 0; i < m_profiles.size(); i++) {
            const DbProfile &p = m_profiles[i];
            if (p.exists("BandBinCenter")) {
                double p_center = ConfKey(p, "BandBinCenter", QString::number(Null)).toDouble();
                double tolerance = ConfKey(p, "BandBinCenterTolerance", QString::number(1.0E-6)).toDouble();
                if (fabs(wavelength - p_center) <= fabs(tolerance)) {
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
     * @brief Extracts necessary HapkeExponential parameters from profile
     *
     * Given a profile read from the input PVL file, this method
     * extracts needed parameters (from Keywords) in the PVL profile
     * and creates a container of the converted values.
     *
     * @author Kris Becker - 2/22/2010
     *
     * @param p Profile to extract/convert
     *
     * @return HapkeExponential::Parameters Container of extracted values
     */
    HapkeExponential::Parameters HapkeExponential::extract ( const DbProfile &p ) const {
        Parameters pars;

        for (int i=1; i<=4; i++)
            pars.aTerms.push_back(ConfKey(p, "A" + QString::number(i), QString::number(0.0)).toDouble());

        for (int i=1; i<=2; i++)
            pars.bTerms.push_back(ConfKey(p, "B" + QString::number(i), QString::number(0.0)).toDouble());

        pars.wavelength = ConfKey(p, "BandBinCenter", QString::number(Null)).toDouble();
        pars.tolerance = ConfKey(p, "BandBinCenterTolerance", QString::number(Null)).toDouble();
        //  Determine equation units - defaults to Radians
        pars.units = ConfKey(p, "HapkeExponentialUnits", QString("Radians"));
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
     * @author Kris Becker - 2/22/2010
     *
     * @param pvl  Input PVL parameter files
     * @param cube Input cube file to correct
     */
    void HapkeExponential::init ( PvlObject &pvl, Cube &cube ) {
        //  Make it reentrant
        m_profiles.clear();
        m_bandpho.clear();

        //  Interate over all Photometric groups
        m_normProf = DbProfile(pvl.findObject("NormalizationModel").findGroup("Algorithm", Pvl::Traverse));
        m_iRef = ConfKey(m_normProf, "IncRef", QString::number(30.0)).toDouble();
        m_eRef = ConfKey(m_normProf, "EmaRef", QString::number(0.0)).toDouble();
        m_gRef = ConfKey(m_normProf, "PhaRef", QString::number(m_iRef)).toDouble();

        PvlObject &phoObj = pvl.findObject("PhotometricModel");
        DbProfile phoProf = DbProfile(phoObj);
        PvlObject::PvlGroupIterator algo = phoObj.beginGroup();
        while (algo != phoObj.endGroup()) {
            if (QString::fromStdString(algo->name()).toLower() == "algorithm") {
                m_profiles.push_back(DbProfile(phoProf, DbProfile(*algo)));
            }
            ++algo;
        }

        Pvl *label = cube.label();
        PvlKeyword center = label->findGroup("BandBin", Pvl::Traverse)["Center"];
        std::string errs("");
        for (int i = 0; i < cube.bandCount(); i++) {
            Parameters parms = findParameters(Isis::toDouble(center[i]));
            if (parms.IsValid()) {
                parms.band = i + 1;
                //_camera->SetBand(i + 1);
                parms.phoStd = photometry(parms, m_iRef, m_eRef, m_gRef);
                m_bandpho.push_back(parms);
            }
            else { // Appropriate photometric parameters not found
                ostringstream mess;
                mess << "Band " << i + 1 << " with wavelength Center = " << center[i]
                        << " does not have PhotometricModel Algorithm group/profile";
                IException e(IException::User, mess.str(), _FILEINFO_);
                errs += e.toString() + "\n";
            }
        }

        // Check for errors and throw them all at the same time
        if (!errs.empty()) {
            errs += " --> Errors in the input PVL file \"" + pvl.fileName() + "\"";
            throw IException(IException::User, errs, _FILEINFO_);
        }

        return;
    }

} // namespace Isis
