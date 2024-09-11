#include "PlotWindowBestFitDialog.h"

#include <iostream>

#include <qwt_plot_spectrogram.h>
#include <qwt_text.h>

#include <QCheckBox>
#include <QColorDialog>
#include <QComboBox>
#include <QGridLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QVariant>

#include "CubePlotCurve.h"
#include "IString.h"
#include "ScatterPlotData.h"

namespace Isis {
  /**
   * Create a PlotWindowBestFitDialog. If a best fit line is accepted by the
   *   user, then the curve will be added directly to the plot window with no
   *   extra code in the plot window. This fully initializes the state of the
   *   dialog and no extra initialization should be required.
   *
   * @param windowWithCurves This must not be NULL. This is the plot window for
   *                         which a best fit line should be calculated.
   * @param parent The Qt-parent of this dialog.
   */
  PlotWindowBestFitDialog::PlotWindowBestFitDialog(
      PlotWindow *windowWithCurves, QWidget *parent) : QDialog(parent) {
    m_plotWindowWithCurves = windowWithCurves;
    QGridLayout *optionsLayout = new QGridLayout;

    int row = 0;
    QLabel *titleLabel = new QLabel("Curve To Fit: ");
    m_curvesCombo = new QComboBox;
//     m_curvesCombo->setInsertPolicy(QComboBox::InsertAlphabetically);
    optionsLayout->addWidget(titleLabel,    row, 0);
    optionsLayout->addWidget(m_curvesCombo, row, 1);
    connect(m_curvesCombo, SIGNAL(currentIndexChanged(int)),
            this, SLOT(refreshWidgetStates()));
    row++;

    QLabel *equationTitleLabel = new QLabel("Equation from Curve: ");
    m_equationLabel = new QLabel;
    optionsLayout->addWidget(equationTitleLabel, row, 0);
    optionsLayout->addWidget(m_equationLabel,    row, 1);
    row++;

    QLabel *correlationTitleLabel = new QLabel("Correlation Coefficient (r): ");
    m_correlationLabel = new QLabel;
    optionsLayout->addWidget(correlationTitleLabel, row, 0);
    optionsLayout->addWidget(m_correlationLabel,    row, 1);
    row++;

    QLabel *determinationTitleLabel = new QLabel(
        "Coefficient of Determination (r<sup>2</sup>): ");
    m_determinationLabel = new QLabel;
    optionsLayout->addWidget(determinationTitleLabel, row, 0);
    optionsLayout->addWidget(m_determinationLabel,    row, 1);
    row++;

    QHBoxLayout *applyButtonsLayout = new QHBoxLayout;
    applyButtonsLayout->addStretch();

    m_okayButton = new QPushButton("&Ok");
    m_okayButton->setIcon(QIcon::fromTheme("dialog-ok"));
    connect(m_okayButton, SIGNAL(clicked()),
            this, SLOT(createBestFitLine()));
    connect(m_okayButton, SIGNAL(clicked()),
            this, SLOT(close()));
    applyButtonsLayout->addWidget(m_okayButton);

    QPushButton *cancel = new QPushButton("&Cancel");
    cancel->setIcon(QIcon::fromTheme("dialog-cancel"));
    connect(cancel, SIGNAL(clicked()),
            this, SLOT(close()));
    applyButtonsLayout->addWidget(cancel);

    QWidget *optionsHolder = new QWidget;
    optionsHolder->setLayout(optionsLayout);

    QWidget *applyButtonsHolder = new QWidget;
    applyButtonsHolder->setLayout(applyButtonsLayout);

    QVBoxLayout *mainLayout = new QVBoxLayout;
    mainLayout->addWidget(optionsHolder);
    mainLayout->addWidget(applyButtonsHolder);

    setLayout(mainLayout);

    readCurvesFromWindow();
    refreshWidgetStates();
  }


  PlotWindowBestFitDialog::~PlotWindowBestFitDialog() {
  }


  /**
   * This is called when the user wants the best fit line. The already
   *   populated m_curveMultivariateStats is used to give us the current
   *   best-fit coefficients. We create y-data points based on the best fit for
   *   every x value inside of the curve we're fitting. The curve is then added
   *   directly to the plot window.
   */
  void PlotWindowBestFitDialog::createBestFitLine() {

    if (m_curveMultivariateStats && selectedCurve() && m_plotWindowWithCurves) {
      CubePlotCurve *selected = selectedCurve();
      double a = Null;
      double b = Null;
      try {
        m_curveMultivariateStats->LinearRegression(a, b);
      }
      catch (IException &) {
      }

      if (!IsSpecial(a) && !IsSpecial(b)) {
        CubePlotCurve *newCurve = new CubePlotCurve(selected->xUnits(),
                                                    selected->yUnits());

        int dataSize = selected->dataSize();
        QVector<QPointF> data(dataSize);

        for (int i = 0; i < dataSize; i++) {
          data[i].setX(selected->sample(i).x());
          data[i].setY(a + b * data[i].x());
        }

        newCurve->setData(new QwtPointSeriesData(data));
        newCurve->setColor(selected->color());

        newCurve->setMarkerSymbol(QwtSymbol::NoSymbol);

        QPen pen(newCurve->pen());
        pen.setStyle(Qt::SolidLine);
        newCurve->setPen(pen);

        newCurve->setTitle(selected->title().text() + " Best Fit");
        newCurve->copySource(*selected);

        m_plotWindowWithCurves->add(newCurve);
      }
    }
    else if (m_curveMultivariateStats && selectedSpectrogram() &&
             m_plotWindowWithCurves) {
      QwtPlotSpectrogram *selected = selectedSpectrogram();
      double a = Null;
      double b = Null;

      try {
        m_curveMultivariateStats->LinearRegression(a, b);
      }
      catch (IException &) {
      }

      ScatterPlotData *scatterData =
          dynamic_cast<ScatterPlotData *>(selected->data());

      if (scatterData && !IsSpecial(a) && !IsSpecial(b)) {
        CubePlotCurve *newCurve = new CubePlotCurve(
            m_plotWindowWithCurves->xAxisUnits(),
            m_plotWindowWithCurves->yAxisUnits());

        QVector<double> rawXValues = scatterData->discreteXValues();

        int dataSize = rawXValues.size();
        QVector<QPointF> data(dataSize);

        for (int i = 0; i < dataSize; i++) {
          data[i].setX(rawXValues[i]);
          data[i].setY(a + b * data[i].x());
        }

        newCurve->setData(new QwtPointSeriesData(data));
        newCurve->setColor(Qt::red);

        newCurve->setMarkerSymbol(QwtSymbol::NoSymbol);

        QPen pen(newCurve->pen());
        pen.setStyle(Qt::SolidLine);
        newCurve->setPen(pen);

        newCurve->setTitle(selected->title().text() + " Best Fit");

        m_plotWindowWithCurves->add(newCurve);
      }
    }
  }


  /**
   * This populates the curve selection combo with all of the available
   *   information inside of the PlotWindow for which we're adding a best fit
   *   line to.
   */
  void PlotWindowBestFitDialog::readCurvesFromWindow() {
    m_curvesCombo->clear();

    if (m_plotWindowWithCurves) {
      QList<QwtPlotSpectrogram *> spectrograms =
          m_plotWindowWithCurves->plotSpectrograms();

      foreach (QwtPlotSpectrogram *spectrogram, spectrograms) {
        if (dynamic_cast<ScatterPlotData *>(spectrogram->data())) {
          m_curvesCombo->addItem( spectrogram->title().text(), QVariant::fromValue(spectrogram) );
        }
      }

      QList<CubePlotCurve *> curves = m_plotWindowWithCurves->plotCurves();

      foreach (CubePlotCurve *curve, curves) {
          m_curvesCombo->addItem( curve->title().text(), QVariant::fromValue(curve) );
      }
    }
  }


  /**
   * This updates all of the widgets in this window's visibility and text
   *   data based on what the user has entered so far. This does not refresh the
   *   list of best fit-able curves/spectrograms.
   */
  void PlotWindowBestFitDialog::refreshWidgetStates() {
    bool canDeriveEquation = false;

    if (selectedCurve()) {
      CubePlotCurve *selected = selectedCurve();
      m_curveMultivariateStats.reset(new MultivariateStatistics);

      int dataSize = selected->dataSize();
      for (int dataPoint = 0; dataPoint < dataSize; dataPoint++) {
        double x = selected->sample(dataPoint).x();
        double y = selected->sample(dataPoint).y();
        m_curveMultivariateStats->AddData(&x, &y, 1);
      }
    }
    else if (selectedSpectrogram()) {
      QwtPlotSpectrogram *selected = selectedSpectrogram();
      m_curveMultivariateStats.reset(new MultivariateStatistics);

      ScatterPlotData *scatterData =
          dynamic_cast<ScatterPlotData *>(selected->data());

      if (scatterData) {
        for (int i = 0; i < scatterData->numberOfBins(); i++) {
          QPair<double, double> pointXY = scatterData->binXY(i);

          int binValue = scatterData->binCount(i);
          m_curveMultivariateStats->AddData(pointXY.first, pointXY.second,
                                            binValue);
        }
      }
    }

    if (m_curveMultivariateStats &&
        m_curveMultivariateStats->ValidPixels() > 1) {
      double a = Null;
      double b = Null;

      try {
        m_curveMultivariateStats->LinearRegression(a, b);
      }
      catch (IException &) {
      }

      if (!IsSpecial(a) && !IsSpecial(b)) {
        canDeriveEquation = true;
        m_equationLabel->setText(
            QString("y = " + QString::number(b) + "x + " + QString::number(a)));

        double correlation = m_curveMultivariateStats->Correlation();

        if (!IsSpecial(correlation)) {
          m_correlationLabel->setText(QString::number(correlation));
          m_determinationLabel->setText(
              QString::number(correlation * correlation));
        }
        else {
          m_correlationLabel->setText("Undefined");
          m_determinationLabel->setText("Undefined");
        }
      }
    }

    m_okayButton->setEnabled(canDeriveEquation);

    if (!canDeriveEquation) {
      m_equationLabel->setText("N/A");
      m_correlationLabel->setText("N/A");
      m_determinationLabel->setText("N/A");
    }
  }


  /**
   * If a curve is selected, this returns it. If something else is selected, or
   *   nothing is selected, this returns NULL.
   *
   * @return The user-selected curve to create a best fit line for
   */
  CubePlotCurve *PlotWindowBestFitDialog::selectedCurve() {
    CubePlotCurve *selected = NULL;

    if (m_plotWindowWithCurves && m_curvesCombo) {
      selected = m_curvesCombo->itemData(
          m_curvesCombo->currentIndex()).value<CubePlotCurve *>();

      QList<CubePlotCurve *> curves = m_plotWindowWithCurves->plotCurves();

      if (selected != NULL && curves.indexOf(selected) == -1) {
        m_curvesCombo->removeItem(m_curvesCombo->currentIndex());
        selected = selectedCurve();
      }
    }

    return selected;
  }


  /**
   * If a spectrogram (scatter plot) is selected, this returns it. If something
   *   else is selected, or nothing is selected, this returns NULL.
   *
   * @return The user-selected spectrogram (scatter plot) to create a best fit
   *         line for
   */
  QwtPlotSpectrogram *PlotWindowBestFitDialog::selectedSpectrogram() {
    QwtPlotSpectrogram *selected = NULL;

    if (m_plotWindowWithCurves && m_curvesCombo) {
      selected = m_curvesCombo->itemData(
          m_curvesCombo->currentIndex()).value<QwtPlotSpectrogram *>();

      QList<QwtPlotSpectrogram *> spectrograms =
          m_plotWindowWithCurves->plotSpectrograms();

      if (selected != NULL && spectrograms.indexOf(selected) == -1) {
        m_curvesCombo->removeItem(m_curvesCombo->currentIndex());
        selected = selectedSpectrogram();
      }
    }

    return selected;
  }
}
