#include "MeasureTool.h"

#include <QApplication>
#include <QCheckBox>
#include <QFileDialog>
#include <QHBoxLayout>
#include <QLineEdit>
#include <QMenu>
#include <QMenuBar>
#include <QMessageBox>
#include <QStackedWidget>
#include <QStatusBar>
#include <QTableWidget>
#include <QToolButton>

#include <geos/geom/Geometry.h>
#include <geos/geom/Point.h>

#include "Camera.h"
#include "Distance.h"
#include "FileName.h"
#include "Latitude.h"
#include "Longitude.h"
#include "MdiCubeViewport.h"
#include "Projection.h"
#include "RingPlaneProjection.h"
#include "TProjection.h"
#include "SurfacePoint.h"
#include "ToolPad.h"
#include "Constants.h"

namespace Isis {
  /**
   * MeasureTool constructor
   *
   *
   * @param parent
   */
  MeasureTool::MeasureTool(QWidget *parent) : Tool(parent) {
    m_rubberBand = NULL;
    m_tableWin = new TableMainWindow("Measurements", parent);
    m_tableWin->setTrackListItems(true);

    // Create the action for showing the table
    m_action = new QAction(parent);
    m_action->setText("Measuring ...");

    connect(m_action, SIGNAL(triggered()), m_tableWin, SLOT(showTable()));
    connect(m_action, SIGNAL(triggered()), m_tableWin, SLOT(raise()));
    connect(m_action, SIGNAL(triggered()), m_tableWin, SLOT(syncColumns()));
    //m_tableWin->installEventFilter(this);

    m_tableWin->addToTable(false, "Feature\nName", "Feature Name");
    m_tableWin->addToTable(false, "Feature\nType", "Feature Type");
    m_tableWin->addToTable(true,
                           "Start\nLatitude:Start\nLongitude:End\nLatitude:End\nLongitude",
                           "Ground Range", -1, Qt::Horizontal, "Start Latitude/Longitude to End Latitude/Longitude");
    m_tableWin->addToTable(false, "Start\nSample:Start\nLine:End\nSample:End\nLine",
                           "Pixel Range", -1, Qt::Horizontal, "Start Sample/Line to End Sample/Line");
    m_tableWin->addToTable(true, "Kilometer\nDistance", "Kilometer Distance");
    m_tableWin->addToTable(false, "Meter\nDistance", "Meter Distance");
    m_tableWin->addToTable(false, "Pixel\nDistance", "Pixel Distance");
    m_tableWin->addToTable(false, "Degree\nAngle", "Degree Angle");
    m_tableWin->addToTable(false, "Radian\nAngle", "Radian Angle");
    m_tableWin->addToTable(false, "Kilometer\nArea", "Kilometer Area");
    m_tableWin->addToTable(false, "Meter\nArea", "Meter Area");
    m_tableWin->addToTable(false, "Pixel\nArea", "Pixel Area");
    m_tableWin->addToTable(false, "Planar \nDistance", "Planar Kilometer Distance");
    m_tableWin->addToTable(false, "Segments Sum\nkm", "Segments Sum", -1, Qt::Horizontal, "Sum of Segment lengths in kilometers");
    m_tableWin->addToTable(false, "Segment Number", "Segment Number", -1, Qt::Horizontal, "Segment number of a segmented line");
    m_tableWin->addToTable(false, "Path", "Path");
    m_tableWin->addToTable(false, "FileName", "FileName");
    m_tableWin->addToTable(false, "Notes", "Notes");

    m_tableWin->setStatusMessage("Click, Drag, and Release to Measure a Line");

    addRow();
  }


  /**
   * Add the measure tool action to the toolpad.
   *
   *
   * @param toolpad
   *
   * @return QAction*
   */
  QAction *MeasureTool::toolPadAction(ToolPad *toolpad) {
    QAction *action = new QAction(toolpad);
    action->setIcon(QPixmap(toolIconDir() + "/measure.png"));
    action->setToolTip("Measure (M)");
    action->setShortcut(Qt::Key_M);

    QString text  =
      "<b>Function:</b>  Measure features in active viewport \
      <p><b>Shortcut:</b> M</p> ";
    action->setWhatsThis(text);

    return action;
  }


  /**
   * Creates the widget (button) that goes on the tool bar
   *
   *
   * @param parent
   *
   * @return QWidget*
   */
  QWidget *MeasureTool::createToolBarWidget(QStackedWidget *parent) {
    QWidget *hbox = new QWidget(parent);
    QToolButton *measureButton = new QToolButton(hbox);
    measureButton->setText("Table");
    measureButton->setToolTip("Record Measurement Data in Table");
    QString text =
      "<b>Function:</b> This button will bring up a table that will record the \
     starting and ending points of the line, along with the distance between \
     the two points on the image.  To measure the distance between two points, \
      click on the first point and releasing the mouse at the second point. \
      <p><b>Shortcut:</b>  CTRL+M</p>";
    measureButton->setWhatsThis(text);
    measureButton->setShortcut(Qt::CTRL + Qt::Key_M);
    connect(measureButton, SIGNAL(clicked()), m_tableWin, SLOT(showTable()));
    connect(measureButton, SIGNAL(clicked()), m_tableWin, SLOT(syncColumns()));
    connect(measureButton, SIGNAL(clicked()), m_tableWin, SLOT(raise()));
    measureButton->setEnabled(true);

    m_rubberBand = new RubberBandComboBox(this,
      RubberBandComboBox::Angle |
      RubberBandComboBox::Circle |
      RubberBandComboBox::Ellipse |
      RubberBandComboBox::Line |
      RubberBandComboBox::Rectangle |
      RubberBandComboBox::RotatedRectangle |
      RubberBandComboBox::Polygon |
      RubberBandComboBox::SegmentedLine, // options
      RubberBandComboBox::Line // default
                                                
    );

    m_distLineEdit = new QLineEdit(hbox);
    m_distLineEdit->setText("");
    m_distLineEdit->setMaxLength(12);
    m_distLineEdit->setToolTip("Line Length");
    QString text2 = "<b>Function: </b> Shows the length of the line drawn on \
                     the image.";
    m_distLineEdit->setWhatsThis(text2);
    m_distLineEdit->setReadOnly(true);

    m_showAllSegments = new QCheckBox(hbox);
    m_showAllSegments->setText("Show All Segments");

    m_unitsComboBox = new QComboBox(hbox);
    m_unitsComboBox->setSizeAdjustPolicy(QComboBox::AdjustToContents);

    miComboUnit = -1;

    updateUnitsCombo();
    connect(m_unitsComboBox, SIGNAL(activated(int)), this, SLOT(updateDistEdit()));
    connect(rubberBandTool(), SIGNAL(modeChanged()), this, SLOT(updateUnitsCombo()));

    QHBoxLayout *layout = new QHBoxLayout(hbox);
    layout->setMargin(0);
    layout->addWidget(m_rubberBand);
    layout->addWidget(m_distLineEdit);
    layout->addWidget(m_unitsComboBox);
    layout->addWidget(measureButton);
    layout->addWidget(m_showAllSegments);
    layout->addStretch(1);
    hbox->setLayout(layout);
    return hbox;
  }


  /**
   * Updates the units combo box.
   *
   */
  void MeasureTool::updateUnitsCombo(void) {
    // get the previous index if not initializing
    if (miComboUnit >= 0) {
      miComboUnit = m_unitsComboBox->currentIndex();
    }

    m_unitsComboBox->clear();
    m_showAllSegments->setEnabled(false);

    if (rubberBandTool()->currentMode() == RubberBandTool::LineMode) {
      m_unitsComboBox->addItem("km");
      m_unitsComboBox->addItem("m");
      m_unitsComboBox->addItem("pixels");
      m_unitsComboBox->addItem("planar km");

      if (miComboUnit < 0 || miComboUnit > 3) {   // default && error checking
        miComboUnit = 2;
      }
    }
    else if (rubberBandTool()->currentMode() == RubberBandTool::SegmentedLineMode) {

      if (rubberBandTool()->currentMode() == RubberBandTool::SegmentedLineMode) {
        m_showAllSegments->setEnabled(true);
      }

      m_unitsComboBox->addItem("km");
      m_unitsComboBox->addItem("m");
      m_unitsComboBox->addItem("pixels");
      if (miComboUnit < 0 || miComboUnit > 2) {   // default && error checking
        miComboUnit = 2;
      }
    }
    else if (rubberBandTool()->currentMode() == RubberBandTool::AngleMode) {
      m_unitsComboBox->addItem("degrees");
      m_unitsComboBox->addItem("radians");
      if (miComboUnit > 1 || miComboUnit < 0) {   // default && error checking
        miComboUnit = 0;
      }
    }
    else {
      m_unitsComboBox->addItem("km^2");
      m_unitsComboBox->addItem("m^2");
      m_unitsComboBox->addItem("pix^2");
      if (miComboUnit < 0 || miComboUnit > 2) {   // default && error checking
        miComboUnit = 2;
      }
    }

    m_unitsComboBox->setCurrentIndex(miComboUnit);
  }


  /**
   * Adds the measure action to the given menu.
   *
   *
   * @param menu
   */
  void MeasureTool::addTo(QMenu *menu) {
    menu->addAction(m_action);
  }


  /**
   * Updates the Measure specifications.
   *
   */
  void MeasureTool::updateMeasure() {
    MdiCubeViewport *cvp = cubeViewport();
    MdiCubeViewport *d;
    m_numLinked = 0;

    int currentRow = m_tableWin->currentRow();

    while (currentRow >= m_tableWin->table()->rowCount()) {
      addRow();
    }

    if (cvp == NULL) {
      m_tableWin->clearRow(m_tableWin->currentRow());
    }
    else {
      updateDist(cvp, currentRow);
      m_tableWin->table()->selectRow(currentRow);

      if (cvp->isLinked()) {
        for (int i = 0; i < (int)cubeViewportList()->size(); i++) {
          d = (*(cubeViewportList()))[i];

          if (d->isLinked() && d != cvp) {
            m_numLinked++;

            if (currentRow + m_numLinked >= m_tableWin->table()->rowCount()) {
              addRow();
            }

            updateDist(d, currentRow + m_numLinked);
          }
        }
      }
    }
  }


  /**
   * Called when the rubberBanding by the user is finished.
   *
   */
  void MeasureTool::rubberBandComplete() {
    updateMeasure();

    if (rubberBandTool()->currentMode() != RubberBandTool::AngleMode && m_unitsComboBox->currentIndex() != 2) {
      if (cubeViewport()->camera() == NULL && cubeViewport()->projection() == NULL) {
        QMessageBox::information((QWidget *)parent(), "Error",
                                 "File must have a Camera Model or Projection to measure in km or m");
        return;
      }
    }

    if (!m_tableWin->table()->isVisible()) return;
    if (m_tableWin->table()->item(m_tableWin->currentRow(), StartLineIndex)->text() == "N/A" &&
        m_tableWin->table()->item(m_tableWin->currentRow(), AngleDegIndex)->text() == "N/A" &&
        m_tableWin->table()->item(m_tableWin->currentRow(), AreaPixIndex)->text() == "N/A") return;

    addRow();
    m_tableWin->setCurrentRow(m_tableWin->table()->rowCount() - 1);

    QApplication::sendPostedEvents(m_tableWin->table(), 0);
  }


  /**
   * Mouse leave event.
   *
   */
  void MeasureTool::mouseLeave() {
    //m_tableWin->clearRow(m_tableWin->currentRow());
  }


  /**
   * Enables/resets the rubberband tool.
   *
   */
  void MeasureTool::enableRubberBandTool() {
    if (m_rubberBand) {
      m_rubberBand->reset();
      rubberBandTool()->setDrawActiveViewportOnly(false);
    }
  }


  /**
   * This method updates the row in the table window with the
   * current measure information.
   *
   *
   * @param row
   */
  void MeasureTool::updateRow(int row) {

    if (row >= m_tableWin->table()->rowCount()) {
      return;
    }

    // Blank out the row to remove stuff left over from previous cvps
    for (int c = 0; c < m_tableWin->table()->columnCount(); c++) {
      m_tableWin->table()->item(row, c)->setText("");
    }

    // Write all the new info to the current row
    if (m_startLat != Null && m_startLon != Null) {
      m_tableWin->table()->item(row, StartLatIndex)->setText(QString::number(m_startLat));
      m_tableWin->table()->item(row, StartLonIndex)->setText(QString::number(m_startLon));
    }
    else {
      m_tableWin->table()->item(row, StartLatIndex)->setText("N/A");
      m_tableWin->table()->item(row, StartLonIndex)->setText("N/A");
    }

    if (m_endLat != Null && m_endLon != Null) {
      m_tableWin->table()->item(row, EndLatIndex)->setText(QString::number(m_endLat));
      m_tableWin->table()->item(row, EndLonIndex)->setText(QString::number(m_endLon));
    }
    else {
      m_tableWin->table()->item(row, EndLatIndex)->setText("N/A");
      m_tableWin->table()->item(row, EndLonIndex)->setText("N/A");
    }

    if (m_mDist != Null && m_kmDist != Null) {
      m_tableWin->table()->item(row, DistanceMIndex)->setText(QString::number(m_mDist));
      m_tableWin->table()->item(row, DistanceKmIndex)->setText(QString::number(m_kmDist));
    }
    else {
      m_tableWin->table()->item(row, DistanceKmIndex)->setText("N/A");
      m_tableWin->table()->item(row, DistanceMIndex)->setText("N/A");
    }

    if (m_degAngle != Null && m_radAngle != Null) {
      m_tableWin->table()->item(row, AngleDegIndex)->setText(QString::number(m_degAngle));
      m_tableWin->table()->item(row, AngleRadIndex)->setText(QString::number(m_radAngle));
    }
    else {
      m_tableWin->table()->item(row, AngleDegIndex)->setText("N/A");
      m_tableWin->table()->item(row, AngleRadIndex)->setText("N/A");
    }

    if (m_startSamp != Null && m_startLine != Null) {
      m_tableWin->table()->item(row, StartSampIndex)->setText(QString::number(m_startSamp));
      m_tableWin->table()->item(row, StartLineIndex)->setText(QString::number(m_startLine));
    }
    else {
      m_tableWin->table()->item(row, StartSampIndex)->setText("N/A");
      m_tableWin->table()->item(row, StartLineIndex)->setText("N/A");
    }

    if (m_endSamp != Null && m_endLine != Null) {
      m_tableWin->table()->item(row, EndSampIndex)->setText(QString::number(m_endSamp));
      m_tableWin->table()->item(row, EndLineIndex)->setText(QString::number(m_endLine));
      m_tableWin->table()->item(row, DistancePixIndex)->setText(QString::number(m_pixDist));
    }
    else {
      m_tableWin->table()->item(row, EndSampIndex)->setText("N/A");
      m_tableWin->table()->item(row, EndLineIndex)->setText("N/A");
      m_tableWin->table()->item(row, DistancePixIndex)->setText("N/A");
    }

    if (m_pixArea != Null) {
      m_tableWin->table()->item(row, AreaPixIndex)->setText(QString::number(m_pixArea));
    }
    else {
      m_tableWin->table()->item(row, AreaPixIndex)->setText("N/A");
    }

    if (m_mArea != Null) {
      m_tableWin->table()->item(row, AreaKmIndex)->setText(QString::number(m_kmArea));
      m_tableWin->table()->item(row, AreaMIndex)->setText(QString::number(m_mArea));
    }
    else {
      m_tableWin->table()->item(row, AreaKmIndex)->setText("N/A");
      m_tableWin->table()->item(row, AreaMIndex)->setText("N/A");
    }

    if (m_kmPlanarDist != Null) {
      m_tableWin->table()->item(row, PlanarDistanceIndex)->setText(QString::number(m_kmPlanarDist));
    }
    else {
      m_tableWin->table()->item(row, PlanarDistanceIndex)->setText("N/A");
    }

    m_tableWin->table()->item(row, PathIndex)->setText(m_path);
    m_tableWin->table()->item(row, FileNameIndex)->setText(m_fname);
  }



  /**
   * This method is called instead of updateRows if the
   * 'Show All Segment' checkbox is checked.
   *
   *
   * @param row
   */
  void MeasureTool::updateRows(int row) {
    int requiredRows = m_distanceSegments.size() + row;
    int rowDiff = (int)(requiredRows - m_tableWin->table()->rowCount());

    //Make sure we have all the necessary rows and items in each table cell.
    if (requiredRows > m_tableWin->table()->rowCount()) {
      for (int r = 0; r < rowDiff; r++) {
        addRow();
      }
    }

    if (rubberBandTool()->currentMode() == RubberBandTool::SegmentedLineMode &&
        m_distanceSegments.size() > 0) {
      double distanceSum = 0;
      for (int i = 0; i < m_distanceSegments.size(); i++) {
        //write a new row for each segment...
        if (m_startLatSegments[i] != Null && m_startLonSegments[i] != Null) {
          m_tableWin->table()->item(row + i, StartLatIndex)->setText(QString::number(m_startLatSegments[i]));
          m_tableWin->table()->item(row + i, StartLonIndex)->setText(QString::number(m_startLonSegments[i]));
        }
        else {
          m_tableWin->table()->item(row + i, StartLatIndex)->setText("N/A");
          m_tableWin->table()->item(row + i, StartLonIndex)->setText("N/A");
        }

        if (m_endLatSegments[i] != Null && m_endLonSegments[i] != Null) {
          m_tableWin->table()->item(row + i, EndLatIndex)->setText(QString::number(m_endLatSegments[i]));
          m_tableWin->table()->item(row + i, EndLonIndex)->setText(QString::number(m_endLonSegments[i]));
        }
        else {
          m_tableWin->table()->item(row + i, EndLatIndex)->setText("N/A");
          m_tableWin->table()->item(row + i, EndLonIndex)->setText("N/A");
        }

        if (m_startSampSegments[i] != Null && m_startLineSegments[i] != Null) {
          m_tableWin->table()->item(row + i, StartSampIndex)->setText(QString::number(m_startSampSegments[i]));
          m_tableWin->table()->item(row + i, StartLineIndex)->setText(QString::number(m_startLineSegments[i]));
        }
        else {
          m_tableWin->table()->item(row + i, StartSampIndex)->setText("N/A");
          m_tableWin->table()->item(row + i, StartLineIndex)->setText("N/A");
        }

        if (m_endSampSegments[i] != Null && m_endLineSegments[i] != Null) {
          m_tableWin->table()->item(row + i, EndSampIndex)->setText(QString::number(m_endSampSegments[i]));
          m_tableWin->table()->item(row + i, EndLineIndex)->setText(QString::number(m_endLineSegments[i]));
        }
        else {
          m_tableWin->table()->item(row + i, EndSampIndex)->setText("N/A");
          m_tableWin->table()->item(row + i, EndLineIndex)->setText("N/A");
        }

        if (m_pixDistSegments[i] != Null) {
          m_tableWin->table()->item(row + i, DistancePixIndex)->setText(QString::number(m_pixDistSegments[i]));
        }
        else {
          m_tableWin->table()->item(row + i, DistancePixIndex)->setText("N/A");
        }

        if (m_distanceSegments[i] != Null) {
          m_tableWin->table()->item(row + i, DistanceKmIndex)->setText(QString::number(m_distanceSegments[i]));
          m_tableWin->table()->item(row + i, DistanceMIndex)->setText(QString::number(m_distanceSegments[i] * 1000.0));
        }
        else {
          m_tableWin->table()->item(row + i, DistanceKmIndex)->setText("N/A");
          m_tableWin->table()->item(row + i, DistanceMIndex)->setText("N/A");
        }

        m_tableWin->table()->item(row + i, PathIndex)->setText(m_path);
        m_tableWin->table()->item(row + i, FileNameIndex)->setText(m_fname);

        distanceSum = (Distance(distanceSum, Distance::Kilometers) +
            Distance(m_distanceSegments[i], Distance::Kilometers)).kilometers();

        if (distanceSum != Null) {
          m_tableWin->table()->item(row + i, SegmentsSumIndex)->setText(QString::number(distanceSum));
        }
        else {
          m_tableWin->table()->item(row + i, SegmentsSumIndex)->setText("N/A");
        }

        m_tableWin->table()->item(row + i, SegmentNumberIndex)->setText(QString::number(i + 1));
      }
    }
  }

  /**
   * Initialize Class data
   *
   * @author sprasad (10/23/2009)
   */
  void MeasureTool::initData(void) {
    // Initialize the class data
    m_startSamp    = Null;
    m_endSamp      = Null;
    m_startLine    = Null;
    m_endLine      = Null;
    m_kmDist       = Null;
    m_mDist        = Null;
    m_pixDist      = Null;
    m_startLon     = Null;
    m_startLat     = Null;
    m_endLon       = Null;
    m_endLat       = Null;
    m_radAngle     = Null;
    m_degAngle     = Null;
    m_pixArea      = Null;
    m_kmArea       = Null;
    m_mArea        = Null;
    m_kmPlanarDist = Null;
  }


  void MeasureTool::addRow() {
    int newRowPos = m_tableWin->table()->rowCount();
    m_tableWin->table()->insertRow(newRowPos);
    for (int c = 0; c < m_tableWin->table()->columnCount(); c++) {
      QTableWidgetItem *item = new QTableWidgetItem("");
      m_tableWin->table()->setItem(newRowPos, c, item);
    }
    m_tableWin->table()->scrollToItem(m_tableWin->table()->item(newRowPos, 0),
                                      QAbstractItemView::PositionAtBottom);
  }


  /**
   * This method updates the distance variables
   *
   *
   * @param cvp - Pointer to CubeViewPort
   * @param row - row index
   *
   * @returns void
   */
  void MeasureTool::updateDist(MdiCubeViewport *cvp, int row) {
    // Initialize class data
    initData();

    // Write out col 8 (the file name)
    Isis::FileName fname = Isis::FileName(
        cvp->cube()->fileName().toStdString()).expanded();
    m_path  = QString::fromStdString(fname.path());
    m_fname = QString::fromStdString(fname.name());

    m_pixDist = Null;
    m_mDist   = Null;
    m_kmDist  = Null;

    // reset the distnace gui
    m_distLineEdit->setText("");

    if (rubberBandTool()->currentMode() == RubberBandTool::LineMode ||
        rubberBandTool()->currentMode() == RubberBandTool::SegmentedLineMode) {
      m_distanceSegments.clear();
      m_pixDistSegments.clear();
      m_startSampSegments.clear();
      m_endSampSegments.clear();
      m_startLineSegments.clear();
      m_endLineSegments.clear();
      m_startLatSegments.clear();
      m_endLatSegments.clear();
      m_startLonSegments.clear();
      m_endLonSegments.clear();

      for (int startIndex = 0; startIndex < rubberBandTool()->vertices().size() - 1; startIndex++) {
        QPoint start = rubberBandTool()->vertices()[startIndex];
        QPoint end   = rubberBandTool()->vertices()[startIndex + 1];

        setDistances(cvp, start, end);

        if (rubberBandTool()->currentMode() == RubberBandTool::SegmentedLineMode) {
          if (m_distanceSegments.size() < 75) {
            m_distanceSegments.append(m_kmDist);
            m_pixDistSegments.append(m_pixDist);
            m_startSampSegments.append(m_startSamp);
            m_endSampSegments.append(m_endSamp);
            m_startLineSegments.append(m_startLine);
            m_endLineSegments.append(m_endLine);
            m_startLatSegments.append(m_startLat);
            m_endLatSegments.append(m_endLat);
            m_startLonSegments.append(m_startLon);
            m_endLonSegments.append(m_endLon);
          }
        }
      }

      if (rubberBandTool()->currentMode() == RubberBandTool::SegmentedLineMode &&
          m_pixDistSegments.size()) {
        m_pixDist = m_pixDistSegments[0];
        m_kmDist = m_distanceSegments[0];
        m_mDist = Distance(m_kmDist, Distance::Kilometers).meters();

        for (int i = 1; i < m_pixDistSegments.size(); i++) {
          m_pixDist = (Distance(m_pixDist, Distance::Pixels) +
              Distance(m_pixDistSegments[i], Distance::Pixels)).pixels();

          Distance thisDistance(m_distanceSegments[i], Distance::Kilometers);
          m_kmDist = (Distance(m_kmDist, Distance::Kilometers) +
                      thisDistance).kilometers();
          m_mDist = (Distance(m_mDist, Distance::Meters) +
                      thisDistance).meters();
        }
      }
    }
    else if (rubberBandTool()->currentMode() == RubberBandTool::AngleMode) {
      m_radAngle = rubberBandTool()->angle().radians();
      m_degAngle = rubberBandTool()->angle().degrees();
    }
    else {
      geos::geom::Geometry *polygon = rubberBandTool()->geometry();
      if (polygon != NULL) {
        // pix area = screenpix^2 / scale^2
        m_pixArea = polygon->getArea() / pow(cvp->scale(), 2);
        geos::geom::Point *center = polygon->getCentroid().release();
        double line, sample;
        cvp->viewportToCube((int)center->getX(), (int)center->getY(), sample, line);

        if (cvp->camera() != NULL) {
          cvp->camera()->SetImage(sample, line);
          // pix^2 * (m/pix)^2 = m^2
          m_mArea = m_pixArea * pow(cvp->camera()->PixelResolution(), 2);
          // m^2 * (km/m)^2 = km^2
          m_kmArea = m_mArea * pow(1 / 1000.0, 2);
        }

        if (cvp->projection() != NULL) {
          cvp->projection()->SetWorld(sample, line);
          // pix^2 * (m/pix)^2 = m^2
          m_mArea = m_pixArea * pow(cvp->projection()->Resolution(), 2);
          // m^2 * (km/m)^2 = km^2
          m_kmArea = m_mArea * pow(1 / 1000.0, 2);
        }
      }

      if (rubberBandTool()->currentMode() == RubberBandTool::RectangleMode) {
        setDistances(cvp, rubberBandTool()->vertices()[0],
                     rubberBandTool()->vertices()[2]);
      }
    }

    updateDistEdit();

    if (m_showAllSegments->isChecked() &&
        rubberBandTool()->currentMode() == RubberBandTool::SegmentedLineMode) {
      updateRows(row);
    }
    else {
      updateRow(row);
    }
  }


  void MeasureTool::setDistances(MdiCubeViewport *cvp, QPoint lineStart,
                                 QPoint lineEnd) {
    // Convert rubber band line to cube coordinates
    cvp->viewportToCube(lineStart.x(), lineStart.y(), m_startSamp, m_startLine);
    cvp->viewportToCube(lineEnd.x(), lineEnd.y(), m_endSamp, m_endLine);

    m_mDist   = Null;
    m_kmDist  = Null;
    m_kmPlanarDist = Null;
    double radius = Null;
    TProjection *tproj = NULL;
    RingPlaneProjection *rproj = NULL;
    Projection::ProjectionType projType = Projection::Triaxial;

    // Get set for dealing with projection types
    if (cvp->projection() != NULL)  projType = cvp->projection()->projectionType();

    // Don't write anything if we are outside the cube
    if ((m_startSamp >= 0.5) && (m_endSamp >= 0.5) &&
        (m_startLine >= 0.5) && (m_endLine >= 0.5) &&
        (m_startSamp <= cvp->cubeSamples() + 0.5) &&
        (m_endSamp <= cvp->cubeSamples() + 0.5) &&
        (m_startLine <= cvp->cubeLines() + 0.5) &&
        (m_endLine <= cvp->cubeLines() + 0.5)) {
      // Check if the image is projected (Projected Images also have camera
      //   except for mosaics)
      if (cvp->projection() != NULL) {
        if (cvp->projection()->SetWorld(m_startSamp, m_startLine)) {
          // If our projection is sky, the lat & lons are switched
          if (cvp->projection()->IsSky()) {

            tproj = (TProjection *) cvp->projection();
            m_startLat = tproj->UniversalLatitude();
            m_startLon = tproj->UniversalLongitude();
          }
          else if (projType == Projection::Triaxial) {
            tproj = (TProjection *) cvp->projection();
            m_startLat = tproj->UniversalLatitude();
            m_startLon = tproj->UniversalLongitude();
          }
          else { // RingPlaneProjection
            rproj = (RingPlaneProjection *) cvp->projection();
            m_startLat = rproj->UniversalRingRadius();
            m_startLon = rproj->UniversalRingLongitude();
          }

          if (cvp->projection()->SetWorld(m_endSamp, m_endLine)) {
            // If our projection is sky, the lat & lons are switched
            if (cvp->projection()->IsSky()) {
              m_endLat = tproj->UniversalLatitude();
              m_endLon = tproj->UniversalLongitude();
            }
            else if (projType == Projection::Triaxial) {
              m_endLat = tproj->UniversalLatitude();
              m_endLon = tproj->UniversalLongitude();
            } // RingPlaneProjection
            else {
              m_endLat = rproj->UniversalRingRadius();
              m_endLon = rproj->UniversalRingLongitude();
            }
          }

          // Calculate and write out the distance between the two points
          if (projType != Projection::RingPlane) {
            radius = tproj->LocalRadius();
          }
          else {
            radius = rproj->RingRadius();
          }
        }
      }
      // Do we have a camera model?
      else if (cvp->camera() != NULL &&
              cvp->camera()->SetImage(m_startSamp, m_startLine)) {
        // Write columns 2-3 (Start lat/lon)
        m_startLat = cvp->camera()->UniversalLatitude();
        m_startLon = cvp->camera()->UniversalLongitude();

        if (cvp->camera()->SetImage(m_endSamp, m_endLine)) {
          // Write columns 4-5 (End lat/lon)
          m_endLat = cvp->camera()->UniversalLatitude();
          m_endLon = cvp->camera()->UniversalLongitude();

          radius = cvp->camera()->LocalRadius().meters();
        }
      }
    }

    // Calculate the pixel difference
    double lineDif = m_startLine - m_endLine;
    double sampDif = m_startSamp - m_endSamp;
    double pixDist = sqrt(lineDif * lineDif + sampDif * sampDif);
    m_pixDist = pixDist;

    Latitude startLat(m_startLat, Angle::Degrees);
    Longitude startLon(m_startLon, Angle::Degrees);
    Latitude endLat(m_endLat, Angle::Degrees);
    Longitude endLon(m_endLon, Angle::Degrees);
    Distance radiusDist(radius, Distance::Meters);

    SurfacePoint startPoint;
    SurfacePoint endPoint;

    if (startLat.isValid() && startLon.isValid() &&
        endLat.isValid() && endLon.isValid() && radiusDist.isValid()) {
      startPoint = SurfacePoint(startLat, startLon, radiusDist);
      endPoint = SurfacePoint(endLat, endLon, radiusDist);
    }

    Distance distance = startPoint.GetDistanceToPoint(endPoint, radiusDist);

    m_mDist = distance.meters();
    m_kmDist = distance.kilometers();

    if (cvp->camera() != NULL) {

      // Make sure start line or end line setimage succeeds, otherwise fail. 
      bool statusStart = cvp->camera()->SetImage(m_startSamp, m_startLine);
      double slantDist = 0;
      if (statusStart) {
        slantDist = cvp->camera()->SlantDistance();
      }

      double ra1 = cvp->camera()->RightAscension() * DEG2RAD;
      double dec1 = cvp->camera()->Declination()* DEG2RAD; 

      bool statusEnd = cvp->camera()->SetImage(m_endSamp, m_endLine);
      if ((!statusStart)&&statusEnd) {
        slantDist = cvp->camera()->SlantDistance();
      }
      
      // Cannot calculate a planar distance with no point on the target.       
      if (!(statusStart||statusEnd)) {
        return;
      }

      double ra2 = cvp->camera()->RightAscension() * DEG2RAD; 
      double dec2 = cvp->camera()->Declination()* DEG2RAD; 

      double dRA = (ra1 - ra2);

      double angle = acos(sin(dec1)*sin(dec2) + cos(dec1)*cos(dec2)*cos(dRA));
      double half_angle = angle/2.0;
      double length = slantDist * sin(half_angle) * 2.0;

      m_kmPlanarDist = length;
    }
  }


  //! Change the value in the distance edit to match the units
  void MeasureTool::updateDistEdit() {
    if (rubberBandTool()->currentMode() == RubberBandTool::LineMode ||
        rubberBandTool()->currentMode() == RubberBandTool::SegmentedLineMode) {
      if (m_unitsComboBox->currentIndex() == 0) {
        if (m_kmDist == Null) {
          m_distLineEdit->setText("N/A");
        }
        else {
          m_distLineEdit->setText(QString::number(m_kmDist));
        }
      }
      else if (m_unitsComboBox->currentIndex() == 1) {
        if (m_mDist == Null) {
          m_distLineEdit->setText("N/A");
        }
        else {
          m_distLineEdit->setText(QString::number(m_mDist));
        }
      }
      else if (m_unitsComboBox->currentIndex() == 3) {
        if (m_kmPlanarDist == Null) {
          m_distLineEdit->setText("N/A");
        }
        else {
          m_distLineEdit->setText(QString::number(m_kmPlanarDist));
        } 
      }
      else {
        m_distLineEdit->setText(QString::number(m_pixDist));
      }
    }
    else if (rubberBandTool()->currentMode() == RubberBandTool::AngleMode) {
      if (m_unitsComboBox->currentIndex() == 0) {
        m_distLineEdit->setText(QString::number(m_degAngle));
      }
      else {
        m_distLineEdit->setText(QString::number(m_radAngle));
      }
    }
    else {
      if (m_unitsComboBox->currentIndex() == 0) {
        if (m_kmArea == Null) {
          m_distLineEdit->setText("N/A");
        }
        else {
          m_distLineEdit->setText(QString::number(m_kmArea));
        }
      }
      else if (m_unitsComboBox->currentIndex() == 1) {
        if (m_mArea == Null) {
          m_distLineEdit->setText("N/A");
        }
        else {
          m_distLineEdit->setText(QString::number(m_mArea));
        }
      }
      else {
        if (m_pixArea != Null) {
          m_distLineEdit->setText(QString::number(m_pixArea));
        }
        else {
          m_distLineEdit->setText("N/A");
        }
      }
    }
  }


  /**
   * Removes the connection on the given cube viewport.
   *
   *
   * @param cvp
   */
  void MeasureTool::removeConnections(MdiCubeViewport *cvp) {
    //  cvp->repaint();
    cvp->update();
  }


  /**
   * Updates the measure tool.
   *
   */
  void MeasureTool::updateTool() {
    m_distLineEdit->clear();
  }

}

