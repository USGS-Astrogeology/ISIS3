/** This is free and unencumbered software released into the public domain.
The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */
#include "Isis.h"

#include <string>
#include <iostream>

#include "Application.h"
#include "Cube.h"
#include "FileName.h"
#include "ImagePolygon.h"
#include "OriginalLabel.h"
#include "Process.h"
#include "Pvl.h"

using namespace std;
using namespace Isis;

void IsisMain() {
  Isis::Preference::Preferences(true);

  cout << "Testing Isis::Process Class ... " << endl;
  Isis::Process p;

  cout << "Testing GetUserInterface ... " << endl;
  Isis::UserInterface &ui = Isis::Application::GetUserInterface();
  cout << ui.GetAsString("FROM").toStdString() << endl;
  cout << endl;

  cout << "Testing SetOutputCube without an input cube ..." << endl;
  try {
    p.SetOutputCube("TO");
  }
  catch(Isis::IException &e) {
    e.print();
  }
  cout << endl;

  cout << "Testing SetOutputCube with bogus samples ..." << endl;
  try {
    p.SetOutputCube("TO", 0, 1, 1);
  }
  catch(Isis::IException &e) {
    e.print();
  }
  cout << endl;

  cout << "Testing SetOutputCube with bogus lines ..." << endl;
  try {
    p.SetOutputCube("TO", 1, 0, 1);
  }
  catch(Isis::IException &e) {
    e.print();
  }
  cout << endl;

  cout << "Testing SetOutputCube with bogus bands ..." << endl;
  try {
    p.SetOutputCube("TO", 1, 1, 0);
  }
  catch(Isis::IException &e) {
    e.print();
  }
  cout << endl;

  cout << "Testing SetInputCube/SetInputWorkCube ... " << endl;
  Isis::Cube *icube = p.SetInputCube("FROM");
  cout << "Samples:  " << icube->sampleCount() << endl;
  cout << "Lines:  " << icube->lineCount() << endl;
  cout << "Bands:  " << icube->bandCount() << endl;
  cout << endl;

  cout << "Testing SetInputWorkCube OneBand Error ..." << endl;
  try {
    p.SetInputCube("FROM", Isis::OneBand);
  }
  catch(Isis::IException &e) {
    cout << "An exception was thrown!" << endl;
  }
  cout << endl;

  cout << "Testing SetInputWorkCube SizeMatch error for bands ..." << endl;
  try {
    Isis::CubeAttributeInput att("+1");
    p.SetInputCube(QString::fromStdString(FileName("$ISISTESTDATA/isis/src/base/unitTestData/Process/unitTest.cub").expanded()),
                   att, Isis::SizeMatch);
  }
  catch(Isis::IException &e) {
    e.print();
  }
  cout << endl;

  cout << "Testing SetInputWorkCube SizeMatch error for lines ..." << endl;
  Isis::Cube cube;
  cube.setDimensions(126, 100, 2);
  cube.create(QString::fromStdString(FileName("$TEMPORARY/isisprocess_01").expanded()));
  cube.close();
  try {
    Isis::Process p2;
    Isis::CubeAttributeInput att;
    p2.SetInputCube(QString::fromStdString(FileName("$TEMPORARY/isisprocess_01").expanded()), att);
    p2.SetInputCube(QString::fromStdString(FileName("$ISISTESTDATA/isis/src/base/unitTestData/Process/unitTest.cub").expanded()), att,
                    Isis::SizeMatch);
  }
  catch(Isis::IException &e) {
    e.print();
  }
  cout << endl;

  cout << "Testing SetInputWorkCube SizeMatch error for samples ..." << endl;
  cube.setDimensions(100, 126, 2);
  cube.create(QString::fromStdString(FileName("$TEMPORARY/isisprocess_02").expanded()));
  cube.close();
  try {
    Isis::Process p2;
    Isis::CubeAttributeInput att;
    p2.SetInputCube(QString::fromStdString(FileName("$TEMPORARY/isisprocess_02").expanded()), att);
    p2.SetInputCube(QString::fromStdString(FileName("$ISISTESTDATA/isis/src/base/unitTestData/Process/unitTest.cub").expanded()), att,
                    Isis::SizeMatch);
  }
  catch(Isis::IException &e) {
    e.print();
  }
  cout << endl;

  cout << "Testing SetInputWorkCube SpatialMatch error for lines ..." << endl;
  try {
    Isis::Process p2;
    Isis::CubeAttributeInput att;
    p2.SetInputCube(QString::fromStdString(FileName("$TEMPORARY/isisprocess_01").expanded()), att);
    p2.SetInputCube(QString::fromStdString(FileName("$ISISTESTDATA/isis/src/base/unitTestData/Process/unitTest.cub").expanded()), att,
                    Isis::SpatialMatch);
  }
  catch(Isis::IException &e) {
    e.print();
  }
  cout << endl;

  cout << "Testing SetInputWorkCube SpatialMatch error for samples ..." << endl;
  try {
    Isis::Process p2;
    Isis::CubeAttributeInput att;
    p2.SetInputCube(QString::fromStdString(FileName("$TEMPORARY/isisprocess_02").expanded()), att);
    p2.SetInputCube(QString::fromStdString(FileName("$ISISTESTDATA/isis/src/base/unitTestData/Process/unitTest.cub").expanded()), att,
                    Isis::SpatialMatch);
  }
  catch(Isis::IException &e) {
    e.print();
  }
  cout << endl;

  cout << "Testing SetInputWorkCube BandMatchOrOne error ..." << endl;
  cube.setDimensions(126, 126, 3);
  cube.create(QString::fromStdString(FileName("$TEMPORARY/isisprocess_03").expanded()));
  cube.close();
  try {
    Isis::Process p2;
    Isis::CubeAttributeInput att;
    p2.SetInputCube(QString::fromStdString(FileName("$TEMPORARY/isisprocess_03").expanded()), att);
    p2.SetInputCube(QString::fromStdString(FileName("$ISISTESTDATA/isis/src/base/unitTestData/Process/unitTest.cub").expanded()), att,
                    Isis::BandMatchOrOne);
  }
  catch(Isis::IException &e) {
    e.print();
  }
  cout << endl;

  cout << "Testing SetInputWorkCube AllMatchOrOne error ..." << endl;
  cube.setDimensions(126, 126, 3);
  cube.create(QString::fromStdString(FileName("$TEMPORARY/isisprocess_03").expanded()));
  cube.close();
  try {
    Isis::Process p2;
    Isis::CubeAttributeInput att;
    p2.SetInputCube(QString::fromStdString(FileName("$TEMPORARY/isisprocess_03").expanded()), att);
    p2.SetInputCube(QString::fromStdString(FileName("$ISISTESTDATA/isis/src/base/unitTestData/Process/unitTest.cub").expanded()), att,
                    Isis::AllMatchOrOne);
  }
  catch(Isis::IException &e) {
    e.print();
  }
  cout << endl;

  cout << "Testing Logging ..." << endl;
  Isis::PvlGroup results("Results");
  results += Isis::PvlKeyword("Test", "Me");
  results += Isis::PvlKeyword("No", "Way");
  Isis::Application::Log(results);
  cout << endl;

  cout << "Testing label propagation (on) ..." << endl;
  Isis::Process p3;
  p3.SetInputCube("FROM");
  Isis::Cube *ocube = p3.SetOutputCube("TO");
  Isis::PvlGroup lab = ocube->group("Test");
  cout << lab["Keyword"] << endl;
  cout << endl;

  cout << "Testing label propagation (off) ..." << endl;
  p3.PropagateLabels(false);
  Isis::Cube *ocube4 = p3.SetOutputCube("TO4", 126, 126, 1);
  if(!ocube4->hasGroup("Test")) {
    cout << "Group Test does not exist" << endl;
  }
  p3.EndProcess();
  cout << endl;

  cout << "Testing OriginalLabel propagation (on) ..." << endl;
  Isis::Process p4;
  p4.SetInputCube("FROM");
  Isis::OriginalLabel ol(ui.GetAsString("FROM"));
  Isis::Pvl labels = ol.ReturnLabels();
  cout << labels << endl;
  cout << endl;

  cout << "Testing OriginalLabel propagation (off) ..." << endl;
  p4.PropagateOriginalLabel(false);
  Isis::Cube *ocube5 = p4.SetOutputCube("TO4", 126, 126, 1);
  if(!ocube5->hasGroup("OriginalLabel")) {
    cout << "Group OriginalLabel does not exist" << endl;
  }
  p4.EndProcess();
  cout << endl;

  cout << "Testing Table propagation (on) ..." << endl;
  Isis::Process p5;
  p5.SetInputCube("FROM");
  Isis::Cube *ocube6 = p5.SetOutputCube("TO");
  Isis::Table table = ocube6->readTable("Table");

  cout << "Number of record = " << table.Records() << endl;
  cout << "Record Size = " << table.RecordSize() << endl;
  cout << endl;

  cout << "Testing Table propagation (off) ..." << endl;
  p4.PropagateTables(false);
  Isis::Cube *ocube7 = p5.SetOutputCube("TO4", 126, 126, 1);
  if(!ocube7->hasGroup("Table")) {
    cout << "Group Table does not exist" << endl;
  }
  p5.EndProcess();
  cout << endl;

  cout << "Testing Table propagation with list of table names to propagate (Table2) ..." << endl;
  Isis::Process pTableNames;
  pTableNames.SetInputCube("FROM");
  pTableNames.PropagateTables(false);
  Isis::Cube *ocubeTableNames = pTableNames.SetOutputCube("TO");

  // Create the list of tables to copy from the unitTest.cub (only copy Table2)
  QList<QString> tables;
  tables << "Table2";
  pTableNames.PropagateTables(QString::fromStdString(FileName("$ISISTESTDATA/isis/src/base/unitTestData/Process/unitTest.cub").expanded()), tables);
  cout << "Does output cube have \"Table\"  ? " << std::boolalpha
       << ocubeTableNames->hasTable("Table") << endl;
  cout << "Does output cube have \"Table2\" ? " << std::boolalpha
       << ocubeTableNames->hasTable("Table2") << endl;

  Isis::Table table2 = ocubeTableNames->readTable("Table2");
  cout << "Number of records = " << table2.Records() << endl;
  cout << "Record Size = " << table2.RecordSize() << endl;

  pTableNames.EndProcess();
  cout << endl;

  cout << "Testing Polygon propagation (on) ..." << endl;
  Isis::Process p6;
  p6.SetInputCube("FROM");
  Isis::Cube *ocube8 = p6.SetOutputCube("TO");

  Isis::Pvl *inlab1 = ocube8->label();
  for(int i = 0; i < inlab1->objects(); i++) {
    if(inlab1->object(i).isNamed("Polygon")) {
      cout << "Image Polygon does exist" << endl;
      cout << "Size: " << (int)inlab1->object(i)["Bytes"] << endl;
      // We cannot instantiate without spice data, so we won't try.
    }
  }

  cout << "Testing Polygon propagation (off) ..." << endl;
  p6.PropagatePolygons(false);
  bool exists = false;
  Isis::Cube *ocube9 = p6.SetOutputCube("TO4", 126, 126, 1);
  Isis::Pvl *inlab2 = ocube9->label();
  for(int i = 0; i < inlab2->objects(); i++) {
    if(inlab2->object(i).isNamed("Polygon")) {
      cout << "Image Polygon does exist" << endl;
      exists = true;
    }
  }
  if(!exists) {
    cout << "Image Polygon does not exist" << endl;
  }

  p6.EndProcess();
  cout << endl;

  cube.open(QString::fromStdString(FileName("$TEMPORARY/isisprocess_01").expanded()));
  cube.close(true);
  cube.open(QString::fromStdString(FileName("$TEMPORARY/isisprocess_02").expanded()));
  cube.close(true);
  cube.open(QString::fromStdString(FileName("$TEMPORARY/isisprocess_03").expanded()));
  cube.close(true);
  cube.open(QString::fromStdString(FileName("$TEMPORARY/isisprocess_04").expanded()));
  cube.close(true);
}

