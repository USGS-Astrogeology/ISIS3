/** This is free and unencumbered software released into the public domain.
The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */
#include "Isis.h"

#include <QString>

#include "Cube.h"
#include "Preference.h"

#include "ProcessExportPds4.h"

using namespace std;
using namespace Isis;

/** 
 * Unit test for ProcessExportPds4 class
 *  
 * @author 2017-05-30 Marjorie Hahn 
 *  
 *  @internal
 *   @history 2017-05-30 Marjorie Hahn - Original Version
 *   @history 2016-12-28 Kristin Berry - Updated to test xml input. 
 *   @history 2018-06-06 Jeannie Backer - Removed file paths from error message written to
 *                           test output.
 *  
 */
void IsisMain() {
  Preference::Preferences(true);

  try {
    std::cout << "Testing ProcessExportPds4" << std::endl << std::endl;

    std::cout << "Testing default object" << std::endl;

    Isis::ProcessExportPds4 defaultProcess;
    QString defaultLabel = defaultProcess.GetLabel().toString();
    defaultLabel.remove(QRegExp(" xmlns.*=\".*\""));
    defaultLabel.remove(QRegExp(" xsi.*=\".*\""));
    std::cout << defaultLabel.toStdString();

    std::cout << std::endl << "Testing default CaSSIS export" << std::endl;

    Isis::ProcessExportPds4 p;

    QString cubeName = "$ISISTESTDATA/isis/src/tgo/unitTestData/CAS-MCO-2016-11-26T22.32.39.582-BLU-03025-00.cub";

    Isis::Cube cub;
    cub.open(cubeName, "r");

    p.SetInputCube(&cub);

    // Remove the schema from the lable because we cannot ensure that
    // attributes come out in the same order every time
    QString rawLabel = p.StandardPds4Label().toString();
    rawLabel.remove(QRegExp(" xmlns.*=\".*\""));
    rawLabel.remove(QRegExp(" xsi.*=\".*\""));
    std::cout << rawLabel.toStdString();

    std::ofstream ofs;
    p.OutputLabel(ofs);

    p.StartProcess(ofs);

    p.addHistory("Test history entry.");

    rawLabel = p.GetLabel().toString();
    rawLabel.remove(QRegExp(" xmlns.*=\".*\""));
    rawLabel.remove(QRegExp(" xsi.*=\".*\""));
    std::cout << rawLabel.toStdString();

    p.WritePds4("temp.img");
    remove("temp.img");
    remove("temp.xml");

    std::cout << std::endl << "Testing xml input" << std::endl; 
    Isis::ProcessExportPds4 xmlTest;
    xmlTest.SetInputCube(&cub);
    xmlTest.WritePds4("tempxml.xml");
    remove("tempxml.img");
    remove("tempxml.xml");

    std::cout << std::endl << "Testing export pixel types" << std::endl;

    ProcessExportPds4 stretchProcess;
    stretchProcess.SetOutputType(Isis::SignedWord);
    stretchProcess.SetOutputEndian(Isis::Msb);
    stretchProcess.SetInputCube(&cub);

    QString stretchedMsbSW = stretchProcess.StandardPds4Label().toString();
    stretchedMsbSW.remove(QRegExp(" xmlns.*=\".*\""));
    stretchedMsbSW.remove(QRegExp(" xsi.*=\".*\""));
    std::cout << stretchedMsbSW.toStdString();

    stretchProcess.SetOutputType(Isis::SignedWord);
    stretchProcess.SetOutputEndian(Isis::Lsb);

    stretchedMsbSW = stretchProcess.StandardPds4Label().toString();
    stretchedMsbSW.remove(QRegExp(" xmlns.*=\".*\""));
    stretchedMsbSW.remove(QRegExp(" xsi.*=\".*\""));
    std::cout << stretchedMsbSW.toStdString();

    stretchProcess.SetOutputType(Isis::Real);
    stretchProcess.SetOutputEndian(Isis::Msb);

    stretchedMsbSW = stretchProcess.StandardPds4Label().toString();
    stretchedMsbSW.remove(QRegExp(" xmlns.*=\".*\""));
    stretchedMsbSW.remove(QRegExp(" xsi.*=\".*\""));
    std::cout << stretchedMsbSW.toStdString();

    stretchProcess.SetOutputType(Isis::UnsignedWord);
    stretchProcess.SetOutputEndian(Isis::Msb);

    stretchedMsbSW = stretchProcess.StandardPds4Label().toString();
    stretchedMsbSW.remove(QRegExp(" xmlns.*=\".*\""));
    stretchedMsbSW.remove(QRegExp(" xsi.*=\".*\""));
    std::cout << stretchedMsbSW.toStdString();

    stretchProcess.SetOutputType(Isis::UnsignedWord);
    stretchProcess.SetOutputEndian(Isis::Lsb);

    stretchedMsbSW = stretchProcess.StandardPds4Label().toString();
    stretchedMsbSW.remove(QRegExp(" xmlns.*=\".*\""));
    stretchedMsbSW.remove(QRegExp(" xsi.*=\".*\""));
    std::cout << stretchedMsbSW.toStdString();

    stretchProcess.SetOutputType(Isis::UnsignedByte);
    stretchProcess.SetOutputEndian(Isis::Lsb);

    stretchedMsbSW = stretchProcess.StandardPds4Label().toString();
    stretchedMsbSW.remove(QRegExp(" xmlns.*=\".*\""));
    stretchedMsbSW.remove(QRegExp(" xsi.*=\".*\""));
    std::cout << stretchedMsbSW.toStdString();

    std::cout << std::endl << "Testing missing start and end times" << std::endl;

    PvlGroup &instGroup = cub.group("Instrument");
    instGroup["StartTime"].setValue("");
    instGroup.addKeyword( PvlKeyword("StopTime", "") );

    ProcessExportPds4 badTimeProcess;
    badTimeProcess.SetInputCube(&cub);
    badTimeProcess.StandardPds4Label();
    badTimeProcess.StartProcess(ofs);

    QString badTimeLabel = badTimeProcess.GetLabel().toString();
    badTimeLabel.remove(QRegExp(" xmlns.*=\".*\""));
    badTimeLabel.remove(QRegExp(" xsi.*=\".*\""));
    std::cout << badTimeLabel.toStdString();

    std::cout << std::endl << "Testing exporting a map projected product" << std::endl << std::endl;

    std::string projectedName("$ISISTESTDATA/isis/src/base/unitTestData/MarsPlanetaryRadius_45bottom.cub");
    Cube projectedCube(projectedName);
    ProcessExportPds4 projectedProcess;
    projectedProcess.SetInputCube(&projectedCube);

    QString projectedLabel = projectedProcess.StandardPds4Label().toString();
    projectedLabel.remove(QRegExp(" xmlns.*=\".*\""));
    projectedLabel.remove(QRegExp(" xsi.*=\".*\""));
    std::cout << projectedLabel.toStdString();

    std::cout << std::endl << "Testing errors" << std::endl << std::endl;

    try {
      std::cout << "Test creating a standard Pds4Label with no input" << std::endl;
      ProcessExportPds4 emptyProcess;
      emptyProcess.StandardPds4Label();
    }
    catch(Isis::IException &e) {
      e.print();
    }

    try {
      std::cout << "Test translating units with a non-existant config" << std::endl;
      ProcessExportPds4::translateUnits(stretchProcess.GetLabel(), "not a file");
    }
    catch(Isis::IException &e) {
      e.print();
    }

    try {
      std::cout << "Test translating units with a bad config" << std::endl;
      ProcessExportPds4::translateUnits(stretchProcess.GetLabel(), cubeName);
    }
    catch(Isis::IException &e) {
      e.print();
    }

    try {
      std::cout << "Test adding a history to an empty label" << std::endl;
      ProcessExportPds4 emptyProcess;
      emptyProcess.addHistory("Test history");
    }
    catch(Isis::IException &e) {
      e.print();
    }

    QString cassisTarget = QString::fromStdString(instGroup["targetName"]);
    try {
      std::cout << "Test a missing target" << std::endl;
      ProcessExportPds4 testProcess;
      instGroup.deleteKeyword("targetName");
      testProcess.SetInputCube(&cub);
      testProcess.StandardPds4Label();
    }
    catch(Isis::IException &e) {
      QString message = QString::fromStdString(e.toString());
      cout << message.replace(QRegExp("file.*/translations"), "file [translations").toStdString();
      cout << endl;
      cout << endl;
    }
    instGroup.addKeyword( PvlKeyword("targetName", cassisTarget.toStdString()) );
  }
  catch(Isis::IException &e) {
    e.print();
  }
}
