/** This is free and unencumbered software released into the public domain.
The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */
#include <iostream>

#include <QFile>

#include "SpecialPixel.h"
#include "Stretch.h"
#include "IException.h"
#include "Preference.h"
#include "ImageHistogram.h"

using namespace Isis;
using namespace std;

int main() {
  Preference::Preferences(true);

  Stretch s;

  s.AddPair(0.0, 1.0);
  s.AddPair(0.25, 50.0);
  s.AddPair(1.0, 100.0);

  cout << "Pairs as Text: " << s.Text().toStdString() << endl;
  cout << "Number of Pairs = " << s.Pairs() << endl;
  cout << "First Input Value = " << s.Input(0) << endl;
  cout << "First Output Value = " << s.Output(0) << endl << endl;
  cout << "Stretch(0.0):    " << s.Map(0.0) << endl;
  cout << "Stretch(0.125):  " << s.Map(0.125) << endl;
  cout << "Stretch(0.25):   " << s.Map(0.25) << endl;
  cout << "Stretch(0.625):  " << s.Map(0.625) << endl;
  cout << "Stretch(1.0):    " << s.Map(1.0) << endl << endl;

  cout << "Stretch(-0.1):   " << s.Map(-0.1) << endl;
  cout << "Stretch(1.1):    " << s.Map(1.1) << endl << endl;

  s.SetNull(1.0);
  s.SetLis(2.0);
  s.SetLrs(3.0);
  s.SetHis(4.0);
  s.SetHrs(5.0);


  cout << "Stretch(Null):   " << s.Map(NULL8) << endl;
  cout << "Stretch(Lis):    " << s.Map(LOW_INSTR_SAT8) << endl;
  cout << "Stretch(Lrs):    " << s.Map(LOW_REPR_SAT8) << endl;
  cout << "Stretch(His):    " << s.Map(HIGH_INSTR_SAT8) << endl;
  cout << "Stretch(Hrs):    " << s.Map(HIGH_REPR_SAT8) << endl;
  cout << endl;

  cout << "Stretch(-0.1):   " << s.Map(-0.1) << endl;
  cout << "Stretch(1.1):    " << s.Map(1.1) << endl << endl;

  s.SetMinimum(6.0);
  s.SetMaximum(7.0);

  cout << "Stretch(-0.1):   " << s.Map(-0.1) << endl;
  cout << "Stretch(1.1):    " << s.Map(1.1) << endl << endl;


  cout << "Test ClearPairs method" << endl;
  cout << "ClearPairs()" << endl;
  s.ClearPairs();
  cout << "Pairs = " << s.Pairs() << endl << endl;

  try {
    s.AddPair(1.0, 200.0);
  }
  catch(IException &e) {
    e.print();
  }

  cout << endl << "Testing Parse" << endl;
  s.Parse("0:0 50:0 100:255 255:255");
  cout << s.Map(75.0) << endl;
  cout << endl;

  try {
    s.Parse("0:0 50:0 49:255 255:255");
  }
  catch(IException &e) {
    e.print();
  }

  try {
    s.Parse("-5xyzzy:0 50:0 100:255 255:255");
  }
  catch(IException &e) {
    e.print();
  }

  // test the Parse for when inputs are %'s
  cout << endl << "Testing new Parse that takes %'s for input side of pairs" << endl;

  ImageHistogram temp(0.0, 100.0, 101);
  ImageHistogram *h = &temp;
  for(double i = 0.0; i <= 100.0; i++) {
    h->AddData(&i, 1);
  }

  s.Parse("0:0 25:0 50:50 100:100", h);
  cout << s.Map(75.0) << endl;
  cout << endl;

  try {
    s.Parse("0:0 50:0 49:255 100:255", h);
  }
  catch(IException &e) {
    e.print();
  }

  // test for % < 0
  try {
    s.Parse("-5:10", h);
  }
  catch(IException &e) {
    e.print();
  }

  // test for % > 100
  try {
    s.Parse("121:215", h);
  }
  catch(IException &e) {
    e.print();
  }

  // test for other bad data
  try {
    s.Parse("-5xyzzy:0 50:0 100:255", h);
  }
  catch(IException &e) {
    e.print();
  }

  try {
    s.ClearPairs();
    QString fname = "unitTest.pvl";
    QString grp = "Pairs";
    s.Load(fname, grp);
    for(int i = 0; i < s.Pairs(); i++) {
      std::cout << s.Input(i) << ", " << s.Output(i) << std::endl;
    }
    std::cout << "testing save" << std::endl;
    QString output = "saveTest.pvl";
    s.Save(output, grp);
    s.ClearPairs();
    s.Load(output, grp);
    for(int i = 0; i < s.Pairs(); i++) {
      std::cout << s.Input(i) << ", " << s.Output(i) << std::endl;
    }
    QFile::remove(output);

  }
  catch(IException &e) {
    e.print();
  }

  std::cout << "testing copy pairs" << std::endl;
  s.ClearPairs();
  s.AddPair(0.0, 0.0);
  s.AddPair(255.0, 255.0);

  std::cout << "original stretch pairs" << std::endl;
  for(int i = 0; i < s.Pairs(); i++) {
    std::cout << s.Input(i) << ", " << s.Output(i) << std::endl;
  }

  Stretch sCopy;
  sCopy.CopyPairs(s);
  std::cout << "copy stretch pairs" << std::endl;
  for(int i = 0; i < s.Pairs(); i++) {
    std::cout << sCopy.Input(i) << ", " << sCopy.Output(i) << std::endl;
  }

  return 0;
}
