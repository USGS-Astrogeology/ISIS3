/** This is free and unencumbered software released into the public domain.
The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */
#include <QColor>
#include <QDebug>

#include "Color.h"
#include "IException.h"
#include "Preference.h"

using namespace Isis;

void testColor(QColor);

int main(int argc, char **argv) {
  Preference::Preferences(true);

  qDebug() << "Test Color to string and back";
  testColor(QColor());
  testColor(QColor(0, 0, 0));
  testColor(QColor(0, 0, 0, 0));
  testColor(QColor(255, 0, 0, 0));
  testColor(QColor(0, 255, 0, 0));
  testColor(QColor(0, 0, 255, 0));
  testColor(QColor(0, 0, 0, 255));
  testColor(QColor(255, 255, 255, 255));
  testColor(QColor(10, 20, 30, 40));

  // Test bad color input
  qDebug() << "\nTest invalid color strings";
  qDebug() << Color::fromRGBAString("#rrggbbaa");
  qDebug() << Color::fromRGBAString(" 00112233");
  qDebug() << Color::fromRGBAString("");
  qDebug() << Color::fromRGBAString("#001122");
}


void testColor(QColor color) {
  qDebug() << "\tTesting color:" << color;
  try {
    QString rgbaString = Color::toRGBAString(color);
    qDebug() << "\t\t" << rgbaString;

    bool regexMatch = Color::colorRGBAFormat().exactMatch(Color::toRGBAString(color));
    qDebug() << "\t\tFormat is good?" << regexMatch;
    bool serializeSuccess = (color == Color::fromRGBAString(Color::toRGBAString(color)));
    qDebug() << "\t\tSerialized successfully?" << serializeSuccess;
  }
  catch (IException &e) {
    qDebug() << "\t\t" << QString::fromStdString(e.toString());
  }
}
