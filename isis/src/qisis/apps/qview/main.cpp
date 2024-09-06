/** This is free and unencumbered software released into the public domain.

The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */

#include <fcntl.h>
#include <iostream>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>

#include <QApplication>
#include <QCoreApplication>
#include <QFontDialog>
#include <QMdiArea>

#include "Application.h"
#include "AdvancedTrackTool.h"
#include "BandTool.h"
#include "BlinkTool.h"
#include "EditTool.h"
#include "LatLonGridTool.h"
#include "FeatureNomenclatureTool.h"
#include "FileName.h"
#include "FileTool.h"
#include "FindTool.h"
#include "Gui.h"
#include "HelpTool.h"
#include "HistogramTool.h"
#include "IString.h"
#include "MatchTool.h"
#include "MeasureTool.h"
#include "PanTool.h"
#include "Preference.h"
#include "PvlGroup.h"
#include "QIsisApplication.h"
#include "RubberBandTool.h"
#include "ScatterPlotTool.h"
#include "SpecialPixelTool.h"
#include "SpatialPlotTool.h"
#include "SpectralPlotTool.h"
#include "SocketThread.h"
#include "StatisticsTool.h"
#include "StereoTool.h"
#include "StretchTool.h"
#include "SunShadowTool.h"
#include "ToolList.h"
#include "ViewportMainWindow.h"
#include "WindowTool.h"
#include "Workspace.h"
#include "ZoomTool.h"

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

  // Add the Qt plugin directory to the library path
  FileName qtpluginpath("$ISISROOT/3rdParty/plugins");
  QCoreApplication::addLibraryPath(qtpluginpath.expanded());

  // Check to see if the user wants to force a new window
  int newWindow = -1;
  for (int i = 1; i < argc; i++) {
    if (IString(argv[i]).UpCase() == "-NEW") {
      newWindow = i;
    }
  }

  QString p_socketFile = "/tmp/isis_qview_" + Application::UserName();
  if (newWindow < 0) {
    struct sockaddr_un p_socketName;
    p_socketName.sun_family = AF_UNIX;
    strcpy(p_socketName.sun_path, p_socketFile.toLatin1().data());
    int p_socket;

    if (((FileName)p_socketFile).fileExists()) {
      // Create a socket
      if ((p_socket = socket(PF_UNIX, SOCK_STREAM, 0)) >= 0) {
        // Try to connect to the socket
        if ((connect(p_socket, (struct sockaddr *)&p_socketName,
                    sizeof(p_socketName))) >= 0) {
          QString temp;
          /*
           * We need a very uncommon token to use for parsing.
           */
          QChar escape(27);
          for (int i = 1; i < argc; i++) {
            temp += QFileInfo(FileName(argv[i]).expanded()).absoluteFilePath();
            temp += QString(escape);
          }
          temp += "raise";
          // Try to send data to the socket
          if (send(p_socket, temp.toLatin1().data(), temp.size(), 0) >= 0) {
            // Success, the other qview will open this file.
            exit(0);
          }
          else {
            std::string msg = "Unable to write to socket";
            std::cout << msg << std::endl;
            remove(p_socketFile.toLatin1().data());
          }
        }

        // If the file already exists but we can't connect to it, assume the
        //   socket is no longer running & remove the tmp file...it falls out &
        //   create a new one. This happens if qview is not already running.
        else {
          remove(p_socketFile.toLatin1().data());
        }
      }
      else {
        std::string msg = "Unable to create socket";
        std::cout << msg << std::endl;
        remove(p_socketFile.toLatin1().data());
      }
    }
  }

  // Creates the qview application window
  QIsisApplication *app = new QIsisApplication(argc, argv);
  QApplication::setApplicationName("qview");

  // check for forcing of gui style
  PvlGroup &uiPref = Preference::Preferences().findGroup(
                             "UserInterface");
  if (uiPref.hasKeyword("GuiStyle")) {
    QString style = QString::fromStdString(uiPref["GuiStyle"]);
    QApplication::setStyle((QString) style);
  }

  ViewportMainWindow *vw = new ViewportMainWindow("qview");

  ToolList tools;
  Tool *rubberBandTool = createTool<RubberBandTool>(vw, &tools);

  Tool *fileTool = createTool<FileTool>(vw, &tools);
  vw->permanentToolBar()->addSeparator();

  Tool *bandTool = createTool<BandTool>(vw, &tools);

  Tool *zoomTool = createTool<ZoomTool>(vw, &tools);
  zoomTool->activate(true);
  vw->getMenu("&View")->addSeparator();

  Tool *panTool = createTool<PanTool>(vw, &tools);
  vw->getMenu("&View")->addSeparator();

  Tool *stretchTool = createTool<StretchTool>(vw, &tools);

  Tool *findTool = createTool<FindTool>(vw, &tools);

  Tool *blinkTool = createTool<BlinkTool>(vw, &tools);

  Tool *advancedTrackTool = createTool<AdvancedTrackTool>(vw, &tools);

  Tool *editTool = createTool<EditTool>(vw, &tools);

  Tool *latLonGridTool = createTool<LatLonGridTool>(vw, &tools);

  Tool *windowTool = createTool<WindowTool>(vw, &tools);

  Tool *measureTool = createTool<MeasureTool>(vw, &tools);

  Tool *sunShadowTool = createTool<SunShadowTool>(vw, &tools);

  Tool *featureNomenclatureTool = createTool<FeatureNomenclatureTool>(vw, &tools);

  Tool *specialPixelTool = createTool<SpecialPixelTool>(vw, &tools);

  Tool *spatialPlotTool = createTool<SpatialPlotTool>(vw, &tools);

  Tool *spectralPlotTool = createTool<SpectralPlotTool>(vw, &tools);

  Tool *scatterPlotTool = createTool<ScatterPlotTool>(vw, &tools);

  Tool *histTool = createTool<HistogramTool>(vw, &tools);

  Tool *statsTool = createTool<StatisticsTool>(vw, &tools);

  Tool *stereoTool = createTool<StereoTool>(vw, &tools);

  Tool *matchTool = createTool<MatchTool>(vw, &tools);

  Tool *helpTool = createTool<HelpTool>(vw, &tools);

  // Show the application window & open the cubes
  vw->show();

  bool openingAFileSucceeded = false;
  for (int i = 1; i < argc; i++) {
    if (i != newWindow) {
      try {
        QString arg(argv[i]);
        if (arg.startsWith("-pref")) {
            i++;
            continue;
        }
        vw->workspace()->addCubeViewport(arg);
        openingAFileSucceeded = true;
      }
      catch (IException &e) {
        e.print();

        // If we're trying to open more later or have opened a file, allow
        //   qview to continue running. Otherwise (this if), crash.
        if (i == argc - 1 && !openingAFileSucceeded) {
          // since we intend to exit, we need to cleanup our heap
          // (tools cleaned since they are vw's children)
          delete vw;
          vw = NULL;
          delete app;
          app = NULL;
          return 1;
        }
      }
    }
  }

  SocketThread *temp = NULL;
  // We don't want to start a thread if the user is forcing a new window
  if (newWindow < 0) {
    temp = new SocketThread();
    temp->connect(temp, SIGNAL(newImage(const QString &)),
                  vw->workspace(), SLOT(addCubeViewport(const QString &)));
    temp->connect(temp, SIGNAL(focusApp()), vw, SLOT(raise()));
    temp->start();
  }

  //Connect the editTool to the file tool in order to save and discard changes
  QObject::connect(editTool, SIGNAL(cubeChanged(bool)),
                   fileTool, SLOT(enableSave(bool)));
  QObject::connect(fileTool, SIGNAL(saveChanges(CubeViewport *)),
                   editTool, SLOT(save(CubeViewport *)));
  QObject::connect(fileTool, SIGNAL(discardChanges(CubeViewport *)),
                   editTool, SLOT(undoAll(CubeViewport *)));
  QObject::connect(editTool, SIGNAL(save()), fileTool, SLOT(save()));
  QObject::connect(editTool, SIGNAL(saveAs()), fileTool, SLOT(saveAs()));
  // Connect the FindTool to the AdvancedTrackTool to record the point if the
  // "record" button is clicked
  QObject::connect(findTool, SIGNAL(recordPoint(QPoint)),
                   advancedTrackTool, SLOT(record(QPoint)));

  //Connect the viewport's close signal to the all windows/subwindows
  QObject::connect(vw, SIGNAL(closeWindow()), fileTool, SLOT(exit()));

  int status = app->exec();

  // If we created a thread for listening to qview connections, then stop the thread and free its memory
  if (temp) {
    temp->stop();
    temp->wait(); // wait for the stop to finish
    delete temp;

    remove(p_socketFile.toLatin1().data());
  }

  delete panTool;
  delete statsTool;
  delete helpTool;
  delete matchTool;
  delete latLonGridTool;
  delete stereoTool;
  delete histTool;
  delete spatialPlotTool;
  delete spectralPlotTool;
  delete scatterPlotTool;
  delete specialPixelTool;
  delete featureNomenclatureTool;
  delete sunShadowTool;
  delete measureTool;
  delete windowTool;
  delete advancedTrackTool;
  delete blinkTool;
  delete findTool;
  delete stretchTool;
  delete bandTool;
  delete rubberBandTool;
  delete vw;
  delete app;
  //delete QCoreApplication::instance();
  return status;
}
