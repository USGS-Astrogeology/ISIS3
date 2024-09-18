#include <QByteArray>
#include <QDataStream>
#include <QDebug>
#include <QFile>
#include <QList>
#include <QScopedPointer>
#include <QString>
#include <QtDebug>
#include <QXmlStreamWriter>

#include "BundleControlPoint.h"
#include "BundleObservation.h"
#include "BundleObservationVector.h"
#include "BundleResults.h"
#include "BundleSettings.h"
#include "BundleSolutionInfo.h"
#include "Camera.h"
#include "Control.h"
#include "ControlMeasure.h"
#include "ControlPoint.h"
#include "ControlNet.h"
#include "FileName.h"
#include "Fixtures.h"
#include "IException.h"
#include "ImageList.h"
#include "Preference.h"
#include "PvlObject.h"

#include "gmock/gmock.h"

/**
 * This class is needed to test the xml read/write methods.
 * @author 2015-??-?? Jeannie Backer
 *
 * @internal
 *   @history 2015-??-?? Jeannie Backer - Original version.
 */
namespace Isis {
  class BundleSolutionInfoXmlHandlerTester : public BundleSolutionInfo {
    public:
      BundleSolutionInfoXmlHandlerTester(QXmlStreamReader *reader, FileName xmlFile) : BundleSolutionInfo() {
        QString xmlPath(QString::fromStdString(xmlFile.expanded()));
        QFile file(xmlPath);

        if (!file.open(QFile::ReadOnly) ) {
          throw IException(IException::Io,
                           "Unable to open xml file, ["+xmlFile.expanded()+"],  with read access",
                           _FILEINFO_);
        }
        if (reader->readNextStartElement()) {
          if (reader->name() == "bundleSolutionInfo") {
            readBundleSolutionInfo(reader);
          }
          else {
            reader->raiseError(QObject::tr("Incorrect file"));
          }
        }
      }

      ~BundleSolutionInfoXmlHandlerTester() {
      }

  };
}

TEST_F(ThreeImageNetwork, BundleSolutionInfoConstructors) {
  BundleSettingsQsp settings(new BundleSettings());
  BundleResults results;
  results.setNumberObservations(1000);
  QList<ImageList *> imageList;
  BundleSolutionInfo solution(settings, networkFile.toStdString(), results, imageList);

  EXPECT_TRUE(solution.adjustedImages().empty());
  EXPECT_EQ(solution.bundleResults().numberObservations(), results.numberObservations());
  EXPECT_FALSE(solution.id().isEmpty());
  EXPECT_EQ(solution.inputControlNetFileName().toStdString(), networkFile.toStdString());
  EXPECT_EQ(solution.bundleSettings(), settings);
}


TEST_F(ThreeImageNetwork, BundleSolutionInfoSerialization) {
  BundleSettingsQsp settings(new BundleSettings());
  BundleResults results;
  QList<ImageList *> imageList;
  BundleSolutionInfo solution(settings, networkFile.toStdString(), results, imageList);

  QString saveFile = tempDir.path() + "/BundleSolutionInfoTestData.xml";
  QFile qXmlFile(saveFile);
  qXmlFile.open(QIODevice::ReadWrite);
  QXmlStreamWriter writer(&qXmlFile);
  writer.setAutoFormatting(true);
  writer.writeStartDocument();
  solution.save(writer, NULL, "");
  writer.writeEndDocument();
  qXmlFile.close();

  QFile xml(saveFile);
  if(!xml.open(QFile::ReadOnly | QFile::Text)){
    throw IException(IException::Unknown,
                      "Failed to parse xml file, ["+xml.fileName().toStdString()+"]",
                      _FILEINFO_);
  }

  QXmlStreamReader reader(&xml);
  BundleSolutionInfoXmlHandlerTester newSolution(&reader, saveFile.toStdString());

  EXPECT_EQ(solution.adjustedImages().size(), newSolution.adjustedImages().size());
  EXPECT_EQ(solution.bundleResults().numberObservations(), newSolution.bundleResults().numberObservations());
  EXPECT_EQ(solution.id().toStdString(), newSolution.id().toStdString());
  EXPECT_EQ(solution.runTime().toStdString(), newSolution.runTime().toStdString());
  EXPECT_EQ(solution.name().toStdString(), newSolution.name().toStdString());
}


TEST_F(ThreeImageNetwork, BundleSolutionInfoMutators) {
  BundleSettingsQsp settings(new BundleSettings());
  BundleResults results;
  QList<ImageList *> imageList;
  BundleSolutionInfo solution(settings, networkFile.toStdString(), results, imageList);

  solution.addAdjustedImages(NULL);

  BundleResults newResults;
  newResults.setNumberObservations(1000);
  solution.setOutputStatistics(newResults);

  Control *control = new Control(networkFile);
  solution.setOutputControl(control);

  QString outputControlName("test.net");
  solution.setOutputControlName(outputControlName);

  QString runTime("hh:mm:ss");
  solution.setRunTime(runTime);

  QString solutionName("Test Solution");
  solution.setName(solutionName);

  EXPECT_FALSE(solution.adjustedImages().empty());
  EXPECT_EQ(solution.bundleResults().numberObservations(), newResults.numberObservations());
  EXPECT_EQ(solution.control(), control);
  EXPECT_EQ(solution.outputControlName().toStdString(), outputControlName.toStdString());
  EXPECT_EQ(solution.runTime().toStdString(), runTime.toStdString());
  EXPECT_EQ(solution.name().toStdString(), solutionName.toStdString());
}


TEST_F(ThreeImageNetwork, BundleSolutionInfoOutputFiles) {
  BundleSettingsQsp settings(new BundleSettings());
  QString filePrefix = tempDir.path() + "/BundleSolutionInfo";
  settings->setOutputFilePrefix(filePrefix);
  BundleResults results;
  QList<ImageList *> imageList;

  Statistics rmsStats;
  rmsStats.SetValidRange(0, 100);
  rmsStats.AddData(0);
  rmsStats.AddData(1);
  rmsStats.AddData(2);
  rmsStats.AddData(3);
  rmsStats.AddData(Isis::Null);// 1 NULL
  rmsStats.AddData(Isis::Lrs); // 2 LRS
  rmsStats.AddData(Isis::Lrs);
  rmsStats.AddData(Isis::Lis); // 3 LIS
  rmsStats.AddData(Isis::Lis);
  rmsStats.AddData(Isis::Lis);
  rmsStats.AddData(Isis::Hrs); // 4 HRS
  rmsStats.AddData(Isis::Hrs);
  rmsStats.AddData(Isis::Hrs);
  rmsStats.AddData(Isis::Hrs);
  rmsStats.AddData(Isis::His); // 5 HIS
  rmsStats.AddData(Isis::His);
  rmsStats.AddData(Isis::His);
  rmsStats.AddData(Isis::His);
  rmsStats.AddData(Isis::His);
  rmsStats.AddData(-1);        // 1 below range
  rmsStats.AddData(1000);      // 2 above range
  rmsStats.AddData(1001);
  // 6, 14, 0, 3, 0, 100, 22, 4, 1, 2, 3, 4, 5, 1, 2, false

  QList<Statistics> rmsImageLineResiduals;
  rmsImageLineResiduals += rmsStats;
  rmsStats.AddData(4);
  // 10, 30, 0, 4, 0, 100, 23, 5, 1, 2, 3, 4, 5, 1, 2, false
  rmsImageLineResiduals += rmsStats;
  rmsStats.AddData(5);
  rmsStats.RemoveData(5);
  // 10, 30, 0, 5, 0, 100, 23, 5, 1, 2, 3, 4, 5, 1, 2, true
  rmsImageLineResiduals += rmsStats;

  QList<Statistics> rmsImageSampleResiduals = rmsImageLineResiduals;
  rmsImageSampleResiduals[0].RemoveData(0);
  rmsImageSampleResiduals[0].AddData(4);
  rmsImageSampleResiduals[2].RemoveData(2);
  // 10, 30, 0, 3, 0, 100, 22, 4, 1, 2, 3, 4, 5, 1, 2, true
  // 10, 30, 0, 4, 0, 100, 23, 5, 1, 2, 3, 4, 5, 1, 2, false
  // 8, 26, 0, 5, 0, 100, 22, 4, 1, 2, 3, 4, 5, 1, 2, true

  QList<Statistics> rmsImageResiduals = rmsImageSampleResiduals;
  rmsImageResiduals[0].AddData(0);
  rmsImageResiduals[0].AddData(1);
  rmsImageResiduals[0].AddData(2);
  rmsImageResiduals[0].AddData(3);
  rmsImageResiduals[1].AddData(0);
  rmsImageResiduals[1].AddData(1);
  rmsImageResiduals[1].AddData(2);
  rmsImageResiduals[1].AddData(3);
  rmsImageResiduals[2].AddData(0);
  rmsImageResiduals[2].AddData(1);
  rmsImageResiduals[2].AddData(2);
  rmsImageResiduals[2].AddData(3);
  // 16, 44, 0, 3, 0, 100, 26, 8, 1, 2, 3, 4, 5, 1, 2, true
  // 16, 44, 0, 4, 0, 100, 27, 9, 1, 2, 3, 4, 5, 1, 2, false
  // 14, 40, 0, 5, 0, 100, 26, 8, 1, 2, 3, 4, 5, 1, 2, true

  results.setRmsImageResidualLists(rmsImageLineResiduals,
                                   rmsImageSampleResiduals,
                                   rmsImageResiduals);

  ControlPoint *freePoint = new ControlPoint("FreePoint");
  ControlMeasure *freeMeasure1 = new ControlMeasure;
  freeMeasure1->SetCubeSerialNumber("Ignored");
  freeMeasure1->SetIgnored(true);
  freePoint->Add(freeMeasure1);
  ControlMeasure *freeMeasure2 = new ControlMeasure;
  freeMeasure2->SetCubeSerialNumber("NotIgnored");
  freeMeasure2->SetIgnored(false);
  freeMeasure2->SetCoordinate(1.0, 2.0);
  freeMeasure2->SetResidual(-3.0, 4.0);
  freePoint->Add(freeMeasure2);
  SurfacePoint freeSurfacePoint(Latitude(45.0, Angle::Degrees),
                                Longitude(120.0, Angle::Degrees),
                                Distance(6.0, Distance::Meters));
  freePoint->SetAdjustedSurfacePoint(freeSurfacePoint);
  ControlPoint *fixedPoint = new ControlPoint("FixedPoint");
  fixedPoint->SetType(ControlPoint::Fixed);
  SurfacePoint fixedSurfacePoint(Latitude(90.0, Angle::Degrees),
                                  Longitude(180.0, Angle::Degrees),
                                  Distance(10.0, Distance::Meters));
  fixedPoint->SetAdjustedSurfacePoint(fixedSurfacePoint);
  ControlNet outNet;
  outNet.AddPoint(freePoint);
  outNet.AddPoint(fixedPoint);
  BundleControlPointQsp freeBundleControlPoint(
                                                new BundleControlPoint(settings, freePoint));
  BundleControlPointQsp fixedBundleControlPoint(
                                    new BundleControlPoint(settings, fixedPoint));
  QVector<BundleControlPointQsp> bundleControlPointVector;
  bundleControlPointVector.append(freeBundleControlPoint);
  bundleControlPointVector.append(fixedBundleControlPoint);

  BundleImage bundleImage(cube1->camera(),
                          "Ignored",
                          "TestImageFileName");
  BundleObservationVector observationVector;
  observationVector.addNew(BundleImageQsp(new BundleImage(bundleImage)),
                           "ObservationNumber1",
                           "Instrument1",
                           settings);
  
  results.setBundleControlPoints(bundleControlPointVector);
  results.setOutputControlNet(ControlNetQsp(new ControlNet(outNet)));
  results.setObservations(observationVector);

  BundleSolutionInfo solution(settings, networkFile.toStdString(), results, imageList);

  solution.outputText();
  solution.outputImagesCSV();
  solution.outputPointsCSV();
  solution.outputResiduals();

  QFile textFile(filePrefix + "bundleout.txt");
  QFile imagesCSV(filePrefix + "bundleout_images.csv");
  QFile pointsCSV(filePrefix + "bundleout_points.csv");
  QFile residualsCSV(filePrefix + "residuals.csv");

  EXPECT_TRUE(textFile.exists());
  EXPECT_TRUE(imagesCSV.exists());
  EXPECT_TRUE(pointsCSV.exists());
  EXPECT_TRUE(residualsCSV.exists());
}