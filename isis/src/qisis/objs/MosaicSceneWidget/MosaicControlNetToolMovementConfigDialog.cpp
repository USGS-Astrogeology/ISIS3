#include "MosaicControlNetToolMovementConfigDialog.h"

#include <float.h>

#include <QCheckBox>
#include <QComboBox>
#include <QDebug>
#include <QDialog>
#include <QDoubleValidator>
#include <QGridLayout>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QPushButton>
#include <QtWidgets>
#include <QElapsedTimer>

#include "Angle.h"
#include "IException.h"
#include "Latitude.h"
#include "Longitude.h"
#include "MosaicControlNetTool.h"
#include "MosaicSceneWidget.h"
#include "Projection.h"
#include "PvlGroup.h"
#include "SpecialPixel.h"

namespace Isis {
  /**
   * Create a config dialog that configures the given MosaicControlNetTool.
   *
   * @param tool The tool to read settings from and write settings to.
   * @param parent The qt-parent relationship parent.
   */
  MosaicControlNetToolMovementConfigDialog::MosaicControlNetToolMovementConfigDialog(
      MosaicControlNetTool *tool, QWidget *parent) : QDialog(parent) {
    m_tool = tool;

    setWindowTitle("Movement Options");

    QGridLayout *mainLayout = new QGridLayout;
    setLayout(mainLayout);

    int row = 0;

    QString showMovementWhatsThis =
        tr("Check or uncheck to draw or clear the movement arrows");
    QLabel *showMovementLabel = new QLabel("&Show Movement");
    showMovementLabel->setWhatsThis(showMovementWhatsThis);
    mainLayout->addWidget(showMovementLabel, row, 0);

    m_showMovementCheckBox = new QCheckBox;
    showMovementLabel->setBuddy(m_showMovementCheckBox);
    m_showMovementCheckBox->setWhatsThis(showMovementWhatsThis);
    connect(m_showMovementCheckBox, SIGNAL(toggled(bool)),
            this, SLOT(refreshWidgetStates()));
    mainLayout->addWidget(m_showMovementCheckBox, row, 1, 1, 1, Qt::AlignRight);
    row++;

    QString colorSourceWhatsThis =
        tr("Select criteria for arrow color");
    QLabel *colorSourceLabel = new QLabel("&Color Criteria");
    colorSourceLabel->setWhatsThis(colorSourceWhatsThis);
    mainLayout->addWidget(colorSourceLabel, row, 0, 1, 1);

    m_colorSourceComboBox = new QComboBox;
    m_colorSourceComboBox->addItem(tr("No Color"), MosaicControlNetTool::NoColor);
    m_colorSourceComboBox->addItem(tr("Measure Count"), MosaicControlNetTool::MeasureCount);
    m_colorSourceComboBox->addItem(tr("Residual Magnitude"),
                                   MosaicControlNetTool::ResidualMagnitude);
    m_colorSourceComboBox->setCurrentIndex(
        m_colorSourceComboBox->findData(MosaicControlNetTool::MeasureCount));
    connect(m_colorSourceComboBox, SIGNAL(currentIndexChanged(int)),
            this, SLOT(refreshWidgetStates()));

    colorSourceLabel->setBuddy(m_colorSourceComboBox);
    m_colorSourceComboBox->setWhatsThis(colorSourceWhatsThis);
    mainLayout->addWidget(m_colorSourceComboBox, row, 1, 1, 1, Qt::AlignRight);
    row++;

    QString brightestMeasureCountValueWhatsThis =
        tr("Measure count of brightest color. Points with this measure count or greater will be "
           "colored the brightest.");
    m_brightestMeasureCountValueLabel = new QLabel("Min &measure count to color");
    m_brightestMeasureCountValueLabel->setWhatsThis(brightestMeasureCountValueWhatsThis);
    mainLayout->addWidget(m_brightestMeasureCountValueLabel, row, 0, 1, 1);

    m_brightestMeasureCountValueLineEdit = new QLineEdit;
    m_brightestMeasureCountValueLineEdit->setValidator(new QIntValidator(1, INT_MAX, NULL));
    connect(m_brightestMeasureCountValueLineEdit, SIGNAL(textChanged(const QString &)),
            this, SLOT(refreshWidgetStates()));

    m_brightestMeasureCountValueLabel->setBuddy(m_brightestMeasureCountValueLineEdit);
    m_colorSourceComboBox->setWhatsThis(brightestMeasureCountValueWhatsThis);
    mainLayout->addWidget(m_brightestMeasureCountValueLineEdit, row, 1, 1, 1, Qt::AlignRight);
    row++;

    QString brightestResidualMagnitudeValueWhatsThis =
        tr("Residual magnitude of brightest color. Points with this maximum residual magnitude or "
           "greater will be colored the brightest.");
    m_brightestResidualMagValueLabel = new QLabel("Min &residual magnitude to color");
    m_brightestResidualMagValueLabel->setWhatsThis(brightestResidualMagnitudeValueWhatsThis);
    mainLayout->addWidget(m_brightestResidualMagValueLabel, row, 0, 1, 1);

    m_brightestResidualMagValueLineEdit = new QLineEdit;
    m_brightestResidualMagValueLineEdit->setValidator(new QDoubleValidator(0.0, DBL_MAX, 8, NULL));
    connect(m_brightestResidualMagValueLineEdit, SIGNAL(textChanged(const QString &)),
            this, SLOT(refreshWidgetStates()));

    m_brightestResidualMagValueLabel->setBuddy(m_brightestResidualMagValueLineEdit);
    m_colorSourceComboBox->setWhatsThis(brightestResidualMagnitudeValueWhatsThis);
    mainLayout->addWidget(m_brightestResidualMagValueLineEdit, row, 1, 1, 1, Qt::AlignRight);
    row++;

    mainLayout->setRowMinimumHeight(row, 10);
    row++;

    QHBoxLayout *buttonsAreaLayout = new QHBoxLayout;
    mainLayout->addLayout(buttonsAreaLayout, row, 0, 1, 2, Qt::AlignRight);

    buttonsAreaLayout->addStretch();

    m_okayButton = new QPushButton("&Ok");
    m_okayButton->setIcon(QIcon::fromTheme("dialog-ok"));
    connect(m_okayButton, SIGNAL(clicked()),
            this, SLOT(applySettings()));
    connect(m_okayButton, SIGNAL(clicked()),
            this, SLOT(accept()));
    buttonsAreaLayout->addWidget(m_okayButton);

    m_applyButton = new QPushButton("&Apply");
    m_applyButton->setIcon(QIcon::fromTheme("dialog-ok-apply"));
    connect(m_applyButton, SIGNAL(clicked()),
            this, SLOT(applySettings()));
    buttonsAreaLayout->addWidget(m_applyButton);

    QPushButton *cancelButton = new QPushButton("&Cancel");
    cancelButton->setIcon(QIcon::fromTheme("dialog-cancel"));
    connect(cancelButton, SIGNAL(clicked()),
            this, SLOT(reject()));
    buttonsAreaLayout->addWidget(cancelButton);

    readSettings();
  }


  /**
   * Clean up allocated memory.
   */
  MosaicControlNetToolMovementConfigDialog::~MosaicControlNetToolMovementConfigDialog() {
  }


  /**
   * Apply the user's current settings to the tool.
   */
  void MosaicControlNetToolMovementConfigDialog::applySettings() {
    bool haveMeasureCountBrightest = !m_brightestMeasureCountValueLineEdit->text().isEmpty();
    bool haveResidualMagBrightest = !m_brightestResidualMagValueLineEdit->text().isEmpty();

    if (!m_showMovementCheckBox->isChecked()) {
      m_tool->setMovementArrowColorSource(MosaicControlNetTool::NoMovement,
          haveMeasureCountBrightest? m_brightestMeasureCountValueLineEdit->text().toInt() : -1,
          haveResidualMagBrightest? m_brightestResidualMagValueLineEdit->text().toDouble() : Null);
    }
    else {

      m_tool->setMovementArrowColorSource((MosaicControlNetTool::MovementColorSource)
            m_colorSourceComboBox->itemData(m_colorSourceComboBox->currentIndex()).toInt(),
          haveMeasureCountBrightest? m_brightestMeasureCountValueLineEdit->text().toInt() : -1,
          haveResidualMagBrightest? m_brightestResidualMagValueLineEdit->text().toDouble() : Null);
    }

    // Now re-read to verify
    readSettings();
  }


  /**
   * Read the tool's current settings and set the widget states to match.
   */
  void MosaicControlNetToolMovementConfigDialog::readSettings() {
    MosaicControlNetTool::MovementColorSource currentSrc = m_tool->movementArrowColorSource();
    m_showMovementCheckBox->setChecked(currentSrc != MosaicControlNetTool::NoMovement);

    if (currentSrc != MosaicControlNetTool::NoMovement) {
      m_colorSourceComboBox->setCurrentIndex(m_colorSourceComboBox->findData(currentSrc));
    }

    if (m_tool->maxMovementColorMeasureCount() != -1) {
      m_brightestMeasureCountValueLineEdit->setText(
          QString::number(m_tool->maxMovementColorMeasureCount()));
    }

    if (m_tool->maxMovementColorResidualMagnitude() != Null) {
      m_brightestResidualMagValueLineEdit->setText(
          QString::number(m_tool->maxMovementColorResidualMagnitude()));
    }

    refreshWidgetStates();
  }


  /**
   * Enables or disables widgets depending on the state of the tool.
   */
  void MosaicControlNetToolMovementConfigDialog::refreshWidgetStates() {

    bool movementEnabled = m_showMovementCheckBox->isChecked();
    m_colorSourceComboBox->setEnabled(movementEnabled);


    bool comboSelectedMeasureCount =
        (m_colorSourceComboBox->itemData(m_colorSourceComboBox->currentIndex()).toInt() ==
         MosaicControlNetTool::MeasureCount);
    bool movementIsMeasureCnt = movementEnabled && comboSelectedMeasureCount;
    m_brightestMeasureCountValueLabel->setEnabled(movementIsMeasureCnt);
    m_brightestMeasureCountValueLineEdit->setEnabled(movementIsMeasureCnt);

    bool comboSelectedResidualMagnitude =
        (m_colorSourceComboBox->itemData(m_colorSourceComboBox->currentIndex()).toInt() ==
         MosaicControlNetTool::ResidualMagnitude);
    bool movementIsResidualMagnitude = movementEnabled && comboSelectedResidualMagnitude;
    m_brightestResidualMagValueLabel->setEnabled(movementIsResidualMagnitude);
    m_brightestResidualMagValueLineEdit->setEnabled(movementIsResidualMagnitude);

    bool comboSelectedNoColor = 
        (m_colorSourceComboBox->itemData(m_colorSourceComboBox->currentIndex()).toInt() ==
         MosaicControlNetTool::NoColor);

    bool validState = !movementEnabled ||
        comboSelectedNoColor ||
        (comboSelectedMeasureCount &&
         !m_brightestMeasureCountValueLineEdit->text().isEmpty()) ||
        (comboSelectedResidualMagnitude &&
         !m_brightestResidualMagValueLineEdit->text().isEmpty());
    m_okayButton->setEnabled(validState);
    m_applyButton->setEnabled(validState);
  }
}
