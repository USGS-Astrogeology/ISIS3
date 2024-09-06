/** This is free and unencumbered software released into the public domain.

The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */

#include "CnetEditorView.h"

#include <QAction>
#include <QGridLayout>
#include <QList>
#include <QMap>
#include <QMapIterator>
#include <QMenu>
#include <QMenuBar>
#include <QSize>
#include <QSizePolicy>
#include <QString>
#include <QTabWidget>
#include <QToolBar>
#include <QtXml>

#include "Control.h"
#include "ControlNet.h"
#include "CnetEditorWidget.h"
#include "Directory.h"
#include "FileName.h"
#include "Project.h"
#include "ProjectItemViewMenu.h"

namespace Isis {
  /**
   * Constructor.
   */
  CnetEditorView::CnetEditorView(Directory *directory, Control *control, FileName configFile,
                                 QWidget *parent) : AbstractProjectItemView(parent) {

    // TODO: This layout should be inside of the cnet editor widget, but I put it here to not
    //     conflict with current work in the cnet editor widget code.
    //QWidget *result = new QWidget;

    QWidget *centralWidget = new QWidget;
    setCentralWidget(centralWidget);
    QGridLayout *resultLayout = new QGridLayout;
    centralWidget->setLayout(resultLayout);

    m_cnetEditorWidget = new CnetEditorWidget(control, QString::fromStdString(configFile.expanded()));
    m_control = control;

    resultLayout->addWidget(m_cnetEditorWidget, 0, 0, 1, 2);

    QTabWidget *treeViews = new QTabWidget;
    treeViews->addTab( m_cnetEditorWidget->pointTreeView(), tr("Point View") );
    treeViews->addTab( m_cnetEditorWidget->serialTreeView(), tr("Serial View") );
    treeViews->addTab( m_cnetEditorWidget->connectionTreeView(), tr("Connection View") );
    resultLayout->addWidget(treeViews, 1, 0, 1, 1);

    QTabWidget *filterViews = new QTabWidget;
    filterViews->addTab( m_cnetEditorWidget->pointFilterWidget(), tr("Filter Points and Measures") );
    filterViews->addTab( m_cnetEditorWidget->serialFilterWidget(), tr("Filter Images and Points") );
    filterViews->addTab( m_cnetEditorWidget->connectionFilterWidget(), tr("Filter Connections") );
    resultLayout->addWidget(filterViews, 1, 1, 1, 1);

    createMenus();
    createToolBars();

    // Store the actions for easy enable/disable.
    foreach (QAction *action, m_permToolBar->actions()) {
      addAction(action);
    }
    // On default, actions are disabled until the cursor enters the view.
    disableActions();
  }

  /**
   * Destructor
   */
  CnetEditorView::~CnetEditorView() {

    delete m_cnetEditorWidget;
    delete m_permToolBar;
    delete m_tablesMenu;

    m_tablesMenu = 0;
    m_permToolBar = 0;
  }

  /**
   * Uses the actions created by CnetEditorWidget, creates the tables menu,
   * and puts the actions into the tables menu.
   */
  void CnetEditorView::createMenus() {
    QMap< QAction *, QList< QString > > actionMap = m_cnetEditorWidget->menuActions();
    QMapIterator< QAction *, QList< QString > > actionMapIter(actionMap);

    m_tablesMenu = new ProjectItemViewMenu("&Tables");
    connect(m_tablesMenu, SIGNAL(menuClosed()), this, SLOT(disableActions()));
    menuBar()->addMenu(m_tablesMenu);

    while ( actionMapIter.hasNext() ) {
      actionMapIter.next();
      QAction *actionToAdd = actionMapIter.key();

      // Skip the "What's This?" action because it is in the main help menu of IPCE
      if (actionToAdd->text() == "What's This?") {
        continue;
      }
      m_tablesMenu->addAction(actionToAdd);
    }
  }


  /**
   * Uses and adds the actions created by CnetEditorWidget to the view's toolbars
   * Right now, all actions created in CnetEditorWidget are added to the toolpad.
   * This was copied from CnetEditorWindow
   */
  void CnetEditorView::createToolBars() {
    m_permToolBar = addToolBar("Standard Tools");
    m_permToolBar->setObjectName("permToolBar");
    m_permToolBar->setIconSize(QSize(22, 22));

    QMap< QString, QList< QAction * > > actionMap;
    actionMap = m_cnetEditorWidget->toolBarActions();
    QMapIterator< QString, QList< QAction * > > actionIter(actionMap);

    while (actionIter.hasNext()) {
      actionIter.next();
      QString objName = actionIter.key();
      QList< QAction * > actionList = actionIter.value();
      foreach (QAction *action, actionList) {
        m_permToolBar->addAction(action);
      }
    }
  }


  /**
   * Disables actions when cursor leaves the view. Overriden method
   * If a menu is visible, i.e. clicked on, this causes a leave event. We want the
   * actions to still be enabled when a menu is visible.
   *
   * @param event The leave event
   */
  void CnetEditorView::leaveEvent(QEvent *event) {
    if (!m_tablesMenu->isVisible()) {
      disableActions();
    }
  }


  /**
   * Returns the cnetEditorWidget.
   *
   * @return (cnetEditorWidget *) The cnetEditorWidget used to
   *         display the footprints.
   */
  CnetEditorWidget *CnetEditorView::cnetEditorWidget() {
    return m_cnetEditorWidget;
  }


  /**
   * @description Returns the Control displayed in the CnetEditorWidget
   *
   * @return (Control *) The Control displayed in the CnetEditorWidget
   */
  Control *CnetEditorView::control() {
    return m_control;
  }


  /**
   * This method saves the Controls object ids to the stream.
   *
   * @param stream The stream that will output to directory.xml
   * @param project The project to save the users settings to
   * @param newProjectRoot New project's root directory
   */
  void CnetEditorView::save(QXmlStreamWriter &stream, Project *, FileName) const {

    stream.writeStartElement("cnetEditorView");
    stream.writeAttribute("objectName", objectName());
    stream.writeAttribute("id", m_control->id());
    stream.writeEndElement();
  }
}
