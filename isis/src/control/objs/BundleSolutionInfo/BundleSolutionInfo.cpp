/** This is free and unencumbered software released into the public domain.

The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */

#include "BundleSolutionInfo.h"

#include <QDataStream>
#include <QDebug>
#include <QFile>
#include <QList>
#include <QString>
#include <QStringList>
#include <QUuid>
#include <QXmlStreamWriter>
#include <QXmlStreamReader>

#include "BundleLidarRangeConstraint.h"
#include "BundleResults.h"
#include "Control.h"
#include "ControlList.h"
#include "ControlMeasure.h"
#include "ControlNet.h"
#include "ControlPoint.h"
#include "FileName.h"
#include "ImageList.h"
#include "IString.h"
#include "iTime.h"
#include "Project.h"
#include "PvlKeyword.h"
#include "PvlObject.h"
#include "StatCumProbDistDynCalc.h"
#include "Statistics.h"

namespace Isis {

  /**
   * Constructor. Creates a BundleSolutionInfo.
   *
   * @param inputSettings The settings saved in BundleSolutionInfo
   * @param controlNetworkFileName The file name and path of the control network
   * @param outputStatistics The results of the BundleAdjust
   * @param parent The Qt-relationship parent
   */
  BundleSolutionInfo::BundleSolutionInfo(BundleSettingsQsp inputSettings,
                                         FileName controlNetworkFileName,
                                         BundleResults outputStatistics,
                                         QList<ImageList *> imgList,
                                         QObject *parent) : QObject(parent) {
    m_id = new QUuid(QUuid::createUuid());
    m_runTime = "";
    m_name = m_runTime;
    m_inputControlNetFileName = new FileName(controlNetworkFileName);
    m_outputControl = NULL;
    m_outputControlName="";
    m_inputLidarDataFileName = NULL;
    m_outputLidarDataSet = NULL;
    m_settings = inputSettings;
    m_statisticsResults = new BundleResults(outputStatistics);
    m_images = new QList<ImageList *>(imgList);
    m_adjustedImages = new QList<ImageList *>;
  }


  /**
   * Constructor. Creates a BundleSolutionInfo.
   *
   * @param inputSettings The settings saved in BundleSolutionInfo
   * @param controlNetworkFileName The file name and path of the control network
   * @param outputStatistics The results of the BundleAdjust
   * @param parent The Qt-relationship parent
   */
  BundleSolutionInfo::BundleSolutionInfo(BundleSettingsQsp inputSettings,
                                         FileName controlNetworkFileName,
                                         FileName lidarDataFileName,
                                         BundleResults outputStatistics,
                                         QList<ImageList *> imgList,
                                         QObject *parent) : QObject(parent) {
    m_id = new QUuid(QUuid::createUuid());
    m_runTime = "";
    m_name = m_runTime;
    m_inputControlNetFileName = new FileName(controlNetworkFileName);
    m_outputControl = NULL;
    m_outputControlName="";
    m_inputLidarDataFileName = new FileName(lidarDataFileName);
    m_outputLidarDataSet = NULL;
    m_settings = inputSettings;
    m_statisticsResults = new BundleResults(outputStatistics);
    m_images = new QList<ImageList *>(imgList);
    m_adjustedImages = new QList<ImageList *>;
  }


  /**
   * Handle an XML start element. This expects <image/> and <displayProperties/> elements.
   *
   * @param namespaceURI ???
   * @param localName The keyword name given to the member variable in the XML.
   * @param qName ???
   * @param atts The attribute containing the keyword value for the given local name.
   *
   * @return @b bool True if we should continue reading the XML.
   */
  BundleSolutionInfo::BundleSolutionInfo(Project *project,
                                         QXmlStreamReader *xmlReader,
                                         QObject *parent) : QObject(parent)
  {
    m_id = new QUuid(QUuid::createUuid());
    m_runTime = "";
    m_name = m_runTime;
    m_inputControlNetFileName = NULL;
    m_outputControl = NULL;
    m_outputControlName = "";
    m_inputLidarDataFileName = NULL;
    m_outputLidarDataSet = NULL;
    m_statisticsResults = NULL;
    // what about the rest of the member data ? should we set defaults ??? CREATE INITIALIZE METHOD
    m_images = new QList<ImageList *>;
    m_adjustedImages = new QList<ImageList *>;

    m_xmlHandlerProject = project;
    m_xmlHandlerCharacters = "";

    readBundleSolutionInfo(xmlReader);
  }

  void BundleSolutionInfo::readBundleSolutionInfo(QXmlStreamReader *xmlReader) {
    QString projectRoot;
    if (m_xmlHandlerProject) {
      projectRoot = m_xmlHandlerProject->projectRoot() + "/";
    }
    if (!m_adjustedImages){
      m_adjustedImages = new QList<ImageList *>;
    }
    Q_ASSERT(xmlReader->name() == "bundleSolutionInfo");
    while(xmlReader->readNextStartElement()) {
      if (xmlReader->qualifiedName() == "generalAttributes") {
        while (xmlReader->readNextStartElement()) {
          if (xmlReader->qualifiedName() == "id") {
            m_id = new QUuid(xmlReader->readElementText());
          }
          else if (xmlReader->qualifiedName() == "name") {
            m_name = xmlReader->readElementText();
          }
          else if (xmlReader->qualifiedName() == "runTime") {
            m_runTime = xmlReader->readElementText();
          }
          else if (xmlReader->qualifiedName() == "inputFileName") {
            m_inputControlNetFileName = new FileName(projectRoot.toStdString() + xmlReader->readElementText().toStdString());
          }
          else if (xmlReader->qualifiedName() == "bundleOutTXT") {
            m_txtBundleOutputFilename = projectRoot + xmlReader->readElementText();
          }
          else if (xmlReader->qualifiedName() == "imagesCSV") {
            m_csvSavedImagesFilename = projectRoot + xmlReader->readElementText();
          }
          else if (xmlReader->qualifiedName() == "pointsCSV") {
            m_csvSavedPointsFilename = projectRoot + xmlReader->readElementText();
          }
          else if (xmlReader->qualifiedName() == "residualsCSV") {
            m_csvSavedResidualsFilename = projectRoot + xmlReader->readElementText();
          }
          else {
            xmlReader->skipCurrentElement();
          }
        }
      }
      else if (xmlReader->name() == "bundleSettings") {
        m_settings = NULL;
        BundleSettings *settings = new BundleSettings();
        settings->readBundleSettings(xmlReader);
        m_settings = BundleSettingsQsp(settings);
      }
      else if (xmlReader->name() == "bundleResults") {
        m_statisticsResults = NULL;
        m_statisticsResults = new BundleResults();
        m_statisticsResults->readBundleResults(xmlReader);
      }
      else {
        xmlReader->skipCurrentElement();
      }
    }  
  }

  /**
   * Destructor
   */
  BundleSolutionInfo::~BundleSolutionInfo() {
    delete m_id;

    delete m_inputControlNetFileName;
    m_inputControlNetFileName = NULL;

    delete m_outputControl;
    m_outputControl = NULL;

    delete m_inputLidarDataFileName;
    m_inputLidarDataFileName = NULL;

    delete m_outputLidarDataSet;
    m_outputLidarDataSet = NULL;

    delete m_statisticsResults;
    m_statisticsResults = NULL;

    if (m_images != NULL) {
      delete m_images;
      m_images = NULL;
    }

    // if (m_adjustedImages != NULL) {
    //   qDeleteAll(*m_adjustedImages);
    //   m_adjustedImages->clear();
    //   delete m_adjustedImages;
    //   m_adjustedImages = NULL;
    // }
  }


  /**
   * Returns bundleout text filename.
   *
   * @return QString Bundleout text filename.
   */
  QString BundleSolutionInfo::savedBundleOutputFilename() {
    return m_txtBundleOutputFilename;
  }


  /**
   * Returns filename of output bundle images csv file.
   *
   * @return QString filename of output bundle images csv file.
   */
  QString BundleSolutionInfo::savedImagesFilename() {
    return m_csvSavedImagesFilename;
  }


  /**
   * Returns filename of output bundle points csv file.
   *
   * @return QString filename of output bundle points csv file.
   */
  QString BundleSolutionInfo::savedPointsFilename() {
    return m_csvSavedPointsFilename;
  }


  /**
   * Returns filename of output bundle residuals csv file.
   *
   * @return QString filename of output bundle residuals csv file.
   */
  QString BundleSolutionInfo::savedResidualsFilename() {
    return m_csvSavedResidualsFilename;
  }


  /**
   * Adds a list of images that were adjusted (their labels were updated).
   *
   * @param ImageList *images The list of images to add that had their labels updated.
   */
  void BundleSolutionInfo::addAdjustedImages(ImageList *images) {
    m_adjustedImages->append(images);
  }


  /**
   * Sets the stat results.
   *
   * @param statisticsResults The new BundleResults
   */
  void BundleSolutionInfo::setOutputStatistics(BundleResults statisticsResults) {
    delete m_statisticsResults;
    m_statisticsResults = NULL;
    m_statisticsResults = new BundleResults(statisticsResults);
  }


  /**
   * TODO: change description below to something more like english.
   *
   * Change the on-disk file name for the control network used to be where the control network
   * ought to be in the given project.
   *
   * This method is modelled after the updateFileName() methods in Image and Control. Those methods
   * close something (cubes for Image and a control net for control) but there is not a close
   * method in BundleSolutionInfo.
   *
   * @param project The project that this BundleSolutionInfo is stored in
   */
  void BundleSolutionInfo::updateFileName(Project *project) {

    //TODO do we need to close anything here?

    FileName oldInputFileName(*m_inputControlNetFileName);
    FileName newInputFileName(project->cnetRoot().toStdString() + "/" +
                     oldInputFileName.dir().filename().string() + "/" + oldInputFileName.name());
    *m_inputControlNetFileName = newInputFileName.expanded();

    FileName oldOutputFileName(m_outputControl->fileName().toStdString());
    FileName newOutputFileName(project->cnetRoot().toStdString() + "/" +
                     oldOutputFileName.dir().filename().string() + "/" + oldOutputFileName.name());

    if (m_outputControl) {
      delete m_outputControl;
    }
    m_outputControl = new Control(QString::fromStdString(newOutputFileName.expanded()));
    m_outputControlName = QString::fromStdString(newOutputFileName.expanded());
  }


  /**
   * Returns the list of images that were adjusted after a bundle. This can potentially be an
   * empty QList if no image labels were updated.
   *
   * @return QList<ImageList*> Returns the adjusted images (images with updated labels).
   */
  QList<ImageList *> BundleSolutionInfo::adjustedImages() const {
    return *m_adjustedImages;
  }


  /**
   * Get a unique, identifying string associated with this BundleSolutionInfo object.
   *
   * @return @b QString A unique ID for this BundleSolutionInfo object
   */
  QString BundleSolutionInfo::id() const {
    return m_id->toString().remove(QRegExp("[{}]"));
  }


  /**
   * Sets the run time, and the name if a name is not already set.
   *
   * @param runTime The run time.
   */
  void BundleSolutionInfo::setRunTime(QString runTime) {
    // ??? validate that a valid time has been given???
    // try {
    //   iTime time(runTime);
    // }
    // catch (...) {
    //   throw IException(IException::Unknown,
    //                    "Invalid bundle adjustment run time [" + runTime + ].",
    //                    _FILEINFO_);
    // }
    m_runTime = runTime;
    if (m_name == m_runTime || m_name == "") {
      m_name = runTime;
    }
  }


  /**
   * Returns the run time.
   *
   * @return @b QString The run time.
   */
  QString BundleSolutionInfo::runTime() const {
    return m_runTime;
  }


  /**
   * Returns the name of the input control network.
   *
   * @return @b QString The name of the input control network.
   */
  QString BundleSolutionInfo::inputControlNetFileName() const {
    return QString::fromStdString(m_inputControlNetFileName->expanded());
  }


  /**
   * Returns the name of the output control network.
   *
   * @return @b QString The name of the output control network.
   */
  QString BundleSolutionInfo::outputControlNetFileName() const {

    if (m_outputControl)
      return m_outputControl->fileName();
    else
      return m_outputControlName;
  }


  /**
   * Returns name of input lidar data file (if any).
   *
   * @return QString Name of input lidar data file.
   */
  QString BundleSolutionInfo::inputLidarDataFileName() const {
    return QString::fromStdString(m_inputLidarDataFileName->expanded());
  }


  /**
   * Returns the name of the output control network.
   *
   * @return @b QString The name of the output control network.
   */
  void BundleSolutionInfo::setOutputControl(Control *outputControl) {
    m_outputControl = outputControl;
  }


  /**
   * Sets m_outputControlName
   *
   * @param name QString of the new value
   */
  void BundleSolutionInfo::setOutputControlName(QString name) {
    m_outputControlName = name;
  }


  /**
   * Returns m_outputControlName
   *
   * @returns QString of the value
   */
  QString BundleSolutionInfo::outputControlName() const {
    return m_outputControlName;
  }

  /**
   * Returns bundle output Control object.
   *
   * @return Control* Pointer to bundle output Control object.
   */
  Control *BundleSolutionInfo::control() const {
    return m_outputControl;
  }


  /**
   * Returns bundle settings.
   *
   * @return BundleSettingsQsp Bundle settings.
   */
  BundleSettingsQsp BundleSolutionInfo::bundleSettings() {
    return m_settings;
  }


  /**
   * Returns the bundle results.
   *
   * @throws IException::Unknown "Results for this bundle is NULL."
   *
   * @return @b BundleResults The bundle results.
   */
  BundleResults BundleSolutionInfo::bundleResults() {
    if (m_statisticsResults) {
      return *m_statisticsResults;
    }
    else {
      throw IException(IException::Unknown,
                       "Results for this bundle is NULL.",
                       _FILEINFO_);
    }
  }


  /**
   * Returns the images used in the bundle
   *
   * @return m_imageList The image list used in the bundle
   */
  QList<ImageList *> BundleSolutionInfo::imageList() {
    return *m_images;
  }


  /**
   * Sets the name of the bundle.
   *
   * @param name QString of the new name
   */
  void BundleSolutionInfo::setName(QString name) {
    m_name = name;
  }


  /**
   * Returns the name of the bundle. The name defaults to the id, unless the name has been set using
   * setName()
   *
   * @return QString Name of the bundle
   */
  QString BundleSolutionInfo::name() const {
    return m_name;
  }


  /**
   * @brief Outputs the header for the bundleout_images.csv file
   * @param fpOut The output file stream.
   * @param observation An observation for the instrument that this header is for.
   * @return True if the write is successful, False otherwise.
   *
   * @internal
   *   @history 2016-12-08 Ian Humphrey - Removed conditions that handle TWIST headers differently
   *                           than the other headers. The number of TWIST headers will be the same
   *                           as each of the other angle headers. Fixes #4557.
   */
  bool BundleSolutionInfo::outputImagesCSVHeader(std::ofstream &fpOut, BundleObservationQsp observation) {

    if (!fpOut) {
      return false;
    }

    char buf[1056];

    // setup column headers
    std::vector<QString> outputColumns;

    outputColumns.push_back("Image,");
    outputColumns.push_back("rms,");
    outputColumns.push_back("rms,");
    outputColumns.push_back("rms,");

    QStringList observationParameters = observation->parameterList();

    for (int i = 0; i < observationParameters.size(); i++) {
      for (int j = 0; j < 5; j++) {
        outputColumns.push_back(observationParameters[i] + ",");
      }
    }

    // print first column header to buffer and output to file
    int ncolumns = outputColumns.size();
    for (int i = 0; i < ncolumns; i++) {
      QString str = outputColumns.at(i);
      snprintf(buf, sizeof(buf), "%s", (const char*)str.toLatin1().data());
      fpOut << buf;
    }
    snprintf(buf, sizeof(buf), "\n");
    fpOut << buf;

    outputColumns.clear();

    outputColumns.push_back("Filename,");
    outputColumns.push_back("sample res,");
    outputColumns.push_back("line res,");
    outputColumns.push_back("total res,");

    for (int i = 0; i < observationParameters.size(); i++) {
      outputColumns.push_back("Initial,");
      outputColumns.push_back("Correction,");
      outputColumns.push_back("Final,");
      outputColumns.push_back("Apriori Sigma,");
      outputColumns.push_back("Adj Sigma,");
    }

    // print second column header to buffer and output to file
    ncolumns = outputColumns.size();
    for (int i = 0; i < ncolumns; i++) {
      QString str = outputColumns.at(i);
      snprintf(buf, sizeof(buf), "%s", (const char*)str.toLatin1().data());
      fpOut << buf;
    }
    snprintf(buf, sizeof(buf), "\n");
    fpOut << buf;

    return true;
  }



  /**
   * Output header for bundle results file.
   *
   * @param fpOut The output stream that the header will be sent to.
   *
   * @return @b bool If the header was successfully output to the output stream.
   *
   * @throws IException::Io "Failed to output residual percentiles for bundleout"
   * @throws IException::Io "Failed to output residual box plot for bundleout"
   *
   * @todo Determine how multiple sensor solve settings should be output.
   */
  bool BundleSolutionInfo::outputHeader(std::ofstream &fpOut) {

    if (!fpOut) {
      return false;
    }

    LidarDataQsp lidarData = m_statisticsResults->outputLidarData();

    char buf[1056];
    int numObservations = m_statisticsResults->observations().size();
    int numImages = 0;
    for (int i = 0; i < numObservations; i++) {
      numImages += m_statisticsResults->observations().at(i)->size();
    }
    int numValidPoints = m_statisticsResults->outputControlNet()->GetNumValidPoints();

    int numValidLidarPoints = 0;
    if (lidarData) {
      numValidLidarPoints = lidarData->numberLidarPoints();
    }

    int numInnerConstraints = 0;
    int numDistanceConstraints = 0;
    int numDegreesOfFreedom = m_statisticsResults->degreesOfFreedom();

    int convergenceCriteria = 1;

    snprintf(buf, sizeof(buf), "JIGSAW: BUNDLE ADJUSTMENT\n=========================\n");
    fpOut << buf;
    snprintf(buf, sizeof(buf), "\n                       Run Time: %s",
                  Isis::iTime::CurrentLocalTime().toLatin1().data());
    fpOut << buf;
    snprintf(buf, sizeof(buf), "\n                       Network Filename: %s",
                  m_inputControlNetFileName->expanded().c_str());
    fpOut << buf;

    snprintf(buf, sizeof(buf),"\n                       Cube List: %s",
                m_settings->cubeList().toStdString().c_str());

    fpOut << buf;

    snprintf(buf, sizeof(buf), "\n                       Output Network Filename: %s",
                              outputControlName().toStdString().c_str() );
    fpOut << buf;
    snprintf(buf, sizeof(buf),"\n                       Output File Prefix: %s",
                m_settings->outputFilePrefix().toStdString().c_str() );
    fpOut <<buf;

    snprintf(buf, sizeof(buf), "\n                       Network Id: %s",
                  m_statisticsResults->outputControlNet()->GetNetworkId().toLatin1().data());
    fpOut << buf;
    snprintf(buf, sizeof(buf), "\n                       Network Description: %s",\
                  m_statisticsResults->outputControlNet()->Description().toLatin1().data());
    fpOut << buf;
    if (m_inputLidarDataFileName) {
      snprintf(buf, sizeof(buf), "\n            Lidar Data Filename: %s",
                    m_inputLidarDataFileName->expanded().c_str());
      fpOut << buf;
    }
    snprintf(buf, sizeof(buf), "\n                       Target: %s",
                  m_statisticsResults->outputControlNet()->GetTarget().toLatin1().data());
    fpOut << buf;
    snprintf(buf, sizeof(buf), "\n\n                       Linear Units: kilometers");
    fpOut << buf;
    snprintf(buf, sizeof(buf), "\n                       Angular Units: decimal degrees");
    fpOut << buf;
    snprintf(buf, sizeof(buf), "\n\nINPUT: SOLVE OPTIONS\n====================\n");
    fpOut << buf;

    m_settings->solveObservationMode() ?
      snprintf(buf, sizeof(buf), "\n                   OBSERVATIONS: ON"):
      snprintf(buf, sizeof(buf), "\n                   OBSERVATIONS: OFF");
    fpOut << buf;

    if (m_settings->controlPointCoordTypeBundle() == SurfacePoint::Latitudinal) {
      m_settings->solveRadius() ?
        snprintf(buf, sizeof(buf), "\n                         RADIUS: ON"):
        snprintf(buf, sizeof(buf), "\n                         RADIUS: OFF");
    }
    else { // Rectangular (XYZ) solution
      snprintf(buf, sizeof(buf), "\n                         RADIUS: N/A");
    }
    fpOut << buf;

    m_settings->solveTargetBody() ?
      snprintf(buf, sizeof(buf), "\n                    TARGET BODY: ON"):
      snprintf(buf, sizeof(buf), "\n                    TARGET BODY: OFF");
    fpOut << buf;

    m_settings->updateCubeLabel() ?
      snprintf(buf, sizeof(buf), "\n                         UPDATE: YES"):
      snprintf(buf, sizeof(buf), "\n                         UPDATE: NO");
    fpOut << buf;

    m_settings->errorPropagation() ?
      snprintf(buf, sizeof(buf), "\n              ERROR PROPAGATION: ON"):
      snprintf(buf, sizeof(buf), "\n              ERROR PROPAGATION: OFF");
    fpOut << buf;

    (m_settings->controlPointCoordTypeReports() == SurfacePoint::Latitudinal) ?
      snprintf(buf, sizeof(buf), "\n  CONTROL POINT COORDINATE TYPE FOR REPORTS: LATITUDINAL"):
      snprintf(buf, sizeof(buf), "\n  CONTROL POINT COORDINATE TYPE FOR REPORTS: RECTANGULAR");
    fpOut << buf;

    (m_settings->controlPointCoordTypeBundle() == SurfacePoint::Latitudinal) ?
      snprintf(buf, sizeof(buf), "\n  CONTROL POINT COORDINATE TYPE FOR BUNDLE: LATITUDINAL"):
      snprintf(buf, sizeof(buf), "\n  CONTROL POINT COORDINATE TYPE FOR BUNDLE: RECTANGULAR");
    fpOut << buf;

    if (m_settings->outlierRejection()) {
      snprintf(buf, sizeof(buf), "\n              OUTLIER REJECTION: ON");
      fpOut << buf;
      snprintf(buf, sizeof(buf), "\n           REJECTION MULTIPLIER: %lf",
                    m_settings->outlierRejectionMultiplier());
      fpOut << buf;

    }
    else {
      snprintf(buf, sizeof(buf), "\n              OUTLIER REJECTION: OFF");
      fpOut << buf;
      snprintf(buf, sizeof(buf), "\n           REJECTION MULTIPLIER: N/A");
      fpOut << buf;
    }

    // Added April 5, 2017
    snprintf(buf, sizeof(buf), "\n              CONTROL POINT COORDINATE TYPE FOR REPORTS:  %s",
       SurfacePoint::coordinateTypeToString(m_settings->controlPointCoordTypeReports()).toLatin1().data());

    // Added July 4, 2017
    snprintf(buf, sizeof(buf), "\n              CONTROL POINT COORDINATE TYPE FOR BUNDLE:  %s",
       SurfacePoint::coordinateTypeToString(m_settings->controlPointCoordTypeBundle()).toLatin1().data());

    snprintf(buf, sizeof(buf), "\n\nMAXIMUM LIKELIHOOD ESTIMATION\n============================\n");
    fpOut << buf;

    for (int tier = 0; tier < 3; tier++) {
      if (tier < m_statisticsResults->numberMaximumLikelihoodModels()) {
        snprintf(buf, sizeof(buf), "\n                         Tier %d Enabled: TRUE", tier);
        fpOut << buf;
        snprintf(buf, sizeof(buf), "\n               Maximum Likelihood Model: %s",
                      MaximumLikelihoodWFunctions::modelToString(
                          m_statisticsResults->
                              maximumLikelihoodModelWFunc(tier).model()).toLatin1().data());
        fpOut << buf;
        snprintf(buf, sizeof(buf), "\n    Quantile used for tweaking constant: %lf",
                      m_statisticsResults->maximumLikelihoodModelQuantile(tier));
        fpOut << buf;
        snprintf(buf, sizeof(buf), "\n   Quantile weighted R^2 Residual value: %lf",
                      m_statisticsResults->maximumLikelihoodModelWFunc(tier).tweakingConstant());
        fpOut << buf;
        snprintf(buf, sizeof(buf), "\n       Approx. weighted Residual cutoff: %s",
                      m_statisticsResults->maximumLikelihoodModelWFunc(tier)
                          .weightedResidualCutoff().toLatin1().data());
        fpOut << buf;
        if (tier != 2) fpOut << "\n";
      }
      else {
        snprintf(buf, sizeof(buf), "\n                         Tier %d Enabled: FALSE", tier);
        fpOut << buf;
      }
    }

    snprintf(buf, sizeof(buf), "\n\nINPUT: CONVERGENCE CRITERIA\n===========================\n");
    fpOut << buf;
    snprintf(buf, sizeof(buf), "\n                         SIGMA0: %e",
                  m_settings->convergenceCriteriaThreshold());
    fpOut << buf;
    snprintf(buf, sizeof(buf), "\n             MAXIMUM ITERATIONS: %d",
                  m_settings->convergenceCriteriaMaximumIterations());
    fpOut << buf;

    //TODO Should it be checked that positionSigmas.size() == positionSolveDegree and
    //     pointingSigmas.size() == pointingSolveDegree somewhere? JAM

    //TODO How do we output this information when using multiple solve settings? JAM

    BundleObservationSolveSettings globalSettings = m_settings->observationSolveSettings(0);
    int pointingSolveDegree = globalSettings.numberCameraAngleCoefficientsSolved();
    QList<double> pointingSigmas = globalSettings.aprioriPointingSigmas();
    int positionSolveDegree = globalSettings.numberCameraPositionCoefficientsSolved();
    QList<double> positionSigmas = globalSettings.aprioriPositionSigmas();

    snprintf(buf, sizeof(buf), "\n\nINPUT: CAMERA POINTING OPTIONS\n==============================\n");
    fpOut << buf;
    switch (pointingSolveDegree) {
      case 0:
        snprintf(buf, sizeof(buf),"\n                          CAMSOLVE: NONE");
        break;
      case 1:
        snprintf(buf, sizeof(buf),"\n                          CAMSOLVE: ANGLES");
        break;
      case 2:
        snprintf(buf, sizeof(buf),"\n                          CAMSOLVE: ANGLES, VELOCITIES");
        break;
      case 3:
        snprintf(buf, sizeof(buf),"\n                          CAMSOLVE: ANGLES, VELOCITIES, ACCELERATIONS");
        break;
      default:
        snprintf(buf, sizeof(buf),"\n                          CAMSOLVE: ALL POLYNOMIAL COEFFICIENTS (%d)"
                    "\n                          CKDEGREE: %d"
                    "\n                     CKSOLVEDEGREE: %d",
                pointingSolveDegree,
                globalSettings.ckDegree(),
                globalSettings.ckSolveDegree());
        break;
    }
    fpOut << buf;
    globalSettings.solveTwist() ?
        snprintf(buf, sizeof(buf), "\n                             TWIST: ON"):
        snprintf(buf, sizeof(buf), "\n                             TWIST: OFF");
    fpOut << buf;
    globalSettings.solvePolyOverPointing() ?
        snprintf(buf, sizeof(buf), "\n POLYNOMIAL OVER EXISTING POINTING: ON"):
        snprintf(buf, sizeof(buf), "\nPOLYNOMIAL OVER EXISTING POINTING : OFF");
    fpOut << buf;

    snprintf(buf, sizeof(buf), "\n\nINPUT: SPACECRAFT OPTIONS\n=========================\n");
    fpOut << buf;
    switch (positionSolveDegree) {
      case 0:
        snprintf(buf, sizeof(buf),"\n                        SPSOLVE: NONE");
        break;
      case 1:
        snprintf(buf, sizeof(buf),"\n                        SPSOLVE: POSITION");
        break;
      case 2:
        snprintf(buf, sizeof(buf),"\n                        SPSOLVE: POSITION, VELOCITIES");
        break;
      case 3:
        snprintf(buf, sizeof(buf),"\n                        SPSOLVE: POSITION, VELOCITIES, ACCELERATIONS");
        break;
      default:
        snprintf(buf, sizeof(buf),"\n                        SPSOLVE: ALL POLYNOMIAL COEFFICIENTS (%d)"
                    "\n                      SPKDEGREE: %d"
                    "\n                 SPKSOLVEDEGREE: %d",
                positionSolveDegree,
                globalSettings.spkDegree(),
                globalSettings.spkSolveDegree());
        break;
    }
    fpOut << buf;
    globalSettings.solvePositionOverHermite() ?
        snprintf(buf, sizeof(buf), "\n POLYNOMIAL OVER HERMITE SPLINE: ON"):
        snprintf(buf, sizeof(buf), "\nPOLYNOMIAL OVER HERMITE SPLINE : OFF");
    fpOut << buf;

    snprintf(buf, sizeof(buf), "\n\nINPUT: GLOBAL IMAGE PARAMETER UNCERTAINTIES\n===========================================\n");
    QString coord1Str;
    QString coord2Str;
    QString coord3Str;
    switch (m_settings->controlPointCoordTypeBundle()) {
      case SurfacePoint::Latitudinal:
        coord1Str = "               POINT LATITUDE";
        coord2Str = "              POINT LONGITUDE";
        coord3Str = "                 POINT RADIUS";
        break;
      case SurfacePoint::Rectangular:
        coord1Str = "                      POINT X";
        coord2Str = "                      POINT Y";
        coord3Str = "                      POINT Z";
        break;
      default:
         IString msg ="Unknown surface point coordinate type enum ["
           + toString(m_settings->controlPointCoordTypeReports()) + "]." ;
         throw IException(IException::Programmer, msg, _FILEINFO_);
        break;
    }

    // Coordinate 1 (latitude or point X)
    fpOut << buf;
    (m_settings->globalPointCoord1AprioriSigma()  == Isis::Null) ?
      snprintf(buf, sizeof(buf),"\n%s SIGMA: N/A", coord1Str.toLatin1().data()):
      snprintf(buf, sizeof(buf),"\n%s SIGMA: %lf (meters)", coord1Str.toLatin1().data(),
              m_settings->globalPointCoord1AprioriSigma());
    // Coordinate 2 (longitude or point Y)
    fpOut << buf;
    (m_settings->globalPointCoord2AprioriSigma() == Isis::Null) ?
      snprintf(buf, sizeof(buf),"\n%s SIGMA: N/A", coord2Str.toLatin1().data()):
      snprintf(buf, sizeof(buf),"\n%s SIGMA: %lf (meters)", coord2Str.toLatin1().data(),
              m_settings->globalPointCoord2AprioriSigma());
    // Coordinate 3 (radius or point Z)
    fpOut << buf;
    (m_settings->globalPointCoord3AprioriSigma() == Isis::Null) ?
      snprintf(buf, sizeof(buf),"\n%s SIGMA: N/A", coord3Str.toLatin1().data()):
      snprintf(buf, sizeof(buf),"\n%s SIGMA: %lf (meters)", coord3Str.toLatin1().data(),
                m_settings->globalPointCoord3AprioriSigma());
    fpOut << buf;
    (positionSolveDegree < 1 || positionSigmas[0] == Isis::Null) ?
        snprintf(buf, sizeof(buf),"\n          SPACECRAFT POSITION SIGMA: N/A"):
        snprintf(buf, sizeof(buf),"\n          SPACECRAFT POSITION SIGMA: %lf (meters)",
                positionSigmas[0]);
    fpOut << buf;

    (positionSolveDegree < 2 || positionSigmas[1] == Isis::Null) ?
        snprintf(buf, sizeof(buf),"\n          SPACECRAFT VELOCITY SIGMA: N/A"):
        snprintf(buf, sizeof(buf),"\n          SPACECRAFT VELOCITY SIGMA: %lf (m/s)",
                positionSigmas[1]);
    fpOut << buf;

    (positionSolveDegree < 3 || positionSigmas[2] == Isis::Null) ?
        snprintf(buf, sizeof(buf),"\n      SPACECRAFT ACCELERATION SIGMA: N/A"):
        snprintf(buf, sizeof(buf),"\n      SPACECRAFT ACCELERATION SIGMA: %lf (m/s/s)",
                positionSigmas[2]);
    fpOut << buf;

    (pointingSolveDegree < 1 || pointingSigmas[0] == Isis::Null) ?
        snprintf(buf, sizeof(buf),"\n                CAMERA ANGLES SIGMA: N/A"):
        snprintf(buf, sizeof(buf),"\n                CAMERA ANGLES SIGMA: %lf (dd)",
                pointingSigmas[0]);
    fpOut << buf;

    (pointingSolveDegree < 2 || pointingSigmas[1] == Isis::Null) ?
        snprintf(buf, sizeof(buf),"\n      CAMERA ANGULAR VELOCITY SIGMA: N/A"):
        snprintf(buf, sizeof(buf),"\n      CAMERA ANGULAR VELOCITY SIGMA: %lf (dd/s)",
                pointingSigmas[1]);
    fpOut << buf;

    (pointingSolveDegree < 3 || pointingSigmas[2] == Isis::Null) ?
        snprintf(buf, sizeof(buf),"\n  CAMERA ANGULAR ACCELERATION SIGMA: N/A"):
        snprintf(buf, sizeof(buf),"\n  CAMERA ANGULAR ACCELERATION SIGMA: %lf (dd/s/s)",
                pointingSigmas[2]);
    fpOut << buf;

    if (m_settings->solveTargetBody()) {
      snprintf(buf, sizeof(buf), "\n\nINPUT: TARGET BODY OPTIONS\n==============================\n");
      fpOut << buf;

      if (m_settings->solvePoleRA() && m_settings->solvePoleDec()) {
        snprintf(buf, sizeof(buf),"\n                             POLE: RIGHT ASCENSION");
        fpOut << buf;
        snprintf(buf, sizeof(buf),"\n                                 : DECLINATION\n");
        fpOut << buf;
      }
      else if (m_settings->solvePoleRA()) {
        snprintf(buf, sizeof(buf),"\n                             POLE: RIGHT ASCENSION\n");
        fpOut << buf;
      }
      else if (m_settings->solvePoleDec()) {
        snprintf(buf, sizeof(buf),"\n                             POLE: DECLINATION\n");
        fpOut << buf;
      }

      if (m_settings->solvePM() || m_settings->solvePMVelocity()
          || m_settings->solvePMAcceleration()) {
        snprintf(buf, sizeof(buf),"\n                   PRIME MERIDIAN: W0 (OFFSET)");
        fpOut << buf;

        if (m_settings->solvePMVelocity()) {
          snprintf(buf, sizeof(buf),"\n                                 : WDOT (SPIN RATE)");
          fpOut << buf;
        }
        if (m_settings->solvePMAcceleration()) {
          snprintf(buf, sizeof(buf),"\n                               :W ACCELERATION");
          fpOut << buf;
        }
      }

      if (m_settings->solveTriaxialRadii() || m_settings->solveMeanRadius()) {
        if (m_settings->solveMeanRadius()) {
          snprintf(buf, sizeof(buf),"\n                            RADII: MEAN");
          fpOut << buf;
        }
        else if (m_settings->solveTriaxialRadii()) {
          snprintf(buf, sizeof(buf),"\n                            RADII: TRIAXIAL");
          fpOut << buf;
        }
      }
    }

    snprintf(buf, sizeof(buf), "\n\nJIGSAW: RESULTS\n===============\n");
    fpOut << buf;
    snprintf(buf, sizeof(buf), "\n                         Images: %6d",numImages);
    fpOut << buf;
    snprintf(buf, sizeof(buf), "\n                         Points: %6d",numValidPoints);
    fpOut << buf;

    if (numValidLidarPoints > 0) {
      snprintf(buf, sizeof(buf), "\n                   Lidar Points: %6d",numValidLidarPoints);
      fpOut << buf;
    }

    snprintf(buf, sizeof(buf), "\n                 Total Measures: %6d",
                  (m_statisticsResults->numberObservations()
                      + m_statisticsResults->numberRejectedObservations()) / 2);
    fpOut << buf;

    snprintf(buf, sizeof(buf), "\n             Total Observations: %6d",
                  m_statisticsResults->numberObservations()
                      + m_statisticsResults->numberRejectedObservations());
    fpOut << buf;

    snprintf(buf, sizeof(buf), "\n              Good Observations: %6d",
                  m_statisticsResults->numberObservations());
    fpOut << buf;

    snprintf(buf, sizeof(buf), "\n          Rejected Observations: %6d",
                  m_statisticsResults->numberRejectedObservations());
    fpOut << buf;

    if (m_statisticsResults->numberConstrainedPointParameters() > 0) {
      snprintf(buf, sizeof(buf), "\n   Constrained Point Parameters: %6d",
                    m_statisticsResults->numberConstrainedPointParameters());
      fpOut << buf;
    }

    if (m_statisticsResults->numberConstrainedImageParameters() > 0) {
      snprintf(buf, sizeof(buf), "\n   Constrained Image Parameters: %6d",
                    m_statisticsResults->numberConstrainedImageParameters());
      fpOut << buf;
    }

    if (m_statisticsResults->numberConstrainedTargetParameters() > 0) {
      snprintf(buf, sizeof(buf), "\n  Constrained Target Parameters: %6d",
                    m_statisticsResults->numberConstrainedTargetParameters());
      fpOut << buf;
    }

    if (m_statisticsResults->numberLidarRangeConstraintEquations() > 0) {
      snprintf(buf, sizeof(buf), "\n        Lidar Range Constraints: %6d",
              m_statisticsResults->numberLidarRangeConstraintEquations());
      fpOut << buf;
    }

    snprintf(buf, sizeof(buf), "\n                       Unknowns: %6d",
                  m_statisticsResults->numberUnknownParameters());
    fpOut << buf;

    if (numInnerConstraints > 0) {
      snprintf(buf, sizeof(buf), "\n      Inner Constraints: %6d", numInnerConstraints);
     fpOut << buf;
    }

    if (numDistanceConstraints > 0) {
      snprintf(buf, sizeof(buf), "\n   Distance Constraints: %d", numDistanceConstraints);
      fpOut << buf;
    }

    snprintf(buf, sizeof(buf), "\n             Degrees of Freedom: %6d", numDegreesOfFreedom);
    fpOut << buf;

    snprintf(buf, sizeof(buf), "\n           Convergence Criteria: %6.3g",
                               m_settings->convergenceCriteriaThreshold());
    fpOut << buf;

    if (convergenceCriteria == 1) {
      snprintf(buf, sizeof(buf), "(Sigma0)");
      fpOut << buf;
    }

    snprintf(buf, sizeof(buf), "\n                     Iterations: %6d", m_statisticsResults->iterations());
    fpOut << buf;

    if (m_statisticsResults->iterations() >= m_settings->convergenceCriteriaMaximumIterations()) {
      snprintf(buf, sizeof(buf), "(Maximum reached)");
      fpOut << buf;
    }

    snprintf(buf, sizeof(buf), "\n                         Sigma0: %30.20lf\n", m_statisticsResults->sigma0());
    fpOut << buf;
    snprintf(buf, sizeof(buf), " Error Propagation Elapsed Time: %6.4lf (seconds)\n",
                  m_statisticsResults->elapsedTimeErrorProp());
    fpOut << buf;
    snprintf(buf, sizeof(buf), "             Total Elapsed Time: %6.4lf (seconds)\n",
                  m_statisticsResults->elapsedTime());
    fpOut << buf;
    if (m_statisticsResults->numberObservations()
        + m_statisticsResults->numberRejectedObservations()
        > 100) {
      snprintf(buf, sizeof(buf), "\n           Residual Percentiles:\n");
      fpOut << buf;

    // residual prob distribution values are calculated/printed
    // even if there is no maximum likelihood estimation
      try {
        for (int bin = 1;bin < 34;bin++) {
          double cumProb = double(bin) / 100.0;
          double resValue =
              m_statisticsResults->
                  residualsCumulativeProbabilityDistribution().value(cumProb);
          double resValue33 =
              m_statisticsResults->
                  residualsCumulativeProbabilityDistribution().value(cumProb + 0.33);
          double resValue66 =
              m_statisticsResults->
                  residualsCumulativeProbabilityDistribution().value(cumProb + 0.66);
          snprintf(buf, sizeof(buf), "                 Percentile %3d: %+8.3lf"
                       "                 Percentile %3d: %+8.3lf"
                       "                 Percentile %3d: %+8.3lf\n",
                                         bin,      resValue,
                                         bin + 33, resValue33,
                                         bin + 66, resValue66);
          fpOut << buf;
        }
      }
      catch (IException &e) {
        std::string msg = "Failed to output residual percentiles for bundleout";
        throw IException(e, IException::Io, msg, _FILEINFO_);
      }
      try {
        snprintf(buf, sizeof(buf), "\n              Residual Box Plot:");
        fpOut << buf;
        snprintf(buf, sizeof(buf), "\n                        minimum: %+8.3lf",
                m_statisticsResults->residualsCumulativeProbabilityDistribution().min());
        fpOut << buf;
        snprintf(buf, sizeof(buf), "\n                     Quartile 1: %+8.3lf",
                m_statisticsResults->residualsCumulativeProbabilityDistribution().value(0.25));
        fpOut << buf;
        snprintf(buf, sizeof(buf), "\n                         Median: %+8.3lf",
                m_statisticsResults->residualsCumulativeProbabilityDistribution().value(0.50));
        fpOut << buf;
        snprintf(buf, sizeof(buf), "\n                     Quartile 3: %+8.3lf",
                m_statisticsResults->residualsCumulativeProbabilityDistribution().value(0.75));
        fpOut << buf;
        snprintf(buf, sizeof(buf), "\n                        maximum: %+8.3lf\n",
                m_statisticsResults->residualsCumulativeProbabilityDistribution().max());
        fpOut << buf;
      }
      catch (IException &e) {
        std::string msg = "Failed to output residual box plot for bundleout";
        throw IException(e, IException::Io, msg, _FILEINFO_);
      }
    }

    // Loop over the observations to find the longest file path/name in the
    // bunch
    int filePadding = 0;

    for (int i = 0; i < numObservations; i++) {

      int numImagesInObservation = m_statisticsResults->observations().at(i)->size();

      for (int j = 0; j < numImagesInObservation; j++) {
        BundleImageQsp bundleImage = m_statisticsResults->observations().at(i)->at(j);

        if (bundleImage->fileName().length() > filePadding) {
          filePadding = bundleImage->fileName().length();
        }
      }
    }

    snprintf(buf, sizeof(buf), "\nIMAGE MEASURES SUMMARY\n==========================\n\n");
    fpOut << buf;

    // Pad each element in the table with the space for the longest image
    // path/name then padd it the length of the element + 1
    QString header("Measures                            RMS(pixels)");
    if (m_statisticsResults->outputLidarData()) {
      header += "                                   Lidar RMS(pixels)";
    }
    // This is padded by an extra 11 to move it center to the table
    snprintf(buf, sizeof(buf),"%*s\n", header.length() + 11 + filePadding, header.toLatin1().data());
    fpOut << buf;

    QString dividers("***************************   *******************************************");
    if (m_statisticsResults->outputLidarData()) {
      dividers += "   *******************************************";
    }
    snprintf(buf, sizeof(buf),"%*s\n", dividers.length() + 1 + filePadding, dividers.toLatin1().data());
    fpOut << buf;

    QString fields("|  Accepted  |   Total    |   |   Samples   |    Lines    |    Total    |");
    if (m_statisticsResults->outputLidarData()) {
      fields += "   |   Samples   |    Lines    |    Total    |";
    }
    snprintf(buf, sizeof(buf),"%*s\n", fields.length() + 1 + filePadding, fields.toLatin1().data());
    fpOut << buf;

    int numMeasures, numLidarMeasures;
    int numRejectedMeasures, numLidarRejectedMeasures;
    int numUsed, numLidarUsed;
    int imageIndex = 0;
    Statistics rmsSamplesTotal,rmsLinesTotal,rmsTotals;

    for (int i = 0; i < numObservations; i++) {

      int numImagesInObservation = m_statisticsResults->observations().at(i)->size();

      for (int j = 0; j < numImagesInObservation; j++) {

        BundleImageQsp bundleImage = m_statisticsResults->observations().at(i)->at(j);

        double rmsSampleResiduals = m_statisticsResults->
                                        rmsImageSampleResiduals()[imageIndex].Rms();
        double rmsLineResiduals =   m_statisticsResults->
                                        rmsImageLineResiduals()[imageIndex].Rms();
        double rmsLandSResiduals =  m_statisticsResults->
                                        rmsImageResiduals()[imageIndex].Rms();
        rmsSamplesTotal.AddData(rmsSampleResiduals);
        rmsLinesTotal.AddData(rmsLineResiduals);
        rmsTotals.AddData(rmsLandSResiduals);

        numMeasures = m_statisticsResults->outputControlNet()->GetNumberOfValidMeasuresInImage
            (bundleImage->serialNumber());

        numRejectedMeasures = m_statisticsResults->outputControlNet()->
            GetNumberOfJigsawRejectedMeasuresInImage(bundleImage->serialNumber());

        numUsed = numMeasures - numRejectedMeasures;

        QString filename = bundleImage->fileName();
        QStringList List;
        List = filename.split("/");

        snprintf(buf, sizeof(buf),"%-*s" ,filePadding + 1, bundleImage->fileName().toLatin1().data());
        fpOut << buf;

        snprintf(buf, sizeof(buf), " %12d %12d     ", numUsed, numMeasures);
        fpOut << buf;

        snprintf(buf, sizeof(buf),"%13.4lf %13.4lf %13.4lf",
                rmsSampleResiduals,rmsLineResiduals,rmsLandSResiduals);

        fpOut << buf;

        if (m_statisticsResults->outputLidarData()) {
          double rmsLidarSampleResiduals = m_statisticsResults->
                                          rmsLidarImageSampleResiduals()[imageIndex].Rms();
          double rmsLidarLineResiduals = m_statisticsResults->
                                        rmsLidarImageLineResiduals()[imageIndex].Rms();
          double rmsLidarLandSResiduals =  m_statisticsResults->
                                          rmsLidarImageResiduals()[imageIndex].Rms();

          numLidarMeasures = m_statisticsResults->outputLidarData()->
                             GetNumberOfValidMeasuresInImage(bundleImage->serialNumber());

          numLidarRejectedMeasures = m_statisticsResults->outputLidarData()->
                             GetNumberOfJigsawRejectedMeasuresInImage(bundleImage->serialNumber());

          numLidarUsed = numLidarMeasures - numLidarRejectedMeasures;

          snprintf(buf, sizeof(buf), " %12d %12d     ", numLidarUsed, numLidarMeasures);
          fpOut << buf;

          snprintf(buf, sizeof(buf),"%13.4lf %13.4lf %13.4lf",
                  rmsLidarSampleResiduals,rmsLidarLineResiduals,rmsLidarLandSResiduals);

          fpOut << buf;
        }

        snprintf(buf, sizeof(buf), " \n");
        fpOut << buf;
        imageIndex++;
      }
    }

    // Do something similar to above but left justify the string and add a 33
    // character buffer
    snprintf(buf, sizeof(buf),"%*s", -(filePadding + 33), "\nTotal RMS:");
    fpOut << buf;
    snprintf(buf, sizeof(buf),"%13.4lf %13.4lf %13.4lf\n",
    rmsSamplesTotal.Rms(),rmsLinesTotal.Rms(),rmsTotals.Rms());
    fpOut << buf;

    return true;
  }


  /**
   * @brief Outputs the bundleout_images.csv file which contains Jigsaw data about the images
   * within each observation.
   * @return True upon success, False if something went wrong.
   *
   * @internal
   *   @history 2016-12-01 Ian Humphrey - Added %s as second argument to sprintf() call to prevent
   *                           -Wformat-security warning. Since image->fileName().toLatin1().data()
   *                           returns a char* at runtime, the compiler does not know if it will
   *                           contain format specifiers and produces the mentioned warning.
   */
  bool BundleSolutionInfo::outputImagesCSV() {

    char buf[1056];

    QList<Statistics> rmsImageSampleResiduals = m_statisticsResults->rmsImageSampleResiduals();
    QList<Statistics> rmsImageLineResiduals = m_statisticsResults->rmsImageLineResiduals();
    QList<Statistics> rmsImageResiduals = m_statisticsResults->rmsImageResiduals();

    bool errorProp = false;
    if (m_statisticsResults->converged() && m_settings->errorPropagation()) {
      errorProp = true;
    }

    QList<QString> outputCsvFileNames;
    QList<QString> instrumentIds = m_statisticsResults->observations().instrumentIds();
    // If there's just a single instrumentId just call it bundleout_images.csv
    if (instrumentIds.size() == 1) {
      QString ofname = "bundleout_images.csv";
      ofname = m_settings->outputFilePrefix() + ofname;
      m_csvSavedImagesFilename = ofname;
      outputCsvFileNames.push_back(ofname);
    }
    // Otherwise append the instrument IDs so it's bundleout_images_spacecraft_sensor.csv
    else {
      for (int i = 0; i < instrumentIds.size(); i++) {
        QString updatedInstrumentId = instrumentIds[i];
        // Replace and "/" or " " characters with "_" to make the filename safer
        // This line must be separate to avoid modifying the instrumentId in the list
        // we will iterate over later
        updatedInstrumentId.replace("/", "_").replace(" ", "_");
        QString ofname = "bundleout_images_" + updatedInstrumentId + ".csv";
        ofname = m_settings->outputFilePrefix() + ofname;
        m_csvSavedImagesFilename = ofname;
        outputCsvFileNames.push_back(ofname);
      }
    }

    for (int i = 0; i < instrumentIds.size(); i++) {

      std::ofstream fpOut(outputCsvFileNames[i].toLatin1().data(), std::ios::out);
      if (!fpOut) {
        return false;
      }

      QList<BundleObservationQsp> observations =
          m_statisticsResults->observations().observationsByInstId(instrumentIds[i]);

      int nObservations = observations.size();

      outputImagesCSVHeader(fpOut, observations.front());

      for (int j = 0; j < nObservations; j++ ) {
        BundleObservationQsp observation = observations[j];

        // We need the image index, not the observation index,
        // so count all of the images prior to this observation
        int observationIndex = observation->index();
        int imgIndex = 0;
        for (int obsIndex = 0; obsIndex < observationIndex; obsIndex++) {
          imgIndex += m_statisticsResults->observations().at(obsIndex)->size();
        }

        if(!observation) {
          continue;
        }

        int numImages = observation->size();

        for (int k = 0; k < numImages; k++) {
          BundleImageQsp image = observation->at(k);

          snprintf(buf, sizeof(buf), "%s", image->fileName().toLatin1().data());
          fpOut << buf;
          snprintf(buf, sizeof(buf),",");
          fpOut << buf;

          fpOut << toString(rmsImageSampleResiduals[imgIndex].Rms()).toLatin1().data();
          snprintf(buf, sizeof(buf),",");
          fpOut << buf;

          fpOut << toString(rmsImageLineResiduals[imgIndex].Rms()).toLatin1().data();
          snprintf(buf, sizeof(buf),",");
          fpOut << buf;

          fpOut << toString(rmsImageResiduals[imgIndex].Rms()).toLatin1().data();
          snprintf(buf, sizeof(buf),",");
          fpOut << buf;

          QString observationString =
              observation->bundleOutputCSV(errorProp);

          //Removes trailing commas
          if (observationString.right(1)==",") {
              observationString.truncate(observationString.length()-1);
          }

          fpOut << (const char*) observationString.toLatin1().data();

          snprintf(buf, sizeof(buf),"\n");
          fpOut << buf;
          imgIndex++;

        }
      }
      fpOut.close();
    }

    return true;
  }


  /**
   * Outputs a text file with the results of the BundleAdjust.
   *
   * @return @b bool If the text file was successfully output.
   */
  bool BundleSolutionInfo::outputText() {

    QString ofname = "bundleout.txt";
    ofname = m_settings->outputFilePrefix() + ofname;

    std::ofstream fpOut(ofname.toLatin1().data(), std::ios::out);
    if (!fpOut) {
      return false;
    }

    m_txtBundleOutputFilename = ofname;

    char buf[4096];
    BundleObservationQsp observation;

    int nObservations = m_statisticsResults->observations().size();

    outputHeader(fpOut);

    bool berrorProp = false;
    if (m_statisticsResults->converged() && m_settings->errorPropagation()) {
      berrorProp = true;
    }

    // output target body header if solving for target
    if (m_settings->solveTargetBody()) {
      snprintf(buf, sizeof(buf), "\nTARGET BODY\n==========================\n");
      fpOut << buf;

      snprintf(buf, sizeof(buf), "\n   Target         Initial            Total               "
                   "Final           Initial           Final\n"
                   "Parameter         Value            Correction           "
                   "Value             Accuracy          Accuracy\n");
      fpOut << buf;

      QString targetString =
          m_settings->bundleTargetBody()->formatBundleOutputString(berrorProp);
      fpOut << (const char*)targetString.toLatin1().data();
    }

    // output image exterior orientation header
    snprintf(buf, sizeof(buf), "\nIMAGE EXTERIOR ORIENTATION\n==========================\n");
    fpOut << buf;

    QMap<QString, QStringList> imagesAndParameters;

    if (m_settings->solveTargetBody()) {
      imagesAndParameters.insert( "target", m_settings->bundleTargetBody()->parameterList() );
    }

    for (int i = 0; i < nObservations; i++) {

      observation = m_statisticsResults->observations().at(i);
      if (!observation) {
        continue;
      }

      int numImages = observation->size();
      for (int j = 0; j < numImages; j++) {
        BundleImageQsp image = observation->at(j);
        snprintf(buf, sizeof(buf), "\nImage Full File Name: %s\n", image->fileName().toLatin1().data());
        fpOut << buf;
        snprintf(buf, sizeof(buf), "\nImage Serial Number: %s\n", image->serialNumber().toLatin1().data());
        fpOut << buf;

        snprintf(buf, sizeof(buf),"Image           Initial                 Total                  Final                                      Accuracy\n");
        fpOut << buf;
        snprintf(buf, sizeof(buf),"Parameter       Value                   Correction             Value                      Initial           Final           Units\n");
        fpOut << buf;

        snprintf(buf, sizeof(buf),"                                                                                          "
                    "***************************************\n");
        fpOut << buf;

        observation->bundleOutputString(fpOut,berrorProp);
        // Build list of images and parameters for correlation matrix.
        foreach ( QString image, observation->imageNames() ) {
          imagesAndParameters.insert( image, observation->parameterList() );
        }
      }
    }

    // Save list of images and their associated parameters for CorrelationMatrix to use in ice.
    m_statisticsResults->setCorrMatImgsAndParams(imagesAndParameters);

    // Save list of images and their associated parameters for CorrelationMatrix to use in ice.
    m_statisticsResults->setCorrMatImgsAndParams(imagesAndParameters);

    // output point uncertainty statistics if error propagation is on
    if (berrorProp) {
      snprintf(buf, sizeof(buf), "\n\n\nPOINTS UNCERTAINTY SUMMARY\n==========================\n\n");
      fpOut << buf;

      // Coordinate 1 (latitude or point x) summary
      QString coordName = surfacePointCoordName(m_settings->controlPointCoordTypeReports(),
                                                SurfacePoint::One);
      snprintf(buf, sizeof(buf), "RMS Sigma %s(m)%20.8lf\n", coordName.toLatin1().data(),
              m_statisticsResults->sigmaCoord1StatisticsRms());
      fpOut << buf;
      snprintf(buf, sizeof(buf), "MIN Sigma %s(m)%20.8lf at %s\n", coordName.toLatin1().data(),
              m_statisticsResults->minSigmaCoord1Distance().meters(),
              m_statisticsResults->minSigmaCoord1PointId().toLatin1().data());
      fpOut << buf;
      snprintf(buf, sizeof(buf), "MAX Sigma %s(m)%20.8lf at %s\n\n", coordName.toLatin1().data(),
              m_statisticsResults->maxSigmaCoord1Distance().meters(),
              m_statisticsResults->maxSigmaCoord1PointId().toLatin1().data());
      fpOut << buf;

      // Coordinate 2 (longitude or point y) summary
      coordName = surfacePointCoordName(m_settings->controlPointCoordTypeReports(),
                                        SurfacePoint::Two);
      snprintf(buf, sizeof(buf), "RMS Sigma %s(m)%20.8lf\n", coordName.toLatin1().data(),
              m_statisticsResults->sigmaCoord2StatisticsRms());
      fpOut << buf;
      snprintf(buf, sizeof(buf), "MIN Sigma %s(m)%20.8lf at %s\n", coordName.toLatin1().data(),
              m_statisticsResults->minSigmaCoord2Distance().meters(),
              m_statisticsResults->minSigmaCoord2PointId().toLatin1().data());
      fpOut << buf;
      snprintf(buf, sizeof(buf), "MAX Sigma %s(m)%20.8lf at %s\n\n", coordName.toLatin1().data(),
              m_statisticsResults->maxSigmaCoord2Distance().meters(),
              m_statisticsResults->maxSigmaCoord2PointId().toLatin1().data());
      fpOut << buf;

      // Coordinate 3 (radius or point z) summary
      coordName = surfacePointCoordName(m_settings->controlPointCoordTypeReports(),
                                        SurfacePoint::Three);

      if (m_settings->controlPointCoordTypeBundle() == SurfacePoint::Latitudinal &&
          m_settings->controlPointCoordTypeReports() == SurfacePoint::Latitudinal &&
          m_settings->solveRadius() == false ) {
            snprintf(buf, sizeof(buf), "   RMS Sigma Radius(m)                 N/A\n");
            fpOut << buf;
            snprintf(buf, sizeof(buf), "   MIN Sigma Radius(m)                 N/A\n");
            fpOut << buf;
            snprintf(buf, sizeof(buf), "   MAX Sigma Radius(m)                 N/A\n");
            fpOut << buf;
      }
      else {
          snprintf(buf, sizeof(buf), "RMS Sigma %s(m)%20.8lf\n", coordName.toLatin1().data(),
                  m_statisticsResults->sigmaCoord3StatisticsRms());
          fpOut << buf;
          snprintf(buf, sizeof(buf), "MIN Sigma %s(m)%20.8lf at %s\n", coordName.toLatin1().data(),
                  m_statisticsResults->minSigmaCoord3Distance().meters(),
                  m_statisticsResults->minSigmaCoord3PointId().toLatin1().data());
          fpOut << buf;
          snprintf(buf, sizeof(buf), "MAX Sigma %s(m)%20.8lf at %s\n", coordName.toLatin1().data(),
                  m_statisticsResults->maxSigmaCoord3Distance().meters(),
                  m_statisticsResults->maxSigmaCoord3PointId().toLatin1().data());
          fpOut << buf;
      }
    }

    // output point summary data header
    if (m_settings->controlPointCoordTypeReports() == SurfacePoint::Latitudinal) {
      snprintf(buf, sizeof(buf), "\n\nPOINTS SUMMARY\n==============\n%103s"
            "Sigma          Sigma              Sigma\n"
            "           Label         Status     Rays    RMS"
            "        Latitude       Longitude          Radius"
            "        Latitude       Longitude          Radius\n", "");
    }
    else {  // Must be Rectangular
      snprintf(buf, sizeof(buf), "\n\nPOINTS SUMMARY\n==============\n%103s"
            "Sigma          Sigma              Sigma\n"
            "           Label         Status     Rays    RMS"
            "         Point X            Point Y          Point Z"
            "         Point X            Point Y          Point Z\n", "");
    }
    fpOut << buf;

    int nPoints = m_statisticsResults->bundleControlPoints().size();
    for (int i = 0; i < nPoints; i++) {
      BundleControlPointQsp bundleControlPoint = m_statisticsResults->bundleControlPoints().at(i);

      QString pointSummaryString =
          bundleControlPoint->formatBundleOutputSummaryString(berrorProp);
      fpOut << (const char*)pointSummaryString.toLatin1().data();
    }

    int nLidarPoints = m_statisticsResults->bundleLidarControlPoints().size();
    for (int i = 0; i < nLidarPoints; i++) {
      BundleLidarControlPointQsp lidarControlPoint
          = m_statisticsResults->bundleLidarControlPoints().at(i);

      QString pointSummaryString =
          lidarControlPoint->formatBundleOutputSummaryString(berrorProp);
      fpOut << (const char*)pointSummaryString.toLatin1().data();
    }

    // output point detail data header
    snprintf(buf, sizeof(buf), "\n\nPOINTS DETAIL\n=============\n\n");
    fpOut << buf;

    bool solveRadius = m_settings->solveRadius();

    for (int i = 0; i < nPoints; i++) {
      BundleControlPointQsp bundleControlPoint = m_statisticsResults->bundleControlPoints().at(i);

      // Removed radiansToMeters argument 9/18/2018 DAC
      QString pointDetailString =
          bundleControlPoint->formatBundleOutputDetailString(berrorProp, solveRadius);
      fpOut << (const char*)pointDetailString.toLatin1().data();
    }

    for (int i = 0; i < nLidarPoints; i++) {
      BundleLidarControlPointQsp bundleLidarControlPoint =
          m_statisticsResults->bundleLidarControlPoints().at(i);

      QString pointDetailString =
          bundleLidarControlPoint->formatBundleOutputDetailString(berrorProp, solveRadius);
      fpOut << (const char*)pointDetailString.toLatin1().data();
    }

    fpOut.close();

    return true;
  }


  /**
   * Outputs point data to a csv file.
   *
   * @return @b bool If the point data was successfully output.
   */
  bool BundleSolutionInfo::outputPointsCSV() {
    char buf[1056];

    QString ofname = "bundleout_points.csv";
    ofname = m_settings->outputFilePrefix() + ofname;
    m_csvSavedPointsFilename = ofname;

    std::ofstream fpOut(ofname.toLatin1().data(), std::ios::out);
    if (!fpOut) {
      return false;
    }

    int numPoints = m_statisticsResults->bundleControlPoints().size();

    double dLat, dLon, dRadius;
    double dX, dY, dZ;
    double dSigmaLat, dSigmaLong, dSigmaRadius;
    QString strStatus;
    double cor_lat_dd = 0.0;                      // lat correction, decimal degrees
    double cor_lon_dd = 0.0;                      // lon correction, decimal degrees
    double cor_rad_km = 0.0;                      // radius correction, kilometers
    double cor_lat_m = 0.0;                       // lat correction, meters
    double cor_lon_m = 0.0;                       // lon correction, meters
    double cor_rad_m = 0.0;                       // radius correction, meters
    double latInit = Isis::Null;
    double lonInit = Isis::Null;
    double radInit = Isis::Null;
    int numMeasures, numRejectedMeasures;
    double dResidualRms;

    // print column headers
    if (m_settings->errorPropagation()) {
      snprintf(buf, sizeof(buf), ",,,,,3-d,3-d,3-d,Sigma,Sigma,Sigma,Correction,Correction,Correction,Coordinate,"
              "Coordinate,Coordinate\nPoint,Point,Accepted,Rejected,Residual,Latitude,Longitude,"
              "Radius,Latitude,Longitude,Radius,Latitude,Longitude,Radius,X,Y,Z\nLabel,Status,"
              "Measures,Measures,RMS,(dd),(dd),(km),(m),(m),(m),(m),(m),(m),(km),(km),(km)\n");
    }
    else {
      snprintf(buf, sizeof(buf), ",,,,,3-d,3-d,3-d,Correction,Correction,Correction,Coordinate,Coordinate,"
              "Coordinate\nPoint,Point,Accepted,Rejected,Residual,Latitude,Longitude,Radius,"
              "Latitude,Longitude,Radius,X,Y,Z\nLabel,Status,Measures,Measures,RMS,(dd),(dd),(km),"
              "(m),(m),(m),(km),(km),(km)\n");
    }
    fpOut << buf;

    for (int i = 0; i < numPoints; i++) {
      BundleControlPointQsp bundlecontrolpoint = m_statisticsResults->bundleControlPoints().at(i);

      if (!bundlecontrolpoint) {
        continue;
      }

      if (bundlecontrolpoint->isRejected()) {
        continue;
      }

      dLat              = bundlecontrolpoint->adjustedSurfacePoint().GetLatitude().degrees();
      dLon              = bundlecontrolpoint->adjustedSurfacePoint().GetLongitude().degrees();
      dRadius           = bundlecontrolpoint->adjustedSurfacePoint().GetLocalRadius().kilometers();
      dX                = bundlecontrolpoint->adjustedSurfacePoint().GetX().kilometers();
      dY                = bundlecontrolpoint->adjustedSurfacePoint().GetY().kilometers();
      dZ                = bundlecontrolpoint->adjustedSurfacePoint().GetZ().kilometers();
      numMeasures         = bundlecontrolpoint->numberOfMeasures();
      numRejectedMeasures = bundlecontrolpoint->numberOfRejectedMeasures();
      dResidualRms      = bundlecontrolpoint->residualRms();

      // Use the local radius in meters, rad*1000., to convert radians to meters now instead of the
      // target body equatorial radius (DAC 09/17/2018; from BundleControlPoint.cpp)
      double rtm = dRadius * 1000.;

      // point corrections and initial sigmas
      boost::numeric::ublas::bounded_vector< double, 3 > corrections = bundlecontrolpoint->
                                                                            corrections();

      if (m_settings->controlPointCoordTypeBundle() == SurfacePoint::Rectangular) {
        double xCor = corrections(0);  // km
        double yCor = corrections(1);  // km
        double zCor = corrections(2);  // km

        if (!IsSpecial(dX) && !IsSpecial(dY) && !IsSpecial(dZ)) {
          SurfacePoint rectPoint(Displacement(dX - xCor, Displacement::Kilometers),
                                Displacement(dY - yCor, Displacement::Kilometers),
                                Displacement(dZ - zCor, Displacement::Kilometers));
          
          latInit = rectPoint.GetLatitude().degrees();
          lonInit = rectPoint.GetLongitude().degrees();
          radInit = rectPoint.GetLocalRadius().kilometers();
          
          if (!IsSpecial(dLat)) {
            cor_lat_dd = (dLat - latInit); // degrees
            cor_lat_m  =  cor_lat_dd * DEG2RAD * rtm;
          }
          if (!IsSpecial(dLon)) {
            cor_lon_dd = (dLon - lonInit); // degrees
            cor_lon_m  =  cor_lon_dd * DEG2RAD * rtm * cos(dLat*DEG2RAD);  // lon corrections meters
          }
          if (!IsSpecial(dRadius)) {
            cor_rad_km  =  dRadius - radInit;
            cor_rad_m  =  cor_rad_km * 1000.;
          }
        }
      }
      else if (m_settings->controlPointCoordTypeBundle() == SurfacePoint::Latitudinal) {
        cor_lat_dd = corrections(0) * RAD2DEG;   // lat correction, decimal degs
        cor_lon_dd = corrections(1) * RAD2DEG;   // lon correction, decimal degs
        cor_rad_m  = corrections(2) * 1000.0;    // radius correction, meters

        cor_lat_m = bundlecontrolpoint->adjustedSurfacePoint().LatitudeToMeters(corrections(0));
        cor_lon_m = bundlecontrolpoint->adjustedSurfacePoint().LongitudeToMeters(corrections(1));
        cor_rad_km = corrections(2);

        if (!IsSpecial(dLat)) {
          latInit = dLat - cor_lat_dd;
        }

        if (!IsSpecial(dLon)) {
          lonInit = dLon - cor_lon_dd;
        }

        if (!IsSpecial(dRadius)) {
          radInit = dRadius - corrections(2); // km
        }
      }

      if (bundlecontrolpoint->type() == ControlPoint::Fixed) {
        strStatus = "FIXED";
      }
      else if (bundlecontrolpoint->type() == ControlPoint::Constrained) {
        strStatus = "CONSTRAINED";
      }
      else if (bundlecontrolpoint->type() == ControlPoint::Free) {
        strStatus = "FREE";
      }
      else {
        strStatus = "UNKNOWN";
      }

      if (m_settings->errorPropagation()) {
        dSigmaLat = bundlecontrolpoint->adjustedSurfacePoint().GetLatSigmaDistance().meters();
        dSigmaLong = bundlecontrolpoint->adjustedSurfacePoint().GetLonSigmaDistance().meters();
        dSigmaRadius = bundlecontrolpoint->adjustedSurfacePoint().GetLocalRadiusSigma().meters();

        snprintf(buf, sizeof(buf), "%s,%s,%d,%d,%6.2lf,%16.8lf,%16.8lf,%16.8lf,%16.8lf,%16.8lf,%16.8lf,%16.8lf,"
                     "%16.8lf,%16.8lf,%16.8lf,%16.8lf,%16.8lf\n",
                bundlecontrolpoint->id().toLatin1().data(), strStatus.toLatin1().data(),
                numMeasures, numRejectedMeasures, dResidualRms, dLat, dLon, dRadius, dSigmaLat,
                dSigmaLong, dSigmaRadius, cor_lat_m, cor_lon_m, cor_rad_m, dX, dY, dZ);
      }
      else
        snprintf(buf, sizeof(buf), "%s,%s,%d,%d,%6.2lf,%16.8lf,%16.8lf,%16.8lf,%16.8lf,%16.8lf,%16.8lf,%16.8lf,"
                     "%16.8lf,%16.8lf\n",
                bundlecontrolpoint->id().toLatin1().data(), strStatus.toLatin1().data(),
                numMeasures, numRejectedMeasures, dResidualRms, dLat, dLon, dRadius, cor_lat_m,
                cor_lon_m, cor_rad_m, dX, dY, dZ);

      fpOut << buf;
    }

    fpOut.close();

    return true;
  }


  /**
   * Outputs lidar data to a csv file.
   *
   * @return bool If the point data was successfully output.
   */
  bool BundleSolutionInfo::outputLidarCSV() {
    char buf[1056];

    QString ofname = "bundleout_lidar.csv";
    ofname = m_settings->outputFilePrefix() + ofname;
    m_csvSavedPointsFilename = ofname;

    std::ofstream fpOut(ofname.toLatin1().data(), std::ios::out);
    if (!fpOut) {
      return false;
    }

    int numPoints = m_statisticsResults->bundleLidarControlPoints().size();

    //                     measured   apriori   adjusted               adjusted
    //                      range      sigma     range      residual     sigma
    // point id  image       (km)       (km)      (km)        (km)       (km)

    // print column headers
    if (m_settings->errorPropagation()) {
      snprintf(buf, sizeof(buf), ",,measured,a priori,adjusted,adjusted\n"
              "point,image,range,sigma,range,sigma,residual\n"
              "id,name,(km),(km),(km),(km),(km)\n");
    }
    else {
      snprintf(buf, sizeof(buf), ",,measured,a priori,adjusted\n"
                   "point,image,range,sigma,range,residual\n"
                   "id,name,(km),(km),(km),(km)\n");
    }
    fpOut << buf;

    for (int i = 0; i < numPoints; i++) {

      BundleLidarControlPointQsp point = m_statisticsResults->bundleLidarControlPoints().at(i);
      if (!point || point->isRejected()) {
        continue;
      }

      int nRangeConstraints = point->numberRangeConstraints();
      for (int j = 0; j < nRangeConstraints; j++) {
        BundleLidarRangeConstraintQsp rangeConstraint = point->rangeConstraint(j);

        QString str = rangeConstraint->formatBundleOutputString(m_settings->errorPropagation());
        fpOut << str;
      }
    }

    fpOut.close();

    return true;
  }


  /**
   * Outputs image coordinate residuals to a csv file.
   *
   * @return @b bool If the residuals were successfully output.
   */
  bool BundleSolutionInfo::outputResiduals() {
    char buf[1056];

    QString ofname = "residuals.csv";
    ofname = m_settings->outputFilePrefix() + ofname;
    m_csvSavedResidualsFilename = ofname;

    std::ofstream fpOut(ofname.toLatin1().data(), std::ios::out);
    if (!fpOut) {
      return false;
    }

    // output column headers

    snprintf(buf, sizeof(buf), ",,,x image,y image,Measured,Measured,sample,line,Residual Vector\n");
    fpOut << buf;
    snprintf(buf, sizeof(buf), "Point,Image,Image,coordinate,coordinate,"
                 "Sample,Line,residual,residual,Magnitude\n");
    fpOut << buf;
    snprintf(buf, sizeof(buf), "Label,Filename,Serial Number,(mm),(mm),"
                 "(pixels),(pixels),(pixels),(pixels),(pixels),Rejected\n");
    fpOut << buf;

    // Setup counts and pointers

    int numPoints = m_statisticsResults->bundleControlPoints().size();
    int numMeasures = 0;

    BundleControlPointQsp bundleControlPoint;
    BundleMeasureQsp bundleMeasure;

    for (int i = 0; i < numPoints; i++) {
      bundleControlPoint = m_statisticsResults->bundleControlPoints().at(i);
      numMeasures = bundleControlPoint->size();

      if (bundleControlPoint->rawControlPoint()->IsIgnored()) {
        continue;
      }

      for (int j = 0; j < numMeasures; j++) {
        bundleMeasure = bundleControlPoint->at(j);

        Camera *measureCamera = bundleMeasure->camera();
        if (!measureCamera) {
          continue;
        }

        if (bundleMeasure->isRejected()) {
          snprintf(buf, sizeof(buf), "%s,%s,%s,%16.8lf,%16.8lf,%16.8lf,%16.8lf,%16.8lf,%16.8lf,%16.8lf,*\n",
                  bundleControlPoint->id().toLatin1().data(),
                  bundleMeasure->parentBundleImage()->fileName().toLatin1().data(),
                  bundleMeasure->cubeSerialNumber().toLatin1().data(),
                  bundleMeasure->focalPlaneMeasuredX(),
                  bundleMeasure->focalPlaneMeasuredY(),
                  bundleMeasure->sample(),
                  bundleMeasure->line(),
                  bundleMeasure->sampleResidual(),
                  bundleMeasure->lineResidual(),
                  bundleMeasure->residualMagnitude());
        }
        else {
          snprintf(buf, sizeof(buf), "%s,%s,%s,%16.8lf,%16.8lf,%16.8lf,%16.8lf,%16.8lf,%16.8lf,%16.8lf\n",
                  bundleControlPoint->id().toLatin1().data(),
                  bundleMeasure->parentBundleImage()->fileName().toLatin1().data(),
                  bundleMeasure->cubeSerialNumber().toLatin1().data(),
                  bundleMeasure->focalPlaneMeasuredX(),
                  bundleMeasure->focalPlaneMeasuredY(),
                  bundleMeasure->sample(),
                  bundleMeasure->line(),
                  bundleMeasure->sampleResidual(),
                  bundleMeasure->lineResidual(),
                  bundleMeasure->residualMagnitude());
        }
        fpOut << buf;
      }
    }

    numPoints = m_statisticsResults->bundleLidarControlPoints().size();
    numMeasures = 0;

    BundleLidarControlPointQsp bundleLidarPoint;

    for (int i = 0; i < numPoints; i++) {
      bundleLidarPoint = m_statisticsResults->bundleLidarControlPoints().at(i);
      numMeasures = bundleLidarPoint->size();

      if (bundleLidarPoint->rawControlPoint()->IsIgnored()) {
        continue;
      }

      for (int j = 0; j < numMeasures; j++) {
        bundleMeasure = bundleLidarPoint->at(j);

        Camera *measureCamera = bundleMeasure->camera();
        if (!measureCamera) {
          continue;
        }

        if (bundleMeasure->isRejected()) {
          snprintf(buf, sizeof(buf), "%s,%s,%s,%16.8lf,%16.8lf,%16.8lf,%16.8lf,%16.8lf,%16.8lf,%16.8lf,*\n",
                  bundleLidarPoint->id().toLatin1().data(),
                  bundleMeasure->parentBundleImage()->fileName().toLatin1().data(),
                  bundleMeasure->cubeSerialNumber().toLatin1().data(),
                  bundleMeasure->focalPlaneMeasuredX(),
                  bundleMeasure->focalPlaneMeasuredY(),
                  bundleMeasure->sample(),
                  bundleMeasure->line(),
                  bundleMeasure->sampleResidual(),
                  bundleMeasure->lineResidual(),
                  bundleMeasure->residualMagnitude());
        }
        else {
          snprintf(buf, sizeof(buf), "%s,%s,%s,%16.8lf,%16.8lf,%16.8lf,%16.8lf,%16.8lf,%16.8lf,%16.8lf\n",
                  bundleLidarPoint->id().toLatin1().data(),
                  bundleMeasure->parentBundleImage()->fileName().toLatin1().data(),
                  bundleMeasure->cubeSerialNumber().toLatin1().data(),
                  bundleMeasure->focalPlaneMeasuredX(),
                  bundleMeasure->focalPlaneMeasuredY(),
                  bundleMeasure->sample(),
                  bundleMeasure->line(),
                  bundleMeasure->sampleResidual(),
                  bundleMeasure->lineResidual(),
                  bundleMeasure->residualMagnitude());
        }
        fpOut << buf;
      }
    }

    fpOut.close();

    return true;
  }


  /**
   * Saves the BundleSolutionInfo to the project
   *
   * Output format:
   *
   *
   * <image id="..." fileName="...">
   *   ...
   * </image>
   *
   * (fileName attribute is just the base name)
   *
   * @param stream The stream to which the BundleSolutionInfo will be saved
   * @param project The project to which this BundleSolutionInfo will be saved
   * @param newProjectRoot The location of the project root directory.
   */
  void BundleSolutionInfo::save(QXmlStreamWriter &stream, const Project *project,
                                FileName newProjectRoot) const {

    // This is done for testing serialization without a Project
    QString relativePath;
    QString relativeBundlePath;
    FileName bundleSolutionInfoRoot;

    if (project) {
      bundleSolutionInfoRoot = FileName(Project::bundleSolutionInfoRoot(QString::fromStdString(newProjectRoot.expanded())).toStdString() +
                                      "/" + runTime().toStdString());
      QString oldPath = project->bundleSolutionInfoRoot(project->projectRoot()) + "/" + runTime();
      QString newPath = project->bundleSolutionInfoRoot(QString::fromStdString(newProjectRoot.toString())) + "/" + runTime();
      //  If project is being saved to new area, create directory and copy files
      if (oldPath != newPath) {
        //  Create project folder for BundleSolutionInfo
        QDir bundleDir(newPath);
        if (!bundleDir.mkpath(bundleDir.path())) {
          throw IException(IException::Io,"Failed to create directory [" + bundleSolutionInfoRoot.path() + "]",
                           _FILEINFO_);
        }
        QString oldFile = oldPath + "/" + QString::fromStdString(FileName(m_outputControl->fileName().toStdString()).name());
        QString newFile = newPath + "/" + QString::fromStdString(FileName(m_outputControl->fileName().toStdString()).name());
        if (!QFile::copy(oldFile, newFile)) {
          throw IException(IException::Io,
                           QString("Failed to copy file [%1] to new file [%2]")
                             .arg(m_outputControl->fileName()).arg(newFile),
                           _FILEINFO_);
        }
        newFile = newPath + "/" + QString::fromStdString(FileName(m_txtBundleOutputFilename.toStdString()).name());
        if (!QFile::copy(m_txtBundleOutputFilename, newFile)) {
          throw IException(IException::Io,
                           QString("Failed to copy file [%1] to new file [%2]")
                             .arg(m_txtBundleOutputFilename).arg(newFile),
                           _FILEINFO_);
        }
        newFile = newPath + "/" + QString::fromStdString(FileName(m_csvSavedImagesFilename.toStdString()).name());
        if (!QFile::copy(m_csvSavedImagesFilename, newFile)) {
          throw IException(IException::Io,
                           QString("Failed to copy file [%1] to new file [%2]")
                             .arg(m_csvSavedImagesFilename).arg(newFile),
                           _FILEINFO_);
        }
        newFile = newPath + "/" + QString::fromStdString(FileName(m_csvSavedPointsFilename.toStdString()).name());
        if (!QFile::copy(m_csvSavedPointsFilename, newFile)) {
          throw IException(IException::Io,
                           QString("Failed to copy file [%1] to new file [%2]")
                             .arg(m_csvSavedPointsFilename).arg(newFile),
                           _FILEINFO_);
        }
        newFile = newPath + "/" + QString::fromStdString(FileName(m_csvSavedResidualsFilename.toStdString()).name());
        if (!QFile::copy(m_csvSavedResidualsFilename, newFile)) {
          throw IException(IException::Io,
                           QString("Failed to copy file [%1] to new file [%2]")
                             .arg(m_csvSavedResidualsFilename).arg(newFile),
                           _FILEINFO_);
        }
      }

      // Create relativePath
      relativePath = QString::fromStdString(m_inputControlNetFileName->expanded()).remove(project->newProjectRoot());
      // Get rid of any preceding "/" , but add on ending "/"
      if (relativePath.startsWith("/")) {
        relativePath.remove(0,1);
      }

      // Create relativeBundlePath for bundleSolutionInfo
      relativeBundlePath = newPath.remove(project->newProjectRoot());
      // Get rid of any preceding "/" , but add on ending "/"
      if (relativeBundlePath.startsWith("/")) {
        relativeBundlePath.remove(0,1);
      }
      relativeBundlePath += "/";
    }

    stream.writeStartElement("bundleSolutionInfo");
    // save ID, cnet file name, and run time to stream
    stream.writeStartElement("generalAttributes");
    stream.writeTextElement("id", m_id->toString());
    stream.writeTextElement("name", m_name);
    stream.writeTextElement("runTime", runTime());

    stream.writeTextElement("inputFileName",
                            relativePath);
    stream.writeTextElement("bundleOutTXT",
                            relativeBundlePath + QString::fromStdString(FileName(m_txtBundleOutputFilename.toStdString()).name()));
    stream.writeTextElement("imagesCSV",
                            relativeBundlePath + QString::fromStdString(FileName(m_csvSavedImagesFilename.toStdString()).name()));
    stream.writeTextElement("pointsCSV",
                            relativeBundlePath + QString::fromStdString(FileName(m_csvSavedPointsFilename.toStdString()).name()));
    stream.writeTextElement("residualsCSV",
                            relativeBundlePath + QString::fromStdString(FileName(m_csvSavedResidualsFilename.toStdString()).name()));
    stream.writeEndElement(); // end general attributes

    // save settings to stream
    m_settings->save(stream, project);

    // save statistics to stream
    m_statisticsResults->save(stream, project);

    if (project) {
      // save adjusted images lists to stream
      if (!m_adjustedImages->isEmpty()) {
        stream.writeStartElement("imageLists");
        for (int i = 0; i < m_adjustedImages->count(); i++) {
          m_adjustedImages->at(i)->save(stream, project, bundleSolutionInfoRoot);
        }
        stream.writeEndElement();
      }

      // save output control
      stream.writeStartElement("outputControl");
      m_outputControl->save(stream, project, relativeBundlePath.toStdString());
      stream.writeEndElement();
    }

    stream.writeEndElement(); //end bundleSolutionInfo
  }


  /**
   * Determine the control point coordinate name.
   *
   * @param type The control point coordinate type (see SurfacePoint.h)
   * @param coordIdx The coordinate index (see SurfacePoint.h)
   *
   * @return @b QString Coordinate name
   */
  QString BundleSolutionInfo::surfacePointCoordName(SurfacePoint::CoordinateType type,
                                                 SurfacePoint::CoordIndex coordIdx) const {
    QString coordName;
    switch (m_settings->controlPointCoordTypeReports()) {
      case SurfacePoint::Latitudinal:
        switch (coordIdx) {
          case SurfacePoint::One:
            coordName = " Latitude";
            break;
          case SurfacePoint::Two:
            coordName = "Longitude";
            break;
          case SurfacePoint::Three:
            coordName = "   Radius";
            break;
          default:
            IString msg = "Unknown surface point index enum ["
              + toString(coordIdx) + "].";
            throw IException(IException::Programmer, msg, _FILEINFO_);
            break;
        }
        break;
      case SurfacePoint::Rectangular:
        switch (coordIdx) {
          case SurfacePoint::One:
            coordName = "POINT X";
            break;
          case SurfacePoint::Two:
            coordName = "POINT Y";
            break;
          case SurfacePoint::Three:
            coordName = "POINT Z";
            break;
          default:
            IString msg = "Unknown surface point index enum ["
              + toString(coordIdx) + "].";
            throw IException(IException::Programmer, msg, _FILEINFO_);
            break;
        }
        break;
    default:
      IString msg = "Unknown surface point coordinate type enum ["
        + toString(m_settings->controlPointCoordTypeReports()) + "].";
      throw IException(IException::Programmer, msg, _FILEINFO_);
      break;
    }
    return coordName;
  }
}