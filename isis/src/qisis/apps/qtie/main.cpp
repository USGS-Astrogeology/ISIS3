/** This is free and unencumbered software released into the public domain.

The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */

#include <QApplication>
#include <QCoreApplication>
#include <QToolBar>

#include "Gui.h"
#include "IException.h"
#include "ViewportMainWindow.h"
#include "QtieFileTool.h"
#include "BandTool.h"
#include "ZoomTool.h"
#include "PanTool.h"
#include "StretchTool.h"
#include "FindTool.h"
#include "WindowTool.h"
#include "AdvancedTrackTool.h"
#include "HelpTool.h"
#include "QtieTool.h"
#include "RubberBandTool.h"
#include "ToolList.h"

#define IN_QTIE

using namespace Isis;

template<typename ToolClass>
ToolClass *createTool(ViewportMainWindow *viewportMainWindow, ToolList *tools) {
  ToolClass *result = new ToolClass(viewportMainWindow);

  tools->append(result);
  ((Tool *)result)->addTo(viewportMainWindow);

  return result;
}

int main(int argc, char *argv[]) {
  if (getenv("ISISROOT") == NULL || QString(getenv("ISISROOT")) == "") {
    std::cerr << "Please set ISISROOT before running any Isis applications" << std::endl;
    exit(1);
  }
  Isis::Gui::checkX11();

  try {

    // Add the Qt plugin directory to the library path
    FileName qtpluginpath("$ISISROOT/3rdParty/plugins");
   QCoreApplication::addLibraryPath(QString::fromStdString(qtpluginpath.expanded()));

    QApplication *app = new QApplication(argc, argv);
    QApplication::setApplicationName("qtie");
    app->setStyle("windows");

    ViewportMainWindow *vw = new ViewportMainWindow("Qtie");

    ToolList tools;
    createTool<RubberBandTool>(vw, &tools);

    QtieFileTool *ftool = createTool<QtieFileTool>(vw, &tools);
    vw->permanentToolBar()->addSeparator();

    createTool<BandTool>(vw, &tools);

    createTool<ZoomTool>(vw, &tools);
    vw->getMenu("&View")->addSeparator();

    createTool<PanTool>(vw, &tools);
    vw->getMenu("&View")->addSeparator();

    Tool *stool = createTool<StretchTool>(vw, &tools);

    createTool<FindTool>(vw, &tools);

    createTool<AdvancedTrackTool>(vw, &tools);

    createTool<WindowTool>(vw, &tools);

    vw->permanentToolBar()->addSeparator();
    createTool<HelpTool>(vw, &tools);

    Tool *tieTool = createTool<QtieTool>(vw, &tools);
    tieTool->activate(true);

    QObject::connect(ftool, SIGNAL(cubesOpened(Cube *, Cube *, ControlNet *)),
                     tieTool, SLOT(setFiles(Cube *, Cube *, ControlNet *)));

    QObject::connect(ftool, SIGNAL(newFiles()), tieTool, SLOT(clearFiles()));

    QObject::connect(vw, SIGNAL(closeWindow()), ftool, SLOT(exit()));

    QObject::connect(stool,
                     SIGNAL(stretchChipViewport(Stretch *, CubeViewport *)),
                     tieTool,
                     SIGNAL(stretchChipViewport(Stretch *, CubeViewport *)));

    vw->show();

    int status = app->exec();

    //Clean up the objects allocated onto the heap

    delete ftool;
    ftool = NULL;
    delete stool;
    stool = NULL;
    delete tieTool;
    tieTool = NULL;
    delete vw;
    vw = NULL;
    delete app;
    app = NULL;

    return status;
  }
  catch (IException &e) {
    e.print();
  }
}
