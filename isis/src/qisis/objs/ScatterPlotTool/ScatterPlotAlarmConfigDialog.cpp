#include "ScatterPlotAlarmConfigDialog.h"

#include <float.h>

#include <iostream>

#include <QCheckBox>
#include <QColorDialog>
#include <QComboBox>
#include <QDoubleSpinBox>
#include <QGridLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QSpinBox>

#include "ScatterPlotWindow.h"
#include "IString.h"

namespace Isis {
  /**
   * Create an alarming configuration dialog. This fully initializes the state,
   *   but a valid scatter plot window must be specified.
   *
   * @param window The scatter plot window to be configured, this must not be
   *               NULL.
   * @param parent The Qt-parent for this dialog.
   */
  ScatterPlotAlarmConfigDialog::ScatterPlotAlarmConfigDialog(
      ScatterPlotWindow *window, QWidget *parent) : QDialog(parent) {
    m_window = window;

    QGridLayout *mainLayout = new QGridLayout;

    /**
     *  The layout is shown below:
     *
     *  |--------------------------------------------------------------|
     *  | Text    rowspan=1, colspan=3                                 |
     *  |--------------------------------------------------------------|
     *  | Alarm onto plot      | [] Checkbox                           |
     *  |--------------------------------------------------------------|
     *  | Text    rowspan=1, colspan=3                                 |
     *  |--------------------------------------------------------------|
     *  |   |   Samples        | Input Spin                            |
     *  |--------------------------------------------------------------|
     *  |   |   Lines          | Input Spin                            |
     *  |--------------------------------------------------------------|
     *  | Alarm onto viewport  | [] Checkbox                           |
     *  |--------------------------------------------------------------|
     *  | Text    rowspan=1, colspan=3                                 |
     *  |--------------------------------------------------------------|
     *  |   |   Range (units)  | Input Combo (screen v. cube)          |
     *  |--------------------------------------------------------------|
     *  |   |   Samples        | Input Spin                            |
     *  |--------------------------------------------------------------|
     *  |   |   Lines          | Input Spin                            |
     *  |--------------------------------------------------------------|
     *  |   |   Width          | Input Spin                            |
     *  |--------------------------------------------------------------|
     *  |   |   Height         | Input Spin                            |
     *  |--------------------------------------------------------------|
     *  |   Ok Apply Cancel                                            |
     *  |--------------------------------------------------------------|
     */

    int row = 0;
    QLabel *headerLabel = new QLabel("<h3>Configure Alarming</h3>");
    mainLayout->addWidget(headerLabel, row, 0, 1, 3);
    row++;

    QLabel *descriptionLabel = new QLabel("Alarming is highlighting the "
        "corresponding pixels between the scatter plot and the source cubes. "
        "Alarming happens as you move the mouse around on the plot or cube.");
    descriptionLabel->setWordWrap(true);
    mainLayout->addWidget(descriptionLabel, row, 0, 1, 3);
    row++;

    QLabel *ontoPlotHeaderLabel =
        new QLabel("<h4>Alarming From Cube to Plot</h4>");
    mainLayout->addWidget(ontoPlotHeaderLabel, row, 0, 1, 3);
    row++;

    QLabel *ontoPlotEnabledLabel = new QLabel("Enabled");
    m_alarmOntoPlot = new QCheckBox;
    connect(m_alarmOntoPlot, SIGNAL(stateChanged(int)),
            this, SLOT(refreshWidgetStates()));
    mainLayout->addWidget(ontoPlotEnabledLabel, row, 1);
    mainLayout->addWidget(m_alarmOntoPlot, row, 2);
    row++;

    QLabel *ontoPlotSamplesLabel = new QLabel("Samples");
    m_alarmOntoPlotSamples = new QSpinBox;
    m_alarmOntoPlotSamples->setSingleStep(2);
    m_alarmOntoPlotSamples->setRange(1, INT_MAX);
    connect(m_alarmOntoPlotSamples, SIGNAL(valueChanged(int)),
            this, SLOT(refreshWidgetStates()));
    mainLayout->addWidget(ontoPlotSamplesLabel, row, 1);
    mainLayout->addWidget(m_alarmOntoPlotSamples, row, 2);
    row++;

    QLabel *ontoPlotLinesLabel = new QLabel("Lines");
    m_alarmOntoPlotLines = new QSpinBox;
    m_alarmOntoPlotLines->setSingleStep(2);
    m_alarmOntoPlotLines->setRange(1, INT_MAX);
    connect(m_alarmOntoPlotLines, SIGNAL(valueChanged(int)),
            this, SLOT(refreshWidgetStates()));
    mainLayout->addWidget(ontoPlotLinesLabel, row, 1);
    mainLayout->addWidget(m_alarmOntoPlotLines, row, 2);
    row++;
    row++;


    QLabel *ontoViewportHeaderLabel =
        new QLabel("<h4>Alarming From Plot to Cube</h4>");
    mainLayout->addWidget(ontoViewportHeaderLabel, row, 0, 1, 3);
    row++;

    QLabel *ontoViewportEnabledLabel = new QLabel("Enabled");
    m_alarmOntoViewport = new QCheckBox;
    connect(m_alarmOntoViewport, SIGNAL(stateChanged(int)),
            this, SLOT(refreshWidgetStates()));
    mainLayout->addWidget(ontoViewportEnabledLabel, row, 1);
    mainLayout->addWidget(m_alarmOntoViewport, row, 2);
    row++;

    QLabel *ontoViewportUnitsLabel = new QLabel("Range (units)");
    m_alarmOntoViewportUnits = new QComboBox;
    m_alarmOntoViewportUnits->addItem("Distance from mouse",
        ScatterPlotWindow::ScreenUnits);
    m_alarmOntoViewportUnits->addItem("DN range around mouse",
        ScatterPlotWindow::CubeUnits);
    connect(m_alarmOntoViewportUnits, SIGNAL(currentIndexChanged(int)),
            this, SLOT(refreshWidgetStates()));
    mainLayout->addWidget(ontoViewportUnitsLabel, row, 1);
    mainLayout->addWidget(m_alarmOntoViewportUnits, row, 2);
    row++;

    QLabel *ontoViewportXDnLabel = new QLabel("X Cube DN Box Size");
    m_alarmOntoViewportXDnSize = new QLineEdit;
    m_alarmOntoViewportXDnSize->setValidator(
        new QDoubleValidator(0.0, DBL_MAX, DBL_MAX_10_EXP + DBL_DIG, this));
    connect(m_alarmOntoViewportXDnSize, SIGNAL(textChanged(QString)),
            this, SLOT(refreshWidgetStates()));
    mainLayout->addWidget(ontoViewportXDnLabel, row, 1);
    mainLayout->addWidget(m_alarmOntoViewportXDnSize, row, 2);
    row++;

    QLabel *ontoViewportYDnLabel = new QLabel("Y Cube DN Box Size");
    m_alarmOntoViewportYDnSize = new QLineEdit;
    m_alarmOntoViewportYDnSize->setValidator(
        new QDoubleValidator(0.0, DBL_MAX, DBL_MAX_10_EXP + DBL_DIG, this));
    connect(m_alarmOntoViewportYDnSize, SIGNAL(textChanged(QString)),
            this, SLOT(refreshWidgetStates()));
    mainLayout->addWidget(ontoViewportYDnLabel, row, 1);
    mainLayout->addWidget(m_alarmOntoViewportYDnSize, row, 2);
    row++;

    QLabel *ontoViewportWidthLabel = new QLabel("Width");
    m_alarmOntoViewportWidth = new QSpinBox;
    m_alarmOntoViewportWidth->setSingleStep(2);
    m_alarmOntoViewportWidth->setRange(1, INT_MAX);
    connect(m_alarmOntoViewportWidth, SIGNAL(valueChanged(int)),
            this, SLOT(refreshWidgetStates()));
    mainLayout->addWidget(ontoViewportWidthLabel, row, 1);
    mainLayout->addWidget(m_alarmOntoViewportWidth, row, 2);
    row++;

    QLabel *ontoViewportHeightLabel = new QLabel("Height");
    m_alarmOntoViewportHeight = new QSpinBox;
    m_alarmOntoViewportHeight->setSingleStep(2);
    m_alarmOntoViewportHeight->setRange(1, INT_MAX);
    connect(m_alarmOntoViewportHeight, SIGNAL(valueChanged(int)),
            this, SLOT(refreshWidgetStates()));
    mainLayout->addWidget(ontoViewportHeightLabel, row, 1);
    mainLayout->addWidget(m_alarmOntoViewportHeight, row, 2);
    row++;

    QHBoxLayout *applyButtonsLayout = new QHBoxLayout;
    applyButtonsLayout->addStretch();

    m_okayButton = new QPushButton("&Ok");
    m_okayButton->setIcon(QIcon::fromTheme("dialog-ok"));
    connect(m_okayButton, SIGNAL(clicked()),
            this, SLOT(applySettingsToScatterPlot()));
    connect(m_okayButton, SIGNAL(clicked()),
            this, SLOT(accept()));
    applyButtonsLayout->addWidget(m_okayButton);

    m_applyButton = new QPushButton("&Apply");
    m_applyButton->setIcon(QIcon::fromTheme("dialog-ok-apply"));
    connect(m_applyButton, SIGNAL(clicked()),
            this, SLOT(applySettingsToScatterPlot()));
    applyButtonsLayout->addWidget(m_applyButton);

    QPushButton *cancelButton = new QPushButton("&Cancel");
    cancelButton->setIcon(QIcon::fromTheme("dialog-cancel"));
    connect(cancelButton, SIGNAL(clicked()),
            this, SLOT(reject()));
    applyButtonsLayout->addWidget(cancelButton);

    QWidget *applyButtonsWrapper = new QWidget;
    applyButtonsWrapper->setLayout(applyButtonsLayout);
    mainLayout->addWidget(applyButtonsWrapper, row, 0, 1, 3);
    row++;

    setLayout(mainLayout);

    readSettingsFromScatterPlot();
  }


  ScatterPlotAlarmConfigDialog::~ScatterPlotAlarmConfigDialog() {
    m_window = NULL;
  }


  /**
   * Take the settings that have been configured and apply them to the scatter
   *   plot. Any settings which fail to apply will be reverted in the GUI.
   */
  void ScatterPlotAlarmConfigDialog::applySettingsToScatterPlot() {
    m_window->setAlarmingPlot(m_alarmOntoPlot->isChecked());

    m_window->setAlarmPlotBoxSize(m_alarmOntoPlotSamples->value(),
                                  m_alarmOntoPlotLines->value());

    m_window->setAlarmingViewport(m_alarmOntoViewport->isChecked());

    m_window->setAlarmViewportUnits((ScatterPlotWindow::AlarmRangeUnits)
        m_alarmOntoViewportUnits->itemData(
          m_alarmOntoViewportUnits->currentIndex()).toInt());

    m_window->setAlarmViewportScreenBoxSize(
      m_alarmOntoViewportWidth->value(),
      m_alarmOntoViewportHeight->value());

    m_window->setAlarmViewportDnBoxSize(
      m_alarmOntoViewportXDnSize->text().toDouble(),
      m_alarmOntoViewportYDnSize->text().toDouble());

    QPair<double, double> alarmViewportDnBoxSize =
        m_window->alarmViewportDnBoxSize();
    m_alarmOntoViewportXDnSize->setText(
        QString::number(alarmViewportDnBoxSize.first));
    m_alarmOntoViewportYDnSize->setText(
        QString::number(alarmViewportDnBoxSize.second));

    QPair<int, int> alarmViewportScreenBoxSize =
        m_window->alarmViewportScreenBoxSize();
    m_alarmOntoViewportWidth->setValue(alarmViewportScreenBoxSize.first);
    m_alarmOntoViewportHeight->setValue(alarmViewportScreenBoxSize.second);

    readSettingsFromScatterPlot();
  }


  /**
   * Update the current widgets' states with the current settings in the
   *   scatter plot window.
   */
  void ScatterPlotAlarmConfigDialog::readSettingsFromScatterPlot() {
    setWindowTitle("Configure Alarming - " + m_window->windowTitle());

    m_alarmOntoPlot->setChecked(m_window->alarmingPlot());

    QPair<int, int> alarmPlotBoxSize = m_window->alarmPlotBoxSize();
    m_alarmOntoPlotSamples->setValue(alarmPlotBoxSize.first);
    m_alarmOntoPlotLines->setValue(alarmPlotBoxSize.second);

    m_alarmOntoViewport->setChecked(m_window->alarmingViewport());

    m_alarmOntoViewportUnits->setCurrentIndex(
        m_alarmOntoViewportUnits->findData(m_window->alarmViewportUnits()));

    QPair<double, double> alarmViewportDnBoxSize =
        m_window->alarmViewportDnBoxSize();
    m_alarmOntoViewportXDnSize->setText(
        QString::number(alarmViewportDnBoxSize.first));
    m_alarmOntoViewportYDnSize->setText(
        QString::number(alarmViewportDnBoxSize.second));

    QPair<int, int> alarmViewportScreenBoxSize =
        m_window->alarmViewportScreenBoxSize();
    m_alarmOntoViewportWidth->setValue(alarmViewportScreenBoxSize.first);
    m_alarmOntoViewportHeight->setValue(alarmViewportScreenBoxSize.second);

    refreshWidgetStates();
  }


  /**
   * Update the enabled/disabled states of the various widgets based on the
   *   current user inputs' states.
   */
  void ScatterPlotAlarmConfigDialog::refreshWidgetStates() {
    bool allValid = true;

    if (m_alarmOntoPlot->isChecked()) {
      m_alarmOntoPlotSamples->setEnabled(true);
      m_alarmOntoPlotLines->setEnabled(true);

      // Box sizes should be odd
      allValid = allValid && ((m_alarmOntoPlotSamples->value() % 2) == 1);
      allValid = allValid && ((m_alarmOntoPlotLines->value() % 2) == 1);
    }
    else {
      m_alarmOntoPlotSamples->setEnabled(false);
      m_alarmOntoPlotLines->setEnabled(false);
    }


    if (m_alarmOntoViewport->isChecked()) {
      m_alarmOntoViewportUnits->setEnabled(true);

      if (m_alarmOntoViewportUnits->itemData(
            m_alarmOntoViewportUnits->currentIndex()).toInt() ==
          ScatterPlotWindow::CubeUnits) {
        m_alarmOntoViewportXDnSize->setEnabled(true);
        m_alarmOntoViewportYDnSize->setEnabled(true);
        m_alarmOntoViewportWidth->setEnabled(false);
        m_alarmOntoViewportHeight->setEnabled(false);

        int unused = 0;
        QString textToTest = m_alarmOntoViewportXDnSize->text();
        const QValidator *xValidator = m_alarmOntoViewportXDnSize->validator();
        allValid = allValid && (xValidator->validate(textToTest, unused) ==
            QValidator::Acceptable);

        textToTest = m_alarmOntoViewportYDnSize->text();
        const QValidator *yValidator = m_alarmOntoViewportYDnSize->validator();
        allValid = allValid && (yValidator->validate(textToTest, unused) ==
            QValidator::Acceptable);
      }
      else {
        m_alarmOntoViewportXDnSize->setEnabled(false);
        m_alarmOntoViewportYDnSize->setEnabled(false);
        m_alarmOntoViewportWidth->setEnabled(true);
        m_alarmOntoViewportHeight->setEnabled(true);

        allValid = allValid && ((m_alarmOntoViewportWidth->value() % 2) == 1);
        allValid = allValid && ((m_alarmOntoViewportHeight->value() % 2) == 1);
      }
    }
    else {
      m_alarmOntoViewportUnits->setEnabled(false);
      m_alarmOntoViewportXDnSize->setEnabled(false);
      m_alarmOntoViewportYDnSize->setEnabled(false);
      m_alarmOntoViewportWidth->setEnabled(false);
      m_alarmOntoViewportHeight->setEnabled(false);
    }

    m_okayButton->setEnabled(allValid);
    m_applyButton->setEnabled(allValid);
  }
}
