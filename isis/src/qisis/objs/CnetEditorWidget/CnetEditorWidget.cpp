/** This is free and unencumbered software released into the public domain.

The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */

#include "CnetEditorWidget.h"

#include <QAction>
#include <QActionGroup>
#include <QApplication>
#include <QBoxLayout>
#include <QByteArray>
#include <QCloseEvent>
#include <QDebug>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QItemSelection>
#include <QMenu>
#include <QMessageBox>
#include <QModelIndex>
#include <QScrollArea>
#include <QScrollBar>
#include <QSettings>
#include <QSplitter>
#include <QString>
#include <QStringList>
#include <QTime>
#include <QTimer>
#include <QToolBar>
#include <QVBoxLayout>
#include <QWhatsThis>
#include <QtXml>

#include "AbstractMeasureItem.h"
#include "AbstractPointItem.h"
#include "AbstractTreeItem.h"
#include "CnetDisplayProperties.h"
#include "CnetEditorSortConfigDialog.h"
#include "Control.h"
#include "ControlMeasure.h"
#include "ControlNet.h"
#include "ControlPoint.h"
#include "FileName.h"
#include "FilterWidget.h"
#include "IException.h"
#include "ImageImageTreeModel.h"
#include "ImagePointTreeModel.h"
#include "MeasureTableModel.h"
#include "PointMeasureTreeModel.h"
#include "PointTableModel.h"
#include "Project.h"
#include "TableView.h"
#include "TableViewHeader.h"
#include "TreeView.h"


namespace Isis {

//**************************************************************
//**************************************************************
//**************************************************************

  const QString CnetEditorWidget::VERSION = "0.1";

//**************************************************************
//**************************************************************
//**************************************************************


  CnetEditorWidget::CnetEditorWidget(Control *control, QString pathForSettings) {
    nullify();

    m_workingVersion = new QString;
    m_menuActions = new QMap< QAction *, QList< QString > >;
    m_toolBarActions = new QMap< QString, QList< QAction * > >;

    m_updatingSelection = false;

    m_sortDialog = NULL;

    m_control = control;
    connect(CnetDisplayProperties::getInstance(), SIGNAL(compositionFinished()),
        this, SLOT(rebuildModels()));

    connect(this, SIGNAL(cnetModified()), this, SLOT(setCnetModified()));
    m_settingsPath = new QString(pathForSettings);

    QBoxLayout *mainLayout = createMainLayout();
    setLayout(mainLayout);

    createActions();

    readSettings();

    upgradeVersion();

    installEventFilter(this);
  }


  /**
   * Destructor
   */
  CnetEditorWidget::~CnetEditorWidget() {

    writeSettings();

    delete m_workingVersion;
    m_workingVersion = NULL;

    delete m_settingsPath;
    m_settingsPath = NULL;

    delete m_pointTreeView;
    m_pointTreeView = NULL;

    delete m_imageTreeView;
    m_imageTreeView = NULL;

    delete m_connectionTreeView;
    m_connectionTreeView = NULL;

    delete m_pointTableView;
    m_pointTableView = NULL;

    delete m_measureTableView;
    m_measureTableView = NULL;

    delete m_pointFilterWidget;
    m_pointFilterWidget = NULL;

    delete m_serialFilterWidget;
    m_serialFilterWidget = NULL;

    delete m_connectionFilterWidget;
    m_connectionFilterWidget = NULL;

    delete m_menuActions;
    m_menuActions = NULL;

    delete m_toolBarActions;
    m_toolBarActions = NULL;

    m_pointTableBox = NULL;
    m_measureTableBox = NULL;
    m_mainSplitter = NULL;

    // TODO: null all member widgets!

    delete m_pointModel;
    m_pointModel = NULL;

    delete m_imageModel;
    m_imageModel = NULL;

    delete m_connectionModel;
    m_connectionModel = NULL;

    delete m_sortDialog;
    m_sortDialog = NULL;
  }


  /**
   * Sets all member variables to NULL
   */
  void CnetEditorWidget::nullify() {
    m_pointTreeView = NULL;
    m_imageTreeView = NULL;
    m_connectionTreeView = NULL;

    m_pointModel = NULL;
    m_imageModel = NULL;
    m_connectionModel = NULL;

    m_pointTableModel = NULL;
    m_measureTableModel = NULL;

    m_pointTableBox = NULL;
    m_measureTableBox = NULL;

    m_pointTableView = NULL;
    m_measureTableView = NULL;

    m_mainSplitter = NULL;

    m_menuActions = NULL;
    m_toolBarActions = NULL;

    m_filterArea = NULL;

    m_pointFilterWidget = NULL;
    m_serialFilterWidget = NULL;
    m_connectionFilterWidget = NULL;

    m_control = NULL;
    m_settingsPath = NULL;
    m_workingVersion = NULL;

    m_sortDialog = NULL;
  }


  /**
   * Deletes the current models, clears the tree, and rebuilds the models
   *
   * @param itemsToDelete Tree to clear
   */
  void CnetEditorWidget::rebuildModels(QList<AbstractTreeItem *> itemsToDelete) {
    m_pointModel->stopWorking();
    m_imageModel->stopWorking();
    m_connectionModel->stopWorking();

    bool ignoreAll = false;
    foreach (AbstractTreeItem * item, itemsToDelete) {
      try {
        item->deleteSource();
      }
      catch (IException &e) {
        std::string message = e.what();

        if (!ignoreAll) {
          if (item == itemsToDelete.last()) {
            QMessageBox::warning(
              this, "Failed to delete row", message, QMessageBox::Ok);
          }
          else {
            message += "\n\nOkay to continue?";

            QMessageBox::StandardButton status = QMessageBox::warning(
                this, "Failed to delete row", message, QMessageBox::Yes |
                QMessageBox::YesToAll | QMessageBox::No);

            if (status == QMessageBox::YesToAll)
              ignoreAll = true;
            else if (status == QMessageBox::No)
              break;
          }
        }
      }
    }

    m_pointModel->rebuildItems();
    m_imageModel->rebuildItems();
    m_connectionModel->rebuildItems();
  }


  /**
   * Creates the layout of the widget
   */
  QBoxLayout *CnetEditorWidget::createMainLayout() {
    createPointTreeView();
    createSerialTreeView();
    createConnectionTreeView();

    connect(m_pointTreeView, SIGNAL(activated()),
        m_imageTreeView, SLOT(deactivate()));
    connect(m_pointTreeView, SIGNAL(activated()),
        m_connectionTreeView, SLOT(deactivate()));

    connect(m_imageTreeView, SIGNAL(activated()),
        m_pointTreeView, SLOT(deactivate()));
    connect(m_imageTreeView, SIGNAL(activated()),
        m_connectionTreeView, SLOT(deactivate()));

    connect(m_connectionTreeView, SIGNAL(activated()),
        m_pointTreeView, SLOT(deactivate()));
    connect(m_connectionTreeView, SIGNAL(activated()),
        m_imageTreeView, SLOT(deactivate()));

    createFilterArea();

    createPointTableView();
    m_pointTableBox = new QGroupBox(tr("Control Point Table"));
    QHBoxLayout *pointTableLayout = new QHBoxLayout;
    pointTableLayout->addWidget(m_pointTableView);
    m_pointTableBox->setLayout(pointTableLayout);

    createMeasureTableView();
    m_measureTableBox = new QGroupBox(tr("Control Measure Table"));
    QHBoxLayout *measureTableLayout = new QHBoxLayout;
    measureTableLayout->addWidget(m_measureTableView);
    m_measureTableBox->setLayout(measureTableLayout);

    m_mainSplitter = new QSplitter(Qt::Vertical);
    m_mainSplitter->addWidget(m_pointTableBox);
    m_mainSplitter->addWidget(m_measureTableBox);

    QBoxLayout *mainLayout = new QHBoxLayout;
    mainLayout->addWidget(m_mainSplitter);

    return mainLayout;
  }


  /**
   * Creates the menus, and options for the widget
   */
  void CnetEditorWidget::createActions() {

    QAction *freezeTablesAct = new QAction(QIcon(
        QString::fromStdString((FileName("$ISISROOT/appdata/images/icons/ice.png").expanded()))),
        tr("&Freeze Tables"), this);
    freezeTablesAct->setCheckable(true);
    freezeTablesAct->setToolTip(tr("Freeze tables (filters will not take "
        "effect until unfrozen)"));
    freezeTablesAct->setStatusTip(tr("Freeze tables (filters will not take "
        "effect until unfrozen)"));
    freezeTablesAct->setWhatsThis(tr("<html>When frozen, the contents of the "
        "tables will be locked.  Current filters will not be applied to the "
        "tables until they are unfrozen.</html>"));
    connect(freezeTablesAct, SIGNAL(toggled(bool)),
        this, SLOT(setTablesFrozen(bool)));
    QList< QString > freezeTablesLocation;
    freezeTablesLocation.append(tr("&Tables"));
    m_menuActions->insert(freezeTablesAct, freezeTablesLocation);

    QAction *configureSortAct = new QAction((QIcon(
        QString::fromStdString(FileName("$ISISROOT/appdata/images/icons/sort.png").expanded()))),
        tr("&Sorting Options..."), this);
    QString configureSortToolTipText = tr("Configure table sorting options");
    configureSortAct->setToolTip(configureSortToolTipText);
    configureSortAct->setStatusTip(configureSortToolTipText);
    configureSortAct->setWhatsThis(tr("<html>Click here to configure options "
        "related to the sorting of table columns.</html>"));
    connect(configureSortAct, SIGNAL(triggered()),
        this, SLOT(configSorting()));
    QList< QString > configureSortLocation;
    configureSortLocation.append(tr("&Tables"));
    m_menuActions->insert(configureSortAct, configureSortLocation);

    QAction *xyzDisplayAct = new QAction(QIcon(QString::fromStdString(FileName("$ISISROOT/appdata/images/icons/xyz.png").expanded())),
        tr("&X,Y,Z"), this);
    xyzDisplayAct->setCheckable(true);
    xyzDisplayAct->setChecked(false);    
    QString configureXYZDisplayToolTipText = tr("X,Y,Z Coordinate Display");
    xyzDisplayAct->setToolTip(configureXYZDisplayToolTipText);
    xyzDisplayAct->setStatusTip(configureXYZDisplayToolTipText);
    xyzDisplayAct->setWhatsThis(tr("<html>Click here to select coordinate display in : "
        "X, Y, Z</html>"));    
    connect(xyzDisplayAct, SIGNAL(triggered()),
        this, SLOT(setXYZCoordinateDisplay()));
    QList< QString > xyzDisplayLocation;
    xyzDisplayLocation.append(tr("&Coordinate Display"));
    QActionGroup *coordDisplayGroup = new QActionGroup(this);
    coordDisplayGroup->addAction(xyzDisplayAct);
    m_menuActions->insert(xyzDisplayAct, xyzDisplayLocation);

    QAction *latLonRadiusDisplayAct
        = new QAction(QIcon(QString::fromStdString(FileName("$ISISROOT/appdata/images/icons/latlonrad.png").expanded())),
          tr("&Lat,Lon,Radius"), this);
    latLonRadiusDisplayAct->setCheckable(true);
    latLonRadiusDisplayAct->setChecked(true);
    QString configureLatLonRadiusDisplayToolTipText = tr("Lat,Lon,Radius Coordinate Display");
    latLonRadiusDisplayAct->setToolTip(configureLatLonRadiusDisplayToolTipText);
    latLonRadiusDisplayAct->setStatusTip(configureLatLonRadiusDisplayToolTipText);
    latLonRadiusDisplayAct->setWhatsThis(tr("<html>Click here to select coordinate display in : "
        "latitude, longitude, radius</html>"));
    connect(latLonRadiusDisplayAct, SIGNAL(triggered()),
        this, SLOT(setLatLonRadiusCoordinateDisplay()));
    QList< QString > latLonRadiusDisplayLocation;
    latLonRadiusDisplayLocation.append(tr("&Coordinate Display"));
    coordDisplayGroup->addAction(latLonRadiusDisplayAct);
    m_menuActions->insert(latLonRadiusDisplayAct, latLonRadiusDisplayLocation);

    QAction *whatsThisAct = QWhatsThis::createAction(this);
    QList< QString > whatsThisLocation;
    whatsThisLocation.append(tr("&Help"));
    m_menuActions->insert(whatsThisAct, whatsThisLocation);

    QList< QAction * > tbActionList;
    tbActionList.append(freezeTablesAct);
    tbActionList.append(configureSortAct);
    tbActionList.append(latLonRadiusDisplayAct);
    tbActionList.append(xyzDisplayAct);
    m_toolBarActions->insert("settingsToolBar", tbActionList);
  }


  /**
   * Creates the point tree view and adds it to the model
   */
  void CnetEditorWidget::createPointTreeView() {
    m_pointTreeView = new TreeView();
    m_pointTreeView->setTitle("Point View");
    m_pointModel = new PointMeasureTreeModel(m_control->controlNet(), m_pointTreeView, qApp);
    m_pointTreeView->setModel(m_pointModel);
  }


  /**
   * Creates the serial tree and adds it to the model
   */
  void CnetEditorWidget::createSerialTreeView() {
    m_imageTreeView = new TreeView();
    m_imageTreeView->setTitle("Cube View");
    m_imageModel = new ImagePointTreeModel(m_control->controlNet(), m_imageTreeView, qApp);
    m_imageTreeView->setModel(m_imageModel);
  }


  /**
   * Creates the connection tree and adds it to the model
   */
  void CnetEditorWidget::createConnectionTreeView() {
    m_connectionTreeView = new TreeView();
    m_connectionTreeView->setTitle("Cube Connection View");
    m_connectionModel = new ImageImageTreeModel(m_control->controlNet(), m_connectionTreeView, qApp);
    m_connectionTreeView->setModel(m_connectionModel);
  }


  /**
   * Creates filter widgets
   */
  void CnetEditorWidget::createFilterArea() {

    FilterWidget *pointFilter = new FilterWidget("Points and Measures");
    if (m_pointModel) {
      m_pointModel->setFilter(pointFilter);
    }

    QHBoxLayout *pointFilterLayout = new QHBoxLayout;
    pointFilterLayout->addWidget(pointFilter);
    QWidget *pointArea = new QWidget;
    pointArea->setLayout(pointFilterLayout);
    QScrollArea *pointFilterScrollArea = new QScrollArea;
    pointFilterScrollArea->setWidget(pointArea);
    pointFilterScrollArea->setWidgetResizable(true);
    m_pointFilterWidget = pointFilterScrollArea;

    FilterWidget *serialFilter = new FilterWidget("Images and Points");
    if (m_imageModel) {
      m_imageModel->setFilter(serialFilter);
    }

    QHBoxLayout *serialFilterLayout = new QHBoxLayout;
    serialFilterLayout->addWidget(serialFilter);
    QWidget *serialArea = new QWidget;
    serialArea->setLayout(serialFilterLayout);
    QScrollArea *serialFilterScrollArea = new QScrollArea;
    serialFilterScrollArea->setWidget(serialArea);
    serialFilterScrollArea->setWidgetResizable(true);
    m_serialFilterWidget = serialFilterScrollArea;

    FilterWidget *connectionFilter = new FilterWidget("Connections");
    if (m_connectionModel) {
      m_connectionModel->setFilter(connectionFilter);
    }

    QHBoxLayout *connectionFilterLayout = new QHBoxLayout;
    connectionFilterLayout->addWidget(connectionFilter);
    QWidget *connectionArea = new QWidget;
    connectionArea->setLayout(connectionFilterLayout);
    QScrollArea *connectionFilterScrollArea = new QScrollArea;
    connectionFilterScrollArea->setWidget(connectionArea);
    connectionFilterScrollArea->setWidgetResizable(true);
    m_connectionFilterWidget = connectionFilterScrollArea;
  }


  /**
   * Creates the point table and adds it to the widget
   */
  void CnetEditorWidget::createPointTableView() {
    m_pointTableModel = new PointTableModel(m_pointModel);

    connect(this, SIGNAL(coordinateDisplayTypeChanged()),
            m_pointTableModel, SLOT(resetColumnHeaders()));

    m_pointTableView = new TableView(m_pointTableModel, *m_settingsPath,
        "m_pointTableView");
    m_pointTableView->setWhatsThis("<html>Each row in the table is a control "
        "point.  Each column in the table is an attribute of a control "
        "point.<br/><br/>Cells that are gray are not editable.</html>");
    connect(m_pointTableView, SIGNAL(modelDataChanged()),
        this, SIGNAL(cnetModified()));

    connect(m_pointTreeView, SIGNAL(selectionChanged()),
        m_pointTableView, SLOT(handleModelSelectionChanged()));
    connect(m_pointTableView, SIGNAL(selectionChanged()),
        m_pointTreeView, SLOT(handleModelSelectionChanged()));

    connect(m_pointTableView,
        SIGNAL(rebuildModels(QList< AbstractTreeItem * >)),
        this,
        SLOT(rebuildModels(QList< AbstractTreeItem * >)));

    connect(m_pointTableView, SIGNAL(filterCountsChanged(int, int)),
        this, SLOT(handlePointTableFilterCountsChanged(int, int)));

    connect(m_pointTableView, SIGNAL(editControlPoint(ControlPoint *, QString)),
            this, SIGNAL(editControlPoint(ControlPoint *, QString)));

    for (int i = 0; i < AbstractPointItem::COLS; i++) {
      QAction *act = new QAction(
        AbstractPointItem::getColumnName((AbstractPointItem::Column) i), this);
      act->setCheckable(true);
      connect(act, SIGNAL(toggled(bool)), this, SLOT(pointColToggled()));
      m_pointTableView->getHorizontalHeader()->addAction(act);
    }

    m_pointTableView->getHorizontalHeader()->setContextMenuPolicy(
      Qt::ActionsContextMenu);
  }


  /**
   * Creates the measure table and adds it to the widget
   */
  void CnetEditorWidget::createMeasureTableView() {
    m_measureTableModel = new MeasureTableModel(m_pointModel);
    m_measureTableView = new TableView(m_measureTableModel, *m_settingsPath,
        "m_measureTableView");
    m_measureTableView->setWhatsThis("<html>Each row in the table is a control "
        "measure.  Each column in the table is an attribute of a control "
        "measure.<br/><br/>Rows with bold text are reference measures.  "
        "Cells that are gray are not editable.</html>");
    connect(m_pointTableView,
        SIGNAL(tableSelectionChanged(QList< AbstractTreeItem * >)),
        m_measureTableModel,
        SLOT(handleTreeSelectionChanged(QList<AbstractTreeItem *>)));

    connect(m_measureTableView,
        SIGNAL(tableSelectionChanged(QList< AbstractTreeItem * >)),
        m_pointTableModel,
        SLOT(handleTreeSelectionChanged(QList< AbstractTreeItem * >)));


    connect(m_measureTableView, SIGNAL(modelDataChanged()),
        this, SIGNAL(cnetModified()));
    connect(m_pointTreeView, SIGNAL(selectionChanged()),
        m_measureTableView, SLOT(handleModelSelectionChanged()));
    connect(m_measureTableView, SIGNAL(selectionChanged()),
        m_pointTreeView, SLOT(handleModelSelectionChanged()));
    connect(m_measureTableView,
        SIGNAL(rebuildModels(QList< AbstractTreeItem * >)),
        this,
        SLOT(rebuildModels(QList< AbstractTreeItem * >)));

    connect(m_measureTableView, SIGNAL(filterCountsChanged(int, int)),
        this, SLOT(handleMeasureTableFilterCountsChanged(int, int)));

    connect(m_measureTableView, SIGNAL(editControlPoint(ControlPoint *, QString)),
            this, SIGNAL(editControlPoint(ControlPoint *, QString)));

    for (int i = 0; i < AbstractMeasureItem::COLS; i++) {
      QAction *act = new QAction(AbstractMeasureItem::getColumnName(
          (AbstractMeasureItem::Column) i), this);
      act->setCheckable(true);
      connect(act, SIGNAL(toggled(bool)), this, SLOT(measureColToggled()));
      m_measureTableView->getHorizontalHeader()->addAction(act);
    }

    m_measureTableView->getHorizontalHeader()->setContextMenuPolicy(
      Qt::ActionsContextMenu);
  }


  /**
   * Rebuilds the models
   */
  void CnetEditorWidget::rebuildModels() {
    rebuildModels(QList< AbstractTreeItem * >());
  }


  /**
   * Toggles the point column
   */
  void CnetEditorWidget::pointColToggled() {
    QList< QAction * > actions =
      m_pointTableView->getHorizontalHeader()->actions();

    for (int i = 0; i < actions.size(); i++) {
      m_pointTableView->setColumnVisible(actions[i]->text(),
          actions[i]->isChecked());
    }
  }


  /**
   * Toggles the measure column
   */
  void CnetEditorWidget::measureColToggled() {
    QList< QAction * > actions =
      m_measureTableView->getHorizontalHeader()->actions();
    for (int i = 0; i < actions.size(); i++)
      m_measureTableView->setColumnVisible(actions[i]->text(),
          actions[i]->isChecked());
  }


  /**
   * Handles point table filter changes.
   *
   * @param visibleRows Number of visible rows
   * @param totalRows Total number of rows
   *
   * @see handleTableFilterCountsChanged(int visibleRows, int totalRows, QGroupBox *box,
   *                                     QString initialText)
   */
  void CnetEditorWidget::handlePointTableFilterCountsChanged(
    int visibleRows, int totalRows) {
    handleTableFilterCountsChanged(visibleRows, totalRows, m_pointTableBox,
        "Control Point Table");
  }


  /**
   * Handles measure table filter changes
   *
   * @param visibleRows Number of visible rows
   * @param totalRows Total number of rows
   *
   * @see handleTableFilterCountsChanged(int visibleRows, int totalRows, QGroupBox *box,
   *                                     QString initialText)
   */
  void CnetEditorWidget::handleMeasureTableFilterCountsChanged(
    int visibleRows, int totalRows) {
    handleTableFilterCountsChanged(visibleRows, totalRows, m_measureTableBox,
        "Control Measure Table");
  }


  /**
   * Handles any table filter changes
   *
   * @param visibleRows Number of visible rows
   * @param totalRows Total number of rows
   * @param box The table box to change
   * @param initialText The initial text at the top of the table
   */
  void CnetEditorWidget::handleTableFilterCountsChanged(
    int visibleRows, int totalRows, QGroupBox *box, QString initialText) {
    if (box) {
      QString newTitle = initialText + " (";
      if (visibleRows > -1)
        newTitle += QString::number(visibleRows);
      else
        newTitle += "???";

      newTitle += " / " + QString::number(totalRows) + ")";

      box->setTitle(newTitle);
    }
  }


  /**
   * Upgrades the working version number
   */
  void CnetEditorWidget::upgradeVersion() {
    if (*m_workingVersion == "") {
      *m_workingVersion = "0.1";
    }

    if (*m_workingVersion != VERSION)
      upgradeVersion();
  }


  /**
   * Reads the working version settings stored at the settings path
   */
  void CnetEditorWidget::readSettings() {

    QSettings settings(*m_settingsPath, QSettings::NativeFormat);
    *m_workingVersion = settings.value("version", "").toString();

    m_mainSplitter->restoreState(settings.value("mainSplitter").toByteArray());

    QString key;

    QList< QAction * > actions =
      m_measureTableView->getHorizontalHeader()->actions();
    for (int i = 0; i < actions.size(); i++) {
      key = m_measureTableView->objectName() + " " +
          AbstractMeasureItem::getColumnName((AbstractMeasureItem::Column) i);
      key.replace(" ", "_");
      actions[i]->setChecked(settings.value(key, true).toBool());
    }

    actions = m_pointTableView->getHorizontalHeader()->actions();
    for (int i = 0; i < actions.size(); i++) {
      key = m_pointTableView->objectName() + " " +
          AbstractPointItem::getColumnName((AbstractPointItem::Column) i);
      key.replace(" ", "_");
      actions[i]->setChecked(settings.value(key, true).toBool());
    }

    // Restore sorting configuration settings.
    setMeasureTableSortingEnabled(
      settings.value("measureTableSortingEnabled", true).toBool());
    setMeasureTableSortLimit(
      settings.value("measureTableSortLimit", 500000).toInt());
    setPointTableSortingEnabled(
      settings.value("pointTableSortingEnabled", true).toBool());
    setPointTableSortLimit(
      settings.value("pointTableSortLimit", 100000).toInt());
  }


  /**
   * Writes the configuration settings used
   */
  void CnetEditorWidget::writeSettings() {

    QSettings settings(*m_settingsPath, QSettings::NativeFormat);
    settings.setValue("version", VERSION);
    settings.setValue("mainSplitter", m_mainSplitter->saveState());

    QString key;

    QList< QAction * > actions =
      m_measureTableView->getHorizontalHeader()->actions();
    for (int i = 0; i < actions.size(); i++) {
      key = m_measureTableView->objectName() + " " +
          AbstractMeasureItem::getColumnName((AbstractMeasureItem::Column) i);
      key.replace(" ", "_");
      settings.setValue(key, actions[i]->isChecked());
    }

    actions = m_pointTableView->getHorizontalHeader()->actions();
    for (int i = 0; i < actions.size(); i++) {
      key = m_pointTableView->objectName() + " " +
          AbstractPointItem::getColumnName((AbstractPointItem::Column) i);
      key.replace(" ", "_");
      settings.setValue(key, actions[i]->isChecked());
    }

    // Write sorting configuration settings.
    settings.setValue("measureTableSortingEnabled",
        measureTableSortingEnabled());
    settings.setValue("measureTableSortLimit",
        measureTableSortLimit());
    settings.setValue("pointTableSortingEnabled",
        pointTableSortingEnabled());
    settings.setValue("pointTableSortLimit",
        pointTableSortLimit());
  }


  /**
   * Returns the point tree view
   *
   * @return QWidget The point tree view
   */
  QWidget *CnetEditorWidget::pointTreeView() {
    return m_pointTreeView;
  }


  /**
   * Returns the serial tree view
   *
   * @return QWidget The serial tree view
   */
  QWidget *CnetEditorWidget::serialTreeView() {
    return m_imageTreeView;
  }


  /**
   * Returns the connection tree view
   *
   * @return QWidget The connection tree view
   */
  QWidget *CnetEditorWidget::connectionTreeView() {
    return m_connectionTreeView;
  }


  /**
   * Returns the point filter widget
   *
   * @return QWidget The point filter widget
   */
  QWidget *CnetEditorWidget::pointFilterWidget() {
    return m_pointFilterWidget;
  }


  /**
   * Returns the serial filter widget
   *
   * @return QWidget The serial filter widget
   */
  QWidget *CnetEditorWidget::serialFilterWidget() {
    return m_serialFilterWidget;
  }


  /**
   * Returns the connection filter widget
   *
   * @return QWidget The connection filter widget
   */
  QWidget *CnetEditorWidget::connectionFilterWidget() {
    return m_connectionFilterWidget;
  }


  /**
   * Returns the point table view
   *
   * @return TableView The point table view
   */
  TableView *CnetEditorWidget::pointTableView() {
    return m_pointTableView;
  }


  /**
   * Returns the measure table view
   *
   * @return TableView The measure table view
   */
  TableView *CnetEditorWidget::measureTableView() {
    return m_measureTableView;
  }


  /**
   * Returns the measure table model
   *
   * @return AbstractTableModel The measure table model
   */
  AbstractTableModel *CnetEditorWidget::measureTableModel() {
    return m_measureTableModel;
  }


  /**
   * Returns the point table model
   *
   * @return AbstractTableModel The point table model
   */
  AbstractTableModel *CnetEditorWidget::pointTableModel() {
    return m_pointTableModel;
  }


  /**
   * Returns the control network
   *
   * @return ControlNet The control network
   */
  ControlNet *CnetEditorWidget::control() {
    return m_control->controlNet();
  }


  /**
   * Returns the menu actions
   *
   * @return QMap< QAction *, QList< QString > > QMap of menu actions
   */
  QMap< QAction *, QList< QString > > CnetEditorWidget::menuActions() {
    return *m_menuActions;
  }


  /**
   * Returns the tool bar actions
   *
   * @return QMap< QAction *, QList< QString > > QMap of tool bar actions
   */
  QMap< QString, QList< QAction * > > CnetEditorWidget::toolBarActions() {
    return *m_toolBarActions;
  }


  /**
   * Returns the filtered control net
   *
   * @return ControlNet Filtered control network
   */
  ControlNet *CnetEditorWidget::filteredNetwork() const {
    ControlNet *filteredCnet = new ControlNet(*(m_control->controlNet()));

    QList<AbstractTreeItem *> networkItems = m_pointModel->getItems(0, -1,
        AbstractTreeModel::MeasureItems | AbstractTreeModel::PointItems, true);

    // Iterate through our copy of the cnet, deleting anything that doesn't
    //   exactly match our networkItems.
    for (int pointIndex = filteredCnet->GetNumPoints() - 1;
        pointIndex >= 0;
        pointIndex--) {
      if (networkItems.isEmpty()) {
        ControlPoint *cp = filteredCnet->GetPoint(pointIndex);
        cp->SetEditLock(false);

        for (int measureIndex = 0;
            measureIndex < cp->GetNumMeasures();
            measureIndex++) {
          cp->GetMeasure(measureIndex)->SetEditLock(false);
        }

        filteredCnet->DeletePoint(cp);
      }
      else if (networkItems.last()->getPointerType() ==
          AbstractTreeItem::Point) {
        ControlPoint *networkItemsCp =
          (ControlPoint *)networkItems.last()->getPointer();
        ControlPoint *cp = filteredCnet->GetPoint(pointIndex);
        if (cp->GetId() != networkItemsCp->GetId()) {
          cp->SetEditLock(false);

          for (int measureIndex = 0;
              measureIndex < cp->GetNumMeasures();
              measureIndex++) {
            cp->GetMeasure(measureIndex)->SetEditLock(false);
          }

          filteredCnet->DeletePoint(cp);
        }
        else {
          networkItems.removeLast();
        }
      }
      else if (networkItems.last()->getPointerType() ==
          AbstractTreeItem::Measure) {
        ControlPoint *cp = filteredCnet->GetPoint(pointIndex);
        ControlMeasure *networkItemsCm =
          (ControlMeasure *)networkItems.last()->getPointer();

        if (cp->GetId() != networkItemsCm->Parent()->GetId()) {
          cp->SetEditLock(false);

          for (int measureIndex = 0;
              measureIndex < cp->GetNumMeasures();
              measureIndex++) {
            cp->GetMeasure(measureIndex)->SetEditLock(false);
          }

          filteredCnet->DeletePoint(cp);
        }
        else {
          // Our CP stays, figure out which CMs stay.
          for (int measureIndex = cp->GetNumMeasures() - 1;
              networkItemsCm && measureIndex >= 0;
              measureIndex--) {
            ControlMeasure *cm = cp->GetMeasure(measureIndex);
            if (cm->GetCubeSerialNumber() !=
                networkItemsCm->GetCubeSerialNumber()) {
              cm->SetEditLock(false);
              cp->Delete(cm);
            }
            else {
              networkItems.removeLast();
              networkItemsCm = NULL;

              if (networkItems.last()->getPointerType() ==
                  AbstractTreeItem::Measure) {
                networkItemsCm =
                  (ControlMeasure *)networkItems.last()->getPointer();
              }
            }
          }

          // We still need to verify the copied CP at this index... although
          //   nothing should go wrong, we know things do go wrong so do
          //   the verify instead of just tossing the last networkItems item.
          pointIndex++;
        }
      }
    }

    return filteredCnet;
  }


  /**
   * Returns true if the measure table can be sorted
   *
   * @return bool True if sorting is enabled on the measure table
   */
  bool CnetEditorWidget::measureTableSortingEnabled() const {
    return m_measureTableModel->sortingIsEnabled();
  }


  /**
   * Returns the sorting limit for the measure table
   *
   * @return int The sorting limit for the measure table
   */
  int CnetEditorWidget::measureTableSortLimit() const {
    return m_measureTableModel->sortLimit();
  }


  /**
   * Returns true if the point table can be sorted
   *
   * @return bool True if sorting is enabled on the point table
   */
  bool CnetEditorWidget::pointTableSortingEnabled() const {
    return m_pointTableModel->sortingIsEnabled();
  }


  /**
   * Returns the sorting limit for the point table
   *
   * @return int The sorting limit for the point table
   */
  int CnetEditorWidget::pointTableSortLimit() const {
    return m_pointTableModel->sortLimit();
  }


  /**
   * Sets if the measure table can be sorted
   *
   * @param bool Bool if the table can be sorted or not
   */
  void CnetEditorWidget::setMeasureTableSortingEnabled(bool enabled) {
    m_measureTableModel->setSortingEnabled(enabled);
  }


  /**
   * Sets the measure table sorting limit
   *
   * @param int Sorting limit
   */
  void CnetEditorWidget::setMeasureTableSortLimit(int limit) {
    m_measureTableModel->setSortLimit(limit);
  }


  /**
   * Sets if the point table can be sorted
   *
   * @param bool Bool if the table can be sorted or not
   */
  void CnetEditorWidget::setPointTableSortingEnabled(bool enabled) {
    m_pointTableModel->setSortingEnabled(enabled);
  }


  /**
   * Sets the point table sorting limit
   *
   * @param int Sorting limit
   */
  void CnetEditorWidget::setPointTableSortLimit(int limit) {
    m_pointTableModel->setSortLimit(limit);
  }


  /**
   * Configures the sorting dialog.
   * If one does not already exist, create it, then show the dialog
   */
  void CnetEditorWidget::configSorting() {
    if (!m_sortDialog) {
      m_sortDialog = new CnetEditorSortConfigDialog(this);
    }
    m_sortDialog->show();
  }


  /**
   * Set control point coordinate display type to latitude, longitude, radius.
   *
   * TODO: can we selectively rebuild only the columns that need to be re-displayed?
   */
  void CnetEditorWidget::setLatLonRadiusCoordinateDisplay() {
    CnetDisplayProperties *displayProperties = CnetDisplayProperties::getInstance();

    if (displayProperties->coordinateDisplayType() == CnetDisplayProperties::LatLonRadius) {
      return;
    }

    displayProperties->setCoordinateDisplayType(CnetDisplayProperties::LatLonRadius);
    emit(coordinateDisplayTypeChanged());
    m_pointModel->rebuildItems();
  }


  /**
   * Set control point coordinate display type to X,Y,Z.
   *
   * TODO: can we selectively rebuild only the columns that need to be re-displayed?
   */
  void CnetEditorWidget::setXYZCoordinateDisplay() {
    CnetDisplayProperties *displayProperties = CnetDisplayProperties::getInstance();

    if (displayProperties->coordinateDisplayType() == CnetDisplayProperties::XYZ) {
      return;
    }

    displayProperties->setCoordinateDisplayType(CnetDisplayProperties::XYZ);
    emit(coordinateDisplayTypeChanged());
    m_pointModel->rebuildItems();
  }


  /**
   * Sets if the tables are frozen
   *
   * @param bool Bool that freezes the tables or not
   */
  void CnetEditorWidget::setTablesFrozen(bool freezeTables) {
    if (freezeTables) {
      m_connectionModel->setFrozen(true);
      m_imageModel->setFrozen(true);
      m_pointModel->setFrozen(true);
    }
    else {
      m_pointModel->setFrozen(false);
      m_imageModel->setFrozen(false);
      m_connectionModel->setFrozen(false);
    }
  }


  /**
   * Connected to cnetModified(). Sets the modification of m_control when the cnet is modified.
   */
  void CnetEditorWidget::setCnetModified() {
    m_control->setModified(true);
  }
}
