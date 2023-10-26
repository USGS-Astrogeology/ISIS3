/** This is free and unencumbered software released into the public domain.
The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */
#include <iostream>

#include "IException.h"
#include "Preference.h"
#include "PvlGroup.h"
#include "PvlKeyword.h"
#include "TableField.h"

using namespace std;
using namespace Isis;

int main(int argc, char *argv[]) {
  Preference::Preferences(true);

  cout << "Testing integer singleton" << endl;
  TableField intSingletonField("Test", TableField::Integer);
  cout << "Name      = " << intSingletonField.name() << endl;
  cout << "Type      = " << intSingletonField.type() << endl;
  cout << "IsInteger = " << intSingletonField.isInteger() << endl;
  cout << "IsDouble  = " << intSingletonField.isDouble() << endl;
  cout << "IsText    = " << intSingletonField.isText() << endl;
  cout << "IsReal    = " << intSingletonField.isReal() << endl;
  cout << "Size      = " << intSingletonField.size() << endl;
  cout << "Bytes     = " << intSingletonField.bytes() << endl;
  intSingletonField = 15;
  cout << "Value     = " << (int)intSingletonField << endl;
  PvlGroup g = intSingletonField.pvlGroup();
  cout << g << endl;
  cout << "----------------------------------------" << endl << endl;
  cout << "Testing double singleton" << endl;
  TableField dblSingletonField("Test", TableField::Double);
  cout << "Name      = " << dblSingletonField.name() << endl;
  cout << "Type      = " << dblSingletonField.type() << endl;
  cout << "IsInteger = " << dblSingletonField.isInteger() << endl;
  cout << "IsDouble  = " << dblSingletonField.isDouble() << endl;
  cout << "IsText    = " << dblSingletonField.isText() << endl;
  cout << "IsReal    = " << dblSingletonField.isReal() << endl;
  cout << "Size      = " << dblSingletonField.size() << endl;
  cout << "Bytes     = " << dblSingletonField.bytes() << endl;
  dblSingletonField = -3.14;
  cout << "Value     = " << (double)dblSingletonField << endl;
  g = dblSingletonField.pvlGroup();
  cout << g << endl;
  cout << "----------------------------------------" << endl << endl;
  cout << "Testing text singleton" << endl;
  TableField textField("Test", TableField::Text, 20);
  cout << "Name      = " << textField.name() << endl;
  cout << "Type      = " << textField.type() << endl;
  cout << "IsInteger = " << textField.isInteger() << endl;
  cout << "IsDouble  = " << textField.isDouble() << endl;
  cout << "IsText    = " << textField.isText() << endl;
  cout << "IsReal    = " << textField.isReal() << endl;
  cout << "Size      = " << textField.size() << endl;
  cout << "Bytes     = " << textField.bytes() << endl;
  textField = (std::string) "Bah humbug";
  cout << "Value     = " << (std::string)textField << endl;
  g = textField.pvlGroup();
  cout << g << endl;
  cout << "----------------------------------------" << endl << endl;
  cout << "Testing real singleton" << endl;
  TableField realSingletonField("Test", TableField::Real);
  cout << "Name      = " << realSingletonField.name() << endl;
  cout << "Type      = " << realSingletonField.type() << endl;
  cout << "IsInteger = " << realSingletonField.isInteger() << endl;
  cout << "IsDouble  = " << realSingletonField.isDouble() << endl;
  cout << "IsText    = " << realSingletonField.isText() << endl;
  cout << "IsReal    = " << realSingletonField.isReal() << endl;
  cout << "Size      = " << realSingletonField.size() << endl;
  cout << "Bytes     = " << realSingletonField.bytes() << endl;
  realSingletonField = (float)15.542;
  cout << "Value     = " << (float)realSingletonField << endl;
  g = realSingletonField.pvlGroup();
  cout << g << endl;
  cout << "----------------------------------------" << endl << endl;
  cout << "Testing integer array" << endl;
  TableField intVectorField3("Test", TableField::Integer, 3);
  cout << "Name      = " << intVectorField3.name() << endl;
  cout << "Type      = " << intVectorField3.type() << endl;
  cout << "IsInteger = " << intVectorField3.isInteger() << endl;
  cout << "IsDouble  = " << intVectorField3.isDouble() << endl;
  cout << "IsText    = " << intVectorField3.isText() << endl;
  cout << "IsReal    = " << intVectorField3.isReal() << endl;
  cout << "Size      = " << intVectorField3.size() << endl;
  cout << "Bytes     = " << intVectorField3.bytes() << endl;
  vector<int> temp;
  temp.push_back(3);
  temp.push_back(2);
  temp.push_back(1);
  intVectorField3 = temp;
  temp.clear();
  temp = intVectorField3;
  cout << "Value     = " << temp[0] << " " << temp[1] << " " << temp[2] << endl;
  g = intVectorField3.pvlGroup();
  cout << g << endl;
  cout << "----------------------------------------" << endl << endl;
  cout << "Testing double array" << endl;
  TableField dblVectorField3("Test", TableField::Double, 3);
  cout << "Name      = " << dblVectorField3.name() << endl;
  cout << "Type      = " << dblVectorField3.type() << endl;
  cout << "IsInteger = " << dblVectorField3.isInteger() << endl;
  cout << "IsDouble  = " << dblVectorField3.isDouble() << endl;
  cout << "IsText    = " << dblVectorField3.isText() << endl;
  cout << "IsReal    = " << dblVectorField3.isReal() << endl;
  cout << "Size      = " << dblVectorField3.size() << endl;
  cout << "Bytes     = " << dblVectorField3.bytes() << endl;
  vector<double> tmp;
  tmp.push_back(1.3);
  tmp.push_back(2.4);
  tmp.push_back(-9.2);
  dblVectorField3 = tmp;
  tmp.clear();
  tmp = dblVectorField3;
  cout << "Value     = " << tmp[0] << " " << tmp[1] << " " << tmp[2] << endl;
  g = dblVectorField3.pvlGroup();
  cout << g << endl;
  cout << "----------------------------------------" << endl << endl;
  cout << "Testing real array" << endl;
  TableField realVectorField3("Test", TableField::Real, 3);
  cout << "Name      = " << realVectorField3.name() << endl;
  cout << "Type      = " << realVectorField3.type() << endl;
  cout << "IsInteger = " << realVectorField3.isInteger() << endl;
  cout << "IsDouble  = " << realVectorField3.isDouble() << endl;
  cout << "IsText    = " << realVectorField3.isText() << endl;
  cout << "IsReal    = " << realVectorField3.isReal() << endl;
  cout << "Size      = " << realVectorField3.size() << endl;
  cout << "Bytes     = " << realVectorField3.bytes() << endl;
  vector<float> tmp2;
  tmp2.push_back(1.3);
  tmp2.push_back(2.4);
  tmp2.push_back(-9.2);
  realVectorField3 = tmp2;
  tmp2.clear();
  tmp2 = realVectorField3;
  cout << "Value     = " << tmp2[0] << " " << tmp2[1] << " " << tmp2[2] << endl;
  g = realVectorField3.pvlGroup();
  cout << g << endl;
  cout << "----------------------------------------" << endl << endl;

  cout << "Testing integer constructor" << endl ;
  PvlGroup group1("Field");

  group1 += PvlKeyword("name", "Test");
  group1 += PvlKeyword("type", "Integer");
  group1 += PvlKeyword("size", std::to_string(20));

  TableField intVectorField20(group1);
  g = intVectorField20.pvlGroup();
  cout << g << endl;
  cout << "----------------------------------------" << endl << endl;

  cout << "Testing double constructor" << endl ;
  PvlGroup group2("Field");

  group2 += PvlKeyword("name", "Test");
  group2 += PvlKeyword("type", "Double");
  group2 += PvlKeyword("size", std::to_string(20));

  TableField dblVectorField20(group2);
  g = dblVectorField20.pvlGroup();
  cout << g << endl;
  cout << "----------------------------------------" << endl << endl;

  cout << "Testing text constructor" << endl ;
  PvlGroup group3("Field");

  group3 += PvlKeyword("name", "Test");
  group3 += PvlKeyword("type", "Text");
  group3 += PvlKeyword("size", std::to_string(20));

  TableField textField20(group3);
  g = textField20.pvlGroup();
  cout << g << endl;
  cout << "----------------------------------------" << endl << endl;

  cout << "Testing real constructor" << endl ;
  PvlGroup group4("Field");

  group4 += PvlKeyword("name", "Test");
  group4 += PvlKeyword("type", "Real");
  group4 += PvlKeyword("size", std::to_string(20));

  TableField realVectorField20(group4);
  g = realVectorField20.pvlGroup();
  cout << g << endl;
  cout << "----------------------------------------" << endl << endl;

  cout << "Testing erroneous type constructor" << endl ;
  try {
    PvlGroup group5("Field");

    group5 += PvlKeyword("name", "Test");
    group5 += PvlKeyword("type", "BLAH");
    group5 += PvlKeyword("size", std::to_string(20));

    TableField unknownTypeField(group5);
    g = unknownTypeField.pvlGroup();
    cout << g << endl;
  }
  catch(IException &e) {
    e.print();
  }
  cout << "----------------------------------------" << endl << endl;

  cout << "Testing erroneous size constructor" << endl ;
  try {
    PvlGroup group6("Field");

    group6 += PvlKeyword("name", "Test");
    group6 += PvlKeyword("type", "Integer");
    group6 += PvlKeyword("size", std::to_string(-7.3));

    TableField invalidSizeField(group6);
    g = invalidSizeField.pvlGroup();
    cout << g << endl;

  }
  catch(IException &e) {
    e.print();
  }

  cout << "----------------------------------------" << endl << endl;
  cout << "Test casting errors..." << endl;
  cout << "     operator int()" << endl;
  try {
    cout << int(dblSingletonField) << endl; // try to cast a non-Integer type
  }
  catch (IException &e) {
    e.print();
  }
  try {
    cout << int(intVectorField3); // try to cast an Integer type with multiple values
  }
  catch (IException &e) {
    e.print();
  }

  cout << endl;
  cout << "     operator double()" << endl;
  try {
    cout << double(textField); // try to cast a non-Double type
  }
  catch (IException &e) {
    e.print();
  }
  try {
    cout << double(dblVectorField3); // try to cast a Double type with multiple values
  }
  catch (IException &e) {
    e.print();
  }

  cout << endl;
  cout << "     operator std::string() " << endl;
  try {
    cout << std::string(realSingletonField); // try to cast a non-Text type
  }
  catch (IException &e) {
    e.print();
  }
  
  cout << endl;
  cout << "     operator float()" << endl;
  try {
    cout << float(intSingletonField); // try to cast a non-Real type
  }
  catch (IException &e) {
    e.print();
  }
  try {
    cout << float(realVectorField3); // try to cast a Real type with multiple values
  }
  catch (IException &e) {
    e.print();
  }

  cout << endl;
  cout << "     operator std::vector<int>() " << endl;
  try {
    vector<int> error = dblSingletonField; // try to cast a non Integer type
  }
  catch (IException &e) {
    e.print();
  }

  cout << endl;
  cout << "     operator std::vector<double>() " << endl;
  try {
    vector<double> error = realSingletonField; // try to cast a non Double type
  }
  catch (IException &e) {
    e.print();
  }

  cout << endl;
  cout << "     operator std::vector<float>() " << endl;
  try {
    vector<float> error = intVectorField3; // try to cast a non Real type
  }
  catch (IException &e) {
    e.print();
  }

  cout << "----------------------------------------" << endl << endl;
  cout << "Test setting errors..." << endl ;
  cout << "     operator=(const int value)" << endl;
  try {
    realSingletonField = (int) 1; // try to set Real type to non-float value
  }
  catch (IException &e) {
    e.print();
  }
  try {
    intVectorField3 = (int) 1; // try to set single value to vector Field
  }
  catch (IException &e) {
    e.print();
  }

  cout << endl;
  cout << "     operator=(const double value)" << endl;
  try {
    intSingletonField = (double) 3.14; // try to set Integer type to non-integer value
  }
  catch (IException &e) {
    e.print();
  }
  try {
    dblVectorField3 = (double) 3.14; // try to set single value to vector Field
  }
  catch (IException &e) {
    e.print();
  }

  cout << endl;
  cout << "     operator=(const std::string &value)" << endl;
  try {
    dblSingletonField = "Error"; // try to set Double type to non-double value
  }
  catch (IException &e) {
    e.print();
  }

  cout << endl;
  cout << "     operator=(const float value)" << endl;
  try {
    textField = (float) 3.14; // try to set Text type to non-string value
  }
  catch (IException &e) {
    e.print();
  }
  try {
    realVectorField3 = (float) 3.14; // try to set single value to vector Field
  }
  catch (IException &e) {
    e.print();
  }

  int size = 2;
  vector<int> intVector2(size, 1);
  vector<double> doubleVector2(size, 3.14);
  vector<float> floatVector2(size, 3.14);
  cout << endl;
  cout << "     operator=(const std::vector<int> &values)" << endl;
  try {
    intVectorField3 = intVector2; // try to set wrong size vector
  }
  catch (IException &e) {
    e.print();
  }
  try {
    realVectorField3 = intVector2;  // try to set float vector to intVector
  }
  catch (IException &e) {
    e.print();
  }

  cout << endl;
  cout << "     operator=(const std::vector<double> &values)" << endl;
  try {
    dblVectorField3 = doubleVector2; // try to set wrong size vector
  }
  catch (IException &e) {
    e.print();
  }
  try {
    intVectorField3 = doubleVector2;  // try to set int vector to double vector
  }
  catch (IException &e) {
    e.print();
  }

  cout << endl;
  cout << "     operator=(const std::vector<float> &value)" << endl;
  try {
    realVectorField3 = floatVector2;  // try to set wrong size vector
  }
  catch (IException &e) {
    e.print();
  }
  try {
    dblVectorField3 = floatVector2;  // try to set double vector to float vector
  }
  catch (IException &e) {
    e.print();
  }
  cout << "----------------------------------------" << endl << endl;
}
