#include "PanTool.h"

#include <QAction>
#include <QComboBox>
#include <QHBoxLayout>
#include <QLineEdit>
#include <QLabel>
#include <QMenu>
#include <QMenuBar>
#include <QMessageBox>
#include <QStackedWidget>
#include <QToolButton>

#include "IString.h"
#include "MainWindow.h"
#include "MdiCubeViewport.h"
#include "ToolPad.h"

namespace Isis {
  PanTool::PanTool(QWidget *parent) : Tool(parent) {
    p_panRight = new QAction(parent);
    p_panRight->setShortcut(Qt::CTRL + Qt::Key_Right);
    p_panRight->setText("&Pan Right");
    p_panRight->setIcon(QPixmap(toolIconDir() + "/forward.png"));
    connect(p_panRight, SIGNAL(triggered()), this, SLOT(panRight()));

    p_panLeft = new QAction(parent);
    p_panLeft->setShortcut(Qt::CTRL + Qt::Key_Left);
    p_panLeft->setText("&Pan Left");
    p_panLeft->setIcon(QPixmap(toolIconDir() + "/back.png"));
    connect(p_panLeft, SIGNAL(triggered()), this, SLOT(panLeft()));

    p_panUp = new QAction(parent);
    p_panUp->setShortcut(Qt::CTRL + Qt::Key_Up);
    p_panUp->setText("&Pan Up");
    p_panUp->setIcon(QPixmap(toolIconDir() + "/up.png"));
    connect(p_panUp, SIGNAL(triggered()), this, SLOT(panUp()));

    p_panDown = new QAction(parent);
    p_panDown->setShortcut(Qt::CTRL + Qt::Key_Down);
    p_panDown->setText("&Pan Down");
    p_panDown->setIcon(QPixmap(toolIconDir() + "/down.png"));
    connect(p_panDown, SIGNAL(triggered()), this, SLOT(panDown()));

    p_dragPan = false;
  }

  QAction *PanTool::toolPadAction(ToolPad *pad) {
    QAction *action = new QAction(pad);
    action->setIcon(QPixmap(toolIconDir() + "/move.png"));
    action->setToolTip("Pan (P)");
    action->setShortcut(Qt::Key_P);
    QString text  =
      "<b>Function:</b>  View different areas of the cube. \
      <p><b>Shortcut:</b>  P</p> ";
    action->setWhatsThis(text);
    return action;
  }

  void PanTool::addTo(QMenu *menu) {
    menu->addAction(p_panLeft);
    menu->addAction(p_panRight);
    menu->addAction(p_panUp);
    menu->addAction(p_panDown);
  }

  QWidget *PanTool::createToolBarWidget(QStackedWidget *parent) {
    QWidget *hbox = new QWidget(parent);

    QToolButton *leftButton = new QToolButton(hbox);
    leftButton->setIcon(QPixmap(toolIconDir() + "/back.png"));
    leftButton->setToolTip("Pan Left");
    QString text =
      "<b>Function: </b>Pan cube in the active viewport to the left \
      <p><b>Shortcut:</b> Ctrl+LeftArrow</p> \
      <p><b>Mouse:</b> Hold LeftButton and drag pointer to the right</p> \
      <p><b>Hint:</b> Arrow keys without Ctrl modifier moves \
      the mouse pointer</p>";

    leftButton->setWhatsThis(text);
    leftButton->setAutoRaise(true);
    leftButton->setIconSize(QSize(22, 22));
    connect(leftButton, SIGNAL(clicked()), this, SLOT(panLeft()));

    QToolButton *rightButton = new QToolButton(hbox);
    rightButton->setIcon(QPixmap(toolIconDir() + "/forward.png"));
    rightButton->setToolTip("Pan Right");
    text =
      "<b>Function: </b>Pan cube in the active viewport to the right \
      <p><b>Shortcut:</b> Ctrl+RightArrow</p> \
      <p><b>Mouse:</b> Hold LeftButton and drag pointer to the left</p>\
      <p><b>Hint:</b> Arrow keys without Ctrl modifier moves \
      the mouse pointer</p>";
    rightButton->setWhatsThis(text);
    rightButton->setAutoRaise(true);
    rightButton->setIconSize(QSize(22, 22));
    connect(rightButton, SIGNAL(clicked()), this, SLOT(panRight()));

    QToolButton *upButton = new QToolButton(hbox);
    upButton->setIcon(QPixmap(toolIconDir() + "/up.png"));
    upButton->setToolTip("Pan Up");
    text =
      "<b>Function: </b>Pan cube in the active viewport up \
      <p><b>Shortcut:</b> Ctrl+UpArrow</p> \
      <p><b>Mouse:</b> Hold LeftButton and drag pointer down</p> \
      <p><b>Hint:</b> Arrow keys without Ctrl modifier moves \
      the mouse pointer</p>";
    upButton->setWhatsThis(text);
    upButton->setAutoRaise(true);
    upButton->setIconSize(QSize(22, 22));
    connect(upButton, SIGNAL(clicked()), this, SLOT(panUp()));

    QToolButton *downButton = new QToolButton(hbox);
    downButton->setIcon(QPixmap(toolIconDir() + "/down.png"));
    downButton->setToolTip("Pan Down");
    text =
      "<b>Function: </b>Pan cube in the active viewport down \
      <p><b>Shortcut:</b> Ctrl+DownArrow</p> \
      <p><b>Mouse:</b> Hold LeftButton and drag pointer up</p> \
      <p><b>Hint:</b> Arrow keys without Ctrl modifier moves \
      the mouse pointer</p>";
    downButton->setWhatsThis(text);
    downButton->setAutoRaise(true);
    downButton->setIconSize(QSize(22, 22));
    connect(downButton, SIGNAL(clicked()), this, SLOT(panDown()));

    p_panRateBox = new QComboBox(hbox);
    p_panRateBox->addItem("1/4 Screen");
    p_panRateBox->addItem("1/2 Screen");
    p_panRateBox->addItem("3/4 Screen");
    p_panRateBox->addItem("Full Screen");
    p_panRateBox->addItem("Custom");
    p_panRateBox->setToolTip("Pan Rate");
    text =
      "<b>Function: </b>Change the rate of panning when using the pan buttons \
      or Ctrl+ArrowKeys";
    p_panRateBox->setWhatsThis(text);
    //p_panRateBox->setCurrentIndex(4);
    connect(p_panRateBox, SIGNAL(activated(int)),
            this, SLOT(updateLineEdit()));

    p_lineEdit = new QLineEdit();
    p_lineEdit->setFixedWidth(50);
    p_lineEdit->setToolTip("Custom Pan Rate");
    text = "<b>Function: </b>Enter a custom percentage pan rate";
    p_lineEdit->setWhatsThis(text);
    //p_lineEdit->setText("75");
    connect(p_lineEdit, SIGNAL(returnPressed()),
            this, SLOT(writeSettings()));

    QIntValidator *ival = new QIntValidator(hbox);
    ival->setRange(1, 100);
    p_lineEdit->setValidator(ival);
    QLabel *percent = new QLabel("%");

    QHBoxLayout *layout = new QHBoxLayout(hbox);
    layout->setMargin(0);
    layout->addWidget(leftButton);
    layout->addWidget(rightButton);
    layout->addWidget(upButton);
    layout->addWidget(downButton);
    layout->addWidget(p_panRateBox);
    layout->addWidget(p_lineEdit);
    layout->addWidget(percent);
    layout->addStretch(1);
    hbox->setLayout(layout);

    readSettings();

    return hbox;
  }

  int PanTool::panRate(bool horz) {
    MdiCubeViewport *d = cubeViewport();
    if(d == NULL) return 0;
    if(p_lineEdit->text() == "") {
      QString text = "You must enter a value in the text box \n "
                     "to use the Custom pan percentage option";
      QMessageBox::information(p_panRateBox, "Invalid Value",
                               text, QMessageBox::Ok);
      return -1;
    }
    else {
      int percent = p_lineEdit->text().toInt();
      if(horz) {
        return(int)(d->viewport()->width() * (percent * 0.01));
      }
      else {
        return(int)(d->viewport()->height() * (percent * 0.01));
      }
    }
  }

  void PanTool::pan(int x, int y) {
    MdiCubeViewport *d = cubeViewport();
    if(d == NULL) return;
    if(x == -1 || y == -1) return;
    d->scrollBy(x, y);

    if(d->isLinked()) {
      for(int i = 0; i < (int)cubeViewportList()->size(); i++) {
        d = (*(cubeViewportList()))[i];
        if(d == cubeViewport()) continue;
        if(d->isLinked()) d->scrollBy(x, y);
      }
    }
  }

  void PanTool::mouseButtonPress(QPoint p, Qt::MouseButton s) {
    if(s == Qt::LeftButton) {
      p_dragPan = true;
      p_lastPoint = p;
    }
  }

  void PanTool::mouseMove(QPoint p) {
    if(p_dragPan) {
      QPoint diff = p_lastPoint - p;
      pan(diff.x(), diff.y());
      p_lastPoint = p;
    }
  }

  void PanTool::mouseButtonRelease(QPoint p, Qt::MouseButton s) {
    p_dragPan = false;
    if(s != Qt::RightButton) return;

    MdiCubeViewport *d = cubeViewport();
    if(d == NULL) return;
    double cx, cy;
    d->viewportToCube(p.x(), p.y(), cx, cy);
    d->center(cx, cy);

    if(d->isLinked()) {
      for(int i = 0; i < (int)cubeViewportList()->size(); i++) {
        d = (*(cubeViewportList()))[i];
        if(d == cubeViewport()) continue;
        if(d->isLinked()) {
          d->viewportToCube(p.x(), p.y(), cx, cy);
          d->center(cx, cy);
        }
      }
    }
  }


  void PanTool::setCustom() {
    p_panRateBox->setCurrentIndex(4);
  }


  void PanTool::updateLineEdit() {
    if(p_panRateBox->currentIndex() == 0) p_lineEdit->setText("25");
    else if(p_panRateBox->currentIndex() == 1) p_lineEdit->setText("50");
    else if(p_panRateBox->currentIndex() == 2) p_lineEdit->setText("75");
    else if(p_panRateBox->currentIndex() == 3) p_lineEdit->setText("100");
    else if(p_panRateBox->currentIndex() == 4) p_lineEdit->setText("");
  }

  void PanTool::writeSettings() {
    FileName config("$HOME/.Isis/qview/Pan Tool.config");
    QSettings settings(QString::fromStdString(config.expanded()),
                       QSettings::NativeFormat);
    settings.setValue("rate", p_lineEdit->text());
  }

  void PanTool::readSettings() {
    FileName config("$HOME/.Isis/qview/Pan Tool.config");
    QSettings settings(QString::fromStdString(config.expanded()),
                       QSettings::NativeFormat);
    QString rate = settings.value("rate", "75").toString();

    p_lineEdit->setText(rate);
    if(rate == "100") p_panRateBox->setCurrentIndex(3);
    else if(rate == "75") p_panRateBox->setCurrentIndex(2);
    else if(rate == "50") p_panRateBox->setCurrentIndex(1);
    else if(rate == "25") p_panRateBox->setCurrentIndex(0);
    else p_panRateBox->setCurrentIndex(4);
  }

}
