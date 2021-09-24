#include "HistogramWidget.h"

#include <QHBoxLayout>
#include <QLayout>
#include <QLabel>
#include <QString>
#include <QColor>

#include <qwt_symbol.h>
#include <qwt_scale_div.h>
#include <qwt_plot_zoomer.h>
#include <qwt_scale_engine.h>
#include <qwt_text.h>

namespace Isis {
  /**
   * HistogramWidget constructor. Initializes all of the widgets and sets the plot
   * title, histogram curve's color and stretch curve's color.
   *
   * @param title
   * @param histColor
   * @param stretchColor
   */
  HistogramWidget::HistogramWidget(const QString title, const QColor histColor, const QColor stretchColor) :
    QwtPlot(QwtText(title)) {
    setCanvasBackground(Qt::white);
    enableAxis(QwtPlot::yRight);
    setAxisScale(QwtPlot::xBottom, 0, 255);
    setAxisLabelRotation(QwtPlot::xBottom, 45);
    setAxisScale(QwtPlot::yRight, 0, 255);

    QwtText axisTitle;
    QFont axisFont;
    axisFont.setBold(true);
    axisTitle.setFont(axisFont);
    axisTitle.setText("Frequency");
    setAxisTitle(QwtPlot::yLeft, axisTitle);
    axisTitle.setText("Input (Cube DN)");
    setAxisTitle(QwtPlot::xBottom, axisTitle);
    axisTitle.setText("Output");
    setAxisTitle(QwtPlot::yRight, axisTitle);

    p_histCurve = new HistogramItem();
    p_histCurve->setColor(histColor);

    p_stretchCurve = new QwtPlotCurve();
    p_stretchCurve->setYAxis(QwtPlot::yRight);
    p_stretchCurve->setPen(QPen(QBrush(stretchColor), 2, Qt::DashLine));
    p_stretchCurve->setSymbol(new QwtSymbol(QwtSymbol::Ellipse, QBrush(stretchColor), QPen(stretchColor), QSize(5, 5)));

    p_histCurve->attach(this);
    p_stretchCurve->attach(this);

    p_zoomer = new QwtPlotZoomer(canvas());
    p_zoomer->setZoomBase();
  }


  /**
   * Creates a histogram curve from the given histogram and plots it.
   *
   * @param hist
   */
  void HistogramWidget::setHistogram(const Histogram &hist) {
    std::vector<double> xarray, yarray;
    for(int i = 0; i < hist.Bins(); i++) {
      if(hist.BinCount(i) > 0) {
        xarray.push_back(hist.BinMiddle(i));

        double freq = (double)hist.BinCount(i) / (double)hist.MaxBinCount();
        yarray.push_back(freq * 100.0);
      }
    }

    //These are all variables needed in the following for loop.
    //----------------------------------------------
    QVector<QwtIntervalSample> intervals(xarray.size());
    double maxYValue = DBL_MIN;
    double minYValue = DBL_MAX;
    // ---------------------------------------------

    for(unsigned int y = 0; y < yarray.size(); y++) {
      intervals[y].interval = QwtInterval(xarray[y], xarray[y] + hist.BinSize());

      intervals[y].value = yarray[y];
      if(yarray[y] > maxYValue)
        maxYValue = yarray[y];
      if(yarray[y] < minYValue)
        minYValue = yarray[y];
    }

    QwtScaleDiv scaleDiv;

    p_histCurve->setData(QwtIntervalSeriesData(intervals));

    double min = hist.Minimum();
    double max = hist.Maximum();
    int maxMajor = 5;
    int maxMinor = 20;

    // Find a good, fixed, axis scale
    QwtScaleEngine *engine = axisScaleEngine(QwtPlot::xBottom);
    QwtScaleDiv scale = engine->divideScale(min, max, maxMajor, maxMinor);
    QwtInterval interval = scale.interval();
    setAxisScale(QwtPlot::xBottom,
                 interval.minValue() - hist.BinSize(),
                 interval.maxValue() + hist.BinSize());
    p_zoomer->setZoomBase();
  }

  /**
   * Creates a stretch curbe from the given stretch and plots it.
   *
   * @param stretch
   */
  void HistogramWidget::setStretch(Stretch stretch) {
    QVector<QPointF> curvePoints(stretch.Pairs());
    for(int i = 0; i < stretch.Pairs(); i++) {
      curvePoints[i] = QPointF(stretch.Input(i), stretch.Output(i));
    }

    p_stretchCurve->setData(new QwtPointSeriesData(curvePoints));
    replot();
  }

  /**
   * Clears the stretch curve from the plot.
   *
   */
  void HistogramWidget::clearStretch() {
    p_stretchCurve->setData(new QwtPointSeriesData());
    replot();
  }
}
