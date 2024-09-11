/** This is free and unencumbered software released into the public domain.

The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */

#include "IpceMainWindow.h"

#include <QApplication>
#include <QBrush>
#include <QColor>
#include <QDebug>
#include <QDesktopWidget>
#include <QDockWidget>
#include <QMap>
#include <QMapIterator>
#include <QMdiArea>
#include <QObject>
#include <QRect>
#include <QRegExp>
#include <QStringList>
#include <QtWidgets>
#include <QSettings>
#include <QSize>
#include <QStatusBar>
#include <QStringList>
#include <QDateTime>
#include <QTreeView>
#include <QVariant>
#include <QTabWidget>


#include "AbstractProjectItemView.h"
#include "ControlHealthMonitorView.h"
#include "Directory.h"
#include "FileName.h"
#include "IException.h"
#include "IString.h"
#include "JigsawRunWidget.h"
#include "MosaicSceneWidget.h"
#include "ProgressWidget.h"
#include "Project.h"
#include "ProjectItemModel.h"
#include "ProjectItemTreeView.h"
#include "OpenProjectWorkOrder.h"
#include "SensorInfoWidget.h"
#include "TargetInfoWidget.h"
#include "TemplateEditorWidget.h"
#include "ViewSubWindow.h"

namespace Isis {
  /**
   * Construct the main window. This will create a Directory, the menus, and the dock areas.
   *
   * @param parent The Qt-relationship parent widget (usually NULL in this case)
   *
   * @internal
   *   @history 2016-11-09 Tyler Wilson - Moved the if-block which loads a project from the
   *                             command line from the start of the constructor to the end
   *                             because if there were warnings and errors, they were not
   *                             being output to the Warnings widget since the project is loaded
   *                             before the GUI is constructed.  Fixes #4488
   *   @history 2016-11-09 Ian Humphrey - Added default readSettings() call to load initial
   *                           default project window state. References #4358.
   */
  IpceMainWindow::IpceMainWindow(QWidget *parent) :
      QMainWindow(parent) {
    m_maxThreadCount = -1;

    QWidget *centralWidget = new QWidget;
    centralWidget->setAutoFillBackground(true);
    QPalette p = centralWidget->palette();
    p.setBrush(QPalette::Window, QBrush(Qt::Dense6Pattern));
    centralWidget->setPalette(p);
    setCentralWidget(centralWidget);

    setTabPosition(Qt::LeftDockWidgetArea, QTabWidget::South);

    // This was causing some buggy behavior, but this is what we would ultimately like.
    // Allows a user to undock a group of tabs.
    //setDockOptions(GroupedDragging | AllowTabbedDocks);

    setDockNestingEnabled(true);

    //  Set the splitter frames to a reasonable color/size for resizing the docks.
    setStyleSheet("QMainWindow::separator {background: black; width: 3; height: 3px;}");

    try {
      m_directory = new Directory(this);
      connect(m_directory, SIGNAL( newWidgetAvailable(QWidget *) ),
              this, SLOT( addView(QWidget *) ) );

      connect(m_directory, SIGNAL(closeView(QWidget *)),
              this, SLOT(removeView(QWidget *)));

      connect(m_directory, SIGNAL( directoryCleaned() ),
              this, SLOT( removeAllViews() ) );
      connect(m_directory->project(), SIGNAL(projectLoaded(Project *)),
              this, SLOT(readSettings(Project *)));
      connect(m_directory->project(), SIGNAL(projectSaved(Project *)),
              this, SLOT(writeSettings(Project *)));
      connect(m_directory, SIGNAL( newWarning() ),
              this, SLOT( raiseWarningTab() ) );
    }
    catch (IException &e) {
      throw IException(e, IException::Programmer,
          "Could not create Directory.", _FILEINFO_);
    }

    m_projectDock = new QDockWidget("Project", this, Qt::SubWindow);
    m_projectDock->setObjectName("projectDock");
    m_projectDock->setFeatures(QDockWidget::DockWidgetMovable |
                              QDockWidget::DockWidgetFloatable);
    m_projectDock->setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);

    ProjectItemTreeView *projectTreeView = m_directory->addProjectItemTreeView();
    projectTreeView->setInternalModel( m_directory->model() );
    projectTreeView->treeView()->expandAll();
    projectTreeView->installEventFilter(this);

    m_projectDock->setWidget(projectTreeView);
    addDockWidget(Qt::LeftDockWidgetArea, m_projectDock, Qt::Horizontal);

    m_warningsDock = new QDockWidget("Warnings", this, Qt::SubWindow);
    m_warningsDock->setObjectName("m_warningsDock");
    m_warningsDock->setFeatures(QDockWidget::DockWidgetClosable |
                         QDockWidget::DockWidgetMovable |
                         QDockWidget::DockWidgetFloatable);
    m_warningsDock->setWhatsThis(tr("This shows notices and warnings from all operations "
                          "on the current project."));
    m_warningsDock->setAllowedAreas(Qt::BottomDockWidgetArea);
    m_directory->setWarningContainer(m_warningsDock);
    addDockWidget(Qt::BottomDockWidgetArea, m_warningsDock);

    QDockWidget *historyDock = new QDockWidget("History", this, Qt::SubWindow);
    historyDock->setObjectName("historyDock");
    historyDock->setFeatures(QDockWidget::DockWidgetClosable |
                         QDockWidget::DockWidgetMovable |
                         QDockWidget::DockWidgetFloatable);
    historyDock->setWhatsThis(tr("This shows all operations performed on the current project."));
    historyDock->setAllowedAreas(Qt::BottomDockWidgetArea);
    m_directory->setHistoryContainer(historyDock);
    tabifyDockWidget(m_warningsDock, historyDock);

    historyDock->raise();

    setCorner(Qt::TopLeftCorner, Qt::LeftDockWidgetArea);
    setCorner(Qt::TopRightCorner, Qt::RightDockWidgetArea);
    setCorner(Qt::BottomLeftCorner, Qt::BottomDockWidgetArea);
    setCorner(Qt::BottomRightCorner, Qt::BottomDockWidgetArea);

    statusBar()->showMessage("Ready");
    statusBar()->addWidget(m_directory->project()->progress());

    foreach (QProgressBar *progressBar, m_directory->progressBars()) {
      statusBar()->addWidget(progressBar);
    }

    // Read default app settings.  NOTE: This must be completed before initializing actions in order
    // to read the recent projects from the config file.
    readSettings(m_directory->project() );

    initializeActions();
    createMenus();
    createToolBars();

    QCoreApplication::setApplicationName("ipce");
    QStringList args = QCoreApplication::arguments();

    if (args.count() == 2) {
      OpenProjectWorkOrder *workorder = new OpenProjectWorkOrder(m_directory->project());
      workorder->execute();
    }
  }


  /**
   * This is connected from Directory's newWidgetAvailable signal
   *
   * @param[in] newWidget (QWidget *)
   */
  void IpceMainWindow::addView(QWidget *newWidget, Qt::DockWidgetArea area,
                               Qt::Orientation orientation) {

    // JigsawRunWidget is already a QDockWidget, and no modifications need to be made to it
    if (qobject_cast<JigsawRunWidget *>(newWidget)) {
      splitDockWidget(m_projectDock, (QDockWidget*)newWidget, Qt::Vertical);

      // Save view docks for cleanup during a project close
      m_specialDocks.append((QDockWidget *)newWidget);
      return;
    }

    QDockWidget *dock = new QDockWidget(newWidget->windowTitle(), this);
    dock->setWidget(newWidget);
    dock->setObjectName(newWidget->objectName());
    dock->setAttribute(Qt::WA_DeleteOnClose);
    dock->setFeatures(QDockWidget::DockWidgetClosable | QDockWidget::DockWidgetMovable |
                      QDockWidget::DockWidgetFloatable);

    if ( qobject_cast<SensorInfoWidget *>(newWidget) ||
         qobject_cast<TargetInfoWidget *>(newWidget) ||
         qobject_cast<ControlHealthMonitorView *>(newWidget) ||
         qobject_cast<TemplateEditorWidget *>(newWidget)) {
      dock->setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);
      splitDockWidget(m_projectDock, dock, Qt::Vertical);

      // Save view docks for cleanup during a project close
      m_specialDocks.append(dock);
    }
    else {
      // Desired behavior of docking views:
      // First regular view (footprint,cubeDisplay) is added to the right of the "special" views
      // (project, sensor, jigsaw, controlHealth).  Adding additional "regular" views are tabbed
      // with the last "regular" view which was added.
      // The following logic not guaranteed to work as intended. If user moves one of the "special"
      // views such as sensor from below the project dock to the right of the project, the following
      // will put the new dock to the right of the moved "special" dock instead of tabbing.  The only
      // way to possibly ensure the intended functionality would be to check for the position of the
      // last added dock and either add or tabify depending on location.  This might also allow the
      // docks to be kept in a single list instead of m_specialDocks & m_viewDocks.
      if (m_viewDocks.count() == 0) {
        addDockWidget(Qt::RightDockWidgetArea, dock, Qt::Horizontal);
      }
      else {
        tabifyDockWidget(m_viewDocks.last(), dock);
        dock->show();
        dock->raise();
      }

      // Save view docks for cleanup during a project close
      m_viewDocks.append(dock);
    }

    // When dock widget is destroyed, make sure the view it holds is also destroyed
    connect(dock, SIGNAL(destroyed(QObject *)), newWidget, SLOT(deleteLater()));
    // The list of dock widgets needs cleanup as each view is destroyed
    connect(dock, SIGNAL(destroyed(QObject *)), this, SLOT(cleanupViewDockList(QObject *)));

    // Only emit the signal when one view is added just for simplicity; behavior would not change
    // if this was emitted after every addition.
    if (m_viewDocks.size() == 1) {
      emit enableViewActions(true);
    }
  }


  /**
   * Cleans up m_viewDocks when a view is closed (object is destroyed).
   *
   * @param view QObject* The dock widget to remove from the m_viewDocks
   */
  void IpceMainWindow::cleanupViewDockList(QObject *obj) {

    QDockWidget *dock = static_cast<QDockWidget *>(obj);
    if (dock) {
      m_viewDocks.removeAll(dock);
      m_specialDocks.removeAll(dock);
    }

    if (m_viewDocks.size() == 0) {
      emit enableViewActions(false);
    }
  }


  /**
   * This slot is connected from Directory::closeView(QWidget *) signal.  It will close the given
   * view and delete the view.
   *
   * @param view QWidget* The view to close.
   */
  void IpceMainWindow::removeView(QWidget *view) {

    QDockWidget *parentDock = qobject_cast<QDockWidget *>(view->parent());
    removeDockWidget(parentDock);
    delete parentDock;
  }


  /**
   * Removes All Views in main window, connected to directory signal directoryCleaned()
   */
  void IpceMainWindow::removeAllViews() {
    setWindowTitle("ipce");
    foreach (QDockWidget *dock, m_viewDocks) {
      if (dock) {
        removeDockWidget(dock);
        delete dock;
      }
    }

    foreach (QDockWidget *dock, m_specialDocks) {
      if (dock) {
        removeDockWidget(dock);
        m_specialDocks.removeAll(dock);
        delete dock;
      }
    }
    m_viewDocks.clear();
    m_specialDocks.clear();
    emit enableViewActions(false);
}


  /**
   * Cleans up the directory.
   */
  IpceMainWindow::~IpceMainWindow() {
    m_directory->deleteLater();
  }


  /**
   * This is needed so that the project clean flag is not set to false when move and resize events
   * are emitted from ipce.cpp when IpceMainWindow::show() is called.
   * The non-spontaneous or internal QShowEvent is only emitted once from ipce.cpp, so the project
   * clean flag can be reset.
   *
   * @param event QShowEvent*
   *
   */
  void IpceMainWindow::showEvent(QShowEvent *event) {
    if (!event->spontaneous()) {
      m_directory->project()->setClean(true);
    }
  }


  /**
   * Filters out events from views so they can be handled by the main
   * window. Filters out DragEnter Drop and ContextMenu events from
   * views.
   *
   * @param[in] watched (QObject *) The object being filtered.
   * @param[in] event (QEvent *) The event that may be filtered.
   */
  bool IpceMainWindow::eventFilter(QObject *watched, QEvent *event) {
    if ( AbstractProjectItemView *view = qobject_cast<AbstractProjectItemView *>(watched) ) {
      if (event->type() == QEvent::DragEnter) {
        return true;
      }
      else if (event->type() == QEvent::Drop) {
        return true;
      }
      else if (event->type() == QEvent::ContextMenu) {
        QMenu contextMenu;

        QList<QAction *> viewActions = view->contextMenuActions();

        if ( !viewActions.isEmpty() ) {
          foreach (QAction *action, viewActions) {
            if (action) {
              contextMenu.addAction(action);
            }
            else {
              contextMenu.addSeparator();
            }
          }
          contextMenu.addSeparator();
        }

        QList<QAction *> workOrders = m_directory->supportedActions( view->currentItem() );

        if ( !workOrders.isEmpty() ) {
          foreach (QAction *action, workOrders) {
            contextMenu.addAction(action);
          }
          contextMenu.addSeparator();
        }

        contextMenu.exec( static_cast<QContextMenuEvent *>(event)->globalPos() );

        return true;
      }
    }

    return QMainWindow::eventFilter(watched, event);
  }


  /**
   * This method takes the max thread count setting and asks
   * QtConcurrent to respect it.
   */
  void IpceMainWindow::applyMaxThreadCount() {
    if (m_maxThreadCount <= 1) {
      // Allow QtConcurrent to use every core and starve the GUI thread
      QThreadPool::globalInstance()->setMaxThreadCount(QThread::idealThreadCount());
    }
    else {
      // subtract 1 to account for the GUI thread
      QThreadPool::globalInstance()->setMaxThreadCount(m_maxThreadCount - 1);
    }
  }


  /**
   * Initializes the internal lists of actions of the main window for
   * use in the menus and toolbars.
   */
  void IpceMainWindow::initializeActions() {
    QAction *exitAction = new QAction("E&xit", this);
    exitAction->setIcon( QIcon::fromTheme("window-close") );
    connect(exitAction, SIGNAL(triggered()), this, SLOT(close()));
    m_fileMenuActions.append(exitAction);
    m_permToolBarActions.append(exitAction);

    QAction *tabViewsAction = new QAction("Tab Views", this);
    connect( tabViewsAction, SIGNAL(triggered()), this, SLOT(tabViews()) );
    connect( this, SIGNAL(enableViewActions(bool)), tabViewsAction, SLOT(setEnabled(bool)) );
    m_viewMenuActions.append(tabViewsAction);
    tabViewsAction->setDisabled(true);  // Disabled on default, until a view is added

    QAction *tileViewsAction = new QAction("Tile Views", this);
    connect( tileViewsAction, SIGNAL(triggered()), this, SLOT(tileViews()) );
    connect( this, SIGNAL(enableViewActions(bool)), tileViewsAction, SLOT(setEnabled(bool)) );
    m_viewMenuActions.append(tileViewsAction);
    tileViewsAction->setDisabled(true); // Disabled on default, until a view is added

    QAction *undoAction = m_directory->undoAction();
    undoAction->setShortcut(Qt::Key_Z | Qt::CTRL);

    QAction *redoAction = m_directory->redoAction();
    redoAction->setShortcut(Qt::Key_Z | Qt::CTRL | Qt::SHIFT);

    m_editMenuActions.append(undoAction);
    m_editMenuActions.append(redoAction);

    QAction *threadLimitAction = new QAction("Set Thread &Limit", this);
    connect(threadLimitAction, SIGNAL(triggered()),
            this, SLOT(configureThreadLimit()));

    m_settingsMenuActions.append(m_directory->project()->userPreferenceActions());
    m_settingsMenuActions.append(threadLimitAction);

    QAction *activateWhatsThisAct = new QAction("&What's This", this);
    activateWhatsThisAct->setShortcut(Qt::SHIFT | Qt::Key_F1);
    activateWhatsThisAct->setIcon(
        QPixmap(QString::fromStdString(FileName("$ISISROOT/appdata/images/icons/contexthelp.png").expanded())));
    activateWhatsThisAct->setToolTip("Activate What's This and click on parts "
        "this program to see more information about them");
    connect(activateWhatsThisAct, SIGNAL(triggered()), this, SLOT(enterWhatsThisMode()));

    m_helpMenuActions.append(activateWhatsThisAct);
  }


  /**
   * Creates and fills the application menus of the menu bar.
   */
  void IpceMainWindow::createMenus() {

    m_fileMenu = menuBar()->addMenu(tr("&File"));
    m_fileMenu->setObjectName("fileMenu");
    // Get Directory FileMenu actions
    foreach ( QAction *action, m_directory->fileMenuActions() ) {
      m_fileMenu->addAction(action);
    }
    m_fileMenu->addSeparator();
    // Get FileMenu actions from the ipceMainWindow, Exit is the only action
    foreach ( QAction *action, m_fileMenuActions ) {
      m_fileMenu->addAction(action);
    }

    m_projectMenu = menuBar()->addMenu(tr("&Project"));
    m_projectMenu->setObjectName("projectMenu");
    //  Get Project menu actions from Directory
    foreach ( QAction *action, m_directory->projectMenuActions() ) {
      m_projectMenu->addAction(action);
    }
    // Allow tool tips to be displayed for the project menu's actions (e.g. "Bundle Adjustment")
    // This is a work around for Qt's what this text not working on disabled actions
    // (even though the Qt documentation says it should work on disabled QAction's).
    m_projectMenu->setToolTipsVisible(true);

    m_editMenu = menuBar()->addMenu(tr("&Edit"));
    m_editMenu->setObjectName("editMenu");
    m_editMenu->addSeparator();
    // Get Edit menu actions from Directory
    foreach ( QAction *action, m_directory->editMenuActions() ) {
      m_editMenu->addAction(action);
    }
    // Get Edit menu actions from IpceMainWindow
    foreach ( QAction *action, m_editMenuActions ) {
      m_editMenu->addAction(action);
    }

    m_viewMenu = menuBar()->addMenu("&View");
    m_viewMenu->setObjectName("viewMenu");
    // Get View menu actions from Directory
    foreach ( QAction *action, m_directory->viewMenuActions() ) {
      m_viewMenu->addAction(action);
    }
    m_viewMenu->addSeparator();
    // Get View menu actions from IpceMainWindow
    foreach ( QAction *action, m_viewMenuActions ) {
      m_viewMenu->addAction(action);
    }

    m_settingsMenu = menuBar()->addMenu("&Settings");
    m_settingsMenu->setObjectName("settingsMenu");
    // Get Settings menu actions from Directory
    foreach ( QAction *action, m_directory->settingsMenuActions() ) {
      m_settingsMenu->addAction(action);
    }
    m_settingsMenu->addSeparator();
    // Get Settings menu actions from IpceMainWindow
    foreach ( QAction *action, m_settingsMenuActions ) {
      m_settingsMenu->addAction(action);
    }

    m_helpMenu = menuBar()->addMenu("&Help");
    m_helpMenu->setObjectName("helpMenu");
    // Get Help menu actions from Directory
    foreach ( QAction *action, m_directory->helpMenuActions() ) {
      m_helpMenu->addAction(action);
    }
    m_helpMenu->addSeparator();
    // Get Help menu actions from IpceMainWindow
    foreach ( QAction *action, m_helpMenuActions ) {
      m_helpMenu->addAction(action);
    }
  }


  /**
   * Create the tool bars and populates them with QActions from several sources. Actions are taken
   * from an internal list of QActions and the Directory.
   */
  void IpceMainWindow::createToolBars() {
    m_permToolBar = new QToolBar(this);
    QSize iconSize(25, 45);
    m_permToolBar->setIconSize(iconSize);
    m_permToolBar->setObjectName("PermanentToolBar");
    addToolBar(m_permToolBar);

    foreach ( QAction *action, m_directory->permToolBarActions() ) {
      m_permToolBar->addAction(action);
    }

    foreach (QAction *action, m_permToolBarActions) {
      m_permToolBar->addAction(action);
    }
  }


  /**
   * Writes the global settings like recent projects and thread count.
   */
  void IpceMainWindow::writeGlobalSettings(Project *project) {

    QString appName = QApplication::applicationName();

    QSettings globalSettings(QString::fromStdString(FileName("$HOME/.Isis/" + appName.toStdString() + "/ipce.config").expanded()),
        QSettings::NativeFormat);

    // If no config file exists and a user immediately opens a project,
    // the project's geometry will be saved as a default for when ipce is
    // opened again. Previously, the ipce's default size was small,
    // until a user opened ipce (but not a project) and resized to how they
    // wanted it to be sized, then closed ipce.
    if (project->isTemporaryProject() || !globalSettings.contains("geometry")) {
      globalSettings.setValue("geometry", QVariant(geometry()));
    }

    globalSettings.setValue("maxThreadCount", m_maxThreadCount);
    globalSettings.setValue("maxRecentProjects",m_maxRecentProjects);

    globalSettings.beginGroup("recent_projects");
    QStringList keys = globalSettings.allKeys();
    QMap<QString,QString> recentProjects;

    foreach (QString key,keys) {
      recentProjects[key]=globalSettings.value(key).toString();
    }

    QList<QString> projectPaths = recentProjects.values();

    if (keys.count() >= m_maxRecentProjects) {

      //Clear out the recent projects before repopulating this group
      globalSettings.remove("");

      //If the currently open project is a project that has been saved and is not within the current
      //list of recently open projects, then remove the oldest project from the list.
      if (!project->projectRoot().contains("tmpProject") &&
          !projectPaths.contains(project->projectRoot()) ) {
        QString s=keys.first();
        recentProjects.remove( s );
      }

      //If the currently open project is already contained within the list,
      //then remove the earlier reference.
      if (projectPaths.contains(project->projectRoot())) {
        QString key = recentProjects.key(project->projectRoot());
        recentProjects.remove(key);

      }

      QMap<QString,QString>::iterator i;

      //Iterate through the recentProjects QMap and set the <key,val> pairs.
      for (i=recentProjects.begin();i!=recentProjects.end();i++) {
          globalSettings.setValue(i.key(),i.value());
      }

      //Get a unique time value for generating a key
      long t0 = QDateTime::currentMSecsSinceEpoch();

      QString projName = project->name();
      QString t0String=QString::number(t0);

      //Save the project location
      if (!project->projectRoot().contains("tmpProject") ) {
              globalSettings.setValue(t0String+"%%%%%"+projName,project->projectRoot());
      }
    }

    //The numer of recent open projects is less than m_maxRecentProjects
    else {

      long t0 = QDateTime::currentMSecsSinceEpoch();
      QString projName = project->name();
      QString t0String=QString::number(t0);

      if (!project->isTemporaryProject() &&
          !projectPaths.contains( project->projectRoot())) {
        globalSettings.setValue(t0String+"%%%%%"+projName,project->projectRoot());
      }
    }
    globalSettings.endGroup();
    globalSettings.sync();
  }


  /**
   * Write the window positioning and state information out to a
   * config file. This allows us to restore the settings when we
   * create another main window (the next time this program is run).
   *
   * The state will be saved according to the currently loaded project and its name.
   *
   * When no project is loaded (i.e. the default "Project" is open), the config file used is
   * $HOME/.Isis/$APPNAME/ipce.config.
   * When a project, ProjectName, is loaded, the config file used is
   * project->projectRoot()/ipce.config.
   *
   * @param[in] project Pointer to the project that is currently loaded (default is "Project")
   *
   * @internal
   *   @history 2016-11-09 Ian Humphrey - Settings are now written according to the loaded project.
   *                           References #4358.
   *   @history 2017-10-17 Tyler Wilson Added a [recent projects] group for the saving and
   *                           restoring of recently opened projects.  References #4492.
   *   @history Kaitlyn Lee 2018-07-09 - Added the value "maximized" in the project settings
   *                           so that a project remembers if it was in fullscreen when saved.
   *                           Fixes #5175.
   */
  void IpceMainWindow::writeSettings(Project *project) {

    // Ensure that we are not using a NULL pointer
    if (!project) {
      std::string msg = "Cannot write settings with a NULL Project pointer.";
      throw IException(IException::Programmer, msg, _FILEINFO_);
    }
    QSettings projectSettings(QString::fromStdString(FileName(project->newProjectRoot().toStdString() + "/ipce.config").expanded()),
        QSettings::NativeFormat);

    projectSettings.setValue("geometry", QVariant(geometry()));
    projectSettings.setValue("windowState", saveState());
    projectSettings.setValue("maximized", isMaximized());
    projectSettings.sync();
  }


  /**
   * Read the window positioning and state information from the config file.
   *
   * When running ipce without opening a project, the config file read is
   * $HOME/.Isis/$APPNAME/ipce.config
   * When running ipce and opening a project (ProjectName), the config file read is
   * project->projectRoot()/ipce.config
   *
   * @param[in] project (Project *) The project that was loaded.
   *
   * @internal
   *   @history Ian Humphrey - Settings are now read on a project name basis. References #4358.
   *   @history Tyler Wilson 2017-11-02 - Settings now read recent projects.  References #4492.
   *   @history Tyler Wilson 2017-11-13 - Commented out a resize call near the end because it
   *                was messing with the positions of widgets after a project was loaded.
   *                Fixes #5075.
   *   @history Makayla Shepherd 2018-06-10 - Settings are read from the project root ipce.config.
   *                If that does not exist then we read from .Isis/ipce/ipce.config.
   *   @history Kaitlyn Lee 2018-07-09 - Added the call showNormal() so when a project is
   *                not saved in fullscreen, the window will resize to the project's
   *                window size. This also fixes the history/warning tabs being misplaced
   *                when opening a project. Fixes #5175.
   */
  void IpceMainWindow::readSettings(Project *project) {
    // Ensure that the Project pointer is not NULL
    if (!project) {
      std::string msg = "Cannot read settings with a NULL Project pointer.";
      throw IException(IException::Programmer, msg, _FILEINFO_);
    }

    // Set the path of the settings file
    // The default is to assume that the project has an ipce.config in it
    // If the file does not exist then we read settings from .Isis/ipce/ipce.config
    QString appName = QApplication::applicationName();
    QString filePath = project->projectRoot() + "/ipce.config";
    bool isFullScreen = false;
    if (!FileName(filePath.toStdString()).fileExists()) {
      filePath = "$HOME/.Isis/" + appName + "/ipce.config";
      // If the $HOME/.Isis/ipce/ipce.config does not exist then we want ipce to show up in
      // in full screen. In other words the default geometry is full screen
      if (!FileName(filePath.toStdString()).fileExists()) {
        isFullScreen = true;
      }
    }

    if (project->name() == "Project") {
      setWindowTitle("ipce");
    }
    else {
      setWindowTitle( project->name() );
    }

    QSettings projectSettings(QString::fromStdString(FileName(filePath.toStdString()).expanded()), QSettings::NativeFormat);

    if (!isFullScreen) {
      // If a project was not in fullscreen when saved, restore the project's window size.
      if (!projectSettings.value("maximized").toBool()) {
        showNormal();
      }
      setGeometry(projectSettings.value("geometry").value<QRect>());

      if (!project->isTemporaryProject()) {
        restoreState(projectSettings.value("windowState").toByteArray());
      }
    }
    else {
      this->showMaximized();
    }

    if (project->name() == "Project") {
      QSettings globalSettings(QString::fromStdString(FileName("$HOME/.Isis/" + appName.toStdString() + "/ipce.config").expanded()),
                              QSettings::NativeFormat);

      QStringList projectNameList;
      QStringList projectPathList;
      globalSettings.beginGroup("recent_projects");
      QStringList keys = globalSettings.allKeys();
      QRegExp underscore("%%%%%");

      foreach (QString key, keys) {
        QString childKey = "recent_projects/"+key;
        QString projectPath = globalSettings.value(key).toString();
        QString projectName = projectPath.split("/").last();
        projectPathList.append(projectPath) ;
        projectNameList.append(projectName);
      }

      globalSettings.endGroup();

      QStringList projectPathReverseList;
      for (int i = projectPathList.count() - 1; i >= 0; i--) {
        projectPathReverseList.append(projectPathList[i]);
      }

      QStringList projectPathListTruncated;

      int i =0;

      foreach (QString proj,projectPathReverseList) {
        if (i <= m_maxRecentProjects) {
          projectPathListTruncated.append(proj);
          i++;
        }
        else
          break;
        }

      m_directory->setRecentProjectsList(projectPathListTruncated);
      m_directory->updateRecentProjects();
      m_maxThreadCount = globalSettings.value("maxThreadCount", m_maxThreadCount).toInt();
      applyMaxThreadCount();
    }

    m_directory->project()->setClean(true);
  }


  /**
   * Handle the close event by writing the window positioning and
   * state information before forwarding the event to the QMainWindow.
   */
  void IpceMainWindow::closeEvent(QCloseEvent *event) {

    foreach(TemplateEditorWidget *templateEditor, m_directory->templateEditorViews()) {
      templateEditor->saveOption();
    }
    // The active control is checked here for modification because this was the simplest solution
    // vs changing the project clean state every time the control is modified or saved.
    if (!m_directory->project()->isClean() || (m_directory->project()->activeControl() &&
                                               m_directory->project()->activeControl()->isModified())) {
      QMessageBox *box = new QMessageBox(QMessageBox::NoIcon, QString("Current Project Has Unsaved Changes"),
                             QString("Would you like to save your current project?"),
                             QMessageBox::NoButton, qobject_cast<QWidget *>(parent()), Qt::Dialog);
      QPushButton *save = box->addButton("Save", QMessageBox::AcceptRole);
      box->addButton("Don't Save", QMessageBox::RejectRole);
      QPushButton *cancel = box->addButton("Cancel", QMessageBox::NoRole);
      box->exec();

      if (box->clickedButton() == (QAbstractButton*)cancel) {
        event->ignore();
        return;
      }
      else if (box->clickedButton() == (QAbstractButton*)save) {
        m_directory->project()->save();
      }
    }
    //  Write global settings, for now this is for the project "Project"
    writeGlobalSettings(m_directory->project());
    m_directory->project()->clear();

    QMainWindow::closeEvent(event);
  }


  /**
   * Ask the user how many threads to use in this program. This
   * includes the GUI thread.
   */
  void IpceMainWindow::configureThreadLimit() {
    bool ok = false;

    QStringList options;

    int current = 0;
    options << tr("Use all available");

    for(int i = 1; i < 24; i++) {
      QString option = tr("Use %1 threads").arg(i + 1);

      options << option;
      if(m_maxThreadCount == i + 1)
        current = i;
    }

    QString res = QInputDialog::getItem(NULL, tr("Concurrency"),
        tr("Set the number of threads to use"),
        options, current, false, &ok);

    if (ok) {
      m_maxThreadCount = options.indexOf(res) + 1;

      if (m_maxThreadCount <= 1)
        m_maxThreadCount = -1;

      applyMaxThreadCount();
    }
  }


  /**
   * Activate the What's This? cursor. This is useful for he What's
   * This? action in the help menu.
   */
  void IpceMainWindow::enterWhatsThisMode() {
    QWhatsThis::enterWhatsThisMode();
  }


  /**
   * Tabs all open attached/detached views
   */
  void IpceMainWindow::tabViews() {
    // tabifyDockWidget() takes two widgets and tabs them, so an easy way to do
    // this is to grab the first view and tab the rest with the first.
    QDockWidget *firstView = m_viewDocks.first();

    foreach (QDockWidget *currentView, m_viewDocks) {
      // We have to reattach a view before it can be tabbed. If it is attached,
      // this will have no affect.
      currentView->setFloating(false);

      if (currentView == firstView) {
        continue;
      }
      tabifyDockWidget(firstView, currentView);
    }
  }


  /**
   * Tile all open attached/detached views
   */
  void IpceMainWindow::tileViews() {
    // splitDockWidget() takes two widgets and tiles them, so an easy way to do
    // this is to grab the first view and tile the rest with the first.
    QDockWidget *firstView = m_viewDocks.first();

    foreach (QDockWidget *currentView, m_viewDocks) {
      // We have to reattach a view before it can be tiled. If it is attached,
      // this will have no affect. We have to call addDockWidget() to untab any views.
      currentView->setFloating(false);
      addDockWidget(Qt::RightDockWidgetArea, currentView, Qt::Horizontal);

      if (currentView == firstView) {
        continue;
      }
      splitDockWidget(firstView, currentView, Qt::Horizontal);
    }
  }


/**
 * Raises the warningWidget to the front of the tabs. Connected to warning signal from directory.
 */
  void IpceMainWindow::raiseWarningTab() {
    m_warningsDock->raise();
  }
}
