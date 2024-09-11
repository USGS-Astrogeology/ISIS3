/** This is free and unencumbered software released into the public domain.

The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */

#include "ControlPointEdit.h"

//TEST
#include <QDebug>

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
#include "FileName.h"
#include "IString.h"
#include "ProgramLauncher.h"
#include "Pvl.h"
#include "UniversalGroundMap.h"

namespace Isis {

  const int VIEWSIZE = 301;

  /**
   * Constructs a ControlPointEdit widget
   *
   * @param parent           Input  Parent of widget
   * @param allowLeftMouse   Input  Allow/Disallow mouse events on Left
   *                                    ChipViewport
   * @author Tracie Sucharski
   * @internal
   *   @history 2008-15-2008 Jeannie Walldren - Added error
   *                           string to iException::Message before
   *                           creating QMessageBox
   *   @history 2017-04-21 Marjorie Hahn - Moved p_autoRegFact creation
   *                           from constructor to registerPoint() method.
   */
  ControlPointEdit::ControlPointEdit(ControlNet * cnet, QWidget *parent,
                                     bool allowLeftMouse, bool useGeometry) : QWidget(parent) {

    p_rotation = 0;
    p_timerOn = false;
    p_autoRegFact = NULL;
    p_allowLeftMouse = allowLeftMouse;
    p_useGeometry = useGeometry;

    //  Initialize some pointers
    p_leftCube = 0;
    p_rightCube = 0;
    p_leftGroundMap = 0;
    p_rightGroundMap = 0;

    p_templateFileName = "$ISISROOT/appdata/templates/autoreg/qnetReg.def";

    createPointEditor(parent);
    if (cnet != NULL) emit newControlNetwork(cnet);
  }


  ControlPointEdit::~ControlPointEdit() {

    delete p_leftChip;
    p_leftChip = NULL;
    delete p_rightChip;
    p_rightChip = NULL;
  }


  /**
   * Design the PointEdit widget
   *
   * @author  Tracie Sucharski
   * @internal
   *   @history 2008-11-19 Tracie Sucharski - Added left pan buttons, but
   *                           default to hidden.
   *   @history 2008-12-02 Jeannie Walldren - Allow connection
   *                           between updateLeftView and refreshView for all
   *                           objects.  Previously this was only done if
   *                           allowLeftMouse = true.
   *   @history 2008-12-02 Tracie Sucharski - Another bug fix due to change
   *                           on 2008-11-19, The leftView tackPointChanged
   *                           connection needs to always be made whether mouse
   *                           button events are allowed or not.
   *   @history 2008-12-10 Jeannie Walldren - Set the default
   *                           value of the new private variable,
   *                           p_templateFileName, to the previously hard-coded
   *                           template filename.
   *   @history 2015-10-29 Ian Humphrey - Added shortcuts for Find (F), Register (R),
   *                           Undo Registration (U), and Save Measure (M) buttons.
   *                           Fixes #2324.
   */
  void ControlPointEdit::createPointEditor(QWidget *parent) {
    // Place everything in a grid
    QGridLayout *gridLayout = new QGridLayout();
    gridLayout->setSizeConstraint(QLayout::SetFixedSize);
    //  grid row
    int row = 0;

    QString tempFileName = QString::fromStdString(FileName("$ISISROOT/appdata/images/icons").expanded());
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
    if (p_allowLeftMouse) {
      // Add arrows for panning
      leftPanUp = new QToolButton(parent);
      leftPanUp->setIcon(QIcon(QString::fromStdString(FileName("$ISISROOT/appdata/images/icons/up.png").
                               expanded())));
      leftPanUp->setIconSize(isize);
      leftPanUp->setToolTip("Move up 1 screen pixel");
      leftPanUp->setStatusTip("Move up 1 screen pixel");
      leftPanUp->setWhatsThis("Move the left measure up 1 screen pixel.");

      leftPanDown = new QToolButton(parent);
      leftPanDown->setIcon(QIcon(QString::fromStdString(FileName("$ISISROOT/appdata/images/icons/down.png").
                                 expanded())));
      leftPanDown->setIconSize(isize);
      leftPanDown->setToolTip("Move down 1 screen pixel");
      leftPanDown->setStatusTip("Move down 1 screen pixel");
      leftPanDown->setWhatsThis("Move the left measure down 1 screen pixel.");

      leftPanLeft = new QToolButton(parent);
      leftPanLeft->setIcon(QIcon(QString::fromStdString(FileName("$ISISROOT/appdata/images/icons/back.png").
                                 expanded())));
      leftPanLeft->setIconSize(isize);
      leftPanLeft->setToolTip("Move left 1 screen pixel");
      leftPanLeft->setWhatsThis("Move the left measure to the left by 1 screen"
                                "pixel.");

      leftPanRight = new QToolButton(parent);
      leftPanRight->setIcon(QIcon(QString::fromStdString(FileName("$ISISROOT/appdata/images/icons/forward.png").
                                  expanded())));
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

    p_rightZoomIn = new QToolButton();
    p_rightZoomIn->setIcon(QPixmap(toolIconDir + "/viewmag+.png"));
    p_rightZoomIn->setIconSize(isize);
    p_rightZoomIn->setToolTip("Zoom In 2x");
    p_rightZoomIn->setWhatsThis("Zoom In 2x on right measure.");

    p_rightZoomOut = new QToolButton();
    p_rightZoomOut->setIcon(QIcon(QString::fromStdString(FileName("$ISISROOT/appdata/images/icons/viewmag-.png").
                                  expanded())));
    p_rightZoomOut->setIconSize(isize);
    p_rightZoomOut->setToolTip("Zoom Out 2x");
    p_rightZoomOut->setWhatsThis("Zoom Out 2x on right measure.");

    p_rightZoom1 = new QToolButton();
    p_rightZoom1->setIcon(QPixmap(toolIconDir + "/viewmag1.png"));
    p_rightZoom1->setIconSize(isize);
    p_rightZoom1->setToolTip("Zoom 1:1");
    p_rightZoom1->setWhatsThis("Show right measure at full resolution.");

    QHBoxLayout *rightZoomPan = new QHBoxLayout;
    rightZoomPan->addWidget(p_rightZoomIn);
    rightZoomPan->addWidget(p_rightZoomOut);
    rightZoomPan->addWidget(p_rightZoom1);

    //  Add arrows for panning
    QToolButton *rightPanUp = new QToolButton(parent);
    rightPanUp->setIcon(QIcon(QString::fromStdString(FileName("$ISISROOT/appdata/images/icons/up.png").
                              expanded())));
    rightPanUp->setIconSize(isize);
    rightPanUp->setToolTip("Move up 1 screen pixel");
    rightPanUp->setWhatsThis("Move the right measure up 1 screen pixel.");

    QToolButton *rightPanDown = new QToolButton(parent);
    rightPanDown->setIcon(QIcon(QString::fromStdString(FileName("$ISISROOT/appdata/images/icons/down.png").
                                expanded())));
    rightPanDown->setIconSize(isize);
    rightPanDown->setToolTip("Move down 1 screen pixel");
    rightPanUp->setWhatsThis("Move the right measure down 1 screen pixel.");

    QToolButton *rightPanLeft = new QToolButton(parent);
    rightPanLeft->setIcon(QIcon(QString::fromStdString(FileName("$ISISROOT/appdata/images/icons/back.png").
                                expanded())));
    rightPanLeft->setIconSize(isize);
    rightPanLeft->setToolTip("Move left 1 screen pixel");
    rightPanLeft->setWhatsThis("Move the right measure to the left by 1 screen"
                              "pixel.");

    QToolButton *rightPanRight = new QToolButton(parent);
    rightPanRight->setIcon(QIcon(QString::fromStdString(FileName("$ISISROOT/appdata/images/icons/forward.png").
                                 expanded())));
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
    p_leftZoomFactor = new QLabel();
    QCheckBox *leftLockStretch = new QCheckBox("lock stretch");
    // there are two "lock stretch" checkboxes (left and right)
    // use same whats this text for both
    QString whatsThisTextForStretchLocking = "If checked then a new stretch "
        "will NOT be calculated for each pan or zoom change.  Note that stretch"
        " changes made using the stretch tool will ALWAYS take effect, "
        "regardless of the state of this checkbox.";
    leftLockStretch->setWhatsThis(whatsThisTextForStretchLocking);
    QHBoxLayout *leftzflsLayout = new QHBoxLayout;
    leftzflsLayout->addWidget(p_leftZoomFactor);
    leftzflsLayout->addWidget(leftLockStretch);
    gridLayout->addLayout(leftzflsLayout, row, 0);

    p_rightZoomFactor = new QLabel();
    QCheckBox *rightLockStretch = new QCheckBox("lock stretch");
    rightLockStretch->setWhatsThis(whatsThisTextForStretchLocking);
    QHBoxLayout *rightzflsLayout = new QHBoxLayout;
    rightzflsLayout->addWidget(p_rightZoomFactor);
    rightzflsLayout->addWidget(rightLockStretch);
    gridLayout->addLayout(rightzflsLayout, row++, 1);


    p_leftView = new ChipViewport(VIEWSIZE, VIEWSIZE, this);
    //  Do not want to accept mouse/keyboard events
    if (!p_allowLeftMouse) p_leftView->setDisabled(true);

    gridLayout->addWidget(p_leftView, row, 0);

    connect(this, SIGNAL(newControlNetwork(ControlNet *)),
            p_leftView, SLOT(setControlNet(ControlNet *)));

    connect(this,
            SIGNAL(stretchChipViewport(Stretch *, CubeViewport *)),
            p_leftView,
            SLOT(stretchFromCubeViewport(Stretch *, CubeViewport *)));

    connect(leftLockStretch, SIGNAL(stateChanged(int)),
            p_leftView,
            SLOT(changeStretchLock(int)));
    leftLockStretch->setChecked(false);


    // Connect left zoom buttons to ChipViewport's zoom slots
    connect(leftZoomIn, SIGNAL(clicked()), p_leftView, SLOT(zoomIn()));
    connect(leftZoomOut, SIGNAL(clicked()), p_leftView, SLOT(zoomOut()));
    connect(leftZoom1, SIGNAL(clicked()), p_leftView, SLOT(zoom1()));

    // If zoom on left, need to re-geom right
    connect(leftZoomIn, SIGNAL(clicked()), this, SLOT(updateRightGeom()));
    connect(leftZoomOut, SIGNAL(clicked()), this, SLOT(updateRightGeom()));
    connect(leftZoom1, SIGNAL(clicked()), this, SLOT(updateRightGeom()));

    // Connect the ChipViewport tackPointChanged signal to
    // the update sample/line label
    connect(p_leftView, SIGNAL(tackPointChanged(double)),
            this, SLOT(updateLeftPositionLabel(double)));

    // we want to allow this connection so that if a changed point is saved
    // and the same image is showing in both viewports, the left will refresh.
    connect(this, SIGNAL(updateLeftView(double, double)),
            p_leftView, SLOT(refreshView(double, double)));

    connect (p_leftView, SIGNAL(userMovedTackPoint()),
             this, SLOT(colorizeSaveButton()));

    if (p_allowLeftMouse) {
      //  Connect pan buttons to ChipViewport
      connect(leftPanUp, SIGNAL(clicked()), p_leftView, SLOT(panUp()));
      connect(leftPanDown, SIGNAL(clicked()), p_leftView, SLOT(panDown()));
      connect(leftPanLeft, SIGNAL(clicked()), p_leftView, SLOT(panLeft()));
      connect(leftPanRight, SIGNAL(clicked()), p_leftView, SLOT(panRight()));

      connect(leftPanUp, SIGNAL(clicked()), this, SLOT(colorizeSaveButton()));
      connect(leftPanDown, SIGNAL(clicked()), this, SLOT(colorizeSaveButton()));
      connect(leftPanLeft, SIGNAL(clicked()), this, SLOT(colorizeSaveButton()));
      connect(leftPanRight, SIGNAL(clicked()), this, SLOT(colorizeSaveButton()));
    }

    p_rightView = new ChipViewport(VIEWSIZE, VIEWSIZE, this);
    gridLayout->addWidget(p_rightView, row, 1);

    connect(this, SIGNAL(newControlNetwork(ControlNet *)),
            p_rightView, SLOT(setControlNet(ControlNet *)));
    connect(this,
            SIGNAL(stretchChipViewport(Stretch *, CubeViewport *)),
            p_rightView,
            SLOT(stretchFromCubeViewport(Stretch *, CubeViewport *)));
    connect(rightLockStretch, SIGNAL(stateChanged(int)),
            p_rightView,
            SLOT(changeStretchLock(int)));
    rightLockStretch->setChecked(false);

    // Connect the ChipViewport tackPointChanged signal to
    // the update sample/line label
    connect(p_rightView, SIGNAL(tackPointChanged(double)),
            this, SLOT(updateRightPositionLabel(double)));
    connect(this, SIGNAL(updateRightView(double, double)),
            p_rightView, SLOT(refreshView(double, double)));

    connect (p_rightView, SIGNAL(userMovedTackPoint()),
             this, SLOT(colorizeSaveButton()));

    connect(p_rightZoomIn, SIGNAL(clicked()), p_rightView, SLOT(zoomIn()));
    connect(p_rightZoomOut, SIGNAL(clicked()), p_rightView, SLOT(zoomOut()));
    connect(p_rightZoom1, SIGNAL(clicked()), p_rightView, SLOT(zoom1()));

    // Connect pan buttons to ChipViewport
    connect(rightPanUp, SIGNAL(clicked()), p_rightView, SLOT(panUp()));
    connect(rightPanDown, SIGNAL(clicked()), p_rightView, SLOT(panDown()));
    connect(rightPanLeft, SIGNAL(clicked()), p_rightView, SLOT(panLeft()));
    connect(rightPanRight, SIGNAL(clicked()), p_rightView, SLOT(panRight()));

    connect(rightPanUp, SIGNAL(clicked()), this, SLOT(colorizeSaveButton()));
    connect(rightPanDown, SIGNAL(clicked()), this, SLOT(colorizeSaveButton()));
    connect(rightPanLeft, SIGNAL(clicked()), this, SLOT(colorizeSaveButton()));
    connect(rightPanRight, SIGNAL(clicked()), this, SLOT(colorizeSaveButton()));

    // Create chips for left and right
    p_leftChip = new Chip(VIEWSIZE, VIEWSIZE);
    p_rightChip = new Chip(VIEWSIZE, VIEWSIZE);

    QButtonGroup *bgroup = new QButtonGroup();
    p_nogeom = new QRadioButton();
    p_nogeom->setChecked(true);
    connect(p_nogeom, SIGNAL(clicked()), this, SLOT(setNoGeom()));

    QCheckBox *linkZoom = NULL;
    if (p_useGeometry) {
      p_nogeom->setText("No geom/rotate");
      p_nogeom->setToolTip("Reset right measure to it's native geometry.");
      p_nogeom->setWhatsThis("Reset right measure to it's native geometry.  "
                             "If measure was rotated, set rotation back to 0.  "
                             "If measure was geomed to match the left measure, "
                             "reset the geometry back to it's native state.");
      p_geom   = new QRadioButton("Geom");
      p_geom->setToolTip("Geom right measure to match geometry of left measure.");
      p_geom->setWhatsThis("Using an affine transform, geom the right measure to match the "
                           "geometry of the left measure.");
      bgroup->addButton(p_geom);
      connect(p_geom, SIGNAL(clicked()), this, SLOT(setGeom()));
    }
    else {
      linkZoom = new QCheckBox("Link Zoom");
      linkZoom->setToolTip("Link zooming between the left and right views.");
      linkZoom->setWhatsThis("When zooming in the left view, the right view will "
                             "be set to the same zoom factor as the left view.");
      connect(linkZoom, SIGNAL(toggled(bool)), this, SLOT(setZoomLink(bool)));

      p_nogeom->setText("No rotate");
      p_nogeom->setToolTip("Reset right measure to it's native geometry.");
      p_nogeom->setWhatsThis("Reset right measure to it's native geometry.  "
                             "If measure was rotated, set rotation back to 0.");
    }
    bgroup->addButton(p_nogeom);

    QRadioButton *rotate = new QRadioButton("Rotate");
    bgroup->addButton(rotate);
    // TODO:  ?? Don't think we need this connection
    connect(rotate, SIGNAL(clicked()), this, SLOT(setRotate()));

    //  Set some defaults
    p_geomIt = false;
    p_rotation = 0;
    p_linkZoom = false;

    p_dial = new QDial();
    p_dial->setRange(0, 360);
    p_dial->setWrapping(false);
    p_dial->setNotchesVisible(true);
    p_dial->setNotchTarget(5.);
    p_dial->setEnabled(false);
    p_dial->setToolTip("Rotate right measure");
    p_dial->setWhatsThis("Rotate the right measure by degrees.");

    p_dialNumber = new QLCDNumber();
    p_dialNumber->setEnabled(false);
    p_dialNumber->setToolTip("Rotate right measure");
    p_dialNumber->setWhatsThis("Rotate the right measure by given number"
                               " of degrees.");
    connect(p_dial, SIGNAL(valueChanged(int)), p_dialNumber, SLOT(display(int)));
    connect(p_dial, SIGNAL(valueChanged(int)), p_rightView, SLOT(rotateChip(int)));

    QCheckBox *showPoints = new QCheckBox("Show control points");
    showPoints->setToolTip("Draw control point crosshairs");
    showPoints->setWhatsThis("This will toggle whether crosshairs are drawn"
                     " for the control points located within the measure''s"
                     " view.  For areas of dense measurements, turning this"
                     " off will allow easier viewing of features.");
    connect(showPoints, SIGNAL(toggled(bool)), p_leftView, SLOT(setPoints(bool)));
    connect(showPoints, SIGNAL(toggled(bool)), p_rightView, SLOT(setPoints(bool)));
    showPoints->setChecked(true);

    QCheckBox *cross = new QCheckBox("Show crosshair");
    connect(cross, SIGNAL(toggled(bool)), p_leftView, SLOT(setCross(bool)));
    connect(cross, SIGNAL(toggled(bool)), p_rightView, SLOT(setCross(bool)));
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

    p_slider = new QScrollBar(Qt::Horizontal);
    p_slider->setRange(1, 100);
    p_slider->setSingleStep(1);
    connect(p_slider, SIGNAL(valueChanged(int)), p_leftView, SLOT(setCircleSize(int)));
    connect(p_slider, SIGNAL(valueChanged(int)), p_rightView, SLOT(setCircleSize(int)));
    p_slider->setValue(20);
    p_slider->setDisabled(true);
    p_slider->hide();
    p_slider->setToolTip("Adjust circle size");
    p_slider->setWhatsThis("This allows the cirle size to be adjusted.");

    QVBoxLayout *vlayout = new QVBoxLayout();
    if (!p_useGeometry) {
      vlayout->addWidget(linkZoom);
    }
    vlayout->addWidget(p_nogeom);
    if (p_useGeometry) {
      vlayout->addWidget(p_geom);
    }
    vlayout->addWidget(rotate);
    vlayout->addWidget(p_dial);
    vlayout->addWidget(p_dialNumber);
    vlayout->addWidget(showPoints);
    vlayout->addWidget(cross);
    vlayout->addWidget(circle);
    vlayout->addWidget(p_slider);
    gridLayout->addLayout(vlayout, row++, 2);

    // Show sample / line for measure of chips shown
    p_leftSampLinePosition = new QLabel();
    p_leftSampLinePosition->setToolTip("Sample/Line under the crosshair");
    gridLayout->addWidget(p_leftSampLinePosition, row, 0);
    p_rightSampLinePosition = new QLabel();
    p_rightSampLinePosition->setToolTip("Sample/Line under the crosshair");
    gridLayout->addWidget(p_rightSampLinePosition, row++, 1);

    if (p_useGeometry) {
      //  Show lat / lon for measure of chips shown
      p_leftLatLonPosition = new QLabel();
      p_leftLatLonPosition->setToolTip("Latitude/Longitude under the crosshair");
      gridLayout->addWidget(p_leftLatLonPosition, row, 0);
      p_rightLatLonPosition = new QLabel();
      p_rightLatLonPosition->setToolTip("Latitude/Longitude under the crosshair");
      gridLayout->addWidget(p_rightLatLonPosition, row++, 1);
    }


    //  Add auto registration extension
    p_autoRegExtension = new QWidget;
    p_oldPosition = new QLabel;
    p_oldPosition->setToolTip("Measure Sample/Line before sub-pixel "
                              "registration");
    p_oldPosition->setWhatsThis("Original Sample/Line of the right measure "
            "before the sub-pixel registration.  If you select the \"Undo\" "
            "button, the measure will revert back to this Sample/Line.");
    p_goodFit = new QLabel;
    p_goodFit->setToolTip("Goodness of Fit result from sub-pixel registration.");
    p_goodFit->setWhatsThis("Resulting Goodness of Fit from sub-pixel "
                              "registration.");
    QVBoxLayout *autoRegLayout = new QVBoxLayout;
    autoRegLayout->setMargin(0);
    autoRegLayout->addWidget(p_oldPosition);
    autoRegLayout->addWidget(p_goodFit);
    p_autoRegExtension->setLayout(autoRegLayout);
    p_autoRegShown = false;
    p_autoRegAttempted = false;
    gridLayout->addWidget(p_autoRegExtension, row++, 1);


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

    p_blinkTimeBox = new QDoubleSpinBox();
    p_blinkTimeBox->setMinimum(0.1);
    p_blinkTimeBox->setMaximum(5.0);
    p_blinkTimeBox->setDecimals(1);
    p_blinkTimeBox->setSingleStep(0.1);
    p_blinkTimeBox->setValue(0.5);
    p_blinkTimeBox->setToolTip("Blink Time Delay");
    text = "<b>Function:</b> Change automatic blink rate between " +
           QString::number(p_blinkTimeBox->minimum()) + " and " +
           QString::number(p_blinkTimeBox->maximum()) + " seconds";
    p_blinkTimeBox->setWhatsThis(text);
    connect(p_blinkTimeBox, SIGNAL(valueChanged(double)),
            this, SLOT(changeBlinkTime(double)));

    leftLayout->addWidget(stop);
    leftLayout->addWidget(start);
    leftLayout->addWidget(p_blinkTimeBox);

    if (p_useGeometry) {
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
    p_autoReg = new QPushButton("Register");
    p_autoReg->setShortcut(Qt::Key_R);
    p_autoReg->setToolTip("Sub-pixel register the right measure to the left. "
                          "<strong>Shortcut: R</strong>");
    p_autoReg->setWhatsThis("Sub-pixel register the right measure to the left "
                       "and move the result under the crosshair.  After "
                       "viewing the results, the option exists to move the "
                       "measure back to the original position by selecting "
                       "<strong>\"Undo Registration\"</strong>.");
    if (p_allowLeftMouse) {
      p_saveMeasure = new QPushButton("Save Measures");
      p_saveMeasure->setToolTip("Save the both the left and right measure to the edit control "
                                "point (control point currently being edited). "
                                "<strong>Shortcut: M</strong>. "
                                " <strong>Note: The edit control point "
                                "will not be saved to the network until you select "
                                "<strong>\"Save Point\"</strong>");
    }
    else {
      p_saveMeasure = new QPushButton("Save Measure");
      p_saveMeasure->setToolTip("Save the right measure to the edit control "
                                "point (control point currently being edited). "
                                "<strong>Shortcut: M</strong>. "
                                " <strong>Note: The edit control point "
                                "will not be saved to the network until you select "
                                "<strong>\"Save Point\"</strong>");
    }
    p_saveMeasure->setShortcut(Qt::Key_M);
    p_saveDefaultPalette = p_saveMeasure->palette();

    rightLayout->addWidget(p_autoReg);
    rightLayout->addWidget(p_saveMeasure);
    rightLayout->addStretch();
    gridLayout->addLayout(rightLayout, row, 1);

    connect(p_autoReg, SIGNAL(clicked()), this, SLOT(registerPoint()));
    connect(p_saveMeasure, SIGNAL(clicked()), this, SLOT(saveMeasure()));

    setLayout(gridLayout);

    //p_pointEditor->setCentralWidget(cw);
    p_autoRegExtension->hide();

    //p_pointEditor->setVisible(true);
    //p_pointEditor->raise();
  }


  /**
   * Set the measure displayed in the left ChipViewport
   *
   * @param leftMeasure  Input  Measure displayed in left ChipViewport
   * @param leftCube     Input  Cube of measure displayed in left ChipViewport
   *
   * @author Tracie Sucharski
   * @internal
   *   @history 2008-11-19 Tracie Sucharski - If left cube changes, get new
   *                           universalGroundMap.
   *   @history 2012-04-17 Tracie Sucharski - If geom is turned on update the
   *                           right measure.
   *   @history 2012-05-07 Tracie Sucharski - Last change introduced bug when
   *                           loading a different control point, so only
   *                           update the right chip if we're not loading a
   *                           different control point.
   *   @history 2013-04-30 Tracie Sucharski - Fixed bug introduced by linking zooms between left
   *                           and right viewports.  Zoom factors were being passed into the
   *                           Chip::Load method as the second argument which should be the rotation
   *                           value.
   */
  void ControlPointEdit::setLeftMeasure(ControlMeasure *leftMeasure,
                                        Cube *leftCube, QString pointId) {

    //  Make sure registration is turned off
    if (p_autoRegShown) {
      //  Undo Registration
      p_autoRegShown = false;
      p_autoRegExtension->hide();
      p_autoReg->setText("Register");
      p_autoReg->setToolTip("Sub-pixel register the right measure to the left."
                            "<strong>Shortcut: R</strong>");
      p_autoReg->setShortcut(Qt::Key_R);
    }

    p_leftMeasure = leftMeasure;

    if (p_useGeometry) {
      //  get new ground map
      if ( p_leftGroundMap != 0 ) delete p_leftGroundMap;
      p_leftGroundMap = new UniversalGroundMap(*leftCube);
    }
    p_leftCube = leftCube;

    p_leftChip->TackCube(p_leftMeasure->GetSample(), p_leftMeasure->GetLine());
    p_leftChip->Load(*p_leftCube);

    // Dump into left chipViewport
    p_leftView->setChip(p_leftChip, p_leftCube);

    // Only update right if not loading a new point.  If it's a new point, right measure
    // hasn't been loaded yet.
    if (pointId == p_pointId && p_geomIt) updateRightGeom();
    p_pointId = pointId;
  }


  /**
   * Set the measure displayed in the right ChipViewport
   *
   *
   * @param rightMeasure  Input  Measure displayed in right ChipViewport
   * @param rightCube     Input  Cube of measure displayed in right ChipViewport
   *
   * @author Tracie Sucharski
   * @internal
   *   @history 2008-11-19 Tracie Sucharski - If right cube changes, get new
   *                           universalGroundMap.
   *   @history 2008-15-2008 Jeannie Walldren - Added error string to
   *                           iException::Message before
   *                           creating QMessageBox
   *   @history 2009-09-14 Tracie Sucharski - Call geomChip to make
   *                           sure left chip is initialized in the
   *                           ChipViewport.  This was done for the changes
   *                           made to the Chip class and the ChipViewport
   *                           class where the Cube info is no longer stored.
   *   @history 2013-04-30 Tracie Sucharski - Fixed bug introduced by linking zooms between left
   *                           and right viewports.  Zoom factors were being passed into the
   *                           Chip::Load method as the second argument which should be the rotation
   *                           value.
   *
   */
  void ControlPointEdit::setRightMeasure(ControlMeasure *rightMeasure,
                                         Cube *rightCube, QString pointId) {

    //  Make sure registration is turned off
    if (p_autoRegShown) {
      //  Undo Registration
      p_autoRegShown = false;
      p_autoRegExtension->hide();
      p_autoReg->setText("Register");
      p_autoReg->setShortcut(Qt::Key_R);
    }
    p_autoRegAttempted = false;

    p_rightMeasure = rightMeasure;
    p_pointId = pointId;

    if (p_useGeometry) {
      //  get new ground map
      if ( p_rightGroundMap != 0 ) delete p_rightGroundMap;
      p_rightGroundMap = new UniversalGroundMap(*rightCube);
    }
    p_rightCube = rightCube;

    p_rightChip->TackCube(p_rightMeasure->GetSample(),
                          p_rightMeasure->GetLine());
    if (p_geomIt == false) {
      p_rightChip->Load(*p_rightCube);
    }
    else {
      try {
        p_rightChip->Load(*p_rightCube, *p_leftChip, *p_leftCube);

      }
      catch (IException &e) {
        IException fullError(e, IException::User, "Geom failed.", _FILEINFO_);
        QString message = QString::fromStdString(fullError.toString());
        QMessageBox::information((QWidget *)parent(), "Error", message);
        p_rightChip->Load(*p_rightCube);
        p_geomIt = false;
        p_nogeom->setChecked(true);
        p_geom->setChecked(false);
      }
    }

    // Dump into right chipViewport
    p_rightView->setChip(p_rightChip, p_rightCube);

    updateRightGeom();
    //p_rightView->geomChip(p_leftChip,p_leftCube);

    // New right measure, make sure Save Measure Button text is default
    p_saveMeasure->setPalette(p_saveDefaultPalette);

  }


  /**
   * Update sample/line, lat/lon and zoom factor of left measure
   *
   * @param zoomFactor  Input  zoom factor
   *
   * @author Tracie Sucharski
   *
   * @internal
   *   @history 2012-07-26 Tracie Sucharski - Added ability to link zooming between left and
   *                          right viewports.  TODO:  Re-think design, should this be put
   *                          somewhere else.  This was the fastest solution for now.
   */
  void ControlPointEdit::updateLeftPositionLabel(double zoomFactor) {
    QString pos = "Sample: " + QString::number(p_leftView->tackSample()) +
                  "    Line:  " + QString::number(p_leftView->tackLine());
    p_leftSampLinePosition->setText(pos);

    if (p_useGeometry) {
      //  Get lat/lon from point in left
      p_leftGroundMap->SetImage(p_leftView->tackSample(), p_leftView->tackLine());
      double lat = p_leftGroundMap->UniversalLatitude();
      double lon = p_leftGroundMap->UniversalLongitude();

      pos = "Latitude: " + QString::number(lat) +
            "    Longitude:  " + QString::number(lon);
      p_leftLatLonPosition->setText(pos);
    }

    //  Print zoom scale factor
    pos = "Zoom Factor: " + QString::number(zoomFactor);
    p_leftZoomFactor->setText(pos);

    //  If zooms are linked, make right match left
    if (p_linkZoom) {
      p_rightView->zoom(p_leftView->zoomFactor());
    }

  }


  /**
   * Update sample/line, lat/lon and zoom factor of right measure
   *
   * @param zoomFactor  Input  zoom factor
   *
   * @author Tracie Sucharski
   */
  void ControlPointEdit::updateRightPositionLabel(double zoomFactor) {

    // If registration Info is on, turn off
    if (p_autoRegShown) {
      //  Undo Registration
      p_autoRegShown = false;
      p_autoRegExtension->hide();
      p_autoReg->setText("Register");
      p_autoReg->setToolTip("Sub-pixel register the right measure to the left. "
                            "<strong>Shortcut: R</strong>");
      p_autoReg->setShortcut(Qt::Key_R);
    }

    QString pos = "Sample: " + QString::number(p_rightView->tackSample()) +
                  "    Line:  " + QString::number(p_rightView->tackLine());
    p_rightSampLinePosition->setText(pos);

    if (p_useGeometry) {
      //  Get lat/lon from point in right
      p_rightGroundMap->SetImage(p_rightView->tackSample(), p_rightView->tackLine());
      double lat = p_rightGroundMap->UniversalLatitude();
      double lon = p_rightGroundMap->UniversalLongitude();

      pos = "Latitude: " + QString::number(lat) +
            "    Longitude:  " + QString::number(lon);
      p_rightLatLonPosition->setText(pos);
    }

    //  Print zoom scale factor
    pos = "Zoom Factor: " + QString::number(zoomFactor);
    p_rightZoomFactor->setText(pos);

  }


  /**
   * Turn "Save Measure" button text to red
   *
   * @author 2011-06-14 Tracie Sucharski
   */
  void ControlPointEdit::colorizeSaveButton() {

    QColor qc = Qt::red;
    QPalette p = p_saveMeasure->palette();
    p.setColor(QPalette::ButtonText,qc);
    p_saveMeasure->setPalette(p);

  }


  /**
   * Find point from left ChipViewport in the right ChipViewport
   *
   * @author tsucharski (6/27/2011)
   *
   * @history 2011-06-27 Tracie Sucharski - If measure moves to different
   *                        samp/line than saved measure, change save button
   *                        to red.
   */
  void ControlPointEdit::findPoint() {

    //  Get lat/lon from point in left
    p_leftGroundMap->SetImage(p_leftView->tackSample(), p_leftView->tackLine());
    double lat = p_leftGroundMap->UniversalLatitude();
    double lon = p_leftGroundMap->UniversalLongitude();

    //  Reload right chipViewport with this new tack point.
    if ( p_rightGroundMap->SetUniversalGround(lat, lon) ) {
      emit updateRightView(p_rightGroundMap->Sample(), p_rightGroundMap->Line());

      //  If moving from saved measure, turn save button to red
      if (p_rightGroundMap->Sample() != p_rightMeasure->GetSample() ||
          p_rightGroundMap->Line() != p_rightMeasure->GetLine())
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
   * Sub-pixel register point in right chipViewport with point in
   * left.
   * @internal
   *   @history 2008-15-2008 Jeannie Walldren - Throw and catch
   *                           error before creating QMessageBox
   *   @history 2009-03-23 Tracie Sucharski - Added private p_autoRegAttempted
   *                           for the SaveChips method.
   *   @history 2010-02-16 Tracie Sucharski- If autoreg fails,
   *                           print registration stats.
   *   @history 2010-02-18 Tracie Sucharski - Registration stats wasn't the
   *                           correct info to print.  Instead, check
   *                           registrationStatus and print separate errors
   *                           for each possibility.
   *   @history 2010-02-22 Tracie Sucharski - Added more info for registration
   *                           failures.
   *   @history 2010-06-08 Jeannie Walldren - Catch error and
   *                           warn user if unable to load
   *                           pattern (left) or search (right)
   *                           chip
   *   @history 2010-10-12 Tracie Sucharski - Clean up try/catch blocks.
   *   @history 2011-06-27 Tracie Sucharski - If un-doing registration, change
   *                           save button back to black.  If registration
   *                           successful, change save button to red.
   *   @history 2011-10-21 Tracie Sucharski - Add try/catch around registration
   *                           to catch errors thrown from autoreg class.
   *   @history 2017-04-21 Marjorie Hahn - Added auto registration factory creation.
   *
   */
  void ControlPointEdit::registerPoint() {

    // if the auto registration factory has not been initialized, do it here
    if (p_autoRegFact == NULL) {
      try {
        Pvl pvl(p_templateFileName.toStdString());
        p_autoRegFact = AutoRegFactory::Create(pvl);
      }
      catch (IException &e) {
        p_autoRegFact = NULL;
        IException fullError(e, IException::Io,
                            "Cannot create AutoRegFactory. As a result, "
                            "sub-pixel registration will not work.",
                            _FILEINFO_);
        QString message = QString::fromStdString(fullError.toString());
        QMessageBox::information((QWidget *)parent(), "Error", message);
        return;
      }
    }

    if (p_autoRegShown) {
      // Undo Registration
      p_autoRegShown = false;
      p_autoRegExtension->hide();
      p_autoReg->setText("Register");
      p_autoReg->setToolTip("Sub-pixel register the right measure to the left. "
                            "<strong>Shortcut: R</strong>");
      p_autoReg->setShortcut(Qt::Key_R);

      //  Reload chip with original measure
      emit updateRightView(p_rightMeasure->GetSample(),
                           p_rightMeasure->GetLine());
      // Since un-doing registration, make sure save button not red
      p_saveMeasure->setPalette(p_saveDefaultPalette);
      return;
    }
    p_autoRegAttempted = true;

    try {
      p_autoRegFact->PatternChip()->TackCube(p_leftMeasure->GetSample(),
                                             p_leftMeasure->GetLine());
      p_autoRegFact->PatternChip()->Load(*p_leftCube);
      p_autoRegFact->SearchChip()->TackCube(p_rightMeasure->GetSample(),
                                            p_rightMeasure->GetLine());
      if (p_useGeometry) {
        p_autoRegFact->SearchChip()->Load(*p_rightCube,
                            *(p_autoRegFact->PatternChip()), *p_leftCube);
      }
      else {
        p_autoRegFact->SearchChip()->Load(*p_rightCube);
      }
    }
    catch (IException &e) {
      std::string msg = "Cannot register this point, unable to Load chips.\n";
      msg += e.toString();
      QMessageBox::information((QWidget *)parent(), "Error", QString::fromStdString(msg));
      return;
    }

    try {
      AutoReg::RegisterStatus status = p_autoRegFact->Register();
      if (!p_autoRegFact->Success()) {
        QString msg = "Cannot sub-pixel register this point.\n";
        if (status == AutoReg::PatternChipNotEnoughValidData) {
          msg += "\n\nNot enough valid data in Pattern Chip.\n";
          msg += "  PatternValidPercent = ";
          msg += QString::number(p_autoRegFact->PatternValidPercent()) + "%";
        }
        else if (status == AutoReg::FitChipNoData) {
          msg += "\n\nNo valid data in Fit Chip.";
        }
        else if (status == AutoReg::FitChipToleranceNotMet) {
          msg += "\n\nGoodness of Fit Tolerance not met.\n";
          msg += "\nGoodnessOfFit = " + QString::number(p_autoRegFact->GoodnessOfFit());
          msg += "\nGoodnessOfFitTolerance = ";
          msg += QString::number(p_autoRegFact->Tolerance());
        }
        else if (status == AutoReg::SurfaceModelNotEnoughValidData) {
          msg += "\n\nNot enough valid points in the fit chip window for sub-pixel ";
          msg += "accuracy.  Probably too close to edge.\n";
        }
        else if (status == AutoReg::SurfaceModelSolutionInvalid) {
          msg += "\n\nCould not model surface for sub-pixel accuracy.\n";
        }
        else if (status == AutoReg::SurfaceModelDistanceInvalid) {
          double sampDist, lineDist;
          p_autoRegFact->Distance(sampDist, lineDist);
          msg += "\n\nSub pixel algorithm moves registration more than tolerance.\n";
          msg += "\nSampleMovement = " + QString::number(sampDist) +
                 "    LineMovement = " + QString::number(lineDist);
          msg += "\nDistanceTolerance = " +
                 QString::number(p_autoRegFact->DistanceTolerance());
        }
        else if (status == AutoReg::PatternZScoreNotMet) {
          double score1, score2;
          p_autoRegFact->ZScores(score1, score2);
          msg += "\n\nPattern data max or min does not pass z-score test.\n";
          msg += "\nMinimumZScore = " + QString::number(p_autoRegFact->MinimumZScore());
          msg += "\nCalculatedZscores = " + QString::number(score1) + ", " + QString::number(score2);
        }
        else if (status == AutoReg::AdaptiveAlgorithmFailed) {
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
      std::string msg = "Cannot register this point.\n";
      msg += e.toString();
      QMessageBox::information((QWidget *)parent(), "Error", QString::fromStdString(msg));
      return;
    }

    //  Load chip with new registered point
    emit updateRightView(p_autoRegFact->CubeSample(), p_autoRegFact->CubeLine());
    // If registered pt different from measure, colorize the save button
    if (p_autoRegFact->CubeSample() != p_rightMeasure->GetSample() ||
        p_autoRegFact->CubeLine() != p_rightMeasure->GetLine()) {
      colorizeSaveButton();
    }

    QString oldPos = "Original Sample: " +
                     QString::number(p_rightMeasure->GetSample()) + "   Original Line:  " +
                     QString::number(p_rightMeasure->GetLine());
    p_oldPosition->setText(oldPos);

    QString goodFit = "Goodness of Fit:  " +
                      QString::number(p_autoRegFact->GoodnessOfFit());
    p_goodFit->setText(goodFit);

    p_autoRegExtension->show();
    p_autoRegShown = true;
    p_autoReg->setText("Undo Registration");
    p_autoReg->setToolTip("Undo sub-pixel registration. "
                          "<strong>Shortcut: U</strong>");
    p_autoReg->setShortcut(Qt::Key_U);
  }


  /**
   * Save control measure under the crosshair in right ChipViewport
   * @internal
   *   @history 2008-12-30 Jeannie Walldren - Modified to update user (chooser) name and date when
   *                           point is saved
   *   @history 2010-11-19 Tracie Sucharski - Renamed from savePoint.
   *   @history 2011-03-04 Tracie Sucharski - If auto reg info is shown, save
   *                           the GoodnessOfFit value to the right
   *                           ControlMeasure log entry.  Changed the way
   *                           the left viewport is updated if the left and
   *                           right measure are the same.  Simply copy the
   *                           right measure into the left and re-load the left
   *                           measure.  If measure currently has type of
   *                           subPixelRegistered, but the new position has
   *                           not be sub-pixel registered, change measure type
   *                           and get rid of goodness of fit.
   *   @history 2011-06-14 Tracie Sucharski - Change Save Measure button text
   *                           back to black.
   *   @history 2011-07-19 Tracie Sucharski - Updated for new functionality
   *                           of registration pixel shift.  ControlMeasure
   *                           will now calculate based on aprioriSample/Line
   *                           and current coordinate.  If autoreg has been
   *                           calculated, save coordinate to apriori before
   *                           updating to subpixel registered coordinate.
   *   @history 2013-11-07 Tracie Sucharski - Moved error checking on edit locked measures from
   *                           QnetTool::measureSaved to ::saveMeasure.  The error checking now
   *                           forces the edit lock check box to be unchecked before the measure
   *                           can be saved.
   *   @history 2015-01-09 Ian Humphrey - Modified to prevent segmentation fault that arises when
   *                           registering, opening a template file, and saving the measure. This
   *                           was caused by not handling the exception thrown by
   *                           ControlMeasure::SetLogData(), which produces undefined behavior
   *                           within the Qt signal-slot connection mechanism.
   */
  void ControlPointEdit::saveMeasure() {

    if (p_rightMeasure != NULL) {

      if (p_rightMeasure->IsEditLocked()) {
        QString message = "The right measure is locked.  You must first unlock the measure by ";
        message += "clicking the check box above labeled \"Edit Lock Measure\".";
        QMessageBox::warning((QWidget *)parent(),"Warning",message);
        return;
      }

      if (p_autoRegShown) {
        try {
          //  Save  autoreg parameters to the right measure log entry
          //  Eccentricity may be invalid, check before writing.
          p_rightMeasure->SetLogData(ControlMeasureLogData(
                                     ControlMeasureLogData::GoodnessOfFit,
                                     p_autoRegFact->GoodnessOfFit()));
          double minZScore, maxZScore;
          p_autoRegFact->ZScores(minZScore,maxZScore);
          p_rightMeasure->SetLogData(ControlMeasureLogData(
                                     ControlMeasureLogData::MinimumPixelZScore,
                                     minZScore));
          p_rightMeasure->SetLogData(ControlMeasureLogData(
                                     ControlMeasureLogData::MaximumPixelZScore,
                                     maxZScore));
        }
        // need to handle exception that SetLogData throws if our data is invalid -
        // unhandled exceptions thrown in Qt signal and slot connections produce undefined behavior
        catch (IException &e) {
          QString message = QString::fromStdString(e.toString());
          QMessageBox::critical((QWidget *)parent(), "Error", message);
          return;
        }

        //  Reset AprioriSample/Line to the current coordinate, before the
        //  coordinate is updated with the registered coordinate.
        p_rightMeasure->SetAprioriSample(p_rightMeasure->GetSample());
        p_rightMeasure->SetAprioriLine(p_rightMeasure->GetLine());

        p_rightMeasure->SetChooserName("Application qnet");
        p_rightMeasure->SetType(ControlMeasure::RegisteredSubPixel);

        p_autoRegShown = false;
        p_autoRegExtension->hide();
        p_autoReg->setText("Register");
        p_autoReg->setToolTip("Sub-pixel register the right measure to the left. "
                              "<strong>Shortcut: R</strong>");
        p_autoReg->setShortcut(Qt::Key_R);
      }
      else {
        p_rightMeasure->SetChooserName(Application::UserName());
        p_rightMeasure->SetType(ControlMeasure::Manual);
        p_rightMeasure->DeleteLogData(
                               ControlMeasureLogData::GoodnessOfFit);
        p_rightMeasure->DeleteLogData(
                               ControlMeasureLogData::MinimumPixelZScore);
        p_rightMeasure->DeleteLogData(
                               ControlMeasureLogData::MaximumPixelZScore);
      }

      //  Get cube position at right chipViewport crosshair
      p_rightMeasure->SetCoordinate(p_rightView->tackSample(),
                                    p_rightView->tackLine());
      p_rightMeasure->SetDateTime();

    }

    if (p_allowLeftMouse) {
      if (p_leftMeasure != NULL) {
        if (p_leftMeasure->IsEditLocked()) {
          QString message = "The left measure is locked.  You must first unlock the measure by ";
          message += "clicking the check box above labeled \"Edit Lock Measure\".";
          QMessageBox::warning((QWidget *)parent(),"Warning",message);
          return;
        }

        p_leftMeasure->SetCoordinate(p_leftView->tackSample(), p_leftView->tackLine());
        p_leftMeasure->SetDateTime();
        p_leftMeasure->SetChooserName(Application::UserName());
        p_leftMeasure->SetType(ControlMeasure::Manual);
      }
    }

    //  If the right chip is the same as the left chip, copy right into left and
    //  re-load the left.
    if (p_rightMeasure->GetCubeSerialNumber() ==
        p_leftMeasure->GetCubeSerialNumber()) {

      *p_leftMeasure = *p_rightMeasure;
      setLeftMeasure(p_leftMeasure,p_leftCube,p_pointId);
    }

    //  Change Save Measure button text back to default palette
    p_saveMeasure->setPalette(p_saveDefaultPalette);

    //  Redraw measures on viewports
    emit measureSaved();
  }


  /**
   * Slot to update the geomed right ChipViewport for zoom
   * operations
   * @internal
   *   @history 2008-15-2008 Jeannie Walldren - Added error string to
   *                             iException::Message before creating QMessageBox
   */
  void ControlPointEdit::updateRightGeom() {

    if (p_geomIt) {
      try {
        p_rightView->geomChip(p_leftChip, p_leftCube);

      }
      catch (IException &e) {
        IException fullError(e, IException::User, "Geom failed.", _FILEINFO_);
        QString message = QString::fromStdString(fullError.toString());
        QMessageBox::information((QWidget *)parent(), "Error", message);
        p_geomIt = false;
        p_nogeom->setChecked(true);
        p_geom->setChecked(false);
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
//void ControlPointEdit::updateRightZoom() {
//
//  if (p_linkZoom) {
//    try {
//      p_rightView->geomChip(p_leftChip, p_leftCube);
//
//    }
//    catch (IException &e) {
//      IException fullError(e, IException::User, "Geom failed.", _FILEINFO_);
//      QString message = QString::fromStdString(fullError.toString());
//      QMessageBox::information((QWidget *)parent(), "Error", message);
//      p_geomIt = false;
//      p_nogeom->setChecked(true);
//      p_geom->setChecked(false);
//    }
//  }
//}


  /**
   * Slot to enable the rotate dial
   * @internal
   *   @history 2012-05-01 Tracie Sucharski - Reset zoom buttons and reload chip
  */
  void ControlPointEdit::setRotate() {
    QApplication::setOverrideCursor(Qt::WaitCursor);

    //  Text needs to be reset because it was changed to
    //  indicate why it's greyed out
    QString text = "Zoom in 2X";
    p_rightZoomIn->setEnabled(true);
    p_rightZoomIn->setWhatsThis(text);
    p_rightZoomIn->setToolTip("Zoom In");
    text = "Zoom out 2X";
    p_rightZoomOut->setEnabled(true);
    p_rightZoomOut->setWhatsThis(text);
    p_rightZoomOut->setToolTip("Zoom Out");
    text = "Zoom 1:1";
    p_rightZoom1->setEnabled(true);
    p_rightZoom1->setWhatsThis(text);
    p_rightZoom1->setToolTip("Zoom 1:1");

    p_geomIt = false;
    p_rightView->nogeomChip();

    QApplication::restoreOverrideCursor();

    p_dial->setEnabled(true);
    p_dialNumber->setEnabled(true);
    p_dial->setNotchesVisible(true);

  }


  /**
   * Turn geom on
   *
   * @internal
   *   @history  2007-06-15 Tracie Sucharski - Grey out zoom buttons
   *   @history 2008-15-2008 Jeannie Walldren - Added error string to iException::Message before
   *                             creating QMessageBox
   **/
  void ControlPointEdit::setGeom() {

    if (p_geomIt == true) return;

    QApplication::setOverrideCursor(Qt::WaitCursor);

    //  Grey right view zoom buttons
    QString text = "Zoom functions disabled when Geom is set";
    p_rightZoomIn->setEnabled(false);
    p_rightZoomIn->setWhatsThis(text);
    p_rightZoomIn->setToolTip(text);
    p_rightZoomOut->setEnabled(false);
    p_rightZoomOut->setWhatsThis(text);
    p_rightZoomOut->setToolTip(text);
    p_rightZoom1->setEnabled(false);
    p_rightZoom1->setWhatsThis(text);
    p_rightZoom1->setToolTip(text);


    //  Reset dial to 0 before disabling
    p_dial->setValue(0);
    p_dial->setEnabled(false);
    p_dialNumber->setEnabled(false);

    p_geomIt = true;

    try {
      p_rightView->geomChip(p_leftChip, p_leftCube);
    }
    catch (IException &e) {
      IException fullError(e, IException::User, "Geom failed.", _FILEINFO_);
      QString message = QString::fromStdString(fullError.toString());
      QMessageBox::information((QWidget *)parent(), "Error", message);
      p_geomIt = false;
      p_nogeom->setChecked(true);
      p_geom->setChecked(false);
    }

    QApplication::restoreOverrideCursor();
  }


  /**
   * Slot to turn off geom
   * @internal
   *   @history 2012-05-01 Tracie Sucharski - Reset zoom buttons and rotate dial, then reload chip
  */
  void ControlPointEdit::setNoGeom() {

    QApplication::setOverrideCursor(Qt::WaitCursor);

    QString text = "Zoom in 2X";
    p_rightZoomIn->setEnabled(true);
    p_rightZoomIn->setWhatsThis(text);
    p_rightZoomIn->setToolTip("Zoom In");
    text = "Zoom out 2X";
    p_rightZoomOut->setEnabled(true);
    p_rightZoomOut->setWhatsThis(text);
    p_rightZoomOut->setToolTip("Zoom Out");
    text = "Zoom 1:1";
    p_rightZoom1->setEnabled(true);
    p_rightZoom1->setWhatsThis(text);
    p_rightZoom1->setToolTip("Zoom 1:1");

    //  Reset dial to 0 before disabling
    p_dial->setValue(0);
    p_dial->setEnabled(false);
    p_dialNumber->setEnabled(false);

    p_geomIt = false;
    p_rightView->nogeomChip();

    QApplication::restoreOverrideCursor();
  }


  /**
   * Turn circle widgets on/off
   *
   * @param checked   Input   Turn cirle on or off
   *
   * @author 2008-09-09 Tracie Sucharski
   */
  void ControlPointEdit::setCircle(bool checked) {

    if (checked == p_circle) return;

    p_circle = checked;
    if (p_circle) {
      // Turn on slider bar
      p_slider->setDisabled(false);
      p_slider->show();
      p_slider->setValue(20);
      p_leftView->setCircle(true);
      p_rightView->setCircle(true);
    }
    else {
      p_slider->setDisabled(true);
      p_slider->hide();
      p_leftView->setCircle(false);
      p_rightView->setCircle(false);
    }

  }


  /**
   * Turn linking of zoom on or off
   *
   * @param checked   Input   Turn zoom linking on or off
   *
   * @author 2012-07-26 Tracie Sucharski
   */
  void ControlPointEdit::setZoomLink(bool checked) {

    if (checked == p_linkZoom) return;

    p_linkZoom = checked;
    if (p_linkZoom) {
      p_rightView->zoom(p_leftView->zoomFactor());
    }
  }


  //!  Slot to start blink function
  void ControlPointEdit::blinkStart() {
    if (p_timerOn) return;

    //  Set up blink list
    p_blinkList.push_back(p_leftView);
    p_blinkList.push_back(p_rightView);
    p_blinkIndex = 0;

    p_timerOn = true;
    int msec = (int)(p_blinkTimeBox->value() * 1000.0);
    p_timer = new QTimer(this);
    connect(p_timer, SIGNAL(timeout()), this, SLOT(updateBlink()));
    p_timer->start(msec);
  }


  //!  Slot to stop blink function
  void ControlPointEdit::blinkStop() {
    p_timer->stop();
    p_timerOn = false;
    p_blinkList.clear();

    //  Reload left chipViewport with original chip
    p_leftView->repaint();

  }


  /**
   * Set blink rate
   *
   * @param interval   Input   Blink rate in seconds
   * @author  Tracie Sucharski
   */
  void ControlPointEdit::changeBlinkTime(double interval) {
    if (p_timerOn) p_timer->setInterval((int)(interval * 1000.));
  }


  //!  Slot to cause the blink to happen coinciding with the timer
  void ControlPointEdit::updateBlink() {

    p_blinkIndex = !p_blinkIndex;
    p_leftView->loadView(*(p_blinkList)[p_blinkIndex]);
  }


  /**
   * Allows user to choose a new template file by opening a window
   * from which to select a filename.  This file is then
   * registered and set as the new template.
   *
   * @author Tracie Sucharski
   * @internal
   *   @history 2008-12-10 Jeannie Walldren - Changed name from
   *                          openTemplateFile() and added functionality to set
   *                          new template filename to the private variable,
   *                          p_templateFileName
   *   @history 2008-12-15 Jeannie Walldren - Modified code to
   *                          only allow the template file to be modified if
   *                          registration is successfull, otherwise the
   *                          original template file is kept.
   *   @history 2014-12-11 Ian Humphrey - Modified code so opening a template file will undo
   *                           registration if a point is already registered.
   */
  bool ControlPointEdit::setTemplateFile(QString fn) {

    AutoReg *reg = NULL;
    // save original template filename
    QString temp = p_templateFileName;
    try {
      // set template filename to user chosen pvl file
      p_templateFileName = fn;

      // Create PVL object with this file
      Pvl pvl(fn.toStdString());

      // try to register file
      reg = AutoRegFactory::Create(pvl);
      if (p_autoRegFact != NULL)
        delete p_autoRegFact;
      p_autoRegFact = reg;

      p_templateFileName = fn;

      // undo registration if a point is already registered
      // this prevents the user from saving a measure with invalid data
      if (p_autoRegShown)
        registerPoint();

      return true;
    }
    catch (IException &e) {
      // set templateFileName back to its original value
      p_templateFileName = temp;
      IException fullError(e, IException::Io,
          "Cannot create AutoRegFactory for " +
          fn.toStdString() +
          ".  As a result, current template file will remain set to " +
          p_templateFileName.toStdString(), _FILEINFO_);
      QString message = QString::fromStdString(fullError.toString());
      QMessageBox::information((QWidget *)parent(), "Error", message);
      return false;
    }
  }


  /**
   * Set the option that allows mouse movements in the left ChipViewport.
   *
   * @author Tracie Sucharski
   * @internal
   */
  void ControlPointEdit::allowLeftMouse(bool allowMouse) {
    p_allowLeftMouse = allowMouse;

    if (p_allowLeftMouse) {
      p_saveMeasure = new QPushButton("Save Measures");
      p_saveMeasure->setToolTip("Save the both the left and right measure to the edit control "
                                "point (control point currently being edited). "
                                " <strong>Note: The edit control point "
                                "will not be saved to the network until you select "
                                "<strong>\"Save Point\"</strong>");
    }
    else {
      p_saveMeasure = new QPushButton("Save Measure");
      p_saveMeasure->setToolTip("Save the right measure to the edit control "
                                "point (control point currently being edited). "
                                " <strong>Note: The edit control point "
                                "will not be saved to the network until you select "
                                "<strong>\"Save Point\"</strong>");
    }
  }


  void ControlPointEdit::refreshChips() {
    p_leftView->update();
    p_rightView->update();
  }


  /**
   * Slot to save registration chips to files and fire off qview.
   *
   * @author 2009-03-18  Tracie Sucharski
   * @internal
   * @history 2009-03-23 Tracie Sucharski - Use p_autoRegAttempted to see
   *                        if chips exist.
   */
  void ControlPointEdit::saveChips() {

    // if (!p_autoRegShown) {
    if (!p_autoRegAttempted) {
      QString message = "Point must be Registered before chips can be saved.";
      QMessageBox::warning((QWidget *)parent(), "Warning", message);
      return;
    }

    //  Save chips - pattern, search and fit
    QString baseFile = p_pointId.replace(" ", "_") + "_" +
                           QString::number((int)(p_leftMeasure ->GetSample())) + "_" +
                           QString::number((int)(p_leftMeasure ->GetLine()))   + "_" +
                           QString::number((int)(p_rightMeasure->GetSample())) + "_" +
                           QString::number((int)(p_rightMeasure->GetLine()))   + "_";
    QString fname = baseFile + "Search.cub";
    QString command = "$ISISROOT/bin/qview \'" + fname + "\'";
    p_autoRegFact->RegistrationSearchChip()->Write(fname);
    fname = baseFile + "Pattern.cub";
    command += " \'" + fname + "\'";
    p_autoRegFact->RegistrationPatternChip()->Write(fname);
    fname = baseFile + "Fit.cub";
    command += " \'" + fname + "\' &";
    p_autoRegFact->FitChip()->Write(fname);
    ProgramLauncher::RunSystemCommand(command);
  }
}
