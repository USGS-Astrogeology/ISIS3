/** This is free and unencumbered software released into the public domain.

The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */

#include "ChipViewport.h"

#include <iostream>

#include <QMessageBox>
#include <QPainter>

#include "ControlMeasure.h"
#include "ControlNet.h"
#include "ControlPoint.h"
#include "Cube.h"
#include "CubeViewport.h"
#include "SerialNumber.h"
#include "Stretch.h"

namespace Isis {

  /**
   * Construct an empty viewport
   *
   * @param width The width of the viewport
   * @param height The height of the viewport
   * @param parent The parent widget
   */
  ChipViewport::ChipViewport(int width, int height, QWidget *parent) : QWidget(parent) {

    setFixedSize(width, height);
    setBackgroundRole(QPalette::Dark);
//    setFocusPolicy(Qt::StrongFocus);

    m_width = width;
    m_height = height;
    m_zoomFactor = 1.0;
    m_rotation = 0;
    m_geomIt = false;
    m_tempView = NULL;
    m_cross = true;
    m_circle = false;
    m_chip = NULL;
    m_chipCube = NULL;
    m_matchChip = NULL;
    m_matchChipCube = NULL;
    m_image = NULL;
    m_image = new QImage(512, 512, QImage::Format_RGB32);
    m_image->fill(Qt::black);
    m_controlNet = NULL;
    m_stretchLocked = false;
    m_stretch = NULL;

    m_stretch = new Stretch;
  }


  /**
   * Destructor
   */
  ChipViewport::~ChipViewport() {
    if (m_stretch) {
      delete m_stretch;
      m_stretch = NULL;
    }
  }


  /**
   * Get viewport x and y from cube sample and line
   *
   * @param samp Sample in cube
   * @param line Line in cube
   * @param x Calcualated viewport x coordinate
   * @param y Calcualated viewport y coordinate
   *
   * @return @b bool true if the point is contained in the viewport, false otherwise
   */
  bool ChipViewport::cubeToViewport(double samp, double line,
                                    int &x, int &y) {

    m_chip->SetCubePosition(samp, line);
    x = ((int) m_chip->ChipSample()) - 1;
    y = ((int) m_chip->ChipLine()) - 1;

    return m_chip->IsInsideChip(samp, line);
  }


  /**
   * Set the chip for this ChipViewport
   *
   * @param chip Pointer to the chip to set the viewport with
   * @param chipCube Pointer to the chip's cube
   *
   * @throws IException::Programmer "Can not view NULL chip pointer"
   *
   * @author Tracie Sucharski
   * @internal
   *   @history 2009-14-2009 Tracie Sucharski - Make sure the m_image is clear
   *                             before allocating new.
   */

  void ChipViewport::setChip(Chip *chip, Cube *chipCube) {
    // Is the chip usable?
    if (chip == NULL || chipCube == NULL) {
      throw IException(IException::Programmer,
                       "Can not view NULL chip pointer",
                       _FILEINFO_);
    }

    m_zoomFactor = 1.0;
    m_rotation = 0;

    m_chip = chip;
    m_chipCube = chipCube;
    if (m_image != NULL)
      delete m_image;
    m_image = new QImage(chip->Samples(), chip->Lines(), QImage::Format_RGB32);

    autoStretch();
    emit tackPointChanged(m_zoomFactor);
  }


  /**
   * Apply automatic stretch using data from entire chip
   */
  void ChipViewport::autoStretch() {
    computeStretch(m_gray.stretch);
    paintImage();
    update();
  }


  /**
   * Applies a new stretch to the specified cube viewport
   *
   * @param newStretch Pointer to the new stretch to apply
   * @param cvp Pointer to the cube viewport to apply the stretch to
   */
  void ChipViewport::stretchFromCubeViewport(Stretch *newStretch, CubeViewport *cvp) {

    if (!cvp || !m_chipCube)
      return;

    // only stretch if the CubeViewport is opened to the same cube as we are,
    // otherwise the signal was meant for a different ChipViewport!
    if (cvp->cube()->fileName() == m_chipCube->fileName()) {

      // if user right clicked in the CubeViewport then we get a SIGNAL with a
      // NULL Stretch.  This is used to signify that we need to restretch on our
      // own (go back to global).
      if (!newStretch) {
        computeStretch(m_gray.stretch, true);
        paintImage();
        update();
      }
      else {
        *m_stretch = *newStretch;
        m_gray.stretch = *newStretch;
        paintImage();
        update();
      }
    }
  }


  /**
   * Locks or unlocks the stretch on the chip viewport during transformations (zoom, pan, etc.)
   *
   * @param newStatus Non-zero locks the stretch
   */
  void ChipViewport::changeStretchLock(int newStatus) {
    if (newStatus == 0)
      m_stretchLocked = false;
    else
      m_stretchLocked = true;
  }


  /**
   * Compute automatic stretch for a portion of the cube. If the stretch is locked and force is
   * false, no computations will be performed.
   *
   * @param stretch Reference to the stretch to be computed (if unlocked)
   * @param force If true, forces the stretch to be computed.
   */
  void ChipViewport::computeStretch(Stretch &stretch, bool force) {
    if (m_stretchLocked && !force) {
      stretch = *m_stretch;
    }
    else {
      Statistics stats;
      for (int line = 1; line < m_chip->Lines(); line++) {
        for (int samp = 1; samp < m_chip->Samples(); samp++) {
          double value = m_chip->GetValue(samp, line);
          stats.AddData(&value, 1);
        }
      }

      Histogram hist(stats.BestMinimum(), stats.BestMaximum());
      for (int line = 1; line <= m_chip->Lines(); line++) {
        for (int samp = 1; samp <= m_chip->Samples(); samp++) {
          double value = m_chip->GetValue(samp, line);
          hist.AddData(&value, 1);
        }
      }

      stretch.ClearPairs();
      if (hist.Percent(0.5) != hist.Percent(99.5)) {
        stretch.AddPair(hist.Percent(0.5), 0.0);
        stretch.AddPair(hist.Percent(99.5), 255.0);
      }
      else {
        stretch.AddPair(-DBL_MAX, 0.0);
        stretch.AddPair(DBL_MAX, 255.0);
      }

      *m_stretch = stretch;
    }
  }


  /**
   * Paints the chip viewport after the chip has been updated
   */
  void ChipViewport::paintImage() {
    //TODO ??? Need something similar to CubeViewport clipping, so that
    //         at edge of image, fill viewport w/ black
    for (int y = 0; y < m_chip->Lines(); y++) {
      QRgb *rgb = (QRgb *) m_image->scanLine(y);
      int r, g, b;
      for (int x = 0; x < m_chip->Samples(); x++) {
        r = g = b = (int) m_gray.stretch.Map(m_chip->GetValue(x + 1, y + 1));
        rgb[x] =  qRgb(r, g, b);
      }
    }
    repaint();

  }


  /**
   * Repaint the viewport
   *
   * @param e QPaintEvent
   *
   * @internal
   *   @history 2011-08-23 Tracie Sucharski - Use the GetMeasuresInCube method
   *                           from ControlNet to get list of measures rather
   *                           than searching through entire net.
   *   @history 2011-11-09 Tracie Sucharski - If there are no measures for
   *                           this cube, return.
   */
  void ChipViewport::paintEvent(QPaintEvent *e) {

    QPainter painter(this);

    if (m_tempView != NULL) {
      painter.drawImage(0, 0, *(m_tempView->m_image));
    }
    else {
      painter.drawImage(0, 0, *m_image);
    }

    if (m_cross == true) {
      painter.setPen(Qt::red);
      painter.drawLine(0, (m_height - 1) / 2, m_width - 1, (m_height - 1) / 2);
      painter.drawLine((m_width - 1) / 2, 0, (m_width - 1) / 2, m_height - 1);
    }

    if (m_circle == true) {
      painter.setPen(Qt::red);
      painter.drawEllipse((m_height - 1) / 2 - m_circleSize / 2,
                          (m_width - 1) / 2 - m_circleSize / 2,
                          m_circleSize, m_circleSize);
    }

    QString serialNumber = m_chipCube? SerialNumber::Compose(*m_chipCube) : QString();
    if (m_controlNet && !serialNumber.isEmpty()
        && m_controlNet->GetCubeSerials().contains(serialNumber)) {
      // draw measure locations if we have a control network
      //  If the serial number is Unknown, we probably have a ground source
      //  file or level 2 which means it does not exist in the network
      //TODO Is there a better way to handle this?
      if (m_showPoints && serialNumber.compare("Unknown") &&
          m_controlNet->GetNumPoints() != 0) {
        QList<ControlMeasure *> measures =
                                   m_controlNet->GetMeasuresInCube(serialNumber);
        // loop through all points in the control net
        for (int i = 0; i < measures.count(); i++) {
          ControlMeasure *m = measures[i];
          // Find the measurments on the viewport
          double samp = m->GetSample();
          double line = m->GetLine();
          int x, y;

          cubeToViewport(samp, line, x, y);
          // Determine pen color
          // if the point or measure is ignored set to yellow
          if (m->Parent()->IsIgnored() ||
             (!m->Parent()->IsIgnored() && m->IsIgnored())) {
            painter.setPen(QColor(255, 255, 0)); // set point marker yellow
          }
          // check for ground measure
          else if (m->Parent()->GetType() == ControlPoint::Fixed) {
            painter.setPen(Qt::magenta);// set point marker magenta
          }
          else {
            painter.setPen(Qt::green); // set all other point markers green
          }

          // draw points which are not under cross
          if (x != (m_width - 1) / 2 || y != (m_height - 1) / 2) {
            painter.drawLine(x - 5, y, x + 5, y);
            painter.drawLine(x, y - 5, x, y + 5);
          }
        }
      }
    }

    m_tempView = NULL;
    //painter.end();

  }


  /**
   * Load with info from given ChipViewport
   *
   * @param newView The chip viewport to load from
   */
  void ChipViewport::loadView(ChipViewport &newView) {
    m_tempView = &newView;
    update();
  }


  /**
   * Returns tack sample
   *
   * @return @b double The tack sample for the chip
   */
  double ChipViewport::tackSample() {
    m_chip->SetChipPosition(m_chip->TackSample(), m_chip->TackLine());
    return m_chip->CubeSample();
  }


  /**
   * Returns tack line
   *
   * @return @b double The tack line for the chip
   */
  double ChipViewport::tackLine() {
    m_chip->SetChipPosition(m_chip->TackSample(), m_chip->TackLine());
    return m_chip->CubeLine();
  }


  /**
   * Pan up
   */
  void ChipViewport::panUp() {
    int x, y;
    x = m_chip->TackSample();
    y = m_chip->TackLine() - 1;
    //  Reload with new cube position
    m_chip->SetChipPosition((double)x, (double)y);
    reloadChip(m_chip->CubeSample(), m_chip->CubeLine());
  }


  /**
   * Pan down
   */
  void ChipViewport::panDown() {
    int x, y;
    x = m_chip->TackSample();
    y = m_chip->TackLine() + 1;
    //  Reload with new cube position
    m_chip->SetChipPosition((double)x, (double)y);
    reloadChip(m_chip->CubeSample(), m_chip->CubeLine());
  }


  /**
   * Pan left
   */
  void ChipViewport::panLeft() {
    int x, y;
    x = m_chip->TackSample() - 1;
    y = m_chip->TackLine();
    //  Reload with new cube position
    m_chip->SetChipPosition((double)x, (double)y);
    reloadChip(m_chip->CubeSample(), m_chip->CubeLine());
  }


  /**
   * Pan right
   */
  void ChipViewport::panRight() {
    int x, y;
    x = m_chip->TackSample() + 1;
    y = m_chip->TackLine();
    //  Reload with new cube position
    m_chip->SetChipPosition((double)x, (double)y);
    reloadChip(m_chip->CubeSample(), m_chip->CubeLine());
  }


  /**
   * Zoom in
   */
  void ChipViewport::zoomIn() {
    m_zoomFactor /= 2.0;
    reloadChip();
  }


  /**
   * Zoom out
   */
  void ChipViewport::zoomOut() {
    m_zoomFactor *= 2.0;
    reloadChip();
  }


  /**
   * Zoom by a factor of one
   */
  void ChipViewport::zoom1() {
    m_zoomFactor = 1.0;
    reloadChip();
  }


  /**
   * Zoom by a specified factor
   *
   * @param zoomFactor The zoom factor
   */
  void ChipViewport::zoom(double zoomFactor) {
    m_zoomFactor = zoomFactor;
    reloadChip();
  }


  /**
   * Returns the current zoom factor
   *
   * @return @b double The current zoom factor
   */
  double ChipViewport::zoomFactor() {
    return m_zoomFactor;
  }


  /**
   * Slot to refresh viewport when the tack point has changed
   *
   * @param tackSample Sample on the cube to load the chip viewport at (center)
   * @param tackLine Line on the cube to load the chip viewport at (center)
   */
  void ChipViewport::refreshView(double tackSample, double tackLine) {
    reloadChip(tackSample, tackLine);
  }


  /**
   * If mouse enters, make sure key events are processed w/o clicking
   *
   * @param e QEvent
   */
  void ChipViewport::enterEvent(QEvent *e) {
    setFocus();
  }


  /**
   * Process arrow keystrokes on cube
   *
   * @param e QKeyEvent
   */
  void ChipViewport::keyPressEvent(QKeyEvent *e) {

    int x, y;
    x = m_chip->TackSample();
    y = m_chip->TackLine();
    if (e->key() == Qt::Key_Up) {
      y--;
    }
    else if (e->key() == Qt::Key_Down) {
      y++;
    }
    else if (e->key() == Qt::Key_Left) {
      x--;
    }
    else if (e->key() == Qt::Key_Right) {
      x++;
    }
    else {
      QWidget::keyPressEvent(e);
      return;
    }

    //  Reload with new cube position
    m_chip->SetChipPosition((double)x, (double)y);
    reloadChip(m_chip->CubeSample(), m_chip->CubeLine());
    emit userMovedTackPoint();
  }


  /**
   * Process mouse events
   *
   * @param event QMouseEvent
   *
   * @internal
   *   @history 2011-09-29 Tracie Sucharski - Added the setFocus call
   *                           so that arrow keys would work.
   */
  void ChipViewport::mousePressEvent(QMouseEvent *event) {

    QPoint p = event->pos();
    if (event->button() == Qt::LeftButton) {
      //  Reload with new cube position
      m_chip->SetChipPosition((double)p.x(), (double)p.y());
      reloadChip(m_chip->CubeSample(), m_chip->CubeLine());
      emit userMovedTackPoint();
      //  This was added when the scrolled area was added to the QnetTool.
      //  For some reason when clicking in the ChipViewport, focus would be
      //  lost and the arrow keys would no longer work.
      //TODO  Is this the correct way to fix this, why is it happening?
      setFocus();
    }
  }


  /**
   * Slot to set whether control points are drawn
   *
   * @param checked Control points are set to be drawn if true
   */
  void ChipViewport::setPoints(bool checked) {

    if (checked == m_showPoints)
      return;

    m_showPoints = checked;
    repaint();

  }


  /**
   * Slot to change state of crosshair
   *
   * @param checked Crosshair set to be drawn if true
   */
  void ChipViewport::setCross(bool checked) {

    if (checked == m_cross)
      return;

    m_cross = checked;
    repaint();

  }


  /**
   * Slot to change state of circle
   *
   * @param checked Circle set to be drawn if true
   */
  void ChipViewport::setCircle(bool checked) {

    if (checked == m_circle)
      return;

    m_circle = checked;
    repaint();
  }


  /**
   * Set the size of the circle
   *
   * @param size The size of the circle
   */
  void ChipViewport::setCircleSize(int size) {

    m_circleSize = size;
    repaint();

  }


  /**
   * Slot to geom chip (apply geometry transformation)
   *
   * @param matchChip The matching chip for the geometry
   * @param matchChipCube The matching chip's cube for the geometry
   *
   * @internal
   *   @history 2010-06-16 Jeannie Walldren - Catch possible iException from Chip's
   *                           Load() method and display in QMessageBox
   */
  void ChipViewport::geomChip(Chip *matchChip, Cube *matchChipCube) {

    m_geomIt = true;
    m_matchChip = matchChip;
    m_matchChipCube = matchChipCube;
    try {
      m_chip->Load(*m_chipCube, *matchChip, *matchChipCube);
//    m_chip->ReLoad(*matchChip,m_zoomFactor);
    }
    catch (IException &e) {
      std::string msg = "Cannot geom chip.\n";
      msg += e.toString();
      QMessageBox::information((QWidget *)parent(), "Error", msg);
      return;
    }

    //TODO ??? Can these be added to paintEvent method ???
    autoStretch();
  }


  /**
   * Slot to un-geom chip (revert geometry transformation)
   *
   * @internal
   *   @history 2010-06-16 Jeannie Walldren - Catch possible iException from Chip's
   *                           Load() method and display in QMessageBox
   */
  void ChipViewport::nogeomChip() {

    m_geomIt = false;
    try {
      m_chip->Load(*m_chipCube, m_rotation, m_zoomFactor);
    }
    catch (IException &e) {
      std::string msg = "Cannot load no geom chip.\n";
      msg += e.toString();
      QMessageBox::information((QWidget *)parent(), "Error", msg);
      return;
    }

    //TODO ??? Can these be added to paintEvent method ???
    autoStretch();
  }


  /**
   * Slot to rotate chip
   *
   * @param rotation How much to rotate the chip
   *
   * @internal
   *   @history 2010-06-16 Jeannie Walldren - Catch possible iException from Chip's
   *                           Load() method and display in QMessageBox
   */

  void ChipViewport::rotateChip(int rotation) {

    m_rotation = -rotation;
    try {
      m_chip->Load(*m_chipCube, -rotation, m_zoomFactor);
    }
    catch (IException &e) {
      std::string msg = "Cannot load rotated chip.\n";
      msg += e.toString();
      QMessageBox::information((QWidget *)parent(), "Error", msg);
      return;
    }

    //TODO ??? Can these be added to paintEvent method ???
    autoStretch();
  }


  /**
   * Reloads the chip at the given tack point on the cube
   *
   * @param tackSample Sample in the cube to load the chip on (center)
   * @param tackLine Line in the cube to load the chip on (center)
   *
   * @throws IException::Programmer "Can not view NULL chip pointer"
   *
   * @internal
   *   @history 2010-06-16 Jeannie Walldren - Catch possible iException from Chip's
   *                           Load() method and display in QMessageBox
   */
  void ChipViewport::reloadChip(double tackSample, double tackLine) {

    // Is the chip usable?
    if (m_chip == NULL) {
      throw IException(IException::Programmer,
                       "Can not view NULL chip pointer",
                       _FILEINFO_);
    }

    if (tackSample != 0. && tackLine != 0.)
      m_chip->TackCube(tackSample, tackLine);
    if (m_geomIt) {
      if (m_matchChip == NULL) {
        throw IException(IException::User, "Invalid match chip", _FILEINFO_);
      }
      try {
        m_chip->Load(*m_chipCube, *m_matchChip, *m_matchChipCube);
      }
      catch (IException &e) {
        std::string msg = "Cannot reload chip.\n";
        msg += e.toString();
        QMessageBox::information((QWidget *)parent(), "Error", msg);
        return;
      }
//      m_chip->ReLoad(*m_matchChip,m_zoomFactor);
    }
    else {
      try {
        m_chip->Load(*m_chipCube, m_rotation, m_zoomFactor);
      }
      catch (IException &e) {
        std::string msg = "Cannot reload chip.\n";
        msg += e.toString();
        QMessageBox::information((QWidget *)parent(), "Error", msg);
        return;
      }
    }


    //TODO ??? Can these be added to paintEvent method ???
    autoStretch();

    //  emit signal indicating tack point changed so that the TieTool
    //  can update the sample/line label.
    emit tackPointChanged(m_zoomFactor);
  }
}
