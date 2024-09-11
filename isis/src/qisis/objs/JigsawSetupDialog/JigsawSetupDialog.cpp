#include "JigsawSetupDialog.h"

#include <vector>

#include <QDebug>
#include <QIdentityProxyModel>
#include <QMessageBox>
#include <QPushButton>
#include <QSortFilterProxyModel>
#include <QStandardItemModel>
#include <QItemSelection>

#include "BundleObservationSolveSettings.h"
#include "BundleSolutionInfo.h"
#include "BundleSettings.h"
#include "BundleTargetBody.h"
#include "Camera.h"
#include "Control.h"
#include "Cube.h"
#include "Image.h"
#include "IString.h"
#include "MaximumLikelihoodWFunctions.h"
#include "Project.h"
#include "ProjectItem.h"
#include "ProjectItemProxyModel.h"
#include "SpecialPixel.h"
#include "SortFilterProxyModel.h"
#include "ui_JigsawSetupDialog.h"

namespace Isis {

  JigsawSetupDialog::JigsawSetupDialog(Project *project, bool useLastSettings, bool readOnly,
                                       QWidget *parent) : QDialog(parent),
                                       m_ui(new Ui::JigsawSetupDialog) {
    //
    // Note: When the ui is set up, all initializations to enabled/disabled
    // are taken care of. Also connections between some widgets will be taken
    // care of in the ui setup.

    // For example:
    //   pointRadiusSigmaCheckBox is connected to pointRadiusSigmaLineEdit
    //   outlierRejectionCheckBox is connected
    //       to outlierRejectionMultiplierLabel and outlierRejectionMultiplierLineEdit
    //
    // These connections and settings can be found in the JigsawSetupDialog.ui file
    // created by QtDesigner and may be edited by opening the ui file in QtDesigner.
    //
    // More complex connections such as the relationship between positionSolveOption and
    // spkDegree are handled in this file by the on_widgetName_signal methods.
    m_ui->setupUi(this);

    m_project = project;

    m_ui->JigsawSetup->setCurrentIndex(0);

    if (readOnly) {
      makeReadOnly();
    }

    m_bundleSettings = BundleSettingsQsp(new BundleSettings());

    //connect( m_project->directory()->model(), SIGNAL(selectionChanged(QList<ProjectItem *> &)),
    //         this, SLOT(on_projectItemSelectionChanged(const QList<ProjectItem *> &) ) );

    // initializations for general tab

    // fill control net combo box from project
    for (int i = 0; i < project->controls().size(); i++) {
      ControlList* conlist = project->controls().at(i);
      for (int j = 0; j < conlist->size(); j++) {
        Control *control = conlist->at(j);

        QVariant v = QVariant::fromValue((void*)control);

        m_ui->inputControlNetCombo->addItem(control->displayProperties()->displayName(), v);
      }
    }
    // add control nets from bundle solutions, if any
    int numBundles = project->bundleSolutionInfo().size();
    for (int i = 0; i < numBundles; i++) {
      Control *bundleControl = project->bundleSolutionInfo().at(i)->control();

      QVariant v = QVariant::fromValue((void*)bundleControl);

      m_ui->inputControlNetCombo->addItem(bundleControl->displayProperties()->displayName(), v);
    }

    // initialize default output control network filename
    FileName fname = m_ui->inputControlNetCombo->currentText().toStdString();
    m_ui->outputControlNetLineEdit->setText(QString::fromStdString(fname.baseName()) + "-out.net");

    // initializations for observation solve settings tab
    createObservationSolveSettingsTreeView();


    // Create default settings for all of the observations
    QList<Image *> imagesToAdd;
    // If we have selected any project items, find the images and add them to imagesToAdd
    if (!m_project->directory()->model()->selectedItems().isEmpty()) {
      foreach (ProjectItem * projItem, m_project->directory()->model()->selectedItems()) {
        if (projItem->isImage()) {
          imagesToAdd.append(projItem->image());  
        }
        else if (projItem->isImageList()) {
          for (int i = 0; i < projItem->rowCount(); i++) {
            imagesToAdd.append(projItem->child(i)->image());  
          }
        }
      }
    }
    // if we didnt have any images selected in the previous case, or no proj items were selected,
    // take all images from the project tree
    if (imagesToAdd.isEmpty()) {
      ProjectItem *imgRoot = m_project->directory()->model()->findItemData(QVariant("Images"),0);
      if (imgRoot) {
        for (int i = 0; i < imgRoot->rowCount(); i++) {
          ProjectItem * imglistItem = imgRoot->child(i);
          for (int j = 0; j < imglistItem->rowCount(); j++) {
            ProjectItem * imgItem = imglistItem->child(j);
            if (imgItem->isImage()) {
              imagesToAdd.append(imgItem->image());     
            }
          }
        } 
      }
    }
    
    // create default  solve settings for supported camera types
    BundleObservationSolveSettings defaultFramingSettings;
    BundleObservationSolveSettings defaultLineScanSettings;
    defaultLineScanSettings.setInstrumentPointingSettings(
                      BundleObservationSolveSettings::AnglesVelocityAcceleration, true, 2, 2, true);


    // sort each chosen image into its camera type 
    foreach (Image * image, imagesToAdd) {
      int cameraType = image->cube()->camera()->GetCameraType();
      if (cameraType == Camera::LineScan) {
        defaultLineScanSettings.addObservationNumber(image->observationNumber());
      }
      else { // assume cameraType == Camera::Framing
        defaultFramingSettings.addObservationNumber(image->observationNumber());  
      }
    }

    // only add defaults that have been applied
    QList<BundleObservationSolveSettings> solveSettingsList;
    if (defaultFramingSettings.observationNumbers().count()) 
      solveSettingsList.append(defaultFramingSettings);
    if (defaultLineScanSettings.observationNumbers().count())
      solveSettingsList.append(defaultLineScanSettings);

    m_bundleSettings->setObservationSolveOptions(solveSettingsList);

    // Populate the solve option comboboxes
    const QStringList positionOptions{"NONE", "POSITION", "VELOCITY", "ACCELERATION", "ALL"};
    m_ui->positionComboBox->insertItems(0, positionOptions);
    m_ui->positionComboBox->setCurrentIndex(0);

    const QStringList pointingOptions{"NONE", "ANGLES", "VELOCITY", "ACCELERATION", "ALL"};
    m_ui->pointingComboBox->insertItems(0, pointingOptions);
    m_ui->pointingComboBox->setCurrentIndex(1);

    // The degree solve options' minimums are -1 (set in ui file), make the -1's display as N/A
    m_ui->spkSolveDegreeSpinBox->setSpecialValueText("N/A");
    m_ui->ckSolveDegreeSpinBox->setSpecialValueText("N/A");


    QStringList tableHeaders;
    tableHeaders << "coefficients" << "description" << "units" << "a priori sigma";
    m_ui->positionAprioriSigmaTable->setHorizontalHeaderLabels(tableHeaders);
    m_ui->pointingAprioriSigmaTable->setHorizontalHeaderLabels(tableHeaders);

    // Set the default size of the columns
    m_ui->positionAprioriSigmaTable->setColumnWidth(0, fontMetrics().horizontalAdvance(tableHeaders.at(0)) + 10);
    m_ui->positionAprioriSigmaTable->setColumnWidth(1, fontMetrics().horizontalAdvance(tableHeaders.at(1)) + 10);
    m_ui->positionAprioriSigmaTable->setColumnWidth(2, fontMetrics().horizontalAdvance(tableHeaders.at(2)) + 10);
    m_ui->positionAprioriSigmaTable->setColumnWidth(3, fontMetrics().horizontalAdvance(tableHeaders.at(3)) + 10);

    m_ui->pointingAprioriSigmaTable->setColumnWidth(0, fontMetrics().horizontalAdvance(tableHeaders.at(0)) + 10);
    m_ui->pointingAprioriSigmaTable->setColumnWidth(1, fontMetrics().horizontalAdvance(tableHeaders.at(1)) + 10);
    m_ui->pointingAprioriSigmaTable->setColumnWidth(2, fontMetrics().horizontalAdvance(tableHeaders.at(2)) + 10);
    m_ui->pointingAprioriSigmaTable->setColumnWidth(3, fontMetrics().horizontalAdvance(tableHeaders.at(3)) + 10);


    // Add validators to the tables in the observation solve settings tab to validate the a priori
    // sigma items
    connect(m_ui->positionAprioriSigmaTable, SIGNAL(itemChanged(QTableWidgetItem *)),
            this, SLOT(validateSigmaValue(QTableWidgetItem *)));
    connect(m_ui->pointingAprioriSigmaTable, SIGNAL(itemChanged(QTableWidgetItem *)),
            this, SLOT(validateSigmaValue(QTableWidgetItem *)));

    // initializations for target body tab

    // fill target combo box from project
    for (int i = 0; i < project->targetBodies().size(); i++) {
      TargetBodyQsp target = project->targetBodies().at(i);

      QVariant v = QVariant::fromValue((void*)target.data());

      QString name = target->displayProperties()->displayName();

      if (name == "MOON")
        m_ui->targetBodyComboBox->addItem(QIcon(QString::fromStdString(FileName(
                  "$ISISROOT/appdata/images/icons/weather-clear-night.png").expanded())), name, v);
      else if (name == "Enceladus")
        m_ui->targetBodyComboBox->addItem(QIcon(QString::fromStdString(FileName(
                  "$ISISROOT/appdata/images/icons/nasa_enceladus.png").expanded())), name, v);
      else if (name == "Mars")
        m_ui->targetBodyComboBox->addItem(QIcon(QString::fromStdString(FileName(
                  "$ISISROOT/appdata/images/icons/nasa_mars.png").expanded())), name, v);
      else if (name == "Titan")
        m_ui->targetBodyComboBox->addItem(QIcon(QString::fromStdString(FileName(
                  "$ISISROOT/appdata/images/icons/nasa_titan.png").expanded())), name, v);
      else
        m_ui->targetBodyComboBox->addItem(QIcon(QString::fromStdString(FileName(
                  "$ISISROOT/appdata/images/icons/weather-clear-night.png").expanded())), name, v);
    }

    m_ui->radiiButtonGroup->setId(m_ui->noneRadiiRadioButton,0);
    m_ui->radiiButtonGroup->setId(m_ui->triaxialRadiiRadioButton,1);
    m_ui->radiiButtonGroup->setId(m_ui->meanRadiusRadioButton,2);
    m_ui->noneRadiiRadioButton->setChecked(true);

    // validators for target body entries...
    QDoubleValidator *sigmaValidator = new QDoubleValidator(0.0, 1.0e+4, 8, this);
    sigmaValidator->setNotation(QDoubleValidator::ScientificNotation);

    // right ascension valid range is from 0 to 360 degrees
    QDoubleValidator *raValidator = new QDoubleValidator(0.0, 360.0, 8, this);
    raValidator->setNotation(QDoubleValidator::StandardNotation);
    m_ui->rightAscensionLineEdit->setValidator(raValidator);
    m_ui->rightAscensionSigmaLineEdit->setValidator(sigmaValidator);

    m_ui->rightAscensionVelocityLineEdit->setValidator(raValidator);
    m_ui->rightAscensionVelocitySigmaLineEdit->setValidator(sigmaValidator);

    // declination valid range is from -90 to +90 degrees
    QDoubleValidator *decValidator = new QDoubleValidator(-90.0, 90.0, 8,
                                                          m_ui->declinationLineEdit);
    decValidator->setNotation(QDoubleValidator::StandardNotation);
    m_ui->declinationLineEdit->setValidator(decValidator);
    m_ui->declinationSigmaLineEdit->setValidator(sigmaValidator);

    m_ui->declinationVelocityLineEdit->setValidator(new QDoubleValidator(0.0, 1.0e+10, 8,
                                                    m_ui->declinationVelocityLineEdit));
    m_ui->declinationVelocitySigmaLineEdit->setValidator(sigmaValidator);

    m_ui->primeMeridianOffsetLineEdit->setValidator(raValidator);
    m_ui->primeMeridianOffsetSigmaLineEdit->setValidator(sigmaValidator);

    // spin rate valid for non-negative values
    m_ui->spinRateLineEdit->setValidator(new QDoubleValidator(0.0, 1.0e+10, 8,
                                                              m_ui->spinRateLineEdit));
    m_ui->spinRateSigmaLineEdit->setValidator(sigmaValidator);

    m_ui->aRadiusLineEdit->setValidator(new QDoubleValidator(0.0, 1.0e+10, 8,
                                                             m_ui->aRadiusLineEdit));
    m_ui->aRadiusSigmaLineEdit->setValidator(sigmaValidator);

    m_ui->bRadiusLineEdit->setValidator(new QDoubleValidator(0.0, 1.0e+10, 8,
                                                             m_ui->bRadiusLineEdit));
    m_ui->bRadiusSigmaLineEdit->setValidator(sigmaValidator);

    m_ui->cRadiusLineEdit->setValidator(new QDoubleValidator(0.0, 1.0e+10, 8,
                                                             m_ui->cRadiusLineEdit));
    m_ui->cRadiusSigmaLineEdit->setValidator(sigmaValidator);

    m_ui->meanRadiusLineEdit->setValidator(new QDoubleValidator(0.0, 1.0e+10, 8,
                                                                m_ui->meanRadiusLineEdit));
    m_ui->meanRadiusSigmaLineEdit->setValidator(sigmaValidator);



    // jigsaw run setup general tab validation
    // global apriori point sigmas
    m_ui->pointLatitudeSigmaLineEdit->setValidator(new QDoubleValidator(1.0e-10, 1.0e+10, 8, this));
    m_ui->pointLongitudeSigmaLineEdit->setValidator(new QDoubleValidator(1.0e-10, 1.0e+10, 8,this));
    m_ui->pointRadiusSigmaLineEdit->setValidator(new QDoubleValidator(1.0e-10, 1.0e+10, 8, this));

    // outlier rejection
    m_ui->outlierRejectionMultiplierLineEdit->setValidator(
                                                  new QDoubleValidator(1.0e-10, 1.0e+10, 8, this));
    m_ui->maximumLikelihoodModel1QuantileLineEdit->setValidator(
                                                  new QDoubleValidator(1.0e-10, 1.0, 8, this));
    m_ui->maximumLikelihoodModel2QuantileLineEdit->setValidator(
                                                  new QDoubleValidator(1.0e-10, 1.0, 8, this));
    m_ui->maximumLikelihoodModel3QuantileLineEdit->setValidator(
                                                  new QDoubleValidator(1.0e-10, 1.0, 8, this));

    // convergence criteria
    m_ui->sigma0ThresholdLineEdit->setValidator(new QDoubleValidator(1.0e-20, 1.0e+10, 8, this));
    m_ui->maximumIterationsLineEdit->setValidator(new QIntValidator(1, 10000, this));



    // signals for target body tab
//    connect(m_ui->radiiButtonGroup, SIGNAL(buttonClicked(int)),
//            this, SLOT(on_radiiGroupClicked(int)));
    connect(m_ui->radiiButtonGroup, SIGNAL(buttonClicked(int)),
            this, SLOT(on_radiiButtonGroupClicked(int)));
    connect(m_ui->aRadiusLineEdit, SIGNAL(textChanged(QString)), SLOT(slotTextChanged(QString)));
    connect(m_ui->aRadiusLineEdit, SIGNAL(returnPressed()), SLOT(checkIsValid()));
    connect(m_ui->aRadiusLineEdit, SIGNAL(editingFinished()), SLOT(checkIsValid()));
    connect(m_ui->aRadiusLineEdit, SIGNAL(textChanged(QString)), SLOT(on_aRadiusLineEdit_textChanged(QString)));
  }



  JigsawSetupDialog::~JigsawSetupDialog() {
    // delete/null m_ui since we did "new" this pointers in the constructor
    if (m_ui) {
      delete m_ui;
    }
    m_ui = NULL;
    // do not delete/null m_project since we didn't "new" this pointer
  }


  void JigsawSetupDialog::on_pointRadiusSigmaCheckBox_toggled(bool checked) {
    m_ui->pointRadiusSigmaLineEdit->setEnabled(checked);
  }

//  m_ui->positionComboBox has been removed from the general tab, it is planned to be moved to 
//  the obs solve settings tab. This function will be commented out until it is added back.
//   void JigsawSetupDialog::on_positionComboBox_currentIndexChanged(int index) {

//     // indices:
//     // 0 = none, 1 = position, 2 = velocity, 3 = acceleration, 4 = all
//     bool solvePosition                  = (bool) (index > 0);
//     bool solveVelocity                  = (bool) (index > 1);
//     bool solveAcceleration              = (bool) (index > 2);
// //    bool solveAllPolynomialCoefficients = (bool) (index > 3);

//     m_ui->hermiteSplineCheckBox->setEnabled(solvePosition);
//     m_ui->positionSigmaLabel->setEnabled(solvePosition);
//     m_ui->positionSigmaLineEdit->setEnabled(solvePosition);
//     m_ui->positionSigmaUnitsLabel->setEnabled(solvePosition);

//     m_ui->velocitySigmaLabel->setEnabled(solveVelocity);
//     m_ui->velocitySigmaLineEdit->setEnabled(solveVelocity);
//     m_ui->velocitySigmaUnitsLabel->setEnabled(solveVelocity);

//     m_ui->accelerationSigmaLabel->setEnabled(solveAcceleration);
//     m_ui->accelerationSigmaLineEdit->setEnabled(solveAcceleration);
//     m_ui->accelerationSigmaUnitsLabel->setEnabled(solveAcceleration);

// //    m_ui->spkDegreeLabel->setEnabled(solveAllPolynomialCoefficients);
// //    m_ui->spkDegreeSpinBox->setEnabled(solveAllPolynomialCoefficients);
// //    m_ui->spkSolveDegreeLabel->setEnabled(solveAllPolynomialCoefficients);
// //    m_ui->spkSolveDegreeSpinBox->setEnabled(solveAllPolynomialCoefficients);

//   }

//  m_ui->pointingComboBox has been removed from the general tab, it is planned to be moved to 
//  the obs solve settings tab. This function will be commented out until it is added back.
//   void JigsawSetupDialog::on_pointingComboBox_currentIndexChanged(int index) {

//     // indices:
//     // 0 = angles, 1 = none, 2 = velocity, 3 = acceleration, 4 = all
//     bool solveAngles                    = (bool) (index == 0 || index > 1);
//     bool solveAngularVelocity           = (bool) (index > 1);
//     bool solveAngularAcceleration       = (bool) (index > 2);
// //    bool solveAllPolynomialCoefficients = (bool) (index > 3);

//     m_ui->twistCheckBox->setEnabled(solveAngles);
//     m_ui->fitOverPointingCheckBox->setEnabled(solveAngles);

// //    m_ui->ckDegreeLabel->setEnabled(solveAllPolynomialCoefficients);
// //    m_ui->ckDegreeSpinBox->setEnabled(solveAllPolynomialCoefficients);
// //    m_ui->ckSolveDegreeSpinBox->setEnabled(solveAllPolynomialCoefficients);
// //    m_ui->ckSolveDegreeLabel->setEnabled(solveAllPolynomialCoefficients);

//     m_ui->pointingAnglesSigmaLabel->setEnabled(solveAngles);
//     m_ui->pointingAnglesSigmaLineEdit->setEnabled(solveAngles);
//     m_ui->pointingAnglesSigmaUnitsLabel->setEnabled(solveAngles);

//     m_ui->pointingAngularVelocitySigmaLabel->setEnabled(solveAngularVelocity);
//     m_ui->pointingAngularVelocitySigmaLineEdit->setEnabled(solveAngularVelocity);
//     m_ui->pointingAngularVelocitySigmaUnitsLabel->setEnabled(solveAngularVelocity);

//     m_ui->pointingAngularAccelerationSigmaLabel->setEnabled(solveAngularAcceleration);
//     m_ui->pointingAngularAccelerationSigmaLineEdit->setEnabled(solveAngularAcceleration);
//     m_ui->pointingAngularAccelerationSigmaUnitsLabel->setEnabled(solveAngularAcceleration);

//   }


  void JigsawSetupDialog::on_maximumLikelihoodModel1ComboBox_currentIndexChanged(int index) {

    bool model1Selected = (bool) (index > 0);

    // lock/unlock current tier's quantile and next tier's model
    m_ui->maximumLikelihoodModel1QuantileLineEdit->setEnabled(model1Selected);
    m_ui->maximumLikelihoodModel2Label->setEnabled(model1Selected);
    m_ui->maximumLikelihoodModel2ComboBox->setEnabled(model1Selected);
    m_ui->maximumLikelihoodModel2QuantileLineEdit->setEnabled(
                                            m_ui->maximumLikelihoodModel2ComboBox->currentIndex());

    // when setting "NONE", set remaining max likelihood options to false  
    if (!model1Selected) {
      m_ui->maximumLikelihoodModel2QuantileLineEdit->setEnabled(false);
      m_ui->maximumLikelihoodModel3QuantileLineEdit->setEnabled(false);
      m_ui->maximumLikelihoodModel3Label->setEnabled(false);
      m_ui->maximumLikelihoodModel3ComboBox->setEnabled(false);
    }

    on_maximumLikelihoodModel1QuantileLineEdit_textChanged("");
    on_maximumLikelihoodModel2QuantileLineEdit_textChanged("");
    on_maximumLikelihoodModel3QuantileLineEdit_textChanged("");

    // sigma and max likelihood options are exclusive
    m_ui->outlierRejectionCheckBox->setEnabled(!model1Selected);
  }


  void JigsawSetupDialog::on_maximumLikelihoodModel2ComboBox_currentIndexChanged(int index) {

    bool model2Selected = (bool)(index > 0);

    // lock/unlock current tier's quantile and next tier's model
    m_ui->maximumLikelihoodModel2QuantileLineEdit->setEnabled(model2Selected);
    m_ui->maximumLikelihoodModel3Label->setEnabled(model2Selected);
    m_ui->maximumLikelihoodModel3ComboBox->setEnabled(model2Selected);
    m_ui->maximumLikelihoodModel3QuantileLineEdit->setEnabled(
                                            m_ui->maximumLikelihoodModel3ComboBox->currentIndex());

    // when setting "NONE", set remaining max likelihood options to false  
    if (!model2Selected) {
      m_ui->maximumLikelihoodModel3QuantileLineEdit->setEnabled(false);
    }

    on_maximumLikelihoodModel2QuantileLineEdit_textChanged("");
    on_maximumLikelihoodModel3QuantileLineEdit_textChanged("");
  }


  void JigsawSetupDialog::on_maximumLikelihoodModel3ComboBox_currentIndexChanged(int index) {

    bool model3Selected = (bool)(index > 0);

    m_ui->maximumLikelihoodModel3QuantileLineEdit->setEnabled(model3Selected);
    on_maximumLikelihoodModel3QuantileLineEdit_textChanged("");

  }


  void JigsawSetupDialog::on_outlierRejectionCheckBox_stateChanged(int arg1) {

    on_outlierRejectionMultiplierLineEdit_textChanged("");
    m_ui->outlierRejectionMultiplierLineEdit->setEnabled(arg1);

    // sigma and maxlikelihood options are exclusive
    m_ui->CQuantileLabel->setEnabled(!arg1);
    m_ui->maxLikelihoodEstimationLabel->setEnabled(!arg1);
    m_ui->maximumLikelihoodModel1ComboBox->setEnabled(!arg1);
    m_ui->maximumLikelihoodModel1Label->setEnabled(!arg1);
  }


  void JigsawSetupDialog::fillFromSettings(const BundleSettingsQsp settings) {

    // general tab
    m_ui->observationModeCheckBox->setChecked(settings->solveObservationMode());
    m_ui->pointRadiusSigmaCheckBox->setChecked(settings->solveRadius());
    // m_ui->updateCubeLabelCheckBox->setChecked(settings->updateCubeLabel());
    m_ui->errorPropagationCheckBox->setChecked(settings->errorPropagation());
    m_ui->outlierRejectionCheckBox->setChecked(settings->outlierRejection());
    m_ui->outlierRejectionMultiplierLineEdit->setText(QString::number(settings->outlierRejectionMultiplier()));
    m_ui->sigma0ThresholdLineEdit->setText(QString::number(settings->convergenceCriteriaThreshold()));
    m_ui->maximumIterationsLineEdit->setText(QString::number(settings->convergenceCriteriaMaximumIterations()));

    // weighting tab
    if ( !IsNullPixel(settings->globalPointCoord1AprioriSigma()) ) {
      m_ui->pointLatitudeSigmaLineEdit->setText(QString::number(settings->globalPointCoord1AprioriSigma()));
      m_ui->pointLatitudeSigmaLineEdit->setModified(true);
    }
    if ( !IsNullPixel(settings->globalPointCoord2AprioriSigma()) ) {
      m_ui->pointLongitudeSigmaLineEdit->setText(QString::number(settings->globalPointCoord2AprioriSigma()));
      m_ui->pointLongitudeSigmaLineEdit->setModified(true);
    }
    if ( !IsNullPixel(settings->globalPointCoord3AprioriSigma()) ) {
      m_ui->pointRadiusSigmaLineEdit->setText(QString::number(settings->globalPointCoord3AprioriSigma()));
      m_ui->pointRadiusSigmaLineEdit->setModified(true);

    }

    // If we click setup after changing image selection in the project tree, not all images will
    // have solve settings. Here we add those images and their default settings to the list
    QList<BundleObservationSolveSettings> defaultSettings = m_bundleSettings->observationSolveSettings();
    QList<BundleObservationSolveSettings> fillSettings = settings->observationSolveSettings();

    for (auto &solveSettings : defaultSettings) {      
      // Remove any images from defaultSettings that exist in fillSettings
      foreach (QString observationNumber, solveSettings.observationNumbers()) {
        // The method will return a default with no obs numbers if none are found
        if (!settings->observationSolveSettings(observationNumber).observationNumbers().isEmpty()) {
          solveSettings.removeObservationNumber(observationNumber);
        }
      }
      // Append leftover defaultSettings
      if (!solveSettings.observationNumbers().isEmpty()) {
        fillSettings.append(solveSettings);
      }
    }

    m_bundleSettings->setObservationSolveOptions(fillSettings);
    
    update();

  }


  BundleSettingsQsp JigsawSetupDialog::bundleSettings() {

    BundleSettingsQsp settings = m_bundleSettings;
    settings->setValidateNetwork(true);

    // solve options
    double latitudeSigma  = -1.0;
    double longitudeSigma = -1.0;
    double radiusSigma    = -1.0;
    if (m_ui->pointLatitudeSigmaLineEdit->isModified()) {
      latitudeSigma = m_ui->pointLatitudeSigmaLineEdit->text().toDouble();
    }
    if (m_ui->pointLongitudeSigmaLineEdit->isModified()) {
      longitudeSigma = m_ui->pointLongitudeSigmaLineEdit->text().toDouble();
    }
    if (m_ui->pointRadiusSigmaLineEdit->isModified()) {
      radiusSigma = m_ui->pointRadiusSigmaLineEdit->text().toDouble();
    }
    // Stick with the default coordinate types for the bundle and reports until
    // a gui is added to get the settings
    settings->setSolveOptions(m_ui->observationModeCheckBox->isChecked(),
                              false,
                              // m_ui->updateCubeLabelCheckBox->isChecked(),
                              m_ui->errorPropagationCheckBox->isChecked(),
                              m_ui->pointRadiusSigmaCheckBox->isChecked(),
                              SurfacePoint::Latitudinal, SurfacePoint::Latitudinal,
                              latitudeSigma,
                              longitudeSigma,
                              radiusSigma);
    settings->setOutlierRejection(m_ui->outlierRejectionCheckBox->isChecked(),
                                  m_ui->outlierRejectionMultiplierLineEdit->text().toDouble());

    // convergence criteria
    settings->setConvergenceCriteria(BundleSettings::Sigma0,
                                     m_ui->sigma0ThresholdLineEdit->text().toDouble(),
                                     m_ui->maximumIterationsLineEdit->text().toInt()); // TODO: change this to give user a choice between sigma0 and param corrections???

    // max likelihood estimation
    if (m_ui->maximumLikelihoodModel1ComboBox->currentText().compare("NONE") != 0) {
      // if model1 is not "NONE", add to the models list with its quantile
      settings->addMaximumLikelihoodEstimatorModel(
          MaximumLikelihoodWFunctions::stringToModel(
              m_ui->maximumLikelihoodModel1ComboBox->currentText()),
          m_ui->maximumLikelihoodModel1QuantileLineEdit->text().toDouble());

      if (m_ui->maximumLikelihoodModel2ComboBox->currentText().compare("NONE") != 0) {
        // if model2 is not "NONE", add to the models list with its quantile
        settings->addMaximumLikelihoodEstimatorModel(
            MaximumLikelihoodWFunctions::stringToModel(
                m_ui->maximumLikelihoodModel2ComboBox->currentText()),
            m_ui->maximumLikelihoodModel2QuantileLineEdit->text().toDouble());

        if (m_ui->maximumLikelihoodModel3ComboBox->currentText().compare("NONE") != 0) {
          // if model3 is not "NONE", add to the models list with its quantile
          settings->addMaximumLikelihoodEstimatorModel(
              MaximumLikelihoodWFunctions::stringToModel(
                  m_ui->maximumLikelihoodModel3ComboBox->currentText()),
              m_ui->maximumLikelihoodModel3QuantileLineEdit->text().toDouble());
        }
      }
    }
    // target body
    // ensure user entered something to adjust
    if (m_ui->poleRaCheckBox->isChecked()              ||
        m_ui->poleRaVelocityCheckBox->isChecked()      ||
        m_ui->poleDecCheckBox->isChecked()             ||
        m_ui->poleDecVelocityCheckBox->isChecked()     ||
        m_ui->primeMeridianOffsetCheckBox->isChecked() ||
        m_ui->spinRateCheckBox->isChecked()            ||
        !m_ui->noneRadiiRadioButton->isChecked()) {

      // create BundleTargetBody utility object
      BundleTargetBodyQsp bundleTargetBody = BundleTargetBodyQsp(new BundleTargetBody);

      int radiiOption = 0;
      if (m_ui->meanRadiusRadioButton->isChecked())
        radiiOption = 1;
      else if (m_ui->triaxialRadiiRadioButton->isChecked())
        radiiOption = 2;

      std::set<int> targetParameterSolveCodes;
      if (m_ui->poleRaCheckBox->isChecked())
        targetParameterSolveCodes.insert(BundleTargetBody::PoleRA);
      if (m_ui->poleRaVelocityCheckBox->isChecked())
        targetParameterSolveCodes.insert(BundleTargetBody::VelocityPoleRA);
      if (m_ui->poleDecCheckBox->isChecked())
        targetParameterSolveCodes.insert(BundleTargetBody::PoleDec);
      if (m_ui->poleDecVelocityCheckBox->isChecked())
        targetParameterSolveCodes.insert(BundleTargetBody::VelocityPoleDec);
      if (m_ui->primeMeridianOffsetCheckBox->isChecked())
        targetParameterSolveCodes.insert(BundleTargetBody::PM);
      if (m_ui->spinRateCheckBox->isChecked())
        targetParameterSolveCodes.insert(BundleTargetBody::VelocityPM);
      if (m_ui->triaxialRadiiRadioButton->isChecked()) {
        targetParameterSolveCodes.insert(BundleTargetBody::TriaxialRadiusA);
        targetParameterSolveCodes.insert(BundleTargetBody::TriaxialRadiusB);
        targetParameterSolveCodes.insert(BundleTargetBody::TriaxialRadiusC);
      }
      else if (m_ui->meanRadiusRadioButton->isChecked())
        targetParameterSolveCodes.insert(BundleTargetBody::MeanRadius);

      double poleRASigma              = -1.0;
      double poleRAVelocitySigma      = -1.0;
//    double poleRAAccelerationSigma  = -1.0;
      double poleDecSigma             = -1.0;
      double poleDecVelocitySigma     = -1.0;
//    double poleDecAccelerationSigma = -1.0;
      double pmSigma                  = -1.0;
      double pmVelocitySigma          = -1.0;
//    double pmAccelerationSigma      = -1.0;
      double aRadiusSigma             = 0.0;
      double bRadiusSigma             = 0.0;
      double cRadiusSigma             = 0.0;
      double meanRadiusSigma          = 0.0;

      if (m_ui->rightAscensionSigmaLineEdit->isModified())
        poleRASigma = m_ui->rightAscensionSigmaLineEdit->text().toDouble();
      if (m_ui->rightAscensionVelocityLineEdit->isModified())
        poleRAVelocitySigma = m_ui->rightAscensionVelocityLineEdit->text().toDouble();
//    if (m_ui->rightAscensionAccelerationLineEdit->isModified())
//      poleRAAccelerationSigma = m_ui->rightAscensionAccelerationLineEdit->text().toDouble();
      if (m_ui->declinationSigmaLineEdit->isModified())
        poleDecSigma = m_ui->declinationSigmaLineEdit->text().toDouble();
      if (m_ui->declinationVelocitySigmaLineEdit->isModified())
        poleDecVelocitySigma = m_ui->declinationVelocitySigmaLineEdit->text().toDouble();
//    if (m_ui->declinationAccelerationSigmaLineEdit->isModified())
//      poleDecAccelerationSigma = m_ui->declinationAccelerationSigmaLineEdit->text().toDouble();
      if (m_ui->primeMeridianOffsetSigmaLineEdit->isModified())
        pmSigma = m_ui->primeMeridianOffsetSigmaLineEdit->text().toDouble();
      if (m_ui->spinRateSigmaLineEdit->isModified())
        pmVelocitySigma = m_ui->spinRateSigmaLineEdit->text().toDouble();
//    if (m_ui->pmAccelerationSigmaLineEdit->isModified())
//      pmAccelerationSigma = m_ui->pmAccelerationSigmaLineEdit->text().toDouble();
      if (m_ui->aRadiusSigmaLineEdit->isModified())
        aRadiusSigma = m_ui->aRadiusSigmaLineEdit->text().toDouble();
      if (m_ui->bRadiusSigmaLineEdit->isModified())
        bRadiusSigma = m_ui->bRadiusSigmaLineEdit->text().toDouble();
      if (m_ui->cRadiusSigmaLineEdit->isModified())
        cRadiusSigma = m_ui->cRadiusSigmaLineEdit->text().toDouble();
      if (m_ui->meanRadiusSigmaLineEdit->isModified())
        meanRadiusSigma = m_ui->meanRadiusSigmaLineEdit->text().toDouble();

      bundleTargetBody->setSolveSettings(targetParameterSolveCodes,
           Angle(m_ui->rightAscensionLineEdit->text().toDouble(), Angle::Degrees),
           Angle(poleRASigma, Angle::Degrees),
           Angle(m_ui->rightAscensionVelocityLineEdit->text().toDouble(), Angle::Degrees),
           Angle(poleRAVelocitySigma, Angle::Degrees),
           Angle(m_ui->declinationLineEdit->text().toDouble(), Angle::Degrees),
           Angle(poleDecSigma, Angle::Degrees),
           Angle(m_ui->declinationVelocityLineEdit->text().toDouble(), Angle::Degrees),
           Angle(poleDecVelocitySigma, Angle::Degrees),
           Angle(m_ui->primeMeridianOffsetLineEdit->text().toDouble(), Angle::Degrees),
           Angle(pmSigma, Angle::Degrees),
           Angle(m_ui->spinRateLineEdit->text().toDouble(), Angle::Degrees),
           Angle(pmVelocitySigma, Angle::Degrees),
           (BundleTargetBody::TargetRadiiSolveMethod)radiiOption,
           Distance(m_ui->aRadiusLineEdit->text().toDouble(), Distance::Kilometers),
           Distance(aRadiusSigma, Distance::Meters),
           Distance(m_ui->bRadiusLineEdit->text().toDouble(), Distance::Kilometers),
           Distance(bRadiusSigma, Distance::Meters),
           Distance(m_ui->cRadiusLineEdit->text().toDouble(), Distance::Kilometers),
           Distance(cRadiusSigma, Distance::Meters),
           Distance(m_ui->meanRadiusLineEdit->text().toDouble(), Distance::Kilometers),
           Distance(meanRadiusSigma, Distance::Meters));

      settings->setBundleTargetBody(bundleTargetBody);
    }
    return settings;
  }


  /**
   * Loads the passed bundle settings into the setup dialog. This is used by JigsawDialog to
   * load its current settings when not using the last (most recent) bundle settings in the project.
   *
   * @param const BundleSettingsQsp settings Shared pointer to the settings to load up.
   */
  void JigsawSetupDialog::loadSettings(const BundleSettingsQsp settings) {
    fillFromSettings(settings);
  }


  /**
   * Selects a control in the control network combo box by trying to find an item with the
   * matching name. If the name is found in the combo box, the box's index is set to that
   * found control network index. If the name is not found and the box is not empty, the
   * current index is set to 0 (the first item). If the name is not found and the box is
   * empty, the index is set to -1 (see Qt).
   *
   * @param const QString &controlName The name of the control to try to find in the combo box.
   */
  void JigsawSetupDialog::selectControl(const QString &controlName) {
    QComboBox &cnetBox = *(m_ui->inputControlNetCombo);
    int foundControlIndex = cnetBox.findText(QString::fromStdString(FileName(controlName.toStdString()).name()));
    // We did not find it, so we need to see if the combo box is empty or not.
    if (foundControlIndex == -1) {
      if (cnetBox.count() == 0) {
       cnetBox.setCurrentIndex(-1);
      }
      // If it is not empty, just set the current index to the first item.
      else {
        cnetBox.setCurrentIndex(0);
      }
    }
    // Otherwise, set the current index to the found control net index.
    else {
      cnetBox.setCurrentIndex(foundControlIndex);
    }
  }


  Control *JigsawSetupDialog::selectedControl() {

      int nIndex = m_ui->inputControlNetCombo->currentIndex();
      Control *selectedControl
                   = (Control *)(m_ui->inputControlNetCombo->itemData(nIndex).value< void * >());
      return selectedControl;

  }


  QString JigsawSetupDialog::selectedControlName() {
    return QString(m_ui->inputControlNetCombo->currentText());
  }

  

  /**
   * @brief updateBundleObservationSolveSettings:  This function sets the parameters of
   * a BundleObservationSolveSettings object by reading user settings from the BOSS
   * tab, as well as reading in the selected images on the BOSS QTreeView these
   * BOSS settings are to be applied to.
   * @param BundleObservationSolveSettings &boss The object which stores user-specified
   * settings from the BOSS tab.
   */
  void JigsawSetupDialog::updateBundleObservationSolveSettings(BundleObservationSolveSettings &boss)
    {

      int ckSolveDegree = m_ui->ckSolveDegreeSpinBox->value();
      int spkSolveDegree = m_ui->spkSolveDegreeSpinBox->value();
      int ckDegree = m_ui->ckDegreeSpinBox->value();
      int spkDegree = m_ui->spkDegreeSpinBox->value();
      int instrumentPointingSolveOption=m_ui->pointingComboBox->currentIndex();
      int instrumentPositionSolveOption=m_ui->positionComboBox->currentIndex();

      BundleObservationSolveSettings::InstrumentPointingSolveOption  pointSolvingOption;
      BundleObservationSolveSettings::InstrumentPositionSolveOption  positionSolvingOption;

      double anglesAprioriSigma(-1.0);
      double angularVelocityAprioriSigma(-1.0);
      double angularAccelerationAprioriSigma(-1.0);
      QList<double> additionalAngularCoefficients;

      double positionAprioriSigma(-1.0);
      double velocityAprioriSigma(-1.0);
      double accelerationAprioriSigma(-1.0);
      QList<double> additionalPositionCoefficients;

      bool solveTwist(false);
      bool solvePolynomialOverExisting(false);
      bool positionOverHermite(false);

      if (m_ui->pointingAprioriSigmaTable->item(0,3))
        anglesAprioriSigma = m_ui->pointingAprioriSigmaTable->item(0,3)->data(0).toDouble();

      if (m_ui->pointingAprioriSigmaTable->item(1,3))
        angularVelocityAprioriSigma = m_ui->pointingAprioriSigmaTable->item(1,3)->data(0).toDouble();

      if (m_ui->pointingAprioriSigmaTable->item(2,3) )
        angularAccelerationAprioriSigma = m_ui->pointingAprioriSigmaTable->item(2,3)->data(0).toDouble();

      if (m_ui->positionAprioriSigmaTable->item(0,3))
        positionAprioriSigma = m_ui->positionAprioriSigmaTable->item(0,3)->data(0).toDouble();

      if (m_ui->positionAprioriSigmaTable->item(1,3))
        velocityAprioriSigma = m_ui->positionAprioriSigmaTable->item(1,3)->data(0).toDouble();

      if (m_ui->positionAprioriSigmaTable->item(2,3) )
        accelerationAprioriSigma = m_ui->positionAprioriSigmaTable->item(2,3)->data(0).toDouble();

      //Saving additionalPositional/Angular coefficients (deg >=3) in case they are needed
      //later.
      if (spkSolveDegree >2) {
        for (int i = 3;i <= spkSolveDegree;i++ ) {
          if (m_ui->positionAprioriSigmaTable->item(i,3))
            additionalPositionCoefficients.append(m_ui->positionAprioriSigmaTable->item(i,3)->data(0).toDouble() );
        }
      }

      if (ckSolveDegree > 2) {
         for (int i = 3;i <= ckSolveDegree;i++ ) {
           if (m_ui->pointingAprioriSigmaTable->item(i,3))
             additionalAngularCoefficients.append(m_ui->pointingAprioriSigmaTable->item(i,3)->data(0).toDouble() );
         }

      }


      if (m_ui->twistCheckBox->checkState() == Qt::Checked)
        solveTwist = true;
      if (m_ui->fitOverPointingCheckBox->checkState() == Qt::Checked)
        solvePolynomialOverExisting = true;
      if (m_ui->hermiteSplineCheckBox->checkState() == Qt::Checked)
        positionOverHermite = true;

      switch(instrumentPointingSolveOption) {

      case 0: pointSolvingOption = BundleObservationSolveSettings::InstrumentPointingSolveOption::NoPointingFactors;
        break;
      case 1:pointSolvingOption = BundleObservationSolveSettings::InstrumentPointingSolveOption::AnglesOnly;
        break;
      case 2:pointSolvingOption = BundleObservationSolveSettings::InstrumentPointingSolveOption::AnglesVelocity;
        break;
      case 3:pointSolvingOption = BundleObservationSolveSettings::InstrumentPointingSolveOption::AnglesVelocityAcceleration;
        break;

      case 4:pointSolvingOption = BundleObservationSolveSettings::InstrumentPointingSolveOption::AllPointingCoefficients;
        break;
      default:  pointSolvingOption = BundleObservationSolveSettings::InstrumentPointingSolveOption::NoPointingFactors;
        break;

      }

      switch(instrumentPositionSolveOption) {

      case 0: positionSolvingOption = BundleObservationSolveSettings::InstrumentPositionSolveOption::NoPositionFactors;
        break;
      case 1:positionSolvingOption = BundleObservationSolveSettings::InstrumentPositionSolveOption::PositionOnly;
        break;
      case 2:positionSolvingOption = BundleObservationSolveSettings::InstrumentPositionSolveOption::PositionVelocity;
        break;
      case 3:positionSolvingOption = BundleObservationSolveSettings::InstrumentPositionSolveOption::PositionVelocityAcceleration;
        break;

      case 4:positionSolvingOption = BundleObservationSolveSettings::InstrumentPositionSolveOption::AllPositionCoefficients;
        break;
      default:  positionSolvingOption = BundleObservationSolveSettings::InstrumentPositionSolveOption::NoPositionFactors;
        break;

      }


      boss.setInstrumentPositionSettings(positionSolvingOption,spkDegree,spkSolveDegree,positionOverHermite,
                                         positionAprioriSigma,velocityAprioriSigma,accelerationAprioriSigma,
                                         &additionalPositionCoefficients);

      boss.setInstrumentPointingSettings(pointSolvingOption,solveTwist,ckDegree,ckSolveDegree,solvePolynomialOverExisting,
                                         anglesAprioriSigma,angularVelocityAprioriSigma,angularAccelerationAprioriSigma,
                                         &additionalAngularCoefficients);

      //What if multiple instrument IDs are represented?
      boss.setInstrumentId("");


      SortFilterProxyModel* proxyModel = (SortFilterProxyModel *)m_ui->treeView->model();
      ProjectItemModel* sourceModel = (ProjectItemModel *)proxyModel->sourceModel();
      QModelIndexList selectedIndexes = m_ui->treeView->selectionModel()->selectedIndexes();

      foreach (QModelIndex index, selectedIndexes) {

        QModelIndex sourceix = proxyModel->mapToSource(index);
        ProjectItem * projItem = sourceModel->itemFromIndex(sourceix);

        if (projItem) {

          if (projItem->isImage() ) {
            Image * img = projItem->data().value<Image *>();
            boss.addObservationNumber(img->observationNumber() );

          }
        }

      }

    }


  QString JigsawSetupDialog::outputControlName() {
    return QString(m_ui->outputControlNetLineEdit->text());
  }


  void JigsawSetupDialog::makeReadOnly() {
    m_ui->inputControlNetCombo->setEnabled(false);
    m_ui->observationModeCheckBox->setEnabled(false);
    m_ui->pointRadiusSigmaCheckBox->setEnabled(false);
    // m_ui->updateCubeLabelCheckBox->setEnabled(false);
    m_ui->errorPropagationCheckBox->setEnabled(false);
    m_ui->outlierRejectionCheckBox->setEnabled(false);
    m_ui->outlierRejectionMultiplierLineEdit->setEnabled(false);
    m_ui->sigma0ThresholdLineEdit->setEnabled(false);
    m_ui->maximumIterationsLineEdit->setEnabled(false);
    // m_ui->positionComboBox->setEnabled(false);
    m_ui->hermiteSplineCheckBox->setEnabled(false);
    m_ui->spkDegreeSpinBox->setEnabled(false);
    m_ui->spkSolveDegreeSpinBox->setEnabled(false);
    m_ui->twistCheckBox->setEnabled(false);
    // m_ui->pointingComboBox->setEnabled(false);
    m_ui->fitOverPointingCheckBox->setEnabled(false);
    m_ui->ckDegreeSpinBox->setEnabled(false);
    m_ui->ckSolveDegreeSpinBox->setEnabled(false);

    // observation solve settings tab
    m_ui->treeView->setEnabled(false);
    m_ui->positionComboBox->setEnabled(false);
    m_ui->spkSolveDegreeSpinBox->setEnabled(false);
    m_ui->spkDegreeSpinBox->setEnabled(false);
    m_ui->hermiteSplineCheckBox->setEnabled(false);
    m_ui->positionAprioriSigmaTable->setEnabled(false);
    m_ui->pointingComboBox->setEnabled(false);
    m_ui->ckSolveDegreeSpinBox->setEnabled(false);
    m_ui->ckDegreeSpinBox->setEnabled(false);
    m_ui->twistCheckBox->setEnabled(false);
    m_ui->fitOverPointingCheckBox->setEnabled(false);
    m_ui->pointingAprioriSigmaTable->setEnabled(false);
    m_ui->applySettingsPushButton->setEnabled(false);

    // target body tab
    m_ui->targetBodyComboBox->setEnabled(false);
    m_ui->poleRaCheckBox->setEnabled(false);
    m_ui->rightAscensionLineEdit->setEnabled(false);
    m_ui->rightAscensionSigmaLineEdit->setEnabled(false);
    m_ui->rightAscensionVelocityLineEdit->setEnabled(false);
    m_ui->rightAscensionVelocitySigmaLineEdit->setEnabled(false);
    m_ui->poleDecCheckBox->setEnabled(false);
    m_ui->declinationLineEdit->setEnabled(false);
    m_ui->declinationSigmaLineEdit->setEnabled(false);
    m_ui->declinationVelocityLineEdit->setEnabled(false);
    m_ui->declinationVelocitySigmaLineEdit->setEnabled(false);
    m_ui->primeMeridianOffsetCheckBox->setEnabled(false);
    m_ui->primeMeridianOffsetLineEdit->setEnabled(false);
    m_ui->primeMeridianOffsetSigmaLineEdit->setEnabled(false);
    m_ui->spinRateCheckBox->setEnabled(false);
    m_ui->spinRateLineEdit->setEnabled(false);
    m_ui->spinRateSigmaLineEdit->setEnabled(false);
    m_ui->radiiGroupBox->setEnabled(false);
    m_ui->aRadiusLineEdit->setEnabled(false);
    m_ui->aRadiusSigmaLineEdit->setEnabled(false);
    m_ui->bRadiusLineEdit->setEnabled(false);
    m_ui->bRadiusSigmaLineEdit->setEnabled(false);
    m_ui->cRadiusLineEdit->setEnabled(false);
    m_ui->cRadiusSigmaLineEdit->setEnabled(false);
    m_ui->meanRadiusLineEdit->setEnabled(false);
    m_ui->meanRadiusSigmaLineEdit->setEnabled(false);

    update();
  }


  void Isis::JigsawSetupDialog::on_poleRaCheckBox_stateChanged(int arg1) {
    if (arg1) {
      m_ui->rightAscensionLineEdit->setEnabled(true);
      m_ui->rightAscensionSigmaLineEdit->setEnabled(true);
    }
    else {
      m_ui->rightAscensionLineEdit->setEnabled(false);
      m_ui->rightAscensionSigmaLineEdit->setEnabled(false);
    }

    update();
  }


  void Isis::JigsawSetupDialog::on_poleRaVelocityCheckBox_stateChanged(int arg1) {
    if (arg1) {
      m_ui->rightAscensionVelocityLineEdit->setEnabled(true);
      m_ui->rightAscensionVelocitySigmaLineEdit->setEnabled(true);
    }
    else {
      m_ui->rightAscensionVelocityLineEdit->setEnabled(false);
      m_ui->rightAscensionVelocitySigmaLineEdit->setEnabled(false);
    }

    update();
  }


  void Isis::JigsawSetupDialog::on_poleDecCheckBox_stateChanged(int arg1) {
    if (arg1) {
      m_ui->declinationLineEdit->setEnabled(true);
      m_ui->declinationSigmaLineEdit->setEnabled(true);
    }
    else {
      m_ui->declinationLineEdit->setEnabled(false);
      m_ui->declinationSigmaLineEdit->setEnabled(false);
    }

    update();
  }


  void Isis::JigsawSetupDialog::on_poleDecVelocityCheckBox_stateChanged(int arg1) {
    if (arg1) {
      m_ui->declinationVelocityLineEdit->setEnabled(true);
      m_ui->declinationVelocitySigmaLineEdit->setEnabled(true);
    }
    else {
      m_ui->declinationVelocityLineEdit->setEnabled(false);
      m_ui->declinationVelocitySigmaLineEdit->setEnabled(false);
    }

    update();
  }


  void Isis::JigsawSetupDialog::on_spinRateCheckBox_stateChanged(int arg1) {
    if (arg1) {
      m_ui->spinRateLineEdit->setEnabled(true);
      m_ui->spinRateSigmaLineEdit->setEnabled(true);
    }
    else {
      m_ui->spinRateLineEdit->setEnabled(false);
      m_ui->spinRateSigmaLineEdit->setEnabled(false);
    }

    update();
  }


  void Isis::JigsawSetupDialog::on_primeMeridianOffsetCheckBox_stateChanged(int arg1) {
    if (arg1) {
      m_ui->primeMeridianOffsetLineEdit->setEnabled(true);
      m_ui->primeMeridianOffsetSigmaLineEdit->setEnabled(true);
    }
    else {
      m_ui->primeMeridianOffsetLineEdit->setEnabled(false);
      m_ui->primeMeridianOffsetSigmaLineEdit->setEnabled(false);
    }

    update();
  }


  void Isis::JigsawSetupDialog::on_radiiButtonGroupClicked(int arg1) {

    if (arg1 == 0) {
      m_ui->aRadiusLabel->setEnabled(false);
      m_ui->aRadiusLineEdit->setEnabled(false);
      m_ui->aRadiusSigmaLineEdit->setEnabled(false);
      m_ui->bRadiusLabel->setEnabled(false);
      m_ui->bRadiusLineEdit->setEnabled(false);
      m_ui->bRadiusSigmaLineEdit->setEnabled(false);
      m_ui->cRadiusLabel->setEnabled(false);
      m_ui->cRadiusLineEdit->setEnabled(false);
      m_ui->cRadiusSigmaLineEdit->setEnabled(false);
      m_ui->meanRadiusLineEdit->setEnabled(false);
      m_ui->meanRadiusSigmaLineEdit->setEnabled(false);

      // if we're not solving for target body triaxial radii or mean radius, we CAN solve for point
      // radii so we ensure the point radius checkbox under the general settings tab is enabled
      m_ui->pointRadiusSigmaCheckBox->setEnabled(true);
    }
    else if (arg1 == 1) {
      m_ui->aRadiusLabel->setEnabled(true);
      m_ui->aRadiusLineEdit->setEnabled(true);
      m_ui->aRadiusSigmaLineEdit->setEnabled(true);
      m_ui->bRadiusLabel->setEnabled(true);
      m_ui->bRadiusLineEdit->setEnabled(true);
      m_ui->bRadiusSigmaLineEdit->setEnabled(true);
      m_ui->cRadiusLabel->setEnabled(true);
      m_ui->cRadiusLineEdit->setEnabled(true);
      m_ui->cRadiusSigmaLineEdit->setEnabled(true);
      m_ui->meanRadiusLineEdit->setEnabled(false);
      m_ui->meanRadiusSigmaLineEdit->setEnabled(false);

      // if we're solving for target body mean radius, we can't solve for point radii
      // so we uncheck and disable the point radius checkbox under the general settings tab
      // and remind the user
      m_ui->pointRadiusSigmaCheckBox->setChecked(false);
      m_ui->pointRadiusSigmaCheckBox->setEnabled(false);

      QMessageBox *msgBox = new QMessageBox(QMessageBox::Information, "Triaxial Radii Reminder!",
                  "Individual point radii and target body triaxial radii can't be solved for"
                  " simultaneously so we've unchecked and disabled the Radius checkbox under the"
                  " General Settings tab.", QMessageBox::Ok, this);
      msgBox->exec();
    }
    else if (arg1 == 2) {
      m_ui->aRadiusLabel->setEnabled(false);
      m_ui->aRadiusLineEdit->setEnabled(false);
      m_ui->aRadiusSigmaLineEdit->setEnabled(false);
      m_ui->bRadiusLabel->setEnabled(false);
      m_ui->bRadiusLineEdit->setEnabled(false);
      m_ui->bRadiusSigmaLineEdit->setEnabled(false);
      m_ui->cRadiusLabel->setEnabled(false);
      m_ui->cRadiusLineEdit->setEnabled(false);
      m_ui->cRadiusSigmaLineEdit->setEnabled(false);
      m_ui->meanRadiusLineEdit->setEnabled(true);
      m_ui->meanRadiusSigmaLineEdit->setEnabled((true));

      // if we're solving for target body triaxial radii, we can't solve for point radii
      // so we uncheck and disable the point radius checkbox under the general settings tab
      // and remind the user
      m_ui->pointRadiusSigmaCheckBox->setChecked(false);
      m_ui->pointRadiusSigmaCheckBox->setEnabled(false);

      QMessageBox *msgBox = new QMessageBox(QMessageBox::Information, "Mean Radius Reminder!",
                  "Individual point radii and target body mean radius can't be solved for"
                  " simultaneously so we've unchecked and disabled the Radius checkbox under the"
                  " General Settings tab.", QMessageBox::Ok, this);
      msgBox->exec();
    }

    update();
  }


  void Isis::JigsawSetupDialog::checkIsValid() {
    if (!m_ui->aRadiusSigmaLineEdit->hasAcceptableInput()) {
//    qDebug() << "invalid input";
    }


  }


  void Isis::JigsawSetupDialog::slotTextChanged(const QString &text) {
//  qDebug() << "Text change to" << text << "and value is valid ==" << m_ui->aRadiusSigmaLineEdit->hasAcceptableInput();
  }


  void Isis::JigsawSetupDialog::on_aRadiusLineEdit_textChanged(const QString &arg1) {
//  qDebug() << "Text change to" << arg1 << "and value is valid ==" << m_ui->aRadiusLineEdit->hasAcceptableInput();
  }


  void Isis::JigsawSetupDialog::on_targetBodyComboBox_currentIndexChanged(int index) {

    TargetBodyQsp target = m_project->targetBodies().at(index);
    if (target) {
      if (target->frameType() != Isis::SpiceRotation::BPC &&
          target->frameType() != Isis::SpiceRotation::UNKNOWN) {
        m_ui->targetParametersMessage->hide();

        std::vector<Angle> raCoefs = target->poleRaCoefs();
        std::vector<Angle> decCoefs = target->poleDecCoefs();
        std::vector<Angle> pmCoefs = target->pmCoefs();

        showTargetParametersGroupBox();

        m_ui->rightAscensionLineEdit->setText(QString::number(raCoefs[0].degrees()));
        m_ui->rightAscensionVelocityLineEdit->setText(QString::number(raCoefs[1].degrees()));
        m_ui->declinationLineEdit->setText(QString::number(decCoefs[0].degrees()));
        m_ui->declinationVelocityLineEdit->setText(QString::number(decCoefs[1].degrees()));
        m_ui->primeMeridianOffsetLineEdit->setText(QString::number(pmCoefs[0].degrees()));
        m_ui->spinRateLineEdit->setText(QString::number(pmCoefs[1].degrees()));
      }
      else {
        // Formulate message indicating why target parameters are disabled
        QString msg;
        if (target->displayProperties()->displayName() == "MOON") {
          msg = "Target body parameter cannot be solved for the Moon.";
        }
        else {
          msg = "The body frame type is unknown.  If you want to solve the target body parameters, "
                "you must run spiceinit on the cubes.";
        }
        m_ui->targetParametersMessage->setText(msg);
        m_ui->targetParametersMessage->show();
        hideTargetParametersGroupBox();
      }

      m_ui->aRadiusLineEdit->setText(QString::number(target->radiusA().kilometers()));
      //m_ui->aRadiusSigmaLineEdit->setText(QString::number(target->sigmaRadiusA().kilometers()));

      m_ui->bRadiusLineEdit->setText(QString::number(target->radiusB().kilometers()));
      //m_ui->bRadiusSigmaLineEdit->setText(QString::number(target->sigmaRadiusB().kilometers()));

      m_ui->cRadiusLineEdit->setText(QString::number(target->radiusC().kilometers()));
      //m_ui->cRadiusSigmaLineEdit->setText(QString::number(target->sigmaRadiusC().kilometers()));

      m_ui->meanRadiusLineEdit->setText(QString::number(target->meanRadius().kilometers()));
      //m_ui->meanRadiusSigmaLineEdit->setText(QString::number(target->sigmaMeanRadius().kilometers()));
    }
  }


  void Isis::JigsawSetupDialog::on_rightAscensionLineEdit_textChanged(const QString &arg1) {
    if (!m_ui->rightAscensionLineEdit->hasAcceptableInput()) {
      m_ui->rightAscensionLineEdit->setStyleSheet("QLineEdit { background-color: red }");
      m_ui->okCloseButtonBox->button(QDialogButtonBox::Ok)->setEnabled(false);
    }
    else {
      m_ui->rightAscensionLineEdit->setStyleSheet("QLineEdit { background-color: white }");
      m_ui->okCloseButtonBox->button(QDialogButtonBox::Ok)->setEnabled(true);
    }
    update();
  }


  void Isis::JigsawSetupDialog::on_declinationLineEdit_textChanged(const QString &arg1) {
    if (!m_ui->declinationLineEdit->hasAcceptableInput()) {
      m_ui->declinationLineEdit->setStyleSheet("QLineEdit { background-color: red }");
      m_ui->okCloseButtonBox->button(QDialogButtonBox::Ok)->setEnabled(false);
    }
    else {
      m_ui->declinationLineEdit->setStyleSheet("QLineEdit { background-color: white }");
      m_ui->okCloseButtonBox->button(QDialogButtonBox::Ok)->setEnabled(true);
    }
    update();
  }


  void Isis::JigsawSetupDialog::on_rightAscensionVelocityLineEdit_textChanged(const QString &arg1) {
    if (!m_ui->rightAscensionVelocityLineEdit->hasAcceptableInput()) {
      m_ui->rightAscensionVelocityLineEdit->setStyleSheet("QLineEdit { background-color: red }");
      m_ui->okCloseButtonBox->button(QDialogButtonBox::Ok)->setEnabled(false);
    }
    else {
      m_ui->rightAscensionVelocityLineEdit->setStyleSheet("QLineEdit { background-color: white }");
      m_ui->okCloseButtonBox->button(QDialogButtonBox::Ok)->setEnabled(true);
    }
    update();
  }


  void Isis::JigsawSetupDialog::on_declinationVelocityLineEdit_textChanged(const QString &arg1) {
    if (!m_ui->declinationVelocityLineEdit->hasAcceptableInput()) {
      m_ui->declinationVelocityLineEdit->setStyleSheet("QLineEdit { background-color: red }");
      m_ui->okCloseButtonBox->button(QDialogButtonBox::Ok)->setEnabled(false);
    }
    else {
      m_ui->declinationVelocityLineEdit->setStyleSheet("QLineEdit { background-color: white }");
      m_ui->okCloseButtonBox->button(QDialogButtonBox::Ok)->setEnabled(true);
    }
    update();
  }


  void Isis::JigsawSetupDialog::on_primeMeridianOffsetLineEdit_textChanged(const QString &arg1) {
    if (!m_ui->primeMeridianOffsetLineEdit->hasAcceptableInput()) {
      m_ui->primeMeridianOffsetLineEdit->setStyleSheet("QLineEdit { background-color: red }");
      m_ui->okCloseButtonBox->button(QDialogButtonBox::Ok)->setEnabled(false);
    }
    else {
      m_ui->primeMeridianOffsetLineEdit->setStyleSheet("QLineEdit { background-color: white }");
      m_ui->okCloseButtonBox->button(QDialogButtonBox::Ok)->setEnabled(true);
    }
    update();
  }


  void Isis::JigsawSetupDialog::on_spinRateLineEdit_textChanged(const QString &arg1) {
    if (!m_ui->spinRateLineEdit->hasAcceptableInput()) {
      m_ui->spinRateLineEdit->setStyleSheet("QLineEdit { background-color: red }");
      m_ui->okCloseButtonBox->button(QDialogButtonBox::Ok)->setEnabled(false);
    }
    else {
      m_ui->spinRateLineEdit->setStyleSheet("QLineEdit { background-color: white }");
      m_ui->okCloseButtonBox->button(QDialogButtonBox::Ok)->setEnabled(true);
    }
    update();
  }

  // general tab text validation
  // global apriori point sigmas
  void Isis::JigsawSetupDialog::on_pointLatitudeSigmaLineEdit_textChanged(const QString &arg1) {
    if (arg1 == "" || m_ui->pointLatitudeSigmaLineEdit->hasAcceptableInput()) {
      m_ui->pointLatitudeSigmaLineEdit->setStyleSheet("QLineEdit { background-color: white }");
      m_ui->okCloseButtonBox->button(QDialogButtonBox::Ok)->setEnabled(true);
    }
    else {
      m_ui->pointLatitudeSigmaLineEdit->setStyleSheet("QLineEdit { background-color: red }");
      m_ui->okCloseButtonBox->button(QDialogButtonBox::Ok)->setEnabled(false);
    }
    update();
  }

  void Isis::JigsawSetupDialog::on_pointLongitudeSigmaLineEdit_textChanged(const QString &arg1) {
    if (arg1 == "" || m_ui->pointLongitudeSigmaLineEdit->hasAcceptableInput()) {
      m_ui->pointLongitudeSigmaLineEdit->setStyleSheet("QLineEdit { background-color: white }");
      m_ui->okCloseButtonBox->button(QDialogButtonBox::Ok)->setEnabled(true);
    }
    else {
      m_ui->pointLongitudeSigmaLineEdit->setStyleSheet("QLineEdit { background-color: red }");
      m_ui->okCloseButtonBox->button(QDialogButtonBox::Ok)->setEnabled(false);
    }
    update();
  }


  void Isis::JigsawSetupDialog::on_pointRadiusSigmaLineEdit_textChanged(const QString &arg1) {
    if (arg1 == "" || m_ui->pointRadiusSigmaLineEdit->hasAcceptableInput()) {
      m_ui->pointRadiusSigmaLineEdit->setStyleSheet("QLineEdit { background-color: white }");
      m_ui->okCloseButtonBox->button(QDialogButtonBox::Ok)->setEnabled(true);
    }
    else {
      m_ui->pointRadiusSigmaLineEdit->setStyleSheet("QLineEdit { background-color: red }");
      m_ui->okCloseButtonBox->button(QDialogButtonBox::Ok)->setEnabled(false);
    }
    update();
  }

  // outlier rejection
  void Isis::JigsawSetupDialog::on_maximumLikelihoodModel1QuantileLineEdit_textChanged(const QString &arg1) {
    if (!m_ui->maximumLikelihoodModel1QuantileLineEdit->isEnabled() ||
        m_ui->maximumLikelihoodModel1QuantileLineEdit->hasAcceptableInput()) {
      m_ui->maximumLikelihoodModel1QuantileLineEdit->setStyleSheet("");
      m_ui->okCloseButtonBox->button(QDialogButtonBox::Ok)->setEnabled(true);
    }
    else {
      m_ui->maximumLikelihoodModel1QuantileLineEdit->setStyleSheet("QLineEdit { background-color: red }");
      m_ui->okCloseButtonBox->button(QDialogButtonBox::Ok)->setEnabled(false);
    }
    update();
  }


  void Isis::JigsawSetupDialog::on_maximumLikelihoodModel2QuantileLineEdit_textChanged(const QString &arg1) {
    if (!m_ui->maximumLikelihoodModel2QuantileLineEdit->isEnabled() ||
        m_ui->maximumLikelihoodModel2QuantileLineEdit->hasAcceptableInput()) {
      m_ui->maximumLikelihoodModel2QuantileLineEdit->setStyleSheet("");
      m_ui->okCloseButtonBox->button(QDialogButtonBox::Ok)->setEnabled(true);
    }
    else {
      m_ui->maximumLikelihoodModel2QuantileLineEdit->setStyleSheet("QLineEdit { background-color: red }");
      m_ui->okCloseButtonBox->button(QDialogButtonBox::Ok)->setEnabled(false);
    }
    update();
  }


  void Isis::JigsawSetupDialog::on_maximumLikelihoodModel3QuantileLineEdit_textChanged(const QString &arg1) {
    if (!m_ui->maximumLikelihoodModel3QuantileLineEdit->isEnabled() ||
        m_ui->maximumLikelihoodModel3QuantileLineEdit->hasAcceptableInput()) {
      m_ui->maximumLikelihoodModel3QuantileLineEdit->setStyleSheet("");
      m_ui->okCloseButtonBox->button(QDialogButtonBox::Ok)->setEnabled(true);
    }
    else {
      m_ui->maximumLikelihoodModel3QuantileLineEdit->setStyleSheet("QLineEdit { background-color: red }");
      m_ui->okCloseButtonBox->button(QDialogButtonBox::Ok)->setEnabled(false);
    }
    update();
  }


  // convergence criteria
  void Isis::JigsawSetupDialog::on_outlierRejectionMultiplierLineEdit_textChanged(const QString &arg1) {
    if (!m_ui->outlierRejectionCheckBox->isChecked() || 
        m_ui->outlierRejectionMultiplierLineEdit->hasAcceptableInput()) {
      m_ui->outlierRejectionMultiplierLineEdit->setStyleSheet("");
      m_ui->okCloseButtonBox->button(QDialogButtonBox::Ok)->setEnabled(true);
    }
    else {
      m_ui->outlierRejectionMultiplierLineEdit->setStyleSheet("QLineEdit { background-color: red }");
      m_ui->okCloseButtonBox->button(QDialogButtonBox::Ok)->setEnabled(false);
    }
    update();
  }


  void Isis::JigsawSetupDialog::on_sigma0ThresholdLineEdit_textChanged(const QString &arg1) {
    if (m_ui->sigma0ThresholdLineEdit->hasAcceptableInput()) {
      m_ui->sigma0ThresholdLineEdit->setStyleSheet("QLineEdit { background-color: white }");
      m_ui->okCloseButtonBox->button(QDialogButtonBox::Ok)->setEnabled(true);
    }
    else {
      m_ui->sigma0ThresholdLineEdit->setStyleSheet("QLineEdit { background-color: red }");
      m_ui->okCloseButtonBox->button(QDialogButtonBox::Ok)->setEnabled(false);
    }
    update();
  }


  void Isis::JigsawSetupDialog::on_maximumIterationsLineEdit_textChanged(const QString &arg1) {
    if (m_ui->maximumIterationsLineEdit->hasAcceptableInput()) {
      m_ui->maximumIterationsLineEdit->setStyleSheet("QLineEdit { background-color: white }");
      m_ui->okCloseButtonBox->button(QDialogButtonBox::Ok)->setEnabled(true);
    }
    else {
      m_ui->maximumIterationsLineEdit->setStyleSheet("QLineEdit { background-color: red }");
      m_ui->okCloseButtonBox->button(QDialogButtonBox::Ok)->setEnabled(false);
    }
    update();
  }


  void JigsawSetupDialog::showTargetParametersGroupBox() {
    m_ui->targetParametersGroupBox->setEnabled(true);
  }


  void JigsawSetupDialog::hideTargetParametersGroupBox() {
    m_ui->targetParametersGroupBox->setEnabled(false);
  }

  void Isis::JigsawSetupDialog::on_inputControlNetCombo_currentTextChanged(const QString &arg1) {
    FileName fname = arg1.toStdString();
    m_ui->outputControlNetLineEdit->setText(QString::fromStdString(fname.baseName()) + "-out.net");
  }


  /**
   * Validates the a priori sigma values for a given QTableWidgetItem.
   *
   * This will only validate items in the a priori sigma column. Valid a priori sigma values are
   * "FREE" or any positive double. Items that are invalid will be marked with a red background
   * color.
   *
   * @param QTableWidgetItem* Pointer to the QTableWidgetItem to be validated.
   */
  void JigsawSetupDialog::validateSigmaValue(QTableWidgetItem *item) {
    // Only validate the "a priori sigma" column
    if (item->column() != 3) {
      return;
    }
    // FREE is a valid value for the a priori sigma column
    int free = item->text().simplified().compare("FREE", Qt::CaseInsensitive);

    // Positive doubles are valid values for the a priori sigma column
    bool convertSuccess = false;
    double sigma = item->text().toDouble(&convertSuccess);
    if ((convertSuccess && sigma >= 0.0) || free == 0) {
      const QTableWidget *table = item->tableWidget();
      item->setData(Qt::UserRole, QVariant(true));
      // Set background color depending on if the table has alternating row colors and row is odd
      if (table->alternatingRowColors() && item->row() % 2 != 0) {
        item->setBackground(table->palette().color(QPalette::AlternateBase));
      }
      else {
        item->setBackground(table->palette().color(QPalette::Base));
      }

      if (sigma == 0.0) {
        item->setText("FREE");
      }
    }
    else {
      item->setData(Qt::UserRole, QVariant(false));
      item->setBackground(Qt::red);
    }

    validateSigmaTables();
  }


  /**
   * Validates the tables in the observation solve settings tab and enables/disables the OK
   * button appropriately.
   *
   * This method validates both the position and pointing a priori sigma tables in the observation
   * solve settings tab. If any of the sigma values are invalid, the JigsawSetupDialog's OK button
   * is disabled. If all of the sigma values are valid, the JigsawSetupDialog's OK button is
   * (re)enabled.
   */
  void JigsawSetupDialog::validateSigmaTables() {
    bool tablesAreValid = true;
    
    // Evaluate both tables; if any value is invalid, the table is invalid
    QList<const QTableWidget *> tables{m_ui->positionAprioriSigmaTable,
                                       m_ui->pointingAprioriSigmaTable};
                                       
    for (const auto &table : tables) {
      for (int i = 0; i < table->rowCount(); i++) {
        // a priori sigma column is column 3
        const QTableWidgetItem *item = table->item(i,3);
        if (item) { 
          if (item->data(Qt::UserRole).toBool() == false) {
            tablesAreValid = false;
            break;
          }
        }
      }
    }

    m_ui->okCloseButtonBox->button(QDialogButtonBox::Ok)->setEnabled(tablesAreValid);
    if (!m_ui->treeView->selectionModel()->selectedRows().isEmpty()) {
      m_ui->applySettingsPushButton->setEnabled(tablesAreValid);
    }
  }


  /**
   * Slot that listens for changes to the SPK Solve Degree spin box.
   *
   * This slot populates the Instrument Position Solve Options table according to the value of the
   * SPK Solve Degree. Rows are added depending on the degree set, where number of rows added is
   * equal to the SPK Solve Degree + 1. Note that this relies on the updateSolveSettingsSigmaTables
   * slot, which uses the SPK Solve Degree if the ALL position option is selected.
   *
   * @param int Value the SPK Solve Degree spin box was changed to.
   */
  void JigsawSetupDialog::on_spkSolveDegreeSpinBox_valueChanged(int i) {
    // Update the position apriori sigma table and use the position combo box solve option
    updateSolveSettingsSigmaTables(m_ui->positionComboBox, m_ui->positionAprioriSigmaTable);
  }


  /**
   * Slot that listens for changes to the CK Solve Degree spin box.
   *
   * This slot populates the Instrument Pointing Solve Options table according to the value of the
   * CK Solve Degree. Rows are added depending on the degree set, where number of rows added is
   * equal to the CK Solve Degree + 1. Note that this relies on the updateSolveSettingsSigmaTables
   * slot, which uses the CK Solve Degree if the ALL pointing option is selected.
   *
   * @param int Value the CK Solve Degree spin box was changed to.
   */
  void JigsawSetupDialog::on_ckSolveDegreeSpinBox_valueChanged(int i) {
    // Update the pointing apriori sigma table and use the pointing combo box solve option
    updateSolveSettingsSigmaTables(m_ui->pointingComboBox, m_ui->pointingAprioriSigmaTable);
  }


  /**
   * Slot that updates the sigma tables based on the current solve option selection.
   *
   * This will add/remove rows based on the solve option selected, and if ALL, the current
   * solve degree value.
   *
   * @param const QComboBox * The solve option combo box to read the solve option from
   * @param QTableWidget * The a priori sigma table we are going to update rows for
   */
  void JigsawSetupDialog::updateSolveSettingsSigmaTables(const QComboBox *solveOptionComboBox,
                                                         QTableWidget *table) {
    int rowCount = solveOptionComboBox->currentIndex();

    // Position: { NONE, POSITION, VELOCITY, ACCELERATION, ALL }
    // Pointing: { NONE, ANGLES, ANGULAR VELOCITY, ANGULAR ACCELERATION, ALL }
    // Need to add to the solve degree value since number of rows == number of solve coefficients,
    // and for our polynomials the number of solve coefficients == solve degree + 1
    // When solve option is ALL (index 4), use the spk/ck solve degree value + 1 for number of rows
    if (rowCount == 4) {
      if (solveOptionComboBox == m_ui->positionComboBox) {
        rowCount = m_ui->spkSolveDegreeSpinBox->value() + 1;
      }
      else { // if (solveOptionComboBox == m_ui->pointingComboBox)
        rowCount = m_ui->ckSolveDegreeSpinBox->value() + 1;
      }
    }

    // number of rows == position solve option == SolveDegree value + 1 (i+1)
    const int oldRowCount = table->rowCount();
    // if solving ALL, don't add extra row
    table->setRowCount(rowCount);
    const int newRowCount = table->rowCount();

    // Need to check if table is valid in case a row is removed (a row is removed implicitly when
    // the setRowCount() is called when newRowCount < oldRowCount
    validateSigmaTables();

    // Determine the units for either position or pointing
    QStringList solveOptions;
    QString longUnits("N/A");
    QString shortUnits("N/A");
    if (solveOptionComboBox == m_ui->positionComboBox) {
      longUnits = "meters";
      shortUnits = "m";
      solveOptions += {"POSITION", "VELOCITY", "ACCELERATION"};
    }
    else { // if (solveOptionComboBox == m_ui->pointingComboBox) {
      longUnits = "degrees";
      shortUnits = "deg";
      solveOptions += {"ANGLES", "ANGULAR VELOCITY", "ANGULAR ACCELERATION"};
    }

    // Rows need to be added
    if (newRowCount > oldRowCount) {
      for (int row = oldRowCount; row < newRowCount; row++) {
        // Headers : coefficient, description, units, a priori sigma
        QTableWidgetItem *coefficient = new QTableWidgetItem();
        coefficient->setFlags(Qt::ItemIsEnabled);
        coefficient->setText(QString::number(row + 1));
        table->setItem(row, 0, coefficient);

        QTableWidgetItem *description = new QTableWidgetItem();
        description->setFlags(Qt::ItemIsEnabled);
        description->setText("N/A");
        table->setItem(row, 1, description);

        QTableWidgetItem *units = new QTableWidgetItem();
        units->setFlags(Qt::ItemIsEnabled);
        units->setText(shortUnits + "/s^" + QString::number(row));
        table->setItem(row, 2, units);

        QTableWidgetItem *sigma = new QTableWidgetItem();
        sigma->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEditable | Qt::ItemIsEnabled);
        sigma->setText("0.0");
        sigma->setData(Qt::UserRole, QVariant(true));
        table->setItem(row, 3, sigma);

        // Solve option: spk degree { NONE: -1, POSITION: 0, VELOCITY: 1, ACCELERATION: 2, ALL: 2 }
        // POSITION
        if (row == 0) { 
          QTableWidgetItem *description = table->item(0, 1);
          description->setText(solveOptions.at(row));

          QTableWidgetItem *units = table->item(0, 2);
          units->setText(longUnits);
        }

        // VELOCITY
        else if (row == 1) {
          QTableWidgetItem *description = table->item(1, 1);
          description->setText(solveOptions.at(row));

          QTableWidgetItem *units = table->item(1, 2);
          units->setText(shortUnits + "/s");
        }

        // ACCELERATION
        else if (row == 2) {
          QTableWidgetItem *description = table->item(2, 1);
          description->setText(solveOptions.at(row));

          QTableWidgetItem *units = table->item(2, 2);
          units->setText(shortUnits + "/s^2");
        }
      }
    }

    table->resizeColumnToContents(1);
    table->resizeColumnToContents(2);
  }


  /**
   * Slot that listens for when the Instrument Position Solve Option combobox changes.
   *
   * This slot updates the value of the SPK Solve Degree spin box according to which solve position
   * option is selected. This slot also disables the SPK spin boxes whenever a solve
   * position that is not ALL is selected.
   *
   * @param const QString & Reference to the value that the position option combobox was changed to.
   */
  void JigsawSetupDialog::on_positionComboBox_currentIndexChanged(const QString &arg1) {
    int solveIndex = m_ui->positionComboBox->currentIndex();
    QList<QSpinBox *> spinBoxes{m_ui->spkSolveDegreeSpinBox, m_ui->spkDegreeSpinBox};
    for (auto &spinBox : spinBoxes) {
      // ALL enables the solve degree spin box, but does not increase the degree
      // Below shows the corresponding degree for each of the solve options:
      //   NONE = -1; ANGLES = 0; VELOCITY = 1; ACCELERATION = 2; ALL = 2
      if (arg1 == "ALL") {
        spinBox->setValue(solveIndex - 2);
        spinBox->setEnabled(true);
      }
      else {
        // The default value for the spk solve degree and spk degree spinboxes is 2. This is
        // emulating jigsaw's defaults for position solve options that are not ALL.
        spinBox->setValue(2);
        spinBox->setEnabled(false);
      }
    }

    updateSolveSettingsSigmaTables(m_ui->positionComboBox, m_ui->positionAprioriSigmaTable);
  }


  /**
   * Slot that listens for when the Instrument Pointing Solve Option combobox changes.
   *
   * This slot updates the value of the CK Solve Degree spin box according to which solve pointing
   * option is selected. This slot also disables the CK spin boxes whenever a solve
   * pointing that is not ALL is selected.
   *
   * @param const QString & Reference to the value that the pointing option combobox was changed to.
   */
 void JigsawSetupDialog::on_pointingComboBox_currentIndexChanged(const QString &arg1) {
    int solveIndex = m_ui->pointingComboBox->currentIndex();
    QList<QSpinBox *> spinBoxes{m_ui->ckSolveDegreeSpinBox, m_ui->ckDegreeSpinBox};
    for (auto &spinBox : spinBoxes) {
      // ALL enables the solve degree spin box, but does not increase the degree
      // Below shows the corresponding degree for each of the solve options:
      //   NONE = -1; ANGLES = 0; ANGULAR VELOCITY = 1; ANGULAR ACCELERATION = 2; ALL = 2
      if (arg1 == "ALL") {
        spinBox->setValue(solveIndex - 2);
        spinBox->setEnabled(true);
      }
      else {
        // The default value for the ck solve degree and spk degree spinboxes is 2. This is
        // emulating jigsaw's defaults for pointing solve options that are not ALL.
        spinBox->setValue(2);
        spinBox->setEnabled(false);
      }
    }

    updateSolveSettingsSigmaTables(m_ui->pointingComboBox, m_ui->pointingAprioriSigmaTable);
  }



  void JigsawSetupDialog::createObservationSolveSettingsTreeView() {

    QList<ProjectItem *> selectedBOSSItems = m_project->directory()->model()->selectedBOSSImages();

    ProjectItemModel *model = m_project->directory()->model();

    SortFilterProxyModel *osspm = new SortFilterProxyModel;
    osspm->setSourceModel(model);

    //osspm->setDynamicSortFilter(true);
    osspm->setSelectedItems(selectedBOSSItems );
    m_ui->treeView->setModel(osspm);




         ProjectItem *imgRoot = model->findItemData(QVariant("Images"),0);
         if (imgRoot) {

            m_ui->treeView->setRootIndex(osspm->mapFromSource(imgRoot->index()));
         }
         else {
          m_ui->treeView->setRootIndex(QModelIndex());
         }
  
    connect(m_ui->treeView->selectionModel(), 
            SIGNAL(selectionChanged(const QItemSelection&, const QItemSelection&)), 
            this, 
            SLOT(treeViewSelectionChanged(const QItemSelection&,const QItemSelection&)));


    // Try to loop through the view here to add the "groups" so they aren't part of the model

    // Add apply button to the tab view
    // set the text to bold?


  }


  void JigsawSetupDialog::treeViewSelectionChanged(const QItemSelection &selected, 
                                                   const QItemSelection &deselected) {
    QModelIndexList indexList = m_ui->treeView->selectionModel()->selectedRows();

    // qDebug() << "\n\nselected: " << selected.indexes();
    // qDebug() << "deselected: " << deselected.indexes();
    // qDebug() << "selectedRows: " << m_ui->treeView->selectionModel()->selectedRows();


    if (!indexList.isEmpty()) {
      QModelIndex displayIndex = indexList[0];

      SortFilterProxyModel * proxyModel = (SortFilterProxyModel *) m_ui->treeView->model(); 
      ProjectItemModel *sourceModel = (ProjectItemModel *) proxyModel->sourceModel();

      QModelIndex sourceIndex = proxyModel->mapToSource(displayIndex);
      ProjectItem * projItem = sourceModel->itemFromIndex(sourceIndex);

      if (projItem->isImageList()) {
        projItem = projItem->child(0);  
      }
      BundleObservationSolveSettings settings = m_bundleSettings->observationSolveSettings(
                                                                projItem->image()->observationNumber());

      // Populate RHS of Observation Solve Settings tab with selected image's BOSS
      // Instrument Position Solve Options
      m_ui->positionComboBox->setCurrentIndex(settings.instrumentPositionSolveOption());
      m_ui->spkSolveDegreeSpinBox->setValue(settings.spkSolveDegree());
      m_ui->spkDegreeSpinBox->setValue(settings.spkDegree());
      m_ui->hermiteSplineCheckBox->setChecked(settings.solvePositionOverHermite());

      for (int row = 0; row < settings.aprioriPositionSigmas().count(); row++) {
        QTableWidgetItem * sigmaItem = m_ui->positionAprioriSigmaTable->item(row, 3);
        double sigma = settings.aprioriPositionSigmas()[row];
        if (sigma == Isis::Null) {
          sigmaItem->setText("FREE");
        }
        else {
          sigmaItem->setText(QString::number(sigma));
        }
      }

      // Instrument Pointing Solve Options
      m_ui->pointingComboBox->setCurrentIndex(settings.instrumentPointingSolveOption());
      m_ui->ckSolveDegreeSpinBox->setValue(settings.ckSolveDegree());
      m_ui->ckDegreeSpinBox->setValue(settings.ckDegree());
      m_ui->twistCheckBox->setChecked(settings.solveTwist());
      m_ui->fitOverPointingCheckBox->setChecked(settings.solvePolyOverPointing());

      for (int row = 0; row < settings.aprioriPointingSigmas().count(); row++) {
        QTableWidgetItem * sigmaItem = m_ui->pointingAprioriSigmaTable->item(row, 3);
        double sigma = settings.aprioriPointingSigmas()[row];
        if (sigma == Isis::Null) {
          sigmaItem->setText("FREE");
        }
        else {
          sigmaItem->setText(QString::number(sigma));
        }
      }      

    } 

    m_ui->applySettingsPushButton->setEnabled(!selected.isEmpty());
  }



  /**
   * Slot for handling the Observation Solve Settings tab's Apply button. Retrieve's the selected
   * ProjectItems and adds their images' serial numbers to a new BundleObservationSolveSettings
   * object. Serial numbers will be removed from all other BOSS objects, and empty BOSS objects will
   * be removed. 
   */ 
  void JigsawSetupDialog::on_applySettingsPushButton_clicked() {

    // Get the current selected images and the item models
    SortFilterProxyModel * proxyModel = (SortFilterProxyModel *) m_ui->treeView->model();
    ProjectItemModel *sourceModel = (ProjectItemModel *) proxyModel->sourceModel();

    QModelIndexList selectedIndexes = m_ui->treeView->selectionModel()->selectedIndexes();
    QStringList selectedObservationNumbers;

    // Append selected images' serial numbers to selectedObservationNumbers
    foreach (QModelIndex index, selectedIndexes) {
      QModelIndex sourceIndex = proxyModel->mapToSource(index);
      ProjectItem * projItem = sourceModel->itemFromIndex(sourceIndex);

      if (projItem) {
        // Tree traversal is top down so we dont need to do this check for imagelists?
        if (projItem->isImage()) {
          // Grab the observation up front so we don't need to re-compose the observation number
          // more than once (@todo: this should not be necessary when 5026 is integrated)
          const QString observationNumber = projItem->image()->observationNumber();
          if (!selectedObservationNumbers.contains(observationNumber)) {
            selectedObservationNumbers.append(observationNumber);
          }
        }
        else if (projItem->isImageList()) {
          // Use the proxymodel's children as it might not include all of the sourcemodel's children 
          for (int i = 0; i < proxyModel->rowCount(index); i++) {
            QModelIndex childProxyIndex = proxyModel->index(i, 0, index);
            QModelIndex childSourceIndex = proxyModel->mapToSource(childProxyIndex);
            ProjectItem * childItem = sourceModel->itemFromIndex(childSourceIndex);
            selectedObservationNumbers.append(childItem->image()->observationNumber());
          }
        }
      }
    }

    QList<BundleObservationSolveSettings> solveSettingsList = m_bundleSettings->observationSolveSettings();

    //find the bundle observation solve settings for each of the selected observations and remove
    // the observation number from them 
    for (auto &solveSettings : solveSettingsList) {      
      foreach (QString observationNumber, selectedObservationNumbers) {
        solveSettings.removeObservationNumber(observationNumber);
      }
    }


    // Remove any existing solve settings that no longer have any observation numbers
    int iter = 0;
    while (iter < solveSettingsList.count()) {
      if (solveSettingsList[iter].observationNumbers().isEmpty() ) {
        solveSettingsList.removeAt(iter);
      }
      else {
        iter++; // This list shrinks as we remove, so we dont need to iterate every time
      }
    }

    // Create a new bundle observation solve settings
    BundleObservationSolveSettings solveSettings;
    foreach (QString observationNumber, selectedObservationNumbers) {
      solveSettings.addObservationNumber(observationNumber);
    }

    // Grab the data from the right hand side of the observation solve settings tab to set
    // up the new bundle observation solve settings
    updateBundleObservationSolveSettings(solveSettings);

    // Add the new solve settings to the solve settings list
    solveSettingsList.append(solveSettings);

    // Update bundle settings with the new list of bundle observation solve settings
    m_bundleSettings->setObservationSolveOptions(solveSettingsList);

    // qDebug()<<"Current BOSS list --------";
    // for (int i = 0; i < solveSettingsList.count(); i++) {
    //   qDebug() << "solveSettingsList["<<i<<"]: " << solveSettingsList[i].observationNumbers();
    // }
  }

}
