#include "IString.h"

#include <cmath>
#include <float.h>
#include <iostream>
#include <sstream>

#include "IException.h"
#include "Preference.h"

using namespace Isis;
using namespace std;

inline string yesOrNo(bool b) {
  if(b) return (string("Yes"));
  return (string("No"));
}

int main(int argc, char *argv[]) {
  Preference::Preferences(true);
  cerr.precision(14);

  // Tests for the functions
  cerr << "Testing toBool" << endl;
  cerr << "\tTrue:  " << toBool("True") << endl;
  cerr << "\tFalse: " << toBool("False") << endl;

  try {
    toBool("...");
  }
  catch (IException &e) {
    cerr << "\t" << e.toString() << endl;
  }

  cerr << endl;
  cerr << "Testing toInt" << endl;
  cerr << "\t" << toInt("0") << endl;
  cerr << "\t" << toInt("5") << endl;
  cerr << "\t" << toInt("-5") << endl;
  cerr << "\t" << toInt("2147483647") << endl;
  cerr << "\t" << toInt("-2147483647") << endl;

  try {
    toInt("-5.0");
  }
  catch (IException &e) {
    cerr << "\t" << e.toString() << endl;
  }

  try {
    toInt("2147483648");
  }
  catch (IException &e) {
    cerr << "\t" << e.toString() << endl;
  }
  cerr << endl;

  cerr << "Testing toBigInt" << endl;
  cerr << "\t" << toBigInt("0") << endl;
  cerr << "\t" << toBigInt("5") << endl;
  cerr << "\t" << toBigInt("-5") << endl;
  cerr << "\t" << toBigInt("9223372036854775807") << endl;
  cerr << "\t" << toBigInt("-9223372036854775807") << endl;

  try {
    toBigInt("-5.0");
  }
  catch (IException &e) {
    cerr << "\t" << e.toString() << endl;
  }

  try {
    toBigInt("9223372036854775808");
  }
  catch (IException &e) {
    cerr << "\t" << e.toString() << endl;
  }
  cerr << endl;

  cerr << "Testing toDouble" << endl;
  cerr << "\t" << toDouble("0.0") << endl;
  cerr << "\t" << toDouble("5.25") << endl;
  cerr << "\t" << toDouble("-5.25") << endl;
  cerr << "\t" << toDouble("16#FF7FFFFB#") << endl;
  cerr << "\t" << toDouble("1e100") << endl;
  cerr << "\t" << toDouble("-1E100") << endl;
  cerr << "\t" << toDouble("1.79769313486232e+308") << endl;

  try {
    toDouble("fred");
  }
  catch (IException &e) {
    cerr << "\t" << e.toString() << endl;
  }
  cerr << endl;

  cerr << "Testing toString" << endl;
  cerr << "\tbool(false):        " << toString(false) << endl;
  cerr << "\tbool(true):         " << toString(true) << endl;
  cerr << "\tchar(A):            " << toString('A') << endl;
  cerr << "\tint(2147483647):    " << toString(2147483647) << endl;
  cerr << "\tuint(2147483648):   " << toString((unsigned int)2147483648) << endl;
  cerr << "\tBigInt(2147483648): " << toString(2147483648) << endl;
  cerr << "\tdouble(0.0):        " << toString(0.0) << endl;
  cerr << "\tdouble(DBL_MIN):    " << toString(DBL_MIN) << endl;
  cerr << "\tdouble(DBL_MAX*2):  " << toString(DBL_MAX * 2) << endl;
  cerr << "\tdouble(-DBL_MAX*2): " << toString(-DBL_MAX * 2) << endl;
  cerr << "\tdouble(sqrt(-1)):   " << toString(sqrt(-1)) << endl;
  cerr << "\tdouble(5.25):       " << toString(5.25) << endl;
  cerr << "\tdouble(-5.25):      " << toString(-5.25) << endl;
  cerr << "\tdouble(1e13):       " << toString(1e13) << endl;
  cerr << "\tdouble(1e13+1):     " << toString(1e13 + 1) << endl;
  cerr << "\tdouble(1e-3):       " << toString(1e-3) << endl;
  cerr << "\tdouble(9e-4):       " << toString(9e-4) << endl;
  cerr << "\tdouble(1e100):      " << toString(1e100) << endl;
  cerr << "\tdouble(-1e100):     " << toString(-1e100) << endl;

  cerr << endl;
  cerr << "Testing cerr with QString and with QStringRef" << endl;
  cerr << QString("\tQString\n");
  cerr << QString("\tQStringRef - FAIL!\n").leftRef(11) << endl;

  cerr << endl;
  cerr << "----------------------------------" << endl;
  cerr << "-- Now testing deprecated class --" << endl;
  cerr << "----------------------------------" << endl;
  cerr << endl;

  // Tests for non-static functions
  try {
    IString str0;
    str0 = "Test string";
    cerr << "No arg construtor : " << str0 << endl;

    IString str1 = str0 + " again";
    cerr << "Constructor IString: " << str1 << endl;

    IString str1_5 = 'A';
    cerr << "Constructor char : " << str1_5 << endl;

    IString str2 = 999;
    cerr << "Constructor int : " << str2 << endl;

    IString str2a = BigInt(9999999999LL);
    cerr << "Constructor BigInt : " << str2a << endl;

    IString str3 = 999.999;
    cerr << "Constructor double :" << str3 << endl;

    IString str4 = "ABCDefghijkBCAD";
    cerr << "Before Trim : " << str4 << endl;
    cerr << "Return Trim : " << str4.Trim("ABCD") << endl;
    cerr << "After Trim  : " << str4 << endl;

    IString str5 = "ABCDefghijkBCAD";
    cerr << "Before TrimHead : " << str5 << endl;
    cerr << "Return TrimHead : " << str5.TrimHead("DBCA") << endl;
    cerr << "After TrimHead  : " << str5 << endl;
    cerr << "Middle test     : " << str5.TrimHead("g") << endl;

    IString str6 = "ABCDefghijkBCAD";
    cerr << "Before TrimTail : " << str6 << endl;
    cerr << "Return TrimTail : " << str6.TrimTail("DBCA") << endl;
    cerr << "After TrimTail  : " << str6 << endl;
    cerr << "Middle test     : " << str6.TrimTail("f") << endl;

    std::string str35 = "ABCDefghijkBCAD";
    cerr << "Before TrimHead : " << str35 << endl;
    str35 = IString::TrimHead("DBCA", str35);
    cerr << "After TrimHead  : " << str35 << endl;
    str35 = IString::TrimHead("g", str35);
    cerr << "Middle test     : " << str35 << endl;

    std::string str36 = "ABCDefghijkBCAD";
    cerr << "Before TrimTail : " << str36 << endl;
    str36 = IString::TrimTail("DBCA", str36);
    cerr << "After TrimTail  : " << str36 << endl;
    str36 = IString::TrimTail("f", str36);
    cerr << "Middle test     : " << str36 << endl;

    IString str7 = "abcdefghijklmnopqrstuvwxyzABC!@#$%^&*()";
    cerr << "Before Upcase : " << str7 << endl;
    cerr << "After Upcase  : " << str7.UpCase() << endl;

    cerr << "Before DownCase : " << str7 << endl;
    cerr << "After DownCase  : " << str7.DownCase() << endl;

    IString str8 = 987654321;
    cerr << "Integer : " << str8 << endl;
    cerr << "Integer : " << str8.ToInteger() << endl;
    str8 = "987trew";
    try {
      str8.ToInteger();
    }
    catch(IException &error) {
      error.print();
    }

    IString str8a = BigInt(9876543210LL);
    cerr << "BigInteger : " << str8a << endl;
    cerr << "BigInteger : " << str8a.ToBigInteger() << endl;
    str8a = "987trew";
    try {
      str8a.ToBigInteger();
    }
    catch(IException &error) {
      error.print();
    }

    IString str9 = 9876.54321;
    cerr << "Double : " << str9.c_str() << endl;
    cerr << "Double : " << str9.ToDouble() << endl;
    str9 = "123$987";
    try {
      str9.ToDouble();
    }
    catch(IException &error) {
      error.print();
    }

    IString str10 = "123.0E45";
    cerr << "Exponent : " << str10 << endl;
    cerr << "Exponent : " << str10.ToDouble() << endl;

    IString str11 = "25:255 35:15";
    cerr << str11 << endl;
    while(str11.length() > 0) {
      cerr << str11.Token(": ") << endl;
    }

    str11 = "key1=tok1 key2=\"t o k 2\" key3=(1,2,3,4)";
    cerr << str11 << endl;
    while(str11.length() > 0) {
      cerr << str11.Token("= ") << endl;
    }

    str11 = "\"abcd\",\"1234\"";
    cerr << str11 << endl;
    while(str11.length() > 0) {
      cerr << str11.Token(",") << endl;
    }

    str11 = "\",1234\",\"ab,cd\"";
    cerr << str11 << endl;
    while(str11.length() > 0) {
      cerr << str11.Token(",") << endl;
    }

    str11 = "/this/is/a/long/filename.jnk,seperated/by/a/comma/ending/with/\r\n";
    cerr << str11.Trim("\r\n") << endl;
    while(str11.length() > 0) {
      cerr << str11.Token(",") << endl;
    }

    str11 = "/this/is/another/long/filename.jnk   seperated/by/3/spaces/file2.tmp";
    cerr << str11.Trim("\r\n") << endl;
    int m = 1;
    while(str11.length() > 0) {
      cerr << "token# " << m++ << ">> " << str11.Token(" ") << endl;
    }

    cerr << endl;

    IString str12 = "  \"    \"    ";
    cerr << "Before compress >" << str12 << "<" << endl;
    str12.Compress();
    cerr << "After compress  >" << str12 << "<" << endl;

    IString str12b = "|  \"    \"    \'    \'   |";
    cerr << "Before compress >" << str12b << "<" << endl;
    str12b.Compress();
    cerr << "After compress  >" << str12b << "<" << endl;

    IString str13 = "  AB  CD  ";
    cerr << "Before compress >" << str13 << "<" << endl;
    str13.Compress();
    cerr << "After compress  >" << str13 << "<" << endl;

    IString str14 = "  \"  \"    ";
    cerr << "Before force compress >" << str14 << "<" << endl;
    cerr << "After force compress  >" << str14.Compress(true) << "<" << endl;

    IString str15 = "  \"AB  CD\"  ";
    cerr << "Before force compress >" << str15 << "<" << endl;
    cerr << "After force compress  >" << str15.Compress(true) << "<" << endl;

    IString str16 = " ";
    cerr << "Before force compress >" << str16 << "<" << endl;
    cerr << "After force compress  >" << str16.Compress(true) << "<" << endl;

    IString str17 = "ABCDEFG";
    cerr << "Before convert >" << str17 << "<" << endl;
    cerr << "After convert  >" << str17.Convert("BDFG", '-') << "<" << endl;

    IString str18 = "Thirteen is bigger than fourteen";
    cerr << "Before replace >" << str18 << "<" << endl;
    cerr << "After replace  >" << str18.Replace("bigger", "smaller") << "<" << endl;

    IString str19 = "a\tb\b\n\v\f\r  c";
    // Don't cerr the before string, it will cause all sorts of problems
    //    cerr << "Before convert >" << str19 << "<" << endl;
    cerr << "After convert white space >" << str19.ConvertWhiteSpace() << "<" << endl;

    IString str20 = "xxxXXX0";
    cerr << "Before Upcase >" << str20 << "<" << endl;
    cerr << "After Upcase  >" << str20.UpCase() << "<" << endl;

    cerr << "Before DownCase >" << str20 << "<" << endl;
    cerr << "After DownCase  >" << str20.DownCase() << "<" << endl;

    IString str21 = "Test String";
    QString qstr1 = "Test String";
    cerr << "Testing QT conversion: " << (str21.ToQt() == qstr1) << endl;

    IString str22(255.0);
    IString str23(0.333);
    IString str24(-255.0);
    IString str25(1.235E-20);
    IString str26(12345678901234567890.0);
    IString str27(0.0005);
    cerr << str22 << endl;
    cerr << str23 << endl;
    cerr << str24 << endl;
    cerr << str25 << endl;
    cerr << str26 << endl;
    cerr << str27 << endl;

    IString str28("a 1 b 2 c 3 d 4 e 5");
    cerr << "Before Remove >" << str28 << "<" << endl;
    cerr << "After Remove >" << str28.Remove("1245") << "<" << endl;

    IString str29("100");
    int i = str29;
    cerr << i << endl;

    IString str29a("100000000000");
    BigInt j = str29a;
    cerr << j << endl;

    IString str30("100.1");
    double d = str30;
    cerr << d << endl;

    IString str31("I is a test string: 'I is' is \"is a string\"");
    IString str31a("I is a test string: 'I is' is \"is a string\"");
    cerr << "Before Replace (honor quotes) >" << str31 <<  "<" << endl;
    str31.Replace("is", "am", true);
    cerr << "After Replace (honor quotes) >" << str31 <<  "<" << endl;
    cerr << "Before Replace (dont honor quotes) >" << str31a <<  "<" << endl;
    str31a.Replace("is", "am", false);
    cerr << "After Replace (dont honor quotes) >" << str31a <<  "<" << endl;

  }
  catch(IException &error) {
    error.print();
  }

  //Tests for static functions untested above
  try {
    //Split test
    string str1 = "This is a test string";
    vector<string> strVec;
    IString::Split(' ', str1, strVec);
    cerr << "Before split >" << str1 << "<" << endl;
    cerr << "After split >";
    for(unsigned int i = 0 ; i < strVec.size() ; i++) {
      cerr << endl << "Element " << i << ": " << strVec[i];
    }
    cerr << endl << "<" << endl;

    QStringList qlist;
    qlist << "String1"
          << "String2"
          << "String3"
          << "String4";

    cerr << "\nQt QStringList values:\n";
    for(int i = 0 ; i < qlist.size() ; i++) {
      cerr << qlist.at(i).toLocal8Bit().constData() << endl;
    }

    cerr << "\nConverted to std::vector<std::string>...\n";
    vector<string> slist = IString::ToStd(qlist);
    copy(slist.begin(), slist.end(), ostream_iterator<string>(cerr, "\n"));

    cerr << "Are they equivalent?\n";
    cerr << "Counts? " << yesOrNo((int) slist.size() == qlist.size()) << endl;
    for(int j = 0 ; j < qlist.size() ; j++) {
      QString qs = qlist.at(j);
      string  ss = slist.at(j);
      cerr << "string(" << ss << ") == QString("
           << qs.toLocal8Bit().constData() << ")? "
           << yesOrNo(ss == qs.toStdString()) << endl;
    }

  }
  catch(IException &error) {
    error.print();
  }
}
