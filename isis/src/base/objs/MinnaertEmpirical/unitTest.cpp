/** This is free and unencumbered software released into the public domain.
The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */
#include <iostream>
#include <cstdlib>
#include "MinnaertEmpirical.h"
#include "PhotoModelFactory.h"
#include "Pvl.h"
#include "PvlGroup.h"
#include "IException.h"
#include "Preference.h"

using namespace Isis;

int main() {
  Isis::Preference::Preferences(true);

  std::cout << "UNIT TEST for Minnaert Empirical photometric function" <<
            std::endl << std::endl;

  PvlGroup alg("Algorithm");
  alg += PvlKeyword("Name", "MinnaertEmpirical");
  alg += PvlKeyword("PhaseList", "0.,10.,20.,30.,40.,50.,60.,70.,80.,90.,100.,110.,120.,130.,140.,150.,160.,170.,180.");
  alg += PvlKeyword("KList", "0.505,0.584,0.650,0.700,0.744,0.787,0.828,0.865,0.894,0.912,0.918,0.926,0.944,0.973,1.004,1.045,1.083,1.088,1.092");
  alg += PvlKeyword("PhaseCurveList", "0.03335,0.03366,0.03328,0.03220,0.03074,0.02909,0.02737,0.02559,0.02369,0.02152,0.01902,0.01651,0.01427,0.01244,0.01079,0.009452,0.007869,0.004866,0.");

  PvlObject o("PhotometricModel");
  o.addGroup(alg);

  Pvl pvl;
  pvl.addObject(o);
  std::cout << pvl << std::endl << std::endl;

  try {
    PhotoModel *pm = PhotoModelFactory::Create(pvl);

    pm->SetPhotoPhaseList("0.,10.,20.,30.,40.,50.,60.,70.,80.,90.,100.,110.,120.,130.,140.,150.,160.,170.,180.");
    pm->SetPhotoKList("0.505,0.584,0.650,0.700,0.744,0.787,0.828,0.865,0.894,0.912,0.918,0.926,0.944,0.973,1.004,1.045,1.083,1.088,1.092");
    pm->SetPhotoPhaseCurveList("0.03335,0.03366,0.03328,0.03220,0.03074,0.02909,0.02737,0.02559,0.02369,0.02152,0.01902,0.01651,0.01427,0.01244,0.01079,0.009452,0.007869,0.004866,0.");

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
  algOtherFormat += PvlKeyword("Name", "MinnaertEmpirical");

  PvlKeyword phaseList("PhaseList");
  PvlKeyword kList("KList"); 
  PvlKeyword phaseCurveList("PhaseCurveList");

  for (int i=0; i < 15; i++) {
    phaseList += std::to_string(i*10);
    kList += std::to_string(i*0.1); 
    phaseCurveList += std::to_string(i*0.3);
  }

  algOtherFormat += phaseList; 
  algOtherFormat += kList; 
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
