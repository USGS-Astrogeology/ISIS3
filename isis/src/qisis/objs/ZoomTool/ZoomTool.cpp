/** This is free and unencumbered software released into the public domain.

The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */

#include <iostream>

#include <QAction>
#include <QApplication>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QMenu>
#include <QMenuBar>
#include <QMessageBox>
#include <QPushButton>
#include <QStackedWidget>
#include <QToolButton>
#include <QValidator>

#include "IException.h"
#include "IString.h"
#include "MainWindow.h"
#include "MdiCubeViewport.h"
#include "RubberBandTool.h"
#include "ToolPad.h"
#include "Workspace.h"
#include "ZoomTool.h"

namespace Isis {
  /**
   * ZoomTool constructor
   *
   *
   * @param parent Parent widget
   */
  ZoomTool::ZoomTool(QWidget *parent) : Tool(parent) {
    p_zoomIn2X = new QAction(parent);
    p_zoomIn2X->setShortcut(Qt::Key_Plus);
    p_zoomIn2X->setText("Zoom In");
    p_zoomIn2X->setIcon(QPixmap(toolIconDir() + "/viewmag+.png"));
    connect(p_zoomIn2X, SIGNAL(triggered()), this, SLOT(zoomIn2X()));

    p_zoomIn4X = new QAction(parent);
    p_zoomIn4X->setText("Zoom In 4X");
    p_zoomIn4X->setShortcut(Qt::CTRL + Qt::Key_Plus);
    connect(p_zoomIn4X, SIGNAL(triggered()), this, SLOT(zoomIn4X()));

    p_zoomIn8X = new QAction(parent);
    p_zoomIn8X->setShortcut(Qt::ALT + Qt::Key_Plus);
    p_zoomIn8X->setText("Zoom In 8X");
    connect(p_zoomIn8X, SIGNAL(triggered()), this, SLOT(zoomIn8X()));

    p_zoomOut2X = new QAction(parent);
    p_zoomOut2X->setShortcut(Qt::Key_Minus);
    p_zoomOut2X->setText("Zoom Out");
    p_zoomOut2X->setIcon(QPixmap(toolIconDir() + "/viewmag-.png"));
    connect(p_zoomOut2X, SIGNAL(triggered()), this, SLOT(zoomOut2X()));

    p_zoomOut4X = new QAction(parent);
    p_zoomOut4X->setShortcut(Qt::CTRL + Qt::Key_Minus);
    p_zoomOut4X->setText("Zoom Out 4X");
    connect(p_zoomOut4X, SIGNAL(triggered()), this, SLOT(zoomOut4X()));

    p_zoomOut8X = new QAction(parent);
    p_zoomOut8X->setShortcut(Qt::ALT + Qt::Key_Minus);
    p_zoomOut8X->setText("Zoom Out 8X");
    connect(p_zoomOut8X, SIGNAL(triggered()), this, SLOT(zoomOut8X()));

    p_zoomActual = new QAction(parent);
    p_zoomActual->setShortcut(Qt::Key_Slash);
    p_zoomActual->setText("&Actual Pixels");
    p_zoomActual->setIcon(QPixmap(toolIconDir() + "/viewmag1.png"));
    connect(p_zoomActual, SIGNAL(triggered()), this, SLOT(zoomActual()));

    p_zoomFit = new QAction(parent);
    p_zoomFit->setShortcut(Qt::Key_Asterisk);
    p_zoomFit->setText("&Fit in Window");
    p_zoomFit->setIcon(QPixmap(toolIconDir() + "/viewmagfit.png"));
    connect(p_zoomFit, SIGNAL(triggered()), this, SLOT(zoomFit()));

  }

  /**
   * Adds the action to the toolpad.  The icon used will be the magnifying
   * glass.  The tool tip reads "Zoom (Z)" with shortcut key "Z".
   *
   *
   * @param toolpad Toolpad to which the zoom tool will be added
   *
   * @return QAction* ZoomTool action defined by the cursor, ToolTip,
   */
  QAction *ZoomTool::toolPadAction(ToolPad *toolpad) {
    QAction *action = new QAction(toolpad);
    action->setIcon(QPixmap(toolIconDir() + "/viewmag.png"));
    action->setToolTip("Zoom (Z)");
    action->setShortcut(Qt::Key_Z);
    QString text  =
      "<b>Function:</b>  Zoom in or out of the current cube. \
      <p><b>Shortcut:</b>  Z</p> ";
    action->setWhatsThis(text);
    return action;
  }


  /**
   * Adds the zoom action to the given menu.  This will include the Zoom In (by
   * factor of 2), Zoom Out (by factor of 1/2), Zoom Actual (1:1) and Zoom Fit
   * actions.
   *
   *
   * @param menu Pointer to the QMenu
   */
  void ZoomTool::addTo(QMenu *menu) {
    menu->addAction(p_zoomFit);
    menu->addAction(p_zoomActual);
    menu->addAction(p_zoomIn2X);
    menu->addAction(p_zoomOut2X);
  }


  /**
   * Creates the widget to add to the tool bar.  For each button, this method
   * assigns the icons, ToolTips, WhatsThis, and connects a slot to the clicked
   * signal.  The following buttons are included
   * <UL>
   *   <LI> Zoom In - uses the magnifying glass with "+" icon and shortcut +
   *   <LI> Zoom Out - uses the magnifying glass with "-" icon and shortcut -
   *   <LI> Zoom 1:1 - uses the magnifying glass with "1:1" icon and shortcut /
   *   <LI> Fit in viewport - uses the magnifying glass with "dotted square"
   *   icon, shortcut * and drop down menu to choose to "Fit Width" or "Fit
   *   Height"
   *   <LI> Scale - Text box to manually enter scale
   * </UL>
   *
   * @param parent Parent stacked widget
   *
   * @return QWidget* Horizontal box to which the zoom tools icons will be added
   */
  QWidget *ZoomTool::createToolBarWidget(QStackedWidget *parent) {
    QWidget *hbox = new QWidget(parent);

    QToolButton *zoomInButton = new QToolButton(hbox);
    zoomInButton->setIcon(QPixmap(toolIconDir() + "/viewmag+.png"));
    zoomInButton->setToolTip("Zoom In");
    QString text =
      "<b>Function:</b> Zoom in 2X at the center of the active viewport \
      <p><b>Shortcut:</b>  +</p> \
      <p><b>Mouse:</b>  LeftButton zooms in 2X under pointer</p> \
      <p><b>Modifiers:</b>  Shortcuts and mouse clicks can be augmented \
      using the Ctrl or Alt key for 4X and 8X zooms, respectively</p> \
      <p><b>Hint:</b>  Left click and drag for a local zoom which scales data \
      in the red marquee to the viewport</p>";
    zoomInButton->setWhatsThis(text);
    connect(zoomInButton, SIGNAL(clicked()), this, SLOT(zoomIn2X()));
    zoomInButton->setAutoRaise(true);
    zoomInButton->setIconSize(QSize(22, 22));

    QToolButton *zoomOutButton = new QToolButton(hbox);
    zoomOutButton->setIcon(QPixmap(toolIconDir() + "/viewmag-.png"));
    zoomOutButton->setToolTip("Zoom Out");
    text =
      "<b>Function:</b> Zoom out 2X at the center of the active viewport \
      <p><b>Shortcut:</b>  +</p> \
      <p><b>Mouse:</b>  RightButton zooms out 2X under pointer</p> \
      <p><b>Modifiers:</b>  Shortcuts and mouse clicks can be augmented \
      using the Ctrl or Alt key for 4X and 8X zooms, respectively</p> \
      <p><b>Hint:</b>  Left click and drag for a local zoom which scales data \
      in the red marquee to the viewport</p>";
    zoomOutButton->setWhatsThis(text);
    connect(zoomOutButton, SIGNAL(clicked()), this, SLOT(zoomOut2X()));
    zoomOutButton->setAutoRaise(true);
    zoomOutButton->setIconSize(QSize(22, 22));

    QToolButton *zoomActButton = new QToolButton(hbox);
    zoomActButton->setIcon(QPixmap(toolIconDir() + "/viewmag1.png"));
    zoomActButton->setToolTip("Zoom 1:1");
    text =
      "<b>Function:</b> Zoom the active viewport to 1:1 such that one \
      viewport pixel represents one cube pixel. That is, 100% scale. \
      <p><b>Shortcut:</b> /</p> \
      <p><b>Mouse:</b>  Ctrl+MiddleButton zooms 1:1 under pointer</p> \
      <p><b>Hint:</b>  MiddleButton (without Ctrl) retains current \
      scale but moves the pixel under the pointer to the center of the \
      viewport</p>";
    zoomActButton->setWhatsThis(text);
    connect(zoomActButton, SIGNAL(clicked()), this, SLOT(zoomActual()));
    zoomActButton->setAutoRaise(true);
    zoomActButton->setIconSize(QSize(22, 22));

    // Create menu on the zoomFit button to select fitting the cube for
    // width or height.
    QMenu *zoomFitMenu = new QMenu();
    QAction *fitWidth = new QAction(this);
    fitWidth->setText("Fit Width");
    connect(fitWidth, SIGNAL(triggered(bool)), this, SLOT(zoomFitWidth()));
    zoomFitMenu->addAction(fitWidth);

    QAction *fitHeight = new QAction(this);
    fitHeight->setText("Fit Height");
    connect(fitHeight, SIGNAL(triggered(bool)), this, SLOT(zoomFitHeight()));
    zoomFitMenu->addAction(fitHeight);

    QToolButton *zoomFitButton = new QToolButton(hbox);
    zoomFitButton->setIcon(QPixmap(toolIconDir() + "/viewmagfit.png"));
    zoomFitButton->setMenu(zoomFitMenu);
    zoomFitButton->setPopupMode(QToolButton::MenuButtonPopup);
    zoomFitButton->setToolTip("Fit in viewport");
    text =
      "<b>Function:</b> Fit the entire cube inside the active viewport. For \
      extremely large cubes, this may not be possible. \
      <p><b>Shortcut:</b> *</p> \
      <p><b>Hint:</b>  Many shortcuts for the zoom tool and other tools \
      are easily available on the numeric keypad </p>";
    zoomFitButton->setWhatsThis(text);
    connect(zoomFitButton, SIGNAL(clicked()), this, SLOT(zoomFit()));
    zoomFitButton->setAutoRaise(true);
    zoomFitButton->setIconSize(QSize(22, 22));

    p_zoomLineEdit = new QLineEdit(hbox);
    p_zoomLineEdit->setText("");
    p_zoomLineEdit->setMaxLength(8);
    p_zoomLineEdit->setMaximumWidth(80);

    QDoubleValidator *dval = new QDoubleValidator(hbox);
    p_zoomLineEdit->setValidator(dval);

    QSizePolicy policy = p_zoomLineEdit->sizePolicy();
    policy.setHorizontalPolicy(QSizePolicy::Fixed);
    p_zoomLineEdit->setSizePolicy(policy);

    p_zoomLineEdit->setToolTip("Scale");
    text =
      "<b>Function:</b> Shows the scale of the active viewport.  Additionally, \
      you can manually enter the scale.";
    p_zoomLineEdit->setWhatsThis(text);
    connect(p_zoomLineEdit, SIGNAL(returnPressed()), this, SLOT(zoomManual()));

    QHBoxLayout *layout = new QHBoxLayout(hbox);
    layout->setMargin(0);
    layout->addWidget(zoomInButton);
    layout->addWidget(zoomOutButton);
    layout->addWidget(zoomActButton);
    layout->addWidget(zoomFitButton);
    layout->addWidget(p_zoomLineEdit);
    layout->addStretch(1);
    hbox->setLayout(layout);
    return hbox;
  }

  /**
   * Zooms in 2 times.
   *
   */
  void ZoomTool::zoomIn2X() {
    zoomBy(2.0);
  }


  /**
   * Zooms in 4 times.
   *
   */
  void ZoomTool::zoomIn4X() {
    zoomBy(4.0);
  }


  /**
   * Zooms in 8 times.
   *
   */
  void ZoomTool::zoomIn8X() {
    zoomBy(8.0);
  }


  /**
   * Zoom out 2 times.
   *
   */
  void ZoomTool::zoomOut2X() {
    zoomBy(1.0 / 2.0);
  }


  /**
   * Zoom out 4 times.
   *
   */
  void ZoomTool::zoomOut4X() {
    zoomBy(1.0 / 4.0);
  }


  /**
   * Zoom out 8 times.
   *
   */
  void ZoomTool::zoomOut8X() {
    zoomBy(1.0 / 8.0);
  }


  /**
   * Zoom back to 1 to 1.
   *
   */
  void ZoomTool::zoomActual() {
    // zoom factor passed in is 0
    // this will indicate to set new scale to 1 in zoomBy()
    zoomBy(0.0);
  }


  /**
   * Zoom by the given factor.
   *
   *
   * @param factor Zoom factor value
   * @internal
   *   @history 2010-07-12 Jeannie Walldren - Modified to call this object's
   *                          setScale method.
   *   @history 2010-07-12 Jeannie Walldren - Replaced checks for newScale==0
   *                          accidentally removed in previous commit.
   */
  void ZoomTool::zoomBy(double factor) {
    MdiCubeViewport *d = cubeViewport();
    if(d == NULL) return;
    double newScale = d->scale() * factor;
    if(newScale == 0.0) {
      // if zoomActual was called (1:1) the factor was set to 0.
      // change scale to 1.0
      newScale = 1.0;
    }
    setScale(d, newScale);
    updateTool();

    if(cubeViewport()->isLinked()) {
      for(int i = 0; i < (int)cubeViewportList()->size(); i++) {
        d = (*(cubeViewportList()))[i];
        if(d == cubeViewport()) continue;
        if(d->isLinked()) {
          newScale = d->scale() * factor;
          if(newScale == 0.0) {
            // if zoomActual was called (1:1) the factor was set to 0.
            // change scale to 1.0
            newScale = 1.0;
          }
          setScale(d, newScale);
        }
      }
    }
  }


  /**
   * Fits the cube in the viewport.
   * @internal
   *   @history 2010-07-12 Jeannie Walldren - Modified to call this object's
   *                          setScale method.
   *   @history 2017-05-10 Ian Humphrey - Modified so that the setScale() is accommodating the ISIS
   *                           pixel center definition (integer center) (+0.5). References #4756.
   */
  void ZoomTool::zoomFit() {
    MdiCubeViewport *d = cubeViewport();
    if (d == NULL) return;
    setScale(d, d->fitScale(), d->cubeSamples() / 2.0 + 0.5, d->cubeLines() / 2.0 + 0.5);
    updateTool();

    if (d->isLinked()) {
      for (int i = 0; i < (int)cubeViewportList()->size(); i++) {
        d = (*(cubeViewportList()))[i];
        if (d == cubeViewport()) continue;
        if (d->isLinked()) {
          setScale(d, d->fitScale(), d->cubeSamples() / 2.0 + 0.5, d->cubeLines() / 2.0 + 0.5);
        }
      }
    }
  }



  /**
   * Slot for the "Fit to Width" menu item on the Fit button.  This will
   * display the cube so that the entire cube width is displayed.
   * @internal
   *   @history 2010-07-12 Jeannie Walldren - Modified to call this object's
   *                          setScale method.
   */
  void ZoomTool::zoomFitWidth() {
    MdiCubeViewport *d = cubeViewport();
    if (d == NULL) return;
    setScale(d, d->fitScaleWidth(), d->cubeSamples() / 2.0 + 0.5, d->cubeLines() / 2.0 + 0.5);
    updateTool();

    if (d->isLinked()) {
      for (int i = 0; i < (int)cubeViewportList()->size(); i++) {
        d = (*(cubeViewportList()))[i];
        if (d == cubeViewport()) continue;
        if (d->isLinked()) {
          setScale(d, d->fitScaleWidth(), d->cubeSamples() / 2.0 + 0.5, d->cubeLines() / 2.0 + 0.5);
        }
      }
    }
  }


  /**
   * Slot for the "Fit to Heighth" menu item on the Fit button.  This will display
   * the cube so that the entire cube heighth is displayed.
   *
   * @internal
   *   @history 2010-07-12 Jeannie Walldren - Modified to call this object's
   *                          setScale method.
   */
  void ZoomTool::zoomFitHeight() {
    MdiCubeViewport *d = cubeViewport();
    if (d == NULL) return;
    setScale(d, d->fitScaleHeight(), d->cubeSamples() / 2.0 + 0.5, d->cubeLines() / 2.0 + 0.5);
    updateTool();

    if (d->isLinked()) {
      for (int i = 0; i < (int)cubeViewportList()->size(); i++) {
        d = (*(cubeViewportList()))[i];
        if (d == cubeViewport()) continue;
        if (d->isLinked()) {
          setScale(d, d->fitScaleHeight(), d->cubeSamples() / 2.0 + 0.5,
              d->cubeLines() / 2.0 + 0.5);
        }
      }
    }
  }


  /**
   * This method zooms by the value input in the line edit next to
   * the zoom tools.
   *
   * @internal
   *   @history 2010-07-12 Jeannie Walldren - Modified to call this object's
   *                          setScale method.
   */
  void ZoomTool::zoomManual() {
    MdiCubeViewport *d = cubeViewport();
    if(d == NULL) return;
    QString strScale = p_zoomLineEdit->text();
    double newScale = strScale.toDouble() / 100.;
    setScale(d, newScale);
    d->setFocus();
    updateTool();

    if(d->isLinked()) {
      for(int i = 0; i < (int)cubeViewportList()->size(); i++) {
        d = (*(cubeViewportList()))[i];
        if(d == cubeViewport()) continue;
        if(d->isLinked()) setScale(d, newScale);
      }
    }
  }


  /**
   * This method updates the line edits text to the correct zoom
   * value.
   *
   */
  void ZoomTool::updateTool() {
    if(cubeViewport() == NULL) {
      p_zoomLineEdit->setText("");
    }
    else {
      double scale = cubeViewport()->scale() * 100.0;
      QString strScale;
      strScale.setNum(scale);
      p_zoomLineEdit->setText(strScale);
    }
  }


  /**
   * This method is called when the RubberBandTool is complete. It
   * will either zoom the CubeViewport to the rectangle specified
   * by the RubberBandTool or will handle different zoom methods
   * specified by the last RubberBandTool's  mouse button.
   *
   * @internal
   *   @history 2010-07-12 Jeannie Walldren - Modified to call this object's
   *                          setScale method.
   *   @history 2010-07-12 Jeannie Walldren - Replaced checks for newScale==0
   *                          accidentally removed in previous commit.
   */
  void ZoomTool::rubberBandComplete() {
    QApplication::processEvents();
    MdiCubeViewport *d = cubeViewport();
    if(!rubberBandTool()->isValid()) return;

    // The RubberBandTool has a rectangle
    if(!rubberBandTool()->figureIsPoint()) {
      QRect r = rubberBandTool()->rectangle();
      if((r.width() >= 5) && (r.height() >= 5)) {
        int x = r.x() + r.width() / 2;
        int y = r.y() + r.height() / 2;
        double xscale = (double) d->viewport()->width() / r.width();
        double yscale = (double) d->viewport()->height() / r.height();
        double newScale = xscale < yscale ? xscale : yscale;
        if(rubberBandTool()->mouseButton() & Qt::RightButton) {
          newScale = 1.0 / newScale;
        }
        newScale *= d->scale();
        setScale(d, newScale, x, y);
        updateTool();
        if(d->isLinked()) {
          for(int i = 0; i < (int)cubeViewportList()->size(); i++) {
            d = (*(cubeViewportList()))[i];
            if(d == cubeViewport()) continue;
            if(d->isLinked()) {
              int x = r.x() + r.width() / 2;
              int y = r.y() + r.height() / 2;
              double xscale = (double) d->viewport()->width() / r.width();
              double yscale = (double) d->viewport()->height() / r.height();
              double newScale = xscale < yscale ? xscale : yscale;
              if(rubberBandTool()->mouseButton() & Qt::RightButton) {
                newScale = 1.0 / newScale;
              }
              newScale *= d->scale();
              setScale(d, newScale, x, y);
            }
          }
        }
      }
    }
    // The RubberBandTool has a point (mouse click)
    else {
      double factor = 2.0;
      if(rubberBandTool()->mouseButton() & Qt::ControlModifier) {
        factor = 4.0;
      }
      if(rubberBandTool()->mouseButton() & Qt::ShiftModifier) {
        factor = 8.0;
      }
      if(rubberBandTool()->mouseButton() & Qt::RightButton) {
        factor = 1.0 / factor;
      }
      if(rubberBandTool()->mouseButton() & Qt::MiddleButton) {
        factor = 1.0;
      }
      if(rubberBandTool()->mouseButton() == Qt::MiddleButton + Qt::ControlModifier) {
        factor = 0.0;
      }
//      MdiCubeViewport *d = cubeViewport();
      double newScale = d->scale() * factor;
      if(newScale == 0.0) {
        // ctrl+middle (1:1) the factor was set to 0.
        // change scale to 1.0
        newScale = 1.0;
      }
      QPoint p = rubberBandTool()->vertices()[0];
      setScale(d, newScale, p.x(), p.y());
      updateTool();

      if(d->isLinked()) {
        for(int i = 0; i < (int)cubeViewportList()->size(); i++) {
          d = (*(cubeViewportList()))[i];
          if(d == cubeViewport()) continue;
          if(d->isLinked()) {
            newScale = d->scale() * factor;
            if(newScale == 0.0) {
              // ctrl+middle (1:1) the factor was set to 0.
              // change scale to 1.0
              newScale = 1.0;
            }
            newScale = setScale(d, newScale, p.x(), p.y());
          }
        }
      }
      p_lastScale = newScale;
    }
  }


  /**
   * This methods enables the RubberBandTool, it also sets the
   * RubberBandTool to allow points and to allow all clicks.
   *
   */
  void ZoomTool::enableRubberBandTool() {
    rubberBandTool()->enable(RubberBandTool::RectangleMode);
    rubberBandTool()->enablePoints();
    rubberBandTool()->enableAllClicks();
    rubberBandTool()->setDrawActiveViewportOnly(false);
  }

  /**
   * This method will attempt to reset the scale for the given MdiCubeViewport
   * using the new scale value.  If this fails, a message box will pop up.
   *
   * @param d Pointer to MdiCubeViewport
   * @param newScale New scale value of the cube
   * @return @b double The scale value used.  If the passed in value fails, this
   *         will be the previous scale value.
   * @throw iException::User "Scale value must be greater than 0."
   * @throw iException::User "Unable to rescale image."
   * @internal
   * @author Jeannie Walldren
   *   @history 2010-07-12 Jeannie Walldren - Original version.
   *   @history 2010-07-14 Jeannie Walldren - Added error message if the new
   *                          scale value is less than or equal to 0.
   *
   */
  double ZoomTool::setScale(MdiCubeViewport *d, double newScale) {

    double oldScale = d->scale();
    try {
      if (newScale <= 0.0) {
        throw IException(IException::User,
                         "Scale value must be greater than 0.",
                         _FILEINFO_);
      }
      d->setScale(newScale);
    }
    catch (IException &e) {
      IException fullError(e,
                           IException::User,
                           "Unable to rescale image to ["
                           + IString(newScale*100) + "]",
                           _FILEINFO_);
      std::string message = fullError.toString();
      QMessageBox::warning((QWidget *)parent(), "Warning", message);
      newScale = oldScale;
      d->setScale(newScale);
    }
    return newScale;
  }


  /**
   * This method will attempt to reset the scale for the given MdiCubeViewport at
   * the x, y values using the new scale value.  If this fails, a message box will
   * pop up.
   *
   * @param d Pointer to MdiCubeViewport
   * @param newScale New scale value of the cube
   * @param x
   * @param y
   * @return @b double The scale value used.  If the passed in value fails, this
   *         will be the previous scale value.
   * @throw iException::User "Scale value must be greater than 0."
   * @throw iException::User "Unable to rescale image."
   * @internal
   * @author Jeannie Walldren
   *   @history 2010-07-12 Jeannie Walldren - Original version.
   *   @history 2010-07-14 Jeannie Walldren - Added error message if the new
   *                          scale value is less than or equal to 0.
   */

  double ZoomTool::setScale(MdiCubeViewport *d, double newScale, int x, int y) {
    double oldScale = d->scale();
    try {
      if (newScale <= 0.0) {
        throw IException(IException::User,
            "Scale value must be greater than 0.", _FILEINFO_);
      }
      d->setScale(newScale, x, y);
    }
    catch (IException &e) {
      IException fullError(e, IException::User,
                           "Unable to rescale image to ["
                           + IString(newScale * 100) + "]",
                           _FILEINFO_);
      std::string message = fullError.toString();
      QMessageBox::warning((QWidget *)parent(), "Warning", message);
      newScale = oldScale;
      d->setScale(newScale, x, y);
    }
    return newScale;
  }

  /**
   * This method will attempt to reset the scale for the given MdiCubeViewport at
   * the x, y values using the new scale value.  If this fails, a message box will
   * pop up.
   *
   * @param d Pointer to MdiCubeViewport
   * @param newScale New scale value of the cube
   * @param samp
   * @param line
   * @return @b double The scale value used.  If the passed in value fails, this
   *         will be the previous scale value.
   * @throw iException::User "Scale value must be greater than 0."
   * @throw iException::User "Unable to rescale image."
   * @internal
   * @author Jeannie Walldren
   *   @history 2010-07-12 Jeannie Walldren - Original version.
   *   @history 2010-07-14 Jeannie Walldren - Added error message if the new
   *                          scale value is less than or equal to 0.
   */

  double ZoomTool::setScale(MdiCubeViewport *d, double newScale, double samp, double line) {
    double oldScale = d->scale();
    try {
      if (newScale <= 0.0) {
        throw IException(IException::User,
            "Scale value must be greater than 0.", _FILEINFO_);
      }
      d->setScale(newScale, samp, line);
    }
    catch (IException &e) {
      IException fullError(e,
                           IException::User,
                           "Unable to rescale image to ["
                           + IString(newScale*100) + "]",
                           _FILEINFO_);
      std::string message = fullError.toString();
      QMessageBox::warning((QWidget *)parent(), "Warning", message);
      newScale = oldScale;
      d->setScale(newScale, samp, line);
    }
    return newScale;
  }

}
