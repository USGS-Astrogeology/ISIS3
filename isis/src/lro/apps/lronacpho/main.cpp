/** This is free and unencumbered software released into the public domain.

The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */

#include "Isis.h"

#include <QDir>
#include <QRegExp>
#include <QString>

#include "Cube.h"
#include "IException.h"
#include "LROCEmpirical.h"
#include "PhotometricFunction.h"
#include "ProcessByLine.h"
#include "Pvl.h"
#include "PvlGroup.h"
#include "SpecialPixel.h"

using namespace std;
using namespace Isis;

PhotometricFunction *g_pho;
static bool g_useDEM;

void phoCal (Buffer &in, Buffer &out);
void phoCalWithBackplane (std::vector<Isis::Buffer *> &in, std::vector<Isis::Buffer *> &out );
void GetCalibrationDirectory(QString calibrationType, QString &calibrationDirectory);

/**
 *
 * @brief Photometric application for the LRO NAC cameras
 *
 * This application provides features that allow multiband cubes for LRO NAC cameras
 *   to be photometrically corrected
 *
 * @author 2016-09-16 Victor Silva
 *
 * @internal
 *   @history 2016-09-19 Victor Silva - Adapted from lrowacpho written by Kris Becker
 *	 @history 2021-03-12 Victor Silva - Updates include ability to run with default values
 * 																			Added new values for 2019 version of LROC Empirical function.
 */
void IsisMain (){
  // Isis Processing by brick
  ProcessByLine p;
  // Set up input cube and get camera info
  Cube *iCube = p.SetInputCube("FROM");
  // Set up output cube
  Cube *oCube = p.SetOutputCube("TO");
  // Set up the user interface
  UserInterface &ui = Application::GetUserInterface();
  // Backplane option
  bool useBackplane = false;

  if (ui.WasEntered("BACKPLANE")) {
    CubeAttributeInput backplaneCai = ui.GetInputAttribute("BACKPLANE");

    Cube bpCube;
    bpCube.open(ui.GetFileName("BACKPLANE"));
    int bpBands = bpCube.bandCount();
    bpCube.close();

    int bpCaiBands = backplaneCai.bands().size();

    if (bpBands < 3 || (bpCaiBands != 3 && bpBands > 3)) {
      string msg = "Invalid Backplane: The backplane must be exactly 3 bands";
      throw IException(IException::User, msg, _FILEINFO_);
    }

    if (iCube->bandCount() != 1) {
      string msg = "Invalid Image: The backplane option can only be used with a single image band at a time.";
      throw IException(IException::User, msg, _FILEINFO_);
    }

    CubeAttributeInput cai;
    bpCaiBands == 3 ? cai.setAttributes("+" + backplaneCai.bands()[0]) : cai.setAttributes("+1" ) ;
    p.SetInputCube(ui.GetFileName("BACKPLANE"), cai);
    bpCaiBands == 3 ? cai.setAttributes("+" + backplaneCai.bands()[1]) : cai.setAttributes("+2" ) ;
    p.SetInputCube(ui.GetFileName("BACKPLANE"), cai);
    bpCaiBands == 3 ? cai.setAttributes("+" + backplaneCai.bands()[2]) : cai.setAttributes("+3" ) ;
    p.SetInputCube(ui.GetFileName("BACKPLANE"), cai);

    useBackplane = true;
  }

  // Get name of parameters file
  QString calDir = "";
  QString algoName = "";
  QString algoFile = ui.GetAsString("PHOPAR");

  if(algoFile.toLower() == "default" || algoFile.length() == 0){
    GetCalibrationDirectory("", calDir);
    algoFile = calDir + "NAC_PHO_LROC_Empirical.????.pvl";
  }

  FileName algoFileName(algoFile);

  if(algoFileName.isVersioned())
    algoFileName = algoFileName.highestVersion();

  if(!algoFileName.fileExists()) {
    QString msg = algoFile + " does not exist.";
    throw IException(IException::User, msg, _FILEINFO_);
  }

  Pvl params(algoFileName.expanded());



  algoName = PhotometricFunction::algorithmName(params);
  algoName = algoName.toUpper();

  // Set NAC algorithm
  if (algoName == "LROC_EMPIRICAL") {
      g_pho = new LROCEmpirical(params, *iCube, !useBackplane);
  }
  else {
    QString msg = " Algorithm Name [" + algoName + "] not recognized. ";
    msg += "Compatible Algorithms are:\n LROC_Empirical\n";
    throw IException(IException::User, msg, _FILEINFO_);
  }

  // Set user selected max and mins
  g_pho->setMinimumPhaseAngle(ui.GetDouble("MINPHASE"));
  g_pho->setMaximumPhaseAngle(ui.GetDouble("MAXPHASE"));
  g_pho->setMinimumEmissionAngle(ui.GetDouble("MINEMISSION"));
  g_pho->setMaximumEmissionAngle(ui.GetDouble("MAXEMISSION"));
  g_pho->setMinimumIncidenceAngle(ui.GetDouble("MININCIDENCE"));
  g_pho->setMaximumIncidenceAngle(ui.GetDouble("MAXINCIDENCE"));

  // Set use of DEM to calculate photometric angles
  g_useDEM = ui.GetBoolean("USEDEM");

  // Begin processing by line
  if(useBackplane) {
    p.StartProcess(phoCalWithBackplane);
  }
  else {
    p.StartProcess(phoCal);
  }
  // Start all the PVL
  PvlGroup photo("Photometry");
  g_pho->report(photo);
  oCube->putGroup(photo);
  Application::Log(photo);
  p.EndProcess();
  delete g_pho;
}//end IsisMain

/**
 * @brief Apply LROC Empirical photometric correction with backplane
 *
 * Short function dispatched for each line to apply the LROC Empirical photometric
 * correction function.
 *
 * @author 2016-09-19 Victor Silva
 *
 * @internal
 *   @history 2016-09-19 Victor silva - Adapted from lrowacpho written by Kris Becker
 *
 * @param in Buffer containing input data
 * @param out Buffer of photometrically corrected data
 */
 void phoCal(Buffer &in, Buffer &out){
   // Iterate through pixels
   for(int i = 0; i < in.size(); i++){
     // Ignore special pixels
     if(IsSpecial(in[i])){
       out[i] = in[i];
     }
     else{
       // Get correction and test for validity
       double ph = g_pho->compute(in.Line(i), in.Sample(i), in.Band(i), g_useDEM);
       out[i] = ( IsSpecial(ph) ? Null : (in[i] * ph) );
     }
   }
   return;
 }//end phoCal

 /**
  * @brief Apply LROC Empirical photometric correction with backplane
  *
  * Short function dispatched for each line to apply the LROC Empirical photometrc
  * correction function.
  *
  * @author 2016-09-19 Victor Silva
  *
  * @internal
  *   @history 2016-09-19 Victor silva - Adapted from lrowacpho written by Kris Becker
  *
  * @param in Buffer containing input data
  * @param out Buffer of photometrically corrected data
  */
 void phoCalWithBackplane ( std::vector<Isis::Buffer *> &in, std::vector<Isis::Buffer *> &out ) {

   Buffer &image = *in[0];
   Buffer &phase = *in[1];
   Buffer &emission = *in[2];
   Buffer &incidence = *in[3];
   Buffer &calibrated = *out[0];

   for (int i = 0; i < image.size(); i++) {
     //  Don't correct special pixels
     if (IsSpecial(image[i])) {
       calibrated[i] = image[i];
     }
     else {
     // Get correction and test for validity
       double ph = g_pho->photometry(incidence[i], emission[i], phase[i], image.Band(i));
       calibrated[i] = (IsSpecial(ph) ? Null : image[i] * ph);
     }
   }
   return;
 }


 /**
  * This method returns an QString containing the path of an
  * LRO calibration directory
  *
  * @param calibrationType
  * @param calibrationDirectory Path of the calibration directory
  *
  * @internal
  *   @history 2021-03-12 Victor Silva - Added option for base calibration directory
  */
 void GetCalibrationDirectory(QString calibrationType, QString &calibrationDirectory) {
   PvlGroup &dataDir = Preference::Preferences().findGroup("DataDirectory");
   QString missionDir = (QString) dataDir["LRO"];
   if(calibrationType != "")
     calibrationType += "/";

   calibrationDirectory = missionDir + "/calibration/" + calibrationType;
 }
