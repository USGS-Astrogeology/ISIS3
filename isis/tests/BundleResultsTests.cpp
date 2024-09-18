#include "BundleResults.h"

#include <random>
#include <cmath>

#include <QByteArray>
#include <QDataStream>
#include <QDebug>
#include <QFile>
#include <QList>
#include <QString>
#include <QtDebug>
#include <QXmlStreamWriter>
#include <QXmlStreamReader>

#include "BundleControlPoint.h"
#include "IsisBundleObservation.h"
#include "BundleObservationVector.h"
#include "BundleResults.h"
#include "Camera.h"
#include "ControlMeasure.h"
#include "ControlPoint.h"
#include "ControlNet.h"
#include "FileName.h"
#include "Fixtures.h"
#include "IException.h"
#include "MaximumLikelihoodWFunctions.h"
#include "Preference.h"
#include "PvlObject.h"

#include "gmock/gmock.h"

namespace Isis {
  /**
   * This class is used to test BundleResults' XmlHandler class.
   *
   * @author 2014-07-28 Jeannie Backer
   *
   * @internal
   *   @history 2014-07-28 Jeannie Backer - Original version.
   */
  class BundleResultsXmlHandlerTester : public BundleResults {
    public:
      /**
       * Constructs the tester object from an xml file.
       *
       * @param xmlFile The xml file to construct the tester from.
       */
      BundleResultsXmlHandlerTester(QXmlStreamReader *reader, FileName xmlFile) : BundleResults() {

        m_file.setFileName(QString::fromStdString(xmlFile.expanded()));

        if (!m_file.open(QFile::ReadOnly) ) {
          throw IException(IException::Io,
                           "Unable to open xml file, ["+xmlFile.expanded()+"],  with read access",
                           _FILEINFO_);
        }

        if (reader->readNextStartElement()) {
          if (reader->name() == "bundleResults") {
            readBundleResults(reader);
          }
          else {
            reader->raiseError(QObject::tr("Incorrect file"));
          }
        }
      }

      /**
       * Destroys the tester object
       */
      ~BundleResultsXmlHandlerTester() {
      }

      QFile m_file;

  };
}

class BundleResultsPopulated : public TempTestingFiles {
  protected:
    BundleResults results;
    int numImages;

    void SetUp() override {
      // Models for MLE
      QList< QPair< MaximumLikelihoodWFunctions::Model, double > > modelsWithQuantiles;
      modelsWithQuantiles.append(qMakePair(MaximumLikelihoodWFunctions::Huber, 0.1));
      modelsWithQuantiles.append(qMakePair(MaximumLikelihoodWFunctions::Chen, 0.3));
      modelsWithQuantiles.append(qMakePair(MaximumLikelihoodWFunctions::Welsch, 0.2));

      // Generate some random residuals to store
      numImages = 8;
      std::vector<double> imageMeasuresCounts = {10, 12, 13, 9, 6, 7, 20, 15};
      std::vector<double> imageLidarMeasuresCounts = {5, 7, 5, 3, 6, 7, 10, 2};

      std::default_random_engine randomGenerator;
      std::normal_distribution<double> residualDistribution(0.0, 2.0);

      QList<Statistics> rmsImageLineResiduals, rmsImageSampleResiduals, rmsImageResiduals;
      QList<Statistics> rmsLidarImageLineResiduals, rmsLidarImageSampleResiduals, rmsLidarImageResiduals;
      QList<double> allResiduals;

      for (int imageIndex = 0; imageIndex < numImages; imageIndex++) {
        Statistics imageLineStats, imageSampleStats, imageRmsStats;
        for (int measureIndex = 0; measureIndex < imageMeasuresCounts[imageIndex]; measureIndex++) {
          double imageLineRms = residualDistribution(randomGenerator);
          double imageSampleRms = residualDistribution(randomGenerator);
          double imageRms = sqrt(imageLineRms * imageLineRms + imageSampleRms * imageSampleRms);

          imageLineStats.AddData(imageLineRms);
          imageSampleStats.AddData(imageSampleRms);
          imageRmsStats.AddData(imageRms);

          allResiduals.append(imageRms);
        }

        rmsImageLineResiduals.push_back(imageLineStats);
        rmsImageSampleResiduals.push_back(imageSampleStats);
        rmsImageResiduals.push_back(imageRmsStats);

        Statistics lidarLineStats, lidarSampleStats, lidarRmsStats;
        for (int measureIndex = 0; measureIndex < imageLidarMeasuresCounts[imageIndex]; measureIndex++) {
          double lidarLineRms = residualDistribution(randomGenerator);
          double lidarSampleRms = residualDistribution(randomGenerator);
          double lidarRms = sqrt(lidarLineRms * lidarLineRms + lidarSampleRms * lidarSampleRms);

          lidarLineStats.AddData(lidarLineRms);
          lidarSampleStats.AddData(lidarSampleRms);
          lidarRmsStats.AddData(lidarRms);

          allResiduals.append(lidarRms);
        }

        rmsLidarImageLineResiduals.push_back(lidarLineStats);
        rmsLidarImageSampleResiduals.push_back(lidarSampleStats);
        rmsLidarImageResiduals.push_back(lidarRmsStats);
      }

      // Generate an arbitrary control network
      ControlNetQsp network(new ControlNet());
      ControlPoint *testPoint = new ControlPoint("test");
      BundleSettingsQsp settings = BundleSettingsQsp(new BundleSettings());
      BundleControlPointQsp bundlePoint(new BundleControlPoint(settings, testPoint));
      QVector<BundleControlPointQsp> bundleControlPointVector = {bundlePoint};

      // Generate arbirary lidar data
      LidarDataQsp lidarNetwork(new LidarData());
      LidarControlPointQsp testLidarPoint(new LidarControlPoint());
      BundleLidarControlPointQsp testLidarBundlePoint(new BundleLidarControlPoint(settings, testLidarPoint));
      QVector<BundleLidarControlPointQsp> bundleLidarPointVector = {testLidarBundlePoint};

      // Populate the results
      results.setRmsImageResidualLists(
              rmsImageLineResiduals,
              rmsImageSampleResiduals,
              rmsImageResiduals);

      results.setRmsLidarImageResidualLists(
              rmsLidarImageLineResiduals,
              rmsLidarImageSampleResiduals,
              rmsLidarImageResiduals);

      results.setSigmaCoord1Range(
              Distance(0.5, Distance::Meters),
              Distance(50, Distance::Meters),
              "minCoord1Point", "maxCoord1Point");
      results.setSigmaCoord2Range(
              Distance(0.3, Distance::Meters),
              Distance(30, Distance::Meters),
              "minCoord2Point", "maxCoord2Point");
      results.setSigmaCoord3Range(
              Distance(0.1, Distance::Meters),
              Distance(10, Distance::Meters),
              "minCoord3Point", "maxCoord3Point");
      results.setRmsFromSigmaStatistics(5, 3, 1);

      results.maximumLikelihoodSetUp(modelsWithQuantiles);
      while (results.maximumLikelihoodModelIndex() <= results.numberMaximumLikelihoodModels()) {
        for(auto & residual : allResiduals) {
          results.addProbabilityDistributionObservation(residual);
          results.addResidualsProbabilityDistributionObservation(residual);
        }
        results.incrementMaximumLikelihoodModelIndex();
      }

      results.setBundleControlPoints(bundleControlPointVector);
      results.setBundleLidarPoints(bundleLidarPointVector);
      results.setOutputControlNet(network);
      results.setOutputLidarData(lidarNetwork);

      for (int i = 0; i < 10; i++) {
        results.incrementFixedPoints();
      }
      for (int i = 0; i < 5; i++) {
        results.incrementIgnoredPoints();
      }
      for (int i = 0; i < 2; i++) {
        results.incrementHeldImages();
      }
      for (int i = 0; i < 6; i++) {
        results.incrementNumberConstrainedImageParameters(1);
      }
      for (int i = 0; i < 3; i++) {
        results.incrementNumberConstrainedTargetParameters(1);
      }

      results.setRejectionLimit(5.0);

      results.setRmsXYResiduals(3.0, 4.0, 5.0);

      results.setNumberRejectedObservations(1);
      results.setNumberImageObservations(100);
      results.setNumberLidarImageObservations(50);
      results.setNumberObservations(150);
      results.setNumberImageParameters(600);
      results.setNumberConstrainedPointParameters(50);
      results.setNumberConstrainedLidarPointParameters(25);
      results.setNumberLidarRangeConstraints(10);
      results.setNumberUnknownParameters(1000);
      results.computeDegreesOfFreedom();

      results.setSigma0(0.5);
      results.setElapsedTime(180);
      results.setElapsedTimeErrorProp(75);
      results.setConverged(true);
      results.setIterations(5);
    }
};


TEST(BundleResults, DefaultConstructor) {
  BundleResults defaultResults;
  EXPECT_EQ(defaultResults.numberFixedPoints(), 0);
  EXPECT_EQ(defaultResults.numberHeldImages(), 0);
  EXPECT_EQ(defaultResults.numberIgnoredPoints(), 0);
  EXPECT_EQ(defaultResults.numberRejectedObservations(), 0);
  EXPECT_EQ(defaultResults.numberObservations(), 0);
  EXPECT_EQ(defaultResults.numberImageObservations(), 0);
  EXPECT_EQ(defaultResults.numberLidarImageObservations(), 0);
  EXPECT_EQ(defaultResults.numberImageParameters(), 0);
  EXPECT_EQ(defaultResults.numberConstrainedPointParameters(), 0);
  EXPECT_EQ(defaultResults.numberConstrainedImageParameters(), 0);
  EXPECT_EQ(defaultResults.numberConstrainedTargetParameters(), 0);
  EXPECT_EQ(defaultResults.numberLidarRangeConstraintEquations(), 0);
  EXPECT_EQ(defaultResults.numberUnknownParameters(), 0);
  EXPECT_EQ(defaultResults.degreesOfFreedom(), -1);

  EXPECT_EQ(defaultResults.rmsImageSampleResiduals().size(), 0);
  EXPECT_EQ(defaultResults.rmsImageLineResiduals().size(), 0);
  EXPECT_EQ(defaultResults.rmsImageResiduals().size(), 0);
  EXPECT_EQ(defaultResults.rmsLidarImageSampleResiduals().size(), 0);
  EXPECT_EQ(defaultResults.rmsLidarImageLineResiduals().size(), 0);
  EXPECT_EQ(defaultResults.rmsLidarImageResiduals().size(), 0);
  EXPECT_EQ(defaultResults.rmsImageXSigmas().size(), 0);
  EXPECT_EQ(defaultResults.rmsImageYSigmas().size(), 0);
  EXPECT_EQ(defaultResults.rmsImageZSigmas().size(), 0);
  EXPECT_EQ(defaultResults.rmsImageRASigmas().size(), 0);
  EXPECT_EQ(defaultResults.rmsImageDECSigmas().size(), 0);
  EXPECT_EQ(defaultResults.rmsImageTWISTSigmas().size(), 0);
  EXPECT_EQ(defaultResults.sigmaCoord1StatisticsRms(), 0);
  EXPECT_EQ(defaultResults.sigmaCoord2StatisticsRms(), 0);
  EXPECT_EQ(defaultResults.sigmaCoord3StatisticsRms(), 0);
  EXPECT_GT(defaultResults.minSigmaCoord1Distance().meters(),
            defaultResults.maxSigmaCoord1Distance().meters());
  EXPECT_GT(defaultResults.minSigmaCoord2Distance().meters(),
            defaultResults.maxSigmaCoord2Distance().meters());
  EXPECT_GT(defaultResults.minSigmaCoord3Distance().meters(),
            defaultResults.maxSigmaCoord3Distance().meters());
  EXPECT_TRUE(defaultResults.maxSigmaCoord1PointId().isEmpty());
  EXPECT_TRUE(defaultResults.minSigmaCoord1PointId().isEmpty());
  EXPECT_TRUE(defaultResults.maxSigmaCoord2PointId().isEmpty());
  EXPECT_TRUE(defaultResults.minSigmaCoord2PointId().isEmpty());
  EXPECT_TRUE(defaultResults.maxSigmaCoord3PointId().isEmpty());
  EXPECT_TRUE(defaultResults.minSigmaCoord3PointId().isEmpty());
  EXPECT_EQ(defaultResults.rmsRx(), 0);
  EXPECT_EQ(defaultResults.rmsRy(), 0);
  EXPECT_EQ(defaultResults.rmsRxy(), 0);
  EXPECT_EQ(defaultResults.rejectionLimit(), 0);

  EXPECT_EQ(defaultResults.sigma0(), 0);
  EXPECT_EQ(defaultResults.elapsedTime(), 0);
  EXPECT_EQ(defaultResults.elapsedTimeErrorProp(), 0);
  EXPECT_EQ(defaultResults.iterations(), 0);
  EXPECT_FALSE(defaultResults.converged());

  EXPECT_TRUE(defaultResults.bundleControlPoints().empty());
  EXPECT_TRUE(defaultResults.bundleLidarControlPoints().empty());
  EXPECT_TRUE(defaultResults.observations().empty());
  EXPECT_EQ(defaultResults.outputLidarData(), LidarDataQsp());

  EXPECT_EQ(defaultResults.numberMaximumLikelihoodModels(), 0);
  EXPECT_EQ(defaultResults.maximumLikelihoodModelIndex(), 0);
  EXPECT_EQ(defaultResults.maximumLikelihoodMedianR2Residuals(), 0);

  EXPECT_EQ(defaultResults.coordTypeReports(), SurfacePoint::Latitudinal);
}


TEST_F(BundleResultsPopulated, CopyConstructor) {
  BundleResults newResults(results);

  EXPECT_EQ(newResults.numberFixedPoints(), results.numberFixedPoints());
  EXPECT_EQ(newResults.numberHeldImages(), results.numberHeldImages());
  EXPECT_EQ(newResults.numberIgnoredPoints(), results.numberIgnoredPoints());
  EXPECT_EQ(newResults.numberRejectedObservations(), results.numberRejectedObservations());
  EXPECT_EQ(newResults.numberObservations(), results.numberObservations());
  EXPECT_EQ(newResults.numberImageObservations(), results.numberImageObservations());
  EXPECT_EQ(newResults.numberLidarImageObservations(), results.numberLidarImageObservations());
  EXPECT_EQ(newResults.numberImageParameters(), results.numberImageParameters());
  EXPECT_EQ(newResults.numberConstrainedPointParameters(), results.numberConstrainedPointParameters());
  EXPECT_EQ(newResults.numberConstrainedImageParameters(), results.numberConstrainedImageParameters());
  EXPECT_EQ(newResults.numberConstrainedTargetParameters(), results.numberConstrainedTargetParameters());
  EXPECT_EQ(newResults.numberLidarRangeConstraintEquations(), results.numberLidarRangeConstraintEquations());
  EXPECT_EQ(newResults.numberUnknownParameters(), results.numberUnknownParameters());
  EXPECT_EQ(newResults.degreesOfFreedom(), results.degreesOfFreedom());

  ASSERT_EQ(newResults.rmsImageSampleResiduals().size(), results.rmsImageSampleResiduals().size());
  for (int imageIndex = 0; imageIndex < newResults.rmsImageSampleResiduals().size(); imageIndex++) {
    EXPECT_EQ(
          newResults.rmsImageSampleResiduals()[imageIndex].Average(),
          results.rmsImageSampleResiduals()[imageIndex].Average()) << "at index " << imageIndex;
  }
  ASSERT_EQ(newResults.rmsImageLineResiduals().size(), results.rmsImageLineResiduals().size());
  for (int imageIndex = 0; imageIndex < newResults.rmsImageLineResiduals().size(); imageIndex++) {
    EXPECT_EQ(
          newResults.rmsImageLineResiduals()[imageIndex].Average(),
          results.rmsImageLineResiduals()[imageIndex].Average()) << "at index " << imageIndex;
  }
  ASSERT_EQ(newResults.rmsImageResiduals().size(), results.rmsImageResiduals().size());
  for (int imageIndex = 0; imageIndex < newResults.rmsImageResiduals().size(); imageIndex++) {
    EXPECT_EQ(
          newResults.rmsImageResiduals()[imageIndex].Average(),
          results.rmsImageResiduals()[imageIndex].Average()) << "at index " << imageIndex;
  }
  ASSERT_EQ(newResults.rmsLidarImageSampleResiduals().size(), results.rmsLidarImageSampleResiduals().size());
  for (int imageIndex = 0; imageIndex < newResults.rmsLidarImageSampleResiduals().size(); imageIndex++) {
    EXPECT_EQ(
          newResults.rmsLidarImageSampleResiduals()[imageIndex].Average(),
          results.rmsLidarImageSampleResiduals()[imageIndex].Average()) << "at index " << imageIndex;
  }
  ASSERT_EQ(newResults.rmsLidarImageLineResiduals().size(), results.rmsLidarImageLineResiduals().size());
  for (int imageIndex = 0; imageIndex < newResults.rmsLidarImageLineResiduals().size(); imageIndex++) {
    EXPECT_EQ(
          newResults.rmsLidarImageLineResiduals()[imageIndex].Average(),
          results.rmsLidarImageLineResiduals()[imageIndex].Average()) << "at index " << imageIndex;
  }
  ASSERT_EQ(newResults.rmsLidarImageResiduals().size(), results.rmsLidarImageResiduals().size());
  for (int imageIndex = 0; imageIndex < newResults.rmsLidarImageResiduals().size(); imageIndex++) {
    EXPECT_EQ(
          newResults.rmsLidarImageResiduals()[imageIndex].Average(),
          results.rmsLidarImageResiduals()[imageIndex].Average()) << "at index " << imageIndex;
  }
  EXPECT_EQ(newResults.minSigmaCoord1Distance().meters(), results.minSigmaCoord1Distance().meters());
  EXPECT_EQ(newResults.maxSigmaCoord1Distance().meters(), results.maxSigmaCoord1Distance().meters());
  EXPECT_EQ(newResults.minSigmaCoord2Distance().meters(), results.minSigmaCoord2Distance().meters());
  EXPECT_EQ(newResults.maxSigmaCoord2Distance().meters(), results.maxSigmaCoord2Distance().meters());
  EXPECT_EQ(newResults.minSigmaCoord3Distance().meters(), results.minSigmaCoord3Distance().meters());
  EXPECT_EQ(newResults.maxSigmaCoord3Distance().meters(), results.maxSigmaCoord3Distance().meters());
  EXPECT_EQ(newResults.minSigmaCoord1PointId().toStdString(), results.minSigmaCoord1PointId().toStdString());
  EXPECT_EQ(newResults.maxSigmaCoord1PointId().toStdString(), results.maxSigmaCoord1PointId().toStdString());
  EXPECT_EQ(newResults.minSigmaCoord2PointId().toStdString(), results.minSigmaCoord2PointId().toStdString());
  EXPECT_EQ(newResults.maxSigmaCoord2PointId().toStdString(), results.maxSigmaCoord2PointId().toStdString());
  EXPECT_EQ(newResults.minSigmaCoord3PointId().toStdString(), results.minSigmaCoord3PointId().toStdString());
  EXPECT_EQ(newResults.maxSigmaCoord3PointId().toStdString(), results.maxSigmaCoord3PointId().toStdString());
  EXPECT_EQ(newResults.rmsRx(), results.rmsRx());
  EXPECT_EQ(newResults.rmsRy(), results.rmsRy());
  EXPECT_EQ(newResults.rmsRxy(), results.rmsRxy());
  EXPECT_EQ(newResults.rejectionLimit(), results.rejectionLimit());
  EXPECT_EQ(newResults.sigma0(), results.sigma0());
  EXPECT_EQ(newResults.elapsedTime(), results.elapsedTime());
  EXPECT_EQ(newResults.elapsedTimeErrorProp(), results.elapsedTimeErrorProp());
  EXPECT_EQ(newResults.iterations(), results.iterations());
  EXPECT_EQ(newResults.converged(), results.converged());

  EXPECT_EQ(newResults.bundleControlPoints().empty(), results.bundleControlPoints().empty());
  EXPECT_EQ(newResults.bundleLidarControlPoints().empty(), results.bundleLidarControlPoints().empty());
  EXPECT_EQ(newResults.outputLidarData(), results.outputLidarData());

  EXPECT_EQ(newResults.numberMaximumLikelihoodModels(), results.numberMaximumLikelihoodModels());

  // Reset the MLE index so we can compare them one by one
  newResults.printMaximumLikelihoodTierInformation();
  results.printMaximumLikelihoodTierInformation();

  EXPECT_EQ(newResults.maximumLikelihoodMedianR2Residuals(), results.maximumLikelihoodMedianR2Residuals());

  newResults.incrementMaximumLikelihoodModelIndex();
  results.incrementMaximumLikelihoodModelIndex();
  EXPECT_EQ(newResults.maximumLikelihoodMedianR2Residuals(), results.maximumLikelihoodMedianR2Residuals());

  newResults.incrementMaximumLikelihoodModelIndex();
  results.incrementMaximumLikelihoodModelIndex();
  EXPECT_EQ(newResults.maximumLikelihoodMedianR2Residuals(), results.maximumLikelihoodMedianR2Residuals());

  EXPECT_EQ(newResults.coordTypeReports(), results.coordTypeReports());
}


TEST_F(BundleResultsPopulated, Assignment) {
  BundleResults newResults;
  newResults = results;

  EXPECT_EQ(newResults.numberFixedPoints(), results.numberFixedPoints());
  EXPECT_EQ(newResults.numberHeldImages(), results.numberHeldImages());
  EXPECT_EQ(newResults.numberIgnoredPoints(), results.numberIgnoredPoints());
  EXPECT_EQ(newResults.numberRejectedObservations(), results.numberRejectedObservations());
  EXPECT_EQ(newResults.numberObservations(), results.numberObservations());
  EXPECT_EQ(newResults.numberImageObservations(), results.numberImageObservations());
  EXPECT_EQ(newResults.numberLidarImageObservations(), results.numberLidarImageObservations());
  EXPECT_EQ(newResults.numberImageParameters(), results.numberImageParameters());
  EXPECT_EQ(newResults.numberConstrainedPointParameters(), results.numberConstrainedPointParameters());
  EXPECT_EQ(newResults.numberConstrainedImageParameters(), results.numberConstrainedImageParameters());
  EXPECT_EQ(newResults.numberConstrainedTargetParameters(), results.numberConstrainedTargetParameters());
  EXPECT_EQ(newResults.numberLidarRangeConstraintEquations(), results.numberLidarRangeConstraintEquations());
  EXPECT_EQ(newResults.numberUnknownParameters(), results.numberUnknownParameters());
  EXPECT_EQ(newResults.degreesOfFreedom(), results.degreesOfFreedom());

  ASSERT_EQ(newResults.rmsImageSampleResiduals().size(), results.rmsImageSampleResiduals().size());
  for (int imageIndex = 0; imageIndex < newResults.rmsImageSampleResiduals().size(); imageIndex++) {
    EXPECT_EQ(
          newResults.rmsImageSampleResiduals()[imageIndex].Average(),
          results.rmsImageSampleResiduals()[imageIndex].Average()) << "at index " << imageIndex;
  }
  ASSERT_EQ(newResults.rmsImageLineResiduals().size(), results.rmsImageLineResiduals().size());
  for (int imageIndex = 0; imageIndex < newResults.rmsImageLineResiduals().size(); imageIndex++) {
    EXPECT_EQ(
          newResults.rmsImageLineResiduals()[imageIndex].Average(),
          results.rmsImageLineResiduals()[imageIndex].Average()) << "at index " << imageIndex;
  }
  ASSERT_EQ(newResults.rmsImageResiduals().size(), results.rmsImageResiduals().size());
  for (int imageIndex = 0; imageIndex < newResults.rmsImageResiduals().size(); imageIndex++) {
    EXPECT_EQ(
          newResults.rmsImageResiduals()[imageIndex].Average(),
          results.rmsImageResiduals()[imageIndex].Average()) << "at index " << imageIndex;
  }
  ASSERT_EQ(newResults.rmsLidarImageSampleResiduals().size(), results.rmsLidarImageSampleResiduals().size());
  for (int imageIndex = 0; imageIndex < newResults.rmsLidarImageSampleResiduals().size(); imageIndex++) {
    EXPECT_EQ(
          newResults.rmsLidarImageSampleResiduals()[imageIndex].Average(),
          results.rmsLidarImageSampleResiduals()[imageIndex].Average()) << "at index " << imageIndex;
  }
  ASSERT_EQ(newResults.rmsLidarImageLineResiduals().size(), results.rmsLidarImageLineResiduals().size());
  for (int imageIndex = 0; imageIndex < newResults.rmsLidarImageLineResiduals().size(); imageIndex++) {
    EXPECT_EQ(
          newResults.rmsLidarImageLineResiduals()[imageIndex].Average(),
          results.rmsLidarImageLineResiduals()[imageIndex].Average()) << "at index " << imageIndex;
  }
  ASSERT_EQ(newResults.rmsLidarImageResiduals().size(), results.rmsLidarImageResiduals().size());
  for (int imageIndex = 0; imageIndex < newResults.rmsLidarImageResiduals().size(); imageIndex++) {
    EXPECT_EQ(
          newResults.rmsLidarImageResiduals()[imageIndex].Average(),
          results.rmsLidarImageResiduals()[imageIndex].Average()) << "at index " << imageIndex;
  }
  EXPECT_EQ(newResults.minSigmaCoord1Distance().meters(), results.minSigmaCoord1Distance().meters());
  EXPECT_EQ(newResults.maxSigmaCoord1Distance().meters(), results.maxSigmaCoord1Distance().meters());
  EXPECT_EQ(newResults.minSigmaCoord2Distance().meters(), results.minSigmaCoord2Distance().meters());
  EXPECT_EQ(newResults.maxSigmaCoord2Distance().meters(), results.maxSigmaCoord2Distance().meters());
  EXPECT_EQ(newResults.minSigmaCoord3Distance().meters(), results.minSigmaCoord3Distance().meters());
  EXPECT_EQ(newResults.maxSigmaCoord3Distance().meters(), results.maxSigmaCoord3Distance().meters());
  EXPECT_EQ(newResults.minSigmaCoord1PointId().toStdString(), results.minSigmaCoord1PointId().toStdString());
  EXPECT_EQ(newResults.maxSigmaCoord1PointId().toStdString(), results.maxSigmaCoord1PointId().toStdString());
  EXPECT_EQ(newResults.minSigmaCoord2PointId().toStdString(), results.minSigmaCoord2PointId().toStdString());
  EXPECT_EQ(newResults.maxSigmaCoord2PointId().toStdString(), results.maxSigmaCoord2PointId().toStdString());
  EXPECT_EQ(newResults.minSigmaCoord3PointId().toStdString(), results.minSigmaCoord3PointId().toStdString());
  EXPECT_EQ(newResults.maxSigmaCoord3PointId().toStdString(), results.maxSigmaCoord3PointId().toStdString());
  EXPECT_EQ(newResults.rmsRx(), results.rmsRx());
  EXPECT_EQ(newResults.rmsRy(), results.rmsRy());
  EXPECT_EQ(newResults.rmsRxy(), results.rmsRxy());
  EXPECT_EQ(newResults.rejectionLimit(), results.rejectionLimit());
  EXPECT_EQ(newResults.sigma0(), results.sigma0());
  EXPECT_EQ(newResults.elapsedTime(), results.elapsedTime());
  EXPECT_EQ(newResults.elapsedTimeErrorProp(), results.elapsedTimeErrorProp());
  EXPECT_EQ(newResults.iterations(), results.iterations());
  EXPECT_EQ(newResults.converged(), results.converged());

  EXPECT_EQ(newResults.bundleControlPoints().empty(), results.bundleControlPoints().empty());
  EXPECT_EQ(newResults.bundleLidarControlPoints().empty(), results.bundleLidarControlPoints().empty());
  EXPECT_EQ(newResults.outputLidarData(), results.outputLidarData());

  EXPECT_EQ(newResults.numberMaximumLikelihoodModels(), results.numberMaximumLikelihoodModels());

  // Reset the MLE index so we can compare them one by one
  newResults.printMaximumLikelihoodTierInformation();
  results.printMaximumLikelihoodTierInformation();

  EXPECT_EQ(newResults.maximumLikelihoodMedianR2Residuals(), results.maximumLikelihoodMedianR2Residuals());

  newResults.incrementMaximumLikelihoodModelIndex();
  results.incrementMaximumLikelihoodModelIndex();
  EXPECT_EQ(newResults.maximumLikelihoodMedianR2Residuals(), results.maximumLikelihoodMedianR2Residuals());

  newResults.incrementMaximumLikelihoodModelIndex();
  results.incrementMaximumLikelihoodModelIndex();
  EXPECT_EQ(newResults.maximumLikelihoodMedianR2Residuals(), results.maximumLikelihoodMedianR2Residuals());

  EXPECT_EQ(newResults.coordTypeReports(), results.coordTypeReports());
}


TEST_F(BundleResultsPopulated, Accessors) {
  EXPECT_EQ(10, results.numberFixedPoints());
  EXPECT_EQ(2, results.numberHeldImages());
  EXPECT_EQ(5, results.numberIgnoredPoints());
  EXPECT_EQ(1, results.numberRejectedObservations());
  EXPECT_EQ(150, results.numberObservations());
  EXPECT_EQ(100, results.numberImageObservations());
  EXPECT_EQ(50, results.numberLidarImageObservations());
  EXPECT_EQ(600, results.numberImageParameters());
  EXPECT_EQ(50, results.numberConstrainedPointParameters());
  EXPECT_EQ(6, results.numberConstrainedImageParameters());
  EXPECT_EQ(3, results.numberConstrainedTargetParameters());
  EXPECT_EQ(10, results.numberLidarRangeConstraintEquations());
  EXPECT_EQ(1000, results.numberUnknownParameters());
  EXPECT_EQ(numImages, results.rmsImageSampleResiduals().size());
  EXPECT_EQ(numImages, results.rmsImageLineResiduals().size());
  EXPECT_EQ(numImages, results.rmsImageResiduals().size());
  EXPECT_EQ(numImages, results.rmsLidarImageSampleResiduals().size());
  EXPECT_EQ(numImages, results.rmsLidarImageLineResiduals().size());
  EXPECT_EQ(numImages, results.rmsLidarImageResiduals().size());
  EXPECT_EQ(0.5, results.minSigmaCoord1Distance().meters());
  EXPECT_EQ(50, results.maxSigmaCoord1Distance().meters());
  EXPECT_EQ(0.3, results.minSigmaCoord2Distance().meters());
  EXPECT_EQ(30, results.maxSigmaCoord2Distance().meters());
  EXPECT_EQ(0.1, results.minSigmaCoord3Distance().meters());
  EXPECT_EQ(10, results.maxSigmaCoord3Distance().meters());
  EXPECT_EQ("minCoord1Point", results.minSigmaCoord1PointId().toStdString());
  EXPECT_EQ("maxCoord1Point", results.maxSigmaCoord1PointId().toStdString());
  EXPECT_EQ("minCoord2Point", results.minSigmaCoord2PointId().toStdString());
  EXPECT_EQ("maxCoord2Point", results.maxSigmaCoord2PointId().toStdString());
  EXPECT_EQ("minCoord3Point", results.minSigmaCoord3PointId().toStdString());
  EXPECT_EQ("maxCoord3Point", results.maxSigmaCoord3PointId().toStdString());
  EXPECT_EQ(3.0, results.rmsRx());
  EXPECT_EQ(4.0, results.rmsRy());
  EXPECT_EQ(5.0, results.rmsRxy());
  EXPECT_EQ(5.0, results.rejectionLimit());
  EXPECT_EQ(0.5, results.sigma0());
  EXPECT_EQ(180, results.elapsedTime());
  EXPECT_EQ(75, results.elapsedTimeErrorProp());
  EXPECT_EQ(5, results.iterations());
  EXPECT_TRUE(results.converged());
}


TEST(BundleResults, Sigma0Computation) {
  BundleResults results;
  try {
    results.computeSigma0(56.0, BundleSettings::Sigma0);
    FAIL() << "Expected an exception";
  }
  catch (IException &e) {
    EXPECT_THAT(
           e.toString(),
          ::testing::HasSubstr("Computed degrees of freedom ["));
  }
  catch (...) {
    FAIL() << "Expected an IException";
  }
  results.setNumberImageObservations(14);
  results.computeDegreesOfFreedom();
  results.computeSigma0(56.0, BundleSettings::Sigma0);
  EXPECT_EQ(2.0, results.sigma0());

  results.setNumberImageObservations(0);
  results.computeDegreesOfFreedom();
  results.computeSigma0(9.0, BundleSettings::ParameterCorrections);
  EXPECT_EQ(3.0, results.sigma0());
}


TEST(BundleResults, NoOutputNet) {
  BundleResults results;
  try {
    results.outputControlNet();
    FAIL() << "Expected and exception";
  }
  catch (IException &e) {
    EXPECT_THAT(
           e.toString(),
          ::testing::HasSubstr("Output Control Network has not been set."));
  }
  catch (...) {
    FAIL() << "Expected an IException";
  }
}


TEST_F(BundleResultsPopulated, Serialization) {
  QString saveFile = tempDir.path() + "/BundleResultsTestData.xml";
  QFile qXmlFile(saveFile);
  qXmlFile.open(QIODevice::ReadWrite);
  QXmlStreamWriter writer(&qXmlFile);
  writer.setAutoFormatting(true);
  writer.writeStartDocument();
  results.save(writer, NULL);
  writer.writeEndDocument();
  qXmlFile.close();

  QFile xml(saveFile);
  if(!xml.open(QFile::ReadOnly | QFile::Text)){
    throw IException(IException::Unknown,
                      "Failed to parse xml file, ["+xml.fileName().toStdString()+"]",
                      _FILEINFO_);
  }

  QXmlStreamReader reader(&xml);
  BundleResultsXmlHandlerTester newResults(&reader, saveFile.toStdString());

  EXPECT_EQ(newResults.numberFixedPoints(), results.numberFixedPoints());
  EXPECT_EQ(newResults.numberHeldImages(), results.numberHeldImages());
  EXPECT_EQ(newResults.numberIgnoredPoints(), results.numberIgnoredPoints());
  EXPECT_EQ(newResults.numberRejectedObservations(), results.numberRejectedObservations());
  EXPECT_EQ(newResults.numberObservations(), results.numberObservations());
  EXPECT_EQ(newResults.numberImageObservations(), results.numberImageObservations());
  EXPECT_EQ(newResults.numberLidarImageObservations(), results.numberLidarImageObservations());
  EXPECT_EQ(newResults.numberImageParameters(), results.numberImageParameters());
  EXPECT_EQ(newResults.numberConstrainedPointParameters(), results.numberConstrainedPointParameters());
  EXPECT_EQ(newResults.numberConstrainedImageParameters(), results.numberConstrainedImageParameters());
  EXPECT_EQ(newResults.numberConstrainedTargetParameters(), results.numberConstrainedTargetParameters());
  EXPECT_EQ(newResults.numberLidarRangeConstraintEquations(), results.numberLidarRangeConstraintEquations());
  EXPECT_EQ(newResults.numberUnknownParameters(), results.numberUnknownParameters());
  EXPECT_EQ(newResults.degreesOfFreedom(), results.degreesOfFreedom());

  // The Statistics class handles saving itself, so just make sure we have the correct amount
  EXPECT_EQ(newResults.rmsImageSampleResiduals().size(), results.rmsImageSampleResiduals().size());
  EXPECT_EQ(newResults.rmsImageLineResiduals().size(), results.rmsImageLineResiduals().size());
  EXPECT_EQ(newResults.rmsImageResiduals().size(), results.rmsImageResiduals().size());
  EXPECT_EQ(newResults.rmsLidarImageSampleResiduals().size(), results.rmsLidarImageSampleResiduals().size());
  EXPECT_EQ(newResults.rmsLidarImageLineResiduals().size(), results.rmsLidarImageLineResiduals().size());
  EXPECT_EQ(newResults.rmsLidarImageResiduals().size(), results.rmsLidarImageResiduals().size());
  EXPECT_EQ(newResults.minSigmaCoord1Distance().meters(), results.minSigmaCoord1Distance().meters());
  EXPECT_EQ(newResults.maxSigmaCoord1Distance().meters(), results.maxSigmaCoord1Distance().meters());
  EXPECT_EQ(newResults.minSigmaCoord2Distance().meters(), results.minSigmaCoord2Distance().meters());
  EXPECT_EQ(newResults.maxSigmaCoord2Distance().meters(), results.maxSigmaCoord2Distance().meters());
  EXPECT_EQ(newResults.minSigmaCoord3Distance().meters(), results.minSigmaCoord3Distance().meters());
  EXPECT_EQ(newResults.maxSigmaCoord3Distance().meters(), results.maxSigmaCoord3Distance().meters());
  EXPECT_EQ(newResults.minSigmaCoord1PointId().toStdString(), results.minSigmaCoord1PointId().toStdString());
  EXPECT_EQ(newResults.maxSigmaCoord1PointId().toStdString(), results.maxSigmaCoord1PointId().toStdString());
  EXPECT_EQ(newResults.minSigmaCoord2PointId().toStdString(), results.minSigmaCoord2PointId().toStdString());
  EXPECT_EQ(newResults.maxSigmaCoord2PointId().toStdString(), results.maxSigmaCoord2PointId().toStdString());
  EXPECT_EQ(newResults.minSigmaCoord3PointId().toStdString(), results.minSigmaCoord3PointId().toStdString());
  EXPECT_EQ(newResults.maxSigmaCoord3PointId().toStdString(), results.maxSigmaCoord3PointId().toStdString());
  EXPECT_EQ(newResults.rmsRx(), results.rmsRx());
  EXPECT_EQ(newResults.rmsRy(), results.rmsRy());
  EXPECT_EQ(newResults.rmsRxy(), results.rmsRxy());
  EXPECT_EQ(newResults.rejectionLimit(), results.rejectionLimit());
  EXPECT_EQ(newResults.sigma0(), results.sigma0());
  EXPECT_EQ(newResults.elapsedTime(), results.elapsedTime());
  EXPECT_EQ(newResults.elapsedTimeErrorProp(), results.elapsedTimeErrorProp());
  EXPECT_EQ(newResults.iterations(), results.iterations());
  EXPECT_EQ(newResults.converged(), results.converged());

  EXPECT_EQ(newResults.numberMaximumLikelihoodModels(), results.numberMaximumLikelihoodModels());

  // Reset the MLE index so we can compare them one by one
  newResults.printMaximumLikelihoodTierInformation();
  results.printMaximumLikelihoodTierInformation();

  EXPECT_EQ(newResults.maximumLikelihoodMedianR2Residuals(), results.maximumLikelihoodMedianR2Residuals());

  newResults.incrementMaximumLikelihoodModelIndex();
  results.incrementMaximumLikelihoodModelIndex();
  EXPECT_EQ(newResults.maximumLikelihoodMedianR2Residuals(), results.maximumLikelihoodMedianR2Residuals());

  newResults.incrementMaximumLikelihoodModelIndex();
  results.incrementMaximumLikelihoodModelIndex();
  EXPECT_EQ(newResults.maximumLikelihoodMedianR2Residuals(), results.maximumLikelihoodMedianR2Residuals());

  EXPECT_EQ(newResults.coordTypeReports(), results.coordTypeReports());

  // The Statistics class handles saving itself, so just make sure we have the correct amount
  EXPECT_EQ(newResults.rmsImageSampleResiduals().size(), results.rmsImageSampleResiduals().size());
  EXPECT_EQ(newResults.rmsImageLineResiduals().size(), results.rmsImageLineResiduals().size());
  EXPECT_EQ(newResults.rmsImageResiduals().size(), results.rmsImageResiduals().size());
  EXPECT_EQ(newResults.rmsLidarImageSampleResiduals().size(), results.rmsLidarImageSampleResiduals().size());
  EXPECT_EQ(newResults.rmsLidarImageLineResiduals().size(), results.rmsLidarImageLineResiduals().size());
  EXPECT_EQ(newResults.rmsLidarImageResiduals().size(), results.rmsLidarImageResiduals().size());
  EXPECT_EQ(newResults.minSigmaCoord1Distance().meters(), results.minSigmaCoord1Distance().meters());
  EXPECT_EQ(newResults.maxSigmaCoord1Distance().meters(), results.maxSigmaCoord1Distance().meters());
  EXPECT_EQ(newResults.minSigmaCoord2Distance().meters(), results.minSigmaCoord2Distance().meters());
  EXPECT_EQ(newResults.maxSigmaCoord2Distance().meters(), results.maxSigmaCoord2Distance().meters());
  EXPECT_EQ(newResults.minSigmaCoord3Distance().meters(), results.minSigmaCoord3Distance().meters());
  EXPECT_EQ(newResults.maxSigmaCoord3Distance().meters(), results.maxSigmaCoord3Distance().meters());
  EXPECT_EQ(newResults.minSigmaCoord1PointId().toStdString(), results.minSigmaCoord1PointId().toStdString());
  EXPECT_EQ(newResults.maxSigmaCoord1PointId().toStdString(), results.maxSigmaCoord1PointId().toStdString());
  EXPECT_EQ(newResults.minSigmaCoord2PointId().toStdString(), results.minSigmaCoord2PointId().toStdString());
  EXPECT_EQ(newResults.maxSigmaCoord2PointId().toStdString(), results.maxSigmaCoord2PointId().toStdString());
  EXPECT_EQ(newResults.minSigmaCoord3PointId().toStdString(), results.minSigmaCoord3PointId().toStdString());
  EXPECT_EQ(newResults.maxSigmaCoord3PointId().toStdString(), results.maxSigmaCoord3PointId().toStdString());
  EXPECT_EQ(newResults.rmsRx(), results.rmsRx());
  EXPECT_EQ(newResults.rmsRy(), results.rmsRy());
  EXPECT_EQ(newResults.rmsRxy(), results.rmsRxy());
  EXPECT_EQ(newResults.rejectionLimit(), results.rejectionLimit());
  EXPECT_EQ(newResults.sigma0(), results.sigma0());
  EXPECT_EQ(newResults.elapsedTime(), results.elapsedTime());
  EXPECT_EQ(newResults.elapsedTimeErrorProp(), results.elapsedTimeErrorProp());
  EXPECT_EQ(newResults.iterations(), results.iterations());
  EXPECT_EQ(newResults.converged(), results.converged());

  EXPECT_EQ(newResults.numberMaximumLikelihoodModels(), results.numberMaximumLikelihoodModels());

  // Reset the MLE index so we can compare them one by one
  newResults.printMaximumLikelihoodTierInformation();
  results.printMaximumLikelihoodTierInformation();

  EXPECT_EQ(newResults.maximumLikelihoodMedianR2Residuals(), results.maximumLikelihoodMedianR2Residuals());

  newResults.incrementMaximumLikelihoodModelIndex();
  results.incrementMaximumLikelihoodModelIndex();
  EXPECT_EQ(newResults.maximumLikelihoodMedianR2Residuals(), results.maximumLikelihoodMedianR2Residuals());

  newResults.incrementMaximumLikelihoodModelIndex();
  results.incrementMaximumLikelihoodModelIndex();
  EXPECT_EQ(newResults.maximumLikelihoodMedianR2Residuals(), results.maximumLikelihoodMedianR2Residuals());

  EXPECT_EQ(newResults.coordTypeReports(), results.coordTypeReports());
}