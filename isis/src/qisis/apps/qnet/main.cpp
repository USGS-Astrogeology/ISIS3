/** This is free and unencumbered software released into the public domain.

The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */

#include <signal.h>

#include <QApplication>

#include "AdvancedTrackTool.h"
#include "BandTool.h"
#include "FileName.h"
#include "FindTool.h"
#include "Gui.h"
#include "HelpTool.h"
#include "HistogramTool.h"
#include "IException.h"
#include "PanTool.h"
#include "Preference.h"
#include "PvlGroup.h"
#include "QIsisApplication.h"
#include "QnetFileTool.h"
#include "QnetTool.h"
#include "QnetNavTool.h"
#include "RubberBandTool.h"
#include "StatisticsTool.h"
#include "StretchTool.h"
#include "ToolList.h"
#include "ViewportMainWindow.h"
#include "WindowTool.h"
#include "ZoomTool.h"

using namespace Isis;
using namespace std;

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
    QCoreApplication::addLibraryPath(qtpluginpath.expanded());


    QIsisApplication *app = new QIsisApplication(argc,argv);
    QApplication::setApplicationName("qnet");

    // check for forcing of gui style
    PvlGroup &uiPref = Preference::Preferences().findGroup(
        "UserInterface");
    if (uiPref.hasKeyword("GuiStyle")) {
      QString style = QString::fromStdString(uiPref["GuiStyle"]);
      QApplication::setStyle((QString) style);
    }

    ViewportMainWindow *vw = new ViewportMainWindow("qnet");

    ToolList tools;
    Tool *rubberBandTool = createTool<RubberBandTool>(vw , &tools);

    QnetTool *qnetTool = new QnetTool(vw);

    /**** ADD FILE TOOL FIRST SO THAT IT APPEARS FIRST IN THE PERMANENT AND MENU TOOLBARS ****/
    // adds file tool buttons and separator on permanent toolbar
    // adds to "File" dropdown of Menu toolbar
    QnetFileTool *ftool = new QnetFileTool(qnetTool, vw);
    ((Tool *)ftool)->addTo(vw);
    tools.append(ftool);
    vw->permanentToolBar()->addSeparator();


    // Show the ViewportMainWindow - we need this because there is an bug with Qt5 that causes the
    // NavTool to go behind the ViewportMainWindow when using Gnome and Cinnamon (KDE doesn't have
    // this bug), unless we show the ViewportMainWindow before the NavTool is created.
    // No idea why this fixes it. This issue appeared after we converted to Qt5. According to Qt
    // a child widget should always be in front of it's parent widget but that isn't the case here
    // for some reason. See redmine ticket #4541.
    //
    // TODO: Figure out why the NavTool wasn't in front in the first place, and figure out why this
    // fixes it
    // TODO: Figure out a way to keep a Qt widget sibling on top of another sibling. (ie keep the
    // NavTool on top of QnetTool)
    vw->show();

    QnetNavTool *ntool = new QnetNavTool(qnetTool, vw);
    ((Tool *)ntool)->addTo(vw);
    tools.append(ntool);

    /**** ADD TOOLS TO TOOL PAD ON LEFT/RIGHT ****/
    // adds band tool button to toolpad on left
    Tool *btool = createTool<BandTool>(vw, &tools);

    // adds zoom tool button to toolpad on left
    // adds to "View" dropdown of Menu toolbar with seperator
    Tool *ztool = createTool<ZoomTool>(vw, &tools);
    vw->getMenu("&View")->addSeparator();

    // adds pan tool button to toolpad on left
    // adds to "View" dropdown of Menu toolbar with seperator
    Tool *ptool = createTool<PanTool>(vw, &tools);
    vw->getMenu("&View")->addSeparator();

    // adds stretch tool button to toolpad on left
    Tool *stool = createTool<StretchTool>(vw, &tools);

    // adds find tool button to toolpad on left
    // adds to "Options" dropdown of Menu toolbar
    Tool *findTool = createTool<FindTool>(vw, &tools);

    // adds hist tool button to toolpad on left
    // adds PlotTool to "Options" dropdown of Menu toolbar
    Tool *histTool = createTool<HistogramTool>(vw, &tools);

    // adds stats tool button to toolpad on left
    Tool *statsTool = createTool<StatisticsTool>(vw, &tools);

    // adds tie tool button to toolpad on left
    // sets tie tool to active button
    tools.append(qnetTool);
    ((Tool *)qnetTool)->addTo(vw);
    qnetTool->activate(true);

    /**** ADD REMAINING TOOLS TO PERMANENT TOOL PAD ON TOP/BOTTOM ****/
    // adds adv track tool button permanent toolbar
    // adds to "Options" dropdown of Menu toolbar
    Tool *ttool = createTool<AdvancedTrackTool>(vw, &tools);

    // adds window tool (Link viewports) button and seperator to permanent toolbar
    // adds "Windows" dropdown of Menu toolbar
    Tool *wtool = createTool<WindowTool>(vw, &tools);
    vw->permanentToolBar()->addSeparator();

    // adds help tool button to permanent toolbar
    // adds "Help" dropdown of Menu toolbar
    Tool *htool = createTool<HelpTool>(vw, &tools);

    /**** MAKE CONNECTIONS ****/
    /**** LOADING IMAGES ****/
    // The fileTool needs to know when the navTool wants to load images
    QObject::connect(ntool, SIGNAL(loadImage(const QString &)),
                     ftool, SLOT(loadImage(const QString &)));
    // The fileTool needs to know when to load the images associated with a point
    QObject::connect(ntool,SIGNAL(loadPointImages(ControlPoint *)),
                     ftool,SLOT(loadPointImages(ControlPoint *)));

    /**** UPDATING LIST ****/
    // The navTool needs to know when the file tool has changed the serialNumberList
    QObject::connect(ftool, SIGNAL(serialNumberListUpdated()),
                     ntool, SLOT(resetList()));
    // The navTool cube name filter needs to know when the file tool
    // has changed the serialNumberList in order to refresh the list
    // Jeannie Walldren 2009-01-26
    QObject::connect(ftool, SIGNAL(serialNumberListUpdated()),
                     ntool, SLOT(resetCubeList()));
    // The QnetTool needs to know when the file tool has changed the
    // serialNumberList and the controlnetwork.
    //  QObject::connect(ftool,SIGNAL(serialNumberListUpdated()),
    //                   qnetTool,SLOT(updateList()));

    //  Connect file tools open ground & dem to QnetTool slots which will create
    //  FSB and open the files
    QObject::connect(ftool, SIGNAL(newGroundFile()), qnetTool, SLOT(openGround()));
    QObject::connect(ftool, SIGNAL(newDemFile()), qnetTool, SLOT(openDem()));

    /**** MAKE NAV TOOL DIALOG VISIBLE  ****/
    // NavigationTool needs to know when the QnetTool (Tie Tool) has been
    // acitvated on the tool pad in order to reopen the NavTool, if necessary.
    // 2010-07-01 Jeannie Walldren
    QObject::connect(qnetTool, SIGNAL(showNavTool()), ntool, SLOT(showNavTool()));

    /**** MODIFYING POINTS ****/
    // Qnet Tool needs to know when navTool modifies/ignores/deletes points
    QObject::connect(ntool, SIGNAL(modifyPoint(ControlPoint *)),
                     qnetTool, SLOT(modifyPoint(ControlPoint *)));
    QObject::connect(ntool, SIGNAL(ignoredPoints()), qnetTool, SLOT(refresh()));
    QObject::connect(ntool, SIGNAL(deletedPoints()), qnetTool, SLOT(refresh()));
    QObject::connect(ntool, SIGNAL(pointChanged(QString)),
                     qnetTool, SLOT(updatePointInfo(QString)));

    /**** RECORDING POINTS ****/
    // Connect the FindTool to the AdvancedTrackTool to record the point if the
    // "record" button is clicked in FindTool's dialog
    QObject::connect(findTool, SIGNAL(recordPoint(QPoint)),
                     ttool, SLOT(record(QPoint)));

    /**** UPDATING CONTROL NETWORK ****/
    // the next command was uncommented to allows the QnetTool to display the name
    // of the control network file and update when a new control network is selected
    // 2008-11-26 Jeannie Walldren
    QObject::connect(ftool, SIGNAL(controlNetworkUpdated(QString)),
                     qnetTool, SLOT(updateNet(QString)));
    QObject::connect(ftool, SIGNAL(newControlNetwork(ControlNet *)),
                     qnetTool, SIGNAL(newControlNetwork(ControlNet *)));
    QObject::connect(qnetTool, SIGNAL(editPointChanged(QString)),
                     ntool, SLOT(updateEditPoint(QString)));
    QObject::connect(qnetTool, SIGNAL(refreshNavList()),
                     ntool, SLOT(refreshList()));

    /**** SAVING CONTROL NET CHANGES ****/
    //  The FileTool needs to now if the control network has changed (delete/
    //  edit/create/ignore point) so that user can be prompted to save net
    QObject::connect(qnetTool, SIGNAL(netChanged()), ftool, SLOT(setDirty()));
    QObject::connect(ntool, SIGNAL(netChanged()), ftool, SLOT(setDirty()));
    QObject::connect(qnetTool, SIGNAL(qnetToolSave()), ftool, SLOT(save()));
    QObject::connect(qnetTool, SIGNAL(qnetToolSaveAs()), ftool, SLOT(saveAs()));

    // First hop for signal from StretchTool to ChipViewport
    QObject::connect(stool,
        SIGNAL(stretchChipViewport(Stretch *, CubeViewport *)),
        qnetTool,
        SIGNAL(stretchChipViewport(Stretch *, CubeViewport *)));


    /**** EXITING ****/
    // Connect the viewport's close signal to the file tool's exit method
    // Added 2008-12-04 by Jeannie Walldren
    // Added 2018-04-24 by Adam Goins - Added in optional parameters to the closeWindow() signal
    //                         And the exit() slot so that the window's closeEvent can be handled
    //                         Appropriately. Fixes #4146.
    QObject::connect(vw , SIGNAL(closeWindow(QCloseEvent *)),
                     ftool, SLOT(exit(QCloseEvent *)));
    //-----------------------------------------------------------------

    vw->show();
    int status = app->exec();
    delete ftool;
    ftool = NULL;
    delete ntool;
    ntool = NULL;
    delete btool;
    btool = NULL;
    delete ztool;
    ztool = NULL;
    delete ptool;
    ptool = NULL;
    delete stool;
    stool = NULL;
    delete findTool;
    findTool = NULL;
    delete histTool;
    histTool = NULL;
    delete statsTool;
    statsTool = NULL;
    delete qnetTool;
    qnetTool = NULL;
    delete ttool;
    ttool = NULL;
    delete wtool;
    wtool = NULL;
    delete htool;
    htool = NULL;
    delete rubberBandTool;
    rubberBandTool = NULL;
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
