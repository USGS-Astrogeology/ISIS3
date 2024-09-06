/** This is free and unencumbered software released into the public domain.

The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */

#include "QtieTool.h"

#include <QApplication>
#include <QCheckBox>
#include <QDebug>
#include <QDialog>
#include <QFileDialog>
#include <QHBoxLayout>
#include <QInputDialog>
#include <QLineEdit>
#include <QList>
#include <QMenuBar>
#include <QMenu>
#include <QMessageBox>
#include <QPoint>
#include <QRadioButton>
#include <QSizePolicy>
#include <QSpinBox>
#include <QStringList>
#include <QTabWidget>
#include <QTimer>
#include <QToolButton>
#include <QVBoxLayout>
#include <QtWidgets>

#include "Application.h"
#include "AutoReg.h"
#include "AutoRegFactory.h"
#include "BundleAdjust.h"
#include "BundleObservationSolveSettings.h"
#include "BundleResults.h"
#include "BundleSettings.h"
#include "ControlMeasure.h"
#include "Distance.h"
#include "FileName.h"
#include "History.h"
#include "IString.h"
#include "iTime.h"
#include "Latitude.h"
#include "Longitude.h"
#include "MainWindow.h"
#include "MdiCubeViewport.h"
#include "PvlEditDialog.h"
#include "PvlObject.h"
#include "QIsisApplication.h"
#include "SerialNumber.h"
#include "Target.h"
#include "ToolPad.h"
//#include "TProjection.h"
#include "UniversalGroundMap.h"


using namespace std;


namespace Isis {
  // initialize static
  QString QtieTool::lastPtIdValue = "";

  /**
   * Construct the QtieTool
   *
   * @author 2008-11-21 Tracie Sucharski
   *
   * @history  2010-05-11 Tracie Sucharski - Moved the creation of control net
   *                           to the QtieFileTool::open.
   */
  QtieTool::QtieTool(QWidget *parent) : Tool(parent) {

#if 0
    p_createPoint = new QAction(parent);
    p_createPoint->setShortcut(Qt::Key_C);
    connect(p_createPoint, SIGNAL(triggered()), this, SLOT(createPoint()));

    p_modifyPoint = new QAction(parent);
    p_modifyPoint->setShortcut(Qt::Key_M);
//    connect(p_modifyPoint,SIGNAL(triggered()),this,SLOT(modifyPoint()));

    p_deletePoint = new QAction(parent);
    p_deletePoint->setShortcut(Qt::Key_D);
    connect(p_deletePoint, SIGNAL(triggered()), this, SLOT(deletePoint()));
#endif
    p_serialNumberList = new SerialNumberList(false);
    p_controlNet = NULL;
    p_controlPoint = NULL;

    p_twist = true;
    p_sigma0 = 1.0e-10;
    p_maxIterations = 10;
    p_ptIdIndex = 0;
    createQtieTool(parent);

  }


  /**
   * Design the QtieTool widget
   *
   */
  void QtieTool::createQtieTool(QWidget *parent) {

    // Create dialog with a main window
    p_tieTool = new QMainWindow(parent);
    p_tieTool->setWindowTitle("Tie Point Tool");
    p_tieTool->layout()->setSizeConstraint(QLayout::SetFixedSize);

    createMenus();
    createToolBars();

    // Place everything in a grid
    QGridLayout *gridLayout = new QGridLayout();
    //gridLayout->setSizeConstraint(QLayout::SetFixedSize);
    //  ???  Very tacky-hardcoded to ChipViewport size of ControlPointEdit + xtra.
    //       Is there a better way to do this?
    gridLayout->setColumnMinimumWidth(0, 310);
    gridLayout->setColumnMinimumWidth(1, 310);
    //  grid row
    int row = 0;

    QCheckBox *twist = new QCheckBox("Twist");
    twist->setToolTip("Solve for twist");
    twist->setStatusTip("Solving for twist includes a rotation in addition "
                        "to a translation.");
    twist->setWhatsThis("Turning off twist will solve for right ascension and "
               "declinatiuon only which is a translation of the image.  "
               "Solving for twist inclues both translation and rotation.");
    twist->setChecked(p_twist);
    connect(twist, SIGNAL(toggled(bool)), this, SLOT(setTwist(bool)));
    QLabel *iterationLabel = new QLabel("Maximum Iterations");
    QSpinBox *iteration = new QSpinBox();
    iteration->setRange(1, 100);
    iteration->setValue(p_maxIterations);
    iteration->setToolTip("Maximum number of iterations.");
    iteration->setWhatsThis("Maximum number of iterations to try for "
                    "convergence to tolerance before stopping.");
    iterationLabel->setBuddy(iteration);
    connect(iteration, SIGNAL(valueChanged(int)), this, SLOT(setIterations(int)));
    QHBoxLayout *itLayout = new QHBoxLayout();
    itLayout->addWidget(iterationLabel);
    itLayout->addWidget(iteration);


    QLabel *tolLabel = new QLabel("Sigma0");
    p_tolValue = new QLineEdit();
    p_tolValue->setToolTip("Sigma0 used for convergence tolerance.");
    p_tolValue->setWhatsThis("Sigma0 is the standard deviation of unit weight."
                          "  Solution converges on stabilization.");
    tolLabel->setBuddy(p_tolValue);
    QHBoxLayout *tolLayout = new QHBoxLayout();
    tolLayout->addWidget(tolLabel);
    tolLayout->addWidget(p_tolValue);
//    QDoubleValidator *dval = new QDoubleValidator
    QString tolStr;
    tolStr.setNum(p_sigma0);
    p_tolValue->setText(tolStr);
//    connect(p_tolValue,SIGNAL(valueChanged()),this,SLOT(setSigma0()));

    QHBoxLayout *options = new QHBoxLayout();
    options->addWidget(twist);
    options->addLayout(itLayout);
    options->addLayout(tolLayout);

    gridLayout->addLayout(options, row++, 0);

    p_ptIdValue = new QLabel();
    gridLayout->addWidget(p_ptIdValue, row++, 0);

    p_pointEditor = new ControlPointEdit(NULL, parent, true);
    gridLayout->addWidget(p_pointEditor, row++, 0, 1, 3);
    connect(p_pointEditor, SIGNAL(measureSaved()), this, SLOT(measureSaved()));
    p_pointEditor->show();
    connect(this,
            SIGNAL(stretchChipViewport(Stretch *, CubeViewport *)),
            p_pointEditor,
            SIGNAL(stretchChipViewport(Stretch *, CubeViewport *)));


    QPushButton *solve = new QPushButton("Solve");
    solve->setToolTip("Start the iterative least-squares bundle adjustment.");
    solve->setWhatsThis("Start the iterative least-squares bundle adjustment.  "
                "Right ascension (angle 1) and declination (angle 2) which "
                "are stored in the cube labels are adjusted to align the "
                "coordinate of each sample/line of the control points from "
                "the \"Match\" level 1 cube with the latitude/longitude from "
                "the \"Base\" map projected cube.  To solve for all three "
                "camera angles, select the <strong>Twist</strong> check box.");
    connect(solve, SIGNAL(clicked()), this, SLOT(solve()));
    gridLayout->addWidget(solve, row++, 0);

    QWidget *cw = new QWidget();
    cw->setLayout(gridLayout);
    p_tieTool->setCentralWidget(cw);

    connect(this, SIGNAL(editPointChanged()), this, SLOT(drawMeasuresOnViewports()));
    connect(this, SIGNAL(newSolution(Table *)),
            this, SLOT(writeNewCmatrix(Table *)));

  }


  /**
   * Create the menus for QtieTool
   *
   * @internal
   * @history 2011-10-06 Tracie Sucharski - Added Help and What's This
   *
   */
  void QtieTool::createMenus() {

    p_saveNet = new QAction(QIcon(FileName("$ISISROOT/appdata/images/icons/mActionFileSaveAs.png").expanded()),
                            "Save Control Network &As...",
                            p_tieTool);
    p_saveNet->setToolTip("Save current control network to chosen file");
    p_saveNet->setStatusTip("Save current control network to chosen file");
    QString whatsThis = "<b>Function:</b> Saves the current <i>"
                        "control network</i> under chosen filename";
    p_saveNet->setWhatsThis(whatsThis);
    connect(p_saveNet, SIGNAL(triggered()), this, SLOT(saveNet()));

    QAction *closeQtieTool = new QAction(p_tieTool);
    closeQtieTool->setText("&Close");
    closeQtieTool->setShortcut(Qt::ALT + Qt::Key_F4);
    whatsThis = "<b>Function:</b> Closes the Qtie Tool window for this point \
                <p><b>Shortcut:</b> Alt+F4 </p>";
    closeQtieTool->setWhatsThis(whatsThis);
    connect(closeQtieTool, SIGNAL(triggered()), p_tieTool, SLOT(close()));

    QMenu *fileMenu = p_tieTool->menuBar()->addMenu("&File");
    fileMenu->addAction(p_saveNet);
    fileMenu->addAction(closeQtieTool);

    QAction *templateFile = new QAction(p_tieTool);
    templateFile->setText("&Set registration template");
    whatsThis =
      "<b>Function:</b> Allows user to select a new file to set as the registration template";
    templateFile->setWhatsThis(whatsThis);
    connect(templateFile, SIGNAL(triggered()), this, SLOT(setTemplateFile()));

    QAction *viewTemplate = new QAction(p_tieTool);
    viewTemplate->setText("&View/edit registration template");
    whatsThis = "<b>Function:</b> Displays the curent registration template.  \
                The user may edit and save changes under a chosen filename.";
    viewTemplate->setWhatsThis(whatsThis);
    connect(viewTemplate, SIGNAL(triggered()), this, SLOT(viewTemplateFile()));


    QMenu *optionMenu = p_tieTool->menuBar()->addMenu("&Options");
    QMenu *regMenu = optionMenu->addMenu("&Registration");

    regMenu->addAction(templateFile);
    regMenu->addAction(viewTemplate);
    //    registrationMenu->addAction(interestOp);

    p_whatsThis = new QAction(QIcon(FileName("$ISISROOT/appdata/images/icons/contexthelp.png").expanded()),
                              "&Whats's This",
                              p_tieTool);
    p_whatsThis->setShortcut(Qt::SHIFT | Qt::Key_F1);
    p_whatsThis->setToolTip("Activate What's This and click on items on "
                            "user interface to see more information.");
    connect(p_whatsThis, SIGNAL(triggered()), this, SLOT(enterWhatsThisMode()));

    QMenu *helpMenu = p_tieTool->menuBar()->addMenu("&Help");
    helpMenu->addAction(p_whatsThis);
  }


  void QtieTool::createToolBars() {

    QToolBar * toolBar = new QToolBar;
    toolBar->setFloatable(false);
    toolBar->addAction(p_saveNet);
    toolBar->addSeparator();
    toolBar->addAction(p_whatsThis);

    p_tieTool->addToolBar(Qt::TopToolBarArea, toolBar);
  }



  /**
   * Put the QtieTool icon on the main window Toolpad
   *
   * @param pad   Input  Toolpad for the main window
   *
   */
  QAction *QtieTool::toolPadAction(ToolPad *pad) {
    QAction *action = new QAction(pad);
    action->setIcon(QPixmap(toolIconDir() + "/stock_draw-connector-with-arrows.png"));
    action->setToolTip("Tie (T)");
    action->setShortcut(Qt::Key_T);
    return action;
  }


  /**
   * Setup the base map and match cube
   *
   * @param baseCube   Input  Base map cube
   * @param matchCube  Input  Match cube
   *
   */
  void QtieTool::setFiles(Cube *baseCube, Cube *matchCube, ControlNet *cnet) {
    //  Save off base map cube, but add matchCube to serial number list
    p_baseCube = baseCube;
    p_matchCube = matchCube;
    p_controlNet = cnet;
    p_baseSN = SerialNumber::Compose(*p_baseCube, true);
    p_matchSN = SerialNumber::Compose(*p_matchCube);

    p_serialNumberList->add(matchCube->fileName());

    //  Save off universal ground maps
    try {
      p_baseGM = new UniversalGroundMap(*p_baseCube);
    }
    catch (IException &e) {
      std::string message = "Cannot initialize universal ground map for basemap.\n";
      std::string errors = e.toString();
      message += errors;
      QMessageBox::critical((QWidget *)parent(), "Error", message);
      return;
    }
    try {
      p_matchGM = new UniversalGroundMap(*matchCube);
    }
    catch (IException &e) {
      std::string message = "Cannot initialize universal ground map for match cube.\n";
      std::string errors = e.toString();
      message += errors;
      QMessageBox::critical((QWidget *)parent(), "Error", message);
      return;
    }

  }


  /**
   * New files selected, clean up old file info
   *
   * @internal
   * @history  2007-06-12 Tracie Sucharski - Added method to allow user to
   *                           run on new files.
   * @history  2010-05-11 Tracie Sucharski - Moved the creation of control net
   *                           to the QtieFileTool::open.
   * @history  2012-05-11 Tracie Sucharski - Delete cubes,  A better solution- rewrite
   *                           QtieFileTool::open , to pass in filenames only?
   *
   */
  void QtieTool::clearFiles() {
    p_tieTool->setVisible(false);

    delete p_serialNumberList;
    p_serialNumberList = NULL;
    p_serialNumberList = new SerialNumberList(false);

    delete p_controlNet;
    p_controlNet = NULL;

    delete p_baseGM;
    p_baseGM = NULL;
    delete p_matchGM;
    p_matchGM = NULL;

    delete p_baseCube;
    p_baseCube = NULL;
    delete p_matchCube;
    p_matchCube = NULL;

    p_controlPoint = NULL;
  }


  /**
   * Save control measures under crosshairs of ChipViewports
   *
   */
  void QtieTool::measureSaved() {
    //  Get sample/line from base map and find lat/lon
    double samp = p_controlPoint->GetMeasure(Base)->GetSample();
    double line = p_controlPoint->GetMeasure(Base)->GetLine();
    p_baseGM->SetImage(samp, line);
    double lat = p_baseGM->UniversalLatitude();
    double lon = p_baseGM->UniversalLongitude();

    //  TODO:  Do not know if radius came from DEM (if cube spiceinit'ed
    //   with DEM) or from ellipsoid.  Once change made to camera to return
    //   DEM filename, update the point aprioiRadiusSource parameters.
    p_matchGM->SetGround(Latitude(lat, Angle::Degrees), Longitude(lon, Angle::Degrees));
    Distance radius = p_matchGM->Camera()->LocalRadius();
    if (!radius.isValid()) {
      std::string message = "Could not determine radius from DEM at lat/lon ["
          + QString::number(lat) + "," + QString::number(lon) + "]";
      QMessageBox::critical((QWidget *)parent(),"Error",message);
      return;
    }
    try {
      p_controlPoint->SetAprioriSurfacePoint(SurfacePoint(
                Latitude(lat, Angle::Degrees), Longitude(lon, Angle::Degrees),
                radius));
    }
    catch (IException &e) {
      std::string message = "Unable to set Apriori Surface Point.\n";
      message += "Latitude = " + QString::number(lat);
      message += "  Longitude = " + QString::number(lon);
      message += "  Radius = " + QString::number(radius.meters()) + "\n";
      message += e.toString();
      QMessageBox::critical((QWidget *)parent(),"Error",message);
    }

    emit editPointChanged();
  }


  /**
   * Handle mouse events on match CubeViewport
   *
   * @param p[in]   (QPoint)            Point under cursor in cubeviewport
   * @param s[in]   (Qt::MouseButton)   Which mouse button was pressed
   *
   * @internal
   * @history  2007-06-12 Tracie Sucharski - Swapped left and right mouse
   *                           button actions.
   * @history  2008-11-19 Tracie Sucharski - Only allow mouse events on
   *                           match cube.
   * @history 2013-05-09 Tracie Sucharski - When deleting (right button) a point, check for empty
   *                          network immediately print warning and return.
   *
   */
  void QtieTool::mouseButtonRelease(QPoint p, Qt::MouseButton s) {
    MdiCubeViewport *cvp = cubeViewport();
    if (cvp  == NULL)
      return;
    if (cubeViewportList()->size() != 2) {
      std::string message = "You must have a basemap and a match cube open.";
      QMessageBox::critical((QWidget *)parent(), "Error", message);
      return;
    }
    if (cvp->cube() == p_baseCube) {
      std::string message = "Select points on match Cube only.";
      QMessageBox::information((QWidget *)parent(), "Warning", message);
      return;
    }

    // ???  do we only allow mouse clicks on level1???
    //    If we allow on both, need to find samp,line on level1 if
    //    they clicked on basemap.
    QString file = cvp->cube()->fileName();
    QString sn = p_serialNumberList->serialNumber(file);

    double samp, line;
    cvp->viewportToCube(p.x(), p.y(), samp, line);

    if (s == Qt::LeftButton) {
      if (!p_controlNet || p_controlNet->GetNumMeasures() == 0) {
        std::string message = "No points exist for editing.  Create points ";
        message += "using the right mouse button.";
        QMessageBox::information((QWidget *)parent(), "Warning", message);
        return;
      }
      //  Find closest control point in network
      ControlPoint *point;
      try {
        point = p_controlNet->FindClosest(sn, samp, line);
      }
      catch (IException &e) {
        std::string message = "No points found for editing.  Create points ";
        message += "using the right mouse button.";
        message += e.toString();
        QMessageBox::critical((QWidget *)parent(), "Error", message);
        return;
      }
      modifyPoint(point);
    }
    else if (s == Qt::MiddleButton) {
      if (!p_controlNet || p_controlNet->GetNumPoints() == 0) {
        std::string message = "No points exist for deleting.  Create points ";
        message += "using the right mouse button.";
        QMessageBox::warning((QWidget *)parent(), "Warning", message);
        return;
      }

      //  Find closest control point in network
      ControlPoint *point =
        p_controlNet->FindClosest(sn, samp, line);
      //  TODO:  test for errors and reality
      if (point == NULL) {
        std::string message = "No points exist for deleting.  Create points ";
        message += "using the right mouse button.";
        QMessageBox::information((QWidget *)parent(), "Warning", message);
        return;
      }
      deletePoint(point);
    }
    else if (s == Qt::RightButton) {
      p_matchGM->SetImage(samp, line);
      double lat = p_matchGM->UniversalLatitude();
      double lon = p_matchGM->UniversalLongitude();

      createPoint(lat, lon);
    }
  }


  /**
   * Create control point at given lat,lon
   *
   * @param lat   Input  Latitude of new point
   * @param lon   Input  Longitude of new point
   *
   * @internal
   * @history  2008-12-06 Tracie Sucharski - Set point type to Ground
   * @history  2010-05-18 Jeannie Walldren - Modified Point ID
   *                          QInputDialog to return if "Cancel"
   *                          is clicked.
   * @history 2012-05-10  Tracie Sucharski - If point doesn't exist on
   *                          base map, return.
   */
  void QtieTool::createPoint(double lat, double lon) {

    //  TODO:   ADD AUTOSEED OPTION (CHECKBOX?)

    double baseSamp = 0, baseLine = 0;
    double matchSamp, matchLine;

    //  if clicked in match, get samp,line
    p_matchGM->SetUniversalGround(lat, lon);
    matchSamp = p_matchGM->Sample();
    matchLine = p_matchGM->Line();

    //  Make sure point is on base
    if (p_baseGM->SetUniversalGround(lat, lon)) {
      //  Make sure point on base cube
      baseSamp = p_baseGM->Sample();
      baseLine = p_baseGM->Line();
      if (baseSamp < 1 || baseSamp > p_baseCube->sampleCount() ||
          baseLine < 1 || baseLine > p_baseCube->lineCount()) {
        // throw error? point not on base
        std::string message = "Point does not exist on base map.";
        QMessageBox::warning((QWidget *)parent(), "Warning", message);
        return;
      }
    }
    else {
      //  throw error?  point not on base cube
      // throw error? point not on base
      std::string message = "Point does not exist on base map.";
      QMessageBox::warning((QWidget *)parent(), "Warning", message);
    }

    //  Get radius
//    double radius = p_baseGM->Projection()->LocalRadius();

    //  Point is on both base and match, create new control point
    ControlPoint *newPoint = NULL;
    // First prompt for pointId
    bool goodId = false;
    while (!goodId) {
      bool ok = false;
      QString id = QInputDialog::getText((QWidget *)parent(),
                                         "Point ID", "Enter Point ID:",
                                         QLineEdit::Normal, lastPtIdValue,
                                         &ok);
      if (!ok)
        // user clicked "Cancel"
      {
        return;
      }
      if (ok && id.isEmpty())
        // user clicked "Ok" but did not enter a point ID
      {
        std::string message = "You must enter a point Id.";
        QMessageBox::warning((QWidget *)parent(), "Warning", message);
      }
      else {
        // Make sure Id doesn't already exist
        newPoint = new ControlPoint(id);
        if (p_controlNet->ContainsPoint(newPoint->GetId())) {
          std::string message = "A ControlPoint with Point Id = [" +
                            newPoint->GetId() +
                            "] already exists.  Re-enter unique Point Id.";
          QMessageBox::warning((QWidget *)parent(), "Unique Point Id", message);
        }
        else {
          goodId = true;
          lastPtIdValue = id;
        }
      }
    }

    newPoint->SetType(ControlPoint::Fixed);

    // Set first measure to match
    ControlMeasure *mM = new ControlMeasure;
    mM->SetCubeSerialNumber(p_matchSN);
    mM->SetCoordinate(matchSamp, matchLine);
    mM->SetType(ControlMeasure::Manual);
    mM->SetDateTime();
    mM->SetChooserName(Application::UserName());
    newPoint->Add(mM);
    //  Second measure is base measure, set to Ignore=yes
    ControlMeasure *mB = new ControlMeasure;
    mB->SetCubeSerialNumber(p_baseSN);
    mB->SetCoordinate(baseSamp, baseLine);
    mB->SetType(ControlMeasure::Manual);
    mB->SetDateTime();
    mB->SetChooserName(Application::UserName());
    mB->SetIgnored(true);
    newPoint->Add(mB);

    //  Add new control point to control network
    p_controlNet->AddPoint(newPoint);

    //  Read newly added point
    p_controlPoint = p_controlNet->GetPoint((QString) newPoint->GetId());
    //  Load new point in QtieTool
    loadPoint();
    p_tieTool->setVisible(true);
    p_tieTool->raise();

    emit editPointChanged();
    //  Call measure Saved to get the initial Apriori values
    measureSaved();

  }


  /**
   * Delete given control point
   *
   * @param point Input  Control Point to delete
   *
   * @history 2010-05-19 Tracie Sucharski - Fixed bug which was causing a seg
   *                        fault.  Set p_controlPoint to NULL, also no sense
   *                        loading point to be deleted.  Should this be
   *                        smartened up to load another Point?
   *
   */
  void QtieTool::deletePoint(ControlPoint *point) {

    p_controlPoint = point;
    //  Change point in viewport to red so user can see what point they are
    //  about to delete.
    emit editPointChanged();

    //loadPoint();

    p_controlNet->DeletePoint(p_controlPoint->GetId());
    p_tieTool->setVisible(false);
    p_controlPoint = NULL;

    emit editPointChanged();
  }


  /**
   * Modify given control point
   *
   * @param point Input  Control Point to modify
   *
   */
  void QtieTool::modifyPoint(ControlPoint *point) {

    p_controlPoint = point;
    loadPoint();
    p_tieTool->setVisible(true);
    p_tieTool->raise();
    emit editPointChanged();
  }


  /**
   * Load control point into the ControlPointEdit widget
   *
   * @history 2010-05-18  Tracie Sucharski - Added pointId to the dialog.
   */
  void QtieTool::loadPoint() {

    //  Initialize pointEditor with measures
    p_pointEditor->setLeftMeasure(p_controlPoint->GetMeasure(Base), p_baseCube,
                                  p_controlPoint->GetId());
    p_pointEditor->setRightMeasure(p_controlPoint->GetMeasure(Match),
                                   p_matchCube, p_controlPoint->GetId());

    //  Write pointId
    QString ptId = "Point ID:  " + p_controlPoint->GetId();
    p_ptIdValue->setText(ptId);
  }


  /**
   * Draw all Control Measures on each viewport
   *
   */
  void QtieTool::drawMeasuresOnViewports() {

    MdiCubeViewport *vp;
    for (int i = 0; i < (int)cubeViewportList()->size(); i++) {
      vp = (*(cubeViewportList()))[i];
      vp->viewport()->update();
    }
  }


  /**
   * Repaint the given CubeViewport
   *
   * @param vp       Input  CubeViewport to repain
   * @param painter  Input  Qt Painter
   *
   */
  void QtieTool::paintViewport(MdiCubeViewport *vp, QPainter *painter) {

    //  Make sure we have points to draw
    if (p_controlNet == NULL || p_controlNet->GetNumPoints() == 0)
      return;

    //  Draw all measures
    QString serialNumber = SerialNumber::Compose(*vp->cube(), true);
    for (int i = 0; i < p_controlNet->GetNumPoints(); i++) {
      ControlPoint &p = *p_controlNet->GetPoint(i);
      if (p_controlPoint != NULL && p.GetId() == p_controlPoint->GetId()) {
        painter->setPen(QColor(200, 0, 0));
      }
      else {
        painter->setPen(QColor(0, 200, 0));
      }

      double samp, line;
      if (vp->cube()->fileName() == p_baseCube->fileName()) {
        // Draw on left viewport (base)
        samp = p[Base]->GetSample();
        line = p[Base]->GetLine();
      }
      else {
        // Draw on right viewport (match)
        samp = p[Match]->GetSample();
        line = p[Match]->GetLine();
      }
      int x, y;
      vp->cubeToViewport(samp, line, x, y);
      painter->drawLine(x - 5, y, x + 5, y);
      painter->drawLine(x, y - 5, x, y + 5);
    }

  }


  /**
   * Perform the BundleAdjust Solve
   *
   * @history 2011-04-11 Debbie A. Cook - Removed obsolete argument from
   *          BundleAdjust method Solve
   */
  void QtieTool::solve() {

    //  First off , get sigma0,  NEED to VALIDATE
    p_sigma0 = p_tolValue->text().toDouble();

    //  Need at least 2 points to solve for twist
    if (p_twist) {
      if (p_controlNet->GetNumPoints() < 2) {
        std::string message = "Need at least 2 points to solve for twist. \n";
        QMessageBox::critical((QWidget *)parent(), "Error", message);
        return;
      }
    }

    //  Create temporary networks for solution which will not contain measures for
    //  the basemap.
    ControlNet inNet;
    ControlNet outNet;
    inNet.SetTarget(*p_matchCube->label());

    // Bundle adjust to solve for new pointing
    try {
      //  Create new control net for bundle adjust , deleting ignored measures
      for (int p = 0; p < p_controlNet->GetNumPoints(); p++) {
        ControlPoint *pt = new ControlPoint(*p_controlNet->GetPoint(p));
        pt->Delete(p_baseSN);
        inNet.AddPoint(pt);
      }


      // =========================================================================================//
      // ============= Use the bundle settings to initialize member variables ====================//
      // =========================================================================================//
      BundleSettingsQsp settings = BundleSettingsQsp(new BundleSettings);
      settings->setValidateNetwork(false);
      // set the following:
      //     solve observation mode = false
      //     update cube label      = false
      //     error propagation      = false
      //     solve radius           = false
      //     latitude sigma         = 1000.0
      //     longitude sigma        = 1000.0
      //     radius sigma           = Null since we are not solving for radius
      //     outlier rejection      = false
      settings->setSolveOptions(false, false, false, false, SurfacePoint::Latitudinal,
                                SurfacePoint::Latitudinal, 1000.0,1000.0, Isis::Null);
  //************************************************************************************************
      QList<BundleObservationSolveSettings> observationSolveSettingsList;
      BundleObservationSolveSettings observationSolveSettings;

      //********************************************************************************************
      // use defaults
      //       pointing option sigmas -1.0
      //       ckDegree = ckSolveDegree = 2
      //       fitOverExisting = false
      //       angle sigma = angular velocity sigma = angular acceleration sigma = -1.0
      observationSolveSettings.setInstrumentPointingSettings(
          BundleObservationSolveSettings::AnglesOnly, p_twist);

      // NOTE: no need to set position sigmas or solve degrees since we are not solving for any
      // position factors
      //       position option sigmas default to -1.0
      //       spkDegree = spkSolveDegree = 2
      //       solveOverHermiteSpline = false
      //       position sigma = velocity sigma = acceleration sigma = -1.0
      observationSolveSettings.setInstrumentPositionSettings(
          BundleObservationSolveSettings::NoPositionFactors);

      observationSolveSettingsList.append(observationSolveSettings);

      settings->setObservationSolveOptions(observationSolveSettingsList);

      settings->setConvergenceCriteria(BundleSettings::ParameterCorrections,
                                      p_sigma0, p_maxIterations);
      settings->setOutputFilePrefix("");
      // =========================================================================================//
      // =============== End Bundle Settings =====================================================//
      // =========================================================================================//

      BundleAdjust bundleAdjust(settings, inNet, *p_serialNumberList, false);
      QObject::connect( &bundleAdjust, SIGNAL( statusUpdate(QString) ),
                        &bundleAdjust, SLOT( outputBundleStatus(QString) ) );

      bundleAdjust.solveCholesky();
      // bundleAdjust.solveCholeskyBR();

      // Print results and give user option of updating cube pointin
      outNet = *bundleAdjust.controlNet().data();
      double maxError = outNet.GetMaximumResidual();
      double avgError = outNet.AverageResidual();

      std::string message = "Maximum Error = " + QString::number(maxError);
      message += "\nAverage Error = " + QString::number(avgError);
      std::string msgTitle = "Update camera pointing?";
//      QDialog resultsDialog((QWidget *)parent(),"Bundle Adjust Summary",true);

      QMessageBox msgBox;
      msgBox.setIcon(QMessageBox::Question);
      msgBox.setWindowTitle(msgTitle);
      msgBox.setText(message);
      QPushButton *update = msgBox.addButton("Update", QMessageBox::AcceptRole);
      update->setToolTip("Update camera pointing on \"Match\" cube labels.");
      update->setWhatsThis("Update the camera angles on the \"Match\" cube "
                           "labels.  The right ascension, declination  and "
                           "twist (if the <strong>Twist option</strong was "
                           "chosen).");
      QPushButton *close = msgBox.addButton("Close", QMessageBox::RejectRole);
      close->setToolTip("Do not update camera pointing.");
      close->setWhatsThis("If you are not happy with the solution, select "
                          "this.  The camera pointing will not be updated.  "
                          "You can attempt to refine the control points and "
                          "attempt a new solution.");
      msgBox.setDetailedText(bundleAdjust.iterationSummaryGroup());
      msgBox.setDefaultButton(close);
      msgBox.setMinimumWidth(5000);
      //msgBox.setSizeGripEnabled(true);
      msgBox.exec();
      if (msgBox.clickedButton() == update) {
        p_matchCube->reopen("rw");
        Table cmatrix = bundleAdjust.cMatrix(0);
        //cmatrix = b.Cmatrix(0);
        emit newSolution(&cmatrix);
      }
      else {
        return;
      }

    }
    catch (IException &e) {
      std::string message = "Unable to bundle adjust. Solution failed.\n";
      std::string errors = e.toString();
      message += errors;
//      message += "\n\nMaximum Error = " + QString::number(outNet.MaximumResiudal());
//      message += "\nAverage Error = " + QString::number(outNet.AverageResidual());
      message += "\n\nMaximum Error = "
               + QString::number(outNet.GetMaximumResidual());
      message += "\nAverage Error = "
               + QString::number(outNet.AverageResidual());
      QMessageBox::warning((QWidget *)parent(), "Error", message);
      return;
    }
  }


  /**
   * Write the new cmatrix to the match cube
   *
   * @param cmatrix    Input  New adjusted cmatrix
   *
   */
  void QtieTool::writeNewCmatrix(Table *cmatrix) {

    //check for existing polygon, if exists delete it
    if (p_matchCube->label()->hasObject("Polygon")) {
      p_matchCube->label()->deleteObject("Polygon");
    }

    // Update the cube history
    p_matchCube->write(*cmatrix);
    History h = p_matchCube->readHistory();
    PvlObject history("qtie");
    history += PvlKeyword("IsisVersion", Application::Version().toStdString());
    QString path = QCoreApplication::applicationDirPath();
    history += PvlKeyword("ProgramPath", path.toStdString());
    history += PvlKeyword("ExecutionDateTime",
                                Application::DateTime().toStdString());
    history += PvlKeyword("HostName", Application::HostName().toStdString());
    history += PvlKeyword("UserName", Application::UserName().toStdString());
    PvlGroup results("Results");
    results += PvlKeyword("CameraAnglesUpdated", "True");
    results += PvlKeyword("BaseMap", p_baseCube->fileName().toStdString());
    history += results;

    h.AddEntry(history);
    p_matchCube->write(h);
    p_matchCube->reopen("r");

  }


  /**
   * Allows user to set a new template file.
   * @author 2008-12-10 Jeannie Walldren
   * @internal
   *   @history 2008-12-10 Jeannie Walldren - Original Version
   */

  void QtieTool::setTemplateFile() {

    QString filename = QFileDialog::getOpenFileName(p_tieTool,
                       "Select a registration template", ".",
                       "Registration template files (*.def *.pvl);;All files (*)");

    if (filename.isEmpty())
      return;

    p_pointEditor->setTemplateFile(filename);
  }


  /**
   * Allows the user to view the template file that is currently
   * set.
   * @author 2008-12-10 Jeannie Walldren
   * @internal
   *   @history 2008-12-10 Jeannie Walldren - Original Version
   *   @history 2008-12-10 Jeannie Walldren - Added ""
   *            namespace to PvlEditDialog reference and changed
   *            registrationDialog from pointer to object
   *   @history 2008-12-15 Jeannie Walldren - Added QMessageBox
   *            warning in case Template File cannot be read.
   */
  void QtieTool::viewTemplateFile() {
    try {
      // Get the template file from the ControlPointEditor object
      Pvl templatePvl(p_pointEditor->templateFileName().toStdString());
      // Create registration dialog window using PvlEditDialog class
      // to view and/or edit the template
      PvlEditDialog registrationDialog(templatePvl);
      registrationDialog.setWindowTitle("View or Edit Template File: "
                                        + QString::fromStdString(templatePvl.fileName()));
      registrationDialog.resize(550, 360);
      registrationDialog.exec();
    }
    catch (IException &e) {
      std::string message = e.toString();
      QMessageBox::warning((QWidget *)parent(), "Error", message);
    }
  }


  /**
   * Save the ground points to a ControlNet.
   *
   * @author 2008-12-30 Tracie Sucharski
   */
  void QtieTool::saveNet() {

    QString filter = "Control net (*.net);;";
    filter += "Text file (*.txt);;";
    filter += "All (*)";
    QString fn = QFileDialog::getSaveFileName((QWidget *)parent(),
                 "Choose filename to save under",
                 ".", filter);
    if (!fn.isEmpty()) {
      try {
        //  Create new control net for bundle adjust , deleting ignored measures
        // which are the basemap measures.
        ControlNet net;
        for (int p = 0; p < p_controlNet->GetNumPoints(); p++) {
          ControlPoint *pt = new ControlPoint(*p_controlNet->GetPoint(p));
          for (int m = 0; m < pt->GetNumMeasures(); m++) {
            if (pt->GetMeasure(m)->IsIgnored())
              pt->Delete(m);
          }
          net.SetTarget(p_matchCube->camera()->target()->name());
          net.SetNetworkId("Qtie");
          net.SetUserName(Application::UserName());
          net.SetCreatedDate(Application::DateTime());
          net.SetModifiedDate(iTime::CurrentLocalTime());
          net.SetDescription("Qtie Ground Points");
          net.AddPoint(pt);
        }

        net.Write(fn);
      }
      catch (IException &e) {
        std::string message = "Error saving control network.  \n";
        std::string errors = e.toString();
        message += errors;
        QMessageBox::information((QWidget *)parent(), "Error", message);
        return;
      }
    }
    else {
      QMessageBox::information((QWidget *)parent(),
                               "Error", "Saving Aborted");
    }
  }


  void QtieTool::enterWhatsThisMode() {
    QWhatsThis::enterWhatsThisMode();
  }
}
