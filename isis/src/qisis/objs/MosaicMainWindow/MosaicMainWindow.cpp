#include "MosaicMainWindow.h"

#include <QApplication>
#include <QDockWidget>
#include <QHBoxLayout>
#include <QLabel>
#include <QMenu>
#include <QMenuBar>
#include <QMessageBox>
#include <QProgressBar>
#include <QPushButton>
#include <QSettings>
#include <QScrollArea>
#include <QStatusBar>
#include <QVBoxLayout>
#include <QWhatsThis>

#include "Camera.h"
#include "FileDialog.h"
#include "MosaicController.h"
#include "ImageFileListWidget.h"
#include "ImageTreeWidgetItem.h"
#include "IString.h"
#include "MosaicSceneWidget.h"
#include "Projection.h"
#include "ProjectionFactory.h"
#include "TextFile.h"
#include "ToolPad.h"


namespace Isis {
  MosaicMainWindow::MosaicMainWindow(QString title, QWidget *parent) :
      MainWindow(title, parent),
      m_settings(QString::fromStdString(FileName("$HOME/.Isis/qmos/qmos.config").expanded()),
                 QSettings::NativeFormat) {
    m_filename = "";
    m_fileMenu = NULL;
    m_settingsMenu = NULL;
    m_viewMenu = NULL;

    setObjectName("MosaicMainWindow");

    m_controllerVisible = false;

    setWindowTitle(title);

    m_permToolbar = new QToolBar("Standard Tools", this);
    m_permToolbar->setObjectName("Standard Tools");
    m_permToolbar->setWhatsThis("This area contains options that are always "
        "present in qmos, regardless of whether or not a project is open. "
        "These options are also found in the File menu.");
    addToolBar(m_permToolbar);

    m_activeToolbar = new QToolBar("Active Tool", this);
    m_activeToolbar->setObjectName("Active Tool");
    m_activeToolbar->setWhatsThis("The currently selected tool's options will "
        "show up here. Not all tools have options.");
    addToolBar(m_activeToolbar);

    statusBar()->showMessage("Ready");

    m_toolpad = new ToolPad("Tool Pad", this);
    m_toolpad->setObjectName("Tool Pad");
    // default to the right hand side for qview-like behavior... we might
    //   want to do something different here
    addToolBar(Qt::RightToolBarArea, m_toolpad);

    setupMenus();

    m_fileListDock = new QDockWidget("File List", this, Qt::SubWindow);
    m_fileListDock->setObjectName("FileListDock");
    m_fileListDock->setFeatures(QDockWidget::DockWidgetFloatable |
                                QDockWidget::DockWidgetMovable |
                                QDockWidget::DockWidgetClosable);
    m_fileListDock->setWhatsThis("This contains the mosaic file list.");

    m_mosaicPreviewDock = new QDockWidget("Mosaic World View",
                                          this, Qt::SubWindow);
    m_mosaicPreviewDock->setObjectName("MosaicPreviewDock");
    m_mosaicPreviewDock->setFeatures(QDockWidget::DockWidgetFloatable |
                                     QDockWidget::DockWidgetMovable |
                                     QDockWidget::DockWidgetClosable);
    m_mosaicPreviewDock->setWhatsThis("This contains a zoomed out view of the "
        "mosaic.");

    addDockWidget(Qt::LeftDockWidgetArea, m_fileListDock);
    addDockWidget(Qt::LeftDockWidgetArea, m_mosaicPreviewDock);

    readSettings();

    setCentralWidget(new QWidget());
    centralWidget()->setLayout(new QHBoxLayout());

    m_mosaicController = NULL;
    createController();
    displayController();
    installEventFilter(this);

    QStringList args = QApplication::arguments();
    args.removeFirst();

    QStringList filesToOpen;
    bool projectLoaded = false;

    foreach (QString argument, args) {
      QRegExp cubeName(".*\\.cub$", Qt::CaseInsensitive);
      QRegExp cubeListName(".*\\.(lis|txt)$", Qt::CaseInsensitive);
      QRegExp projectName(".*\\.mos$", Qt::CaseInsensitive);

      try {
        if (cubeName.exactMatch(argument)) {
          filesToOpen.append(argument);
        }
        else if (cubeListName.exactMatch(argument)) {
          TextFile fileList(argument);
          QString line;

          while(fileList.GetLine(line)) {
            filesToOpen.append(line);
          }
        }
        else if (projectName.exactMatch(argument)) {
          if (!projectLoaded) {
            loadProject(argument);
            projectLoaded = true;
          }
          else {
            QMessageBox::warning(this, "Multiple Projects Specified",
                "qmos can only open one project at a time. The first project "
                "specified is the one that will be used.");
          }
        }
      }
      catch (IException &e) {
        QMessageBox::warning(this, "Problem Loading File", e.what());
      }
    }

    m_lastOpenedFile = QFileInfo(".");

    if (!filesToOpen.isEmpty())
      openFiles(filesToOpen);
  }


  /**
   * This event filter is installed in the constructor.
   * @param o
   * @param e
   *
   * @return bool
   */
  bool MosaicMainWindow::eventFilter(QObject *o, QEvent *e) {
    switch(e->type()) {
      case QEvent::Close:
        closeMosaic();

      default:
        return false;
    }
  }


  /**
   * Sets up the menus on the menu bar for the qmos window.
   *
   * This should be the job of the widgets this application uses... inheritance
   *   off of something that says it has menu options probably. This should
   *   probably have open project, save project, close project, and exit.
   *   Projects need to be an accumulated file also, if we want them.
   */
  void MosaicMainWindow::setupMenus() {
    // Create the file menu
    m_fileMenu = menuBar()->addMenu("&File");

    IString iconDir = FileName("$ISISROOT/appdata/images/icons").expanded();

    QAction *open = new QAction(this);
    open->setText("Open Cube...");
    open->setIcon(QPixmap(QString::fromStdString(iconDir.c_str()) + "/fileopen.png"));
    connect(open, SIGNAL(triggered()), this, SLOT(open()));

    QAction *openList = new QAction(this);
    openList->setText("Open Cube List...");
    openList->setIcon(QPixmap(QString::fromStdString(iconDir.c_str()) + "/mActionHelpContents.png"));
    connect(openList, SIGNAL(triggered()), this, SLOT(openList()));

    QAction *saveProject = new QAction(this);
    saveProject->setText("Save Project");
    saveProject->setShortcut(Qt::CTRL + Qt::Key_S);
    saveProject->setIcon(QPixmap(QString::fromStdString(iconDir.c_str()) + "/mActionFileSave.png"));
    m_actionsRequiringOpen.append(saveProject);
    connect(saveProject, SIGNAL(triggered()), this, SLOT(saveProject()));

    QAction *saveProjectAs = new QAction(this);
    saveProjectAs->setText("Save Project As...");
    saveProjectAs->setIcon(QPixmap(QString::fromStdString(iconDir.c_str()) + "/mActionFileSaveAs.png"));
    m_actionsRequiringOpen.append(saveProjectAs);
    connect(saveProjectAs, SIGNAL(triggered()), this, SLOT(saveProjectAs()));

    QAction *loadProject = new QAction(this);
    loadProject->setText("Load Project...");
    loadProject->setIcon(QPixmap(QString::fromStdString(iconDir.c_str()) + "/mActionExportMapServer.png"));
    connect(loadProject, SIGNAL(triggered()), this, SLOT(loadProject()));

    QAction *closeProject = new QAction(this);
    closeProject->setText("Close Project");
    m_actionsRequiringOpen.append(closeProject);
    connect(closeProject, SIGNAL(triggered()), this, SLOT(closeMosaic()));

    QAction *exit = new QAction(this);
    exit->setText("Exit");
    exit->setIcon(QIcon::fromTheme("window-close"));
    connect(exit, SIGNAL(triggered()), this, SLOT(close()));

    QAction *actionRequiringOpen = NULL;
    foreach(actionRequiringOpen, m_actionsRequiringOpen) {
      actionRequiringOpen->setEnabled(false);
    }

    QAction *actionRequiringClosed = NULL;
    foreach(actionRequiringClosed, m_actionsRequiringClosed) {
      actionRequiringClosed->setEnabled(true);
    }

    m_fileMenu->addAction(open);
    m_fileMenu->addAction(openList);
    m_fileMenu->addSeparator();
    m_fileMenu->addAction(loadProject);
    m_fileMenu->addAction(saveProject);
    m_fileMenu->addAction(saveProjectAs);
    m_fileMenu->addAction(closeProject);
    m_fileMenu->addSeparator();
    m_exportMenu = m_fileMenu->addMenu("&Export");
    m_fileMenu->addAction(exit);

    permanentToolBar()->addAction(loadProject);
    permanentToolBar()->addAction(saveProject);
    permanentToolBar()->addAction(saveProjectAs);
    permanentToolBar()->addSeparator();
    permanentToolBar()->addAction(open);
    permanentToolBar()->addAction(openList);
    permanentToolBar()->addSeparator();

    m_viewMenu = menuBar()->addMenu("&View");
    m_settingsMenu = menuBar()->addMenu("&Settings");
    QMenu *helpMenu = menuBar()->addMenu("&Help");

    QAction *activateWhatsThisAct = new QAction("&What's This", this);
    activateWhatsThisAct->setShortcut(Qt::SHIFT | Qt::Key_F1);
    activateWhatsThisAct->setIcon(
        QPixmap(QString::fromStdString(FileName("$ISISROOT/appdata/images/icons/contexthelp.png").expanded())));
    activateWhatsThisAct->setToolTip("Activate What's This and click on parts "
        "this program to see more information about them");
    connect(activateWhatsThisAct, SIGNAL(triggered()),
            this, SLOT(enterWhatsThisMode()));

    QAction *showHelpAct = new QAction("qmos &Help", this);
    showHelpAct->setIcon(QIcon::fromTheme("help-contents"));
    connect(showHelpAct, SIGNAL(triggered()),
            this, SLOT(showHelp()));

    helpMenu->addAction(activateWhatsThisAct);
    helpMenu->addAction(showHelpAct);

    updateMenuVisibility();
  }


  /**
   * Calles MosaicWidget's open method which opens a cube file and
   * displays the footprint in the graphics view.
   *
   */
  void MosaicMainWindow::open() {
    QStringList filterList;
    filterList.append("Isis cubes (*.cub)");
    filterList.append("All Files (*)");

    QDir directory = m_lastOpenedFile.dir();
    QStringList selected = QFileDialog::getOpenFileNames(this, "Open Cubes",
        directory.path(), filterList.join(";;"));

    if (!selected.empty()) {
      m_lastOpenedFile = QFileInfo(selected.last());
      openFiles(selected);
    }
  }


  void MosaicMainWindow::enterWhatsThisMode() {
    QWhatsThis::enterWhatsThisMode();
  }


  void MosaicMainWindow::showHelp() {
    QDialog *helpDialog = new QDialog(this);

    QVBoxLayout *mainLayout = new QVBoxLayout;
    helpDialog->setLayout(mainLayout);

    // Let's add some text
    QLabel *qmosTitle = new QLabel("<h1>qmos</h1>");
//     qmosTitle->setMinimumSize(QSize(800, qmosTitle->minimumSize().height()));
    mainLayout->addWidget(qmosTitle);

    QLabel *qmosSubtitle = new QLabel("A tool for visualizing image "
                                      "footprints for a mosaic.");
    mainLayout->addWidget(qmosSubtitle);

    QTabWidget *tabArea = new QTabWidget;
    mainLayout->addWidget(tabArea);

    QScrollArea *overviewTab = new QScrollArea;

    QWidget *overviewContainer = new QWidget;

    QVBoxLayout *overviewLayout = new QVBoxLayout;
    overviewContainer->setLayout(overviewLayout);

    QLabel *purposeTitle = new QLabel("<h2>Purpose</h2>");
    overviewLayout->addWidget(purposeTitle);

    QLabel *purposeText = new QLabel("<p>qmos is designed "
        "specifically for visualizing large amounts of images, how images "
        "overlap, where control points lie on the images, and how jigsaw has "
        "moved control points.");
    purposeText->setWordWrap(true);
    overviewLayout->addWidget(purposeText);

    QLabel *shortcomingsTitle = new QLabel("<h2>Known Issues</h2>");
    overviewLayout->addWidget(shortcomingsTitle);

    QLabel *shortcomingsText = new QLabel("<p>The known shortcomings of qmos "
        "include:<ul>"
        "<li>All input files are read-only, you cannot edit your input "
            "data</li>"
        "<li>Large control networks are slow and memory intensive to load</li>"
        "<li>Show cube DN data is extremely slow</li>"
        "<li>Warnings are not displayed graphically</li>"
        "<li>Zooming in too far causes you to pan off of your data</li></ul>");
    shortcomingsText->setWordWrap(true);
    overviewLayout->addWidget(shortcomingsText);

    overviewTab->setWidget(overviewContainer);

    QScrollArea *preparationsTab = new QScrollArea;

    QWidget *preparationsContainer = new QWidget;
    QVBoxLayout *preparationsLayout = new QVBoxLayout;
    preparationsContainer->setLayout(preparationsLayout);

    QLabel *preparationTitle = new QLabel("<h2>Before Using qmos</h2>");
    preparationsLayout->addWidget(preparationTitle);

    QLabel *preparationText = new QLabel("<p>qmos only supports files which "
        "have latitude and longitude information associated with them. Global "
        "projections are also not supported. If your files meet these "
        "requirements, it is beneficial to run a couple of Isis programs on "
        "your files before loading them into qmos. The programs you should run "
        "are:<ul>"
        "<li><i>camstats from=future_input_to_qmos.cub attach=true "
            "sinc=... linc=...</i></li>"
        "  <br>This enables qmos to give you the emission angle, incidence "
               "angle, phase angle, and resolution in the <b>File List</b>"
        "<li><i>footprintinit from=future_input_to_qmos.cub "
            "sinc=... linc=...</i></li>"
        "  <br>Running <i>footprintinit</i> beforehand will significantly speed up loading images "
            "into qmos.<br/><br/>"
        "The footprint is created by \"walking\" around the valid image data, and qmos reprojects "
            "the footprint according to the loaded map file.<br/><br/>"
        "Qmos displays the footprints, and optionally the image data and map grid to the default "
            "IAU radius, unless the radius is specified within the loaded map file.<br/><br/>"
        "For Level1 (raw camera space) images, when calculating the "
               "footprint polygons, footprintinit refers to the image labels "
               "and uses the SPICE kernels and the shape model (DEM if one "
               "exists and is specified, otherwise, the IAU sphere or "
               "ellipsoid is used).  Refer to spiceinit for more information "
               "on loading SPICE onto Level0 and Level1 images. This enables "
               "qmos to use the given footprints instead of trying to "
               "calculate its own. The 'linc' and 'sinc' parameters can have a "
               "significant effect on your image's footprint. Also, images "
               "without footprints cannot be opened more than one at a time. "
               "Running footprintinit will significantly speed up loading "
               "images into qmos.<br>"
               "For Level2 images, do not run footprintinit. The footprint "
               "polygon is created by 'walking' around the valid image data. "
               "qmos 'reprojects' the footprint polygons according to the "
               "loaded Map File.<br>"
        "</ul>");
    preparationText->setWordWrap(true);
    preparationsLayout->addWidget(preparationText);

    preparationsTab->setWidget(preparationsContainer);

    QScrollArea *projectsTab = new QScrollArea;

    QWidget *projectsContainer = new QWidget;
    QVBoxLayout *projectsLayout = new QVBoxLayout;
    projectsContainer->setLayout(projectsLayout);

    QLabel *projectsTitle = new QLabel("<h2>Projects</h2>");
    projectsLayout->addWidget(projectsTitle);

    QLabel *projectsText = new QLabel("<p>The contents of qmos can be saved as a project file, "
        "which allows the user to restore back to the previous state at any given time. The stored "
        "files or qmos project files must have a \".mos\" extension.<br/><br/>"
        "These project files store the input file location information and their qmos properties "
        "(color, group information, and other attributes).<br/><br/>"
        "When you initially open qmos you start with a blank project. "
        "To load a project, you can specify the project "
        "file's name on the command line (qmos myProject.mos) or go to "
        "File -> Load Project after qmos is started. When "
        "loading a project, all current data in the qmos window is lost (your cubes are closed)."
        "These project files are relatively small files. You can "
        "save your current project any time by going to File -> Save Project. ");
    projectsText->setWordWrap(true);
    projectsLayout->addWidget(projectsText);

    projectsTab->setWidget(projectsContainer);

    if (m_controllerVisible) {
      tabArea->addTab(overviewTab, "&Overview");
      tabArea->addTab(preparationsTab, "Preparing &Input Cubes");

      tabArea->addTab(ImageFileListWidget::getLongHelp(m_fileListDock),
                      "File &List");
      tabArea->addTab(MosaicSceneWidget::getLongHelp(centralWidget()),
                      "Mosaic &Scene");
      tabArea->addTab(MosaicSceneWidget::getPreviewHelp(m_mosaicPreviewDock),
                      "Mosaic &World View");

      tabArea->addTab(MosaicSceneWidget::getMapHelp(),
                      "&Map File");
      tabArea->addTab(projectsTab, "&Project Files");

      tabArea->addTab(MosaicSceneWidget::getControlNetHelp(),
                      "&Control Networks");
      tabArea->addTab(MosaicSceneWidget::getGridHelp(),
                      "Mosaic &Grid");
    }
    else {
      tabArea->addTab(overviewTab, "&Overview");
      tabArea->addTab(preparationsTab, "Preparing &Input Cubes");

      tabArea->addTab(ImageFileListWidget::getLongHelp(),
                      "File &List");
      tabArea->addTab(MosaicSceneWidget::getLongHelp(),
                      "Mosaic &Scene");
      tabArea->addTab(MosaicSceneWidget::getPreviewHelp(),
                      "Mosaic &World View");

      tabArea->addTab(MosaicSceneWidget::getMapHelp(),
                      "&Map File");
      tabArea->addTab(projectsTab, "&Project Files");

      tabArea->addTab(MosaicSceneWidget::getControlNetHelp(),
                      "&Control Networks");
      tabArea->addTab(MosaicSceneWidget::getGridHelp(),
                      "Mosaic &Grid");
    }

    // Now add close option
    QWidget *buttonsArea = new QWidget;
    mainLayout->addWidget(buttonsArea);

    QHBoxLayout *buttonsLayout = new QHBoxLayout;
    buttonsArea->setLayout(buttonsLayout);

    // Flush the buttons to the right
    buttonsLayout->addStretch();

    QPushButton *closeButton = new QPushButton(QIcon::fromTheme("window-close"),
        "&Close");
    closeButton->setDefault(true);
    connect(closeButton, SIGNAL(clicked()),
            helpDialog, SLOT(close()));
    buttonsLayout->addWidget(closeButton);

    helpDialog->show();
  }


  void MosaicMainWindow::updateMenuVisibility() {
    QMenuBar *rootMenu = menuBar();

    QAction *rootAction = NULL;
    foreach(rootAction, rootMenu->actions()) {
      QMenu *rootMenu = rootAction->menu();

      if (rootMenu) {
        rootAction->setVisible(updateMenuVisibility(rootAction->menu()));
      }
    }
  }


  void MosaicMainWindow::createController() {
    if (m_mosaicController == NULL) {
      m_mosaicController = new MosaicController(statusBar(), m_settings);

      QList<QAction *> settingsActs = m_mosaicController->getSettingsActions();

      QAction *settingsAct;
      foreach(settingsAct, settingsActs) {
        connect(settingsAct, SIGNAL(destroyed(QObject *)),
                this, SLOT(updateMenuVisibility()));

        m_settingsMenu->addAction(settingsAct);
      }

      updateMenuVisibility();
    }
  }


  void MosaicMainWindow::displayController() {
    if (m_mosaicController && !m_controllerVisible) {
      m_controllerVisible = true;
      m_mosaicController->addExportActions(*m_exportMenu);

      m_fileListDock->setWidget(m_mosaicController->getImageFileList());
      m_mosaicPreviewDock->setWidget(m_mosaicController->getMosaicWorldScene());

      centralWidget()->layout()->addWidget(
          m_mosaicController->getMosaicScene());

      QAction *actionRequiringOpen = NULL;
      foreach(actionRequiringOpen, m_actionsRequiringOpen) {
        actionRequiringOpen->setEnabled(true);
      }

      QAction *actionRequiringClosed = NULL;
      foreach(actionRequiringClosed, m_actionsRequiringClosed) {
        actionRequiringClosed->setEnabled(false);
      }

      m_mosaicController->getMosaicScene()->addTo(m_toolpad);
      m_mosaicController->getMosaicScene()->addToPermanent(m_permToolbar);
      m_mosaicController->getMosaicScene()->addTo(m_activeToolbar);

      statusBar()->addWidget(m_mosaicController->getProgress());
      statusBar()->addWidget(
          m_mosaicController->getMosaicScene()->getProgress());
      statusBar()->addWidget(
          m_mosaicController->getMosaicWorldScene()->getProgress());
      statusBar()->addWidget(
          m_mosaicController->getImageFileList()->getProgress());

      QList<QAction *> sceneViewActs =
          m_mosaicController->getMosaicScene()->getViewActions();

      foreach(QAction *viewAct, sceneViewActs) {
        connect(viewAct, SIGNAL(destroyed(QObject *)),
                this, SLOT(updateMenuVisibility()));

        m_viewMenu->addAction(viewAct);
      }

      m_viewMenu->addSeparator();


      QList<QAction *> fileListViewActs =
          m_mosaicController->getImageFileList()->getViewActions();

      foreach(QAction *viewAct, fileListViewActs) {
        connect(viewAct, SIGNAL(destroyed(QObject *)),
                this, SLOT(updateMenuVisibility()));

        m_viewMenu->addAction(viewAct);
      }

      updateMenuVisibility();
    }
  }


  bool MosaicMainWindow::updateMenuVisibility(QMenu *menu) {
    bool anythingVisible = false;

    if (menu) {
      QList<QAction *> actions = menu->actions();

      // Recursively search the menu for other menus to show or hide and handle
      //   every internal being invisible
      QAction *menuAction = NULL;
      foreach(menuAction, menu->actions()) {
        bool thisVisible = true;

        if (menuAction->menu() != NULL) {
          thisVisible = updateMenuVisibility(menuAction->menu());
        }
        else {
          thisVisible = menuAction->isVisible();
        }

        if (thisVisible)
          anythingVisible = true;
      }

      menu->menuAction()->setVisible(anythingVisible);
    }

    return anythingVisible;
  }


  /**
   * Opens a list of cube files instead of one at a time.
   *
   */
  void MosaicMainWindow::openList() {
    // Set up the list of filters that are default with this dialog.
    QStringList filterList;
    filterList.append("List Files (*.lis)");
    filterList.append("Text Files (*.txt)");
    filterList.append("All files (*)");

    QDir directory = m_lastOpenedFile.dir();

    QString selected = QFileDialog::getOpenFileName(this, "Open Cube List",
        directory.path(), filterList.join(";;"));

    if (selected != "") {
      m_lastOpenedFile = QFileInfo(selected);
      TextFile fileList((QString) selected);

      QStringList filesInList;
      QString line;

      while(fileList.GetLine(line)) {
        filesInList.append(line);
      }

      if (filesInList.empty()) {
        IString msg = "No files were found inside the file list";
        throw IException(IException::Unknown, msg, _FILEINFO_);
      }

      openFiles(filesInList);
    }
  }


  /**
   * This overriden method is called from the constructor so that
   * when the Mosaicmainwindow is created, it know's it's size
   * and location and the tool bar location.
   *
   */
  void MosaicMainWindow::readSettings(QSize defaultSize) {
    MainWindow::readSettings(defaultSize);
  }


  void MosaicMainWindow::openFiles(QStringList cubeNames) {
    // Create a mosaic widget if we don't have one
    if (!cubeNames.empty()) {
      displayController();
    }

    if (m_mosaicController)
      m_mosaicController->openImages(cubeNames);
  }


  /**
   * This overriden method is called when the MosaicMainWindow
   * is closed or hidden to write the size and location settings
   * (and tool bar location) to a config file in the user's home
   * directory.
   *
   */
  void MosaicMainWindow::saveSettings2() {
  }


  /**
   * Allows the user to save a project file.
   */
  void MosaicMainWindow::saveProjectAs() {
    if (m_mosaicController) {
      QString fn =  QFileDialog::getSaveFileName(this, "Save Project",
                    QDir::currentPath() + "/untitled.mos",
                    "Mosaic (*.mos)");
      if (fn.isEmpty()) return;

      m_mosaicController->saveProject(fn);
      m_filename = fn;
    }
  }


  /**
   * Called from the file menu to save a project file.
   *
   */
  void MosaicMainWindow::saveProject() {
    if (m_filename == "") {
      saveProjectAs();
    }
    else {
      m_mosaicController->saveProject(m_filename);
    }
  }


  /**
   * Allows users to select a project which is then read in and
   * displayed in the qmos window.
   *
   */
  void MosaicMainWindow::loadProject() {
    QString fn =  QFileDialog::getOpenFileName(this, "Load Project",
                  QDir::currentPath(),
                  "Mosaic (*.mos)");

    if (!fn.isEmpty()) {
      closeMosaic();

      m_lastOpenedFile = QFileInfo(fn);
      loadProject(fn);
    }
  }


  void MosaicMainWindow::loadProject(QString fn) {
    if (!fn.isEmpty()) {
      createController();
      displayController();

      if (m_mosaicController)
        m_mosaicController->readProject(fn);

      m_filename = fn;
    }
  }


  void MosaicMainWindow::closeMosaic() {
    if (m_mosaicController) {
      QAction *actionRequiringOpen = NULL;
      foreach(actionRequiringOpen, m_actionsRequiringOpen) {
        actionRequiringOpen->setEnabled(false);
      }

      QAction *actionRequiringClosed = NULL;
      foreach(actionRequiringClosed, m_actionsRequiringClosed) {
        actionRequiringClosed->setEnabled(true);
      }

      m_mosaicController->saveSettings(m_settings);
      delete m_mosaicController;
      m_mosaicController = NULL;

      m_filename = "";
      m_controllerVisible = false;
    }

    createController();
    displayController();
  }
}
