/** This is free and unencumbered software released into the public domain.

The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */

#include "ControlPointEditWidget.h"

#include <iomanip>
#include <sstream>
#include <vector>

#include <QAction>
#include <QCheckBox>
#include <QComboBox>
#include <QFileDialog>
#include <QFileInfo>
#include <QFormLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QMainWindow>
#include <QMessageBox>
#include <QObject>
#include <QPushButton>
#include <QScrollBar>
#include <QShortcut>
#include <QSplitter>
#include <QTableWidget>
#include <QTextEdit>
#include <QVBoxLayout>

#include "Application.h"
#include "Camera.h"
#include "Control.h"
#include "ControlMeasureEditWidget.h"
#include "ControlMeasure.h"
#include "ControlMeasureLogData.h"
#include "ControlNet.h"
#include "ControlPoint.h"
#include "DeleteControlPointDialog.h"
#include "Directory.h"
#include "FileName.h"
#include "IException.h"
#include "Latitude.h"
#include "Longitude.h"
#include "MainWindow.h"
#include "MdiCubeViewport.h"
#include "NewControlPointDialog.h"
#include "NewGroundSourceLocationDialog.h"
#include "Project.h"
#include "Pvl.h"
#include "PvlEditDialog.h"
#include "SerialNumber.h"
#include "SerialNumberList.h"
#include "Shape.h"
#include "ShapeList.h"
#include "SpecialPixel.h"
#include "Table.h"
#include "TemplateList.h"
#include "ToolPad.h"
#include "UniversalGroundMap.h"
#include "ViewportMainWindow.h"
#include "Workspace.h"

using namespace std;

namespace Isis {
  /**
   * Consructs the ControlPointEditWidget widget
   *
   * @param parent Pointer to the parent widget for the ControlPointEditWidget
   * @param addMeasures Whether or not to add the Add Measure to Point button
   *
   */
  ControlPointEditWidget::ControlPointEditWidget (Directory *directory, QWidget *parent,
                                                  bool addMeasures) : QWidget(parent) {

    m_directory = directory;
    m_addMeasuresButton = addMeasures;
    m_cnetModified = false;
    m_templateModified = false;
    m_serialNumberList = NULL;
    m_editPoint = NULL;

    m_changeAllGroundLocation = false;
    m_changeGroundLocationInNet = false;
    m_demOpen = false;

    m_parent = parent;

    createPointEditor(parent, m_addMeasuresButton);

    connect(this, SIGNAL(newControlNetwork(ControlNet *)),
            m_measureEditor, SIGNAL(newControlNetwork(ControlNet *)));

    connect(m_directory->project(), SIGNAL(templatesAdded(TemplateList *)),
            this, SLOT(addTemplates(TemplateList *)));
  }


  ControlPointEditWidget::~ControlPointEditWidget () {

  }


  QString ControlPointEditWidget::editPointId() {

    QString result = "";
    if (m_editPoint) {
      result = m_editPoint->GetId();
    }
    return result;
  }


  ControlPoint *ControlPointEditWidget::editPoint() {

    ControlPoint *result = NULL;
    if (m_editPoint) {
      result = m_editPoint;
    }
    return result;
  }


  /**
   * Create the widget for editing control points
   *
   * @param parent Pointer to parent QWidget
   * @param addMeasures Whether or not to add the Add Measure to Point button
   *
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
   */
  void ControlPointEditWidget::createPointEditor(QWidget *parent, bool addMeasures) {

    setWindowTitle("Control Point Editor");
    setObjectName("ControlPointEditWidget");
    connect(this, SIGNAL(destroyed(QObject *)), this, SLOT(clearEditPoint()));

    createActions();

    // create m_measureEditor first since we need to get its templateFileName
    // later
    m_measureEditor = new ControlMeasureEditWidget(parent, true, true);

    //  TODO Does this need to be moved to ControlNetEditMainWindow???
    connect(this, SIGNAL(newControlNetwork(ControlNet *)),
        m_measureEditor, SIGNAL(newControlNetwork(ControlNet *)));


    connect(this, SIGNAL(stretchChipViewport(Stretch *, CubeViewport *)),
            m_measureEditor, SIGNAL(stretchChipViewport(Stretch *, CubeViewport *)));
    connect(m_measureEditor, SIGNAL(measureSaved()), this, SLOT(measureSaved()));
    connect(this, SIGNAL(cnetModified()), this, SLOT(colorizeSaveNetButton()));

    QPushButton *addMeasure = NULL;
    if (m_addMeasuresButton) {
      addMeasure = new QPushButton("Add Measure(s) to Point");
      addMeasure->setToolTip("Add a new measure to the edit control point.");
      addMeasure->setWhatsThis("This allows a new control measure to be added "
         "to the currently edited control point.  A selection "
         "box with all cubes from the input list will be "
         "displayed with those that intersect with the "
         "control point highlighted.");
      //TODO addMeasure() slot is not implemented ???
      connect(addMeasure, SIGNAL(clicked()), this, SLOT(addMeasure()));
    }

    m_reloadPoint = new QPushButton("Reload Point");
    m_reloadPoint->setToolTip("Reload the control point.");
    m_reloadPoint->setWhatsThis("Reload the measures for the control point"
                            " in the Chip Viewports to its saved values. ");
    connect(m_reloadPoint, SIGNAL(clicked()), this, SLOT(reloadPoint()));

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
    connect (m_savePoint, SIGNAL(clicked()), this, SLOT(savePoint()));

    m_saveNet = new QPushButton ("Save Control Net");
    m_saveNet->setShortcut(Qt::Key_S);
    m_saveNet->setToolTip("Save current control network. "
                          "<strong>Shortcut: S</strong>");
    m_savePoint->setWhatsThis("Save the control network.");
//    m_saveDefaultPalette = m_savePoint->palette();
//  This slot is needed because we cannot directly emit a signal with a ControlNet
//  argument after the "Save Net" push button is selected since the parameter list must match.
//  The saveNet slot will simply emit a signal with the ControlNet as the argument.
    connect (m_saveNet, SIGNAL(clicked()), this, SLOT(saveNet()));

    QHBoxLayout * saveMeasureLayout = new QHBoxLayout;
    if (m_addMeasuresButton) {
      saveMeasureLayout->addWidget(addMeasure);
    }

    saveMeasureLayout->addWidget(m_reloadPoint);
    saveMeasureLayout->addWidget(m_savePoint);
    saveMeasureLayout->addWidget(m_saveNet);
    saveMeasureLayout->insertStretch(-1);

    m_cnetFileNameLabel = new QLabel("Control Network: " + m_cnetFileName);

    // Create a combobox to allow user to select either the default registration file or one of the
    // imported registration files.
    m_templateComboBox = new QComboBox();
    m_templateComboBox->setToolTip("Choose a template file");
    m_templateComboBox->setWhatsThis("FileName of the sub-pixel "
                  "registration template.  Refer to https://astrogeology.usgs.gov/docs/concepts/control-networks/pattern-matching/ for a description of the "
                  "contents of this file.");
    m_templateComboBox->addItem(m_measureEditor->templateFileName());
    QList <TemplateList *> regTemplates = m_directory->project()->regTemplates();
    foreach(TemplateList *templateList, regTemplates) {
      foreach(Template *templateFile, *templateList){
        m_templateComboBox->addItem(templateFile->importName()
                                    + "/" + FileName(templateFile->fileName()).name());
      }
    }
    QFormLayout *templateFileLayout = new QFormLayout();
    templateFileLayout->addRow("Template File:", m_templateComboBox);

    // Set-up connections to give registration combobox functionality
    connect(m_templateComboBox, SIGNAL(activated(QString)),
            this, SLOT(setTemplateFile(QString)));
    connect(m_measureEditor, SIGNAL(setTemplateFailed(QString)),
            this, SLOT(resetTemplateComboBox(QString)));

    QVBoxLayout * centralLayout = new QVBoxLayout;

    centralLayout->addWidget(m_cnetFileNameLabel);
    centralLayout->addLayout(templateFileLayout);
    centralLayout->addWidget(createTopSplitter());
    centralLayout->addStretch();
    centralLayout->addWidget(m_measureEditor);
    centralLayout->addLayout(saveMeasureLayout);

    QWidget *centralWidget = new QWidget;
    centralWidget->setLayout(centralLayout);

    QScrollArea *scrollArea = new QScrollArea();
    scrollArea->setObjectName("ControlPointEditWidgetScroll");
    scrollArea->setWidget(centralWidget);
    scrollArea->setWidgetResizable(true);
    centralWidget->adjustSize();

    QHBoxLayout *mainLayout = new QHBoxLayout;
    mainLayout->addWidget(scrollArea);
    setLayout(mainLayout);

//  connect(this, SIGNAL(controlPointChanged()), this, SLOT(paintAllViewports()));

//  readSettings();
  }


  /**
   * Creates everything above the ControlPointEdit
   *
   * @return @b QSplitter * The splitter containing the widgets above
   */
  QSplitter * ControlPointEditWidget::createTopSplitter() {

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
//  topSplitter->setStretchFactor(0, 4);
//  topSplitter->setStretchFactor(1, 3);

    m_templateEditorWidget->hide();

    return topSplitter;
  }


  /**
   * Creates the "Control Point" groupbox
   *
   * @return @b QGroupBox * The groupbox labeled "Control Point"
   */
  QGroupBox * ControlPointEditWidget::createControlPointGroupBox() {

    // create left vertical layout
    m_ptIdValue = new QLabel;

    m_numMeasures = new QLabel;

    m_aprioriLatitude = new QLabel;
    m_aprioriLongitude = new QLabel;
    m_aprioriRadius = new QLabel;

    // create right vertical layout's top layout
    m_lockPoint = new QCheckBox("Edit Lock Point");
    connect(m_lockPoint, SIGNAL(clicked(bool)), this, SLOT(setLockPoint(bool)));
    m_ignorePoint = new QCheckBox("Ignore Point");
    connect(m_ignorePoint, SIGNAL(clicked(bool)),
      this, SLOT(setIgnorePoint(bool)));
    connect(this, SIGNAL(ignorePointChanged()), m_ignorePoint, SLOT(toggle()));

    m_pointTypeCombo = new QComboBox;
    for (int i=0; i<ControlPoint::PointTypeCount; i++) {
      m_pointTypeCombo->insertItem(i, ControlPoint::PointTypeToString(
            (ControlPoint::PointType) i));
    }
    QFormLayout *pointTypeLayout = new QFormLayout;
    pointTypeLayout->addRow("PointType:", m_pointTypeCombo);
    connect(m_pointTypeCombo, SIGNAL(activated(int)),
            this, SLOT(setPointType(int)));

    m_groundSourceCombo = new QComboBox();
    m_radiusSourceCombo = new QComboBox();
    connect(m_groundSourceCombo, SIGNAL(currentIndexChanged(int)),
            this, SLOT(groundSourceFileSelectionChanged(int)));
    QFormLayout *groundSourceLayout = new QFormLayout;
    groundSourceLayout->addRow("Ground Source:", m_groundSourceCombo);
    QFormLayout *radiusSourceLayout = new QFormLayout;
    radiusSourceLayout->addRow("Radius Source:", m_radiusSourceCombo);

    QVBoxLayout * mainLayout = new QVBoxLayout;
    mainLayout->addWidget(m_ptIdValue);
    mainLayout->addWidget(m_numMeasures);
    mainLayout->addLayout(groundSourceLayout);
    mainLayout->addLayout(radiusSourceLayout);
    mainLayout->addWidget(m_aprioriLatitude);
    mainLayout->addWidget(m_aprioriLongitude);
    mainLayout->addWidget(m_aprioriRadius);
    mainLayout->addWidget(m_lockPoint);
    mainLayout->addWidget(m_ignorePoint);
    mainLayout->addLayout(pointTypeLayout);

    // create the groupbox
    QGroupBox * groupBox = new QGroupBox("Control Point");
    groupBox->setLayout(mainLayout);

    return groupBox;
  }


  /**
   * Creates the "Left Measure" groupbox
   *
   * @return @b QGroupBox * The groupbox labeled "Left Measure"
   */
  QGroupBox * ControlPointEditWidget::createLeftMeasureGroupBox() {

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
    QVBoxLayout * leftLayout = new QVBoxLayout;
    leftLayout->addWidget(m_leftCombo);
    leftLayout->addWidget(m_lockLeftMeasure);
    leftLayout->addWidget(m_ignoreLeftMeasure);
    leftLayout->addWidget(m_leftReference);
    leftLayout->addWidget(m_leftMeasureType);

    QGroupBox * leftGroupBox = new QGroupBox("Left Measure");
    leftGroupBox->setLayout(leftLayout);

    return leftGroupBox;
  }


  /**
   * Create the "Right Measure" groupbox
   *
   * @returns The groupbox labeled "Right Measure"
   *
   * @internal
   *   @history 2015-10-29 Ian Humphrey - Added shortcuts (PageUp/PageDown) for selecting previous
   *                           or next measure in right measures box. References #2324.
   */
  QGroupBox * ControlPointEditWidget::createRightMeasureGroupBox() {

    // create widgets for the right groupbox
    m_rightCombo = new QComboBox;
    m_model = new QStandardItemModel();
    m_rightCombo->setModel(m_model);
    m_rightCombo->view()->installEventFilter(this);
    m_rightCombo->setToolTip("Choose right control measure. "
                             "<strong>Shortcuts: PageUp/PageDown</strong>");
    m_rightCombo->setWhatsThis("Choose right control measure identified by "
                               "cube filename. "
                               "Note: PageUp selects previous measure; "
                               "PageDown selects next meausure.");

    m_rightCombo->view()->setSelectionMode(QAbstractItemView::SingleSelection);
    m_rightCombo->view()->setDragEnabled(true);
    m_rightCombo->view()->setAcceptDrops(true);
    m_rightCombo->view()->setDropIndicatorShown(true);
    m_rightCombo->view()->setDragDropMode(QAbstractItemView::InternalMove);

    // Attach shortcuts to this widget for selecting right measures
    // Note: Qt handles this memory for us since ControlPointEditWidget is the parent of these shortcuts
    QShortcut *nextMeasure = new QShortcut(Qt::Key_PageDown, this);
    connect(nextMeasure, SIGNAL(activated()), this, SLOT(nextRightMeasure()));
    QShortcut *prevMeasure = new QShortcut(Qt::Key_PageUp, this);
    connect(prevMeasure, SIGNAL(activated()), this, SLOT(previousRightMeasure()));

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

    // create right groupbox
    QVBoxLayout * rightLayout = new QVBoxLayout;
    rightLayout->addWidget(m_rightCombo);
    rightLayout->addWidget(m_lockRightMeasure);
    rightLayout->addWidget(m_ignoreRightMeasure);
    rightLayout->addWidget(m_rightReference);
    rightLayout->addWidget(m_rightMeasureType);

    QGroupBox * rightGroupBox = new QGroupBox("Right Measure");
    rightGroupBox->setLayout(rightLayout);

    return rightGroupBox;
  }


  /**
   * Creates the Widget which contains the template editor and its toolbar
   */
  void ControlPointEditWidget::createTemplateEditorWidget() {

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
   * Creates the actions for the widget
   */
  void ControlPointEditWidget::createActions() {

    m_closePointEditor = new QAction(QIcon(FileName("base/icons/fileclose.png").expanded()),
                                   "&Close", this);
    m_closePointEditor->setToolTip("Close this window");
    m_closePointEditor->setStatusTip("Close this window");
    m_closePointEditor->setShortcut(Qt::ALT + Qt::Key_F4);
    QString whatsThis = "<b>Function:</b> Closes the Match Tool window for this point "
        "<p><b>Shortcut:</b> Alt+F4 </p>";
    m_closePointEditor->setWhatsThis(whatsThis);
    connect(m_closePointEditor, SIGNAL(triggered()), this, SLOT(close()));

    m_showHideTemplateEditor = new QAction(QIcon(FileName("base/icons/view_text.png").expanded()),
                                           "&View/edit registration template", this);
    m_showHideTemplateEditor->setCheckable(true);
    m_showHideTemplateEditor->setToolTip("View and/or edit the registration template");
    m_showHideTemplateEditor->setStatusTip("View and/or edit the registration template");
    whatsThis = "<b>Function:</b> Displays the curent registration template.  "
       "The user may edit and save changes under a chosen filename.";
    m_showHideTemplateEditor->setWhatsThis(whatsThis);
    connect(m_showHideTemplateEditor, SIGNAL(triggered()), this,
        SLOT(showHideTemplateEditor()));

    m_saveChips = new QAction(QIcon(FileName("base/icons/window_new.png").expanded()),
                              "Save registration chips", this);
    m_saveChips->setToolTip("Save registration chips");
    m_saveChips->setStatusTip("Save registration chips");
    whatsThis = "<b>Function:</b> Save registration chips to file.  "
       "Each chip: pattern, search, fit will be saved to a separate file.";
    m_saveChips->setWhatsThis(whatsThis);
    connect(m_saveChips, SIGNAL(triggered()), this, SLOT(saveChips()));

    m_openTemplateFile = new QAction(QIcon(FileName("base/icons/fileopen.png").expanded()),
                                     "&Open registration template", this);
    m_openTemplateFile->setToolTip("Set registration template");
    m_openTemplateFile->setStatusTip("Set registration template");
    whatsThis = "<b>Function:</b> Allows user to select a new file to set as "
        "the registration template";
    m_openTemplateFile->setWhatsThis(whatsThis);
    connect(m_openTemplateFile, SIGNAL(triggered()), this, SLOT(openTemplateFile()));

    m_saveTemplateFile = new QAction(QIcon(FileName("base/icons/mActionFileSave.png").expanded()),
                                     "&Save template file", this);
    m_saveTemplateFile->setToolTip("Save the template file");
    m_saveTemplateFile->setStatusTip("Save the template file");
    m_saveTemplateFile->setWhatsThis("Save the registration template file");
    connect(m_saveTemplateFile, SIGNAL(triggered()), this,
        SLOT(saveTemplateFile()));

    m_saveTemplateFileAs = new QAction(QIcon(FileName("base/icons/mActionFileSaveAs.png").expanded()),
                                       "&Save template as...", this);
    m_saveTemplateFileAs->setToolTip("Save the template file as");
    m_saveTemplateFileAs->setStatusTip("Save the template file as");
    m_saveTemplateFileAs->setWhatsThis("Save the registration template file as");
    connect(m_saveTemplateFileAs, SIGNAL(triggered()), this,
        SLOT(saveTemplateFileAs()));
  }


  /**
   * Fill m_projectShapeNames with ALL shapes currently in project. The first
   * m_numberProjectShapesWithPoint actually contain the location of m_editPoint.
   *
   * @param latitude (double) Latitude for determining point location.  Defaults to Null which
   *                          results in the m_editPoint location being used, AprioriCoordinates
   *                          if they exist, otherwise the reference measure's location will be
   *                          used.
   * @param longitude (double) Longitude for determining point location.  Defaults to Null which
   *                          results in the m_editPoint location being used, AprioriCoordinates
   *                          if they exist, otherwise the reference measure's location will be
   *                          used.
   *
   */
  void ControlPointEditWidget::setShapesForPoint(double latitude, double longitude) {

    if (latitude == Null || longitude == Null) {
      //  Use current editPoint to get latitude, longitude.
      // Use apriori surface point to find location on ground source.  If
      // apriori surface point does not exist use reference measure
      if (m_editPoint->HasAprioriCoordinates()) {
        SurfacePoint sPt = m_editPoint->GetAprioriSurfacePoint();
        latitude = sPt.GetLatitude().degrees();
        longitude = sPt.GetLongitude().degrees();
      }
      else {
        ControlMeasure m = *(m_editPoint->GetRefMeasure());
        int camIndex = m_serialNumberList->serialNumberIndex(m.GetCubeSerialNumber());
        Camera *cam;
        cam = m_controlNet->Camera(camIndex);
        cam->SetImage(m.GetSample(),m.GetLine());
        latitude = cam->UniversalLatitude();
        longitude = cam->UniversalLongitude();
      }
    }

    m_numberProjectShapesWithPoint = 0;
    m_projectShapeNames.clear();
    m_nameToShapeMap.clear();

    // Get all shapes from project, putting shapes that contain the current m_editPoint at the
    // top of the list.
    QStringList shapeNamesNoPoint;
    // Create map between the Shape file name & Shape
    QList<ShapeList *> shapeLists = m_directory->project()->shapes();
    foreach (ShapeList *shapeList, shapeLists) {
      foreach (Shape *shape, *shapeList) {
        UniversalGroundMap *gmap = new UniversalGroundMap(*(shape->cube()));
        if (gmap->SetUniversalGround(latitude, longitude)) {
          m_projectShapeNames<<shape->fileName();
        }
        else {
          shapeNamesNoPoint<<shape->fileName();
        }
        m_nameToShapeMap[shape->fileName()] = shape;
        delete gmap;
      }
    }
    m_numberProjectShapesWithPoint = m_projectShapeNames.count();
    // Add shapes that do not contain point
    if (shapeNamesNoPoint.count() > 0) {
      m_projectShapeNames<<shapeNamesNoPoint;
    }
  }


  /**
   * Set both chip viewports to their original measures for the control point
   *
   * @internal
   *   @history 2017-07-31 Christopher Combs - Original version
   */
  void ControlPointEditWidget::reloadPoint() {
    m_measureEditor->setLeftMeasure(m_leftMeasure, m_leftCube.data(), m_editPoint->GetId());
    m_measureEditor->setRightMeasure(m_rightMeasure, m_rightCube.data(), m_editPoint->GetId());
  }


  /**
   * Set the serial number list
   *
   * @param snList Pointer to the SerialNumberList
   */
  void ControlPointEditWidget::setSerialNumberList(SerialNumberList *snList) {

    // TODO   If network & snList already exists do some error checking
    // Make copy;  we add ground source files to the list, and we don't want to cause problems for
    //  other ipce entities that are using
//  if (m_serialNumberList) {
//    delete m_serialNumberList;
//    m_serialNumberList = NULL;
//  }
//  m_serialNumberList = new SerialNumberList(snList);
    m_serialNumberList = snList;
  }


  /**
   * New control network being edited
   *
   * @param cnet (Control *) The control network to edit
   *
   * @internal
  */
  void ControlPointEditWidget::setControl(Control *control) {
    //  TODO  more error checking
    m_control = control;
    m_controlNet = control->controlNet();
    m_cnetFileName = control->fileName();

    QStringList cnetDirs = m_cnetFileName.split('/');
    QString strippedCnetFilename = cnetDirs.value(cnetDirs.length() -1);
    m_cnetFileNameLabel->setText("Control Network: " + strippedCnetFilename);
    m_cnetFileNameLabel->setToolTip(m_cnetFileName);
    m_cnetFileNameLabel->setWhatsThis(m_cnetFileName);
    setWindowTitle("Control Point Editor- Control Network File: " + m_cnetFileName);

    emit newControlNetwork(m_controlNet);
  }


  /**
   * New active control was set from ipce
   *
   * TODO:  This will need to be redesigned with the ::setControl method to better handle editing
   * points from different cnets.
   */
  void ControlPointEditWidget::setControlFromActive() {

    if (m_directory->project()->activeControl()) {
      m_control = m_directory->project()->activeControl();
      m_controlNet = m_control->controlNet();
      m_cnetFileName = m_control->fileName();

      m_cnetFileNameLabel->setText("Control Network: " + m_cnetFileName);
      setWindowTitle("Control Point Editor- Control Network File: " + m_cnetFileName);

      emit newControlNetwork(m_controlNet);
    }
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
  void ControlPointEditWidget::loadGroundMeasure() {

    ControlMeasure *groundMeasure = createTemporaryGroundMeasure();
    if (groundMeasure) {
      m_editPoint->Add(groundMeasure);

      // Add to measure combo boxes
      QString groundFile = m_serialNumberList->fileName(groundMeasure->GetCubeSerialNumber());
      QString tempFileName = FileName(groundFile).name();

      m_pointFiles<<groundFile;
      m_leftCombo->addItem(tempFileName);
      m_rightCombo->addItem(tempFileName);
      int rightIndex = m_rightCombo->findText(tempFileName);
      m_rightCombo->setCurrentIndex(rightIndex);
      selectRightMeasure(rightIndex);
      updateSurfacePointInfo();
    }
  }


  /**
   * @brief Create a temporary measure to hold the ground point info for ground source
   *
   * @return ControlMeasure* the created ground measure
   *
   * @internal
   *   @history 2015-05-19 Ian Humphrey and Makayla Shepherd - Original version adapted from
   *                           loadPoint() to encapsulate duplicated code in loadGroundMeasure().
   *   @history 2016-11-04 Tracie Sucharski - Combined findPointLocation and
   *                           createTemporaryGroundMeasure so that ground information doesn't need
   *                           to be re-created
   */
  ControlMeasure *ControlPointEditWidget::createTemporaryGroundMeasure() {

    ControlMeasure *groundMeasure = NULL;

    //  Try to set ground source file information.  If unsuccessful, return null ground measure
    if (!setGroundSourceInfo()) {
      return groundMeasure;
    }

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
    if (!m_groundGmap->SetUniversalGround(lat,lon)) {
      QString message = "This point does not exist on the ground source.\n";
      message += "Latitude = " + QString::number(lat);
      message += "  Longitude = " + QString::number(lon);
      message += "\n A ground measure will not be created.";
      QMessageBox::warning(this, "Warning", message);
    }
    else {
      groundMeasure = new ControlMeasure;
      groundMeasure->SetCubeSerialNumber(m_groundSN);
      groundMeasure->SetType(ControlMeasure::Candidate);
      groundMeasure->SetCoordinate(m_groundGmap->Sample(), m_groundGmap->Line());
      groundMeasure->SetChooserName("GroundMeasureTemporary");
    }

    return groundMeasure;
  }


  /**
   *  Find the ground source location: First look at current edit point for parameter,
   *  AprioriXYZSourceFile. If not there, see if user has selected a groundSource file from the
   *  groundSourceCombo.  If file does not exist, give option to look in another location and change
   *  location in the ControlNet for either this point and/or all points in net.
   *
   * @return bool success Returns the success status of setting ground source info
   *
   */
  bool ControlPointEditWidget::setGroundSourceInfo() {

    FileName groundFile;
    ControlPoint::SurfacePointSource::Source groundSourceType =
      ControlPoint::SurfacePointSource::None;

    bool success = false;

    //  No ground source chosen, clear out any old info
    if (m_groundSourceCombo->currentText().contains("NONE")) {
      success = false;
    }
    else {
      //  Chosen ground source is an imported shape in project
      if (m_groundSourceCombo->currentText().contains(".ecub")) {
        Shape *shape = m_nameToShapeMap[m_groundSourceCombo->currentText()];
        groundFile = FileName(shape->fileName());
        //groundSourceType = shape->surfacePointSource();
        success = true;
      }
      //  Not imported shape, must be from m_editPoints AprioriXYZSource in the cnet
      else if (m_editPoint->HasAprioriSurfacePointSourceFile()) {
        groundFile = FileName(m_groundSourceCombo->currentText());
        //  Apriori ground source does not exist and user chose not to give new location so simply
        //  return unsuccessful
        if (!groundFile.fileExists()) {
          success = false;
        }
        else {
          groundSourceType = m_editPoint->GetAprioriSurfacePointSource();
          success = true;
        }
      }
    }

    // If a new ground file was found set ground source information for later use, first clearing
    // out the old ground source information.  If new ground same as old ground, we will not change
    // anything, simply return successful.
    if (success && (groundFile.expanded() != m_groundFilename)) {
      clearGroundSource();
      m_groundFilename = groundFile.expanded();

      // Get cube, then universal groundmap
      QScopedPointer<Cube> groundCube(new Cube(groundFile, "r"));
      m_groundGmap.reset(NULL);
      QScopedPointer<UniversalGroundMap> newGroundGmap(new UniversalGroundMap(*groundCube));
      m_groundGmap.reset(newGroundGmap.take());

      //  Create new serial number for ground source and add to serial number list
      m_groundSN = SerialNumber::Compose(groundFile.expanded(), true);
      m_serialNumberList->add(m_groundFilename, true);

      m_groundSourceType = groundSourceType;
    }
    // Could not successfully find a ground source file, clear out any old info and return
    // unsuccessful
    else if (!success) {
      clearGroundSource();
    }

    return success;
  }


  /**
   * Ground source file from control net cannot be found, give user option to give new location.
   * The option also exists to change for the rest of the ground points in the control net and to
   * change the apriori xyz source file in the control net to the new location.
   *
   * @param groundFile FileName of ground source that has moved location
   *
   * @return FileName Ground source with new location path
   *
   */
  FileName ControlPointEditWidget::checkGroundFileLocation(FileName groundFile) {

    FileName newGroundFile;

    if (!groundFile.fileExists()) {

      // If user previously chose to change all ground source locations, but didn't update the net
      // fix this ground source to new location
      // If all groundLocations are to be changed...
      if (m_changeAllGroundLocation) {
        QFileInfo oldFile(groundFile.expanded());
        QFileInfo newFile(m_newGroundDir, oldFile.fileName());

        newGroundFile = FileName(newFile.absoluteFilePath());
      }

      //  If can't find ground, re-prompt user for new location. Maybe it's a new ground source.
      if (!newGroundFile.fileExists()) {
        //  Give options for finding ground source file location. A new location
        //  for new location or new source, either a Shape in the project, or import a new shape,
        //  or simplay choose file?
        QString message = "Ground Source file " + groundFile.expanded();
        message += " doesn't exist.  Has the file moved?  Would you like to enter a new location for"
                   " this ground source?";
        int ret = QMessageBox::question(this, "Ground Source not found", message);
        if (ret == QMessageBox::Yes) {
          QString dir = m_directory->project()->shapeDataRoot();
          NewGroundSourceLocationDialog *dialog = new NewGroundSourceLocationDialog(
              "New Ground Source Location", dir);
          if (dialog->exec() == QDialog::Accepted) {
            m_newGroundDir = dialog->selectedFiles().value(0);
            m_changeAllGroundLocation = dialog->changeAllGroundSourceLocation();
            m_changeGroundLocationInNet = dialog->changeControlNet();
            //  Change all ground source locations to reflect new directory
            if (m_changeGroundLocationInNet) {
              changeGroundLocationsInNet();
            }
            //  Change location of apriori for current edit point so combo boxes updated properly
            QFileInfo oldFile(groundFile.expanded());
            QFileInfo newFile(m_newGroundDir, oldFile.fileName());
            newGroundFile = newFile.absoluteFilePath();
            m_editPoint->SetAprioriSurfacePointSourceFile(newGroundFile.toString());
          }
          else {
            //  Either user does not want to change location of ground source or the new location
            //  Dialog was cancelled. Load point without the ground source.
            newGroundFile = NULL;
          }
        }
        else {
          //  Either user does not want to change location of ground source or the new location
          //  Dialog was cancelled. Load point without the ground source.
          newGroundFile = NULL;
        }
      }
    }
    return newGroundFile;
  }


  /**
   * Change the location of all ground source locations in the ControlNet.  This changes the
   * ControlPoint parameter, AprioriSurfacePointSourceFile.
   *
   */
  void ControlPointEditWidget::changeGroundLocationsInNet() {

    for (int i = 0; i < m_controlNet->GetNumPoints(); i++ ) {
      ControlPoint *cp = m_controlNet->GetPoint(i);
      if (cp->HasAprioriSurfacePointSourceFile()) {
        FileName groundFile(cp->GetAprioriSurfacePointSourceFile());
        QFileInfo oldFile(groundFile.expanded());
        QFileInfo newFile(m_newGroundDir, oldFile.fileName());
        groundFile = newFile.absoluteFilePath();
        cp->SetAprioriSurfacePointSourceFile(groundFile.expanded());
      }
    }
    emit cnetModified();
  }


  /**
   * Open a radius source using the shape model of the reference measure of m_editPoint
   *
   * @author 2016-10-07 Makayla Shepherd - Changed radius source handling and moved it from OpenGround.
   *
   */
  void ControlPointEditWidget::openReferenceRadius() {

    //Get the reference image's shape model
    QString referenceSN = m_editPoint->GetReferenceSN();
    QString referenceFileName = m_serialNumberList->fileName(referenceSN);
    QScopedPointer<Cube> referenceCube(new Cube(referenceFileName, "r"));
    PvlGroup kernels = referenceCube->group("Kernels");
    QString shapeFile = kernels["ShapeModel"];

    //  If the reference measure has a shape model cube then set that as the radius
    //  This will NOT WORK for shape model files (not the default of Null or Ellipsoid)
    //  that are not cubes
    if (shapeFile.contains(".cub")) {
      if (shapeFile.contains("dem")) {
        m_radiusSourceType = ControlPoint::RadiusSource::DEM;
      }
      else {
        m_radiusSourceType = ControlPoint::RadiusSource::Ellipsoid;
      }

      m_radiusFilename = shapeFile;
      initDem(shapeFile);
    }
    //  If no shape model then use the ABC of the target body
    else {
      m_radiusSourceType = ControlPoint::RadiusSource::Ellipsoid;
      Spice *refSpice = new Spice(*referenceCube);
      Distance refRadii[3];
      refSpice->radii(refRadii);
      m_demFile = QString::number(refRadii[0].meters()) + ", " +
                  QString::number(refRadii[1].meters()) + ", " +
                  QString::number(refRadii[2].meters());

      m_radiusFilename = "";
    }
  }


  /**
   * Initialize the given Dem and appropriate member variables for later use editing Fixed or
   * Constrained control points.
   *
   * @param demFile QString The file name of the DEM
   *
   */
  void ControlPointEditWidget::initDem (QString demFile) {

      // If a DEM is already opened, check if new is same as old. If new,
      //  close old, open new.
      if (m_demOpen) {
        if (m_demFile == demFile) {
          return;
        }

        m_demCube.reset(NULL);
        m_demFile.clear();
      }

      QApplication::setOverrideCursor(Qt::WaitCursor);
      try {
        QScopedPointer<Cube> newDemCube(new Cube(demFile, "r"));

        m_demFile = FileName(newDemCube->fileName()).name();
        m_demCube.reset(newDemCube.take());
      }
      catch (IException &e) {
        QMessageBox::critical(this, "Error", e.toString());
        QApplication::restoreOverrideCursor();
        return;
      }
      m_demOpen = true;

      //  Make sure this is a dem
      if (!m_demCube->hasTable("ShapeModelStatistics")) {
        QString message = m_demFile + " is not a DEM.";
        QMessageBox::critical(this, "Error", message);
        m_demCube.reset(NULL);
        m_demOpen = false;
        m_demFile.clear();
        QApplication::restoreOverrideCursor();
        return;
      }
      m_radiusSourceType = ControlPoint::RadiusSource::DEM;
      m_radiusFilename = demFile;

      QApplication::restoreOverrideCursor();
  }


  /**
   * Return a radius values from the dem using bilinear interpolation
   *
   * @author  2011-08-01 Tracie Sucharski
   *
   * @internal
   */
  double ControlPointEditWidget::demRadius(double latitude, double longitude) {

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

    return radius;
  }


  /**
   * Slot called when user changes selection in m_groundSourceCombo
   *
   * @param index int
   *
   */
  void ControlPointEditWidget::groundSourceFileSelectionChanged(int index) {

    QString newChosenGroundFile = m_groundSourceCombo->currentText();
    if (newChosenGroundFile == m_groundFilename) {
      return;
    }
    loadGroundMeasure();
  }


  /**
   *   Clear out the ground source used for Constrained or Fixed control points.  Clears serial
   *   number, cube and gui elements.
   */
  void ControlPointEditWidget::clearGroundSource () {

    if (m_groundSN.isEmpty()) {
      return;
    }

    //  If the loaded point is a fixed point, see if there is a temporary measure
    //  holding the coordinate information for the current ground source. If so,
    //  delete this measure and remove from the Chip viewport and measure selection combo.
    if (m_editPoint && m_editPoint->GetType() != ControlPoint::Free &&
        m_editPoint->HasSerialNumber(m_groundSN)) {
      m_editPoint->Delete(m_groundSN);

      if (m_leftCombo->findText(QFileInfo(m_groundFilename).fileName()) >= 0) {
        m_leftCombo->removeItem(m_leftCombo->findText(QFileInfo(m_groundFilename).fileName()));
        if (m_leftMeasure->GetCubeSerialNumber() == m_groundSN) {
          selectLeftMeasure(0);
        }
      }
      if (m_rightCombo->findText(QFileInfo(m_groundFilename).fileName())) {
        m_rightCombo->removeItem(m_rightCombo->findText(QFileInfo(m_groundFilename).fileName()));
        if (m_rightMeasure->GetCubeSerialNumber() == m_groundSN) {
          selectRightMeasure(0);
        }
      }
      m_pointFiles.removeAll(m_groundFilename);
    }
    // Remove from serial number list
    m_serialNumberList->remove(m_groundSN);

    //  Reset ground source variables
    m_groundFilename.clear();
    m_groundSN.clear();
    m_groundGmap.reset(NULL);
    m_groundSourceType = ControlPoint::SurfacePointSource::None;
  }


  /**
   * Slot called by Directory to set the control point for editing
   *
   * @param controlPoint (ControlPoint *) ControlPoint that will be loaded into editor
   * @param serialNumber (QString) Optional parameter indicating the serial number of the cube that
   *                                 the point was chosen from
   */
  void ControlPointEditWidget::setEditPoint(ControlPoint *controlPoint, QString serialNumber) {

    //  If m_editPoint was never saved to control net (parent=NULL), delete
    if (m_editPoint != NULL && m_editPoint->Parent() == NULL) {
      delete m_editPoint;
      m_editPoint = NULL;
    }

    //  If incoming control point is new point (has not been saved to control net, simply assign
    //  to m_editPoint, otherwise create copy.  It will not be saved to net until "Save Point"
    //  is selected
    if (controlPoint->Parent() == NULL) {
      m_editPoint = controlPoint;

      // New point in editor, so colorize all save buttons
      colorizeAllSaveButtons("red");
    }
    else {
      m_editPoint = new ControlPoint;
      *m_editPoint = *controlPoint;

      // New point loaded, make sure all save button's text is default black color
      colorizeAllSaveButtons("black");
    }
    loadPoint(serialNumber);
    loadTemplateFile(m_measureEditor->templateFileName());
  }


  void ControlPointEditWidget::colorizeAllSaveButtons(QString color) {

    if (color == "black") {
      // Don't need to colorize save measure button, when loading new measure, the measure editor
      // will set back to default palette.
    m_savePoint->setPalette(m_saveDefaultPalette);
      m_saveNet->setPalette(m_saveDefaultPalette);
    }
    else if (color == "red") {
      m_measureEditor->colorizeSaveButton();
      colorizeSavePointButton();
      colorizeSaveNetButton();
    }
  }


  /**
   * Load point into ControlPointEditWidget.
   *
   * @param serialNumber (QString) The serial number of cube point was chosen from.
   *
   * @internal
   *   @history 2008-11-26  Jeannie Walldren - Added "Number of Measures" to
   *                           ControlPointEditWidget point information.
   *   @history 2010-06-03 Jeannie Walldren - Removed "std::" since "using
   *                          namespace std"
   *   @history 2010-10-29 Tracie Sucharski - Changed pointfiles to QStringList
   *   @history 2011-04-20 Tracie Sucharski - Was not setting EditLock check box
   *   @history 2011-07-18 Tracie Sucharski - Fixed bug with loading
   *                          ground measure-use AprioriSurface point, not lat,lon
   *                          of reference measure unless there is no apriori
   *                          surface point.
   *   @history 2012-05-08 Tracie Sucharski - m_leftFile changed from IString to QString.
   *   @history 2012-10-02 Tracie Sucharski - When creating a new point, load the cube the user
   *                          clicked on first on the left side, use m_leftFile.
   */
  void ControlPointEditWidget::loadPoint(QString serialNumber) {

    //  Write pointId
    QString CPId = m_editPoint->GetId();

    QString ptId("Point ID:  ");
    ptId += (QString) CPId;
    m_ptIdValue->setText(ptId);

    // Set shapes for this point.  Shapes are what has been imported into project.
    setShapesForPoint();

    //  Write number of measures
    QString ptsize = "Number of Measures:  " +
                   QString::number(m_editPoint->GetNumMeasures());
    m_numMeasures->setText(ptsize);

    //  Set EditLock box correctly
    m_lockPoint->setChecked(m_editPoint->IsEditLocked());

    //  Set ignore box correctly
    m_ignorePoint->setChecked(m_editPoint->IsIgnored());


    //  Refill combos since they are dependent on the current edit point
    //  Turn off signals until filled since we don't want slot called.
    m_groundSourceCombo->blockSignals(true);
    m_radiusSourceCombo->blockSignals(true);
    m_groundSourceCombo->clear();
    m_radiusSourceCombo->clear();
    m_groundSourceCombo->addItem("NONE");
    m_groundSourceCombo->setCurrentText("NONE");
    m_radiusSourceCombo->addItem("NONE - Use reference measure's radius");
    m_radiusSourceCombo->setCurrentText("NONE - Use reference measure's radius");

    //  Load any imported project shapes that contain m_editPoint into the ground and any Dem's into
    //  radius source combo boxes. Only add Dems to radius combo.
    if (m_projectShapeNames.count() > 0) {
      for (int i=0; i<m_numberProjectShapesWithPoint; i++) {
        Shape *shape = m_nameToShapeMap[m_projectShapeNames.at(i)];
        if (shape->radiusSource() == ControlPoint::RadiusSource::DEM) {
          m_radiusSourceCombo->addItem(shape->fileName());
        }
        else {
          m_groundSourceCombo->addItem(shape->fileName());
        }
      }
    }

    //  If available, add the CP AprioriSurfacePointSourceFile and AprioriRadiusSourceFile
    if (m_editPoint->HasAprioriSurfacePointSourceFile()) {
      FileName aprioriSurfacePointFile = FileName(m_editPoint->GetAprioriSurfacePointSourceFile());
      //  If file doesn't exist, prompt user for changing location before adding to combo
      if (!aprioriSurfacePointFile.fileExists()) {
        aprioriSurfacePointFile = checkGroundFileLocation(aprioriSurfacePointFile);
      }
      if (!aprioriSurfacePointFile.toString().isEmpty()) {
        m_groundSourceCombo->addItem(aprioriSurfacePointFile.toString());
        m_groundSourceCombo->setCurrentText(aprioriSurfacePointFile.toString());
        m_groundSourceCombo->setItemData(m_groundSourceCombo->currentIndex(),
                                         QColor(Qt::darkGreen), Qt::ForegroundRole);
        m_groundSourceCombo->setItemData(m_groundSourceCombo->currentIndex(),
                                         QFont("DejaVu Sans", 10, QFont::Bold), Qt::FontRole);
      }
    }

    if (m_editPoint->HasAprioriRadiusSourceFile()) {
      //TODO  check location of radius file
      m_radiusSourceCombo->addItem(m_editPoint->GetAprioriRadiusSourceFile());
      m_radiusSourceCombo->setCurrentText(m_editPoint->GetAprioriRadiusSourceFile());
      m_radiusSourceCombo->setItemData(m_radiusSourceCombo->currentIndex(),
                                       QColor(Qt::green), Qt::ForegroundRole);
      m_radiusSourceCombo->setItemData(m_groundSourceCombo->currentIndex(),
                                       QFont("DejaVu Sans", 10, QFont::Bold), Qt::FontRole);
    }
    if (m_editPoint->GetType() == ControlPoint::Free) {
      m_groundSourceCombo->setEnabled(false);
      m_radiusSourceCombo->setEnabled(false);
    }
    else {
      m_groundSourceCombo->setEnabled(true);
      m_radiusSourceCombo->setEnabled(true);
    }
    m_groundSourceCombo->blockSignals(false);
    m_radiusSourceCombo->blockSignals(false);


    //  If constrained or fixed point, create a measure for
    //  the ground source, load reference on left, ground source on right
    if (m_editPoint->GetType() != ControlPoint::Free) {
      // If m_editPoint already has a ground measure, delete
      for (int i=0; i<m_editPoint->GetNumMeasures(); i++) {
        ControlMeasure &m = *(*m_editPoint)[i];
        if (m.GetChooserName() == "GroundMeasureTemporary") {
          m_editPoint->Delete(&m);
        }
      }
      // Create a temporary measure to hold the ground point info for ground source
      // This measure will be deleted when the ControlPoint is saved to the
      // ControlNet.
      ControlMeasure *groundMeasure = createTemporaryGroundMeasure();
      if (groundMeasure) {
        m_editPoint->Add(groundMeasure);
      }
    }



    // Reset PointType combo box appropriately.
    m_pointTypeCombo->clear();
    for (int i=0; i<ControlPoint::PointTypeCount; i++) {
      m_pointTypeCombo->insertItem(i, ControlPoint::PointTypeToString(
            (ControlPoint::PointType) i));
    }
    m_pointTypeCombo->setCurrentText(ControlPoint::PointTypeToString(m_editPoint->GetType()));
    m_pointTypeCombo->setToolTip("Change ControlPoint type");

    updateSurfacePointInfo();



    // Clear combo boxes
    m_leftCombo->clear();
    m_rightCombo->clear();
    m_pointFiles.clear();


    //  Need all files for this point
    for (int i=0; i<m_editPoint->GetNumMeasures(); i++) {
      ControlMeasure &m = *(*m_editPoint)[i];
      QString file = m_serialNumberList->fileName(m.GetCubeSerialNumber());
      m_pointFiles<<file;
      QString tempFileName = FileName(file).name();

      // This actually fills the right combo box for selecting measures.  A model was used to enable
      // drag & drop for ordering measures which will also set the blink order.
      QStandardItem *item = new QStandardItem(tempFileName);
      item->setFlags(item->flags() & ~Qt::ItemIsDropEnabled);
      m_model->appendRow(item);

      m_leftCombo->addItem(tempFileName);

      if (m_editPoint->IsReferenceExplicit() &&
          (QString)m.GetCubeSerialNumber() == m_editPoint->GetReferenceSN()) {
        m_leftCombo->setItemData(i,QFont("DejaVu Sans", 12, QFont::Bold), Qt::FontRole);
        m_rightCombo->setItemData(i,QFont("DejaVu Sans", 12, QFont::Bold), Qt::FontRole);
      }
    }

    m_measureEditor->setPoint(m_editPoint, m_serialNumberList);

    int leftIndex = -1;
    int rightIndex = -1;

    QString referenceSerialNumber;
    //  Check for reference
    if (m_editPoint->IsReferenceExplicit()) {
      referenceSerialNumber = m_editPoint->GetReferenceSN();
      leftIndex = m_editPoint->IndexOfRefMeasure();
    }

    if (!serialNumber.isEmpty() && serialNumber != referenceSerialNumber) {
      QString file = m_serialNumberList->fileName(serialNumber);
      rightIndex = m_rightCombo->findText(FileName(file).name());
      if (leftIndex == -1) {
        if (rightIndex == 0) {
          leftIndex = 1;
        }
        else {
          leftIndex = 0;
        }
      }
    }

    if (leftIndex == -1) {
      if (rightIndex == 0) {
        leftIndex = 1;
      }
      else {
        leftIndex = 0;
      }
    }

    //  If ground measure exists, load in right viewport
    if (m_editPoint->HasSerialNumber(m_groundSN)) {
      rightIndex = m_rightCombo->findText(m_groundSN);
    }
    if (rightIndex <= 0) {
      if (leftIndex == 0) {
        rightIndex =  1;
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

    this->setVisible(true);
    this->raise();
  }


  /**
   * Create a new control point at the given latitude, longitude
   *
   * @param latitude The latitude position for the new control point
   * @param longitude The longitude position for the new control point
   * @param cube The cube that the user used to select position for new control point
   * @param isGroundSource Boolean indicating whether the cube used to choose position is a ground source
   *
   */

  void ControlPointEditWidget::createControlPoint(double latitude, double longitude, Cube *cube,
                                                  bool isGroundSource) {

    //  TODO:   CHECK SUBPIXEL REGISTER RADIO BUTTON OPTION (CHECKBOX?)

    //  Create list box of all files highlighting those that
    //  contain the point.
    QStringList pointFiles;

    Camera *cam;
    for (int i = 0; i < m_serialNumberList->size(); i++) {
      if (m_serialNumberList->serialNumber(i) == m_groundSN) continue;
      cam = m_controlNet->Camera(i);
      if (cam->SetUniversalGround(latitude, longitude)) {
        //  Make sure point is within image boundary
        double samp = cam->Sample();
        double line = cam->Line();
        if (samp >= 1 && samp <= cam->Samples() &&
            line >= 1 && line <= cam->Lines()) {
          pointFiles<<m_serialNumberList->fileName(i);
        }
      }
    }

    // Set shapes from project to fill dialog, indicating number of shapes that contain the CP.
    // The shapes which contain the CP will be at the front of the QStringList.
    setShapesForPoint(latitude, longitude);

    NewControlPointDialog *newPointDialog = new NewControlPointDialog(m_controlNet,
        m_serialNumberList, m_lastUsedPointId, this, true, true, true);
    newPointDialog->setFiles(pointFiles);
    newPointDialog->setGroundSource(m_projectShapeNames, m_numberProjectShapesWithPoint);

    //  Load imported project shapes that are Dems and contain point location into the radius combo.
    if (m_projectShapeNames.count() > 0) {
      QStringList radiusSourceFiles;
      for (int i=0; i<m_numberProjectShapesWithPoint; i++) {
        Shape *shape = m_nameToShapeMap[m_projectShapeNames.at(i)];
        if (shape->radiusSource() == ControlPoint::RadiusSource::DEM) {
          radiusSourceFiles<<shape->fileName();
        }
      }
      newPointDialog->setRadiusSource(radiusSourceFiles);
    }



    if (newPointDialog->exec() == QDialog::Accepted) {
      m_lastUsedPointId = newPointDialog->pointId();
      ControlPoint *newPoint =
          new ControlPoint(m_lastUsedPointId);

      // If this ControlPointId already exists, message box pops up and user is
      // asked to enter a new value.
      if (m_controlNet->ContainsPoint(newPoint->GetId())) {
        QString message = "A ControlPoint with Point Id = [" + newPoint->GetId();
        message += "] already exists.  Re-enter Point Id for this ControlPoint.";
        QMessageBox::warning(this, "New Point Id", message);
        pointFiles.clear();
        delete newPoint;
        newPoint = NULL;
        createControlPoint(latitude, longitude);
        return;
      }

      newPoint->SetChooserName(Application::UserName());

      QStringList selectedFiles = newPointDialog->selectedFiles();
      foreach (QString selectedFile, selectedFiles) {
        //  Create measure for any file selected
        ControlMeasure *m = new ControlMeasure;
        //  Find serial number for this file
        QString sn = m_serialNumberList->serialNumber(selectedFile);
        m->SetCubeSerialNumber(sn);
        int camIndex = m_serialNumberList->fileNameIndex(selectedFile);
        cam = m_controlNet->Camera(camIndex);
        cam->SetUniversalGround(latitude, longitude);
        m->SetCoordinate(cam->Sample(),cam->Line());
        m->SetAprioriSample(cam->Sample());
        m->SetAprioriLine(cam->Line());
        m->SetType(ControlMeasure::Manual);
        m->SetChooserName(Application::UserName());
        m->SetCamera(cam);
        newPoint->Add(m);
      }

      //  Get point type from dialog
      bool isGroundPoint = (newPointDialog->pointType() != ControlPoint::Free);
      newPoint->SetType((ControlPoint::PointType) newPointDialog->pointType());

      if (isGroundPoint) {
        Shape *shape = m_nameToShapeMap[newPointDialog->groundSource()];
        //  Save ground source information in control point
        if (shape) {
          newPoint->SetAprioriSurfacePointSource(shape->surfacePointSource());
        }
        else {
          newPoint->SetAprioriSurfacePointSource(ControlPoint::SurfacePointSource::None);
        }
        newPoint->SetAprioriSurfacePointSourceFile(shape->cube()->externalCubeFileName().expanded());
      }

      setEditPoint(newPoint);
      emit controlPointAdded(newPoint->GetId());
    }
  }


  /**
   * Gives user options for deleting a control point from the control network.
   *
   * @param controlPoint (ControlPoint *) Control point to be deleted
   */
  void ControlPointEditWidget::deletePoint(ControlPoint *controlPoint) {

    // Make a copy and make sure editPoint is a copy (which means it does not
    // have a parent network.
    if (m_editPoint != NULL && m_editPoint->Parent() == NULL) {
      delete m_editPoint;
      m_editPoint = NULL;
    }
    m_editPoint = new ControlPoint;
    *m_editPoint = *controlPoint;
    loadPoint();

    DeleteControlPointDialog *deletePointDialog = new DeleteControlPointDialog;
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
          int  response = QMessageBox::question(this,
                                    "Delete control point", message,
                                    QMessageBox::Yes | QMessageBox::No,
                                    QMessageBox::Yes);
          // If No, do nothing
          if (response == QMessageBox::No) {
            return;
          }
        }

        //this->setVisible(false);
        // remove this point from the control network
        if (m_controlNet->DeletePoint(m_editPoint->GetId()) ==
                                          ControlPoint::PointLocked) {
          QMessageBox::information(this, "EditLocked Point",
              "This point is EditLocked and cannot be deleted.");
          return;
        }
        if (m_editPoint != NULL && m_editPoint->Parent() == NULL) {
//        delete m_editPoint;
//        m_editPoint = NULL;
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
            switch (QMessageBox::question(this,
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
          QMessageBox::information(this,"EditLocked Measures",
                QString::number(lockedMeasures) + " / "
                + QString::number(
                  deletePointDialog->fileList->selectedItems().size()) +
                " measures are EditLocked and were not deleted.");
        }

        loadPoint();

//      loadTemplateFile(m_pointEditor->templateFileName());
      }

      // emit a signal to alert user to save when exiting
      m_control->setModified(true);
      emit cnetModified();

      if (m_editPoint != NULL) {
        //  Change Save Point button text to red
        colorizeSaveNetButton();
      }
    }
  }


  /**
   * This method is connected with the measureSaved() signal from ControlMeasureEditWidget.
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
   *   @history 2013-12-05 Tracie Sucharski - If saving measure on fixed or constrained point and
   *                          reference measure is ignored, print warning and return.
   *   @history 2015-06-05 Makayla Shepherd and Ian Humphery - Modified to return out of the
   *                          method when checkReference returns false.
   */
  void ControlPointEditWidget::measureSaved() {

    // Read original measures from the network for comparison with measures
    // that have been edited
    ControlMeasure *origLeftMeasure =
                m_editPoint->GetMeasure(m_leftMeasure->GetCubeSerialNumber());
    ControlMeasure *origRightMeasure =
                m_editPoint->GetMeasure(m_rightMeasure->GetCubeSerialNumber());
    // Neither measure has changed, return
    if (*origLeftMeasure == *m_leftMeasure && *origRightMeasure == *m_rightMeasure) {
      return;
    }

    if (m_editPoint->IsIgnored()) {
      QString message = "You are saving changes to a measure on an ignored ";
      message += "point.  Do you want to set Ignore = False on the point and ";
      message += "both measures?";
      switch (QMessageBox::question(this, "Save Measure", message, "&Yes", "&No", 0, 0)) {
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

    // If this is a fixed or constrained point, and either the left or right measure is the ground
    // source, update the lat,lon,radius.
    //
    if (m_editPoint->GetType() != ControlPoint::Free &&
        (m_leftMeasure->GetCubeSerialNumber() == m_groundSN ||
         m_rightMeasure->GetCubeSerialNumber() == m_groundSN)) {
      // If point is locked and it is not a new point, print error
      if (m_editPoint->IsEditLocked() && m_controlNet->ContainsPoint(m_editPoint->GetId())) {
        QString message = "This control point is edit locked.  The Apriori latitude, longitude and ";
        message += "radius cannot be updated.  You must first unlock the point by clicking the ";
        message += "check box above labeled \"Edit Lock Point\".";
        QMessageBox::warning(this, "Point Locked", message);
        return;
      }
      if (m_leftMeasure->IsIgnored()) {
        QString message = "This is a Constrained or Fixed point and the reference measure is ";
        message += "Ignored.  Unset the Ignore flag on the reference measure before saving.";
        QMessageBox::warning(this, "Point Locked", message);
        return;
      }
      updateGroundPosition();
    }

    // If left measure == right measure, update left
    if (m_leftMeasure->GetCubeSerialNumber() == m_rightMeasure->GetCubeSerialNumber()) {
      *m_leftMeasure = *m_rightMeasure;
      //  Update left measure of measureEditor
      m_measureEditor->setLeftMeasure (m_leftMeasure, m_leftCube.data(),
                                     m_editPoint->GetId());
    }

    //  Change Save Point button text to red
    if (savedAMeasure) {
      colorizeSavePointButton();
    }

    // Update measure info
    updateLeftMeasureInfo();
    updateRightMeasureInfo();
  }


  /**
   * Validates a change to a control measure
   *
   * @param m (ControlMeasure *) Pointer to the ControlMeasure
   *
   * @return @b bool True if the change to the measure is valid
   */
  bool ControlPointEditWidget::validateMeasureChange(ControlMeasure *m) {

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
      int response = QMessageBox::question(this, "Save Measure",
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
      switch (QMessageBox::question(this, "Save Measure", message, "&Yes", "&No", 0, 0)) {
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
          switch(QMessageBox::question(this, "Save Measure",
                                       message, "&Yes", "&No", 0, 0)){
            // Yes:  Save measure
            case 0:
              break;
            // No:  keep original reference, return without saving
            case 1:
              ControlMeasure *origLeftMeasure =
                m_editPoint->GetMeasure(m_leftMeasure->GetCubeSerialNumber());
              m_measureEditor->setLeftPosition(origLeftMeasure->GetSample(),
                                               origLeftMeasure->GetLine());
              return false;
          }
        }
      }
      //  New reference measure
      else if (side == "left" && (refMeasure->GetCubeSerialNumber() != m->GetCubeSerialNumber())) {
        QString message = "This point already contains a reference measure.  ";
        message += "Would you like to replace it with the measure on the left?";
        int  response = QMessageBox::question(this,
                                  "Save Measure", message,
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

          m_editPoint->SetRefMeasure(m->GetCubeSerialNumber());
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


  /**
  * Change which measure is the reference.
  *
  * @author 2012-04-26 Tracie Sucharski - moved funcitonality from measureSaved
  *
  * @internal
  *   @history 2012-06-12 Tracie Sucharski - Moved check for ground loaded on left from the
  *                          measureSaved method.
  */
  void ControlPointEditWidget::checkReference() {

    // Check if ControlPoint has reference measure, if reference Measure is
    // not the same measure that is on the left chip viewport, set left
    // measure as reference.
    ControlMeasure *refMeasure = m_editPoint->GetRefMeasure();
    if (refMeasure->GetCubeSerialNumber() != m_leftMeasure->GetCubeSerialNumber()) {
      QString message = "This point already contains a reference measure.  ";
      message += "Would you like to replace it with the measure on the left?";
      int  response = QMessageBox::question(this,
                                "Match Tool Save Measure", message,
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
      }

          // ??? Need to set rest of measures to Candiate and add more warning. ???//
    }
  }


  /*
  * Update the position of ground point
  *
  * @author 2012-04-26 Tracie Sucharski - moved functionality from measureSaved
  *
  * @internal
  *
  */
  void ControlPointEditWidget::updateGroundPosition() {

    //  Determine if the left or right measure is the ground.  Use ground measure to update
    //  apriori surface point.
    ControlMeasure *groundMeasure;
    if (m_leftMeasure->GetCubeSerialNumber() == m_groundSN) {
      groundMeasure = m_leftMeasure;
    }
    else {
      groundMeasure = m_rightMeasure;
    }
    m_groundGmap->SetImage(groundMeasure->GetSample(), groundMeasure->GetLine());

    double lat = m_groundGmap->UniversalLatitude();
    double lon = m_groundGmap->UniversalLongitude();

    //  Find radius source file
    //  If nothing has been selected from the Radius source combo, use the Reference measure
    if (m_radiusSourceCombo->currentText().contains("NONE")) {
      m_radiusFilename.clear();
      m_demOpen = false;
      m_demFile.clear();
      m_demCube.reset(NULL);
      openReferenceRadius();
    }
    else {
      Shape *shape = m_nameToShapeMap[m_radiusSourceCombo->currentText()];
      if (shape) {
        m_radiusFilename = shape->cube()->externalCubeFileName().toString();
        //m_radiusSourceType = shape->radiusSource();
      }
      // Radius source comes from what is already saved in the cnet as AprioriRadiusSourceFile
      // This will contain the path
      else {
        m_radiusFilename = m_radiusSourceCombo->currentText();
        m_radiusSourceType = m_editPoint->GetAprioriRadiusSource();
      }
      initDem(m_radiusFilename);
    }


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
        QMessageBox::warning(this, "Warning", msg);
        if (m_editPoint->GetRefMeasure()->Camera()->SetGround(Latitude(lat, Angle::Degrees),
                                                              Longitude(lon, Angle::Degrees))) {
          radius = m_editPoint->GetRefMeasure()->Camera()->LocalRadius().meters();
          //  TODO  Should this be set here, this is probably not working as intended since it is
          //         overwritten below outside of if (radius == Null)
          m_editPoint->SetAprioriRadiusSource(ControlPoint::RadiusSource::None);
        }
        else {
          QString message = "Error trying to get radius at this pt.  "
              "Lat/Lon does not fall on the reference measure.  "
              "Cannot save this measure.";
          QMessageBox::critical(this,"Error",message);
          return;
        }
      }
      m_editPoint->SetAprioriRadiusSource(m_radiusSourceType);
      m_editPoint->SetAprioriRadiusSourceFile(m_radiusFilename);
    }
    else {
      //  Get radius from reference image
      if (m_editPoint->GetRefMeasure()->Camera()->SetGround(Latitude(lat, Angle::Degrees),
                                                            Longitude(lon, Angle::Degrees))) {
        radius = m_editPoint->GetRefMeasure()->Camera()->LocalRadius().meters();
      }
      else {
        QString message = "Error trying to get radius at this pt.  "
            "Lat/Lon does not fall on the reference measure.  "
            "Cannot save this measure.";
        QMessageBox::critical(this,"Error",message);
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
      QMessageBox::critical(this,"Error",message);
      return;
    }

    m_editPoint->SetAprioriSurfacePointSource(m_groundSourceType);
    QString fullGroundFilename;
    if (m_groundFilename.contains(".ecub")) {
      // Find shape to get external cube filename
      fullGroundFilename = m_nameToShapeMap[m_groundFilename]->cube()->externalCubeFileName().expanded();
    }
    else {
      fullGroundFilename = m_groundFilename;
    }
    m_editPoint->SetAprioriSurfacePointSourceFile(fullGroundFilename);

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
  void ControlPointEditWidget::savePoint() {

    //  Make a copy of edit point for updating the control net since the edit
    //  point is still loaded in the point editor.
    ControlPoint *updatePoint = new ControlPoint;
    *updatePoint = *m_editPoint;

    //  If this is a fixed or constrained point, see if there is a temporary
    //  measure holding the coordinate information from the ground source.
    //  If so, delete this measure before saving point.  Clear out the
    //  fixed Measure variable (memory deleted in ControlPoint::Delete).
    if (updatePoint->GetType() != ControlPoint::Free && updatePoint->HasSerialNumber(m_groundSN)) {
      // Delete measure with m_groundSN
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
      emit controlPointChanged(m_editPoint->GetId());
    }
    else {
      m_controlNet->AddPoint(updatePoint);
      emit controlPointAdded(m_editPoint->GetId());
    }

    //  Change Save Measure button text back to default
    m_savePoint->setPalette(m_saveDefaultPalette);

    // At exit, or when opening new net, use for prompting user for a save
    m_cnetModified = true;
    m_control->setModified(true);
    emit cnetModified();
    //   Refresh chipViewports to show new positions of controlPoints
    m_measureEditor->refreshChips();
  }


  /**
   * Set the point type
   *
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
  void ControlPointEditWidget::setPointType (int pointType) {

    if (m_editPoint == NULL) return;

    //  If pointType is equal to current type, nothing to do
    if (m_editPoint->GetType() == pointType) return;
    int oldType = m_editPoint->GetType();

    // Error check ignored and locked status
    if (pointType != ControlPoint::Free && m_leftMeasure->IsIgnored()) {
      m_pointTypeCombo->setCurrentIndex((int) m_editPoint->GetType());
      QString message = "The reference measure is Ignored.  Unset the Ignore flag on the ";
      message += "reference measure before setting the point type to Constrained or Fixed.";
      QMessageBox::warning(m_parent, "Ignored Reference Measure", message);
      return;
    }
    ControlPoint::Status status = m_editPoint->SetType((ControlPoint::PointType) pointType);
    if (status == ControlPoint::PointLocked) {
      m_pointTypeCombo->setCurrentIndex((int) m_editPoint->GetType());
      QString message = "This control point is edit locked.  The point type cannot be changed.  You ";
      message += "must first unlock the point by clicking the check box above labeled ";
      message += "\"Edit Lock Point\".";
      QMessageBox::warning(m_parent, "Point Locked", message);
      return;
    }

    // Changing type between Contrained and Fixed, simply colorize Save CP button and return.  Do
    // not re-load CP or re-create ground CM.
    if (oldType != ControlPoint::Free && m_editPoint->GetType() != ControlPoint::Free) {
      colorizeSavePointButton();
    }
    // If changing from Constrained or Fixed to Free, remove ground CM, disable ground/radius source
    // combos, re-load CP, colorize Save CP.
    else if (oldType != ControlPoint::Free && m_editPoint->GetType() == ControlPoint::Free) {
      //  Find temporary measure holding the coordinate information from the ground source and
      //  delete from CP. This CM has a chooser name = GroundMeasureTemporary.
      //  Clear out the fixed Measure variable (memory deleted in ControlPoint::Delete).
      for (int i=0; i<m_editPoint->GetNumMeasures(); i++) {
        ControlMeasure &m = *(*m_editPoint)[i];
        if (m.GetChooserName() == "GroundMeasureTemporary") {
          m_editPoint->Delete(&m);
        }
      }
      loadPoint();
      m_groundSourceCombo->setEnabled(false);
      m_radiusSourceCombo->setEnabled(false);
      colorizeSavePointButton();
    }
    // Changing from Free to Constrained or Fixed, loadGroundMeasure,which will create temporary
    // gound measure, load into the measure combo boxes and colorize Save CP button.
    else if (oldType == ControlPoint::Free && m_editPoint->GetType() != ControlPoint::Free) {
      loadGroundMeasure();
      m_groundSourceCombo->setEnabled(true);
      m_radiusSourceCombo->setEnabled(true);
      colorizeSavePointButton();
    }
  }


  /**
   * Set point's "EditLock" keyword to the value of the input parameter.
   *
   * @param lock Boolean value that determines the EditLock value for this point.
   *
   * @author 2011-03-07 Tracie Sucharski
   *
   * @internal
   */
  void ControlPointEditWidget::setLockPoint (bool lock) {

    if (m_editPoint == NULL) return;

    m_editPoint->SetEditLock(lock);
    colorizeSavePointButton();
  }


  /**
   * Set point's "Ignore" keyword to the value of the input parameter.
   *
   * @param ignore Boolean value that determines the Ignore value for this point.
   *
   * @internal
   * @history 2010-12-15 Tracie Sucharski - Remove netChanged, the point is
   *                        not changed in the net unless "Save Point" is
   *                        selected.
   */
  void ControlPointEditWidget::setIgnorePoint (bool ignore) {

    if (m_editPoint == NULL) return;

    ControlPoint::Status status = m_editPoint->SetIgnored(ignore);
    if (status == ControlPoint::PointLocked) {
      m_ignorePoint->setChecked(m_editPoint->IsIgnored());
      QString message = "Unable to change Ignored on point.  Set EditLock ";
      message += " to False.";
      QMessageBox::critical(this, "Error", message);
      return;
    }
    colorizeSavePointButton();
  }


  /**
   * Set the "EditLock" keyword of the measure shown in the left viewport to the
   * value of the input parameter.
   *
   * @param lock Boolean value that determines the EditLock value for the left measure.
   *
   * @author 2011-03-07 Tracie Sucharski
   *
   * @internal
   * @history 2011-06-27 Tracie Sucharski - emit signal indicating a measure
   *                        parameter has changed.
   * @history 2012-04-16 Tracie Sucharski - When attempting to un-lock a measure
   *                        print error if point is locked.
   */
  void ControlPointEditWidget::setLockLeftMeasure (bool lock) {

    if (m_editPoint->IsEditLocked()) {
      m_lockLeftMeasure->setChecked(m_leftMeasure->IsEditLocked());
      QMessageBox::warning(this, "Point Locked","Point is Edit Locked.  You must un-lock point"
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
   * @param ignore Boolean value that determines the Ignore value for the left measure.
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
  void ControlPointEditWidget::setIgnoreLeftMeasure (bool ignore) {

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
   * @param lock Boolean value that determines the EditLock value for the right measure.
   *
   * @author 2011-03-07 Tracie Sucharski
   *
   * @internal
   * @history 2011-06-27 Tracie Sucharski - emit signal indicating a measure
   *                        parameter has changed.
   * @history 2012-04-16 Tracie Sucharski - When attempting to un-lock a measure
   *                        print error if point is locked.
   */
  void ControlPointEditWidget::setLockRightMeasure (bool lock) {

    if (m_editPoint->IsEditLocked()) {
      m_lockRightMeasure->setChecked(m_rightMeasure->IsEditLocked());
      QMessageBox::warning(this, "Point Locked","Point is Edit Locked.  You must un-lock point"
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
   * @param ignore Boolean value that determines the Ignore value for the right measure.
   *
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
  void ControlPointEditWidget::setIgnoreRightMeasure (bool ignore) {

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
   * @brief Selects the next right measure when activated by key shortcut
   *
   * This slot is intended to handle selecting the next right measure when the attached shortcut
   * (PageDown) is activated. This slot checks if the next index is in bounds.
   *
   * @internal
   *   @history 2015-10-29 Ian Humphrey - Created slot. References #2324.
   */
  void ControlPointEditWidget::nextRightMeasure() {

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
  void ControlPointEditWidget::previousRightMeasure() {

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
   *   @history 2010-06-03 Jeannie Walldren - Removed "std::" since "using namespace std"
   * @history 2011-07-06 Tracie Sucharski - If point is Locked, and measure is
   *                          reference, lock the measure.
   * @history 2012-10-02 Tracie Sucharski - If measure's cube is not viewed, print error and
   *                          make sure old measure is retained.
   */
  void ControlPointEditWidget::selectLeftMeasure(int index) {

    QString file = m_pointFiles[index];
    QString serial;
    try {
      serial = m_serialNumberList->serialNumber(file);
    }
    catch (IException &e) {
      QString message = "Make sure the correct cube is opened.\n\n";
      message += e.toString();
      QMessageBox::critical(this, "Error", message);

      //  Set index of combo back to what it was before user selected new.  Find the index
      //  of current left measure.
      QString file = m_serialNumberList->fileName(m_leftMeasure->GetCubeSerialNumber());
      int i = m_leftCombo->findText(FileName(file).name());
      if (i < 0) i = 0;
      m_leftCombo->setCurrentIndex(i);
      return;
    }

    if (m_leftMeasure != NULL) {
      delete m_leftMeasure;
      m_leftMeasure = NULL;
    }

    m_leftMeasure = new ControlMeasure;
    *m_leftMeasure = *((*m_editPoint)[serial]);

    //  If m_leftCube is not null, delete before creating new one
    m_leftCube.reset(new Cube(file, "r"));

    //  Update left measure of pointEditor
    m_measureEditor->setLeftMeasure (m_leftMeasure, m_leftCube.data(), m_editPoint->GetId());
    updateLeftMeasureInfo ();
  }


  /**
   * Select right measure
   *
   * @param index  Index of file from the point files vector
   *
   * @internal
   *   @history 2010-06-03 Jeannie Walldren - Removed "std::" since "using namespace std"
   * @history 2012-10-02 Tracie Sucharski - If measure's cube is not viewed, print error and
   *                          make sure old measure is retained.
   */
  void ControlPointEditWidget::selectRightMeasure(int index) {

    QString file = m_pointFiles[index];

    QString serial;
    try {
      serial = m_serialNumberList->serialNumber(file);
    }
    catch (IException &e) {
      QString message = "Make sure the correct cube is opened.\n\n";
      message += e.toString();
      QMessageBox::critical(this, "Error", message);

      //  Set index of combo back to what it was before user selected new.  Find the index
      //  of current left measure.
      QString file = m_serialNumberList->fileName(m_rightMeasure->GetCubeSerialNumber());
      int i = m_rightCombo->findText(FileName(file).name());
      if (i < 0) i = 0;
      m_rightCombo->setCurrentIndex(i);
      return;
    }

    if (m_rightMeasure != NULL) {
      delete m_rightMeasure;
      m_rightMeasure = NULL;
    }

    m_rightMeasure = new ControlMeasure;
    *m_rightMeasure = *((*m_editPoint)[serial]);

    //  If m_rightCube is not null, delete before creating new one
    m_rightCube.reset(new Cube(file, "r"));

    //  Update left measure of pointEditor
    m_measureEditor->setRightMeasure (m_rightMeasure, m_rightCube.data(), m_editPoint->GetId());
    updateRightMeasureInfo ();
  }


  /**
   * Update the left measure information
   *
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
  void ControlPointEditWidget::updateLeftMeasureInfo () {

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
    if (m_leftMeasure->GetType() == ControlMeasure::Candidate) s += "Candidate";
    if (m_leftMeasure->GetType() == ControlMeasure::Manual) s += "Manual";
    if (m_leftMeasure->GetType() == ControlMeasure::RegisteredPixel) s += "RegisteredPixel";
    if (m_leftMeasure->GetType() == ControlMeasure::RegisteredSubPixel) s += "RegisteredSubPixel";
    m_leftMeasureType->setText(s);
  }


  /**
   * Update the right measure information
   *
   * @internal
   * @history 2008-11-24 Jeannie Walldren - Added "Goodness of Fit" to right
   *                         measure info.
   * @history 2010-06-03 Jeannie Walldren - Removed "std::" since "using
   *                         namespace std"
   * @history 2010-07-22 Tracie Sucharski - Updated new measure types
   *                         associated with implementation of binary
   *                         control networks.
   * @history 2010-12-27 Tracie Sucharski - Write textual Null instead of
   *                         the numeric Null for sample & line residuals.
   * @history 2011-04-20 Tracie Sucharski - Set EditLock check box correctly
   * @history 2011-05-20 Tracie Sucharski - Added Reference output
   * @history 2011-07-19 Tracie Sucharski - Did some re-arranging and added
   *                         sample/line shifts.
   *
   */
  void ControlPointEditWidget::updateRightMeasureInfo() {

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
  }



  /**
   * Event filter for ControlPointEditWidget.  Determines whether to update left or right
   * measure info.
   *
   * @param o Pointer to QObject
   * @param e Pointer to QEvent
   *
   * @return @b bool Indicates whether the event type is "Leave".
   *
   */
  bool ControlPointEditWidget::eventFilter(QObject *o, QEvent *e) {

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
   * Checks the state of the template registration file and determines if it is safe to
   * continue opening a template file
   *
   * @return @b bool True if the template file was saved (user clicked "Yes")
   */
  bool ControlPointEditWidget::okToContinue() {

    if (m_templateModified) {
      int r = QMessageBox::warning(this, tr("OK to continue?"),
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
   * Prompt user for a registration template file to open. Once the file is
   * selected, loadTemplateFile() is called to update the current template file
   * being used.
   */
  void ControlPointEditWidget::openTemplateFile() {

    if (!okToContinue())
      return;

    QString filename = QFileDialog::getOpenFileName(this,
        "Select a registration template", ".",
        "Registration template files (*.def *.pvl);;All files (*)");

    if (filename.isEmpty())
      return;

    if (m_measureEditor->setTemplateFile(filename)) {
      loadTemplateFile(filename);
    }
  }


  /**
   * Updates the current template file being used.
   *
   * @param fn The file path of the new template file
   */
  void ControlPointEditWidget::loadTemplateFile(QString fn) {

    QFile file(FileName((QString) fn).expanded());
    if (!file.open(QIODevice::ReadOnly)) {
      QString msg = "Failed to open template file \"" + fn + "\"";
      QMessageBox::warning(this, "IO Error", msg);
      return;
    }

    QTextStream stream(&file);
    m_templateEditor->setText(stream.readAll());
    file.close();

    QScrollBar * sb = m_templateEditor->verticalScrollBar();
    sb->setValue(sb->minimum());

    m_templateModified = false;
    m_saveTemplateFile->setEnabled(false);
  }


  /**
   * Called when the template file is modified by the template editor
   */
  void ControlPointEditWidget::setTemplateModified() {

    m_templateModified = true;
    m_saveTemplateFile->setEnabled(true);
  }


  /**
   * Save the file opened in the template editor
   */
  void ControlPointEditWidget::saveTemplateFile() {

    if (!m_templateModified)
      return;

    QString filename =
        m_measureEditor->templateFileName();

    writeTemplateFile(filename);
  }


  /**
   * Save the contents of template editor to a file chosen by the user
   */
  void ControlPointEditWidget::saveTemplateFileAs() {

    QString filename = QFileDialog::getSaveFileName(this,
        "Save registration template", ".",
        "Registration template files (*.def *.pvl);;All files (*)");

    if (filename.isEmpty())
      return;

    writeTemplateFile(filename);
  }


  /**
   * Write the contents of the template editor to the file provided.
   *
   * @param fn The filename to write to
   */
  void ControlPointEditWidget::writeTemplateFile(QString fn) {

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
      QMessageBox::warning(this, "Error", message);
      return;
    }

    QString expandedFileName(FileName((QString) fn).expanded());

    QFile file(expandedFileName);

    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
      QString msg = "Failed to save template file to \"" + fn + "\"\nDo you "
          "have permission?";
      QMessageBox::warning(this, "IO Error", msg);
      return;
    }

    // now save contents
    QTextStream stream(&file);
    stream << contents;
    file.close();

    if (m_measureEditor->setTemplateFile(fn)) {
      m_templateModified = false;
      m_saveTemplateFile->setEnabled(false);
    }
  }


  /**
   * Allows the user to view the template file that is currently set.
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
  void ControlPointEditWidget::viewTemplateFile() {

    try{
      // Get the template file from the ControlPointEditWidget object
      Pvl templatePvl(m_measureEditor->templateFileName());
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
      QMessageBox::information(this, "Error", message);
    }
  }


  /**
   * Slot which calls ControlPointEditWidget slot to save chips
   *
   * @author 2009-03-17 Tracie Sucharski
   */

  void ControlPointEditWidget::saveChips() {

    m_measureEditor->saveChips();
  }


  /**
   * Toggles the visibility of the template editor widget
   */
  void ControlPointEditWidget::showHideTemplateEditor() {

    if (!m_templateEditorWidget)
      return;

    m_templateEditorWidget->setVisible(!m_templateEditorWidget->isVisible());
  }


  /**
  * Add registration TemplateList to combobox when imported to project
  *
  * @param templateList Reference to TemplateList that was imported
  */
  void ControlPointEditWidget::addTemplates(TemplateList *templateList) {
    if(templateList->type() == "registrations") {
      for(int i = 0; i < templateList->size(); i++) {
        m_templateComboBox->addItem(templateList->at(i)->importName()
                                    + "/" + FileName(templateList->at(i)->fileName()).name());
      }
    }
  }


  /**
  * Appends the filename to the registrations path (unless this is the default template) and calls
  * setTemplateFile for the control measure
  *
  * @param filename This is the import directory and the filename of the template file selected from
  *                 the QComboBox.
  */
  void ControlPointEditWidget::setTemplateFile(QString filename) {
    QString expandedFileName = filename;
    if(!filename.startsWith("$base")){
      expandedFileName = m_directory->project()->templateRoot()
                                 + "/registrations/" + filename;
    }
    if (m_measureEditor->setTemplateFile(expandedFileName)) {
      loadTemplateFile(expandedFileName);
    }
  }


  /**
  * Reset the selected template in the template combobox if the template selected by the user does
  * not satisfy requirements for the control measure.
  *
  * @param fileName The filename that was previously selected in the template combo box
  */
  void ControlPointEditWidget::resetTemplateComboBox(QString fileName) {
    if(fileName.startsWith("$base")) {
      m_templateComboBox->setCurrentIndex(0);
    }
    QList<QString> components = fileName.split("/");
    int size = components.size();
    int index = m_templateComboBox->findText(components[size - 2] + "/" + components[size - 1]);
    if (index != -1) {
      m_templateComboBox->setCurrentIndex(index);
    }
  }


  /**
   * Update the Surface Point Information in the ControlPointEditWidget
   *
   * @author 2011-03-01 Tracie Sucharski
   *
   * @internal
   * @history 2011-05-12 Tracie Sucharski - Type printing Apriori Values
   * @history 2011-05-24 Tracie Sucharski - Set target radii on apriori
   *                        surface point, so that sigmas can be converted to
   *                        meters.
   */
  void ControlPointEditWidget::updateSurfacePointInfo() {

    QString s;

    SurfacePoint aprioriPoint = m_editPoint->GetAprioriSurfacePoint();
    if (aprioriPoint.GetLatitude().degrees() == Null) {
      s = "Apriori Latitude:  Null";
    }
    else {
      s = "Apriori Latitude:  " +
          QString::number(aprioriPoint.GetLatitude().degrees());
    }
    m_aprioriLatitude->setText(s);
    if (aprioriPoint.GetLongitude().degrees() == Null) {
      s = "Apriori Longitude:  Null";
    }
    else {
      s = "Apriori Longitude:  " +
          QString::number(aprioriPoint.GetLongitude().degrees());
    }
    m_aprioriLongitude->setText(s);
    if (aprioriPoint.GetLocalRadius().meters() == Null) {
      s = "Apriori Radius:  Null";
    }
    else {
      s = "Apriori Radius:  " +
          QString::number(aprioriPoint.GetLocalRadius().meters(),'f',2) +
          " <meters>";
    }
    m_aprioriRadius->setText(s);
  }


  /**
   * Refresh all necessary widgets in ControlPointEditWidget including the PointEditor and
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
//  TODO  Is this needed?
//
//  void ControlPointEditWidget::refresh() {
//
//    //  Check point being edited, make sure it still exists, if not ???
//    //  Update ignored checkbox??
//    if (m_editPoint != NULL) {
//      try {
//        QString id = m_ptIdValue->text().remove("Point ID:  ");
//        m_controlNet->GetPoint(id);
//      }
//      catch (IException &) {
//        delete m_editPoint;
//        m_editPoint = NULL;
//        emit controlPointChanged();
////      this->setVisible(false);
////      m_measureWindow->setVisible(false);
//      }
//    }
//  }


  /**
   * Turn "Save Point" button text to red
   *
   * @author 2011-06-14 Tracie Sucharski
   */
  void ControlPointEditWidget::colorizeSavePointButton() {

    QColor qc = Qt::red;
    QPalette p = m_savePoint->palette();
    p.setColor(QPalette::ButtonText,qc);
    m_savePoint->setPalette(p);
  }


  /**
   * Turn "Save Net" button text to red
   *
   * TODO  Need whoever is actually saving network to emit signal when net has been saved, so that
   * button can be set back to black.
   *
   * @author 2014-07-11 Tracie Sucharski
   */
  void ControlPointEditWidget::colorizeSaveNetButton(bool reset) {

    if (reset) {
      //  Change Save Net button text back to default black
      m_saveNet->setPalette(m_saveDefaultPalette);
    }
    else {
    QColor qc = Qt::red;
    QPalette p = m_savePoint->palette();
    p.setColor(QPalette::ButtonText,qc);
    m_saveNet->setPalette(p);
    }

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
   * @author 2011-07-06 Tracie Sucharski
   */
  bool ControlPointEditWidget::IsMeasureLocked (QString serialNumber) {

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
  *  This slot is needed because we cannot directly emit a signal with a ControlNet
  *  argument after the "Save Net" push button is selected.
  *
  * @internal
   *   @history 2014-07-11 Tracie Sucharski - Original version.
  */
  void ControlPointEditWidget::saveNet() {

    m_control->write();

    //  Change Save Measure button text back to default
    m_saveNet->setPalette(m_saveDefaultPalette);
  }


  /**
   * Cleans up the edit point memory
   */
  void ControlPointEditWidget::clearEditPoint() {
    delete m_editPoint;
    m_editPoint = NULL;
  }


  // 2014-07-21 TLS  Ipce  This needs to be changed to return the help information or
  //              widget?? to the calling program, so that it can be added to a menu or toolbar.
#if 0
  void ControlPointEditWidget::showHelp() {

    QDialog *helpDialog = new QDialog(this);
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

    QString toolIconDir = FileName("$base/icons").expanded();

    QLabel *quickPrep = new QLabel("<p><ul>"
        "<li>Open the cubes with overlapping areas for choosing control points</li>"
        "<li>Choose the match tool <img src=\"" + toolIconDir +
        "/stock_draw-connector-with-arrows.png\" width=22 height=22> "
        "from the toolpad on the right side of the <i>qview</i> main window</li>");
    quickPrep->setWordWrap(true);
    quickLayout->addWidget(quickPrep);

    QLabel *morePrep = new QLabel("<p>Once the Match tool is activated the tool bar at the top "
        "of the main window contains file action buttons and a help button:");
    morePrep->setWordWrap(true);
    quickLayout->addWidget(morePrep);

    QLabel *fileButtons = new QLabel("<p><ul>"
        "<li><img src=\"" + toolIconDir + "/fileopen.png\" width=22 height=22>  Open an existing "
        "control network  <b>Note:</b> If you do not open an existing network, a new one will "
        "be created</li>"
        "<li><img src=\"" + toolIconDir + "/mActionFileSaveAs.png\" width=22 height=22> Save "
        "control network as ...</li>"
        "<li><img src=\"" + toolIconDir + "/mActionFileSave.png\" width=22 height=22> Save "
        "control network to current file</li>"
        "<li><img src=\"" + toolIconDir + "/help-contents.png\" width=22 height=22> Show Help "
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
          "left view window using the \"Blink Start\" button <img src=\"" + toolIconDir +
          "/blinkStart.png\" width=22 height=22> and \"Blink Stop\" button <img src=\"" +
          toolIconDir + "/blinkStop.png\" width=22 height=22>.  The arrow keys above the left "
          "and right views and the keyboard arrow keys may be used to move the both views while "
          "blinking.</li>"
        "<li><strong>Register:</strong>  Sub-pixel register the right view to "
              "the left view. A default registration template is used for setting parameters "
              "passed to the sub-pixel registration tool.  The user may load in a predefined "
              "template or edit the current loaded template to influence successful "
              "co-registration results.  For more information regarding the pattern matching "
              "functionlity or how to create a parameter template, refer to the Isis PatternMatch "
              "document and the <i>autoregtemplate</i> application.</li>"
        "<li><strong>Save Measures:</strong>  Save the two control measures using the sample, "
              "line positions under the crosshairs.</li>"
        "<li><strong>Save Point:</strong>  Save the control point to the control network.</li>"
        "</ul>");
    controlPointEditing->setWordWrap(true);
    controlPointLayout->addWidget(controlPointEditing);

    controlPointTab->setWidget(controlPointContainer);

    tabArea->addTab(overviewTab, "&Overview");
    tabArea->addTab(quickTab, "&Quick Start");
    tabArea->addTab(controlPointTab, "&Control Point Editing");

    QHBoxLayout *buttonsLayout = new QHBoxLayout;
    // Flush the buttons to the right
    buttonsLayout->addStretch();

    QPushButton *closeButton = new QPushButton("&Close");
    closeButton->setIcon(QIcon(FileName("$base/icons/guiStop.png").expanded()));
    closeButton->setDefault(true);
    connect(closeButton, SIGNAL(clicked()),
            helpDialog, SLOT(close()));
    buttonsLayout->addWidget(closeButton);

    mainLayout->addLayout(buttonsLayout);

    helpDialog->show();
  }
#endif
}
