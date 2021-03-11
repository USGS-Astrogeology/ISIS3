#include <iostream>
#include <iomanip>
#include "IException.h"
#include "IString.h"
#include "SpecialPixel.h"
#include "Preference.h"

#include <climits>

using namespace Isis;
using namespace std;

int main(int argc, char *argv[]) {
  Isis::Preference::Preferences(true);

  cout << "Unit test for IsisSpecialPixel" << endl;

  cout << "Valid minimum (1 byte):           " << (int) Isis::VALID_MIN1 << endl;
  cout << "Null (1 byte):                    " << (int) Isis::NULL1 << endl;
  cout << "Low Representation (1 byte):      " << (int) Isis::LOW_REPR_SAT1 << endl;
  cout << "Low Instrument (1 byte):          " << (int) Isis::LOW_INSTR_SAT1 << endl;
  cout << "High Representation (1 byte):     " << (int) Isis::HIGH_REPR_SAT1 << endl;
  cout << "High Instrument (1 byte):         " << (int) Isis::HIGH_INSTR_SAT1 << endl;
  cout << "Valid maximum (1 byte):           " << (int) Isis::VALID_MAX1 << endl;
  cout << endl;

  cout << "Valid minimum (2 byte):           " << Isis::VALID_MIN2 << endl;
  cout << "Null (2 byte):                    " << Isis::NULL2 << endl;
  cout << "Low Representation (2 byte):      " << Isis::LOW_REPR_SAT2 << endl;
  cout << "Low Instrument (2 byte):          " << Isis::LOW_INSTR_SAT2 << endl;
  cout << "High Representation (2 byte):     " << Isis::HIGH_REPR_SAT2 << endl;
  cout << "High Instrument (2 byte):         " << Isis::HIGH_INSTR_SAT2 << endl;
  cout << "Valid maximum (2 byte):           " << Isis::VALID_MAX2 << endl;
  cout << endl;

  cout << "Valid minimum (4 byte):           " << Isis::VALID_MIN4 << endl;
  cout << "Null (4 byte):                    " << Isis::NULL4 << endl; 
  cout << "Low Representation (4 byte):      " << Isis::LOW_REPR_SAT4 << endl;
  cout << "Low Instrument (4 byte):          " << Isis::LOW_INSTR_SAT4 << endl;
  cout << "High Representation (4 byte):     " << Isis::HIGH_REPR_SAT4 << endl;
  cout << "High Instrument (4 byte):         " << Isis::HIGH_INSTR_SAT4 << endl;
  cout << "Valid maximum (4 byte):           " << Isis::VALID_MAX4 << endl;
  cout << endl;

  cout << setprecision(16);
  cout << "Valid minimum (8 byte):           " << Isis::VALID_MIN8 << endl;
  cout << "Null (8 byte):                    " << Isis::NULL8 << endl;
  cout << "Low Representation (8 byte):      " << Isis::LOW_REPR_SAT8 << endl;
  cout << "Low Instrument (8 byte):          " << Isis::LOW_INSTR_SAT8 << endl;
  cout << "High Representation (8 byte):     " << Isis::HIGH_REPR_SAT8 << endl;
  cout << "High Instrument (8 byte):         " << Isis::HIGH_INSTR_SAT8 << endl;
  cout << "Valid maximum (8 byte):           " << Isis::VALID_MAX8 << endl;
  cout << endl;



  double d = 0.0;
  cout << "Testing 0.0 ... " << endl;
  cout << "IsSpecial:     " << Isis::IsSpecial(d) << endl;
  cout << "IsValidPixel:  " << Isis::IsValidPixel(d) << endl;
  cout << "IsNullPixel:   " << Isis::IsNullPixel(d) << endl;
  cout << "IsLowPixel:    " << Isis::IsLowPixel(d) << endl;
  cout << "IsHighPixel:   " << Isis::IsHighPixel(d) << endl;
  cout << "IsHrsPixel:    " << Isis::IsHrsPixel(d) << endl;
  cout << "IsHisPixel:    " << Isis::IsHisPixel(d) << endl;
  cout << "IsLrsPixel:    " << Isis::IsLrsPixel(d) << endl;
  cout << "IsLisPixel:    " << Isis::IsLisPixel(d) << endl;
  cout << endl;

  d = Isis::Null;
  cout << "Testing Isis::Null ... " << endl;
  cout << "IsSpecial:     " << Isis::IsSpecial(d) << endl;
  cout << "IsValidPixel:  " << Isis::IsValidPixel(d) << endl;
  cout << "IsNullPixel:   " << Isis::IsNullPixel(d) << endl;
  cout << "IsLowPixel:    " << Isis::IsLowPixel(d) << endl;
  cout << "IsHighPixel:   " << Isis::IsHighPixel(d) << endl;
  cout << "IsHrsPixel:    " << Isis::IsHrsPixel(d) << endl;
  cout << "IsHisPixel:    " << Isis::IsHisPixel(d) << endl;
  cout << "IsLrsPixel:    " << Isis::IsLrsPixel(d) << endl;
  cout << "IsLisPixel:    " << Isis::IsLisPixel(d) << endl;
  cout << endl;

  d = Isis::Lis;
  cout << "Testing Isis::Lis ... " << endl;
  cout << "IsSpecial:     " << Isis::IsSpecial(d) << endl;
  cout << "IsValidPixel:  " << Isis::IsValidPixel(d) << endl;
  cout << "IsNullPixel:   " << Isis::IsNullPixel(d) << endl;
  cout << "IsLowPixel:    " << Isis::IsLowPixel(d) << endl;
  cout << "IsHighPixel:   " << Isis::IsHighPixel(d) << endl;
  cout << "IsHrsPixel:    " << Isis::IsHrsPixel(d) << endl;
  cout << "IsHisPixel:    " << Isis::IsHisPixel(d) << endl;
  cout << "IsLrsPixel:    " << Isis::IsLrsPixel(d) << endl;
  cout << "IsLisPixel:    " << Isis::IsLisPixel(d) << endl;
  cout << endl;

  d = Isis::Lrs;
  cout << "Testing Isis::Lrs ... " << endl;
  cout << "IsSpecial:     " << Isis::IsSpecial(d) << endl;
  cout << "IsValidPixel:  " << Isis::IsValidPixel(d) << endl;
  cout << "IsNullPixel:   " << Isis::IsNullPixel(d) << endl;
  cout << "IsLowPixel:    " << Isis::IsLowPixel(d) << endl;
  cout << "IsHighPixel:   " << Isis::IsHighPixel(d) << endl;
  cout << "IsHrsPixel:    " << Isis::IsHrsPixel(d) << endl;
  cout << "IsHisPixel:    " << Isis::IsHisPixel(d) << endl;
  cout << "IsLrsPixel:    " << Isis::IsLrsPixel(d) << endl;
  cout << "IsLisPixel:    " << Isis::IsLisPixel(d) << endl;
  cout << endl;

  d = Isis::His;
  cout << "Testing Isis::His ... " << endl;
  cout << "IsSpecial:     " << Isis::IsSpecial(d) << endl;
  cout << "IsValidPixel:  " << Isis::IsValidPixel(d) << endl;
  cout << "IsNullPixel:   " << Isis::IsNullPixel(d) << endl;
  cout << "IsLowPixel:    " << Isis::IsLowPixel(d) << endl;
  cout << "IsHighPixel:   " << Isis::IsHighPixel(d) << endl;
  cout << "IsHrsPixel:    " << Isis::IsHrsPixel(d) << endl;
  cout << "IsHisPixel:    " << Isis::IsHisPixel(d) << endl;
  cout << "IsLrsPixel:    " << Isis::IsLrsPixel(d) << endl;
  cout << "IsLisPixel:    " << Isis::IsLisPixel(d) << endl;
  cout << endl;

  d = Isis::Hrs;
  cout << "Testing Isis::Hrs ... " << endl;
  cout << "IsSpecial:     " << Isis::IsSpecial(d) << endl;
  cout << "IsValidPixel:  " << Isis::IsValidPixel(d) << endl;
  cout << "IsNullPixel:   " << Isis::IsNullPixel(d) << endl;
  cout << "IsLowPixel:    " << Isis::IsLowPixel(d) << endl;
  cout << "IsHighPixel:   " << Isis::IsHighPixel(d) << endl;
  cout << "IsHrsPixel:    " << Isis::IsHrsPixel(d) << endl;
  cout << "IsHisPixel:    " << Isis::IsHisPixel(d) << endl;
  cout << "IsLrsPixel:    " << Isis::IsLrsPixel(d) << endl;
  cout << "IsLisPixel:    " << Isis::IsLisPixel(d) << endl;
  cout << endl;

#if 0
  cout << Isis::IsEqual(Isis::NULL4, Isis::NULL4) << endl;
  cout << Isis::IsEqual(Isis::NULL4, Isis::LOW_INSTR_SAT4) << endl;
  cout << Isis::IsEqual(Isis::NULL4, Isis::LOW_REPR_SAT4) << endl;
  cout << Isis::IsEqual(Isis::NULL4, Isis::HIGH_INSTR_SAT4) << endl;
  cout << Isis::IsEqual(Isis::NULL4, Isis::HIGH_REPR_SAT4) << endl;
  cout << endl;

  cout << Isis::IsEqual(Isis::LOW_INSTR_SAT4, Isis::NULL4) << endl;
  cout << Isis::IsEqual(Isis::LOW_INSTR_SAT4, Isis::LOW_INSTR_SAT4) << endl;
  cout << Isis::IsEqual(Isis::LOW_INSTR_SAT4, Isis::LOW_REPR_SAT4) << endl;
  cout << Isis::IsEqual(Isis::LOW_INSTR_SAT4, Isis::HIGH_INSTR_SAT4) << endl;
  cout << Isis::IsEqual(Isis::LOW_INSTR_SAT4, Isis::HIGH_REPR_SAT4) << endl;
  cout << endl;

  cout << Isis::IsEqual(Isis::LOW_REPR_SAT4, Isis::NULL4) << endl;
  cout << Isis::IsEqual(Isis::LOW_REPR_SAT4, Isis::LOW_INSTR_SAT4) << endl;
  cout << Isis::IsEqual(Isis::LOW_REPR_SAT4, Isis::LOW_REPR_SAT4) << endl;
  cout << Isis::IsEqual(Isis::LOW_REPR_SAT4, Isis::HIGH_INSTR_SAT4) << endl;
  cout << Isis::IsEqual(Isis::LOW_REPR_SAT4, Isis::HIGH_REPR_SAT4) << endl;
  cout << endl;

  cout << Isis::IsEqual(Isis::HIGH_INSTR_SAT4, Isis::NULL4) << endl;
  cout << Isis::IsEqual(Isis::HIGH_INSTR_SAT4, Isis::LOW_INSTR_SAT4) << endl;
  cout << Isis::IsEqual(Isis::HIGH_INSTR_SAT4, Isis::LOW_REPR_SAT4) << endl;
  cout << Isis::IsEqual(Isis::HIGH_INSTR_SAT4, Isis::HIGH_INSTR_SAT4) << endl;
  cout << Isis::IsEqual(Isis::HIGH_INSTR_SAT4, Isis::HIGH_REPR_SAT4) << endl;
  cout << endl;

  cout << Isis::IsEqual(Isis::HIGH_REPR_SAT4, Isis::NULL4) << endl;
  cout << Isis::IsEqual(Isis::HIGH_REPR_SAT4, Isis::LOW_INSTR_SAT4) << endl;
  cout << Isis::IsEqual(Isis::HIGH_REPR_SAT4, Isis::LOW_REPR_SAT4) << endl;
  cout << Isis::IsEqual(Isis::HIGH_REPR_SAT4, Isis::HIGH_INSTR_SAT4) << endl;
  cout << Isis::IsEqual(Isis::HIGH_REPR_SAT4, Isis::HIGH_REPR_SAT4) << endl;
  cout << endl;
#endif

  cout << "Testing PixelToString(double):" << endl;
  d = Isis::Null;
  cout << "Isis::Null to String:\t" << Isis::PixelToString(d) << endl;
  d = Isis::Lrs;
  cout << "Isis::Lrs to String:\t" << Isis::PixelToString(d) << endl;
  d = Isis::Hrs;
  cout << "Isis::Hrs to String:\t" << Isis::PixelToString(d) << endl;
  d = Isis::His;
  cout << "Isis::His to String:\t" << Isis::PixelToString(d) << endl;
  d = Isis::Lis;
  cout << "Isis::Lis to String:\t" << Isis::PixelToString(d) << endl;
  d = 0.0;
  cout << "0.0 to String:\t\t" << Isis::PixelToString(d) << endl;
  d = 1.5;
  cout << "1.5 to String:\t\t" << Isis::PixelToString(d) << endl;
  d = -6.0;
  cout << "-6.0 to String:\t\t" << Isis::PixelToString(d) << endl;
  cout << endl;

  cout << "Testing StringToPixel(QString):" << endl;
  QString s = "NuLl";
  cout << "NuLl to Pixel:\t\t" << Isis::StringToPixel(s) << endl;
  s = "lrs";
  cout << "lrs to Pixel:\t\t" << Isis::StringToPixel(s) << endl;
  s = "Hrs";
  cout << "Hrs to Pixel:\t\t" << Isis::StringToPixel(s) << endl;
  s = "HIs";
  cout << "HIs to Pixel:\t\t" << Isis::StringToPixel(s) << endl;
  s = "LIS";
  cout << "LIS to Pixel:\t\t" << Isis::StringToPixel(s) << endl;
  s = "0.0";
  cout << "0.0 to Pixel:\t\t" << Isis::StringToPixel(s) << endl;
  s = "1.5";
  cout << "1.5 to Pixel:\t\t" << Isis::StringToPixel(s) << endl;
  s = "-6.0";
  cout << "-6.0 to Pixel:\t\t" << Isis::StringToPixel(s) << endl;
  cout << Isis::PixelToString(d) << " to Pixel:\t" << Isis::StringToPixel(Isis::PixelToString(d)) << endl;
  cout << endl;

  cout << "Testing unique value shortcut for StringToPixel(QString):" << endl;
  s = "Nul";
  cout << "Nul to Pixel:\t\t" << Isis::StringToPixel(s) << endl;
  s = "N";
  cout << "N to Pixel:\t\t" << Isis::StringToPixel(s) << endl;
  s = "Lr";
  cout << "Lr to Pixel:\t\t" << Isis::StringToPixel(s) << endl;
  s = "Hr";
  cout << "Hr to Pixel:\t\t" << Isis::StringToPixel(s) << endl;
  s = "Hi";
  cout << "Hi to Pixel:\t\t" << Isis::StringToPixel(s) << endl;
  s = "Li";
  cout << "Li to Pixel:\t\t" << Isis::StringToPixel(s) << endl;
  cout << endl;

  cout << "Testing illegal shortcuts/expressions for StringToPixel(QString):" << endl;
  try {
    s = "Nl";
    Isis::StringToPixel(s);
  }
  catch(Isis::IException &e) {
    e.print();
  }
  try {
    s = "Nulll";
    Isis::StringToPixel(s);
  }
  catch(Isis::IException &e) {
    e.print();
  }
  try {
    s = "L";
    Isis::StringToPixel(s);
  }
  catch(Isis::IException &e) {
    e.print();
  }
  try {
    s = "Ls";
    Isis::StringToPixel(s);
  }
  catch(Isis::IException &e) {
    e.print();
  }
  try {
    s = "H";
    Isis::StringToPixel(s);
  }
  catch(Isis::IException &e) {
    e.print();
  }
  try {
    s = "Hs";
    Isis::StringToPixel(s);
  }
  catch(Isis::IException &e) {
    e.print();
  }
  try {
    s = "Hrsr";
    Isis::StringToPixel(s);
  }
  catch(Isis::IException &e) {
    e.print();
  }
  try {
    s = "nonsense";
    Isis::StringToPixel(s);
  }
  catch(Isis::IException &e) {
    e.print();
  }
  cout << endl;










}

