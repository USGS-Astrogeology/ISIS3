/** This is free and unencumbered software released into the public domain.
The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */
#include "Isis.h"

#include <QFile>

#include "ProcessImportPds.h"
#include "Application.h"
#include "IString.h"
#include "OriginalLabel.h"
#include "Statistics.h"

#include <regex>

using namespace std;
using namespace Isis;
/**
 * @internal
 *   @history 2012-05-08 Tracie Sucharski - Moved test data to /usgs/cpks/mer/testData and
 *                         /usgs/cpkgs/clementine1/testData.  Added test for invalid label.z
 *   @history 2018-01-25 Ian Humphrey - Moved .img file out of ISIS tree (since we are not
 *                           storing .img's on GitHub).
 */
void IsisMain() {

  Isis::Preference::Preferences(true);
  void ReportError(QString err);

  // Test an IMAGE file
  try {
    cout << "Testing PDS file containing an ^IMAGE pointer" << endl;
    Isis::ProcessImportPds p;
    Isis::Pvl plab;
    p.SetPdsFile("$ISISTESTDATA/isis/src/mer/unitTestData/mer.img", 
                 "$ISISTESTDATA/isis/src/mer/unitTestData/mer.img", plab);
    p.SetOutputCube("TO");
    p.StartProcess();
    p.EndProcess();

    cout << plab << endl;
    Isis::Process p2;
    Isis::CubeAttributeInput att;
    QString file = Isis::Application::GetUserInterface().GetCubeName("TO");
    Isis::Cube *cube = p2.SetInputCube(file, att);
    Isis::Statistics *stat = cube->statistics();
    cout << stat->Average() << endl;
    cout << stat->Variance() << endl;
    p2.EndProcess();
    Isis::OriginalLabel ol(file);
    Isis::Pvl label = ol.ReturnLabels();
    cout << label << endl;
    QFile::remove(file);
  }
  catch(Isis::IException &e) {
    e.print();
  }

  // Test a QUBE file
  try {
    cout << endl;
    cout << "Testing PDS file containing a ^QUBE pointer" << endl;
    Isis::ProcessImportPds p;
    Isis::Pvl plab;
    p.SetPdsFile("$ISISTESTDATA/isis/src/mer/unitTestData/mer.lab", "", plab);
    p.SetOutputCube("TO");
    p.StartProcess();
    p.OmitOriginalLabel();
    p.EndProcess();

    cout << plab << endl;
    Isis::Process p2;
    Isis::CubeAttributeInput att;
    QString file = Isis::Application::GetUserInterface().GetCubeName("TO");
    Isis::Cube *cube = p2.SetInputCube(file, att);
    Isis::Statistics *stat = cube->statistics();
    cout << stat->Average() << endl;
    cout << stat->Variance() << endl;
    p2.EndProcess();

    // Check input file error
    try {
      Isis::OriginalLabel ol(file);
    }
    catch(Isis::IException &e) {
      ReportError(QString::fromStdString(e.toString()));
    }
    QFile::remove(file);
  }
  catch(Isis::IException &e) {
    e.print();
  }

  // Test an Isis2 file
  try {
    cout << endl;
    cout << "Testing Isis2 file" << endl;
    Isis::ProcessImportPds p;
    Isis::Pvl plab;
    p.SetPdsFile("$ISISTESTDATA/isis/src/clementine/unitTestData/clemuvvis_Isis2.cub",
                 "$ISISTESTDATA/isis/src/clementine/unitTestData/clemuvvis_Isis2.cub", plab);
    p.SetOutputCube("TO");
    p.StartProcess();
    Isis::Pvl ilab;
    p.TranslateIsis2Labels(ilab);
    p.EndProcess();

    cout << ilab << endl;
    QString file = Isis::Application::GetUserInterface().GetCubeName("TO");
    QFile::remove(file);
  }
  catch(Isis::IException &e) {
    e.print();
  }

  try{// this file is saved locally since it is not needed in the data area for
      // the rest of isis
    cout << "Testing PDS file containing an ^IMAGE pointer and ^TABLE pointer" << endl;
    Isis::ProcessImportPds p;
    Isis::Pvl plab;
    p.SetPdsFile("data/pdsImageWithTables.lbl",
                 "$ISISTESTDATA/isis/src/base/objs/ProcessImportPds/pdsImageWithTables.img", plab);
    p.SetOutputCube("TO");
    p.ImportTable("SUN_POSITION_TABLE");
    p.StartProcess();
    p.EndProcess();

    cout << plab << endl;
    Isis::Process p2;
    Isis::CubeAttributeInput att;
    QString file = Isis::Application::GetUserInterface().GetCubeName("TO");
    Isis::Cube *cube = p2.SetInputCube(file, att);
    Pvl isisCubeLab = *(cube->label());
    (isisCubeLab.findObject("IsisCube").findObject("Core")["StartByte"]).setValue("");
    (isisCubeLab.findObject("Table")["StartByte"]).setValue("");
    (isisCubeLab.findObject("Table")["Bytes"]).setValue("");
    (isisCubeLab.findObject("History")["StartByte"]).setValue("");
    (isisCubeLab.findObject("History")["Bytes"]).setValue("");
    (isisCubeLab.findObject("OriginalLabel")["StartByte"]).setValue("");
    (isisCubeLab.findObject("OriginalLabel")["Bytes"]).setValue("");
    cout << isisCubeLab << endl;
    Isis::Statistics *stat = cube->statistics();
    cout << stat->Average() << endl;
    cout << stat->Variance() << endl;
    p2.EndProcess();
    Isis::OriginalLabel ol(file);
    Isis::Pvl label = ol.ReturnLabels();
    cout << label << endl;
    QFile::remove(file);
  }
  catch(Isis::IException &e) {
    e.print();
  }

  //  Test an invalid label file
  try {
    cout << endl;
    cout << "Testing file with invalid Pds label" << endl;
    Isis::ProcessImportPds p;
    Isis::Pvl plab;
    p.SetPdsFile("$ISISTESTDATA/isis/src/galileo/unitTestData/1213r.img", "$ISISTESTDATA/isis/src/galileo/unitTestData/1213r.img", plab);
  }
  catch (Isis::IException &e) {
    ReportError(QString::fromStdString(e.toString()));
  }
  
  //  Test that defaults for projection offsets are changed and can be returned
  {
    cout << endl << "********************************************************************" << endl;
    cout << "Test that defaults for projection offsets are changed and can be returned" << endl;
    Isis::ProcessImportPds p;
    Pvl plab;
    p.SetPdsFile("$ISISTESTDATA/isis/src/base/unitTestData/ff17.lbl", "$ISISTESTDATA/isis/src/base/unitTestData/ff17.img", plab);
    p.SetOutputCube("TO");
    p.StartProcess();
    Pvl proj;
    p.TranslatePdsProjection(proj);
    if (p.GetProjectionOffsetChange()) {
      cout << "Projection offsets were changed. New values:" << endl;
      PvlGroup group = p.GetProjectionOffsetGroup();
      for (int i = 0; i < group.keywords(); i++) {
        PvlKeyword temp = group[i];
        cout << temp.name() + " = " + temp[0] << endl;
      }
    }
    
    p.EndProcess();
    QFile::remove(Isis::Application::GetUserInterface().GetCubeName("TO"));
  }
  
}

/**
 * Reports error messages from Isis:iException without full paths of filenames
 * @param err Error QString of iException
 * @author Jeannie Walldren
 * @internal
 *   @history 2011-08-05 Jeannie Backer - Copied from Cube class.
 *
 *   @history 2017-05-19 Christopher Combs - Changed to remove paths that would cause the
 *                           cause the test to fail when not using the standard data area.
 *                           Fixes #4738.
 */
void ReportError(QString err) {
  std::regex pattern("(\\/[\\w\\-\\. ]*)+\\/galileo");
  cout << std::regex_replace(err.toStdString(), pattern, "galileo") << endl;
}
