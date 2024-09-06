#include "ControlPointGraphicsItem.h"

#include <float.h>
#include <iostream>

#include <QDebug>
#include <QEvent>
#include <QGraphicsScene>
#include <QGraphicsSceneContextMenuEvent>
#include <QMenu>
#include <QMessageBox>
#include <QBrush>
#include <QPainterPath>
#include <QPen>

#include "Constants.h"
#include "ControlMeasure.h"
#include "ControlPoint.h"
#include "Directory.h"
#include "FileName.h"
#include "MosaicGraphicsView.h"
#include "MosaicSceneWidget.h"
#include "SerialNumberList.h"
#include "Statistics.h"

using namespace std;

namespace Isis {
  /**
   * Create a CP graphics item. This will colorize and set the appropriate
   *   toolTip for this control point.
   */
  ControlPointGraphicsItem::ControlPointGraphicsItem(QPointF center,
      QPointF apriori, ControlPoint *cp, SerialNumberList *snList,
      MosaicSceneWidget *boundingRectSrc, QGraphicsItem *parent) :
      QGraphicsRectItem(parent) {
    m_centerPoint = new QPointF(center);
    m_mosaicScene = boundingRectSrc;
    m_controlPoint = cp;
    m_showArrow = false;
    m_colorByMeasureCount = false;
    m_colorByResidualMagnitude = false;

    m_measureCount = -1;
    m_residualMagnitude = Null;

    setZValue(DBL_MAX);

    m_origPoint = new QPointF(apriori);

    // Providing a width of 0 makes pens cosmetic (i.e. always appear as 1 pixel on screen)
    if (cp->IsIgnored()) {
      setPen(QPen(Qt::yellow, 0.0));
    }
    else if ( (cp->GetType() == ControlPoint::Fixed)
      || (cp->GetType() == ControlPoint::Constrained) ) {
      setPen(QPen(Qt::magenta, 0.0));
    }
    else { // Free and editLocked
      setPen(QPen(Qt::darkGreen, 0.0));
    }

    setBrush(Qt::NoBrush);

    setToolTip(makeToolTip(snList));

    setRect(calcRect());

    setFlag(QGraphicsItem::ItemIsSelectable, true);
    setFlag(QGraphicsItem::ItemIsFocusable, true);

//  setFiltersChildEvents(true);
//  installEventFilter(this);
  }


  ControlPointGraphicsItem::~ControlPointGraphicsItem() {
    if (m_centerPoint) {
      delete m_centerPoint;
      m_centerPoint = NULL;
    }

    if (m_origPoint) {
      delete m_origPoint;
      m_origPoint = NULL;
    }

    m_mosaicScene = NULL;
  }


  /**
   * This virtual paint method is called anytime an update() or paintEvent() is called
   *
   * @param painter (QPainter *) Painter used to draw
   * @param style (QStyleOptionGraphicsItem *) Describes parameters used to draw a QGraphicsItem
   * @param widget (QWidget *) Optional argument which indicates the widget that is being painted on
   */
  void ControlPointGraphicsItem::paint(QPainter *painter,
      const QStyleOptionGraphicsItem *style,  QWidget * widget) {

    QRectF fullRect = calcRect();
    QRectF crosshairRect = calcCrosshairRect();

    if (crosshairRect.isNull()) {
      return;
    }

    if (rect() != fullRect) {
      setRect(fullRect);
    }
    else {
      painter->setPen(pen());
      painter->setBrush(brush());

      QPointF center = crosshairRect.center();

      QPointF centerLeft(crosshairRect.left(), center.y());
      QPointF centerRight(crosshairRect.right(), center.y());
      QPointF centerTop(center.x(), crosshairRect.top());
      QPointF centerBottom(center.x(), crosshairRect.bottom());

      if (m_mosaicScene->directory() &&
          m_mosaicScene->directory()->editPointId() == m_controlPoint->GetId()) {
        QPainterPath path;
        //  Draw circle, then crosshair inside circle
        path.addEllipse(crosshairRect);
        path.moveTo(centerTop);
        path.lineTo(centerBottom);
        path.moveTo(centerLeft);
        path.lineTo(centerRight);

        painter->setPen(QPen(Qt::red, 0.0));
        painter->drawPath(path);
      }
      else {
        painter->drawLine(centerLeft, centerRight);
        painter->drawLine(centerTop, centerBottom);
      }

      if (!m_origPoint->isNull() && *m_origPoint != *m_centerPoint
         && m_showArrow) {

        if (!m_colorByMeasureCount && !m_colorByResidualMagnitude) {
          painter->setPen(Qt::black);
          painter->setBrush(Qt::black);
        }
        else {
          QColor zeroColor(Qt::black);
          QColor fullColor(Qt::green);

          bool isColored = false;

          if (m_colorByMeasureCount) {
            int fullColorMeasureCount = qMax(1, m_measureCount);
            int measureCount = m_controlPoint->getMeasures(true).count();
            isColored = (measureCount >= fullColorMeasureCount);
          }
          else {
            fullColor = QColor(Qt::red);

            double fullColorErrorMag = 0.0;

            if (!IsSpecial(m_residualMagnitude)) {
              fullColorErrorMag = m_residualMagnitude;
            }

            Statistics residualStats;
            foreach (ControlMeasure *cm, m_controlPoint->getMeasures(true)) {
              residualStats.AddData(cm->GetResidualMagnitude());
            }

            if (residualStats.Average() != Null) {
              double errorMag = residualStats.Maximum();
              if (errorMag >= fullColorErrorMag) {
                isColored = true;
              }
            }
          }

          QColor finalColor = isColored? fullColor : zeroColor;

          painter->setPen(finalColor);
          painter->setBrush(finalColor);
        }
        painter->drawLine(*m_origPoint, *m_centerPoint);

        QPolygonF arrowHead = calcArrowHead();
        painter->drawPolygon(arrowHead);
      }
    }
  }


  ControlPoint *ControlPointGraphicsItem::controlPoint() {
    return m_controlPoint;
  }

  void ControlPointGraphicsItem::setArrowVisible(bool visible,
                                                 bool colorByMeasureCount,
                                                 int measureCount,
                                                 bool colorByResidualMagnitude,
                                                 double residualMagnitude) {
    m_showArrow = visible;
    m_colorByMeasureCount = colorByMeasureCount;
    m_measureCount = measureCount;
    m_colorByResidualMagnitude = colorByResidualMagnitude;
    m_residualMagnitude = residualMagnitude;
    setRect(calcRect());
    update();
  }


  void ControlPointGraphicsItem::contextMenuEvent(
      QGraphicsSceneContextMenuEvent * event) {
    QMenu menu;

    QAction *title = menu.addAction(
        m_controlPoint->GetId());
    title->setEnabled(false);
    menu.addSeparator();

    QAction *infoAction = menu.addAction("Show Point Info");

    QAction *selected = menu.exec(event->screenPos());

    if (selected == infoAction) {
      QMessageBox::information(m_mosaicScene, "Control Point Information",
          toolTip());
    }
  }


  QRectF ControlPointGraphicsItem::calcRect() const {
    QRectF pointRect(calcCrosshairRect());

    if (!m_origPoint->isNull() && pointRect.isValid()) {
      // Make the rect contain the orig point
      if (pointRect.left() > m_origPoint->x()) {
        pointRect.setLeft(m_origPoint->x());
      }
      else if (pointRect.right() < m_origPoint->x()) {
        pointRect.setRight(m_origPoint->x());
      }

      if (pointRect.top() > m_origPoint->y()) {
        pointRect.setTop(m_origPoint->y());
      }
      else if (pointRect.bottom() < m_origPoint->y()) {
        pointRect.setBottom(m_origPoint->y());
      }
    }

    QPolygonF arrowHead = calcArrowHead();

    if (arrowHead.size() > 2) {
      pointRect = pointRect.united(arrowHead.boundingRect());
    }

    return pointRect;
  }


  QRectF ControlPointGraphicsItem::calcCrosshairRect() const {
    QRectF pointRect;

    if (m_centerPoint && !m_centerPoint->isNull() && m_mosaicScene) {
      static const int size = 12;
      QPoint findSpotScreen =
          m_mosaicScene->getView()->mapFromScene(*m_centerPoint);
      QPoint findSpotTopLeftScreen =
          findSpotScreen - QPoint(size / 2, size / 2);

      QRect pointRectScreen(findSpotTopLeftScreen, QSize(size, size));

      pointRect =
          m_mosaicScene->getView()->mapToScene(pointRectScreen).boundingRect();
    }

    return pointRect;
  }


  QPolygonF ControlPointGraphicsItem::calcArrowHead() const {
    QPolygonF arrowHead;

    if (m_showArrow && !m_origPoint->isNull() &&
       *m_origPoint != *m_centerPoint) {
      QRectF crosshairRect = calcCrosshairRect();
      double headSize = crosshairRect.width() * 4.0 / 5.0;

      double crosshairSize = headSize;

      // Draw arrow
      QPointF lineVector = *m_centerPoint - *m_origPoint;
      double lineVectorMag = sqrt(lineVector.x() * lineVector.x() +
                                  lineVector.y() * lineVector.y());
      double thetaPointOnLine = crosshairSize / (2 * (tanf(PI / 6) / 2) *
                            lineVectorMag);
      QPointF pointOnLine = *m_centerPoint - thetaPointOnLine * lineVector;

      QPointF normalVector = QPointF(-lineVector.y(), lineVector.x());
      double thetaNormal = crosshairSize / (2 * lineVectorMag);

      QPointF leftPoint = pointOnLine + thetaNormal * normalVector;
      QPointF rightPoint = pointOnLine - thetaNormal * normalVector;

      arrowHead << leftPoint << crosshairRect.center() << rightPoint;
    }

    return arrowHead;
  }


  QString ControlPointGraphicsItem::makeToolTip(SerialNumberList *snList) {
    QString toolTip = "<div>Point ID: " +
        m_controlPoint->GetId();
    toolTip += "<br />Point Type: " +
        m_controlPoint->GetPointTypeString();
    toolTip += "<br />Number of Measures: ";
    toolTip += toString(m_controlPoint->GetNumMeasures());
    toolTip += "<br />Ignored: ";
    toolTip += m_controlPoint->IsIgnored() ? "Yes" : "No";
    toolTip += "<br />Edit Locked: ";
    toolTip += m_controlPoint->IsEditLocked() ? "Yes" : "No";

    toolTip += "<br />";

    if (snList == NULL) {
      toolTip += QStringList(m_controlPoint->getCubeSerialNumbers()).join("\n");
    }
    else {
      QStringList serialNums(m_controlPoint->getCubeSerialNumbers());

      for(int snIndex = 0; snIndex < serialNums.size(); snIndex ++) {
        QString serialNum = serialNums[snIndex];

        if (snIndex > 0) {
          toolTip += "<br />";
        }

        if (snList->hasSerialNumber(serialNum)) {
          toolTip +=
              QString::fromStdString(FileName(snList->fileName(serialNum).toStdString()).name());
          toolTip += " (" + serialNum + ")";
        }
        else {
          toolTip += serialNum;
        }

        double residMag = m_controlPoint->GetMeasure(serialNum)->GetResidualMagnitude();
        if (residMag != Null) {
          toolTip += " [residual: <font color='red'>" + toString(residMag) + "</font>]";
        }
      }
    }

    toolTip += "</div>";

    return toolTip;
  }


//bool MosaicSceneWidget::
//(QObject *obj, QEvent *event) {
//
//  switch(event->type()) {
//    case QMouseEvent::GraphicsSceneMousePress: {


}
