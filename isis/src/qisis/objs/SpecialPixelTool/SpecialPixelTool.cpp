#include "SpecialPixelTool.h"

#include <QCheckBox>
#include <QColorDialog>
#include <QDialog>
#include <QHBoxLayout>
#include <QLabel>
#include <QMenu>
#include <QPainter>
#include <QPalette>
#include <QPixmap>
#include <QPushButton>
#include <QToolBar>
#include <QToolButton>
#include <QVBoxLayout>

#include "FileName.h"
#include "MainWindow.h"
#include "MdiCubeViewport.h"
#include "Stretch.h"
#include "CubeStretch.h"
#include "Workspace.h"

namespace Isis {
  /**
   * SpecialPixelTool constructor
   *
   *
   * @param parent
   */
  SpecialPixelTool::SpecialPixelTool(QWidget *parent) : Tool(parent) {
    // Create the SpecialPixel window
    p_parent = parent;

    p_dialog = new QDialog(parent);
    p_dialog->setWindowTitle("Special Pixel Tool");
    p_dialog->setSizeGripEnabled(true);
    p_spWindow = new QWidget(p_dialog);
    p_spWindow->setMinimumSize(492, 492);
    p_spWindow->installEventFilter(this);

    QWidget *buttons = new QWidget(p_dialog);
    QWidget *colors = new QWidget(p_dialog);
    QWidget *labels = new QWidget(p_dialog);
    QWidget *defaults = new QWidget(p_dialog);
    QWidget *main = new QWidget(p_dialog);

    QVBoxLayout *layout = new QVBoxLayout();
    layout->addWidget(defaults, 0);
    layout->addWidget(main, 0);
    layout->addWidget(buttons, 0);
    p_dialog->setLayout(layout);

    QPushButton *ok = new QPushButton("Ok", buttons);
    ok->setShortcut(Qt::Key_Enter);
    connect(ok, SIGNAL(released()), this, SLOT(apply()));
    connect(ok, SIGNAL(released()), p_dialog, SLOT(hide()));

    QPushButton *apply = new QPushButton("Apply", buttons);
    connect(apply, SIGNAL(released()), this, SLOT(apply()));

    QPushButton *cancel = new QPushButton("Cancel", buttons);
    connect(cancel, SIGNAL(released()), p_dialog, SLOT(hide()));

    QPushButton *defaultBlackWhite = new QPushButton("Default B&W", defaults);
    connect(defaultBlackWhite, SIGNAL(released()), this, SLOT(defaultBW()));

    QPushButton *defaultColor = new QPushButton("Default Color", defaults);
    connect(defaultColor, SIGNAL(released()), this, SLOT(defaultColor()));

    QHBoxLayout *hlayout = new QHBoxLayout();
    hlayout->addWidget(ok);
    hlayout->addWidget(apply);
    hlayout->addWidget(cancel);
    //  hlayout->addStretch(1);
    buttons->setLayout(hlayout);

    QHBoxLayout *h2layout = new QHBoxLayout();
    h2layout->addWidget(defaultBlackWhite);
    h2layout->addWidget(defaultColor);
    defaults->setLayout(h2layout);

    p_nullColor = new QToolButton(p_dialog);
    connect(p_nullColor, SIGNAL(released()), this, SLOT(setNullColor()));
    QSize *size = new QSize(25, 25);
    QLabel *nullLabel = new QLabel("Null");
    p_nullColor->setFixedSize(*size);

    p_lisColor = new QToolButton(p_dialog);
    connect(p_lisColor, SIGNAL(released()), this, SLOT(setLisColor()));
    QLabel *lisLabel = new QLabel("Low Instrument Saturation");
    p_lisColor->setFixedSize(*size);

    p_lrsColor = new QToolButton(p_dialog);
    connect(p_lrsColor, SIGNAL(released()), this, SLOT(setLrsColor()));
    QLabel *lrsLabel = new QLabel("Low Representation Saturation");
    p_lrsColor->setFixedSize(*size);

    p_ldsColor = new QToolButton(p_dialog);
    connect(p_ldsColor, SIGNAL(released()), this, SLOT(setLdsColor()));
    QLabel *ldsLabel = new QLabel("Low Display Saturation");
    p_ldsColor->setFixedSize(*size);

    p_hisColor = new QToolButton(p_dialog);
    connect(p_hisColor, SIGNAL(released()), this, SLOT(setHisColor()));
    QLabel *hisLabel = new QLabel("High Instrument Saturation");
    p_hisColor->setFixedSize(*size);

    p_hrsColor = new QToolButton(p_dialog);
    connect(p_hrsColor, SIGNAL(released()), this, SLOT(setHrsColor()));
    QLabel *hrsLabel = new QLabel("High Representation Saturation");
    p_hrsColor->setFixedSize(*size);

    p_hdsColor = new QToolButton(p_dialog);
    connect(p_hdsColor, SIGNAL(released()), this, SLOT(setHdsColor()));
    QLabel *hdsLabel = new QLabel("High Display Saturation");
    p_hdsColor->setFixedSize(*size);

    p_bgColor = new QToolButton(p_dialog);
    connect(p_bgColor, SIGNAL(released()), this, SLOT(setBgColor()));
    QLabel *bgLabel = new QLabel("Background");
    p_bgColor->setFixedSize(*size);

    connect(this, SIGNAL(setDefaultColors()), this, SLOT(defaultBW()));
    emit setDefaultColors();

    QVBoxLayout *vlayout = new QVBoxLayout();
    vlayout->addWidget(nullLabel);
    vlayout->addWidget(lisLabel);
    vlayout->addWidget(hisLabel);
    vlayout->addWidget(lrsLabel);
    vlayout->addWidget(hrsLabel);
    vlayout->addWidget(ldsLabel);
    vlayout->addWidget(hdsLabel);
    vlayout->addWidget(bgLabel);
    labels->setLayout(vlayout);

    QVBoxLayout *v2layout = new QVBoxLayout();
    v2layout->addWidget(p_nullColor);
    v2layout->addWidget(p_lisColor);
    v2layout->addWidget(p_hisColor);
    v2layout->addWidget(p_lrsColor);
    v2layout->addWidget(p_hrsColor);
    v2layout->addWidget(p_ldsColor);
    v2layout->addWidget(p_hdsColor);
    v2layout->addWidget(p_bgColor);
    colors->setLayout(v2layout);

    QHBoxLayout *mainlayout = new QHBoxLayout();
    mainlayout->addWidget(colors);
    mainlayout->addWidget(labels);
    main->setLayout(mainlayout);

    // Create the action to bring up the SpecialPixel window
    p_action = new QAction(parent);
    //p_action->setShortcut(Qt::CTRL+Qt::Key_C);
    p_action->setText("&Special Pixel Tool ...");
    p_action->setIcon(QPixmap(toolIconDir() + "/colorize.png"));
    p_action->setToolTip("SpecialPixelTool");
    QString text =
      "<b>Function:</b> Opens a window that allows you to chose what color to \
       display each different type of special pixel \
       <p><b>Shortcut:</b> Ctrl+C</p>";
    p_action->setWhatsThis(text);
    p_action->setEnabled(false);
    connect(p_action, SIGNAL(triggered()), p_dialog, SLOT(show()));

    readSettings();
  }


  /**
   * Adds the tool to the given menu.
   *
   *
   * @param menu
   */
  void SpecialPixelTool::addTo(QMenu *menu) {
    menu->addAction(p_action);
  }


  /**
   * Adds the tool to the permanent tool bar.
   *
   *
   * @param perm
   */
  void SpecialPixelTool::addToPermanent(QToolBar *perm) {
    perm->addAction(p_action);
  }
  
  
  /**
   * 
   * @param ws
   */
  void SpecialPixelTool::addTo(Workspace *ws) {
    Tool::addTo(ws);
    connect(ws, SIGNAL(cubeViewportAdded(MdiCubeViewport *)),
            this, SLOT(apply()));
  }


  /**
   * Applies the colors picked for the special pixels.
   *
   */
  void SpecialPixelTool::apply() {
    for(int i = 0; i < (int)cubeViewportList()->size(); i++) {
      MdiCubeViewport *cvp = (*(cubeViewportList()))[i];

      // Get the Stretch objects from the cubeViewport
      CubeStretch redStretch = cvp->redStretch();
      CubeStretch greenStretch = cvp->greenStretch();
      CubeStretch blueStretch = cvp->blueStretch();

      // Apply selected null color
      QPalette palette = p_nullColor->palette();
      QColor nullColor = palette.color(QPalette::Button);
      int r, g, b;
      nullColor.getRgb(&r, &g, &b);
      redStretch.SetNull(r);
      greenStretch.SetNull(g);
      blueStretch.SetNull(b);

      // Apply selected lis
      palette = p_lisColor->palette();
      QColor lisColor = palette.color(QPalette::Button);
      lisColor.getRgb(&r, &g, &b);
      redStretch.SetLis(r);
      greenStretch.SetLis(g);
      blueStretch.SetLis(b);

      // Apply selected lrs
      palette = p_lrsColor->palette();
      QColor lrsColor = palette.color(QPalette::Button);
      lrsColor.getRgb(&r, &g, &b);
      redStretch.SetLrs(r);
      greenStretch.SetLrs(g);
      blueStretch.SetLrs(b);

      // Apply selected lds
      palette = p_ldsColor->palette();
      QColor ldsColor = palette.color(QPalette::Button);
      ldsColor.getRgb(&r, &g, &b);
      redStretch.SetMinimum(r);
      greenStretch.SetMinimum(g);
      blueStretch.SetMinimum(b);

      // Apply selected his
      palette = p_hisColor->palette();
      QColor hisColor = palette.color(QPalette::Button);
      hisColor.getRgb(&r, &g, &b);
      redStretch.SetHis(r);
      greenStretch.SetHis(g);
      blueStretch.SetHis(b);

      // Apply selected hrs
      palette = p_hrsColor->palette();
      QColor hrsColor = palette.color(QPalette::Button);
      hrsColor.getRgb(&r, &g, &b);
      redStretch.SetHrs(r);
      greenStretch.SetHrs(g);
      blueStretch.SetHrs(b);

      // Apply selected hds
      palette = p_hdsColor->palette();
      QColor hdsColor = palette.color(QPalette::Button);
      hdsColor.getRgb(&r, &g, &b);
      redStretch.SetMaximum(r);
      greenStretch.SetMaximum(g);
      blueStretch.SetMaximum(b);

      // Apply selected background
      palette = p_bgColor->palette();
      QColor bgColor = palette.color(QPalette::Button);
      bgColor.getRgb(&r, &g, &b);

      cvp->setBackground(bgColor);
      cvp->stretchRed(redStretch);
      cvp->stretchGreen(greenStretch);
      cvp->stretchBlue(blueStretch);

      //If any of the defaults changed, make sure to write them
      if(p_color) {
        p_nullDefault = nullColor;
        p_lisDefault = lisColor;
        p_lrsDefault = lrsColor;
        p_ldsDefault = ldsColor;
        p_hisDefault = hisColor;
        p_hrsDefault = hrsColor;
        p_hdsDefault = hdsColor;
        p_bgDefault = bgColor;
        writeSettings();
      }
    }
  }


  /**
   * Sets the color for null pixels.
   *
   */
  void SpecialPixelTool::setNullColor() {
    setColor(p_nullColor);
  }


  /**
   * Sets the color for Lis pixels.
   *
   */
  void SpecialPixelTool::setLisColor() {
    setColor(p_lisColor);
  }


  /**
   * Sets the color for Lrs pixels.
   *
   */
  void SpecialPixelTool::setLrsColor() {
    setColor(p_lrsColor);
  }


  /**
   * Sets the color for Lds pixels.
   *
   */
  void SpecialPixelTool::setLdsColor() {
    setColor(p_ldsColor);
  }


  /**
   * Sets the color for His pixels.
   *
   */
  void SpecialPixelTool::setHisColor() {
    setColor(p_hisColor);
  }


  /**
   * Sets the color for Hrs pixels.
   *
   */
  void SpecialPixelTool::setHrsColor() {
    setColor(p_hrsColor);
  }


  /**
   * Sets the color for Hds pixels.
   *
   */
  void SpecialPixelTool::setHdsColor() {
    setColor(p_hdsColor);
  }

  void SpecialPixelTool::setBgColor() {
    setColor(p_bgColor);
  }

  /**
   * Gets the selected color from the color dialog.
   *
   *
   * @param button
   */
  void SpecialPixelTool::setColor(QToolButton *button) {
    // Let the user pick a color
    QColor color = QColorDialog::getColor();

    // Set the color if they didnt cancel out of the window
    if(color.isValid()) {
      QPalette *palette = new QPalette();
      palette->setColor(QPalette::Button, color);
      button->setPalette(*palette);
    }

  }


  /**
   * Reset the default black/white colors.
   *
   */
  void SpecialPixelTool::defaultBW() {
    p_color = false;
    QPalette *palette = new QPalette();
    palette->setColor(QPalette::Button, Qt::black);
    p_nullColor->setPalette(*palette);
    p_lisColor->setPalette(*palette);
    p_lrsColor->setPalette(*palette);
    p_ldsColor->setPalette(*palette);
    p_bgColor->setPalette(*palette);

    palette->setColor(QPalette::Button, Qt::white);
    p_hisColor->setPalette(*palette);
    p_hrsColor->setPalette(*palette);
    p_hdsColor->setPalette(*palette);
  }


  /**
   * Reset the default color colors.
   *
   */
  void SpecialPixelTool::defaultColor() {
    p_color = true;
    QPalette *palette = new QPalette();
    palette->setColor(QPalette::Button, p_nullDefault);
    p_nullColor->setPalette(*palette);

    palette->setColor(QPalette::Button, p_lisDefault);
    p_lisColor->setPalette(*palette);

    palette->setColor(QPalette::Button, p_lrsDefault);
    p_lrsColor->setPalette(*palette);

    palette->setColor(QPalette::Button, p_ldsDefault);
    p_ldsColor->setPalette(*palette);

    palette->setColor(QPalette::Button, p_hisDefault);
    p_hisColor->setPalette(*palette);

    palette->setColor(QPalette::Button, p_hrsDefault);
    p_hrsColor->setPalette(*palette);

    palette->setColor(QPalette::Button, p_hdsDefault);
    p_hdsColor->setPalette(*palette);

    palette->setColor(QPalette::Button, p_bgDefault);
    p_bgColor->setPalette(*palette);
  }


  /**
   * Updates special pixel tool.
   *
   */
  void SpecialPixelTool::updateTool() {
    if(cubeViewport() == NULL) {
      p_action->setEnabled(false);
    }
    else {
      p_action->setEnabled(true);
    }
  }


  /**
   * This method reads in the default special pixel value colors
   * from a config file.
   *
   */
  void SpecialPixelTool::readSettings() {
    /*Now read the settings that are specific to this window.*/
    QString appName = p_parent->windowTitle();

    /*Now read the settings that are specific to this window.*/
    QString instanceName = p_dialog->windowTitle();

    FileName config("$HOME/.Isis/" + appName.toStdString() + "/" + instanceName.toStdString() + ".config");
    p_settings = new QSettings(QString::fromStdString(config.expanded()), QSettings::NativeFormat);

    //For each special pixel value, if it exists set it, otherwise set
    //it to the system defaults.

    //Default value for Null
    if(p_settings->value("defaultNull", 1).toInt() == 0) {
      p_nullDefault = p_settings->value("defaultNull", 1).value<QColor>();
    }
    else {
      p_nullDefault = Qt::blue;
    }

    //Default value for Lis
    if(p_settings->value("defaultLis", 1).toInt() == 0) {
      p_lisDefault = p_settings->value("defaultLis", 1).value<QColor>();
    }
    else {
      p_lisDefault = Qt::cyan;
    }

    //Default value for Lrs
    if(p_settings->value("defaultLrs", 1).toInt() == 0) {
      p_lrsDefault = p_settings->value("defaultLrs", 1).value<QColor>();
    }
    else {
      p_lrsDefault = Qt::yellow;
    }

    //Default value for Lds
    if(p_settings->value("defaultLds", 1).toInt() == 0) {
      p_ldsDefault = p_settings->value("defaultLds", 1).value<QColor>();
    }
    else {
      p_ldsDefault = Qt::black;
    }

    //Default value for His
    if(p_settings->value("defaultHis", 1).toInt() == 0) {
      p_hisDefault = p_settings->value("defaultHis", 1).value<QColor>();
    }
    else {
      p_hisDefault = Qt::magenta;
    }

    //Default value for Hrs
    if(p_settings->value("defaultHrs", 1).toInt() == 0) {
      p_hrsDefault = p_settings->value("defaultHrs", 1).value<QColor>();
    }
    else {
      p_hrsDefault = Qt::green;
    }

    //Default value for Hds
    if(p_settings->value("defaultHds", 1).toInt() == 0) {
      p_hdsDefault = p_settings->value("defaultHds", 1).value<QColor>();
    }
    else {
      p_hdsDefault = Qt::white;
    }

    //Default value for Bg
    if(p_settings->value("defaultBg", 1).toInt() == 0) {
      p_bgDefault = p_settings->value("defaultBg", 1).value<QColor>();
    }
    else {
      p_bgDefault = Qt::black;
    }
  }


  /**
   * This methods writes the default special pixel values to a
   * config file that will be read by the readSettings() method.
   *
   */
  void SpecialPixelTool::writeSettings() {
    QString appName = p_parent->windowTitle();

    /*Now read the settings that are specific to this window.*/
    QString instanceName = p_dialog->windowTitle();

    //Write all of the special pixel value colors
    FileName config("$HOME/.Isis/" + appName.toStdString() + "/" + instanceName.toStdString() + ".config");
    QSettings settings(QString::fromStdString(config.expanded()), QSettings::NativeFormat);
    settings.setValue("defaultNull", p_nullDefault);
    settings.setValue("defaultLis", p_lisDefault);
    settings.setValue("defaultLrs", p_lrsDefault);
    settings.setValue("defaultLds", p_ldsDefault);
    settings.setValue("defaultHis", p_hisDefault);
    settings.setValue("defaultHrs", p_hrsDefault);
    settings.setValue("defaultHds", p_hdsDefault);
    settings.setValue("defaultBg", p_bgDefault);
  }
}
