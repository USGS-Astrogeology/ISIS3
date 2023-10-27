/** This is free and unencumbered software released into the public domain.
The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */
#include <iostream>
#include <cstdlib>
#include "LunarLambertEmpirical.h"
#include "PhotoModelFactory.h"
#include "Pvl.h"
#include "PvlGroup.h"
#include "IException.h"
#include "Preference.h"

using namespace Isis;

int main() {
  Isis::Preference::Preferences(true);

  std::cout << "UNIT TEST for Lunar Lambert Empirical photometric function" <<
            std::endl << std::endl;

  PvlGroup alg("Algorithm");
  alg += PvlKeyword("Name", "LunarLambertEmpirical");
  alg += PvlKeyword("PhaseList", "0.,10.,20.,30.,40.,50.,60.,70.,80.,90.,100.,110.,120.,130.,140.,150.,160.,170.,180.");
  alg += PvlKeyword("LList", "0.986,0.778,0.641,0.545,0.457,0.372,0.289,0.211,0.143,0.086,0.041,0.009,-0.009,-0.020,-0.025,-0.029,-0.027,-0.011,-0.010");
  alg += PvlKeyword("PhaseCurveList", "0.03338,0.03386,0.03350,0.03247,0.03109,0.02949,0.02780,0.02608,0.02432,0.02246,0.02050,0.01832,0.01599,0.01363,0.01134,0.009113,0.006710,0.003510,0.");

  PvlObject o("PhotometricModel");
  o.addGroup(alg);

  Pvl pvl;
  pvl.addObject(o);
  std::cout << pvl << std::endl << std::endl;

  try {
    PhotoModel *pm = PhotoModelFactory::Create(pvl);

    pm->SetPhotoPhaseList("0.,10.,20.,30.,40.,50.,60.,70.,80.,90.,100.,110.,120.,130.,140.,150.,160.,170.,180.");
    pm->SetPhotoLList("0.986,0.778,0.641,0.545,0.457,0.372,0.289,0.211,0.143,0.086,0.041,0.009,-0.009,-0.020,-0.025,-0.029,-0.027,-0.011,-0.010");
    pm->SetPhotoPhaseCurveList("0.03338,0.03386,0.03350,0.03247,0.03109,0.02949,0.02780,0.02608,0.02432,0.02246,0.02050,0.01832,0.01599,0.01363,0.01134,0.009113,0.006710,0.003510,0.");

    std::vector<double>phaselist = pm->PhotoPhaseList();

    std::cout << "Test phase=0.0, incidence=0.0, emission=0.0 ..." <<
              std::endl;
    std::cout << "Albedo = " << pm->CalcSurfAlbedo(0.0, 0.0, 0.0) <<
              std::endl;
    std::cout << "Test phase=38.0, incidence=11.0, emission=20.0 ..." <<
              std::endl;
    std::cout << "Albedo = " << pm->CalcSurfAlbedo(38.0, 11.0, 20.0) <<
              std::endl;
    std::cout << "Test phase=65.0, incidence=45.0, emission=30.0 ..." <<
              std::endl;
    std::cout << "Albedo = " << pm->CalcSurfAlbedo(65.0, 45.0, 30.0) <<
              std::endl;
    std::cout << "Test phase=127.0, incidence=52.0, emission=33.0 ..." <<
              std::endl;
    std::cout << "Albedo = " << pm->CalcSurfAlbedo(127.0, 52.0, 33.0) <<
              std::endl;
    std::cout << "Test phase=180.0, incidence=90.0, emission=90.0 ..." <<
              std::endl;
    std::cout << "Albedo = " << pm->CalcSurfAlbedo(180.0, 90.0, 90.0) <<
              std::endl << std::endl;
  }
  catch(IException &e) {
    e.print();
  }

  // Test Keyword = (1,2,3,4,5) format for input
  // 
  // The actual numbers used for this test are not relevant -- this test's primary purpose is to 
  // ensure that this format of input is usable for the calculations done by the class without the 
  // program error-ing out. 
  PvlGroup algOtherFormat("Algorithm");
  algOtherFormat += PvlKeyword("Name", "LunarLambertEmpirical");

  PvlKeyword phaseList("PhaseList");
  PvlKeyword lList("LList"); 
  PvlKeyword phaseCurveList("PhaseCurveList");

  for (int i=0; i < 15; i++) {
    phaseList += std::to_string(i*10);
    lList += std::to_string(i*0.1); 
    phaseCurveList += std::to_string(i*0.3);
  }

  algOtherFormat += phaseList; 
  algOtherFormat += lList; 
  algOtherFormat += phaseCurveList; 

  PvlObject photometricModel("PhotometricModel");
  photometricModel.addGroup(algOtherFormat);

  Pvl pvlOtherFormat;
  pvlOtherFormat.addObject(photometricModel);
  std::cout << pvlOtherFormat << std::endl << std::endl;

  try {
    PhotoModel *pm = PhotoModelFactory::Create(pvlOtherFormat);

    std::vector<double>phaselist = pm->PhotoPhaseList();

    std::cout << "Test phase=0.0, incidence=0.0, emission=0.0 ..." <<
              std::endl;
    std::cout << "Albedo = " << pm->CalcSurfAlbedo(0.0, 0.0, 0.0) <<
              std::endl;
    std::cout << "Test phase=38.0, incidence=11.0, emission=20.0 ..." <<
              std::endl;
    std::cout << "Albedo = " << pm->CalcSurfAlbedo(38.0, 11.0, 20.0) <<
              std::endl;
    std::cout << "Test phase=65.0, incidence=45.0, emission=30.0 ..." <<
              std::endl;
    std::cout << "Albedo = " << pm->CalcSurfAlbedo(65.0, 45.0, 30.0) <<
              std::endl;
    std::cout << "Test phase=127.0, incidence=52.0, emission=33.0 ..." <<
              std::endl;
    std::cout << "Albedo = " << pm->CalcSurfAlbedo(127.0, 52.0, 33.0) <<
              std::endl;
    std::cout << "Test phase=180.0, incidence=90.0, emission=90.0 ..." <<
              std::endl;
    std::cout << "Albedo = " << pm->CalcSurfAlbedo(180.0, 90.0, 90.0) <<
              std::endl << std::endl;
  }
  catch(IException &e) {
    e.print();
  }

  return 0;
}
