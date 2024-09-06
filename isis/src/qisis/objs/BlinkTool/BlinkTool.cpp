/** This is free and unencumbered software released into the public domain.

The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */

#include "BlinkTool.h"

#include "FileName.h"
#include "IString.h"
#include "MainWindow.h"
#include "MdiCubeViewport.h"

#include <QDialog>
#include <QDoubleSpinBox>
#include <QEvent>
#include <QHBoxLayout>
#include <QListWidgetItem>
#include <QMenu>
#include <QPainter>
#include <QPushButton>
#include <QSettings>
#include <QSplitter>
#include <QTimer>
#include <QToolBar>
#include <QToolButton>
#include <QVBoxLayout>

namespace Isis {

  /**
   * Blink Tool Constructor
   *
   *
   * @param parent
   */
  BlinkTool::BlinkTool(QWidget *parent) : Tool(parent) {
    // Create the blink window
    p_dialog = new QDialog(parent);
    p_dialog->setWindowTitle("Blink Comparator");
    p_dialog->setSizeGripEnabled(true);
    p_blinkWindow = new QWidget(p_dialog);
    p_blinkWindow->setMinimumSize(492, 492);
    p_blinkWindow->setBackgroundRole(QPalette::Shadow);
    //p_blinkWindow->installEventFilter(this);

    p_listWidget = new QListWidget(p_dialog);
    p_listWidget->setMinimumHeight(100);
    connect(p_listWidget, SIGNAL(itemActivated(QListWidgetItem *)),
            this, SLOT(toggleLink(QListWidgetItem *)));
    connect(p_listWidget, SIGNAL(currentRowChanged(int)),
            this, SLOT(updateWindow()));

    p_splitter = new QSplitter(Qt::Vertical, p_dialog);
    p_splitter->addWidget(p_blinkWindow);
    p_splitter->addWidget(p_listWidget);
    p_splitter->setCollapsible(0, false);
    p_splitter->setCollapsible(1, false);
    p_splitter->setStretchFactor(0, 0);
    p_splitter->setStretchFactor(1, 1);

    QWidget *buttons = new QWidget(p_dialog);

    QVBoxLayout *layout = new QVBoxLayout();
//    layout->addWidget(p_blinkWindow,2);
//    layout->addWidget(p_listWidget,1);
    layout->addWidget(p_splitter, 1);
    layout->addWidget(buttons, 0);
    p_dialog->setLayout(layout);

    QToolButton *reverse = new QToolButton(buttons);
    reverse->setIcon(QPixmap(toolIconDir() + "/blinkReverse.png"));
    reverse->setIconSize(QSize(22, 22));
    reverse->setShortcut(Qt::CTRL + Qt::Key_Delete);
    reverse->setToolTip("Previous");
    QString text = "<b>Function:</b> Show previous linked viewport and \
      stop automatic timed blinking \
      <p><b>Shortcut:</b> Ctrl+Delete</p>";
    reverse->setWhatsThis(text);
    connect(reverse, SIGNAL(released()), this, SLOT(reverse()));

    QToolButton *stop = new QToolButton(buttons);
    stop->setIcon(QPixmap(toolIconDir() + "/blinkStop.png"));
    stop->setIconSize(QSize(22, 22));
    stop->setToolTip("Stop");
    text = "<b>Function:</b> Stop automatic timed blinking";
    stop->setWhatsThis(text);
    connect(stop, SIGNAL(released()), this, SLOT(stop()));

    QToolButton *start = new QToolButton(buttons);
    start->setIcon(QPixmap(toolIconDir() + "/blinkStart.png"));
    start->setIconSize(QSize(22, 22));
    start->setToolTip("Start");
    text = "<b>Function:</b> Start automatic timed blinking.  Cycles \
           through linked viewports at variable rate";
    start->setWhatsThis(text);
    connect(start, SIGNAL(released()), this, SLOT(start()));

    QToolButton *forward = new QToolButton(buttons);
    forward->setIcon(QPixmap(toolIconDir() + "/blinkAdvance.png"));
    forward->setIconSize(QSize(22, 22));
    forward->setToolTip("Next");
    forward->setShortcut(Qt::Key_Delete);
    text = "<b>Function:</b> Show next linked viewport and stop \
      automatic timed blinking \
      <p><b>Shortcut:</b> Delete</p>";
    forward->setWhatsThis(text);
    connect(forward, SIGNAL(released()), this, SLOT(advance()));

    p_timeBox = new QDoubleSpinBox(buttons);
    p_timeBox->setMinimum(0.1);
    p_timeBox->setMaximum(5.0);
    p_timeBox->setDecimals(1);
    p_timeBox->setSingleStep(0.1);
    p_timeBox->setValue(0.5);
    p_timeBox->setToolTip("iTime Delay");
    text = "<b>Function:</b> Change automatic blink rate between " +
           QString::number(p_timeBox->minimum()) + " and " +
           QString::number(p_timeBox->maximum()) + " seconds";
    p_timeBox->setWhatsThis(text);

    QPushButton *close = new QPushButton("Close", buttons);
    close->setDefault(false);
    close->setAutoDefault(false);
    connect(close, SIGNAL(released()), p_dialog, SLOT(hide()));
    connect(close, SIGNAL(released()), this, SLOT(stop()));

    QHBoxLayout *hlayout = new QHBoxLayout();
    hlayout->addWidget(reverse);
    hlayout->addWidget(stop);
    hlayout->addWidget(start);
    hlayout->addWidget(forward);
    hlayout->addWidget(p_timeBox);
    hlayout->addStretch(1);
    hlayout->addWidget(close);
    buttons->setLayout(hlayout);

    // Create the action to bring up the blink window
    p_action = new QAction(parent);
    p_action->setShortcut(Qt::Key_K);
    p_action->setText("&Blink ...");
    p_action->setIcon(QPixmap(toolIconDir() + "/blink.png"));
    p_action->setToolTip("Blink");
    text =
      "<b>Function:</b> Opens a blink comparator for linked viewports. \
       <p><b>Shortcut:</b>K</p>";
    p_action->setWhatsThis(text);
    p_action->setEnabled(false);
    connect(p_action, SIGNAL(triggered()), p_dialog, SLOT(show()));

    p_timerOn = false;
    readSettings();
    p_blinkWindow->installEventFilter(this);
  }


  /**
   * Adds this action to the given menu.
   *
   *
   * @param menu
   */
  void BlinkTool::addTo(QMenu *menu) {
    menu->addAction(p_action);
  }


  /**
   * Adds this action to the permanent tool bar.
   *
   *
   * @param perm
   */
  void BlinkTool::addToPermanent(QToolBar *perm) {
    perm->addAction(p_action);
  }


  /**
   * Updates the blink tool.
   *
   */
  void BlinkTool::updateTool() {
    QString unlinkedIcon = QString::fromStdString(FileName("$ISISROOT/appdata/images/icons/unlinked.png").expanded());
    static QIcon unlinked(unlinkedIcon);
    QString linkedIcon = QString::fromStdString(FileName("$ISISROOT/appdata/images/icons/linked.png").expanded());
    static QIcon linked(linkedIcon);
    p_listWidget->clear();
    for(int i = 0; i < (int)cubeViewportList()->size(); i++) {
      MdiCubeViewport *d = (*(cubeViewportList()))[i];
      disconnect(d, SIGNAL(linkChanging(bool)), this, 0);
      disconnect(d, SIGNAL(windowTitleChanged()), this, 0);
      QListWidgetItem *item = new QListWidgetItem(p_listWidget);
      item->setText(d->parentWidget()->windowTitle());
      if(d->isLinked()) {
        item->setIcon(linked);
      }
      else {
        item->setIcon(unlinked);
      }
      connect(d, SIGNAL(linkChanging(bool)), this, SLOT(updateTool()));
      connect(d, SIGNAL(windowTitleChanged()), this, SLOT(updateTool()));
    }

    if(cubeViewport() == NULL) {
      p_action->setEnabled(false);
    }
    else if(cubeViewportList()->size() <= 1) {
      p_action->setEnabled(false);
    }
    else {
      p_action->setEnabled(true);
    }
  }


  /**
   * Links/unlinks viewports
   *
   *
   * @param item
   */
  void BlinkTool::toggleLink(QListWidgetItem *item) {
    for(int i = 0; i < p_listWidget->count(); i++) {
      if(p_listWidget->item(i) == item) {
        MdiCubeViewport *d = (*(cubeViewportList()))[i];
        d->setLinked(!d->isLinked());
        return;
      }
    }
  }


  /**
   * Reverses the order of the blinking.
   *
   */
  void BlinkTool::reverse() {
    p_timerOn = false;

    int row = p_listWidget->currentRow() - 1;
    for(int i = 0; i < (int)cubeViewportList()->size(); i++) {
      if(row < 0) row = (int)cubeViewportList()->size() - 1;
      MdiCubeViewport *d = (*(cubeViewportList()))[row];
      if(d->isLinked()) {
        p_listWidget->setCurrentRow(row);
        p_listWidget->scrollToItem(p_listWidget->currentItem());
        break;
      }
      row--;
    }
  }


  /**
   * Stops the blinking.
   *
   */
  void BlinkTool::stop() {
    p_timerOn = false;
  }


  /**
   * Starts the blinking.
   *
   */
  void BlinkTool::start() {
    if(p_timerOn) return;
    p_timerOn = true;
    int msec = (int)(p_timeBox->value() * 1000.0);
    QTimer::singleShot(msec, this, SLOT(timeout()));
  }

  /**
   * Manual blinking.
   *
   */
  void BlinkTool::advance() {
    p_timerOn = false;

    int row = p_listWidget->currentRow() + 1;
    for(int i = 0; i < (int)cubeViewportList()->size(); i++) {
      if(row >= (int) cubeViewportList()->size()) row = 0;
      MdiCubeViewport *d = (*(cubeViewportList()))[row];
      if(d->isLinked()) {
        p_listWidget->setCurrentRow(row);
        p_listWidget->scrollToItem(p_listWidget->currentItem());
        break;
      }
      row++;
    }
  }


  /**
   * The blink tool's timer.
   *
   */
  void BlinkTool::timeout() {
    if(p_timerOn) {
      advance();
      start();
    }
  }


  /**
   * Repaints the blink tool window.
   *
   */
  void BlinkTool::updateWindow() {
    p_blinkWindow->repaint();
  }


  /**
   * Writes the current settings of this window so the next time
   * this tool is used, certain user prefs. are remembered.
   *
   */
  void BlinkTool::writeSettings() {
    QSettings settings(MainWindow::settingsFileName("BlinkTool"), QSettings::NativeFormat);
    settings.setValue("rate", p_timeBox->value());
    settings.setValue("size", p_dialog->size());
    settings.setValue("pos", p_dialog->pos());
    settings.setValue("geom", p_dialog->saveGeometry());
    settings.setValue("splitterState", p_splitter->saveState());
  }


  /**
   * Reads the settings saved from the last time this tool was
   * used.
   *
   */
  void BlinkTool::readSettings() {
    QSettings settings(MainWindow::settingsFileName("BlinkTool"), QSettings::NativeFormat);
    double rate = settings.value("rate", 0.5).toDouble();

    p_dialog->restoreGeometry(settings.value("geom").toByteArray());
    p_splitter->restoreState(settings.value("splitterState").toByteArray());

    if (!settings.value("pos").toPoint().isNull())
      p_dialog->move(settings.value("pos").toPoint());

    p_dialog->resize(settings.value("size", QSize(492, 492)).toSize());

    p_timeBox->setValue(rate);
  }


  /**
   * Catches the events happening so we can make this tool do what
   * we want.
   *
   *
   * @param o
   * @param e
   *
   * @return bool
   */
  bool BlinkTool::eventFilter(QObject *o, QEvent *e) {
    if(o != p_blinkWindow) return false;
    if(e->type() == QEvent::Hide) writeSettings();
    if(e->type() != QEvent::Paint) return false;

    static QPixmap lastPixmap;
    int row = p_listWidget->currentRow();
    if(row != -1) {
      MdiCubeViewport *d = (*(cubeViewportList()))[row];
      lastPixmap = d->pixmap();
    }

    int dx = lastPixmap.width() - p_blinkWindow->width();
    int x = 0;
    int sx = dx / 2;
    if(dx < 0) {
      x = -dx / 2;
      sx = 0;
    }
    int dy = lastPixmap.height() - p_blinkWindow->height();
    int y = 0;
    int sy = dy / 2;
    if(dy < 0) {
      y = -dy / 2;
      sy = 0;
    }

    QPainter p(p_blinkWindow);
    p.drawPixmap(x, y, lastPixmap, sx, sy, -1, -1);

    return true;
  }
}
