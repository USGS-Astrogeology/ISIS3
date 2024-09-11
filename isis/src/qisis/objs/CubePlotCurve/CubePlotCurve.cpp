/** This is free and unencumbered software released into the public domain.

The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */

#include "CubePlotCurve.h"

#include <iostream>

#include <QAction>
#include <QApplication>
#include <QBuffer>
#include <QByteArray>
#include <QClipboard>
#include <QDataStream>
#include <QDrag>
#include <QFileInfo>
#include <QEvent>
#include <QMenu>
#include <QMimeData>
#include <QMouseEvent>
#include <QPainter>
#include <QString>

#include <qwt_legend.h>
#include <qwt_text.h>

#include "Cube.h"
#include "CubeViewport.h"
#include "FileName.h"
#include "IException.h"
#include "IString.h"
#include "CubePlotCurveConfigureDialog.h"

namespace Isis {
  /**
   * This constructs a CubePlotCurve... a subclass of PlotCurve.
   *   This class was created specifically for use with the plot
   *   tools. With this class the programmer can set the cube view
   *   port that the curve is associated with along with the
   *   vertices on the cvp of which the curve gets it data.
   *
   * @param xUnits The units that the x-axis data is in
   * @param yUnits The units that the x-axis data is in
   */
  CubePlotCurve::CubePlotCurve(Units xUnits, Units yUnits) :
      PlotCurve(xUnits, yUnits) {
    m_legendItem = NULL;
    m_renameAutomatically = true;
  }


  /**
   * Construct the plot tool curve given the past results of toByteArray(...).
   * This is used for copy/paste and drag/drop.
   *
   * @param parentAndChildData A serialized, binary representation of an
   *                           instance of this class.
   */
    CubePlotCurve::CubePlotCurve(const QByteArray &parentAndChildData) :
      PlotCurve(Unknown, Unknown) {
    // Copy will get a new legend item widget when attached to the plot in PlotWindow
    m_legendItem = NULL;

    QByteArray classData = fromByteArray(parentAndChildData);

    QString expectedHeader("PLOT_TOOL_CURVE_V1");
    int headerKeySize = expectedHeader.toUtf8().size();

    if (classData.size() > headerKeySize) {
      int dataPos = 0;
      const char *rawClassData = classData.data();

      QString givenKey = QString::fromUtf8(classData.data() + dataPos,
                                           headerKeySize);
      dataPos += headerKeySize;
      if (givenKey != expectedHeader) {
        IString msg = "The given byte array does not contain the required "
            "header";
        throw IException(IException::Programmer, msg, _FILEINFO_);
      }

      int numSourceCubes = *(((int *)(rawClassData + dataPos)));
      dataPos += sizeof(int);

      for (int i = 0; i < numSourceCubes; i++) {
        int sourceCubeSize = *(((int *)(rawClassData + dataPos)));
        dataPos += sizeof(int);
        QString data = QString::fromUtf8(classData.data() + dataPos,
                                         sourceCubeSize);
        m_sourceCube << data;
        dataPos += sourceCubeSize;
      }

      int pointListSize = *(((int *)(rawClassData + dataPos)));
      dataPos += sizeof(int);

      for (int i = 0; i < pointListSize; i ++) {
        int npts = *(((int *)(rawClassData + dataPos)));
        dataPos += sizeof(int);

        QList<QPointF> points;
        for (int pt = 0; pt < npts; pt++) {
          double x = *((double *)(rawClassData + dataPos));
          dataPos += sizeof(double);

          double y = *((double *)(rawClassData + dataPos));
          dataPos += sizeof(double);

          points.append(QPointF(x, y));
        }
        m_pointList.append(points);
      }

    }
    else {
      IString msg = "The given byte array is not large enough to contain the "
          "required header";
      throw IException(IException::Programmer, msg, _FILEINFO_);
    }
  }


  /**
   * This will start the drag & drop operation for these curves.
   *
   * @param o The object that the event relates to. This is usually the legend
   *          item, but some children install their own event filters for other
   *          objects.
   * @param e The event that occurred on the given object.
   *
   * @return True if we don't want any more processing of the event
   */
  bool CubePlotCurve::eventFilter(QObject *o, QEvent *e) {
    bool blockWidgetFromEvent = false;

    switch (e->type()) {
      case QEvent::MouseButtonPress: {
        mousePressEvent((QMouseEvent *)e);
        blockWidgetFromEvent = true;
        break;
      }

      case QEvent::MouseButtonDblClick:
      case QEvent::MouseButtonRelease:
        blockWidgetFromEvent = true;
        break;


      default:
        break;
    }

    return blockWidgetFromEvent;
  }


  /**
   * Use information inside of the plot curve to paint onto a cube viewport.
   *   This paints the originating data points, in the color of the curve, onto
   *   the viewport. This will not paint onto unrelated cube viewports.
   *
   * @param vp The viewport we're going to potentially paint onto
   * @param painter The painter to use for painting onto the viewport
   */
  void CubePlotCurve::paint(CubeViewport *vp, QPainter *painter) {
    if (m_sourceCube.contains(vp->cube()->fileName())) {
      int i = m_sourceCube.indexOf(vp->cube()->fileName());
      QPen customPen;
      customPen.setColor(pen().color());
      customPen.setWidth(pen().width());

      if (pen().style() != Qt::NoPen)
        customPen.setStyle(pen().style());

      painter->setPen(customPen);
      QList< QList <QPointF> > allPoints = sourceVertices();
      QList<QPointF> points = allPoints.at(i);

      for (int p = 1; p <= points.size(); p++) {
        int sample1;
        int sample2;
        int line1;
        int line2;

        if (p != points.size()) {
          vp->cubeToViewport(points[p - 1].x(), points[p - 1].y(), sample1, line1);
          vp->cubeToViewport(points[p].x(), points[p].y(), sample2, line2);
        }
        else {
          vp->cubeToViewport(points[p - 1].x(), points[p - 1].y(),
                             sample1, line1);
          vp->cubeToViewport(points[0].x(), points[0].y(), sample2, line2);
        }

        painter->drawLine(QPoint(sample1, line1), QPoint(sample2,  line2));
      }
    }
  }


  /**
   * This method returns a list of points which are the vertices
   * of the selected area (by the rubberband) on the cvp.
   *
   * @return QList<QPoint>
   */
  QList< QList <QPointF> > CubePlotCurve::sourceVertices()  const {
    return m_pointList;
  }


  /**
   * This method is necessary for getting the correct (event filter altered)
   *   legend item. Without this, drag & drop and context menus do not work.
   *
   * @return The legend item widget associated with this curve
   */
  QWidget *CubePlotCurve::legendItem() const {
    return m_legendItem;
  }


  /**
   * This method returns the cube view port associated with the
   * curve.
   *
   * @return CubeViewport*
   */
  QStringList CubePlotCurve::sourceCube() const {
    return m_sourceCube;
  }


  /**
   * This enables/disables the plot curve from changing it's title when the
   *   source data changes. This is enabled by default and typically disabled
   *   when a user manually renames a curve.
   *
   * @param allowed True for automatic renaming, false to preserve curve title
   */
  void CubePlotCurve::enableAutoRenaming(bool allowed) {
    m_renameAutomatically = allowed;
  }


  /**
   * This copies the source data from another CubePlotCurve. This curve will now
   *   act as if it has the same exact source data as the other curve.
   *
   * @param other The curve with the source (origination) data to copy.
   */
  void CubePlotCurve::copySource(const CubePlotCurve &other) {
    m_sourceCube = other.m_sourceCube;
    m_pointList = other.m_pointList;
    emit needsRepaint();
  }


  void CubePlotCurve::clearSource() {

    if (m_originalName != "" && !m_sourceCube.empty() && m_renameAutomatically) {
      setTitle(m_originalName);
    }
    else if (m_originalName == "") {
      m_originalName = title().text();
    }

    m_sourceCube.clear();
    m_pointList.clear();

  }


  void CubePlotCurve::addSource(CubeViewport *cvp, QList<QPoint> screenPoints,
                                int band) {

    if (cvp) {
      m_sourceCube << cvp->cube()->fileName();

      if (m_renameAutomatically) {
        setTitle(title().text() + " - " +
                 QFileInfo(m_sourceCube.at(m_sourceCube.size()-1)).baseName());

        if (band != -1) {
          setTitle(title().text() + "+" + QString::number(band));
        }
      }

      QList<QPointF> points;
      foreach (QPoint screenpoint, screenPoints) {
        double sample = 0.0;
        double line = 0.0;

        cvp->viewportToCube(screenpoint.x(), screenpoint.y(), sample, line);

        points.append(QPointF(sample, line));
      }
      m_pointList.append(points);

      emit needsRepaint();
    }

  }

  /**
   * Tell this plot curve from where its data originated.
   *
   * If you set a source, you must keep it up to date. Any time the data in
   *   the curve changes, you should be calling this method again.
   *
   * @param cvp The viewport from which this curve's data came from
   * @param screenPoints The pertinent points on the viewport from which this
   *                     curve's data came from.
   * @param band The band of the cube inside of said viewport from which this
   *             curve's data came from.
   */
  void CubePlotCurve::setSource(CubeViewport *cvp, QList<QPoint> screenPoints,
                                int band) {

    clearSource();
    addSource(cvp,  screenPoints,  band);
//    emit needsRepaint();
  }


  void CubePlotCurve::setSource(QList<CubeViewport *> cvps,
                                QList< QList<QPoint> > screenPoints,
                                QList<int> band) {
    for (int i = 0; i < cvps.size(); i++) {
      addSource(cvps.at(i), screenPoints.at(i), band.at(i));
    }
//    emit needsRepaint();
  }


  /**
   * This creates a legend item and overrides events from it.
   */
  void CubePlotCurve::updateLegendItemWidget(QWidget *legendItem) {
    m_legendItem = legendItem;
    m_legendItem->installEventFilter(this);
  //  if (!m_legendItem) {
     //// m_legendItem = PlotCurve::legendItem();
     // m_legendItem->installEventFilter(this);
     // connect(m_legendItem, SIGNAL(destroyed(QObject *)),
     //         this, SLOT(updateLegendItemWidget()));
    //}
  }


  /**
   * This converts the plot curve into a binary, clipboard-compatible storage
   *   format (QMimeData). The format is 'application/isis3-plot-curve'.
   *
   * @return A mime-data representation of this class. Ownership of the return
   *         value is passed to the caller.
   */
  QMimeData *CubePlotCurve::createMimeData() const {
    QMimeData *mimeData = new QMimeData;
    mimeData->setData("application/isis3-plot-curve",
                      toByteArray());
    return mimeData;
  }


  /**
   * Serialize this plot curve into a binary byte array. This is useful for
   *   storing the curve on the clipboard or in a drag & drop event.
   *
   * @return This curve represented as binary data
   */
  QByteArray CubePlotCurve::toByteArray() const {
    QByteArray classData;

    classData.append(PlotCurve::toByteArray());

    QString header("PLOT_TOOL_CURVE_V1");
    int size = header.size();
    classData.append(header.toUtf8());

    size = m_sourceCube.size();
    classData.append((char *)&size, sizeof(int));

    for (int i = 0; i < size; i++) {
      int sourceCubeSize = m_sourceCube.at(i).toUtf8().size();
      classData.append((char *)&sourceCubeSize, sizeof(int));
      classData.append(m_sourceCube.at(i).toUtf8());
    }

    size = m_pointList.size();
    classData.append((char *)&size, sizeof(int));

    for (int i = 0; i < size; i ++) {
      QList<QPointF> points = m_pointList[i];
      int npts = points.size();
      classData.append((char *)&npts, sizeof(int));

      for (int pt = 0; pt < npts; pt++) {
        QPointF data = points[pt];
        double x = data.x();
        double y = data.y();

        classData.append((char *)&x, sizeof(double));
        classData.append((char *)&y, sizeof(double));
      }
    }

    return classData;
  }


  /**
   * Handle various events on the legend item. This will only be called for the
   *   legend item's events and handles drag & drop, context menus and similar
   *   actions. Basically all user interaction with legend items happen here.
   *
   * @param event The event which happened on the legend item.
   */
  void CubePlotCurve::mousePressEvent(QMouseEvent *event) {
    bool deleteThisCurve = false;
    if (event->button() == Qt::LeftButton) {
      QDrag *drag = new QDrag(m_legendItem);

      // The icon for drag & drop sometimes gets in the way of the image,
      //   so let's move the image a little more to the right of the cursor
      //   than usual.
      QPoint newHotSpot = drag->hotSpot();
      newHotSpot.setX(newHotSpot.x() * 2);
      drag->setHotSpot(newHotSpot);

      drag->setMimeData(createMimeData());
      drag->setPixmap( m_legendItem->grab(m_legendItem->rect()) );

      Qt::DropAction dropAction = drag->exec(Qt::CopyAction | Qt::MoveAction,
          Qt::CopyAction);

      if (dropAction == Qt::MoveAction) {
        deleteThisCurve = true;
      }
    }
    else if (event->button() == Qt::RightButton) {
      QMenu contextMenu;

      QAction *cutAct = new QAction(QIcon::fromTheme("edit-cut"), "Cut", this);
      contextMenu.addAction(cutAct);
      QAction *copyAct = new QAction(QIcon::fromTheme("edit-copy"), "Copy",
                                     this);
      // copyAct
      contextMenu.addAction(copyAct);

      contextMenu.addSeparator();

      QAction *deleteAct = new QAction(QIcon::fromTheme("edit-delete"), "Delete",
                                       this);
      contextMenu.addAction(deleteAct);

      contextMenu.addSeparator();

      QString configureIconFile =
          QString::fromStdString(FileName("$ISISROOT/appdata/images/icons/plot_configure.png").expanded());
      QAction *configureAct = new QAction(QPixmap(configureIconFile),
                                          "Configure...", this);
      contextMenu.addAction(configureAct);

      QAction *chosenAct = contextMenu.exec(
          m_legendItem->mapToGlobal(event->pos()));

      if (chosenAct == cutAct || chosenAct == copyAct) {
        QApplication::clipboard()->setMimeData(createMimeData());

        if (chosenAct == cutAct) {
          deleteThisCurve = true;
        }
      }
      // handle the configure action dialog
      // opens the dialog with only the right-clicked CubePlotCurve
      else if (chosenAct == configureAct) {
        CubePlotCurveConfigureDialog configure(this);
        configure.exec();
        emit needsRepaint();
      }
      else if (chosenAct == deleteAct) {
        deleteThisCurve = true;
      }
    }

    if (deleteThisCurve) {
      setColor(QColor(255, 255, 255, 0));
      emit needsRepaint();

      QwtPlot *plotToReplot = plot();

      connect(this, SIGNAL(removing()),
              this, SLOT(deleteLater()), Qt::QueuedConnection);
      connect(this, SIGNAL(removing()),
              plotToReplot, SLOT(replot()), Qt::QueuedConnection);
      emit removing();
    }
  }
}
