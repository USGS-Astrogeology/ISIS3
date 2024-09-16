/** This is free and unencumbered software released into the public domain.

The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */

#include <QDebug>
#include <QLocale>
#include <QTranslator>

#include "FileName.h"
#include "Gui.h"
#include "IException.h"
#include "IpceMainWindow.h"
#include "QIsisApplication.h"

using namespace std;
using namespace Isis;

int main(int argc, char *argv[]) {
  if (getenv("ISISROOT") == NULL || QString(getenv("ISISROOT")) == "") {
    std::cerr << "Please set ISISROOT before running any Isis applications" << std::endl;
    exit(1);
  }
  Gui::checkX11();

  try {

    // Add the Qt plugin directory to the library path
    FileName qtpluginpath("$ISISROOT/3rdParty/plugins");
    QCoreApplication::addLibraryPath(QString::fromStdString(qtpluginpath.expanded()));

    QApplication *app = new QIsisApplication(argc, argv);
    QApplication::setApplicationName("ipce");

    IpceMainWindow *mainWindow = new IpceMainWindow();

    //  For OSX, had problems with cneteditor view because it has it's own menus, caused the
    //  menubar on OSX to lockup
    QCoreApplication::setAttribute(Qt::AA_DontUseNativeMenuBar, true);

    // We do not want a showMaximized call, as that will negate the settings read during the main
    // window's initialization. References #4358.
    mainWindow->show();
    int status = app->exec();

    delete mainWindow;
    delete app;

    return status;
  }
  catch(IException &e) {
    e.print();
  }

}
