#include "StereoTool.h"

#include <iomanip>
#include <cmath>
#include <vector>

#include <QAction>
#include <QCheckBox>
#include <QComboBox>
#include <QFileDialog>
#include <QInputDialog>
#include <QMenuBar>
#include <QMessageBox>
#include <QStackedWidget>

#include "AbstractPlotTool.h"
#include "Application.h"
#include "AutoReg.h"
#include "AutoRegFactory.h"
#include "BundleAdjust.h"
#include "ControlMeasure.h"
#include "ControlNet.h"
#include "ControlPoint.h"
#include "ControlPointEdit.h"
#include "CubePlotCurve.h"
#include "Distance.h"
#include "FileName.h"
#include "History.h"
#include "IException.h"
#include "IString.h"
#include "iTime.h"
#include "Latitude.h"
#include "Longitude.h"
#include "MainWindow.h"
#include "MdiCubeViewport.h"
#include "PlotCurve.h"
#include "PlotWindow.h"
#include "ProfileDialog.h"
#include "PvlEditDialog.h"
#include "PvlObject.h"
#include "Projection.h"
#include "QIsisApplication.h"
#include "RubberBandTool.h"
#include "SerialNumber.h"
#include "Stereo.h"
#include "SurfacePoint.h"
#include "Target.h"
#include "ToolPad.h"
#include "UniversalGroundMap.h"


using namespace std;


namespace Isis {
  // initialize static
/*        !!!!   TODOS   !!!!

1.  If DEM radius comboBox, update to DEM  ,  when?


*/



  QString StereoTool::lastPtIdValue = "";

  /**
   * Construct the StereoTool
   *
   * @author 2011-09-19 Tracie Sucharski
   *
   */
  StereoTool::StereoTool(QWidget *parent) : AbstractPlotTool(parent) {

    m_editPoint = NULL;
    m_startPoint = NULL;
    m_endPoint = NULL;

    m_serialNumberList = NULL;
    m_controlNet = NULL;
    m_leftCube = NULL;
    m_rightCube = NULL;
    m_leftGM = NULL;
    m_rightGM = NULL;

    m_profileDialog = NULL;

    m_showWarning = true;

    m_targetRadius = Distance(0., Distance::Meters);
    m_baseRadius = Distance(0., Distance::Meters);

    createStereoTool(parent);
    connect( this, SIGNAL( toolActivated() ), this, SLOT( activateTool() ) );
  }


  /**
   * Design the StereoTool widget
   *
   */
  void StereoTool::createStereoTool(QWidget *parent) {

    m_stereoTool = new QMainWindow(parent);
    m_stereoTool->setWindowTitle("Elevation Calculator (via stereo pairs)");

    createMenus();

    // Place everything in a grid
    QGridLayout *gridLayout = new QGridLayout();
    //gridLayout->setSizeConstraint(QLayout::SetFixedSize);
    //  ???  Very tacky-hardcoded to ChipViewport size of ControlPointEdit + xtra.
    //       Is there a better way to do this?
    gridLayout->setColumnMinimumWidth(0, 310);
    gridLayout->setColumnMinimumWidth(1, 310);
    //  grid row
    int row = 0;

    m_ptIdValue = new QLabel();
    gridLayout->addWidget(m_ptIdValue, row++, 0);

    m_leftCubeLabel = new QLabel();
    m_rightCubeLabel = new QLabel();
    gridLayout->addWidget(m_leftCubeLabel, row, 0);
    gridLayout->addWidget(m_rightCubeLabel, row++, 1);

    m_pointEditor = new ControlPointEdit(NULL, parent, true);
    gridLayout->addWidget(m_pointEditor, row++, 0, 1, 3);
    connect( m_pointEditor, SIGNAL( measureSaved() ), this, SLOT( measureSaved() ) );
    m_pointEditor->show();
    connect(this,
            SIGNAL( stretchChipViewport(Stretch *, CubeViewport *) ),
            m_pointEditor,
            SIGNAL( stretchChipViewport(Stretch *, CubeViewport *) ) );

    m_elevationLabel = new QLabel;
    m_elevationLabel->setToolTip("Calculated elevation in meters.");
    m_elevationLabel->setWhatsThis("Calculated elevation based on parallax.");
    m_elevationErrorLabel = new QLabel;
    m_elevationErrorLabel->setToolTip(
                        "Error in calculated elevation in meters.");
    m_elevationErrorLabel->setWhatsThis("Error in calculated elevation.");
    gridLayout->addWidget(m_elevationLabel, row, 0);
    gridLayout->addWidget(m_elevationErrorLabel, row++, 1);

    m_baseRadiiLabel = new QLabel;
    m_baseRadiiLabel->setToolTip("Subtracted from the calculated radius to "
                               "determine elevation.");
//    m_baseRadiiLabel->setToolTip("Local Radius");
    m_leftDemRadiiLabel = new QLabel;
    m_leftDemRadiiLabel->setToolTip("Left Cube DEM Radius");
    m_rightDemRadiiLabel = new QLabel;
    m_rightDemRadiiLabel->setToolTip("Right Cube DEM Radius");
    gridLayout->addWidget(m_baseRadiiLabel, row, 0);
    gridLayout->addWidget(m_leftDemRadiiLabel, row, 1);
    gridLayout->addWidget(m_rightDemRadiiLabel, row++, 2);

//  QPushButton *elevationButton = new QPushButton("Calculate Elevation");
//  connect(elevationButton, SIGNAL(clicked() ),
//          this, SLOT(calculateElevation() ));
//  gridLayout->addWidget(elevationButton, row++, 1);

    QWidget *cw = new QWidget();
    cw->setLayout(gridLayout);
    m_stereoTool->setCentralWidget(cw);

    connect( this, SIGNAL( editPointChanged() ), this, SLOT( paintAllViewports() ) );

  }


  /**
   * Create the menus for StereoTool
   *
   *
   */
  void StereoTool::createMenus() {

    m_save = new QAction(m_stereoTool);
    m_save->setText("Save Elevation Data...");
    QString whatsThis =
      "<b>Function:</b> Saves the elevation calulations to current file.";
    m_save->setWhatsThis(whatsThis);
    connect( m_save, SIGNAL( triggered() ), this, SLOT( saveElevations() ) );
    m_save->setDisabled(true);

    QAction *saveAs = new QAction(m_stereoTool);
    saveAs->setText("Save Elevation Data As...");
    whatsThis =
      "<b>Function:</b> Saves the elevation calulations to a file.";
    saveAs->setWhatsThis(whatsThis);
    connect( saveAs, SIGNAL( triggered() ), this, SLOT( saveAsElevations() ) );

    QAction *closeStereoTool = new QAction(m_stereoTool);
    closeStereoTool->setText("&Close");
    closeStereoTool->setShortcut(Qt::ALT + Qt::Key_F4);
    whatsThis =
      "<b>Function:</b> Closes the Stereo Tool window for this point \
       <p><b>Shortcut:</b> Alt+F4 </p>";
    closeStereoTool->setWhatsThis(whatsThis);
    connect( closeStereoTool, SIGNAL( triggered() ), m_stereoTool, SLOT( close() ) );

    QMenu *fileMenu = m_stereoTool->menuBar()->addMenu("&File");
    fileMenu->addAction(m_save);
    fileMenu->addAction(saveAs);
    fileMenu->addAction(closeStereoTool);

    QAction *templateFile = new QAction(m_stereoTool);
    templateFile->setText("&Set registration template");
    whatsThis =
      "<b>Function:</b> Allows user to select a new file to set as the registration template";
    templateFile->setWhatsThis(whatsThis);
    connect( templateFile, SIGNAL( triggered() ), this, SLOT( setTemplateFile() ) );

    QAction *viewTemplate = new QAction(m_stereoTool);
    viewTemplate->setText("&View/edit registration template");
    whatsThis =
      "<b>Function:</b> Displays the curent registration template.  \
       The user may edit and save changes under a chosen filename.";
    viewTemplate->setWhatsThis(whatsThis);
    connect( viewTemplate, SIGNAL( triggered() ), this, SLOT( viewTemplateFile() ) );


    QMenu *optionMenu = m_stereoTool->menuBar()->addMenu("&Options");
    QMenu *regMenu = optionMenu->addMenu("&Registration");

    regMenu->addAction(templateFile);
    regMenu->addAction(viewTemplate);
    //    registrationMenu->addAction(interestOp);

    QAction *showHelpAct = new QAction("stereo tool &Help", m_stereoTool);
    showHelpAct->setIcon(QPixmap(toolIconDir() + "/help-contents.png") );
    connect( showHelpAct, SIGNAL( triggered() ), this, SLOT( showHelp() ));

    QMenu *helpMenu = m_stereoTool->menuBar()->addMenu("&Help");
    helpMenu->addAction(showHelpAct);

  }



  /**
   * Put the StereoTool icon on the main window Toolpad
   *
   * @param pad   Input  Toolpad for the main window
   *
   */

  QAction *StereoTool::toolPadAction(ToolPad *pad) {
    QAction *action = new QAction(pad);
    action->setIcon( QPixmap(toolIconDir() + "/3d-glasses-icon.png") );
    action->setToolTip("Stereo");
    action->setWhatsThis("<strong>Functionality:</strong> "
               "<ul>"
                 "<li>Calculate elevation at a single point by creating a "
                 "control point between the image pair. "
                 "<ul>"
                 "<h4><strong>Control Point mouse Button Functions:</strong></h4>"
                 "<li>Left:  Edit closest point.</li>"
                 "<li>Middle:  Delete closest point.</li>"
                 "<li>Right:  Create new point at cursor position.</li></ul>"
                 "<li>Left click and drag will create an elevation profile "
                 "after you create the start and end control points.  A dialog "
                 "box will be shown to assist.</li>"
                 "<li>Right click and drag will create an elevation profile "
                 "between previously created control points.</li></ul>");
    return action;
  }



  /**
   * Attaches this tool to the toolbar
   *
   * @param parent
   *
   * @return QWidget*
   */
  QWidget *StereoTool::createToolBarWidget(QStackedWidget *parent) {
    QWidget *hbox = new QWidget(parent);

    QLabel *boxLabel = new QLabel("Local Radius:");
    m_radiusBox = new QComboBox(hbox);
    m_radiusBox->addItem("Ellipsoid Equitorial Radius");
    m_radiusBox->addItem("DEM Radius");
    m_radiusBox->addItem("Custom Radius");
    m_radiusBox->setToolTip("Source for local radius");
    QString text =
      "<b>Function: </b>Source for the local radius used for elevation "
      "calculations.";
    m_radiusBox->setWhatsThis(text);
    connect( m_radiusBox, SIGNAL( activated(int) ),
            this, SLOT( updateRadiusLineEdit() ));

    m_radiusLineEdit = new QLineEdit(hbox);
    QDoubleValidator *dval = new QDoubleValidator(hbox);
    m_radiusLineEdit->setValidator(dval);
    m_radiusLineEdit->setReadOnly(true);
    m_radiusLineEdit->setToolTip("Custom local radius used for elevation "
                                 "calculations.  To enter a value, set box to "
                                 "the left to \"Custom Radius\"");
    text =
      "<b>Function: </b>Custom local radius used to calculate elevations.  "
      "This can be changed by selecting \"Custom Radius\" in the box to "
      "the left.";
    m_radiusLineEdit->setWhatsThis(text);
//  connect(m_radiusLineEdit, SIGNAL(textEdited(QString) ),
//          this, SLOT(userBaseRadius(QString) ));
    connect( m_radiusLineEdit, SIGNAL( editingFinished() ),
            this, SLOT( userBaseRadius() ));
    //  Do not enable unless radius box set to Custom Radius
    m_radiusLineEdit->setEnabled(false);

    QLabel *radiusUnit = new QLabel("Meters");

    QToolButton *helpButton = new QToolButton(hbox);
    helpButton->setIcon( QPixmap(toolIconDir() + "/help-contents.png") );
    helpButton->setToolTip("Help");
    helpButton->setIconSize( QSize(22, 22) );
    connect( helpButton, SIGNAL( clicked() ), this, SLOT( showHelp() ));

    QHBoxLayout *layout = new QHBoxLayout;
    layout->setMargin(0);
    layout->addWidget(AbstractPlotTool::createToolBarWidget(parent) );
    layout->addWidget(boxLabel);
    layout->addWidget(m_radiusBox);
    layout->addWidget(m_radiusLineEdit);
    layout->addWidget(radiusUnit);
    layout->addStretch();
    layout->addWidget(helpButton);
    hbox->setLayout(layout);

    readSettings();

    return hbox;
  }



  void StereoTool::activateTool() {

    warningDialog();
  }



  PlotWindow *StereoTool::createWindow() {
    return new PlotWindow("Elevation Profile", PlotCurve::PixelNumber,
                          PlotCurve::Elevation,
                          qobject_cast<QWidget *>( parent() ) );

  }



  void StereoTool::detachCurves() {
  }



  void StereoTool::warningDialog() {

    if (m_showWarning) {
      QDialog  *warningDialog = new QDialog(m_stereoTool);

      QVBoxLayout *mainLayout = new QVBoxLayout;
      warningDialog->setLayout(mainLayout);

      QLabel *warningsText = new QLabel("<p><strong>Warning:</strong> "
          "The camera orientations are very critical for correct results.  "
          "Poor orientations will result in bad elevation measurements.  The "
          "camera orientations can be corrected with the programs "
          "<i>jigsaw, deltack, or qtie.");
      warningsText->setWordWrap(true);
      mainLayout->addWidget(warningsText);
      QCheckBox *showWarning = new QCheckBox("Do not show this message again");
      showWarning->setChecked(false);
      mainLayout->addWidget(showWarning);

      QPushButton *okButton = new QPushButton("OK");
      mainLayout->addStretch();
      mainLayout->addWidget(okButton);
      connect( okButton, SIGNAL( clicked() ), warningDialog, SLOT( accept() ));

      if ( warningDialog->exec() ) {
        if ( showWarning->isChecked() ) m_showWarning = false;
      }
      writeSettings();
    }

  }



  void StereoTool::showHelp() {

    QDialog *helpDialog = new QDialog(m_stereoTool);
    helpDialog->setWindowTitle("Stereo Tool Help");

    QVBoxLayout *mainLayout = new QVBoxLayout;
    helpDialog->setLayout(mainLayout);

    QLabel *stereoTitle = new QLabel("<h2>Stereo Tool</h2>");
    mainLayout->addWidget(stereoTitle);

    QLabel *stereoSubtitle = new QLabel("A tool for calculating point elevations "
                                  "and elevation profiles using stereo pairs "
                                  "of cubes.");
    stereoSubtitle->setWordWrap(true);
    mainLayout->addWidget(stereoSubtitle);

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

    QLabel *purposeText = new QLabel("<p>This tool will use parallax from a "
        "stereo pair of cubes to calculate elevations at chosen control points "
        "or create elevation profiles between two chosen control points. "
        "Elevations are computed from points between the left and right cubes. "
        "Vectors from the target (planet center) to the spacecraft and target "
        "to the surface registration points are computed for each point. From "
        "these points, the elevation is computed.");
    purposeText->setWordWrap(true);
    overviewLayout->addWidget(purposeText);

    QLabel *warningsTitle = new QLabel("<h2>Warnings</h2>");
    overviewLayout->addWidget(warningsTitle);

    QLabel *warningsText = new QLabel("<p>The camera orientations are very "
        "critical for correct results.  Poor orientations will result in "
        "bad elevation measurements.  The camera orientations can be corrected "
        "with the programs <i>jigsaw, deltack, or qtie.");
    warningsText->setWordWrap(true);
    overviewLayout->addWidget(warningsText);

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
        "<li>Open the two cubes of a stereo pair</li>"
        "<li>Link the two displayed cube windows</li>");
    quickPrep->setWordWrap(true);
    quickLayout->addWidget(quickPrep);

    QLabel *quickFunctionTitle = new QLabel("<h3>Cube Viewport Functions:</h3>");
    quickLayout->addWidget(quickFunctionTitle);

    QLabel *quickFunction = new QLabel(
        "The stereo tool window will be shown once "
        "you click on a cube viewport window using one of the following "
        "cube viewport functions.");
    quickFunction->setWordWrap(true);
    quickLayout->addWidget(quickFunction);

    QLabel *quickDesc = new QLabel("<p><ul>"
      "<li>Calculate elevation at a single point by creating a "
      "control point between the image pair by right clicking in the cube "
      "viewport window on the location you are interested in.  Once the "
      "control point is refined, click the \"Save Measure\" button in "
      "the Stereo Tool window and the elevation will be calculated.  The elevation "
      "reported is relative to the radius which is defined on the toolbar.</li>"
      "<li>Left click and drag will create an elevation profile "
      "after you create the start and end control points.  A dialog "
      "box will be shown to assist in creating the control points.</li>"
      "<li>Right click and drag will create an elevation profile "
      "between two previously created control points.</li></ul>"
      "<p><strong>Note:</strong>  The quality of the profiles is dependent on the registration "
      "between the two images at each point along the profile.  Registration "
      "parameters can be changed under Options->Registration mentu of the "
      "Elevation Calculator window. A discussion of these parameters can be found at: "
      "<a href=\"https://astrogeology.usgs.gov/docs/concepts/control-networks/pattern-matching/\">"
      "Pattern Matching</a>");
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

    QLabel *mouseLabel = new QLabel("<p><h3>When the \"Stereo\" tool "
      "is activated, the mouse buttons have the following function in the "
      "cube viewports of the main qview window:</h3>");
    mouseLabel->setWordWrap(true);
    mouseLabel->setScaledContents(true);
    controlPointLayout->addWidget(mouseLabel);

    QLabel *controlPointDesc = new QLabel("<ul>"
      "<li>Left click   - Edit the closest control point</li>"
      "<li>Middle click - Delete the closest control point</li>"
      "<li>Right click  - Create new control point at cursor location</li>"
      "<li>Left click and drag - Create an elevation profile "
      "after you create the start and end control points.  A dialog "
      "box will be shown to assist in creating the control points.</li>"
      "<li>Right click and drag - Create an elevation profile "
      "between two previously created control points.</li></ul>");
    controlPointDesc->setWordWrap(true);
    controlPointLayout->addWidget(controlPointDesc);

    QLabel *controlPointEditing = new QLabel(
      "<h4>Changing Measure Locations</h4>"
        "<p>The measure location can be adjusted by:"
      "<ul>"
      "<li>Move the cursor location under the crosshair by clicking the left mouse "
            "button</li>"
      "<li>Move 1 pixel at a time by using arrow keys on the keyboard</li>"
      "<li>Move 1 pixel at a time by using arrow buttons above the right view</li>"
      "</ul></p>"
      "<h4>Other Point Editor Functions</h4>"
        "<p>Along the right border of the window:</p>"
        "<ul>"
          "<li><strong>Geom:</strong>   Geometrically match the right view to the left"
              "view</li>"
          "<li><strong>Rotate:</strong>   Rotate the right view using either the dial"
              "or entering degrees </li>"
          "<li><strong>Show control points:</strong>  Draw crosshairs at all control"
               "point locations visible within the view</li>"
          "<li><strong>Show crosshair:</strong>  Show a red crosshair across the entire"
              "view</li>"
          "<li><strong>Circle:</strong>  Draw circle which may help center measure"
              "on a crater</li></ul"
        "<p>Below the left view:</p>"
          "<ul><li><strong>Blink controls:</strong>  Blink the left and right view in the"
          "left view window using the \"Blink Start\" button (with play icon) and "
          "\"Blink Stop\" button (with stop icon).  Both arrow keys above the right view "
          "and the keyboard arrow keys may be used to move the right view while"
          "blinking.</li>"
          "<li><strong>Find:</strong> Center the right view so that the same latitude "
          "longitude is under the crosshair as the left view.</li></ul"
        "<p>Below the right view:</p>"
        "<ul><li><strong>Register:</strong>  Sub-pixel register the the right view to"
              "the left view.</li>"
          "<li><strong>Save Measure:</strong>  Save the control point under the"
              "crosshairs and calculated elevation.</li></ul>");
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
    closeButton->setIcon( QPixmap( toolIconDir() + "/guiStop.png" ) );
    closeButton->setDefault(true);
    connect( closeButton, SIGNAL( clicked() ),
             helpDialog, SLOT( close() ) );
    buttonsLayout->addWidget(closeButton);

    mainLayout->addLayout(buttonsLayout);

    helpDialog->show();


  }



  void StereoTool::userBaseRadius() {
    //  TODO  test validity
    try {
        m_baseRadius = Distance(m_radiusLineEdit->text().toDouble(),
                                Distance::Meters);

        //  If stereo Tool visible and has valid point, update elevation and
        //  save new elevation to point
        if (m_stereoTool->isVisible() && m_editPoint != NULL) {
          calculateElevation();
        }
    }
    catch (IException &e) {
      QString message = "Invalid base radius entered.";
      message += QString::fromStdString(e.toString());
      m_radiusLineEdit->setText("");
      QMessageBox::critical(m_stereoTool, "Error", message);
      m_baseRadius = Distance(0., Distance::Meters);
      return;
    }

  }



  void StereoTool::updateRadiusLineEdit() {


    if ( m_radiusBox->currentText() == "Ellipsoid Equitorial Radius" ) {
      if ( m_targetRadius.isValid() ) {
        m_radiusLineEdit->setText( QString::number( m_targetRadius.meters(), 'f',
                                                  6) );
        m_baseRadius = m_targetRadius;
      }
      else {
        m_radiusLineEdit->setText("");
      }
      m_radiusLineEdit->setReadOnly(true);
      m_radiusLineEdit->setEnabled(false);
    }
    else if ( m_radiusBox->currentText() == "DEM Radius" ) {
      //  If cubes set, make sure they have an elevation model
      if (m_leftCube) {
        m_leftCube->camera()->IgnoreElevationModel(false);
        if ( m_leftCube->camera()->target()->shape()->name() == "Ellipsoid" ) {
        QString message = "No valid Dem on cube.  Run <i>spicinit</i> using a "
           "dem shape model.  The local radius will default back to the ellipsoid.";
        QMessageBox::warning(m_stereoTool, "Warning", message);
        m_radiusBox->setCurrentIndex(0);

        m_radiusLineEdit->setReadOnly(true);
        m_radiusLineEdit->setEnabled(false);
        return;
        }
      }
      m_radiusLineEdit->setText("");
      m_radiusLineEdit->setReadOnly(true);
      m_radiusLineEdit->setEnabled(false);
      m_baseRadius = Distance(0., Distance::Meters);
    }
    else {  //  "Custom Radius"
      m_radiusLineEdit->setReadOnly(false);
      m_radiusLineEdit->setEnabled(true);
    }

    //  If stereo Tool visible and has valid point, update elevation
    if ( m_stereoTool->isVisible() && m_editPoint != NULL ) {
      calculateElevation();
    }
  }



  void StereoTool::setupFiles() {

    /*   TODO
      .5   Get vector of all linked viewports
       1.  If no files linked or > 2 linked, print errror & return
       2.  Check if new files
          if yes:  clear old
          if no: return

    */
    m_linkedViewports.clear();
    for (int i = 0; i < (int) cubeViewportList()->size(); i++) {
      if ( (*( cubeViewportList() ) )[i]->isLinked() )
        m_linkedViewports.push_back( (*( cubeViewportList() ) )[i]);
    }

    if ( m_linkedViewports.size() < 2 ) {
      IString message = "Two cube viewports containing a stereo pair "
                        "need to be linked.";
      throw IException(IException::User, message, _FILEINFO_);
    }
    if ( m_linkedViewports.size() > 2 ) {
      IString message = "Only two cube viewports containing a stereo pair "
                        "may be linked.";
      throw IException(IException::User, message, _FILEINFO_);
    }

    //  If linked viewports contain the same cubes,  simply return,
    //  all data should be retained.
    if ( m_linkedViewports.at(0)->cube() == m_leftCube ||
         m_linkedViewports.at(0)->cube() == m_rightCube ) {
      if ( m_linkedViewports.at(1)->cube() == m_leftCube ||
           m_linkedViewports.at(1)->cube() == m_rightCube ) {
        return;
      }
    }

    //  Control net already exists, make sure new cubes are the same target
    //  as the current control net.
    if (m_controlNet) {
      if ( m_controlNet->GetTarget() !=
           m_linkedViewports.at(0)->cube()->camera()->target()->name() ) {
        //  Allow opportunity to save current data before clearing for new
        //  target.
        QString message = "You have changed targets.  All data must be re-set";
        message += " for new target.  Would you like to save your current";
        message += " points before resetting?";
        int response = QMessageBox::question(m_stereoTool,
                            "Save current points", message,
                            QMessageBox::Yes | QMessageBox::No,
                            QMessageBox::Yes);
        if (response == QMessageBox::Yes) {
          saveAsElevations();
        }
        m_stereoTool->setVisible(false);
        clearNetData();
        m_controlNet = new ControlNet();
        m_controlNet->SetTarget(*m_linkedViewports.at(0)->cube()->label());
        m_serialNumberList = new SerialNumberList(false);
      }
    }
    else {
      m_controlNet = new ControlNet();
      m_controlNet->SetTarget(
                              m_linkedViewports.at(0)->cube()->camera()->target()->name() );
      m_serialNumberList = new SerialNumberList(false);
    }


    // TODO:  For now simply clear & set always, but should check if cubes are
    // not new.
    clearFiles();
    setFiles( m_linkedViewports.at(0)->cube(), m_linkedViewports.at(1)->cube() );

  }



  /**
   * New files selected, clean up old file info
   *
   * @internal
   * @history  2007-06-12 Tracie Sucharski - Added method to allow user to
   *                           run on new files.
   * @history  2010-05-11 Tracie Sucharski - Moved the creation of control net
   *                           to the StereoFileTool::open.
   */
  void StereoTool::clearFiles() {
    m_stereoTool->setVisible(false);

    if (m_leftGM) {
      delete m_leftGM;
      m_leftGM = NULL;
    }
    if (m_rightGM) {
      delete m_rightGM;
      m_rightGM = NULL;
    }

  }



  /**
   * Setup the stereo cubes
   *
   * @param cube1  First cube of stereo pair
   * @param cube2  Second cube of stereo pair
   *
   */
  void StereoTool::setFiles(Cube *leftCube, Cube *rightCube) {

    //  Save off base map cube, but add matchCube to serial number list
    m_leftCube = leftCube;
    m_rightCube = rightCube;

    QString leftName = QString::fromStdString(FileName( m_leftCube->fileName().toStdString() ).name());
    QString rightName = QString::fromStdString(FileName( m_rightCube->fileName().toStdString() ).name());
    //  Update cube name labels
    m_leftCubeLabel->setText(leftName);
    m_rightCubeLabel->setText(rightName);

    m_leftSN = SerialNumber::Compose(*m_leftCube);
    m_rightSN = SerialNumber::Compose(*m_rightCube);

    //  TODO   Do I need list?
    if ( !m_serialNumberList->hasSerialNumber(m_leftSN) ) {
      m_serialNumberList->add( m_leftCube->fileName() );
    }
    if ( !m_serialNumberList->hasSerialNumber(m_rightSN) ) {
      m_serialNumberList->add( m_rightCube->fileName() );
    }

    try {
      PvlGroup pvlRadii = Target::radiiGroup(m_controlNet->GetTarget());
      m_targetRadius = Distance(pvlRadii["EquatorialRadius"], Distance::Meters);
    }
    catch(IException &e) {
      QString message = "Could not determine target radius.";
      QMessageBox::critical(m_stereoTool,"Error",message);
      m_baseRadius = Distance(0., Distance::Meters);
      updateRadiusLineEdit();
      return;
    }
    updateRadiusLineEdit();

    //  Save off universal ground maps
    try {
      m_leftGM = new UniversalGroundMap(*m_leftCube);
    }
    catch (IException &e) {
      QString message = "Cannot initialize universal ground map for " +
                        m_leftCube->fileName() + ".\n";
      message += QString::fromStdString(e.toString());
      QMessageBox::critical(m_stereoTool, "Error", message);
      return;
    }
    try {
      m_rightGM = new UniversalGroundMap(*m_rightCube);
    }
    catch (IException &e) {
      QString message = "Cannot initialize universal ground map for" +
                        m_rightCube->fileName() + ".\n";
      message += QString::fromStdString(e.toString());
      QMessageBox::critical(m_stereoTool, "Error", message);
      return;
    }


  }



  /**
   * Save control measures under crosshairs of ChipViewports
   *
   */
  void StereoTool::measureSaved() {

    double samp = m_editPoint->GetMeasure(Left)->GetSample();
    double line = m_editPoint->GetMeasure(Left)->GetLine();
    m_leftGM->SetImage(samp, line);
    double lat = m_leftGM->UniversalLatitude();
    double lon = m_leftGM->UniversalLongitude();

    m_rightGM->SetGround(
              Latitude(lat, Angle::Degrees), Longitude(lon, Angle::Degrees) );
    try {
      m_editPoint->SetAprioriSurfacePoint(SurfacePoint (
                Latitude(lat, Angle::Degrees), Longitude(lon, Angle::Degrees),
                m_targetRadius ) );
    }
    catch (IException &e) {
      QString message = "Unable to set Apriori Surface Point.\n";
      message += "Latitude = " + QString::number(lat);
      message += "  Longitude = " + QString::number(lon);
      message += "  Radius = " + QString::number(m_targetRadius.meters(), 'f',
                                                 6) + "\n";
      message += QString::fromStdString(e.toString());
      QMessageBox::critical(m_stereoTool, "Error", message);
    }

    calculateElevation();
    emit editPointChanged();
  }



  /**
   * This methods enables the RubberBandTool, it also sets the
   * RubberBandTool to allow points and to allow all clicks.
   *
   */
  void StereoTool::enableRubberBandTool() {
    rubberBandTool()->enable(RubberBandTool::LineMode);
    rubberBandTool()->enablePoints();
    rubberBandTool()->enableAllClicks();
    rubberBandTool()->setDrawActiveViewportOnly(true);
  }



  void StereoTool::rubberBandComplete() {

//    clearProfile();

    try {
      setupFiles();
    }
    catch (IException &e) {
      QString message = QString::fromStdString(e.toString());
      QMessageBox::critical(m_stereoTool, "Error setting stereo pair", message);
      rubberBandTool()->clear();
      return;
    }

    MdiCubeViewport *cvp = cubeViewport();
    if (cvp  == NULL)
      return;

    QString file = cvp->cube()->fileName();
    QString sn;
    try {
      sn = m_serialNumberList->serialNumber(file);
    }
    catch (IException &e) {
      QString message = "This cube is not linked as a stereo pair.  Make ";
      message += "sure you have two stereo pair cube viewports linked.";
      QMessageBox::critical(m_stereoTool,"Viewport not linked", message);
      return;
    }

    if ( rubberBandTool()->figureIsPoint() ) {
      double samp, line;
      cvp->viewportToCube( rubberBandTool()->vertices()[0].rx(),
                           rubberBandTool()->vertices()[0].ry(),
                           samp, line );
      if ( rubberBandTool()->mouseButton() & Qt::LeftButton ) {
        if ( !m_controlNet || m_controlNet->GetNumMeasures() == 0 ) {
          QString message = "No points exist for editing.  Create points ";
          message += "using the right mouse button.";
          QMessageBox::information(m_stereoTool, "Warning", message);
          return;
        }
        //  Find closest control point in network
        ControlPoint *point;
        try {
          point = m_controlNet->FindClosest(sn, samp, line);
        }
        catch (IException &e) {
          QString message = "No points found for editing.  Create points ";
          message += "using the right mouse button.";
          message += QString::fromStdString(e.toString());
          QMessageBox::critical(m_stereoTool, "Error", message);
          return;
        }
        modifyPoint(point);
      }
      else if ( rubberBandTool()->mouseButton() & Qt::MiddleButton ) {
        if ( !m_controlNet || m_controlNet->GetNumPoints() == 0 ) {
          QString message = "No points exist for deleting.  Create points ";
          message += "using the right mouse button.";
          QMessageBox::warning(m_stereoTool, "Warning", message);
          return;
        }

        //  Find closest control point in network
        ControlPoint *point =
        m_controlNet->FindClosest(sn, samp, line);
        //  TODO:  test for errors and reality
        if (point == NULL) {
          QString message = "No points exist for deleting.  Create points ";
          message += "using the right mouse button.";
          QMessageBox::information(m_stereoTool, "Warning", message);
          return;
        }
        deletePoint(point);
      }
      else if ( rubberBandTool()->mouseButton() & Qt::RightButton ) {
        double lat, lon;
        if (cvp->cube() == m_leftCube) {
          m_leftGM->SetImage(samp, line);
          lat = m_leftGM->UniversalLatitude();
          lon = m_leftGM->UniversalLongitude();
        }
        else {
          m_rightGM->SetImage(samp, line);
          lat = m_rightGM->UniversalLatitude();
          lon = m_rightGM->UniversalLongitude();
        }
        try {
          createPoint(lat, lon);
        }
        catch (IException &e) {
          QString message = "Cannot create control point.\n\n";
          message += QString::fromStdString(e.toString());
          QMessageBox::critical(m_stereoTool, "Error", message);
          m_startPoint = NULL;
          rubberBandTool()->clear();
          return;
        }
      }
    }
    //  RubberBand line drawn:
    else {
      m_startPoint = NULL;
      m_endPoint = NULL;
      //    Right click/drag:  Find closest end points
      if ( rubberBandTool()->mouseButton() & Qt::RightButton ) {
        double samp, line;
        cvp->viewportToCube( rubberBandTool()->vertices()[0].rx(),
                             rubberBandTool()->vertices()[0].ry(),
                             samp, line );
        try {
          m_startPoint = m_controlNet->FindClosest(sn, samp, line);
        }
        catch (IException &e) {
          QString message = "Cannot find start point for profile.  Either ";
          message += "create end points individually using the right mouse ";
          message += "button.  Or, create profile end points by clicking and ";
          message += "dragging with the right mouse button.\n\n";
          message += QString::fromStdString(e.toString());
          QMessageBox::critical(m_stereoTool, "Error", message);
          m_startPoint = NULL;
          rubberBandTool()->clear();
          return;
        }
        cvp->viewportToCube( rubberBandTool()->vertices()[1].rx(),
                             rubberBandTool()->vertices()[1].ry(),
                            samp, line );
        try {
          m_endPoint = m_controlNet->FindClosest(sn, samp, line);
          if ( m_startPoint->GetId() == m_endPoint->GetId() ) {
            throw IException(IException::User, "No End Point",
                                      _FILEINFO_);
          }
        }
        catch (IException &e) {
          QString message = "Cannot find end point for profile.  Either ";
          message += "create end points individually using the right mouse ";
          message += "button.  Or, create profile end points by clicking and ";
          message += "dragging with the right mouse button.\n\n";
          message += QString::fromStdString(e.toString());
          QMessageBox::critical(m_stereoTool, "Error", message);
          m_startPoint = NULL;
          m_endPoint = NULL;
          rubberBandTool()->clear();
          return;
        }
        profile();
      }
      else {
        //    Left click/drag:  Create control points at the line endpoints.
        m_profileDialog = new ProfileDialog();
        connect( m_profileDialog, SIGNAL( createStart() ),
                 this, SLOT( createStartPoint() ) );
        connect( m_profileDialog, SIGNAL( createEnd() ),
                 this, SLOT( createEndPoint() ) );
        connect( m_profileDialog, SIGNAL( accepted() ),
                 this, SLOT( profile() ) );
        connect( m_profileDialog, SIGNAL( accepted() ),
                this, SLOT( profile() ) );
        connect( m_profileDialog, SIGNAL( rejected() ),
                this, SLOT( clearProfile() ) );
        m_profileDialog->show();
        m_profileDialog->activateWindow();
      }
    }
  }



  void StereoTool::clearProfile() {
    m_startPoint = NULL;
    m_endPoint = NULL;
    rubberBandTool()->clear();
    delete m_profileDialog;
    m_profileDialog = NULL;
  }



  void StereoTool::createStartPoint() {
    MdiCubeViewport *cvp = cubeViewport();
    if (cvp == NULL)
      return;

    double samp, line;
    double lat, lon;
    cvp->viewportToCube( rubberBandTool()->vertices()[0].rx(),
                         rubberBandTool()->vertices()[0].ry(),
                        samp, line );
    if ( cvp->cube() == m_leftCube ) {
      m_leftGM->SetImage(samp, line);
      lat = m_leftGM->UniversalLatitude();
      lon = m_leftGM->UniversalLongitude();
    }
    else {
      m_rightGM->SetImage(samp, line);
      lat = m_rightGM->UniversalLatitude();
      lon = m_rightGM->UniversalLongitude();
    }
    try {
      createPoint(lat, lon);
    }
    catch (IException &e) {
      QString message = "Cannot create control point.\n\n";
      message += QString::fromStdString(e.toString());
      QMessageBox::critical(m_stereoTool, "Error", message);
      delete m_profileDialog;
      m_profileDialog = NULL;
      rubberBandTool()->clear();
      return;
    }
    m_startPoint = m_editPoint;
  }



  void StereoTool::createEndPoint() {
    MdiCubeViewport *cvp = cubeViewport();
    if (cvp == NULL)
      return;

    double samp, line;
    double lat, lon;
    cvp->viewportToCube( rubberBandTool()->vertices()[1].rx(),
                         rubberBandTool()->vertices()[1].ry(),
                         samp, line );
    if ( cvp->cube() == m_leftCube ) {
      m_leftGM->SetImage(samp, line);
      lat = m_leftGM->UniversalLatitude();
      lon = m_leftGM->UniversalLongitude();
    }
    else {
      m_rightGM->SetImage(samp, line);
      lat = m_rightGM->UniversalLatitude();
      lon = m_rightGM->UniversalLongitude();
    }
    try {
      createPoint(lat, lon);
    }
    catch (IException &e) {
      QString message = "Cannot create control point.\n\n";
      message += QString::fromStdString(e.toString());
      QMessageBox::critical(m_stereoTool, "Error", message);
      m_startPoint = NULL;
      delete m_profileDialog;
      m_profileDialog = NULL;
      rubberBandTool()->clear();
      return;
    }
    m_endPoint = m_editPoint;

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
   */
  void StereoTool::createPoint(double lat, double lon) {

    //  TODO:   ADD AUTOSEED OPTION (CHECKBOX?)

    double leftSamp = 0, leftLine = 0;
    double rightSamp = 0, rightLine = 0;

    //  Make sure point exists on both linked cubes
    if ( m_leftGM->SetUniversalGround(lat, lon) ) {
      leftSamp = m_leftGM->Sample();
      leftLine = m_leftGM->Line();

      //  Make sure point is on Right cube
      if ( m_rightGM->SetUniversalGround(lat, lon) ) {
        //  Make sure point on Right cube
        rightSamp = m_rightGM->Sample();
        rightLine = m_rightGM->Line();
        if ( rightSamp < 1 || rightSamp > m_rightCube->sampleCount() ||
             rightLine < 1 || rightLine > m_rightCube->lineCount() ) {
          IString message = "Point does not exist on cube, " +
                            m_rightCube->fileName().toStdString() + ".";
          throw IException(IException::User, message, _FILEINFO_);
//        QString message = "Point does not exist on cube, " +
//                          QString(m_rightCube->fileName().c_str() ) + ".";
//        QMessageBox::critical(m_stereoTool, "Error", message);
//        return;
        }
      }
      else {
        IString message = "Point does not exist on cube, " +
                          m_rightCube->fileName().toStdString() + ".";
        throw IException(IException::User, message, _FILEINFO_);
//      QString message = "Point does not exist on cube, " +
//                        QString(m_rightCube->fileName().c_str() ) + ".";
//      QMessageBox::critical(m_stereoTool, "Error", message);
//      return;
      }
    }
    else {
      IString message = "Point does not exist on cube, " +
                        m_leftCube->fileName().toStdString() + ".";
      throw IException(IException::User, message, _FILEINFO_);
//    QString message = "Point does not exist on cube, " +
//                      QString(m_leftCube->fileName().c_str() ) + ".";
//    QMessageBox::critical(m_stereoTool, "Error", message);
//    return;
    }

    //  Point is on both Left and Right cubes, create new control point
    ControlPoint *newPoint = NULL;
    //  Prompt for point Id
    bool goodId = false;
    while (!goodId) {
      bool ok = false;
      QString id = QInputDialog::getText(m_stereoTool,
                                         "Point ID", "Enter Point ID:",
                                         QLineEdit::Normal, lastPtIdValue,
                                         &ok);
      if (!ok)
        // user clicked "Cancel"
      {
        return;
      }
      if ( ok && id.isEmpty() ) {
        // user clicked "Ok" but did not enter a point ID
        QString message = "You must enter a point Id.";
        QMessageBox::warning(m_stereoTool, "Warning", message);
      }
      else {
        // Make sure Id doesn't already exist
        newPoint = new ControlPoint(id);
        if ( m_controlNet->GetNumPoints() > 0 &&
             m_controlNet->ContainsPoint(newPoint->GetId() ) ) {
          QString message = "A ControlPoint with Point Id = [" +
                            newPoint->GetId() +
                            "] already exists.  Re-enter unique Point Id.";
          QMessageBox::warning(m_stereoTool, "Unique Point Id", message);
        }
        else {
          goodId = true;
          lastPtIdValue = id;
        }
      }
    }

    newPoint->SetType(ControlPoint::Free);
    newPoint->SetAprioriSurfacePoint( SurfacePoint(
              Latitude(lat, Angle::Degrees), Longitude(lon, Angle::Degrees),
              m_targetRadius) );

    // Set first measure to Left
    ControlMeasure *mLeft = new ControlMeasure;
    mLeft->SetCubeSerialNumber(m_leftSN);
    mLeft->SetCoordinate(leftSamp, leftLine);
    mLeft->SetType(ControlMeasure::Manual);
    mLeft->SetDateTime();
    mLeft->SetChooserName( Application::UserName() );
    newPoint->Add(mLeft);
    //  Second measure is Right measure
    ControlMeasure *mRight = new ControlMeasure;
    mRight->SetCubeSerialNumber(m_rightSN);
    mRight->SetCoordinate(rightSamp, rightLine);
    mRight->SetType(ControlMeasure::Manual);
    mRight->SetDateTime();
    mRight->SetChooserName( Application::UserName() );
    newPoint->Add(mRight);

    //  Add new control point to control network
    m_controlNet->AddPoint(newPoint);
    //  Read newly added point
    m_editPoint = m_controlNet->GetPoint( (QString) newPoint->GetId() );
    //  Load new point in StereoTool
    loadPoint();
    m_stereoTool->setVisible(true);
    m_stereoTool->raise();

    emit editPointChanged();
  }


  /**
   * Delete given control point
   *
   * @param point Input  Control Point to delete
   *
   * @history 2010-05-19 Tracie Sucharski - Fixed bug which was causing a seg
   *                        fault.  Set m_editPoint to NULL, also no sense
   *                        loading point to be deleted.  Should this be
   *                        smartened up to load another Point?
   *
   */
  void StereoTool::deletePoint(ControlPoint *point) {

    m_editPoint = point;
    //  Change point in viewport to red so user can see what point they are
    //  about to delete.
    emit editPointChanged();

    //loadPoint();

    m_controlNet->DeletePoint( m_editPoint->GetId() );
    m_stereoTool->setVisible(false);
    m_editPoint = NULL;

    emit editPointChanged();
  }


  /**
   * Modify given control point
   *
   * @param point Input  Control Point to modify
   *
   */
  void StereoTool::modifyPoint(ControlPoint *point) {

    m_editPoint = point;
    loadPoint();
    m_stereoTool->setVisible(true);
    m_stereoTool->raise();
    emit editPointChanged();
  }


  /**
   * Load control point into the ControlPointEdit widget
   *
   * @history 2010-05-18  Tracie Sucharski - Added pointId to the dialog.
   */
  void StereoTool::loadPoint() {

    //  Initialize pointEditor with measures
    m_pointEditor->setLeftMeasure( m_editPoint->GetMeasure(Left), m_leftCube,
                                   m_editPoint->GetId() );
    m_pointEditor->setRightMeasure( m_editPoint->GetMeasure(Right),
                                    m_rightCube, m_editPoint->GetId() );

    //  Write pointId
    QString ptId = "Point ID:  " + m_editPoint->GetId();
    m_ptIdValue->setText(ptId);

    updateLabels();
  }



  void StereoTool::paintProfile(MdiCubeViewport *vp, QPainter *painter,
                                QString serialNumber) {

    // Draw profile
    int x1, y1, x2, y2;
    //MdiCubeViewport *cvp = cubeViewport();
    vp->cubeToViewport( m_startPoint->GetMeasure(serialNumber)->GetSample(),
                        m_startPoint->GetMeasure(serialNumber)->GetLine(),
                        x1, y1 );
    vp->cubeToViewport( m_endPoint->GetMeasure(serialNumber)->GetSample(),
                        m_endPoint->GetMeasure(serialNumber)->GetLine(),
                        x2, y2 );
    painter->setPen(Qt::green); // set all other point markers green
    painter->drawLine(x1, y1, x2, y2);

  }



  /**
   * Repaint the given CubeViewport
   *
   * @param vp       Input  CubeViewport to repain
   * @param painter  Input  Qt Painter
   *
   */
  void StereoTool::paintViewport(MdiCubeViewport *vp, QPainter *painter) {

    AbstractPlotTool::paintViewport(vp, painter);

    //  Make sure we have points to draw
    if ( m_controlNet == NULL || m_controlNet->GetNumPoints() == 0 )
      return;

    QString serialNumber = SerialNumber::Compose(*vp->cube(), true);

    //  If viewport serial number not found in control net, return
    if ( !m_controlNet->GetCubeSerials().contains(
               serialNumber) ) return;

    //  Draw profile if it exists
//  if (m_startPoint != NULL && m_endPoint != NULL) {
//    paintProfile(vp, painter, serialNumber);
//  }

    //  Get all measures for this viewport
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
      if ( m->Parent()->IsIgnored() ) {
        painter->setPen( QColor(255, 255, 0) ); // set point marker yellow
      }
      // point is not ignored, but measure matching this image is ignored,
      else if ( m->IsIgnored() ) {
        painter->setPen( QColor(255, 255, 0) ); // set point marker yellow
      }
      // Neither point nor measure is not ignored and the measure is fixed,
      else if ( m->Parent()->GetType() != ControlPoint::Free) {
        painter->setPen(Qt::magenta);// set point marker magenta
      }
      else {
        painter->setPen(Qt::green); // set all other point markers green
      }
      // draw points
      painter->drawLine(x - 5, y, x + 5, y);
      painter->drawLine(x, y - 5, x, y + 5);
    }

    // if StereoTool is open,
    if ( m_editPoint != NULL ) {
      // and the selected point is in the image,
      if ( m_editPoint->HasSerialNumber(serialNumber) ) {
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
   * This method will repaint the control measures in each viewport
   * @internal
   */
  void StereoTool::paintAllViewports() {

    // Take care of drawing things on all viewPorts.
    // Calling update will cause the Tool class to call all registered tools
    // if point has been deleted, this will remove it from the main window
    MdiCubeViewport *vp;
    for (int i = 0; i < (int) cubeViewportList()->size(); i++) {
      vp = (*( cubeViewportList() ) )[i];
       vp->viewport()->update();
    }
  }



  void StereoTool::calculateElevation() {
    calculateElevation(m_editPoint);
  }



  /**
   *
   * @history 2011-09-21 Tracie Sucharski, adapted from Kay Edwards,tvstereo
   */
  void StereoTool::calculateElevation(ControlPoint *point) {

    double elevation=0., elevationError=0.;
    Camera *leftCamera = m_leftCube->camera();

    //  If the local radius combo box is set to DEM, get the dem radius
    //  First, SetImage using the Elevation model, before turning off
    //  to get camera angles.
    if ( m_radiusBox->currentText() == "DEM Radius" ) {
      leftCamera->IgnoreElevationModel(false);
      leftCamera->SetImage( (*point)[Left]->GetSample(),
                           (*point)[Left]->GetLine() );
      m_baseRadius = leftCamera->LocalRadius( leftCamera->GetLatitude(),
                                              leftCamera->GetLongitude() );
      if ( !m_baseRadius.isValid() ) {
        QString message = "Invalid Dem radius, defaulting to ellipsoidal.";
        QMessageBox::warning(m_stereoTool, "Invalid Dem radius", message);
        m_baseRadius = m_targetRadius;
      }
    }

    leftCamera->IgnoreElevationModel(true);
    leftCamera->SetImage( (*point)[Left]->GetSample(),
                         (*point)[Left]->GetLine() );
    Camera *rightCamera = m_rightCube->camera();
    rightCamera->IgnoreElevationModel(true);
    rightCamera->SetImage( (*point)[Right]->GetSample(),
                          (*point)[Right]->GetLine() );

    double radius, lat, lon, sepang;
    if ( Stereo::elevation( *leftCamera, *rightCamera, radius, lat, lon, sepang,
                            elevationError ) ) {
      elevation = radius - m_baseRadius.meters();
//      cout<<setprecision(15)<<"radius = "<<radius<<"  baseRadius = "<<m_baseRadius.meters()<<"  elevation = "<<elevation<<endl;
    }
    leftCamera->IgnoreElevationModel(false);
    rightCamera->IgnoreElevationModel(false);

    // Save elevation and error info to the left ControlMeasure FocalPlaneComputeX/Y.
    // TODO:  Find better way - This is not a good way to do this, using
    // ControlMeasure to save other values.  Save The baseRadius in Diameter
    point->GetMeasure(Left)->SetFocalPlaneMeasured(elevation, elevationError);
    point->GetMeasure(Left)->SetDiameter(m_baseRadius.meters() );
    updateLabels();

    return;
  }



  /**
   * Allows user to set a new template file.
   * @author 2008-12-10 Jeannie Walldren
   * @internal
   *   @history 2008-12-10 Jeannie Walldren - Original Version
   */

  void StereoTool::setTemplateFile() {
    QString filename = QFileDialog::getOpenFileName( m_stereoTool,
                       "Select a registration template", ".",
                       "Registration template files (*.def *.pvl);;All files (*)" );

    if ( filename.isEmpty() )
      return;

    m_pointEditor->setTemplateFile(filename);
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
  void StereoTool::viewTemplateFile() {
    try {
      // Get the template file from the ControlPointEditor object
      Pvl templatePvl( m_pointEditor->templateFileName().toStdString() );
      // Create registration dialog window using PvlEditDialog class
      // to view and/or edit the template
      PvlEditDialog registrationDialog(templatePvl);
      registrationDialog.setWindowTitle( "View or Edit Template File: "
                                         + QString::fromStdString(templatePvl.fileName()) );
      registrationDialog.resize(550, 360);
      registrationDialog.exec();
    }
    catch (IException &e) {
      QString message = QString::fromStdString(e.toString());
      QMessageBox::warning(m_stereoTool, "Error", message);
    }
  }


  /**
   * Save the elevation information to file
   *
   * @author 2011-12-29 Tracie Sucharski
   */
  void StereoTool::saveAsElevations() {

    QString fn = QFileDialog::getSaveFileName( m_stereoTool,
                 "Choose filename to save under",
                 ".",
                 "CSV Files (*.csv)" );
    QString filename;

    //Make sure the filename is valid
    if ( !fn.isEmpty() ) {
      if ( !fn.endsWith(".csv") ) {
        filename = fn + ".csv";
      }
      else {
        filename = fn;
      }
    }
    //The user cancelled, or the filename is empty
    else {
      return;
    }

    m_currentFile.setFileName(filename);

    m_save->setEnabled(true);
    saveElevations();
  }



  void StereoTool::saveElevations() {

    if ( m_currentFile.fileName().isEmpty() ) return;

    bool success = m_currentFile.open(QIODevice::WriteOnly);
    if (!success) {
      QMessageBox::critical(m_stereoTool, "Error",
                            "Cannot open file, please check permissions");
      m_currentFile.setFileName("");
      m_save->setDisabled(true);
      return;
    }

    QTextStream text(&m_currentFile);
    QString header = "Point ID, Latitude, Longitude, Radius, ";
    header += "Elevation, Elevation Error, ";
    header += "Image 1, Sample, Line, Image  2, Sample, Line";
    text << header << Qt::endl;

    QString leftFile = QString::fromStdString(FileName( m_leftCube->fileName().toStdString() ).name());
    QString rightFile = QString::fromStdString(FileName( m_rightCube->fileName().toStdString() ).name());
    QString data;
    for (int i = 0; i < m_controlNet->GetNumPoints(); i++) {
      ControlPoint &p = *( (*m_controlNet)[i] );
      SurfacePoint apriori = p.GetAprioriSurfacePoint();
      data = p.GetId() + "," +
             QString::number( apriori.GetLatitude().degrees() ) + "," +
             QString::number( apriori.GetLongitude().degrees() ) + "," +
             QString::number( p.GetMeasure(Left)->GetDiameter(), 'f', 6 ) +
             "," +
             QString::number( p.GetMeasure(Left)->GetFocalPlaneMeasuredX(), 'f',
                              6 ) + "," +
             QString::number( p.GetMeasure(Left)->GetFocalPlaneMeasuredY(), 'f',
                              6 ) + "," + leftFile + "," +
             QString::number( p.GetMeasure(Left)->GetSample() ) + "," +
             QString::number( p.GetMeasure(Left)->GetLine() ) + "," +
             rightFile + "," +
             QString::number( p.GetMeasure(Right)->GetSample() ) + "," +
             QString::number( p.GetMeasure(Right)->GetLine() );
      text << data << Qt::endl;
    }
    m_currentFile.close();

  }



  void StereoTool::clearNetData() {

    delete m_controlNet;
    m_controlNet = NULL;
    delete m_serialNumberList;
    m_serialNumberList = NULL;

  }



  void StereoTool::profile() {

//  (*m_startPoint)[Left]->SetCoordinate(1361.92,1317.9433);
//  (*m_endPoint)[Left]->SetCoordinate(1188.43,2763.23);
//  (*m_startPoint)[Right]->SetCoordinate(867.04,2141.16);
//  (*m_endPoint)[Right]->SetCoordinate(731.82,2666.75);

    //  Delete the profile dialog
    if (m_profileDialog) {
      delete m_profileDialog;
      m_profileDialog = NULL;
    }

//  calculateElevation(m_startPoint);
//  calculateElevation(m_endPoint);

    QPointF leftStart( (*m_startPoint)[Left]->GetSample(),
                      (*m_startPoint)[Left]->GetLine() );
    QPointF leftEnd( (*m_endPoint)[Left]->GetSample(),
                    (*m_endPoint)[Left]->GetLine() );

    QPointF rightStart( (*m_startPoint)[Right]->GetSample(),
                       (*m_startPoint)[Right]->GetLine() );
    QPointF rightEnd( (*m_endPoint)[Right]->GetSample(),
                     (*m_endPoint)[Right]->GetLine() );

    // Convert these to screen coordinates for updating the rubberband
    QList< QList<QPoint> > rubberBandVertices;
    QList<QPoint> rubberBand1;
    int sx, sy, ex, ey;
    m_linkedViewports.at(0)->cubeToViewport( (*m_startPoint)[Left]->GetSample(),
                      (*m_startPoint)[Left]->GetLine(), sx, sy);
    m_linkedViewports.at(0)->cubeToViewport( (*m_endPoint)[Left]->GetSample(),
                      (*m_endPoint)[Left]->GetLine(), ex, ey);
    rubberBand1.push_back( QPoint(sx, sy) );
    rubberBand1.push_back( QPoint(ex, ey) );

    rubberBandVertices.push_back(rubberBand1);

    QList<QPoint> rubberBand2;
    m_linkedViewports.at(1)->cubeToViewport( (*m_startPoint)[Right]->GetSample(),
                      (*m_startPoint)[Right]->GetLine(), sx, sy);
    m_linkedViewports.at(1)->cubeToViewport( (*m_endPoint)[Right]->GetSample(),
                      (*m_endPoint)[Right]->GetLine(), ex, ey);
    rubberBand2.push_back( QPoint(sx, sy) );
    rubberBand2.push_back( QPoint(ex, ey) );

    rubberBandVertices.push_back(rubberBand2);

    // Create line for left image
    QLineF leftProfile(leftStart,leftEnd);
    QLineF rightProfile(rightStart,rightEnd);

    // Determine shortest line, we will step through shortest line,
    // finding the matching position on the longer line.
    QLineF longProfile, shortProfile;
    Cube *longCube, *shortCube;
    if ( leftProfile.length() > rightProfile.length() ) {
      longProfile = leftProfile;
      longCube = m_leftCube;
      shortProfile = rightProfile;
      shortCube = m_rightCube;
    }
    else {
      longProfile = rightProfile;
      longCube = m_rightCube;
      shortProfile = leftProfile;
      shortCube = m_leftCube;
    }

    QVector<QPointF> profileData;
    double elevation = 0.;
    double elevationError = 0.;

    Pvl regDef = m_pointEditor->templateFileName().toStdString();
    AutoReg *ar = AutoRegFactory::Create(regDef);

    int failureCount = 0;
    QApplication::setOverrideCursor(Qt::WaitCursor);

    for (int i = 0; i <= (int) shortProfile.length(); i++) {
      double shortSamp=0, shortLine=0, longSamp=0, longLine=0;
      try {
        shortSamp = shortProfile.pointAt( 1/shortProfile.length() * i ).x();
        shortLine = shortProfile.pointAt( 1/shortProfile.length() * i ).y();

        longSamp = longProfile.pointAt( 1/shortProfile.length() * i ).x();
        longLine = longProfile.pointAt( 1/shortProfile.length() * i ).y();

        // Coreg
        ar->PatternChip()->TackCube(shortSamp, shortLine);
        ar->PatternChip()->Load(*shortCube);
        ar->SearchChip()->TackCube(longSamp, longLine);
        ar->SearchChip()->Load( *longCube, *( ar->PatternChip() ), *shortCube );
        ar->Register();
//        AutoReg::RegisterStatus status = ar->Register();
        if ( ar->Success() ) {
          longSamp = ar->CubeSample();
          longLine = ar->CubeLine();

          //  If the local radius combo box is set to DEM, get the dem radius
          //  First, SetImage using the Elevation model, before turning off
          //  to get camera angles.
          if ( m_radiusBox->currentText() == "DEM Radius" ) {
            shortCube->camera()->IgnoreElevationModel(false);
            shortCube->camera()->SetImage(shortSamp, shortLine);
            m_baseRadius = shortCube->camera()->LocalRadius(
                                  shortCube->camera()->GetLatitude(),
                                  shortCube->camera()->GetLongitude() );
            if ( !m_baseRadius.isValid() ) {
              QString message = "Invalid Dem radius, defaulting to ellipsoidal.";
              QMessageBox::warning(m_stereoTool, "Invalid Dem radius", message);
              m_baseRadius = m_targetRadius;
            }
          }

          shortCube->camera()->IgnoreElevationModel(true);
          longCube->camera()->IgnoreElevationModel(true);

          shortCube->camera()->SetImage(shortSamp, shortLine);
          longCube->camera()->SetImage(longSamp,longLine);
          double radius, lat, lon, sepang;
          if (Stereo::elevation( *shortCube->camera(), *longCube->camera(),
                                 radius, lat, lon, sepang, elevationError) )
          elevation = radius - m_baseRadius.meters();
          profileData.append( QPointF(i, elevation) );
//        elevations.push_back(elevation);
//        pixels.push_back(i);
        }
        else {
          failureCount++;
        }
      }
      catch (IException &e) {
        QString message = "Error registering cubes along profile line.\n";
        message += "Image 1 Sample = " + QString::number(shortSamp);
        message += "   Line = " + QString::number(shortLine);
        message += "\nImage 2 Sample = " + QString::number(longSamp);
        message += "   Line = " + QString::number(longLine) + "\n\n";
        message += QString::fromStdString(e.toString());
        QMessageBox::critical(m_stereoTool, "Error", message);
        rubberBandTool()->clear();
      }

    }
    QApplication::restoreOverrideCursor();

//    cout<<"Registration attempts = "<<(int)shortLength<<"  failures = "<<failureCount<<endl;
    QString message = "Registration attempts (pixels on line) = " +
                      QString::number( (int)shortProfile.length() ) +
                      "\n\nRegistration failures = " +
                      QString::number(failureCount) +
                      "\n\nYou can adjust registration parameters in the "
                      "\"Options\" menu in the Elevation Calculator window. "
                      "Select \"Options\", then \"Registration\", then either "
                      "\"Select registration template\" or "
                      "\"View/edit registration template\".";
    QMessageBox::information(m_stereoTool, "Registration Report", message);

    if ( ( (int)shortProfile.length() + 1 - failureCount ) < 2 ) {
      QString message = "Cannot create profile, all auto-registration between ";
      message += "the left and right cubes along the profile failed.  Try ";
      message += "adjusting the registration parameters.";
      QMessageBox::critical(m_stereoTool, "Error", message);
      return;
    }
    PlotWindow *plotWindow = selectedWindow(true);
    plotWindow->setAxisLabel(0,"Elevation (meters)");
    CubePlotCurve *plotCurve = new CubePlotCurve(PlotCurve::PixelNumber,
                                                 PlotCurve::Elevation);
//  plotCurve->setData(&pixels[0], &elevations[0], pixels.size() );
    plotCurve->setData( new QwtPointSeriesData(profileData) );
    plotCurve->setTitle("Elevations (Meters)");
    plotCurve->setPen( QPen(Qt::white) );
    plotCurve->setColor(Qt::white);
    //  Create vertices for rubberband based on refined profile end points

    //  TODO:  This needs to be changed to band displayed???
    QList<int> bands;
    bands.push_back(1);
    bands.push_back(1);
    plotCurve->setSource(m_linkedViewports, rubberBandVertices, bands);
    plotWindow->add(plotCurve);

    delete m_profileDialog;
    m_profileDialog = NULL;
//  m_startPoint = NULL;
//  m_endPoint = NULL;
//  rubberBandTool()->clear();

  }



  void StereoTool::updateLabels() {
    // Empty elevation info if nothing there
    QString elevationLabel, elevationErrorLabel;
    QString baseRadiiLabel, leftDemRadiiLabel, rightDemRadiiLabel;
    if ( m_editPoint->GetMeasure(Left)->GetFocalPlaneMeasuredX() != Isis::Null ) {
      elevationLabel = "Elevation:  " +
                       QString::number( m_editPoint->GetMeasure(Left)->
                       GetFocalPlaneMeasuredX(), 'f', 6 );
      elevationErrorLabel = "Elevation Error:  " +
                            QString::number( m_editPoint->GetMeasure(Left)->
                            GetFocalPlaneMeasuredY(), 'f', 6 );
      baseRadiiLabel = "Local Radii:  " + QString::number(
                         m_baseRadius.meters(), 'f', 6 );

      Camera *leftCamera = m_leftCube->camera();
      leftCamera->SetImage( (*m_editPoint)[Left]->GetSample(),
                           (*m_editPoint)[Left]->GetLine() );
      double leftDemRadii =
                  leftCamera->GetSurfacePoint().GetLocalRadius().meters();
      leftDemRadiiLabel = "Left DEM Radii:  " +
                          QString::number(leftDemRadii, 'f', 6);

      Camera *rightCamera = m_rightCube->camera();
      rightCamera->SetImage( (*m_editPoint)[Right]->GetSample(),
                            (*m_editPoint)[Right]->GetLine() );
      double rightDemRadii =
                  rightCamera->GetSurfacePoint().GetLocalRadius().meters();
      rightDemRadiiLabel = "Right DEM Radii:  " +
                      QString::number(rightDemRadii, 'f', 6);
    }
    else {
      elevationLabel = "Elevation:  ";
      elevationErrorLabel = "Elevation Error:  ";
      baseRadiiLabel = "Local Radii:  ";
      leftDemRadiiLabel = "Left DEM Radii:  ";
      rightDemRadiiLabel = "Right DEM Radii:  ";
    }
    m_elevationLabel->setText(elevationLabel);
    m_elevationErrorLabel->setText(elevationErrorLabel);
    m_baseRadiiLabel->setText(baseRadiiLabel);
    m_leftDemRadiiLabel->setText(leftDemRadiiLabel);
    m_rightDemRadiiLabel->setText(rightDemRadiiLabel);
  }



  void StereoTool::readSettings() {
    FileName config("$HOME/.Isis/qview/Stereo Tool.config");
    QSettings settings(QString::fromStdString(config.expanded()),
                       QSettings::NativeFormat);
    m_showWarning = settings.value("showWarning", true).toBool();

  }



  void StereoTool::writeSettings() {
    FileName config("$HOME/.Isis/qview/Stereo Tool.config");
    QSettings settings(QString::fromStdString(config.expanded()),
                       QSettings::NativeFormat);
    settings.setValue("showWarning", m_showWarning);

  }
}
