#include "SpectralPlotTool.h"

#include <iostream>

#include "geos/geom/Polygon.h"
#include "geos/geom/CoordinateSequence.h"
#include "geos/geom/Point.h"

#include <QAction>
#include <QCheckBox>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QLabel>
#include <QMenu>
#include <QMessageBox>
#include <QPushButton>

#include "Brick.h"
#include "Cube.h"
#include "CubePlotCurve.h"
#include "InterestOperator.h"
#include "MdiCubeViewport.h"
#include "SpectralPlotTool.h"
#include "SpectralPlotWindow.h"
#include "PolygonTools.h"
#include "Pvl.h"
#include "RubberBandComboBox.h"
#include "RubberBandTool.h"
#include "Statistics.h"
#include "ToolPad.h"

using std::cerr;

namespace Isis {

  /**
   * This constructs a spectral plot tool. The spectral plot tool graphs statistics across a
   * spectrum (bands).
   *
   *
   * @param parent
   */
  SpectralPlotTool::SpectralPlotTool(QWidget *parent) :
      AbstractPlotTool(parent),
      m_maxCurves(new QMap< MdiCubeViewport *, QPointer<CubePlotCurve> >),
      m_minCurves(new QMap< MdiCubeViewport *, QPointer<CubePlotCurve> >),
      m_avgCurves(new QMap< MdiCubeViewport *, QPointer<CubePlotCurve> >),
      m_stdDev1Curves(new QMap< MdiCubeViewport *, QPointer<CubePlotCurve> >),
      m_stdDev2Curves(new QMap< MdiCubeViewport *, QPointer<CubePlotCurve> >),
      m_stdErr1Curves(new QMap< MdiCubeViewport *, QPointer<CubePlotCurve> >),
      m_stdErr2Curves(new QMap< MdiCubeViewport *, QPointer<CubePlotCurve> >) {
    connect(this, SIGNAL(viewportChanged()), this, SLOT(viewportSelected()));

    m_displayCombo = new QComboBox;
  }


  /**
   * This protected slot is called when user selects a viewport.
   *
   */
  void SpectralPlotTool::viewportSelected() {
    //m_autoScale->setChecked(true);
  }


  /**
   * Get the combo box which toggles between units of wavelength and band number
   *
   * @return A combo box for switching plot window x-axis units
   */
  QComboBox *SpectralPlotTool::spectralDisplayCombo() const
  {
    return m_displayCombo;
  }


  /**
   * This method is called when the tool is activated by the
   *   parent, or when the plot mode is changed. It's used to
   *   activate or change the rubber banding mode to be either
   *   rectangle or line, depending on the current plot type.
   */
  void SpectralPlotTool::enableRubberBandTool() {
    if (m_rubberBandCombo) {
      m_rubberBandCombo->reset();
      rubberBandTool()->setDrawActiveViewportOnly(false);

      m_rubberBandCombo->setEnabled(true);
      m_rubberBandCombo->setVisible(true);
    }
  }


  /**
   * This prompts the user for which curves they want to plot. This is an
   *   alternative method to just right clicking this tool's options area.
   */
  void SpectralPlotTool::selectCurvesToPlot() {
    QDialog *selectCurvesDialog = new QDialog;
    selectCurvesDialog->setWindowTitle("Select Curves to Plot");

    QGridLayout *layout = new QGridLayout;

    QLabel *header = new QLabel("Select which curves to plot when new data is "
                                "selected");
    layout->addWidget(header, 0, 0, 1, 2, Qt::AlignHCenter);

    QList<QAction *> actions;
    actions.append(m_plotAvgAction);
    actions.append(m_plotMinAction);
    actions.append(m_plotMaxAction);
    actions.append(m_plotStdDev1Action);
    actions.append(m_plotStdDev2Action);
    actions.append(m_plotStdErr1Action);
    actions.append(m_plotStdErr2Action);

    int row = 2;
    foreach (QAction *action, actions) {
      QLabel *label = new QLabel(action->text());
      layout->addWidget(label, row, 0, 1, 1);

      QCheckBox *actionCheckbox = new QCheckBox;
      actionCheckbox->setChecked(action->isChecked());
      connect(actionCheckbox, SIGNAL(stateChanged(int)),
              action, SLOT(toggle()));
      layout->addWidget(actionCheckbox, row, 1, 1, 1, Qt::AlignRight);
      row++;
    }

    row++;
    QPushButton *okButton = new QPushButton("Ok");
    connect(okButton, SIGNAL(clicked()),
            selectCurvesDialog, SLOT(close()));
    layout->addWidget(okButton, row, 0, 1, 2);

    selectCurvesDialog->setLayout(layout);
    selectCurvesDialog->exec();
  }


  /**
   * This method configures the QAction for this tool
   *
   * @param toolpad - the ToolPad to add the SpectralPlotTool to
   *
   * @return QAction* - the QAction that was created for this tool
   */
  QAction *SpectralPlotTool::toolPadAction(ToolPad *toolpad) {
    m_toolPadAction = new QAction(toolpad);
    m_toolPadAction->setText("Spectral Plot Tool");
    m_toolPadAction->setIcon(QPixmap(toolIconDir() + "/spectral_plot.png"));
    QString text = "<b>Function:</b> Create a spectral plot using statistics across a spectrum "
                   "(bands).";
    m_toolPadAction->setWhatsThis(text);
    return m_toolPadAction;
  }


  /**
   * Creates the widgets for the tool bar.
   *
   *
   * @param parent
   *
   * @return QWidget*
   */
  QWidget *SpectralPlotTool::createToolBarWidget(QStackedWidget *parent) {
    QWidget *wrapper = new QWidget;
    wrapper->setContextMenuPolicy(Qt::ActionsContextMenu);

    m_plotAvgAction = new QAction("Average", this);
    m_plotAvgAction->setCheckable(true);
    m_plotAvgAction->setChecked(true);

    m_plotMinAction = new QAction("Minimum", this);
    m_plotMinAction->setCheckable(true);
    m_plotMinAction->setChecked(false);

    m_plotMaxAction = new QAction("Maximum", this);
    m_plotMaxAction->setCheckable(true);
    m_plotMaxAction->setChecked(false);

    m_plotStdDev1Action = new QAction("+ Sigma", this);
    m_plotStdDev1Action->setCheckable(true);
    m_plotStdDev1Action->setChecked(false);

    m_plotStdDev2Action = new QAction("- Sigma", this);
    m_plotStdDev2Action->setCheckable(true);
    m_plotStdDev2Action->setChecked(false);

    m_plotStdErr1Action = new QAction("+ Std Error", this);
    m_plotStdErr1Action->setCheckable(true);
    m_plotStdErr1Action->setChecked(false);

    m_plotStdErr2Action = new QAction("- Std Error", this);
    m_plotStdErr2Action->setCheckable(true);
    m_plotStdErr2Action->setChecked(false);

    wrapper->addAction(m_plotAvgAction);
    wrapper->addAction(m_plotMinAction);
    wrapper->addAction(m_plotMaxAction);
    wrapper->addAction(m_plotStdDev1Action);
    wrapper->addAction(m_plotStdDev2Action);
    wrapper->addAction(m_plotStdErr1Action);
    wrapper->addAction(m_plotStdErr2Action);

    m_rubberBandCombo = new RubberBandComboBox(this,
      RubberBandComboBox::Polygon |
      RubberBandComboBox::Rectangle,
      RubberBandComboBox::Rectangle
    );

    QWidget *abstractToolWidgets =
        AbstractPlotTool::createToolBarWidget(parent);

    QPushButton *plotCurvesButton = new QPushButton("Select Curves to Plot");
    connect(plotCurvesButton, SIGNAL(clicked()),
            this, SLOT(selectCurvesToPlot()));

    QHBoxLayout *layout = new QHBoxLayout(wrapper);
    layout->setMargin(0);
    layout->addWidget(m_rubberBandCombo);
    layout->addWidget(spectralDisplayCombo());
    layout->addWidget(plotCurvesButton);
    layout->addWidget(abstractToolWidgets);
    layout->addStretch(1);
    wrapper->setLayout(layout);

    return wrapper;
  }


  /**
   * Adds the plot tool to the menu.
   *
   *
   * @param menu
   */
  void SpectralPlotTool::addTo(QMenu *menu) {
    menu->addAction(m_toolPadAction);
  }


  /**
   * Updates plot tool.
   *
   */
  void SpectralPlotTool::updateTool() {
    AbstractPlotTool::updateTool();

    PlotCurve::Units preferredUnits =
        (PlotCurve::Units)m_displayCombo->itemData(
          m_displayCombo->currentIndex()).toInt();

    while (m_displayCombo->count())
      m_displayCombo->removeItem(0);

    m_displayCombo->addItem("Band Number", PlotCurve::Band);

    bool supportsWavelength = true;

    foreach (MdiCubeViewport *cvp, viewportsToPlot()) {
      int bandCount = cvp->cube()->bandCount();

      // if single band then disable spectral plot
      Pvl &pvl = *cvp->cube()->label();
      supportsWavelength = supportsWavelength &&
                           pvl.findObject("IsisCube").hasGroup("BandBin");

      if (supportsWavelength) {
        PvlGroup &bandBin = pvl.findObject("IsisCube").findGroup("BandBin");
        supportsWavelength = supportsWavelength &&
                             bandBin.hasKeyword("Center") &&
                             bandBin["Center"].size() == bandCount;
      }
    }

    if (supportsWavelength) {
      m_displayCombo->addItem("Wavelength", PlotCurve::Wavelength);
    }

    if (m_displayCombo->findData(preferredUnits) != -1) {
      m_displayCombo->setCurrentIndex(
        m_displayCombo->findData(preferredUnits));
    }

    m_displayCombo->setVisible(m_displayCombo->count() > 1);
  }


  /**
   * Creates a new plot window compatible with the curves in this tool.
   *
   * @return a newly allocated plot window, ownership is passed to the caller.
   */
  PlotWindow *SpectralPlotTool::createWindow() {
    PlotWindow *window = new SpectralPlotWindow(
        (PlotCurve::Units)m_displayCombo->itemData(
          m_displayCombo->currentIndex()).toInt(),
        qobject_cast<QWidget *>(parent()));
    window->setWindowTitle("Spectral " + PlotWindow::defaultWindowTitle());

    return window;
  }


  /**
   * Forget about all existing plot curves. Don't delete them, just
   *   forget them so that when the user requests new ones they get brand
   *   new curves.
   */
  void SpectralPlotTool::detachCurves() {
    m_minCurves->clear();
    m_maxCurves->clear();
    m_avgCurves->clear();
    m_stdDev1Curves->clear();
    m_stdDev2Curves->clear();
    m_stdErr1Curves->clear();
    m_stdErr2Curves->clear();
  }


  /**
   * Called when the user has finished drawing with the rubber
   * band.  ChangePlot is called to plot the data within the
   * rubber band.
   *
   */
  void SpectralPlotTool::rubberBandComplete() {
    if (selectedWindow()) {
      selectedWindow()->raise();
    }

    if (rubberBandTool()->isValid()) {
      refreshPlot();
    }
    else {
      QMessageBox::information(NULL, "Error",
                               "The selected Area contains no valid pixels",
                               QMessageBox::Ok);
    }
  }


  /**
   * This method replots the data, with current settings and rubber band, in the plot window.
   */
  void SpectralPlotTool::refreshPlot() {
    MdiCubeViewport *activeViewport = cubeViewport();

    if (activeViewport && rubberBandTool()->isValid()) {
      // Find which window we want to paste into
      PlotWindow *targetWindow = selectedWindow(true);

      // if the selected window won't work, create a new one
      if (targetWindow->xAxisUnits() !=
          m_displayCombo->itemData(m_displayCombo->currentIndex()).toInt()) {
        targetWindow = addWindow();
      }

      // get curves for active viewport and also for any linked viewports
      foreach (MdiCubeViewport *viewport, viewportsToPlot()) {
        /* We'll need X-Axis labels and a xMax to scale to.*/
        QVector<double> labels;
        Statistics wavelengthStats;

        QVector<QPointF> avgData, minData, maxData, std1Data, std2Data,
            stdErr1Data, stdErr2Data, wavelengthData;
        QVector<Statistics> plotStats;

        getSpectralStatistics(labels, plotStats, viewport);

        for (int index = 0; index < labels.size(); index++) {
          if (!IsSpecial(plotStats[index].Average()) &&
              !IsSpecial(plotStats[index].Minimum()) &&
              !IsSpecial(plotStats[index].Maximum())) {
            avgData.append(QPointF(labels[index], plotStats[index].Average()));
            minData.append(QPointF(labels[index], plotStats[index].Minimum()));
            maxData.append(QPointF(labels[index], plotStats[index].Maximum()));

            if (!IsSpecial(plotStats[index].StandardDeviation())) {
              std1Data.append(QPointF(labels[index],
                  plotStats[index].Average() +
                  plotStats[index].StandardDeviation()));
              std2Data.append(QPointF(labels[index],
                  plotStats[index].Average() -
                  plotStats[index].StandardDeviation()));

              double standardError = plotStats[index].StandardDeviation() /
                                     sqrt(plotStats[index].ValidPixels());

              stdErr1Data.append(QPointF(labels[index],
                                         plotStats[index].Average() +
                                         standardError));
              stdErr2Data.append(QPointF(labels[index],
                                         plotStats[index].Average() -
                                         standardError));
            }
          }
        } /*end for loop*/

        if (labels.size() > 0) {
          QList<QPoint> rubberBandPoints = rubberBandTool()->vertices();

          validatePlotCurves();
          if (m_plotAvgAction->isChecked()) {
            (*m_avgCurves)[viewport]->setData(new QwtPointSeriesData(avgData));
            (*m_avgCurves)[viewport]->setSource(viewport, rubberBandPoints);
          }

          if (m_plotMinAction->isChecked()) {
            (*m_minCurves)[viewport]->setData(new QwtPointSeriesData(minData));
            (*m_minCurves)[viewport]->setSource(viewport, rubberBandPoints);
          }

          if (m_plotMaxAction->isChecked()) {
            (*m_maxCurves)[viewport]->setData(new QwtPointSeriesData(maxData));
            (*m_maxCurves)[viewport]->setSource(viewport, rubberBandPoints);
          }

          if (m_plotStdDev1Action->isChecked()) {
            (*m_stdDev1Curves)[viewport]->setData(
                new QwtPointSeriesData(std1Data));
            (*m_stdDev1Curves)[viewport]->setSource(viewport,
                                                    rubberBandPoints);
          }

          if (m_plotStdDev2Action->isChecked()) {
            (*m_stdDev2Curves)[viewport]->setData(
                new QwtPointSeriesData(std2Data));
            (*m_stdDev2Curves)[viewport]->setSource(viewport,
                                                    rubberBandPoints);
          }

          if (m_plotStdErr1Action->isChecked()) {
            (*m_stdErr1Curves)[viewport]->setData(
                new QwtPointSeriesData(stdErr1Data));
            (*m_stdErr1Curves)[viewport]->setSource(viewport,
                                                    rubberBandPoints);
          }

          if (m_plotStdErr2Action->isChecked()) {
            (*m_stdErr2Curves)[viewport]->setData(
                new QwtPointSeriesData(stdErr2Data));
            (*m_stdErr2Curves)[viewport]->setSource(viewport,
                                                    rubberBandPoints);
          }
        }
      }

      targetWindow->replot();
      updateTool();
    }
  }


  /**
   * This method sets up the names, line style, and color  of the
   * all the CubePlotCurves that will be used in this class.
   */
  void SpectralPlotTool::validatePlotCurves() {
    PlotWindow *targetWindow = selectedWindow();

    if (targetWindow) {
      PlotCurve::Units targetUnits = (PlotCurve::Units)m_displayCombo->itemData(
            m_displayCombo->currentIndex()).toInt();

      QPen avgPen(Qt::white);
      avgPen.setWidth(1);
      avgPen.setStyle(Qt::SolidLine);

      QPen minMaxPen(Qt::cyan);
//       minMaxPen.setStyle(Qt::DashLine);
      minMaxPen.setWidth(1);
      minMaxPen.setStyle(Qt::SolidLine);

      QPen stdDevPen(Qt::red);
      stdDevPen.setWidth(1);
//       stdDevPen.setStyle(Qt::DotLine);
      stdDevPen.setStyle(Qt::SolidLine);

      QPen stdErrPen(Qt::green);
      stdErrPen.setWidth(1);
//       stdErrPen.setStyle(Qt::DotLine);
      stdErrPen.setStyle(Qt::SolidLine);

      foreach (MdiCubeViewport *viewport, viewportsToPlot()) {
        if (m_plotAvgAction->isChecked() &&
           (!(*m_avgCurves)[viewport] ||
            (*m_avgCurves)[viewport]->xUnits() != targetUnits)) {
          CubePlotCurve *plotCurve = createCurve("Average", avgPen,
              targetUnits, CubePlotCurve::CubeDN);
          m_avgCurves->insert(viewport, plotCurve);
          targetWindow->add(plotCurve);
        }

        if (m_plotMinAction->isChecked() &&
           (!(*m_minCurves)[viewport] ||
            (*m_minCurves)[viewport]->xUnits() != targetUnits)) {
          CubePlotCurve *plotCurve = createCurve("Minimum", minMaxPen,
              targetUnits, CubePlotCurve::CubeDN);
          m_minCurves->insert(viewport, plotCurve);
          targetWindow->add(plotCurve);
        }

        if (m_plotMaxAction->isChecked() &&
           (!(*m_maxCurves)[viewport] ||
            (*m_maxCurves)[viewport]->xUnits() != targetUnits)) {
          CubePlotCurve *plotCurve = createCurve("Maximum", minMaxPen,
              targetUnits, CubePlotCurve::CubeDN);
          m_maxCurves->insert(viewport, plotCurve);
          targetWindow->add(plotCurve);
        }

        if (m_plotStdDev1Action->isChecked() &&
           (!(*m_stdDev1Curves)[viewport] ||
            (*m_stdDev1Curves)[viewport]->xUnits() != targetUnits)) {
          CubePlotCurve *plotCurve = createCurve("+ Sigma", stdDevPen,
              targetUnits, CubePlotCurve::CubeDN);
          m_stdDev1Curves->insert(viewport, plotCurve);
          targetWindow->add(plotCurve);
        }

        if (m_plotStdDev2Action->isChecked() &&
           (!(*m_stdDev2Curves)[viewport] ||
            (*m_stdDev2Curves)[viewport]->xUnits() != targetUnits)) {
          CubePlotCurve *plotCurve = createCurve("- Sigma", stdDevPen,
              targetUnits, CubePlotCurve::CubeDN);
          m_stdDev2Curves->insert(viewport, plotCurve);
          targetWindow->add(plotCurve);
        }

        if (m_plotStdErr1Action->isChecked() &&
           (!(*m_stdErr1Curves)[viewport] ||
            (*m_stdErr1Curves)[viewport]->xUnits() != targetUnits)) {
          CubePlotCurve *plotCurve = createCurve("+ Std Error", stdErrPen,
              targetUnits, CubePlotCurve::CubeDN);
          m_stdErr1Curves->insert(viewport, plotCurve);
          targetWindow->add(plotCurve);
        }

        if (m_plotStdErr2Action->isChecked() &&
           (!(*m_stdErr2Curves)[viewport] ||
            (*m_stdErr2Curves)[viewport]->xUnits() != targetUnits)) {
          CubePlotCurve *plotCurve = createCurve("- Std Error", stdErrPen,
              targetUnits, CubePlotCurve::CubeDN);
          m_stdErr2Curves->insert(viewport, plotCurve);
          targetWindow->add(plotCurve);
        }
      }
    }
  }


  /**
   * This method processes the spectral plot tool's selection and creates statistics
   * for the selected pixels. For rectangular selections, a pixel is selected for statistics
   * if any part of the pixel intersects with the rectangle. For polygon selections, a pixel
   * is selected for statistics only when its center is within the polygon.
   *
   * @param labels
   * @param data
   * @param viewport
   */
  void SpectralPlotTool::getSpectralStatistics(QVector<double> &labels,
                                               QVector<Statistics> &data,
                                               MdiCubeViewport *viewport) {
    QList<QPoint> vertices = rubberBandTool()->vertices();

    if (vertices.size() < 1) return;

    double ss, sl, es, el, x, y;
    std::vector <int> x_contained, y_contained;

    // Convert vertices to their sub-pixel sample/line values
    viewport->viewportToCube(vertices[0].x(), vertices[0].y(), ss, sl);
    viewport->viewportToCube(vertices[2].x(), vertices[2].y(), es, el);

    // round the start and end sample/line sub-pixel points to the nearest int (pixel)
    ss = round(ss);
    sl = round(sl);
    es = round(es);
    el = round(el);

    // calculate number of samples will be in Brick's shape buffer with absolute value
    // in case user makes a rectangle from right to left
    int samps = ( std::abs(es - ss) + 1) ;
    Cube *cube = viewport->cube();
    Brick *brick = new Brick(*cube, samps, 1, 1);
    Pvl &pvl = *viewport->cube()->label();

    if (rubberBandTool()->currentMode() == RubberBandTool::PolygonMode) {
//       samps = 1;
      geos::geom::CoordinateSequence pts;
      for (int i = 0; i < vertices.size(); i++) {
        viewport->viewportToCube(vertices[i].x(), vertices[i].y(), x, y);
        // add the x,y vertices (double) to the pts CoordinateSequence
        pts.add(geos::geom::Coordinate(x, y));
      }/*end for*/

      /*Add the first point again in order to make a closed line string*/
      viewport->viewportToCube(vertices[0].x(), vertices[0].y(), x, y);
      pts.add(geos::geom::Coordinate(x, y));

      geos::geom::Polygon *poly = globalFactory->createPolygon(
          globalFactory->createLinearRing(pts)).release();

      const geos::geom::Envelope *envelope = poly->getEnvelopeInternal();

      // round the (double) max x's and y's and min x's and y's to the nearest pixel
      for (int y = (int)round(envelope->getMinY());
           y <= (int)round(envelope->getMaxY()); y++) {
        for (int x = (int)round(envelope->getMinX());
             x <= (int)round(envelope->getMaxX()); x++) {
          // create a point at the center of the pixel
          geos::geom::Coordinate c(x, y);
          geos::geom::Point *p = globalFactory->createPoint(c).release();
          // check if the center of the pixel is in the polygon's envelope (the selection)
          bool contains = p->within(poly);
          delete p;
          if (contains) {
            // these pixels will be used for computing statistics
            x_contained.push_back(x);
            y_contained.push_back(y);
          }

        } /*end x*/
      }/*end y*/

      delete poly;
      poly = NULL;
    }


    for (int band = 1; band <= cube->bandCount(); band++) {
      Statistics stats;

      /*Rectangle*/
      if (rubberBandTool()->currentMode() == RubberBandTool::RectangleMode) {
        for (int line = (int)std::min(sl, el); line <= (int)std::max(sl, el); line++) {
          // set Brick's base position at left-most endpoint
          brick->SetBasePosition(std::min(ss, es), line, band);
          cube->read(*brick);
          stats.AddData(brick->DoubleBuffer(), samps);
        }
      }

      /*Polygon*/
      if (rubberBandTool()->currentMode() == RubberBandTool::PolygonMode) {
        for (unsigned int j = 0; j < x_contained.size(); j++) {

          brick->SetBasePosition(x_contained[j], y_contained[j], band);
          cube->read(*brick);
          stats.AddData(*brick->DoubleBuffer());

        }
      }


      PlotCurve::Units targetUnits = (PlotCurve::Units)m_displayCombo->itemData(
            m_displayCombo->currentIndex()).toInt();
      if (targetUnits == PlotCurve::Band) {
        labels.push_back(band);
      }
      else if (targetUnits == PlotCurve::Wavelength) {
        if (pvl.findObject("IsisCube").hasGroup("BandBin")) {
          PvlGroup &bandBin = pvl.findObject("IsisCube").findGroup("BandBin");
          if (bandBin.hasKeyword("Center")) {
            PvlKeyword &wavelength = bandBin.findKeyword("Center");
            if (wavelength.size() > (band - 1)) {
              labels.push_back(IString::ToDouble(wavelength[band-1]));
            }
          }
        }
      }

      if (stats.Average() == Null) {
        data.push_back(stats);
      }
      else {
        data.push_back(stats);
      }
    }

    delete brick;
  }
}
