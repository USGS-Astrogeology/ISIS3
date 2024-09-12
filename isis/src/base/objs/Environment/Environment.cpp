/** This is free and unencumbered software released into the public domain.
The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */
#include "Environment.h"

#include <iostream>
#include <stdlib.h>

#include <QString>
#include <QStringList>
#include <QCoreApplication>

#include "IException.h"
#include "IString.h"
#include "TextFile.h"

 /**
  *
  *
  */
  static void QStartup() {
    // Set the Qt plugin directory
    QStringList pluginPaths;

    Isis::IString root = Isis::Environment::getEnvironmentValue("ISISROOT", "").toStdString();
    QCoreApplication *core = QCoreApplication::instance();
    if ( !core ) {
      std::cout << "****  Qt Plugin Path is not set because no instance of QCoreApplication ****\n";
    }

    return;
  }

// Add this to ensure Qt is set up properly
Q_COREAPP_STARTUP_FUNCTION(QStartup);

namespace Isis {


  Environment Environment::automaticEnvironmentSetup;

  Environment::Environment() {

#if 0
    // Set the Qt plugin directory
    QStringList pluginPaths;

    IString root = getEnvironmentValue("ISISROOT", "");
    std::cout << "ISISROOT=" << root << "\n";

    if (root != "") {

      QCoreApplication *core = QCoreApplication::instance();
      if ( !core ) {
        std::cout << "QCoreApplication doesn't exist yet!\n";
      }
      IString thirdPartyPluginPath = root + "/3rdParty/plugins";
      pluginPaths << thirdPartyPluginPath;
      std::cout << "PluginPaths=" << pluginPaths.join(";") << "\n";
      QCoreApplication::setLibraryPaths(pluginPaths);
    }
#endif
  }


  /**
   * @Returns the user name. Returns 'Unknown' if it cannot find the user name.
   */
  QString Environment::userName() {
    return getEnvironmentValue("USER", "Unknown");
  }


  /**
   * @returns the host name.  Returns 'Unknown' if it cannot find the host name.
   */
  QString Environment::hostName() {
    return getEnvironmentValue("HOST", "Unknown");
  }


  /**
   * @param variable The environment variable to get
   * @param defaultValue The returned value for variable if variable doesn't
   *                     exist.
   *
   * @returns The value for the environment variable requested.
   */
  QString Environment::getEnvironmentValue(QString variable,
      QString defaultValue) {

    QString value = defaultValue;

    char *envValue = getenv(variable.toLatin1().data());
    if (envValue)
      value = envValue;

    return value;
  }


  /**
   * @returns the Isis version in the format isis?.?.?.?qualifier | date
   */
  QString Environment::isisVersion() {
    TextFile versionFile("$ISISROOT/isis_version.txt");
    QString line1, line2, line3, line4;
    versionFile.GetLine(line1);
    versionFile.GetLine(line2);
    versionFile.GetLine(line3);
    versionFile.GetLine(line4);

    QRegExp validPartOfLine("[^ #]*");
    if (validPartOfLine.indexIn(line1) != -1) {
      line1 = validPartOfLine.cap();
    }
    else {
      IString msg = "$ISISROOT/isis_version.txt line 1, no valid text found";
      throw IException(IException::Programmer, msg, _FILEINFO_);
    }

    if (validPartOfLine.indexIn(line2) != -1) {
      line2 = validPartOfLine.cap();
    }
    else {
      IString msg = "$ISISROOT/isis_version.txt line 2, no valid text found";
      throw IException(IException::Programmer, msg, _FILEINFO_);
    }

    if (validPartOfLine.indexIn(line4) != -1) {
      line4 = validPartOfLine.cap();
    }
    else {
      IString msg = "$ISISROOT/isis_version.txt line 4, no valid text found";
      throw IException(IException::Programmer, msg, _FILEINFO_);
    }

    return line1 + " " + line4 + " | " + line2;
  }


}
