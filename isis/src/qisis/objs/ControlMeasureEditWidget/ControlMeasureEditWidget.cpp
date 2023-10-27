/** This is free and unencumbered software released into the public domain.

The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */

#include "ControlMeasureEditWidget.h"

#include <QApplication>
#include <QButtonGroup>
#include <QCheckBox>
#include <QColor>
#include <QDial>
#include <QDoubleSpinBox>
#include <QFileDialog>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QLCDNumber>
#include <QListWidget>
#include <QMessageBox>
#include <QPalette>
#include <QPushButton>
#include <QRadioButton>
#include <QScrollBar>
#include <QSize>
#include <QString>
#include <QTimer>
#include <QToolButton>

#include "Application.h"
#include "AutoReg.h"
#include "AutoRegFactory.h"
#include "ChipViewport.h"
#include "ControlMeasure.h"
#include "ControlMeasureLogData.h"
#include "ControlPoint.h"
#include "FileName.h"
#include "IString.h"
#include "ProgramLauncher.h"
#include "Pvl.h"
#include "SerialNumberList.h"
#include "UniversalGroundMap.h"

namespace Isis {

  //! Constant representing the length and width of the chip viewports
  const int VIEWSIZE = 301;

  /**
   * Constructs a ControlMeasureEditWidget widget
   *
   * @param parent[in]         Parent of widget
   * @param allowLeftMouse[in] Allow/Disallow mouse events on Left ChipViewport
   * @param useGeometry[in]    Allow/Disallow geometry and rotation on right ChipViewport
   *
   * @author Tracie Sucharski
   * @internal
   *   @history 2008-15-??  Jeannie Walldren - Added error
   *                            string to iException::Message before
   *                            creating QMessageBox
   */
  ControlMeasureEditWidget::ControlMeasureEditWidget(QWidget *parent,
                                                     bool allowLeftMouse,
                                                     bool useGeometry) : QWidget(parent) {

    m_rotation = 0;
    m_timerOn = false;
    m_timerOnRight = false;
    m_autoRegFact = NULL;
    m_allowLeftMouse = allowLeftMouse;
    m_useGeometry = useGeometry;

    //  Initialize some pointers
    m_leftCube = 0;
    m_rightCube = 0;
    m_leftGroundMap = 0;
    m_rightGroundMap = 0;

    m_templateFileName = "$ISISROOT/appdata/templates/autoreg/qnetReg.def";

    createMeasureEditor(parent);
  }


  /**
   * Destructor
   */
  ControlMeasureEditWidget::~ControlMeasureEditWidget () {

    delete m_leftChip;
    m_leftChip = NULL;
    delete m_rightChip;
    m_rightChip = NULL;
  }


  /**
   * Design the MeasureEdit widget
   *
   * @param parent The parent QWidget
   *
   * @author  Tracie Sucharski
   * @internal
   *   @history 2008-11-19  Tracie Sucharski - Added left pan buttons, but
   *                           default to hidden.
   *   @history 2008-12-02  Jeannie Walldren - Allow connection
   *                           between updateLeftView and refreshView for all
   *                           objects.  Previously this was only done if
   *                           allowLeftMouse = true.
   *   @history 2008-12-02  Tracie Sucharski - Another bug fix due to change
   *                           on 2008-11-19, The leftView tackPointChanged
   *                           connection needs to always be made whether mouse
   *                           button events are allowed or not.
   *   @history 2008-12-10  Jeannie Walldren - Set the default
   *                           value of the new private variable,
   *                           m_templateFileName, to the previously hard-coded
   *                           template filename.
   */
  void ControlMeasureEditWidget::createMeasureEditor(QWidget *parent) {
    // Place everything in a grid
    QGridLayout *gridLayout = new QGridLayout();
    gridLayout->setSizeConstraint(QLayout::SetFixedSize);
    //  grid row
    int row = 0;

    QString tempFileName = FileName("$ISISROOT/appdata/images/icons").expanded();
    QString toolIconDir = tempFileName;

    QSize isize(27, 27);
    //  Add zoom buttons
    QToolButton *leftZoomIn = new QToolButton();
    leftZoomIn->setIcon(QPixmap(toolIconDir + "/viewmag+.png"));
    leftZoomIn->setIconSize(isize);
    leftZoomIn->setToolTip("Zoom In 2x");
    leftZoomIn->setWhatsThis("Zoom In 2x on left measure.");

    QToolButton *leftZoomOut = new QToolButton();
    leftZoomOut->setIcon(QPixmap(toolIconDir + "/viewmag-.png"));
    leftZoomOut->setIconSize(isize);
    leftZoomOut->setToolTip("Zoom Out 2x");
    leftZoomOut->setWhatsThis("Zoom Out 2x on left measure.");

    QToolButton *leftZoom1 = new QToolButton();
    leftZoom1->setIcon(QPixmap(toolIconDir + "/viewmag1.png"));
    leftZoom1->setIconSize(isize);
    leftZoom1->setToolTip("Zoom 1:1");
    leftZoom1->setWhatsThis("Show left measure at full resolution.");

    QHBoxLayout *leftZoomPan = new QHBoxLayout;
    leftZoomPan->addWidget(leftZoomIn);
    leftZoomPan->addWidget(leftZoomOut);
    leftZoomPan->addWidget(leftZoom1);

    // These buttons only used if allow mouse events in leftViewport
    QToolButton *leftPanUp = 0;
    QToolButton *leftPanDown = 0;
    QToolButton *leftPanLeft = 0;
    QToolButton *leftPanRight = 0;
    if ( m_allowLeftMouse ) {
      //  Add arrows for panning
      leftPanUp = new QToolButton(parent);
      leftPanUp->setIcon(QIcon(FileName("$ISISROOT/appdata/images/icons/up.png").
                               expanded()));
      leftPanUp->setIconSize(isize);
      leftPanUp->setToolTip("Move up 1 screen pixel");
      leftPanUp->setStatusTip("Move up 1 screen pixel");
      leftPanUp->setWhatsThis("Move the left measure up 1 screen pixel.");

      leftPanDown = new QToolButton(parent);
      leftPanDown->setIcon(QIcon(FileName("$ISISROOT/appdata/images/icons/down.png").
                                 expanded()));
      leftPanDown->setIconSize(isize);
      leftPanDown->setToolTip("Move down 1 screen pixel");
      leftPanDown->setStatusTip("Move down 1 screen pixel");
      leftPanDown->setWhatsThis("Move the left measure down 1 screen pixel.");

      leftPanLeft = new QToolButton(parent);
      leftPanLeft->setIcon(QIcon(FileName("$ISISROOT/appdata/images/icons/back.png").
                                 expanded()));
      leftPanLeft->setIconSize(isize);
      leftPanLeft->setToolTip("Move left 1 screen pixel");
      leftPanLeft->setWhatsThis("Move the left measure to the left by 1 screen"
                                "pixel.");

      leftPanRight = new QToolButton(parent);
      leftPanRight->setIcon(QIcon(FileName("$ISISROOT/appdata/images/icons/forward.png").
                                  expanded()));
      leftPanRight->setIconSize(isize);
      leftPanRight->setToolTip("Move right 1 screen pixel");
      leftPanRight->setWhatsThis("Move the left measure to the right by 1"
                                 "screen pixel.");

      leftZoomPan->addWidget(leftPanUp);
      leftZoomPan->addWidget(leftPanDown);
      leftZoomPan->addWidget(leftPanLeft);
      leftZoomPan->addWidget(leftPanRight);
    }

    leftZoomPan->addStretch();
    gridLayout->addLayout(leftZoomPan, row, 0);

    m_rightZoomIn = new QToolButton();
    m_rightZoomIn->setIcon(QPixmap(toolIconDir + "/viewmag+.png"));
    m_rightZoomIn->setIconSize(isize);
    m_rightZoomIn->setToolTip("Zoom In 2x");
    m_rightZoomIn->setWhatsThis("Zoom In 2x on right measure.");

    m_rightZoomOut = new QToolButton();
    m_rightZoomOut->setIcon(QIcon(FileName("$ISISROOT/appdata/images/icons/viewmag-.png").
                                  expanded()));
    m_rightZoomOut->setIconSize(isize);
    m_rightZoomOut->setToolTip("Zoom Out 2x");
    m_rightZoomOut->setWhatsThis("Zoom Out 2x on right measure.");

    m_rightZoom1 = new QToolButton();
    m_rightZoom1->setIcon(QPixmap(toolIconDir + "/viewmag1.png"));
    m_rightZoom1->setIconSize(isize);
    m_rightZoom1->setToolTip("Zoom 1:1");
    m_rightZoom1->setWhatsThis("Show right measure at full resolution.");

    QHBoxLayout *rightZoomPan = new QHBoxLayout;
    rightZoomPan->addWidget(m_rightZoomIn);
    rightZoomPan->addWidget(m_rightZoomOut);
    rightZoomPan->addWidget(m_rightZoom1);

    //  Add arrows for panning
    QToolButton *rightPanUp = new QToolButton(parent);
    rightPanUp->setIcon(QIcon(FileName("$ISISROOT/appdata/images/icons/up.png").
                              expanded()));
    rightPanUp->setIconSize(isize);
    rightPanUp->setToolTip("Move up 1 screen pixel");
    rightPanUp->setWhatsThis("Move the right measure up 1 screen pixel.");

    QToolButton *rightPanDown = new QToolButton(parent);
    rightPanDown->setIcon(QIcon(FileName("$ISISROOT/appdata/images/icons/down.png").
                                expanded()));
    rightPanDown->setIconSize(isize);
    rightPanDown->setToolTip("Move down 1 screen pixel");
    rightPanUp->setWhatsThis("Move the right measure down 1 screen pixel.");

    QToolButton *rightPanLeft = new QToolButton(parent);
    rightPanLeft->setIcon(QIcon(FileName("$ISISROOT/appdata/images/icons/back.png").
                                expanded()));
    rightPanLeft->setIconSize(isize);
    rightPanLeft->setToolTip("Move left 1 screen pixel");
    rightPanLeft->setWhatsThis("Move the right measure to the left by 1 screen"
                              "pixel.");

    QToolButton *rightPanRight = new QToolButton(parent);
    rightPanRight->setIcon(QIcon(FileName("$ISISROOT/appdata/images/icons/forward.png").
                                 expanded()));
    rightPanRight->setIconSize(isize);
    rightPanRight->setToolTip("Move right 1 screen pixel");
    rightPanRight->setWhatsThis("Move the right measure to the right by 1"
                                "screen pixel.");

    rightZoomPan->addWidget(rightPanUp);
    rightZoomPan->addWidget(rightPanDown);
    rightZoomPan->addWidget(rightPanLeft);
    rightZoomPan->addWidget(rightPanRight);
    rightZoomPan->addStretch();

    gridLayout->addLayout(rightZoomPan, row++, 1);

    //  Add zoom factor label and stretch locking checkbox
    m_leftZoomFactor = new QLabel();
    QCheckBox *leftLockStretch = new QCheckBox("lock stretch");
    // there are two "lock stretch" checkboxes (left and right)
    // use same whats this text for both
    QString whatsThisTextForStretchLocking = "If checked then a new stretch "
        "will NOT be calculated for each pan or zoom change.  Note that stretch"
        " changes made using the stretch tool will ALWAYS take effect, "
        "regardless of the state of this checkbox.";
    leftLockStretch->setWhatsThis(whatsThisTextForStretchLocking);
    QHBoxLayout *leftzflsLayout = new QHBoxLayout;
    leftzflsLayout->addWidget(m_leftZoomFactor);
    leftzflsLayout->addWidget(leftLockStretch);
    gridLayout->addLayout(leftzflsLayout, row, 0);

    m_rightZoomFactor = new QLabel();
    QCheckBox *rightLockStretch = new QCheckBox("lock stretch");
    rightLockStretch->setWhatsThis(whatsThisTextForStretchLocking);
    QHBoxLayout *rightzflsLayout = new QHBoxLayout;
    rightzflsLayout->addWidget(m_rightZoomFactor);
    rightzflsLayout->addWidget(rightLockStretch);
    gridLayout->addLayout(rightzflsLayout, row++, 1);


    m_leftView = new ChipViewport(VIEWSIZE, VIEWSIZE, this);
    //  Do not want to accept mouse/keyboard events
    if ( !m_allowLeftMouse ) m_leftView->setDisabled(true);

    gridLayout->addWidget(m_leftView, row, 0);

    connect(this, SIGNAL(newControlNetwork(ControlNet *)),
            m_leftView, SLOT(setControlNet(ControlNet *)));

    connect(this,
            SIGNAL(stretchChipViewport(Stretch *, CubeViewport *)),
            m_leftView,
            SLOT(stretchFromCubeViewport(Stretch *, CubeViewport *)));

    connect(leftLockStretch, SIGNAL(stateChanged(int)),
            m_leftView,
            SLOT(changeStretchLock(int)));
    leftLockStretch->setChecked(false);


    //  Connect left zoom buttons to ChipViewport's zoom slots
    connect(leftZoomIn, SIGNAL(clicked()), m_leftView, SLOT(zoomIn()));
    connect(leftZoomOut, SIGNAL(clicked()), m_leftView, SLOT(zoomOut()));
    connect(leftZoom1, SIGNAL(clicked()), m_leftView, SLOT(zoom1()));

    //  If zoom on left, need to re-geom right
    connect(leftZoomIn, SIGNAL(clicked()), this, SLOT(updateRightGeom()));
    connect(leftZoomOut, SIGNAL(clicked()), this, SLOT(updateRightGeom()));
    connect(leftZoom1, SIGNAL(clicked()), this, SLOT(updateRightGeom()));

    //  Connect the ChipViewport tackPointChanged signal to
    //  the update sample/line label
    connect(m_leftView, SIGNAL(tackPointChanged(double)),
            this, SLOT(updateLeftPositionLabel(double)));

    // we want to allow this connection so that if a changed point is saved
    // and the same image is showing in both viewports, the left will refresh.
    connect(this, SIGNAL(updateLeftView(double, double)),
            m_leftView, SLOT(refreshView(double, double)));

    connect (m_leftView, SIGNAL(userMovedTackPoint()),
             this, SLOT(colorizeSaveButton()));

    if ( m_allowLeftMouse ) {
      //  Connect pan buttons to ChipViewport
      connect(leftPanUp, SIGNAL(clicked()), m_leftView, SLOT(panUp()));
      connect(leftPanDown, SIGNAL(clicked()), m_leftView, SLOT(panDown()));
      connect(leftPanLeft, SIGNAL(clicked()), m_leftView, SLOT(panLeft()));
      connect(leftPanRight, SIGNAL(clicked()), m_leftView, SLOT(panRight()));

      connect(leftPanUp, SIGNAL(clicked()), this, SLOT(colorizeSaveButton()));
      connect(leftPanDown, SIGNAL(clicked()), this, SLOT(colorizeSaveButton()));
      connect(leftPanLeft, SIGNAL(clicked()), this, SLOT(colorizeSaveButton()));
      connect(leftPanRight, SIGNAL(clicked()), this, SLOT(colorizeSaveButton()));
    }

    m_rightView = new ChipViewport(VIEWSIZE, VIEWSIZE, this);
    gridLayout->addWidget(m_rightView, row, 1);

    connect(this, SIGNAL(newControlNetwork(ControlNet *)),
            m_rightView, SLOT(setControlNet(ControlNet *)));
    connect(this, SIGNAL(stretchChipViewport(Stretch *, CubeViewport *)),
            m_rightView, SLOT(stretchFromCubeViewport(Stretch *, CubeViewport *)));
    connect(rightLockStretch, SIGNAL(stateChanged(int)),
            m_rightView, SLOT(changeStretchLock(int)));
    rightLockStretch->setChecked(false);

    //  Connect the ChipViewport tackPointChanged signal to
    //  the update sample/line label
    connect(m_rightView, SIGNAL(tackPointChanged(double)),
            this, SLOT(updateRightPositionLabel(double)));
    connect(this, SIGNAL(updateRightView(double, double)),
            m_rightView, SLOT(refreshView(double, double)));

    connect (m_rightView, SIGNAL(userMovedTackPoint()),
             this, SLOT(colorizeSaveButton()));

    connect(m_rightZoomIn, SIGNAL(clicked()), m_rightView, SLOT(zoomIn()));
    connect(m_rightZoomOut, SIGNAL(clicked()), m_rightView, SLOT(zoomOut()));
    connect(m_rightZoom1, SIGNAL(clicked()), m_rightView, SLOT(zoom1()));

    //  Connect pan buttons to ChipViewport
    connect(rightPanUp, SIGNAL(clicked()), m_rightView, SLOT(panUp()));
    connect(rightPanDown, SIGNAL(clicked()), m_rightView, SLOT(panDown()));
    connect(rightPanLeft, SIGNAL(clicked()), m_rightView, SLOT(panLeft()));
    connect(rightPanRight, SIGNAL(clicked()), m_rightView, SLOT(panRight()));

    connect(rightPanUp, SIGNAL(clicked()), this, SLOT(colorizeSaveButton()));
    connect(rightPanDown, SIGNAL(clicked()), this, SLOT(colorizeSaveButton()));
    connect(rightPanLeft, SIGNAL(clicked()), this, SLOT(colorizeSaveButton()));
    connect(rightPanRight, SIGNAL(clicked()), this, SLOT(colorizeSaveButton()));

    //  Create chips for left and right
    m_leftChip = new Chip(VIEWSIZE, VIEWSIZE);
    m_rightChip = new Chip(VIEWSIZE, VIEWSIZE);

    QButtonGroup *bgroup = new QButtonGroup();
    m_nogeom = new QRadioButton();
    m_nogeom->setChecked(true);
    connect(m_nogeom, SIGNAL(clicked()), this, SLOT(setNoGeom()));

    QCheckBox *linkZoom = NULL;
    if (m_useGeometry) {
      m_nogeom->setText("No geom/rotate");
      m_nogeom->setToolTip("Reset right measure to it's native geometry.");
      m_nogeom->setWhatsThis("Reset right measure to it's native geometry.  "
                             "If measure was rotated, set rotation back to 0.  "
                             "If measure was geomed to match the left measure, "
                             "reset the geometry back to it's native state.");
      m_geom   = new QRadioButton("Geom");
      m_geom->setToolTip("Geom right measure to match geometry of left measure.");
      m_geom->setWhatsThis("Using an affine transform, geom the right measure to match the "
                           "geometry of the left measure.");
      bgroup->addButton(m_geom);
      connect(m_geom, SIGNAL(clicked()), this, SLOT(setGeom()));
    }
    else {
      linkZoom = new QCheckBox("Link Zoom");
      linkZoom->setToolTip("Link zooming between the left and right views.");
      linkZoom->setWhatsThis("When zooming in the left view, the right view will "
                             "be set to the same zoom factor as the left view.");
      connect(linkZoom, SIGNAL(toggled(bool)), this, SLOT(setZoomLink(bool)));

      m_nogeom->setText("No rotate");
      m_nogeom->setToolTip("Reset right measure to it's native geometry.");
      m_nogeom->setWhatsThis("Reset right measure to it's native geometry.  "
                             "If measure was rotated, set rotation back to 0.");
    }
    bgroup->addButton(m_nogeom);

    QRadioButton *rotate = new QRadioButton("Rotate");
    bgroup->addButton(rotate);
    //  TODO:  ?? Don't think we need this connection
    connect(rotate, SIGNAL(clicked()), this, SLOT(setRotate()));

    //  Set some defaults
    m_geomIt = false;
    m_rotation = 0;
    m_linkZoom = false;

    m_dial = new QDial();
    m_dial->setRange(0, 360);
    m_dial->setWrapping(false);
    m_dial->setNotchesVisible(true);
    m_dial->setNotchTarget(5.);
    m_dial->setEnabled(false);
    m_dial->setToolTip("Rotate right measure");
    m_dial->setWhatsThis("Rotate the right measure by degrees.");

    m_dialNumber = new QLCDNumber();
    m_dialNumber->setEnabled(false);
    m_dialNumber->setToolTip("Rotate right measure");
    m_dialNumber->setWhatsThis("Rotate the right measure by given number"
                               " of degrees.");
    connect(m_dial, SIGNAL(valueChanged(int)), m_dialNumber, SLOT(display(int)));
    connect(m_dial, SIGNAL(valueChanged(int)), m_rightView, SLOT(rotateChip(int)));

    QCheckBox *showPoints = new QCheckBox("Show control points");
    showPoints->setToolTip("Draw control point crosshairs");
    showPoints->setWhatsThis("This will toggle whether crosshairs are drawn"
                     " for the control points located within the measure''s"
                     " view.  For areas of dense measurements, turning this"
                     " off will allow easier viewing of features.");
    connect(showPoints, SIGNAL(toggled(bool)), m_leftView, SLOT(setPoints(bool)));
    connect(showPoints, SIGNAL(toggled(bool)), m_rightView, SLOT(setPoints(bool)));
    showPoints->setChecked(true);

    QCheckBox *cross = new QCheckBox("Show crosshair");
    connect(cross, SIGNAL(toggled(bool)), m_leftView, SLOT(setCross(bool)));
    connect(cross, SIGNAL(toggled(bool)), m_rightView, SLOT(setCross(bool)));
    cross->setChecked(true);
    cross->setToolTip("Show the red crosshair across measure view");
    cross->setWhatsThis("This will toggle whether the crosshair across the"
                        " measure view will be shown");

    QCheckBox *circle = new QCheckBox("Circle");
    circle->setChecked(false);
    circle->setToolTip("Draw circle");
    circle->setWhatsThis("Draw circle on measure view.  This can aid in"
                         " centering a crater under the crosshair.");
    connect(circle, SIGNAL(toggled(bool)), this, SLOT(setCircle(bool)));

    m_slider = new QScrollBar(Qt::Horizontal);
    m_slider->setRange(1, 100);
    m_slider->setSingleStep(1);
    connect(m_slider, SIGNAL(valueChanged(int)), m_leftView, SLOT(setCircleSize(int)));
    connect(m_slider, SIGNAL(valueChanged(int)), m_rightView, SLOT(setCircleSize(int)));
    m_slider->setValue(20);
    m_slider->setDisabled(true);
    m_slider->hide();
    m_slider->setToolTip("Adjust circle size");
    m_slider->setWhatsThis("This allows the cirle size to be adjusted.");

    QVBoxLayout *vlayout = new QVBoxLayout();
    if (!m_useGeometry) {
      vlayout->addWidget(linkZoom);
    }
    vlayout->addWidget(m_nogeom);
    if (m_useGeometry) {
      vlayout->addWidget(m_geom);
    }
    vlayout->addWidget(rotate);
    vlayout->addWidget(m_dial);
    vlayout->addWidget(m_dialNumber);
    vlayout->addWidget(showPoints);
    vlayout->addWidget(cross);
    vlayout->addWidget(circle);
    vlayout->addWidget(m_slider);
    gridLayout->addLayout(vlayout, row++, 2);

    // Show sample / line for measure of chips shown
    m_leftSampLinePosition = new QLabel();
    m_leftSampLinePosition->setToolTip("Sample/Line under the crosshair");
    gridLayout->addWidget(m_leftSampLinePosition, row, 0);
    m_rightSampLinePosition = new QLabel();
    m_rightSampLinePosition->setToolTip("Sample/Line under the crosshair");
    gridLayout->addWidget(m_rightSampLinePosition, row++, 1);

    if (m_useGeometry) {
      //  Show lat / lon for measure of chips shown
      m_leftLatLonPosition = new QLabel();
      m_leftLatLonPosition->setToolTip("Latitude/Longitude under the crosshair");
      gridLayout->addWidget(m_leftLatLonPosition, row, 0);
      m_rightLatLonPosition = new QLabel();
      m_rightLatLonPosition->setToolTip("Latitude/Longitude under the crosshair");
      gridLayout->addWidget(m_rightLatLonPosition, row++, 1);
    }

    //  Add auto registration extension
    m_autoRegExtension = new QWidget;
    m_oldPosition = new QLabel;
    m_oldPosition->setToolTip("Measure Sample/Line before sub-pixel "
                              "registration");
    m_oldPosition->setWhatsThis("Original Sample/Line of the right measure "
            "before the sub-pixel registration.  If you select the \"Undo\" "
            "button, the measure will revert back to this Sample/Line.");
    m_goodFit = new QLabel;
    m_goodFit->setToolTip("Goodness of Fit result from sub-pixel registration.");
    m_goodFit->setWhatsThis("Resulting Goodness of Fit from sub-pixel "
                              "registration.");
    QVBoxLayout *autoRegLayout = new QVBoxLayout;
    autoRegLayout->setMargin(0);
    autoRegLayout->addWidget(m_oldPosition);
    autoRegLayout->addWidget(m_goodFit);
    m_autoRegExtension->setLayout(autoRegLayout);
    m_autoRegShown = false;
    m_autoRegAttempted = false;
    gridLayout->addWidget(m_autoRegExtension, row++, 1);


    QHBoxLayout *leftLayout = new QHBoxLayout();
    QToolButton *stop = new QToolButton();
    stop->setIcon(QPixmap(toolIconDir + "/blinkStop.png"));
    stop->setIconSize(QSize(22, 22));
    stop->setToolTip("Blink Stop");
    QString text = "<b>Function:</b> Stop automatic timed blinking";
    stop->setWhatsThis(text);
    connect(stop, SIGNAL(released()), this, SLOT(blinkStop()));

    QToolButton *start = new QToolButton();
    start->setIcon(QPixmap(toolIconDir + "/blinkStart.png"));
    start->setIconSize(QSize(22, 22));
    start->setToolTip("Blink Start");
    text = "<b>Function:</b> Start automatic timed blinking.  Cycles \
           through linked viewports at variable rate";
    start->setWhatsThis(text);
    connect(start, SIGNAL(released()), this, SLOT(blinkStart()));

    m_blinkTimeBox = new QDoubleSpinBox();
    m_blinkTimeBox->setMinimum(0.1);
    m_blinkTimeBox->setMaximum(5.0);
    m_blinkTimeBox->setDecimals(1);
    m_blinkTimeBox->setSingleStep(0.1);
    m_blinkTimeBox->setValue(0.5);
    m_blinkTimeBox->setToolTip("Blink Time Delay");
    text = "<b>Function:</b> Change automatic blink rate between " +
           QString::number(m_blinkTimeBox->minimum()) + " and " +
           QString::number(m_blinkTimeBox->maximum()) + " seconds";
    m_blinkTimeBox->setWhatsThis(text);
    connect(m_blinkTimeBox, SIGNAL(valueChanged(double)),
            this, SLOT(changeBlinkTime(double)));

    leftLayout->addWidget(stop);
    leftLayout->addWidget(start);
    leftLayout->addWidget(m_blinkTimeBox);

    if (m_useGeometry) {
      QPushButton *find = new QPushButton("Find");
      find->setShortcut(Qt::Key_F);
      find->setToolTip("Move right measure to same Latitude/Longitude as left. "
                       "<strong>Shortcut: F</strong>");
      find->setWhatsThis("Find the Latitude/Longitude under the crosshair in the "
                         "left measure and move the right measure to the same "
                         "latitude/longitude.");
      leftLayout->addWidget(find);
      connect(find, SIGNAL(clicked()), this, SLOT(findPoint()));
    }

    leftLayout->addStretch();
    gridLayout->addLayout(leftLayout, row, 0);

    QHBoxLayout *rightLayout = new QHBoxLayout();
    m_autoReg = new QPushButton("Register");
    m_autoReg->setShortcut(Qt::Key_R);
    m_autoReg->setToolTip("Sub-pixel register the right measure to the left. "
                          "<strong>Shortcut: R</strong>");
    m_autoReg->setWhatsThis("Sub-pixel register the right measure to the left "
                       "and move the result under the crosshair.  After "
                       "viewing the results, the option exists to move the "
                       "measure back to the original position by selecting "
                       "<strong>\"Undo Registration\"</strong>.");
    if (m_allowLeftMouse) {
      m_saveMeasure = new QPushButton("Save Measures");
      m_saveMeasure->setToolTip("Save the both the left and right measure to the edit control "
                                "point (control point currently being edited). "
                                "<strong>Shortcut: M</strong>. "
                                " <strong>Note: The edit control point "
                                "will not be saved to the network until you select "
                                "<strong>\"Save Point\"</strong>");
    }
    else {
      m_saveMeasure = new QPushButton("Save Measure");
      m_saveMeasure->setToolTip("Save the right measure to the edit control "
                                "point (control point currently being edited). "
                                "<strong>Shortcut: M</strong>. "
                                " <strong>Note: The edit control point "
                                "will not be saved to the network until you select "
                                "<strong>\"Save Point\"</strong>");
    }
    m_saveMeasure->setShortcut(Qt::Key_M);
    m_saveDefaultPalette = m_saveMeasure->palette();

    //  Blink extension allows all measures in the current control point to be blinked and gives
    //  user ability to select which measures and the order for blinking
    m_blinkExtension = new QWidget;

    QPushButton *blinkButton = new QPushButton("Advanced Blink");
    blinkButton->setCheckable(true);
    connect(blinkButton, &QAbstractButton::toggled, m_blinkExtension, &QWidget::setVisible);
    connect(blinkButton, SIGNAL(clicked()), this, SLOT(showBlinkExtension()));

    QHBoxLayout *rightBlinkLayout = new QHBoxLayout();
    QToolButton *stopRight = new QToolButton();
    stopRight->setIcon(QPixmap(toolIconDir + "/blinkStop.png"));
    stopRight->setIconSize(QSize(22, 22));
    stopRight->setToolTip("Blink Stop");
    text = "<b>Function:</b> Stop automatic timed blinking";
    stopRight->setWhatsThis(text);
    connect(stopRight, SIGNAL(released()), this, SLOT(blinkStopRight()));

    QToolButton *startRight = new QToolButton();
    startRight->setIcon(QPixmap(toolIconDir + "/blinkStart.png"));
    startRight->setIconSize(QSize(22, 22));
    startRight->setToolTip("Blink Start");
    text = "<b>Function:</b> Start automatic timed blinking.  Cycles \
           through linked viewports at variable rate";
    startRight->setWhatsThis(text);
    connect(startRight, SIGNAL(released()), this, SLOT(blinkStartRight()));

    m_blinkTimeBoxRight = new QDoubleSpinBox();
    m_blinkTimeBoxRight->setMinimum(0.1);
    m_blinkTimeBoxRight->setMaximum(5.0);
    m_blinkTimeBoxRight->setDecimals(1);
    m_blinkTimeBoxRight->setSingleStep(0.1);
    m_blinkTimeBoxRight->setValue(0.5);
    m_blinkTimeBoxRight->setToolTip("Blink Time Delay");
    text = "<b>Function:</b> Change automatic blink rate between " +
           QString::number(m_blinkTimeBox->minimum()) + " and " +
           QString::number(m_blinkTimeBox->maximum()) + " seconds";
    m_blinkTimeBoxRight->setWhatsThis(text);
    connect(m_blinkTimeBoxRight, SIGNAL(valueChanged(double)),
            this, SLOT(changeBlinkTimeRight(double)));

    rightBlinkLayout->addWidget(stopRight);
    rightBlinkLayout->addWidget(startRight);
    rightBlinkLayout->addWidget(m_blinkTimeBoxRight);

    m_blinkListWidget = new QListWidget(m_blinkExtension);
    m_blinkListWidget->setMinimumHeight(100);
    m_blinkListWidget->setSelectionMode(QAbstractItemView::ExtendedSelection);
    m_blinkListWidget->setDragEnabled(true);
    m_blinkListWidget->setAcceptDrops(true);
    m_blinkListWidget->setDropIndicatorShown(true);
    m_blinkListWidget->setDragDropMode(QAbstractItemView::InternalMove);
//  connect(m_blinkListWidget, SIGNAL(itemActivated(QListWidgetItem *)),
//          this, SLOT(toggleBlink(QListWidgetItem *)));
//  connect(m_blinkListWidget, SIGNAL(currentRowChanged(int)),
//          this, SLOT(updateWindow()));

    rightBlinkLayout->addWidget(m_blinkListWidget);

    m_blinkExtension->setLayout(rightBlinkLayout);






    rightLayout->addWidget(m_autoReg);
    rightLayout->addWidget(m_saveMeasure);
    rightLayout->addWidget(blinkButton);
    rightLayout->addStretch();
    gridLayout->addLayout(rightLayout, row++, 1);
    gridLayout->addWidget(m_blinkExtension, row, 1);
    connect(m_autoReg, SIGNAL(clicked()), this, SLOT(registerPoint()));
    connect(m_saveMeasure, SIGNAL(clicked()), this, SLOT(saveMeasure()));

    setLayout(gridLayout);

    //m_pointEditor->setCentralWidget(cw);
    m_autoRegExtension->hide();
    m_blinkExtension->hide();

    //m_pointEditor->setVisible(true);
    //m_pointEditor->raise();
  }


  /**
   * Set the measure displayed in the left ChipViewport
   *
   * @param leftMeasure[in]  Measure displayed in left ChipViewport
   * @param leftCube[in]     Cube of measure displayed in left ChipViewport
   * @param pointId[in]      Control point id associated with the left measure
   *
   * @author Tracie Sucharski
   * @internal
   *   @history 2008-11-19  Tracie Sucharski - If left cube changes, get new
   *                           universalGroundMap.
   *   @history 2012-04-17  Tracie Sucharski - If geom is turned on update the
   *                           right measure.
   *   @history 2012-05-07  Tracie Sucharski - Last change introduced bug when
   *                           loading a different control point, so only
   *                           update the right chip if we're not loading a
   *                           different control point.
   *   @history 2013-04-30  Tracie Sucharski - Fixed bug introduced by linking zooms between left
   *                           and right viewports.  Zoom factors were being passed into the
   *                           Chip::Load method as the second argument which should be the rotation
   *                           value.
   */
  void ControlMeasureEditWidget::setLeftMeasure(ControlMeasure *leftMeasure,
                                        Cube *leftCube, QString pointId) {

    //  Make sure registration is turned off
    if ( m_autoRegShown ) {
      //  Undo Registration
      m_autoRegShown = false;
      m_autoRegExtension->hide();
      m_autoReg->setText("Register");
      m_autoReg->setToolTip("Sub-pixel register the right measure to the left."
                            "<strong>Shortcut: R</strong>");
      m_autoReg->setShortcut(Qt::Key_R);
    }

    m_leftMeasure = leftMeasure;

    if (m_useGeometry) {
      //  get new ground map
      if ( m_leftGroundMap != 0 ) delete m_leftGroundMap;
      m_leftGroundMap = new UniversalGroundMap(*leftCube);
    }
    m_leftCube = leftCube;

    m_leftChip->TackCube(m_leftMeasure->GetSample(), m_leftMeasure->GetLine());
    m_leftChip->Load(*m_leftCube);

    // Dump into left chipViewport
    m_leftView->setChip(m_leftChip, m_leftCube);

    // Only update right if not loading a new point.  If it's a new point, right measure
    // hasn't been loaded yet.
    if (pointId == m_pointId && m_geomIt) updateRightGeom();
    m_pointId = pointId;
  }


  /**
   * Set the tack position of the measure in the left ChipViewport
   *
   * @param sample[in]  Sample of the tack position for the right ChipViewport
   * @param line[in]    Line of the tack position for the left ChipViewport
   *
   */
  void ControlMeasureEditWidget::setLeftPosition(double sample, double line) {

    m_leftChip->TackCube(sample, line);
    emit updateLeftView(sample, line);
  }


  /**
   * Set the tack position of the measure in the right ChipViewport
   *
   * @param sample[in]  Sample of the tack position for the right ChipViewport
   * @param line[in]    Line of the tack position for the left ChipViewport
   *
   */
  void ControlMeasureEditWidget::setRightPosition(double sample, double line) {

    m_rightChip->TackCube(sample, line);
    emit updateRightView(sample, line);
  }


  /**
   * Set the measure displayed in the right ChipViewport
   *
   *
   * @param rightMeasure[in]  Measure displayed in right ChipViewport
   * @param rightCube[in]     Cube of measure displayed in right ChipViewport
   * @param pointId[in]       Control point id associated with the measure
   *
   * @author Tracie Sucharski
   * @internal
   *   @history 2008-11-19  Tracie Sucharski - If right cube changes, get new
   *                           universalGroundMap.
   *   @history 2008-15-?? Jeannie Walldren - Added error string to
   *                           iException::Message before
   *                           creating QMessageBox
   *   @history 2009-09-14  Tracie Sucharski - Call geomChip to make
   *                           sure left chip is initialized in the
   *                           ChipViewport.  This was done for the changes
   *                           made to the Chip class and the ChipViewport
   *                           class where the Cube info is no longer stored.
   *   @history 2013-04-30  Tracie Sucharski - Fixed bug introduced by linking zooms between left
   *                           and right viewports.  Zoom factors were being passed into the
   *                           Chip::Load method as the second argument which should be the rotation
   *                           value.
   *
   */
  void ControlMeasureEditWidget::setRightMeasure(ControlMeasure *rightMeasure,
                                         Cube *rightCube, QString pointId) {

    //  Make sure registration is turned off
    if ( m_autoRegShown ) {
      //  Undo Registration
      m_autoRegShown = false;
      m_autoRegExtension->hide();
      m_autoReg->setText("Register");
      m_autoReg->setShortcut(Qt::Key_R);
      m_autoReg->setToolTip("Sub-pixel register the right measure to the left. "
                            "<strong>Shortcut: R</strong>");
    }
    m_autoRegAttempted = false;

    m_rightMeasure = rightMeasure;
    m_pointId = pointId;

    if (m_useGeometry) {
      //  get new ground map
      if ( m_rightGroundMap != 0 ) delete m_rightGroundMap;
      m_rightGroundMap = new UniversalGroundMap(*rightCube);
    }
    m_rightCube = rightCube;

    m_rightChip->TackCube(m_rightMeasure->GetSample(),
                          m_rightMeasure->GetLine());
    if ( m_geomIt == false ) {
      m_rightChip->Load(*m_rightCube);
    }
    else {
      try {
        m_rightChip->Load(*m_rightCube, *m_leftChip, *m_leftCube);

      }
      catch (IException &e) {
        IException fullError(e, IException::User, "Geom failed.", _FILEINFO_);
        QString message = fullError.toString();
        QMessageBox::information((QWidget *)parent(), "Error", message);
        m_rightChip->Load(*m_rightCube);
        m_geomIt = false;
        m_nogeom->setChecked(true);
        m_geom->setChecked(false);
      }
    }

    // Dump into left chipViewport
    m_rightView->setChip(m_rightChip, m_rightCube);

    updateRightGeom();

    // New right measure, make sure Save Measure Button text is default
    m_saveMeasure->setPalette(m_saveDefaultPalette);
  }


  /**
   * Update sample/line, lat/lon and zoom factor of left measure
   *
   * @param zoomFactor[in]  zoom factor
   *
   * @author Tracie Sucharski
   *
   * @internal
   *   @history 2012-07-26 Tracie Sucharski - Added ability to link zooming between left and
   *                          right viewports.  TODO:  Re-think design, should this be put
   *                          somewhere else.  This was the fastest solution for now.
   *   @history 2017-08-03 Christopher Combs - Added label updates for sample, line, lat and lon
   */
  void ControlMeasureEditWidget::updateLeftPositionLabel(double zoomFactor) {
    QString pos = "Sample: " + QString::number(m_leftView->tackSample()) +
                  "    Line:  " + QString::number(m_leftView->tackLine());
    m_leftSampLinePosition->setText(pos);

    if (m_useGeometry) {
      //  Get lat/lon from point in left
      m_leftGroundMap->SetImage(m_leftView->tackSample(), m_leftView->tackLine());
      double lat = m_leftGroundMap->UniversalLatitude();
      double lon = m_leftGroundMap->UniversalLongitude();

      pos = "Latitude: " + QString::number(lat) +
            "    Longitude:  " + QString::number(lon);
      m_leftLatLonPosition->setText(pos);
    }

    //  Print zoom scale factor
    pos = "Zoom Factor: " + QString::number(zoomFactor);
    m_leftZoomFactor->setText(pos);

    //  If zooms are linked, make right match left
    if (m_linkZoom) {
      m_rightView->zoom(m_leftView->zoomFactor());
    }
  }


  /**
   * Update sample/line, lat/lon and zoom factor of right measure
   *
   * @param zoomFactor[in]  zoom factor
   *
   * @author Tracie Sucharski
   *
   * @internal
   *   @history 2017-08-03 Christopher Combs - Added label updates for sample, line, lat and lon
   */
  void ControlMeasureEditWidget::updateRightPositionLabel(double zoomFactor) {

    // If registration Info is on, turn off
    if (m_autoRegShown) {
      //  Undo Registration
      m_autoRegShown = false;
      m_autoRegExtension->hide();
      m_autoReg->setText("Register");
      m_autoReg->setToolTip("Sub-pixel register the right measure to the left. "
                            "<strong>Shortcut: R</strong>");
      m_autoReg->setShortcut(Qt::Key_R);
    }

    QString pos = "Sample: " + QString::number(m_rightView->tackSample()) +
                  "    Line:  " + QString::number(m_rightView->tackLine());
    m_rightSampLinePosition->setText(pos);

    if (m_useGeometry) {
      //  Get lat/lon from point in right
      m_rightGroundMap->SetImage(m_rightView->tackSample(), m_rightView->tackLine());
      double lat = m_rightGroundMap->UniversalLatitude();
      double lon = m_rightGroundMap->UniversalLongitude();

      pos = "Latitude: " + QString::number(lat) +
            "    Longitude:  " + QString::number(lon);
      m_rightLatLonPosition->setText(pos);
    }

    //  Print zoom scale factor
    pos = "Zoom Factor: " + QString::number(zoomFactor);
    m_rightZoomFactor->setText(pos);

  }


  /**
   * Turn "Save Measure" button text to red
   *
   * @author 2011-06-14 Tracie Sucharski
   */
  void ControlMeasureEditWidget::colorizeSaveButton() {

    QColor qc = Qt::red;
    QPalette p = m_saveMeasure->palette();
    p.setColor(QPalette::ButtonText,qc);
    m_saveMeasure->setPalette(p);

  }


  /**
   * Find point from left ChipViewport in the right ChipViewport
   *
   * @author Tracie Sucharski
   *
   * @history 2011-06-27 Tracie Sucharski - If measure moves to different
   *                        samp/line than saved measure, change save button
   *                        to red.
   */
  void ControlMeasureEditWidget::findPoint() {

    //  Get lat/lon from point in left
    m_leftGroundMap->SetImage(m_leftView->tackSample(), m_leftView->tackLine());
    double lat = m_leftGroundMap->UniversalLatitude();
    double lon = m_leftGroundMap->UniversalLongitude();

    //  Reload right chipViewport with this new tack point.
    if ( m_rightGroundMap->SetUniversalGround(lat, lon) ) {
      emit updateRightView(m_rightGroundMap->Sample(), m_rightGroundMap->Line());

      //  If moving from saved measure, turn save button to red
      if (m_rightGroundMap->Sample() != m_rightMeasure->GetSample() ||
          m_rightGroundMap->Line() != m_rightMeasure->GetLine())
        colorizeSaveButton();
    }
    else {
      QString message = "Latitude: " + QString::number(lat) + "  Longitude: " +
        QString::number(lon) + " is not on the right image. Right measure " +
        "was not moved.";
      QMessageBox::warning((QWidget *)parent(),"Warning",message);
    }

  }


  /**
   * Sub-pixel register point in right chipViewport with point in left.
   *
   * @internal
   *   @history 2008-15-?? Jeannie Walldren - Throw and catch
   *            error before creating QMessageBox
   *   @history 2009-03-23  Tracie Sucharski - Added private m_autoRegAttempted
   *                             for the SaveChips method.
   *   @history 2010-02-16  Tracie Sucharski- If autoreg fails,
   *                             print registration stats.
   *   @history 2010-02-18  Tracie Sucharski - Registration stats wasn't the
   *                             correct info to print.  Instead, check
   *                             registrationStatus and print separate errors
   *                             for each possibility.
   *   @history 2010-02-22  Tracie Sucharski - Added more info for registration
   *                             failures.
   *   @history 2010-06-08  Jeannie Walldren - Catch error and
   *                             warn user if unable to load
   *                             pattern (left) or search (right)
   *                             chip
   *   @history 2010-10-12  Tracie Sucharski - Clean up try/catch blocks.
   *   @history 2011-06-27  Tracie Sucharski - If un-doing registration, change
   *                             save button back to black.  If registration
   *                             successful, change save button to red.
   *   @history 2011-10-21  Tracie Sucharski - Add try/catch around registration
   *                             to catch errors thrown from autoreg class.
   *   @history 2017-04-21 Marjorie Hahn - Added auto registration factory creation.
   *
   */
  void ControlMeasureEditWidget::registerPoint() {

    // if the auto registration factory has not been initialized, do it here
    if (m_autoRegFact == NULL) {
      try {
        Pvl pvl(m_templateFileName.toStdString());
        m_autoRegFact = AutoRegFactory::Create(pvl);
      }
      catch (IException &e) {
        m_autoRegFact = NULL;
        IException fullError(e, IException::Io,
                            "Cannot create AutoRegFactory. As a result, "
                            "sub-pixel registration will not work.",
                            _FILEINFO_);
        QString message = fullError.toString();
        QMessageBox::information((QWidget *)parent(), "Error", message);
        return;
      }
    }

    if ( m_autoRegShown ) {
      //  Undo Registration
      m_autoRegShown = false;
      m_autoRegExtension->hide();
      m_autoReg->setText("Register");
      m_autoReg->setShortcut(Qt::Key_R);
      m_autoReg->setToolTip("Sub-pixel register the right measure to the left. "
                            "<strong>Shortcut: R</strong>");

      //  Reload chip with original measure
      emit updateRightView(m_rightMeasure->GetSample(),
                           m_rightMeasure->GetLine());
      // Since un-doing registration, make sure save button not red
      m_saveMeasure->setPalette(m_saveDefaultPalette);
      return;

    }
    m_autoRegAttempted = true;

    try {
      m_autoRegFact->PatternChip()->TackCube(
                          m_leftMeasure->GetSample(), m_leftMeasure->GetLine());
      m_autoRegFact->PatternChip()->Load(*m_leftCube);
      m_autoRegFact->SearchChip()->TackCube(
                          m_rightMeasure->GetSample(),
                          m_rightMeasure->GetLine());
      if (m_useGeometry) {
        m_autoRegFact->SearchChip()->Load(*m_rightCube,
                            *(m_autoRegFact->PatternChip()), *m_leftCube);
      }
      else {
        m_autoRegFact->SearchChip()->Load(*m_rightCube);
      }
    }
    catch (IException &e) {
      QString msg = "Cannot register this point, unable to Load chips.\n";
      msg += e.toString();
      QMessageBox::information((QWidget *)parent(), "Error", msg);
      return;
    }

    try {
      AutoReg::RegisterStatus status = m_autoRegFact->Register();
      if ( !m_autoRegFact->Success() ) {
        QString msg = "Cannot sub-pixel register this point.\n";
        if ( status == AutoReg::PatternChipNotEnoughValidData ) {
          msg += "\n\nNot enough valid data in Pattern Chip.\n";
          msg += "  PatternValidPercent = ";
          msg += QString::number(m_autoRegFact->PatternValidPercent()) + "%";
        }
        else if ( status == AutoReg::FitChipNoData ) {
          msg += "\n\nNo valid data in Fit Chip.";
        }
        else if ( status == AutoReg::FitChipToleranceNotMet ) {
          msg += "\n\nGoodness of Fit Tolerance not met.\n";
          msg += "\nGoodnessOfFit = " + QString::number(m_autoRegFact->GoodnessOfFit());
          msg += "\nGoodnessOfFitTolerance = ";
          msg += QString::number(m_autoRegFact->Tolerance());
        }
        else if ( status == AutoReg::SurfaceModelNotEnoughValidData ) {
          msg += "\n\nNot enough valid points in the fit chip window for sub-pixel ";
          msg += "accuracy.  Probably too close to edge.\n";
        }
        else if ( status == AutoReg::SurfaceModelSolutionInvalid ) {
          msg += "\n\nCould not model surface for sub-pixel accuracy.\n";
        }
        else if ( status == AutoReg::SurfaceModelDistanceInvalid ) {
          double sampDist, lineDist;
          m_autoRegFact->Distance(sampDist, lineDist);
          msg += "\n\nSub pixel algorithm moves registration more than tolerance.\n";
          msg += "\nSampleMovement = " + QString::number(sampDist) +
                 "    LineMovement = " + QString::number(lineDist);
          msg += "\nDistanceTolerance = " +
                 QString::number(m_autoRegFact->DistanceTolerance());
        }
        else if ( status == AutoReg::PatternZScoreNotMet ) {
          double score1, score2;
          m_autoRegFact->ZScores(score1, score2);
          msg += "\n\nPattern data max or min does not pass z-score test.\n";
          msg += "\nMinimumZScore = " + QString::number(m_autoRegFact->MinimumZScore());
          msg += "\nCalculatedZscores = " + QString::number(score1) + ", " + QString::number(score2);
        }
        else if ( status == AutoReg::AdaptiveAlgorithmFailed ) {
          msg += "\n\nError occured in Adaptive algorithm.";
        }
        else {
          msg += "\n\nUnknown registration error.";
        }

        QMessageBox::information((QWidget *)parent(), "Error", msg);
        return;
      }
    }
    catch (IException &e) {
      QString msg = "Cannot register this point.\n";
      msg += e.toString();
      QMessageBox::information((QWidget *)parent(), "Error", msg);
      return;
    }



    //  Load chip with new registered point
    emit updateRightView(m_autoRegFact->CubeSample(), m_autoRegFact->CubeLine());
    //  If registered pt different from measure, colorize the save button
    if (m_autoRegFact->CubeSample() != m_rightMeasure->GetSample() ||
        m_autoRegFact->CubeLine() != m_rightMeasure->GetLine()) {
      colorizeSaveButton();
    }

    QString oldPos = "Original Sample: " +
                     QString::number(m_rightMeasure->GetSample()) + "   Original Line:  " +
                     QString::number(m_rightMeasure->GetLine());
    m_oldPosition->setText(oldPos);

    QString goodFit = "Goodness of Fit:  " +
                      QString::number(m_autoRegFact->GoodnessOfFit());
    m_goodFit->setText(goodFit);

    m_autoRegExtension->show();
    m_autoRegShown = true;
    m_autoReg->setText("Undo Registration");
    m_autoReg->setToolTip("Undo sub-pixel registration. "
                          "<strong>Shortcut: U</strong>");
    m_autoReg->setShortcut(Qt::Key_U);
  }


  /**
   * Save control measure under the crosshair in right ChipViewport
   *
   * @internal
   *   @history 2008-12-30 Jeannie Walldren - Modified to update
   *                          user (chooser) name and date when
   *                          point is saved
   *   @history 2010-11-19 Tracie Sucharski - Renamed from savePoint.
   *   @history 2011-03-04 Tracie Sucharski - If auto reg info is shown, save
   *                          the GoodnessOfFit value to the right
   *                          ControlMeasure log entry.  Changed the way
   *                          the left viewport is updated if the left and
   *                          right measure are the same.  Simply copy the
   *                          right measure into the left and re-load the left
   *                          measure.  If measure currently has type of
   *                          subPixelRegistered, but the new position has
   *                          not be sub-pixel registered, change measure type
   *                          and get rid of goodness of fit.
   *   @history 2011-06-14 Tracie Sucharski - Change Save Measure button text
   *                          back to black.
   *   @history 2011-07-19 Tracie Sucharski - Updated for new functionality
   *                          of registration pixel shift.  ControlMeasure
   *                          will now calculate based on aprioriSample/Line
   *                          and current coordinate.  If autoreg has been
   *                          calculated, save coordinate to apriori before
   *                          updating to subpixel registered coordinate.
   *   @history 2013-11-07 Tracie Sucharski - Moved error checking on edit locked measures from
   *                          QnetTool::measureSaved to ::saveMeasure.  The error checking now
   *                          forces the edit lock check box to be unchecked before the measure
   *                          can be saved.
   *   @history 2015-01-09 Ian Humphrey - Modified to prevent segmentation fault that arises when
   *                           registering, opening a template file, and saving the measure. This
   *                           was caused by not handling the exception thrown by
   *                           ControlMeasure::SetLogData(), which produces undefined behavior
   *                           within the Qt signal-slot connection mechanism.
   *
   */
  void ControlMeasureEditWidget::saveMeasure() {

    if ( m_rightMeasure != NULL ) {

      if (m_rightMeasure->IsEditLocked()) {
        QString message = "The right measure is locked.  You must first unlock the measure by ";
        message += "clicking the check box above labeled \"Edit Lock Measure\".";
        QMessageBox::warning((QWidget *)parent(),"Warning",message);
        return;
      }

      if ( m_autoRegShown ) {
        try {
          //  Save  autoreg parameters to the right measure log entry
          //  Eccentricity may be invalid, check before writing.
          m_rightMeasure->SetLogData(ControlMeasureLogData(
                                     ControlMeasureLogData::GoodnessOfFit,
                                     m_autoRegFact->GoodnessOfFit()));
          double minZScore, maxZScore;
          m_autoRegFact->ZScores(minZScore,maxZScore);
          m_rightMeasure->SetLogData(ControlMeasureLogData(
                                     ControlMeasureLogData::MinimumPixelZScore,
                                     minZScore));
          m_rightMeasure->SetLogData(ControlMeasureLogData(
                                     ControlMeasureLogData::MaximumPixelZScore,
                                     maxZScore));
        }
        // need to handle exception that SetLogData throws if our data is invalid -
        // unhandled exceptions thrown in Qt signal and slot connections produce undefined behavior
        catch (IException &e) {
          QString message = e.toString();
          QMessageBox::critical((QWidget *)parent(), "Error", message);
          return;
        }

        //  Reset AprioriSample/Line to the current coordinate, before the
        //  coordinate is updated with the registered coordinate.
        m_rightMeasure->SetAprioriSample(m_rightMeasure->GetSample());
        m_rightMeasure->SetAprioriLine(m_rightMeasure->GetLine());

        m_rightMeasure->SetChooserName("Application qnet");
        m_rightMeasure->SetType(ControlMeasure::RegisteredSubPixel);

        m_autoRegShown = false;
        m_autoRegExtension->hide();
        m_autoReg->setText("Register");
        m_autoReg->setToolTip("Sub-pixel register the right measure to the left. "
                              "<strong>Shortcut: R</strong>");
        m_autoReg->setShortcut(Qt::Key_R);
      }
      else {
        m_rightMeasure->SetChooserName(Application::UserName());
        m_rightMeasure->SetType(ControlMeasure::Manual);
        m_rightMeasure->DeleteLogData(
                               ControlMeasureLogData::GoodnessOfFit);
        m_rightMeasure->DeleteLogData(
                               ControlMeasureLogData::MinimumPixelZScore);
        m_rightMeasure->DeleteLogData(
                               ControlMeasureLogData::MaximumPixelZScore);
      }

      //  Get cube position at right chipViewport crosshair
      m_rightMeasure->SetCoordinate(m_rightView->tackSample(),
                                    m_rightView->tackLine());
      m_rightMeasure->SetDateTime();

    }

    if ( m_allowLeftMouse ) {
      if ( m_leftMeasure != NULL ) {
        if (m_leftMeasure->IsEditLocked()) {
          QString message = "The left measure is locked.  You must first unlock the measure by ";
          message += "clicking the check box above labeled \"Edit Lock Measure\".";
          QMessageBox::warning((QWidget *)parent(),"Warning",message);
          return;
        }

        m_leftMeasure->SetCoordinate(m_leftView->tackSample(), m_leftView->tackLine());
        m_leftMeasure->SetDateTime();
        m_leftMeasure->SetChooserName(Application::UserName());
        m_leftMeasure->SetType(ControlMeasure::Manual);
      }
    }

    //  If the right chip is the same as the left chip, copy right into left and
    //  re-load the left.
    if ( m_rightMeasure->GetCubeSerialNumber() ==
         m_leftMeasure->GetCubeSerialNumber() ) {

      *m_leftMeasure = *m_rightMeasure;
      setLeftMeasure(m_leftMeasure,m_leftCube,m_pointId);
    }

    //  Change Save Measure button text back to default palette
    m_saveMeasure->setPalette(m_saveDefaultPalette);

    //  Redraw measures on viewports
    emit measureSaved();
  }


  /**
   * Slot to update the geomed right ChipViewport for zoom operations
   *
   * @internal
   *   @history 2008-15-?? Jeannie Walldren - Added error string to
   *                            iException::Message before
   *                            creating QMessageBox
   */
  void ControlMeasureEditWidget::updateRightGeom() {

    if ( m_geomIt ) {
      try {
        m_rightView->geomChip(m_leftChip, m_leftCube);

      }
      catch (IException &e) {
        IException fullError(e, IException::User, "Geom failed.", _FILEINFO_);
        QString message = fullError.toString();
        QMessageBox::information((QWidget *)parent(), "Error", message);
        m_geomIt = false;
        m_nogeom->setChecked(true);
        m_geom->setChecked(false);
      }
    }
  }



///**
// * Slot to update the right ChipViewport for zoom
// * operations
// *
// * @author 2012-07-26 Tracie Sucharski
// *
// * @internal
// */
//void ControlMeasureEditWidget::updateRightZoom() {
//
//  if ( m_linkZoom ) {
//    try {
//      m_rightView->geomChip(m_leftChip, m_leftCube);
//
//    }
//    catch (IException &e) {
//      IException fullError(e, IException::User, "Geom failed.", _FILEINFO_);
//      QString message = fullError.toString();
//      QMessageBox::information((QWidget *)parent(), "Error", message);
//      m_geomIt = false;
//      m_nogeom->setChecked(true);
//      m_geom->setChecked(false);
//    }
//  }
//}


  /**
   * Slot to enable the rotate dial
   *
   * @internal
   *   @history 2012-05-01 Tracie Sucharski - Reset zoom buttons and reload chip
  */
  void ControlMeasureEditWidget::setRotate() {
    QApplication::setOverrideCursor(Qt::WaitCursor);

    //  Text needs to be reset because it was changed to
    //  indicate why it's greyed out
    QString text = "Zoom in 2X";
    m_rightZoomIn->setEnabled(true);
    m_rightZoomIn->setWhatsThis(text);
    m_rightZoomIn->setToolTip("Zoom In");
    text = "Zoom out 2X";
    m_rightZoomOut->setEnabled(true);
    m_rightZoomOut->setWhatsThis(text);
    m_rightZoomOut->setToolTip("Zoom Out");
    text = "Zoom 1:1";
    m_rightZoom1->setEnabled(true);
    m_rightZoom1->setWhatsThis(text);
    m_rightZoom1->setToolTip("Zoom 1:1");

    m_geomIt = false;
    m_rightView->nogeomChip();

    QApplication::restoreOverrideCursor();

    m_dial->setEnabled(true);
    m_dialNumber->setEnabled(true);
    m_dial->setNotchesVisible(true);

  }


  /**
   * Turn geom on
   *
   * @internal
   *   @history  2007-06-15 Tracie Sucharski - Grey out zoom buttons
   *   @history 2008-15-?? Jeannie Walldren - Added error string to
   *                            iException::Message before
   *                            creating QMessageBox
   */
  void ControlMeasureEditWidget::setGeom() {

    if ( m_geomIt == true ) return;

    QApplication::setOverrideCursor(Qt::WaitCursor);

    //  Grey right view zoom buttons
    QString text = "Zoom functions disabled when Geom is set";
    m_rightZoomIn->setEnabled(false);
    m_rightZoomIn->setWhatsThis(text);
    m_rightZoomIn->setToolTip(text);
    m_rightZoomOut->setEnabled(false);
    m_rightZoomOut->setWhatsThis(text);
    m_rightZoomOut->setToolTip(text);
    m_rightZoom1->setEnabled(false);
    m_rightZoom1->setWhatsThis(text);
    m_rightZoom1->setToolTip(text);


    //  Reset dial to 0 before disabling
    m_dial->setValue(0);
    m_dial->setEnabled(false);
    m_dialNumber->setEnabled(false);

    m_geomIt = true;

    try {
      m_rightView->geomChip(m_leftChip, m_leftCube);

    }
    catch (IException &e) {
      IException fullError(e, IException::User, "Geom failed.", _FILEINFO_);
      QString message = fullError.toString();
      QMessageBox::information((QWidget *)parent(), "Error", message);
      m_geomIt = false;
      m_nogeom->setChecked(true);
      m_geom->setChecked(false);
    }

    QApplication::restoreOverrideCursor();
  }


  /**
   * Slot to turn off geom
   *
   * @internal
   *   @history 2012-05-01 Tracie Sucharski - Reset zoom buttons and rotate dial, then reload chip
  */
  void ControlMeasureEditWidget::setNoGeom() {

    QApplication::setOverrideCursor(Qt::WaitCursor);

    QString text = "Zoom in 2X";
    m_rightZoomIn->setEnabled(true);
    m_rightZoomIn->setWhatsThis(text);
    m_rightZoomIn->setToolTip("Zoom In");
    text = "Zoom out 2X";
    m_rightZoomOut->setEnabled(true);
    m_rightZoomOut->setWhatsThis(text);
    m_rightZoomOut->setToolTip("Zoom Out");
    text = "Zoom 1:1";
    m_rightZoom1->setEnabled(true);
    m_rightZoom1->setWhatsThis(text);
    m_rightZoom1->setToolTip("Zoom 1:1");

    //  Reset dial to 0 before disabling
    m_dial->setValue(0);
    m_dial->setEnabled(false);
    m_dialNumber->setEnabled(false);

    m_geomIt = false;
    m_rightView->nogeomChip();

    QApplication::restoreOverrideCursor();
  }


  /**
   * Turn circle widgets on/off
   *
   * @param checked[in] Turn cirle on or off
   *
   * @author 2008-09-09 Tracie Sucharski
   */
  void ControlMeasureEditWidget::setCircle(bool checked) {

    if ( checked == m_circle ) return;

    m_circle = checked;
    if ( m_circle ) {
      // Turn on slider bar
      m_slider->setDisabled(false);
      m_slider->show();
      m_slider->setValue(20);
      m_leftView->setCircle(true);
      m_rightView->setCircle(true);
    }
    else {
      m_slider->setDisabled(true);
      m_slider->hide();
      m_leftView->setCircle(false);
      m_rightView->setCircle(false);
    }
  }


  /**
   * Turn linking of zoom on or off
   *
   * @param checked[in] Turn zoom linking on or off
   *
   * @author 2012-07-26 Tracie Sucharski
   */
  void ControlMeasureEditWidget::setZoomLink(bool checked) {

    if ( checked == m_linkZoom ) return;

    m_linkZoom = checked;
    if ( m_linkZoom ) {
      m_rightView->zoom(m_leftView->zoomFactor());
    }
  }


  /**
   * Slot to start blink function
   */
  void ControlMeasureEditWidget::blinkStart() {
    if ( m_timerOn ) return;

    //  Set up blink list
    m_blinkList.push_back(m_leftView);
    m_blinkList.push_back(m_rightView);
    m_blinkIndex = 0;

    m_timerOn = true;
    int msec = (int)(m_blinkTimeBox->value() * 1000.0);
    m_timer = new QTimer(this);
    connect(m_timer, SIGNAL(timeout()), this, SLOT(updateBlink()));
    m_timer->start(msec);
  }


  /**
   * Slot to stop blink function
   */
  void ControlMeasureEditWidget::blinkStop() {
    m_timer->stop();
    m_timerOn = false;
    m_blinkList.clear();

    //  Reload left chipViewport with original chip
    m_leftView->repaint();

  }


  /**
   * Set blink rate
   *
   * @param interval[in] Blink rate in seconds
   * @author  Tracie Sucharski
   */
  void ControlMeasureEditWidget::changeBlinkTime(double interval) {
    if ( m_timerOn ) m_timer->setInterval((int)(interval * 1000.));
  }


  /**
   * Slot to cause the blink to happen coinciding with the timer
   */
  void ControlMeasureEditWidget::updateBlink() {

    m_blinkIndex = !m_blinkIndex;
    m_leftView->loadView(*(m_blinkList)[m_blinkIndex]);
  }


  /**
   * Allows user to choose a new template file by opening a window
   * from which to select a filename.  This file is then
   * registered and set as the new template.
   *
   * @param fn Template filename
   *
   * @author Tracie Sucharski
   * @internal
   *   @history 2008-12-10 Jeannie Walldren - Changed name from
   *                          openTemplateFile() and added functionality to set
   *                          new template filename to the private variable,
   *                          m_templateFileName
   *   @history 2008-12-15 Jeannie Walldren - Modified code to
   *                          only allow the template file to be modified if
   *                          registration is successfull, otherwise the
   *                          original template file is kept.
   */
  bool ControlMeasureEditWidget::setTemplateFile(QString fn) {

    AutoReg *reg = NULL;
    // save original template filename
    QString temp = m_templateFileName;
    try {
      // set template filename to user chosen pvl file
      m_templateFileName = fn;

      // Create PVL object with this file
      Pvl pvl(fn.toStdString());

      // try to register file
      reg = AutoRegFactory::Create(pvl);
      if ( m_autoRegFact != NULL )
        delete m_autoRegFact;
      m_autoRegFact = reg;

      m_templateFileName = fn;
      return true;
    }
    catch (IException &e) {
      // set templateFileName back to its original value
      m_templateFileName = temp;
      IException fullError(e, IException::Io,
          "Cannot create AutoRegFactory for " +
          fn +
          ".  As a result, current template file will remain set to " +
          m_templateFileName, _FILEINFO_);
      QString message = fullError.toString();
      QMessageBox::information((QWidget *)parent(), "Error", message);
      emit setTemplateFailed(m_templateFileName);
      return false;
    }
  }


  /**
   * Set the option that allows mouse movements in the left ChipViewport.
   *
   * @param allowMouse Whether or not to allow mouse event on left chip viewport
   *
   * @author Tracie Sucharski
   */
  void ControlMeasureEditWidget::allowLeftMouse(bool allowMouse) {
    m_allowLeftMouse = allowMouse;

    if (m_allowLeftMouse) {
      m_saveMeasure = new QPushButton("Save Measures");
      m_saveMeasure->setToolTip("Save the both the left and right measure to the edit control "
                                "point (control point currently being edited). "
                                " <strong>Note: The edit control point "
                                "will not be saved to the network until you select "
                                "<strong>\"Save Point\"</strong>");
    }
    else {
      m_saveMeasure = new QPushButton("Save Measure");
      m_saveMeasure->setToolTip("Save the right measure to the edit control "
                                "point (control point currently being edited). "
                                " <strong>Note: The edit control point "
                                "will not be saved to the network until you select "
                                "<strong>\"Save Point\"</strong>");
    }
  }


  void ControlMeasureEditWidget::refreshChips() {
    m_leftView->update();
    m_rightView->update();
  }


  /**
   * Slot to save registration chips to files and fire off qview.
   *
   * @author 2009-03-18  Tracie Sucharski
   *
   * @internal
   * @history 2009-03-23 Tracie Sucharski - Use m_autoRegAttempted to see
   *                        if chips exist.
   */
  void ControlMeasureEditWidget::saveChips() {

//    if (!m_autoRegShown) {
    if ( !m_autoRegAttempted ) {
      QString message = "Point must be Registered before chips can be saved.";
      QMessageBox::warning((QWidget *)parent(), "Warning", message);
      return;
    }

    //  Save chips - pattern, search and fit
    QString baseFile = m_pointId.replace(" ", "_") + "_" +
                           toString((int)(m_leftMeasure ->GetSample())) + "_" +
                           toString((int)(m_leftMeasure ->GetLine()))   + "_" +
                           toString((int)(m_rightMeasure->GetSample())) + "_" +
                           toString((int)(m_rightMeasure->GetLine()))   + "_";
    QString fname = baseFile + "Search.cub";
    QString command = "$ISISROOT/bin/qview \'" + fname + "\'";
    m_autoRegFact->RegistrationSearchChip()->Write(fname);
    fname = baseFile + "Pattern.cub";
    command += " \'" + fname + "\'";
    m_autoRegFact->RegistrationPatternChip()->Write(fname);
    fname = baseFile + "Fit.cub";
    command += " \'" + fname + "\' &";
    m_autoRegFact->FitChip()->Write(fname);
    ProgramLauncher::RunSystemCommand(command);
  }


  /**
   * Set the Control Point for this widget
   *
   * @param editPoint[in]  ControlPoint for this widget
   * @param snList[in]     SerialNumberList associated with the control net containing the ControlPoint
   *
   */
  void ControlMeasureEditWidget::setPoint(ControlPoint *editPoint, SerialNumberList *snList) {

    m_editPoint = editPoint;
    m_serialNumberList = snList;


    m_blinkListWidget->clear();
  }


  void ControlMeasureEditWidget::showBlinkExtension() {
    m_blinkListWidget->clear();
    //  Get all measure filenames for ListWidget
    for (int i=0; i<m_editPoint->GetNumMeasures(); i++) {
      ControlMeasure &m = *(*m_editPoint)[i];
      QString file = m_serialNumberList->fileName(m.GetCubeSerialNumber());
      // TODO  Ipce TLS Look at QnetNavTool for how selectedItems is used so don't need map
      //  between full cubename and base name.
//    QString tempFileName = FileName(file).name();
      m_blinkListWidget->addItem(file);



//    if (m_editPoint->IsReferenceExplicit() &&
//        (QString)m.GetCubeSerialNumber() == m_editPoint->GetReferenceSN()) {
//      m_blinkListWidget->setItemData(i,QFont("DejaVu Sans", 12, QFont::Bold), Qt::FontRole);
//    }
    }
  }


  //!  Slot to start blink function for advanced blink functionality
  void ControlMeasureEditWidget::blinkStartRight() {

    if ( m_timerOnRight ) return;

    //  Set up blink list.  Create ChipViewport for each cube active in the ListWidget, using the
    //  correct zoom and geom selections
    QList<QListWidgetItem *> selected = m_blinkListWidget->selectedItems();
    if (selected.size() < 1) {
      QMessageBox::information((QWidget *)parent(), "Error", "No files selected for blinking.");
      return;
    }

    //  Find measure for each selected file, create cube, chip and chipViewport
    for (int i=0; i<selected.size(); i++) {
      QString file = selected.at(i)->text();
      QString serial = m_serialNumberList->serialNumber(file);
      Cube *blinkCube = new Cube(selected.at(i)->text());
      Chip *blinkChip = new Chip(VIEWSIZE, VIEWSIZE);
      ControlMeasure *blinkMeasure = m_editPoint->GetMeasure(serial);
      blinkChip->TackCube(blinkMeasure->GetSample(), blinkMeasure->GetLine());
      blinkChip->Load(*blinkCube);
      ChipViewport *blinkViewport = new ChipViewport(VIEWSIZE, VIEWSIZE, this);
      blinkViewport->setChip(blinkChip, blinkCube);
      if (m_geomIt) {
        blinkViewport->geomChip(m_leftChip, m_leftCube);
      }
      else {
        blinkViewport->zoom(m_leftView->zoomFactor());
      }
      m_blinkChipViewportListRight.append(blinkViewport);
    }

    m_blinkIndexRight = 0;

    m_timerOnRight = true;
    int msec = (int)(m_blinkTimeBoxRight->value() * 1000.0);
    m_timerRight = new QTimer(this);
    connect(m_timerRight, SIGNAL(timeout()), this, SLOT(updateBlinkRight()));
    m_timerRight->start(msec);
  }


  //!  Slot to stop blink function
  void ControlMeasureEditWidget::blinkStopRight() {

    m_timerRight->stop();
    m_timerOnRight = false;
    m_blinkChipViewportListRight.clear();
    //  Reload left chipViewport with original chip
    m_rightView->repaint();
  }


  /**
   * Set blink rate
   *
   * @param interval   Input   Blink rate in seconds
   * @author  Tracie Sucharski
   */
  void ControlMeasureEditWidget::changeBlinkTimeRight(double interval) {
    if ( m_timerOnRight ) m_timerRight->setInterval((int)(interval * 1000.));
  }


  //!  Slot to cause the blink to happen coinciding with the timer
  void ControlMeasureEditWidget::updateBlinkRight() {

    m_blinkIndexRight++;
    if (m_blinkIndexRight > m_blinkChipViewportListRight.size()-1) {
      m_blinkIndexRight = 0;
    }
    m_rightView->loadView(*(m_blinkChipViewportListRight)[m_blinkIndexRight]);
  }
}
