#include "Isis.h"

#include <cmath>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <set>
#include <vector>

// Gnu Scientific Library
#include <gsl/gsl_errno.h>
#include <gsl/gsl_math.h>
#include <gsl/gsl_min.h>

#include "AtmosModel.h"
#include "AtmosModelFactory.h"
#include "GuiEditFile.h"
#include "IException.h"
#include "PhotoModel.h"
#include "PhotoModelFactory.h"
#include "Photometry.h"
#include "Progress.h"
#include "Pvl.h"
#include "PvlGroup.h"
#include "SpecialPixel.h"

using namespace Isis;
using namespace std;

#define NLP  51
#define NS   (NLP * 2 -1)
#define NL   NLP

//  Array OF HAPKE Values
typedef struct {
  double m_hapkeImg[NS][NL];
  double m_incImg  [NS][NL];
  double m_emaImg  [NS][NL];
}HapkeArrs;

// Get Emission, Incidence, Phase Mean Ground Plane Geometry (Datum)
typedef struct {
  double m_emission;
  double m_incidence;
  double m_phase;
  double m_rmsSlope;
}Datum;

// Linear Fit params designed to fit the parameter for data type gsl_function
// of the Gnu Scientific Library
struct linearFitParams {
  QString empirical;
  HapkeArrs hapkeArrs;
  Datum datum;
  Pvl pvl;
  double c0, c1; // Constant and linear coefficients of linear
                 // fit of empirical fn to Hapke @ fixed PAR
  bool iord;
  PhotoModel *pModel;
};

// Save output to the Results group
PvlGroup results("Results");
PvlGroup note("Note");

// Random number generation
float rando(int init);
bool useSeed=0;
int seedNumber=0;

// Fits a simple photometric model to the Hapke model by linear least squares
double LinearFitPhotometricToHapke(double pPar, void* pParams);

// Fills the buffer HAPKE_IMG with the Hapke-model radiances of point
void GetHapkeImgLocation(PhotoModel *pHapke, AtmosModel *pAsmModel, HapkeArrs & pHapkeArrs,
                         Datum & pDatum);

void IsisMain() {
  UserInterface &ui = Application::GetUserInterface();
  Datum datum;
  HapkeArrs hapkeVal;

  // Determine if user is providing a seed for the random number
  // generator
  useSeed = ui.GetBoolean("SEED");
  if (useSeed) {
    seedNumber = ui.GetInteger("SEED_NUMBER");
  }

  // Get Hapke function and parameters
  IString sHapkeFunc = ui.GetAsString("PHTNAME").toStdString();
  sHapkeFunc = sHapkeFunc.UpCase();

  // Should contains parameter names matching GUI not to be included
  // in the Pvl defFile
  vector<QString> inclusion;
  inclusion.clear();
  inclusion.push_back("PHTNAME");
  inclusion.push_back("WH");
  inclusion.push_back("HH");
  inclusion.push_back("B0");
  inclusion.push_back("THETA");

  if (sHapkeFunc == "HAPKEHEN") { // Single Particle Phase Function HENYEY-GREENSTEIN
    inclusion.push_back("HG1");
    inclusion.push_back("HG2");
  }
  else if (sHapkeFunc == "HAPKELEG") { // Single Particle Phase Function LEGENDRE
    inclusion.push_back("BH");
    inclusion.push_back("CH");
  }
  else {
    std::string sErrMsg = "Invalid Hapke Function\n";
    throw IException(IException::User, sErrMsg, _FILEINFO_);
  }

  Pvl hapkePvl;
  ui.CreatePVL(hapkePvl, "Hapke", "PhotometricModel", "Algorithm", inclusion);

  // Log the Hapke Def File
  PvlGroup hapkeGrp = hapkePvl.findObject("PhotometricModel").findGroup("Algorithm");
  Application::Log(hapkeGrp);

  PhotoModel *hapkeModel = PhotoModelFactory::Create(hapkePvl);

  // Type of photometric function to fit (lunar-lambert, Minnaert) to the Hapke Model
  QString sEmpirical = ui.GetAsString("MODEL");
  sEmpirical = sEmpirical.toUpper();

  Pvl empPvl;
  empPvl.addObject(PvlObject("PhotometricModel"));
  empPvl.findObject("PhotometricModel").addGroup(PvlGroup("Algorithm"));
  if (sEmpirical == "LUNARLAMBERT") {
    empPvl.findObject("PhotometricModel").findGroup("Algorithm").addKeyword(PvlKeyword("Name", "LunarLambert"), Pvl::Replace);
  } else {
    empPvl.findObject("PhotometricModel").findGroup("Algorithm").addKeyword(PvlKeyword("Name", "Minnaert"), Pvl::Replace);
  }

  // Get Emission, Incidence, Phase Mean Ground Plane Geometry (Datum)
  datum.m_emission  = ui.GetDouble("EMISSION");
  datum.m_incidence = ui.GetDouble("INCIDENCE");
  datum.m_phase     = ui.GetDouble("PHASE");
  datum.m_rmsSlope  = ui.GetDouble("RMS_SLOPE");

  // Save output to the Results group
  stringstream os;
  os << "Group = Results" << endl;
  os << "# EMPIRICAL PHOTOMETRIC PARAMETER AND PHASE CURVES CREATED BY PHO_EMP_LOCAL" << endl;
  os << "# HAPKE PARAMETERS:" << endl;
  os << "WH = " << ui.GetDouble("WH") << endl;
  os << "THETA = " << ui.GetDouble("THETA") << endl;
  os << "HH = " << ui.GetDouble("HH") << endl;
  os << "B0 = " << ui.GetDouble("B0") << endl;

  if (sHapkeFunc == "HAPKEHEN") {
    os << "# SINGLE PARTICLE PHASE FUNCTION IS HENYEY-GREENSTEIN WITH:" << endl;
    os << "HG1 = " << ui.GetDouble("HG1") << endl;
    os << "HG2 = " << ui.GetDouble("HG2") << endl;
  }
  else if (sHapkeFunc == "HAPKELEG") {
    os << "# SINGLE PARTICLE PHASE FUNCTION IS LEGENDRE WITH:" << endl;
    os << "BH = " << ui.GetDouble("BH") << endl;
    os << "CH = " << ui.GetDouble("CH") << endl;
  }
  else {
    std::string errMsg = "Undefined Hapke Model\n";
    throw IException(IException::User, errMsg, _FILEINFO_);
  }

  // Order of approximation in atmospheric scatter model
  bool doAsm = false;
  IString sAsmType = ui.GetAsString("ATMNAME").toStdString();
  sAsmType = sAsmType.UpCase();
  if (sAsmType != "NONE") {
    doAsm = true;
  }

  AtmosModel *asmModel=NULL;
  if (doAsm) {
    Pvl asmPvl;
    inclusion.clear();
    inclusion.push_back("ATMNAME");
    inclusion.push_back("TAU");
    inclusion.push_back("WHA");
    inclusion.push_back("HNORM");
    inclusion.push_back("ADDOFFSET");

    if (sAsmType=="ANISOTROPIC1" || sAsmType=="ANISOTROPIC2" ){
      inclusion.push_back("BHA");
    }
    else if (sAsmType=="HAPKEATM1" || sAsmType=="HAPKEATM2" ) {
      inclusion.push_back("HGA");
    }

    ui.CreatePVL(asmPvl, "Atmospheric Scattering Model(ATM)", "AtmosphericModel", "Algorithm", inclusion);

    // Log the AtmosphericModel Def File
    PvlGroup asmGrp = asmPvl.findObject("AtmosphericModel").findGroup("Algorithm");
    Application::Log(asmGrp);

    asmModel = AtmosModelFactory::Create(asmPvl, *hapkeModel);
    if (sAsmType=="ISOTROPIC1") {
      os << "# FIRST ORDER ISOTROPIC ATMOSPHERIC SCATTERING MODEL" << endl;
    } else if (sAsmType=="ANISOTROPIC1") {
      os << "# FIRST ORDER ANISOTROPIC ATMOSPHERIC SCATTERING MODEL" << endl;
    } else if (sAsmType=="HAPKEATM1") {
      os << "# FIRST ORDER HAPKE ATMOSPHERIC SCATTERING MODEL" << endl;
    } else if (sAsmType=="ISOTROPIC2") {
      os << "# SECOND ORDER ISOTROPIC ATMOSPHERIC SCATTERING MODEL" << endl;
    } else if (sAsmType=="ANISOTROPIC2") {
      os << "# SECOND ORDER ANISOTROPIC ATMOSPHERIC SCATTERING MODEL" << endl;
    } else if (sAsmType=="HAPKEATM2") {
      os << "# SECOND ORDER HAPKE ATMOSPHERIC SCATTERING MODEL" << endl;
    }
    else {
      std::string errMsg = "Undefined Atmospheric Scattering Model\n";
      throw IException(IException::User, errMsg, _FILEINFO_);
    }

    os << "TAU = " << asmModel->AtmosTau() << endl;
    os << "SCALE_HEIGHT/PLANET_RADIUS = " << asmModel->AtmosHnorm() << endl;
    os << "ATMOSPHERIC_SS_ALBEDO_WHA = " << asmModel->AtmosWha() << endl;
    os << "H-G_ASYMMETRY_FAC._HGA = " << asmModel->AtmosHga() << endl;

    asmModel->GenerateAhTable();
  }

  os << "GRID_POINTS = " << NL << endl;
  os << "INCIDENCE_ANGLE_TO_DATUM = " << datum.m_incidence << endl;
  os << "EMISSION_ANGLE_TO_DATUM = " << datum.m_emission << endl;
  os << "PHASE_ANGLE = " << datum.m_phase << endl;
  os << "RMS_SLOPE = " << datum.m_rmsSlope << endl;

  if (sEmpirical == "LUNARLAMBERT") {
    os << "# FITTED EMPIRICAL FUNCTION IS LUNAR-LAMBERT: LIMB-DARKENING PARAMETER IS L" << endl;
  }
  else {
    os << "# FITTED EMPIRICAL FUNCTION IS MINNAERT: LIMB-DARKENING PARAMETER IS K" << endl;
  }

  if (doAsm) {
    if (!asmModel->AtmosAdditiveOffset()) {
      os << "# FIT INCLUDES MULTIPLIER ONLY" << endl;
    }
    else {
      os << "# FIT INCLUDES MULTIPLIER AND OFFSET" << endl;
    }
  } else {
    os << "# FIT INCLUDES MULTIPLIER ONLY" << endl;
  }

  if (datum.m_phase > (datum.m_emission + datum.m_incidence)) {
    std::string sErrMsg = "No valid fit points\n";
    throw IException(IException::User, sErrMsg, _FILEINFO_);
  }
  else {
    GetHapkeImgLocation(hapkeModel, asmModel, hapkeVal, datum);
  }

  struct linearFitParams lFitParams;
  lFitParams.empirical = sEmpirical;
  lFitParams.hapkeArrs = hapkeVal;
  lFitParams.datum     = datum;
  lFitParams.pvl       = hapkePvl;
  lFitParams.iord      = (doAsm ? asmModel->AtmosAdditiveOffset() : 0);
  // Coefficients of the fit C0 (additive) and C1 (multiplicative)
  lFitParams.c0        = 0;
  lFitParams.c1        = 0;

  lFitParams.pModel = PhotoModelFactory::Create(empPvl);
  // Log the Empirical Photometric Model Def File
  PvlGroup empGrp = empPvl.findObject("PhotometricModel").findGroup("Algorithm");
  Application::Log(empGrp);

  gsl_function Func;
  Func.function = &LinearFitPhotometricToHapke;
  Func.params   = &lFitParams;

  double xa = 0.0;
  double xb = 1.0;
  double xc,fa,fb,fc;
  Photometry::minbracket(xa, xb, xc, fa, fb, fc,
      &LinearFitPhotometricToHapke, &lFitParams); // minimum parabola (approximation)

  double tolerance = 1e-6;
  Photometry::brentminimizer(xa, xc, &Func, xb, tolerance);

  double parmin = LinearFitPhotometricToHapke(xb, &lFitParams);

  if (!lFitParams.iord) {
    os << "LIMB_DARKENING_PARAMETER = " << xb << endl;
    os << "BEST_FIT_MULTIPLIER = " << lFitParams.c1 << endl;
    os << "RMS_ERROR_OF_FIT = " << parmin << endl;
    PvlGroup fitParams("Results");
    fitParams += PvlKeyword("PhaseAngle", toString(datum.m_phase), "degrees");
    fitParams += PvlKeyword("LimbDarkeningParameter", toString(xb));
    fitParams += PvlKeyword("BestFitMultiplier", toString(lFitParams.c1));
    fitParams += PvlKeyword("RMSErrorOfFit", toString(parmin));
    Application::Log(fitParams);
  }
  else {
    os << "LIMB_DARKENING_PARAMETER = " << xb << endl;
    os << "BEST_FIT_ADDITIVE_TERM = " << lFitParams.c0 << endl;
    os << "BEST_FIT_MULTIPLIER = " << lFitParams.c1 << endl;
    os << "RMS_ERROR_OF_FIT = " << parmin << endl;
    PvlGroup fitParams("Results");
    fitParams += PvlKeyword("PhaseAngle", toString(datum.m_phase), "degrees");
    fitParams += PvlKeyword("LimbDarkeningParameter", toString(xb));
    fitParams += PvlKeyword("BestFitAdditiveTerm", toString(lFitParams.c0));
    fitParams += PvlKeyword("BestFitMultiplier", toString(lFitParams.c1));
    fitParams += PvlKeyword("RMSErrorOfFit", toString(parmin));
    Application::Log(fitParams);
  }

  os << "EndGroup" << endl;
  os >> results;

  if (ui.WasEntered("NOTE")) {
    note.addComment("NOTE DESCRIBING DATA IN THE FOLLOWING RESULTS SECTION");
    note += PvlKeyword("NOTE", ui.GetString("NOTE").toStdString());
  }


  if (ui.WasEntered("TO")) {
    Pvl mainpvl;
    if (ui.WasEntered("NOTE")) {
      mainpvl.addGroup(note);
    }
    mainpvl.addGroup(results);
    QString sOutFile = ui.GetFileName("TO");
    bool append = ui.GetBoolean("APPEND");
    ofstream os;
    if (append) {
      mainpvl.append(sOutFile.toStdString());
    } else {
      mainpvl.write(sOutFile.toStdString());
    }
  }

  if (lFitParams.pModel != NULL) {
    delete lFitParams.pModel;
    lFitParams.pModel = NULL;
  }
  if (asmModel != NULL) {
    delete asmModel;
    asmModel = NULL;
  }
  if (hapkeModel != NULL) {
    delete(hapkeModel);
    hapkeModel = NULL;
  }
}

/**
 * Fits a simple photometric model to the Hapke model by linear least
 * squares fit at a contant given value of the limb-darkening parameter pPar.
 * The RMS error of the fit is returned.
 *
 * The prototype is set to match the gsl_function data type to use gsl's brentminimizer function.
 *
 * The gsl_function is a pointer to function of the following prototype:
 * double (* function) (double x, void * params)
 *
 * Following is an example to use gsl_function
 * struct my_f_params { double a; double b; double c; };
 *
 *  double my_f (double x, void * p) {
 *    struct my_f_params * params = (struct my_f_params *)p;
 *    double a = (params->a);
 *    double b = (params->b);
 *    double c = (params->c);
 *
 *    return  (a * x + b) * x + c;
 *  }
 *
 *  gsl_function F;
 *  struct my_f_params params = { 3.0, 2.0, 1.0 };
 *
 *  F.function = &my_f;
 *  F.params = &params;
 *
 * @author Sharmila Prasad (8/10/2011)
 *
 * @param pPar - limb-darkening parameter
 *
 * @return double - RMS error of fit
 */
double LinearFitPhotometricToHapke(double pPar, void* pParams){
  double lfit_pho_local;  // Output: RMS error of fit
  double c0, c1;          // Constant and linear coefficients of linear
                          // fit of empirical fn to Hapke @ fixed PAR

  struct linearFitParams * lFitParams = (struct linearFitParams *)pParams;
  const QString pEmpirical    = lFitParams->empirical;
  const HapkeArrs pHapkeArrs = lFitParams->hapkeArrs;
  const Datum pDatum         = lFitParams->datum;
  const bool iord            = lFitParams->iord;
  PhotoModel *pmodel = lFitParams->pModel;

  if (pEmpirical == "LUNARLAMBERT") {
    pmodel->SetPhotoL(pPar);
  }
  else {
    pmodel->SetPhotoK(pPar);
  }

  // Accumulation buffers for least-squares fit information:
  // sums of products of abcissa and ordinate...
  double sum1=0, sumX=0, sumY=0, sumXX=0, sumXY=0, sumYY=0;

  double inc, ema;      // Incidence and emission angles

  // Here, abcissa and ordinate of fit, i.e., Hapke model value,
  // simple model value
  double x, y;

  double den;         // Denominator for least squares calcs
  double arg;

  // Accumulate statistics
  for (int j=0; j<NL; j++) {
    for (int i=0; i<NS; i++) {
      inc = pHapkeArrs.m_incImg[i][j];
      ema = pHapkeArrs.m_emaImg[i][j];
      x = pmodel->CalcSurfAlbedo(pDatum.m_phase, inc, ema);
      y = pHapkeArrs.m_hapkeImg[i][j];
      sum1 += 1;
      sumX += x;
      sumY += y;
      sumXX += (x*x);
      sumXY += (x*y);
      sumYY += (y*y);
    }
  }

  // Check that some points were found so the fit can be done
  if (sum1 < 1 || sumXX <= 0 ||
      (iord && ((sum1 * sumXX - sumX * sumX) == 0))) {
   return -1; // nofit
  }

  // Evaluate coefficients and RMS error of the linear least-squares fit
  if (!iord) {
    c0 = 0;
    c1 = sumXY/sumXX;
    arg = (sumYY - 2 * c1 * sumXY + c1 * c1 * sumXX) / sum1;
  }
  else {
    den = sum1 * sumXX - sumX * sumX;
    c0  = (sumXX * sumY - sumX * sumXY) / den;
    c1  = (sum1 * sumXY - sumX * sumY) / den;
    arg = (sumYY + 2 * (c0 * c1 * sumX - c0 * sumY -c1 * sumXY) + c0 * c0 * sum1 + c1 * c1 * sumXX) / sum1;
  }

  if (arg > 0) {
    lfit_pho_local = sqrt(arg);
  }
  else {
    lfit_pho_local = 0;
  }

  lFitParams->c0 = c0;
  lFitParams->c1 = c1;

  return lfit_pho_local;
}

/**
 * Fills the buffer HAPKE_IMG with the Hapke-model radiances of
 * points with given PHASE and random incidence and emission angles
 * stored in INC_IMG, EMA_IMG.
 *
 * Ported from Isis2
 *
 * @author Sharmila Prasad (8/9/2011)
 *
 * @param pHapke     - Hapke Photo Model
 * @param pAsmModel  - Is Atmospheric Model selected
 * @param pHapkeArrs - Structure of hapke, inc, ema arrays
 * @param pDatum     - Structure with datum incidence, emission, phase and rms slope
 */
void GetHapkeImgLocation(PhotoModel *pHapke, AtmosModel *pAsmModel, HapkeArrs & pHapkeArrs, Datum & pDatum) {
  if (NS != (NL * 2 - 1)) {
    std::string errMsg = "Bad Buffer Dimensions\n";
    throw IException(IException::User, errMsg, _FILEINFO_);
  }

  double incX = sin(pDatum.m_incidence * DEG2RAD);
  double incZ = cos(pDatum.m_incidence * DEG2RAD);
  double emaZ = cos(pDatum.m_emission  * DEG2RAD);
  double cosP = cos(pDatum.m_phase     * DEG2RAD);

  double emaX, emaY, scaZrad=0;

  if (pDatum.m_incidence==0 || pDatum.m_emission==0) {
    scaZrad = 0;
    emaX    = sin(pDatum.m_emission  * DEG2RAD);
    emaY    = 0;
  }
  else {
    emaX    = (cosP - incZ*emaZ) / incX;
    emaY    = sin(pDatum.m_emission  * DEG2RAD);
    scaZrad = acos(emaX/emaY);
    emaY   *= sin(scaZrad);
  }

  double rmsBI = tan(pDatum.m_rmsSlope * DEG2RAD) / sqrt(2);

  // Calculates Angles based on Random Orientation to Datum
  // Algorithm for Gaussian random deviates is that of Box and Muller,
  // taken from Forsyth, Malcom, and Moler.
  double u1 = rando(1);
  double u2 = 0;
  for (int j=0; j<NL; j++) {
    for (int i=0; i<NS; i++) {
      double s=0;
      while(s<=0 || s>1) {
        u1 = 2 * rando(0) - 1;
        u2 = 2 * rando(0) - 1;
        s  = u1*u1 + u2*u2;
      }
      double t = sqrt(-2 * log(s) / s);
      double dzdx = rmsBI * u1 * t;
      double dzdy = rmsBI * u2 * t;
      double den  = sqrt(1 + dzdx * dzdx + dzdy * dzdy);
      double munot = (incZ - incX * dzdx) / den;
      double mu = (emaZ-emaX*dzdx-emaY*dzdy)/den;
      double inc = acos(munot)/DEG2RAD;
      double ema = acos(mu)/DEG2RAD;

      pHapkeArrs.m_hapkeImg[i][j] = pHapke->CalcSurfAlbedo(pDatum.m_phase, inc, ema);
      pHapkeArrs.m_incImg[i][j]   = inc;
      pHapkeArrs.m_emaImg[i][j]   = ema;

      // Note that incidence and emission Datum replace inc and ema in Atmospheric Models
      //ISOTROPIC1, ISOTROPIC2, ANISOTROPIC1, ANISOTROPIC2
      double pstd=0, trans=0, trans0=0, sbar=0, transs=0;
      double ahi=0;
      if (pAsmModel != NULL) {
        pAsmModel->CalcAtmEffect(pDatum.m_phase, pDatum.m_incidence, pDatum.m_emission,
                                 &pstd, &trans, &trans0, &sbar, &transs);

        //R8SPLINT(INCTABLE,AHTABLE,AHTABLE2,NINC,INCDAT,AHI)
        ahi = (pAsmModel->AtmosAhSpline()).Evaluate(pDatum.m_incidence, NumericalApproximation::Extrapolate);

        munot = cos(DEG2RAD * pDatum.m_incidence);
        pHapkeArrs.m_hapkeImg[i][j] = pstd + trans * munot * ahi / (1 - pAsmModel->AtmosAb() * sbar)
                                      + trans0 * (pHapkeArrs.m_hapkeImg[i][j] - ahi * munot);
      }
    }
  }
}

/**
 * Random number generator
 *
 * @author Janet Barrett USGS Flagstaff Original Version Mar 12 2003
 *
 * @return float rand
 */
float rando(int init) {
  float rnd;
  int i,stime,ltime;

  if (init) {
    if (useSeed) {
      srand(seedNumber);
    } else {
      ltime = time(NULL);
      stime = ltime/2;
      srand(stime);
    }
    return(0);
  } else {
    i = rand();
    rnd = (float)i/(float)RAND_MAX;
  }

  return(rnd);
}
