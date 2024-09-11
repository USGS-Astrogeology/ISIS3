#include "Tool.h"

#include <QLayout>
#include <QMenuBar>
#include <QStackedWidget>
#include <QVector>

#include "CubeViewport.h"
#include "MdiCubeViewport.h"
#include "FileName.h"
#include "IString.h"
#include "RubberBandTool.h"
#include "ToolPad.h"
#include "ToolList.h"
#include "Workspace.h"
#include "ViewportMainWindow.h"


namespace Isis {
//   QStackedWidget *Tool::m_activeToolBarStack = NULL;

  /**
   * Tool constructor
   *
   * @param parent
   */
  Tool::Tool(QWidget *parent) : QObject(parent) {
    m_cvp = NULL;
    m_workspace = NULL;
    m_active = false;
    m_toolPadAction = NULL;
    m_toolBarWidget = NULL;
    m_toolList = NULL;

    QString tempFileName = QString::fromStdString(FileName("$ISISROOT/appdata/images/icons").expanded());
    m_toolIconDir = tempFileName;
  }


  /**
   * Adds the given workspace to the cubeviewport list.
   *
   * @param ws
   */
  void Tool::addTo(Workspace *ws) {
    m_workspace = ws;

    connect(ws, SIGNAL(cubeViewportAdded(MdiCubeViewport *)),
            this, SLOT(setCubeViewport(MdiCubeViewport *)));
    connect(ws, SIGNAL(cubeViewportActivated(MdiCubeViewport *)),
            this, SLOT(setCubeViewport(MdiCubeViewport *)));
    connect(ws, SIGNAL(cubeViewportAdded(MdiCubeViewport *)),
            this, SLOT(registerTool(MdiCubeViewport *)));
  }


  RubberBandTool *Tool::rubberBandTool() {
    RubberBandTool *result = NULL;

    if (m_toolList) {
      result = m_toolList->rubberBandTool();
    }

    return result;
  }


  void Tool::setList(ToolList *currentList) {
    m_toolList = currentList;
  }


  /**
   * Adds the tool to the application
   *
   * @param pViewPortMnWin
   */
  void Tool::addTo(ViewportMainWindow *pViewPortMnWin) {
    addTo(pViewPortMnWin->workspace());
    addToPermanent(pViewPortMnWin->permanentToolBar());
    addToActive(pViewPortMnWin->activeToolBar());
    addTo(pViewPortMnWin->toolPad());
    if(!menuName().isEmpty()) {
      QMenu *menu = pViewPortMnWin->getMenu(menuName());
//      if (menu->actions().size() > 0) menu->addSeparator();
      addTo(menu);
    }

    //connect(this, SIGNAL(clearWarningSignal ()), pViewPortMnWin, SLOT(resetWarning ()));
  }


  /**
   * Adds the tool to the toolpad
   *
   * @param toolpad
   */
  void Tool::addTo(ToolPad *toolpad) {
    m_toolPadAction = toolPadAction(toolpad);
    if(m_toolPadAction != NULL) {
      toolpad->addAction(m_toolPadAction);
      connect(m_toolPadAction, SIGNAL(toggled(bool)), this, SLOT(activate(bool)));
    }
  }


  /**
   *
   *
   * @param toolbar
   */
  void Tool::addToActive(QToolBar *toolbar) {
    if (m_toolList) {
      QStackedWidget *activeToolBarStack = m_toolList->toolBarStack(toolbar);

      m_toolBarWidget = createToolBarWidget(activeToolBarStack);
      if(m_toolBarWidget != NULL) {
        activeToolBarStack->addWidget(m_toolBarWidget);
      }

      disableToolBar();
    }
  }


  /**
   * Activates the tool.
   *
   * @param on
   */
  void Tool::activate(bool on) {
    if(m_active) {
      emit clearWarningSignal();
      if(on)
        return;
      removeViewportConnections();
      disableToolBar();
      if(m_toolPadAction != NULL)
        m_toolPadAction->setChecked(false);
      m_active = false;
    }
    else {
      if(!on)
        return;
      if(m_toolPadAction != NULL)
        m_toolPadAction->setChecked(true);
      addViewportConnections();
      enableToolBar();
      emit toolActivated();
      m_active = true;
    }
  }


  /**
   * Sets the current viewport to the given cvp
   *
   * @param cvp
   */
  void Tool::setCubeViewport(MdiCubeViewport *cvp) {
    if(cvp == m_cvp) {
      updateTool();
      return;
    }

    if(m_active)
      removeViewportConnections();

    m_cvp = cvp;

    if(m_active) {
      addViewportConnections();
      enableToolBar();
    }
    else {
      updateTool();
    }

    emit viewportChanged();
  }


  /**
   * Makes all the connections for the tool.
   *
   */
  void Tool::addViewportConnections() {
    if(m_cvp == NULL)
      return;

    connect(m_cvp, SIGNAL(scaleChanged()),
            this, SLOT(scaleChanged()));

    if (rubberBandTool()) {
      connect(rubberBandTool(), SIGNAL(measureChange()),
              this, SLOT(updateMeasure()));

      connect(rubberBandTool(), SIGNAL(bandingComplete()),
              this, SLOT(rubberBandComplete()));
    }

    connect(m_cvp, SIGNAL(mouseEnter()),
            this, SLOT(mouseEnter()));

    connect(m_cvp, SIGNAL(screenPixelsChanged()),
            this, SLOT(screenPixelsChanged()));

    connect(m_cvp, SIGNAL(mouseMove(QPoint)),
            this, SLOT(mouseMove(QPoint)), Qt::DirectConnection);

    connect(m_cvp, SIGNAL(mouseMove(QPoint, Qt::MouseButton)),
            this, SLOT(mouseMove(QPoint, Qt::MouseButton)), Qt::DirectConnection);

    connect(m_cvp, SIGNAL(mouseLeave()),
            this, SLOT(mouseLeave()));

    connect(m_cvp, SIGNAL(mouseDoubleClick(QPoint)),
            this, SLOT(mouseDoubleClick(QPoint)));

    connect(m_cvp, SIGNAL(mouseButtonPress(QPoint, Qt::MouseButton)),
            this, SLOT(mouseButtonPress(QPoint, Qt::MouseButton)));

    connect(m_cvp, SIGNAL(mouseButtonRelease(QPoint, Qt::MouseButton)),
            this, SLOT(mouseButtonRelease(QPoint, Qt::MouseButton)));

    addConnections(m_cvp);

    if(m_toolPadAction != NULL) {
      enableRubberBandTool();
    }
  }


  /**
   * Removes all the connections from the tool.
   *
   */
  void Tool::removeViewportConnections() {
    if(m_cvp == NULL)
      return;

    disconnect(m_cvp, SIGNAL(scaleChanged()),
               this, SLOT(scaleChanged()));

    if (rubberBandTool()) {
      disconnect(rubberBandTool(), SIGNAL(measureChange()),
                 this, SLOT(updateMeasure()));

      disconnect(rubberBandTool(), SIGNAL(bandingComplete()),
                 this, SLOT(rubberBandComplete()));
    }

    disconnect(m_cvp, SIGNAL(mouseEnter()),
               this, SLOT(mouseEnter()));

    disconnect(m_cvp, SIGNAL(screenPixelsChanged()),
               this, SLOT(screenPixelsChanged()));

    disconnect(m_cvp, SIGNAL(mouseMove(QPoint)),
               this, SLOT(mouseMove(QPoint)));

    disconnect(m_cvp, SIGNAL(mouseMove(QPoint, Qt::MouseButton)),
               this, SLOT(mouseMove(QPoint, Qt::MouseButton)));

    disconnect(m_cvp, SIGNAL(mouseLeave()),
               this, SLOT(mouseLeave()));

    disconnect(m_cvp, SIGNAL(mouseDoubleClick(QPoint)),
               this, SLOT(mouseDoubleClick(QPoint)));

    disconnect(m_cvp, SIGNAL(mouseButtonPress(QPoint, Qt::MouseButton)),
               this, SLOT(mouseButtonPress(QPoint, Qt::MouseButton)));

    disconnect(m_cvp, SIGNAL(mouseButtonRelease(QPoint, Qt::MouseButton)),
               this, SLOT(mouseButtonRelease(QPoint, Qt::MouseButton)));

    removeConnections(m_cvp);
  }


  /**
   * Disables entire tool bar.
   *
   */
  void Tool::disableToolBar() {
    if(m_toolBarWidget == NULL)
      return;
//    if (m_toolBarWidget->isVisible()) m_toolBarWidget->hide();
    m_toolBarWidget->setEnabled(false);
  }


  /**
   * Enables entire tool bar.
   *
   */
  void Tool::enableToolBar() {
    updateTool();
    if(m_toolBarWidget == NULL)
      return;
    if(cubeViewport() == NULL) {
      m_toolBarWidget->setEnabled(false);
    }
    else {
      m_toolBarWidget->setEnabled(true);
    }

    if (m_toolList && m_toolList->toolBarStack()) {
      m_toolList->toolBarStack()->setCurrentWidget(m_toolBarWidget);
    }
  }


  /**
   * Updates the tool.
   *
   */
  void Tool::updateTool() {
  }


  /**
   * Registers the tool to the viewport.
   *
   * @param viewport
   */
  void Tool::registerTool(MdiCubeViewport *viewport) {
    viewport->registerTool(this);

    connect(m_cvp, SIGNAL(requestRestretch(MdiCubeViewport *, int)),
            this, SLOT(stretchRequested(MdiCubeViewport *, int)));
  }


  /**
   * Enable the use of the rubberband tool.
   *
   */
  void Tool::enableRubberBandTool() {
    rubberBandTool()->disable();
  }


  Workspace *Tool::workspace() {
    return m_workspace;
  }


  /**
   * @param p
   */
  void Tool::mouseMove(QPoint p) {};


  /**
   * @param p
   */
  void Tool::mouseDoubleClick(QPoint p) {
    emit clearWarningSignal();
  }


  /**
   * @param p
   * @param s
   */
  void Tool::mouseButtonPress(QPoint p, Qt::MouseButton s) {
    emit clearWarningSignal();
  }


  /**
   * Resets the Warning to Nowarning when a different activity
   * occurs on the application. This is called by all the
   * mouseButtonRelease events in all the tools.
   *
   * @param p
   * @param s
   */
  void Tool::mouseButtonRelease(QPoint p, Qt::MouseButton s) {
    emit clearWarningSignal();
  }


  /**
   * Return the list of cubeviewports.
   *
   * @return CubeViewportList*
   */
  Tool::CubeViewportList *Tool::cubeViewportList() const {
    return m_workspace->cubeViewportList();
  }
}



