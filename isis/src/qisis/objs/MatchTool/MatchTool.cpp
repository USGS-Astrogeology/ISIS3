#include "MatchTool.h"

#include <sstream>
#include <vector>
#include <iomanip>

#include <QAction>
#include <QComboBox>
#include <QFileDialog>
#include <QLabel>
#include <QLineEdit>
#include <QMenuBar>
#include <QMessageBox>
#include <QScrollBar>
#include <QShortcut>
#include <QSplitter>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QtWidgets>
#include <QWhatsThis>

#include "Application.h"
#include "ControlMeasure.h"
#include "ControlMeasureLogData.h"
#include "ControlNet.h"
#include "ControlPoint.h"
#include "ControlPointEdit.h"
#include "FileName.h"
#include "IException.h"
#include "MainWindow.h"
#include "MatchToolDeletePointDialog.h"
#include "MatchToolNewPointDialog.h"
#include "MdiCubeViewport.h"
#include "Progress.h"
#include "Pvl.h"
#include "PvlEditDialog.h"
#include "SerialNumber.h"
#include "SerialNumberList.h"
#include "SpecialPixel.h"
#include "ToolPad.h"
#include "ViewportMainWindow.h"
#include "Workspace.h"

using namespace std;

namespace Isis {

  /**
   * Consructs the Match Tool window
   *
   * @param parent Pointer to the parent widget for the Match tool
   *
   * @internal
   * @history 2010-06-03 Jeannie Walldren - Initialized pointers to null.
   *
   */
  MatchTool::MatchTool (QWidget *parent) : Tool(parent) {
    m_controlNet = NULL;
    m_coregNet = false;
    m_netChanged = false;
    m_pointEditor = NULL;
    m_newPointDialog = NULL;
    m_newPoint = NULL;
    m_leftCube = NULL;
    m_rightCube = NULL;
    m_editPoint = NULL;
    m_createPoint = NULL;
    m_modifyPoint = NULL;
    m_deletePoint = NULL;
    m_whatsThis = NULL;
    m_showHelp = NULL;
    m_ptIdValue = NULL;
    m_numMeasures = NULL;
    m_lockPoint = NULL;
    m_ignorePoint = NULL;
    m_leftReference = NULL;
    m_leftMeasureType = NULL;
    m_leftGoodness = NULL;
    m_rightReference = NULL;
    m_rightMeasureType = NULL;
    m_rightGoodness = NULL;
    m_lockLeftMeasure = NULL;
    m_ignoreLeftMeasure = NULL;
    m_lockRightMeasure = NULL;
    m_ignoreRightMeasure = NULL;
    m_leftCombo = NULL;
    m_rightCombo = NULL;
    m_leftMeasure = NULL;
    m_rightMeasure = NULL;
    m_templateModified = false;
    m_measureWindow = NULL;
    m_measureTable = NULL;

    // try to get status bar from viewportmain window which is parent
//  QStatusBar *m_statusBar = qobject_cast<QMainWindow *>(parent)->statusBar();
//  m_statusBar->showMessage("STATUS BAR TEST");

//  QString warn = "TEST   TEST   TEST";
//  QString warn2 = "JUNK";
//  qobject_cast<ViewportMainWindow *>(parent)->displayWarning(warn,warn2);
    m_parent = parent;
    connect(this, SIGNAL(toolActivated()), this, SLOT(activateTool()));

    // Connect the ViewportMainWindow's (parent) closeWindow signal to a exit slot for
    // prompting user to save net
    ViewportMainWindow *parentMainWindow = qobject_cast<ViewportMainWindow *>(parent);

    if (parentMainWindow) {
      connect(parent, SIGNAL(closeWindow()), this, SLOT(exiting()));
    }

    createMatchTool(parent);

  }


  MatchTool::~MatchTool () {
    // FIXME: Don't write settings in destructor, must do this earlier in close event
    writeSettings();

    delete m_controlNet;
    m_controlNet = NULL;
    delete m_pointEditor;
    m_pointEditor = NULL;
    delete m_newPointDialog;
    m_newPointDialog = NULL;
    delete m_newPoint;
    m_newPoint = NULL;
    delete m_leftMeasure;
    m_leftMeasure = NULL;
    delete m_rightMeasure;
    m_rightMeasure = NULL;
    delete m_leftCube;
    m_leftCube = NULL;
    delete m_rightCube;
    m_rightCube = NULL;
  }



  /**
   * create the main window for editing control points
   *
   * @param parent Pointer to parent QWidget
   * @internal
   *   @history 2008-11-24  Jeannie Walldren - Added "Goodness of Fit" to right
   *                           and left measure info.
   *   @history 2008-11-26  Jeannie Walldren - Added "Number of Measures" to
   *                           QnetTool point information. Moved setWindowTitle()
   *                           command to updateNet() method. Added connection
   *                           between Ignore checkbox toggle() slot and
   *                           ignoreChanged() signal
   *   @history 2008-12-29 Jeannie Walldren - Disabled ground point check box and
   *                          commented out connection between check box and
   *                          setGroundPoint() method.
   *   @history 2008-12-30 Jeannie Walldren - Added connections to toggle
   *                          measures' Ignore check boxes if ignoreLeftChanged()
   *                          and ignoreRightChanged() are emitted. Replaced
   *                          reference to ignoreChanged() with
   *                          ignorePointChanged().
   *   @history 2010-06-03 Jeannie Walldren - Removed "std::" since "using
   *                          namespace std"
   *   @history 2015-11-09 Ian Humphrey - Added shortcut for savePoint (P). References #2324.
   */
  void MatchTool::createMatchTool(QWidget *parent) {

    m_matchTool = new QMainWindow(parent);
    m_matchTool->setWindowTitle("Match Tool");
    m_matchTool->setObjectName("MatchTool");
    connect(m_matchTool, SIGNAL(destroyed(QObject *)), this, SLOT(clearEditPoint()));

    createActions();
    createMenus();
    createToolBars();

    // create m_pointEditor first since we need to get its templateFileName
    // later
    m_pointEditor = new ControlPointEdit(m_controlNet, parent, true, false);
    connect(this, SIGNAL(newControlNetwork(ControlNet *)),
        m_pointEditor, SIGNAL(newControlNetwork(ControlNet *)));
    connect(this, SIGNAL(stretchChipViewport(Stretch *, CubeViewport *)),
            m_pointEditor, SIGNAL(stretchChipViewport(Stretch *, CubeViewport *)));
    connect(m_pointEditor, SIGNAL(measureSaved()), this, SLOT(measureSaved()));
    connect(this, SIGNAL(measureChanged()),
            m_pointEditor, SLOT(colorizeSaveButton()));

    m_savePoint = new QPushButton ("Save Point");
    m_savePoint->setShortcut(Qt::Key_P);
    m_savePoint->setToolTip("Save the edit control point to the control network. "
                            "<strong>Shortcut: P</strong>");
    m_savePoint->setWhatsThis("Save the edit control point to the control "
                    "network which is loaded into memory in its entirety. "
                    "When a control point is selected for editing, "
                    "a copy of the point is made so that the original control "
                    "point remains in the network.");
    m_saveDefaultPalette = m_savePoint->palette();
    connect (m_savePoint,SIGNAL(clicked()),this,SLOT(savePoint()));

    QHBoxLayout * addMeasureLayout = new QHBoxLayout;
    addMeasureLayout->addStretch();
    addMeasureLayout->addWidget(m_savePoint);
//    addMeasureLayout->addStretch();

    m_cnetFileNameLabel = new QLabel("Control Network: " + m_cnetFileName);
    m_cnetFileNameLabel->setToolTip("Name of opened control network file.");
    m_cnetFileNameLabel->setWhatsThis("Name of opened control network file.");

    m_templateFileNameLabel = new QLabel("Template File: " +
        m_pointEditor->templateFileName());
    m_templateFileNameLabel->setToolTip("Sub-pixel registration template File.");
//  QString patternMatchDoc =
//          FileName("$ISISROOT/doc/documents/PatternMatch/PatternMatch.html").fileName();
//    m_templateFileNameLabel->setOpenExternalLinks(true);
    m_templateFileNameLabel->setWhatsThis("FileName of the sub-pixel "
                  "registration template.  Refer to https://astrogeology.usgs.gov/docs/concepts/control-networks/pattern-matching/ for a description of the "
                  "contents of this file.");

    QVBoxLayout * centralLayout = new QVBoxLayout;

    centralLayout->addWidget(m_cnetFileNameLabel);
    centralLayout->addWidget(m_templateFileNameLabel);
    centralLayout->addWidget(createTopSplitter());
    centralLayout->addStretch();
    centralLayout->addWidget(m_pointEditor);
    centralLayout->addLayout(addMeasureLayout);
    QWidget * centralWidget = new QWidget;
    centralWidget->setLayout(centralLayout);

    QScrollArea *scrollArea = new QScrollArea();
    scrollArea->setObjectName("MatchToolScroll");
    scrollArea->setWidget(centralWidget);
    scrollArea->setWidgetResizable(true);
    centralWidget->adjustSize();
    m_matchTool->setCentralWidget(scrollArea);
//    m_matchTool->setCentralWidget(centralWidget);


    connect(this, SIGNAL(editPointChanged()),
            this, SLOT(paintAllViewports()));

    readSettings();
  }


  //! creates everything above the ControlPointEdit
  QSplitter * MatchTool::createTopSplitter() {

    QHBoxLayout * measureLayout = new QHBoxLayout;
    measureLayout->addWidget(createLeftMeasureGroupBox());
    measureLayout->addWidget(createRightMeasureGroupBox());

    QVBoxLayout * groupBoxesLayout = new QVBoxLayout;
    groupBoxesLayout->addWidget(createControlPointGroupBox());
    groupBoxesLayout->addStretch();
    groupBoxesLayout->addLayout(measureLayout);

    QWidget * groupBoxesWidget = new QWidget;
    groupBoxesWidget->setLayout(groupBoxesLayout);

    createTemplateEditorWidget();

    QSplitter * topSplitter = new QSplitter;
    topSplitter->addWidget(groupBoxesWidget);
    topSplitter->addWidget(m_templateEditorWidget);
    topSplitter->setStretchFactor(0, 4);
    topSplitter->setStretchFactor(1, 3);

    m_templateEditorWidget->hide();

    return topSplitter;
  }


  //! @returns The groupbox labeled "Control Point"
  QGroupBox * MatchTool::createControlPointGroupBox() {

    // create left vertical layout
    m_ptIdValue = new QLabel;
    m_numMeasures = new QLabel;
    QVBoxLayout * leftLayout = new QVBoxLayout;
    leftLayout->addWidget(m_ptIdValue);
    leftLayout->addWidget(m_numMeasures);

    // create right vertical layout's top layout
    m_lockPoint = new QCheckBox("Edit Lock Point");
    connect(m_lockPoint, SIGNAL(clicked(bool)), this, SLOT(setLockPoint(bool)));
    m_ignorePoint = new QCheckBox("Ignore Point");
    connect(m_ignorePoint, SIGNAL(clicked(bool)),
            this, SLOT(setIgnorePoint(bool)));
    connect(this, SIGNAL(ignorePointChanged()), m_ignorePoint, SLOT(toggle()));

    QVBoxLayout * rightLayout = new QVBoxLayout;
    rightLayout->addWidget(m_lockPoint);
    rightLayout->addWidget(m_ignorePoint);

    QHBoxLayout * mainLayout = new QHBoxLayout;
    mainLayout->addLayout(leftLayout);
    mainLayout->addStretch();
    mainLayout->addLayout(rightLayout);

    // create the groupbox
    QGroupBox * groupBox = new QGroupBox("Control Point");
    groupBox->setLayout(mainLayout);

    return groupBox;
  }


  //! @returns The groupbox labeled "Left Measure"
  QGroupBox * MatchTool::createLeftMeasureGroupBox() {

    m_leftCombo = new QComboBox;
    m_leftCombo->view()->installEventFilter(this);
    m_leftCombo->setToolTip("Choose left control measure");
    m_leftCombo->setWhatsThis("Choose left control measure identified by "
                              "cube filename.");
    connect(m_leftCombo, SIGNAL(activated(int)),
            this, SLOT(selectLeftMeasure(int)));
    m_lockLeftMeasure = new QCheckBox("Edit Lock Measure");
    connect(m_lockLeftMeasure, SIGNAL(clicked(bool)),
            this, SLOT(setLockLeftMeasure(bool)));
    m_ignoreLeftMeasure = new QCheckBox("Ignore Measure");
    connect(m_ignoreLeftMeasure, SIGNAL(clicked(bool)),
            this, SLOT(setIgnoreLeftMeasure(bool)));
    connect(this, SIGNAL(ignoreLeftChanged()),
            m_ignoreLeftMeasure, SLOT(toggle()));
    m_leftReference = new QLabel();
    m_leftMeasureType = new QLabel();
    m_leftSampShift = new QLabel();
    m_leftSampShift->setToolTip("Sample shift between apriori and current");
    m_leftSampShift->setWhatsThis("The shift between the apriori sample and "
                           "the current sample.  The apriori sample is set "
                           "when creating a new measure.");
    m_leftLineShift = new QLabel();
    m_leftLineShift->setToolTip("Line shift between apriori and current");
    m_leftLineShift->setWhatsThis("The shift between the apriori line and "
                           "the current line.  The apriori line is set "
                           "when creating a new measure.");
    m_leftGoodness = new QLabel();
    m_leftGoodness->setToolTip("Goodness of Fit result from sub-pixel "
                               "registration.");
    m_leftGoodness->setWhatsThis("Resulting Goodness of Fit from sub-pixel "
                                 "registration.");
    QVBoxLayout * leftLayout = new QVBoxLayout;
    leftLayout->addWidget(m_leftCombo);
    leftLayout->addWidget(m_lockLeftMeasure);
    leftLayout->addWidget(m_ignoreLeftMeasure);
    leftLayout->addWidget(m_leftReference);
    leftLayout->addWidget(m_leftMeasureType);
    leftLayout->addWidget(m_leftSampShift);
    leftLayout->addWidget(m_leftLineShift);
    leftLayout->addWidget(m_leftGoodness);

    QGroupBox * leftGroupBox = new QGroupBox("Left Measure");
    leftGroupBox->setLayout(leftLayout);

    return leftGroupBox;
  }


  /**
   * Creates the right measure group box.
   *
   * @returns The groupbox labeled "Right Measure"
   *
   * @internal
   *   @history 2015-11-08 Ian Humphrey - Added the shorcuts (PageUp/PageDown) for selecting
   *                           previous or next measure in right measures box. Referecnes #2324.
   */
  QGroupBox * MatchTool::createRightMeasureGroupBox() {

    // create widgets for the right groupbox
    m_rightCombo = new QComboBox;
    m_rightCombo->view()->installEventFilter(this);

    // Attach shortcuts to Match TOol's window for selecting right measures
    // Note: Qt handles this memory for us since m_matchTool is the parent of these shortcuts
    QShortcut *nextMeasure = new QShortcut(Qt::Key_PageDown, m_matchTool);
    connect(nextMeasure, SIGNAL(activated()), this, SLOT(nextRightMeasure()));
    QShortcut *prevMeasure = new QShortcut(Qt::Key_PageUp, m_matchTool);
    connect(prevMeasure, SIGNAL(activated()), this, SLOT(previousRightMeasure()));

    m_rightCombo->setToolTip("Choose right control measure. "
                             "<strong>Shorcuts: PageUp/PageDown</strong>");
    m_rightCombo->setWhatsThis("Choose right control measure identified by "
                               "cube filename. "
                               "Note: PageUp selects previous measure; "
                               "PageDown selects next measure.");
    connect(m_rightCombo, SIGNAL(activated(int)),
            this, SLOT(selectRightMeasure(int)));
    m_lockRightMeasure = new QCheckBox("Edit Lock Measure");
    connect(m_lockRightMeasure, SIGNAL(clicked(bool)),
            this, SLOT(setLockRightMeasure(bool)));
    m_ignoreRightMeasure = new QCheckBox("Ignore Measure");
    connect(m_ignoreRightMeasure, SIGNAL(clicked(bool)),
            this, SLOT(setIgnoreRightMeasure(bool)));
    connect(this, SIGNAL(ignoreRightChanged()),
            m_ignoreRightMeasure, SLOT(toggle()));
    m_rightReference = new QLabel();
    m_rightMeasureType = new QLabel();
    m_rightSampShift = new QLabel();
    m_rightSampShift->setToolTip(m_leftSampShift->toolTip());
    m_rightSampShift->setWhatsThis(m_leftSampShift->whatsThis());
    m_rightLineShift = new QLabel();
    m_rightLineShift->setToolTip(m_leftLineShift->toolTip());
    m_rightLineShift->setWhatsThis(m_leftLineShift->whatsThis());
    m_rightGoodness = new QLabel();
    m_rightGoodness->setToolTip(m_leftGoodness->toolTip());
    m_rightGoodness->setWhatsThis(m_leftGoodness->whatsThis());

    // create right groupbox
    QVBoxLayout * rightLayout = new QVBoxLayout;
    rightLayout->addWidget(m_rightCombo);
    rightLayout->addWidget(m_lockRightMeasure);
    rightLayout->addWidget(m_ignoreRightMeasure);
    rightLayout->addWidget(m_rightReference);
    rightLayout->addWidget(m_rightMeasureType);
    rightLayout->addWidget(m_rightSampShift);
    rightLayout->addWidget(m_rightLineShift);
    rightLayout->addWidget(m_rightGoodness);

    QGroupBox * rightGroupBox = new QGroupBox("Right Measure");
    rightGroupBox->setLayout(rightLayout);

    return rightGroupBox;
  }


  //! Creates the Widget which contains the template editor and its toolbar
  void MatchTool::createTemplateEditorWidget() {

    QToolBar *toolBar = new QToolBar("Template Editor ToolBar");

    toolBar->addAction(m_openTemplateFile);
    toolBar->addSeparator();
    toolBar->addAction(m_saveTemplateFile);
    toolBar->addAction(m_saveTemplateFileAs);

    m_templateEditor = new QTextEdit;
    connect(m_templateEditor, SIGNAL(textChanged()), this,
        SLOT(setTemplateModified()));

    QVBoxLayout *mainLayout = new QVBoxLayout;
    mainLayout->addWidget(toolBar);
    mainLayout->addWidget(m_templateEditor);

    m_templateEditorWidget = new QWidget;
    m_templateEditorWidget->setLayout(mainLayout);
  }


/**
   * @brief Creates the menu actions for Match Tool.
   *
   * @internal
   *   @author ???
   *   @history Ian Humphrey - Added CTRL+S shortcut for saving control network. References #2324.
   */
  void MatchTool::createActions() {

    m_saveNet = new QAction(QPixmap(toolIconDir() + "/mActionFileSave.png"),
                            "Save Control Network ...",
                            m_matchTool);
    m_saveNet->setShortcut(Qt::CTRL + Qt::Key_S);
    m_saveNet->setToolTip("Save current control network");
    m_saveNet->setStatusTip("Save current control network");
    QString whatsThis = "<b>Function:</b> Saves the current <i>"
                        "control network</i>";
    m_saveNet->setWhatsThis(whatsThis);
    connect(m_saveNet, SIGNAL(triggered()), this, SLOT(saveNet()));

    m_saveAsNet = new QAction(QPixmap(toolIconDir() + "/mActionFileSaveAs.png"),
                              "Save Control Network &As...",
                              m_matchTool);
    m_saveAsNet->setToolTip("Save current control network to chosen file");
    m_saveAsNet->setStatusTip("Save current control network to chosen file");
    whatsThis = "<b>Function:</b> Saves the current <i>"
        "control network</i> under chosen filename";
    m_saveAsNet->setWhatsThis(whatsThis);
    connect(m_saveAsNet, SIGNAL(triggered()), this, SLOT(saveAsNet()));

    m_closeMatchTool = new QAction(QPixmap(toolIconDir() + "/fileclose.png"),
                                   "&Close",
                                   m_matchTool);
    m_closeMatchTool->setToolTip("Close this window");
    m_closeMatchTool->setStatusTip("Close this window");
    m_closeMatchTool->setShortcut(Qt::ALT + Qt::Key_F4);
    whatsThis = "<b>Function:</b> Closes the Match Tool window for this point "
        "<p><b>Shortcut:</b> Alt+F4 </p>";
    m_closeMatchTool->setWhatsThis(whatsThis);
    connect(m_closeMatchTool, SIGNAL(triggered()), m_matchTool, SLOT(close()));

    m_showHideTemplateEditor = new QAction(QPixmap(toolIconDir() + "/view_text.png"),
                                           "&View/edit registration template",
                                           m_matchTool);
    m_showHideTemplateEditor->setCheckable(true);
    m_showHideTemplateEditor->setToolTip("View and/or edit the registration template");
    m_showHideTemplateEditor->setStatusTip("View and/or edit the registration template");
    whatsThis = "<b>Function:</b> Displays the curent registration template.  "
       "The user may edit and save changes under a chosen filename.";
    m_showHideTemplateEditor->setWhatsThis(whatsThis);
    connect(m_showHideTemplateEditor, SIGNAL(triggered()), this,
        SLOT(showHideTemplateEditor()));

    m_saveChips = new QAction(QPixmap(toolIconDir() + "/window_new.png"),
                              "Save registration chips",
                              m_matchTool);
    m_saveChips->setToolTip("Save registration chips");
    m_saveChips->setStatusTip("Save registration chips");
    whatsThis = "<b>Function:</b> Save registration chips to file.  "
       "Each chip: pattern, search, fit will be saved to a separate file.";
    m_saveChips->setWhatsThis(whatsThis);
    connect(m_saveChips, SIGNAL(triggered()), this, SLOT(saveChips()));

    m_openTemplateFile = new QAction(QPixmap(toolIconDir() + "/fileopen.png"),
                                     "&Open registration template",
                                     m_matchTool);
    m_openTemplateFile->setToolTip("Set registration template");
    m_openTemplateFile->setStatusTip("Set registration template");
    whatsThis = "<b>Function:</b> Allows user to select a new file to set as "
        "the registration template";
    m_openTemplateFile->setWhatsThis(whatsThis);
    connect(m_openTemplateFile, SIGNAL(triggered()), this, SLOT(openTemplateFile()));

    m_saveTemplateFile = new QAction(QPixmap(toolIconDir() + "/mActionFileSave.png"),
                                     "&Save template file",
                                     m_matchTool);
    m_saveTemplateFile->setToolTip("Save the template file");
    m_saveTemplateFile->setStatusTip("Save the template file");
    m_saveTemplateFile->setWhatsThis("Save the registration template file");
    connect(m_saveTemplateFile, SIGNAL(triggered()), this,
        SLOT(saveTemplateFile()));

    m_saveTemplateFileAs = new QAction(QPixmap(toolIconDir() + "/mActionFileSaveAs.png"),
                                       "&Save template as...",
                                       m_matchTool);
    m_saveTemplateFileAs->setToolTip("Save the template file as");
    m_saveTemplateFileAs->setStatusTip("Save the template file as");
    m_saveTemplateFileAs->setWhatsThis("Save the registration template file as");
    connect(m_saveTemplateFileAs, SIGNAL(triggered()), this,
        SLOT(saveTemplateFileAs()));

    m_whatsThis = new QAction(QPixmap(toolIconDir() + "/contexthelp.png"),
                              "&Whats's This",
                              m_matchTool);
    m_whatsThis->setShortcut(Qt::SHIFT | Qt::Key_F1);
    m_whatsThis->setToolTip("Activate What's This and click on items on "
        "user interface to see more information.");
    connect(m_whatsThis, SIGNAL(triggered()), this, SLOT(enterWhatsThisMode()));

    m_showHelp = new QAction(QPixmap(toolIconDir() + "/help-contents.png"), "Help", m_matchTool);
    m_showHelp->setToolTip("Help");
    connect(m_showHelp, SIGNAL(triggered()), this, SLOT(showHelp()));

  }



  /**
   * Customize dropdown menus below title bar.
   *
   * @internal
   *   @history 2008-11-18 Jeannie Walldren - Added "Close" action to the file
   *                          menu on the match tool window.
   *   @history 2008-12-10 Jeannie Walldren - Added "What's this?" function to
   *                          match tool actions.
   */
  void MatchTool::createMenus () {

    QMenu *fileMenu = m_matchTool->menuBar()->addMenu("&File");
    fileMenu->addAction(m_saveNet);
    fileMenu->addAction(m_saveAsNet);
    fileMenu->addAction(m_closeMatchTool);

    QMenu * regMenu = m_matchTool->menuBar()->addMenu("&Registration");
    regMenu->addAction(m_openTemplateFile);
    regMenu->addAction(m_showHideTemplateEditor);
    regMenu->addAction(m_saveChips);

    QMenu *helpMenu = m_matchTool->menuBar()->addMenu("&Help");
    helpMenu->addAction(m_whatsThis);
  }


  void MatchTool::createToolBars() {

    QToolBar * toolBar = new QToolBar;
    toolBar->setObjectName("TemplateEditorToolBar");
    toolBar->setFloatable(false);
    toolBar->addAction(m_saveNet);
    toolBar->addSeparator();
    toolBar->addAction(m_showHideTemplateEditor);
    toolBar->addAction(m_saveChips);
    toolBar->addAction(m_showHelp);
    toolBar->addAction(m_whatsThis);

    m_matchTool->addToolBar(Qt::TopToolBarArea, toolBar);
  }



  /**
   * Adds the Tie tool action to the tool pad.  When the Tie tool is selected, the
   * Navigation Tool will automatically open.
   *
   * @param pad Tool pad
   * @return @b QAction* Pointer to Tie tool action
   *
   * @internal
   *   @history 2010-07-01 Jeannie Walldren - Added connection between match's
   *                          TieTool button and the showNavWindow() method
   */
  QAction *MatchTool::toolPadAction(ToolPad *pad) {
    QAction *action = new QAction(pad);
    action->setIcon(QPixmap(toolIconDir()+"/stock_draw-connector-with-arrows.png"));
    action->setToolTip("Match Tool - Control Point Editor (T)");
    action->setShortcut(Qt::Key_T);
    return action;
  }



  QWidget *MatchTool::createToolBarWidget(QStackedWidget *parent) {

    QWidget *hbox = new QWidget;

    QToolButton *openNetButton = new QToolButton(hbox);
    openNetButton->setIcon(QPixmap(toolIconDir() + "/fileopen.png"));
    openNetButton->setIconSize(QSize(22,22));
    openNetButton->setToolTip("Open control network");
    openNetButton->setEnabled(true);
    connect(openNetButton, SIGNAL(clicked()), this, SLOT(openNet()));

    QToolButton *saveAsNetButton = new QToolButton(hbox);
    saveAsNetButton->setDefaultAction(m_saveAsNet);
    saveAsNetButton->setIconSize(QSize(22,22));

    QToolButton *saveNetButton = new QToolButton(hbox);
    saveNetButton->setDefaultAction(m_saveNet);
    saveNetButton->setIconSize(QSize(22,22));

    QToolButton *helpButton = new QToolButton(hbox);
    helpButton->setDefaultAction(m_showHelp);
    helpButton->setIconSize(QSize(22, 22));

    QHBoxLayout *layout = new QHBoxLayout;
    layout->setMargin(0);
    layout->addWidget(openNetButton);
    layout->addWidget(saveAsNetButton);
    layout->addWidget(saveNetButton);
    layout->addStretch();
    layout->addWidget(helpButton);
    hbox->setLayout(layout);

    return hbox;
  }



  void MatchTool::activateTool() {

    if (!m_controlNet) {
      m_controlNet = new ControlNet();
    }
  }



  /**
   * Creates a serial number list based on open cube viewports
   *
   * @return SerialNumberList The serial number list based on currently opened cube viewports
   *
   * @author 2012-10-02  Tracie Sucharski
   *
   * @internal
   */
  SerialNumberList MatchTool::serialNumberList() {

    SerialNumberList list(false);
    foreach (MdiCubeViewport *mvp, *cubeViewportList()) {
      try {
        //  Attempt to Compose Serial number and see if list already has duplicate.  If so,
        //  use filenames as serial numbers for both cubes.  This needs to be checked because
        //  coreg networks will often have 2 cubes with the same serial number.make cl
        QString sn = SerialNumber::Compose(mvp->cube()->fileName(), true);
        if (list.hasSerialNumber(sn)) {
          // TODO  Before removing serial number, make sure current network does not have
          // measures with old serial number.  If it does, now what?  Print error?
          //
          // Remove old serial number & change to filename
          FileName fileName = Isis::FileName(list.fileName(sn));
          list.remove(sn);
          list.add(fileName.name(),fileName.expanded());
          // Add new serial number as filename
          list.add(Isis::FileName(mvp->cube()->fileName()).name(),
                                  mvp->cube()->fileName());
        }
        else {
          list.add(mvp->cube()->fileName(), true);
        }
      }
      catch (...) {
      }
    }
    return list;
  }



  QString MatchTool::serialNumber(MdiCubeViewport *mvp) {

    QString serialNumber;
    try {
      SerialNumberList list = serialNumberList();
      serialNumber = list.serialNumber(mvp->cube()->fileName());
//    serialNumber = serialNumberList().SerialNumber(mvp->cube()->fileName());
    }
    catch (IException &e) {
      serialNumber = "Unknown";
    }
    return serialNumber;

  }



  /**
   * This method is connected with the measureSaved() signal from
   * ControlPointEdit.
   *
   * @internal
   *   @history 2008-11-26 Jeannie Walldren - Added message box to warn the user
   *                          that they are saving an "Ignore" point and ask
   *                          whether they would like to set Ignore=false. This
   *                          emits an ignoreChanged() signal so the "Ignore" box
   *                          in the window is unchecked.
   *   @history 2008-12-30 Jeannie Walldren - Modified to set measures in
   *                          viewports to Ignore=False if when saving, the user
   *                          chooses to set a point's Ignore=False. Replaced
   *                          reference to ignoreChanged() with
   *                          ignorePointChanged().
   *   @history 2008-12-31 Jeannie Walldren - Added question box to ask user
   *                          whether the current reference measure should be
   *                          replaced with the measure in the left viewport.
   *   @history 2010-01-27 Jeannie Walldren - Added question box to warn the user
   *                          that they are saving an "Ignore" measure and ask
   *                          whether they would like to set Ignore=False. This
   *                          emits an ignoreRightChanged() signal so the "Ignore"
   *                          box in the window is unchecked. Modified Ignore
   *                          Point message for clarity.
   *   @history 2010-11-19 Tracie Sucharski - Renamed from pointSaved.
   *   @history 2011-03-03 Tracie Sucharski - Do not save left measure unless
   *                          the ignore flag was changed, that is the only
   *                          change allowed on the left measure.
   *   @history 2011-04-20 Tracie Sucharski - If left measure equals right
   *                          measure, copy right into left.  Also if EditLock
   *                          true and user does not want to change, then
   *                          do not save measure.  Remove signals
   *                          EditPointChanged and netChanged, since these
   *                          should only happen when the point is saved.
   *   @history 2011-07-01 Tracie Sucharski - Fixed bug where the edit measure
   *                          EditLocked=True, but the original measure was
   *                          False, and we woouldn't allow the measure to be
   *                          saved.
   *   @history 2011-07-25 Tracie Sucharski - Removed editPointChanged signal
   *                          since the editPoint is not changed.  This helped
   *                          with match windows blinking due to refresh.
   *   @history 2011-09-22 Tracie Sucharski - When checking ignore status
   *                          on right measure, check both original and edit
   *                          measure.
   *   @history 2012-04-09 Tracie Sucharski - When checking if left measure
   *                          editLock has changed, use measure->IsEditLocked()
   *                          instead of this classes IsMeasureLocked() because
   *                          it checks the m_editPoint measure instead of
   *                          the measure loaded into the point editor.
   *   @history 2012-04-26 Tracie Sucharski - cleaned up, moved reference checking
   *                          and updating ground surface point to new methods.
   *   @history 2012-05-07 Tracie Sucharski - Removed code to re-load left measure if
   *                          left and right are the same, this is already handled in
   *                          ControlPointEdit::saveMeasure.
   *   @history 2012-06-12 Tracie Sucharski - Change made on 2012-04-26 caused a bug where
   *                          if no ground is loaded the checkReference was not being called and
   *                          reference measure could not be changed and there was no warning
   *                          printed.
   */
  void MatchTool::measureSaved() {

    // Read original measures from the network for comparison with measures
    // that have been edited
    ControlMeasure *origLeftMeasure =
                m_editPoint->GetMeasure(m_leftMeasure->GetCubeSerialNumber());
    ControlMeasure *origRightMeasure =
                m_editPoint->GetMeasure(m_rightMeasure->GetCubeSerialNumber());
    //  Neither measure has changed, return
    if (*origLeftMeasure == *m_leftMeasure && *origRightMeasure == *m_rightMeasure) {
      return;
    }

    if (m_editPoint->IsIgnored()) {
      QString message = "You are saving changes to a measure on an ignored ";
      message += "point.  Do you want to set Ignore = False on the point and ";
      message += "both measures?";
      switch (QMessageBox::question(m_matchTool, "Match Tool Save Measure",
                                    message, "&Yes", "&No", 0, 0)) {
        // Yes:  set Ignore=false for the point and measures and save point
        case 0:
          m_editPoint->SetIgnored(false);
          emit ignorePointChanged();
          if (m_leftMeasure->IsIgnored()) {
            m_leftMeasure->SetIgnored(false);
            emit ignoreLeftChanged();
          }
          if (m_rightMeasure->IsIgnored()) {
            m_rightMeasure->SetIgnored(false);
            emit ignoreRightChanged();
          }
        // No: keep Ignore=true and save measure
        case 1:
          break;
      }
    }

    bool savedAMeasure = false;
    //  Error check both measures for edit lock, ignore status and reference
    bool leftChangeOk = validateMeasureChange(m_leftMeasure);
    if (leftChangeOk) {
      m_leftMeasure->SetChooserName(Application::UserName());
      *origLeftMeasure = *m_leftMeasure;
      savedAMeasure = true;
    }
    bool rightChangeOk = validateMeasureChange(m_rightMeasure);
    if (rightChangeOk) {
      m_rightMeasure->SetChooserName(Application::UserName());
      *origRightMeasure = *m_rightMeasure;
      savedAMeasure = true;
    }

    // If left measure == right measure, update left
    if (m_leftMeasure->GetCubeSerialNumber() == m_rightMeasure->GetCubeSerialNumber()) {
      *m_leftMeasure = *m_rightMeasure;
      //  Update left measure of pointEditor
      m_pointEditor->setLeftMeasure (m_leftMeasure, m_leftCube,
                                     m_editPoint->GetId());
    }

    //  Change Save Point button text to red
    if (savedAMeasure) {
      colorizeSaveButton();
    }

    emit editPointChanged();

    // Update measure info
    updateLeftMeasureInfo();
    updateRightMeasureInfo();
    loadMeasureTable();

  }



  bool MatchTool::validateMeasureChange(ControlMeasure *m) {


    // Read original measures from the network for comparison with measures
    // that have been edited
    ControlMeasure *origMeasure =
                m_editPoint->GetMeasure(m->GetCubeSerialNumber());

    //  If measure hasn't changed, return false, to keep original

    if (*m == *origMeasure) return false;

    // Is measure on Left or Right?  This is needed to print correct information
    // to users in identifying the measure and for updating information widgets.
    QString side = "right";
    if (m->GetCubeSerialNumber() == m_leftMeasure->GetCubeSerialNumber()) {
      side = "left";
    }

    //  Only print error if both original measure in network and the current
    //  edit measure are both editLocked and measure has changed.  If only the edit measure is
    //  locked, then user just locked and it needs to be saved.
    //  Do not use this classes IsMeasureLocked since we actually want to
    //  check the original againsted the edit measure and we don't care
    //  if this is a reference measure.  The check for moving a reference is
    //  done below.
    if (origMeasure->IsEditLocked() && m->IsEditLocked()) {
      QString message = "The " + side + " measure is editLocked ";
      message += "for editing.  Do you want to set EditLock = False for this ";
      message += "measure?";
      int response = QMessageBox::question(m_matchTool, "Match Tool Save Measure",
                                    message, QMessageBox::Yes | QMessageBox::No);
      // Yes:  set EditLock=false for the right measure
      if (response == QMessageBox::Yes) {
        m->SetEditLock(false);
        if (side == "left") {
          m_lockLeftMeasure->setChecked(false);
        }
        else {
          m_lockRightMeasure->setChecked(false);
        }
      }
      // No:  keep EditLock=true and do NOT save measure
      else {
        return false;
      }
    }

    if (origMeasure->IsIgnored() && m->IsIgnored()) {
      QString message = "The " + side + "measure is ignored.  ";
      message += "Do you want to set Ignore = False on the measure?";
      switch(QMessageBox::question(m_matchTool, "Match Tool Save Measure",
                                   message, "&Yes", "&No", 0, 0)){
        // Yes:  set Ignore=false for the right measure and save point
        case 0:
            m->SetIgnored(false);
            if (side == "left") {
              emit ignoreLeftChanged();
            }
            else {
              emit ignoreRightChanged();
            }
        // No:  keep Ignore=true and save point
        case 1:
          break;;
      }
    }

    //  If measure is explicit reference and it has moved,warn user
    ControlMeasure *refMeasure = m_editPoint->GetRefMeasure();
    if (m_editPoint->IsReferenceExplicit()) {
      if (refMeasure->GetCubeSerialNumber() == m->GetCubeSerialNumber()) {
        if (m->GetSample() != origMeasure->GetSample() || m->GetLine() != origMeasure->GetLine()) {
          QString message = "You are making a change to the reference measure.  You ";
          message += "may need to move all of the other measures to match the new ";
          message += " coordinate of the reference measure.  Do you really want to ";
          message += " change the reference measure's location? ";
          switch(QMessageBox::question(m_matchTool, "Match Tool Save Measure",
                                       message, "&Yes", "&No", 0, 0)){
            // Yes:  Save measure
            case 0:
              break;
            // No:  keep original reference, return without saving
            case 1:
              loadPoint();
              return false;
          }
        }
      }
      //  New reference measure
      else if (side == "left" && (refMeasure->GetCubeSerialNumber() != m->GetCubeSerialNumber())) {
        if (m_coregNet) {
          QString message = "This control network was created by the <i>coreg</i> program, and the "
                            "reference measure needs to remain the same as what <i>coreg</i> set.  "
                            "Therefore, you cannot change which measure is the reference.  To "
                            "save this point, move the reference measure (measure in BOLD) back "
                            "to the left side.";
          QMessageBox::information(m_matchTool, "Cannot change reference", message);
        }
        else {
          QString message = "This point already contains a reference measure.  ";
          message += "Would you like to replace it with the measure on the left?";
          int  response = QMessageBox::question(m_matchTool,
                                    "Match Tool Save Measure", message,
                                    QMessageBox::Yes | QMessageBox::No,
                                    QMessageBox::Yes);
          // Replace reference measure
          if (response == QMessageBox::Yes) {
            //  Update measure file combo boxes:  old reference normal font,
            //    new reference bold font
            QString file = serialNumberList().fileName(m_leftMeasure->GetCubeSerialNumber());
            QString fname = FileName(file).name();
            int iref = m_leftCombo->findText(fname);

            //  Save normal font from new reference measure
            QVariant font = m_leftCombo->itemData(iref,Qt::FontRole);
            m_leftCombo->setItemData(iref,QFont("DejaVu Sans", 12, QFont::Bold), Qt::FontRole);
            iref = m_rightCombo->findText(fname);
            m_rightCombo->setItemData(iref,QFont("DejaVu Sans", 12, QFont::Bold), Qt::FontRole);

            file = serialNumberList().fileName(refMeasure->GetCubeSerialNumber());
            fname = FileName(file).name();
            iref = m_leftCombo->findText(fname);
            m_leftCombo->setItemData(iref,font,Qt::FontRole);
            iref = m_rightCombo->findText(fname);
            m_rightCombo->setItemData(iref,font,Qt::FontRole);

            m_editPoint->SetRefMeasure(m->GetCubeSerialNumber());
          }
        }
      }
    }
    else {
      //  No explicit reference, If left, set explicit reference
      if (side == "left") {
        m_editPoint->SetRefMeasure(m->GetCubeSerialNumber());
      }
    }

    //  All test pass, return true (ok to change measure)
    return true;


  }



  /*
  * Change which measure is the reference.
  *
  * @author 2012-04-26 Tracie Sucharski - moved funcitonality from measureSaved
  *
  * @internal
  *   @history 2012-06-12 Tracie Sucharski - Moved check for ground loaded on left from the
  *                          measureSaved method.
  */
  void MatchTool::checkReference() {

    // Check if ControlPoint has reference measure, if reference Measure is
    // not the same measure that is on the left chip viewport, set left
    // measure as reference.
    ControlMeasure *refMeasure = m_editPoint->GetRefMeasure();
    if (refMeasure->GetCubeSerialNumber() != m_leftMeasure->GetCubeSerialNumber()) {
      QString message = "This point already contains a reference measure.  ";
      message += "Would you like to replace it with the measure on the left?";
      int  response = QMessageBox::question(m_matchTool,
                                "Match Tool Save Measure", message,
                                QMessageBox::Yes | QMessageBox::No,
                                QMessageBox::Yes);
      // Replace reference measure
      if (response == QMessageBox::Yes) {
        //  Update measure file combo boxes:  old reference normal font,
        //    new reference bold font
        QString file = serialNumberList().fileName(m_leftMeasure->GetCubeSerialNumber());
        QString fname = FileName(file).name();
        int iref = m_leftCombo->findText(fname);

        //  Save normal font from new reference measure
        QVariant font = m_leftCombo->itemData(iref,Qt::FontRole);
        m_leftCombo->setItemData(iref,QFont("DejaVu Sans", 12, QFont::Bold), Qt::FontRole);
        iref = m_rightCombo->findText(fname);
        m_rightCombo->setItemData(iref,QFont("DejaVu Sans", 12, QFont::Bold), Qt::FontRole);

        file = serialNumberList().fileName(refMeasure->GetCubeSerialNumber());
        fname = FileName(file).name();
        iref = m_leftCombo->findText(fname);
        m_leftCombo->setItemData(iref,font,Qt::FontRole);
        iref = m_rightCombo->findText(fname);
        m_rightCombo->setItemData(iref,font,Qt::FontRole);

        m_editPoint->SetRefMeasure(m_leftMeasure->GetCubeSerialNumber());
      }

          // ??? Need to set rest of measures to Candiate and add more warning. ???//
    }


  }



  /**
   * Save edit point to the Control Network.  Up to this point the point is
   * simply a copy and does not exist in the network.
   *
   * @author 2010-11-19 Tracie Sucharski
   *
   * @internal
   * @history 2011-04-20 Tracie Sucharski - If EditLock set, prompt for changing
   *                        and do not save point if editLock not changed.
   * @history 2011-07-05 Tracie Sucharski - Move point EditLock error checking
   *                        to individual point parameter setting methods, ie.
   *                        SetPointType, SetIgnorePoint.
   *
   */
  void MatchTool::savePoint () {

    //  Make a copy of edit point for updating the control net since the edit
    //  point is still loaded in the point editor.
    ControlPoint *updatePoint = new ControlPoint;
    *updatePoint = *m_editPoint;

    //  If edit point exists in the network, save the updated point.  If it
    //  does not exist, add it.
    if (m_controlNet->ContainsPoint(updatePoint->GetId())) {
      ControlPoint *p;
      p = m_controlNet->GetPoint(QString(updatePoint->GetId()));
      *p = *updatePoint;
      delete updatePoint;
      updatePoint = NULL;
    }
    else {
      m_controlNet->AddPoint(updatePoint);
    }

    //  Change Save Measure button text back to default
    m_savePoint->setPalette(m_saveDefaultPalette);

    // emit signal so the nav tool can update edit point
    emit editPointChanged();
    // At exit, or when opening new net, use for prompting user for a save
    m_netChanged = true;
    //   Refresh chipViewports to show new positions of controlPoints
    m_pointEditor->refreshChips();
  }



  /**
   * Set point's "EditLock" keyword to the value of the input parameter.
   * @param ignore Boolean value that determines the EditLock value for this
   *               point.
   *
   * @author 2011-03-07 Tracie Sucharski
   *
   * @internal
   */
  void MatchTool::setLockPoint (bool lock) {
    if (m_editPoint == NULL) return;

    m_editPoint->SetEditLock(lock);
    colorizeSaveButton();
  }



  /**
   * Set point's "Ignore" keyword to the value of the input
   * parameter.
   * @param ignore Boolean value that determines the Ignore value for this point.
   *
   * @internal
   * @history 2010-12-15 Tracie Sucharski - Remove netChanged, the point is
   *                        not changed in the net unless "Save Point" is
   *                        selected.
   */
  void MatchTool::setIgnorePoint (bool ignore) {
    if (m_editPoint == NULL) return;

    ControlPoint::Status status = m_editPoint->SetIgnored(ignore);
    if (status == ControlPoint::PointLocked) {
      m_ignorePoint->setChecked(m_editPoint->IsIgnored());
      QString message = "Unable to change Ignored on point.  Set EditLock ";
      message += " to False.";
      QMessageBox::critical(m_matchTool, "Error", message);
      return;
    }
    colorizeSaveButton();
  }




  /**
   * Set the "EditLock" keyword of the measure shown in the left viewport to the
   * value of the input parameter.
   *
   * @param ignore Boolean value that determines the EditLock value for the left
   *               measure.
   *
   * @author 2011-03-07 Tracie Sucharski
   *
   * @internal
   * @history 2011-06-27 Tracie Sucharski - emit signal indicating a measure
   *                        parameter has changed.
   * @history 2012-04-16 Tracie Sucharski - When attempting to un-lock a measure
   *                        print error if point is locked.
   */
  void MatchTool::setLockLeftMeasure (bool lock) {

    if (m_editPoint->IsEditLocked()) {
      m_lockLeftMeasure->setChecked(m_leftMeasure->IsEditLocked());
      QMessageBox::warning(m_matchTool, "Point Locked","Point is Edit Locked.  You must un-lock point"
        " before changing a measure.");
      m_lockLeftMeasure->setChecked(m_leftMeasure->IsEditLocked());
      return;
    }

    if (m_leftMeasure != NULL) m_leftMeasure->SetEditLock(lock);

    //  If the right chip is the same as the left chip , update the right editLock
    //  box.
    if (m_rightMeasure != NULL) {
      if (m_rightMeasure->GetCubeSerialNumber() == m_leftMeasure->GetCubeSerialNumber()) {
        m_rightMeasure->SetEditLock(lock);
        m_lockRightMeasure->setChecked(lock);
      }
    }
    emit measureChanged();
  }


  /**
   * Set the "Ignore" keyword of the measure shown in the left
   * viewport to the value of the input parameter.
   *
   * @param ignore Boolean value that determines the Ignore value for the left
   *               measure.
   * @internal
   * @history 2010-01-27 Jeannie Walldren - Fixed bug that resulted in segfault.
   *                          Moved the check whether m_rightMeasure is null
   *                          before the check whether m_rightMeasure equals
   *                          m_leftMeasure.
   * @history 2010-12-15 Tracie Sucharski - Remove netChanged, the point is
   *                        not changed in the net unless "Save Point" is
   *                        selected.
   * @history 2011-06-27 Tracie Sucharski - emit signal indicating a measure
   *                        parameter has changed.
   */
  void MatchTool::setIgnoreLeftMeasure (bool ignore) {
    if (m_leftMeasure != NULL) m_leftMeasure->SetIgnored(ignore);

    //  If the right chip is the same as the left chip , update the right
    //  ignore box.
    if (m_rightMeasure != NULL) {
      if (m_rightMeasure->GetCubeSerialNumber() == m_leftMeasure->GetCubeSerialNumber()) {
        m_rightMeasure->SetIgnored(ignore);
        m_ignoreRightMeasure->setChecked(ignore);
      }
    }
    emit measureChanged();
  }


  /**
   * Set the "EditLock" keyword of the measure shown in the right viewport to
   * the value of the input parameter.
   *
   * @param ignore Boolean value that determines the EditLock value for the
   *               right measure.
   *
   * @author 2011-03-07 Tracie Sucharski
   *
   * @internal
   * @history 2011-06-27 Tracie Sucharski - emit signal indicating a measure
   *                        parameter has changed.
   * @history 2012-04-16 Tracie Sucharski - When attempting to un-lock a measure
   *                        print error if point is locked.
   */
  void MatchTool::setLockRightMeasure (bool lock) {

    if (m_editPoint->IsEditLocked()) {
      m_lockRightMeasure->setChecked(m_rightMeasure->IsEditLocked());
      QMessageBox::warning(m_matchTool, "Point Locked","Point is Edit Locked.  You must un-lock point"
        " before changing a measure.");
      m_lockRightMeasure->setChecked(m_rightMeasure->IsEditLocked());
      return;
    }

    if (m_rightMeasure != NULL) m_rightMeasure->SetEditLock(lock);

    //  If the left chip is the same as the right chip , update the left editLock box.
    if (m_leftMeasure != NULL) {
      if (m_leftMeasure->GetCubeSerialNumber() == m_rightMeasure->GetCubeSerialNumber()) {
        m_leftMeasure->SetEditLock(lock);
        m_lockLeftMeasure->setChecked(lock);
      }
    }
    emit measureChanged();
  }


  /**
   * Set the "Ignore" keyword of the measure shown in the right
   * viewport to the value of the input parameter.
   *
   * @param ignore Boolean value that determines the Ignore value for the right
   *               measure.
   * @internal
   * @history 2010-01-27 Jeannie Walldren - Fixed bug that resulted in segfault.
   *                          Moved the check whether m_leftMeasure is null before
   *                          the check whether m_rightMeasure equals
   *                          m_leftMeasure.
   * @history 2010-12-15 Tracie Sucharski - Remove netChanged, the point is
   *                        not changed in the net unless "Save Point" is
   *                        selected.
   * @history 2011-06-27 Tracie Sucharski - emit signal indicating a measure
   *                        parameter has changed.
   */
  void MatchTool::setIgnoreRightMeasure (bool ignore) {
    if (m_rightMeasure != NULL) m_rightMeasure->SetIgnored(ignore);

    //  If the right chip is the same as the left chip , update the right
    //  ignore blox.
    if (m_leftMeasure != NULL) {
      if (m_rightMeasure->GetCubeSerialNumber() == m_leftMeasure->GetCubeSerialNumber()) {
        m_leftMeasure->SetIgnored(ignore);
        m_ignoreLeftMeasure->setChecked(ignore);
      }
    }
    emit measureChanged();
  }



  void MatchTool::openNet() {

    if (m_controlNet) {
      if (m_controlNet->GetNumPoints() != 0 && m_netChanged) {
        QString message = "A control net has already been created.  Do you want to save before "
                          "opening a new control net?";
        int response = QMessageBox::question(m_matchTool, "Save current control net?",
                                             message,
                                             QMessageBox::Yes | QMessageBox::No,
                                             QMessageBox::Yes);
        // Yes:  Save old net, so return without opening network.
        if (response == QMessageBox::Yes) {
          saveAsNet();
        }
        m_matchTool->setVisible(false);
      }
      delete m_controlNet;
      m_controlNet = NULL;
      m_editPoint = NULL;
      m_newPoint = NULL;
      m_newPointDialog = NULL;
    }

    // At exit, or when opening new net, use for prompting user for a save
    m_netChanged = false;

    QApplication::restoreOverrideCursor();
    QString filter = "Control net (*.net *.cnet *.ctl);;";
    filter += "Pvl file (*.pvl);;";
    filter += "Text file (*.txt);;";
    filter += "All (*)";
    m_cnetFileName = QFileDialog::getOpenFileName((QWidget *)parent(),
                                                "Select a control network",
                                                ".",
                                                filter);
    QApplication::setOverrideCursor(Qt::WaitCursor);
    if (!m_cnetFileName.isEmpty()) {
      try {
        Progress progress;
        m_controlNet = new ControlNet(m_cnetFileName, &progress);
        m_coregNet = false;
        m_coregReferenceSN = "";
        if (m_controlNet->GetNetworkId() == "Coreg") {
          m_coregNet = true;
          //  Find reference image of first point, the rest of the points will have the same
          //  reference.  When creating new point, use the same reference.
          m_coregReferenceSN = m_controlNet->GetPoint(0)->GetReferenceSN();
        }
      }
      catch (IException &e) {
        QApplication::restoreOverrideCursor();
        QString message = "Invalid control network.  \n";
        message += e.toString();
        QMessageBox::critical(m_matchTool, "Error", message);
        m_cnetFileName.clear();
        delete m_controlNet;
        m_controlNet = NULL;
        return;
      }
    }
    QApplication::restoreOverrideCursor();
    m_matchTool->setWindowTitle("Match Tool - Control Network File: " + m_cnetFileName);
    m_cnetFileNameLabel->setText("Control Network: " + m_cnetFileName);

    paintAllViewports();
  }



  /**
   * Signal to save control net
   *
   * @author 2011-10-31 Tracie Sucharski
   */
  void MatchTool::saveNet() {
    if (m_cnetFileName.isEmpty()) {
      QString message = "This is a new network, you must select "
                        "\"Save As\" under the File Menu or on the toolbar.";
      QMessageBox::critical(m_matchTool, "Error", message);
      return;
    }
    try {
      m_controlNet->Write(m_cnetFileName);
      m_netChanged = false;
    }
    catch (IException &e) {
      QMessageBox::critical(m_matchTool, tr("Error Writing Control Net"), e.what());
      return;
    }
  }



  /**
   * Signal to save the control net.
   *
   * @internal
   * @history 2017-11-22 Adam Goins - Set the MatchTool window title and
   *                         the CNet file name label to the newly saved file.
   *                         Fixes #3922.
   */
  void MatchTool::saveAsNet() {

    QString fn = QFileDialog::getSaveFileName(m_matchTool,
                 "Choose filename to save under",
                 ".",
                 "Control Files (*.net)");

    //Make sure the filename is valid
    if(!fn.isEmpty()) {
      try {
        m_controlNet->Write(fn);
        m_netChanged = false;
      }
      catch (IException &e) {
        QMessageBox::critical(m_matchTool, tr("Error Writing Control Net"), e.what());
        return;
      }
      m_cnetFileName = fn;
      m_matchTool->setWindowTitle("Match Tool - Control Network File: " + m_cnetFileName);
      m_cnetFileNameLabel->setText("Control Network: " + m_cnetFileName);
    }
    //The user cancelled, or the filename is empty
    else {
      return;
    }

  }



  /**
   * Handle mouse events on CubeViewport
   *
   * @param p[in]   (QPoint)            Point under cursor in cubeviewport
   * @param s[in]   (Qt::MouseButton)   Which mouse button was pressed
   *
   * @internal
   * @history  2007-06-12 Tracie Sucharski - Swapped left and right mouse
   *                           button actions.
   * @history  2009-06-08 Tracie Sucharski - Add error checking for editing
   *                           or deleting points when no point exists.
   * @history 2010-06-03 Jeannie Walldren - Removed "std::" since "using
   *                          namespace std"
   * @history 2010-11-19 Tracie Sucharski - Make a copy of the point to be
   *                          edited or added.  Do not add to control
   *                          network until user selects
   *                          "Save Point To Control Network".
   * @history 2012-01-11 Tracie Sucharski - Add error check for invalid lat, lon
   *                          when creating new control point.
   * @history 2012-05-08 Tracie Sucharski - Clear m_leftFile, only set if creating
   *                          new point. Change m_leftFile from a QString to
   *                          a QString.
   * @history 2012-10-02 Tracie Sucharski - The member variable leftFile was never set. It is now
   *                          set when a new point is created and cleared after all measures have
   *                          been selected.
   *
   */
  void MatchTool::mouseButtonRelease(QPoint p, Qt::MouseButton s) {
    MdiCubeViewport *mvp = cubeViewport();
    if (mvp  == NULL) return;

    QString file = mvp->cube()->fileName();
    QString sn = serialNumberList().serialNumber(file);

    double samp,line;
    mvp->viewportToCube(p.x(),p.y(),samp,line);

    if (s == Qt::LeftButton) {

      if (!m_controlNet || m_controlNet->GetNumPoints() == 0) {
        QString message = "No points exist for editing.  Create points ";
        message += "using the right mouse button.";
        QMessageBox::warning(m_matchTool, "Warning", message);
        return;
      }

      //  Find closest control point in network
      QString sn = serialNumberList().serialNumber(file);
      ControlPoint *point = NULL;
      try {
        point = m_controlNet->FindClosest(sn, samp, line);
      }
      catch (IException &e) {
        QString message = "Cannot find point for editing.";
        message += e.toString();
        QMessageBox::warning(m_matchTool, "Warning", message);
        return;
      }

      modifyPoint(point);
    }
    else if (s == Qt::MiddleButton) {
      if (!m_controlNet || m_controlNet->GetNumPoints() == 0) {
        QString message = "No points exist for deleting.  Create points ";
        message += "using the right mouse button.";
        QMessageBox::warning(m_matchTool, "Warning", message);
        return;
      }

      //  Find closest control point in network
      ControlPoint *point = m_controlNet->FindClosest(sn, samp, line);

      if (point == NULL) {
        QString message = "No points exist for deleting.  Create points ";
        message += "using the right mouse button.";
        QMessageBox::warning(m_matchTool, "Warning", message);
        return;
      }

      deletePoint(point);
    }
    else if (s == Qt::RightButton) {
      if (m_newPointDialog) {
        addMeasure(mvp, samp, line);
      }
      else {
        try {
          createPoint(mvp, samp, line);
          m_leftFile = mvp->cube()->fileName();
        }
        catch (IException &e) {
          QString message = "Cannot create control point.\n\n";
          message += e.toString();
          QMessageBox::critical(m_matchTool, "Error", message);
          return;
        }
      }
    }
  }



  QStringList MatchTool::missingCubes(ControlPoint *point) {

    //  Make sure all measures are loaded into viewports, otherwise we cannot edit this point
    QStringList missingCubes;
    for (int i=0; i<point->GetNumMeasures(); i++) {
      ControlMeasure &m = *(*point)[i];
      if (!serialNumberList().hasSerialNumber(m.GetCubeSerialNumber())) {
        missingCubes << m.GetCubeSerialNumber();
      }
    }
    return missingCubes;
  }



  /**
   *   Create new control point at given samp, line for viewport
   *
   * @internal
   *
   */
  void MatchTool::createPoint(MdiCubeViewport *cvp, double sample, double line) {

    m_newPointDialog = new MatchToolNewPointDialog(*m_controlNet, m_lastUsedPointId, m_matchTool);
    connect(m_newPointDialog, SIGNAL(measuresFinished()), this, SLOT(doneWithMeasures()));
    connect(m_newPointDialog, SIGNAL(newPointCanceled()), this, SLOT(cancelNewPoint()));

    QStringList images;
    for (int i = 0; i < (int)cubeViewportList()->size(); i++) {
      FileName cubeFile = (*(cubeViewportList()))[i]->cube()->fileName();
      images<<cubeFile.name();
    }
    m_newPointDialog->setFiles(images);
    m_newPointDialog->show();

    //  Highlight the current cubeViewport
    QString current = FileName(cvp->cube()->fileName()).name();
    m_newPointDialog->highlightFile(current);

    m_newPoint = new ControlPoint();
    m_newPoint->SetType(ControlPoint::Free);
    m_newPoint->SetChooserName(Application::UserName());

    ControlMeasure *m = new ControlMeasure;
    m->SetCubeSerialNumber(serialNumber(cvp));
    m->SetCoordinate(sample, line);
    m->SetType(ControlMeasure::Manual);
    m->SetDateTime();
    m->SetChooserName(Application::UserName());
    m_newPoint->Add(m);

    paintAllViewports();
  }



  void MatchTool::addMeasure(MdiCubeViewport *cvp, double sample, double line) {

    //  Highlight the current cubeViewport
    QString current = FileName(cvp->cube()->fileName()).name();
    m_newPointDialog->highlightFile(current);
    m_newPointDialog->raise();

    ControlMeasure *m = new ControlMeasure;
    m->SetCubeSerialNumber(serialNumber(cvp));

    //  If serial number already exists, delete old measure before creating new
    if (m_newPoint->HasSerialNumber(serialNumber(cvp))) {
      m_newPoint->Delete(m_newPoint->GetMeasure(serialNumber(cvp)));
    }
    m->SetCoordinate(sample, line);
    m->SetType(ControlMeasure::Manual);
    m->SetDateTime();
    m->SetChooserName(Application::UserName());
    m_newPoint->Add(m);

    paintAllViewports();
  }



  void MatchTool::doneWithMeasures() {

    m_lastUsedPointId = m_newPointDialog->pointId();
    m_newPoint->SetId(m_lastUsedPointId);
//  //  Add new control point to control network
//  m_controlNet->AddPoint(m_newPoint);
//  //  Read newly added point
//  // TODO  Make sure pt exists
//  m_editPoint = m_controlNet->GetPoint((QString) m_newPoint->GetId());

    //  If this is a coreg network, make sure the reference SN exists in the new point
    // and set it to the reference measure.
    if (m_coregNet) {
      if (!m_newPoint->HasSerialNumber(m_coregReferenceSN)) {
        QString message = "This is a coreg network which needs the cube with serial number " +
          m_coregReferenceSN + " as the reference measure.  This new control point does "
          "not have a measure for that serial number, so this point cannot be created until "
          "the cube listed above is added (Right-click on cube).";
        QMessageBox::critical(m_matchTool, "Error", message);
        m_newPointDialog->show();
        return;
      }
      //  Set the reference measure to match the rest of the points
      m_newPoint->SetRefMeasure(m_newPoint->GetMeasure(m_coregReferenceSN));
    }

    //  If the editPoint has been used, but there is not currently a network, delete the editPoint
    if (m_editPoint != NULL && m_editPoint->Parent() == NULL) {
      delete m_editPoint;
      m_editPoint = NULL;
    }
    m_editPoint = m_newPoint;

    m_newPoint = NULL;
    delete m_newPointDialog;
    m_newPointDialog = NULL;

    //  Load new point in MatchTool
    loadPoint();
    m_matchTool->setVisible(true);
    m_matchTool->raise();

    emit editPointChanged();
    colorizeSaveButton();
  }



  void MatchTool::cancelNewPoint() {

    delete m_newPointDialog;
    m_newPointDialog = NULL;
    delete m_newPoint;
    m_newPoint = NULL;
    m_leftFile.clear();

    paintAllViewports();

  }



  /**
   * Delete control point
   *
   * @param point Pointer to control point (net memory) to be deleted.
   *
   * @internal
   * @history 2010-06-03 Jeannie Walldren - Removed "std::" since "using
   *                           namespace std"
   * @history 2010-07-12 Jeannie Walldren - Fixed bug by setting control point
   *                          to NULL if removed from the control net and check
   *                          for NULL points before emitting editPointChanged
   * @history 2011-04-04 Tracie Sucharski - Move code that was after the exec
   *                          block within, so that if the Cancel button is
   *                          selected, nothing else happens.
   * @history 2011-07-15 Tracie Sucharski - Print info about deleting editLock
   *                          points and reference measures.
   *
   */
  void MatchTool::deletePoint(ControlPoint *point) {

    QStringList mCubes = missingCubes(point);
    if (mCubes.size() > 0) {
      QString msgTitle = "Missing Cubes";
      QString message = "This point is missing cubes for the following measures and cannot be ";
      message += "loaded into the editor. Do you still want to delete this point?\n\n";
      for (int i=0; i<mCubes.size(); i++) {
        message += mCubes.at(i) + "\n";
      }
      QMessageBox msgBox(QMessageBox::Critical, msgTitle, message, QMessageBox::NoButton, m_matchTool,
                         Qt::Dialog);
      QPushButton *yesButton = msgBox.addButton("Yes", QMessageBox::AcceptRole);
      QPushButton *noButton = msgBox.addButton("No", QMessageBox::RejectRole);
      msgBox.setDefaultButton(yesButton);
      msgBox.exec();
      if (msgBox.clickedButton() == noButton) {
        return;
      }
      else {
        m_matchTool->setVisible(false);
      }

    }

    // Make a copy and make sure editPoint is a copy (which means it does not
    // have a parent network.
    if (m_editPoint != NULL && m_editPoint->Parent() == NULL) {
      delete m_editPoint;
      m_editPoint = NULL;
    }
    m_editPoint = new ControlPoint;
    *m_editPoint = *point;

    //  No missing cubes, load edit point as usual
    if (mCubes.size() == 0) {
      loadPoint();

      //  Change point in viewport to red so user can see what point they are
      //  about to delete.
      // the nav tool will update edit point
      emit editPointChanged();
    }

    MatchToolDeletePointDialog *deletePointDialog = new MatchToolDeletePointDialog;
    QString CPId = m_editPoint->GetId();
    deletePointDialog->pointIdValue->setText(CPId);
    //  Need all files for this point
    for (int i=0; i<m_editPoint->GetNumMeasures(); i++) {
      ControlMeasure &m = *(*m_editPoint)[i];
      QString file;
      if (serialNumberList().hasSerialNumber(m.GetCubeSerialNumber())) {
        file = serialNumberList().fileName(m.GetCubeSerialNumber());
      }
      else {
        file = m.GetCubeSerialNumber();
      }
      deletePointDialog->fileList->addItem(file);
    }

    if (deletePointDialog->exec()) {

      int numDeleted = deletePointDialog->fileList->selectedItems().count();

      //  Delete entire control point, either through deleteAllCheckBox or all measures selected
      if (deletePointDialog->deleteAllCheckBox->isChecked() ||
          numDeleted == m_editPoint->GetNumMeasures()) {

        //  If all measures being deleted, let user know and give them the option to quit operation
        if (!deletePointDialog->deleteAllCheckBox->isChecked()) {
          QString message = "You have selected all measures in this point to be deleted.  This "
            "control point will be deleted.  Do you want to delete this control point?";
          int  response = QMessageBox::question(m_matchTool,
                                    "Delete control point", message,
                                    QMessageBox::Yes | QMessageBox::No,
                                    QMessageBox::Yes);
          // If No, do nothing
          if (response == QMessageBox::No) {
            return;
          }
        }

        m_matchTool->setVisible(false);
        // remove this point from the control network
        if (m_controlNet->DeletePoint(m_editPoint->GetId()) == ControlPoint::PointLocked) {
          QMessageBox::information(m_matchTool, "EditLocked Point",
              "This point is EditLocked and cannot be deleted.");
          return;
        }
        if (m_editPoint != NULL && m_editPoint->Parent() == NULL) {
          delete m_editPoint;
          m_editPoint = NULL;
        }
      }

      //  Delete specific measures from control point
      else {
        //  Keep track of editLocked measures for reporting
        int lockedMeasures = 0;
        for (int i=0; i<deletePointDialog->fileList->count(); i++) {
          QListWidgetItem *item = deletePointDialog->fileList->item(i);
          if (!item->isSelected()) continue;

          //  Do not delete reference without asking user
          if (m_editPoint->IsReferenceExplicit() &&
                (m_editPoint->GetRefMeasure()->GetCubeSerialNumber() ==
                (*m_editPoint)[i]->GetCubeSerialNumber())) {
            QString message = "You are trying to delete the Reference measure."
                "  Do you really want to delete the Reference measure?";
            switch (QMessageBox::question(m_matchTool,
                                          "Delete Reference measure?", message,
                                          "&Yes", "&No", 0, 0)) {
              //  Yes:  skip to end of switch to delete the measure
              case 0:
                break;
              //  No:  continue to next measure in the loop
              case 1:
                //  if only a single measure and it's reference and user chooses not to delete,
                //  simply return.  The point has not changed.
                if (numDeleted == 1) {
                  return;
                }
                continue;
            }
          }

          if (m_editPoint->Delete(i) == ControlMeasure::MeasureLocked) {
            lockedMeasures++;
          }
        }

        if (lockedMeasures > 0) {
          QMessageBox::information(m_matchTool,"EditLocked Measures",
                QString::number(lockedMeasures) + " / "
                + QString::number(
                  deletePointDialog->fileList->selectedItems().size()) +
                " measures are EditLocked and were not deleted.");
        }

        if (mCubes.size() == 0) {
          loadPoint();
          m_matchTool->setVisible(true);
          m_matchTool->raise();

          loadTemplateFile(
              m_pointEditor->templateFileName());
        }
        //  Since the delete point is not loaded into the editor for saving by the user, we need
        //  to save the point.
        else {
          ControlPoint *p = m_controlNet->GetPoint(QString(m_editPoint->GetId()));
          *p = *m_editPoint;

          if (m_editPoint != NULL && m_editPoint->Parent() == NULL) {
            delete m_editPoint;
            m_editPoint = NULL;
          }
        }
      }

      // emit a signal to alert user to save when exiting
      // At exit, or when opening new net, use for prompting user for a save
      m_netChanged = true;

      if (m_editPoint) {
        colorizeSaveButton();
      }
      emit editPointChanged();
    }
  }



  /**
   * Modify control point
   *
   * @param point Pointer to control point to be modified.
   *
   * @history 2009-09-15 Tracie Sucharski - Add error check for points
   *                       with no measures.
   */
  void MatchTool::modifyPoint(ControlPoint *point) {

    //  If no measures, print info and return
    if (point->GetNumMeasures() == 0) {
      QString message = "This point has no measures.";
      QMessageBox::warning(m_matchTool, "Warning", message);
      emit editPointChanged();
      return;
    }

    //  Make sure all measures have a cube loaded
    QStringList mCubes = missingCubes(point);
    if (mCubes.size() > 0) {
      QString msgTitle = "Missing Cubes";
      QString message = "This point is missing cubes and cannot be loaded into the editor. Open ";
      message += "the cubes for the following measures before selecting this point.\n\n";
      for (int i=0; i<mCubes.size(); i++) {
        message += mCubes.at(i) + "\n";
      }
      QMessageBox msgBox(QMessageBox::Critical, msgTitle, message, QMessageBox::NoButton, m_matchTool,
                         Qt::Dialog);
      msgBox.exec();
      return;
    }

    //  Make a copy of point for editing, first make sure memory not already
    //  allocated
    if (m_editPoint != NULL && m_editPoint->Parent() == NULL) {
      delete m_editPoint;
      m_editPoint = NULL;
    }
    m_editPoint = new ControlPoint;
    *m_editPoint = *point;

    loadPoint();
    m_matchTool->setVisible(true);
    m_matchTool->raise();
    loadTemplateFile(
        m_pointEditor->templateFileName());

    // emit signal so the nav tool can update edit point
    emit editPointChanged();

    // New point loaded, make sure Save Measure Button text is default
    m_savePoint->setPalette(m_saveDefaultPalette);
  }



  /**
   * @brief Load point into MatchTool.
   * @internal
   *   @history 2008-11-26  Jeannie Walldren - Added "Number of Measures" to
   *                           MatchTool point information.
   *   @history 2010-06-03 Jeannie Walldren - Removed "std::" since "using
   *                          namespace std"
   *   @history 2010-10-29 Tracie Sucharski - Changed pointfiles to QStringList
   *   @history 2011-04-20 Tracie Sucharski - Was not setting EditLock check box
   *   @history 2011-07-18 Tracie Sucharski - Fixed bug with loading
   *                          ground measure-use AprioriSurface point, not lat,lon
   *                          of reference measure unless there is no apriori
   *                          surface point.
   *   @history 2012-05-08 Tracie Sucharski - m_leftFile changed from QString to QString.
   *   @history 2012-10-02 Tracie Sucharski - When creating a new point, load the cube the user
   *                          clicked on first on the left side, use m_leftFile.
   */
  void MatchTool::loadPoint () {

    //  Write pointId
    QString CPId = m_editPoint->GetId();
    QString ptId("Point ID:  ");
    ptId += (QString) CPId;
    m_ptIdValue->setText(ptId);

    //  Write number of measures
    QString ptsize = "Number of Measures:  " +
                   QString::number(m_editPoint->GetNumMeasures());
    m_numMeasures->setText(ptsize);

    //  Set EditLock box correctly
    m_lockPoint->setChecked(m_editPoint->IsEditLocked());

    //  Set ignore box correctly
    m_ignorePoint->setChecked(m_editPoint->IsIgnored());

    // Clear combo boxes
    m_leftCombo->clear();
    m_rightCombo->clear();
    m_pointFiles.clear();

    //  Need all files for this point
    for (int i=0; i<m_editPoint->GetNumMeasures(); i++) {
      ControlMeasure &m = *(*m_editPoint)[i];
      QString file = serialNumberList().fileName(m.GetCubeSerialNumber());
      m_pointFiles<<file;
      QString tempFileName = FileName(file).name();
      m_leftCombo->addItem(tempFileName);
      m_rightCombo->addItem(tempFileName);
      if (m_editPoint->IsReferenceExplicit() &&
          (QString)m.GetCubeSerialNumber() == m_editPoint->GetReferenceSN()) {
        m_leftCombo->setItemData(i,QFont("DejaVu Sans", 12, QFont::Bold), Qt::FontRole);
        m_rightCombo->setItemData(i,QFont("DejaVu Sans", 12, QFont::Bold), Qt::FontRole);
      }
    }

    //  TODO:  WHAT HAPPENS IF THERE IS ONLY ONE MEASURE IN THIS CONTROLPOINT??
    // Assuming combo loaded in same order as measures in the control point-is
    // this a safe assumption???
    //
    //  Find the file from the cubeViewport that was originally used to select
    //  the point, this will be displayed on the left ChipViewport, unless the
    //  point was selected on the ground source image.  In this case, simply
    //  load the first measure on the left.
    int leftIndex = 0;
    int rightIndex = 0;
    //  Check for reference
    if (m_editPoint->IsReferenceExplicit()) {
      leftIndex = m_editPoint->IndexOfRefMeasure();
    }
    else {
      if (!m_leftFile.isEmpty()) {
        leftIndex = m_leftCombo->findText(FileName(m_leftFile).name());
        //  Sanity check
        if (leftIndex < 0 ) leftIndex = 0;
        m_leftFile.clear();
      }
    }

    if (leftIndex == 0) {
      rightIndex = 1;
    }
    else {
      rightIndex = 0;
    }

    //  Handle pts with a single measure, for now simply put measure on left/right
    //  Evenutally put on left with black on right??
    if (rightIndex > m_editPoint->GetNumMeasures()-1) rightIndex = 0;
    m_rightCombo->setCurrentIndex(rightIndex);
    m_leftCombo->setCurrentIndex(leftIndex);
    //  Initialize pointEditor with measures
    selectLeftMeasure(leftIndex);
    selectRightMeasure(rightIndex);

    loadMeasureTable();
  }



  /**
   * @brief Load measure information into the measure table
   * @internal
   *   @history 2011-12-05 Tracie Sucharski - Turn off sorting until table
   *                          is loaded.
   *
   */
  void MatchTool::loadMeasureTable () {
    if (m_measureWindow == NULL) {
      m_measureWindow = new QMainWindow(m_parent);
      m_measureTable = new QTableWidget();
      m_measureTable->setMinimumWidth(1600);
      m_measureTable->setAlternatingRowColors(true);
      m_measureWindow->setCentralWidget(m_measureTable);
    }
    else {
      m_measureTable->clear();
      m_measureTable->setSortingEnabled(false);
    }
    m_measureTable->setRowCount(m_editPoint->GetNumMeasures());
    m_measureTable->setColumnCount(NUMCOLUMNS);

    QStringList labels;
    for (int i=0; i<NUMCOLUMNS; i++) {
      labels<<measureColumnToString((MeasureColumns)i);
    }
    m_measureTable->setHorizontalHeaderLabels(labels);

    //  Fill in values
    for (int row=0; row<m_editPoint->GetNumMeasures(); row++) {
      int column = 0;
      ControlMeasure &m = *(*m_editPoint)[row];

      QString file = serialNumberList().fileName(m.GetCubeSerialNumber());
      QTableWidgetItem *tableItem = new QTableWidgetItem(QString(file));
      m_measureTable->setItem(row,column++,tableItem);

      tableItem = new QTableWidgetItem(QString(m.GetCubeSerialNumber()));
      m_measureTable->setItem(row,column++,tableItem);

      tableItem = new QTableWidgetItem();
      tableItem->setData(0,m.GetSample());
      m_measureTable->setItem(row,column++,tableItem);

      tableItem = new QTableWidgetItem();
      tableItem->setData(0,m.GetLine());
      m_measureTable->setItem(row,column++,tableItem);

      if (m.GetAprioriSample() == Null) {
        tableItem = new QTableWidgetItem("Null");
      }
      else {
        tableItem = new QTableWidgetItem();
        tableItem->setData(0,m.GetAprioriSample());
      }
      m_measureTable->setItem(row,column++,tableItem);

      if (m.GetAprioriLine() == Null) {
        tableItem = new QTableWidgetItem("Null");
      }
      else {
        tableItem = new QTableWidgetItem();
        tableItem->setData(0,m.GetAprioriLine());
      }
      m_measureTable->setItem(row,column++,tableItem);

      if (m.GetSampleResidual() == Null) {
        tableItem = new QTableWidgetItem(QString("Null"));
      }
      else {
        tableItem = new QTableWidgetItem();
        tableItem->setData(0,m.GetSampleResidual());
      }
      m_measureTable->setItem(row,column++,tableItem);

      if (m.GetLineResidual() == Null) {
        tableItem = new QTableWidgetItem(QString("Null"));
      }
      else {
        tableItem = new QTableWidgetItem();
        tableItem->setData(0,m.GetLineResidual());
      }
      m_measureTable->setItem(row,column++,tableItem);

      if (m.GetResidualMagnitude() == Null) {
        tableItem = new QTableWidgetItem(QString("Null"));
      }
      else {
        tableItem = new QTableWidgetItem();
        tableItem->setData(0,m.GetResidualMagnitude());
      }
      m_measureTable->setItem(row,column++,tableItem);

      double sampleShift = m.GetSampleShift();
      if (sampleShift == Null) {
        tableItem = new QTableWidgetItem(QString("Null"));
      }
      else {
        tableItem = new QTableWidgetItem();
        tableItem->setData(0,sampleShift);
      }
      m_measureTable->setItem(row,column++,tableItem);

      double lineShift = m.GetLineShift();
      if (lineShift == Null) {
        tableItem = new QTableWidgetItem(QString("Null"));
      }
      else {
        tableItem = new QTableWidgetItem();
        tableItem->setData(0,lineShift);
      }
      m_measureTable->setItem(row,column++,tableItem);

      double pixelShift = m.GetPixelShift();
      if (pixelShift == Null) {
        tableItem = new QTableWidgetItem(QString("Null"));
      }
      else {
        tableItem = new QTableWidgetItem();
        tableItem->setData(0,pixelShift);
      }
      m_measureTable->setItem(row,column++,tableItem);

      double goodnessOfFit = m.GetLogData(
                      ControlMeasureLogData::GoodnessOfFit).GetNumericalValue();
      if (goodnessOfFit == Null) {
        tableItem = new QTableWidgetItem(QString("Null"));
      }
      else {
        tableItem = new QTableWidgetItem();
        tableItem->setData(0,goodnessOfFit);
      }
      m_measureTable->setItem(row,column++,tableItem);

      if (m.IsIgnored()) tableItem = new QTableWidgetItem("True");
      if (!m.IsIgnored()) tableItem = new QTableWidgetItem("False");
      m_measureTable->setItem(row,column++,tableItem);

      if (IsMeasureLocked(m.GetCubeSerialNumber()))
        tableItem = new QTableWidgetItem("True");
      if (!IsMeasureLocked(m.GetCubeSerialNumber()))
        tableItem = new QTableWidgetItem("False");
      m_measureTable->setItem(row,column++,tableItem);

      tableItem = new QTableWidgetItem(
                  ControlMeasure::MeasureTypeToString(m.GetType()));
      m_measureTable->setItem(row,column,tableItem);

      //  If reference measure set font on this row to bold
      if (m_editPoint->IsReferenceExplicit() &&
          (QString)m.GetCubeSerialNumber() == m_editPoint->GetReferenceSN()) {
        QFont font;
        font.setBold(true);

        for (int col=0; col<m_measureTable->columnCount(); col++)
          m_measureTable->item(row, col)->setFont(font);
      }

    }

    m_measureTable->resizeColumnsToContents();
    m_measureTable->resizeRowsToContents();
    m_measureTable->setSortingEnabled(true);
    m_measureWindow->show();
  }



  QString MatchTool::measureColumnToString(MatchTool::MeasureColumns column) {
    switch (column) {
      case FILENAME:
        return "FileName";
      case CUBESN:
        return "Serial #";
      case SAMPLE:
        return "Sample";
      case LINE:
        return "Line";
      case SAMPLERESIDUAL:
        return "Sample Residual";
      case LINERESIDUAL:
        return "Line Residual";
      case RESIDUALMAGNITUDE:
        return "Residual Magnitude";
      case SAMPLESHIFT:
        return "Sample Shift";
      case LINESHIFT:
        return "Line Shift";
      case PIXELSHIFT:
        return "Pixel Shift";
      case GOODNESSOFFIT:
        return "Goodness of Fit";
      case IGNORED:
        return "Ignored";
      case EDITLOCK:
        return "Edit Lock";
      case TYPE:
        return "Measure Type";
      case APRIORISAMPLE:
        return "Apriori Sample";
      case APRIORILINE:
        return "Apriori Line";
    }
    throw IException(IException::Programmer,
        "Invalid measure column passed to measureColumnToString", _FILEINFO_);
  }



  /**
   * @brief Selects the next right measure when activated by key shortcut
   *
   * This slot is intended to handle selecting the next right measure when the attached shortcut
   * (PageDown) is activated. This slot checks if the next index is in bounds.
   *
   * @internal
   *   @history 2015-11-08 Ian Humphrey - Created slot. References #2324.
   */
  void MatchTool::nextRightMeasure() {
    int curIndex = m_rightCombo->currentIndex();
    if (curIndex < m_rightCombo->count() - 1) {
      // update the right measure list index and select that measure
      m_rightCombo->setCurrentIndex(curIndex + 1);
      selectRightMeasure(curIndex+1);
    }
  }


  /**
   * @brief Selects the previous right measure when activated by key shortcut
   *
   * This slot is intended to handle selecting the previous right measure when the attached
   * shortcut (PageUp) is activated. This slot checks if the previous index is in bounds.
   *
   * @internal
   *   @history 2015-11-08 Ian Humphrey - Created slot. References #2324.
   */
  void MatchTool::previousRightMeasure() {
    int curIndex = m_rightCombo->currentIndex();
    if (curIndex > 0) {
      // update the right measure list index and select that measure
      m_rightCombo->setCurrentIndex(curIndex - 1);
      selectRightMeasure(curIndex-1);
    }
  }



  /**
   * Select left measure
   *
   * @param index Index of file from the point files vector
   *
   * @internal
   * @history 2010-06-03 Jeannie Walldren - Removed "std::" since "using
   *                          namespace std"
   * @history 2011-07-06 Tracie Sucharski - If point is Locked, and measure is
   *                          reference, lock the measure.
   * @history 2012-10-02 Tracie Sucharski - If measure's cube is not viewed, print error and
   *                          make sure old measure is retained.
   */
  void MatchTool::selectLeftMeasure(int index) {
    QString file = m_pointFiles[index];

    QString serial;
    try {
      serial = serialNumberList().serialNumber(file);
    }
    catch (IException &e) {
      QString message = "Make sure the correct cube is opened.\n\n";
      message += e.toString();
      QMessageBox::critical(m_matchTool, "Error", message);

      //  Set index of combo back to what it was before user selected new.  Find the index
      //  of current left measure.
      QString file = serialNumberList().fileName(m_leftMeasure->GetCubeSerialNumber());
      int i = m_leftCombo->findText(FileName(file).name());
      if (i < 0) i = 0;
      m_leftCombo->setCurrentIndex(i);
      return;
    }

    // Make sure to clear out leftMeasure before making a copy of the selected
    // measure.
    if (m_leftMeasure != NULL) {
      delete m_leftMeasure;
      m_leftMeasure = NULL;
    }
    m_leftMeasure = new ControlMeasure();
    //  Find measure for each file
    *m_leftMeasure = *((*m_editPoint)[serial]);

    //  If m_leftCube is not null, delete before creating new one
    if (m_leftCube != NULL) delete m_leftCube;
    m_leftCube = new Cube();
    m_leftCube->open(file);

    //  Update left measure of pointEditor
    m_pointEditor->setLeftMeasure (m_leftMeasure, m_leftCube,
                                   m_editPoint->GetId());
    updateLeftMeasureInfo ();

  }


  /**
   * Select right measure
   *
   * @param index  Index of file from the point files vector
   *
   * @internal
   * @history 2010-06-03 Jeannie Walldren - Removed "std::" since "using
   *                          namespace std"
   * @history 2012-10-02 Tracie Sucharski - If measure's cube is not viewed, print error and
   *                          make sure old measure is retained.
   */
  void MatchTool::selectRightMeasure(int index) {

    QString file = m_pointFiles[index];

    QString serial;
    try {
      serial = serialNumberList().serialNumber(file);
    }
    catch (IException &e) {
      QString message = "Make sure the correct cube is opened.\n\n";
      message += e.toString();
      QMessageBox::critical(m_matchTool, "Error", message);

      //  Set index of combo back to what it was before user selected new.  Find the index
      //  of current left measure.
      QString file = serialNumberList().fileName(m_rightMeasure->GetCubeSerialNumber());
      int i = m_rightCombo->findText(FileName(file).name());
      if (i < 0) i = 0;
      m_rightCombo->setCurrentIndex(i);
      return;
    }

    // Make sure to clear out rightMeasure before making a copy of the selected
    // measure.
    if (m_rightMeasure != NULL) {
      delete m_rightMeasure;
      m_rightMeasure = NULL;
    }
    m_rightMeasure = new ControlMeasure();
    //  Find measure for each file
    *m_rightMeasure = *((*m_editPoint)[serial]);

    //  If m_leftCube is not null, delete before creating new one
    if (m_rightCube != NULL) delete m_rightCube;
    m_rightCube = new Cube();
    m_rightCube->open(file);

    //  Update left measure of pointEditor
    m_pointEditor->setRightMeasure (m_rightMeasure,m_rightCube,
                                    m_editPoint->GetId());
    updateRightMeasureInfo ();

  }




  /**
   * @internal
   * @history 2008-11-24  Jeannie Walldren - Added "Goodness of Fit" to left
   *                         measure info.
   * @history 2010-07-22  Tracie Sucharski - Updated new measure types
   *                           associated with implementation of binary
   *                           control networks.
   * @history 2010-12-27  Tracie Sucharski - Write textual Null instead of
   *                           the numeric Null for sample & line residuals.
   * @history 2011-04-20  Tracie Sucharski - Set EditLock check box correctly
   * @history 2011-05-20  Tracie Sucharski - Added Reference output
   * @history 2011-07-19  Tracie Sucharski - Did some re-arranging and added
   *                           sample/line shifts.
   *
   */
  void MatchTool::updateLeftMeasureInfo () {

    //  Set editLock measure box correctly
    m_lockLeftMeasure->setChecked(IsMeasureLocked(
                                       m_leftMeasure->GetCubeSerialNumber()));
    //  Set ignore measure box correctly
    m_ignoreLeftMeasure->setChecked(m_leftMeasure->IsIgnored());

    QString s = "Reference: ";
    if (m_editPoint->IsReferenceExplicit() &&
        (QString(m_leftMeasure->GetCubeSerialNumber()) == m_editPoint->GetReferenceSN())) {
      s += "True";
    }
    else {
      s += "False";
    }
    m_leftReference->setText(s);

    s = "Measure Type: ";
    if (m_leftMeasure->GetType() == ControlMeasure::Candidate) s+= "Candidate";
    if (m_leftMeasure->GetType() == ControlMeasure::Manual) s+= "Manual";
    if (m_leftMeasure->GetType() == ControlMeasure::RegisteredPixel) s+= "RegisteredPixel";
    if (m_leftMeasure->GetType() == ControlMeasure::RegisteredSubPixel) s+= "RegisteredSubPixel";
    m_leftMeasureType->setText(s);

    if (m_leftMeasure->GetSampleShift() == Null) {
      s = "Sample Shift: Null";
    }
    else {
      s = "Sample Shift: " + QString::number(m_leftMeasure->GetSampleShift());
    }
    m_leftSampShift->setText(s);

    if (m_leftMeasure->GetLineShift() == Null) {
      s = "Line Shift: Null";
    }
    else {
      s = "Line Shift: " + QString::number(m_leftMeasure->GetLineShift());
    }
    m_leftLineShift->setText(s);

    double goodnessOfFit = m_leftMeasure->GetLogData(
                    ControlMeasureLogData::GoodnessOfFit).GetNumericalValue();
    if (goodnessOfFit == Null) {
      s = "Goodness of Fit: Null";
    }
    else {
      s = "Goodness of Fit: " + QString::number(goodnessOfFit);
    }
    m_leftGoodness->setText(s);

  }



  /**
   * @internal
   * @history 2008-11-24  Jeannie Walldren - Added "Goodness of Fit" to right
   *                           measure info.
   * @history 2010-06-03 Jeannie Walldren - Removed "std::" since "using
   *                           namespace std"
   * @history 2010-07-22  Tracie Sucharski - Updated new measure types
   *                           associated with implementation of binary
   *                           control networks.
   * @history 2010-12-27  Tracie Sucharski - Write textual Null instead of
   *                           the numeric Null for sample & line residuals.
   * @history 2011-04-20  Tracie Sucharski - Set EditLock check box correctly
   * @history 2011-05-20  Tracie Sucharski - Added Reference output
   * @history 2011-07-19  Tracie Sucharski - Did some re-arranging and added
   *                           sample/line shifts.
   *
   */

  void MatchTool::updateRightMeasureInfo () {

  //  Set editLock measure box correctly
    m_lockRightMeasure->setChecked(IsMeasureLocked(
                                        m_rightMeasure->GetCubeSerialNumber()));
      //  Set ignore measure box correctly
    m_ignoreRightMeasure->setChecked(m_rightMeasure->IsIgnored());

    QString s = "Reference: ";
    if (m_editPoint->IsReferenceExplicit() &&
        (QString(m_rightMeasure->GetCubeSerialNumber()) == m_editPoint->GetReferenceSN())) {
      s += "True";
    }
    else {
      s += "False";
    }

    m_rightReference->setText(s);

    s = "Measure Type: ";
    if (m_rightMeasure->GetType() == ControlMeasure::Candidate) s+= "Candidate";
    if (m_rightMeasure->GetType() == ControlMeasure::Manual) s+= "Manual";
    if (m_rightMeasure->GetType() == ControlMeasure::RegisteredPixel) s+= "RegisteredPixel";
    if (m_rightMeasure->GetType() == ControlMeasure::RegisteredSubPixel) s+= "RegisteredSubPixel";
    m_rightMeasureType->setText(s);

    if (m_rightMeasure->GetSampleShift() == Null) {
      s = "Sample Shift: Null";
    }
    else {
      s = "Sample Shift: " + QString::number(m_rightMeasure->GetSampleShift());
    }
    m_rightSampShift->setText(s);

    if (m_rightMeasure->GetLineShift() == Null) {
      s = "Line Shift: Null";
    }
    else {
      s = "Line Shift: " + QString::number(m_rightMeasure->GetLineShift());
    }
    m_rightLineShift->setText(s);

    double goodnessOfFit = m_rightMeasure->GetLogData(
                    ControlMeasureLogData::GoodnessOfFit).GetNumericalValue();
    if (goodnessOfFit == Null) {
      s = "Goodness of Fit: Null";
    }
    else {
      s = "Goodness of Fit: " + QString::number(goodnessOfFit);
    }
    m_rightGoodness->setText(s);

  }



  /**
   * Event filter for MatchTool.  Determines whether to update left or right
   * measure info.
   *
   * @param o Pointer to QObject
   * @param e Pointer to QEvent
   *
   * @return @b bool Indicates whether the event type is "Leave".
   *
   */
  bool MatchTool::eventFilter(QObject *o, QEvent *e) {
    if(e->type() != QEvent::Leave) return false;
    if(o == m_leftCombo->view()) {
      updateLeftMeasureInfo();
      m_leftCombo->hidePopup();
    }
    if (o == m_rightCombo->view()) {
      updateRightMeasureInfo ();
      m_rightCombo->hidePopup();
    }
    return true;
  }


  /**
   * Take care of drawing things on a viewPort.
   * This is overiding the parents paintViewport member.
   *
   * @param vp Pointer to Viewport to be painted
   * @param painter
   */
  void MatchTool::paintViewport(MdiCubeViewport *mvp, QPainter *painter) {
    drawAllMeasurments (mvp,painter);

  }


  /**
   * This method will repaint the given Point ID in each viewport
   * Note: The pointId parameter is here even though it's not used because
   * the signal (MatchTool::editPointChanged connected to this slot is also
   * connected to another slot (MatchNavTool::updateEditPoint which needs the
   * point Id.  TODO:  Clean this up, use 2 different signals?
   *
   * @param pointId
   *
   * @internal
   * @history 2010-06-03 Jeannie Walldren - Removed "std::" since "using
   *                          namespace std"
   */
  void MatchTool::paintAllViewports() {

    // Take care of drawing things on all viewPorts.
    // Calling update will cause the Tool class to call all registered tools
    // if point has been deleted, this will remove it from the main window
    MdiCubeViewport *mvp;
    for (int i=0; i<(int)cubeViewportList()->size(); i++) {
      mvp = (*(cubeViewportList()))[i];
      mvp->viewport()->update();
    }
  }

  /**
   * Draw all measurments which are on this viewPort
   * @param vp Viewport whose measurements will be drawn
   * @param painter
   * @internal
   *   @history 2010-06-03 Jeannie Walldren - Removed "std::" since "using
   *                          namespace std"
   *   @history 2010-06-08 Jeannie Walldren - Fixed bug that was causing ignored
   *                          measures not be drawn as yellow unless MatchTool was
   *                          open
   *   @history 2010-07-01 Jeannie Walldren - Modified to draw points selected in
   *                          MatchTool last so they lay on top of all other points
   *                          in the image.
   *   @history 2011-04-15 Tracie Sucharski - Fixed bug which was causing all
   *                          measures to be drawn on all cubes.  Also removed
   *                          loop through measures, instead just get measure
   *                          for given serial number.
   *   @history 2011-10-20 Tracie Sucharski - Add check for a control network
   *                          that does not yet have any control points.
   *   @history 2011-11-09 Tracie Sucharski - If there are no measures for
   *                          this cube, return.
   */
  void MatchTool::drawAllMeasurments(MdiCubeViewport *mvp, QPainter *painter) {

    //  Make sure we have points to draw
    if ( (m_controlNet == NULL || m_controlNet->GetNumPoints() == 0) && m_newPoint == NULL &&
         m_editPoint == NULL)
      return;

    QString sn = serialNumber(mvp);
//  QString serialNumber = SerialNumber::Compose(*vp->cube(), true);

    // In the middle of creating a new point
    if (m_newPoint != NULL) {
      // and the selected point is in the image,
      if (m_newPoint->HasSerialNumber(sn)) {
        // find the measurement
        double samp = (*m_newPoint)[sn]->GetSample();
        double line = (*m_newPoint)[sn]->GetLine();
        int x, y;
        mvp->cubeToViewport(samp, line, x, y);
        // set point marker red
        QBrush brush(Qt::red);
        // set point marker bold - line width 2
        QPen pen(brush, 2);
        // draw the selected point in each image last so it's on top of the rest of the points
        painter->setPen(pen);
        painter->drawLine(x - 5, y, x + 5, y);
        painter->drawLine(x, y - 5, x, y + 5);
      }
    }

    //  If viewport serial number not found in control net, return
    if (!m_controlNet->GetCubeSerials().contains(
               sn)) return;

//  if (!m_controlNet->GetCubeSerials().contains(
//                    QString::fromStdString(sn))) return;
//  if (!serialNumberList().HasSerialNumber(sn)) return;

    QList<ControlMeasure *> measures =
        m_controlNet->GetMeasuresInCube(sn);
    // loop through all measures contained in this cube
    for (int i = 0; i < measures.count(); i++) {
      ControlMeasure *m = measures[i];
      // Find the measurments on the viewport
      double samp = m->GetSample();
      double line = m->GetLine();
      int x, y;
      mvp->cubeToViewport(samp, line, x, y);
      // if the point is ignored,
      if (m->Parent()->IsIgnored()) {
        painter->setPen(QColor(255, 255, 0)); // set point marker yellow
      }
      // point is not ignored, but measure matching this image is ignored,
      else if (m->IsIgnored()) {
        painter->setPen(QColor(255, 255, 0)); // set point marker yellow
      }
      else {
        painter->setPen(Qt::green); // set all other point markers green
      }
      // draw points
      painter->drawLine(x - 5, y, x + 5, y);
      painter->drawLine(x, y - 5, x, y + 5);
    }
    // if MatchTool is open,
    if (m_editPoint != NULL && m_newPoint == NULL) {
      // and the selected point is in the image,
      if (m_editPoint->HasSerialNumber(sn)) {
        // find the measurement
        double samp = (*m_editPoint)[sn]->GetSample();
        double line = (*m_editPoint)[sn]->GetLine();
        int x, y;
        mvp->cubeToViewport(samp, line, x, y);
        // set point marker red
        QBrush brush(Qt::red);
        // set point marker bold - line width 2
        QPen pen(brush, 2);
        // draw the selected point in each image last so it's on top of the rest of the points
        painter->setPen(pen);
        painter->drawLine(x - 5, y, x + 5, y);
        painter->drawLine(x, y - 5, x, y + 5);
      }
    }
  }



  /**
   * Allows user to set a new template file.
   * @author 2008-12-10 Jeannie Walldren
   * @internal
   *   @history 2008-12-10 Jeannie Walldren - Original Version
   *

  void MatchTool::setTemplateFile() {
    m_pointEditor->setTemplateFile();
  }*/



  bool MatchTool::okToContinue() {

    if (m_templateModified) {
      int r = QMessageBox::warning(m_matchTool, tr("OK to continue?"),
          tr("The currently opened registration template has been modified.\n"
          "Save changes?"),
          QMessageBox::Yes | QMessageBox::No | QMessageBox::Cancel,
          QMessageBox::Yes);

      if (r == QMessageBox::Yes)
        saveTemplateFileAs();
      else if (r == QMessageBox::Cancel)
        return false;
    }

    return true;
  }



  /**
   * prompt user for a registration template file to open.  Once the file is
   * selected, loadTemplateFile is called to update the current template file
   * being used.
   */
  void MatchTool::openTemplateFile() {

    if (!okToContinue())
      return;

    QString filename = QFileDialog::getOpenFileName(m_matchTool,
        "Select a registration template", ".",
        "Registration template files (*.def *.pvl);;All files (*)");

    if (filename.isEmpty())
      return;

    if (m_pointEditor->setTemplateFile(filename)) {
      loadTemplateFile(filename);
    }
  }



  /**
   * Updates the current template file being used.
   *
   * @param fn The file path of the new template file
   */
  void MatchTool::loadTemplateFile(QString fn) {

    QFile file(FileName((QString) fn).expanded());
    if (!file.open(QIODevice::ReadOnly)) {
      QString msg = "Failed to open template file \"" + fn + "\"";
      QMessageBox::warning(m_matchTool, "IO Error", msg);
      return;
    }

    QTextStream stream(&file);
    m_templateEditor->setText(stream.readAll());
    file.close();

    QScrollBar * sb = m_templateEditor->verticalScrollBar();
    sb->setValue(sb->minimum());

    m_templateModified = false;
    m_saveTemplateFile->setEnabled(false);
    m_templateFileNameLabel->setText("Template File: " + fn);
  }



  //! called when the template file is modified by the template editor
  void MatchTool::setTemplateModified() {
    m_templateModified = true;
    m_saveTemplateFile->setEnabled(true);
  }



  //! save the file opened in the template editor
  void MatchTool::saveTemplateFile() {

    if (!m_templateModified)
      return;

    QString filename =
        m_pointEditor->templateFileName();

    writeTemplateFile(filename);
  }



  //! save the contents of template editor to a file chosen by the user
  void MatchTool::saveTemplateFileAs() {

    QString filename = QFileDialog::getSaveFileName(m_matchTool,
        "Save registration template", ".",
        "Registration template files (*.def *.pvl);;All files (*)");

    if (filename.isEmpty())
      return;

    writeTemplateFile(filename);
  }



  /**
   * write the contents of the template editor to the file provided.
   *
   * @param fn The filename to write to
   */
  void MatchTool::writeTemplateFile(QString fn) {

    QString contents = m_templateEditor->toPlainText();

    // catch errors in Pvl format when populating pvl object
    stringstream ss;
    ss << contents;
    try {
      Pvl pvl;
      ss >> pvl;
    }
    catch(IException &e) {
      QString message = e.toString();
      QMessageBox::warning(m_matchTool, "Error", message);
      return;
    }

    QString expandedFileName(
        FileName((QString) fn).expanded());

    QFile file(expandedFileName);

    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
      QString msg = "Failed to save template file to \"" + fn + "\"\nDo you "
          "have permission?";
      QMessageBox::warning(m_matchTool, "IO Error", msg);
      return;
    }

    // now save contents
    QTextStream stream(&file);
    stream << contents;
    file.close();

    if (m_pointEditor->setTemplateFile(fn)) {
      m_templateModified = false;
      m_saveTemplateFile->setEnabled(false);
      m_templateFileNameLabel->setText("Template File: " + fn);
    }
  }



  /**
   * Allows the user to view the template file that is currently
   * set.
   *
   * @author 2008-12-10 Jeannie Walldren
   * @internal
   *   @history 2008-12-10 Jeannie Walldren - Original Version
   *   @history 2008-12-10 Jeannie Walldren - Added "" namespace to
   *                          PvlEditDialog reference and changed
   *                          registrationDialog from pointer to object
   *   @history 2008-12-15 Jeannie Walldren - Added QMessageBox warning in case
   *                          Template File cannot be read.
   */
  void MatchTool::viewTemplateFile() {
    try{
      // Get the template file from the ControlPointEditor object
      Pvl templatePvl(m_pointEditor->templateFileName());
      // Create registration dialog window using PvlEditDialog class
      // to view and/or edit the template
      PvlEditDialog registrationDialog(templatePvl);
      registrationDialog.setWindowTitle("View or Edit Template File: "
                                         + templatePvl.fileName());
      registrationDialog.resize(550,360);
      registrationDialog.exec();
    }
    catch (IException &e) {
      QString message = e.toString();
      QMessageBox::information(m_matchTool, "Error", message);
    }
  }



  /**
   * Slot which calls ControlPointEditor slot to save chips
   *
   * @author 2009-03-17 Tracie Sucharski
   */

  void MatchTool::saveChips() {
    m_pointEditor->saveChips();
  }



  void MatchTool::showHideTemplateEditor() {

    if (!m_templateEditorWidget)
      return;

    m_templateEditorWidget->setVisible(!m_templateEditorWidget->isVisible());
  }



  /**
   * Update the current editPoint information in the Point Editor labels
   *
   * @author 2011-05-05 Tracie Sucharski
   *
   * @TODO  Instead of a single method, should slots be separate for each
   *        updated point parameter, ie. ignore, editLock, apriori, etc.
   *        This is not robust, if other point attributes are changed outside
   *        of MatchTool, this method will need to be updated.
   *       *** THIS METHOD SHOULD GO AWAY WHEN CONTROLpOINTEDITOR IS INCLUDED
   *           IN MATCH ***
   */
  void MatchTool::updatePointInfo(QString pointId) {
    if (m_editPoint == NULL) return;
    if (pointId != m_editPoint->GetId()) return;
    //  The edit point has been changed by SetApriori, so m_editPoint needs
    //  to possibly update some values.  Need to retain measures from m_editPoint
    //  because they might have been updated, but not yet saved to the network
    //   ("Save Point").
    ControlPoint *updatedPoint = m_controlNet->GetPoint(pointId);
    m_editPoint->SetEditLock(updatedPoint->IsEditLocked());
    m_editPoint->SetIgnored(updatedPoint->IsIgnored());

    //  Set EditLock box correctly
    m_lockPoint->setChecked(m_editPoint->IsEditLocked());

    //  Set ignore box correctly
    m_ignorePoint->setChecked(m_editPoint->IsIgnored());

  }



  /**
   * Refresh all necessary widgets in MatchTool including the PointEditor and
   * CubeViewports.
   *
   * @author 2008-12-09 Tracie Sucharski
   *
   * @internal
   * @history 2010-12-15 Tracie Sucharski - Before setting m_editPoint to NULL,
   *                        release memory.  TODO: Why is the first if statement
   *                        being done???
   * @history 2011-10-20 Tracie Sucharski - If no control points exist in the
   *                        network, emit proper signal and make sure editor
   *                        and measure table are hidden.
   *
   */
  void MatchTool::refresh() {

    //  Check point being edited, make sure it still exists, if not ???
    //  Update ignored checkbox??
    if (m_editPoint != NULL) {
      try {
        QString id = m_ptIdValue->text().remove("Point ID:  ");
        m_controlNet->GetPoint(id);
      }
      catch (IException &) {
        delete m_editPoint;
        m_editPoint = NULL;
        emit editPointChanged();
//      m_matchTool->setVisible(false);
//      m_measureWindow->setVisible(false);
      }
    }

    paintAllViewports();
  }



  /**
   * Turn "Save Point" button text to red
   *
   * @author 2011-06-14 Tracie Sucharski
   */
  void MatchTool::colorizeSaveButton() {

    QColor qc = Qt::red;
    QPalette p = m_savePoint->palette();
    p.setColor(QPalette::ButtonText,qc);
    m_savePoint->setPalette(p);

  }



  /**
   * Check for implicitly locked measure in m_editPoint.  If point is Locked,
   * and this measure is the reference, it is implicity Locked.
   * Because measure is a copy, the ControlPoint::IsEditLocked() which checks
   * for implicit Lock on Reference measures does not work because there is
   * not a parent point.
   *
   * @param[in] serialNumber (QString)   Serial number of measure to be checked
   *
   *
   * @author 2011-07-06 Tracie Sucharski
   */
  bool MatchTool::IsMeasureLocked (QString serialNumber) {

    if (m_editPoint == NULL) return false;

    // Reference implicitly editLocked
    if (m_editPoint->IsEditLocked() && m_editPoint->IsReferenceExplicit() &&
        (m_editPoint->GetReferenceSN() == serialNumber)) {
      return true;
    }
    // Return measures explicit editLocked value
    else {
      return m_editPoint->GetMeasure(serialNumber)->IsEditLocked();
    }

  }



  /**
   * This method is called from the constructor so that when the
   * Main window is created, it know's it's size and location.
   *
   */
  void MatchTool::readSettings() {
    FileName config("$HOME/.Isis/qview/MatchTool.config");
    QSettings settings(config.expanded(),
                       QSettings::NativeFormat);
    QPoint pos = settings.value("pos", QPoint(300, 100)).toPoint();
    QSize size = settings.value("size", QSize(900, 500)).toSize();
    m_matchTool->resize(size);
    m_matchTool->move(pos);
  }


  /**
   * This method is called when the Main window is closed or
   * hidden to write the size and location settings to a config
   * file in the user's home directory.
   *
   */
  void MatchTool::writeSettings() const {
    /*We do not want to write the settings unless the window is
      visible at the time of closing the application*/
    if (!m_matchTool->isVisible()) return;
    FileName config("$HOME/.Isis/qview/MatchTool.config");
    QSettings settings(config.expanded(),
                       QSettings::NativeFormat);
    settings.setValue("pos", m_matchTool->pos());
    settings.setValue("size", m_matchTool->size());
  }



  void MatchTool::enterWhatsThisMode() {
    QWhatsThis::enterWhatsThisMode();
  }


  void MatchTool::clearEditPoint() {
    m_editPoint = NULL;
  }



  void MatchTool::showHelp() {

    QDialog *helpDialog = new QDialog(m_matchTool);
    helpDialog->setWindowTitle("Match Tool Help");

    QVBoxLayout *mainLayout = new QVBoxLayout;
    helpDialog->setLayout(mainLayout);

    QLabel *matchTitle = new QLabel("<h2>Match Tool</h2>");
    mainLayout->addWidget(matchTitle);

    QLabel *matchSubtitle = new QLabel("A tool for interactively measuring and editing sample/line "
                                  "registration points between cubes.  These "
                                  "points contain sample, line postions only, no latitude or "
                                  "longitude values are used or recorded.");
    matchSubtitle->setWordWrap(true);
    mainLayout->addWidget(matchSubtitle);

    QTabWidget *tabArea = new QTabWidget;
    tabArea->setDocumentMode(true);
    mainLayout->addWidget(tabArea);

    //  TAB 1 - Overview
    QScrollArea *overviewTab = new QScrollArea;
    overviewTab->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    overviewTab->setWidgetResizable(true);
    QWidget *overviewContainer = new QWidget;
    QVBoxLayout *overviewLayout = new QVBoxLayout;
    overviewContainer->setLayout(overviewLayout);

    QLabel *purposeTitle = new QLabel("<h2>Purpose</h2>");
    overviewLayout->addWidget(purposeTitle);

    QLabel *purposeText = new QLabel("<p>This tool is for recording and editing registration "
        "points measured between cubes displayed in the <i>qview</i> main window.</p> <p>The "
        "recorded registration points are sample and line pixel coordinates only.  Therefore, this "
        "tool can be used on any images including ones that do not contain a camera model "
        "(i.e, The existence of the Isis Instrument Group on the image labels is not required). "
        "This also means that the tool differs from the <i>qnet</i> control point network "
        "application in that no latitude or longitude values are ever used or recorded "
        "(regardless if the image has a camera model in Isis).</p>"
        "<p>The output control point network that this tool generates is primarily used 1) as "
        "input for an image-wide sample/line translation to register one image to another by "
        "'moving' pixel locations - refer to the documentation for applications such as "
        "<i>translate</i> and <i>warp</i>, or 2) to export the file and use the recorded "
        "measurements in other spreadsheet or plotting packages to visualize magnitude "
        "and direction of varying translations of the images relative to one another.</p> "
        "<p>An automated version of this match tool is the <i>coreg</i> application.  This tool "
        "can be used to visually evaluate and edit the control point network created by "
        "<i>coreg</i>.</p> "
        "<p>The format of the output point network file is binary. This tool uses the Isis control "
        " network framework to create, co-register and save all control points and pixel "
        "measurements.  The application <i>cnetbin2pvl</i> can be used to convert from binary to "
        "a readable PVL format."
        "<p>The Mouse Button functions are: (same as <i>qnet</i>)<ul><li>Modify Point=Left</li> "
        "<li>Delete Point=Middle</li><li>Create New Point=Right</li></ul></p>"
        "<p>Control Points are drawn on the associated displayed cubes with the following colors:  "
        "Green=Valid registration point; Yellow=Ignored point; Red=Active point being edited");
    purposeText->setWordWrap(true);
    overviewLayout->addWidget(purposeText);

    overviewTab->setWidget(overviewContainer);

    //  TAB 2 - Quick Start
    QScrollArea *quickTab = new QScrollArea;
    quickTab->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    quickTab->setWidgetResizable(true);
    QWidget *quickContainer = new QWidget;
    QVBoxLayout *quickLayout = new QVBoxLayout;
    quickContainer->setLayout(quickLayout);

    QLabel *quickTitle = new QLabel("<h2>Quick Start</h2>");
    quickLayout->addWidget(quickTitle);

    QLabel *quickSubTitle = new QLabel("<h3>Preparation:</h3>");
    quickLayout->addWidget(quickSubTitle);

    QLabel *quickPrep = new QLabel("<p><ul>"
        "<li>Open the cubes with overlapping areas for choosing control points</li>"
        "<li>Choose the match tool <img src=\"" + toolIconDir() +
        "/stock_draw-connector-with-arrows.png\" width=22 height=22> "
        "from the toolpad on the right side of the <i>qview</i> main window</li>");
    quickPrep->setWordWrap(true);
    quickLayout->addWidget(quickPrep);

    QLabel *morePrep = new QLabel("<p>Once the Match tool is activated the tool bar at the top "
        "of the main window contains file action buttons and a help button:");
    morePrep->setWordWrap(true);
    quickLayout->addWidget(morePrep);

    QLabel *fileButtons = new QLabel("<p><ul>"
        "<li><img src=\"" + toolIconDir() + "/fileopen.png\" width=22 height=22>  Open an existing "
        "control network  <b>Note:</b> If you do not open an existing network, a new one will "
        "be created</li>"
        "<li><img src=\"" + toolIconDir() + "/mActionFileSaveAs.png\" width=22 height=22> Save "
        "control network as ...</li>"
        "<li><img src=\"" + toolIconDir() + "/mActionFileSave.png\" width=22 height=22> Save "
        "control network to current file</li>"
        "<li><img src=\"" + toolIconDir() + "/help-contents.png\" width=22 height=22> Show Help "
        "</li></ul></p>");
    fileButtons->setWordWrap(true);
    quickLayout->addWidget(fileButtons);

    QLabel *quickFunctionTitle = new QLabel("<h3>Cube Viewport Functions:</h3>");
    quickLayout->addWidget(quickFunctionTitle);

    QLabel *quickFunction = new QLabel(
        "The match tool window will be shown once "
        "you click in a cube viewport window using one of the following "
        "mouse functions.  <b>Note:</b>  Existing control points are drawn on the cube viewports");
    quickFunction->setWordWrap(true);
    quickLayout->addWidget(quickFunction);

    QLabel *quickDesc = new QLabel("<p><ul>"
      "<li>Left Click - Modify the control point closest to the click  <b>Note:</b>  "
      "All cubes in the control point must be displayed before loading the point</li>"
      "<li>Middle Click - Delete the control point closest to the click</li>"
      "<li>Right Click - Create a new control point at the click location</li></ul></p>");
    quickDesc->setWordWrap(true);
    quickDesc->setOpenExternalLinks(true);
    quickLayout->addWidget(quickDesc);

    quickTab->setWidget(quickContainer);

    //  TAB 3 - Control Point Editing
    QScrollArea *controlPointTab = new QScrollArea;
    controlPointTab->setWidgetResizable(true);
    controlPointTab->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    QWidget *controlPointContainer = new QWidget;
    QVBoxLayout *controlPointLayout = new QVBoxLayout;
    controlPointContainer->setLayout(controlPointLayout);

    QLabel *controlPointTitle = new QLabel("<h2>Control Point Editing</h2>");
    controlPointLayout->addWidget(controlPointTitle);

    QLabel *mouseLabel = new QLabel("<p><h3>When the \"Match\" tool "
      "is activated, the mouse buttons have the following function in the "
      "cube viewports of the main qview window:</h3>");
    mouseLabel->setWordWrap(true);
    mouseLabel->setScaledContents(true);
    controlPointLayout->addWidget(mouseLabel);

    QLabel *controlPointDesc = new QLabel("<ul>"
      "<li>Left click   - Edit the closest control point   <b>Note:</b>  "
      "All cubes in the control point must be displayed before loading the point</li>"
      "<li>Middle click - Delete the closest control point</li>"
      "<li>Right click  - Create new control point at cursor location.  This will bring up a new "
      "point dialog which allows you to enter a point id and will list all cube viewports, "
      "highlighting cubes where the point has been chosen by clicking on the cube's viewport.  "
      "When the desired cubes have been chosen, select the \"Done\" button which will load the "
      "control point into the control point editor window which will allow the control measure "
      "positions to be refined.</li>");
    controlPointDesc->setWordWrap(true);
    controlPointLayout->addWidget(controlPointDesc);

    QLabel *controlPointEditing = new QLabel(
      "<h4>Changing Control Measure Locations</h4>"
        "<p>Both the left and right control measure positions can be adjusted by:"
      "<ul>"
      "<li>Move the cursor location under the crosshair by clicking the left mouse "
            "button</li>"
      "<li>Move 1 pixel at a time by using arrow keys on the keyboard</li>"
      "<li>Move 1 pixel at a time by using arrow buttons above the right and left views</li>"
      "</ul></p>"
      "<h4>Other Point Editor Functions</h4>"
        "<p>Along the right border of the window:</p>"
        "<ul>"
          "<li><strong>Link Zoom</strong>   This will link the two small viewports together when "
              "zooming (ie.  If this is checked, if the left view is zoomed, the right view will "
              "match the left view's zoom factor.  "
              "<b>Note:</b>   Zooming is controlled from the left view.</li>"
         "<li><strong>No Rotate:</strong>  Turn off the rotation and bring right view back to "
          "its original orientation</li>"
          "<li><strong>Rotate:</strong>   Rotate the right view using either the dial "
              "or entering degrees </li>"
          "<li><strong>Show control points:</strong>  Draw crosshairs at all control "
               "point locations visible within the view</li>"
          "<li><strong>Show crosshair:</strong>  Show a red crosshair across the entire "
              "view</li>"
          "<li><strong>Circle:</strong>  Draw circle which may help center measure "
              "on a crater</li></ul"
        "<p>Below the left view:</p>"
          "<ul><li><strong>Blink controls:</strong>  Blink the left and right view in the "
          "left view window using the \"Blink Start\" button <img src=\"" + toolIconDir() +
          "/blinkStart.png\" width=22 height=22> and \"Blink Stop\" button <img src=\"" +
          toolIconDir() + "/blinkStop.png\" width=22 height=22>.  The arrow keys above the left "
          "and right views and the keyboard arrow keys may be used to move the both views while "
          "blinking.</li>"
        "<li><strong>Register:</strong>  Sub-pixel register the right view to "
              "the left view. A default registration template is used for setting parameters "
              "passed to the sub-pixel registration tool.  The user may load in a predefined "
              "template or edit the current loaded template to influence successful "
              "co-registration results.  For more information regarding the pattern matching "
              "functionlity or how to create a parameter template, refer to the Isis PatternMatch "
              "document and the <i>autoregtemplate</i> application. <strong>Shortcut: R.</strong>"
              "</li>"
        "<li><strong>Save Measures:</strong>  Save the two control measures using the sample, "
              "line positions under the crosshairs. <strong>Shortcut: M.</strong></li>"
        "<li><strong>Save Point:</strong>  Save the control point to the control network. "
              "<strong>Shortcut: P.</strong></li>"
        "</ul>");
    controlPointEditing->setWordWrap(true);
    controlPointLayout->addWidget(controlPointEditing);

    controlPointTab->setWidget(controlPointContainer);

    //  TAB 4 - Coreg Guidance
    QScrollArea *coregTab = new QScrollArea;
    coregTab->setWidgetResizable(true);
    coregTab->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    QWidget *coregContainer = new QWidget;
    QVBoxLayout *coregLayout = new QVBoxLayout;
    coregContainer->setLayout(coregLayout);

    QLabel *coregTitle = new QLabel("<h2>Coreg Guidance</h2>");
    coregLayout->addWidget(coregTitle);

    QLabel *coregDesc = new QLabel("<p>When opening control networks created by <i>coreg</i>, "
      "there are some things to keep in mind.  First, all control points must have the same "
      "reference measure (this is the image filename passed to the <i>coreg</i> 'match' parameter)."
      "<p>In order to retain the integrity of the input <i>coreg</i> network, you cannot change "
      "which image is the reference measure on any of the existing points. "
      "<p>When creating a new control point to add to the <i>coreg</i> network, this tool will "
      "automatically set the reference measure to the same image as the other control points in "
      "the network as long as the reference image was one of the images selected with "
      "the right mouse button from the new point dialog.  If this image was not selected when "
      "creating a new point, it "
      "does not contain a required measurement, therefore, you will get an error and the new "
      "point will not be created.</p> <p>The reference measure is always loaded on the left side "
      "of the control point editor.  If you load a measure that is not the reference measure "
      "on the left side and try to save the point, you will receive an error message.  You will "
      "need to move the reference measure back to the left side before saving the control point."
      "</p><p><b>Note:</b>  This error checking will not happen on control networks created by "
      "<i>coreg</i> prior to Isis3.4.2. For older <i>coreg</i> control networks the user must be "
      "aware and make sure the correct image is set to the same <i>coreg</i> reference measure.");
    coregDesc->setWordWrap(true);
    coregDesc->setScaledContents(true);
    coregLayout->addWidget(coregDesc);

    coregTab->setWidget(coregContainer);

    tabArea->addTab(overviewTab, "&Overview");
    tabArea->addTab(quickTab, "&Quick Start");
    tabArea->addTab(controlPointTab, "&Control Point Editing");
    tabArea->addTab(coregTab, "&Coreg Guidance");

    QHBoxLayout *buttonsLayout = new QHBoxLayout;
    // Flush the buttons to the right
    buttonsLayout->addStretch();

    QPushButton *closeButton = new QPushButton("&Close");
    closeButton->setIcon(QPixmap(toolIconDir() + "/guiStop.png"));
    closeButton->setDefault(true);
    connect(closeButton, SIGNAL(clicked()),
            helpDialog, SLOT(close()));
    buttonsLayout->addWidget(closeButton);

    mainLayout->addLayout(buttonsLayout);

    helpDialog->show();


  }



  /**
   * This called when qview is exiting.  We need to possibly prompt user for saving the edit point
   * and network
   *
   * @author 2012-07-20 Tracie Sucharski
   */
  void MatchTool::exiting() {

    if (m_editPoint) {
      //  If point changed or if a new point, prompt for saving.
      if ( m_controlNet->GetNumPoints() == 0 ||
          !m_controlNet->ContainsPoint(m_editPoint->GetId())) {
        QString message = "\n\nDo you want to save the point in the editor?";
        int response = QMessageBox::question(m_matchTool, "Save point in editor", message,
                                             QMessageBox::Yes | QMessageBox::No,
                                             QMessageBox::Yes);
        if (response == QMessageBox::Yes) {
          savePoint();
        }
      }
    }

    if (m_controlNet && m_controlNet->GetNumPoints() != 0 && m_netChanged) {
      QString message = "The currently open control net has changed.  Do you want to save?";
      int response = QMessageBox::question(m_matchTool, "Save current control net?",
                                           message,
                                           QMessageBox::Yes | QMessageBox::No,
                                           QMessageBox::Yes);
      // Yes:  Save old net, so return without opening network.
      if (response == QMessageBox::Yes) {
        saveAsNet();
      }
    }
  }
}
