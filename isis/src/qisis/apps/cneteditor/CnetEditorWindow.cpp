/** This is free and unencumbered software released into the public domain.

The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */

#include "CnetEditorWindow.h"

#include <QAction>
#include <QApplication>
#include <QButtonGroup>
#include <QCheckBox>
#include <QCloseEvent>
#include <QCoreApplication>
#include <QDebug>
#include <QDockWidget>
#include <QHBoxLayout>
#include <QLayout>
#include <QFont>
#include <QFileDialog>
#include <QLabel>
#include <QMessageBox>
#include <QMenu>
#include <QMenuBar>
#include <QQueue>
#include <QRadioButton>
#include <QSettings>
#include <QStatusBar>
#include <QString>
#include <QToolBar>

#include "ConcurrentControlNetReader.h"
#include "Control.h"
#include "ControlNet.h"
#include "FileName.h"
#include "IException.h"
#include "ProgressBar.h"
#include "Pvl.h"

#include "CnetDisplayProperties.h"
#include "CnetEditorWidget.h"

using std::endl;

namespace Isis {

  CnetEditorFileDialog::CnetEditorFileDialog(QLayout *l,
      QWidget *parent) : QFileDialog(parent) {

    setAcceptMode(AcceptSave);
    setNameFilter("Control Network files (*.net *.bin);;All files (*)");
    QGridLayout *mainLayout = qobject_cast< QGridLayout * >(layout());

    if (mainLayout)
      mainLayout->addLayout(l, mainLayout->rowCount(), 0, 1, -1);
  }


  CnetEditorWindow::CnetEditorWindow() {
    // Add the Qt plugin directory to the library path
    FileName qtpluginpath("$ISISROOT/3rdParty/plugins");
    QCoreApplication::addLibraryPath(qtpluginpath.expanded());

    // For some reason GUI style is not detected correctly by Qt for Isis.
    // This solution is less than ideal since it assumes one of at least 4
    // possible styles that could exist for X11 systems.  Plastique will
    // probably be the most common style for KDE users.  If its wrong (say it
    // was really Oxygen) then it will still be better than using Windows style,
    // which is what Qt is falling back to after it fails to choose the correct
    // style.
#ifdef Q_WS_X11
    QApplication::setStyle("Plastique");
#endif
#ifdef Q_WS_MACX
    QApplication::setStyle("macintosh");
#endif

    nullify();

    displayProperties = CnetDisplayProperties::getInstance();
    curFile = new QString;
    cubeListFile = new QString;
    labelFont = new QFont("Sansserif", 9);
    toolBars = new QList<QToolBar *>;

    createActions();
    createDockWidgets();
    createMenus();
    createToolBars();
    createStatusBar();
    readSettings();

    setFileState(NoFile, "");
    setSaveFilteredNetwork(false);

    QHBoxLayout *horizontalLayout = new QHBoxLayout;
    QWidget *dummyCentralWidget = new QWidget;
    dummyCentralWidget->setLayout(horizontalLayout);
    setCentralWidget(dummyCentralWidget);

    QStringList args = QApplication::arguments();
    args.removeFirst();

    // Can only load two file at a time
    if (args.size() > 2) {
      QString msg = tr("Cannot open more than one .net file and one .lis file at a time.");
      std::cerr << msg << endl;
      QMessageBox::warning(this, tr("Unable to Open Files"), msg);
    }
    else {
      // Check for invalid file extensions
      foreach (QString arg, args) {
        QString extension = QFileInfo(arg).suffix();
        if (extension.compare("net") != 0 && extension.compare("lis") != 0) {
          args.removeAll(arg);
          QString msg = tr("Invalid file extension [%1]. "
                           "Expected .net or .lis.").arg(arg);
          std::cerr << msg << endl;
          QMessageBox::warning(this, tr("Invalid File Extension"), msg);
        }
      }
      // Prevent multiple files of the same type from loading
      if (args.size() == 2 && QFileInfo(args[0]).suffix() == QFileInfo(args[1]).suffix()) {
        QString msg = tr("Cannot open two [%1] files.").arg(args[0]);
        std::cerr << msg << endl;
        QMessageBox::warning(this, tr("Unable to Open Files"), msg);
      }
      // Load file(s)
      else if (args.size() != 0) {
        foreach (QString arg, args) {
          QString extension = QFileInfo(arg).suffix();
            if (extension == "net")
              load(arg);
            else if (extension == "lis")
              loadCubeList(arg);
        }
      }
    }
  }


  CnetEditorWindow::~CnetEditorWindow() {
    delete openNetAct;
    openNetAct = NULL;

    delete saveAct;
    saveAct = NULL;

    delete saveAsAct;
    saveAsAct = NULL;

    delete aboutAct;
    aboutAct = NULL;

    delete closeAct;
    closeAct = NULL;

    delete quitAct;
    quitAct = NULL;

    delete cnet;
    cnet = NULL;

    delete displayProperties;
    displayProperties = NULL;

    delete cnetReader;
    cnetReader = NULL;

    delete editorWidget;
    editorWidget = NULL;

    delete curFile;
    curFile = NULL;

    delete cubeListFile;
    cubeListFile = NULL;

    delete labelFont;
    labelFont = NULL;

    delete cubeListProgressBar;
    cubeListProgressBar = NULL;

    delete toolBars;
    toolBars = NULL;

    nullify();
  }


  void CnetEditorWindow::nullify() {
    cnet = NULL;
    displayProperties = NULL;
    cnetReader = NULL;
    editorWidget = NULL;
    curFile = NULL;
    cubeListFile = NULL;
    labelFont = NULL;

    openNetAct = NULL;
    saveAct = NULL;
    saveAsAct = NULL;
    aboutAct = NULL;
    closeAct = NULL;
    quitAct = NULL;

    fileMenu = NULL;
    helpMenu = NULL;

    mainToolBar = NULL;
    toolBars = NULL;

    cubeListProgressBar = NULL;

    pointTreeDockWidget = NULL;
    serialTreeDockWidget = NULL;
    connectionTreeDockWidget = NULL;

    pointFilterDockWidget = NULL;
    serialFilterDockWidget = NULL;
    connectionFilterDockWidget = NULL;
  }


  void CnetEditorWindow::closeEvent(QCloseEvent *event) {
    if (okToContinue()) {
      writeSettings();

      if (editorWidget)
        editorWidget->writeSettings();

      event->accept();
    }
    else {
      event->ignore();
    }
  }


  void CnetEditorWindow::createActions() {
    openNetAct = new QAction(QIcon(FileName("$ISISROOT/appdata/images/icons/fileopen.png").expanded()),
                             tr("&Open control network"), this);
    openNetAct->setShortcut(tr("Ctrl+O"));
    openNetAct->setStatusTip(tr("Open a control network file"));
    connect(openNetAct, SIGNAL(triggered()), this, SLOT(openNet()));

    openCubeListAct = new QAction(QIcon(FileName("$ISISROOT/appdata/images/icons/openList.png").expanded()),
                                  tr("Open cube &list"), this);
    openCubeListAct->setShortcut(tr("Ctrl+L"));
    openCubeListAct->setStatusTip(tr("Open a cube list file"));
    connect(openCubeListAct, SIGNAL(triggered()), this, SLOT(openCubeList()));

    saveAct = new QAction(QIcon(FileName("$ISISROOT/appdata/images/icons/filesave.png").expanded()),
                          tr("&Save"), this);
    saveAct->setShortcut(tr("Ctrl+S"));
    saveAct->setStatusTip(tr("save changes"));
    connect(saveAct, SIGNAL(triggered()), this, SLOT(save()));

    saveAsAct = new QAction(QIcon(FileName("$ISISROOT/appdata/images/icons/filesaveas.png").expanded()),
                            tr("Save&As"), this);
    saveAsAct->setStatusTip(tr("Save control network to specified file"));
    connect(saveAsAct, SIGNAL(triggered()), this, SLOT(saveAs()));

    closeAct = new QAction(QIcon(FileName("$ISISROOT/appdata/images/icons/fileclose.png").expanded()),
                           tr("&Close"), this);
    closeAct->setStatusTip(tr("Close control net file"));
    connect(closeAct, SIGNAL(triggered()), this, SLOT(closeNetwork()));

    aboutAct = new QAction(tr("&About"), this);
    aboutAct->setStatusTip(tr("Show cneteditor's about box"));
    connect(aboutAct, SIGNAL(triggered()), this, SLOT(about()));

    quitAct = new QAction(QIcon(FileName("$ISISROOT/appdata/images/icons/exit.png").expanded()),
                          tr("&Quit"), this);
    quitAct->setShortcut(tr("Ctrl+Q"));
    quitAct->setStatusTip(tr("Quit cneteditor"));
    connect(quitAct, SIGNAL(triggered()), this, SLOT(close()));
  }


  void CnetEditorWindow::createDockWidgets() {
    setCorner(Qt::BottomLeftCorner, Qt::LeftDockWidgetArea);
    setCorner(Qt::BottomRightCorner, Qt::RightDockWidgetArea);
    setCorner(Qt::TopLeftCorner, Qt::LeftDockWidgetArea);
    setCorner(Qt::TopRightCorner, Qt::RightDockWidgetArea);

    pointTreeDockWidget = new QDockWidget("Point View", this, Qt::SubWindow);
    pointTreeDockWidget->setObjectName("PointTreeDock");
    pointTreeDockWidget->setFeatures(QDockWidget::DockWidgetFloatable |
        QDockWidget::DockWidgetMovable | QDockWidget::DockWidgetClosable);
    addDockWidget(Qt::BottomDockWidgetArea, pointTreeDockWidget);

    serialTreeDockWidget = new QDockWidget("Serial View", this, Qt::SubWindow);
    serialTreeDockWidget->setObjectName("SerialTreeDock");
    serialTreeDockWidget->setFeatures(QDockWidget::DockWidgetFloatable |
        QDockWidget::DockWidgetMovable | QDockWidget::DockWidgetClosable);
    addDockWidget(Qt::BottomDockWidgetArea, serialTreeDockWidget);
    tabifyDockWidget(pointTreeDockWidget, serialTreeDockWidget);

    connectionTreeDockWidget = new QDockWidget("Connection View",
        this, Qt::SubWindow);
    connectionTreeDockWidget->setObjectName("ConnectionTreeDock");
    connectionTreeDockWidget->setFeatures(QDockWidget::DockWidgetFloatable |
        QDockWidget::DockWidgetMovable | QDockWidget::DockWidgetClosable);
    addDockWidget(Qt::BottomDockWidgetArea, connectionTreeDockWidget);
    tabifyDockWidget(serialTreeDockWidget, connectionTreeDockWidget);
    pointTreeDockWidget->raise();

    pointFilterDockWidget = new QDockWidget("Filter Points and Measures",
        this, Qt::SubWindow);
    pointFilterDockWidget->setObjectName("PointFilterDock");
    pointFilterDockWidget->setFeatures(QDockWidget::DockWidgetFloatable |
        QDockWidget::DockWidgetMovable | QDockWidget::DockWidgetClosable);
    addDockWidget(Qt::BottomDockWidgetArea, pointFilterDockWidget);

    serialFilterDockWidget = new QDockWidget("Filter Images and Points",
        this, Qt::SubWindow);
    serialFilterDockWidget->setObjectName("SerialFilterDock");
    serialFilterDockWidget->setFeatures(QDockWidget::DockWidgetFloatable |
        QDockWidget::DockWidgetMovable | QDockWidget::DockWidgetClosable);
    addDockWidget(Qt::BottomDockWidgetArea, serialFilterDockWidget);
    tabifyDockWidget(pointFilterDockWidget, serialFilterDockWidget);

    connectionFilterDockWidget = new QDockWidget("Filter Connections",
        this, Qt::SubWindow);
    connectionFilterDockWidget->setObjectName("ConnectionFilterDock");
    connectionFilterDockWidget->setFeatures(QDockWidget::DockWidgetFloatable |
        QDockWidget::DockWidgetMovable | QDockWidget::DockWidgetClosable);
    addDockWidget(Qt::BottomDockWidgetArea, connectionFilterDockWidget);
    tabifyDockWidget(serialFilterDockWidget, connectionFilterDockWidget);
    pointFilterDockWidget->raise();
  }


  void CnetEditorWindow::createMenus() {
    fileMenu = menuBar()->addMenu("&File");
    fileMenu->addAction(openNetAct);
    fileMenu->addSeparator();
    fileMenu->addAction(openCubeListAct);
    fileMenu->addSeparator();
    fileMenu->addAction(saveAct);
    fileMenu->addAction(saveAsAct);
    fileMenu->addSeparator();
    fileMenu->addAction(closeAct);
    fileMenu->addSeparator();
    fileMenu->addAction(quitAct);

    menuBar()->addMenu("&Tables");

    menuBar()->addMenu("&Coordinate Display");

    helpMenu = menuBar()->addMenu("&Help");
  }


  void CnetEditorWindow::createToolBars() {
    mainToolBar = new QToolBar(tr("Main ToolBar"));
    mainToolBar->setObjectName("main toolbar");
    mainToolBar->setFloatable(false);
    mainToolBar->addAction(openCubeListAct);
    mainToolBar->addSeparator();
    mainToolBar->addAction(openNetAct);
    mainToolBar->addSeparator();
    mainToolBar->addAction(saveAct);
    mainToolBar->addAction(saveAsAct);
    mainToolBar->addSeparator();
    mainToolBar->addAction(closeAct);
    mainToolBar->addSeparator();

    addToolBar(Qt::TopToolBarArea, mainToolBar);
  }


  void CnetEditorWindow::createStatusBar() {
    cubeListProgressBar = new ProgressBar("Reading cube list");
    statusBar()->addPermanentWidget(cubeListProgressBar);
    cubeListProgressBar->setVisible(false);
  }


  void CnetEditorWindow::readSettings() {

    QSettings settings(FileName(
        "$HOME/.Isis/cneteditor/cneteditor.config").expanded(),
        QSettings::NativeFormat);

    // load window position and size
    QPoint pos = settings.value("pos", QPoint(100, 100)).toPoint();
    QSize size = settings.value("size", QSize(defaultWindowWidth,
        defaultWindowHeight)).toSize();
    resize(size);
    move(pos);

    setWindowIcon(QIcon(":usgs"));
    restoreState(settings.value("windowState").toByteArray());
  }


  void CnetEditorWindow::writeSettings() {

    QSettings settings(FileName(
        "$HOME/.Isis/cneteditor/cneteditor.config").expanded(),
        QSettings::NativeFormat);

    // save window position and size
    settings.setValue("pos", pos());
    settings.setValue("size", size());
    settings.setValue("windowState", saveState());
  }


  bool CnetEditorWindow::okToContinue() {
    bool ok = true;

    if (dirty) {
      QString name = QFileInfo(*curFile).fileName();
      int r = QMessageBox::warning(this, tr("cneteditor"),
          "The network \"" + name + "\" has been modified.\n"
          "Do you want to save your changes or discard them?",
          QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel);

      if (r == QMessageBox::Cancel)
        ok = false;
      else if (r == QMessageBox::Save)
        save();
    }

    return ok;
  }


  void CnetEditorWindow::setDirty() {
    setDirty(true);
  }


  void CnetEditorWindow::setSaveAsPvl(int state) {
    saveAsPvl = (bool) state;
  }


  void CnetEditorWindow::setSaveFilteredNetwork(bool enabled) {
    saveFilteredNetwork = enabled;
  }


  void CnetEditorWindow::openCubeList() {
    QString filename = QFileDialog::getOpenFileName(this,
        tr("Open a cube list file"), ".",
        tr("Cube list files (*.lis);;All files (*)"));

    if (filename.size())
      loadCubeList(filename);
  }


  void CnetEditorWindow::openNet() {
    QString filename = QFileDialog::getOpenFileName(this,
        tr("Open a control net file"), ".",
        tr("Control Network files (*.net *.bin);;All files (*)"));

    if (!filename.isEmpty())
      load(filename);
  }


  void CnetEditorWindow::setFileState(CnetEditorWindow::FileState state,
      QString filename) {

    switch (state) {
      case HasFile:
        centralWidget()->layout()->addWidget(editorWidget);

        openCubeListAct->setEnabled(true);
        openNetAct->setEnabled(false);
        saveAsAct->setEnabled(true);
        closeAct->setEnabled(true);
        setDirty(false);
        *curFile = filename;
        setWindowTitle(filename + "[*] - cneteditor *BETA VERSION*");
        break;

      case NoFile:
        openCubeListAct->setEnabled(true);
        openNetAct->setEnabled(true);
        saveAsAct->setEnabled(false);
        closeAct->setEnabled(false);
        setDirty(false);
        saveAsPvl = false;
        saveFilteredNetwork = false;
        *curFile = "";
        setWindowTitle("cneteditor *BETA VERSION*");
        break;

      case FileLoading:
        openCubeListAct->setEnabled(false);
        openNetAct->setEnabled(false);
        saveAsAct->setEnabled(false);
        closeAct->setEnabled(false);
        *curFile = filename;
        setWindowTitle("cneteditor *BETA VERSION* (loading "
            + filename + "...)");
        break;
    }
  }


  void CnetEditorWindow::load(QString filename) {

    try {
      cnetReader = new ConcurrentControlNetReader;

      statusBar()->addWidget(cnetReader->progressBar());
      cnetReader->progressBar()->setText("Reading network");

      // call networkLoaded when the reading is complete
      connect(cnetReader, SIGNAL(networksReady(QList<Control *>)),
              this, SLOT(networkLoaded(QList<Control *>)));

      cnetReader->read(filename);

      setFileState(FileLoading, filename);
    }
    catch (IException &e) {
      QString msg = tr("Failed to open the file [%1].").arg(filename);
      std::cerr << msg << endl;
      QMessageBox::critical(this, tr("cneteditor"), msg);
      setFileState(NoFile, "");
    }
  }


  void CnetEditorWindow::loadCubeList(QString filename) {

    try {

    connect(displayProperties, SIGNAL(composeProgressRangeChanged(int, int)),
            cubeListProgressBar, SLOT(setRange(int, int)));
    connect(displayProperties, SIGNAL(composeProgressChanged(int)),
            cubeListProgressBar, SLOT(setValue(int)));
    connect(displayProperties, SIGNAL(compositionFinished()),
            this, SLOT(cubeListLoaded()));

    // Show the cube list progress bar and start loading the cube list.
    cubeListProgressBar->setValue(cubeListProgressBar->minimum());
    cubeListProgressBar->setVisible(true);
    displayProperties->setCubeList(filename);
    *cubeListFile = filename;
    }
    catch (IException &e) {
      QString msg = tr("Failed to open the file [%1].").arg(filename);
      std::cerr << msg << endl;
      QMessageBox::critical(this, tr("cneteditor"), msg);
      setFileState(NoFile, "");
    }
  }


  void CnetEditorWindow::save() {

    if (saveFilteredNetwork) {
      QString newCubeListFileName;

      // Get our filtered network and write it to disk.
      ControlNet *filteredCnet = editorWidget->filteredNetwork();
      filteredCnet->Write(*curFile, saveAsPvl);

      if (cubeListFile->size()) {
        // Get the path of the new cube list file that we should create from our
        // filtered network.
        newCubeListFileName = QFileDialog::getSaveFileName(this,
            tr("Save the cube list based on the filtered network"), ".",
            tr("Cube list files (*.lis);;All files (*)"));

        if (newCubeListFileName.size()) {
          QFile newCubeListFile(newCubeListFileName);

          if (!newCubeListFile.open(QIODevice::WriteOnly | QIODevice::Text |
              QIODevice::Truncate)) {
            IString msg = "The file [";
            msg += (IString) newCubeListFileName;
            msg += "failed to open for writing.\n";
            throw IException(IException::Programmer, msg, _FILEINFO_);
          }

          QStringList cubeFileNames =
            displayProperties->getCubeList(filteredCnet);

          QTextStream ts(&newCubeListFile);
          foreach (QString cubeFileName, cubeFileNames) {
            ts << cubeFileName << Qt::endl;
          }

          newCubeListFile.close();
        }
      }

      // Close the current network and load the new filtered network. We have to
      // save off our current filename so that we can restore it after the call
      // to closeNetwork(...).
      QString currentFile = *curFile;
      closeNetwork(false);
      *curFile = currentFile;
      networkLoaded(filteredCnet, currentFile);

      if (newCubeListFileName != "")
        loadCubeList(newCubeListFileName);
    }
    else {
      // Save the entire control network.
      cnet->Write(*curFile, saveAsPvl);
    }

    setDirty(false);
  }


  void CnetEditorWindow::saveAs() {
    QString whatsThis = "Use these radio buttons to select how the file will "
        "be saved.  Your choice is either plain text (in the PVL format) or "
        "binary.  Although the default is inherited from the currently opened "
        "file (no matter how big the network), binary is recommended ("
        "especially for large control networks).  Choose plain text if you "
        "need to be able to view and/or edit your control net file using a "
        "text editor.";

    QRadioButton *binButton = new QRadioButton("Save in binary format");
    binButton->setToolTip("Save the control network as a binary file");
    binButton->setWhatsThis(whatsThis);

    QRadioButton *pvlButton = new QRadioButton("Save in text (PVL) format");
    pvlButton->setToolTip("Save the control network in plain text (PVL "
        "format)");
    pvlButton->setWhatsThis(whatsThis);

    QButtonGroup *buttonGroup = new QButtonGroup;
    buttonGroup->addButton(binButton, 0);
    buttonGroup->addButton(pvlButton, 1);
    connect(buttonGroup, SIGNAL(buttonClicked(int)),
        this, SLOT(setSaveAsPvl(int)));

    if (saveAsPvl)
      pvlButton->click();
    else
      binButton->click();

    // Create a checkbox to allow users to save only their currently filtered
    // network and its corresponding cube list. We explicitly disable this
    // action so that the user must select it each time. It wouldn't be a good
    // idea to have this option default to enabled.
    setSaveFilteredNetwork(false);
    QCheckBox *saveFilteredNetworkCheckBox = new QCheckBox(
      "Save currently filtered network");
    saveFilteredNetworkCheckBox->setToolTip(
      "Save only the currently filtered control network");
    connect(saveFilteredNetworkCheckBox, SIGNAL(toggled(bool)),
        this, SLOT(setSaveFilteredNetwork(bool)));

    QHBoxLayout *buttonLayout = new QHBoxLayout;
    buttonLayout->addWidget(binButton);
    buttonLayout->addWidget(pvlButton);
    buttonLayout->addWidget(saveFilteredNetworkCheckBox);

    CnetEditorFileDialog *fileDialog = new CnetEditorFileDialog(buttonLayout);

    if (fileDialog->exec() == QDialog::Accepted) {
      QString filename = fileDialog->selectedFiles().value(0);

      if (!filename.isEmpty()) {
        setFileState(CnetEditorWindow::HasFile, filename);
        save();
      }
    }

    // Reset the 'save filtered network' in all cases as we don't want save() to
    // save the filtered network unless the user explicitly chose this option.
    setSaveFilteredNetwork(false);
  }


  void CnetEditorWindow::closeNetwork(bool promptToSave) {
    if (!promptToSave || okToContinue()) {
      disconnect(editorWidget, SIGNAL(cnetModified()), this, SLOT(setDirty()));
      delete editorWidget;
      editorWidget = NULL;
      delete cnet;
      cnet = NULL;
      delete cnetReader;
      cnetReader = NULL;

      foreach (QToolBar *tb, *toolBars) {
        foreach (QAction *tbAct, tb->actions()) {
          delete tbAct;
        }
        delete tb;
        toolBars->removeOne(tb);
      }

      setFileState(NoFile, "");
    }
  }


  void CnetEditorWindow::networkLoaded(ControlNet *net, QString currentFile) {

    cnet = net;
    Control *control = new Control(net, currentFile);
    editorWidget = new CnetEditorWidget(control, FileName(
        "$HOME/.Isis/cneteditor/cneteditor.config").expanded());
    populateMenus();
    populateToolBars();
    connect(editorWidget, SIGNAL(cnetModified()), this, SLOT(setDirty()));

    pointTreeDockWidget->setWidget(editorWidget->pointTreeView());
    serialTreeDockWidget->setWidget(editorWidget->serialTreeView());
    connectionTreeDockWidget->setWidget(editorWidget->connectionTreeView());

    pointFilterDockWidget->setWidget(
      editorWidget->pointFilterWidget());
    serialFilterDockWidget->setWidget(
      editorWidget->serialFilterWidget());
    connectionFilterDockWidget->setWidget(
      editorWidget->connectionFilterWidget());

    setFileState(HasFile, *curFile);
    saveAsPvl = !Pvl(curFile->toStdString()).hasObject("ProtoBuffer");
  }


  void CnetEditorWindow::networkLoaded(QList<Control *> nets) {
    networkLoaded(nets.first()->controlNet(), nets.first()->fileName());
  }


  void CnetEditorWindow::cubeListLoaded() {
    cubeListProgressBar->setVisible(false);
  }


  void CnetEditorWindow::populateMenus() {

    QMap< QAction *, QList< QString > > actionMap;
    actionMap = editorWidget->menuActions();
    QMapIterator< QAction *, QList< QString > > i(actionMap);

    QWidget *widget = NULL;

    while (i.hasNext()) {
      i.next();
      QAction *act = i.key();
      QList< QString > location = i.value();

      widget = menuBar();

      while (location.size()) {
        QString menuName = location.takeFirst();

        int actListIndex = indexOfActionList(widget->actions(), menuName);

        // if menuName not found in current widget's actions,
        // then add needed submenu
        if (actListIndex == -1) {
          QMenuBar *mb = qobject_cast< QMenuBar * >(widget);
          QMenu *m = qobject_cast< QMenu * >(widget);

          if (mb) {
            mb->addMenu(menuName);
            int index = indexOfActionList(mb->actions(), menuName);
            widget = mb->actions()[index]->menu();
          }
          else {
            m->addMenu(menuName);
            int index = indexOfActionList(m->actions(), menuName);
            widget = m->actions()[index]->menu();
          }
        }
        else { // menu exists so just update widget
          widget = widget->actions()[actListIndex]->menu();
        }
      }

      widget->addAction(act);
    }
  }


  int CnetEditorWindow::indexOfActionList(QList< QAction * > actionList,
      QString actionText) {

    int index = -1;
    for (int i = 0; index == -1 && i < actionList.size(); i++)
      if (actionList[i]->text() == actionText)
        index = i;

    return index;
  }


  void CnetEditorWindow::populateToolBars() {
    QMap< QString, QList< QAction * > > actionMap;
    actionMap = editorWidget->toolBarActions();
    QMapIterator< QString, QList< QAction * > > i(actionMap);

    while (i.hasNext()) {
      i.next();
      QString objName = i.key();
      QList< QAction * > actionList = i.value();

      // if toolbar already exists, just add the actions to it
      int index = indexOfToolBar(objName);
      if (index != -1) {
        foreach (QAction *action, actionList) {
          (*toolBars)[index]->addAction(action);
        }
      }
      // otherwise, it needs to be created
      else {
        // don't allow use of mainToolBar outside this class!
        if (objName != mainToolBar->objectName()) {
          QToolBar *newToolBar = new QToolBar(objName);
          toolBars->append(newToolBar);
          newToolBar->setObjectName(objName);
          newToolBar->setFloatable(false);
          foreach (QAction *action, actionList) {
            newToolBar->addAction(action);
          }

          addToolBar(Qt::TopToolBarArea, newToolBar);
        }
      }
    }
  }


  int CnetEditorWindow::indexOfToolBar(QString objName) {

    int index = -1;

    for (int i = 0; index == -1 && i < toolBars->size(); i++)
      if (toolBars->at(i)->objectName() == objName)
        index = i;

    return index;
  }


  void CnetEditorWindow::removeEmptyMenus() {
    QQueue< QWidget * > q;
    q.enqueue(menuBar());

    while (q.size()) {
      QWidget *widget = q.dequeue();
      QList<QAction *> actionList = widget->actions();
      foreach (QAction *action, actionList) {
        QMenu *menu = action->menu();
        if (menu) {
          if (menu->actions().size())
            q.enqueue(menu);
          else
            widget->removeAction(action);
        }
      }
    }
  }


  void CnetEditorWindow::about() {
  }


  void CnetEditorWindow::setDirty(bool state) {
    dirty = state;
    saveAct->setEnabled(state);
    setWindowModified(state);
  }


  void CnetEditorWindow::setDockWidgetsVisible(bool visibilityState) {
    pointTreeDockWidget->setVisible(visibilityState);
    serialTreeDockWidget->setVisible(visibilityState);
    connectionTreeDockWidget->setVisible(visibilityState);
    pointFilterDockWidget->setVisible(visibilityState);
    serialFilterDockWidget->setVisible(visibilityState);
    connectionFilterDockWidget->setVisible(visibilityState);
  }
}
