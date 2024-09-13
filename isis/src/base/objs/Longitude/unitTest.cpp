/** This is free and unencumbered software released into the public domain.
The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */
#include "Longitude.h"

#include <iostream>

#include <QList>
#include <QListIterator>
#include <QPair>

#include "IException.h"
#include "Constants.h"
#include "Preference.h"

using std::cout;
using std::endl;

using namespace Isis;

int main(int argc, char *argv[]) {
  Preference::Preferences(true);

  cout.precision(14);

  cout << "----- Testing Constructors -----" << endl << endl;

  try {
    cout << "Default constructor" << endl;
    Longitude lon;
    cout << lon.degrees() << " degrees" << endl;
  }
  catch(IException &e) {
    e.print();
  }

  try {
    cout << "Constructor given a value in degrees" << endl;
    Longitude lon(180.0, Angle::Degrees);
    cout << lon.degrees() << " degrees" << endl;
  }
  catch(IException &e) {
    e.print();
  }

  try {
    cout << "Constructor given a positive west value in degrees" << endl;
    Longitude lon(180.0, Angle::Degrees, Longitude::PositiveWest);
    cout << lon.degrees() << " degrees" << endl;
  }
  catch(IException &e) {
    e.print();
  }

  try {
    cout << "Constructor given a positive west, -90 value in degrees" << endl;
    Longitude lon(-90.0, Angle::Degrees, Longitude::PositiveWest,
        Longitude::Domain180);
    cout << lon.degrees() << " degrees" << endl;
  }
  catch(IException &e) {
    e.print();
  }

  try {
    cout << "Constructor given -90 degrees PW & 360 domain" << endl;
    Longitude lon(-90.0, Angle::Degrees, Longitude::PositiveWest,
        Longitude::Domain360);
    cout << lon.degrees() << " degrees" << endl;
  }
  catch(IException &e) {
    e.print();
  }

  try {
    cout << "Copy constructor" << endl;
    Longitude lon(-90.0, Angle::Degrees, Longitude::PositiveWest,
        Longitude::Domain360);
    cout << lon.degrees() << " degrees == " << Longitude(lon).degrees() <<
      " degrees" << endl;
  }
  catch(IException &e) {
    e.print();
  }

  cout << endl << "----- Testing Set Methods -----" << endl << endl;

  try {
    cout << "Set to 90 degrees" << endl;
    Longitude lon(270.0, Angle::Degrees);
    lon.setPositiveEast(90, Angle::Degrees);
    cout << lon.degrees() << " degrees" << endl;
  }
  catch(IException &e) {
    e.print();
  }

  try {
    cout << "Set to 90 degrees PW" << endl;
    Longitude lon(270.0, Angle::Degrees);
    Longitude lonCopy(lon);
    lon.setPositiveWest(90, Angle::Degrees);
    cout << lon.degrees() << " degrees" << endl;

    lonCopy = lon;
    cout << "After assignment: " << lonCopy.degrees() << " degrees" << endl;
  }
  catch(IException &e) {
    e.print();
  }

  cout << endl << "----- Testing Get Methods -----" << endl << endl;

  try {
    cout << "90 degrees is" << endl;
    Longitude lon(90.0, Angle::Degrees);
    cout << lon.degrees() << " degrees universal" << endl;
    cout << lon.positiveEast(Angle::Degrees) << " degrees positive east"
        << endl;
    cout << lon.positiveEast(Angle::Radians) / PI << "*pi radians positive "
        "east" << endl;
    cout << lon.positiveWest(Angle::Degrees) << " degrees positive west"
      << endl;
    cout << lon.positiveWest(Angle::Radians) / PI << "*pi radians positive "
        "west" << endl;
  }
  catch(IException &e) {
    e.print();
  }

  cout << endl;

  try {
    cout << "450 degrees is" << endl;
    Longitude lon(450.0, Angle::Degrees);
    cout << lon.degrees() << " degrees universal" << endl;
    cout << lon.positiveEast(Angle::Degrees) << " degrees positive east"
        << endl;
    cout << lon.positiveEast(Angle::Radians) / PI << "*pi radians positive "
        "east" << endl;
    cout << lon.positiveWest(Angle::Degrees) << " degrees positive west"
      << endl;
    cout << lon.positiveWest(Angle::Radians) / PI << "*pi radians positive "
        "west" << endl;
  }
  catch(IException &e) {
    e.print();
  }

  cout << endl;

  try {
    cout << "-450 degrees is" << endl;
    Longitude lon(-450.0, Angle::Degrees);
    cout << lon.degrees() << " degrees universal" << endl;
    cout << lon.positiveEast(Angle::Degrees) << " degrees positive east"
        << endl;
    cout << lon.positiveEast(Angle::Radians) / PI << "*pi radians positive "
        "east" << endl;
    cout << lon.positiveWest(Angle::Degrees) << " degrees positive west"
      << endl;
    cout << lon.positiveWest(Angle::Radians) / PI << "*pi radians positive "
        "west" << endl;
  }
  catch(IException &e) {
    e.print();
  }

  cout << endl;

  try {
    cout << "-450 degrees PW is" << endl;
    Longitude lon(-450.0, Angle::Degrees, Longitude::PositiveWest);
    cout << lon.degrees() << " degrees universal" << endl;
    cout << lon.positiveEast(Angle::Degrees) << " degrees positive east"
        << endl;
    cout << lon.positiveEast(Angle::Radians) / PI << "*pi radians positive "
        "east" << endl;
    cout << lon.positiveWest(Angle::Degrees) << " degrees positive west"
      << endl;
    cout << lon.positiveWest(Angle::Radians) / PI << "*pi radians positive "
        "west" << endl;
  }
  catch(IException &e) {
    e.print();
  }

  cout << endl << "----- Testing Domain Methods -----" << endl << endl;

  try {
    cout << "Test force180Domain" << endl;
    Longitude lon(270.0, Angle::Degrees);
    cout << lon.force180Domain().degrees() << " degrees" << endl;

    cout << "Test force360Domain" << endl;
    cout << lon.force360Domain().degrees() << " degrees" << endl;
    lon.setPositiveEast(360.0, Angle::Degrees);
    cout << "Force " << lon.degrees() << " to the 360 domain: "
         << lon.force360Domain().degrees() << " degrees" << endl;
  }
  catch(IException &e) {
    e.print();
  }
  
  cout << endl << "----- Testing Range Methods -----" << endl << endl;
  try {
    cout << "Test to360Range" << endl;
    QList< QPair<Longitude, Longitude> > data;
    data  << qMakePair(Longitude(), Longitude());
    data  << qMakePair(Longitude(120, Angle::Degrees), Longitude());
    data  << qMakePair(Longitude(), Longitude(130, Angle::Degrees));
    data  << qMakePair(Longitude(120, Angle::Degrees), Longitude(120, Angle::Degrees));
    data  << qMakePair(Longitude(315, Angle::Degrees), Longitude(0, Angle::Degrees));
    data  << qMakePair(Longitude(350, Angle::Degrees), Longitude(10, Angle::Degrees));
    data  << qMakePair(Longitude(350, Angle::Degrees), Longitude(300, Angle::Degrees));
    data  << qMakePair(Longitude(120, Angle::Degrees), Longitude(130, Angle::Degrees));
    data  << qMakePair(Longitude(-10, Angle::Degrees), Longitude(-5, Angle::Degrees));
    data  << qMakePair(Longitude(-200, Angle::Degrees), Longitude(-190, Angle::Degrees));
    data  << qMakePair(Longitude(0, Angle::Degrees), Longitude(360, Angle::Degrees));
    data  << qMakePair(Longitude(-180, Angle::Degrees), Longitude(180, Angle::Degrees));
    data  << qMakePair(
        Longitude(-180.1, Angle::Degrees, Longitude::PositiveEast, Longitude::Domain180),
        Longitude(180.1, Angle::Degrees, Longitude::PositiveEast, Longitude::Domain180));
    data  << qMakePair(Longitude(-180.1, Angle::Degrees), Longitude(160, Angle::Degrees));
    data  << qMakePair(Longitude(-800, Angle::Degrees), Longitude(-200, Angle::Degrees));
    data  << qMakePair(Longitude(-0.1/1e-100, Angle::Degrees), Longitude(-200, Angle::Degrees));
    data  << qMakePair(Longitude(100, Angle::Degrees), Longitude(20, Angle::Degrees));
    data  << qMakePair(Longitude(460, Angle::Degrees), Longitude(740, Angle::Degrees));
    data  << qMakePair(Longitude(100, Angle::Degrees), Longitude(465, Angle::Degrees));
    data  << qMakePair(Longitude(300, Angle::Degrees), Longitude(465, Angle::Degrees));
    data  << qMakePair(Longitude(-10, Angle::Degrees), Longitude(10, Angle::Degrees));
    data  << qMakePair(Longitude(-45, Angle::Degrees), Longitude(0, Angle::Degrees));

    QPair<Longitude, Longitude> current;
    foreach(current, data) {
      QList< QPair<Longitude, Longitude> > results =
          Longitude::to360Range(current.first, current.second);
      cout << "Input Range: " << current.first.toString().toStdString() << " to " << current.second.toString().toStdString()
           << endl;
      cout << "Input Range (PW): "
            << Angle(current.first.positiveWest(Angle::Degrees), Angle::Degrees).toString().toStdString()
            << " to "
            << Angle(current.second.positiveWest(Angle::Degrees), Angle::Degrees).toString().toStdString()
            << endl;

        cout << "\tTest inRange" << endl;
        cout << "\t\t0 degrees: "
             << Longitude(0, Angle::Degrees).inRange(current.first, current.second) << endl
             << "\t\t200 degrees: "
             << Longitude(200, Angle::Degrees).inRange(current.first, current.second) << endl
             << "\t\t360 degrees: "
             << Longitude(360, Angle::Degrees).inRange(current.first, current.second) << endl
             << "\t\t-270 degrees: "
             << Longitude(-270, Angle::Degrees).inRange(current.first, current.second) << endl
             << endl;

      QListIterator< QPair<Longitude, Longitude> > it(results);
      while (it.hasNext()) {
        current = it.next();
        cout << "\tOutput Range: " << current.first.toString().toStdString() << " to "
             << current.second.toString().toStdString() << endl;
        cout << "\tOutput Range (PW): "
             << Angle(current.first.positiveWest(Angle::Degrees), Angle::Degrees).toString().toStdString()
             << " to "
             << Angle(current.second.positiveWest(Angle::Degrees), Angle::Degrees).toString().toStdString()
             << endl;

        cout << "\tTest inRange" << endl;
        cout << "\t\t0 degrees: "
             << Longitude(0, Angle::Degrees).inRange(current.first, current.second) << endl
             << "\t\t45 degrees: "
             << Longitude(45, Angle::Degrees).inRange(current.first, current.second) << endl
             << "\t\t90 degrees: "
             << Longitude(90, Angle::Degrees).inRange(current.first, current.second) << endl
             << "\t\t273 degrees: "
             << Longitude(273, Angle::Degrees).inRange(current.first, current.second) << endl
             << endl;
      }   
    }
  }
  catch(IException &e) {
    e.print();
  }
}
