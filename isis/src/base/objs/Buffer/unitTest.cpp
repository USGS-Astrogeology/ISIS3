/** This is free and unencumbered software released into the public domain.
The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */
#include <iostream>
#include "Buffer.h"
#include "IException.h"
#include "Preference.h"

using namespace std;
using namespace Isis;

class Test : public Buffer {
  public:
    Test(int s, int l, int b) : Isis::Buffer(s, l, b, Isis::SignedInteger) {};
    ~Test() {};
    void print();
};

void Test::print() {
  SetBasePosition(3, 2, 1);
  cout << "Sample():         " << Sample() << endl;
  cout << "Line():           " << Line() << endl;
  cout << "Band():           " << Band() << endl;

  int samp, line, band;
  Position(0, samp, line, band);
  cout << "Position:         " << samp << " " << line << " " << band
       << endl;
  cout << "Index:            " << Index(samp, line, band) << endl << endl;

  cout << "Sample(16):       " << Sample(16) << endl;
  cout << "Line(16):         " << Line(16) << endl;
  cout << "Band(16):         " << Band(16) << endl;

  Position(16, samp, line, band);
  cout << "Position:         " << samp << " " << line << " " << band
       << endl;
  cout << "Index:            " << Index(samp, line, band) << endl << endl;
}

int main(int argc, char *argv[]) {
  Isis::Preference::Preferences(true);
  cout << "Isis::Buffer Unit Test" << endl << endl;

  Test b(4, 3, 2);
  for(int i = 0; i < b.size(); i++) {
    b[i] = i;
  }

  cout << "SampleDimension:  " << b.SampleDimension() << endl;
  cout << "LineDimension:    " << b.LineDimension() << endl;
  cout << "BandDimension:    " << b.BandDimension() << endl;
  cout << "Size:             " << b.size() << endl << endl;

  cout << "Sample():         " << b.Sample() << endl;
  cout << "Line():           " << b.Line() << endl;
  cout << "Band():           " << b.Band() << endl;

  int samp, line, band;
  b.Position(0, samp, line, band);
  cout << "Position:         " << samp << " " << line << " " << band
       << endl;
  cout << "Index:            " << b.Index(samp, line, band) << endl << endl;

  cout << "at(0):            " << b.at(0) << endl;
  cout << "at(10):           " << b.at(10) << endl;
  cout << "at(23):           " << b.at(23) << endl;
  cout << "b[0]:             " << b[0] << endl;
  cout << "b[10]:            " << b[10] << endl;
  cout << "b[23]:            " << b[23] << endl << endl;

  b.print();

  Test a = b;
  if(a.DoubleBuffer() != b.DoubleBuffer()) {
    cout << "Copy constructor: Worked" << endl;
  }
  else {
    cout << "Copy constructor: Failed" << endl;
  }
  cout << "SampleDimension:  " << a.SampleDimension() << endl;
  cout << "LineDimension:    " << a.LineDimension() << endl;
  cout << "BandDimension:    " << a.BandDimension() << endl;
  cout << "Size:             " << a.size() << endl ;
  cout << "a[0]:             " << a[0] << endl;
  cout << "a[23]:            " << a[23] << endl << endl;

  cout << "PixelType =       " << Isis::PixelTypeName(a.PixelType()).toStdString() << endl;
  cout << endl;

  //  Test new default constructor.   Enclose in braces so destructor is tested
  {
    Buffer nullbuf;
    cout << "Null Buffer size: " << nullbuf.size() << endl << endl;
  }

  // Test assignment operator
  cout << "Test assignment operator for a constant...\n";
  Buffer d(2, 2, 2, Double);
  d = 999.0;
  cout << "d.size():         " << d.size() << endl;
  cout << "d[0]:             " << d[0] << endl;
  cout << "d[2]:             " << d[2] << endl;
  cout << "d[n]:             " << d[d.size()-1] << endl << endl;

  try {
    a.at(-1);
  }
  catch(Isis::IException &e) {
    e.print();
  }

  try {
    a.at(24);
  }
  catch(Isis::IException &e) {
    e.print();
  }

  try {
    Buffer a2(2, 2, 2, Isis::SignedWord);
    Buffer b2(2, 2, 2, Isis::SignedByte);

    a2.Copy(b2);
  }
  catch(Isis::IException &e) {
    e.print();
  }

  return 0;
}
