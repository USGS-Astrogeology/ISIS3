/** This is free and unencumbered software released into the public domain.

The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */

#include "ViewportBuffer.h"
#include "ViewportBufferAction.h"
#include "ViewportBufferStretch.h"
#include "ViewportBufferFill.h"
#include "ViewportBufferTransform.h"

#include <QApplication>
#include <QQueue>
#include <QRect>
#include <QScrollBar>

#include "Brick.h"
#include "CubeDataThread.h"
#include "CubeViewport.h"
#include "SpecialPixel.h"
#include "PixelType.h"


#define round(x) ((x) > 0.0 ? (x) + 0.5 : (x) - 0.5)


using namespace std;


namespace Isis {
  /**
   * ViewportBuffer constructor. Viewport and Cube can not be null.
   * Band is not initialized
   *
   * @param viewport viewport that will use the buffer
   * @param cubeData cube to read from
   * @param cubeId The identifier for the cube in the data thread
   */
  ViewportBuffer::ViewportBuffer(CubeViewport *viewport,
                                 CubeDataThread *cubeData,
                                 int cubeId) {
    p_dataThread = cubeData;
    p_cubeId = cubeId;

    p_actions = 0;

    p_actions = new QQueue< ViewportBufferAction * >();
    p_viewport = viewport;
    p_bufferInitialized = false;
    p_band = -1;
    p_enabled = true;
    p_initialStretchDone = false;
    p_viewportHeight = p_viewport->viewport()->height();
    p_oldViewportHeight = p_viewport->viewport()->height();
    p_vertScrollBarPos = p_viewport->verticalScrollBar()->value();
    p_oldVertScrollBarPos = p_viewport->verticalScrollBar()->value();

    p_requestedFillArea = 0.0;
    p_bricksOrdered = true;

    connect(this, SIGNAL(ReadCube(int, int, int, int, int, int, void *)),
            p_dataThread, SLOT(ReadCube(int, int, int, int, int, int, void *)));

    connect(p_dataThread, SIGNAL(ReadReady(void *, int, const Isis::Brick *)),
            this, SLOT(DataReady(void *, int, const Isis::Brick *)));

    connect(this, SIGNAL(DoneWithData(int, const Isis::Brick *)),
            p_dataThread, SLOT(DoneWithData(int, const Isis::Brick *)));
  }

  /**
   * Updates total buffer size on destruction.
   *
   */
  ViewportBuffer::~ViewportBuffer() {
    disconnect(this, SIGNAL(ReadCube(int, int, int, int, int, int, void *)),
               p_dataThread, SLOT(ReadCube(int, int, int, int, int, int, void *)));

    disconnect(p_dataThread, SIGNAL(ReadReady(void *, int, const Isis::Brick *)),
               this, SLOT(DataReady(void *, int, const Isis::Brick *)));

    disconnect(this, SIGNAL(DoneWithData(int, const Isis::Brick *)),
               p_dataThread, SLOT(DoneWithData(int, const Isis::Brick *)));

    p_dataThread = NULL;

    if(p_actions) {
      while(!p_actions->empty()) {
        ViewportBufferAction *action = p_actions->dequeue();
        if(action) {
          delete action;
          action = NULL;
        }
      }
      delete p_actions;
      p_actions = NULL;
    }

    emptyBuffer(true);
  }


  /**
   * This method will convert the rect to sample/line positions and read from
   * the cube into the buffer. The rect is in viewport pixels.
   *
   * @param rect
   */
  void ViewportBuffer::fillBuffer(QRect rect) {
    if(p_band == -1) {
      throw IException(IException::Programmer, "invalid band", _FILEINFO_);
    }

    ViewportBufferFill *newFill = createViewportBufferFill(
                                    rect.intersected(bufferXYRect()), false);
    enqueueAction(newFill);
    doQueuedActions();
  }


  /**
   * This method will convert the rect to sample/line positions and read from
   * the cube into the buffer. The rect is in viewport pixels.
   *
   * @param rect
   * @param data
   */
  void ViewportBuffer::fillBuffer(QRect rect, const Brick *data) {
    if(p_band == -1) {
      throw IException(IException::Programmer, "invalid band", _FILEINFO_);
    }

    rect = rect.intersected(bufferXYRect());

    if (!rect.isValid())
      return;

    try {
      ViewportBufferFill *fill = createViewportBufferFill(rect, false);

      while(fill->shouldRequestMore()) {
        fill->incRequestPosition();
        fill->incReadPosition();

        for(int x = rect.left(); x <= rect.right(); x ++) {
          // Index into internal buffer is minus leftmost/topmost pixel
          int xIndex = x - fill->getLeftmostPixelPosition();
          int yIndex = fill->getRequestPosition() -
          fill->getTopmostPixelPosition();

          double samp = fill->viewportToSample(x);
          double line = fill->viewportToLine(fill->getRequestPosition());
          if (samp < data->Sample())
            samp = data->Sample();
          if (samp > data->Sample() + data->SampleDimension())
            samp = data->Sample() + data->SampleDimension();
          if (line < data->Line())
            line = data->Line();
          if (line > data->Line() + data->LineDimension())
            line = data->Line() + data->LineDimension();

          // Index into buffer is current sample - start sample
            //   *Brick indices are in units of cube pixels, not screen pixels
          int brickIndex = data->Index((int)(samp + 0.5), (int)(line + 0.5), p_band);

            if(brickIndex < 0) {
              p_buffer.at(yIndex).at(xIndex) = data->at(0);
            }
            else if(brickIndex >= data->size()) {
              p_buffer.at(yIndex).at(xIndex) = data->at(data->size() - 1);
            }
            else {
              if(yIndex < 0 || xIndex < 0 || yIndex >= (int) p_buffer.size() ||
                xIndex >= (int) p_buffer.at(yIndex).size()) {
                throw IException(IException::Programmer,
                                 "index out of range",
                                 _FILEINFO_);
                }
                else {
                  p_buffer.at(yIndex).at(xIndex) = data->at(brickIndex);
                }
            }
        }
      }
    }
    catch (IException &e) {
      throw IException(e, IException::Programmer, "Failed to load brick "
                       "into buffer", _FILEINFO_);
    }
  }


  /**
   * This method is called when requested bricks become available.
   * This processes the new cube data and requests more if
   * necessary.
   *
   * @param requester
   * @param cubeId
   * @param brick
   */
  void ViewportBuffer::DataReady(void *requester, int cubeId,
                                 const Brick *brick) {
    if(this != requester)
      return;

    if(p_actions->empty()) {
      throw IException(IException::Programmer, "no actions", _FILEINFO_);
    }

    ViewportBufferAction *curAction = p_actions->head();

    if(curAction->getActionType() != ViewportBufferAction::fill ||
        !curAction->started()) {
      throw IException(IException::Programmer, "not a fill action", _FILEINFO_);
    }

    ViewportBufferFill *fill = (ViewportBufferFill *) curAction;

    const QRect *rect = fill->getRect();

    int y = fill->getReadPosition(); // screen line

    // check to see if the next screen line's brick differs from this screen
    // line's brick.  If it does, which brick do we use?
    int curBrickLine = (int) (fill->viewportToLine(y) + 0.5);
    int nextBrickLine = (int) (fill->viewportToLine(y + 1) + 0.5);
    bool brickOrderCorrection = p_bricksOrdered;
    if (curBrickLine != nextBrickLine &&
        nextBrickLine == (int) (brick->Line() + 0.5)) {
      y++;
      p_bricksOrdered = false;
    }
    else {
      p_bricksOrdered = true;
    }

    double samp;

    // Loop through x values of rect on screen that we want to fill
    for(int x = rect->left(); x <= rect->right(); x++) {
      // Index into internal buffer is minus leftmost/topmost pixel
      int xIndex = x - fill->getLeftmostPixelPosition();
      int yIndex = y - fill->getTopmostPixelPosition();

      samp = fill->viewportToSample(x);

      // Index into buffer is current sample - start sample
      //   *Brick indices are in units of cube pixels, not screen pixels
      int brickIndex = (int)(samp + 0.5) - brick->Sample();

      if(brickIndex < 0) {
        p_buffer.at(yIndex).at(xIndex) = brick->at(0);
      }
      else if(brickIndex  >= brick->size()) {
        p_buffer.at(yIndex).at(xIndex) = brick->at(brick->size() - 1);
      }
      else {
        if(yIndex < 0 || xIndex < 0 || yIndex >= (int) p_buffer.size() ||
            xIndex >= (int) p_buffer.at(yIndex).size()) {
          IString msg = "An index out of range error was detected. ";

          if(yIndex < 0)
            msg += "The Y-Index [" + IString(yIndex) + "] is less than 0";
          else if(xIndex < 0)
            msg += "The X-Index [" + IString(xIndex) + "] is less than 0";
          else if(yIndex > (int)p_buffer.size())
            msg += "The Y-Index [" + IString(yIndex) + "] is greater than the "
                "Y-Size of [" + IString((int)p_buffer.size()) + "]";
          else if(xIndex > (int)p_buffer.at(yIndex).size())
            msg += "The X-Index [" + IString(xIndex) + " is greater than the "
                "X-Size of [" + IString((int) p_buffer.at(yIndex).size()) + "]";

          throw IException(IException::Programmer, msg, _FILEINFO_);
        }
        else {
          p_buffer.at(yIndex).at(xIndex) = brick->at(brickIndex);
        }
      }
    }
    fill->incReadPosition();


    if(fill->shouldRequestMore()) {
      if (p_bricksOrdered) {
        requestCubeLine(fill);
      }
      else {
        if (brickOrderCorrection) {
          requestCubeLine(fill);
          requestCubeLine(fill);
        }
      }
    }
    else if(fill->doneReading()) {
      delete fill;
      fill = NULL;
      p_actions->dequeue();
      doQueuedActions();
    }

    emit DoneWithData(cubeId, brick);
  }


  /**
   * This enqueues the given action. Please use this and don't put
   * actions directly into p_actions. Calling this method can be
   * thought of as equivalent to calling
   * "p_actions->push_back(action)"
   *
   * @param action
   */
  void ViewportBuffer::enqueueAction(ViewportBufferAction *action) {
    if(action->getActionType() == ViewportBufferAction::fill) {
      QRect *fillRect = ((ViewportBufferFill *)action)->getRect();
      p_requestedFillArea += fillRect->width() * fillRect->height();
    }

    if(p_actions->empty()) {
      p_viewport->enableProgress();
    }

    p_actions->enqueue(action);
  }


  /**
   * Retrieves a line from the buffer. Line is relative to the top of the
   * visible area of the cube in the viewport.
   *
   * @param line
   *
   * @return const vector<double> &
   */
  const vector<double> &ViewportBuffer::getLine(int line) {
    if(!p_bufferInitialized || !p_enabled) {
      throw IException(IException::Programmer, "no data", _FILEINFO_);
    }

    if(line < 0 || line >= (int)p_buffer.size()) {
      throw IException(IException::Programmer,
                       "Invalid call to getLine",
                       _FILEINFO_);
    }

    return p_buffer.at(line);
  }


  /**
   * Retrieves the current bounding rect in viewport pixels of the visible cube
   * area.
   *
   *
   * @return QRect
   */
  QRect ViewportBuffer::getXYBoundingRect() {
    int startx, starty, endx, endy;
    p_viewport->cubeToViewport(0.5, 0.5, startx, starty);

    // Handle case where x,y 0,0 is sample,line 0,0 (which is outside the cube)
    //   and cubeToViewport still tells us 0.5, 0.5 is at x,y 0,0.
    double startSamp, startLine;
    p_viewport->viewportToCube(startx, starty, startSamp, startLine);

    if(startSamp < 0.5)
      startx ++;

    if(startLine < 0.5)
      starty ++;

    double rightmost = p_viewport->cubeSamples() + 0.5;
    double bottommost = p_viewport->cubeLines() + 0.5;

    p_viewport->cubeToViewport(rightmost, bottommost, endx, endy);

    if(endx < 0 || endy < 0)
      return QRect();

    double endSamp = -1, endLine = -1;
    p_viewport->viewportToCube(endx, endy, endSamp, endLine);

    if(endSamp > rightmost)
      endx --;

    if(endLine > bottommost)
      endy --;

    // Make sure our rect makes sense
    if(startx < 0) {
      startx = 0;
    }

    if(starty < 0) {
      starty = 0;
    }

    if(endx >= p_viewport->viewport()->width()) {
      endx = p_viewport->viewport()->width() - 1;
    }

    if(endy >= p_viewport->viewport()->height()) {
      endy = p_viewport->viewport()->height() - 1;
    }

    return QRect(startx, starty, endx - startx + 1, endy - starty + 1);
  }


  /**
   * Method to see if the entire cube is in the buffer.
   *
   *
   * @return bool
   */
  bool ViewportBuffer::hasEntireCube() {
    double sampTolerance = 0.05 * p_viewport->cubeSamples();
    double lineTolerance = 0.05 * p_viewport->cubeLines();

    bool hasCube = true;

    hasCube &= !working();
    hasCube &= p_sampLineBoundingRect[rectLeft] <= (1 + sampTolerance);
    hasCube &= p_sampLineBoundingRect[rectTop] <= (1 + lineTolerance);
    hasCube &= p_sampLineBoundingRect[rectRight] >= (p_viewport->cubeSamples() -
               sampTolerance);
    hasCube &= p_sampLineBoundingRect[rectBottom] >= (p_viewport->cubeLines() -
               lineTolerance);
    return hasCube;
  }


  /**
   * Retrieves the current bounding rect in sample/line coordinates of the
   * visible cube area.
   *
   *
   * @return QList<double>
   */
  QList<double> ViewportBuffer::getSampLineBoundingRect() {
    QRect xyRect = getXYBoundingRect();
    double ssamp, esamp, sline, eline;
    p_viewport->viewportToCube(xyRect.left(), xyRect.top(), ssamp, sline);
    p_viewport->viewportToCube(xyRect.right(), xyRect.bottom(), esamp, eline);

    QList<double> boundingRect;

    boundingRect.insert(rectLeft, ssamp);
    boundingRect.insert(rectTop, sline);
    boundingRect.insert(rectRight, esamp);
    boundingRect.insert(rectBottom, eline);

    return boundingRect;
  }


  /**
   * Sets the old and new bounding rects.
   *
   */
  void ViewportBuffer::updateBoundingRects() {
    p_oldXYBoundingRect = p_XYBoundingRect;
    p_XYBoundingRect = getXYBoundingRect();

    p_oldSampLineBoundingRect = p_sampLineBoundingRect;
    p_sampLineBoundingRect = getSampLineBoundingRect();

    p_oldViewportHeight = p_viewportHeight;
    p_viewportHeight = p_viewport->viewport()->height();

    p_oldVertScrollBarPos = p_vertScrollBarPos;
    // Add +1 to remove the black line at the top
    p_vertScrollBarPos = p_viewport->verticalScrollBar()->value() + 1;
  }


  /**
   * This method creates a fill action based on a rect and using
   * new versus old Y values. The use of old Y values is so we can
   * think of X and Y independently in complicated transforms - do
   * X first (with old Y values), then do Y. This does not enqueue
   * the created action.
   *
   * @param someRect
   * @param useOldY
   *
   * @return ViewportBufferFill*
   */
  ViewportBufferFill *ViewportBuffer::createViewportBufferFill(
    QRect someRect, bool useOldY) {
    QScrollBar *hsb = p_viewport->horizontalScrollBar();
    int xConstCoef = hsb->value();
    xConstCoef -= p_viewport->viewport()->width() / 2;

    // If panning over a full screen, it will try to create a fill rect that
    //   isn't actually valid. So, in the case of any bad fill rects we will
    //   fill everything.
    if(!someRect.isValid()) {
      throw IException(IException::Programmer, "Fill rect invalid", _FILEINFO_);
    }

    double xScale = p_viewport->scale();

    int yConstCoef = 0;

    if(!useOldY)
      yConstCoef = (p_vertScrollBarPos) - p_viewportHeight / 2 - 1;
    else
      yConstCoef = (p_oldVertScrollBarPos) - p_oldViewportHeight / 2 - 1;

    double yScale = xScale;

    QPoint topLeft;

    if(!useOldY) {
      topLeft = p_XYBoundingRect.topLeft();
    }
    else {
      topLeft = QPoint(p_XYBoundingRect.left(), p_oldXYBoundingRect.top());
    }

    ViewportBufferFill *newFill = new ViewportBufferFill(someRect, xConstCoef,
        xScale, yConstCoef, yScale, topLeft);

    return newFill;
  }


  /**
   * This requests the next line in a fill action.
   *
   * @param fill
   */
  void ViewportBuffer::requestCubeLine(ViewportBufferFill *fill) {
    if(p_band == -1) {
      throw IException(IException::Programmer, "invalid band", _FILEINFO_);
    }

    // Prep to create minimal buffer(s) to read the cube
    QRect &rect = *fill->getRect();

    double ssamp = fill->viewportToSample(rect.left());

    double esamp = fill->viewportToSample(rect.right());

    int brickWidth = (int)(ceil(esamp) - floor(ssamp)) + 1;

    if(brickWidth <= 0)
      return;

    double line = fill->viewportToLine(fill->getRequestPosition());
    int roundedSamp = (int)(ssamp + 0.5);
    int roundedLine = (int)(line + 0.5);

    emit ReadCube(p_cubeId, roundedSamp, roundedLine, roundedSamp + brickWidth,
                  roundedLine, p_band, this);

    fill->incRequestPosition();
  }


  /**
   * This processes the next available action, or starts
   * processing it, if possible. This method keeps the buffer
   * alive until the actions queue is empty.
   *
   * Any time the actions queue is modified this method should be
   * called, it doesn't hurt to call this too much. A side effect
   * of not calling this is qview never completing a
   * read/displaying new pixels.
   */
  void ViewportBuffer::doQueuedActions() {
    bool doNextAction = false;

    ViewportBufferAction *curAction = NULL;

    // if we aren't preserving data, and we don't still need the initial
    //   stretch (on startup), let's reset the buffer.
    if(!reinitializeActionExists() && !actionsPreserveData() &&
        p_initialStretchDone) {
      // Actions don't preserve data - call reinitialize!
      reinitialize();
    }

    if(!working()) {
      p_requestedFillArea = 0.0;
    }

    if(!p_actions->empty()) {
      curAction = p_actions->head();
      doNextAction = !curAction->started();
    }

    while(doNextAction) {
      if(curAction->getActionType() == ViewportBufferAction::transform) {
        doTransformAction((ViewportBufferTransform *) curAction);
      }
      else if(curAction->getActionType() == ViewportBufferAction::fill) {
        startFillAction((ViewportBufferFill *) curAction);
      }
      else {
        doStretchAction((ViewportBufferStretch *) curAction);
        p_initialStretchDone = true;
      }

      doNextAction = !p_actions->empty();

      if(doNextAction) {
        curAction = p_actions->head();
        doNextAction = !curAction->started();
      }
    }

    if(p_actions->empty()) {
      // Buffer Updated - Giving it BufferXYRect
      p_viewport->bufferUpdated(bufferXYRect());
    }
  }


  /**
   * Returns the viewport buffer's loading progress
   *
   * @return 0 to 1 where 1 is 100%
   */
  double ViewportBuffer::currentProgress() {
    if(!working())
      return 1.0;
    if(p_requestedFillArea <= 0.0)
      return 0.0;

    return 1.0 - totalUnfilledArea() / p_requestedFillArea;
  }


  /**
   * This returns the amount of area in the queue that needs new
   * cube data/will be filled by fill actions.
   *
   * @return double
   */
  double ViewportBuffer::totalUnfilledArea() {
    double totalFillArea = 0.0;

    // If at any time the total X or Y shift exceeds the buffer size, we aren't
    //   preserving data. Check to see if this is the case!
    for(int actionIndex = 0; actionIndex < p_actions->size(); actionIndex ++) {
      ViewportBufferAction *action = (*p_actions)[actionIndex];

      if(action->getActionType() == ViewportBufferAction::fill) {
        ViewportBufferFill *fill = (ViewportBufferFill *)action;

        QRect unfilledRect(*fill->getRect());
        unfilledRect.setTop(fill->getReadPosition());
        totalFillArea += unfilledRect.width() * unfilledRect.height();
      }
    }

    return totalFillArea;
  }


  /**
   * This returns true if any data currently in the buffer would be preserved if
   *   all of the queued actions are executed.
   */
  bool ViewportBuffer::actionsPreserveData() {
    int totalXShift = 0;
    int totalYShift = 0;

    QRect currentBufferRect(bufferXYRect());

    int bufferWidth  = currentBufferRect.width();
    int bufferHeight = currentBufferRect.height();

    // If at any time the total X or Y shift exceeds the buffer size, we aren't
    //   preserving data. Check to see if this is the case!
    for(int actionIndex = 0; actionIndex < p_actions->size(); actionIndex ++) {
      ViewportBufferAction *action = (*p_actions)[actionIndex];

      if(action->getActionType() == ViewportBufferAction::transform) {
        ViewportBufferTransform *transform = (ViewportBufferTransform *)action;

        if(transform->resizeFirst()) {
          bufferWidth = transform->getBufferWidth();
          bufferHeight = transform->getBufferHeight();
        }

        if(abs(totalXShift) >= bufferWidth)
          return false;
        if(abs(totalYShift) >= bufferHeight)
          return false;

        // Without the absolute value this will calculate
        //   if any data on the screen is preserved, however
        //   a better method is to see if its quicker to reread
        //   it all which happens when we use abs
        totalXShift += abs(transform->getXTranslation());
        totalYShift += abs(transform->getYTranslation());

        if(!transform->resizeFirst()) {
          bufferWidth = transform->getBufferWidth();
          bufferHeight = transform->getBufferHeight();
        }

        if(abs(totalXShift) >= bufferWidth)
          return false;
        if(abs(totalYShift) >= bufferHeight)
          return false;
      }
    }

    return true;
  }


  /**
   * This searches for actions that will reset the entire buffer's
   * contents.
   *
   * @return bool True if a transform to size 0,0 exists
   */
  bool ViewportBuffer::reinitializeActionExists() {
    QRect currentBufferRect(bufferXYRect());

    if(currentBufferRect.width() == 0 || currentBufferRect.height() == 0) {
      return true;
    }

    for(int actionIndex = 0; actionIndex < p_actions->size(); actionIndex ++) {
      ViewportBufferAction *action = (*p_actions)[actionIndex];

      if(action->getActionType() == ViewportBufferAction::transform) {
        ViewportBufferTransform *transform = (ViewportBufferTransform *)action;

        if(transform->getBufferWidth()  == 0)
          return true;
        if(transform->getBufferHeight() == 0)
          return true;
      }
    }

    return false;
  }


  /**
   * This tests if queued actions exist in the viewport buffer.
   *
   * @return bool
   */
  bool ViewportBuffer::working() {
    bool hasFillAction = false;
    for (auto action : *p_actions) {
      if (action->getActionType() == ViewportBufferAction::fill) {
        hasFillAction = true;
        break;
      }
    }
    return hasFillAction || !p_bufferInitialized || !p_enabled;
  }


  /**
   * Does a transformation on the internal viewport buffer
   *
   * @param action
   */
  void ViewportBuffer::doTransformAction(ViewportBufferTransform *action) {
    bool needResize = action->getBufferWidth() > -1 &&
                      action->getBufferHeight() > -1;

    if(action->resizeFirst() && needResize) {
      resizeBuffer(action->getBufferWidth(), action->getBufferHeight());
    }

    shiftBuffer(action->getXTranslation(), action->getYTranslation());

    if(!action->resizeFirst() && needResize) {
      resizeBuffer(action->getBufferWidth(), action->getBufferHeight());
    }

    delete action;
    action = NULL;
    p_actions->dequeue();
  }


  /**
   * Initializes a fill action by requesting the initial cube
   * data.
   *
   * @param action
   */
  void ViewportBuffer::startFillAction(ViewportBufferFill *action) {
    if(action->started())
      return;

    action->started(true);

    requestCubeLine(action);

    if(action->shouldRequestMore()) {
      requestCubeLine(action);
    }
  }


  /**
   * Tells the cube viewport to restretch.
   *
   * @param action
   */
  void ViewportBuffer::doStretchAction(ViewportBufferStretch *action) {
    delete action;
    action = NULL;
    p_actions->dequeue();

    p_viewport->restretch(this);
  }


  /**
   * Enlarges or shrinks the buffer and fills with nulls if necessary.
   *
   * @param width
   * @param height
   */
  void ViewportBuffer::resizeBuffer(unsigned int width, unsigned int height) {
    p_buffer.resize(height);

    for(unsigned int i = 0; i < p_buffer.size(); i++) {
      p_buffer[i].resize(width, Null);
    }
  }


  /**
   * Shifts the DN values in the buffer by deltaX and deltaY. Does not fill from
   * outside the buffer.
   *
   * @param deltaX
   * @param deltaY
   */
  void ViewportBuffer::shiftBuffer(int deltaX, int deltaY) {
    if(deltaY >= 0) {
      for(int i = p_buffer.size() - 1; i >= deltaY; i--) {
        p_buffer[i] = p_buffer[i - deltaY];

        // If we have y shift, null out original data (keep only moved data)
        if(deltaY != 0) {
          for(unsigned int x = 0; x < p_buffer[i - deltaY].size(); x++) {
            p_buffer[i - deltaY][x] = Null;
          }
        }

        if(deltaX > 0) {
          for(int j = p_buffer[i].size() - 1; j >= deltaX; j--) {
            p_buffer[i][j] = p_buffer[i][j - deltaX];
            p_buffer[i][j - deltaX] = Null;
          }
        }
        else if(deltaX < 0) {
          for(int j = 0; j < (int)p_buffer[i].size() + deltaX; j++) {
            p_buffer[i][j] = p_buffer[i][j - deltaX];
            p_buffer[i][j - deltaX] = Null;
          }
        }
      }
    }
    else if(deltaY < 0) {
      for(int i = 0; i < (int)p_buffer.size() + deltaY; i++) {
        p_buffer[i] = p_buffer[i - deltaY];

        // null out original data (keep only moved data)
        for(unsigned int x = 0; x < p_buffer[i - deltaY].size(); x++) {
          p_buffer[i - deltaY][x] = Null;
        }

        if(deltaX > 0) {
          for(int j = p_buffer[i].size() - 1; j >= deltaX; j--) {
            p_buffer[i][j] = p_buffer[i][j - deltaX];
            p_buffer[i][j - deltaX] = Null;
          }
        }
        else if(deltaX < 0) {
          for(int j = 0; j < (int)p_buffer[i].size() + deltaX; j++) {
            p_buffer[i][j] = p_buffer[i][j - deltaX];
            p_buffer[i][j - deltaX] = Null;
          }
        }
      }
    }
  }


  /**
   * Call this when the viewport is resized (not zoomed).
   *
   */
  void ViewportBuffer::resizedViewport() {
    updateBoundingRects();

    if(!p_bufferInitialized || !p_enabled)
      return;

    // ensure we have a valid bounding rect!  For example, If the cube viewport
    // is hidden and then shown again this could happen.
    if(!p_XYBoundingRect.isValid())
      return;

    if(!p_oldXYBoundingRect.isValid()) {
      reinitialize();
      return;
    }

    //We need to know how much data was gained/lost on each side of the cube
    double deltaLeftSamples =  p_sampLineBoundingRect[rectLeft] -
                               p_oldSampLineBoundingRect[rectLeft];
    //The input to round should be close to an integer
    int deltaLeftPixels = (int)round(deltaLeftSamples * p_viewport->scale());

    double deltaRightSamples = p_sampLineBoundingRect[rectRight] -
                               p_oldSampLineBoundingRect[rectRight];
    int deltaRightPixels = (int)round(deltaRightSamples * p_viewport->scale());

    double deltaTopSamples = p_sampLineBoundingRect[rectTop] -
                             p_oldSampLineBoundingRect[rectTop];
    int deltaTopPixels = (int)round(deltaTopSamples * p_viewport->scale());

    double deltaBottomSamples = p_sampLineBoundingRect[rectBottom] -
                                p_oldSampLineBoundingRect[rectBottom];
    int deltaBottomPixels = (int)round(deltaBottomSamples *
                                       p_viewport->scale());

    //deltaW is the change in width in the visible area of the cube
    int deltaW = - deltaLeftPixels + deltaRightPixels;

    //deltaH is the change in height in the visible area of the cube
    int deltaH = - deltaTopPixels + deltaBottomPixels;

    //If the new visible width has changed (resized in the horizontal direction)
    if(p_XYBoundingRect.width() != p_oldXYBoundingRect.width()) {
      //Resized larger in the horizontal direction
      if(deltaW > 0) {
        //Using old height because we might lose data if new height is smaller
        ViewportBufferTransform *transform = new ViewportBufferTransform();
        transform->setTranslation(-deltaLeftPixels, 0);
        transform->setResize(p_XYBoundingRect.width(),
                             p_oldXYBoundingRect.height());
        transform->resizeFirst(true);

        enqueueAction(transform);

        // left side that needs filled
        QPoint topLeftOfLeftRect(p_XYBoundingRect.left(),
                                 p_oldXYBoundingRect.top());

        QPoint bottomRightOfLeftRect(p_XYBoundingRect.left() - deltaLeftPixels,
                                     p_oldXYBoundingRect.bottom());

        QRect leftRect(topLeftOfLeftRect, bottomRightOfLeftRect);

        ViewportBufferFill *leftFill = createViewportBufferFill(leftRect,
                                       true);
        enqueueAction(leftFill);

        // right side that needs filled
        QPoint topLeftOfRightRect(p_XYBoundingRect.right() - deltaRightPixels,
                                  p_oldXYBoundingRect.top());

        QPoint bottomRightOfRightRect(p_XYBoundingRect.right(),
                                      p_oldXYBoundingRect.bottom());

        QRect rightRect(topLeftOfRightRect, bottomRightOfRightRect);

        ViewportBufferFill *rightFill = createViewportBufferFill(rightRect,
                                        true);
        enqueueAction(rightFill);
      }
      //Resized smaller in the horizontal direction
      else if(deltaW < 0) {
        ViewportBufferTransform *transform = new ViewportBufferTransform();
        transform->setTranslation(-deltaLeftPixels, 0);
        transform->setResize(p_XYBoundingRect.width(),
                             p_oldXYBoundingRect.height());
        transform->resizeFirst(false);
        enqueueAction(transform);
      }
    }

    //If the new visible height has changed (resized in the vertical direction)
    if(p_XYBoundingRect.height() != p_oldXYBoundingRect.height()) {
      //Resized larger in the vertical direction
      if(deltaH > 0) {
        ViewportBufferTransform *transform = new ViewportBufferTransform();
        transform->setTranslation(0, -deltaTopPixels);
        transform->setResize(p_XYBoundingRect.width(),
                             p_XYBoundingRect.height());
        transform->resizeFirst(true);

        enqueueAction(transform);

        QPoint bottomRightOfTopSide(p_XYBoundingRect.right(),
                                    p_XYBoundingRect.top() - deltaTopPixels);

        QRect topSideToFill(p_XYBoundingRect.topLeft(), bottomRightOfTopSide);


        QPoint topLeftOfbottomSide(p_XYBoundingRect.left(),
                                   p_XYBoundingRect.bottom() -
                                   deltaBottomPixels);

        QRect bottomSideToFill(topLeftOfbottomSide,
                               p_XYBoundingRect.bottomRight());

        ViewportBufferFill *topFill = createViewportBufferFill(topSideToFill,
                                      false);
        enqueueAction(topFill);

        ViewportBufferFill *bottomFill =
          createViewportBufferFill(bottomSideToFill, false);
        enqueueAction(bottomFill);
      }
      //Resized smaller in the vertical direction
      else if(deltaH < 0) {
        ViewportBufferTransform *transform = new ViewportBufferTransform();

        transform->setTranslation(0, -deltaTopPixels);
        transform->setResize(p_XYBoundingRect.width(),
                             p_oldXYBoundingRect.height());

        enqueueAction(transform);
      }
    }

    doQueuedActions();
  }


  /**
   * Call this when the viewport is panned. DeltaX and deltaY are relative to
   * the direction the buffer needs to shift.
   *
   * @param deltaX
   * @param deltaY
   */
  void ViewportBuffer::pan(int deltaX, int deltaY) {
    updateBoundingRects();

    if(!p_bufferInitialized || !p_enabled) {
      return;
    }


    if(p_sampLineBoundingRect == p_oldSampLineBoundingRect) {
      //The sample line bounding rect contains the bounds of the
      //screen pixels to the cube sample/line bounds. If the cube
      //bounds do not change, then we do not need to do anything to
      //the buffer.
      return;
    }

    double deltaLeftSamples = p_sampLineBoundingRect[rectLeft] -
                              p_oldSampLineBoundingRect[rectLeft];
    int deltaLeftPixels = (int)round(deltaLeftSamples * p_viewport->scale());

    double deltaTopLines = p_sampLineBoundingRect[rectTop] -
                           p_oldSampLineBoundingRect[rectTop];
    int deltaTopPixels = (int)round(deltaTopLines * p_viewport->scale());

    // Don't try to figure out panning beyond a full screen,
    //   even though data could very well be preserved.
    if(abs(deltaY) >= p_XYBoundingRect.height() ||
        abs(deltaX) >= p_XYBoundingRect.width()) {
      reinitialize();
      return;
    }

    //Left side of the visible area changed (start sample is different)
    if(p_sampLineBoundingRect[rectLeft] != p_oldSampLineBoundingRect[rectLeft]) {
      //Shifting data to the right
      if(deltaX > 0) {
        // The buffer is getting bigger
        ViewportBufferTransform *transform = new ViewportBufferTransform();
        transform->setResize(p_XYBoundingRect.width(),
                             p_oldXYBoundingRect.height());
        transform->setTranslation(-deltaLeftPixels, 0);
        transform->resizeFirst(true);

        enqueueAction(transform);

        QPoint topLeftOfRefill(p_XYBoundingRect.left(),
                               p_oldXYBoundingRect.top());

        QPoint bottomRightOfRefill(p_XYBoundingRect.left() + deltaX,
                                   p_oldXYBoundingRect.bottom());
        QRect fillArea(topLeftOfRefill, bottomRightOfRefill);

        ViewportBufferFill *fill = createViewportBufferFill(fillArea, true);
        enqueueAction(fill);
      }
      //Shifting data to the left
      else if(deltaX < 0) {
        //The buffer is getting smaller - no new data
        ViewportBufferTransform *transform = new ViewportBufferTransform();
        transform->setTranslation(-deltaLeftPixels, 0);
        transform->setResize(p_XYBoundingRect.width(),
                             p_oldXYBoundingRect.height());
        transform->resizeFirst(false);
        enqueueAction(transform);

        // if new samples on the screen at all, mark it for reading
        if(p_sampLineBoundingRect[rectRight] !=
            p_oldSampLineBoundingRect[rectRight]) {
          QPoint topLeftOfRefill(p_XYBoundingRect.right() + deltaX,
                                 p_oldXYBoundingRect.top());
          QPoint bottomRightOfRefill(p_XYBoundingRect.right(),
                                     p_oldXYBoundingRect.bottom());

          QRect refillArea(topLeftOfRefill, bottomRightOfRefill);

          ViewportBufferFill *fill = createViewportBufferFill(refillArea,
                                     true);
          enqueueAction(fill);
        }
      }
    }
    // Left side of the visible area is the same (start sample has not changed,
    // but end sample may be different)
    else {
      ViewportBufferTransform *transform = new ViewportBufferTransform();
      transform->setResize(p_XYBoundingRect.width(),
                           p_oldXYBoundingRect.height());
      enqueueAction(transform);

      if(deltaX < 0) {
        QPoint topLeftOfFillArea(p_XYBoundingRect.right() + deltaX,
                                 p_oldXYBoundingRect.top());

        QPoint bottomRightOfFillArea(p_XYBoundingRect.right(),
                                     p_oldXYBoundingRect.bottom());

        QRect fillArea(topLeftOfFillArea, bottomRightOfFillArea);
        ViewportBufferFill *fill = createViewportBufferFill(fillArea, true);

        enqueueAction(fill);
      }
    }

    //Top side of the visible area changed (start line is different)
    if(p_sampLineBoundingRect[rectTop] != p_oldSampLineBoundingRect[rectTop]) {
      //Shifting data down
      if(deltaY > 0) {
        ViewportBufferTransform *transform = new ViewportBufferTransform();
        transform->setTranslation(0, -deltaTopPixels);
        transform->setResize(p_XYBoundingRect.width(),
                             p_XYBoundingRect.height());
        transform->resizeFirst(true);
        enqueueAction(transform);

        QPoint topLeftOfFillArea(p_XYBoundingRect.left(),
                                 p_XYBoundingRect.top());
        QPoint bottomRightOfFillArea(p_XYBoundingRect.right(),
                                     p_XYBoundingRect.top() + deltaY);
        QRect fillArea(topLeftOfFillArea, bottomRightOfFillArea);
        ViewportBufferFill *fill = createViewportBufferFill(fillArea, false);
        enqueueAction(fill);
      }
      //Shifting data up
      else if(deltaY < 0) {
        ViewportBufferTransform *transform = new ViewportBufferTransform();
        transform->setTranslation(0, -deltaTopPixels);
        transform->setResize(p_XYBoundingRect.width(),
                             p_XYBoundingRect.height());
        transform->resizeFirst(false);
        enqueueAction(transform);

        // if new lines on the screen at all, mark it for reading
        if(p_sampLineBoundingRect[rectBottom] !=
            p_oldSampLineBoundingRect[rectBottom]) {
          QPoint topLeftOfFillArea(p_XYBoundingRect.left(),
                                   p_oldXYBoundingRect.bottom() + deltaY);
          QPoint bottomRightOfFillArea(p_XYBoundingRect.right(),
                                       p_XYBoundingRect.bottom());
          QRect fillArea(topLeftOfFillArea, bottomRightOfFillArea);
          ViewportBufferFill *fill = createViewportBufferFill(fillArea, false);
          enqueueAction(fill);
        }
      }
    }
    // Top side of the visible area is the same (start line has not changed, but
    // end line may be different)
    else {
      ViewportBufferTransform *transform = new ViewportBufferTransform();
      transform->setResize(p_XYBoundingRect.width(),
                           p_XYBoundingRect.height());
      enqueueAction(transform);

      if(deltaY < 0) {
        QPoint topLeftOfFillArea(p_XYBoundingRect.left(),
                                 p_XYBoundingRect.bottom() + deltaY);
        QPoint bottomRightOfFillArea(p_XYBoundingRect.right(),
                                     p_XYBoundingRect.bottom());
        QRect fillArea(topLeftOfFillArea, bottomRightOfFillArea);
        ViewportBufferFill *fill = createViewportBufferFill(fillArea, false);
        enqueueAction(fill);
      }
    }

    doQueuedActions();
  }


  /**
   * When all current operations finish the cube viewport will be
   * asked to do a stretch if you call this. Existing requests
   * will be removed.
   */
  void ViewportBuffer::addStretchAction() {
    for(int i = 0; i < p_actions->size(); i++) {
      if((*p_actions)[i]->getActionType() == ViewportBufferAction::stretch) {
        p_actions->removeAt(i);
        i --;
      }
    }

    enqueueAction(new ViewportBufferStretch());
    doQueuedActions();
  }

  /**
   * This is meant to clear up ram on non-active viewports.
   *
   * cubeViewport is supposed to call this method on viewports
   * buffers that are not related to the active viewport.
   *
   * @param force If true, memory will be freed regardless of current total
   *              buffer size (b/w -> rgb for example).
   */
  void ViewportBuffer::emptyBuffer(bool force) {
    if(force) {
      p_buffer.clear();
      p_bufferInitialized = false;
    }
  }


  /**
   * Returns a rect, in screen pixels, of the area this buffer
   * covers.
   *
   * @author slambright (4/30/2010)
   *
   * @return QRect
   */
  QRect ViewportBuffer::bufferXYRect() {
    QRect rect = p_XYBoundingRect;

    if(!rect.height() || !p_buffer.size())
      return QRect();

    if(rect.height() > (int) p_buffer.size())
      rect.setBottom(rect.top() + p_buffer.size() - 1);

    if(rect.width() > (int) p_buffer[0].size())
      rect.setRight(rect.left() + p_buffer[0].size() - 1);

    return rect;
  }


  /**
   * Call this when zoomed, re-reads visible area.
   *
   * @throw iException - "Unable to change scale"
   * @internal
   *   @history 2010-07-12 Jeannie Walldren - Added exception to help track
   *                          errors.
   */
  void ViewportBuffer::scaleChanged() {
    if(!p_enabled)
      return;

    try {
      updateBoundingRects();
      reinitialize();
    }
    catch (IException &e) {
      throw IException(
          e, IException::Programmer, "Unable to change scale.", _FILEINFO_);
    }
  }


  /**
   * This turns on or off reading from the cube. If enabled is true the buffer
   * will be re-read.
   *
   * @param enabled
   */
  void ViewportBuffer::enable(bool enabled) {
    bool wasEnabled = p_enabled;

    p_enabled = enabled;

    if(!wasEnabled && p_enabled) {
      updateBoundingRects();
      reinitialize();
    }
  }


  /**
   * Sets the band to read from, the buffer will be re-read if the band changes.
   *
   * @param band
   */
  void ViewportBuffer::setBand(int band) {
    if(p_band == band)
      return;
    p_band = band;

    updateBoundingRects();

    if(!p_enabled)
      return;

    reinitialize();
  }


  /**
   * This resizes and fills entire buffer.
   * @throw iException - "Unable to resize and fill buffer."
   * @internal
   *   @history 2010-07-12 Jeannie Walldren - Added exception to help track
   *                          errors.
   */
  void ViewportBuffer::reinitialize() {

    try {
      // If we're in the middle of a process, we got an okay stretch on startup,
      // then we can stop what we're doing.
      if(working() && p_initialStretchDone) {
        // We only need to handle the current action, can ignore others
        ViewportBufferAction *curAction = p_actions->head();

        // Delete older actions
        for(int i = p_actions->size() - 1; i > 0; i--) {
          delete(*p_actions)[i];
          p_actions->pop_back();
        }

        // Deal with current action
        if(curAction->started()) {
          if(curAction->getActionType() == ViewportBufferAction::fill) {
            ViewportBufferFill *fill = (ViewportBufferFill *)curAction;

            fill->stop();

            p_requestedFillArea = fill->getRect()->height() *
                                  fill->getRect()->width();
          }
        }
        else {
          delete curAction;
          p_actions->clear();
          p_requestedFillArea = 0.0;
        }
      }


      p_bufferInitialized = true;

      ViewportBufferTransform *reset = new ViewportBufferTransform();
      reset->setResize(0, 0);
      enqueueAction(reset);

      if (p_XYBoundingRect.isValid()) {
        ViewportBufferTransform *transform = new ViewportBufferTransform();
        transform->setResize(p_XYBoundingRect.width(), p_XYBoundingRect.height());
        enqueueAction(transform);
        ViewportBufferFill *fill = createViewportBufferFill(p_XYBoundingRect,
                                   false);
        enqueueAction(fill);
      }

      doQueuedActions();
    }
    catch (IException &e) {
      throw IException(IException::Programmer,
                       "Unable to resize and fill buffer.",
                       _FILEINFO_);
    }
  }
}
