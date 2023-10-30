#include "Application.h"
#include "ProcessByLine.h"
#include "LROCEmpirical.h"
#include "PhotometricFunction.h"
#include "Buffer.h"
#include "Camera.h"
#include "iTime.h"
#include "SpecialPixel.h"
#include "Pvl.h"
#include "PvlGroup.h"
#include "UserInterface.h"
#include "lronacpho.h"

using namespace std;

namespace Isis{

  // globals
  static bool g_useDEM;
  PhotometricFunction *g_phoFunction;

  // forward declare helper functions
  void phoCal (Buffer &in, Buffer &out);
  void phoCalWithBackplane (vector<Isis::Buffer *> &in, vector<Isis::Buffer *> &out );

  /**
    * @brief Photometric application for the LRO NAC cameras
    *
    * This application provides features that allow multiband cubes for LRO NAC cameras
    * to be photometrically correctedConstructor
    *
    * @param ui The user interfact to parse the parameters from. 
    */
  void lronacpho(UserInterface &ui, Pvl *log){
    Cube iCube(ui.GetCubeName("FROM"));
    lronacpho(&iCube, ui, log);
  }

  /**
    * @brief Photometric application for the LRO NAC cameras
    *
    * This application provides features that allow multiband cubes for LRO NAC cameras
    * to be photometrically corrected
    * @author 2016-09-16 Victor Silva
    *
    * @internal
    *  @history 2016-09-19 Victor Silva - Adapted from lrowacpho written by Kris Becker
    *	 @history 2021-03-12 Victor Silva - Updates"PostCall" include ability to run with default values
    * 																			Added new values for 2019 version of LROC Empirical function.
    *  @history 2022-04-18 Victor Silva - Refactored to make callable for GTest framework                                   
    *
    * @param iCube The input cube to be photometrically corrected.
    * @param ui The user interfact to parse the parameters from. 
    */
  void lronacpho(Cube *iCube, UserInterface &ui, Pvl *log){
    ProcessByLine p;
    p.SetInputCube(iCube);

    Cube *oCube = p.SetOutputCube(ui.GetCubeName("TO"), ui.GetOutputAttribute("TO"));//, 
    
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
    QString algoName = "";
    QString algoFile = ui.GetAsString("PHOPAR");

    FileName algoFileName(algoFile);

    if(algoFileName.isVersioned())
      algoFileName = algoFileName.highestVersion();
    
    if(!algoFileName.fileExists()) {
      QString msg = algoFile + " does not exist.";
      throw IException(IException::User, msg, _FILEINFO_);
    }

    Pvl params(algoFileName.expanded().toStdString());

    algoName = PhotometricFunction::algorithmName(params);
    algoName = algoName.toUpper();

    // Set NAC algorithm
    if (algoName == "LROC_EMPIRICAL") {
        g_phoFunction = new LROCEmpirical(params, *iCube, !useBackplane);
    }
    else {
      QString msg = " Algorithm Name [" + algoName + "] not recognized. ";
      msg += "Compatible Algorithms are:\n LROC_Empirical\n";
      throw IException(IException::User, msg, _FILEINFO_);
    }

    // Set user selected max and mins
    g_phoFunction->setMinimumPhaseAngle(ui.GetDouble("MINPHASE"));
    g_phoFunction->setMaximumPhaseAngle(ui.GetDouble("MAXPHASE"));
    g_phoFunction->setMinimumEmissionAngle(ui.GetDouble("MINEMISSION"));
    g_phoFunction->setMaximumEmissionAngle(ui.GetDouble("MAXEMISSION"));
    g_phoFunction->setMinimumIncidenceAngle(ui.GetDouble("MININCIDENCE"));
    g_phoFunction->setMaximumIncidenceAngle(ui.GetDouble("MAXINCIDENCE"));

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
    g_phoFunction->report(photo);
        
    oCube->putGroup(photo);

    Application::Log(photo);

    p.EndProcess();
    p.ClearInputCubes();
    delete g_phoFunction;
  }
  
  /**
    * @brief Apply LROC Empirical photometric correction
    *
    * Short function dispatched for each line to apply the LROC Empirical photometric
    * correction function.
    *
    * @author 2016-09-19 Victor Silva
    *
    * @internal
    *   @history 2016-09-19 Victor silva - Adapted from lrowacpho written by Kris Becker
    *   @history 2022-04-18 Victor Silva - Refactored to make callable for GTest framework
    *
    * @param in Buffer containing input data
    * @param out Buffer of photometrically corrected data
    */
  void phoCal(Buffer &in, Buffer &out) {
    // Iterate through pixels
    for(int i = 0; i < in.size(); i++){
      // Ignore special pixels
      if(IsSpecial(in[i])){
        out[i] = in[i];
      }
      else{
        // Get correction and test for validity
        double ph = g_phoFunction->compute(in.Line(i), in.Sample(i), in.Band(i), g_useDEM);
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
    *   @history 2022-04-18 Victor Silva - Refactored to make callable for GTest framework
    *
    * @param in Buffer containing input data
    * @param out Buffer of photometrically corrected data
    */
  void phoCalWithBackplane( std::vector<Isis::Buffer *> &in, std::vector<Isis::Buffer *> &out ){

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
        double ph = g_phoFunction->photometry(incidence[i], emission[i], phase[i], image.Band(i));
        calibrated[i] = (IsSpecial(ph) ? Null : image[i] * ph);
      }
    }
    return;
  }
}

