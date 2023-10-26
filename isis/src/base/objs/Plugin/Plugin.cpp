/** This is free and unencumbered software released into the public domain.
The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */
#include "Plugin.h"

#include <ostream>

#include <QCoreApplication>
#include <QLibrary>
#include <QStringList>

#include "FileName.h"
#include "IException.h"

using namespace std;

namespace Isis {
  //! Constructs a Plugin object.
  Plugin::Plugin() : Pvl() {
  }

  /**
   * This method returns a void pointer to a C function (i.e., the plugin) It
   * does this by looking in itself (Pvl) for the keyword LIBRARY and the
   * keyword ROUTINE.  When you write this function make sure to place extern
   * "C" infront of it to eliminate C++ symbol mangling.
   *
   * @param group The group name.
   *
   * @return A void pointer to a C function (i.e., the plugin)
   *
   * @history 2010-03-16  Tracie Sucharski,  Added paths to plugin.  First
   *                            try to load from current working directory,
   *                            then from $ISISROOT/lib.
   */
  QFunctionPointer Plugin::GetPlugin(const QString &group) {
    // Get the library and plugin to load
    PvlGroup &g = findGroup(group.toStdString());
    QString library = QString::fromStdString(g["Library"]);

    QString path = "./";
    Isis::FileName libraryFile(path + library);

    QString pluginName = QString::fromStdString(g["Routine"]);

    // Open the library, resolve the routine name, and return the function
    // address. The function will stay in memory until the application exists
    // so the scope of lib does not matter.
    QLibrary lib(libraryFile.expanded());
    bool loadedOk = lib.load();

    if(!loadedOk) {
      path = "$ISISROOT/lib/";
      libraryFile = path + library;
    }

    lib.setFileName(libraryFile.expanded());

    QFunctionPointer plugin = lib.resolve(pluginName.toLatin1().data());
    if (plugin == 0) {
      QString msg = "Unable to find plugin [" + pluginName +
                    "] in shared library [" + lib.fileName() + "]";
      throw IException(IException::Unknown, msg, _FILEINFO_);
    }

    return plugin;
  }
} // end namespace isis
