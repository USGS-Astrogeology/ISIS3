#include "Isis.h"

#include <algorithm>
#include <cmath>
#include <fstream>
#include <iostream>
#include <QString>
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

const int NLP(51);
const int NS((NLP * 2 -1));
const int NL(NLP);

// Linear Fit params designed to fit the parameter for data type gsl_function
// of the Gnu Scientific Library
struct linearFitParams {
  QString empirical;
  double **hapke_img;
  PhotoModel *pmodel;
  double phase;
  Pvl pvl;
  double c0, c1; // Constant and linear coefficients of linear
                 // fit of empirical fn to Hapke @ fixed PAR
  double incmin, incmax, emamin, emamax, phmin, phmax;
  bool iord;
};

// Fits a simple photometric model to the Hapke model by linear least squares
double LinearFitPhotometricToHapkeGlobal(double pPar, void* pParams);

void GetHapkeImage(PhotoModel *hapkeModel, AtmosModel *pAsmModel, double **hapkeImg,
                   double phase, double emaMax);
bool PhaseGetAngles(int j, int i, double phase, double & inc, double & ema);

void IsisMain() {
  UserInterface &ui = Application::GetUserInterface();

  PvlKeyword phaseAngle("PhaseList");
  PvlKeyword phaseCurve("PhaseCurveList");

  // Allocate 2 Dimensional hapkeImg array
  double **hapkeImgPtr = new double*[NS];
  for (int r=0; r<NS; r++) {
    hapkeImgPtr[r] = new double[NL];
  }

  // Get Hapke function and parameters
  QString sHapkeFunc = ui.GetAsString("PHTNAME");
  sHapkeFunc = sHapkeFunc.toUpper();

  // Should contains parameter names matching GUI to be included in the Pvl defFile
  vector<QString> inclusion;
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
    QString sErrMsg = "Invalid Hapke Function\n";
    throw IException(IException::User, sErrMsg, _FILEINFO_);
  }

  Pvl hapkePvl;
  ui.CreatePVL(hapkePvl, "HAPKE", "PhotometricModel", "Algorithm", inclusion);

  // Log the Hapke Def File
  PvlGroup hapkeGrp = hapkePvl.findObject("PhotometricModel").findGroup("Algorithm");
  Application::Log(hapkeGrp);

  PhotoModel *hapkeModel = PhotoModelFactory::Create(hapkePvl);

  // Type of photometric function to fit (lunar-lambert, Minnaert) to the Hapke Model
  QString sEmpirical = ui.GetAsString("MODEL");
  sEmpirical = sEmpirical.toUpper();

  PvlKeyword limbValue;
  if (sEmpirical == "MINNAERT") {
    limbValue.setName("KList");
  } else if (sEmpirical == "LUNARLAMBERT") {
    limbValue.setName("LList");
  } else {
    QString sErrMsg = "Invalid Photometric Model\n";
    throw IException(IException::User, sErrMsg, _FILEINFO_);
  }

  Pvl empPvl;
  empPvl.addObject(PvlObject("PhotometricModel"));
  empPvl.findObject("PhotometricModel").addGroup(PvlGroup("Algorithm"));
  empPvl.findObject("PhotometricModel").findGroup("Algorithm").
        addKeyword(PvlKeyword("PhtName", sEmpirical.toStdString()), Pvl::Replace);
  PhotoModel *empModel = PhotoModelFactory::Create(empPvl);

  // Order of approximation in atmospheric scatter model
  bool doAsm = false;
  QString sAsmType = ui.GetAsString("ATMNAME");
  sAsmType = sAsmType.toUpper();
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
    //inclusion.push_back("ADDOFFSET");

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
    if(!ui.IsInteractive()) {
      cerr << asmGrp << endl;
    }

    asmModel = AtmosModelFactory::Create(asmPvl, *hapkeModel);
    asmModel->GenerateAhTable();
  }

  double incmin = ui.GetDouble("INCMIN");
  double incmax = ui.GetDouble("INCMAX");
  double emamin = ui.GetDouble("EMAMIN");
  double emamax = ui.GetDouble("EMAMAX");
  double phmin = ui.GetDouble("PHMIN");
  double phmax = ui.GetDouble("PHMAX");
  double emamax_phase_coeff= ui.GetDouble("EMAMAX_PCOEFF");
  double nph   = ui.GetInteger("NPH");

  bool iord = ui.GetBoolean("ADDOFFSET");

  struct linearFitParams lFitParams;
  lFitParams.empirical = sEmpirical;
  lFitParams.hapke_img = hapkeImgPtr;
  lFitParams.pvl       = hapkePvl;
  lFitParams.iord      = iord;
  lFitParams.pmodel    = empModel;
  // Coefficients of the fit C0 (additive) and C1 (multiplicative)
  lFitParams.c0        = 0e0;
  lFitParams.c1        = 0e0;
  lFitParams.incmin    = incmin;
  lFitParams.incmax    = incmax;
  lFitParams.emamin    = emamin;
  lFitParams.emamax    = emamax;
  lFitParams.phmin     = phmin;
  lFitParams.phmax     = phmax;

  gsl_function Func;
  Func.function = &LinearFitPhotometricToHapkeGlobal;
  Func.params   = &lFitParams;

  double para=0.0e0, parb=1.0e0;
  double parc;
  double emamaxupdated;
  double fa, fb, fc;
  double tolerance = 1e-6;
  double c1_0=0.0;
  if (!iord && phmin > 1e-6) {
    lFitParams.phase = 0.0e0;
    emamaxupdated  = emamax;
    GetHapkeImage(hapkeModel, asmModel, hapkeImgPtr, lFitParams.phase, emamaxupdated);
    Photometry::minbracket(para, parb, parc, fa, fb, fc,
        &LinearFitPhotometricToHapkeGlobal, &lFitParams); // minimum parabola (approximation)
    double a = para;
    if (a > parb) a = parb;
    if (a > parc) a = parc;
    double c = para;
    if (c < parb) c = parb;
    if (c < parc) c = parc;
    Photometry::brentminimizer(a, c, &Func, parb, tolerance);
    LinearFitPhotometricToHapkeGlobal(parb, &lFitParams);
    c1_0 = lFitParams.c1;
  }

  // Now loop to create the table of results versus phase angle
  double dph = (phmax-phmin) / (double)(nph-1);
  for(int iph=0; iph<nph; iph++) {
    // Fill the buffer with the Hapke results at the right phase
    lFitParams.phase = phmin + dph * (double)iph;
    emamaxupdated = emamax + emamax_phase_coeff * lFitParams.phase;
    if (lFitParams.phase >= (incmax+emamaxupdated)) {
      // No valid fit points
      return;
    }
    GetHapkeImage(hapkeModel, asmModel, hapkeImgPtr, lFitParams.phase, emamaxupdated);
    lFitParams.emamax = emamaxupdated;
    para = 0.0e0;
    LinearFitPhotometricToHapkeGlobal(para, &lFitParams);
    parb = 1.0e0;
    LinearFitPhotometricToHapkeGlobal(parb, &lFitParams);
    Photometry::minbracket(para, parb, parc, fa, fb, fc,
        &LinearFitPhotometricToHapkeGlobal, &lFitParams); // minimum parabola (approximation)
    double a = para;
    if (a > parb) a = parb;
    if (a > parc) a = parc;
    double c = para;
    if (c < parb) c = parb;
    if (c < parc) c = parc;
    Photometry::brentminimizer(a, c, &Func, parb, tolerance);
    LinearFitPhotometricToHapkeGlobal(parb, &lFitParams);

    double c1 = lFitParams.c1;
    if (lFitParams.phase < 1e-6) {
      c1_0 = lFitParams.c1;
    }
    double rmsmin = parb;
    if (!iord) {
      // Fit with no additive offset:  output multiplier normalized to
      // zero phase, which is the desired phase curve B, and unnormalized
      phaseAngle.addValue(std::to_string(lFitParams.phase));
      limbValue.addValue(std::to_string(rmsmin));
      phaseCurve.addValue(std::to_string(c1/c1_0));
    }
    else {
      // Fit with additive offset:  normalizing would make no sense, just
      // output additive offset and multiplier from fit
      phaseAngle.addValue(std::to_string(lFitParams.phase));
      limbValue.addValue(std::to_string(rmsmin));
      phaseCurve.addValue(std::to_string(c1));
    }
  }

  if (ui.WasEntered("TO")) {
    QString sOutfile = ui.GetFileName("TO");
    Pvl mainPvl;
    PvlObject photoObj("PhotometricModel");
    PvlGroup photoGrp("Algorithm");
    if (sEmpirical == "MINNAERT") {
      photoGrp += PvlKeyword("Name", "MinnaertEmpirical");
    } else {
      photoGrp += PvlKeyword("Name", "LunarLambertEmpirical");
    }
    photoGrp += phaseAngle;
    photoGrp += limbValue;
    photoGrp += phaseCurve;
    if (ui.WasEntered("NOTE")) {
      PvlGroup note("Note");
      note.addComment("NOTE DESCRIBING THE FOLLOWING PHOTOMETRIC MODEL");
      note += PvlKeyword("NOTE", ui.GetString("NOTE").toStdString());
      photoObj += note;
    }
    photoObj += photoGrp;
    mainPvl.addObject(photoObj);
    mainPvl.write(sOutfile.toStdString());
  }

  for (int r=0; r<NS; ++r){
    delete [] hapkeImgPtr[r];
  }
  delete [] hapkeImgPtr;


  if (asmModel != NULL) {
    delete asmModel;
    asmModel = NULL;
  }

  if (hapkeModel != NULL) {
    delete(hapkeModel);
    hapkeModel = NULL;
  }

  if (empModel != NULL) {
    delete(empModel);
    empModel = NULL;
  }
}

/**
 * Fits a simple photometric model to the Hapke model by linear least
 * squares fit at a contant given value of the limb-darkening parameter pPar.
 * The RMS error of the fit is returned.
 *
 * 23 Nov 99  Randy L Kirk - USGS, Flagstaff; Original Code.
 * 23 Dec 99  K Teal Thompson - Port to Unix; add implicit none.
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
 * @author Sharmila Prasad (8/30/2011)
 *
 * @param pPar - limb-darkening parameter
 *
 * @return double - RMS error of fit
 */
double LinearFitPhotometricToHapkeGlobal(double pPar, void* pParams){
  double lfit_pho_global; // Output: RMS error of fit

  struct linearFitParams * lFitParams = (struct linearFitParams *)pParams;
  const QString pEmpirical = lFitParams->empirical;
  double ** hapke_img     = lFitParams->hapke_img;
  const bool iord         = lFitParams->iord;
  PhotoModel *pmodel      = lFitParams->pmodel;

  if (pEmpirical == "LUNARLAMBERT") {
    pmodel->SetPhotoL(pPar);
  }
  else {
    pmodel->SetPhotoK(pPar);
  }

  // Accumulation buffers for least-squares fit information:
  // sums of products of abcissa and ordinate...
  double sum1=0e0, sumx=0e0, sumy=0e0, sumxx=0e0, sumxy=0e0, sumyy=0e0;

  double inc=0e0, ema=0e0;      // Incidence and emission angles

  // Here, abcissa and ordinate of fit, i.e., Hapke model value,
  // simple model value
  double x, y;

  // Accumulate statistics
  for (int j=0; j<NL; j++) {
    for (int i=0; i<NS; i++) {
      bool result = PhaseGetAngles(j, i, lFitParams->phase, inc, ema);
      if (result &&
          inc >= lFitParams->incmin && inc <= lFitParams->incmax &&
          ema >= lFitParams->emamin && ema <= lFitParams->emamax) {
        x = pmodel->CalcSurfAlbedo(lFitParams->phase, inc, ema);
        y = hapke_img[i][j];
        sum1 += 1e0;
        sumx += x;
        sumy += y;
        sumxx += (x*x);
        sumxy += (x*y);
        sumyy += (y*y);
      }
    }
  }

  // Check that some points were found so the fit can be done
  if (sum1 < 1e0 || sumxx <= 0e0 ||
      (iord && ((sum1 * sumxx - sumx * sumx) == 0e0))) {
   return -1; // nofit
  }

  // Evaluate coefficients and RMS error of the linear least-squares fit
  double c0, c1; // Constant and linear coefficients of linear
                 // fit of empirical fn to Hapke @ fixed PAR
  double den;    // Denominator for least squares calcs
  double arg;

  if (!iord) {
    c0 = 0e0;
    c1 = sumxy/sumxx;
    arg = (sumyy - 2e0 * c1 * sumxy + c1 * c1 * sumxx) / sum1;
  }
  else {
    den = sum1 * sumxx - sumx * sumx;
    c0  = (sumxx * sumy - sumx * sumxy) / den;
    c1  = (sum1 * sumxy - sumx * sumy) / den;
    arg = (sumyy + 2e0 * (c0 * c1 * sumx - c0 * sumy -c1 * sumxy) + c0 * c0 * sum1 + c1 * c1 * sumxx) / sum1;
  }

  if (arg > 0e0) {
    lfit_pho_global = sqrt(arg);
  }
  else {
    lfit_pho_global = 0e0;
  }

  lFitParams->c0 = c0;
  lFitParams->c1 = c1;

  return lfit_pho_global;
}


/**
 * Fills the buffer hapkeImg with the Hapke-model shaded image of
 * a sphere with illumination from the left at the given PHASE.
 *
 * 23 Nov 99  Randy L Kirk - USGS, Flagstaff; Original Code.
 * 23 Dec 99  K Teal Thompson - Port to Unix; add implicit none.
 * 24 Sep 02  Janet Barrett - Added support for 1st and 2nd order
 *               Henyey-Greenstein
 *
 * Ported from Isis2 subroutine pht_get_hapke_img
 *
 * @author Sharmila Prasad (8/26/2011)
 *
 * @param hapkeImg  - Array for hapke image of sphere
 * @param pAsmModel - Atmospheric scattering Model
 * @param phase     - Current phase angle
 * @param emaMax    - Max emission angle needed
 */
void GetHapkeImage(PhotoModel *hapkeModel, AtmosModel *pAsmModel, double **hapkeImg, double phase, double emaMax){
  double munot=0e0;
  double inc=0e0;
  double ema=0e0;
  double ahi;
  double pstd=0e0, trans=0e0, trans0=0e0, sbar=0e0, transs=0e0;

  for (int j=0; j<NL; j++) {
    for (int i=0; i<NS; i++) {
      if (PhaseGetAngles(j, i, phase, inc, ema) && ema <= emaMax) {
        hapkeImg[i][j] = hapkeModel->CalcSurfAlbedo(phase, inc, ema);
        if (pAsmModel != NULL) {
          pAsmModel->CalcAtmEffect(phase, inc, ema, &pstd, &trans, &trans0, &sbar, &transs);
          ahi = (pAsmModel->AtmosAhSpline()).Evaluate(inc, NumericalApproximation::Extrapolate);
          munot = cos(DEG2RAD * inc);
          hapkeImg[i][j] = pstd + trans * munot * ahi /
            (1.0e0-pAsmModel->AtmosAb()*sbar) + trans0 * (hapkeImg[i][j] - ahi * munot);
        }
      }
    }
  }
}

/**
 * Calculate the incidence and emission angles at a point in a buffer
 * corresponding to one half of a Gaussian sphere illuminated from the
 * left at phase angle PHASE
 *
 *  23 Nov 99  Randy L Kirk    - USGS, Flagstaff; Original Code.
 *  23 Dec 99  K Teal Thompson - Port to Unix; add implicit none.
 *
 * @author Sharmila Prasad (8/26/2011)
 * Ported from Isis2 subroutine pht_get_hapke_img
 *
 * @param j - Buffer index
 * @param i - Buffer index
 * @param phase - Phase Angle
 * @param inc   - Output Incidence Angle
 * @param ema   - Output Emission Angle
 *
 * @return bool - Success / Failure
 */
bool PhaseGetAngles(int j, int i, double phase, double & inc, double & ema) {
  // Precompute Radius, radius squared
  double r = (double)(NL-1);
  double r2= r * r;

  // Given buffer dimensions and location in buffer, calculate angles
  double x = i-NL+1e0;
  double y = j;
  double s2 = x*x+y*y;
  if (s2 >= r2) {
    return false;
  }

  double z   = sqrt(r2-s2);
  inc = acos(cos(phase*DEG2RAD)*z/r - sin(phase*DEG2RAD)*x/r)/DEG2RAD;
  ema = acos(z/r)/DEG2RAD;

  return true;
}
