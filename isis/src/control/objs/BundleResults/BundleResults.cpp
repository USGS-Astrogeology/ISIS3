/** This is free and unencumbered software released into the public domain.

The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */

#include "BundleResults.h"

#include <QDataStream>
#include <QDebug>
#include <QString>
#include <QStringRef>
#include <QtGlobal> // qMax()
#include <QUuid>
#include <QXmlStreamWriter>
#include <QXmlStreamReader>

#include <boost/lexical_cast.hpp>
#include <boost/numeric/ublas/io.hpp>
#include <boost/numeric/ublas/matrix_sparse.hpp>
#include <boost/numeric/ublas/vector_proxy.hpp>

#include "Camera.h"
#include "ControlMeasure.h"
#include "ControlPoint.h"
#include "CorrelationMatrix.h"
#include "Distance.h"
#include "FileName.h"
#include "IString.h"
#include "MaximumLikelihoodWFunctions.h"
#include "Project.h"
#include "PvlKeyword.h"
#include "PvlObject.h"
#include "SerialNumberList.h"
#include "StatCumProbDistDynCalc.h"
#include "Statistics.h"

#include <iostream>

using namespace boost::numeric::ublas;

namespace Isis {

  /**
   * Constructs a BundleResults object.
   *
   * @param parent The Qt-relationship parent.
   */
  BundleResults::BundleResults(QObject *parent) : QObject(parent) {

    initialize();

    m_correlationMatrix = new CorrelationMatrix;
    m_cumPro = new StatCumProbDistDynCalc;
    m_cumProRes = new StatCumProbDistDynCalc;

    // residual prob distribution is calculated even if there is no maximum likelihood estimation.
    // so set up the solver to have a node at every percent of the distribution
    initializeResidualsProbabilityDistribution(101);

  }

  /**
   * Construct this BundleResults object from XML.
   *
   * @param bundleSettingsFolder Where the settings XML for this bundle adjustment
   *                             resides - /work/.../projectRoot/images/import1
   * @param xmlReader An XML reader that's up to a <bundleSettings/> tag.
   * @param parent The Qt-relationship parent.
   */
  BundleResults::BundleResults(QXmlStreamReader *xmlReader, 
                               QObject *parent) : QObject(parent)
  {
    // TODO: does xml stuff need project???

    initialize();

    readBundleResults(xmlReader);
  }

  void BundleResults::readBundleResults(QXmlStreamReader *xmlReader) {
    Q_ASSERT(xmlReader->name() == "bundleResults");
    while(xmlReader->readNextStartElement()) {
      if (xmlReader->qualifiedName() == "correlationMatrix") {
        readCorrelationMatrix(xmlReader);
      }
      else if (xmlReader->qualifiedName() == "generalStatisticsValues") {
        readGenStatsValues(xmlReader);
      }
      else if (xmlReader->qualifiedName() == "rms") {
        readRms(xmlReader);
      }
      else if (xmlReader->qualifiedName() == "elapsedTime") {

        QStringRef time = xmlReader->attributes().value("time");
        if (!time.isEmpty()) {
          m_elapsedTime = time.toDouble();
        }

        QStringRef errorProp = xmlReader->attributes().value("errorProp");
        if (!errorProp.isEmpty()) {
          m_elapsedTimeErrorProp = errorProp.toDouble();
        }
        xmlReader->skipCurrentElement();
      }
      else if (xmlReader->qualifiedName() == "minMaxSigmas") {
        readMinMaxSigmas(xmlReader);
      }
      else if (xmlReader->qualifiedName() == "maximumLikelihoodEstimation") {
        readMaxLikelihoodEstimation(xmlReader);
      }

      // ???      else if (xmlReader->qualifiedName() == "minMaxSigmaDistances") {
      // ???        QStringRef units = xmlReader->attributes().value("units");
      // ???        if (!QStringRef::compare(units, "meters", Qt::CaseInsensitive)) {
      // ???          QStringRef msg = "Unable to read BundleResults xml. Sigma distances must be "
      // ???                        "provided in meters.";
      // ???          throw IException(IException::Io, msg, _FILEINFO_);
      // ???        }
      // ???      }
      else {
        xmlReader->skipCurrentElement();
      }
    }
  }

  void BundleResults::readCorrelationMatrix(QXmlStreamReader *xmlReader) {
    Q_ASSERT(xmlReader->name() == "correlationMatrix");
    m_correlationMatrix = NULL;
    m_correlationMatrix = new CorrelationMatrix();

    QString correlationFileName = *(xmlReader->attributes().value("correlationFileName").string());
    if (!correlationFileName.isEmpty()) {
      FileName correlationFile(correlationFileName.toStdString());
      m_correlationMatrix->setCorrelationFileName(correlationFile);
    }

    QString covarianceFileName = *(xmlReader->attributes().value("covarianceFileName").string());
    if (!covarianceFileName.isEmpty()) {
      FileName covarianceFile(covarianceFileName.toStdString());
      m_correlationMatrix->setCovarianceFileName(covarianceFile);
    }
    while (xmlReader->readNextStartElement()) {
      if (xmlReader->qualifiedName() == "image") {
        QString correlationMatrixImageId = *(xmlReader->attributes().value("id").string());
        if (!correlationMatrixImageId.isEmpty()) {
          m_xmlHandlerCorrelationImageId = correlationMatrixImageId;
        }
      }
      else {
        xmlReader->skipCurrentElement();
      }
    }
  }

  void BundleResults::readGenStatsValues(QXmlStreamReader *xmlReader) {
    Q_ASSERT(xmlReader->name() == "generalStatisticsValues");
    while (xmlReader->readNextStartElement()) {
      if (xmlReader->qualifiedName() == "numberFixedPoints") {
        m_numberFixedPoints = xmlReader->readElementText().toInt();
      }
      else if (xmlReader->qualifiedName() == "numberIgnoredPoints") {
        m_numberIgnoredPoints = xmlReader->readElementText().toInt();
      }
      else if (xmlReader->qualifiedName() == "numberHeldImages") {
        m_numberHeldImages = xmlReader->readElementText().toInt();
      }
      else if (xmlReader->qualifiedName() == "rejectionLimit") {
        m_rejectionLimit = xmlReader->readElementText().toDouble();
      }
      else if (xmlReader->qualifiedName() == "numberRejectedObservations") {
        m_numberRejectedObservations = xmlReader->readElementText().toInt();
      }
      else if (xmlReader->qualifiedName() == "numberLidarRangeConstraintEquations") {
        m_numberLidarRangeConstraintEquations = xmlReader->readElementText().toInt();
      }
      else if (xmlReader->qualifiedName() == "numberObservations") {
        m_numberObservations = xmlReader->readElementText().toInt();
      }
      else if (xmlReader->qualifiedName() == "numberImageObservations") {
        m_numberImageObservations = xmlReader->readElementText().toInt();
      }
      else if (xmlReader->qualifiedName() == "numberLidarImageObservations") {
        m_numberLidarImageObservations = xmlReader->readElementText().toInt();
      }
      else if (xmlReader->qualifiedName() == "numberImageParameters") {
        m_numberImageParameters = xmlReader->readElementText().toInt();
      }
      else if (xmlReader->qualifiedName() == "numberConstrainedPointParameters") {
        m_numberConstrainedPointParameters =
                                       xmlReader->readElementText().toInt();
      }
      else if (xmlReader->qualifiedName() == "numberConstrainedImageParameters") {
        m_numberConstrainedImageParameters =
                                       xmlReader->readElementText().toInt();
      }
      else if (xmlReader->qualifiedName() == "numberConstrainedTargetParameters") {
        m_numberConstrainedTargetParameters =
                                       xmlReader->readElementText().toInt();
      }
      else if (xmlReader->qualifiedName() == "numberUnknownParameters") {
        m_numberUnknownParameters = xmlReader->readElementText().toInt();
      }
      else if (xmlReader->qualifiedName() == "degreesOfFreedom") {
        m_degreesOfFreedom = xmlReader->readElementText().toInt();
      }
      else if (xmlReader->qualifiedName() == "sigma0") {
        m_sigma0 = xmlReader->readElementText().toDouble();
      }
      else if (xmlReader->qualifiedName() == "converged") {
        m_converged = toBool(xmlReader->readElementText().toStdString());
      }
      else if (xmlReader->qualifiedName() == "iterations") {
        m_iterations = xmlReader->readElementText().toInt();
      }
      else {
        xmlReader->skipCurrentElement();
      }
    }
  }

  void BundleResults::readRms(QXmlStreamReader *xmlReader) {
    Q_ASSERT(xmlReader->name() == "rms");
    while (xmlReader->readNextStartElement()) {
      if (xmlReader->qualifiedName() == "residuals") {
        QStringRef rx = xmlReader->attributes().value("x");
        if (!rx.isEmpty()) {
          m_rmsXResiduals = rx.toDouble();
        }

        QStringRef ry = xmlReader->attributes().value("y");
        if (!ry.isEmpty()) {
          m_rmsYResiduals = ry.toDouble();
        }

        QStringRef rxy = xmlReader->attributes().value("xy");
        if (!rxy.isEmpty()) {
          m_rmsXYResiduals = rxy.toDouble();
        }
        xmlReader->skipCurrentElement();
      }
      else if (xmlReader->qualifiedName() == "sigmas")
      {
        QStringRef lat = xmlReader->attributes().value("lat");
        if (!lat.isEmpty()){
          m_rmsSigmaCoord1Stats = lat.toDouble();
        }

        QStringRef lon = xmlReader->attributes().value("lon");
        if (!lon.isEmpty()){
          m_rmsSigmaCoord2Stats = lon.toDouble();
        }

        QStringRef rad = xmlReader->attributes().value("rad");
        if (!rad.isEmpty()){
          m_rmsSigmaCoord3Stats = rad.toDouble();
        }

        QStringRef x = xmlReader->attributes().value("x");
        if (!x.isEmpty()){
          m_rmsSigmaCoord1Stats = x.toDouble();
        }

        QStringRef y = xmlReader->attributes().value("y");
        if (!y.isEmpty()){
          m_rmsSigmaCoord2Stats = y.toDouble();
        }

        QStringRef z = xmlReader->attributes().value("z");
        if (!z.isEmpty()){
          m_rmsSigmaCoord3Stats = z.toDouble();
        }
        xmlReader->skipCurrentElement();
      }
      else if (xmlReader->qualifiedName() == "imageResidualsLists") {
        readImageResidualsLists(xmlReader);
      }
      else if (xmlReader->qualifiedName() == "imageSigmasLists") {
        readSigmasLists(xmlReader);
      }
      else {
        xmlReader->skipCurrentElement();
      }
    }
  }

  void BundleResults::readImageResidualsLists(QXmlStreamReader *xmlReader) {
    Q_ASSERT(xmlReader->name() == "imageResidualsLists");
    while (xmlReader->readNextStartElement()) {
      if (xmlReader->qualifiedName() ==  "residualsList") {
        readStatsToList(m_rmsImageResiduals, xmlReader);
      }
      else if (xmlReader->qualifiedName() == "sampleList") {
        readStatsToList(m_rmsImageSampleResiduals, xmlReader);
      }
      else if (xmlReader->qualifiedName() == "lineList") {
        readStatsToList(m_rmsImageLineResiduals, xmlReader);
      }
      else if (xmlReader->qualifiedName() == "lidarResidualsList") {
        readStatsToList(m_rmsLidarImageResiduals, xmlReader);
      }
      else if (xmlReader->qualifiedName() == "lidarSampleList") {
        readStatsToList(m_rmsLidarImageSampleResiduals, xmlReader);
      }
      else if (xmlReader->qualifiedName() == "lidarLineList") {
        readStatsToList(m_rmsLidarImageLineResiduals, xmlReader);
      }
      else {
        xmlReader->skipCurrentElement();
      }
    }
  }

  void BundleResults::readSigmasLists(QXmlStreamReader *xmlReader) {
    Q_ASSERT(xmlReader->name() == "imageSigmasLists");
    while (xmlReader->readNextStartElement()) {
      if (xmlReader->qualifiedName() ==  "xSigmas") {
        readStatsToVector(m_rmsImageXSigmas, xmlReader);
      }
      else if (xmlReader->qualifiedName() == "ySigmas") {
        readStatsToVector(m_rmsImageYSigmas, xmlReader);
      }
      else if (xmlReader->qualifiedName() == "zSigmas") {
        readStatsToVector(m_rmsImageZSigmas, xmlReader);
      }
      else if (xmlReader->qualifiedName() == "raSigmas") {
        readStatsToVector(m_rmsImageRASigmas, xmlReader);
      }
      else if (xmlReader->qualifiedName() == "decSigmas") {
        readStatsToVector(m_rmsImageDECSigmas, xmlReader);
      }
      else if (xmlReader->qualifiedName() == "twistSigmas") {
        readStatsToVector(m_rmsImageTWISTSigmas, xmlReader);
      }
      else {
        xmlReader->skipCurrentElement();
      }
    }
  }

  void BundleResults::readStatsToList(QList<Statistics> &list, QXmlStreamReader *xmlReader) {
    Q_ASSERT(xmlReader->attributes().hasAttribute("listSize"));
    int listSize = xmlReader->attributes().value("listSize").toInt();
    for (int i = 0; i < listSize; i++) {
      xmlReader->readNextStartElement();
      Q_ASSERT(xmlReader->name() == "statisticsItem");
      xmlReader->readNextStartElement();
      Q_ASSERT(xmlReader->name() == "statistics");
      list.append(new Statistics(xmlReader));
      xmlReader->readNextStartElement();
    }
    xmlReader->readNextStartElement();
  }

  void BundleResults::readStatsToVector(QVector<Statistics> &vec, QXmlStreamReader *xmlReader) {
    Q_ASSERT(xmlReader->attributes().hasAttribute("listSize"));
    int listSize = xmlReader->attributes().value("listSize").toInt();
    for (int i = 0; i < listSize; i++) {
      xmlReader->readNextStartElement();
      Q_ASSERT(xmlReader->name() == "statisticsItem");
      xmlReader->readNextStartElement();
      Q_ASSERT(xmlReader->name() == "statistics");
      vec.append(new Statistics(xmlReader));
      xmlReader->readNextStartElement();
    }
    xmlReader->readNextStartElement();
  }

  void BundleResults::readMinMaxSigmas(QXmlStreamReader *xmlReader) {
    Q_ASSERT(xmlReader->name() == "minMaxSigmas");
    while (xmlReader->readNextStartElement()) {
      if (xmlReader->qualifiedName() == "minLat" ||
          xmlReader->qualifiedName() == "minX") {
        readSigma(m_minSigmaCoord1Distance, m_minSigmaCoord1PointId, xmlReader);
      }
      else if (xmlReader->qualifiedName() == "maxLat" || 
               xmlReader->qualifiedName() == "maxX") {
        readSigma(m_maxSigmaCoord1Distance, m_maxSigmaCoord1PointId, xmlReader);
      }
      else if (xmlReader->qualifiedName() == "minLon" || 
               xmlReader->qualifiedName() == "minY") {
        readSigma(m_minSigmaCoord2Distance, m_minSigmaCoord2PointId, xmlReader);
      }
      else if (xmlReader->qualifiedName() == "maxLon" || 
               xmlReader->qualifiedName() == "maxY") {
        readSigma(m_maxSigmaCoord2Distance, m_maxSigmaCoord2PointId, xmlReader);
      }
      else if (xmlReader->qualifiedName() == "minRad" || 
               xmlReader->qualifiedName() == "minZ") {
        readSigma(m_minSigmaCoord3Distance, m_minSigmaCoord3PointId, xmlReader);
      }
      else if (xmlReader->qualifiedName() == "maxRad" || 
               xmlReader->qualifiedName() == "maxZ") {
        readSigma(m_maxSigmaCoord3Distance, m_maxSigmaCoord3PointId, xmlReader);
      }
      else {
        xmlReader->skipCurrentElement();
      }
    } 
  }

  void BundleResults::readSigma(Distance &dist, QString &pointId, QXmlStreamReader *xmlReader) {
    Q_ASSERT(xmlReader->attributes().hasAttribute("value"));
    Q_ASSERT(xmlReader->attributes().hasAttribute("pointId"));
    QStringRef sigmaValue = xmlReader->attributes().value("value");
    if (!sigmaValue.isEmpty()) {
      dist.setMeters(sigmaValue.toDouble());
    }

    QString sigmaPointId = xmlReader->attributes().value("pointId").toString();
    if (!sigmaPointId.isEmpty()) {
      pointId = sigmaPointId;
    }
    xmlReader->skipCurrentElement();
  }

  void BundleResults::readMaxLikelihoodEstimation(QXmlStreamReader *xmlReader) {
    Q_ASSERT(xmlReader->name() == "maximumLikelihoodEstimation");
    QStringRef maximumLikelihoodIndex = xmlReader->attributes().value("maximumLikelihoodIndex");
    if (!maximumLikelihoodIndex.isEmpty()) {
      m_maximumLikelihoodIndex = maximumLikelihoodIndex.toInt();
    }

    QStringRef maximumLikelihoodMedianR2Residuals =
        xmlReader->attributes().value("maximumLikelihoodMedianR2Residuals");
    if (!maximumLikelihoodMedianR2Residuals.isEmpty()) {
      m_maximumLikelihoodMedianR2Residuals =
          maximumLikelihoodMedianR2Residuals.toDouble();
    }
    while (xmlReader->readNextStartElement()) {
      if (xmlReader->qualifiedName() == "cumulativeProbabilityCalculator") {
        m_cumPro = NULL;
        m_cumPro =  new StatCumProbDistDynCalc(xmlReader);
      }
      else if (xmlReader->qualifiedName() == "residualsCumulativeProbabilityCalculator") {
        m_cumProRes = NULL;
        m_cumProRes = new StatCumProbDistDynCalc();
        m_cumProRes->readStatistics(xmlReader);
      }
      else if (xmlReader->qualifiedName() == "model") {
        QString model = xmlReader->attributes().value("modelSelection").toString();
        QStringRef tweakingConstant = xmlReader->attributes().value("tweakingConstant");
        QStringRef quantile = xmlReader->attributes().value("quantile");
        bool validModel = true;
        if (model.isEmpty())
          validModel = false;
        if (tweakingConstant.isEmpty())
          validModel = false;
        if (quantile.isEmpty())
          validModel = false;
        if (validModel)
        {
          m_maximumLikelihoodFunctions.append(
              qMakePair(MaximumLikelihoodWFunctions(
                            MaximumLikelihoodWFunctions::stringToModel(model),
                            tweakingConstant.toDouble()),
                        quantile.toDouble()));
        }
        xmlReader->skipCurrentElement();
      }
      else {
        xmlReader->skipCurrentElement();
      }
    }
  }

  /**
   * Copy constructor for BundleResults.  Creates this BundleResults object as a copy
   * of another BundleResults object.
   *
   * @param src The other BundleResults object to be copied.
   */
  BundleResults::BundleResults(const BundleResults &src)
      : m_correlationMatrix(new CorrelationMatrix(*src.m_correlationMatrix)),
        m_numberFixedPoints(src.m_numberFixedPoints),
        m_numberIgnoredPoints(src.m_numberIgnoredPoints),
        m_numberHeldImages(src.m_numberHeldImages),
        m_rmsXResiduals(src.m_rmsXResiduals),
        m_rmsYResiduals(src.m_rmsYResiduals),
        m_rmsXYResiduals(src.m_rmsXYResiduals),
        m_rejectionLimit(src.m_rejectionLimit),
        m_numberObservations(src.m_numberObservations),
        m_numberImageObservations(src.m_numberImageObservations),
        m_numberLidarImageObservations(src.m_numberLidarImageObservations),
        m_numberRejectedObservations(src.m_numberRejectedObservations),
        m_numberLidarRangeConstraintEquations(src.m_numberLidarRangeConstraintEquations),
        m_numberUnknownParameters(src.m_numberUnknownParameters),
        m_numberImageParameters(src.m_numberImageParameters),
        m_numberConstrainedImageParameters(src.m_numberConstrainedImageParameters),
        m_numberConstrainedPointParameters(src.m_numberConstrainedPointParameters),
        m_numberConstrainedLidarPointParameters(src.m_numberConstrainedLidarPointParameters),
        m_numberConstrainedTargetParameters(src.m_numberConstrainedTargetParameters),
        m_degreesOfFreedom(src.m_degreesOfFreedom),
        m_sigma0(src.m_sigma0),
        m_elapsedTime(src.m_elapsedTime),
        m_elapsedTimeErrorProp(src.m_elapsedTimeErrorProp),
        m_converged(src.m_converged),
        m_bundleControlPoints(src.m_bundleControlPoints),
        m_bundleLidarPoints(src.m_bundleLidarPoints),
        m_outNet(src.m_outNet),
        m_outLidarData(src.m_outLidarData),
        m_iterations(src.m_iterations),
        m_observations(src.m_observations),
        m_rmsImageSampleResiduals(src.m_rmsImageSampleResiduals),
        m_rmsImageLineResiduals(src.m_rmsImageLineResiduals),
        m_rmsImageResiduals(src.m_rmsImageResiduals),
        m_rmsLidarImageSampleResiduals(src.m_rmsLidarImageSampleResiduals),
        m_rmsLidarImageLineResiduals(src.m_rmsLidarImageLineResiduals),
        m_rmsLidarImageResiduals(src.m_rmsLidarImageResiduals),
        m_rmsImageXSigmas(src.m_rmsImageXSigmas),
        m_rmsImageYSigmas(src.m_rmsImageYSigmas),
        m_rmsImageZSigmas(src.m_rmsImageZSigmas),
        m_rmsImageRASigmas(src.m_rmsImageRASigmas),
        m_rmsImageDECSigmas(src.m_rmsImageDECSigmas),
        m_rmsImageTWISTSigmas(src.m_rmsImageTWISTSigmas),
        m_minSigmaCoord1Distance(src.m_minSigmaCoord1Distance),
        m_maxSigmaCoord1Distance(src.m_maxSigmaCoord1Distance),
        m_minSigmaCoord2Distance(src.m_minSigmaCoord2Distance),
        m_maxSigmaCoord2Distance(src.m_maxSigmaCoord2Distance),
        m_minSigmaCoord3Distance(src.m_minSigmaCoord3Distance),
        m_maxSigmaCoord3Distance(src.m_maxSigmaCoord3Distance),
        m_minSigmaCoord1PointId(src.m_minSigmaCoord1PointId),
        m_maxSigmaCoord1PointId(src.m_maxSigmaCoord1PointId),
        m_minSigmaCoord2PointId(src.m_minSigmaCoord2PointId),
        m_maxSigmaCoord2PointId(src.m_maxSigmaCoord2PointId),
        m_minSigmaCoord3PointId(src.m_minSigmaCoord3PointId),
        m_maxSigmaCoord3PointId(src.m_maxSigmaCoord3PointId),
        m_rmsSigmaCoord1Stats(src.m_rmsSigmaCoord1Stats),
        m_rmsSigmaCoord2Stats(src.m_rmsSigmaCoord2Stats),
        m_rmsSigmaCoord3Stats(src.m_rmsSigmaCoord3Stats),
        m_maximumLikelihoodFunctions(src.m_maximumLikelihoodFunctions),
        m_maximumLikelihoodIndex(src.m_maximumLikelihoodIndex),
        m_cumPro(new StatCumProbDistDynCalc(*src.m_cumPro)),
        m_cumProRes(new StatCumProbDistDynCalc(*src.m_cumProRes)),
        m_maximumLikelihoodMedianR2Residuals(src.m_maximumLikelihoodMedianR2Residuals)
  {
  }

  /**
   * Destroys this BundleResults object.
   */
  BundleResults::~BundleResults() {

    delete m_correlationMatrix;
    m_correlationMatrix = NULL;

    delete m_cumPro;
    m_cumPro = NULL;

    delete m_cumProRes;
    m_cumProRes = NULL;

  }


  /**
   * Assignment operator for BundleResults. Overwrites this BundleResults object with
   * another BundleResults object.
   *
   * @param src The other BundleResults object to be copied from.
   */
  BundleResults &BundleResults::operator=(const BundleResults &src) {

    if (&src != this) {
      delete m_correlationMatrix;
      m_correlationMatrix = NULL;
      m_correlationMatrix = new CorrelationMatrix(*src.m_correlationMatrix);

      m_numberFixedPoints = src.m_numberFixedPoints;
      m_numberIgnoredPoints = src.m_numberIgnoredPoints;
      m_numberHeldImages = src.m_numberHeldImages;
      m_rmsXResiduals = src.m_rmsXResiduals;
      m_rmsYResiduals = src.m_rmsYResiduals;
      m_rmsXYResiduals = src.m_rmsXYResiduals;
      m_rejectionLimit = src.m_rejectionLimit;
      m_numberObservations = src.m_numberObservations;
      m_numberImageObservations = src.m_numberImageObservations;
      m_numberLidarImageObservations = src.m_numberLidarImageObservations;
      m_numberRejectedObservations = src.m_numberRejectedObservations;
      m_numberLidarRangeConstraintEquations = src.m_numberLidarRangeConstraintEquations;
      m_numberUnknownParameters = src.m_numberUnknownParameters;
      m_numberImageParameters = src.m_numberImageParameters;
      m_numberConstrainedImageParameters = src.m_numberConstrainedImageParameters;
      m_numberConstrainedPointParameters = src.m_numberConstrainedPointParameters;
      m_numberConstrainedLidarPointParameters = src.m_numberConstrainedLidarPointParameters;
      m_numberConstrainedTargetParameters = src.m_numberConstrainedTargetParameters;
      m_degreesOfFreedom = src.m_degreesOfFreedom;
      m_sigma0 = src.m_sigma0;
      m_elapsedTime = src.m_elapsedTime;
      m_elapsedTimeErrorProp = src.m_elapsedTimeErrorProp;
      m_converged = src.m_converged;
      m_bundleControlPoints = src.m_bundleControlPoints;
      m_bundleLidarPoints = src.m_bundleLidarPoints;
      m_outNet = src.m_outNet;
      m_outLidarData = src.m_outLidarData;
      m_iterations = src.m_iterations;
      m_observations = src.m_observations;
      m_rmsImageSampleResiduals = src.m_rmsImageSampleResiduals;
      m_rmsImageLineResiduals = src.m_rmsImageLineResiduals;
      m_rmsImageResiduals = src.m_rmsImageResiduals;
      m_rmsLidarImageSampleResiduals = src.m_rmsLidarImageSampleResiduals;
      m_rmsLidarImageLineResiduals = src.m_rmsLidarImageLineResiduals;
      m_rmsLidarImageResiduals = src.m_rmsLidarImageResiduals;
      m_rmsImageXSigmas = src.m_rmsImageXSigmas;
      m_rmsImageYSigmas = src.m_rmsImageYSigmas;
      m_rmsImageZSigmas = src.m_rmsImageZSigmas;
      m_rmsImageRASigmas = src.m_rmsImageRASigmas;
      m_rmsImageDECSigmas = src.m_rmsImageDECSigmas;
      m_rmsImageTWISTSigmas = src.m_rmsImageTWISTSigmas;
      m_minSigmaCoord1Distance = src.m_minSigmaCoord1Distance;
      m_maxSigmaCoord1Distance = src.m_maxSigmaCoord1Distance;
      m_minSigmaCoord2Distance = src.m_minSigmaCoord2Distance;
      m_maxSigmaCoord2Distance = src.m_maxSigmaCoord2Distance;
      m_minSigmaCoord3Distance = src.m_minSigmaCoord3Distance;
      m_maxSigmaCoord3Distance = src.m_maxSigmaCoord3Distance;
      m_minSigmaCoord1PointId = src.m_minSigmaCoord1PointId;
      m_maxSigmaCoord1PointId = src.m_maxSigmaCoord1PointId;
      m_minSigmaCoord2PointId = src.m_minSigmaCoord2PointId;
      m_maxSigmaCoord2PointId = src.m_maxSigmaCoord2PointId;
      m_minSigmaCoord3PointId = src.m_minSigmaCoord3PointId;
      m_maxSigmaCoord3PointId = src.m_maxSigmaCoord3PointId;
      m_rmsSigmaCoord1Stats = src.m_rmsSigmaCoord1Stats;
      m_rmsSigmaCoord2Stats = src.m_rmsSigmaCoord2Stats;
      m_rmsSigmaCoord3Stats = src.m_rmsSigmaCoord3Stats;
      m_maximumLikelihoodFunctions = src.m_maximumLikelihoodFunctions;
      m_maximumLikelihoodIndex = src.m_maximumLikelihoodIndex;

      delete m_cumPro;
      m_cumPro = NULL;
      m_cumPro = new StatCumProbDistDynCalc(*src.m_cumPro);

      delete m_cumProRes;
      m_cumProRes = NULL;
      m_cumProRes = new StatCumProbDistDynCalc(*src.m_cumProRes);

      m_maximumLikelihoodMedianR2Residuals = src.m_maximumLikelihoodMedianR2Residuals;
    }
    return *this;
  }


  /**
   * Initializes the BundleResults to a default state where all numeric members are set to
   * 0 or another default value, all QString members are set to empty, all QVectors and
   * QLists are cleared, and all other members are set to NULL.
   */
  void BundleResults::initialize() {
    m_correlationMatrix = NULL;

    m_numberFixedPoints = 0; // set in BA constructor->init->fillPointIndexMap
    m_numberIgnoredPoints = 0; // set in BA constructor->init->fillPointIndexMap


    // set in BundleAdjust init()
    m_numberHeldImages = 0;

    // members set while computing bundle stats
    m_rmsImageSampleResiduals.clear();
    m_rmsImageLineResiduals.clear();
    m_rmsImageResiduals.clear();
    m_rmsLidarImageSampleResiduals.clear();
    m_rmsLidarImageLineResiduals.clear();
    m_rmsLidarImageResiduals.clear();
    m_rmsImageXSigmas.clear();
    m_rmsImageYSigmas.clear();
    m_rmsImageZSigmas.clear();
    m_rmsImageRASigmas.clear();
    m_rmsImageDECSigmas.clear();
    m_rmsImageTWISTSigmas.clear();

    // Initialize coordinate sigma boundaries.  Units are meters for sigmas in both
    // latitudinal and rectangular coordinates
    m_minSigmaCoord1Distance.setMeters(1.0e+12);
    m_maxSigmaCoord1Distance.setMeters(0.0);
    m_minSigmaCoord2Distance.setMeters(1.0e+12);
    m_maxSigmaCoord2Distance.setMeters(0.0);;
    m_minSigmaCoord3Distance.setMeters(1.0e+12);
    m_maxSigmaCoord3Distance.setMeters(0.0);
    m_minSigmaCoord1PointId = "";
    m_maxSigmaCoord1PointId = "";
    m_minSigmaCoord2PointId = "";
    m_maxSigmaCoord2PointId = "";
    m_minSigmaCoord3PointId = "";
    m_maxSigmaCoord3PointId = "";

    m_rmsSigmaCoord1Stats = 0.0;
    m_rmsSigmaCoord2Stats = 0.0;
    m_rmsSigmaCoord3Stats = 0.0;


    // set by compute residuals
    m_rmsXResiduals = 0.0;
    m_rmsYResiduals = 0.0;
    m_rmsXYResiduals = 0.0;

    // set by compute rejection limit
    m_rejectionLimit = 0.0;

    // set by flag outliers
    m_numberRejectedObservations = 0;

    // set by formNormalEquations_CHOLMOD, formNormalEquations_SPECIALK, or solve
    m_numberObservations = 0;
    m_numberImageObservations = 0;
    m_numberLidarImageObservations = 0;
    m_numberImageParameters = 0;

// ??? unused variable ???    m_numberHeldPoints = 0;

    // set by formNormalEquations_CHOLMOD, formNormalEquations_SPECIALK, or
    // setParameterWeights (i.e. solve)
    m_numberConstrainedPointParameters = 0;
    m_numberConstrainedLidarPointParameters = 0;
    m_numberConstrainedImageParameters = 0;
    m_numberConstrainedTargetParameters = 0;
    m_numberLidarRangeConstraintEquations = 0;

    // set by initialize, formNormalEquations_CHOLMOD, formNormalEquations_SPECIALK, or solve
    m_numberUnknownParameters = 0;

    // solve and solve cholesky
    m_degreesOfFreedom = -1;
    m_iterations = 0;
    m_sigma0 = 0.0;
    m_elapsedTime = 0.0;
    m_elapsedTimeErrorProp = 0.0;
    m_converged = false; // or initialze method

    m_cumPro = NULL;
    m_maximumLikelihoodIndex = 0;
    m_maximumLikelihoodMedianR2Residuals = 0.0;
    m_maximumLikelihoodFunctions.clear();
    m_cumProRes = NULL;

    m_observations.clear();
    m_outNet.clear();
    m_outLidarData.clear();

  }


  /**
   * Resizes all image sigma vectors.
   *
   * @param numberImages The new size for the image sigma vectors.
   */
  void BundleResults::resizeSigmaStatisticsVectors(int numberImages) {
    m_rmsImageXSigmas.resize(numberImages);
    m_rmsImageYSigmas.resize(numberImages);
    m_rmsImageZSigmas.resize(numberImages);
    m_rmsImageRASigmas.resize(numberImages);
    m_rmsImageDECSigmas.resize(numberImages);
    m_rmsImageTWISTSigmas.resize(numberImages);
  }


#if 0
  void BundleResults::setRmsImageResidualLists(QVector<Statistics> rmsImageLineResiduals,
                                               QVector<Statistics> rmsImageSampleResiduals,
                                               QVector<Statistics> rmsImageResiduals) {
    // QList??? jigsaw apptest gives - ASSERT failure in QList<T>::operator[]: "index out of range",
    m_rmsImageLineResiduals = rmsImageLineResiduals.toList();
    m_rmsImageSampleResiduals = rmsImageSampleResiduals.toList();
    m_rmsImageResiduals = rmsImageResiduals.toList();
  }
#endif


  /**
   * Sets the root mean square image residual Statistics lists.
   *
   * @param rmsImageLineResiduals The new image line residuals list.
   * @param rmsImageSampleResiduals The new image sample residuals list.
   * @param rmsImageResiduals The new image residuals list.
   */
  void BundleResults::setRmsImageResidualLists(QList<Statistics> rmsImageLineResiduals,
                                               QList<Statistics> rmsImageSampleResiduals,
                                               QList<Statistics> rmsImageResiduals) {
    m_rmsImageLineResiduals = rmsImageLineResiduals;
    m_rmsImageSampleResiduals = rmsImageSampleResiduals;
    m_rmsImageResiduals = rmsImageResiduals;
  }


  /**
   * Sets the root mean square lidar image residual Statistics lists.
   *
   * @param rmsLidarImageLineResiduals The new image line residuals list.
   * @param rmsLidarImageSampleResiduals The new image sample residuals list.
   * @param rmsLidarImageResiduals The new image residuals list.
   */
  void BundleResults::setRmsLidarImageResidualLists(QList<Statistics> rmsLidarImageLineResiduals,
                                               QList<Statistics> rmsLidarImageSampleResiduals,
                                               QList<Statistics> rmsLidarImageResiduals) {
    m_rmsLidarImageLineResiduals = rmsLidarImageLineResiduals;
    m_rmsLidarImageSampleResiduals = rmsLidarImageSampleResiduals;
    m_rmsLidarImageResiduals = rmsLidarImageResiduals;
  }


  /**
   * Sets the min and max sigma distances and point ids for coordinate 1.
   *
   * @param minLatDist The new minimum sigma latitude distance.
   * @param maxLatDist The new maximum sigma latitude distance.
   * @param minLatPointId The new minimum sigma latitude point id.
   * @param maxLatPointId The new maximum sigma latitude point id.
   */
  void BundleResults::setSigmaCoord1Range(Distance minCoord1Dist, Distance maxCoord1Dist,
                                            QString minCoord1PointId, QString maxCoord1PointId) {
    m_minSigmaCoord1Distance = minCoord1Dist;
    m_maxSigmaCoord1Distance = maxCoord1Dist;
    m_minSigmaCoord1PointId  = minCoord1PointId;
    m_maxSigmaCoord1PointId  = maxCoord1PointId;
  }


  /**
   * Sets the min and max sigma distances and point ids for coordinate 2.
   *
   * @param minLonDist The new minimum sigma longitude distance.
   * @param maxLonDist The new maximum sigma longitude distance.
   * @param minLonPointId The new minimum sigma longitude point id.
   * @param maxLonPointId The new maximum sigma longitude point id.
   */
  void BundleResults::setSigmaCoord2Range(Distance minCoord2Dist, Distance maxCoord2Dist,
                                             QString minCoord2PointId, QString maxCoord2PointId) {
    m_minSigmaCoord2Distance = minCoord2Dist;
    m_maxSigmaCoord2Distance = maxCoord2Dist;
    m_minSigmaCoord2PointId  = minCoord2PointId;
    m_maxSigmaCoord2PointId  = maxCoord2PointId;
  }


  /**
   * Sets the min and max sigma distances and point ids for coordinate 3.
   *
   * @param minRadDist The new minimum sigma radius distance.
   * @param maxRadDist The new maximum sigma radius distance.
   * @param minRadPointId The new minimum sigma radius point id.
   * @param maxRadPointId The new maximum sigma radius point id.
   */
  void BundleResults::setSigmaCoord3Range(Distance minCoord3Dist, Distance maxCoord3Dist,
                                          QString minCoord3PointId, QString maxCoord3PointId) {
    m_minSigmaCoord3Distance = minCoord3Dist;
    m_maxSigmaCoord3Distance = maxCoord3Dist;
    m_minSigmaCoord3PointId  = minCoord3PointId;
    m_maxSigmaCoord3PointId  = maxCoord3PointId;
  }


  /**
   * Sets the root mean square values of the adjusted sigmas for all three coordinates.
   *
   * @param rmsFromSigmaCoord1Stats The new RMS value of the adjusted coord1 sigmas.
   * @param rmsFromSigmaCoord2Stats The new RMS value of the adjusted coord2 sigmas.
   * @param rmsFromSigmaCoord3Stats The new RMS value of the adjusted coord3 sigmas.
   */
  void BundleResults::setRmsFromSigmaStatistics(
                                                double rmsFromSigmaCoord1Stats,
                                                double rmsFromSigmaCoord2Stats,
                                                double rmsFromSigmaCoord3Stats) {
    m_rmsSigmaCoord1Stats = rmsFromSigmaCoord1Stats;
    m_rmsSigmaCoord2Stats = rmsFromSigmaCoord2Stats;
    m_rmsSigmaCoord3Stats = rmsFromSigmaCoord3Stats;
  }


  /**
  * This method steps up the maximum likelihood estimation solution.  Up to three successive
  * solutions models are available.
  *
  * @param modelsWithQuantiles The maixmum likelihood models and their quantiles.  If empty,
  *                            then maximum likelihood estimation will not be used.
  */
  void BundleResults::maximumLikelihoodSetUp(
      QList< QPair< MaximumLikelihoodWFunctions::Model, double > > modelsWithQuantiles) {


    // reinitialize variables if this setup has already been called
    m_maximumLikelihoodIndex = 0;
    m_maximumLikelihoodMedianR2Residuals = 0.0;

    // residual prob distribution is calculated even if there is no maximum likelihood estimation.
    // set up the solver to have a node at every percent of the distribution
    m_cumProRes = NULL;
    m_cumProRes = new StatCumProbDistDynCalc;
    initializeResidualsProbabilityDistribution(101);

    // if numberMaximumLikelihoodModels > 0, then MaximumLikeliHood Estimation is being used.
    for (int i = 0; i < modelsWithQuantiles.size(); i++) {

      // if maximum likelihood is being used, the cum prob calculator is initialized.
      if (i == 0) {
        m_cumPro = NULL;
        m_cumPro = new StatCumProbDistDynCalc;
        // set up the solver to have a node at every percent of the distribution
        initializeProbabilityDistribution(101);
      }

      // set up the w functions for the maximum likelihood estimation
      m_maximumLikelihoodFunctions.append(
          qMakePair(MaximumLikelihoodWFunctions(modelsWithQuantiles[i].first),
                    modelsWithQuantiles[i].second));

    }


    //maximum likelihood estimation tiered solutions requiring multiple convergeances are supported,
    // this index keeps track of which tier the solution is in
    m_maximumLikelihoodIndex = 0;
  }


  /**
   * Prints out information about which tier the solution is in and the status of the residuals.
   */
  void BundleResults::printMaximumLikelihoodTierInformation() {
//  printf("Maximum Likelihood Tier: %d\n", m_maximumLikelihoodIndex);
    if (numberMaximumLikelihoodModels() > m_maximumLikelihoodIndex) {
      // if maximum likelihood estimation is being used
      // at the end of every iteration
      // reset the tweaking contant to the desired quantile of the |residual| distribution
      double quantile = m_maximumLikelihoodFunctions[m_maximumLikelihoodIndex].second;
      double tc = m_cumPro->value(quantile);
      m_maximumLikelihoodFunctions[m_maximumLikelihoodIndex].first.setTweakingConstant(tc);
      //  print meadians of residuals
      m_maximumLikelihoodMedianR2Residuals = m_cumPro->value(0.5);
//    printf("Median of R^2 residuals:  %lf\n", m_maximumLikelihoodMedianR2Residuals);

      //restart the dynamic calculation of the cumulative probility distribution of |R^2 residuals|
      // so it will be up to date for the next iteration
      initializeProbabilityDistribution(101);
    }
  }


  /**
   * Initializes or resets the cumulative probability distribution of |R^2 residuals|.
   *
   * @param nodes The number of quantiles in the cumulative probability distribution.
   */
  void BundleResults::initializeProbabilityDistribution(unsigned int nodes) {
    m_cumPro->setQuantiles(nodes);
  }


  /**
   * Initializes or resets the cumulative probability distribution of residuals used for reporting.
   *
   * @param nodes The number of quantiles in the cumulative probability distribution.
   */
  void BundleResults::initializeResidualsProbabilityDistribution(unsigned int nodes) {
    m_cumProRes->setQuantiles(nodes);
  }


  /**
   * Adds an observation to the cumulative probability distribution
   * of |R^2 residuals|.
   *
   * @param observationValue The value of the added observation.
   */
  void BundleResults::addProbabilityDistributionObservation(double observationValue) {
    m_cumPro->addObs(observationValue);
  }


  /**
   * Adds an observation to the cumulative probability distribution
   * of residuals used for reporting.
   *
   * @param observationValue The value of the added observation.
   */
  void BundleResults::addResidualsProbabilityDistributionObservation(double observationValue) {
    m_cumProRes->addObs(observationValue);
  }


  /**
   * Increases the value that indicates which stage the maximum likelihood adjustment
   * is currently on.
   */
  void BundleResults::incrementMaximumLikelihoodModelIndex() {
    m_maximumLikelihoodIndex++;
  }


  /**
   * Increase the number of 'fixed' (ground) points.
   */
  void BundleResults::incrementFixedPoints() {
    m_numberFixedPoints++;
  }


  /**
   * Returns the number of 'fixed' (ground) points.
   *
   * @return @b int The number of fixed points.
   */
  int BundleResults::numberFixedPoints() const {
    return m_numberFixedPoints;
  }


  /**
   * Increases the number of 'held' images.
   */
  void BundleResults::incrementHeldImages() {
    m_numberHeldImages++;
  }


  /**
   * Returns the number of 'held' images.
   *
   * @return @b int The number of held images.
   */
  int BundleResults::numberHeldImages() const {
    return m_numberHeldImages;
  }


  /**
   * Increase the number of ignored points.
   */
  void BundleResults::incrementIgnoredPoints() {
    m_numberIgnoredPoints++;
  }


  /**
   * Returns the number of ignored points.
   *
   * @return @b int The number of ignored points.
   */
  int BundleResults::numberIgnoredPoints() const {
    return m_numberIgnoredPoints;
  }


  /**
   * Sets the root mean square of the x and y residuals.
   *
   * @param rx The RMS value of the x residuals.
   * @param ry The RMS value of the y residuals.
   * @param rxy The RMS value of both the x and y residuals.
   */
  void BundleResults::setRmsXYResiduals(double rx, double ry, double rxy) {
    m_rmsXResiduals = rx;
    m_rmsYResiduals = ry;
    m_rmsXYResiduals = rxy;
  }


  /**
   * Sets the rejection limit.
   *
   * @param rejectionLimit The rejection limit.
   */
  void BundleResults::setRejectionLimit(double rejectionLimit) {
    m_rejectionLimit = rejectionLimit;
  }


  /**
   * Sets the number of rejected observations.
   *
   * @param numberRejectedObservations The number of rejected observations.
   */
  void BundleResults::setNumberRejectedObservations(int numberRejectedObservations) {
    m_numberRejectedObservations = numberRejectedObservations;
  }


  /**
   * Sets the number of observations.
   *
   * @param numberObservations The number of observations.
   */
  void BundleResults::setNumberObservations(int numberObservations) {
    m_numberObservations = numberObservations;
  }


  /**
   * Sets the number of photogrammetric image observations. Note in this terminology an image
   * measurement contributes two observations to the adjustment (i.e. sample/line).
   *
   * So, the number of observations divided by 2 should equal the number of image measures.
   *
   * @param numberObservations The number of photogrammetric image observations.
   */
  void BundleResults::setNumberImageObservations(int numberObservations) {
    m_numberImageObservations = numberObservations;
  }


  /**
   * Sets the number of lidar observations.
   *
   * @param numberLidarObservations The number of lidar observations.
   */
  void BundleResults::setNumberLidarImageObservations(int numberLidarObservations) {
    m_numberLidarImageObservations = numberLidarObservations;
  }


  /**
   * Sets the number of image parameters.
   *
   * @param numberParameters The number of image parameters.
   */
  void BundleResults::setNumberImageParameters(int numberParameters) {
    m_numberImageParameters = numberParameters;
  }


  /**
   * Set number of contrained point parameters.
   *
   * @param numberParameters Number of contrained point parameters.
   */
  void BundleResults::setNumberConstrainedPointParameters(int numberParameters) {
    m_numberConstrainedPointParameters = numberParameters;
  }


  /**
   * Set number of contrained point parameters.
   *
   * @param numberParameters Number of contrained point parameters.
   */
  void BundleResults::setNumberConstrainedLidarPointParameters(int numberParameters) {
    m_numberConstrainedLidarPointParameters = numberParameters;
  }


  /**
   * Resets the number of contrained point parameters to 0.
   */
  void BundleResults::resetNumberConstrainedPointParameters() {
    m_numberConstrainedPointParameters = 0;
  }


  /**
   * Increase the number of contrained point parameters.
   *
   * @param incrementAmount The amount to increase by.
   */
  void BundleResults::incrementNumberConstrainedPointParameters(int incrementAmount) {
    m_numberConstrainedPointParameters += incrementAmount;
  }


  /**
   * Resets the number of constrained image parameters to 0.
   */
  void BundleResults::resetNumberConstrainedImageParameters() {
    m_numberConstrainedImageParameters = 0;
  }


  /**
   * Increase the number of constrained image parameters.
   *
   * @param incrementAmount The amount to increase by.
   */
  void BundleResults::incrementNumberConstrainedImageParameters(int incrementAmount) {
    m_numberConstrainedImageParameters += incrementAmount;
  }


  /**
   * Resets the number of constrained target parameters to 0.
   */
  void BundleResults::resetNumberConstrainedTargetParameters() {
    m_numberConstrainedTargetParameters = 0;
  }


  /**
   * Increases the number of constrained target parameters.
   *
   * @param incrementAmount The amount to increase by.
   */
  void BundleResults::incrementNumberConstrainedTargetParameters(int incrementAmount) {
    m_numberConstrainedTargetParameters += incrementAmount;
  }


  /**
   * Sets the total number of parameters to solve for.
   *
   * @param numberParameters The number of parameters to solve for.
   */
  void BundleResults::setNumberUnknownParameters(int numberParameters) {
    m_numberUnknownParameters = numberParameters;
  }


  /**
   * Sets the total number of lidar range constraints.
   *
   * @param numberLidarRangeConstraints The total number of lidar range constraints.
   */
  void BundleResults::setNumberLidarRangeConstraints(int numberLidarRangeConstraints) {
    m_numberLidarRangeConstraintEquations = numberLidarRangeConstraints;
  }


  /**
   * Computes the degrees of freedom of the bundle adjustment and stores it internally.
   */
  void BundleResults::computeDegreesOfFreedom() {
    m_degreesOfFreedom = m_numberImageObservations
                         + m_numberLidarImageObservations
                         + m_numberConstrainedPointParameters
                         + m_numberConstrainedLidarPointParameters
                         + m_numberConstrainedImageParameters
                         + m_numberConstrainedTargetParameters
                         + m_numberLidarRangeConstraintEquations
                         - m_numberUnknownParameters;
  }


  /**
   * Computes the sigma0 and stores it internally.
   *
   * Sigma0 is the standard deviation of an observation of unit weight. Sigma0^2 is the variance of
   * an observation of unit weight (also reference variance or variance factor).
   *
   * Sigma0^2 = vtpv/degrees of freedom.
   *
   * @param dvtpv The weighted sum of the squares of the residuals.  Computed by
   *              V transpose * P * V, where
   *              V is the vector of residuals and
   *              P is the weight matrix.
   * @param criteria The convergence criteria for the bundle adjustment.
   *
   * @throws IException::Io "Computed degrees of freedom is invalid."
   */
  void BundleResults::computeSigma0(double dvtpv, BundleSettings::ConvergenceCriteria criteria) {
    computeDegreesOfFreedom();

    if (m_degreesOfFreedom > 0) {
      m_sigma0 = dvtpv / m_degreesOfFreedom;
    }
    else if (m_degreesOfFreedom == 0 && criteria == BundleSettings::ParameterCorrections) {
      m_sigma0 = dvtpv;
    }
    else {
      std::string msg = "Computed degrees of freedom [" + toString(m_degreesOfFreedom)
                    + "] is invalid.";
      throw IException(IException::Io, msg, _FILEINFO_);
    }

    m_sigma0 = sqrt(m_sigma0);
  }


  /**
   * Sets the degrees of freedom.
   *
   * @param degreesOfFreedom The degrees of freedom.
   */
  void BundleResults::setDegreesOfFreedom(double degreesOfFreedom) { // old sparse
    m_degreesOfFreedom = degreesOfFreedom;
  }


  /**
   * Sets the sigma0.
   *
   * @param sigma0 The sigma0.
   */
  void BundleResults::setSigma0(double sigma0) { // old sparse
    m_sigma0 = sigma0;
  }


  /**
   * Sets the elapsed time for the bundle adjustment.
   *
   * @param time The elapsed time.
   */
  void BundleResults::setElapsedTime(double time) {
    m_elapsedTime = time;
  }


  /**
   * Sets the elapsed time for error propegation.
   *
   * @param time The elapsed time.
   */
  void BundleResults::setElapsedTimeErrorProp(double time) {
    m_elapsedTimeErrorProp = time;
  }


  /**
   * Sets if the bundle adjustment converged.
   *
   * @param converged If the bundle adjustment converged.
   */
  void BundleResults::setConverged(bool converged) {
    m_converged = converged;
  }


  /**
   * Sets the bundle control point vector.
   *
   * @param controlPoints The vector of BundleControlPointQsps.
   */
  void BundleResults::setBundleControlPoints(QVector<BundleControlPointQsp> controlPoints) {
    m_bundleControlPoints = controlPoints;
  }


  /**
   * Sets the bundle lidar point vector.
   *
   * @param lidarPoints Vector of BundleLidarControlPointQsps.
   */
  void BundleResults::setBundleLidarPoints(QVector<BundleLidarControlPointQsp> lidarPoints) {
    m_bundleLidarPoints = lidarPoints;
  }


  /**
   * Sets the output ControlNet.
   *
   * @param outNet A QSharedPointer to the output ControlNet.
   */
  void BundleResults::setOutputControlNet(ControlNetQsp outNet) {
    m_outNet = outNet;
  }


  /**
   * Sets the output LidarData object.
   *
   * @param outLidarData A QSharedPointer to the output LidarData object.
   */
  void BundleResults::setOutputLidarData(LidarDataQsp outLidarData) {
    m_outLidarData = outLidarData;
  }


  /**
   * Sets the number of iterations taken by the BundleAdjust.
   *
   * @param iterations The number of iterations.
   */
  void BundleResults::setIterations(int iterations) {
    m_iterations = iterations;
  }


  /**
   * Sets the vector of BundleObservations.
   *
   * @param observations The vector of BundleObservations.
   */
  void BundleResults::setObservations(BundleObservationVector observations) {
    m_observations = observations;
  }



  //************************* Accessors **********************************************************//

  /**
   * Returns the list of RMS image sample residuals statistics.
   *
   * @return @b QList<Statistics> The RMS image sample residual statistics.
   */
  QList<Statistics> BundleResults::rmsImageSampleResiduals() const {
    return m_rmsImageSampleResiduals;
  }


  /**
   * Returns the list of RMS image line residuals statistics.
   *
   * @return @b QList<Statistics> The RMS image line residual statistics.
   */
  QList<Statistics> BundleResults::rmsImageLineResiduals() const {
    return m_rmsImageLineResiduals;
  }


  /**
   * Returns the list of RMS image residuals statistics.
   *
   * @return @b QList<Statistics> The RMS image residual statistics.
   */
  QList<Statistics> BundleResults::rmsImageResiduals() const {
    return m_rmsImageResiduals;
  }


  /**
   * Returns the list of RMS image lidar sample residuals statistics.
   *
   * @return QList<Statistics> The RMS image lidar sample residual statistics.
   */
  QList<Statistics> BundleResults::rmsLidarImageSampleResiduals() const {
    return m_rmsLidarImageSampleResiduals;
  }


  /**
   * Returns the list of RMS image lidar line residuals statistics.
   *
   * @return QList<Statistics> The RMS image lidar line residual statistics.
   */
  QList<Statistics> BundleResults::rmsLidarImageLineResiduals() const {
    return m_rmsLidarImageLineResiduals;
  }


  /**
   * Returns the list of RMS image lidar residuals statistics.
   *
   * @return QList<Statistics> The RMS image lidar residual statistics.
   */
  QList<Statistics> BundleResults::rmsLidarImageResiduals() const {
    return m_rmsLidarImageResiduals;
  }


  /**
   * Returns the list of RMS image x sigma statistics.
   *
   * @return @b QList<Statistics> The RMS image x sigma statistics.
   */
  QVector<Statistics> BundleResults::rmsImageXSigmas() const {
    return m_rmsImageXSigmas;
  }


  /**
   * Returns the list of RMS image y sigma statistics.
   *
   * @return @b QList<Statistics> The RMS image y sigma statistics.
   */
  QVector<Statistics> BundleResults::rmsImageYSigmas() const {
    return m_rmsImageYSigmas;
  }


  /**
   * Returns the list of RMS image z sigma statistics.
   *
   * @return @b QList<Statistics> The RMS image z sigma statistics.
   */
  QVector<Statistics> BundleResults::rmsImageZSigmas() const {
    return m_rmsImageZSigmas;
  }


  /**
   * Returns the list of RMS image right ascension sigma statistics.
   *
   * @return @b QList<Statistics> The RMS image right ascension sigma statistics.
   */
  QVector<Statistics> BundleResults::rmsImageRASigmas() const {
    return m_rmsImageRASigmas;
  }


  /**
   * Returns the list of RMS image declination sigma statistics.
   *
   * @return @b QList<Statistics> The RMS image declination sigma statistics.
   */
  QVector<Statistics> BundleResults::rmsImageDECSigmas() const {
    return m_rmsImageDECSigmas;
  }


  /**
   * Returns the list of RMS image twist sigma statistics.
   *
   * @return @b QList<Statistics> The RMS image twist sigma statistics.
   */
  QVector<Statistics> BundleResults::rmsImageTWISTSigmas() const {
    return m_rmsImageTWISTSigmas;
  }


  /**
   * Returns the minimum sigma distance for coordinate 1.
   *
   * @return @b Distance The minimum sigma for Coord1.
   */
  Distance BundleResults::minSigmaCoord1Distance() const {
    return m_minSigmaCoord1Distance;
  }


  /**
   * Returns the maximum sigma distance for coordinate 1.
   *
   * @return @b Distance The maximum sigma Coord1.
   */
  Distance BundleResults::maxSigmaCoord1Distance() const {
    return m_maxSigmaCoord1Distance;
  }


  /**
   * Returns the minimum sigma distance for coordinate 2.
   *
   * @return @b Distance The minimum sigma Coord2.
   */
  Distance BundleResults::minSigmaCoord2Distance() const {
    return m_minSigmaCoord2Distance;
  }


  /**
   * Returns the maximum sigma distance for coordinate 2.
   *
   * @return @b Distance The maximum sigma Coord2.
   */
  Distance BundleResults::maxSigmaCoord2Distance() const {
    return m_maxSigmaCoord2Distance;
  }


  /**
   * Returns the minimum sigma distance for coordinate 3.
   *
   * @return @b Distance The minimum sigma Coord3.
   */
  Distance BundleResults::minSigmaCoord3Distance() const {
    return m_minSigmaCoord3Distance;
  }


  /**
   * Returns the maximum sigma distance for coordinate 3.
   *
   * @return @b Distance The maximum sigma Coord3.
   */
  Distance BundleResults::maxSigmaCoord3Distance() const {
    return m_maxSigmaCoord3Distance;
  }


  /**
   * Returns the minimum sigma point id for coordinate 1.
   *
   * @return @b @QString The minimum sigma Coord1 point id.
   */
  QString BundleResults::minSigmaCoord1PointId() const {
    return m_minSigmaCoord1PointId;
  }


  /**
   * Returns the maximum sigma point id for coordinate 1.
   *
   * @return @b @QString The maximum sigma Coord1 point id.
   */
  QString BundleResults::maxSigmaCoord1PointId() const {
    return m_maxSigmaCoord1PointId;
  }


  /**
   * Returns the minimum sigma point id for coordinate 2.
   *
   * @return @b @QString The minimum sigma longitude point id.
   */
  QString BundleResults::minSigmaCoord2PointId() const {
    return m_minSigmaCoord2PointId;
  }


  /**
   * Returns the maximum sigma point id for coordinate 2.
   *
   * @return @b @QString The maximum sigma Coord2 point id.
   */
  QString BundleResults::maxSigmaCoord2PointId() const {
    return m_maxSigmaCoord2PointId;
  }


  /**
   * Returns the minimum sigma point id for coordinate 3.
   *
   * @return @b @QString The minimum sigma Coord3 point id.
   */
  QString BundleResults::minSigmaCoord3PointId() const {
    return m_minSigmaCoord3PointId;
  }


  /**
   * Returns the maximum sigma point id for coordinate 3.
   *
   * @return @b @QString The maximum sigma Coord3 point id.
   */
  QString BundleResults::maxSigmaCoord3PointId() const {
    return m_maxSigmaCoord3PointId;
  }


  /**
   * Returns the RMS of the adjusted sigmas for coordinate 1.
   *
   * @return @b double The RMS of the adjusted Coord1 sigmas.
   */
  double BundleResults::sigmaCoord1StatisticsRms() const {
    return m_rmsSigmaCoord1Stats;
  }


  /**
   * Returns the RMS of the adjusted sigmas for coordinate 2.
   *
   * @return @b double The RMS of the adjusted Coord2 sigmas.
   */
  double BundleResults::sigmaCoord2StatisticsRms() const {
    return m_rmsSigmaCoord2Stats;
  }


  /**
   * Returns the RMS of the adjusted sigmas for coordinate 3.
   *
   * @return @b double The RMS of the adjusted Coord3 sigmas.
   */
  double BundleResults::sigmaCoord3StatisticsRms() const {
    return m_rmsSigmaCoord3Stats;
  }


  /**
   * Returns the RMS of the x residuals.
   *
   * @return @b double The RMS of the x residuals.
   */
  double BundleResults::rmsRx() const {
    return m_rmsXResiduals;
  }


  /**
   * Returns the RMS of the y residuals.
   *
   * @return @b double The RMS of the y residuals.
   */
  double BundleResults::rmsRy() const {
    return m_rmsYResiduals;
  }


  /**
   * Returns the RMS of the x and y residuals.
   *
   * @return @b double The RMS of the x and y residuals.
   */
  double BundleResults::rmsRxy() const {
    return m_rmsXYResiduals;
  }


  /**
   * Returns the rejection limit.
   *
   * @return @b double The rejection limit.
   */
  double BundleResults::rejectionLimit() const {
    return m_rejectionLimit;
  }


  /**
   * Returns the number of observation that were rejected.
   *
   * @return @b int The number of rejected observations.
   */
  int BundleResults::numberRejectedObservations() const {
    return m_numberRejectedObservations;
  }


  /**
   * Returns the number of observations.
   *
   * @return @b int The number of observations.
   */
  int BundleResults::numberObservations() const {
    return m_numberImageObservations + m_numberLidarImageObservations;
  }


  /**
   * Returns the number of observations.
   *
   * @return int The number of observations.
   */
  int BundleResults::numberImageObservations() const {
    return m_numberImageObservations;
  }


  /**
   * Returns the number of lidar observations.
   *
   * @return int The number of lidar observations.
   */
  int BundleResults::numberLidarImageObservations() const {
    return m_numberLidarImageObservations;
  }


  /**
   * Returns the total number of image parameters.
   *
   * @return @b int The total number of image parameters.
   */
  int BundleResults::numberImageParameters() const {
    return m_numberImageParameters;
  }


  /**
   * Returns the number of constrained point parameters.
   *
   * @return @b int The number of constrained point parameters.
   */
  int BundleResults::numberConstrainedPointParameters() const {
    return m_numberConstrainedPointParameters;
  }


  /**
   * Returns the number of constrained image parameters.
   *
   * @return @b int The number of constrained image parameters.
   */
  int BundleResults::numberConstrainedImageParameters() const {
    return m_numberConstrainedImageParameters;
  }


  /**
   * Return the number of constrained target parameters.
   *
   * @return @b int The number of constrained target parameters.
   */
  int BundleResults::numberConstrainedTargetParameters() const {
    return m_numberConstrainedTargetParameters;
  }


  /**
   * Return the number of lidar range constraint equations.
   *
   * @return int The number of lidar range constraint equations.
   */
  int BundleResults::numberLidarRangeConstraintEquations() const {
    return m_numberLidarRangeConstraintEquations;
  }


  /**
   * Returns the number of unknown parameters.
   *
   * @return @b int The number of unknown parameters.
   */
  int BundleResults::numberUnknownParameters() const {
    return m_numberUnknownParameters;
  }


  /**
   * Returns the degrees of freedom.
   *
   * @return @b int the degrees of freedom.
   */
  int BundleResults::degreesOfFreedom() const {
    return m_degreesOfFreedom;
  }


  /**
   * Returns the Sigma0 of the bundle adjustment.
   *
   * @return @b double The Sigma0.
   */
  double BundleResults::sigma0() const {
    return m_sigma0;
  }


  /**
   * Returns the elapsed time for the bundle adjustment.
   *
   * @return @b double The elapsed time for the bundle adjustment.
   */
  double BundleResults::elapsedTime() const {
    return m_elapsedTime;
  }


  /**
   * Returns the elapsed time for error propagation.
   *
   * @return @b double The elapsed time for error propagation.
   */
  double BundleResults::elapsedTimeErrorProp() const {
    return m_elapsedTimeErrorProp;
  }


  /**
   * Returns whether or not the bundle adjustment converged.
   *
   * @return @b bool If the bundle adjustment converged.
   */
  bool BundleResults::converged() const {
    return m_converged;
  }


  /**
   * Returns a reference to the BundleControlPoint vector.
   *
   * @return @b QVector<BundleControlPointQsp>& The BundleControlPoint vector.
   */
  QVector<BundleControlPointQsp> &BundleResults::bundleControlPoints() {
    return m_bundleControlPoints;
  }


  /**
   * Returns a reference to the BundleLidarControlPoint vector.
   *
   * @return QVector<BundleLidarControlPointQsp>& The BundleLidarControlPoint vector.
   */
  QVector<BundleLidarControlPointQsp> &BundleResults::bundleLidarControlPoints() {
    return m_bundleLidarPoints;
  }


  /**
   * Returns a shared pointer to the output control network.
   *
   * @return @b ControlNetQsp A shared pointer to the output control network.
   *
   * @throws IException::Programmer "Output Control Network has not been set."
   */
  ControlNetQsp BundleResults::outputControlNet() const {
    if (!m_outNet) {
      throw IException(IException::Programmer,
                       "Output Control Network has not been set.",
                       _FILEINFO_);
    }
    return m_outNet;
  }


  /**
   * Returns a shared pointer to the output LidarData object.
   *
   * @return LidarDataQsp A shared pointer to the output LidarData object.
   *
   * @throws IException::Programmer "Output LidarData object has not been set."
   */
  LidarDataQsp BundleResults::outputLidarData() const {
    return m_outLidarData;
  }


  /**
   * Returns the number of iterations taken by the BundleAdjust.
   *
   * @return @b int The number of iterations.
   */
  int BundleResults::iterations() const {
    return m_iterations;
  }


  /**
   * Returns a reference to the observations used by the BundleAdjust.
   *
   * @return @b BundleObservationVector& A reference to the observation vector.
   */
  const BundleObservationVector &BundleResults::observations() const {
    return m_observations;
  }


  /**
   * Returns how many maximum likelihood models were used in the bundle adjustment.
   *
   * @return @b int The number fo maximum likelihood models.
   */
  int BundleResults::numberMaximumLikelihoodModels() const {
    return m_maximumLikelihoodFunctions.size();
  }


  /**
   * Returns which step the bundle adjustment is on.
   *
   * @return @b int The maximum likelihood model that the bundle adjustment is currently using.
   */
  int BundleResults::maximumLikelihoodModelIndex() const {
    return m_maximumLikelihoodIndex;
  }


  /**
   * Returns the cumulative probability distribution of the |R^2 residuals|.
   *
   * @return @b StatCumProbDistDynCalc The cumulative probability distribution of the
   *                                   |R^2 residuals|.
   */
  StatCumProbDistDynCalc BundleResults::cumulativeProbabilityDistribution() const {
    return *m_cumPro;
  }


  /**
   * Returns the cumulative probability distribution of the residuals used for reporting.
   *
   * @return @b StatCumProbDistDynCalc the cumulative probability distribution of the residuals.
   */
  StatCumProbDistDynCalc BundleResults::residualsCumulativeProbabilityDistribution() const {
    return *m_cumProRes;
  }


  /**
   * Returns the median of the |R^2 residuals|.
   *
   * @return @b double The median of the |R^2 residuals|.
   */
  double BundleResults::maximumLikelihoodMedianR2Residuals() const {
    return m_maximumLikelihoodMedianR2Residuals;
  }


  /**
   * Returns the maximum likelihood model at the given index.
   *
   * @param modelIndex The index of the maximum likelihood model to be returned.
   *
   * @return @b MaximumLikelihoodWFunctions The maximum likelihood model at the input index.
   */
  MaximumLikelihoodWFunctions BundleResults::maximumLikelihoodModelWFunc(int modelIndex) const {
    return m_maximumLikelihoodFunctions[modelIndex].first;
  }


  /**
   * Returns the quantile of the maximum likelihood model at the given index.
   *
   * @param modelIndex The index of the maximum likelihood model whose quantile will be returned.
   *
   * @return @b double The quantile of the desired maximum likelihood model.
   */
  double BundleResults::maximumLikelihoodModelQuantile(int modelIndex) const {
    return m_maximumLikelihoodFunctions[modelIndex].second;
  }


//  QList< QPair< MaximumLikelihoodWFunctions, double > >
//      BundleResults::maximumLikelihoodModels() const {
//    return m_maximumLikelihoodFunctions;
//  }


  /**
   * Returns the Correlation Matrix.
   *
   * @return @b CorrelationMatrix The correlation matrix.
   *
   * @throws IException::Unknown "Correlation matrix for this bundle is NULL."
   */
  CorrelationMatrix BundleResults::correlationMatrix() const {
    if (m_correlationMatrix) {
      return *m_correlationMatrix;
    }
    else {
      throw IException(IException::Unknown,
                       "Correlation matrix for this bundle is NULL.",
                       _FILEINFO_);
    }
  }


  /**
   * Set the covariance file name for the matrix used to calculate the correlation matrix.
   *
   * @param name The name of the file used to store the covariance matrix.
   */
  void BundleResults::setCorrMatCovFileName(FileName name) {
    correlationMatrix();// throw error if null
    m_correlationMatrix->setCovarianceFileName(name);
  }


  /**
   * Set the images and their associated parameters of the correlation matrix.
   *
   * @param imgsAndParams The QMap with all the images and parameters used for this bundle.
   */
  void BundleResults::setCorrMatImgsAndParams(QMap<QString, QStringList> imgsAndParams) {
    correlationMatrix();// throw error if null
    m_correlationMatrix->setImagesAndParameters(imgsAndParams);
  }


  SurfacePoint::CoordinateType BundleResults::coordTypeReports() {
    // Get the coordinate type from the output net if it exists.  Otherwise use the default.
    SurfacePoint::CoordinateType type = SurfacePoint::Latitudinal;

    if (m_outNet) {
        type = outputControlNet()->GetCoordType();
    }

    return type;
  }


  /**
   * Saves the BundleResults object to an XML file.
   *
   * @param stream The QXMLStreamWriter that will be used to write out the XML file.
   * @param project The project that the BundleResults object belongs to.
   */
  void BundleResults::save(QXmlStreamWriter &stream, const Project *project) const {
    // Get the coordinate type from the output net if it exists.  Otherwise use the default.
    SurfacePoint::CoordinateType coordType = SurfacePoint::Latitudinal;

    if (m_outNet) {
        coordType = outputControlNet()->GetCoordType();
    }

    stream.writeStartElement("bundleResults");
    stream.writeStartElement("correlationMatrix");
    stream.writeAttribute("correlationFileName",
                          QString::fromStdString(correlationMatrix().correlationFileName().expanded()));
    stream.writeAttribute("covarianceFileName",
                          QString::fromStdString(correlationMatrix().covarianceFileName().expanded()));
    stream.writeStartElement("imagesAndParameters");
    QMapIterator<QString, QStringList> imgParamIt(*correlationMatrix().imagesAndParameters());
    while (imgParamIt.hasNext()) {
      imgParamIt.next();
      stream.writeStartElement("image");
      stream.writeAttribute("id", imgParamIt.key());
      QStringList parameters = imgParamIt.value();
      for (int i = 0; i < parameters.size(); i++) {
        stream.writeTextElement("parameter", parameters[i]);
      }
      stream.writeEndElement(); // end image

    }
    stream.writeEndElement(); // end images and parameters
    stream.writeEndElement(); // end correlationMatrix

    stream.writeStartElement("generalStatisticsValues");
    stream.writeTextElement("numberFixedPoints", QString::number(numberFixedPoints()));
    stream.writeTextElement("numberIgnoredPoints", QString::number(numberIgnoredPoints()));
    stream.writeTextElement("numberHeldImages", QString::number(numberHeldImages()));
    stream.writeTextElement("rejectionLimit", QString::number(rejectionLimit()));
    stream.writeTextElement("numberRejectedObservations", QString::number(numberRejectedObservations()));
    stream.writeTextElement("numberObservations", QString::number(numberObservations()));
    stream.writeTextElement("numberLidarRangeConstraintEquations", QString::number(numberLidarRangeConstraintEquations()));
    stream.writeTextElement("numberImageObservations", QString::number(numberImageObservations()));
    stream.writeTextElement("numberLidarImageObservations", QString::number(numberLidarImageObservations()));
    stream.writeTextElement("numberImageParameters", QString::number(numberImageParameters()));
    stream.writeTextElement("numberConstrainedPointParameters",
                            QString::number(numberConstrainedPointParameters()));
    stream.writeTextElement("numberConstrainedImageParameters",
                            QString::number(numberConstrainedImageParameters()));
    stream.writeTextElement("numberConstrainedTargetParameters",
                            QString::number(numberConstrainedTargetParameters()));
    stream.writeTextElement("numberUnknownParameters", QString::number(numberUnknownParameters()));
    stream.writeTextElement("degreesOfFreedom", QString::number(degreesOfFreedom()));
    stream.writeTextElement("sigma0", QString::number(sigma0()));
    stream.writeTextElement("converged", QString::number(converged()));
    stream.writeTextElement("iterations", QString::number(iterations()));
    stream.writeEndElement(); // end generalStatisticsValues

    stream.writeStartElement("rms");
    stream.writeStartElement("residuals");
    stream.writeAttribute("x", QString::number(rmsRx()));
    stream.writeAttribute("y", QString::number(rmsRy()));
    stream.writeAttribute("xy", QString::number(rmsRxy()));
    stream.writeEndElement(); // end residuals element
    stream.writeStartElement("sigmas");

    // Set the label based of the coordinate type set for reports
    switch (coordType) {
      case SurfacePoint::Latitudinal:
        stream.writeAttribute("lat", QString::number(sigmaCoord1StatisticsRms()));
        stream.writeAttribute("lon", QString::number(sigmaCoord2StatisticsRms()));
        stream.writeAttribute("rad", QString::number(sigmaCoord3StatisticsRms()));
        break;
      case SurfacePoint::Rectangular:
        stream.writeAttribute("x", QString::number(sigmaCoord1StatisticsRms()));
        stream.writeAttribute("y", QString::number(sigmaCoord2StatisticsRms()));
        stream.writeAttribute("z", QString::number(sigmaCoord3StatisticsRms()));
        break;
      default:
         IString msg ="Unknown surface point coordinate type enum [" + toString(coordType) + "]." ;
         throw IException(IException::Programmer, msg, _FILEINFO_);
    }
    stream.writeEndElement(); // end sigmas element

    stream.writeStartElement("imageResidualsLists");
    stream.writeStartElement("residualsList");
    stream.writeAttribute("listSize", QString::number(rmsImageResiduals().size()));
    for (int i = 0; i < m_rmsImageResiduals.size(); i++) {
      stream.writeStartElement("statisticsItem");
      m_rmsImageResiduals[i].save(stream, project);
      stream.writeEndElement(); // end statistics item
    }
    stream.writeEndElement(); // end residuals list
    stream.writeStartElement("sampleList");
    stream.writeAttribute("listSize", QString::number(rmsImageSampleResiduals().size()));
    for (int i = 0; i < m_rmsImageSampleResiduals.size(); i++) {
      stream.writeStartElement("statisticsItem");
      m_rmsImageSampleResiduals[i].save(stream, project);
      stream.writeEndElement(); // end statistics item
    }
    stream.writeEndElement(); // end sample residuals list

    stream.writeStartElement("lineList");
    stream.writeAttribute("listSize", QString::number(rmsImageLineResiduals().size()));
    for (int i = 0; i < m_rmsImageLineResiduals.size(); i++) {
      stream.writeStartElement("statisticsItem");
      m_rmsImageLineResiduals[i].save(stream, project);
      stream.writeEndElement(); // end statistics item
    }
    stream.writeEndElement(); // end line residuals list

    stream.writeStartElement("lidarResidualsList");
    stream.writeAttribute("listSize", QString::number(rmsLidarImageResiduals().size()));
    for (int i = 0; i < m_rmsLidarImageResiduals.size(); i++) {
      stream.writeStartElement("statisticsItem");
      m_rmsLidarImageResiduals[i].save(stream, project);
      stream.writeEndElement(); // end statistics item
    }
    stream.writeEndElement(); // end line residuals list

    stream.writeStartElement("lidarSampleList");
    stream.writeAttribute("listSize", QString::number(rmsLidarImageSampleResiduals().size()));
    for (int i = 0; i < m_rmsLidarImageSampleResiduals.size(); i++) {
      stream.writeStartElement("statisticsItem");
      m_rmsLidarImageSampleResiduals[i].save(stream, project);
      stream.writeEndElement(); // end statistics item
    }
    stream.writeEndElement(); // end line residuals list

    stream.writeStartElement("lidarLineList");
    stream.writeAttribute("listSize", QString::number(rmsLidarImageLineResiduals().size()));
    for (int i = 0; i < m_rmsLidarImageLineResiduals.size(); i++) {
      stream.writeStartElement("statisticsItem");
      m_rmsLidarImageLineResiduals[i].save(stream, project);
      stream.writeEndElement(); // end statistics item
    }
    stream.writeEndElement(); // end line residuals list
    stream.writeEndElement(); // end image residuals lists

    stream.writeStartElement("imageSigmasLists");
    stream.writeStartElement("xSigmas");
    stream.writeAttribute("listSize", QString::number(rmsImageXSigmas().size()));
    for (int i = 0; i < m_rmsImageXSigmas.size(); i++) {
      stream.writeStartElement("statisticsItem");
      m_rmsImageXSigmas[i].save(stream, project);
      stream.writeEndElement(); // end statistics item
    }

    stream.writeEndElement(); // end x sigma list

    stream.writeStartElement("ySigmas");
    stream.writeAttribute("listSize", QString::number(rmsImageYSigmas().size()));
    for (int i = 0; i < m_rmsImageYSigmas.size(); i++) {
      stream.writeStartElement("statisticsItem");
      m_rmsImageYSigmas[i].save(stream, project);
      stream.writeEndElement(); // end statistics item
    }
    stream.writeEndElement(); // end y sigma list

    stream.writeStartElement("zSigmas");
    stream.writeAttribute("listSize", QString::number(rmsImageZSigmas().size()));
    for (int i = 0; i < m_rmsImageZSigmas.size(); i++) {
      stream.writeStartElement("statisticsItem");
      m_rmsImageZSigmas[i].save(stream, project);
      stream.writeEndElement(); // end statistics item
    }
    stream.writeEndElement(); // end z sigma list

    stream.writeStartElement("raSigmas");
    stream.writeAttribute("listSize", QString::number(rmsImageRASigmas().size()));
    for (int i = 0; i < m_rmsImageRASigmas.size(); i++) {
      stream.writeStartElement("statisticsItem");
      m_rmsImageRASigmas[i].save(stream, project);
      stream.writeEndElement(); // end statistics item
    }
    stream.writeEndElement(); // end ra sigma list

    stream.writeStartElement("decSigmas");
    stream.writeAttribute("listSize", QString::number(rmsImageDECSigmas().size()));
    for (int i = 0; i < m_rmsImageDECSigmas.size(); i++) {
      stream.writeStartElement("statisticsItem");
      m_rmsImageDECSigmas[i].save(stream, project);
      stream.writeEndElement(); // end statistics item
    }
    stream.writeEndElement(); // end dec sigma list

    stream.writeStartElement("twistSigmas");
    stream.writeAttribute("listSize", QString::number(rmsImageTWISTSigmas().size()));
    for (int i = 0; i < m_rmsImageTWISTSigmas.size(); i++) {
      stream.writeStartElement("statisticsItem");
      m_rmsImageTWISTSigmas[i].save(stream, project);
      stream.writeEndElement(); // end statistics item
    }
    stream.writeEndElement(); // end twist sigma list
    stream.writeEndElement(); // end sigmas lists
    stream.writeEndElement(); // end rms

    stream.writeStartElement("elapsedTime");
    stream.writeAttribute("time", QString::number(elapsedTime()));
    stream.writeAttribute("errorProp", QString::number(elapsedTimeErrorProp()));
    stream.writeEndElement(); // end elapsed time

    stream.writeStartElement("minMaxSigmas");

    // Write the labels corresponding to the coordinate type set for reports
    switch (coordType) {
      case SurfacePoint::Latitudinal:
        stream.writeStartElement("minLat");
        stream.writeAttribute("value", QString::number(minSigmaCoord1Distance().meters()));
        stream.writeAttribute("pointId", minSigmaCoord1PointId());
        stream.writeEndElement();
        stream.writeStartElement("maxLat");
        stream.writeAttribute("value", QString::number(maxSigmaCoord1Distance().meters()));
        stream.writeAttribute("pointId", maxSigmaCoord1PointId());
        stream.writeEndElement();
        stream.writeStartElement("minLon");
        stream.writeAttribute("value", QString::number(minSigmaCoord2Distance().meters()));
        stream.writeAttribute("pointId", minSigmaCoord2PointId());
        stream.writeEndElement();
        stream.writeStartElement("maxLon");
        stream.writeAttribute("value", QString::number(maxSigmaCoord2Distance().meters()));
        stream.writeAttribute("pointId", maxSigmaCoord2PointId());
        stream.writeEndElement();
        stream.writeStartElement("minRad");
        stream.writeAttribute("value", QString::number(minSigmaCoord3Distance().meters()));
        stream.writeAttribute("pointId", minSigmaCoord3PointId());
        stream.writeEndElement();
        stream.writeStartElement("maxRad");
        stream.writeAttribute("value", QString::number(maxSigmaCoord3Distance().meters()));
        stream.writeAttribute("pointId", maxSigmaCoord3PointId());
        stream.writeEndElement();
        break;
      case SurfacePoint::Rectangular:
        stream.writeStartElement("minX");
        stream.writeAttribute("value", QString::number(minSigmaCoord1Distance().meters()));
        stream.writeAttribute("pointId", minSigmaCoord1PointId());
        stream.writeEndElement();
        stream.writeStartElement("maxX");
        stream.writeAttribute("value", QString::number(maxSigmaCoord1Distance().meters()));
        stream.writeAttribute("pointId", maxSigmaCoord1PointId());
        stream.writeEndElement();
        stream.writeStartElement("minY");
        stream.writeAttribute("value", QString::number(minSigmaCoord2Distance().meters()));
        stream.writeAttribute("pointId", minSigmaCoord2PointId());
        stream.writeEndElement();
        stream.writeStartElement("maxY");
        stream.writeAttribute("value", QString::number(maxSigmaCoord2Distance().meters()));
        stream.writeAttribute("pointId", maxSigmaCoord2PointId());
        stream.writeEndElement();
        stream.writeStartElement("minZ");
        stream.writeAttribute("value", QString::number(minSigmaCoord3Distance().meters()));
        stream.writeAttribute("pointId", minSigmaCoord3PointId());
        stream.writeEndElement();
        stream.writeStartElement("maxZ");
        stream.writeAttribute("value", QString::number(maxSigmaCoord3Distance().meters()));
        stream.writeAttribute("pointId", maxSigmaCoord3PointId());
        stream.writeEndElement();
        break;
      default:
         IString msg ="Unknown surface point coordinate type enum [" + toString(coordType) + "]." ;
         throw IException(IException::Programmer, msg, _FILEINFO_);
    }
    stream.writeEndElement(); // end minMaxSigmas

    // call max likelihood setup from startElement to fill the rest of these values...
    stream.writeStartElement("maximumLikelihoodEstimation");
    stream.writeAttribute("numberModels", QString::number(numberMaximumLikelihoodModels()));
    stream.writeAttribute("maximumLikelihoodIndex", QString::number(maximumLikelihoodModelIndex()));
    stream.writeAttribute("maximumLikelihoodMedianR2Residuals",
                          QString::number(maximumLikelihoodMedianR2Residuals()));

    stream.writeStartElement("cumulativeProbabilityCalculator");
    // cumulativeProbabilityDistribution().save(stream, project);
    stream.writeEndElement(); // end cumulativeProbabilityCalculator

    stream.writeStartElement("residualsCumulativeProbabilityCalculator");
    // residualsCumulativeProbabilityDistribution().save(stream, project);
    stream.writeEndElement(); // end residualsCumulativeProbabilityCalculator

    for (int i = 0; i < numberMaximumLikelihoodModels(); i++) {
      stream.writeStartElement("model");
      stream.writeAttribute("modelNumber", QString::number(i+1));
      stream.writeAttribute("modelSelection",
        MaximumLikelihoodWFunctions::modelToString(m_maximumLikelihoodFunctions[i].first.model()));
      stream.writeAttribute("tweakingConstant",
                            QString::number(m_maximumLikelihoodFunctions[i].first.tweakingConstant()));
      stream.writeAttribute("quantile", QString::number(m_maximumLikelihoodFunctions[i].second));
      stream.writeEndElement(); // end this model
    }
    stream.writeEndElement(); // end maximumLikelihoodEstimation
    stream.writeEndElement(); // end bundleResults
  }
}