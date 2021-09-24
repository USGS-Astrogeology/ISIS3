#include "IsisDebug.h"

#include "PlotCurve.h"

#include <iostream>

#include <qwt_text.h>
#include <qwt_plot.h>
#include <qwt_plot_curve.h>
#include <qwt_plot_marker.h>
#include <qwt_symbol.h>

#include <QBuffer>
#include <QByteArray>

#include "SpecialPixel.h"

namespace Isis {
  /**
   * Constructs and instance of a PlotCurve with some default
   * properties.
   */
  PlotCurve::PlotCurve(Units xUnits, Units yUnits)  : QwtPlotCurve() {
    m_markerSymbol = new QwtSymbol();
    m_markerSymbol->setStyle(QwtSymbol::NoSymbol);
    m_markerSymbol->setSize(6, 6);
    m_color = QColor(Qt::white);
    m_xUnits = xUnits;
    m_yUnits = yUnits;
  }


  PlotCurve::~PlotCurve() {
    if (plot()) {
      clearMarkers();
    }
  }


  /**
   * After attaching this curve to a plot, due to an inheritance/implementation
   *   complication with qwt the markers will remain detached until they are
   *   recreated. This forces the markers to be updated immediately.
   *
   * Please always call this method after attaching this curve to a plot.
   */
  void PlotCurve::attachMarkers() {
    recreateMarkers();
  }


  /**
   * This method returns the color of the curve
   *
   * @return QColor
   */
  QColor PlotCurve::color() const {
    return m_color;
  }


  /**
   * This method returns the shape of the markers.
   *
   * @return QwtSymbol *
   */
  QwtSymbol *PlotCurve::markerSymbol() const {
    return m_markerSymbol;
  }


  /**
   * Get the units of the x-axis double data.
   *
   * @return The units of the x-axis data
   */
  PlotCurve::Units PlotCurve::xUnits() const {
    return m_xUnits;
  }


  /**
   * Get the units of the y-axis double data.
   *
   * @return The units of the y-axis data
   */
  PlotCurve::Units PlotCurve::yUnits() const {
    return m_yUnits;
  }


  /**
   * Set the color of this curve and it's markers. This color will override the
   *   pen's color always.
   *
   * @param color The color of this curve.
   */
  void PlotCurve::setColor(const QColor &color) {
    //set the data for the curve
    m_color = color;
    setPen(pen());
  }


  /**
   * This method sets the data for the curve, then sets the value
   * for the markers associated with the curve.
   * @param data
   */
  void PlotCurve::setData(QwtSeriesData<QPointF> *data) {
    //set the data for the curve
    QwtPlotCurve::setData(data);
    recreateMarkers();
  }


  /**
   *  This method sets the shape of the markers.
   * @param style
   */
  void PlotCurve::setMarkerSymbol(QwtSymbol::Style style) {
    m_markerSymbol->setStyle(style);
    recreateMarkers();
  }


  /**
   * This method sets the visibility states of the markers at each value point.
   *
   * @param visible True to show markers, false to hide
   */
  void PlotCurve::setMarkerVisible(bool visible) {
    foreach (QwtPlotMarker *marker, m_valuePointMarkers) {
      marker->setVisible(visible);
    }
  }



  /**
   * Construct the plot curve given the past results of toByteArray(...).
   * This is used for copy/paste and drag/drop.
   *
   * @return The unconsumed part of the byte array.
   */
  QByteArray PlotCurve::fromByteArray(const QByteArray &classData) {
    QString expectedHeader("PLOT_CURVE_V1");
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

      int titleSize = *(((int *)(rawClassData + dataPos)));
      dataPos += sizeof(int);

      setTitle(QString::fromUtf8(classData.data() + dataPos, titleSize));
      dataPos += titleSize;

      m_xUnits = (Units)(*((int *)(rawClassData + dataPos)));
      dataPos += sizeof(int);

      m_yUnits = (Units)(*((int *)(rawClassData + dataPos)));
      dataPos += sizeof(int);

      // Read the pen...
      int penBufferSize = *(((int *)(rawClassData + dataPos)));
      dataPos += sizeof(int);

      QByteArray penBufferBytes(rawClassData + dataPos, penBufferSize);
      dataPos += penBufferSize;

      QBuffer penDataBuffer(&penBufferBytes);
      penDataBuffer.open(QIODevice::ReadWrite);

      QDataStream penDataStream(&penDataBuffer);
      QPen pen;
      penDataStream >> pen;
      setPen(pen);


      // Read the color...
      int colorBufferSize = *(((int *)(rawClassData + dataPos)));
      dataPos += sizeof(int);

      QByteArray colorBufferBytes(rawClassData + dataPos, colorBufferSize);
      dataPos += colorBufferSize;

      QBuffer colorDataBuffer(&colorBufferBytes);
      colorDataBuffer.open(QIODevice::ReadWrite);

      QDataStream colorDataStream(&colorDataBuffer);
      QColor newColor;
      colorDataStream >> newColor;
      setColor(newColor);

      // Read the marker symbol...
      int markerSymbolBufferSize = *(((int *)(rawClassData + dataPos)));
      dataPos += sizeof(int);

      QByteArray markerSymbolBufferBytes(rawClassData + dataPos,
                                         markerSymbolBufferSize);
      dataPos += markerSymbolBufferSize;

      QBuffer markerSymbolDataBuffer(&markerSymbolBufferBytes);
      markerSymbolDataBuffer.open(QIODevice::ReadWrite);

      QDataStream markerSymbolDataStream(&markerSymbolDataBuffer);

      QBrush markerBrush;
      markerSymbolDataStream >> markerBrush;
      m_markerSymbol->setBrush(markerBrush);

      QPen markerPen;
      markerSymbolDataStream >> markerPen;
      m_markerSymbol->setPen(markerPen);

      QSize markerSize;
      markerSymbolDataStream >> markerSize;
      m_markerSymbol->setSize(markerSize);

      int markerStyle;
      markerSymbolDataStream >> markerStyle;
      m_markerSymbol->setStyle((QwtSymbol::Style)markerStyle);

      // Done reading the more advanced items, finish up with the data
      int plotDataSize = *((int *)(rawClassData + dataPos));
      dataPos += sizeof(int);
      QVector<QPointF> plotDataValues;

      for (int i = 0; i < plotDataSize; i ++) {
        double x = *((double *)(rawClassData + dataPos));
        dataPos += sizeof(double);

        double y = *((double *)(rawClassData + dataPos));
        dataPos += sizeof(double);

        plotDataValues.append(QPointF(x, y));
      }

      setData(new QwtPointSeriesData(plotDataValues));
      ASSERT(dataPos <= classData.size());

      return classData.right(classData.size() - dataPos);
    }
    else {
      IString msg = "The given byte array is not large enough to contain the "
          "required header";
      throw IException(IException::Programmer, msg, _FILEINFO_);
    }
  }


  QByteArray PlotCurve::toByteArray() const {
    QByteArray classData;

    QString header("PLOT_CURVE_V1");
    classData.append(header.toUtf8());

    QByteArray titleArray = title().text().toUtf8();
    int size = titleArray.size();
    classData.append((char *)&size, sizeof(int));
    classData.append(titleArray);

    int xUnitsInt = (int)m_xUnits;
    int yUnitsInt = (int)m_yUnits;
    classData.append((char *)&xUnitsInt, sizeof(int));
    classData.append((char *)&yUnitsInt, sizeof(int));

    // Store the pen... to do this we need to serialize using QPen's operators
    QBuffer penDataBuffer;
    penDataBuffer.open(QIODevice::ReadWrite);

    QDataStream penDataStream(&penDataBuffer);
    penDataStream << pen();
    penDataBuffer.seek(0);

    size = penDataBuffer.buffer().size();
    classData.append((char *)&size, sizeof(int));
    classData.append(penDataBuffer.buffer());

    // Store the color...
    QBuffer colorDataBuffer;
    colorDataBuffer.open(QIODevice::ReadWrite);

    QDataStream colorDataStream(&colorDataBuffer);
    colorDataStream << m_color;
    colorDataBuffer.seek(0);

    size = colorDataBuffer.buffer().size();
    classData.append((char *)&size, sizeof(int));
    classData.append(colorDataBuffer.buffer());

    // Store the marker symbol...
    QBuffer markerSymbolDataBuffer;
    markerSymbolDataBuffer.open(QIODevice::ReadWrite);

    QDataStream markerSymbolDataStream(&markerSymbolDataBuffer);
    markerSymbolDataStream << m_markerSymbol->brush();
    markerSymbolDataStream << m_markerSymbol->pen();
    markerSymbolDataStream << m_markerSymbol->size();
    markerSymbolDataStream << (int)m_markerSymbol->style();
    markerSymbolDataBuffer.seek(0);

    size = markerSymbolDataBuffer.buffer().size();
    classData.append((char *)&size, sizeof(int));
    classData.append(markerSymbolDataBuffer.buffer());

    // Store the X/Y plot values
    const QwtSeriesData<QPointF> &plotData = *data();
    size = plotData.size();
    classData.append((char *)&size, sizeof(int));

    for (int i = 0; i < size; i ++) {
      double x = plotData.sample(i).x();
      double y = plotData.sample(i).y();

      classData.append((char *)&x, sizeof(double));
      classData.append((char *)&y, sizeof(double));
    }

    return classData;
  }


  /**
   * Sets the plot pen to the passed-in pen.
   *
   *
   * @param pen
   */
  void PlotCurve::setPen(const QPen &pen) {
    QPen newPen(pen);
    newPen.setColor(m_color);

    QwtPlotCurve::setPen(newPen);

    recreateMarkers();
  }


  void PlotCurve::clearMarkers() {
    foreach (QwtPlotMarker *marker, m_valuePointMarkers) {
      marker->detach();
      //delete marker;
    }

    m_valuePointMarkers.clear();
  }


  void PlotCurve::recreateMarkers() {
    bool markersVisible = true;
    if (m_valuePointMarkers.size()) {
      markersVisible = m_valuePointMarkers.first()->isVisible();
    }
    clearMarkers();

    QPen markerPen = m_markerSymbol->pen();
    markerPen.setColor(m_color);
    m_markerSymbol->setPen(markerPen);

    const QwtSeriesData<QPointF> &plotData = *data();
    for(unsigned int i = 0; i < plotData.size(); i++) {
      QwtPlotMarker *newMarker = new QwtPlotMarker();
      newMarker->setValue(plotData.sample(i).x(), plotData.sample(i).y());
      newMarker->setAxes(xAxis(), yAxis());
      newMarker->setSymbol(m_markerSymbol);
      newMarker->setVisible(markersVisible);
      newMarker->attach(plot());
      m_valuePointMarkers.append(newMarker);
    }
  }
}
