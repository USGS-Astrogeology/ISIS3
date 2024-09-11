#include "SunShadowTool.h"

#include <QApplication>
#include <QComboBox>
#include <QCheckBox>
#include <QFileDialog>
#include <QHBoxLayout>
#include <QLineEdit>
#include <QMenu>
#include <QMenuBar>
#include <QMessageBox>
#include <QStatusBar>
#include <QStackedWidget>
#include <QTableWidget>
#include <QToolButton>

#include "Angle.h"
#include "Camera.h"
#include "Distance.h"
#include "FileName.h"
#include "Latitude.h"
#include "Longitude.h"
#include "MdiCubeViewport.h"
#include "Projection.h"
#include "SurfacePoint.h"
#include "ToolPad.h"

namespace Isis {
  /**
   * Construct a sun shadow tool.
   *
   * @param parent The qt-parent relationship parent.
   */
  SunShadowTool::SunShadowTool(QWidget *parent) : Tool(parent) {
    m_enabled = false;
    m_tracking = false;
    m_shadowHeight = NULL;
    m_shadowLength = NULL;
    m_trackingAngle = NULL;
    m_drawInSunDirection = NULL;
    m_startSurfacePoint = NULL;
    m_endSurfacePoint = NULL;
    m_incidenceAngle = NULL;

    m_tableWin = new TableMainWindow("Sun Shadow Measurements", parent);
    m_tableWin->setTrackListItems(true);
    m_tableWin->installEventFilter(this);

    m_tableWin->addToTable(false, "Feature\nName", "Feature Name");
    m_tableWin->addToTable(false, "Feature\nType", "Feature Type");
    m_tableWin->addToTable(true,
        "Start\nLatitude:Start\nLongitude:End\nLatitude:End\nLongitude",
        "Ground Range", -1, Qt::Horizontal,
        "Start Latitude/Longitude to End Latitude/Longitude");
    m_tableWin->addToTable(false,
        "Start\nSample:Start\nLine:End\nSample:End\nLine",
        "Pixel Range", -1, Qt::Horizontal,
        "Start Sample/Line to End Sample/Line");
    m_tableWin->addToTable(true, "Shadow Length\n(km)", "Shadow Length (km)");
    m_tableWin->addToTable(true, "Shadow Length\n(m)", "Shadow Length (m)");
    m_tableWin->addToTable(true, "Shadow Height\n(km)", "Shadow Height (km)");
    m_tableWin->addToTable(true, "Shadow Height\n(m)", "Shadow Height (m)");
    m_tableWin->addToTable(true, "Incidence Angle\n(degrees)",
                           "Incidence Angle (degrees)");
    m_tableWin->addToTable(true, "Incidence Angle\n(radians)",
                           "Incidence Angle (radians)");
    m_tableWin->addToTable(false, "Path", "Path");
    m_tableWin->addToTable(false, "FileName", "FileName");
    m_tableWin->addToTable(false, "Notes", "Notes");

    m_tableWin->setStatusMessage("Click, Drag, and Release to Measure a Line");

    connect(this, SIGNAL(viewportChanged()),
            this, SLOT(reinitialize()));

    m_shadowHeight = new Distance;
    m_shadowLength = new Distance;
    m_trackingAngle = new Angle;
    m_startSurfacePoint = new SurfacePoint;
    m_endSurfacePoint = new SurfacePoint;
    m_incidenceAngle = new Angle;
  }


  /**
   * Create an action for activating this tool.
   *
   * @param toolpad The tool pad that will contain the given action
   * @return The created action
   */
  QAction *SunShadowTool::toolPadAction(ToolPad *toolpad) {
    QAction *action = new QAction(toolpad);
    action->setIcon(QPixmap(toolIconDir() + "/sunshadow.png"));
    action->setToolTip("Sun Shadow (U)");
    action->setShortcut(Qt::Key_U);

    QString text  =
      "<b>Function:</b> Calculate heights or depths of features in the active "
      "viewport given the measurement of a shadow. The shadow measurement "
      "should originate from the top of the feature and end when the shadow "
      "ends.\n"
      "<p><b>Shortcut:</b> U</p> ";
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
  QWidget *SunShadowTool::createToolBarWidget(QStackedWidget *parent) {
    QWidget *hbox = new QWidget(parent);
    QToolButton *showTableButton = new QToolButton(hbox);
    showTableButton->setText("Table");
    showTableButton->setToolTip("Record Measurement Data in Table");
    QString text =
        "<b>Function:</b> This button will bring up a table that will record "
        "the starting and ending points of the line, along with the calculated "
        "values for the two points on the image. To measure a shadow, "
        "click on the first point and releasing the mouse at the second point."
        "\n<p><b>Shortcut:</b>  CTRL+M</p>";
    showTableButton->setWhatsThis(text);
    showTableButton->setShortcut(Qt::CTRL + Qt::Key_M);
    connect(showTableButton, SIGNAL(clicked()), m_tableWin, SLOT(showTable()));
    connect(showTableButton, SIGNAL(clicked()), m_tableWin, SLOT(syncColumns()));
    connect(showTableButton, SIGNAL(clicked()), m_tableWin, SLOT(raise()));
    showTableButton->setEnabled(true);

    m_shadowHeightLineEdit = new QLineEdit(hbox);
    m_shadowHeightLineEdit->setText("");
    m_shadowHeightLineEdit->setMaxLength(12);
    m_shadowHeightLineEdit->setToolTip("Shadow Height");
    text = "<b>Function: </b> Shows the height of the shadow drawn on "
           "the image.";
    m_shadowHeightLineEdit->setWhatsThis(text);
    m_shadowHeightLineEdit->setReadOnly(true);

    m_unitsComboBox = new QComboBox(hbox);
    m_unitsComboBox->setSizeAdjustPolicy(QComboBox::AdjustToContents);
    m_unitsComboBox->addItem("Meters", Distance::Meters);
    m_unitsComboBox->addItem("Kilometers", Distance::Kilometers);

    connect(m_unitsComboBox, SIGNAL(activated(int)),
            this, SLOT(updateShadowHeightEdit()));

    m_drawInSunDirection = new QCheckBox("Draw in Sun Direction");
    m_drawInSunDirection->setChecked(true);

    QHBoxLayout *layout = new QHBoxLayout(hbox);
    layout->setMargin(0);
    layout->addWidget(m_drawInSunDirection);
    layout->addWidget(m_shadowHeightLineEdit);
    layout->addWidget(m_unitsComboBox);
    layout->addWidget(showTableButton);
    layout->addStretch(1);
    hbox->setLayout(layout);
    return hbox;
  }


  /**
   * Adds the measure action to the given menu.
   *
   *
   * @param menu
   */
  void SunShadowTool::addTo(QMenu *menu) {
  }


  /**
   * Paint anything we need to on the viewport. Currently, we draw a line along
   *   where the user is measuring a shadow.
   *
   * @param vp Viewport to paint
   * @param painter The painter to use for painting
   */
  void SunShadowTool::paintViewport(MdiCubeViewport *vp, QPainter *painter) {
    if (vp == cubeViewport()) {
      if (m_startSamp != Null && m_endSamp != Null &&
          m_startLine != Null && m_endLine != Null) {
        int vpStartX;
        int vpStartY;
        vp->cubeToViewport(m_startSamp, m_startLine,
                           vpStartX, vpStartY);

        int vpEndX;
        int vpEndY;
        vp->cubeToViewport(m_endSamp, m_endLine,
                           vpEndX, vpEndY);

        painter->setPen(QPen(Qt::red));
        painter->drawLine(QPoint(vpStartX, vpStartY), QPoint(vpEndX, vpEndY));
      }
    }
  }


  /**
   * When the mouse moves, if we're tracking then we go ahead and update all of
   *   our calculated values for the shadow measurement.
   *
   * @param p The current mouse position in viewport screen pixel coordinates
   */
  void SunShadowTool::mouseMove(QPoint p) {
    if (m_tracking && m_trackingAngle->isValid()) {
      cubeViewport()->viewportToCube(p.x(), p.y(),
                                     m_endSamp, m_endLine);

      if (m_drawInSunDirection->isChecked()) {
        // Recalculate the end line based on our drawing angle...
        // y = x * tan(angle) for the right triangle created from the
        //   user drawing a line.
        //
        //           E
        //         / |
        //        /  |
        //       /   | L
        //      /    |
        //     /A    |
        //   S-------|
        // S = Mouse Start Pos
        // A = sun angle
        // E = mouse end pos
        // L = Line height of the triangle
        //       (needs calculated, L = m_endLine - m_startLine)
        //
        Angle verticalDown(90, Angle::Degrees);
        Angle verticalUp(270, Angle::Degrees);
        bool adjustLine = true;

        if (*m_trackingAngle == verticalDown ||
            *m_trackingAngle == verticalUp) {
          m_endSamp = m_startSamp;

          adjustLine = false;
        }

        if (adjustLine) {
          m_endLine = m_startLine +
              (m_endSamp - m_startSamp) * tan(m_trackingAngle->radians());
        }
      }

      recalculateShadowHeight();

      if (m_tableWin->table()->rowCount()) {
        updateRow(m_tableWin->table()->rowCount() - 1);
      }

      cubeViewport()->viewport()->update();
    }
  }


  /**
   * When the mouse left button is pressed we start tracking.
   *
   * @param p The current mouse position in viewport screen pixel coordinates
   * @param s The mouse button that was pressed.
   */
  void SunShadowTool::mouseButtonPress(QPoint p, Qt::MouseButton s) {
    if (m_enabled && s == Qt::LeftButton) {
      if (m_tableWin->isVisible())
        addRow();

      reinitialize();

      cubeViewport()->viewportToCube(p.x(), p.y(),
                                     m_startSamp, m_startLine);

      Camera *cam = cubeViewport()->cube()->camera();

      if (cam->SetImage(m_startSamp, m_startLine)) {
        m_tracking = true;
        *m_trackingAngle = Angle(cam->SunAzimuth(), Angle::Degrees);
      }
      else {
        m_tracking = false;
        m_startSamp = Null;
        m_startLine = Null;
      }
      cubeViewport()->viewport()->update();
    }
  }


  /**
   * When the mouse left button is released we finish tracking.
   *
   * @param p The current mouse position in viewport screen pixel coordinates
   * @param s The mouse button that was pressed.
   */
  void SunShadowTool::mouseButtonRelease(QPoint p, Qt::MouseButton s) {
    if (s == Qt::LeftButton && m_tracking) {
      mouseMove(p);
    }

    m_tracking = false;
  }



  /**
   * This method updates the row in the table window with the
   * current measure information.
   *
   *
   * @param row
   */
  void SunShadowTool::updateRow(int row) {

    if (row >= m_tableWin->table()->rowCount() || !
        m_tableWin->isVisible()) {
      return;
    }

    // Blank out the row to remove stuff left over from previous cvps
    for (int c = 0; c < m_tableWin->table()->columnCount(); c++) {
      m_tableWin->table()->item(row, c)->setText("");
    }

    // Write all the new info to the current row
    if (m_startSurfacePoint->Valid()) {
      m_tableWin->table()->item(row, StartLatIndex)->setText(
          QString::number(m_startSurfacePoint->GetLatitude().degrees()));
      m_tableWin->table()->item(row, StartLonIndex)->setText(
          QString::number(m_startSurfacePoint->GetLongitude().degrees()));
    }
    else {
      m_tableWin->table()->item(row, StartLatIndex)->setText("N/A");
      m_tableWin->table()->item(row, StartLonIndex)->setText("N/A");
    }

    if (m_endSurfacePoint->Valid()) {
      m_tableWin->table()->item(row, EndLatIndex)->setText(
          QString::number(m_endSurfacePoint->GetLatitude().degrees()));
      m_tableWin->table()->item(row, EndLonIndex)->setText(
          QString::number(m_endSurfacePoint->GetLongitude().degrees()));
    }
    else {
      m_tableWin->table()->item(row, EndLatIndex)->setText("N/A");
      m_tableWin->table()->item(row, EndLonIndex)->setText("N/A");
    }

    if (m_startSamp != Null && m_startLine != Null) {
      m_tableWin->table()->item(row, StartSampIndex)->setText(
          QString::number(m_startSamp));
      m_tableWin->table()->item(row, StartLineIndex)->setText(
          QString::number(m_startLine));
    }
    else {
      m_tableWin->table()->item(row, StartSampIndex)->setText("N/A");
      m_tableWin->table()->item(row, StartLineIndex)->setText("N/A");
    }

    if (m_endSamp != Null && m_endLine != Null) {
      m_tableWin->table()->item(row, EndSampIndex)->setText(
          QString::number(m_endSamp));
      m_tableWin->table()->item(row, EndLineIndex)->setText(
          QString::number(m_endLine));
    }
    else {
      m_tableWin->table()->item(row, EndSampIndex)->setText("N/A");
      m_tableWin->table()->item(row, EndLineIndex)->setText("N/A");
    }

    if (m_shadowLength->isValid()) {
      m_tableWin->table()->item(row, ShadowLengthKmIndex)->setText(
          QString::number(m_shadowLength->kilometers()));
      m_tableWin->table()->item(row, ShadowLengthMIndex)->setText(
          QString::number(m_shadowLength->meters()));
    }
    else {
      m_tableWin->table()->item(row, ShadowLengthKmIndex)->setText("N/A");
      m_tableWin->table()->item(row, ShadowLengthMIndex)->setText("N/A");
    }

    if (m_shadowHeight->isValid()) {
      m_tableWin->table()->item(row, ShadowHeightKmIndex)->setText(
          QString::number(m_shadowHeight->kilometers()));
      m_tableWin->table()->item(row, ShadowHeightMIndex)->setText(
          QString::number(m_shadowHeight->meters()));
    }
    else {
      m_tableWin->table()->item(row, ShadowHeightKmIndex)->setText("N/A");
      m_tableWin->table()->item(row, ShadowHeightMIndex)->setText("N/A");
    }

    if (m_incidenceAngle->isValid()) {
      m_tableWin->table()->item(row, IncidenceAngleDegreesIndex)->setText(
          QString::number(m_incidenceAngle->degrees()));
      m_tableWin->table()->item(row, IncidenceAngleRadiansIndex)->setText(
          QString::number(m_incidenceAngle->radians()));
    }
    else {
      m_tableWin->table()->item(row, IncidenceAngleDegreesIndex)->setText("N/A");
      m_tableWin->table()->item(row, IncidenceAngleRadiansIndex)->setText("N/A");
    }

    m_tableWin->table()->item(row, PathIndex)->setText(m_path);
    m_tableWin->table()->item(row, FileNameIndex)->setText(m_fileName);
  }


  /**
   * Clear all calculated values and then re-calculate them.
   */
  void SunShadowTool::reinitialize() {
    m_startSamp = Null;
    m_endSamp   = Null;
    m_startLine = Null;
    m_endLine   = Null;

    *m_shadowHeight = Distance();
    *m_shadowLength = Distance();
    *m_startSurfacePoint = SurfacePoint();
    *m_endSurfacePoint = SurfacePoint();
    *m_incidenceAngle = Angle();

    recalculateShadowHeight();
  }


  /**
   * Add a results row to the table.
   */
  void SunShadowTool::addRow() {
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
   * Try to calculate the shadow height. Initialize as many member data
   *   variables as possible along the way for reporting to the user.
   */
  void SunShadowTool::recalculateShadowHeight() {
    try {
      if (m_startSamp != Null && m_startLine != Null &&
          m_endSamp != Null && m_endLine != Null) {
        m_path = QString::fromStdString(FileName(cubeViewport()->cube()->fileName().toStdString()).path());
        m_fileName = QString::fromStdString(FileName(cubeViewport()->cube()->fileName().toStdString()).name());

        /*     |
         *   \ _ /
         * -= (_) =-    THE SUN
         *   /   \ -
         *     |     -      <--- vector from the sun that intersects P1 and P2
         *              -
         *                -_         |
         *                /^\-       | 
         *               / | \  -    |
         *              / H|  \   -  |
         *     ________/   |   \__T_-|_________
         *                 P1     ^  P2
         *                     Shadow
         *
         *  T: Angle from the horizon to the sun
         *  H: Difference in planetary radius between P1 and P2
         *  L : length(Shadow)
         *  H = L * tan(T)
         *
         * We do not want the local incidence angle for T.
         *
         * Equation to variable mapping:
         *  T: theta
         *  H: m_shadowHeight
         *  L: m_shadowLength
         *  P1: m_startSurfacePoint
         *  P2: m_endSurfacePoint
         */
        
        bool success = true;
        Camera *cam = cubeViewport()->cube()->camera();
        success = cam->SetImage(m_startSamp, m_startLine);

        // Vector is in meters
        QVector3D sunDirection;

        if (success) {
          *m_startSurfacePoint = cam->GetSurfacePoint();
          double sunPosition[3];
          cam->sunPosition(sunPosition);

          Distance targetRadii[3];
          cam->radii(targetRadii);

          double origin[3] = {0.0, 0.0, 0.0};
          SpiceBoolean surfptSuccess;
          // Vector is in kilometers
          double naifVectorFromSunToP1[3] = {0.0, 0.0, 0.0};

          surfpt_c(origin, sunPosition, targetRadii[0].kilometers(),
                   targetRadii[1].kilometers(), targetRadii[2].kilometers(),
                   naifVectorFromSunToP1, &surfptSuccess);
          success = surfptSuccess;

          if (success) {
            sunDirection = QVector3D(
                naifVectorFromSunToP1[0] * 1000.0,
                naifVectorFromSunToP1[1] * 1000.0,
                naifVectorFromSunToP1[2] * 1000.0).normalized();
          }
        }

        if (success) {
          success = cam->SetImage(m_endSamp, m_endLine);
        }

        if (success) {
          *m_endSurfacePoint = cam->GetSurfacePoint();

          *m_incidenceAngle = Angle(cam->IncidenceAngle(), Angle::Degrees);
          Angle theta = Angle(90.0, Angle::Degrees) - *m_incidenceAngle;

          Displacement deltaX = m_startSurfacePoint->GetX() - m_endSurfacePoint->GetX();

          Displacement deltaY = m_startSurfacePoint->GetY() - m_endSurfacePoint->GetY();

          Displacement deltaZ = m_startSurfacePoint->GetZ() - m_endSurfacePoint->GetZ();

          *m_shadowLength = Distance(sqrt( deltaX.meters() * deltaX.meters() +
                                           deltaY.meters() * deltaY.meters() +
                                           deltaZ.meters() * deltaZ.meters() ),
                                     Distance::Meters);

          *m_shadowHeight = Distance(m_shadowLength->meters() * tan( theta.radians() ),
                                     Distance::Meters);
        }
      }
    }
    catch (IException &) {
      reinitialize();
    }

    updateShadowHeightEdit();
  }


  //! Change the value in the distance edit to match the units
  void SunShadowTool::updateShadowHeightEdit() {
    if (m_shadowHeight->isValid()) {
      Distance::Units displayUnits =
          (Distance::Units)m_unitsComboBox->itemData(
            m_unitsComboBox->currentIndex()).toInt();

      switch (displayUnits) {
        case Distance::Meters:
          m_shadowHeightLineEdit->setText(
              QString::number(m_shadowHeight->meters()));
          break;
        case Distance::Kilometers:
          m_shadowHeightLineEdit->setText(
              QString::number(m_shadowHeight->kilometers()));
          break;
        case Distance::SolarRadii:
        case Distance::Pixels:
          m_shadowHeightLineEdit->setText("Not Supported");
          break;
      }

    }
    else {
      m_shadowHeightLineEdit->setText("");
    }
  }


  /**
   * This enables/disables this tool's functionality based on the active
   *   viewport's compatibility.
   */
  void SunShadowTool::updateTool() {
    MdiCubeViewport *activeViewport = cubeViewport();

    bool hasCamera = true;
    try {
      hasCamera = activeViewport &&
                  (activeViewport->cube()->camera() != NULL);
    }
    catch (IException &) {
      hasCamera = false;
    }

    m_shadowHeightLineEdit->setEnabled(hasCamera);
    m_unitsComboBox->setEnabled(hasCamera);
    m_enabled = hasCamera;

    updateShadowHeightEdit();
  }

}

