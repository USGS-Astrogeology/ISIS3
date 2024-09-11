#include "ScatterPlotWindow.h"

#include <QMenuBar>
#include <QVector>

#include <qwt_color_map.h>
#include <qwt_interval.h>
#include <qwt_plot.h>
#include <qwt_plot_layout.h>
#include <qwt_plot_panner.h>
#include <qwt_plot_spectrogram.h>
#include <qwt_plot_zoomer.h>
#include <qwt_scale_draw.h>
#include <qwt_scale_widget.h>
#include <qwt_text.h>

#include "Histogram.h"
#include "MdiCubeViewport.h"
#include "Portal.h"
#include "ScatterPlotAlarmConfigDialog.h"
#include "ScatterPlotData.h"
#include "ScatterPlotTool.h"
#include "ViewportBuffer.h"
#include "ViewportMainWindow.h"
#include "Workspace.h"

using namespace std;

namespace Isis {
  /**
   * Create a scatter plot window with the given data. This will fully populate
   *   the window with scatter plot data automatically.
   *
   * @param title The window title and plot title
   * @param xAxisCube The cube to use for reading X values
   * @param xAxisBand The band of the x axis cube to read
   * @param xAxisBinCount The resolution of the x axis data
   * @param yAxisCube The cube to use for reading X values
   * @param yAxisBand The band of the y axis cube to read
   * @param yAxisBinCount The resolution of the y axis data
   * @param sampleRange The sample range, inclusive 1-based, to read data from
   * @param lineRange The line range, inclusive 1-based, to read data from
   * @param parent The Qt-parent relationship parent widget
   */
  ScatterPlotWindow::ScatterPlotWindow(QString title,
      Cube *xAxisCube, int xAxisBand, int xAxisBinCount,
      Cube *yAxisCube, int yAxisBand, int yAxisBinCount,
      QwtInterval sampleRange, QwtInterval lineRange,
      QWidget *parent) :
      PlotWindow("Scatter Plot", PlotCurve::CubeDN, PlotCurve::CubeDN, parent,
                 (MenuOptions)(
                   AllMenuOptions &
                     ~BackgroundSwitchMenuOption &
                     ~ShowTableMenuOption &
                     ~ClearPlotMenuOption &
                     ~ShowHideMarkersMenuOption &
                     ~ShowHideCurvesMenuOption &
                     ~ConfigurePlotMenuOption)) {
    m_xAxisCube = xAxisCube;
    m_yAxisCube = yAxisCube;
    m_xAxisCubeBand = xAxisBand;
    m_yAxisCubeBand = yAxisBand;

    m_sampleRange = sampleRange;
    m_lineRange = lineRange;

    m_xCubeDnAlarmRange.first = Null;
    m_xCubeDnAlarmRange.second = Null;
    m_yCubeDnAlarmRange.first = Null;
    m_yCubeDnAlarmRange.second = Null;

    ScatterPlotData *data = new ScatterPlotData(
        xAxisCube, xAxisBand, xAxisBinCount,
        yAxisCube, yAxisBand, yAxisBinCount,
        sampleRange, lineRange);

    m_spectrogram = new QwtPlotSpectrogram;

    m_spectrogram->setData(data);
    m_spectrogram->setTitle("Scatter Plot Counts");
    m_spectrogram->attach(plot());

    disableAxisAutoScale();
    zoomer()->zoom(0);
    plot()->setAxisScale(QwtPlot::xBottom, data->xCubeMin(), data->xCubeMax());
    plot()->setAxisScale(QwtPlot::yLeft, data->yCubeMin(), data->yCubeMax());
    zoomer()->setZoomBase();
    replot();

    QwtScaleWidget *rightAxis = plot()->axisWidget(QwtPlot::yRight);
    rightAxis->setTitle("Counts");
    rightAxis->setColorBarEnabled(true);
//    rightAxis->setColorMap(m_spectrogram->data()->interval(Qt::ZAxis),
//                           m_spectrogram->colorMap());

    plot()->setAxisScale(QwtPlot::yRight,
                         m_spectrogram->data()->interval(Qt::ZAxis).minValue(),
                         m_spectrogram->data()->interval(Qt::ZAxis).maxValue());
    plot()->enableAxis(QwtPlot::yRight);

    plot()->setAxisTitle(QwtPlot::xBottom,
        QFileInfo(xAxisCube->fileName()).baseName() + " Band " +
        QString::number(xAxisBand) + " " +
        plot()->axisTitle(QwtPlot::xBottom).text());
    plot()->setAxisTitle(QwtPlot::yLeft,
        QFileInfo(yAxisCube->fileName()).baseName() + " Band " +
        QString::number(yAxisBand) + " " +
        plot()->axisTitle(QwtPlot::yLeft).text());

    QList<double> contourLevels;
    QwtInterval range = m_spectrogram->data()->interval(Qt::ZAxis);

    for (double level = range.minValue();
         level < range.maxValue();
         level += ((range.maxValue() - range.minValue()) / 6.0)) {
      contourLevels += level;
    }

    m_spectrogram->setContourLevels(contourLevels);

    m_colorize = new QAction(this);
    m_colorize->setText("Colorize");
    m_colorize->setIcon(QPixmap(QString::fromStdString(FileName("$ISISROOT/appdata/images/icons/rgb.png").expanded())));
    connect(m_colorize, SIGNAL(triggered()),
            this, SLOT(colorPlot()));

    m_contour = new QAction(this);
    m_contour->setText("Hide Contour Lines");
    m_contour->setIcon(
        QPixmap(QString::fromStdString(FileName("$ISISROOT/appdata/images/icons/scatterplotcontour.png").expanded())));
    connect(m_contour, SIGNAL(triggered()),
            this, SLOT(showHideContour()));

    QAction *configureAlarmingAct = new QAction(this);
    configureAlarmingAct->setText("Change Alarming");
    configureAlarmingAct->setIcon(
        QPixmap(QString::fromStdString(FileName("$ISISROOT/appdata/images/icons/scatterplotalarming.png").expanded())));
    connect(configureAlarmingAct, SIGNAL(triggered()),
            this, SLOT(configureAlarming()));

    foreach (QAction *menuAction, menuBar()->actions()) {
      if (menuAction->text() == "&Options") {
        QMenu *optsMenu = qobject_cast<QMenu *>(menuAction->parentWidget());
        optsMenu->addAction(m_colorize);
        optsMenu->addAction(m_contour);
        optsMenu->addAction(configureAlarmingAct);
      }
    }

    colorPlot();
    showHideContour();

    plot()->canvas()->installEventFilter(this);
    plot()->canvas()->setMouseTracking(true);

    replot();

    QString instanceName = windowTitle();
    FileName config("$HOME/.Isis/qview/" + instanceName.toStdString() + ".config");
    QSettings settings(QString::fromStdString(config.expanded()),
                       QSettings::NativeFormat);
    m_alarmPlot = settings.value("alarmOntoPlot", true).toBool();
    m_alarmViewport = settings.value("alarmOntoViewport", true).toBool();

    m_alarmPlotSamples = settings.value("alarmPlotSamples", 25).toInt();
    m_alarmPlotLines = settings.value("alarmPlotLines", 25).toInt();

    m_alarmViewportUnits = (AlarmRangeUnits)settings.value("alarmViewportUnits",
                                                           ScreenUnits).toInt();

    m_alarmViewportScreenWidth =
        settings.value("alarmViewportScreenWidth", 5).toInt();
    m_alarmViewportScreenHeight =
        settings.value("alarmViewportScreenHeight", 5).toInt();

    m_alarmViewportXDnBoxSize =
        settings.value("alarmViewportXDnBoxSize", 1.0).toDouble();
    m_alarmViewportYDnBoxSize =
        settings.value("alarmViewportYDnBoxSize", 1.0).toDouble();
  }


  ScatterPlotWindow::~ScatterPlotWindow() {
    QString instanceName = windowTitle();
    FileName config("$HOME/.Isis/qview/" + instanceName.toStdString() + ".config");
    QSettings settings(QString::fromStdString(config.expanded()),
                       QSettings::NativeFormat);
    settings.setValue("alarmOntoPlot", m_alarmPlot);
    settings.setValue("alarmOntoViewport", m_alarmViewport);

    settings.setValue("alarmPlotSamples", m_alarmPlotSamples);
    settings.setValue("alarmPlotLines", m_alarmPlotLines);

    settings.setValue("alarmViewportUnits", (int)m_alarmViewportUnits);

    settings.setValue("alarmViewportScreenWidth", m_alarmViewportScreenWidth);
    settings.setValue("alarmViewportScreenHeight", m_alarmViewportScreenHeight);

    settings.setValue("alarmViewportXDnBoxSize", m_alarmViewportXDnBoxSize);
    settings.setValue("alarmViewportYDnBoxSize", m_alarmViewportYDnBoxSize);
  }


  /**
   * This indicates if we are alarming from viewport to plot.
   *
   * @return True if alarming viewport->plot, false otherwise
   */
  bool ScatterPlotWindow::alarmingPlot() const {
    return m_alarmPlot;
  }


  /**
   * This indicates if we are alarming from plot to viewport.
   *
   * @return True if alarming plot->viewport, false otherwise
   */
  bool ScatterPlotWindow::alarmingViewport() const {
    return m_alarmViewport;
  }


  /**
   * This is the sample/line box sizes for alarming from viewport to plot.
   *
   * @return Sample Box Size, Line Box Size
   */
  QPair<int, int> ScatterPlotWindow::alarmPlotBoxSize() const {
    return QPair<int, int>(m_alarmPlotSamples, m_alarmPlotLines);
  }


  /**
   * This is the active alarming units for plot->viewport. We either alarm a
   *   screen pixel box size or a cube sample/line range around the mouse.
   *
   * @return The units used for alarming plot->viewport
   */
  ScatterPlotWindow::AlarmRangeUnits
      ScatterPlotWindow::alarmViewportUnits() const {
    return m_alarmViewportUnits;
  }


  /**
   * This is the alarming box size for plot->viewport in screen units. If the
   *   current units are not screen units, this is not actively alarming the
   *   given box. These values are valid even when alarming is a Cube DN
   *   box, just not use, and they are never translated/re-calculated based on
   *   the conversion from screen pixels to cube pixels.
   *
   * @return The screen pixel alarming box size
   */
  QPair<int, int> ScatterPlotWindow::alarmViewportScreenBoxSize() const {
    return QPair<int, int>(m_alarmViewportScreenWidth,
                           m_alarmViewportScreenHeight);
  }


  /**
   * This is the alarming box size for plot->viewport in cube units (number of
   *   samples/lines). If the current units are not cube units then this is not
   *   actively alarming the given box. These values are valid even when
   *   alarming is a screen pixel box, just not use, and they are never
   *   translated/re-calculated based on the conversion from screen pixels to
   *   cube pixels.
   *
   * @return The sample/line alarming box size
   */
  QPair<double, double> ScatterPlotWindow::alarmViewportDnBoxSize() const {
    return QPair<double, double>(m_alarmViewportXDnBoxSize,
                                 m_alarmViewportYDnBoxSize);
  }


  /**
   * We override events done on the plot canvas for alarming purposes. This
   *   method will forward mouse moves and leaves to the appropriate methods.
   *
   * @param o The object on which the event happened
   * @param e The event that triggered this method call
   * @return True if no more processing should happen with this event
   */
  bool ScatterPlotWindow::eventFilter(QObject *o, QEvent *e) {
    if (o == plot()->canvas()) {
      switch (e->type()) {
        case QEvent::MouseMove: {
          if (((QMouseEvent *)e)->buttons() == Qt::NoButton)
            mouseMoveEvent((QMouseEvent *)e);
          break;
        }
        case QEvent::Leave: {
          mouseLeaveEvent((QMouseEvent *)e);
          break;
        }
        default:
          break;
      }

      return false;
    }
    else {
      return PlotWindow::eventFilter(o, e);
    }
  }


  /**
   * If the viewport is showing the x axis cube data or y axis cube data,
   *   and alarming is enabled, this paints alarmed values from the plot onto
   *   the viewport.
   *
   * @param vp The viewport that might need to be alarmed/painted red
   * @param painter The painter to paint with
   */
  void ScatterPlotWindow::paint(MdiCubeViewport *vp, QPainter *painter) {
    PlotWindow::paint(vp, painter);

    // Do alarming from plot onto viewport
    if (alarmingViewport() &&
        !IsSpecial(m_xCubeDnAlarmRange.first) &&
        !IsSpecial(m_xCubeDnAlarmRange.second) &&
        !IsSpecial(m_yCubeDnAlarmRange.first) &&
        !IsSpecial(m_yCubeDnAlarmRange.second)) {
      painter->setPen(QPen(Qt::red));

      ViewportBuffer *buffer = vp->grayBuffer();

      if ((isXCube(vp) || isYCube(vp)) && buffer && !buffer->working()) {
        int numLines = buffer->bufferXYRect().height();

        QScopedPointer<Portal> portal;

        // We are going to read DNs from the cube that isn't in the passed in
        //   viewport. For example, if we're painting X, we're missing the
        //   corresponding Y DN values.
        if (isXCube(vp))
          portal.reset(new Portal(1, 1, m_yAxisCube->pixelType()));
        else
          portal.reset(new Portal(1, 1, m_xAxisCube->pixelType()));

        // Iterate through the in-memory DN values for the passed in viewport
        for (int yIndex = 0; yIndex < numLines; yIndex++) {
          const vector<double> &line = buffer->getLine(yIndex);

          for (int xIndex = 0; xIndex < (int)line.size(); xIndex++) {
            int viewportPointX = xIndex + buffer->bufferXYRect().left();
            int viewportPointY = yIndex + buffer->bufferXYRect().top();

            double cubeSample = Null;
            double cubeLine = Null;
            vp->viewportToCube(viewportPointX, viewportPointY,
                               cubeSample, cubeLine);

            // The sample/line range is the actual scatter plotted sample/line
            //   range. Don't alarm outside of this range on the cube ever.
            if (cubeSample >= m_sampleRange.minValue() - 0.5 &&
                cubeSample <= m_sampleRange.maxValue() + 0.5 &&
                cubeLine >= m_lineRange.minValue() - 0.5 &&
                cubeLine <= m_lineRange.maxValue() + 0.5) {
              // If the in-memory DN values are within the alarm box range, check
              //   the corresponding DN values for the other axis via cube I/O.
              if (isXCube(vp) &&
                  line[xIndex] >= m_xCubeDnAlarmRange.first &&
                  line[xIndex] <= m_xCubeDnAlarmRange.second) {
                portal->SetPosition(cubeSample, cubeLine, m_yAxisCubeBand);
                m_yAxisCube->read(*portal);

                double yDnValue = (*portal)[0];

                if (yDnValue >= m_yCubeDnAlarmRange.first &&
                    yDnValue <= m_yCubeDnAlarmRange.second) {
                  painter->drawPoint(viewportPointX, viewportPointY);
                }
              }
              else if (isYCube(vp) &&
                       line[xIndex] >= m_yCubeDnAlarmRange.first &&
                       line[xIndex] <= m_yCubeDnAlarmRange.second) {
                portal->SetPosition(cubeSample, cubeLine, m_xAxisCubeBand);
                m_xAxisCube->read(*portal);

                double xDnValue = (*portal)[0];

                if (xDnValue >= m_xCubeDnAlarmRange.first &&
                    xDnValue <= m_xCubeDnAlarmRange.second) {
                  painter->drawPoint(viewportPointX, viewportPointY);
                }
              }
            }
          }
        }
      }
    }
  }


  /**
   * Saves the current mouse position in the viewport so that the plot can be
   *   alarmed around that position.
   *
   * @param vp The viewport that received the mouse event
   * @param mouseLoc The location on the viewport wheere the mouse is, in
   *                 screen coordinates.
   */
  void ScatterPlotWindow::setMousePosition(MdiCubeViewport *vp,
                                           QPoint mouseLoc) {
    ScatterPlotData *scatterData =
        dynamic_cast<ScatterPlotData *>(m_spectrogram->data());

    if (scatterData) {
      scatterData->clearAlarms();

      if (alarmingPlot() && (isXCube(vp) || isYCube(vp))) {
        QScopedPointer<Portal> xCubePortal(
            new Portal(m_alarmPlotSamples, m_alarmPlotLines,
                       m_xAxisCube->pixelType()));
        QScopedPointer<Portal> yCubePortal(
            new Portal(m_alarmPlotSamples, m_alarmPlotLines,
                       m_yAxisCube->pixelType()));

        double cubeSample = Null;
        double cubeLine = Null;

        vp->viewportToCube(mouseLoc.x(), mouseLoc.y(), cubeSample, cubeLine);

        // The sample/line range is the actual scatter plotted sample/line
        //   range. Don't alarm outside of this range on the cube ever.
        if (cubeSample >= m_sampleRange.minValue() - 0.5 &&
            cubeSample <= m_sampleRange.maxValue() + 0.5 &&
            cubeLine >= m_lineRange.minValue() - 0.5 &&
            cubeLine <= m_lineRange.maxValue() + 0.5) {
          xCubePortal->SetPosition(cubeSample, cubeLine, m_xAxisCubeBand);
          m_xAxisCube->read(*xCubePortal);
          yCubePortal->SetPosition(cubeSample, cubeLine, m_yAxisCubeBand);
          m_yAxisCube->read(*yCubePortal);

          for (int i = 0; i < xCubePortal->size(); i++) {
            double x = (*xCubePortal)[i];
            double y = (*yCubePortal)[i];

            if (!IsSpecial(x) && !IsSpecial(y)) {
              scatterData->alarm(x, y);
            }
          }
        }
      }

      plot()->replot();
    }
  }


  /**
   * This enables or disables alarming viewport->plot.
   *
   * @param alarming True to enable alarming viewport->plot, false to disable
   *                 alarming viewport->plot
   */
  void ScatterPlotWindow::setAlarmingPlot(bool alarming) {
    m_alarmPlot = alarming;
  }


  /**
   * This enables or disables alarming plot->viewport.
   *
   * @param alarming True to enable alarming plot->viewport, false to disable
   *                 alarming plot->viewport
   */
  void ScatterPlotWindow::setAlarmingViewport(bool alarming) {
    m_alarmViewport = alarming;
  }


  /**
   * This sets the box size for alarming viewport->plot in cube samples/lines.
   *
   * @param samples How many samples (total) the alarming box size should be.
   *                This should be odd because the mouse is in the center.
   * @param lines How many lines (total) the alarming box size should be.
   *                This should be odd because the mouse is in the center.
   */
  void ScatterPlotWindow::setAlarmPlotBoxSize(int samples, int lines) {
    m_alarmPlotSamples = samples;
    m_alarmPlotLines = lines;
  }


  /**
   * This sets the units to be used for alarming plot->viewport.
   *
   * @param units The units (screen or cube DN range) to use for alarming.
   */
  void ScatterPlotWindow::setAlarmViewportUnits(AlarmRangeUnits units) {
    m_alarmViewportUnits = units;
  }


  /**
   * This sets the screen pixel box size for alarming plot->viewport. If the
   *   current alarming units for plot->viewport isn't screen pixels, these
   *   values will still be stored off and just not be active until the units
   *   are changed.
   *
   * @param width The screen pixels (total) around the mouse in the X direction
   *              to be used for alarming.
   * @param height The screen pixels (total) around the mouse in the Y direction
   *              to be used for alarming.
   */
  void ScatterPlotWindow::setAlarmViewportScreenBoxSize(int width, int height) {
    m_alarmViewportScreenWidth = width;
    m_alarmViewportScreenHeight = height;
  }


  /**
   * This sets the cube DN box size for alarming plot->viewport. If the
   *   current alarming units for plot->viewport isn't cube pixels, these
   *   values will still be stored off and just not be active until the units
   *   are changed.
   *
   * @param xDnBoxSize The Cube DN box size (total) around the mouse in the X
   *                   direction to be used for alarming.
   * @param yDnBoxSize The Cube DN box size (total) around the mouse in the Y
   *                   direction to be used for alarming.
   */
  void ScatterPlotWindow::setAlarmViewportDnBoxSize(double xDnBoxSize,
                                                    double yDnBoxSize) {
    m_alarmViewportXDnBoxSize = xDnBoxSize;
    m_alarmViewportYDnBoxSize = yDnBoxSize;
  }


  /**
   * This causes the window to lose it's pointers to the input cubes. When a
   *   viewport is closed, this will prevent using the cube pointers still for
   *   alarming.
   */
  void ScatterPlotWindow::forgetCubes() {
    m_xAxisCube = NULL;
    m_yAxisCube = NULL;
  }


  /**
   * This method switches the color mode of the scatter plot from
   * black and white to color and visa versa.
   *
   */
  void ScatterPlotWindow::colorPlot() {
    if (m_colorize->text().compare("Colorize") == 0) {
      m_colorize->setIcon(QPixmap(QString::fromStdString(FileName("$ISISROOT/appdata/images/icons/gray.png").expanded())));
      m_colorize->setText("Gray");
      QwtLinearColorMap *colorMap = new QwtLinearColorMap(Qt::darkCyan, Qt::red);
      colorMap->addColorStop(DBL_EPSILON, Qt::cyan);
      colorMap->addColorStop(0.3, Qt::green);
      colorMap->addColorStop(0.50, Qt::yellow);
      m_spectrogram->setColorMap(colorMap);
      plot()->setCanvasBackground(Qt::darkCyan);
    }
    else {
      m_colorize->setIcon(QPixmap(QString::fromStdString(FileName("$ISISROOT/appdata/images/icons/rgb.png").expanded())));
      m_colorize->setText("Colorize");
      QwtLinearColorMap *colorMap = new QwtLinearColorMap(Qt::black, Qt::white);
      colorMap->addColorStop(DBL_EPSILON, Qt::darkGray);
      m_spectrogram->setColorMap(colorMap);
      plot()->setCanvasBackground(Qt::black);
    }

//    plot()->axisWidget(QwtPlot::yRight)->setColorMap(
//        m_spectrogram->interval(Qt::ZAxis),
//        m_spectrogram->colorMap());
    updateContourPen();

    replot();
  }


  /**
   * This method hides or displays the contour lines on the
   * spectrogram.
   *
   */
  void ScatterPlotWindow::showHideContour() {
    if (m_contour->text() == "Show Contour Lines") {
      m_contour->setText("Hide Contour Lines");
      m_spectrogram->setDisplayMode(QwtPlotSpectrogram::ContourMode, true);
      updateContourPen();
    }
    else {
      m_contour->setText("Show Contour Lines");
      m_spectrogram->setDisplayMode(QwtPlotSpectrogram::ContourMode, false);
    }

    replot();
  }


  /**
   * Returns true if the viewport's cube is the cube currently being used on the
   *   x-axis.
   *
   * @param vp The viewport to test for a cube match
   * @return True if the vp is showing the x axis cube data
   */
  bool ScatterPlotWindow::isXCube(MdiCubeViewport *vp) const {
    return (vp && m_xAxisCube &&
            vp->cube() == m_xAxisCube && vp->grayBand() == m_xAxisCubeBand &&
            vp->isGray());
  }


  /**
   * Returns true if the viewport's cube is the cube currently being used on the
   *   y-axis.
   *
   * @param vp The viewport to test for a cube match
   * @return True if the vp is showing the y axis cube data
   */
  bool ScatterPlotWindow::isYCube(MdiCubeViewport *vp) const {
    return (vp && m_yAxisCube &&
            vp->cube() == m_yAxisCube && vp->grayBand() == m_yAxisCubeBand &&
            vp->isGray());
  }


  /**
   * When the mosue moves, this updates the alarming information and causes
   *   repaints on the cube viewports in order to show the alarming
   *   appropriately.
   *
   * @param e The event that caused this method to be called
   */
  void ScatterPlotWindow::mouseMoveEvent(QMouseEvent *e) {
    if (alarmingViewport()) {
      if (m_alarmViewportUnits == ScreenUnits) {
        m_xCubeDnAlarmRange.first = plot()->invTransform(
            QwtPlot::xBottom, e->pos().x() - m_alarmViewportScreenWidth / 2);
        m_xCubeDnAlarmRange.second = plot()->invTransform(
            QwtPlot::xBottom, e->pos().x() + m_alarmViewportScreenWidth / 2);

        m_yCubeDnAlarmRange.first = plot()->invTransform(
            QwtPlot::yLeft, e->pos().y() + m_alarmViewportScreenHeight / 2);
        m_yCubeDnAlarmRange.second = plot()->invTransform(
            QwtPlot::yLeft, e->pos().y() - m_alarmViewportScreenHeight / 2);

        if (m_xCubeDnAlarmRange.first > m_xCubeDnAlarmRange.second)
          std::swap(m_xCubeDnAlarmRange.first, m_xCubeDnAlarmRange.second);

        if (m_yCubeDnAlarmRange.first > m_yCubeDnAlarmRange.second)
          std::swap(m_yCubeDnAlarmRange.first, m_yCubeDnAlarmRange.second);
      }
      else {
        m_xCubeDnAlarmRange.first = plot()->invTransform(
            QwtPlot::xBottom, e->pos().x()) - m_alarmViewportXDnBoxSize / 2.0;
        m_xCubeDnAlarmRange.second = plot()->invTransform(
            QwtPlot::xBottom, e->pos().x()) + m_alarmViewportXDnBoxSize / 2.0;

        m_yCubeDnAlarmRange.first = plot()->invTransform(
            QwtPlot::yLeft, e->pos().y()) - m_alarmViewportYDnBoxSize / 2.0;
        m_yCubeDnAlarmRange.second = plot()->invTransform(
            QwtPlot::yLeft, e->pos().y()) + m_alarmViewportYDnBoxSize / 2.0;
      }
    }
    else {
      m_xCubeDnAlarmRange.first = Null;
      m_xCubeDnAlarmRange.second = Null;
      m_yCubeDnAlarmRange.first = Null;
      m_yCubeDnAlarmRange.second = Null;
    }

    emit plotChanged();
  }


  /**
   * When the mouse leaves the plot canvas we disable all alarming from
   *   plot->viewport.
   *
   * @param e The event that caused this method to be called
   */
  void ScatterPlotWindow::mouseLeaveEvent(QMouseEvent *e) {
    m_xCubeDnAlarmRange.first = Null;
    m_xCubeDnAlarmRange.second = Null;
    m_yCubeDnAlarmRange.first = Null;
    m_yCubeDnAlarmRange.second = Null;

    emit plotChanged();
  }


  /**
   * This sets the contour pen to an appropriate color based on the color of the
   *   plot (B/W v. Color). The contour pen is set to red for color, white for
   *   B/W.
   */
  void ScatterPlotWindow::updateContourPen() {
    if (m_colorize->text() == "Gray") {
      m_spectrogram->setDefaultContourPen(QPen(Qt::red));
    }
    else {
      m_spectrogram->setDefaultContourPen(QPen(Qt::white));
    }
  }


  /**
   * Give the users an alarm config dialog to change the alarming settings.
   */
  void ScatterPlotWindow::configureAlarming() {
    ScatterPlotAlarmConfigDialog * config =
        new ScatterPlotAlarmConfigDialog(this);
    connect(config, SIGNAL(finished(int)),
            config, SLOT(deleteLater()));
    config->show();
  }
}
