#include "QnetTool.h"

#include <sstream>
#include <vector>
#include <iomanip>

#include <QComboBox>
#include <QFileDialog>
#include <QMenuBar>
#include <QMessageBox>
#include <QScrollBar>
#include <QShortcut>
#include <QSplitter>
#include <QStackedWidget>
#include <QtWidgets>
#include <QTableWidget>
#include <QWhatsThis>

#include "QnetDeletePointDialog.h"
#include "QnetNewMeasureDialog.h"
#include "NewControlPointDialog.h"
#include "QnetFixedPointDialog.h"
#include "Workspace.h"

#include "Application.h"
#include "Brick.h"
#include "Camera.h"
#include "CameraFactory.h"
#include "ControlMeasure.h"
#include "ControlMeasureLogData.h"
#include "ControlNet.h"
#include "ControlPoint.h"
#include "ControlPointEdit.h"
#include "Distance.h"
#include "FileName.h"
#include "IException.h"
#include "Latitude.h"
#include "Longitude.h"
#include "MainWindow.h"
#include "MdiCubeViewport.h"
#include "Projection.h"
#include "ProjectionFactory.h"
#include "Pvl.h"
#include "PvlEditDialog.h"
#include "SerialNumber.h"
#include "SpecialPixel.h"
#include "Spice.h"
#include "SurfacePoint.h"
#include "Target.h"
#include "ToolPad.h"
#include "UniversalGroundMap.h"
#include "ViewportMainWindow.h"

using namespace std;

namespace Isis {

  /**
   * Consructs the Qnet Tool window
   *
   * @param parent Pointer to the parent widget for the Qnet tool
   *
   * @internal
   *   @history 2010-06-03 Jeannie Walldren - Initialized pointers to null.
   *
   */
  QnetTool::QnetTool (QWidget *parent) : Tool(parent) {
    m_controlNet = NULL;
    m_demOpen = false;
    m_groundOpen = false;
    m_serialNumberList = NULL;
    m_templateModified = false;

    m_controlNet = new ControlNet;
    m_serialNumberList = new SerialNumberList;

    if (qobject_cast<Workspace *>(parent)) {
      m_workspace = qobject_cast<Workspace *>(parent);
    }
    else if (qobject_cast<ViewportMainWindow *>(parent)) {
      m_workspace = qobject_cast<ViewportMainWindow *>(parent)->workspace();
    }
    else {
      throw IException(IException::Programmer,
          tr("Could not find the workspace with the given parent, expected a Workspace or "
             "ViewportMainWindow."),
          _FILEINFO_);
    }

    createQnetTool(parent);
  }


  QnetTool::~QnetTool () {
    writeSettings();

    delete m_editPoint;
    delete m_leftMeasure;
    delete m_rightMeasure;
    delete m_measureTable;
    delete m_measureWindow;
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
   *                           commented out connection between check box and
   *                           setGroundPoint() method.
   *   @history 2008-12-30 Jeannie Walldren - Added connections to toggle
   *                           measures' Ignore check boxes if ignoreLeftChanged()
   *                           and ignoreRightChanged() are emitted. Replaced
   *                           reference to ignoreChanged() with
   *                           ignorePointChanged().
   *   @history 2010-06-03 Jeannie Walldren - Removed "std::" since "using
   *                           namespace std"
   *   @history 2015-10-29 Ian Humphrey - Added shortcuts for the addMeasure (A) and savePoint (P)
   *                           button. References #2324.
   */
  void QnetTool::createQnetTool(QWidget *parent) {

    m_qnetTool = new MainWindow("Qnet Tool", parent);
    m_qnetTool->setObjectName("QnetTool");

    createActions();
    createMenus();
    createToolBars();

    // create m_pointEditor first since we need to get its templateFileName
    // later
    m_pointEditor = new ControlPointEdit(m_controlNet, parent);
    connect(this, SIGNAL(newControlNetwork(ControlNet *)),
        m_pointEditor, SIGNAL(newControlNetwork(ControlNet *)));
    connect(this,
        SIGNAL(stretchChipViewport(Stretch *, CubeViewport *)),
        m_pointEditor,
        SIGNAL(stretchChipViewport(Stretch *, CubeViewport *)));
    connect(m_pointEditor, SIGNAL(measureSaved()), this, SLOT(measureSaved()));
    connect(this, SIGNAL(measureChanged()),
            m_pointEditor, SLOT(colorizeSaveButton()));

    QPushButton *addMeasure = new QPushButton("Add Measure(s) to Point");
    addMeasure->setShortcut(Qt::Key_A);
    addMeasure->setToolTip("Add a new measure to the edit control point. "
                           "<strong>Shortcut: A</strong>");
    addMeasure->setWhatsThis("This allows a new control measure to be added "
                         "to the currently edited control point.  A selection "
                         "box with all cubes from the input list will be "
                         "displayed with those that intersect with the "
                         "control point highlighted.");
    connect(addMeasure, SIGNAL(clicked()), this, SLOT(addMeasure()));

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
    addMeasureLayout->addWidget(addMeasure);
    addMeasureLayout->addWidget(m_savePoint);
//    addMeasureLayout->addStretch();

    m_templateFileNameLabel = new QLabel("Template File: " +
        m_pointEditor->templateFileName());
    m_templateFileNameLabel->setToolTip("Sub-pixel registration template File.");
//  QString patternMatchDoc =
//          FileName("$ISISROOT/doc/documents/PatternMatch/PatternMatch.html").fileName();
//    m_templateFileNameLabel->setOpenExternalLinks(true);
    m_templateFileNameLabel->setWhatsThis("FileName of the sub-pixel "
                  "registration template.  Refer to https://astrogeology.usgs.gov/docs/concepts/control-networks/pattern-matching/ for a description of the "
                  "contents of this file.");

    m_groundFileNameLabel = new QLabel("Ground Source File: ");
    m_groundFileNameLabel->setToolTip("Cube used to create ground control "
                               "points, either Fixed or Constrained.");
    m_groundFileNameLabel->setWhatsThis("This cube is used to create ground "
                             "control points, Fixed or Constrained.  This may "
                             "be a Dem, a shaded relief version of a Dem, "
                             "a projected basemap or an unprojected cube with "
                             "corrected camera pointing.  This will be used "
                             "to set the apriori latitude, longitude.");
    m_radiusFileNameLabel = new QLabel("Radius Source: ");
    m_radiusFileNameLabel->setToolTip("Dem used to set the radius of ground "
                             "control points, Fixed or Constrained.  This must "
                             "be a Dem and is strictly used to set the apriori "
                             "radius for ground control points.");

    QVBoxLayout * centralLayout = new QVBoxLayout;

    centralLayout->addWidget(m_templateFileNameLabel);
    centralLayout->addWidget(m_groundFileNameLabel);
    centralLayout->addWidget(m_radiusFileNameLabel);
    centralLayout->addWidget(createTopSplitter());
    centralLayout->addStretch();
    centralLayout->addWidget(m_pointEditor);
    centralLayout->addLayout(addMeasureLayout);
    QWidget * centralWidget = new QWidget;
    centralWidget->setLayout(centralLayout);

    QScrollArea *scrollArea = new QScrollArea();
    scrollArea->setObjectName("QnetToolScroll");
    scrollArea->setWidget(centralWidget);
    scrollArea->setWidgetResizable(true);
    centralWidget->adjustSize();
    m_qnetTool->setCentralWidget(scrollArea);
//    m_qnetTool->setCentralWidget(centralWidget);


    connect(this, SIGNAL(editPointChanged(QString)),
            this, SLOT(paintAllViewports(QString)));

    readSettings();
  }


  //! creates everything above the ControlPointEdit
  QSplitter * QnetTool::createTopSplitter() {

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
  QGroupBox * QnetTool::createControlPointGroupBox() {

    // create left vertical layout
    m_ptIdValue = new QLabel;
    m_pointType = new QComboBox;
    for (int i=0; i<ControlPoint::PointTypeCount; i++) {
      m_pointType->insertItem(i, ControlPoint::PointTypeToString(
                              (ControlPoint::PointType) i));
    }
    QHBoxLayout *pointTypeLayout = new QHBoxLayout;
    QLabel *pointTypeLabel = new QLabel("PointType:");
    pointTypeLayout->addWidget(pointTypeLabel);
    pointTypeLayout->addWidget(m_pointType);
    connect(m_pointType, SIGNAL(activated(int)),
            this, SLOT(setPointType(int)));
    m_numMeasures = new QLabel;
    m_pointAprioriLatitude = new QLabel;
    m_pointAprioriLongitude = new QLabel;
    m_pointAprioriRadius = new QLabel;
    m_pointAprioriLatitudeSigma = new QLabel;
    m_pointAprioriLongitudeSigma = new QLabel;
    m_pointAprioriRadiusSigma = new QLabel;
    QVBoxLayout * leftLayout = new QVBoxLayout;
    leftLayout->addWidget(m_ptIdValue);
    leftLayout->addLayout(pointTypeLayout);
    leftLayout->addWidget(m_pointAprioriLatitude);
    leftLayout->addWidget(m_pointAprioriLongitude);
    leftLayout->addWidget(m_pointAprioriRadius);
    leftLayout->addWidget(m_pointAprioriLatitudeSigma);
    leftLayout->addWidget(m_pointAprioriLongitudeSigma);
    leftLayout->addWidget(m_pointAprioriRadiusSigma);

    // create right vertical layout's top layout
    m_lockPoint = new QCheckBox("Edit Lock Point");
    connect(m_lockPoint, SIGNAL(clicked(bool)), this, SLOT(setLockPoint(bool)));
    m_ignorePoint = new QCheckBox("Ignore Point");
    connect(m_ignorePoint, SIGNAL(clicked(bool)),
            this, SLOT(setIgnorePoint(bool)));
    connect(this, SIGNAL(ignorePointChanged()), m_ignorePoint, SLOT(toggle()));
    m_pointLatitude = new QLabel;
    m_pointLongitude = new QLabel;
    m_pointRadius = new QLabel;

    QVBoxLayout * rightLayout = new QVBoxLayout;
    rightLayout->addWidget(m_numMeasures);
    rightLayout->addWidget(m_lockPoint);
    rightLayout->addWidget(m_ignorePoint);
    rightLayout->addWidget(m_pointLatitude);
    rightLayout->addWidget(m_pointLongitude);
    rightLayout->addWidget(m_pointRadius);


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
  QGroupBox * QnetTool::createLeftMeasureGroupBox() {

    m_leftCombo = new QComboBox;
    m_leftCombo->setEditable(true);
    m_leftCombo->setInsertPolicy(QComboBox::NoInsert);
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
    m_leftSampError = new QLabel();
    m_leftSampError->setToolTip("<strong>Jigsaw</strong> sample residual.");
    m_leftSampError->setWhatsThis("This is the sample residual for the left "
                          "measure calculated by the application, "
                          "<strong>jigsaw</strong>.");
    m_leftLineError = new QLabel();
    m_leftLineError->setToolTip("<strong>Jigsaw</strong> line residual.");
    m_leftLineError->setWhatsThis("This is the line residual for the left "
                          "measure calculated by the application, "
                          "<strong>jigsaw</strong>.");
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
    leftLayout->addWidget(m_leftSampError);
    leftLayout->addWidget(m_leftLineError);
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
   *   @history 2015-10-29 Ian Humphrey - Added shortcuts (PageUp/PageDown) for selecting previous
   *                           or next measure in right measures box. References #2324.
   */
  QGroupBox * QnetTool::createRightMeasureGroupBox() {

    // create widgets for the right groupbox
    m_rightCombo = new QComboBox;
    m_rightCombo->setEditable(true);
    m_rightCombo->setInsertPolicy(QComboBox::NoInsert);

    // Attach shortcuts to Qnet Tool's window for selecting right measures
    // Note: Qt handles this memory for us since m_qnetTool is the parent of these shortcuts
    QShortcut *nextMeasure = new QShortcut(Qt::Key_PageDown, m_qnetTool);
    connect(nextMeasure, SIGNAL(activated()), this, SLOT(nextRightMeasure()));
    QShortcut *prevMeasure = new QShortcut(Qt::Key_PageUp, m_qnetTool);
    connect(prevMeasure, SIGNAL(activated()), this, SLOT(previousRightMeasure()));

    m_rightCombo->setToolTip("Choose right control measure. "
                             "<strong>Shortcuts: PageUp/PageDown</strong>");
    m_rightCombo->setWhatsThis("Choose right control measure identified by "
                               "cube filename. "
                               "Note: PageUp selects previous measure; "
                               "PageDown selects next meausure.");
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
    m_rightSampError = new QLabel();
    m_rightSampError->setToolTip("<strong>Jigsaw</strong> sample residual.");
    m_rightSampError->setWhatsThis("This is the sample residual for the right "
                          "measure which was calculated by the application, "
                          "<strong>jigsaw</strong>.");
    m_rightLineError = new QLabel();
    m_rightLineError->setToolTip("<strong>Jigsaw</strong> line residual.");
    m_rightLineError->setWhatsThis("This is the line residual for the right "
                          "measure which was calculated by the application, "
                          "<strong>jigsaw</strong>.");
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
    rightLayout->addWidget(m_rightSampError);
    rightLayout->addWidget(m_rightLineError);
    rightLayout->addWidget(m_rightSampShift);
    rightLayout->addWidget(m_rightLineShift);
    rightLayout->addWidget(m_rightGoodness);

    QGroupBox * rightGroupBox = new QGroupBox("Right Measure");
    rightGroupBox->setLayout(rightLayout);

    return rightGroupBox;
  }


  //! Creates the Widget which contains the template editor and its toolbar
  void QnetTool::createTemplateEditorWidget() {

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
   * @brief Creates the menu actions for Qnet Tool.
   *
   * @internal
   *   @author ???
   *   @history Ian Humphrey - Added CTRL+S shortcut for saving control network. References #2324.
   */
  void QnetTool::createActions() {
    m_openGround = new QAction(m_qnetTool);
    m_openGround->setText("Open &Ground Source");
    m_openGround->setToolTip("Open a ground source for choosing ground control "
                             "points");
    m_openGround->setStatusTip("Open a ground source for choosing ground "
                               "control points");
    QString whatsThis =
      "<b>Function:</b> Open and display a ground source for choosing "
      "ground control points, both Fixed and Constrained."
      "This cube can be a level1, level2 or dem cube.";
    m_openGround->setWhatsThis(whatsThis);
    connect (m_openGround,SIGNAL(triggered()),this,SLOT(openGround()));

    m_openDem = new QAction(m_qnetTool);
    m_openDem->setText("Open &Radius Source");
    m_openDem->setToolTip("Open radius source file for ground control points");
    m_openDem->setStatusTip("Open radius source file for ground control points");
    whatsThis =
      "<b>Function:</b> Open a DEM for determining the radius when "
      "choosing ground control points.  This is not the file that will be "
      "displayed for visually picking points.  This is strictly used to "
      "determine the radius value for ground control points.";
    m_openDem->setWhatsThis(whatsThis);
    connect (m_openDem,SIGNAL(triggered()),this,SLOT(openDem()));

    m_saveNet = new QAction(QIcon(toolIconDir() + "/filesave.png"), "Save Control Network ...",
                            m_qnetTool);
    m_saveNet->setShortcut(Qt::CTRL + Qt::Key_S);
    m_saveNet->setToolTip("Save current control network");
    m_saveNet->setStatusTip("Save current control network");
    whatsThis = "<b>Function:</b> Saves the current <i>"
        "control network</i>";
    m_saveNet->setWhatsThis(whatsThis);
    connect(m_saveNet, SIGNAL(triggered()), this, SLOT(saveNet()));

    m_saveAsNet = new QAction(QIcon(toolIconDir() + "/filesaveas.png"), "Save Control Network &As...",
                              m_qnetTool);
    m_saveAsNet->setToolTip("Save current control network to chosen file");
    m_saveAsNet->setStatusTip("Save current control network to chosen file");
    whatsThis = "<b>Function:</b> Saves the current <i>"
        "control network</i> under chosen filename";
    m_saveAsNet->setWhatsThis(whatsThis);
    connect(m_saveAsNet, SIGNAL(triggered()), this, SLOT(saveAsNet()));

    m_closeQnetTool = new QAction(QIcon(toolIconDir() + "/fileclose.png"), "&Close", m_qnetTool);
    m_closeQnetTool->setToolTip("Close this window");
    m_closeQnetTool->setStatusTip("Close this window");
    m_closeQnetTool->setShortcut(Qt::ALT + Qt::Key_F4);
    whatsThis = "<b>Function:</b> Closes the Qnet Tool window for this point "
        "<p><b>Shortcut:</b> Alt+F4 </p>";
    m_closeQnetTool->setWhatsThis(whatsThis);
    connect(m_closeQnetTool, SIGNAL(triggered()), m_qnetTool, SLOT(close()));

    m_showHideTemplateEditor = new QAction(QIcon(toolIconDir() + "/view_text.png"),
        "&View/edit registration template", m_qnetTool);
    m_showHideTemplateEditor->setCheckable(true);
    m_showHideTemplateEditor->setToolTip("View and/or edit the registration "
                                         "template");
    m_showHideTemplateEditor->setStatusTip("View and/or edit the registration "
                                           "template");
    whatsThis = "<b>Function:</b> Displays the curent registration template.  "
       "The user may edit and save changes under a chosen filename.";
    m_showHideTemplateEditor->setWhatsThis(whatsThis);
    connect(m_showHideTemplateEditor, SIGNAL(triggered()), this,
        SLOT(showHideTemplateEditor()));

    m_saveChips = new QAction(QIcon(toolIconDir() + "/savechips.png"), "Save registration chips",
        m_qnetTool);
    m_saveChips->setToolTip("Save registration chips");
    m_saveChips->setStatusTip("Save registration chips");
    whatsThis = "<b>Function:</b> Save registration chips to file.  "
       "Each chip: pattern, search, fit will be saved to a separate file.";
    m_saveChips->setWhatsThis(whatsThis);
    connect(m_saveChips, SIGNAL(triggered()), this, SLOT(saveChips()));

    m_openTemplateFile = new QAction(QIcon(toolIconDir() + "/fileopen.png"), "&Open registration "
        "template", m_qnetTool);
    m_openTemplateFile->setToolTip("Set registration template");
    m_openTemplateFile->setStatusTip("Set registration template");
    whatsThis = "<b>Function:</b> Allows user to select a new file to set as "
        "the registration template";
    m_openTemplateFile->setWhatsThis(whatsThis);
    connect(m_openTemplateFile, SIGNAL(triggered()), this, SLOT(openTemplateFile()));

    m_saveTemplateFile = new QAction(QIcon(toolIconDir() + "/filesave.png"), "&Save template file",
        m_qnetTool);
    m_saveTemplateFile->setToolTip("Save the template file");
    m_saveTemplateFile->setStatusTip("Save the template file");
    m_saveTemplateFile->setWhatsThis("Save the registration template file");
    connect(m_saveTemplateFile, SIGNAL(triggered()), this,
        SLOT(saveTemplateFile()));

    m_saveTemplateFileAs = new QAction(QIcon(toolIconDir() + "/filesaveas.png"), "&Save template as...",
        m_qnetTool);
    m_saveTemplateFileAs->setToolTip("Save the template file as");
    m_saveTemplateFileAs->setStatusTip("Save the template file as");
    m_saveTemplateFileAs->setWhatsThis("Save the registration template file as");
    connect(m_saveTemplateFileAs, SIGNAL(triggered()), this,
        SLOT(saveTemplateFileAs()));

    m_whatsThis = new QAction(QIcon(FileName(
      "$ISISROOT/appdata/images/icons/contexthelp.png").expanded()),"&Whats's This", m_qnetTool);
    m_whatsThis->setShortcut(Qt::SHIFT | Qt::Key_F1);
    m_whatsThis->setToolTip("Activate What's This and click on items on "
        "user interface to see more information.");
    connect(m_whatsThis, SIGNAL(triggered()), this, SLOT(enterWhatsThisMode()));

  }



  /**
   * Customize dropdown menus below title bar.
   *
   * @internal
   *   @history 2008-11-18 Jeannie Walldren - Added "Close" action to the file
   *                          menu on the qnet tool window.
   *   @history 2008-12-10 Jeannie Walldren - Added "What's this?" function to
   *                          qnet tool actions.
   */
  void QnetTool::createMenus () {

    QMenu *fileMenu = m_qnetTool->menuBar()->addMenu("&File");
    fileMenu->addAction(m_openGround);
    fileMenu->addAction(m_openDem);
    fileMenu->addAction(m_saveNet);
    fileMenu->addAction(m_saveAsNet);
    fileMenu->addAction(m_closeQnetTool);

    QMenu * regMenu = m_qnetTool->menuBar()->addMenu("&Registration");
    regMenu->addAction(m_openTemplateFile);
    regMenu->addAction(m_showHideTemplateEditor);
    regMenu->addAction(m_saveChips);

    QMenu *helpMenu = m_qnetTool->menuBar()->addMenu("&Help");
    helpMenu->addAction(m_whatsThis);
  }


  void QnetTool::createToolBars() {

    toolBar = new QToolBar;
    toolBar->setObjectName("TemplateEditorToolBar");
    toolBar->setFloatable(false);
    toolBar->addAction(m_saveNet);
    toolBar->addSeparator();
    toolBar->addAction(m_showHideTemplateEditor);
    toolBar->addAction(m_saveChips);
    toolBar->addAction(m_whatsThis);

    m_qnetTool->addToolBar(Qt::TopToolBarArea, toolBar);
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
   *                          with qnet windows blinking due to refresh.
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
   *   @history 2013-12-05 Tracie Sucharski - If saving measure on fixed or constrained point and
   *                          reference measure is ignored, print warning and return.
   *   @history 2015-06-05 Makayla Shepherd and Ian Humphery - Modified to return out of the
   *                          method when checkReference returns false.
   */
  void QnetTool::measureSaved() {
    // Read original measures from the network for comparison with measures
    // that have been edited
    ControlMeasure *origLeftMeasure =
                m_editPoint->GetMeasure(m_leftMeasure->GetCubeSerialNumber());
    ControlMeasure *origRightMeasure =
                m_editPoint->GetMeasure(m_rightMeasure->GetCubeSerialNumber());

    if (m_editPoint->IsIgnored()) {
      QString message = "You are saving changes to a measure on an ignored ";
      message += "point.  Do you want to set Ignore = False on the point and ";
      message += "both measures?";
      switch (QMessageBox::question(m_qnetTool, "Qnet Tool Save Measure",
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
    if (origRightMeasure->IsIgnored() && m_rightMeasure->IsIgnored()) {
      QString message = "You are saving changes to an ignored measure.  ";
      message += "Do you want to set Ignore = False on the right measure?";
      switch(QMessageBox::question(m_qnetTool, "Qnet Tool Save Measure",
                                   message, "&Yes", "&No", 0, 0)){
        // Yes:  set Ignore=false for the right measure and save point
        case 0:
            m_rightMeasure->SetIgnored(false);
            emit ignoreRightChanged();
        // No:  keep Ignore=true and save point
        case 1:
          break;
      }
    }

    //  Only check reference if point contains explicit reference.  Otherwise,
    //  there has not been a reference set, set the measure on the left as the reference.
    if (m_editPoint->IsReferenceExplicit()) {
      if (m_editPoint->IsEditLocked()) {
        QString message = "This control point is edit locked.  The Apriori latitude, longitude and ";
        message += "radius cannot be updated.  You must first unlock the point by clicking the ";
        message += "check box above labeled \"Edit Lock Point\".";
        QMessageBox::warning(m_qnetTool, "Point Locked", message);
        return;
      }
      // If user does not want to change the reference then don't update the measure
      if (!checkReference()) {
        return;
      }
    }
    else if (m_leftMeasure->GetCubeSerialNumber() != m_groundSN) {
      m_editPoint->SetRefMeasure(m_leftMeasure->GetCubeSerialNumber());
    }

    // If this is a fixed or constrained point, and the right measure is the ground source,
    // update the lat,lon,radius.  Only the right measure can be moved, so only need to update
    // position if the ground measure is loaded on the right.
    //
    // If point is locked and it is not a new point, print error
    //  TODO::  Only update if the measure moved
    if (m_editPoint->GetType() != ControlPoint::Free && (m_groundOpen &&
        m_rightMeasure->GetCubeSerialNumber() == m_groundSN)) {
      if (m_editPoint->IsEditLocked() && m_controlNet->ContainsPoint(m_editPoint->GetId())) {
        QString message = "This control point is edit locked.  The Apriori latitude, longitude and ";
        message += "radius cannot be updated.  You must first unlock the point by clicking the ";
        message += "check box above labeled \"Edit Lock Point\".";
        QMessageBox::warning(m_qnetTool, "Point Locked", message);
        return;
      }
      if (m_leftMeasure->IsIgnored()) {
        QString message = "This is a Constrained or Fixed point and the reference measure is ";
        message += "Ignored.  Unset the Ignore flag on the reference measure before saving.";
        QMessageBox::warning(m_qnetTool, "Point Locked", message);
        return;
      }
      updateGroundPosition();
    }

    // Save the right measure and left (if ignore or edit lock flag changed) to
    // the editPoint The Ignore flag is the only thing that can change on the left
    // measure.
    m_rightMeasure->SetChooserName(Application::UserName());
    *origRightMeasure = *m_rightMeasure;

    //  Only save the left measure if the ignore flag or editLock has changed
    if (m_leftMeasure->IsIgnored() != origLeftMeasure->IsIgnored() ||
        m_leftMeasure->IsEditLocked() != origLeftMeasure->IsEditLocked()) {
      m_leftMeasure->SetChooserName(Application::UserName());
      *origLeftMeasure = *m_leftMeasure;
    }

    // If left measure == right measure, update left
    if (m_leftMeasure->GetCubeSerialNumber() ==
        m_rightMeasure->GetCubeSerialNumber()) {
      *m_leftMeasure = *m_rightMeasure;
      //  Update left measure of pointEditor
      m_pointEditor->setLeftMeasure (m_leftMeasure, m_leftCube.data(),
                                     m_editPoint->GetId());
    }

    //  Change Save Point button text to red
    colorizeSaveButton();

    editPointChanged(m_editPoint->GetId());

    // Update measure info
    updateLeftMeasureInfo();
    updateRightMeasureInfo();
    loadMeasureTable();
  }



  /**
   * Change which measure is the reference.
   *
   * @author 2012-04-26 Tracie Sucharski - moved funcitonality from measureSaved
   *
   * @return bool If false reset reference ChipViewport to original value.
   *
   * @internal
   *   @history 2012-06-12 Tracie Sucharski - Moved check for ground loaded on left from the
   *                          measureSaved method.
   *   @history 2015-06-01 Makayla Shepherd and Ian Humphrey - Modified to return a boolean
   *                          value to indicate if the user wants to update the reference or not
   *                          and modified the case when the user clicks no to reload the left and
   *                          right ChipViewports to reset back to the previous locations.
   */
  bool QnetTool::checkReference() {

    // Check if ControlPoint has reference measure, if reference Measure is
    // not the same measure that is on the left chip viewport, set left
    // measure as reference.
    ControlMeasure *refMeasure = m_editPoint->GetRefMeasure();
    if ( (m_leftMeasure->GetCubeSerialNumber() != m_groundSN) &&
         (refMeasure->GetCubeSerialNumber() != m_leftMeasure->GetCubeSerialNumber()) ) {
      QString message = "This point already contains a reference measure.  ";
      message += "Would you like to replace it with the measure on the left?";
      int  response = QMessageBox::question(m_qnetTool,
                                "Qnet Tool Save Measure", message,
                                QMessageBox::Yes | QMessageBox::No,
                                QMessageBox::Yes);
      // Replace reference measure
      if (response == QMessageBox::Yes) {
        //  Update measure file combo boxes:  old reference normal font,
        //    new reference bold font
        QString file = m_serialNumberList->fileName(m_leftMeasure->GetCubeSerialNumber());
        QString fname = FileName(file).name();
        int iref = m_leftCombo->findText(fname);

        //  Save normal font from new reference measure
        QVariant font = m_leftCombo->itemData(iref,Qt::FontRole);
        m_leftCombo->setItemData(iref,QFont("DejaVu Sans", 12, QFont::Bold), Qt::FontRole);
        iref = m_rightCombo->findText(fname);
        m_rightCombo->setItemData(iref,QFont("DejaVu Sans", 12, QFont::Bold), Qt::FontRole);

        file = m_serialNumberList->fileName(refMeasure->GetCubeSerialNumber());
        fname = FileName(file).name();
        iref = m_leftCombo->findText(fname);
        m_leftCombo->setItemData(iref,font,Qt::FontRole);
        iref = m_rightCombo->findText(fname);
        m_rightCombo->setItemData(iref,font,Qt::FontRole);

        m_editPoint->SetRefMeasure(m_leftMeasure->GetCubeSerialNumber());
        // Update reference measure to new reference measure
        refMeasure = m_editPoint->GetRefMeasure();
      }
      // ??? Need to set rest of measures to Candidate and add more warning. ???//
    }

    // If the right measure is the reference, make sure they really want
    // to move the reference.
    if (refMeasure->GetCubeSerialNumber() == m_rightMeasure->GetCubeSerialNumber()) {
      QString message = "You are making a change to the reference measure.  You ";
      message += "may need to move all of the other measures to match the new ";
      message += " coordinate of the reference measure.  Do you really want to ";
      message += " change the reference measure? ";
      switch(QMessageBox::question(m_qnetTool, "Qnet Tool Save Measure",
                                   message, "&Yes", "&No", 0, 0)){
        // Yes:  Save measure
        case 0:
          return true;
        // No:  keep original reference, return ChipViewports to previous states
        case 1:
          selectRightMeasure(m_rightCombo->currentIndex());
          selectLeftMeasure(m_leftCombo->currentIndex());
          return false;
      }
    }
    return true;
  }



  /*
  * Update the position of ground point
  *
  * @author 2012-04-26 Tracie Sucharski - moved functionality from measureSaved
  *
  * @internal
  *
  */
  void QnetTool::updateGroundPosition() {

    //  TODO:  If groundSource file opened does not match the SurfacePoint Source
    //  file, print warning.

    //  This method only called if ground measure is on the right.  Use ground measure to update
    //  apriori surface point.
    ControlMeasure *groundMeasure = m_rightMeasure;
    if (!m_groundGmap->SetImage(groundMeasure->GetSample(),
                                groundMeasure->GetLine())) {
      // TODO :  should never happen, either add error check or
       //      get rid of
    }

    double lat = m_groundGmap->UniversalLatitude();
    double lon = m_groundGmap->UniversalLongitude();
//    cout<<"lat = "<<setprecision(15)<<lat<<endl;
//    cout<<"lon = "<<lon<<endl;
    double radius;
    //  Update radius, order of precedence
    //  1.  If a dem has been opened, read radius from dem.
    //  2.  Get radius from reference measure
    //        If image has shape model, radius will come from shape model
    //
    if (m_demOpen) {
      radius = demRadius(lat,lon);
      if (radius == Null) {
        QString msg = "Could not read radius from DEM, will default to "
          "local radius of reference measure.";
        QMessageBox::warning(m_qnetTool, "Warning", msg);
        if (m_editPoint->GetRefMeasure()->Camera()->SetGround(
            Latitude(lat, Angle::Degrees), Longitude(lon, Angle::Degrees))) {
          radius =
            m_editPoint->GetRefMeasure()->Camera()->LocalRadius().meters();
            m_editPoint->SetAprioriRadiusSource(
                ControlPoint::RadiusSource::None);
            //m_editPoint->SetAprioriRadiusSourceFile(m_radiusSourceFile);
        }
        else {
          QString message = "Error trying to get radius at this pt.  "
              "Lat/Lon does not fall on the reference measure.  "
              "Cannot save this measure.";
          QMessageBox::critical(m_qnetTool,"Error",message);
          return;
        }
      }
      m_editPoint->SetAprioriRadiusSource(m_groundRadiusSource);
      m_editPoint->SetAprioriRadiusSourceFile(m_radiusSourceFile);
    }
    else {
      //  Get radius from reference image
      if (m_editPoint->GetRefMeasure()->Camera()->SetGround(
            Latitude(lat, Angle::Degrees), Longitude(lon, Angle::Degrees))) {
        radius =
            m_editPoint->GetRefMeasure()->Camera()->LocalRadius().meters();
//      cout.width(15);
//      cout.precision(4);
//      cout<<"Camera Radius = "<<fixed<<radius<<endl;
        //radius = m_groundGmap->Projection()->LocalRadius();
      }
      else {
        QString message = "Error trying to get radius at this pt.  "
            "Lat/Lon does not fall on the reference measure.  "
            "Cannot save this measure.";
        QMessageBox::critical(m_qnetTool,"Error",message);
        return;
      }
    }

    try {
      //  Read apriori surface point if it exists so that point is
      //  replaced, but sigmas are retained.  Save sigmas because the
      //  SurfacePoint class will change them if the coordinates change.
      if (m_editPoint->HasAprioriCoordinates()) {
        SurfacePoint aprioriPt = m_editPoint->GetAprioriSurfacePoint();
        Distance latSigma = aprioriPt.GetLatSigmaDistance();
        Distance lonSigma = aprioriPt.GetLonSigmaDistance();
        Distance radiusSigma = aprioriPt.GetLocalRadiusSigma();
        aprioriPt.SetSphericalCoordinates(Latitude(lat, Angle::Degrees),
                                          Longitude(lon, Angle::Degrees),
                                          Distance(radius, Distance::Meters));
        aprioriPt.SetSphericalSigmasDistance(latSigma, lonSigma, radiusSigma);
        m_editPoint->SetAprioriSurfacePoint(aprioriPt);
      }
      else {
        m_editPoint->SetAprioriSurfacePoint(SurfacePoint(
                                        Latitude(lat, Angle::Degrees),
                                        Longitude(lon, Angle::Degrees),
                                        Distance(radius, Distance::Meters)));
      }
    }
    catch (IException &e) {
      QString message = "Unable to set Apriori Surface Point.\n";
      message += "Latitude = " + QString::number(lat);
      message += "  Longitude = " + QString::number(lon);
      message += "  Radius = " + QString::number(radius) + "\n";
      message += e.toString();
      QMessageBox::critical(m_qnetTool,"Error",message);
    }
    m_editPoint->SetAprioriSurfacePointSource(m_groundSurfacePointSource);
    m_editPoint->SetAprioriSurfacePointSourceFile(m_groundSourceFile);

    updateSurfacePointInfo ();

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
  void QnetTool::savePoint () {

    //  Make a copy of edit point for updating the control net since the edit
    //  point is still loaded in the point editor.
    ControlPoint *updatePoint = new ControlPoint;
    *updatePoint = *m_editPoint;

    //  If this is a fixed or constrained point, see if there is a temporary
    //  measure holding the coordinate information from the ground source.
    //  If so, delete this measure before saving point.  Clear out the
    //  fixed Measure variable (memory deleted in ControlPoint::Delete).
    if (updatePoint->GetType() != ControlPoint::Free &&
        updatePoint->HasSerialNumber(m_groundSN)) {
      updatePoint->Delete(m_groundSN);
    }

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

    //  ????  Why was this here??? loadPoint();
    // emit signal so the nav tool refreshes the list
    emit refreshNavList();
    // emit signal so the nav tool can update edit point
    emit editPointChanged(m_editPoint->GetId());
    // emit a signal to alert user to save when exiting
    emit netChanged();
    //   Refresh chipViewports to show new positions of controlPoints
    m_pointEditor->refreshChips();
  }



  /**
   * Set the point type
   * @param pointType int Index from point type combo box
   *
   * @author 2011-07-05 Tracie Sucharski
   *
   * @internal
   *   @history 2013-12-06 Tracie Sucharski - If changing point type to constrained or fixed make
   *                           sure reference measure is not ignored.
   *   @history 2015-05-19 Ian Humphrey and Makayla Shepherd - When changing a ground point between
   *                           fixed and constrained and vice versa, the ground measure will not be
   *                           reloaded (otherwise m_editPoint->Add() will throw an exception
   *                           within a connected slot).
   */
  void QnetTool::setPointType (int pointType) {
    if (m_editPoint == NULL) return;

    //  If pointType is equal to current type, nothing to do
    if (m_editPoint->GetType() == pointType) return;

    if (pointType != ControlPoint::Free && m_leftMeasure->IsIgnored()) {
      m_pointType->setCurrentIndex((int) m_editPoint->GetType());
      QString message = "The reference measure is Ignored.  Unset the Ignore flag on the ";
      message += "reference measure before setting the point type to Constrained or Fixed.";
      QMessageBox::warning(m_qnetTool, "Ignored Reference Measure", message);
      return;
    }

    bool unloadGround = false;
    if (m_editPoint->GetType() != ControlPoint::Free && pointType == ControlPoint::Free)
      unloadGround = true;

    // save the old point's type
    int temp = m_editPoint->GetType();

    ControlPoint::Status status = m_editPoint->SetType((ControlPoint::PointType) pointType);
    if (status == ControlPoint::PointLocked) {
      m_pointType->setCurrentIndex((int) m_editPoint->GetType());
      QString message = "This control point is edit locked.  The point type cannot be changed.  You ";
      message += "must first unlock the point by clicking the check box above labeled ";
      message += "\"Edit Lock Point\".";
      QMessageBox::warning(m_qnetTool, "Point Locked", message);
      return;
    }

    // If ground loaded and changing from Free to ground point,
    // read temporary ground measure to the point
    // Note: changing between Fixed and Constrained will not reload the ground measure
    if (pointType != ControlPoint::Free && temp == ControlPoint::Free && m_groundOpen) {
      loadGroundMeasure();
      m_pointEditor->colorizeSaveButton();
    }
    //  If going from constrained or fixed to free, unload the ground measure.
    else if (unloadGround) {
      // Find in point and delete, it will be re-created with current
      // ground source if this is a fixed point
      if (m_editPoint->HasSerialNumber(m_groundSN)) {
          m_editPoint->Delete(m_groundSN);
      }

      loadPoint();
      m_pointEditor->colorizeSaveButton();
    }

    colorizeSaveButton();
  }



  /**
   * Load ground measure into right side and add to file combo boxes.
   *
   * @author 2013-12-06 Tracie Sucharski
   *
   * @internal
   *   @history 2013-12-06 Tracie Sucharski - Original version.
   *   @history 2015-05-19 Ian Humphrey and Makayla Shepherd - moved duplicated code to
   *                           findPointLocation() and createTemporaryGroundMeasure().
   *
   */
  void QnetTool::loadGroundMeasure () {

    if (!m_groundOpen) return;

    if (findPointLocation()) {
      ControlMeasure *groundMeasure = createTemporaryGroundMeasure();

      // Add to measure combo boxes
      QString file = m_serialNumberList->fileName(groundMeasure->GetCubeSerialNumber());
      m_pointFiles<<file;
      QString tempFileName = FileName(file).name();

      m_leftCombo->addItem(tempFileName);
      m_rightCombo->addItem(tempFileName);
      int rightIndex = m_rightCombo->findText((QString)m_groundFile);
      m_rightCombo->setCurrentIndex(rightIndex);
      selectRightMeasure(rightIndex);

      updateSurfacePointInfo ();

      loadMeasureTable();
    }
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
  void QnetTool::setLockPoint (bool lock) {
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
  void QnetTool::setIgnorePoint (bool ignore) {
    if (m_editPoint == NULL) return;

    ControlPoint::Status status = m_editPoint->SetIgnored(ignore);
    if (status == ControlPoint::PointLocked) {
      m_ignorePoint->setChecked(m_editPoint->IsIgnored());
      QString message = "This control point is edit locked.  The Ignored status cannot be ";
      message += "changed.  You must first unlock the point by clicking the check box above ";
      message += "labeled \"Edit Lock Point\".";
      QMessageBox::warning(m_qnetTool, "Point Locked", message);
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
  void QnetTool::setLockLeftMeasure (bool lock) {

    if (m_editPoint->IsEditLocked()) {
      m_lockLeftMeasure->setChecked(m_leftMeasure->IsEditLocked());
      QMessageBox::warning(m_qnetTool, "Point Locked","Point is Edit Locked.  You must un-lock point"
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
  void QnetTool::setIgnoreLeftMeasure (bool ignore) {
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
  void QnetTool::setLockRightMeasure (bool lock) {

    if (m_editPoint->IsEditLocked()) {
      m_lockRightMeasure->setChecked(m_rightMeasure->IsEditLocked());
      QMessageBox::warning(m_qnetTool, "Point Locked","Point is Edit Locked.  You must un-lock point"
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
  void QnetTool::setIgnoreRightMeasure (bool ignore) {
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



  /**
   * Signal to save control net
   *
   * @author 2011-10-31 Tracie Sucharski
   */
  void QnetTool::saveNet() {
    if (m_cnetFileName.isEmpty()) {
      QString message = "This is a new network, you must select "
                        "\"Save As\" under the File Menu.";
      QMessageBox::critical(m_qnetTool, "Error", message);
      return;
    }
    emit qnetToolSave();
    //m_controlNet->Write(m_cnetFileName);
  }



  /**
   * Signal to save the control net.
   */
  void QnetTool::saveAsNet () {
    emit qnetToolSaveAs();
  }




  /**
   *
   */
  void QnetTool::updateList() {

    //m_pointEditor->setSerialList (*m_serialNumberList);

  }


  /**
   * Updates the Control Network displayed in the Qnet Tool title
   * bar. This slot is connected to QnetFileTool's
   * controlNetworkUpdated(QString cNetFileName) signal.
   *
   * @param cNetFileName FileName of the most recently selected
   *                     control network.
   * @see QnetFileTool
   * @internal
   *   @history Tracie Sucharski - Original version
   *   @history 2008-11-26 Jeannie Walldren - Added cNetFileName input parameter
   *                          in order to show the file path in the window's title
   *                          bar.
   *   @history 2011-10-31 Tracie Sucharski - Save filename for implementation
   *                          of Save option.
   */
  void QnetTool::updateNet(QString cNetFileName) {
    m_cnetFileName = cNetFileName;
    m_qnetTool->setWindowTitle("Qnet Tool - Control Network File: " +
                               cNetFileName);
    //m_pointEditor->setControlNet(*m_controlNet);

  }



  /**
   * Adds the Tie tool action to the tool pad.  When the Tie tool is selected, the
   * Navigation Tool will automatically open.
   *
   * @param pad Tool pad
   * @return @b QAction* Pointer to Tie tool action
   *
   * @internal
   *   @history 2010-07-01 Jeannie Walldren - Added connection between qnet's
   *                          TieTool button and the showNavWindow() method
   */
  QAction *QnetTool::toolPadAction(ToolPad *pad) {
    QAction *action = new QAction(pad);
    action->setIcon(QPixmap(toolIconDir()+"/stock_draw-connector-with-arrows.png"));
    action->setToolTip("Control Point Editor (T)");
    action->setShortcut(Qt::Key_T);
    QObject::connect(action,SIGNAL(triggered(bool)),this,SLOT(showNavWindow(bool)));
    return action;
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
   *                          new point. Change m_leftFile from a std::string to
   *                          a QString.
   * @history 2013-05-09 Tracie Sucharski - For editing (left button) and deleting (right button),
   *                          Swapped checking for empty network and not allowing mouse clicks on
   *                          the ground source. First check if there are any points in the network.
   *                          If not print message and return.
   * @history 2013-12-17 Tracie Sucharski - Check for valid serial number at beginning.  This
   *                          prevents seg fault if user opens a new cube list but clicks on a
   *                          cube that was open from a previous list.
   * @history 2015-05-13 Ian Humphrey and Makayla Shepherd - Add try/catch when trying to find
   *                          closest control point. Since FindClosest() can throw exceptions,
   *                          we need to handle them within this connected slot to avoid undefined
   *                          behavior.
   */
  void QnetTool::mouseButtonRelease(QPoint p, Qt::MouseButton s) {
    MdiCubeViewport *cvp = cubeViewport();
    if (cvp  == NULL) return;

    QString file = cvp->cube()->fileName();
    QString sn;
    try {
      sn = m_serialNumberList->serialNumber(file);
    }
    catch (IException &e) {
      QString message = "Cannot get serial number for " + file + ".  Is file contained in the ";
      message += "cube list?\n";
      message += e.toString();
      QMessageBox::critical(m_qnetTool, "Error", message);
      return;
    }

    double samp,line;
    cvp->viewportToCube(p.x(),p.y(),samp,line);

    m_leftFile.clear();

    if (s == Qt::LeftButton) {
      if (!m_controlNet || m_controlNet->GetNumPoints() == 0) {
        QString message = "No points exist for editing.  Create points ";
        message += "using the right mouse button.";
        QMessageBox::warning(m_qnetTool, "Warning", message);
        return;
      }

      if (sn == m_groundSN) {
        QString message = "Cannot select point for editing on ground source.  Select ";
        message += "point using un-projected images or the Navigator Window.";
        QMessageBox::critical(m_qnetTool, "Error", message);
        return;
      }

      //  Find closest control point in network
      QString sn = m_serialNumberList->serialNumber(file);

      // since we are in a connected slot, we need to handle exceptions thrown by FindClosest
      try {
        ControlPoint *point = m_controlNet->FindClosest(sn, samp, line);
        modifyPoint(point);
      }
      catch (IException &ie) {
        QString message = "No points exist for editing. Create points using the right mouse";
        message += " button.";
        QMessageBox::warning(m_qnetTool, "Warning", message);
        return;
      }
    }

    else if (s == Qt::MiddleButton) {
      if (!m_controlNet || m_controlNet->GetNumPoints() == 0) {
        QString message = "No points exist for deleting.  Create points ";
        message += "using the right mouse button.";
        QMessageBox::warning(m_qnetTool, "Warning", message);
        return;
      }

      if (m_groundOpen && file == m_groundCube->fileName()) {
        QString message = "Cannot select point for deleting on ground source.  Select ";
        message += "point using un-projected images or the Navigator Window.";
        QMessageBox::critical(m_qnetTool, "Error", message);
        return;
      }
      //  Find closest control point in network
      ControlPoint *point = NULL;
      try {
        point = m_controlNet->FindClosest(sn, samp, line);

        if (point == NULL) {
          QString message = "No points exist for deleting.  Create points ";
          message += "using the right mouse button.";
          QMessageBox::warning(m_qnetTool, "Warning", message);
          return;
        }
      }
      catch (IException &e) {
        QString message = "Cannot find point on this image for deleting.";
        QMessageBox::critical(m_qnetTool, "Error", message);
        return;
      }

      deletePoint(point);
    }
    else if (s == Qt::RightButton) {
      m_leftFile = file;
      UniversalGroundMap *gmap = cvp->universalGroundMap();
      if (!gmap->SetImage(samp,line)) {
        QString message = "Invalid latitude or longitude at this point. ";
        QMessageBox::critical(m_qnetTool,"Error", message);
        return;
      }
      double lat = gmap->UniversalLatitude();
      double lon = gmap->UniversalLongitude();
      if (m_groundOpen && file == m_groundCube->fileName()) {
        createFixedPoint (lat,lon);
      }
      else {
        createPoint(lat,lon);
      }
    }
  }



  /**
   *   Create new control point
   *
   * @param lat Latitude value of control point to be created.
   * @param lon Longitude value of control point to be created.
   *
   * @internal
   *   @history 2008-11-20 Jeannie Walldren - Added message box if pointID value
   *                          entered already exists for another ControlPoint.
   *                          Previously this resulted in a PROGRAMMER ERROR from
   *                          ControlPoint. Now, the user will be alerted in a
   *                          message box and prompted to enter a new value for
   *                          pointID.
   *   @history 2008-12-03  Tracie Sucharski - Add error message and cancel
   *                          create new point if the point falls on a single
   *                          image.
   *   @history 2008-12-15  Jeannie Walldren - Throw and catch error before
   *                           creating QMessageBox
   *   @history 2009-03-09  Jeannie Walldren - Clear error message stack after it
   *                           is displayed to user in message box.
   *   @history 2009-04-20  Tracie Sucharski - Set camera for each measure.
   *   @history 2010-06-03 Jeannie Walldren - Removed "std::" since "using
   *                          namespace std"
   *   @history 2010-07-21  Tracie Sucharski - Modified for new keywords
   *                           associated with implementation of binary
   *                           control networks.
   *   @history 2010-11-19  Tracie Sucharski - Changed m_controlPoint to
   *                           m_editPoint which is a copy rather than a pointer
   *                           directly to the network.
   *   @history 2010-12-15 Tracie Sucharski - Remove netChanged, the point is
   *                           not changed in the net unless "Save Point" is
   *                           selected.
   *   @history 2011-03-31 Tracie Sucharski - Remove check for point only
   *                           existing on a single image.  This will be
   *                           shown on new point dialog and user can always
   *                           hit "Cancel".
   *   @history 2011-04-08 Tracie Sucharski - Added check for NULL pointer
   *                           before deleting m_editPOint if parent is NULL.
   *   @history 2011-07-19 Tracie Sucharski - Remove call to
   *                           SetAprioriSurfacePoint, this should only be
   *                           done for constrained or fixed points.
   *   @history 2012-05-08 Tracie Sucharski - m_leftFile changed from std::string to QString.
   *
   */
  void QnetTool::createPoint(double lat,double lon) {

    //  TODO:   ADD AUTOSEED OPTION (CHECKBOX?)

    //  Create list box of all files highlighting those that
    //  contain the point.
    QStringList pointFiles;

    Camera *cam;
    for(int i = 0; i < m_serialNumberList->size(); i++) {
      if (m_serialNumberList->serialNumber(i) == m_groundSN) continue;
      cam = m_controlNet->Camera(i);
      if(cam->SetUniversalGround(lat, lon)) {
        //  Make sure point is within image boundary
        double samp = cam->Sample();
        double line = cam->Line();
        if (samp >= 1 && samp <= cam->Samples() &&
            line >= 1 && line <= cam->Lines()) {
          pointFiles<<m_serialNumberList->fileName(i);
        }
      }
    }

    NewControlPointDialog *newPointDialog =
        new NewControlPointDialog(m_controlNet, m_serialNumberList, m_lastUsedPointId);
    newPointDialog->setFiles(pointFiles);
    if (newPointDialog->exec()) {
      m_lastUsedPointId = newPointDialog->pointId();
      ControlPoint *newPoint =
          new ControlPoint(m_lastUsedPointId);

      // If this ControlPointId already exists, message box pops up and user is
      // asked to enter a new value.
      if (m_controlNet->ContainsPoint(newPoint->GetId())) {
        QString message = "A ControlPoint with Point Id = [" + newPoint->GetId();
        message += "] already exists.  Re-enter Point Id for this ControlPoint.";
        QMessageBox::warning(m_qnetTool, "New Point Id", message);
        pointFiles.clear();
        delete newPoint;
        newPoint = NULL;
        createPoint(lat, lon);
        return;
      }

      newPoint->SetChooserName(Application::UserName());

      QStringList selectedFiles = newPointDialog->selectedFiles();
      foreach (QString selectedFile, selectedFiles) {
        //  Create measure for any file selected
        ControlMeasure *m = new ControlMeasure;
        //  Find serial number for this file
        QString sn =
                  m_serialNumberList->serialNumber(selectedFile);
        m->SetCubeSerialNumber(sn);
        int camIndex =
              m_serialNumberList->fileNameIndex(selectedFile);
        cam = m_controlNet->Camera(camIndex);
        cam->SetUniversalGround(lat,lon);
        m->SetCoordinate(cam->Sample(),cam->Line());
        m->SetAprioriSample(cam->Sample());
        m->SetAprioriLine(cam->Line());
        m->SetType(ControlMeasure::Manual);
        m->SetChooserName(Application::UserName());
        m->SetCamera(cam);
        newPoint->Add(m);
      }
      if (m_editPoint != NULL && m_editPoint->Parent() == NULL) {
        delete m_editPoint;
        m_editPoint = NULL;
      }
      m_editPoint = newPoint;

      //  If the image that the user clicked on to select the point is not
      //  included, clear out the leftFile value.
      if (!m_leftFile.isEmpty()) {
        if (selectedFiles.indexOf(m_leftFile) == -1) {
          m_leftFile.clear();
        }
      }

      //  Load new point in QnetTool
      loadPoint();
      m_qnetTool->setVisible(true);
      m_qnetTool->raise();

      loadTemplateFile(
          m_pointEditor->templateFileName());


      // emit signal so the nave tool refreshes the list
      emit refreshNavList();
      // emit signal so the nav tool can update edit point
      emit editPointChanged(m_editPoint->GetId());
      colorizeSaveButton();
      delete newPointDialog;

    }
  }






  /**
   *   Create new Fixed control point
   *
   * @param lat Latitude value of control point to be created.
   * @param lon Longitude value of control point to be created.
   *
   * @author 2010-11-09 Tracie Sucharski
   *
   * @internal
   *
   */
  void QnetTool::createFixedPoint(double lat,double lon) {

    //  TODO:   ADD AUTOSEED OPTION (CHECKBOX?)

    //  Create list of list box of all files highlighting those that
    //  contain the point.
    QStringList pointFiles;

    Camera *cam;
    for (int i=0; i<m_serialNumberList->size(); i++) {
      if (m_serialNumberList->serialNumber(i) == m_groundSN) continue;
      cam = m_controlNet->Camera(i);
      if (cam->SetUniversalGround(lat,lon)) {
        //  Make sure point is within image boundary
        double samp = cam->Sample();
        double line = cam->Line();
        if (samp >= 1 && samp <= cam->Samples() &&
            line >= 1 && line <= cam->Lines()) {
          pointFiles<<m_serialNumberList->fileName(i);
        }
      }
    }

    if (pointFiles.count() == 0) {
      QString message = "Point does not intersect any images.";
      QMessageBox::critical(m_qnetTool, "No intersection", message);
      return;
    }

    QnetFixedPointDialog *fixedPointDialog = new QnetFixedPointDialog(this, m_lastUsedPointId);
    fixedPointDialog->setFiles(pointFiles);
    if (fixedPointDialog->exec()) {
      ControlPoint *fixedPoint =
      new ControlPoint(fixedPointDialog->pointId());

      if (fixedPointDialog->isFixed()) {
        fixedPoint->SetType(ControlPoint::Fixed);
      }
      else {
        fixedPoint->SetType(ControlPoint::Constrained);
      }

      // If this ControlPointId already exists, message box pops up and user is
      // asked to enter a new value.
      if (m_controlNet->ContainsPoint(fixedPoint->GetId())) {
        QString message = "A ControlPoint with Point Id = [" + fixedPoint->GetId();
        message += "] already exists.  Re-enter Point Id for this ControlPoint.";
        QMessageBox::warning(m_qnetTool, "New Point Id", message);
        pointFiles.clear();
        delete fixedPoint;
        fixedPoint = NULL;
        createFixedPoint(lat,lon);
        return;
      }

      fixedPoint->SetChooserName(Application::UserName());

      QStringList selectedFiles = fixedPointDialog->selectedFiles();
      foreach (QString selectedFile, selectedFiles) {
        //  Create measure for any file selected
        ControlMeasure *m = new ControlMeasure;
        //  Find serial number for this file
        QString sn =
            m_serialNumberList->serialNumber(selectedFile);

        //  If ground, do not add measure, it will be added in loadPoint
        if (sn == m_groundSN) continue;

        m->SetCubeSerialNumber(sn);
        int camIndex =
                 m_serialNumberList->fileNameIndex(selectedFile);
        cam = m_controlNet->Camera(camIndex);
        cam->SetUniversalGround(lat,lon);
        m->SetCoordinate(cam->Sample(),cam->Line());
        m->SetType(ControlMeasure::Manual);
        m->SetChooserName(Application::UserName());
        m->SetCamera(cam);
        fixedPoint->Add(m);
      }

      //  ??????       What radius , check for dem or shape model
      double radius = 0;
      if (m_demOpen) {
        radius = demRadius(lat,lon);
        if (radius == Null) {
          QString msg = "Could not read radius from DEM, will default to the "
            "local radius of the first measure in the control point.  This "
            "will be updated to the local radius of the chosen reference "
            "measure.";
          QMessageBox::warning(m_qnetTool, "Warning", msg);
          if ((*fixedPoint)[0]->Camera()->SetGround(
               Latitude(lat, Angle::Degrees), Longitude(lon, Angle::Degrees))) {
            radius = (*fixedPoint)[0]->Camera()->LocalRadius().meters();
          }
          else {
            QString msg = "Error trying to get radius at this pt.  "
                "Lat/Lon does not fall on the reference measure.  "
                "Cannot create this point.";
            QMessageBox::critical(m_qnetTool, "Error", msg);
            delete fixedPoint;
            fixedPoint = NULL;
            delete fixedPointDialog;
            fixedPointDialog = NULL;
            return;
          }
        }
      }
      else {
        if ((*fixedPoint)[0]->Camera()->SetGround(
             Latitude(lat, Angle::Degrees), Longitude(lon, Angle::Degrees))) {
          radius = (*fixedPoint)[0]->Camera()->LocalRadius().meters();
        }
        else {
          QString msg = "Error trying to get radius at this pt.  "
              "Lat/Lon does not fall on the reference measure.  "
              "Cannot create this point.";
          QMessageBox::critical(m_qnetTool, "Error", msg);
          delete fixedPoint;
          fixedPoint = NULL;
          delete fixedPointDialog;
          fixedPointDialog = NULL;
          return;
        }
      }

      fixedPoint->SetAprioriSurfacePoint(SurfacePoint(
                                          Latitude(lat, Angle::Degrees),
                                          Longitude(lon, Angle::Degrees),
                                          Distance(radius, Distance::Meters)));

      if (m_editPoint != NULL && m_editPoint->Parent() == NULL) {
        delete m_editPoint;
        m_editPoint = NULL;
      }
      m_editPoint = fixedPoint;

      //  Load new point in QnetTool
      loadPoint();
      m_qnetTool->setVisible(true);
      m_qnetTool->raise();

      delete fixedPointDialog;
      fixedPointDialog = NULL;

      // emit signal so the nave tool refreshes the list
      emit refreshNavList();
      // emit signal so the nav tool can update edit point
      emit editPointChanged(m_editPoint->GetId());
      colorizeSaveButton();
    }
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
   * @history 2013-05-09 Tracie Sucharski - Check for user selecting all measures for deletion and
   *                          print warning that point will be deleted.
   *
   */
  void QnetTool::deletePoint(ControlPoint *point) {

    // Make a copy and make sure editPoint is a copy (which means it does not
    // have a parent network.
    if (m_editPoint != NULL && m_editPoint->Parent() == NULL) {
      delete m_editPoint;
      m_editPoint = NULL;
    }
    m_editPoint = new ControlPoint;
    *m_editPoint = *point;
    loadPoint();

    //  Change point in viewport to red so user can see what point they are
    //  about to delete.
    // the nav tool will update edit point
    emit editPointChanged(m_editPoint->GetId());

    QnetDeletePointDialog *deletePointDialog = new QnetDeletePointDialog;
    QString CPId = m_editPoint->GetId();
    deletePointDialog->pointIdValue->setText(CPId);

    //  Need all files for this point
    for (int i=0; i<m_editPoint->GetNumMeasures(); i++) {
      ControlMeasure &m = *(*m_editPoint)[i];
      QString file = m_serialNumberList->fileName(m.GetCubeSerialNumber());
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
          int  response = QMessageBox::question(m_qnetTool,
                                    "Delete control point", message,
                                    QMessageBox::Yes | QMessageBox::No,
                                    QMessageBox::Yes);
          // If No, do nothing
          if (response == QMessageBox::No) {
            return;
          }
        }

        //  First get rid of deleted point from m_filteredPoints list
        //  need index in control net for pt
        //int i = m_controlNet->
        //m_filteredPoints.
        m_qnetTool->setVisible(false);
        // remove this point from the control network
        if (m_controlNet->DeletePoint(m_editPoint->GetId()) ==
                                          ControlPoint::PointLocked) {
          QMessageBox::information(m_qnetTool, "EditLocked Point",
              "This point is EditLocked and cannot be deleted.");
          return;
        }
        if (m_editPoint != NULL && m_editPoint->Parent() == NULL) {
          delete m_editPoint;
          m_editPoint = NULL;
        }
        //  emit signal so the nav tool refreshes the list
        emit refreshNavList();
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
            switch (QMessageBox::question(m_qnetTool,
                                          "Delete Reference measure?", message,
                                          "&Yes", "&No", 0, 0)) {
              //  Yes:  skip to end of switch todelete the measure
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
          QMessageBox::information(m_qnetTool,"EditLocked Measures",
                QString::number(lockedMeasures) + " / "
                + QString::number(
                  deletePointDialog->fileList->selectedItems().size()) +
                " measures are EditLocked and were not deleted.");
        }

        loadPoint();
        m_qnetTool->setVisible(true);
        m_qnetTool->raise();

        loadTemplateFile(m_pointEditor->templateFileName());
      }

      // emit a signal to alert user to save when exiting
      emit netChanged();

      // emit signal so the nav tool can update edit point
      if (m_editPoint != NULL) {
        emit editPointChanged(m_editPoint->GetId());
        //  Change Save Point button text to red
        colorizeSaveButton();
      }
      else {
        // if the entire point is deleted, update with point Id = ""
        // this signal is connected to QnetTool::paintAllViewports
        // and QnetNavTool::updateEditPoint
        emit editPointChanged("");
      }
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
  void QnetTool::modifyPoint(ControlPoint *point) {

    //  If no measures, print info and return
    if (point->GetNumMeasures() == 0) {
      QString message = "This point has no measures.";
      QMessageBox::warning(m_qnetTool, "Warning", message);
      // update nav list to re-highlight old point
      if (m_editPoint != NULL) {
        // emit signal so the nav tool can update edit point
        emit editPointChanged(m_editPoint->GetId());
      }
      else {
        // this signal is connected to QnetTool::paintAllViewports
        // and QnetNavTool::updateEditPoint
        emit editPointChanged("");
      }
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

    //  If navTool modfify button pressed, m_leftFile needs to be reset
    //  TODO: better way - have 2 slots
    if (sender() != this) m_leftFile.clear();
    loadPoint();
    m_qnetTool->setVisible(true);
    m_qnetTool->raise();
    loadTemplateFile(m_pointEditor->templateFileName());

    // emit signal so the nav tool can update edit point
    emit editPointChanged(m_editPoint->GetId());

    // New point loaded, make sure Save Measure Button text is default
    m_savePoint->setPalette(m_saveDefaultPalette);
  }


  /**
   * @brief Attempt to find the control point's location on the ground source
   *
   * @return bool true if the location is found on the ground source
   *
   * @internal
   *   @history 2015-05-19 Ian Humphrey and Makayla Shepherd - Orignal version adapted from
   *                           loadPoint() to encapsulate duplicated code in loadGroundMeasure().
   */
  bool QnetTool::findPointLocation() {
    bool located = true;

    // Use apriori surface point to find location on ground source.  If
      // apriori surface point does not exist use reference measure
      double lat = 0.;
      double lon = 0.;
      if (m_editPoint->HasAprioriCoordinates()) {
        SurfacePoint sPt = m_editPoint->GetAprioriSurfacePoint();
        lat = sPt.GetLatitude().degrees();
        lon = sPt.GetLongitude().degrees();
      }
      else {
        ControlMeasure m = *(m_editPoint->GetRefMeasure());
        int camIndex = m_serialNumberList->serialNumberIndex(m.GetCubeSerialNumber());
        Camera *cam;
        cam = m_controlNet->Camera(camIndex);
        cam->SetImage(m.GetSample(),m.GetLine());
        lat = cam->UniversalLatitude();
        lon = cam->UniversalLongitude();
      }

      //  Try to locate point position on current ground source,
      //  TODO ???if doesn't exist,???
      if (!m_groundGmap->SetUniversalGround(lat,lon)) {
        located = false;
        QString message = "This point does not exist on the ground source.\n";
        message += "Latitude = " + QString::number(lat);
        message += "  Longitude = " + QString::number(lon);
        message += "\n A ground measure will not be created.";
        QMessageBox::warning(m_qnetTool, "Warning", message);
      }

      return located;
  }


  /**
   * @brief Create a temporary measure to hold the ground point info for ground source
   *
   * @return ControlMeasure* the created ground measure
   *
   * @internal
   *   @history 2015-05-19 Ian Humphrey and Makayla Shepherd - Original version adapted from
   *                           loadPoint() to encapsulate duplicated code in loadGroundMeasure().
   */
  ControlMeasure *QnetTool::createTemporaryGroundMeasure() {

     // This measure will be deleted when the ControlPoint is saved to the
     // ControlNet.
     ControlMeasure *groundMeasure = new ControlMeasure;
     groundMeasure->SetCubeSerialNumber(m_groundSN);
     groundMeasure->SetType(ControlMeasure::Candidate);
     groundMeasure->SetCoordinate(m_groundGmap->Sample(),m_groundGmap->Line());
     m_editPoint->Add(groundMeasure);

     return groundMeasure;
  }


  /**
   * @brief Load point into QnetTool.
   * @internal
   *   @history 2008-11-26  Jeannie Walldren - Added "Number of Measures" to
   *                           QnetTool point information.
   *   @history 2010-06-03 Jeannie Walldren - Removed "std::" since "using
   *                          namespace std"
   *   @history 2010-10-29 Tracie Sucharski - Changed pointfiles to QStringList
   *   @history 2011-04-20 Tracie Sucharski - Was not setting EditLock check box
   *   @history 2011-07-18 Tracie Sucharski - Fixed bug with loading
   *                          ground measure-use AprioriSurface point, not lat,lon
   *                          of reference measure unless there is no apriori
   *                          surface point.
   *   @history 2012-05-08 Tracie Sucharski - m_leftFile changed from std::string to QString.
   *   @history 2012-09-28 Tracie Sucharski - When looking for ground source in right combo, use
   *                          fileName, not the serial number.  The ground source serial number
   *                          will not be the fileName if the Instrument group is retained in the
   *                          labels.  Fixes #1018
   *   @history 2015-05-19 Ian Humphrey and Makayla Shepherd - moved duplicated code to
   *                          findPointLocation() and createTemporaryGroundMeasure().
   *   @history 2016-10-07 Makayla Shepherd - Added radius source handling if there is a ground
   *                          source and there is not a radius source already open.
   */
  void QnetTool::loadPoint () {

    //  Write pointId
    QString CPId = m_editPoint->GetId();
    QString ptId("Point ID:  ");
    ptId += (QString) CPId;
    m_ptIdValue->setText(ptId);

    //  Write point type
//    QString ptType("Point Type:  ");
//    ptType += (QString) m_editPoint->GetPointTypeString();
//    m_pointType->setText(ptType);
    m_pointType->setCurrentIndex((int) m_editPoint->GetType());

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

    // Find in point and delete, it will be re-created with current
    // ground source if this is a fixed point
    if (m_editPoint->HasSerialNumber(m_groundSN)) {
        m_editPoint->Delete(m_groundSN);
    }

    //  If fixed, add ground source file to combos, create a measure for
    //  the ground source, load reference on left, ground source on right
    if (m_groundOpen && (m_editPoint->GetType() != ControlPoint::Free)) {

      if (findPointLocation()) {
        // Create a temporary measure to hold the ground point info for ground source
        // This measure will be deleted when the ControlPoint is saved to the
        // ControlNet.
        // TODO:  Does open ground source match point ground source
        createTemporaryGroundMeasure();
      }
    }

    //  Load a radius source if there isn't a radius source already open, and there is a ground source
    if (m_groundOpen && !m_demOpen) {
      openReferenceRadius();
    }


    //  Need all files for this point
    for (int i=0; i<m_editPoint->GetNumMeasures(); i++) {
      ControlMeasure &m = *(*m_editPoint)[i];
      QString file = m_serialNumberList->fileName(m.GetCubeSerialNumber());
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
      if ((m_editPoint->GetType() == ControlPoint::Free) && !m_leftFile.isEmpty()) {
        QString baseFileName = FileName(m_leftFile).name();
        leftIndex = m_leftCombo->findText(baseFileName);
        //  Sanity check
        if (leftIndex < 0 ) leftIndex = 0;
      }
    }

    //  Determine index for right measure.
    //  First, try to find correct ground.  If no correct ground, set right index to either 0 or 1,
    //  depending on value of the left index.
    if (m_groundOpen && (m_editPoint->GetType() != ControlPoint::Free))  {
      rightIndex = m_rightCombo->findText((QString)m_groundFile);
    }
    if (rightIndex <= 0) {
      if (leftIndex == 0) {
        rightIndex = 1;
      }
      else {
        rightIndex = 0;
      }
    }

    //  Handle pts with a single measure, for now simply put measure on left/right
    //  Evenutally put on left with black on right??
    if (rightIndex > m_editPoint->GetNumMeasures()-1) rightIndex = 0;
    m_rightCombo->setCurrentIndex(rightIndex);
    m_leftCombo->setCurrentIndex(leftIndex);
    //  Initialize pointEditor with measures
    selectLeftMeasure(leftIndex);
    selectRightMeasure(rightIndex);

    updateSurfacePointInfo ();

    loadMeasureTable();
  }





  /**
   * @brief Load measure information into the measure table
   * @internal
   *   @history 2011-12-05 Tracie Sucharski - Turn off sorting until table
   *                          is loaded.
   *
   */
  void QnetTool::loadMeasureTable () {
    if (m_measureWindow == NULL) {
      m_measureWindow = new QMainWindow();
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

      QString file = m_serialNumberList->fileName(m.GetCubeSerialNumber());
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



  QString QnetTool::measureColumnToString(QnetTool::MeasureColumns column) {
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


  ControlNet *QnetTool::controlNet() {
    return m_controlNet;
  }


  const ControlNet *QnetTool::controlNet() const {
    return m_controlNet;
  }


  SerialNumberList *QnetTool::serialNumberList() {
    return m_serialNumberList;
  }


  const SerialNumberList *QnetTool::serialNumberList() const {
    return m_serialNumberList;
  }


  Workspace *QnetTool::workspace() const {
    return m_workspace;
  }


  /**
   * Update the Surface Point Information in the QnetTool window
   *
   * @author 2011-03-01 Tracie Sucharski
   *
   * @internal
   * @history 2011-05-12 Tracie Sucharski - Type printing Apriori Values
   * @history 2011-05-24 Tracie Sucharski - Set target radii on apriori
   *                        surface point, so that sigmas can be converted to
   *                        meters.
   */
  void QnetTool::updateSurfacePointInfo () {

    QString s;

    SurfacePoint aprioriPoint = m_editPoint->GetAprioriSurfacePoint();
    if (aprioriPoint.GetLatitude().degrees() == Null) {
      s = "Apriori Latitude:  Null";
    }
    else {
      s = "Apriori Latitude:  " +
          QString::number(aprioriPoint.GetLatitude().degrees());
    }
    m_pointAprioriLatitude->setText(s);
    if (aprioriPoint.GetLongitude().degrees() == Null) {
      s = "Apriori Longitude:  Null";
    }
    else {
      s = "Apriori Longitude:  " +
          QString::number(aprioriPoint.GetLongitude().degrees());
    }
    m_pointAprioriLongitude->setText(s);
    if (aprioriPoint.GetLocalRadius().meters() == Null) {
      s = "Apriori Radius:  Null";
    }
    else {
      s = "Apriori Radius:  " +
          QString::number(aprioriPoint.GetLocalRadius().meters(),'f',2) +
          " <meters>";
    }
    m_pointAprioriRadius->setText(s);

    if (aprioriPoint.Valid()) {
      if (aprioriPoint.GetLatSigmaDistance().meters() == Null) {
        s = "Apriori Latitude Sigma:  Null";
      }
      else {
        s = "Apriori Latitude Sigma:  " +
            QString::number(aprioriPoint.GetLatSigmaDistance().meters()) +
            " <meters>";
      }
      m_pointAprioriLatitudeSigma->setText(s);
      if (aprioriPoint.GetLonSigmaDistance().meters() == Null) {
        s = "Apriori Longitude Sigma:  Null";
      }
      else {
        s = "Apriori Longitude Sigma:  " +
            QString::number(aprioriPoint.GetLonSigmaDistance().meters()) +
            " <meters>";
      }
      m_pointAprioriLongitudeSigma->setText(s);
      if (aprioriPoint.GetLocalRadiusSigma().meters() == Null) {
        s = "Apriori Radius Sigma:  Null";
      }
      else {
        s = "Apriori Radius Sigma:  " +
            QString::number(aprioriPoint.GetLocalRadiusSigma().meters()) +
            " <meters>";
      }
      m_pointAprioriRadiusSigma->setText(s);
    }
    else {
      s = "Apriori Latitude Sigma:  Null";
      m_pointAprioriLatitudeSigma->setText(s);
      s = "Apriori Longitude Sigma:  Null";
      m_pointAprioriLongitudeSigma->setText(s);
      s = "Apriori Radius Sigma:  Null";
      m_pointAprioriRadiusSigma->setText(s);
    }


    SurfacePoint point = m_editPoint->GetAdjustedSurfacePoint();
    if (point.GetLatitude().degrees() == Null) {
      s = "Adjusted Latitude:  Null";
    }
    else {
      s = "Adjusted Latitude:  " + QString::number(point.GetLatitude().degrees());
    }
    m_pointLatitude->setText(s);
    if (point.GetLongitude().degrees() == Null) {
      s = "Adjusted Longitude:  Null";
    }
    else {
      s = "Adjusted Longitude:  " + QString::number(point.GetLongitude().degrees());
    }
    m_pointLongitude->setText(s);
    if (point.GetLocalRadius().meters() == Null) {
      s = "Adjusted Radius:  Null";
    }
    else {
      s = "Adjusted Radius:  " +
          QString::number(point.GetLocalRadius().meters(),'f',2) + " <meters>";
    }
    m_pointRadius->setText(s);



  }



  /**
   * @brief Selects the next right measure when activated by key shortcut
   *
   * This slot is intended to handle selecting the next right measure when the attached shortcut
   * (PageDown) is activated. This slot checks if the next index is in bounds.
   *
   * @internal
   *   @history 2015-10-29 Ian Humphrey - Created slot. References #2324.
   */
  void QnetTool::nextRightMeasure() {
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
   *   @history 2015-10-29 Ian Humphrey - Created slot. References #2324.
   */
  void QnetTool::previousRightMeasure() {
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
   */
  void QnetTool::selectLeftMeasure(int index) {
    QString file = m_pointFiles[index];

    QString serial = m_serialNumberList->serialNumber(file);

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
    m_leftCube.reset(new Cube(file, "r"));

    //  Update left measure of pointEditor
    m_pointEditor->setLeftMeasure (m_leftMeasure, m_leftCube.data(),
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
   */
  void QnetTool::selectRightMeasure(int index) {
    QString file = m_pointFiles[index];

    QString serial = m_serialNumberList->serialNumber(file);

    // Make sure to clear out rightMeasure before making a copy of the selected
    // measure.
    if (m_rightMeasure != NULL) {
      delete m_rightMeasure;
      m_rightMeasure = NULL;
    }
    m_rightMeasure = new ControlMeasure();
    //  Find measure for each file
    *m_rightMeasure = *((*m_editPoint)[serial]);

    //  If m_rightCube is not null, delete before creating new one
    m_rightCube.reset(new Cube(file, "r"));

    //  Update right measure of pointEditor
    m_pointEditor->setRightMeasure (m_rightMeasure, m_rightCube.data(),
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
  void QnetTool::updateLeftMeasureInfo () {

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

    if (m_leftMeasure->GetSampleResidual() == Null) {
      s = "Sample Residual: Null";
    }
    else {
      s = "Sample Residual: " + QString::number(m_leftMeasure->GetSampleResidual());
    }
    m_leftSampError->setText(s);
    if (m_leftMeasure->GetLineResidual() == Null) {
      s = "Line Residual: Null";
    }
    else {
      s = "Line Residual: " + QString::number(m_leftMeasure->GetLineResidual());
    }
    m_leftLineError->setText(s);

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

  void QnetTool::updateRightMeasureInfo () {

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

    if (m_rightMeasure->GetSampleResidual() == Null) {
      s = "Sample Residual: Null";
    }
    else {
      s = "Sample Residual: " + QString::number(m_rightMeasure->GetSampleResidual());
    }
    m_rightSampError->setText(s);
    if (m_rightMeasure->GetLineResidual() == Null) {
      s = "Line Residual: Null";
    }
    else {
      s = "Line Residual: " + QString::number(m_rightMeasure->GetLineResidual());
    }
    m_rightLineError->setText(s);

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
   * Add measure to point
   *
   * @internal
   * @history 2010-07-22  Tracie Sucharski - MeasureType of Estimated is now
   *                           Reference.  This change associated with
   *                           implementation of binary control networks.
   * @history 2011-04-06  Tracie Sucharski - If not a fixed point, use the
   *                           Reference measure to get lat,lon.
   * @history 2016-03-17  Makayla Shepherd - All new measures added use the
   *                           Reference Measure to get the lat, lon, in order
   *                           to make keep behavior consistant. Fixes #2326.
   *
   */
  void QnetTool::addMeasure() {

    //  Create list of list box of all files highlighting those that
    //  contain the point, but that do not already have a measure.
    QStringList pointFiles;

    //  Initialize camera for all images in control network,
    //  TODO::   Needs to be moved to QnetFileTool.cpp
    Camera *cam;

    // Use lat/lon of first measure
    double lat;
    double lon;

    ControlMeasure m = *(m_editPoint->GetRefMeasure());
    int camIndex = m_serialNumberList->serialNumberIndex(m.GetCubeSerialNumber());
    cam = m_controlNet->Camera(camIndex);
    //cam = m.Camera();
    cam->SetImage(m.GetSample(),m.GetLine());
    lat = cam->UniversalLatitude();
    lon = cam->UniversalLongitude();

    for (int i=0; i<m_serialNumberList->size(); i++) {
      cam = m_controlNet->Camera(i);
      if (m_serialNumberList->serialNumber(i) == m_groundSN) continue;
      if (cam->SetUniversalGround(lat,lon)) {
        //  Make sure point is within image boundary
        double samp = cam->Sample();
        double line = cam->Line();
        if (samp >= 1 && samp <= cam->Samples() &&
            line >= 1 && line <= cam->Lines()) {
          pointFiles<<m_serialNumberList->fileName(i);
        }
      }
    }

    QnetNewMeasureDialog *newMeasureDialog = new QnetNewMeasureDialog(this);
    newMeasureDialog->setFiles(*m_editPoint,pointFiles);
    if (newMeasureDialog->exec()) {
      QStringList selectedFiles = newMeasureDialog->selectedFiles();
      foreach (QString selectedFile, selectedFiles) {
        //  Create measure for any file selected
        ControlMeasure *m = new ControlMeasure;
        //  Find serial number for this file
        QString sn = m_serialNumberList->serialNumber(selectedFile);
        m->SetCubeSerialNumber(sn);
        int camIndex =
              m_serialNumberList->fileNameIndex(selectedFile);
        cam = m_controlNet->Camera(camIndex);
        cam->SetUniversalGround(lat,lon);
        m->SetCoordinate(cam->Sample(),cam->Line());
        m->SetAprioriSample(cam->Sample());
        m->SetAprioriLine(cam->Line());
        m->SetType(ControlMeasure::Manual);
        m->SetChooserName(Application::UserName());
        m_editPoint->Add(m);
      }
      loadPoint();
      m_qnetTool->setVisible(true);
      m_qnetTool->raise();

      loadTemplateFile(
          m_pointEditor->templateFileName());


      // emit signal so the nav tool can update edit point
      emit editPointChanged(m_editPoint->GetId());
      colorizeSaveButton();
    }
  }


  /**
   * Event filter for QnetTool.  Determines whether to update left or right
   * measure info.
   *
   * @param o Pointer to QObject
   * @param e Pointer to QEvent
   *
   * @return @b bool Indicates whether the event type is "Leave".
   *
   */
  bool QnetTool::eventFilter(QObject *o, QEvent *e) {
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
  void QnetTool::paintViewport(MdiCubeViewport *vp, QPainter *painter) {
    drawAllMeasurments (vp,painter);

  }


  /**
   * This method will repaint the given Point ID in each viewport
   * Note: The pointId parameter is here even though it's not used because
   * the signal (QnetTool::editPointChanged connected to this slot is also
   * connected to another slot (QnetNavTool::updateEditPoint which needs the
   * point Id.  TODO:  Clean this up, use 2 different signals?
   *
   * @param pointId
   *
   * @internal
   * @history 2010-06-03 Jeannie Walldren - Removed "std::" since "using
   *                          namespace std"
   */
  void QnetTool::paintAllViewports(QString pointId) {

    // Take care of drawing things on all viewPorts.
    // Calling update will cause the Tool class to call all registered tools
    // if point has been deleted, this will remove it from the main window
    MdiCubeViewport *vp;
    for (int i=0; i<(int)cubeViewportList()->size(); i++) {
      vp = (*(cubeViewportList()))[i];
       vp->viewport()->update();
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
   *                          measures not be drawn as yellow unless QnetTool was
   *                          open
   *   @history 2010-07-01 Jeannie Walldren - Modified to draw points selected in
   *                          QnetTool last so they lay on top of all other points
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
  void QnetTool::drawAllMeasurments(MdiCubeViewport *vp, QPainter *painter) {
    // Without a controlnetwork there are no points, or if new net, no points
    if (m_controlNet == 0 || m_controlNet->GetNumPoints() == 0) return;

    // Don't show the measurments on cubes not in the serial number list
    // TODO: Should we show them anyway
    // TODO: Should we add the SN to the viewPort
    QString serialNumber = SerialNumber::Compose(*vp->cube(), true);

    if (serialNumber == m_groundSN) {
      drawGroundMeasures(vp, painter);
      return;
    }
    if (!m_controlNet->GetCubeSerials().contains(
                      serialNumber)) return;
    if (!m_serialNumberList->hasSerialNumber(serialNumber)) return;
    QList<ControlMeasure *> measures =
        m_controlNet->GetMeasuresInCube(serialNumber);
    // loop through all measures contained in this cube
    for (int i = 0; i < measures.count(); i++) {
      ControlMeasure *m = measures[i];
      // Find the measurments on the viewport
      double samp = m->GetSample();
      double line = m->GetLine();
      int x, y;
      vp->cubeToViewport(samp, line, x, y);
      // if the point is ignored,
      if (m->Parent()->IsIgnored()) {
        painter->setPen(QColor(255, 255, 0)); // set point marker yellow
      }
      // point is not ignored, but measure matching this image is ignored,
      else if (m->IsIgnored()) {
        painter->setPen(QColor(255, 255, 0)); // set point marker yellow
      }
      // Neither point nor measure is not ignored and the measure is fixed,
      else if (m->Parent()->GetType() != ControlPoint::Free) {
        painter->setPen(Qt::magenta);// set point marker magenta
      }
      else {
        painter->setPen(Qt::green); // set all other point markers green
      }
      // draw points
      painter->drawLine(x - 5, y, x + 5, y);
      painter->drawLine(x, y - 5, x, y + 5);
    }
    // if QnetTool is open,
    if (m_editPoint != NULL) {
      // and the selected point is in the image,
      if (m_editPoint->HasSerialNumber(serialNumber)) {
        // find the measurement
        double samp = (*m_editPoint)[serialNumber]->GetSample();
        double line = (*m_editPoint)[serialNumber]->GetLine();
        int x, y;
        vp->cubeToViewport(samp, line, x, y);
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
   * Draw all Fixed or Constrained points on the ground source viewport
   * @param vp Viewport whose measurements will be drawn
   * @param painter
   *
   * @author 2011-09-16  Tracie Sucharski
   *
   * @internal
   */
  void QnetTool::drawGroundMeasures(MdiCubeViewport *vp, QPainter *painter) {

    // loop through control network looking for fixed and constrained points
    for (int i = 0; i < m_controlNet->GetNumPoints(); i++) {
      ControlPoint &p = *((*m_controlNet)[i]);
      if (p.GetType() == ControlPoint::Free) continue;
      if (!p.HasAprioriCoordinates()) continue;

      // Find the measure on the ground image
      if (m_groundGmap->SetGround(p.GetAprioriSurfacePoint().GetLatitude(),
                                  p.GetAprioriSurfacePoint().GetLongitude())) {
        double samp = m_groundGmap->Sample();
        double line = m_groundGmap->Line();
        int x, y;
        vp->cubeToViewport(samp, line, x, y);
        // if the point is ignored,
        if (p.IsIgnored()) {
          painter->setPen(QColor(255, 255, 0)); // set point marker yellow
        }
        else if (p.GetType() != ControlPoint::Free) {
          painter->setPen(Qt::magenta);// set point marker magenta
        }
        else if (&p == m_editPoint) {
          // set point marker red
          QBrush brush(Qt::red);
          // set point marker bold - line width 2
          QPen pen(brush, 2);
        }
        else {
          painter->setPen(Qt::green); // set all other point markers green
        }
        // draw points
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

  void QnetTool::setTemplateFile() {
    m_pointEditor->setTemplateFile();
  }*/


  bool QnetTool::okToContinue() {

    if (m_templateModified) {
      int r = QMessageBox::warning(m_qnetTool, tr("OK to continue?"),
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
  void QnetTool::openTemplateFile() {

    if (!okToContinue())
      return;

    QString filename = QFileDialog::getOpenFileName(m_qnetTool,
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
  void QnetTool::loadTemplateFile(QString fn) {

    QFile file(FileName(fn).expanded());
    if (!file.open(QIODevice::ReadOnly)) {
      QString msg = "Failed to open template file \"" + fn + "\"";
      QMessageBox::warning(m_qnetTool, "IO Error", msg);
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
  void QnetTool::setTemplateModified() {
    m_templateModified = true;
    m_saveTemplateFile->setEnabled(true);
  }


  //! save the file opened in the template editor
  void QnetTool::saveTemplateFile() {

    if (!m_templateModified)
      return;

    QString filename = m_pointEditor->templateFileName();

    writeTemplateFile(filename);
  }


  //! save the contents of template editor to a file chosen by the user
  void QnetTool::saveTemplateFileAs() {

    QString filename = QFileDialog::getSaveFileName(m_qnetTool,
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
  void QnetTool::writeTemplateFile(QString fn) {

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
      QMessageBox::warning(m_qnetTool, "Error", message);
      return;
    }

    QString expandedFileName(FileName(fn).expanded());

    QFile file(expandedFileName);

    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
      QString msg = "Failed to save template file to \"" + fn + "\"\nDo you "
          "have permission?";
      QMessageBox::warning(m_qnetTool, "IO Error", msg);
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
  void QnetTool::viewTemplateFile() {
    try{
      // Get the template file from the ControlPointEditor object
      Pvl templatePvl(m_pointEditor->templateFileName().toStdString());
      // Create registration dialog window using PvlEditDialog class
      // to view and/or edit the template
      PvlEditDialog registrationDialog(templatePvl);
      registrationDialog.setWindowTitle("View or Edit Template File: "
                                         + QString::fromStdString(templatePvl.fileName()));
      registrationDialog.resize(550,360);
      registrationDialog.exec();
    }
    catch (IException &e) {
      QString message = e.toString();
      QMessageBox::information(m_qnetTool, "Error", message);
    }
  }



  /**
   * Slot which calls ControlPointEditor slot to save chips
   *
   * @author 2009-03-17 Tracie Sucharski
   */

  void QnetTool::saveChips() {
    m_pointEditor->saveChips();
  }


  void QnetTool::showHideTemplateEditor() {

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
   *        of QnetTool, this method will need to be updated.
   *       *** THIS METHOD SHOULD GO AWAY WHEN CONTROLpOINTEDITOR IS INCLUDED
   *           IN QNET ***
   */
  void QnetTool::updatePointInfo(QString pointId) {
    if (m_editPoint == NULL) return;
    if (pointId != m_editPoint->GetId()) return;
    //  The edit point has been changed by SetApriori, so m_editPoint needs
    //  to possibly update some values.  Need to retain measures from m_editPoint
    //  because they might have been updated, but not yet saved to the network
    //   ("Save Point").
    ControlPoint *updatedPoint = m_controlNet->GetPoint(pointId);
    m_editPoint->SetEditLock(updatedPoint->IsEditLocked());
    m_editPoint->SetIgnored(updatedPoint->IsIgnored());
    m_editPoint->SetAprioriSurfacePoint(updatedPoint->GetAprioriSurfacePoint());

    //  Set EditLock box correctly
    m_lockPoint->setChecked(m_editPoint->IsEditLocked());

    //  Set ignore box correctly
    m_ignorePoint->setChecked(m_editPoint->IsIgnored());

    updateSurfacePointInfo();

  }




  /**
   * Refresh all necessary widgets in QnetTool including the PointEditor and
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
  void QnetTool::refresh() {


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
        emit editPointChanged("");
        m_qnetTool->setVisible(false);
        m_measureWindow->setVisible(false);
      }
    }

    if (m_editPoint == NULL) {
      paintAllViewports("");
    }
    else {
      paintAllViewports(m_editPoint->GetId());
    }
  }


  /**
   * Emits a signal to displays the Navigation window.  This signal is connected
   * to QnetNavTool.
   *
   * @internal
   *   @history 2010-07-01 Jeannie Walldren - Original version
   */
  void QnetTool::showNavWindow(bool checked){
    emit showNavTool();
  }

  /**
   * This method creates the widgets for the tool bar.  A "Show Nav Tool" button
   * is created so that the navigation tool may be reopened if it has been closed.
   *
   * @param parent The parent QStackedWidget
   * @return @b QWidget*
   *
   * @internal
   * @todo Find a way to enable Show Nav Button even when there are no images open
   *       in the main window.
   *
   * @history 2010-07-01 Jeannie Walldren - Original version.
   */
  QWidget *QnetTool::createToolBarWidget (QStackedWidget *parent) {
    QWidget *hbox = new QWidget(parent);

    QToolButton *showNavToolButton = new QToolButton();
    showNavToolButton->setText("Show Nav Tool");
    showNavToolButton->setToolTip("Shows the Navigation Tool Window");
    QString text =
    "<b>Function:</b> This button will bring up the Navigation Tool window that allows \
     the user to view, modify, ignore, delete, or filter points and cubes.";
    showNavToolButton->setWhatsThis(text);
    connect(showNavToolButton,SIGNAL(clicked(bool)),this,SLOT(showNavWindow(bool)));

    QHBoxLayout *layout = new QHBoxLayout(hbox);
    layout->setMargin(0);
    layout->addWidget(showNavToolButton);
    layout->addStretch(1);
    hbox->setLayout(layout);
    return hbox;
  }




  /**
   * Open a ground source for selecting fixed points.  This file could be
   * a DEM, a shaded version of a DEM or a level1 image with corrected pointing
   * or some other type of basemap.
   *
   * @author  2009-07-20 Tracie Sucharski
   *
   * @internal
   *   @history 2011-06-03 Tracie Sucharski - Make sure edit point valid before
   *                           loading.
   *   @history 2012-10-04 Tracie Sucharski - If the ground source serial number already exists in
   *                           the serial number list, print error and clear out ground
   *                           information.
   *   @history 2012-10-05 Tracie Sucharski - Re-factored most of this method, attempting to clean
   *                           up logic, error checking and recovery.
   *   @history 2016-09-26 Makayla Shepherd - Changed the logic for determining the radius source
   *                           when there is not a radius source already open.
   */
  void QnetTool::openGround() {

    QString filter = "Isis cubes (*.cub *.cub.*);;";
    filter += "Detached labels (*.lbl);;";
    filter += "All (*)";

    QString ground = QFileDialog::getOpenFileName((QWidget*)parent(),
                                                  "Open ground source",
                                                  ".",
                                                  filter);
    if (ground.isEmpty()) return;

    //  First off, find serial number of new ground, it is needed for a couple of error checks.
    QString newGroundSN = SerialNumber::Compose(ground, true);

    //  If new ground same file as old ground file simply set as active window.
    if (m_groundOpen && m_groundFile == FileName(ground).name()) {
      //  See if ground source is already opened in a cubeviewport.  If so, simply
      //  activate the viewport and return.
      MdiCubeViewport *vp;
      for (int i=0; i<(int)cubeViewportList()->size(); i++) {
        vp = (*(cubeViewportList()))[i];
        if (vp->cube()->fileName() == ground) {
          m_workspace->mdiArea()->setActiveSubWindow(
              (QMdiSubWindow *)vp->parentWidget()->parent());
          return;
        }
      }
    }

    //  Make sure there are not serial number conflicts.  If there are serial number conflicts,
    //  simply return, retaining current ground source.
    if (newGroundSN != m_groundSN && m_serialNumberList->hasSerialNumber(newGroundSN)) {
      //  TODO  If it already exists, are the files different?  Now what?
      //     For now, do not allow.
      QString message = "A cube in the cube list has the same serial number as this ground file.  ";
      message += "If this ground source is a level 1, un-projected cube, it is probably included ";
      message += "in the cube list.  If the ground source is a projected version of a cube in ";
      message += "the list and has the Instrument Group in the labels, the un-projected and ";
      message += "projected cube will have the same serial number. \n";
      message += "Because of duplicate serial numbers this cube cannot be used as a ground ";
      message += "source.\n\n";
      message += "NOTE:  If this cube is the reference cube you can select points in ";
      message += "the Navigator window, then select the Set Apriori button to use this cube to ";
      message += "set the apriori latitude, longitude and radius.";
      QMessageBox::critical(m_qnetTool, "Cannot set ground source", message);
      return;
    }

    //  So far, so good.  If previous ground, clear out ground source info.
    if (m_groundOpen) {
      //  ....otherwise if new ground source, close old.  We only want a single
      //  ground source opened at once.  Delete old ground source from left/right
      //  combo if it's there, and delete from serial number list.
      clearGroundSource ();
    }

    QApplication::setOverrideCursor(Qt::WaitCursor);

    //  Create new ground cube,  if failure, there will be not ground source, clear all ground
    //  source data.  (Cannot call clearGroundSource because it assumes a ground was successfully
    //  loaded)
    m_groundCube.reset(NULL);
    m_groundGmap.reset(NULL);

    try {
      QScopedPointer<Cube> newGroundCube(new Cube(ground, "r"));
      QScopedPointer<UniversalGroundMap> newGroundGmap(new UniversalGroundMap(*newGroundCube));

      m_groundFile = FileName(newGroundCube->fileName()).name();
      m_groundCube.reset(newGroundCube.take());
      m_groundGmap.reset(newGroundGmap.take());

      m_serialNumberList->add(ground, true);
    }
    catch (IException &e) {
      QApplication::restoreOverrideCursor();
      QMessageBox::critical(m_qnetTool, "Error", e.toString());

      m_groundFile.clear();

      // Re-load point w/o ground source
      if (m_editPoint) {
        loadPoint();
      }

      emit refreshNavList();
      return;
    }

    m_groundSN = newGroundSN;
    m_groundSourceFile = ground;
    m_groundOpen = true;

    m_workspace->addCubeViewport(m_groundCube.data());

    //  Get viewport so connect can be made when ground source viewport closed to clean up
    // ground source
    MdiCubeViewport *vp;
    for (int i=0; i<(int)cubeViewportList()->size(); i++) {
      vp = (*(cubeViewportList()))[i];
      if (vp->cube()->fileName() == ground) {
        connect(vp, SIGNAL(viewportClosed(CubeViewport *)),
                this, SLOT(groundViewportClosed(CubeViewport *)), Qt::UniqueConnection);
      }
    }

    if (!m_demOpen) {
      //  If there isn't a radius source already open and there is a point selected
      if (m_editPoint != NULL) {
        openReferenceRadius();
      }

      //  Determine file type of ground for setting AprioriSurfacePointSource
      //  and AprioriRadiusSource.
      else if (m_groundCube->hasTable("ShapeModelStatistics")) {
        m_groundSurfacePointSource = ControlPoint::SurfacePointSource::Basemap;
        if (!m_demOpen) {
          m_groundRadiusSource = ControlPoint::RadiusSource::DEM;
          m_radiusSourceFile = ground;
        }
      }
      // Is this a level 1 or level 2?
      else {
        try {
          ProjectionFactory::CreateFromCube(*m_groundCube);
          m_groundSurfacePointSource = ControlPoint::SurfacePointSource::Basemap;
          if (!m_demOpen) {
            m_groundRadiusSource = ControlPoint::RadiusSource::Ellipsoid;
            m_radiusSourceFile = "";
          }
        }
        catch (IException &) {
          try {
            CameraFactory::Create(*m_groundCube);
            m_groundSurfacePointSource = ControlPoint::SurfacePointSource::Reference;
            if (!m_demOpen) {
              PvlGroup kernels = m_groundCube->group("Kernels");
              QString shapeFile = QString::fromStdString(kernels["ShapeModel"]);
              if (shapeFile.contains("dem")) {
                m_groundRadiusSource = ControlPoint::RadiusSource::DEM;
                m_radiusSourceFile = shapeFile;
              }
              else {
                m_groundRadiusSource = ControlPoint::RadiusSource::Ellipsoid;
                //  Find pck file from Kernels group
                m_radiusSourceFile = QString::fromStdString(kernels["TargetAttitudeShape"]);
              }
            }
          }
          catch (IException &) {
            QString message = "Cannot create either Camera or Projections ";
            message += "for the ground source file.  Check the validity of the ";
            message += " cube labels.  The cube must either be projected or ";
            message += " run through spiceinit.";
            QMessageBox::critical(m_qnetTool, "Error", message);
            //  Clear out everything relating to ground source
            clearGroundSource ();
            QApplication::restoreOverrideCursor();
            emit refreshNavList();
            return;
          }
        }
      }
    }

    if (m_editPoint != NULL &&
        (m_editPoint->GetType() != ControlPoint::Free)) loadPoint();


    emit refreshNavList();
    QApplication::restoreOverrideCursor();
  }


  /**
   * Open a DEM for ground source radii
   *
   * @author  2011-01-18 Tracie Sucharski
   *
   *
   */
  void QnetTool::openDem() {

      if (m_groundFile.isEmpty()) {
        QString message = "You must enter a ground source before opening a Dem.";
        QMessageBox::critical(m_qnetTool, "Error", message);
        return;
      }

      QString filter = "Isis cubes (*.cub *.cub.*);;";
      filter += "Detached labels (*.lbl);;";
      filter += "All (*)";
      QString dem = QFileDialog::getOpenFileName((QWidget*)parent(),
                                                  "Open DEM",
                                                  ".",
                                                  filter);
      if (dem.isEmpty()) return;

      initDem(dem);

  }

  /**
   * Open a radius source using the shape model of the reference measure of m_editPoint
   *
   * @author 2016-10-07 Makayla Shepherd - Changed radius source handling and moved it from OpenGround.
   *
   *
   */
  void QnetTool::openReferenceRadius() {
    //Get the reference image's shape model
    QString referenceSN = m_editPoint->GetReferenceSN();
    QString referenceFileName = m_serialNumberList->fileName(referenceSN);
    QScopedPointer<Cube> referenceCube(new Cube(referenceFileName, "r"));
    PvlGroup kernels = referenceCube->group("Kernels");
    QString shapeFile = QString::fromStdString(kernels["ShapeModel"]);

    //  If the reference measure has a shape model cube then set that as the radius
    //  This will NOT WORK for shape model files (not the default of Null or Ellipsoid)
    //  that are not cubes
    if (shapeFile.contains(".cub")) {
      if (shapeFile.contains("dem")) {
        m_groundRadiusSource = ControlPoint::RadiusSource::DEM;
      }
      else {
        m_groundRadiusSource = ControlPoint::RadiusSource::Ellipsoid;
      }

      m_radiusSourceFile = shapeFile;

      //  Open shape file for reading radius later
      initDem(shapeFile);  //This will write the labels for us
    }

    //  If no shape model then use the ABC of the target body
    else {
      m_groundRadiusSource = ControlPoint::RadiusSource::Ellipsoid;
      Spice *refSpice = new Spice(*referenceCube);
      Distance refRadii[3];
      refSpice->radii(refRadii);
      m_demFile = QString::number(refRadii[0].meters()) + ", " +
                  QString::number(refRadii[1].meters()) + ", " +
                  QString::number(refRadii[2].meters());

      m_radiusSourceFile = "";

      //  Write out the labels
      m_groundFileNameLabel->setText("Ground Source File:  " + m_groundFile);
      m_radiusFileNameLabel->setText("Radius Source:  " + m_demFile);
    }
  }

  void QnetTool::initDem (QString demFile) {

      //  If a DEM is already opened, check if new is same as old. If new,
      //  close old, open new.
      QApplication::setOverrideCursor(Qt::WaitCursor);
      if (m_demOpen) {
        if (m_demFile == demFile) {
          QApplication::restoreOverrideCursor();
          return;
        }

        m_demCube.reset(NULL);
        m_demFile.clear();
      }

      try {
        QScopedPointer<Cube> newDemCube(new Cube(demFile, "r"));

        m_demFile = FileName(newDemCube->fileName()).name();
        m_demCube.reset(newDemCube.take());
      }
      catch (IException &e) {
        QMessageBox::critical(m_qnetTool, "Error", e.toString());
        QApplication::restoreOverrideCursor();
        return;
      }
      m_demOpen = true;

      //  Make sure this is a dem
      if (!m_demCube->hasTable("ShapeModelStatistics")) {
        QString message = m_demFile + " is not a DEM.";
        QMessageBox::critical(m_qnetTool, "Error", message);
        m_demCube.reset(NULL);
        m_demOpen = false;
        m_demFile.clear();
        QApplication::restoreOverrideCursor();
        return;
      }
      m_groundRadiusSource = ControlPoint::RadiusSource::DEM;
      m_groundFileNameLabel->setText("Ground Source File:  " + m_groundFile);
      m_radiusFileNameLabel->setText("Radius Source File:  " + m_demFile);
      m_radiusSourceFile = demFile;

      QApplication::restoreOverrideCursor();
  }



  /**
   * Slot called when the ground source cube viewport is closed
   *
   * @author  2013-05-15 Tracie Sucharski
   *
   */
  void QnetTool::groundViewportClosed(CubeViewport *) {

    //  Only continue to clearGroundSource if the viewport is not already closed
    //  Otherwise, it could be called twice
    clearGroundSource();
  }



  void QnetTool::clearGroundSource () {

    m_leftCombo->removeItem(m_leftCombo->findText(m_groundFile));
    m_rightCombo->removeItem(m_rightCombo->findText(m_groundFile));

    //  Close viewport containing ground source
    MdiCubeViewport *vp;
    for (int i=0; i<(int)cubeViewportList()->size(); i++) {
      vp = (*(cubeViewportList()))[i];
      if (vp->cube() == m_groundCube.data()) {
        //  disconnect signal to avoid recursive situartion.  When a viewport is closed, a signal
        //  is emitted which would then call groundViewportClosed, then this method again.
        disconnect(vp, SIGNAL(viewportClosed(CubeViewport *)),
                   this, SLOT(groundViewportClosed(CubeViewport *)));
        vp->parentWidget()->parentWidget()->close();
        QApplication::processEvents();
        break;
      }
    }
    //  If we could not find the ground source in the open viewports, user might
    //  have closed the viewport , reset ground source variables and re-open.
    m_groundOpen = false;
    m_groundCube.take();
    m_groundFile.clear();
    m_groundGmap.reset(NULL);

    m_groundFileNameLabel->setText("Ground Source File:  ");
    if (!m_demOpen) {
      m_radiusFileNameLabel->setText("Radius Source File:  " + m_demFile);
    }

    // Remove from serial number list
    m_serialNumberList->remove(m_groundSN);

    //  If the loaded point is a fixed point, see if there is a temporary measure
    //  holding the coordinate information for the currentground source. If so,
    //  delete this measure and re-load point
    if (m_editPoint && m_editPoint->GetType() != ControlPoint::Free &&
        m_editPoint->HasSerialNumber(m_groundSN)) {
      m_editPoint->Delete(m_groundSN);
      m_groundSN = "";
      loadPoint();
    }
    else {
      m_groundSN = "";
    }
  }





  /**
   * Return a radius values from the dem using bilinear interpolation
   *
   * @author  2011-08-01 Tracie Sucharski
   *
   * @internal
   */
  double QnetTool::demRadius(double latitude, double longitude) {

    if (!m_demOpen) return Null;

    UniversalGroundMap *demMap = new UniversalGroundMap(*m_demCube);
    if (!demMap->SetUniversalGround(latitude, longitude)) {
      delete demMap;
      demMap = NULL;
      return Null;
    }

    //  Use bilinear interpolation to read radius from DEM
    //   Use bilinear interpolation from dem
    Interpolator *interp = new Interpolator(Interpolator::BiLinearType);

    //   Buffer used to read from the model
    Portal *portal = new Portal(interp->Samples(), interp->Lines(),
                                m_demCube->pixelType(),
                                interp->HotSample(), interp->HotLine());
    portal->SetPosition(demMap->Sample(), demMap->Line(), 1);
    m_demCube->read(*portal);
    double radius = interp->Interpolate(demMap->Sample(), demMap->Line(),
                                        portal->DoubleBuffer());
    delete demMap;
    demMap = NULL;
    delete interp;
    interp = NULL;
    delete portal;
    portal = NULL;

//  cout.width(15);
//  cout.precision(4);
//  cout<<"DEM Radius = "<<fixed<<radius<<endl;
    return radius;
  }



  /**
   * Turn "Save Point" button text to red
   *
   * @author 2011-06-14 Tracie Sucharski
   */
  void QnetTool::colorizeSaveButton() {

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
  bool QnetTool::IsMeasureLocked (QString serialNumber) {

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
  void QnetTool::readSettings() {
    FileName config("$HOME/.Isis/qnet/QnetTool.config");
    QSettings settings(config.expanded(), QSettings::NativeFormat);
    QPoint pos = settings.value("pos", QPoint(300, 100)).toPoint();
    QSize size = settings.value("size", QSize(900, 500)).toSize();
    m_qnetTool->resize(size);
    m_qnetTool->move(pos);
  }


  /**
   * This method is called when the Main window is closed or
   * hidden to write the size and location settings to a config
   * file in the user's home directory.
   *
   */
  void QnetTool::writeSettings() const {
    /*We do not want to write the settings unless the window is
      visible at the time of closing the application*/
    if(!m_qnetTool->isVisible()) return;
    FileName config("$HOME/.Isis/qnet/QnetTool.config");
    QSettings settings(config.expanded(), QSettings::NativeFormat);
    settings.setValue("pos", m_qnetTool->pos());
    settings.setValue("size", m_qnetTool->size());
  }



  void QnetTool::enterWhatsThisMode() {
    QWhatsThis::enterWhatsThisMode();
  }


}
