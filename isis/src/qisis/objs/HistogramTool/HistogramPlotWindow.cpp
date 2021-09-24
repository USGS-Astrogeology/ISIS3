#include "HistogramPlotWindow.h"

#include <qwt_scale_engine.h>

#include <QDockWidget>

#include "Histogram.h"
#include "HistogramItem.h"

namespace Isis {
  /**
   * Constructor, creates a new HistogramPlotWindow
   *
   * @param title
   * @param parent
   */
  HistogramPlotWindow::HistogramPlotWindow(QString title, QWidget *parent) :
                       PlotWindow(title, PlotCurve::CubeDN,
                                  PlotCurve::Percentage, parent) {
    //m_plot->enableAxis(QwtPlot::yRight);
    QwtText frequencyLabel(QString("Frequency"), QwtText::PlainText);

    frequencyLabel.setColor(Qt::darkCyan);
    QFont font = frequencyLabel.font();
    font.setPointSize(13);
    font.setBold(true);
    frequencyLabel.setFont(font);
    plot()->enableAxis(QwtPlot::yRight);
    plot()->setAxisTitle(QwtPlot::yRight, frequencyLabel);

    p_dock = new QDockWidget("Histogram Info", this);
    p_dock->setAllowedAreas(Qt::AllDockWidgetAreas);
    p_dock->setFloating(false);
    p_dock->setObjectName("DockWidget");
    p_dock->setMinimumWidth(130);
    addDockWidget(Qt::LeftDockWidgetArea, p_dock, Qt::Vertical);

//     QwtText *rtLabel = new QwtText("Percentage",
//                                    QwtText::PlainText);
//     rtLabel->setColor(Qt::red);
//     font = rtLabel->font();
//     font.setPointSize(13);
//     font.setBold(true);
//     rtLabel->setFont(font);
//     m_plot->setAxisTitle(QwtPlot::yRight, *rtLabel);

//     setAxisLabel(QwtPlot::xBottom, "Pixel Value (DN)");

//     setScale(QwtPlot::yRight, 0, 100);
    setPlotBackground(Qt::white);
  }


  /**
   * Add a HistogramItem to the plot.
   *
   *
   * @param hi
   */
  void HistogramPlotWindow::add(HistogramItem *hi) {
    hi->attach(plot());
    p_histItems.push_back(hi);
    plot()->replot();
  }
}
