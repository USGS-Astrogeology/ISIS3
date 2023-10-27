/** This is free and unencumbered software released into the public domain.

The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */

#include "CubeViewport.h"

#include <iomanip>
#include <iostream>
#include <QApplication>
#include <QClipboard>
#include <QCloseEvent>
#include <QCursor>
#include <QDebug>
#include <QIcon>
#include <QPen>
#include <QPainter>
#include <QFileInfo>
#include <QMessageBox>
#include <QMouseEvent>
#include <QRgb>
#include <QScrollBar>
#include <QString>
#include <QTimer>

#include "Brick.h"
#include "Camera.h"
#include "CubeDataThread.h"
#include "IException.h"
#include "IString.h"
#include "FileName.h"
#include "Histogram.h"
#include "Pvl.h"
#include "PvlGroup.h"
#include "PvlKeyword.h"
#include "PvlObject.h"
#include "Stretch.h"
#include "CubeStretch.h"
#include "StretchTool.h"
#include "Tool.h"
#include "UniversalGroundMap.h"
#include "ViewportBuffer.h"


using namespace std;


namespace Isis {
  /**
   * Construct a cube viewport
   *
   *
   * @param cube
   * @param parent
   */
  CubeViewport::CubeViewport(Cube *cube, CubeDataThread * cubeData,
      QWidget *parent) : QAbstractScrollArea(parent) {
    // Is the cube usable?
    if(cube == NULL) {
      throw IException(IException::Programmer,
                       "Can not view NULL cube pointer",
                       _FILEINFO_);
    }
    else if(!cube->isOpen()) {
      throw IException(IException::Programmer,
                       "Can not view unopened cube",
                       _FILEINFO_);
    }

    p_cube = cube;
    p_cubeData = NULL;

    if (cubeData)
    {
      p_cubeData = cubeData;
      p_thisOwnsCubeData = false;
      p_cubeId = p_cubeData->FindCubeId(cube);
    }
    else
    {
      p_cubeData = new CubeDataThread();
      p_thisOwnsCubeData = true;
      p_cubeId = p_cubeData->AddCube(p_cube);
    }

    if(p_cube->hasGroup("Tracking")) {
      setTrackingCube();
    }
    else {
      p_trackingCube = NULL;
    }


    connect(p_cubeData, SIGNAL(BrickChanged(int, const Isis::Brick *)),
            this, SLOT(cubeDataChanged(int, const Isis::Brick *)));
    connect(this, SIGNAL(doneWithData(int, const Isis::Brick *)),
            p_cubeData, SLOT(DoneWithData(int, const Isis::Brick *)));

    p_cubeData->AddChangeListener();

    void doneWithData(int, const Brick *);

    // Set up the scroll area
    setAttribute(Qt::WA_DeleteOnClose);
    setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
    setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
    viewport()->setObjectName("viewport");
    viewport()->setCursor(QCursor(Qt::CrossCursor));
    viewport()->installEventFilter(this);
    viewport()->setAttribute(Qt::WA_OpaquePaintEvent);

    setAttribute(Qt::WA_NoSystemBackground);
    setFrameShadow(QFrame::Plain);
    setFrameShape(QFrame::NoFrame);
    setAutoFillBackground(false);

//    viewport()->setAttribute(Qt::WA_NoSystemBackground);
//    viewport()->setAttribute(Qt::WA_PaintOnScreen,false);

    p_saveEnabled = false;
    // Save off info about the cube
    p_scale = -1.0;
    p_color = false;


    setCaption();

    p_redBrick = new Brick(4, 1, 1, cube->pixelType());
    p_grnBrick = new Brick(4, 1, 1, cube->pixelType());
    p_bluBrick = new Brick(4, 1, 1, cube->pixelType());
    p_gryBrick = new Brick(4, 1, 1, cube->pixelType());
    p_pntBrick = new Brick(4, 1, 1, cube->pixelType());

    p_paintPixmap = false;
    p_image = NULL;
    p_cubeShown = true;
    p_updatingBuffers = false;

    //updateScrollBars(1,1);

    p_groundMap = NULL;
    p_projection = NULL;
    p_camera = NULL;

    // Setup a universal ground map
    try {
      p_groundMap = new UniversalGroundMap(*p_cube);
    }
    catch(IException &) {
    }

    if(p_groundMap != NULL) {
      // Setup the camera or the projection
      if(p_groundMap->Camera() != NULL) {
        p_camera = p_groundMap->Camera();
        if(p_camera->HasProjection()) {
          try {
            p_projection = cube->projection();
          }
          catch(IException &) {
          }
        }
      }
      else {
        p_projection = p_groundMap->Projection();
      }
    }


    // Setup context sensitive help
    QString cubeFileName = p_cube->fileName();
    p_whatsThisText = QString("<b>Function: </b>Viewport to ") + cubeFileName;

    p_cubeWhatsThisText =
      "<p><b>Cube Dimensions:</b> \
      <blockQuote>Samples = " +
      QString::number(cube->sampleCount()) + "<br>" +
      "Lines = " +
      QString::number(cube->lineCount()) + "<br>" +
      "Bands = " +
      QString::number(cube->bandCount()) + "</blockquote></p>";

    /*setting up the qlist of CubeBandsStretch objs.
    for( int b = 0; b < p_cube->bandCount(); b++) {
      CubeBandsStretch *stretch = new CubeBandsStretch();
      p_bandsStretchList.push_back(stretch);
    }*/

    p_grayBuffer = new ViewportBuffer(this, p_cubeData, p_cubeId);
    p_grayBuffer->enable(false);
    p_grayBuffer->setBand(1);

    p_redBuffer = NULL;
    p_greenBuffer = NULL;
    p_blueBuffer = NULL;
    p_pixmapPaintRects = NULL;

    p_pixmapPaintRects = new QList<QRect *>();
    p_progressTimer = new QTimer();
    p_progressTimer->setInterval(250);

    p_knownStretches = new QVector< Stretch * >();
    p_globalStretches = new QVector< Stretch * >();

    while(p_cube->bandCount() > p_knownStretches->size()) {
      p_knownStretches->push_back(NULL);
      p_globalStretches->push_back(NULL);
    }

    connect(p_progressTimer, SIGNAL(timeout()), this, SLOT(onProgressTimer()));

    p_bgColor = Qt::black;

    p_comboCount = 0;
    p_comboIndex = 0;

    p_image = new QImage(viewport()->size(), QImage::Format_RGB32);
  }


  /**
   * This method is called to initially show the viewport. It will set the
   * scale to show the entire cube and enable the gray buffer.
   *
   * @param event Show event being received.
   *
   * @see http://doc.qt.io/qt-5/qwidget.html#showEvent
   */
  void CubeViewport::showEvent(QShowEvent *event) {
    if(p_scale == -1) {
      // This doesn't equal fitScale() ... causes misalignment in qview initial
      //   view versus zoomFit
      //double sampScale = (double) sizeHint().width() / (double) cubeSamples();
      //double lineScale = (double) sizeHint().height() / (double) cubeLines();
      //double scale = sampScale < lineScale ? sampScale : lineScale;

      setScale(fitScale(), cubeSamples() / 2.0 + 0.5, cubeLines() / 2.0 + 0.5);
    }

    if(p_grayBuffer && !p_grayBuffer->enabled()) {
      p_grayBuffer->enable(true);

      // gives a proper initial stretch (entire cube)
      p_grayBuffer->addStretchAction();
    }

    QAbstractScrollArea::show();

    p_paintPixmap = true;

    paintPixmap();
  }


  /**
   * This updates the progress bar visually. Conceptually it emits
   * the progressChanged signal with the current progress.
   */
  void CubeViewport::onProgressTimer() {
    double progress = 0.0;
    bool completed = false;

    if(p_grayBuffer) {
      progress += p_grayBuffer->currentProgress();
      completed = !p_grayBuffer->working();
    }

    if(p_redBuffer) {
      progress += p_redBuffer->currentProgress() / 3.0;
      completed = !p_redBuffer->working();
    }

    if(p_greenBuffer) {
      progress += p_greenBuffer->currentProgress() / 3.0;
      completed = completed && !p_greenBuffer->working();
    }

    if(p_blueBuffer) {
      progress += p_blueBuffer->currentProgress() / 3.0;
      completed = completed && !p_blueBuffer->working();
    }

    int realProgress = (int)(progress * 100.0);

    if(completed) {
      realProgress = 100;
      p_progressTimer->stop();
      emit progressComplete();
      emit screenPixelsChanged();
    }
    else if(realProgress == 100) {
      realProgress = 99;
    }

    emit progressChanged(realProgress);
  }


  /**
   * Destructor
   *
   */
  CubeViewport::~CubeViewport() {
    if(p_redBrick) {
      delete p_redBrick;
      p_redBrick = NULL;
    }

    if(p_grnBrick) {
      delete p_grnBrick;
      p_grnBrick = NULL;
    }

    if(p_bluBrick) {
      delete p_bluBrick;
      p_bluBrick = NULL;
    }

    if(p_gryBrick) {
      delete p_gryBrick;
      p_gryBrick = NULL;
    }

    if(p_pntBrick) {
      delete p_pntBrick;
      p_pntBrick = NULL;
    }

    if(p_groundMap) {
      delete p_groundMap;
      p_groundMap = NULL;
    }

    if(p_grayBuffer) {
      delete p_grayBuffer;
      p_grayBuffer = NULL;
    }

    if(p_redBuffer) {
      delete p_redBuffer;
      p_redBuffer = NULL;
    }

    if(p_greenBuffer) {
      delete p_greenBuffer;
      p_greenBuffer = NULL;
    }

    if(p_blueBuffer) {
      delete p_blueBuffer;
      p_blueBuffer = NULL;
    }

    // p_cubeData MUST be deleted AFTER all viewport buffers!!!
    if(p_cubeData) {
      p_cubeData->RemoveChangeListener();

      if(p_thisOwnsCubeData)
        delete p_cubeData;

      p_cubeData = NULL;
    }

    p_cube = NULL;

    delete p_trackingCube;
    p_trackingCube = NULL;

    if(p_progressTimer) {
      delete p_progressTimer;
      p_progressTimer = NULL;
    }

    if(p_image) {
      delete p_image;
      p_image = NULL;
    }

    if(p_pixmapPaintRects) {
      for(int rect = 0; rect < p_pixmapPaintRects->size(); rect++) {
        delete(*p_pixmapPaintRects)[rect];
      }

      delete p_pixmapPaintRects;
      p_pixmapPaintRects = NULL;
    }

    if(p_knownStretches) {
      for(int stretch = 0; stretch < p_knownStretches->size(); stretch ++) {
        if((*p_knownStretches)[stretch] != NULL) {
          delete(*p_knownStretches)[stretch];
          (*p_knownStretches)[stretch] = NULL;
        }
      }

      p_knownStretches->clear();

      delete p_knownStretches;
      p_knownStretches = NULL;
    }

    if(p_globalStretches) {
      for(int stretch = 0; stretch < p_globalStretches->size(); stretch ++) {
        if((*p_globalStretches)[stretch] != NULL) {
          delete(*p_globalStretches)[stretch];
          (*p_globalStretches)[stretch] = NULL;
        }
      }

      p_globalStretches->clear();

      delete p_globalStretches;
      p_globalStretches = NULL;
    }
  }

  /**
   * This method sets the viewports cube.
   *
   * @param cube
   */
  void CubeViewport::setCube(Cube *cube) {
    p_cube = cube;
    setCaption();
  }


  //! Return the number of samples in the cube
  int CubeViewport::cubeSamples() const {
    return p_cube->sampleCount();
  }


  //! Return the number of lines in the cube
  int CubeViewport::cubeLines() const {
    return p_cube->lineCount();
  }


  //! Return the number of bands in the cube
  int CubeViewport::cubeBands() const {
    return p_cube->bandCount();
  }


  /**
   * This method updates the internal viewport buffer based on
   * changes in cube DN values.
   *
   * @param cubeId Cube that the changed brick belongs to
   * @param * data New data
   */
  void CubeViewport::cubeDataChanged(int cubeId, const Brick *data) {
    if(cubeId == p_cubeId) {
      double ss, sl, es, el;
      ss = data->Sample();
      sl = data->Line();
      es = data->Sample() + data->SampleDimension();
      el = data->Line() + data->LineDimension();
      if(ss < 0.5){
        ss = 0.5;
      }
      if(sl < 0.5){
        sl = 0.5;
      }
      if(es > cube()->sampleCount() + 0.5){
        es = cube()->sampleCount() + 0.5;
      }
      if(el > cube()->lineCount() + 0.5){
        el = cube()->lineCount() + 0.5;
      }

      int sx, sy, ex, ey;

      cubeToViewport(ss, sl, sx, sy);
      cubeToViewport(es, el, ex, ey);
      if(sx < 0){
        sx = 0;
      }
      if(sy < 0){
        sy = 0;
      }
      if(ex > viewport()->width()){
        ex = viewport()->width();
      }
      if(ey > viewport()->height()){
        ey = viewport()->height();
      }
      QRect vpRect(sx, sy, ex - sx + 1, ey - sy + 1);

      p_updatingBuffers = true;
      if(p_grayBuffer){
        p_grayBuffer->fillBuffer(vpRect, data);
      }
      if(p_redBuffer){
        p_redBuffer->fillBuffer(vpRect, data);
      }
      if(p_greenBuffer){
        p_greenBuffer->fillBuffer(vpRect, data);
      }
      if(p_blueBuffer){
        p_blueBuffer->fillBuffer(vpRect, data);
      }
      p_updatingBuffers = false;

      paintPixmapRects();
    }

    emit doneWithData(cubeId, data);
  }


  /**
   * This method should be called during a close event that would cause this viewport to
   *   close. If changes have been made to this viewport it opens an
   *   information dialog that asks the user if they want to save,
   *   discard changes, or cancel.
   *
   * @return True if closing is OK, false if it needs to be cancelled.
   */
  bool CubeViewport::confirmClose() {
    bool canClose = true;
    if(p_saveEnabled) {
      // Enter == button 0, Escape == button 2
      switch(QMessageBox::information(this, tr("Confirm Save"),
        tr("The cube [<font color='red'>%1</font>] contains unsaved changes. "
           "Do you want to save the changes before exiting?").arg(cube()->fileName()),
           QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel)) {
        //Save changes and close viewport
        case QMessageBox::Save:
          emit saveChanges(this);
          break;

        //Discard changes and close viewport
        case QMessageBox::Discard:
          emit discardChanges(this);
          break;

        //Cancel, don't close viewport
        case QMessageBox::Cancel:
        default:
          canClose = false;
          break;
      }
    }

//  if (canClose) emit viewportClosed(this);
    return canClose;
  }

  /**
   * This method is called when the cube has changed or changes
   * have been finalized.
   *
   * @param changed
   */
  void CubeViewport::cubeChanged(bool changed) {
    p_saveEnabled = changed;
  }

  /**
   *  Make viewports show up as 512 by 512
   *
   *
   * @return QSize
   */
  QSize CubeViewport::sizeHint() const {
    QSize s(512, 512);
    return s;
  }


  /**
   *  Change the scale of the cube to the given parameter value.  This method will
   *  reset the scale value if the value passed in is too large or too small.
   *
   * @param scale Value by which to scale the image.
   * @internal
   *  @history 2010-07-12 Jeannie Walldren - Changed scale maximum value from
   *                         hard-coded 16 to the max of the viewport width and
   *                         height.  Added scale minimum value of
   *                         1/min(samps,lines).
   */
  void CubeViewport::setScale(double scale) {
    // Sanitize the scale value
    if (scale == p_scale){
      return;
    }

    if (viewport()->width() && viewport()->height()) {
      // don't let zoom scale be larger than the viewport size
      double maxScale = max(viewport()->width(),viewport()->height());
      if (scale > maxScale) {
        scale = maxScale;
      }
      // don't let zoom scale be smaller than one pixel high/wide showing
      double minScale = 1.0 / (min(cubeSamples(),cubeLines()));
      if (scale < minScale) {
        scale = minScale;
      }
    }

    // Resize the scrollbars to reflect the new scale
    double samp, line;
    contentsToCube(horizontalScrollBar()->value(), verticalScrollBar()->value(),
                   samp, line);
    p_scale = scale;
    updateScrollBars(1, 1); // Setting to 1,1 make sure we don't have bad values

    // Now update the scroll bar value to the old line/sample
    int x, y;
    cubeToContents(samp, line, x, y);
    updateScrollBars(x, y);

    p_updatingBuffers = true;
    if (p_grayBuffer){
      p_grayBuffer->scaleChanged();
    }
    if (p_redBuffer){
      p_redBuffer->scaleChanged();
    }
    if (p_greenBuffer){
      p_greenBuffer->scaleChanged();
    }
    if (p_blueBuffer){
      p_blueBuffer->scaleChanged();
    }
    p_updatingBuffers = false;

    paintPixmapRects();

    // Notify other tools about the scale change
    emit scaleChanged();

    // Update the display
    setCaption();
    paintPixmap(); // Will get rid of old data in the display

    viewport()->repaint();
    emit screenPixelsChanged();
  }

  /**
   *  Change the scale of the cube after moving x,y to the center
   *
   *
   * @param scale
   * @param x
   * @param y
   */
  void CubeViewport::setScale(double scale, int x, int y) {
    double samp, line;
    viewportToCube(x, y, samp, line);
    setScale(scale, samp, line);
  }


  /**
   * Change the scale of the cube after moving samp/line to the
   * center
   *
   *
   * @param scale
   * @param sample
   * @param line
   */
  void CubeViewport::setScale(double scale, double sample, double line) {
    viewport()->setUpdatesEnabled(false);

    bool wasEnabled = false;

    if ((p_grayBuffer && p_grayBuffer->enabled()) ||
        (p_redBuffer && p_redBuffer->enabled())) {
      wasEnabled = true;
    }

    if (p_grayBuffer){
      p_grayBuffer->enable(false);
    }
    if (p_redBuffer){
      p_redBuffer->enable(false);
    }
    if (p_greenBuffer){
      p_greenBuffer->enable(false);
    }
    if (p_blueBuffer){
      p_blueBuffer->enable(false);
    }
    if (p_paintPixmap) {
      p_paintPixmap = false;
      setScale(scale);
      p_paintPixmap = true;
    }
    else {
      setScale(scale);
    }
    center(sample, line);

    if (p_grayBuffer){
      p_grayBuffer->enable(wasEnabled);
    }
    if (p_redBuffer){
      p_redBuffer->enable(wasEnabled);
    }
    if (p_greenBuffer){
      p_greenBuffer->enable(wasEnabled);
    }
    if (p_blueBuffer){
      p_blueBuffer->enable(wasEnabled);
    }

    paintPixmap();
    viewport()->setUpdatesEnabled(true);
    viewport()->update();
    emit screenPixelsChanged();
  }


  /**
   *  Bring the cube pixel under viewport x/y to the center
   *
   *
   * @param x
   * @param y
   */
  void CubeViewport::center(int x, int y) {
    double sample, line;
    viewportToCube(x, y, sample, line);
    center(sample, line);
  }


  /**
   * Bring the cube sample/line the center.
   *
   * @param sample Value of the sample to center on.
   * @param line Value of the line to center on.
   */
  void CubeViewport::center(double sample, double line) {
    int x, y;
    cubeToContents(sample, line, x, y);

    int panX = horizontalScrollBar()->value() - x;
    int panY = verticalScrollBar()->value() - y;

    updateScrollBars(x, y);

    p_updatingBuffers = true;
    if(p_grayBuffer){
      p_grayBuffer->pan(panX, panY);
    }
    if(p_redBuffer){
      p_redBuffer->pan(panX, panY);
    }
    if(p_greenBuffer){
      p_greenBuffer->pan(panX, panY);
    }
    if(p_blueBuffer){
      p_blueBuffer->pan(panX, panY);
    }
    p_updatingBuffers = false;

    paintPixmapRects();

    shiftPixmap(panX, panY);

    emit screenPixelsChanged();
  }


  /**
   * Goes through the list of requested paints, from the viewport
   * buffer, and paints them.
   */
  void CubeViewport::paintPixmapRects() {
    for(int rect = 0; rect < p_pixmapPaintRects->size(); rect++) {
      paintPixmap(*(*p_pixmapPaintRects)[rect]);
    }

    p_pixmapPaintRects->clear();
  }


  /**
   *  Convert a contents x/y to a cube sample/line (may be outside
   *  the cube)
   *
   *
   * @param x
   * @param y
   * @param sample
   * @param line
   */
  void CubeViewport::contentsToCube(int x, int y,
                                    double &sample, double &line) const {
    sample = x / p_scale;
    line = y / p_scale;
  }


  /**
   * Convert a viewport x/y to a cube sample/line (may be outside
   * the cube)
   *
   *
   * @param x
   * @param y
   * @param sample
   * @param line
   */
  void CubeViewport::viewportToCube(int x, int y,
                                    double &sample, double &line) const {
    x += horizontalScrollBar()->value();
    x -= viewport()->width() / 2;
    y += verticalScrollBar()->value();
    y -= viewport()->height() / 2;
    contentsToCube(x, y, sample, line);
  }


  /**
   * Convert a cube sample/line to a contents x/y (should not be
   * outside)
   *
   *
   * @param sample
   * @param line
   * @param x
   * @param y
   */
  void CubeViewport::cubeToContents(double sample, double line,
                                    int &x, int &y) const {
    x = (int)(sample * p_scale + 0.5);
    y = (int)(line * p_scale + 0.5);
  }


  /**
   * Convert a cube sample/line to a viewport x/y (may be outside
   * the viewport)
   *
   *
   * @param sample
   * @param line
   * @param x
   * @param y
   */
  void CubeViewport::cubeToViewport(double sample, double line,
                                    int &x, int &y) const {
    cubeToContents(sample, line, x, y);
    x -= horizontalScrollBar()->value();
    x += viewport()->width() / 2;
    y -= verticalScrollBar()->value();
    y += viewport()->height() / 2;
  }


  /**
   * Move the scrollbars by dx/dy screen pixels
   *
   *
   * @param dx
   * @param dy
   */
  void CubeViewport::scrollBy(int dx, int dy) {
    // Make sure we don't generate bad values outside of the scroll range
    int x = horizontalScrollBar()->value() + dx;
    if(x <= 1) {
      dx = 1 - horizontalScrollBar()->value();
    }
    else if(x >= horizontalScrollBar()->maximum()) {
      dx = horizontalScrollBar()->maximum() - horizontalScrollBar()->value();
    }

    // Make sure we don't generate bad values outside of the scroll range
    int y = verticalScrollBar()->value() + dy;
    if(y <= 1) {
      dy = 1 - verticalScrollBar()->value();
    }
    else if(y >= verticalScrollBar()->maximum()) {
      dy = verticalScrollBar()->maximum() - verticalScrollBar()->value();
    }

    // Do we do anything?
    if((dx == 0) && (dy == 0)){
      return;
    }

    // We do so update the scroll bars
    updateScrollBars(horizontalScrollBar()->value() + dx,
                     verticalScrollBar()->value() + dy);

    // Scroll the the pixmap
    scrollContentsBy(-dx, -dy);
  }


  /**
   * Scroll the viewport contents by dx/dy screen pixels
   *
   *
   * @param dx
   * @param dy
   */
  void CubeViewport::scrollContentsBy(int dx, int dy) {

    // We shouldn't do anything if scrollbars are being updated
    if(viewport()->signalsBlocked()) {
      return;
    }

    // Tell the buffers to update appropriate for the pan and upon completion
    //   they will call bufferUpdated. We don't want bufferUpdated to happen
    //   until afterwards. If only shrinking and no new data then the viewport
    //   buffers will complete immediately. When all buffers don't have their
    //   appropriate actions queued bufferUpdated can't succeed.

    // Also block paints because we'll repaint it all when we're done
    //   telling each buffer about the pan.
    bool panQueued = false;
    QRect bufferXYRect;

    p_updatingBuffers = true;

    if(p_grayBuffer) {
      p_grayBuffer->pan(dx, dy);
      panQueued |= p_grayBuffer->working();
      bufferXYRect = p_grayBuffer->bufferXYRect();
    }

    if(p_redBuffer) {
      p_redBuffer->pan(dx, dy);
      panQueued |= p_redBuffer->working();
      bufferXYRect = p_redBuffer->bufferXYRect();
    }

    if(p_greenBuffer) {
      p_greenBuffer->pan(dx, dy);
      panQueued |= p_greenBuffer->working();
    }

    if(p_blueBuffer) {
      p_blueBuffer->pan(dx, dy);
      panQueued |= p_blueBuffer->working();
    }

    p_updatingBuffers = false;

    // shift the pixmap by x,y if the viewport buffer didn't do it immediately
    if(panQueued) {
      shiftPixmap(dx, dy);
    }
    else {
      // Need to do this to clear area outside of cube
      p_pixmapPaintRects->clear();
      paintPixmap();
    }

    viewport()->update();
    emit screenPixelsChanged();
  }


  /**
   * This restarts the progress bar. Does nothing if already
   * loading.
   */
  void CubeViewport::enableProgress() {
    if(!p_progressTimer->isActive()) {
      p_progressTimer->start();

      emit progressChanged(0);
    }
  }


  /**
   * Change the caption on the viewport title bar
   *
   */
  void CubeViewport::setCaption() {
    QString cubeFileName = p_cube->fileName();
    QString str = QFileInfo(cubeFileName).fileName();
    str += QString(" @ ");
    str += QString::number(p_scale * 100.0);
    str += QString("% ");

    if(p_color) {
      str += QString("(RGB = ");
      str += QString::number(p_red.band);
      str += QString(",");
      str += QString::number(p_green.band);
      str += QString(",");
      str += QString::number(p_blue.band);
      str += QString(")");
    }
    else {
      str += QString("(Gray = ");
      str += QString::number(p_gray.band);
      str += QString(")");
    }

    //If changes have been made make sure to add '*' to the end
    if(p_saveEnabled) {
      str += "*";
    }

    parentWidget()->setWindowTitle(str);
    emit windowTitleChanged();
  }


  /**
   * The viewport is being resized
   *
   *
   * @param e
   */
  void CubeViewport::resizeEvent(QResizeEvent *e) {
    p_paintPixmap = false;

    // Tell the buffers to update their coefficients to reinitialize
    //   and have their immediate paint events happen afterwards. Should not
    //   happen, but if buffers have actions complete immediately and call
    //   bufferUpdated. We don't want to have bufferUpdated ever when all
    //   buffers don't have their appropriate actions queued. It can't succeed.
    p_updatingBuffers = true;
    if(p_grayBuffer){
      p_grayBuffer->resizedViewport();
    }
    if(p_redBuffer){
      p_redBuffer->resizedViewport();
    }
    if(p_greenBuffer){
      p_greenBuffer->resizedViewport();
    }
    if(p_blueBuffer){
      p_blueBuffer->resizedViewport();
    }
    p_updatingBuffers = false;

    paintPixmapRects();

    // Change the size of the image and pixmap
    if(p_image) {
      delete p_image;
      p_image = NULL;
    }

    p_image = new QImage(viewport()->size(), QImage::Format_RGB32);
    p_pixmap = QPixmap(viewport()->size());

    p_paintPixmap = true;

    // Fixup the scroll bars
    updateScrollBars(horizontalScrollBar()->value(),
                     verticalScrollBar()->value());

    p_viewportWhatsThisText =
      "<p><b>Viewport Dimensions:</b> \
      <blockQuote>Samples = " +
      QString::number(viewport()->width()) + "<br>" +
      "Lines = " +
      QString::number(viewport()->height()) + "</blockquote></p>";

    // Repaint the pixmap
    paintPixmap();

    // Repaint the internal viewport and scroll bars
    viewport()->update();
    emit screenPixelsChanged();
  }


  /**
   * Repaint the viewport
   *
   * @param e [in]  (QPaintEvent *)  event
   *
   * @internal
   *
   * @history  2007-04-30  Tracie Sucharski - Add the QPainter to the call to
   *                           Tool::paintViewport.
   */
  void CubeViewport::paintEvent(QPaintEvent *e) {
    if(!p_cubeShown || !viewport()->isVisible()){
      return;
    }
  }


  /**
   * This method is called by ViewportBuffer upon successful
   * completion of all operations and gives the appropriate rect
   * to be repainted. This is intended to update the screen once
   * all data is done and ready to be displayed.
   *
   * @param rect Area to update screen
   */
  void CubeViewport::bufferUpdated(QRect rect) {
    paintPixmap(rect);

    // Don't repaint from buffer updates if any buffers are working...
    // This set of statements fixes a flash of black when in RGB mode
    //   and a pan (or other operation?) completes. What would happen is
    //   the first would reset to black then other buffers still working so
    //   not restored to DN values.
    if(p_grayBuffer && p_grayBuffer->working()){
      return;
    }
    if(p_redBuffer && p_redBuffer->working()){
      return;
    }
    if(p_greenBuffer && p_greenBuffer->working()){
      return;
    }
    if(p_blueBuffer && p_blueBuffer->working()){
      return;
    }

    viewport()->repaint(rect);
  }

//! Paint the whole pixmap
  void CubeViewport::paintPixmap() {
    QRect rect(0, 0, p_image->width(), p_image->height());
    paintPixmap(rect);
  }


  /**
   * Paint a region of the pixmap
   *
   *
   * @param rect
   */
  void CubeViewport::paintPixmap(QRect rect) {
    if(!p_paintPixmap) {
      return;
    }

    if(p_updatingBuffers) {
      p_pixmapPaintRects->push_back(new QRect(rect));
      return;
    }

    if(p_pixmap.isNull()){
      return;
    }

    QPainter p(&p_pixmap);

    p.fillRect(rect, QBrush(p_bgColor));

    QRect dataArea;

    if(p_grayBuffer && p_grayBuffer->enabled()) {
      if(p_grayBuffer->working()){
        return;
      }

      dataArea = QRect(p_grayBuffer->bufferXYRect().intersected(rect));

      for(int y = dataArea.top();
          !dataArea.isNull() && y <= dataArea.bottom();
          y++) {
        const vector< double > & line =
          p_grayBuffer->getLine(y - p_grayBuffer->bufferXYRect().top());

        if(line.size() == 0) {
          break;
        }

        if(y >= p_image->height()) {
          throw IException(IException::Programmer, "y too big", _FILEINFO_);
        }

        QRgb *rgb = (QRgb *) p_image->scanLine(y);

        CubeStretch redStretch = p_red.getStretch();
        CubeStretch greenStretch = p_green.getStretch();
        CubeStretch blueStretch = p_blue.getStretch();

        for(int x = dataArea.left(); x <= dataArea.right(); x++) {
          int bufferLeft = p_grayBuffer->bufferXYRect().left();
          int bufferX = x - bufferLeft;

          if(bufferX >= (int)line.size()){
            break;
          }

          if(bufferX < 0) {
            throw IException(IException::Programmer, "bufferX < 0", _FILEINFO_);
          }

          if(x >= p_image->width()) {
            throw IException(IException::Programmer, "x too big", _FILEINFO_);
          }

          double bufferVal = line.at(bufferX);

          // This is still RGB; the pairs are identical but the boundary
          //   conditions are different. Display saturations cause this.
          int redPix = (int)(redStretch.Map(bufferVal) + 0.5);
          int greenPix = (int)(greenStretch.Map(bufferVal) + 0.5);
          int bluePix = (int)(blueStretch.Map(bufferVal) + 0.5);
          rgb[x] =  qRgb(redPix, greenPix, bluePix);
        }
      }
    }
    else {
      if(p_redBuffer && p_redBuffer->enabled()) {
        if(p_redBuffer->working() || p_greenBuffer->working() ||
            p_blueBuffer->working()) {
          return;
        }


        if((p_greenBuffer->bufferXYRect().top() !=
            p_redBuffer->bufferXYRect().top()) ||
            (p_greenBuffer->bufferXYRect().top() !=
             p_blueBuffer->bufferXYRect().top())) {
          throw IException(IException::Programmer,
                           "Buffer rects mismatched",
                           _FILEINFO_);
        }

        if((p_greenBuffer->bufferXYRect().left() !=
            p_redBuffer->bufferXYRect().left()) ||
            (p_greenBuffer->bufferXYRect().left() !=
             p_blueBuffer->bufferXYRect().left())) {
          throw IException(IException::Programmer,
                           "Buffer rects mismatched",
                           _FILEINFO_);
        }

        dataArea = QRect(p_redBuffer->bufferXYRect().intersected(rect));

        for(int y = dataArea.top();
            !dataArea.isNull() && y <= dataArea.bottom();
            y++) {
          int bufferLine = y - p_redBuffer->bufferXYRect().top();

          const vector<double> &redLine   = p_redBuffer->getLine(bufferLine);
          const vector<double> &greenLine = p_greenBuffer->getLine(bufferLine);
          const vector<double> &blueLine  = p_blueBuffer->getLine(bufferLine);

          if((int)redLine.size() < dataArea.width() ||
              (int)greenLine.size() < dataArea.width() ||
              (int)blueLine.size() < dataArea.width()) {
            throw IException(IException::Programmer,
                             "Empty buffer line",
                             _FILEINFO_);
          }

          QRgb *rgb = (QRgb *) p_image->scanLine(y);

          CubeStretch redStretch = p_red.getStretch();
          CubeStretch greenStretch = p_green.getStretch();
          CubeStretch blueStretch = p_blue.getStretch();

          for(int x = dataArea.left(); x <= dataArea.right(); x++) {
            int redPix = (int)(redStretch.Map(redLine[ x - p_redBuffer->bufferXYRect().left()]) + 0.5);
            int greenPix = (int)(greenStretch.Map(greenLine[ x - p_greenBuffer->bufferXYRect().left()]) + 0.5);
            int bluePix = (int)(blueStretch.Map(blueLine[ x - p_blueBuffer->bufferXYRect().left()]) + 0.5);

            rgb[x] = qRgb(redPix, greenPix, bluePix);
          }
        }
      }
    }

    if(!dataArea.isNull()){
      p.drawImage(dataArea.topLeft(), *p_image, dataArea);
    }

    // Change whats this info
    updateWhatsThis();
  }


  /**
   * Shifts the pixels on the pixmap without reading new data.
   *
   * @param dx
   * @param dy
   */
  void CubeViewport::shiftPixmap(int dx, int dy) {
    if(!p_paintPixmap || !p_pixmap){
      return;
    }

    // Prep to scroll the pixmap
    int drawStartX = dx;
    int pixmapStartX = 0;
    if(drawStartX < 0) {
      drawStartX = 0;
      pixmapStartX = -dx;
    }

    int drawStartY = dy;
    int pixmapStartY = 0;
    if(dy < 0) {
      drawStartY = 0;
      pixmapStartY = -dy;
    }

    // Ok we can shift the pixmap and filling
    int pixmapDrawWidth  = p_pixmap.width()  - pixmapStartX + 1;
    int pixmapDrawHeight = p_pixmap.height() - pixmapStartY + 1;

    QRect rect(0, 0, p_pixmap.width(), p_pixmap.height());
    QPixmap pixmapCopy   = p_pixmap.copy();

    QPainter painter(&p_pixmap);
    painter.fillRect(rect, QBrush(p_bgColor));
    painter.drawPixmap(drawStartX, drawStartY,
                       pixmapCopy,
                       pixmapStartX, pixmapStartY,
                       pixmapDrawWidth, pixmapDrawHeight);
    painter.end();

    // Now fill in the left or right side
    QRect xFillRect;
    QRect yFillRect;

    if(dx > 0) {
      xFillRect = QRect(QPoint(0, 0),
                        QPoint(dx, p_pixmap.height()));
    }
    else {
      if(dx < 0){
        xFillRect = QRect(QPoint(p_pixmap.width() + dx, 0),
                          QPoint(p_pixmap.width(), p_pixmap.height()));
      }
    }

    // Fill in the top or bottom side
    if(dy > 0) {
      yFillRect = QRect(QPoint(0, 0),
                        QPoint(p_pixmap.width(), dy));
    }
    else {
      if(dy < 0){
        yFillRect = QRect(QPoint(0, p_pixmap.height() + dy),
                          QPoint(p_pixmap.width(), p_pixmap.height()));
      }
    }

    if(dx != 0) {
      paintPixmap(xFillRect);
    }

    if(dy != 0) {
      paintPixmap(yFillRect);
    }

    viewport()->update();
  }

  /**
   * Get All WhatsThis info - viewport, cube, area in PVL format
   *
   * @author Sharmila Prasad (4/5/2011)
   *
   * @param pWhatsThisPvl - Pvl for all whatsthis info
   */
  void CubeViewport::getAllWhatsThisInfo(Pvl & pWhatsThisPvl)
  {
    // Get Cube Info
    PvlObject whatsThisObj = PvlObject("WhatsThis");
    whatsThisObj += PvlKeyword("Cube", p_cube->fileName().toStdString());

    PvlGroup cubeGrp("CubeDimensions");
    cubeGrp += PvlKeyword("Samples", std::to_string(p_cube->sampleCount()));
    cubeGrp += PvlKeyword("Lines",   std::to_string(p_cube->lineCount()));
    cubeGrp += PvlKeyword("Bands",   std::to_string(p_cube->bandCount()));
    whatsThisObj += cubeGrp;

    // Get Viewport Info
    PvlGroup viewportGrp("ViewportDimensions");
    viewportGrp += PvlKeyword("Samples", std::to_string(viewport()->width()));
    viewportGrp += PvlKeyword("Lines",   std::to_string(viewport()->height()));
    whatsThisObj += viewportGrp;

    // Get Cube area Info
    PvlObject cubeAreaPvl("CubeArea");
    PvlGroup bandGrp("Bands");

    PvlKeyword filterName;
    getBandFilterName(filterName);
    int iFilterSize = filterName.size();

    // color
    if(p_color ) {
      PvlKeyword virtualKey("Virtual"), physicalKey("Physical"), filterNameKey;
      int iRedBand   = p_redBuffer->getBand();
      int iGreenBand = p_greenBuffer->getBand();
      int iBlueBand  = p_blueBuffer->getBand();

      bandGrp += PvlKeyword("Color", "RGB");

      virtualKey = std::to_string(iRedBand);
      virtualKey += std::to_string(iGreenBand);
      virtualKey += std::to_string(iBlueBand);
      bandGrp   += virtualKey;

      physicalKey =  std::to_string(p_cube->physicalBand(iRedBand));
      physicalKey += std::to_string(p_cube->physicalBand(iGreenBand));
      physicalKey += std::to_string(p_cube->physicalBand(iBlueBand));
      bandGrp += physicalKey;

      if(iFilterSize) {
        if(iRedBand <= iFilterSize) {
          filterNameKey += filterName[iRedBand-1];
        }
        else {
          filterNameKey += "None";
        }

        if(iGreenBand <= iFilterSize) {
          filterNameKey += filterName[iGreenBand-1];
        }
        else {
          filterNameKey += "None";
        }

        if(iBlueBand <= iFilterSize) {
          filterNameKey += filterName[iBlueBand-1];
        }
        else {
          filterNameKey += "None";
        }
        bandGrp += filterNameKey;
      }
    }
    else { // gray
      int iGrayBand = p_grayBuffer->getBand();

      bandGrp  += PvlKeyword("Color", "Gray");

      bandGrp  += PvlKeyword("Virtual", std::to_string(iGrayBand));
      bandGrp  += PvlKeyword("Physical", std::to_string(p_cube->physicalBand(iGrayBand)));

      if(iFilterSize && iGrayBand <= iFilterSize) {
        bandGrp  += PvlKeyword("FilterName", filterName[iGrayBand-1]);
      }
    }

    //start, end  line and sample
    double sl, ss, es, el;
    getCubeArea(ss, es, sl, el);
    cubeAreaPvl += PvlKeyword("StartSample", std::to_string(int(ss + 0.5)));
    cubeAreaPvl += PvlKeyword("EndSample",   std::to_string(int(es + 0.5)));
    cubeAreaPvl += PvlKeyword("StartLine",   std::to_string(int(sl + 0.5)));
    cubeAreaPvl += PvlKeyword("EndLine",     std::to_string(int(el + 0.5)));
    cubeAreaPvl += bandGrp;
    whatsThisObj += cubeAreaPvl;
    pWhatsThisPvl += whatsThisObj;
  }

  /**
   * Get Band Filter name from the Isis cube label
   *
   * @author Sharmila Prasad (4/5/2011)
   *
   * @param pFilterNameKey - FilterName keyword containing the
   *              corresponding keyword from the Isis Cube label
   */
  void CubeViewport::getBandFilterName(PvlKeyword & pFilterNameKey)
  {
    // get the band info
    Pvl* cubeLbl = p_cube->label();
    PvlObject isisObj = cubeLbl->findObject("IsisCube");
    if (isisObj.hasGroup("BandBin")) {
      PvlGroup bandBinGrp = isisObj.findGroup("BandBin");
      if(bandBinGrp.hasKeyword("FilterName")) {
        pFilterNameKey =bandBinGrp.findKeyword("FilterName") ;
      }
    }
  }

  /**
   * Get Cube area corresponding to the viewport's dimension
   *
   * @param pdStartSample - Cube Start Sample
   * @param pdEndSample   - Cube End Sample
   * @param pdStartLine   - Cube Start Line
   * @param pdEndLine     - Cube End Line
   */
  void CubeViewport::getCubeArea(double & pdStartSample, double & pdEndSample,
                                 double & pdStartLine, double & pdEndLine)
  {
    viewportToCube(0, 0, pdStartSample, pdStartLine);
    if(pdStartSample < 1.0){
      pdStartSample = 1.0;
    }
    if(pdStartLine < 1.0){
      pdStartLine = 1.0;
    }

    //end line and samples
    viewportToCube(viewport()->width() - 1, viewport()->height() - 1, pdEndSample, pdEndLine);
    if(pdEndSample > cubeSamples()){
      pdEndSample = cubeSamples();
    }
    if(pdEndLine > cubeLines()){
      pdEndLine = cubeLines();
    }
  }

  /**
   * Update the What's This text.
   *
   */
  void CubeViewport::updateWhatsThis() {
    //start, end  line and sample
    double sl, ss, es, el;
    getCubeArea(ss, es, sl, el);

    QString sBandInfo ;
    PvlKeyword filterNameKey;
    getBandFilterName(filterNameKey);
    int iFilterSize = filterNameKey.size();

    // color
    if(p_color ) {
      int iRedBand   = p_redBuffer->getBand();
      int iGreenBand = p_greenBuffer->getBand();
      int iBlueBand  = p_blueBuffer->getBand();

      sBandInfo = "Bands(RGB)&nbsp;Virtual  = " +
          QString::number(iRedBand) + ", ";
      sBandInfo += QString::number(iGreenBand) + ", ";
      sBandInfo += QString::number(iBlueBand) + " ";

      sBandInfo += "Physical = " +
          QString::number(p_cube->physicalBand(iRedBand)) + ", ";
      sBandInfo += QString::number(p_cube->physicalBand(iGreenBand)) + ", ";
      sBandInfo += QString::number(p_cube->physicalBand(iBlueBand));

      if(iFilterSize) {
        sBandInfo += "<br>FilterName = ";
        if(iRedBand <= iFilterSize) {
          sBandInfo += QString::fromStdString(filterNameKey[iRedBand-1]);
        }
        else {
          sBandInfo += "None";
        }
        sBandInfo += ", ";

        if(iGreenBand <= iFilterSize) {
          sBandInfo += QString::fromStdString(filterNameKey[iGreenBand-1]);
        }
        else {
          sBandInfo += "None";
        }
        sBandInfo += ", ";

        if(iBlueBand <= iFilterSize) {
          sBandInfo += QString::fromStdString(filterNameKey[iBlueBand-1]);
        }
        else {
          sBandInfo += "None";
        }
      }
    }
    else { // gray
      int iGrayBand = p_grayBuffer->getBand();

      sBandInfo = "Band(Gray)&nbsp;Virtual = " + QString::number(iGrayBand) + " ";

      sBandInfo += "Physical = " + QString::number(p_cube->physicalBand(iGrayBand));

      if(iFilterSize && iGrayBand <= iFilterSize) {
        sBandInfo += "<br>FilterName = " + QString::fromStdString(filterNameKey[iGrayBand-1]);
      }
    }

    QString area =
      "<p><b>Visible Cube Area:</b><blockQuote> \
      Samples = " + QString::number(int(ss + 0.5)) + "-" +
      QString::number(int(es + 0.5)) + "<br> \
      Lines = " + QString::number(int(sl + 0.5)) + "-" +
      QString::number(int(el + 0.5)) + "<br> " +
      sBandInfo + "</blockQuote></p>";

    QString fullWhatsThis = p_whatsThisText + area + p_cubeWhatsThisText + p_viewportWhatsThisText;
    setWhatsThis(fullWhatsThis);
    viewport()->setWhatsThis(fullWhatsThis);
  }

  /**
   * Return the red pixel value at a sample/line
   *
   *
   * @param sample
   * @param line
   *
   * @return double
   */
  double CubeViewport::redPixel(int sample, int line) {
    p_pntBrick->SetBasePosition(sample, line, p_red.band);
    p_cube->read(*p_pntBrick);
    return (*p_pntBrick)[0];
  }


  /**
   * Return the green pixel value at a sample/line
   *
   *
   * @param sample
   * @param line
   *
   * @return double
   */
  double CubeViewport::greenPixel(int sample, int line) {
    p_pntBrick->SetBasePosition(sample, line, p_green.band);
    p_cube->read(*p_pntBrick);
    return (*p_pntBrick)[0];
  }


  /**
   * Return the blue pixel value at a sample/line
   *
   *
   * @param sample
   * @param line
   *
   * @return double
   */
  double CubeViewport::bluePixel(int sample, int line) {
    p_pntBrick->SetBasePosition(sample, line, p_blue.band);
    p_cube->read(*p_pntBrick);
    return (*p_pntBrick)[0];
  }


  /**
   * Return the gray pixel value at a sample/line
   *
   *
   * @param sample
   * @param line
   *
   * @return double
   */
  double CubeViewport::grayPixel(int sample, int line) {
    p_pntBrick->SetBasePosition(sample, line, p_gray.band);
    p_cube->read(*p_pntBrick);
    return (*p_pntBrick)[0];
  }


  //! Return the gray band stretch
  CubeStretch CubeViewport::grayStretch() const {
    return p_gray.getStretch();
  }


  //! Return the red band stretch
  CubeStretch CubeViewport::redStretch() const {
    return p_red.getStretch();
  };


  //! Return the green band stretch
  CubeStretch CubeViewport::greenStretch() const {
    return p_green.getStretch();
  };


  //! Return the blue band stretch
  CubeStretch CubeViewport::blueStretch() const {
    return p_blue.getStretch();
  };


  /**
   * Event filter to watch for mouse events on viewport
   *
   *
   * @param o
   * @param e
   *
   * @return bool
   */
  bool CubeViewport::eventFilter(QObject *o, QEvent *e) {
    // Handle standard mouse tracking on the viewport
    if(o == viewport()) {
      switch(e->type()) {
        case QEvent::Enter: {
          viewport()->setMouseTracking(true);
          emit mouseEnter();
          return true;
        }

        case QEvent::MouseMove: {
          QMouseEvent *m = (QMouseEvent *) e;
          emit mouseMove(m->pos());
          emit mouseMove(m->pos(), (Qt::MouseButton)(m->button() +
                                    m->modifiers()));
          return true;
        }

        case QEvent::Leave: {
          viewport()->setMouseTracking(false);
          emit mouseLeave();
          return true;
        }

        case QEvent::MouseButtonPress: {
          QMouseEvent *m = (QMouseEvent *) e;
          emit mouseButtonPress(m->pos(),
                                (Qt::MouseButton)(m->button() + m->modifiers()));
          return true;
        }

        case QEvent::MouseButtonRelease: {
          QMouseEvent *m = (QMouseEvent *) e;
          emit mouseButtonRelease(m->pos(),
                                  (Qt::MouseButton)(m->button() + m->modifiers()));
          return true;
        }

        case QEvent::MouseButtonDblClick: {
          QMouseEvent *m = (QMouseEvent *) e;
          emit mouseDoubleClick(m->pos());
          return true;
        }

        default: {
          return false;
        }
      }
    }
    else {
      return QAbstractScrollArea::eventFilter(o, e);
    }
  }


  /**
   * Process arrow keystrokes on cube
   *
   *
   * @param e
   *
   * @history  2017-08-11 Adam Goins - Added the ability to ctrl + c to copy the filename
   *                             of the current cube into the system's clipboard.
   *                             Fixes #5098.
   */
  void CubeViewport::keyPressEvent(QKeyEvent *e) {
    if(e->key() == Qt::Key_Plus) {
      double scale = p_scale * 2.0;
      setScale(scale);
      e->accept();
    }
    else if(e->key() == Qt::Key_Minus) {
      double scale = p_scale / 2.0;
      setScale(scale);
      e->accept();
    }
    else if(e->key() == Qt::Key_Up) {
      moveCursor(0, -1);
      e->accept();
    }
    else if(e->key() == Qt::Key_Down) {
      moveCursor(0, 1);
      e->accept();
    }
    else if(e->key() == Qt::Key_Left) {
      moveCursor(-1, 0);
      e->accept();
    }
    else if(e->key() == Qt::Key_Right) {
      moveCursor(1, 0);
      e->accept();
    }
    else if ((e->key() == Qt::Key_C) &&
             QApplication::keyboardModifiers() &
             Qt::ControlModifier) {

      //QString fileName = p_cube->fileName();
      QFileInfo fileName = p_cube->fileName();

      // Grabs the clipboard and copies the file name into it.
      QClipboard *clipboard = QApplication::clipboard();
      clipboard->setText(fileName.absoluteFilePath());
    }
    else {
      QAbstractScrollArea::keyPressEvent(e);
    }
  }


  /**
   * Is cursor inside viewport
   *
   *
   * @return bool
   */
  bool CubeViewport::cursorInside() const {
    QPoint g = QCursor::pos();
    QPoint v = viewport()->mapFromGlobal(g);
    if(v.x() < 0){
      return false;
    }
    if(v.y() < 0){
      return false;
    }
    if(v.x() >= viewport()->width()){
      return false;
    }
    if(v.y() >= viewport()->height()){
      return false;
    }
    return true;
  }


  /**
   * Return the cursor position in the viewport
   *
   *
   * @return QPoint
   */
  QPoint CubeViewport::cursorPosition() const {
    QPoint g = QCursor::pos();
    return viewport()->mapFromGlobal(g);
  }


  /**
   * Move the cursor by x,y if possible
   *
   *
   * @param x
   * @param y
   */
  void CubeViewport::moveCursor(int x, int y) {
    QPoint g = QCursor::pos();
    g += QPoint(x, y);
    QPoint v = viewport()->mapFromGlobal(g);
    if(v.x() < 0){
      return;
    }
    if(v.y() < 0){
      return;
    }
    if(v.x() >= viewport()->width()){
      return;
    }
    if(v.y() >= viewport()->height()){
      return;
    }
    QCursor::setPos(g);
  }


  /**
   * Set the cursor position to x/y in the viewport
   *
   *
   * @param x
   * @param y
   */
  void CubeViewport::setCursorPosition(int x, int y) {
    QPoint g(x, y);
    QPoint v = viewport()->mapToGlobal(g);
    QCursor::setPos(v);
  }


  /**
   * Update the scroll bar
   *
   *
   * @param x
   * @param y
   */
  void CubeViewport::updateScrollBars(int x, int y) {
    viewport()->blockSignals(true);

    verticalScrollBar()->setValue(1);
    verticalScrollBar()->setMinimum(1);
    verticalScrollBar()->setMaximum((int)(ceil(cubeLines() * p_scale) + 0.5));
    verticalScrollBar()->setPageStep(viewport()->height() / 2);

    horizontalScrollBar()->setValue(1);
    horizontalScrollBar()->setMinimum(1);
    horizontalScrollBar()->setMaximum((int)(ceil(cubeSamples() * p_scale) + 0.5));
    horizontalScrollBar()->setPageStep(viewport()->width() / 2);

    if(horizontalScrollBar()->value() != x || verticalScrollBar()->value() != y) {
      horizontalScrollBar()->setValue(x);
      verticalScrollBar()->setValue(y);
      emit scaleChanged();
    }

    QApplication::sendPostedEvents(viewport(), 0);
    viewport()->blockSignals(false);
  }


  /**
   * View cube as gray
   *
   *
   * @param band
   */
  void CubeViewport::viewGray(int band) {
    p_gray.band = band;
    p_color = false;
    setCaption();

    if(!p_grayBuffer){
      p_grayBuffer = new ViewportBuffer(this, p_cubeData,
                                        p_cubeId);
    }

    if(p_redBuffer){
      delete p_redBuffer;
    }
    p_redBuffer = NULL;

    if(p_greenBuffer){
      delete p_greenBuffer;
    }
    p_greenBuffer = NULL;

    if(p_blueBuffer){
      delete p_blueBuffer;
    }
    p_blueBuffer = NULL;

    if(p_grayBuffer->getBand() != band) {
      int oldBand = p_grayBuffer->getBand();

      if(oldBand >= 0) {
        if((*p_knownStretches)[oldBand - 1]) {
          delete(*p_knownStretches)[oldBand - 1];
        }

        (*p_knownStretches)[oldBand - 1] = new Stretch(p_gray.getStretch());
      }

      p_grayBuffer->setBand(band);
      p_gray.band = band;

      if((*p_knownStretches)[band - 1]) {
        stretchGray(*(*p_knownStretches)[band - 1]);
      }
      else {
        p_grayBuffer->addStretchAction();
      }
    }

    if(p_camera) {
      p_camera->SetBand(band);
    }

    viewport()->repaint();
  }


  void CubeViewport::forgetStretches() {
    for(int stretch = 0; stretch < p_knownStretches->size(); stretch++) {
      if((*p_knownStretches)[stretch]) {
        delete(*p_knownStretches)[stretch];
        (*p_knownStretches)[stretch] = NULL;
      }
    }
  }


  void CubeViewport::setAllBandStretches(Stretch stretch) {
    for(int index = 0; index < p_knownStretches->size(); index ++) {
      if((*p_knownStretches)[index]) {
        delete(*p_knownStretches)[index];
      }

      (*p_knownStretches)[index] = new Stretch(stretch);
    }
  }


  /**
   * View cube as color
   *
   *
   * @param rband
   * @param gband
   * @param bband
   */
  void CubeViewport::viewRGB(int rband, int gband, int bband) {
    p_red.band = rband;
    p_green.band = gband;
    p_blue.band = bband;
    p_color = true;
    setCaption();

    if(!p_redBuffer) {
      p_redBuffer = new ViewportBuffer(this, p_cubeData, p_cubeId);
    }

    if(!p_greenBuffer) {
      p_greenBuffer = new ViewportBuffer(this, p_cubeData, p_cubeId);
    }

    if(!p_blueBuffer) {
      p_blueBuffer = new ViewportBuffer(this, p_cubeData, p_cubeId);
    }

    if(p_redBuffer->getBand() != rband) {
      int oldBand = p_redBuffer->getBand();

      // Remember current stretch for future band changes
      if(oldBand >= 0) {
        if((*p_knownStretches)[oldBand - 1]) {
          delete(*p_knownStretches)[oldBand - 1];
        }

        (*p_knownStretches)[oldBand - 1] = new Stretch(p_red.getStretch());
      }

      p_redBuffer->setBand(rband);
      p_red.band = rband;

      if((*p_knownStretches)[rband - 1]) {
        p_red.setStretch(*(*p_knownStretches)[rband - 1]);
      }
      else {
        p_redBuffer->addStretchAction();
      }
    }

    if(p_greenBuffer->getBand() != gband) {
      int oldBand = p_greenBuffer->getBand();

      // Remember current stretch for future band changes
      if(oldBand >= 0) {
        if((*p_knownStretches)[oldBand - 1]) {
          delete(*p_knownStretches)[oldBand - 1];
        }

        (*p_knownStretches)[oldBand - 1] = new Stretch(p_green.getStretch());
      }

      p_greenBuffer->setBand(gband);
      p_green.band = gband;

      if((*p_knownStretches)[gband - 1]) {
        p_green.setStretch(*(*p_knownStretches)[gband - 1]);
      }
      else {
        p_greenBuffer->addStretchAction();
      }
    }

    if(p_blueBuffer->getBand() != bband) {
      int oldBand = p_blueBuffer->getBand();

      // Remember current stretch for future band changes
      if(oldBand >= 0) {
        if((*p_knownStretches)[oldBand - 1]) {
          delete(*p_knownStretches)[oldBand - 1];
        }

        (*p_knownStretches)[oldBand - 1] = new Stretch(p_blue.getStretch());
      }

      p_blueBuffer->setBand(bband);
      p_blue.band = bband;

      if((*p_knownStretches)[bband - 1]) {
        p_blue.setStretch(*(*p_knownStretches)[bband - 1]);
      }
      else {
        p_blueBuffer->addStretchAction();
      }
    }

    if(p_grayBuffer){
      delete p_grayBuffer;
    }
    p_grayBuffer = NULL;

    if(p_camera) {
      p_camera->SetBand(rband);
    }
  }


  /**
   *  Apply stretch pairs to gray band
   *
   *
   * @param string
   */
  void CubeViewport::stretchGray(const QString &string) {
    Stretch stretch;
    stretch.Parse(string);
    stretchGray(stretch);
  }


  /**
   * Apply stretch pairs to red bands
   *
   * @param string The stretch
   */
  void CubeViewport::stretchRed(const QString &string) {
    Stretch stretch;
    stretch.Parse(string);
    stretchRed(stretch);
  }


  /**
   * Apply stretch pairs to green bands
   *
   * @param string the stretch
   */
  void CubeViewport::stretchGreen(const QString &string) {
    Stretch stretch;
    stretch.Parse(string);
    stretchGreen(stretch);
  }


  /**
   * Apply stretch pairs to blue bands
   *
   * @param string
   */
  void CubeViewport::stretchBlue(const QString &string) {
    Stretch stretch;
    stretch.Parse(string);
    stretchBlue(stretch);
  }


  /**List<Tool *> p
   * This stretches to the global stretch
   */
  void CubeViewport::stretchKnownGlobal() {
    if(!p_globalStretches){
      return;
    }

    if(isGray()) {
      if((*p_globalStretches)[grayBand() - 1]) {
        stretchGray(*(*p_globalStretches)[grayBand() - 1]);
      }
    }
    else {
      if((*p_globalStretches)[redBand() - 1]) {
        stretchRed(*(*p_globalStretches)[redBand() - 1]);
      }

      if((*p_globalStretches)[greenBand() - 1]) {
        stretchGreen(*(*p_globalStretches)[greenBand() - 1]);
      }

      if((*p_globalStretches)[blueBand() - 1]) {
        stretchBlue(*(*p_globalStretches)[blueBand() - 1]);
      }
    }
  }


  /**
   * Sets the stretch for gray mode
   *
   * @param stretch
   */
  void CubeViewport::stretchGray(const Stretch &stretch) {
    // Assume first stretch is always the global stretch (and it should be)
    if((*p_globalStretches)[grayBand() - 1] == NULL && stretch.Pairs()) {
      (*p_globalStretches)[grayBand() - 1] = new Stretch(stretch);
    }

    p_gray.setStretch(stretch);

    Stretch newRed(p_red.getStretch());
    newRed.CopyPairs(stretch);
    p_red.setStretch(newRed);

    Stretch newGreen(p_green.getStretch());
    newGreen.CopyPairs(stretch);
    p_green.setStretch(newGreen);

    Stretch newBlue(p_blue.getStretch());
    newBlue.CopyPairs(stretch);
    p_blue.setStretch(newBlue);

    paintPixmap();
    viewport()->update();
  }


  /**
   * Sets the stretch for red in rgb mode
   *
   * @param stretch
   */
  void CubeViewport::stretchRed(const Stretch &stretch) {
    p_red.setStretch(stretch);

    // Assume first stretch is always the global stretch (and it should be)
    if((*p_globalStretches)[redBand() - 1] == NULL && stretch.Pairs()) {
      (*p_globalStretches)[redBand() - 1] = new Stretch(p_red.getStretch());
    }

    paintPixmap();
    viewport()->update();
  }


  /**
   * Sets the stretch for green in rgb mode
   *
   * @param stretch
   */
  void CubeViewport::stretchGreen(const Stretch &stretch) {
    p_green.setStretch(stretch);

    // Assume first stretch is always the global stretch (and it should be)
    if((*p_globalStretches)[greenBand() - 1] == NULL && stretch.Pairs()) {
      (*p_globalStretches)[greenBand() - 1] = new Stretch(p_green.getStretch());
    }

    paintPixmap();
    viewport()->update();
  }


  /**
   * Sets the stretch for blue in rgb mode
   *
   * @param stretch
   */
  void CubeViewport::stretchBlue(const Stretch &stretch) {
    p_blue.setStretch(stretch);

    // Assume first stretch is always the global stretch (and it should be)
    if((*p_globalStretches)[blueBand() - 1] == NULL && stretch.Pairs()) {
      (*p_globalStretches)[blueBand() - 1] = new Stretch(p_blue.getStretch());
    }

    paintPixmap();
    viewport()->update();
  }


  /**
   * Determine the scale that causes the full cube to fit in the viewport.
   *
   * @return The scale
   *
   * @internal
   */
  double CubeViewport::fitScale() const {
    double sampScale = (double) viewport()->width() / (double) cubeSamples();
    double lineScale = (double) viewport()->height() / (double) cubeLines();
    double scale = sampScale < lineScale ? sampScale : lineScale;
//    scale = floor(scale * 100.0) / 100.0;
    return scale;
  }


  /**
   * Determine the scale of cube in the width to fit in the viewport
   *
   * @return The scale for width
   *
   */
  double CubeViewport::fitScaleWidth() const {
    double scale = (double) viewport()->width() / (double) cubeSamples();

//    scale = floor(scale * 100.0) / 100.0;
    return scale;
  }

  /**
   * Determine the scale of cube in heighth to fit in the viewport
   *
   * @return The scale for height
   *
   */
  double CubeViewport::fitScaleHeight() const {
    double scale = (double) viewport()->height() / (double) cubeLines();

//    scale = floor(scale * 100.0) / 100.0;
    return scale;
  }

  /**
   * Cube changed, repaint given area
   *
   * @param rect Rectangle containing portion of cube
   *                                  (sample/line) that changed.
   *
   */
  void CubeViewport::cubeContentsChanged(QRect rect) {
    //start sample/line and end sample/line
    double ss, sl, es, el;
    ss = (double)(rect.left()) - 1.;
    sl = (double)(rect.top()) - 1.;
    es = (double)(rect.right()) + 1.;
    el = (double)(rect.bottom()) + 1.;
    if(ss < 1){
      ss = 0.5;
    }
    if(sl < 1){
      sl = 0.5;
    }
    if(es > cube()->sampleCount()){
      es = cube()->sampleCount() + 0.5;
    }
    if(el > cube()->lineCount()){
      el = cube()->lineCount() + 0.5;
    }

    //start x/y and end x/y
    int sx, sy, ex, ey;

    cubeToViewport(ss, sl, sx, sy);
    cubeToViewport(es, el, ex, ey);
    if(sx < 0){
      sx = 0;
    }
    if(sy < 0){
      sy = 0;
    }
    if(ex > viewport()->width()){
      ex = viewport()->width();
    }
    if(ey > viewport()->height()){
      ey = viewport()->height();
    }
    QRect vpRect(sx, sy, ex - sx + 1, ey - sy + 1);

    p_updatingBuffers = true;
    if(p_grayBuffer){
      p_grayBuffer->fillBuffer(vpRect);
    }
    if(p_redBuffer){
      p_redBuffer->fillBuffer(vpRect);
    }
    if(p_greenBuffer){
      p_greenBuffer->fillBuffer(vpRect);
    }
    if(p_blueBuffer){
      p_blueBuffer->fillBuffer(vpRect);
    }
    p_updatingBuffers = false;

    paintPixmapRects();
  }


/**
 * Finds the Tracking group from p_cube and stores the tracking cube name
 * so that we can grab it in AdvancedTrackTool and get mosaic information.
 * This way, we are not opening the tracking cube every time the cursor is moved.
 */
  void CubeViewport::setTrackingCube() {
    PvlGroup trackingGroup = p_cube->group("Tracking");
    //Because the tracking group does not have a path, get the path from the main cube
    FileName cubeName(p_cube->fileName());
    QString trackingCubeName = QString::fromStdString(trackingGroup.findKeyword("Filename")[0]);
    FileName trackingCubeFileName(cubeName.path() + "/" + trackingCubeName);
    Cube *trackingCube = new Cube(trackingCubeFileName);
    p_trackingCube = trackingCube;
  }


  /**
   * Allows users to change the cursor type on the viewport.
   *
   *
   * @param cursor
   */
  void CubeViewport::changeCursor(QCursor cursor) {
    viewport()->setCursor(cursor);
  }


  CubeViewport::BandInfo::BandInfo() : band(1), stretch(NULL) {
    stretch = new CubeStretch;
    stretch->SetNull(0.0);
    stretch->SetLis(0.0);
    stretch->SetLrs(0.0);
    stretch->SetHis(255.0);
    stretch->SetHrs(255.0);
    stretch->SetMinimum(0.0);
    stretch->SetMaximum(255.0);
  };


  CubeViewport::BandInfo::BandInfo(const CubeViewport::BandInfo &other) :
    band(other.band) {
    stretch = NULL;
    stretch = new CubeStretch(*other.stretch);
  }


  CubeViewport::BandInfo::~BandInfo() {
    if(stretch) {
      delete stretch;
      stretch = NULL;
    }
  }


  CubeStretch CubeViewport::BandInfo::getStretch() const {
    return *stretch;
  }


  void CubeViewport::BandInfo::setStretch(const Stretch &newStretch) {
    *stretch = newStretch;
  }


  const CubeViewport::BandInfo &CubeViewport::BandInfo::operator=(
    CubeViewport::BandInfo other) {

    stretch = NULL;
    stretch = new CubeStretch;
    *stretch = *other.stretch;
    band = other.band;

    return *this;
  }

}
