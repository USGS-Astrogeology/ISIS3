/** This is free and unencumbered software released into the public domain.

The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */

#include <QCoreApplication>

#include "Gui.h"
#include "IException.h"
#include "MosaicMainWindow.h"
#include "Preference.h"
#include "PvlGroup.h"
#include "QIsisApplication.h"

using namespace std;
using namespace Isis;

int main(int argc, char *argv[]) {
  if (getenv("ISISROOT") == NULL || QString(getenv("ISISROOT")) == "") {
    std::cerr << "Please set ISISROOT before running any Isis applications" << std::endl;
    exit(1);
  }
  Isis::Gui::checkX11();

  // Add the Qt plugin directory to the library path
  FileName qtpluginpath("$ISISROOT/3rdParty/plugins");
  QCoreApplication::addLibraryPath(QString::fromStdString(qtpluginpath.expanded()));

  QApplication *app = new QIsisApplication(argc, argv);
  QApplication::setApplicationName("qmos");

  MosaicMainWindow *mainWindow = new MosaicMainWindow("qmos");
  mainWindow->show();

  int status = app->exec();

  delete mainWindow;
  delete app;

  return status;
}
