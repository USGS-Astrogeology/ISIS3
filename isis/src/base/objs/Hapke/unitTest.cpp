/** This is free and unencumbered software released into the public domain.
The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */
#include <iostream>
#include <cstdlib>
#include "Hapke.h"
#include "PhotoModelFactory.h"
#include "Pvl.h"
#include "PvlGroup.h"
#include "IException.h"
#include "Preference.h"

using namespace Isis;

int main() {
  Isis::Preference::Preferences(true);

  std::cout << "UNIT TEST for HapkeHen photometric function" <<
            std::endl << std::endl;

  PvlGroup algp1("Algorithm");
  algp1 += PvlKeyword("Name", "HapkeHen");

  PvlObject op1("PhotometricModel");
  op1.addGroup(algp1);

  Pvl pvl1;
  pvl1.addObject(op1);
  std::cout << pvl1 << std::endl << std::endl;

  try {
    PhotoModel *pm = PhotoModelFactory::Create(pvl1);

    std::cout << "Test phase=0.0, incidence=0.0, emission=0.0 ..." <<
              std::endl;
    std::cout << "Albedo = " << pm->CalcSurfAlbedo(0.0, 0.0, 0.0) << std::endl << std::endl;

    std::cout << "Test phase=60.0, incidence=45.0, emission=30.0 ..." <<
              std::endl;
    std::cout << "Albedo = " << pm->CalcSurfAlbedo(60.0, 45.0, 30.0) << std::endl << std::endl;

    std::cout << "Test phase=180.0, incidence=90.0, emission=90.0 ..." <<
              std::endl;
    std::cout << "Albedo = " << pm->CalcSurfAlbedo(180.0, 90.0, 90.0) << std::endl << std::endl;
  }
  catch(IException &e) {
    e.print();
  }

  PvlGroup algp2("Algorithm");
  algp2 += PvlKeyword("Name", "HapkeHen");
  algp2 += PvlKeyword("Wh", toString(0.52));
  algp2 += PvlKeyword("B0", toString(1.0));
  algp2 += PvlKeyword("Hh", toString(1.0));
  algp2 += PvlKeyword("Theta", toString(30.0));
  algp2 += PvlKeyword("Hg1", toString(0.213));
  algp2 += PvlKeyword("Hg2", toString(1.0));
  algp2 += PvlKeyword("ZeroB0St", "TRUE");

  PvlObject op2("PhotometricModel");
  op2.addGroup(algp2);

  Pvl pvl2;
  pvl2.addObject(op2);
  std::cout << pvl2 << std::endl << std::endl;

  try {
    PhotoModel *pm = PhotoModelFactory::Create(pvl2);

    std::cout << "Test phase=0.0, incidence=0.0, emission=0.0 ..." <<
              std::endl;
    std::cout << "Albedo = " << pm->CalcSurfAlbedo(0.0, 0.0, 0.0) << std::endl << std::endl;

    std::cout << "Test phase=60.0, incidence=45.0, emission=30.0 ..." <<
              std::endl;
    std::cout << "Albedo = " << pm->CalcSurfAlbedo(60.0, 45.0, 30.0) << std::endl << std::endl;

    std::cout << "Test phase=180.0, incidence=90.0, emission=90.0 ..." <<
              std::endl;
    std::cout << "Albedo = " << pm->CalcSurfAlbedo(180.0, 90.0, 90.0) << std::endl << std::endl;
  }
  catch(IException &e) {
    e.print();
  }

  PvlGroup algp3("Algorithm");
  algp3 += PvlKeyword("Name", "HapkeHen");
  algp3 += PvlKeyword("Wh", toString(0.52));
  algp3 += PvlKeyword("B0", toString(1.0));
  algp3 += PvlKeyword("Hh", toString(1.0));
  algp3 += PvlKeyword("Theta", toString(30.0));
  algp3 += PvlKeyword("Hg1", toString(0.213));
  algp3 += PvlKeyword("Hg2", toString(1.0));
  algp3 += PvlKeyword("ZeroB0St", "YES");

  PvlObject op3("PhotometricModel");
  op3.addGroup(algp3);

  Pvl pvl3;
  pvl3.addObject(op3);
  std::cout << pvl3 << std::endl << std::endl;

  try {
    PhotoModel *pm = PhotoModelFactory::Create(pvl3);

    std::cout << "Test phase=0.0, incidence=0.0, emission=0.0 ..." <<
              std::endl;
    std::cout << "Albedo = " << pm->CalcSurfAlbedo(0.0, 0.0, 0.0) << std::endl << std::endl;

    std::cout << "Test phase=60.0, incidence=45.0, emission=30.0 ..." <<
              std::endl;
    std::cout << "Albedo = " << pm->CalcSurfAlbedo(60.0, 45.0, 30.0) << std::endl << std::endl;

    std::cout << "Test phase=180.0, incidence=90.0, emission=90.0 ..." <<
              std::endl;
    std::cout << "Albedo = " << pm->CalcSurfAlbedo(180.0, 90.0, 90.0) << std::endl << std::endl;
  }
  catch(IException &e) {
    e.print();
  }

  std::cout << "UNIT TEST for HapkeLeg photometric function" <<
            std::endl << std::endl;

  PvlGroup algp4("Algorithm");
  algp4 += PvlKeyword("Name", "HapkeLeg");
  algp4 += PvlKeyword("Wh", toString(0.52));
  algp4 += PvlKeyword("B0", toString(1.0));
  algp4 += PvlKeyword("Hh", toString(1.0));
  algp4 += PvlKeyword("Bh", toString(0.0));
  algp4 += PvlKeyword("Ch", toString(0.0));
  algp4 += PvlKeyword("Theta", toString(30.0));
  algp4 += PvlKeyword("ZeroB0Standard", "FALSE");

  PvlObject op4("PhotometricModel");
  op4.addGroup(algp4);

  Pvl pvl4;
  pvl4.addObject(op4);
  std::cout << pvl4 << std::endl << std::endl;

  try {
    PhotoModel *pm = PhotoModelFactory::Create(pvl4);

    std::cout << "Test phase=0.0, incidence=0.0, emission=0.0 ..." <<
              std::endl;
    std::cout << "Albedo = " << pm->CalcSurfAlbedo(0.0, 0.0, 0.0) << std::endl << std::endl;

    std::cout << "Test phase=60.0, incidence=45.0, emission=30.0 ..." <<
              std::endl;
    std::cout << "Albedo = " << pm->CalcSurfAlbedo(60.0, 45.0, 30.0) << std::endl << std::endl;

    std::cout << "Test phase=180.0, incidence=90.0, emission=90.0 ..." <<
              std::endl;
    std::cout << "Albedo = " << pm->CalcSurfAlbedo(180.0, 90.0, 90.0) << std::endl << std::endl;
  }
  catch(IException &e) {
    e.print();
  }

  PvlGroup algp5("Algorithm");
  algp5 += PvlKeyword("Name", "HapkeLeg");
  algp5 += PvlKeyword("Wh", toString(0.52));
  algp5 += PvlKeyword("B0", toString(1.0));
  algp5 += PvlKeyword("Hh", toString(1.0));
  algp5 += PvlKeyword("Bh", toString(0.0));
  algp5 += PvlKeyword("Ch", toString(0.0));
  algp5 += PvlKeyword("Theta", toString(30.0));
  algp5 += PvlKeyword("ZeroB0Standard", "NO");

  PvlObject op5("PhotometricModel");
  op5.addGroup(algp5);

  Pvl pvl5;
  pvl5.addObject(op5);
  std::cout << pvl5 << std::endl << std::endl;

  try {
    PhotoModel *pm = PhotoModelFactory::Create(pvl5);

    std::cout << "Test phase=0.0, incidence=0.0, emission=0.0 ..." <<
              std::endl;
    std::cout << "Albedo = " << pm->CalcSurfAlbedo(0.0, 0.0, 0.0) << std::endl << std::endl;

    std::cout << "Test phase=60.0, incidence=45.0, emission=30.0 ..." <<
              std::endl;
    std::cout << "Albedo = " << pm->CalcSurfAlbedo(60.0, 45.0, 30.0) << std::endl << std::endl;

    std::cout << "Test phase=180.0, incidence=90.0, emission=90.0 ..." <<
              std::endl;
    std::cout << "Albedo = " << pm->CalcSurfAlbedo(180.0, 90.0, 90.0) << std::endl << std::endl;
  }
  catch(IException &e) {
    e.print();
  }

  PvlGroup algp6("Algorithm");
  algp6 += PvlKeyword("Name", "HapkeLeg");
  algp6 += PvlKeyword("Wh", toString(0.52));
  algp6 += PvlKeyword("B0", toString(1.0));
  algp6 += PvlKeyword("Hh", toString(1.0));
  algp6 += PvlKeyword("Bh", toString(0.0));
  algp6 += PvlKeyword("Ch", toString(0.0));
  algp6 += PvlKeyword("Theta", toString(30.0));
  algp6 += PvlKeyword("ZeroB0Standard", "MAYBE");

  PvlObject op6("PhotometricModel");
  op6.addGroup(algp6);

  Pvl pvl6;
  pvl6.addObject(op6);
  std::cout << pvl6 << std::endl << std::endl;

  try {
    PhotoModel *pm = PhotoModelFactory::Create(pvl6);

    std::cout << "Test phase=0.0, incidence=0.0, emission=0.0 ..." <<
              std::endl;
    std::cout << "Albedo = " << pm->CalcSurfAlbedo(0.0, 0.0, 0.0) << std::endl << std::endl;

    std::cout << "Test phase=60.0, incidence=45.0, emission=30.0 ..." <<
              std::endl;
    std::cout << "Albedo = " << pm->CalcSurfAlbedo(60.0, 45.0, 30.0) << std::endl << std::endl;

    std::cout << "Test phase=180.0, incidence=90.0, emission=90.0 ..." <<
              std::endl;
    std::cout << "Albedo = " << pm->CalcSurfAlbedo(180.0, 90.0, 90.0) << std::endl << std::endl;
  }
  catch(IException &e) {
    e.print();
  }

  return 0;
}
