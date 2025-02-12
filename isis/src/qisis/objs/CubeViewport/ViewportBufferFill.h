#ifndef VieportBufferFill_h
#define VieportBufferFill_h

/** This is free and unencumbered software released into the public domain.

The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */

#include "ViewportBufferAction.h"

class QRect;
class QPoint;

namespace Isis {
  /**
   * @author ????-??-?? Unknown
   *
   * @internal
   *   @history 2011-04-25 Steven Lambright - Fixed a problem where 1 too few
   *                       lines were being read
   *   @history 2015-06-30 Makayla Shepherd and Ian Humphrey - Modified stop() to keep the viewport
   *                       buffer object from reading an extra line at the bottom of the viewport
   *                       rectangle. This could potentially cause a crash from reading outside of
   *                       the bounds. Fixes #2171.
   */
  class ViewportBufferFill : public ViewportBufferAction {
    public:
      ViewportBufferFill(const QRect &rect, const int &xCoef,
                         const double &xScale, const int &yCoef,
                         const double &yScale, const QPoint &topLeftPixel);
      ~ViewportBufferFill();


      /**
       * Returns the type of this class
       *
       * @return ViewportBufferAction::ActionType
       */
      virtual ViewportBufferAction::ActionType getActionType() {
        return ViewportBufferAction::fill;
      };

      /**
       * Converts screen x position to cube sample position
       *
       * @param x
       *
       * @return double
       */
      double viewportToSample(int x) {
        return (x + p_xCoef) / p_xScale;
      }

      /**
       * Converts screen y position to cube line position
       *
       * @param y
       *
       * @return double
       */
      double viewportToLine(int y) {
        return (y + p_yCoef) / p_yScale;
      }

      /**
       * Returns the current request position (>= read position)
       *
       * @return int
       */
      int getRequestPosition() const {
        return p_requestPosition;
      }

      /**
       * Returns the current read position
       *
       * @return int
       */
      int getReadPosition() const {
        return p_readPosition;
      }

      //! Increment read position
      void incReadPosition() {
        p_readPosition++;
      }

      //! Increment request position
      void incRequestPosition() {
        p_requestPosition++;
      }

      /**
       * Returns the rect that this action is filling in screen pixels
       *
       * @return QRect*
       */
      QRect *getRect() {
        return p_rect;
      }

      int getTopmostPixelPosition();
      int getLeftmostPixelPosition();

      bool doneReading();
      bool shouldRequestMore();
      bool shouldPaint(int &linesToPaint);

      void stop();

      double scale() {
        return p_xScale;
      }

    private:
      //! Position of the cube reads
      unsigned int p_readPosition;
      //! Position of the cube requests
      unsigned int p_requestPosition;
      //! Rect this fill represents
      QRect *p_rect;
      //! Top left of the viewport for this fill
      QPoint *p_topLeftPixel;
      //! viewport to sample/line x coef
      int p_xCoef;
      //! viewport to sample/line x scalar
      double p_xScale;
      //! viewport to sample/line y coef
      int p_yCoef;
      //! viewport to sample/line y scalar
      double p_yScale;

      //! how many cube lines per paint if painting inbetween gets re-enabled
      const static int STEPSIZE = 20;
  };
}

#endif
