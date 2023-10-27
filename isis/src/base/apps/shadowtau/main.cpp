#include "Isis.h"

#include <cmath>
#include <fstream>
#include <iostream>
#include <string>
#include <set>
#include <vector>

// Gnu Scientific Library
#include <gsl/gsl_errno.h>
#include <gsl/gsl_math.h>
#include <gsl/gsl_min.h>

#include "AtmosModel.h"
#include "AtmosModelFactory.h"
#include "CSVReader.h"
#include "IException.h"
#include "Photometry.h"
#include "PhotoModel.h"
#include "PhotoModelFactory.h"
#include "TextFile.h"

using namespace Isis;
using namespace std;

enum imgdata {eimgid, einc, eema, ephase, epflat, epshad } ;

// structure to send parameters to the routine stan_err to
// accomodate prototype for gsl_function
struct StanData {
  AtmosModel* m_AtmModel;
  double m_pstd;
  double m_trans;
  double m_trans0;
  double m_sbar;
  double m_inc;
  double m_ema;
  double m_phase;
  double m_pshad;
  double m_pflat;
  double m_transs;
  double m_rho;
};

// structure to send parameters to the routine shad_err to
// accomodate prototype for gsl_function
struct ShadData {
  AtmosModel* m_AtmModel;
  double m_pstd;
  double m_trans;
  double m_trans0;
  double m_sbar;
  double m_inc;
  double m_ema;
  double m_phase;
  double m_pshad;
  double m_pflat;
  double m_psurf;
  double m_ahi;
  double m_munot;
  double m_transs;
  double m_rho;
};

double stan_err(double tau_guess, void * pParams);
double shad_err(double tau_guess, void * pParams);

void IsisMain() {
  UserInterface &ui = Application::GetUserInterface();

  // Input datafile
  QString sInFile = ui.GetFileName("FROM");

  // Output datafile
  QString sOutFile = ui.GetFileName("TO");
  ofstream outfile;
  outfile.open (sOutFile.toLatin1().data());

  // Get Hapke function and parameters
  QString sPhotoFunc = ui.GetAsString("PHTNAME");
  sPhotoFunc = sPhotoFunc.toUpper();

  // Should contains parameter names matching GUI not to be inclusiond
  // in the Pvl defFile
  Pvl photoPvl;
  photoPvl.addObject(PvlObject("PhotometricModel"));
  photoPvl.findObject("PhotometricModel").addGroup(PvlGroup("Algorithm"));
  photoPvl.findObject("PhotometricModel").findGroup("Algorithm").
           addKeyword(PvlKeyword("PHTNAME",sPhotoFunc.toStdString()),Pvl::Replace);

  if (sPhotoFunc == "HAPKEHEN" || sPhotoFunc == "HAPKELEG") { // Single Particle Phase Function HENYEY-GREENSTEIN
    if (ui.WasEntered("WH")) {
      QString keyval = ui.GetString("WH");
      double wh = toDouble(keyval);
      photoPvl.findObject("PhotometricModel").findGroup("Algorithm").
               addKeyword(PvlKeyword("WH",std::to_string(wh)),Pvl::Replace);
    }
    if (ui.WasEntered("HH")) {
      QString keyval = ui.GetString("HH");
      double hh = toDouble(keyval);
      photoPvl.findObject("PhotometricModel").findGroup("Algorithm").
               addKeyword(PvlKeyword("HH",std::to_string(hh)),Pvl::Replace);
    }
    if (ui.WasEntered("B0")) {
      QString keyval = ui.GetString("B0");
      double b0 = toDouble(keyval);
      photoPvl.findObject("PhotometricModel").findGroup("Algorithm").
               addKeyword(PvlKeyword("B0",std::to_string(b0)),Pvl::Replace);
    }
    if (ui.WasEntered("THETA")) {
      QString keyval = ui.GetString("THETA");
      double theta = toDouble(keyval);
      photoPvl.findObject("PhotometricModel").findGroup("Algorithm").
               addKeyword(PvlKeyword("THETA",std::to_string(theta)),Pvl::Replace);
    }
    if (ui.WasEntered("ZEROB0STANDARD")) {
      QString keyval = ui.GetString("ZEROB0STANDARD");
      keyval = keyval.toUpper();
      photoPvl.findObject("PhotometricModel").findGroup("Algorithm").
               addKeyword(PvlKeyword("ZEROB0STANDARD",keyval.toStdString()),Pvl::Replace);
    }
    if (sPhotoFunc == "HAPKEHEN") {
      if (ui.WasEntered("HG1")) {
        QString keyval = ui.GetString("HG1");
        double hg1 = toDouble(keyval);
        photoPvl.findObject("PhotometricModel").findGroup("Algorithm").
                 addKeyword(PvlKeyword("HG1",std::to_string(hg1)),Pvl::Replace);
      }
      if (ui.WasEntered("HG2")) {
        QString keyval = ui.GetString("HG2");
        double hg2 = toDouble(keyval);
        photoPvl.findObject("PhotometricModel").findGroup("Algorithm").
                 addKeyword(PvlKeyword("HG2",std::to_string(hg2)),Pvl::Replace);
      }
    }
    else {
      if (ui.WasEntered("BH")) {
        QString keyval = ui.GetString("BH");
        double bh = toDouble(keyval);
        photoPvl.findObject("PhotometricModel").findGroup("Algorithm").
                 addKeyword(PvlKeyword("BH",std::to_string(bh)),Pvl::Replace);
      }
      if (ui.WasEntered("CH")) {
        QString keyval = ui.GetString("CH");
        double ch = toDouble(keyval);
        photoPvl.findObject("PhotometricModel").findGroup("Algorithm").
                 addKeyword(PvlKeyword("CH",std::to_string(ch)),Pvl::Replace);
      }
    }
  }
  else if (sPhotoFunc == "LUNARLAMBERT") {  // Single Particle Phase Function LEGENDRE
    if (ui.WasEntered("L")) {
      QString keyval = ui.GetString("L");
      double l = toDouble(keyval);
      photoPvl.findObject("PhotometricModel").findGroup("Algorithm").
               addKeyword(PvlKeyword("L",std::to_string(l)),Pvl::Replace);
    }
  }
  else if (sPhotoFunc == "MINNAERT") {
    if (ui.WasEntered("K")) {
      QString keyval = ui.GetString("K");
      double k = toDouble(keyval);
      photoPvl.findObject("PhotometricModel").findGroup("Algorithm").
               addKeyword(PvlKeyword("K",std::to_string(k)),Pvl::Replace);
    }
  }

  PhotoModel *photoModel = NULL;
  photoModel = PhotoModelFactory::Create(photoPvl);

  // Log the Hapke Def File
  PvlGroup hapkeGrp = photoPvl.findObject("PhotometricModel").findGroup("Algorithm");
  Application::Log(hapkeGrp);

  //datafile':get data file only for photometric functions that need it
  if (sPhotoFunc == "LUNARLAMBERTEMPIRICAL" || sPhotoFunc == "MINNAERTEMPIRICAL") {
    QString sDataFile =  ui.GetFileName("DATAFILE");
  }

  // Get Atmospheric Model
  // Order of approximation in atmospheric scatter model
  QString sAsmType = ui.GetAsString("ATMNAME");
  sAsmType = sAsmType.toUpper();

  Pvl asmPvl;
  asmPvl.addObject(PvlObject("AtmosphericModel"));
  asmPvl.findObject("AtmosphericModel").addGroup(PvlGroup("Algorithm"));
  asmPvl.findObject("AtmosphericModel").findGroup("Algorithm").
           addKeyword(PvlKeyword("ATMNAME",sAsmType.toStdString()),Pvl::Replace);

  if (ui.WasEntered("WHA")) {
    QString keyval = ui.GetString("WHA");
    double wha = toDouble(keyval);
    asmPvl.findObject("AtmosphericModel").findGroup("Algorithm").
           addKeyword(PvlKeyword("WHA",std::to_string(wha)),Pvl::Replace);
  }
  if (ui.WasEntered("HNORM")) {
    QString keyval = ui.GetString("HNORM");
    double hnorm = toDouble(keyval);
    asmPvl.findObject("AtmosphericModel").findGroup("Algorithm").
           addKeyword(PvlKeyword("HNORM",std::to_string(hnorm)),Pvl::Replace);
  }

  if (sAsmType=="ANISOTROPIC1" || sAsmType=="ANISOTROPIC2" ){
    if (ui.WasEntered("BHA")) {
      QString keyval = ui.GetString("BHA");
      double bha = toDouble(keyval);
      asmPvl.findObject("AtmosphericModel").findGroup("Algorithm").
             addKeyword(PvlKeyword("BHA",std::to_string(bha)),Pvl::Replace);
    }
  }
  else if (sAsmType=="HAPKEATM1" || sAsmType=="HAPKEATM2" ) {
    if (ui.WasEntered("HGA")) {
      QString keyval = ui.GetString("HGA");
      double hga = toDouble(keyval);
      asmPvl.findObject("AtmosphericModel").findGroup("Algorithm").
             addKeyword(PvlKeyword("HGA",std::to_string(hga)),Pvl::Replace);
    }
  }

  AtmosModel *asmModel=NULL;

  // Make sure that optical depth estimation is turned on
  asmPvl.findObject("AtmosphericModel").findGroup("Algorithm").addKeyword(PvlKeyword("EstTau", "YES"), Pvl::Replace);

  // Log the AtmosphericModel Def File
  PvlGroup asmGrp = asmPvl.findObject("AtmosphericModel").findGroup("Algorithm");
  Application::Log(asmGrp);

  asmModel = AtmosModelFactory::Create(asmPvl, *photoModel);
  if (asmModel == NULL) {
    QString errMsg = "Unable create an Atmospheric Model\n";
    throw IException(IException::User, errMsg, _FILEINFO_);
  }

  double increff  = 0;
  double emaref   = 0;
  double phaseref = increff;
  photoModel->SetStandardConditions(true);
  double psurfref = photoModel->CalcSurfAlbedo(phaseref, increff, emaref);
  asmModel->GenerateAhTable();
  photoModel->SetStandardConditions(false);

  // Loop to calculate optical depth and surface I/F for various input data
  // Start loop by getting new angles and brightnesses to model
  // Prototype uses Kirk's buffered I/O routines.  The input file line
  // is read as a string, parsed into "words" (separated by white space
  // or commas) and then parameters read from the words.  The line is
  // rejected if there aren't enough words or if any of them don't make
  // sense as the corresponding parameter.
  FileName sInFileName(sInFile);
  // Try with the default ',' delimiter, if that only produces one row
  // item, try with spaces
  CSVReader inFile(sInFileName.expanded());
  if (inFile.getRow(0).dim() <= 1) {
    inFile = CSVReader(sInFileName.expanded(), false, 0, ' ');
  }

  if (inFile.getRow(0).dim() <= 1) {
    QString msg = "File [" + sInFileName.expanded() + "] either has only one line item or is not delimited by a ',' or ' '.";
    throw IException(IException::User, msg, _FILEINFO_);
  }

  QString infileString;
  for (int i = 0; i < inFile.rows(); i++) {
    CSVReader::CSVAxis row = inFile.getRow(i);

    if (row.dim1() < 6) {
      continue;
    }

    QString imgId = row[0];
    std::vector<double> angles = {};
    for (int j = 1; j < row.dim(); j++) {
      try {
        angles.push_back(toDouble(row[j]));
      }
      catch (IException &e) {
        QString msg = "Unable to convert (" + toString(i) + ", " + toString(j) + 
        ") element [" + row[j] + "] to double. You may want to check for excess delimiters." + 
        "Current delimiter is set to '" + inFile.getDelimiter() + "'";
        throw IException(IException::User, msg, _FILEINFO_);
      }
    }

    double inc    = angles[0];
    double ema    = angles[1];
    double phase  = angles[2];
    double pflat  = angles[3];
    double pshad  = angles[4];

    // checking validity
    if (!imgId.length() || (inc < 0 || inc >= 89.9) || (ema < 0 || ema >= 89.9) ||
        (phase < 0 || phase >= 180) || pflat < 0 || (pshad < 0 || pshad >= pflat)) {
      continue;
    }

    // Calculate the optical depth
    // First precalculate some constants
    double psurf = photoModel->CalcSurfAlbedo(phase, inc, ema);

//    asmModel->GenerateAhTable();
    double ahi = (asmModel->AtmosAhSpline()).Evaluate(inc, NumericalApproximation::Extrapolate);

    double munot = cos(DEG2RAD * inc);
    double rho=0;

    // Zero in shadow means no atmosphere
    double tausol = 0; // TAU value found as Solution; name keeps it distinct from
                       // temporary TAU in common
    if (pshad <= 0) {
      tausol = 0.0;
      rho = pflat/psurf;
    }
    else {
      /*
        Now, we may do a preliminary search to find an upper limit on optical
        depth such that the "standard" term (scattering in atmosphere only) is
        less than the observed shadow.  If STAN_ERR is negative even at TAU2
        then this maximum tau is greater than 3 and we won't bother to reach
        it but if positive then we find a lower value of tau to start our
        search for the solution.  This avoids negative albedos during the
        solution process.
      */
      double tau1, tau2, tau3; // Optical depths that should bracket the correct solution
      double pstd   = 0; // Pure atmospheric-scattering term
      double trans  = 0; // Transmission of surface reflected light through the atmosphere overall
      double trans0 = 0; // Transmission of surface reflected light through the atmosphere
                         // with no scatterings in the atmosphere
      double sbar   = 0; // Illumination of the ground by the sky
      double transs = 0; // Transmission of surface reflected light that must be subtracted
                         // to model shadow
      double albsol = 0; // I/F of surface at reference geometry and no atmosphere,
                         // based on fitting to PFLAT and PSHAD
      double tol = 1e-6; // Fractional accuracy to seek in TAUSOL

      double se1, se3;

      tau1 = 0;
      tau2 = 5;
      tau3 = 0;

      struct StanData stanData;
      stanData.m_AtmModel = asmModel;
      stanData.m_pstd     = pstd;
      stanData.m_trans    = trans;
      stanData.m_trans0   = trans0;
      stanData.m_sbar     = sbar;
      stanData.m_inc      = inc;
      stanData.m_ema      = ema;
      stanData.m_phase    = phase;
      stanData.m_pshad    = pshad;
      stanData.m_pflat    = pflat;
      stanData.m_rho      = rho;

      double stanErr = stan_err(tau2, &stanData);

      if (stanErr >= 0) {
        gsl_function Func;
        Func.function = &stan_err;
        Func.params   = &stanData;
        double x_lo = tau1;
        double x_hi = tau2;
        Photometry::brentsolver(x_lo, x_hi, &Func, tol, tau3);
        tau1 = 0;
      }
      else {
        tau3 = tau2;
      }

      // No detectable atmosphere
      if (tau3 <= tol) {
        tausol = 0;
        rho = pflat/psurf;
      }
      else {
        struct ShadData shadData;
        shadData.m_AtmModel = asmModel;
        shadData.m_pstd     = pstd;
        shadData.m_trans    = trans;
        shadData.m_trans0   = trans0;
        shadData.m_sbar     = sbar;
        shadData.m_inc      = inc;
        shadData.m_ema      = ema;
        shadData.m_phase    = phase;
        shadData.m_pshad    = pshad;
        shadData.m_pflat    = pflat;
        shadData.m_psurf    = psurf;
        shadData.m_ahi      = ahi;
        shadData.m_munot    = munot;
        shadData.m_munot    = transs;
        shadData.m_rho      = rho;

        tau3 = min(1.5*tau3,tau3+0.5);
        se1 = shad_err(tau1, &shadData);
        se3 = shad_err(tau3, &shadData);
        if ((se1 < 0 && se3 < 0) || (se1 > 0 && se3 > 0)) {
          outfile << "Root not bracketed for image " << imgId << ", " << inc << ", " ;
          outfile << ema << ", " << phase << ", " << pflat << ", " << pshad << "\n";
          continue;
        }
        else {
          // Now it is safe to seek the solution
          gsl_function Func;
          Func.function = &shad_err;
          Func.params   = &shadData;

          double x_lo = tau1;
          double x_hi = tau3;
          Photometry::brentsolver(x_lo, x_hi, &Func, tol, tausol);
          albsol = shadData.m_rho * psurfref;
        }
      }
      outfile << imgId << ", " << inc << ", " << ema << ", " << phase << ", " << pflat << ", " ;
      outfile << pshad << ", " << tausol << ", " << albsol << "\n";
    }
  }
  outfile.close();
}

/**
 * Return difference between observed and calculated brightness of the "standard"
 * atmosphere term.
 *
 * Given a current guess at the optical depth (and a whole lot of other
 * parameters passed in common), returns the difference between the
 * observed shadow brightness and the calculated brightness of the
 * "standard" atmosphere term.  The "standard" term involves scattering
 * ONLY in the atmosphere, so it is the brightness over a black surface
 * (which is less than the shadow brightness).  Thus by limiting our
 * second search to optical depths for which the standard term is less
 * than the observed brightnesses we will avoid negative albedos.
 * between observed and estimated brightness of a shadow.
 *
 * _hist  14 Dec 1999  Randy Kirk U.S.G.S. Flagstaff Original Vax Version.
 *   15 Dec 1999  K Teal Thompson - Port to Unix; add implicit none.
 *   02 Nov 2000  RLK - Add transs to argument list in call to atm_func_shad.
 *
 * @author Sharmila Prasad (9/8/2011) - Ported from Isis2 to Isis3
 *
 * @param tau_guess - current guess at the optical depth
 *
 * @return double - the difference between the observed shadow brightness and
 * the calculated brightness of the "standard" atmosphere term
 *
 */
double stan_err(double tau_guess, void * pParams) {
  struct StanData * stanData = (struct StanData *)pParams;
  stanData->m_AtmModel->SetAtmosTau(tau_guess);
  stanData->m_AtmModel->CalcAtmEffect(stanData->m_phase, stanData->m_inc, stanData->m_ema,
                                     &stanData->m_pstd, &stanData->m_trans, &stanData->m_trans0,
                                     &stanData->m_sbar, &stanData->m_transs);

  // ratio of the true reflectance of the surface to the model PHO_FUNC
  double rho = stanData->m_pstd - stanData->m_pshad;
  stanData->m_rho = rho;
  return rho;
}

/**
 * Returns the error between observed and estimated brightness of a shadow.
 * Given a current guess at the optical depth (and a whole lot of other
 * parameters passed in common), sets the surface albedo to agree with the
 * observed brightness of a level surface and then returns the error
 * between observed and estimated brightness of a shadow.
 *
 * hist  27 Nov 1999  Randy Kirk U.S.G.S. Flagstaff Original Vax Version.
 *       03 Dec 1999  K Teal Thompson - Port to Unix; add implicit none.
 *       02 Nov 2000  RLK - Add transs to argument list in call to atm_
 *       func_shad.  Change trans0 to transs in final shad_err calculation.
 *
 * @author Sharmila Prasad (9/9/2011) - Ported from Isis2 to Isis3
 *
 * @param tau_guess
 * @param shadData
 *
 * @return double
 */
double shad_err(double tau_guess, void * pParams) {
  struct ShadData * shadData = (struct ShadData *)pParams;

  // Pass the guess for optical depth and calculate atmospheric scattering
  shadData->m_AtmModel->SetAtmosTau(tau_guess);
  shadData->m_AtmModel->CalcAtmEffect(shadData->m_phase, shadData->m_inc, shadData->m_ema,
               &shadData->m_pstd, &shadData->m_trans, &shadData->m_trans0, &shadData->m_sbar, &shadData->m_transs);

  // Solve for RHO (which is the surface albedo relative to the model
  // photometric function of the surface) by matching PFLAT.
  double dpo = shadData->m_pflat - shadData->m_pstd;
  double dpm = (shadData->m_psurf - shadData->m_ahi * shadData->m_AtmModel->AtmosMunot()) * shadData->m_trans0;
  double ab  = shadData->m_AtmModel->AtmosAb();
  double q   = shadData->m_ahi * shadData->m_AtmModel->AtmosMunot() * shadData->m_trans + ab * shadData->m_sbar * dpo + dpm;
  double rho = 2 * dpo / (q + sqrt(pow(q,2) - 4 * ab * shadData->m_sbar * dpo *dpm ));
  shadData->m_rho = rho;

  // Now use this value of RHO to model the shadow
  double shadErr = shadData->m_pstd + rho * shadData->m_ahi * shadData->m_AtmModel->AtmosMunot() *
    (shadData->m_trans / (1 - rho * ab * shadData->m_sbar) - shadData->m_transs) - shadData->m_pshad;

  return shadErr;
}
