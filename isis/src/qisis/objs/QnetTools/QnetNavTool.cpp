#include "QnetNavTool.h"

#include <QAction>
#include <QApplication>
#include <QComboBox>
#include <QDialog>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QListWidget>
#include <QListWidgetItem>
#include <QMessageBox>
#include <QPushButton>
#include <QScrollArea>
#include <QSettings>
#include <QStackedWidget>
#include <QString>
#include <QTabWidget>
#include <QVBoxLayout>
#include <QWidget>

#include "QnetCubeDistanceFilter.h"
#include "QnetCubeNameFilter.h"
#include "QnetCubePointsFilter.h"
#include "QnetPointCubeNameFilter.h"
#include "QnetPointDistanceFilter.h"
#include "QnetPointJigsawErrorFilter.h"
#include "QnetPointRegistrationErrorFilter.h"
#include "QnetPointGoodnessFilter.h"
#include "QnetPointIdFilter.h"
#include "QnetPointImagesFilter.h"
#include "QnetPointMeasureFilter.h"
#include "QnetPointRangeFilter.h"
#include "QnetPointTypeFilter.h"
#include "QnetSetAprioriDialog.h"
#include "QnetTool.h"

#include "Angle.h"
#include "Camera.h"
#include "ControlMeasure.h"
#include "ControlNet.h"
#include "ControlPoint.h"
#include "Distance.h"
#include "FileName.h"
#include "IException.h"
#include "Latitude.h"
#include "Longitude.h"
#include "SerialNumberList.h"
#include "SpecialPixel.h"
#include "SurfacePoint.h"

using namespace std;

namespace Isis {
  /**
   * Constructs the Navigation Tool window
   *
   * @param parent The parent widget for the navigation tool
   *
   * @internal
   *   @history  2008-12-09 Tracie Sucharski - Added m_filtered indicating whether
   *                           the listBox contains filtered or unfiltered
   *                           list.
   *   @history 2010-06-03 Jeannie Walldren - Initialized pointers to null.
   *
   */
  QnetNavTool::QnetNavTool(QnetTool *qnetTool, QWidget *parent) : Tool(parent) {
    m_navDialog = NULL;
    m_filter = NULL;
    m_tie = NULL;
    m_multiIgnore = NULL;
    m_multiDelete = NULL;
    m_setApriori = NULL;
    m_filterStack = NULL;
    m_listCombo = NULL;
    m_listBox = NULL;
    m_filterCountLabel = NULL;
    m_aprioriDialog = NULL;
    m_qnetTool = qnetTool;
    m_filtered = false;

    createNavigationDialog(parent);
    connect(this, SIGNAL(deletedPoints()), this, SLOT(refreshList()));
    connect(this, SIGNAL( activityUpdate(QString) ),
            this, SLOT( updateActivityHistory(QString) ) );
  }

  /**
   * Creates and shows the dialog box for the navigation tool
   *
   * @param parent The parent widget for the navigation dialog
   *
   * @internal
   *   @history  2008-10-29 Tracie Sucharski - Added filter count
   *   @history  2008-12-31 Jeannie Walldren - Added keyboard shortcuts
   *   @history  2010-11-04 Tracie Sucharski - Move listBox double-click
   *                            connection to the slot for changing the
   *                            listBox.
   */
  void QnetNavTool::createNavigationDialog(QWidget *parent) {
    // Create the combo box selector
    m_listCombo = new QComboBox();
    m_listCombo->addItem("Points");
    m_listCombo->addItem("Cubes");

    m_listBox = new QListWidget();
    m_listBox->setSelectionMode(QAbstractItemView::ExtendedSelection);

    // Create the filter area
    QLabel *filterLabel = new QLabel("Filters");
    filterLabel->setAlignment(Qt::AlignHCenter);
    m_filterStack = new QStackedWidget();

    connect(m_listCombo, SIGNAL(activated(int)),
        m_filterStack, SLOT(setCurrentIndex(int)));
    connect(m_listCombo, SIGNAL(activated(int)),
        this, SLOT(filterList()));
    connect(m_listCombo, SIGNAL(activated(int)),
        this, SLOT(enableButtons()));

    //  Create filter count label
    m_filterCountLabel = new QLabel("Filter Count: ");

    // Create action options
    QPushButton *load = new QPushButton("&View Cube(s)");
    load->setAutoDefault(false);
    load->setToolTip("Open Selected Images");
    load->setWhatsThis("<b>Function: </b> Opens all selected images, or images \
                        that are associated with the given point or overlap.  \
                        <p><b>Hint: </b> You can select more than one item in \
                        the list by using the shift or control key.</p>");
    connect(load, SIGNAL(clicked()),
        this, SLOT(load()));
    m_tie = new QPushButton("&Modify Point");
    m_tie->setAutoDefault(true);
    m_tie->setToolTip("Modify Selected Point");
    m_tie->setWhatsThis("<b>Function: </b> Opens the tie tool to modify the \
                         selected point from the list.  This option is only \
                         available when the nav tool is in point mode");
    connect(m_tie, SIGNAL(clicked()),
        this, SLOT(tie()));

    m_multiIgnore = new QPushButton("&Ignore Points");
    m_multiIgnore->setAutoDefault(false);
    m_multiIgnore->setToolTip("Set selected points to Ignore");
    m_multiIgnore->setWhatsThis("<b>Function: </b> Sets the selected points \
                               Ignore = True.  You will not be able to preview \
                               in the Point Editor before their Ignore switch \
                               is set to true. \
                               <p><b>Hint: </b> You can select more than one \
                               item in the list by using the shift or control \
                               key.</p>");
    connect(m_multiIgnore, SIGNAL(clicked()),
        this, SLOT(ignorePoints()));

    m_multiDelete = new QPushButton("&Delete Points");
    m_multiDelete->setAutoDefault(false);
    m_multiIgnore->setToolTip("Set selected points to Delete");
    m_multiIgnore->setWhatsThis("<b>Function: </b> Delete the selected points \
                               from control network.  You will not be able to \
                               preview in the Point Editor before they are \
                               deleted. \
                               <p><b>Hint: </b> You can select more than one \
                               item in the list by using the shift or control \
                               key.</p>");
    connect(m_multiDelete, SIGNAL(clicked()),
        this, SLOT(deletePoints()));

    m_setApriori = new QPushButton("&Set Apriori/Sigmas");
    m_setApriori->setAutoDefault(false);
    m_setApriori->setToolTip("Set selected points apriori/sigmas");
    m_setApriori->setWhatsThis("<b>Function: </b> Set the apriori points \
                               and sigmas. \
                               <p><b>Hint: </b> You can select more than one \
                               item in the list by using the shift or control \
                               key.</p>");
    connect(m_setApriori, SIGNAL(clicked()), this, SLOT(aprioriDialog()));

    m_filter = new QPushButton("&Filter");
    m_filter->setAutoDefault(false);
    m_filter->setToolTip("Filter Current List");
    m_filter->setWhatsThis("<b>Function: </b> Filters the current list by user \
                            specifications made in the selected filter. \
                            <p><b>Note: </b> Any filter options selected in a \
                            filter that is not showing will be ignored.</p>");
    connect(m_filter, SIGNAL(clicked()),
        this, SLOT(filter()));

    QPushButton *reset = new QPushButton("&Show All");
    reset->setAutoDefault(false);
    reset->setToolTip("Reset the Current List to show all the values in the list");
    reset->setWhatsThis("<b>Function: </b> Resets the list of points, \
                         overlaps, or images to the complete initial list.  \
                         Any filtering that has been done will be lost.");
    connect(reset, SIGNAL(clicked()),
        this, SLOT(resetList()));
    connect(reset, SIGNAL(clicked()),
        this, SLOT(resetFilter()));

    // Set up layout
    QHBoxLayout *layout = new QHBoxLayout();
    layout->addWidget(load);
    layout->addWidget(m_tie);
    layout->addWidget(m_multiIgnore);
    layout->addWidget(m_multiDelete);
    layout->addWidget(m_setApriori);
    layout->addWidget(m_filter);
    layout->addWidget(reset);

    // Create filter stacked widgets
    createFilters();
    m_filterStack->adjustSize();

    // Set up the main window
    m_navDialog = new QDialog(parent);
    m_navDialog->setWindowTitle("Control Network Navigator");

    // Layout everything in the dialog
    QGridLayout *gridLayout = new QGridLayout();
    gridLayout->addWidget(m_listCombo, 0, 0);
    gridLayout->addWidget(filterLabel, 0, 1);
    gridLayout->addWidget(m_listBox, 1, 0);
    gridLayout->addWidget(m_filterStack, 1, 1);
    gridLayout->addWidget(m_filterCountLabel, 2, 0);
    gridLayout->addLayout(layout, 3, 0, 1, 2);
    m_navDialog->setLayout(gridLayout);

    QString settingsFileName =
        FileName("$HOME/.Isis/" + QApplication::applicationName() + "/NavTool.config").expanded();
    QSettings settings(settingsFileName, QSettings::NativeFormat);
    m_navDialog->resize(settings.value("size").toSize());

    // View the dialog - we need this to get the size of the dialog which we're using
    // for positioning it.
    m_navDialog->setVisible(true);

    QPoint defaultPos = parent->pos() +
                        QPoint(parent->size().width() / 2,
                               parent->size().height() / 2);
    defaultPos -= QPoint(m_navDialog->size().width() / 2,
                         m_navDialog->size().height() / 2);
    m_navDialog->move(settings.value("pos", defaultPos).toPoint());

  }


  QnetNavTool::~QnetNavTool() {
    QString settingsFileName =
        FileName("$HOME/.Isis/" + QApplication::applicationName() + "/NavTool.config").expanded();
    QSettings settings(settingsFileName, QSettings::NativeFormat);

    settings.setValue("size", m_navDialog->size());
    settings.setValue("pos", m_navDialog->pos());
  }


  /**
   * Sets up the tabbed widgets for the different types of filters available
   *
   * @internal
   *   @history  2007-06-05 Tracie Sucharski - Added enumerators for the filter
   *                           indices to make it easier to re-order filters.
   *                           Also, re-ordered the filters to put commonly used
   *                           first. Comment out overlap/polygon code
   *                           temporarily.
   *   @history  2008-11-26 Jeannie Walldren - Added Goodness of Fit to the filter
   *                           tabs.
   *   @history  2008-12-31 Jeannie Walldren - Added keyboard shortcuts to tabs.
   *   @history  2009-01-26 Jeannie Walldren - Clarified tab names. Added points
   *                           cube name filter tab.
   *   @history  2010-06-02 Jeannie Walldren - Changed tab labels from "Type" to
   *                           "Properties". Updated "What's This?" documentation
   *                           for Measure Properties to explain use of ignore
   *                           status and measure type filters.
   *   @history  2018-01-10 Adam Goins - Added the Activity History tab to the window.
   *                           This tab will keep track of edits made to control points/measures.
   *                           More history entries can be kept track of
   *                           by emitting the activityUpdate(std::string message) signal.
   */
  void QnetNavTool::createFilters() {
    // Set up the point filters
    QTabWidget *pointFilters = new QTabWidget();

    QWidget *jigsawErrorFilter = new QnetPointJigsawErrorFilter(this);
    connect(jigsawErrorFilter, SIGNAL(filteredListModified()),
            this, SLOT(filterList()));
    pointFilters->insertTab(JigsawErrors, jigsawErrorFilter, "&Jigsaw Errors");
    pointFilters->setTabToolTip(JigsawErrors, "Filter Points by Jigsaw Error");
    pointFilters->setTabWhatsThis(JigsawErrors,
        "<b>Function: </b> Filter points list by \
                                     the bundle adjust error value at each  \
                                     point.  You can filter for points that \
                                     have an error greater than, or less than \
                                     the entered value.");

    QWidget *registrationErrorFilter = new QnetPointRegistrationErrorFilter(this);
    connect(registrationErrorFilter, SIGNAL(filteredListModified()),
            this, SLOT(filterList()));
    pointFilters->insertTab(RegistrationErrors, registrationErrorFilter,
                            "&Registration Errors");
    pointFilters->setTabToolTip(RegistrationErrors,
                                "Filter Points by Registration Error");
    pointFilters->setTabWhatsThis(RegistrationErrors,
        "<b>Function: </b> Filter points list by \
                                     the registration pixel shift value at each  \
                                     point.  You can filter for points that \
                                     have an error greater than, or less than \
                                     the entered value.  The maximum for all \
                                     measures in the point is used");

    QWidget *ptIdFilter = new QnetPointIdFilter(this);
    connect(ptIdFilter, SIGNAL(filteredListModified()),
            this, SLOT(filterList()));
    pointFilters->insertTab(Id, ptIdFilter, "&Point ID");
    pointFilters->setTabToolTip(Id, "Filter Points by PointID");

    QWidget *ptImageFilter = new QnetPointImagesFilter(this);
    connect(ptImageFilter, SIGNAL(filteredListModified()),
            this, SLOT(filterList()));
    pointFilters->insertTab(NumberImages, ptImageFilter, "&Number of Measures");
    pointFilters->setTabToolTip(NumberImages, "Filter Points by Number of Images");
    pointFilters->setTabWhatsThis(NumberImages, "<b>Function: </b> Filter points list \
                                     by the number of images that are in  \
                                     each point. You can filter for         \
                                     points that have more than the given   \
                                     number of images, or less than the \
                                     given number of images.  Points with   \
                                     the exact number of images specified \
                                     will not be included in the filtered \
                                     list.");
    QWidget *typeFilter = new QnetPointTypeFilter(this);
    connect(typeFilter, SIGNAL(filteredListModified()),
        this, SLOT(filterList()));
    pointFilters->insertTab(Type, typeFilter, "Point Properties");
    pointFilters->setTabToolTip(Type, "Filter Points by Listed Properties");
    pointFilters->setTabWhatsThis(Type, "<b>Function: </b> Filter points list by \
                                     their Point Type, Ignore status, or Held status properties");
    QWidget *rangeFilter = new QnetPointRangeFilter(this);
    connect(rangeFilter, SIGNAL(filteredListModified()),
        this, SLOT(filterList()));
    pointFilters->insertTab(LatLonRange, rangeFilter, "&Range");
    pointFilters->setTabToolTip(LatLonRange, "Filter Points by Range");
    pointFilters->setTabWhatsThis(LatLonRange, "<b>Function: </b> Filters out points \
                                     that are within a user set range lat/lon \
                                     range.");
    QWidget *ptDistFilter = new QnetPointDistanceFilter(this);
    connect(ptDistFilter, SIGNAL(filteredListModified()),
        this, SLOT(filterList()));
    pointFilters->insertTab(Distance, ptDistFilter, "Dist&ance");
    pointFilters->setTabToolTip(Distance, "Filter Points by Distance");
    pointFilters->setTabWhatsThis(Distance,
        "<b>Function: </b> Filter points list by \
                                     a user specified maximum distance from \
                                     any other point.");
    QWidget *measureFilter = new QnetPointMeasureFilter(this);
    connect(measureFilter, SIGNAL(filteredListModified()),
        this, SLOT(filterList()));
    pointFilters->insertTab(MeasureType, measureFilter, "Measure Properties");
    pointFilters->setTabToolTip(MeasureType, "Filter Points by Listed Measure Properties");
    pointFilters->setTabWhatsThis(MeasureType,
        "<b>Function: </b> Filter points list by \
                                     the properties of their measures. User may \
                                     filter by Measure Type or Ignore status. \
                                     If one or more measure from a point is found to \
                                     match a selected measure type, and that measure \
                                     satisfies the ignore status selected, the point \
                                     will be left in the filtered list.  More \
                                     than one measure type can be selected. \
                                     Only one Ignore status may be selected.");

    QWidget *goodnessFilter = new QnetPointGoodnessFilter(this);
    connect(goodnessFilter, SIGNAL(filteredListModified()),
            this, SLOT(filterList()));
    pointFilters->insertTab(GoodnessOfFit, goodnessFilter, "&Goodness of Fit");
    pointFilters->setTabToolTip(GoodnessOfFit, "Filter Points by the Goodness of Fit of its measures");
    pointFilters->setTabWhatsThis(GoodnessOfFit,
                                  "<b>Function: </b> Filter points list by \
                                     the goodness of fit.");
    QWidget *cubeNamesFilter = new QnetPointCubeNameFilter(this);
    connect(cubeNamesFilter, SIGNAL(filteredListModified()),
        this, SLOT(filterList()));
    connect(this, SIGNAL(serialListModified()),
        cubeNamesFilter, SLOT(createCubeList()));

    pointFilters->insertTab(CubeName, cubeNamesFilter, "&Cube Name(s)");
    pointFilters->setTabToolTip(CubeName, "Filter Points by Cube FileName(s)");
    pointFilters->setTabWhatsThis(CubeName,
        "<b>Function: </b> Filter points list by \
                                     the filenames of cubes. This filter will \
                                     show all points contained in a single \
                                     image or all points contained in every \
                                     cube selected.");
    // Set up the cube filters
    QTabWidget *cubeFilters = new QTabWidget();

    QWidget *cubeNameFilter = new QnetCubeNameFilter(this);
    connect(cubeNameFilter, SIGNAL(filteredListModified()),
        this, SLOT(filterList()));
    cubeFilters->insertTab(Name, cubeNameFilter, "&Cube Name");
    cubeFilters->setTabToolTip(Name, "Filter Images by Cube Name");

    QWidget *cubePtsFilter = new QnetCubePointsFilter(this);
    connect(cubePtsFilter, SIGNAL(filteredListModified()),
        this, SLOT(filterList()));
    cubeFilters->insertTab(NumberPoints, cubePtsFilter, "&Number of Points");
    cubeFilters->setTabToolTip(NumberPoints, "Filter Images by Number of Points");
    cubeFilters->setTabWhatsThis(NumberPoints,
        "<b>Function: </b> Filter images list by \
                                    the number of points that are in each \
                                    image. You can filter for images that have \
                                    more than the given number of points, or \
                                    less than the given number of point.  \
                                    Images with the exact number of points \
                                    specified will not be included in the \
                                    filtered list.");
    QWidget *cubeDistFilter = new QnetCubeDistanceFilter(this);
    connect(cubeDistFilter, SIGNAL(filteredListModified()),
        this, SLOT(filterList()));
    cubeFilters->insertTab(PointDistance, cubeDistFilter, "Dist&ance");
    cubeFilters->setTabToolTip(PointDistance, "Filter Images by Distance between Points");
    cubeFilters->setTabWhatsThis(PointDistance,
        "<b>Function: </b> Filter images list by \
                                    a user specified distance between points \
                                    in the image. This may be calculated in \
                                    meters or by pixel distance.");

    // Add widgets to the filter stack
    m_filterStack->addWidget(pointFilters);
    m_filterStack->addWidget(cubeFilters);


    // Create the Activity History Tab
    QScrollArea *scrollArea = new QScrollArea();

    QWidget *historyWidget = new QWidget();
    QWidget *innerWidget = new QWidget();

    QVBoxLayout *innerLayout = new QVBoxLayout();
    QLabel *title = new QLabel("<b>History</b>");
    innerLayout->addWidget(title);
    innerLayout->addWidget(scrollArea);

    m_historyLayout = new QVBoxLayout(scrollArea);
    m_historyLayout->setAlignment(Qt::AlignTop);

    innerWidget->setLayout(innerLayout);
    historyWidget->setLayout(m_historyLayout);
    scrollArea->setWidget(historyWidget);
    scrollArea->setWidgetResizable(true);

    pointFilters->addTab(innerWidget, QString("&Activity History") );

  }


  QList<int> &QnetNavTool::filteredImages() {
    return m_filteredImages;
  }


  const QList<int> &QnetNavTool::filteredImages() const {
    return m_filteredImages;
  }


  QList<int> &QnetNavTool::filteredPoints() {
    return m_filteredPoints;
  }


  const QList<int> &QnetNavTool::filteredPoints() const {
    return m_filteredPoints;
  }


  ControlNet *QnetNavTool::controlNet() {
    return m_qnetTool->controlNet();
  }


  const ControlNet *QnetNavTool::controlNet() const {
    return m_qnetTool->controlNet();
  }


  SerialNumberList *QnetNavTool::serialNumberList() {
    return m_qnetTool->serialNumberList();
  }


  const SerialNumberList *QnetNavTool::serialNumberList() const {
    return m_qnetTool->serialNumberList();
  }


  /**
   * Resets the list box with whatever is in the global lists
   *
   * @internal
   *   @history  2007-06-05 Tracie Sucharski - Use enumerators to test which
   *                           filter is chosen.  Comment overlap/polygon code
   *                           temporarily.
   *   @history  2008-10-29 Tracie Sucharski - Added filter count 2008-11-26
   *                           Tracie Sucharski - Remove all polygon/overlap
   *                           references, this functionality will be qmos.
   *   @history  2008-12-09 Tracie Sucharski - Renamed method from updateList to
   *                           resetList since it it reseting all of the filtered
   *                           lists and the listBox to the entire network of
   *                           points and serial numbers.
   *   @history  2008-12-09 Tracie Sucharski - Added m_filtered indicating whether
   *                           the listBox contains filtered or unfiltered list.
   *   @history  2009-01-08 Jeannie Walldren - Reset filtered list with all points
   *                           in control net and all images in serial number
   *                           list.
   *   @history  2010-11-04 Tracie Sucharski - Added double-click connections.
   */
  void QnetNavTool::resetList() {
    m_filtered = false;
    // Dont do anything if there are no cubes loaded
    if (m_qnetTool->serialNumberList() == NULL)
      return;

    // Clear the old list and filtered lists and update with the entire list
    m_listBox->setCurrentRow(-1);
    m_listBox->clear();
    m_filteredPoints.clear();
    m_filteredImages.clear();

    // copy the control net indices into the filtered points list
    int numCp = controlNet()->GetNumPoints();
    m_filteredPoints.reserve(numCp);
    for (int i = 0; i < numCp; i++) {
      m_filteredPoints.push_back(i);
    }
    // copy the serial number indices into the filtered images list
    int numSns = m_qnetTool->serialNumberList()->size();
    m_filteredImages.reserve(numSns);
    for (int i = 0; i < numSns; i++) {
      m_filteredImages.push_back(i);
    }

    // We are dealing with points so output the point numbers
    if (m_listCombo->currentIndex() == Points) {
      // disconnect any old signals
      disconnect(m_listBox, SIGNAL(itemDoubleClicked(QListWidgetItem *)),
          this, SLOT(load(QListWidgetItem *)));
      connect(m_listBox, SIGNAL(itemDoubleClicked(QListWidgetItem *)),
          this, SLOT(editPoint(QListWidgetItem *)), Qt::UniqueConnection);
      //m_listBox->setSelectionMode(QAbstractItemView::SingleSelection);
      for (int i = 0; i < controlNet()->GetNumPoints(); i++) {
        QString cNetId = (*controlNet())[i]->GetId();
        QString itemString = cNetId;
        m_listBox->insertItem(i, itemString);
        int images = (*controlNet())[i]->GetNumMeasures();
        m_listBox->item(i)->setToolTip(QString::number(images) + " image(s) in point");
      }
      //  Make sure edit point is selected and in view.
      updateEditPoint(m_editPointId);

      std::string msg = "Filter Count: " + QString::number(m_listBox->count()) +
          " / " + QString::number(controlNet()->GetNumPoints());
      m_filterCountLabel->setText(msg);
    }
    // We are dealing with images so output the cube names
    else if (m_listCombo->currentIndex() == Cubes) {
      disconnect(m_listBox, SIGNAL(itemDoubleClicked(QListWidgetItem *)),
          this, SLOT(editPoint(QListWidgetItem *)));
      connect(m_listBox, SIGNAL(itemDoubleClicked(QListWidgetItem *)),
          this, SLOT(load(QListWidgetItem *)), Qt::UniqueConnection);
      //m_listBox->setSelectionMode(QAbstractItemView::ExtendedSelection);
      for (int i = 0; i < serialNumberList()->size(); i++) {
        FileName filename = FileName(serialNumberList()->fileName(i));
        QString tempFileName = filename.name();
        m_listBox->insertItem(i, tempFileName);
      }
      std::string msg = "Filter Count: " + QString::number(m_listBox->count()) +
          " / " + QString::number(serialNumberList()->size());
      m_filterCountLabel->setText(msg);

    }
  }


  /**
   * Update the list showing the new point highlighted.
   *
   * @param pointId Value of the PointId keyword for the new point.
   * @internal
   *   @history  2008-12-30 Jeannie Walldren - Modified to setCurrentItem() rather
   *                           than simply highlight the new point using
   *                           setItemSelected() and scrollToItem().
   *   @history  2010-11-01 Tracie Sucharski - Changed pointId from std::string
   *                           to QString.
   */
  void QnetNavTool::updateEditPoint(QString pointId) {

    m_editPointId = pointId;
    if (pointId == "")
      return;

    QList<QListWidgetItem *> items = m_listBox->findItems(pointId, Qt::MatchExactly);
    if (items.isEmpty()) {
      m_listBox->clearSelection();
    }
    else {
      m_listBox->setCurrentItem(items.at(0));
    }
    QString activityMessage("Point selected: " + pointId);
    emit(activityUpdate(activityMessage));
    return;
  }

  /**
  *   Slot to update the history tab with current edits.
  *   It is deisgned not to allow duplicate history entries back to back.
  *
  *   @internal
  *     @history 2018-01-10 Adam Goins - Slot was created.
  */
  void QnetNavTool::updateActivityHistory(QString activityMessage) {

    // Check to ensure duplicate entries aren't added back to back.
    if (m_historyLayout->count() > 0) {
      QWidget *firstEntry = m_historyLayout->layout()->itemAt(0)->widget();
      QLabel *firstLabel = dynamic_cast<QLabel*>(firstEntry);
      if (firstLabel->text() == activityMessage) {
        return;
      }
    }

    QLabel *historyEntry = new QLabel(activityMessage);
    m_historyLayout->insertWidget(0, historyEntry);
  }

  /**
   *   Slot to refresh the listBox
   *
   *   @internal
   *     @history  2008-12-09 Tracie Sucharski - Slot to refresh the ListBox
   */
  void QnetNavTool::refreshList() {
    if (m_filtered) {
      filter();
    }
    else {
      resetList();
    }

  }

  /**
   * Resets the visible filter to the default values
   */
  void QnetNavTool::resetFilter() {

  }

  /**
   * Updates the list box in the nav window with a new list from one of the
   * filters
   *
   * @internal
   *   @history  2007-06-05 Tracie Sucharski - Use enumerators for the filter
   *                           indices.  Comment out overlap/polygon code
   *                           temporarily.
   *   @history  2008-10-29 Tracie Sucharski - Added filter count
   *
   */
  void QnetNavTool::filterList() {
    // Don't do anything if there are no cubes loaded
    if (serialNumberList() == NULL)
      return;

    // Clears the old list and puts the filtered list in its place
    m_listBox->setCurrentRow(-1);
    m_listBox->clear();

    // We are dealing with points so output the point numbers
    if (m_listCombo->currentIndex() == Points) {
      disconnect(m_listBox, SIGNAL(itemDoubleClicked(QListWidgetItem *)),
          this, SLOT(load(QListWidgetItem *)));
      connect(m_listBox, SIGNAL(itemDoubleClicked(QListWidgetItem *)),
          this, SLOT(editPoint(QListWidgetItem *)), Qt::UniqueConnection);

      for (int i = 0; i < m_filteredPoints.size(); i++) {
        QString cNetId = (*controlNet())[m_filteredPoints[i]]->GetId();
        QString itemString = cNetId;
        m_listBox->insertItem(i, itemString);
        int images = (*controlNet())[m_filteredPoints[i]]->GetNumMeasures();
        m_listBox->item(i)->setToolTip(QString::number(images) + " image(s) in point");
      }
      std::string msg = "Filter Count: " + QString::number(m_listBox->count()) +
          " / " + QString::number(controlNet()->GetNumPoints());
      m_filterCountLabel->setText(msg);
      updateEditPoint(m_editPointId);
    }
    // We are dealing with images so write out the cube names
    else if (m_listCombo->currentIndex() == Cubes) {
      disconnect(m_listBox, SIGNAL(itemDoubleClicked(QListWidgetItem *)),
          this, SLOT(editPoint(QListWidgetItem *)));
      connect(m_listBox, SIGNAL(itemDoubleClicked(QListWidgetItem *)),
          this, SLOT(load(QListWidgetItem *)), Qt::UniqueConnection);
      for (int i = 0; i < m_filteredImages.size(); i++) {
        FileName filename = FileName(serialNumberList()->fileName(m_filteredImages[i]));
        QString tempFileName = filename.name();
        m_listBox->insertItem(i, tempFileName);
      }
      std::string msg = "Filter Count: " + QString::number(m_listBox->count()) +
          " / " + QString::number(serialNumberList()->size());
      m_filterCountLabel->setText(msg);
    }
  }



  /**
   * Tells the filetool to load an image, slot for "View Cube(s)" button
   *
   * @internal
   *   @history  2007-06-05 Tracie Sucharski - Use enumerators for the filter
   *                           indices.  Comment out overlap/polygon code
   *                           temporarily.
   *   @history  2008-11-19 Jeannie Walldren - Added Qt::WaitCursor (i.e. clock or
   *                           hourglass) to indicate that there is background
   *                           activity while this method is running
   *   @history  2008-11-26 Tracie Sucharski - Remove all polygon/overlap
   *                           references, this functionality will be qmos.
   *   @history 2010-06-03 Jeannie Walldren - Modified "No file selected" warning
   *                          to check whether selected list is empty rather than
   *                          the index of current row.
   */
  void QnetNavTool::load() {
    // Dont do anything if no cubes are loaded
    if (serialNumberList() == NULL)
      return;

    // TODO: How do we handle them opening 2 overlaps or points that have the
    //       cube in them, open 2 of the same file or only open 1???
    QList<QListWidgetItem *> selected = m_listBox->selectedItems();
    if (selected.size() < 1) {
      QMessageBox::information((QWidget *)parent(),
          "Error", "No file selected to load.");
      return;
    }

    QApplication::setOverrideCursor(Qt::WaitCursor);
    for (int i = 0; i < selected.size(); i++) {
      int index = m_listBox->row(selected[i]);
      // Tell the filetool to load all images for the given point
      if (m_listCombo->currentIndex() == Points) {
        if (m_filteredPoints.size() == 0) {
          emit loadPointImages((*controlNet())[index]);
        }
        else {
          emit loadPointImages((*controlNet())[m_filteredPoints[index]]);
        }
      }
      // Tell the filetool to load the given image
      else if (m_listCombo->currentIndex() == Cubes) {
        if (m_filteredImages.size() == 0) {
          QString serialNumber = (*serialNumberList()).serialNumber(index);
          QString sn = serialNumber;
          emit loadImage(sn);
        }
        else {
          QString serialNumber = (*serialNumberList()).serialNumber(m_filteredImages[index]);
          QString sn = serialNumber;
          emit loadImage(sn);
        }
      }
    }
    QApplication::restoreOverrideCursor();
    return;
  }




  /**
   * Slot for double-clicking cube list.  Needed this slot because the signal
   * has a QListWidgetItem parameter.  TODO:  Clean this up by possibly combining
   * the two different load slots???
   *
   * @author 2010-11-04 Tracie Sucharski
   *
   * @internal
   */
  void QnetNavTool::load(QListWidgetItem *) {
    load();
  }

  /**
   * Emits a modifyPoint signal
   *
   * @param ptItem
   */
  void QnetNavTool::editPoint(QListWidgetItem *ptItem) {

    int index = m_listBox->row(ptItem);
    if (m_filteredPoints.size() == 0) {
      emit modifyPoint((*controlNet())[index]);
    }
    else {
      emit modifyPoint((*controlNet())[m_filteredPoints[index]]);
    }

  }

  /**
   * Calls the qnet tool for the given control point.
   * @internal
   *   @history 2010-06-03 Jeannie Walldren - Modified "No file selected" warning
   *                          to check whether selected list is empty rather than
   *                          the index of current row.
   */
  void QnetNavTool::tie() {

    QList<QListWidgetItem *> selected = m_listBox->selectedItems();
    if (selected.size() > 1) {
      QMessageBox::information((QWidget *)parent(),
          "Error", "Only one point can be modified at a time");
      return;
    }
    else if (selected.size() < 1) {
      QMessageBox::information((QWidget *)parent(),
          "Error", "No point selected to modify.");
      return;
    }
    int index = m_listBox->row(selected[0]);
    if (m_filteredPoints.size() == 0) {
      emit modifyPoint((*controlNet())[index]);
    }
    else {
      emit modifyPoint((*controlNet())[m_filteredPoints[index]]);
    }
  }


  /**
   * Set Ignored=True for selected Points
   *
   * @author 2008-12-09 Tracie Sucharski
   * @internal
   *   @history  2008-12-29 Jeannie Walldren - Added question box to verify that
   *                           the user wants to set the selected points to
   *                           ignore=true.
   *
   */
  void QnetNavTool::ignorePoints() {
    // do nothing if no cubes are loaded
    if (serialNumberList() == NULL) return;

    int index = m_listBox->currentRow();
    if (index < 0) {
      QApplication::restoreOverrideCursor();
      QMessageBox::information((QWidget *)parent(),
          "Error", "No point selected to ignore");
      return;
    }
    QList<QListWidgetItem *> selected = m_listBox->selectedItems();
    switch (QMessageBox::question((QWidget *)parent(),
        "Control Network Navigator - Ignore Points",
        "You have chosen to set "
        + QString::number(selected.size())
        + " point(s) to ignore. Do you want to continue?",
        "&Yes", "&No", 0, 0)) {
      case 0: // Yes was clicked or Enter was pressed, delete points
        QApplication::setOverrideCursor(Qt::WaitCursor);
        int lockedPoints = 0;
        for (int i = 0; i < selected.size(); i++) {
          int index = m_listBox->row(selected[i]);
          if (m_filteredPoints.size() == 0) {
            if ((*controlNet())[index]->SetIgnored(true) ==
                ControlPoint::PointLocked) lockedPoints++;
          }
          else {
            if ((*controlNet())[m_filteredPoints[index]]->SetIgnored(true)
                == ControlPoint::PointLocked) lockedPoints++;
          }
          emit pointChanged((*controlNet())[m_filteredPoints[index]]->GetId());
        }
        //  Print info about locked points if there are any
        if (lockedPoints > 0) {
          QMessageBox::information((QWidget *)parent(),"EditLocked Points",
                QString::number(lockedPoints) + " / "
                + QString::number(selected.size())
                + " points are EditLocked and were not set to Ignored.");
        }
        QApplication::restoreOverrideCursor();
        if (lockedPoints != selected.size()) emit netChanged();

        break;
        //  case 1: // No was clicked, close window and do nothing to points
    }
    return;
  }


  /**
   * Delete selected Points from control network
   *
   * @author 2008-12-09 Tracie Sucharski
   * @internal
   *   @history 2008-12-29 Jeannie Walldren - Added question box to verify that
   *                          the user wants to delete the selected points.
   *   @history 2011-07-25 Tracie Sucharski - Fixed bug in refreshing list
   *                          changed to delete starting at end of list so
   *                          indices stay accurate.
   *   @history 2011-10-20 Tracie Sucharski - Fixed bug with filter list not
   *                          being updated correctly after deleting points.
   *                          The simple fix was to reset the list, then
   *                          re-filter.  For most filters this is probably
   *                          adequate.  However, for computationally
   *                          intensive filters, we might need a smarter
   *                          algorithm which would involve actually adjusting
   *                          the indices of the filtered list.
   */
  void QnetNavTool::deletePoints() {
    // do nothing if no cubes are loaded
    if (serialNumberList() == NULL)
      return;

    QList<QListWidgetItem *> selected = m_listBox->selectedItems();

    if (selected.size() < 1) {
      QApplication::restoreOverrideCursor();
      QMessageBox::information((QWidget *)parent(),
          "Error", "No point selected to delete");
      return;
    }
    switch (QMessageBox::question((QWidget *)parent(),
        "Control Network Navigator - Delete Points",
        "You have chosen to delete "
        + QString::number(selected.size())
        + " point(s). Do you want to continue?",
        "&Yes", "&No", 0, 0)) {
      case 0: // Yes was clicked or Enter was pressed, delete points


#if 0 //  If resetting filtered list, then re-filtering too slow, try code below

        // Keep track of rows to be deleted.  They cannot be deleted until
        // the end, or indices in list widget will be incorrect.
        vector<int> deletedRows;
        // Keep track of points deleted so far, to keep indices into network
        // (stored in g_filteredPoints) accurate, they will need to be adjusted
        // on the fly.
        int deletedSoFar = 0;

        for (int i = 0; i < g_filteredPoints.size(); i++) {
          if (m_listBox->item(i)->isSelected() {
            QString id = m_listBox->item(i)->text();
            if (g_controlNetwork->DeletePoint(id) == ControlPoint::PointLocked) {
              lockedPoints++;
            }
          }
        }
#endif
        int lockedPoints = 0;
        for (int i = 0; i < selected.size(); i++) {
          QString id = selected.at(i)->text();
          if (controlNet()->DeletePoint(id) == ControlPoint::PointLocked) {
            lockedPoints++;
          }
        }

        //  Print info about locked points if there are any
        if (lockedPoints > 0) {
          QMessageBox::information((QWidget *)parent(),"EditLocked Points",
                QString::number(lockedPoints) + " / "
                + QString::number(selected.size())
                + " points are EditLocked and were not deleted.");
        }

        //  Reset filter list, if this becomes too slow, we might need to
        //  implement smarter algorithm, which would require adjusting indices
        //  as points get deleted.  See commented out code above.
        m_filteredPoints.clear();
        // copy the control net indices into the filtered points list
        for (int i = 0; i < controlNet()->GetNumPoints(); i++) {
          m_filteredPoints.push_back(i);
        }
        emit deletedPoints();
        emit netChanged();
        break;
        //  case 1: // No was clicked, close window and do nothing to points
    }
    return;
  }


  /**
   * Bring up apriori dialog
   *
   * @author 2011-04-19 Tracie Sucharski
   *
   * @internal
   *   @history 2016-11-18 Makayla Shepherd - Added a connection to disconnectAprioriDialog() which
   *                           will disconnect the dialog and delete it.
   *
   * @todo  This method should be temporary until the control point editor
   *           comes online.  If this stick around, needs to be re-designed-
   *           put in a separate class??
   *
   */
  void QnetNavTool::aprioriDialog() {
    // If no cubes are loaded, simply return
    if (serialNumberList() == NULL)
      return;

    if (!m_aprioriDialog) {
      m_aprioriDialog = new QnetSetAprioriDialog(m_qnetTool);
      setAprioriDialogPoints();
      connect(m_listBox, SIGNAL(itemSelectionChanged()),
              this, SLOT(setAprioriDialogPoints()));
      connect(m_aprioriDialog, SIGNAL(pointChanged(QString)),
              this, SIGNAL(pointChanged(QString)));
      connect(m_aprioriDialog, SIGNAL(netChanged()),
              this, SIGNAL(netChanged()));
      connect(m_aprioriDialog, SIGNAL(aprioriDialogClosed()),
              this, SLOT(disconnectAprioriDialog()));
    }
    m_aprioriDialog->setVisiblity();
  }


  /**
   * Slot to pass points selected in Nav List Widget to Apriori Dialog
   *
   * @internal
   * @history 2011-05-04 Tracie Sucharski - Do not print error if no pts
   *                        selected, simply return.
   */
  void QnetNavTool::setAprioriDialogPoints() {

    if (m_aprioriDialog == NULL) return;

    //  If cubes selected, return
    if (m_listCombo->currentIndex() == Cubes) return;

    int index = m_listBox->currentRow();
    if (index < 0) return;

    QList<QListWidgetItem *> selected = m_listBox->selectedItems();
    m_aprioriDialog->setPoints(selected);

  }


  /**
   * Apriori dialog has been closed and needs to be disconnected and deleted so a new dialog can
   * be brought up next time.
   *
   * @author 2016-11-14 Makayla Shepherd
   *
   */
  void QnetNavTool::disconnectAprioriDialog() {
    if (m_aprioriDialog) {
      disconnect(m_listBox, SIGNAL(itemSelectionChanged()),
                 this, SLOT(setAprioriDialogPoints()));
      disconnect(m_aprioriDialog, 0, 0, 0);
      m_aprioriDialog = NULL;
    }
  }


  /**
   * Figures out what type of widget the filter was selected for and calls the
   * filter method for that filter class
   *
   * @internal
   *   @history  2007-06-05 Tracie Sucharski - Use enumerators for the filter
   *                           indices.  Comment out overlap/polygon code
   *                           temporarily.
   *   @history  2008-11-19 Jeannie Walldren - Added WaitCursor (i.e. clock or
   *                           hourglass) to indicate that there is background
   *                           activity while this method is running
   *   @history  2008-11-26 Tracie Sucharski - Remove all polygon/overlap
   *                           references, this functionality will be qmos.
   *   @history  2008-12-09 Tracie Sucharski - Added m_filtered indicating whether
   *                           the listBox contains filtered or unfiltered list.
   *   @history  2009-01-08 Jeannie Walldren - Removed command to clear filtered
   *                           points and images lists
   *   @history  2009-01-26 Jeannie Walldren - Added filter call for points cube
   *                           name filter.
   *
   */
  void QnetNavTool::filter() {

    m_filtered = true;
    QApplication::setOverrideCursor(Qt::WaitCursor);
    m_filter->setEnabled(false);

    QTabWidget *tab = (QTabWidget *)(m_filterStack->currentWidget());

    // We're dealing with points
    if (m_listCombo->currentIndex() == Points) {
      PointFilterIndex pointIndex = (PointFilterIndex) tab->currentIndex();
      // We have a jigsaw errors filter
      if (pointIndex == JigsawErrors) {
        QnetPointJigsawErrorFilter *widget =
          (QnetPointJigsawErrorFilter *)(tab->currentWidget());
        widget->filter();
      }
      // We have a registration errors filter
      if (pointIndex == RegistrationErrors) {
        QnetPointRegistrationErrorFilter *widget =
          (QnetPointRegistrationErrorFilter *)(tab->currentWidget());
        widget->filter();
      }
      // We have a point id filter
      else if (pointIndex == Id) {
        QnetPointIdFilter *widget =
          (QnetPointIdFilter *)(tab->currentWidget());
        widget->filter();
      }
      // We have a number of images filter
      else if (pointIndex == NumberImages) {
        QnetPointImagesFilter *widget =
          (QnetPointImagesFilter *)(tab->currentWidget());
        widget->filter();
      }
      // We have a point type filter
      else if (pointIndex == Type) {
        QnetPointTypeFilter *widget =
          (QnetPointTypeFilter *)(tab->currentWidget());
        widget->filter();
      }
      // We have a lat/lon range filter
      else if (pointIndex == LatLonRange) {
        QnetPointRangeFilter *widget =
          (QnetPointRangeFilter *)(tab->currentWidget());
        widget->filter();
      }
      // We have a distance filter
      else if (pointIndex == Distance) {
        QnetPointDistanceFilter *widget =
          (QnetPointDistanceFilter *)(tab->currentWidget());
        widget->filter();
      }
      // We have a measure filter
      else if (pointIndex == MeasureType) {
        QnetPointMeasureFilter *widget =
          (QnetPointMeasureFilter *)(tab->currentWidget());
        widget->filter();
      }
      // We have a goodness of fit filter
      else if(pointIndex == GoodnessOfFit) {
        QnetPointGoodnessFilter *widget =
          (QnetPointGoodnessFilter *)(tab->currentWidget());
        widget->filter();
      }
      // We have a cube name filter
      else if (pointIndex == CubeName) {
        QnetPointCubeNameFilter *widget =
          (QnetPointCubeNameFilter *)(tab->currentWidget());
        widget->filter();
      }
    }

    // We're dealing with cubes
    else if (m_listCombo->currentIndex() == Cubes) {
      CubeFilterIndex cubeIndex = (CubeFilterIndex) tab->currentIndex();
      // We have a cube name filter
      if (cubeIndex == Name) {
        QnetCubeNameFilter *widget =
          (QnetCubeNameFilter *)(tab->currentWidget());
        widget->filter();
      }
      // We have a point filter
      else if (cubeIndex == NumberPoints) {
        QnetCubePointsFilter *widget =
          (QnetCubePointsFilter *)(tab->currentWidget());
        widget->filter();
      }
      // We have a distance filter
      else if (cubeIndex == PointDistance) {
        QnetCubeDistanceFilter *widget =
          (QnetCubeDistanceFilter *)(tab->currentWidget());
        widget->filter();
      }
    }
    m_filter->setEnabled(true);
    QApplication::restoreOverrideCursor();
    return;
  }

  /**
   * Enable/disable buttons depending on whether Points or Cubes is chosen
   *
   * @internal
   *   @history  2008-12-09 Tracie Sucharski - Renamed from enableTie to
   *                           enableButtons.  Added ignore and delete buttons.
   */
  void QnetNavTool::enableButtons() {
    if (m_listCombo->currentIndex() == Points) {
      m_tie->setEnabled(true);
      m_multiIgnore->setEnabled(true);
      m_multiDelete->setEnabled(true);
      m_setApriori->setEnabled(true);
    }
    else {
      m_tie->setEnabled(false);
      m_multiIgnore->setEnabled(false);
      m_multiDelete->setEnabled(false);
      m_setApriori->setEnabled(false);
      if (m_aprioriDialog != NULL) {
        m_aprioriDialog->close();
      }
    }
  }

  /**
   * This slot is connected to the file tool in qnet.cpp.
   * It emits a signal that the serial list has been modified so
   * the points cube name filter knows to change the list box
   * displayed.
   * @see QnetPointCubeNameFilter
   * @internal
   *   @history 2009-01-26 Jeannie Walldren - Original version.
   */
  void QnetNavTool::resetCubeList() {
    emit serialListModified();
  }

  /**
   * This method sets the Navigation Dialog window to shown=true.
   *
   * @author Jeannie Walldren
   * @internal
   *   @history 2010-07-01 Jeannie Walldren - Original version.
   */
  void QnetNavTool::showNavTool() {
    m_navDialog->setVisible(true);
  }

#if 0
  /**
   * List selected points which are EditLocked and allow user to un-lock
   */
  void QnetNavTool::listLockedPoints() {

    QDialog lockDialog = new QDialog();
    lockDialog.setWindowTitle("Un-lock Points");
    lockDialog.setModal(true);

    QListWidget editLockPointsListBox = new QListWidget(lockDialog);
    connect(editLockPointsListBox, SIGNAL(itemChanged(QListWidgetItem *)),
            this, SLOT(unlockPoint(QListWidgetItem *)));

    QList<QListWidgetItem *> selected = m_listBox->selectedItems();
    QList<QListWidgetItem *> lockedPoints;

    // Fill editLock List Box
    for (int i=0; i<selected.size(); i++) {
      QString id = selected.at(i)->text();
      ControlPoint *pt = g_controlNetwork->GetPoint(id);
      if (pt->IsEditLocked()) {
        QListWidgetItem *item = new QListWidgetItem(*(selected[i]));
        item->setCheckState(Qt::Checked);
        editLockPointsListBox->addItem(item);
      }
    }

    if (lockDialog.exec()) {

    }



    switch (QMessageBox::question((QWidget *)parent(),
        "Control Network Navigator - Ignore Points",
        "You have chosen to set "
        + QString::number(selected.size())
        + " point(s) to ignore. Do you want to continue?",
        "&Yes", "&No", 0, 0)) {
      case 0: // Yes was clicked or Enter was pressed, delete points
        QApplication::setOverrideCursor(Qt::WaitCursor);
          emit pointChanged((*g_controlNetwork)[g_filteredPoints[index]]->GetId());
        }


  }

  void QnetNavTool::unlockPoint(QListWidgetItem *pointId) {
    ControlPoint *pt = g_controlNetwork->GetPoint(pointId->text());
    if (pt->IsEditLocked() && pointId->checkState() == Qt::Unchecked) {
      pt->SetEditLock(false);
      editLockPointsListBox->removeItemWidget(pointId);
      pointId->setHidden(true);
      editLockPointsListBox->repaint();
      this->repaint();
      emit netChanged();
    }

  }
#endif
}
