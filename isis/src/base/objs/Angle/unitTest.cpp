/** This is free and unencumbered software released into the public domain.
The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */

#include <iostream>
#include <iomanip>
#include <limits>

#include <QDebug>

#include "Angle.h"
#include "Constants.h"
#include "IException.h"
#include "Preference.h"
#include "SpecialPixel.h"

using namespace Isis;
using namespace std;

class MyAngle : public Isis::Angle {
public:
  MyAngle(double angle, Angle::Units unit) : Isis::Angle(angle, unit) {}

  void TestUnitWrapValue() {
    cerr << "  Degree wrap value = " << unitWrapValue(Angle::Degrees) << endl;
    cerr << "  Radian wrap value = " << unitWrapValue(Angle::Radians) << endl;
  }

  void setAngle(const double& angleValue, const Units& unit) {
    Angle::setAngle(angleValue, unit);
    cerr << "angle set to " << angle(unit) << endl;
  }

};


int main(int argc, char *argv[]) {
  Isis::Preference::Preferences(true);
  cerr << setprecision(9);

  cerr << "UnitTest for Angle" << endl << endl;
  cerr << "Testing constructors" << endl;

  try {
    Angle angle;
    cerr << "  Default constructor - valid?:  " << angle.isValid() <<
      " values: " << angle.radians() << " and " << angle.degrees() <<
      endl;
    cerr << "  " << angle.toString().toStdString() << endl;
  }
  catch(Isis::IException &e) {
    e.print();
  }

  try {
    Angle angle(Isis::Null, Angle::Degrees);
    cerr << "  Null input and degree output:  " << angle.degrees() <<
      " degrees" <<endl;
    cerr << "  Valid? " << angle.isValid() << endl;
    cerr << "  " << angle.toString().toStdString() << endl;
  }
  catch(Isis::IException &e) {
    e.print();
  }

  try {
    Angle angle(30., Angle::Degrees );
    cerr << "  Degree input and radian output:  " << angle.radians() <<
      " radians" << endl;
    cerr << "  Valid? " << angle.isValid() << endl;
    cerr << "  " << angle.toString().toStdString() << endl;
  }
  catch(Isis::IException &e) {
    e.print();
  }

  try {
    Angle angle(30. * PI / 180., Angle::Radians );
    cerr << "  Radian input and degree output:  " << angle.degrees() <<
      " degrees" <<endl;
    cerr << "  " << angle.toString(false).toStdString() << endl;
  }
  catch(Isis::IException &e) {
    e.print();
  }

  try {
    Angle angle(30., Angle::Degrees );
    Angle angleCopy(angle);
    cerr <<"  Copy constructor:  " << angleCopy.degrees() << " degrees" <<
        endl;
  }
  catch(Isis::IException &e) {
    e.print();
  }

  cerr << endl << "Testing mutators" << endl;

  try {
    Angle angle(30., Angle::Degrees );
    angle.setDegrees(180);
    cerr <<"  setDegrees:  " << angle.degrees() << " degrees" <<
        endl;
    angle.setRadians(PI);
    cerr <<"  setRadians:  " << angle.radians() << " radians" <<
        endl;
  }
  catch(Isis::IException &e) {
    e.print();
  }

  cerr << endl << "Testing operators" << endl;

  try {
    Angle angle(45.0, Angle::Degrees);
    qDebug() << "  QDebug operator:  " << angle;
  }
  catch(Isis::IException &e) {
    e.print();
  }

  try {
    Angle angle(30., Angle::Degrees );
    angle = Angle(45., Angle::Degrees);
    cerr << "  Assignment operator:  " << angle.degrees() << " degrees" <<
        endl;
  }
  catch(Isis::IException &e) {
    e.print();
  }

  try {
    Angle angle1(30., Angle::Degrees );
    Angle angle2(60., Angle::Degrees );
    angle1 = angle1 + angle2;
    // Begin with 30 degrees and end with 30 degrees
    cerr << "  + and - operators..." << endl;
    cerr << "    angle + angle: " << angle1.degrees() << " degrees" <<endl;
    angle1 += angle2;
    cerr << "    angle += angle: " << angle1.degrees() << " degrees" <<endl;
    angle1 -= angle2;
    cerr << "    angle -= angle: " << angle1.degrees() << " degrees" <<endl;
    angle1 = angle1 - angle2;
    cerr << "    angle - angle: " << angle1.degrees() << " degrees" <<endl;
  }
  catch(Isis::IException &e) {
    e.print();
  }

  try {
    Angle angle(30., Angle::Degrees );
    // Begin with 30 degrees and end with 30 degrees
    cerr << "  * and / operators..." << endl;
    angle = 2. * angle;
    cerr << "    double * angle: " << angle.degrees() << " degrees" <<endl;
    angle *= 2;
    cerr << "    angle *= double: " << angle.degrees() << " degrees" <<endl;
    angle /= 2;
    cerr << "    angle /= double: " << angle.degrees() << " degrees" <<endl;
    angle = angle / 2;
    cerr << "    angle / double: " << angle.degrees() << " degrees" <<endl;
  }
  catch(Isis::IException &e) {
    e.print();
  }

  cerr << endl << "Testing logical operators" << endl;

  try {
    Angle angle1(30., Angle::Degrees);
    Angle angle2(45., Angle::Degrees);
    cerr << "  angle1 == angle2?  " << (angle1 == angle2) << endl;
    cerr << "  angle1 == angle2?  " << (Angle() == Angle()) << endl;
    cerr << "  angle1 == angle2?  " << (Angle() == angle2) <<endl;
  }
  catch(Isis::IException &e) {
    e.print();
  }

  try {
    Angle angle1(30., Angle::Degrees );
    Angle angle2(45., Angle::Degrees);
    cerr << "  angle1 <  angle2?  ";
    cerr << (angle1 < angle2) << endl;
  }
  catch(Isis::IException &e) {
    e.print();
  }

  try {
    Angle angle1(30., Angle::Degrees );
    Angle angle2(45., Angle::Degrees);
    //Angle epsilon((double)numeric_limits<float>::epsilon(), Angle::Degrees);//1.1920929e-07
    Angle epsilon(1.1920929e-12, Angle::Degrees);//1.1920929e-07
    cerr << "  angle1 <  (angle1 + epsilon)?  ";
    cerr << (angle1 < angle1 + epsilon) << endl;
  }
  catch(Isis::IException &e) {
    e.print();
  }

  try {
    Angle angle1(30., Angle::Degrees );
    Angle angle2(45., Angle::Degrees);
    Angle epsilon(1.1920929e-12, Angle::Degrees);
    cerr << "  angle2 >  (angle2 - epsilon)?  ";
    cerr << (angle2 > angle2 - epsilon) << endl;
  }
  catch(Isis::IException &e) {
    e.print();
  }

  try {
    Angle angle1(30., Angle::Degrees );
    Angle angle2(45., Angle::Degrees);
    cerr << "  angle1 <= angle2?  ";
    cerr << (angle1 <= angle2) << endl;
  }
  catch(Isis::IException &e) {
    e.print();
  }

  try {
    Angle angle1(30., Angle::Degrees );
    cout << "  angle1 <  angle2?  ";
    cerr << (angle1 < Angle());
  }
  catch(Isis::IException &e) {
    e.print();
  }

  try {
    Angle angle1(30., Angle::Degrees );
    cout << "  angle1 <  angle2?  ";
    cerr  << (Angle() < angle1);
  }
  catch(Isis::IException &e) {
    e.print();
  }

  try {
    Angle angle1(30., Angle::Degrees );
    cerr << "  angle1 <=  angle2?  ";
    cerr << (Angle() <= angle1);
  }
  catch(Isis::IException &e) {
    e.print();
  }

  try {
    Angle angle1(30., Angle::Degrees );
    Angle angle2(45., Angle::Degrees);
    cerr << "  angle1 >  angle2?  ";
    cerr << (angle1 > angle2) << endl;
  }
  catch(Isis::IException &e) {
    e.print();
  }

  try {
    Angle angle1(30., Angle::Degrees );
    cerr << "  angle1 >  angle2?  ";
    cerr << (angle1 > Angle()) << endl;
  }
  catch(Isis::IException &e) {
    e.print();
  }

  try {
    Angle angle1(30., Angle::Degrees );
    Angle angle2(45., Angle::Degrees);
    cerr << "  angle1 >= angle2?  ";
    cerr << (angle1 >= angle2) << endl;
  }
  catch(Isis::IException &e) {
    e.print();
  }

  try {
    Angle angle1(30., Angle::Degrees );
    cerr << "  angle1 >= angle2?  ";
    cerr << (angle1 >= Angle()) << endl;
  }
  catch(Isis::IException &e) {
    e.print();
  }

  cerr << endl << "Testing protected methods" << endl;

  try {
    MyAngle angle(30., Angle::Degrees);
    angle.TestUnitWrapValue();
  }
  catch(Isis::IException &e) {
    e.print();
  }

  try {
    MyAngle angle(0., Angle::Degrees);
    cerr << "  Degree ";
    angle.setAngle(60., Angle::Degrees);
  }
  catch(Isis::IException &e) {
    e.print();
  }

  try {
    MyAngle angle(0., Angle::Radians);
    cerr << "  Radian ";
    angle.setAngle(.5, Angle::Degrees);
  }
  catch(Isis::IException &e) {
    e.print();
  }

  //Test Angle::Angle(QString):
  try {
    Angle angle(QString("-70 15 30.125"));
    cerr << angle.toString().toStdString() << endl;
  }
  catch (Isis::IException &e) {
    e.print();
  }

  try {
    Angle angle(QString("  +70  30 11     "));
    cerr << angle.toString().toStdString() << endl;
  }
  catch (Isis::IException &e) {
    e.print();
  }

  try {
    Angle angle(QString("100"));
    cerr << angle.toString().toStdString() << endl;
  }
  catch (Isis::IException &e) {
    e.print();
  }

  try {
    Angle angle(QString("70 11"));
    cerr << angle.toString().toStdString() << endl;
  }
  catch (Isis::IException &e) {
    e.print();
  }

  try {
    Angle angle(QString("this 79 should 00 fail 0.111"));
    cerr << angle.toString().toStdString() << endl;
  }
  catch (Isis::IException &e) {
    e.print();
  }

  try {
    Angle angle(QString("100 00 00"));
    cerr << angle.toString().toStdString() << endl;
  }
  catch (Isis::IException &e) {
    e.print();
  }

}
