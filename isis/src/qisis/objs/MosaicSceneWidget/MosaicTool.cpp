#include "MosaicTool.h"

#include <iostream>

#include <QDebug>
#include <QLabel>

#include "FileName.h"
#include "IException.h"
#include "IString.h"
#include "MosaicSceneWidget.h"
#include "PvlObject.h"
#include "ToolPad.h"

namespace Isis {
  MosaicTool::MosaicTool(MosaicSceneWidget *scene) : QObject(scene) {
    p_active = false;
    p_primaryAction = NULL;
    p_toolBarAction = NULL;
    p_widget = scene;

    connect(this, SIGNAL(activated(bool)), this, SLOT(updateTool()));

    if (scene) {
      connect(scene, SIGNAL(mouseEnter()), this, SLOT(mouseEnter()));
      connect(scene, SIGNAL(mouseLeave()), this, SLOT(mouseLeave()));
      connect(scene, SIGNAL(mouseMove(QPointF)), this, SLOT(mouseMove(QPointF)));

      connect(scene, SIGNAL(mouseDoubleClick(QPointF)),
              this, SLOT(mouseDoubleClick(QPointF)));
      connect(scene, SIGNAL(mouseButtonPress(QPointF, Qt::MouseButton)),
              this, SLOT(mouseButtonPress(QPointF, Qt::MouseButton)));
      connect(scene, SIGNAL(mouseButtonRelease(QPointF, Qt::MouseButton)),
              this, SLOT(mouseButtonRelease(QPointF, Qt::MouseButton)));
      connect(scene, SIGNAL(mouseWheel(QPointF, int)),
              this, SLOT(mouseWheel(QPointF, int)));
      connect(scene, SIGNAL(rubberBandComplete(QRectF, Qt::MouseButton)),
              this, SLOT(rubberBandComplete(QRectF, Qt::MouseButton)));
    }
  }


  MosaicTool::~MosaicTool() {
    // This will call toolBarDestroyed which will NULL the pointer
    if(p_toolBarAction)
      delete p_toolBarAction;

    // The actions not derived from widgets will auto-destroy
  }


  void MosaicTool::addTo(ToolPad *toolPad) {
    if(!p_primaryAction) {
      p_primaryAction = getPrimaryAction();

      if(p_primaryAction) {
        p_primaryAction->setCheckable(true);
        p_primaryAction->setChecked(p_active);
      }
    }

    if(p_primaryAction) {
      connect(p_primaryAction, SIGNAL(toggled(bool)),
              this, SLOT(activate(bool)));
      toolPad->addAction(p_primaryAction);
    }
  }


  void MosaicTool::addTo(QToolBar *toolBar) {
    QWidget *toolBarWidget = NULL;
    if(!p_toolBarAction)
      toolBarWidget = getToolBarWidget();

    if(toolBarWidget) {
      p_toolBarAction = toolBar->addWidget(toolBarWidget);
      connect(p_toolBarAction, SIGNAL(destroyed(QObject *)),
              this, SLOT(toolBarDestroyed(QObject *)));
      disableToolBar();
    }
  }


  QList<QAction *> MosaicTool::getViewActions() {
    return QList<QAction *>();
  }


  PvlObject MosaicTool::toPvl() const {
    if(projectPvlObjectName() == "") {
      PvlObject obj("Invalid");
      return obj;
    }

    throw IException(IException::Programmer,
                     "Please re-implement toPvl in your tool",
                     _FILEINFO_);
  }


  void MosaicTool::fromPvl(const PvlObject &obj) {
    if(projectPvlObjectName() != "") {
      throw IException(IException::Programmer,
                       "Please re-implement fromPvl in your tool",
                       _FILEINFO_);
    }
  }


  QString MosaicTool::projectPvlObjectName() const {
    return "";
  }


  QPixmap MosaicTool::getIcon(QString iconName) const {
    QString path = QString::fromStdString(FileName("$ISISROOT/appdata/images/icons").expanded());
    QString fullPathToFile = QString(path + "/" + iconName);
    return QPixmap(fullPathToFile);
  }


  void MosaicTool::mouseMove(QPointF) {
  }


  void MosaicTool::mouseDoubleClick(QPointF) {
  }


  void MosaicTool::mouseButtonPress(QPointF, Qt::MouseButton s) {
  }


  void MosaicTool::mouseButtonRelease(QPointF, Qt::MouseButton s) {
    //qDebug()<<"MosaicTool::mouseButtonRelease";
  }


  void MosaicTool::mouseWheel(QPointF, int) {
  }


  /**
   * Activates the tool.
   *
   * @param on
   */
  void MosaicTool::activate(bool on) {
    bool activeStateChanged = (p_active != on);

    if(p_active && !on) {
      disableToolBar();
    }
    else if(!p_active && on) {
      enableToolBar();
    }

    p_active = on;

    if(activeStateChanged)
      emit activated(p_active);
  }


  void MosaicTool::toolBarDestroyed(QObject *) {
    p_toolBarAction = NULL;
  }


  /**
   * Disables entire tool bar.
   *
   */
  void MosaicTool::disableToolBar() {
    if(p_toolBarAction) {
      p_toolBarAction->setVisible(false);
    }
  }


  /**
   * Enables entire tool bar.
   *
   */
  void MosaicTool::enableToolBar() {
    if(p_toolBarAction) {
      p_toolBarAction->setVisible(true);
    }
  }


  QWidget *MosaicTool::getToolBarWidget() {
    return new QWidget();
  }
}
