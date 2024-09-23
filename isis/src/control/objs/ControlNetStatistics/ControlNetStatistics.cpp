/** This is free and unencumbered software released into the public domain.

The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */

#include "ControlNetStatistics.h"

#include <QDebug>

#include <geos_c.h>
#include <geos/algorithm/ConvexHull.h>
#include <geos/geom/CoordinateSequence.h>
#include <geos/geom/Envelope.h>
#include <geos/geom/Geometry.h>
#include <geos/geom/GeometryFactory.h>
#include <geos/geom/Polygon.h>

#include "ControlNet.h"
#include "ControlPoint.h"
#include "ControlMeasure.h"
#include "ControlMeasureLogData.h"
#include "Cube.h"
#include "CubeManager.h"
#include "FileName.h"
#include "IString.h"
#include "Progress.h"
#include "Pvl.h"
#include "SpecialPixel.h"
#include "Statistics.h"

namespace Isis {

  //! String names for Point Type
  QString sPointType [] = { "Fixed", "Constrained", "Free" };

  //! String values for Boolean
  QString sBoolean[]    = { "False", "True" };

  /**
   * ControlNetStatistics Constructor has ctor to it's base Control Network
   *
   * @author Sharmila Prasad (8/24/2010)
   *
   * @param pCNet - Input Control network
   * @param psSerialNumFile - Serial Number List file
   * @param pProgress - Check Progress if not Null
   */
  ControlNetStatistics::ControlNetStatistics(ControlNet *pCNet, const QString &psSerialNumFile,
                                             Progress *pProgress) {
    numCNetImages = 0;
    mCNet = pCNet;

    mSerialNumList = SerialNumberList(psSerialNumFile);
    InitSerialNumMap();

    mProgress = pProgress;

    GetPointIntStats();
    GetPointDoubleStats();
    GenerateImageStats();
  }

  /**
   * Constructor with  ControlNet
   *
   * @author Sharmila Prasad (11/5/2010)
   *
   * @param pCNet
   * @param pProgress
   */
  ControlNetStatistics::ControlNetStatistics(ControlNet *pCNet, Progress *pProgress) {
    mCNet = pCNet;
    mProgress = pProgress;

    GetPointIntStats();
    GetPointDoubleStats();
  }

  /**
   * Destructor
   *
   * @author Sharmila Prasad (9/17/2010)
   */
  ControlNetStatistics::~ControlNetStatistics() {
    mCNet = NULL;
  }

  /**
   * Init SerialNum map
   *
   * @author Sharmila Prasad (12/20/2011)
   */
  void ControlNetStatistics::InitSerialNumMap() {
    int numSn = mSerialNumList.size();
    numCNetImages = 0;
    for (int i=0; i<numSn; i++) {
      QString sn = mSerialNumList.serialNumber(i);
      mSerialNumMap[sn] = false;
    }
  }

  /**
   * Generates the summary stats for the entire control network.
   * Stats include Total images, Total, Valid, Ignored, Fixed Points,
   * Total, Valid, Ignored Measures and also Average, Min, Max Error,
   * Min, Max Line and Sample Errors
   *
   * @author Sharmila Prasad (8/25/2010)
   *
   * @param pStatsGrp - Output Control Net Stats in Pvl format
   */
  void ControlNetStatistics::GenerateControlNetStats(PvlGroup &pStatsGrp) {
    pStatsGrp = PvlGroup("ControlNetSummary");
    int numSN = mSerialNumList.size();

    if (numSN) {
      pStatsGrp += PvlKeyword("TotalImages",             Isis::toString(numSN));
      pStatsGrp += PvlKeyword("ImagesInControlNet", Isis::toString(numCNetImages));
    }

    pStatsGrp += PvlKeyword("TotalPoints",       Isis::toString(mCNet->GetNumPoints()));
    pStatsGrp += PvlKeyword("ValidPoints",       Isis::toString(NumValidPoints()));
    pStatsGrp += PvlKeyword("IgnoredPoints",     Isis::toString(mCNet->GetNumPoints() - NumValidPoints()));
    pStatsGrp += PvlKeyword("FixedPoints",       Isis::toString(NumFixedPoints()));
    pStatsGrp += PvlKeyword("ConstrainedPoints", Isis::toString(NumConstrainedPoints()));
    pStatsGrp += PvlKeyword("FreePoints",        Isis::toString(NumFreePoints()));
    pStatsGrp += PvlKeyword("EditLockPoints",    Isis::toString(mCNet->GetNumEditLockPoints()));

    pStatsGrp += PvlKeyword("TotalMeasures",     Isis::toString(NumMeasures()));
    pStatsGrp += PvlKeyword("ValidMeasures",     Isis::toString(NumValidMeasures()));
    pStatsGrp += PvlKeyword("IgnoredMeasures",   Isis::toString(NumIgnoredMeasures()));
    pStatsGrp += PvlKeyword("EditLockMeasures",  Isis::toString(mCNet->GetNumEditLockMeasures()));

    double dValue = GetAverageResidual();
    pStatsGrp += PvlKeyword("AvgResidual",       (dValue == Null ? "Null" : Isis::toString(dValue)));

    dValue = GetMinimumResidual();
    pStatsGrp += PvlKeyword("MinResidual",       (dValue == Null ? "Null" : Isis::toString(dValue)));

    dValue = GetMaximumResidual();
    pStatsGrp += PvlKeyword("MaxResidual",       (dValue == Null ? "Null" : Isis::toString(dValue)));

    dValue = GetMinLineResidual();
    pStatsGrp += PvlKeyword("MinLineResidual",   (dValue == Null ? "Null" : Isis::toString(dValue)));

    dValue = GetMaxLineResidual();
    pStatsGrp += PvlKeyword("MaxLineResidual",   (dValue == Null ? "Null" : Isis::toString(dValue)));

    dValue = GetMinSampleResidual();
    pStatsGrp += PvlKeyword("MinSampleResidual", (dValue == Null ? "Null" : Isis::toString(dValue)));

    dValue = GetMaxSampleResidual();
    pStatsGrp += PvlKeyword("MaxSampleResidual", (dValue == Null ? "Null" : Isis::toString(dValue)));

    // Shifts - Line, Sample, Pixel
    dValue = GetMinLineShift();
    pStatsGrp += PvlKeyword("MinLineShift",      (dValue == Null ? "Null" : Isis::toString(dValue)));

    dValue = GetMaxLineShift();
    pStatsGrp += PvlKeyword("MaxLineShift",      (dValue == Null ? "Null" : Isis::toString(dValue)));

    dValue = GetMinSampleShift();
    pStatsGrp += PvlKeyword("MinSampleShift",    (dValue == Null ? "Null" : Isis::toString(dValue)));

    dValue = GetMaxSampleShift();
    pStatsGrp += PvlKeyword("MaxSampleShift",    (dValue == Null ? "Null" : Isis::toString(dValue)));

    dValue = GetAvgPixelShift();
    pStatsGrp += PvlKeyword("AvgPixelShift",     (dValue == Null ? "NA" : Isis::toString(dValue)));

    dValue = GetMinPixelShift();
    pStatsGrp += PvlKeyword("MinPixelShift",     (dValue == Null ? "NA" : Isis::toString(dValue)));

    dValue = GetMaxPixelShift();
    pStatsGrp += PvlKeyword("MaxPixelShift",     (dValue == Null ? "NA" : Isis::toString(dValue)));

    dValue = mPointDoubleStats[minGFit];
    pStatsGrp += PvlKeyword("MinGoodnessOfFit",  (dValue == Null ? "NA" : Isis::toString(dValue)));

    dValue = mPointDoubleStats[maxGFit];
    pStatsGrp += PvlKeyword("MaxGoodnessOfFit",  (dValue == Null ? "NA" : Isis::toString(dValue)));

    dValue = mPointDoubleStats[minEccentricity];
    pStatsGrp += PvlKeyword("MinEccentricity",   (dValue == Null ? "NA" : Isis::toString(dValue)));

    dValue = mPointDoubleStats[maxEccentricity];
    pStatsGrp += PvlKeyword("MaxEccentricity",   (dValue == Null ? "NA" : Isis::toString(dValue)));

    dValue = mPointDoubleStats[minPixelZScore];
    pStatsGrp += PvlKeyword("MinPixelZScore",    (dValue == Null ? "NA" : Isis::toString(dValue)));

    dValue = mPointDoubleStats[maxPixelZScore];
    pStatsGrp += PvlKeyword("MaxPixelZScore",    (dValue == Null ? "NA" : Isis::toString(dValue)));

    // Convex Hull
    if (mSerialNumList.size()) {
      dValue = mConvexHullRatioStats.Minimum();
      pStatsGrp += PvlKeyword("MinConvexHullRatio", (dValue == Null ? "Null" : Isis::toString(dValue)));

      dValue = mConvexHullRatioStats.Maximum();
      pStatsGrp += PvlKeyword("MaxConvexHullRatio", (dValue == Null ? "Null" : Isis::toString(dValue)));

      dValue = mConvexHullRatioStats.Average();
      pStatsGrp += PvlKeyword("AvgConvexHullRatio", (dValue == Null ? "Null" : Isis::toString(dValue)));
    }
  }


  /**
   * Generate the Image stats -
   *  imgSamples, imgLines, imgTotalPoints, imgIgnoredPoints, imgFixedPoints, imgLockedPoints,
   *  imgLocked, imgConstrainedPoints, imgFreePoints, imgConvexHullArea, imgConvexHullRatio
   *
   * @author Sharmila Prasad (11/1/2011)
   */
  void ControlNetStatistics::GenerateImageStats() {
    geos::geom::GeometryFactory::Ptr geosFactory = geos::geom::GeometryFactory::create();

    CubeManager cubeMgr;
    cubeMgr.SetNumOpenCubes(50);

    QList<QString> cnetSerials = mCNet->GetCubeSerials();

    if (mProgress != NULL) {
      mProgress->SetText("Generating Image Stats.....");
      mProgress->SetMaximumSteps(cnetSerials.size());
      mProgress->CheckStatus();
    }

    foreach (QString sn, cnetSerials) {
      geos::geom::CoordinateSequence ptCoordinates;

      // setup vector for number of image properties and init to 0
      QVector<double> imgStats(numImageStats, 0);

      // Open the cube to get the dimensions
      Cube *cube = cubeMgr.OpenCube(mSerialNumList.fileName(sn));

      mSerialNumMap[sn] = true;
      numCNetImages++;

      imgStats[imgSamples] = cube->sampleCount();
      imgStats[imgLines]   = cube->lineCount();
      double cubeArea      = imgStats[imgSamples] * imgStats[imgLines];

      QList< ControlMeasure * > measures = mCNet->GetMeasuresInCube(sn);

      // Populate pts with a list of control points
      if (!measures.isEmpty()) {
        foreach (ControlMeasure * measure, measures) {
          ControlPoint *parentPoint = measure->Parent();
          imgStats[imgTotalPoints]++;
          if (parentPoint->IsIgnored()) {
            imgStats[imgIgnoredPoints]++;
          }
          if (parentPoint->GetType() == ControlPoint::Fixed) {
            imgStats[imgFixedPoints]++;
          }
          if (parentPoint->GetType() == ControlPoint::Constrained) {
            imgStats[imgConstrainedPoints]++;
          }
          if (parentPoint->GetType() == ControlPoint::Free) {
            imgStats[imgFreePoints]++;
          }
          if (parentPoint->IsEditLocked()) {
            imgStats[imgLockedPoints]++;
          }
          if (measure->IsEditLocked()) {
            imgStats[imgLocked]++;
          }
          ptCoordinates.add(geos::geom::Coordinate(measure->GetSample(),
                                                    measure->GetLine()));
        }

        ptCoordinates.add(geos::geom::Coordinate(measures[0]->GetSample(),
                                                  measures[0]->GetLine()));
      }

      if (ptCoordinates.size() >= 4) {
        // Calculate the convex hull

        // Even though geos doesn't create valid linear rings/polygons from this set of coordinates,
        //   because it self-intersects many many times, it still correctly does a convex hull
        //   calculation on the points in the polygon.
        geos::geom::Geometry * convexHull = geosFactory->createPolygon(
          geosFactory->createLinearRing(ptCoordinates))->convexHull().release();

        // Calculate the area of the convex hull
        imgStats[imgConvexHullArea] = convexHull->getArea();
        imgStats[imgConvexHullRatio] = imgStats[imgConvexHullArea] / cubeArea;
      }

      // Add info to statistics to get min, max and avg convex hull
      mConvexHullStats.AddData(imgStats[imgConvexHullArea]);
      mConvexHullRatioStats.AddData(imgStats[imgConvexHullRatio]);

      mImageMap[sn] = imgStats;

      // Update Progress
      if (mProgress != NULL)
        mProgress->CheckStatus();
    }
  }


  /**
   * Print the Image Stats into specified output file
   *
   * @author Sharmila Prasad (9/1/2010)
   *
   * Header: FileName, SerialNumber, TotalPoints, PointsIgnored, PointsLocked, Fixed, Constrained, Free
   *
   * @param psImageFile - Output Image Stats File
   */
  void ControlNetStatistics::PrintImageStats(const QString &psImageFile) {
    // Check if the image list has been provided
    if (!mSerialNumList.size()) {
      std::string msg = "Serial Number of Images has not been provided to get Image Stats";
      throw IException(IException::User, msg, _FILEINFO_);
    }

    FileName outFile(psImageFile.toStdString());
    ofstream ostm;
    QString outName(QString::fromStdString(outFile.expanded()));
    ostm.open(outName.toLatin1().data(), std::ios::out);

    if ( ostm.fail() ) {
      std::string msg = "Cannot open file [" + psImageFile.toStdString() + "]";
      throw IException(IException::Io, msg, _FILEINFO_);
     }

    //map< QString, vector<double> >::iterator it;
    // imgSamples, imgLines, imgTotalPoints, imgIgnoredPoints, imgFixedPoints, imgLockedPoints, imgLocked, imgConstrainedPoints, imgFreePoints, imgConvexHullArea, imgConvexHullRatio
    QMap<QString, bool>::iterator it;
    // Log into the output file
    ostm << "Filename, SerialNumber, TotalPoints, PointsIgnored, PointsEditLocked, Fixed, Constrained, Free, ConvexHullRatio" <<  endl;
    //for (it = mImageMap.begin(); it != mImageMap.end(); it++) {

    for (it = mSerialNumMap.begin(); it != mSerialNumMap.end(); it++) {
      ostm << mSerialNumList.fileName(it.key()).toStdString() << ", " << it.key().toStdString() << ", ";
      bool serialNumExists = it.value();
      if (serialNumExists) {
        QVector<double> imgStats = mImageMap[(it).key()] ;
        ostm << imgStats[imgTotalPoints]<< ", " << imgStats[imgIgnoredPoints] << ", " ;
        ostm << imgStats[imgLockedPoints] << ", " << imgStats[imgFixedPoints] << ", " ;
        ostm << imgStats[imgConstrainedPoints] << ", " << imgStats[imgFreePoints] << ", ";
        ostm << imgStats[imgConvexHullRatio] << endl;
      }
      else {
        ostm << "0, 0, 0, 0, 0, 0, 0" << endl;
      }
    }

    if (!ostm) {
      std::string msg = "Error writing to file: [" + psImageFile.toStdString() + "]";
      throw IException(IException::Io, msg, _FILEINFO_);
    }
    ostm.close();
  }


  /**
   * Returns the Image Stats by Serial Number
   *
   * @author Sharmila Prasad (11/3/2011)
   *
   * @param psSerialNum - Image serialNum
   *
   * @return const QVector<double>
   */
  QVector<double> ControlNetStatistics::GetImageStatsBySerialNum(QString psSerialNum) const {
    return (mImageMap.find(psSerialNum)).value();
  }


  /**
   * Generate the statistics of a Control Network by Point
   * Stats include ID, Type of each Control Point and
   * Total, Ignored measures in each Control Point
   *
   * @author Sharmila Prasad (8/24/2010)
   *
   * @param psPointFile - Output Point Statisitics File
   */
  void ControlNetStatistics::GeneratePointStats(const QString &psPointFile) {
    Isis::FileName outFile(psPointFile.toStdString());

    ofstream ostm;
    QString outName(QString::fromStdString(outFile.expanded()));
    ostm.open(outName.toLatin1().data(), std::ios::out);

    if ( ostm.fail() ) {
      std::string msg = "Cannot open file [" + psPointFile.toStdString() + "]";
      throw IException(IException::Io, msg, _FILEINFO_);
    }

    ostm << " PointId, PointType, PointIgnore, PointEditLock, TotalMeasures, MeasuresValid, MeasuresIgnore, MeasuresEditLock" << endl;

    int iNumPoints = mCNet->GetNumPoints();

    // Initialise the Progress object
    if (mProgress != NULL && iNumPoints > 0) {
      mProgress->SetText("Point Stats: Loading Control Points...");
      mProgress->SetMaximumSteps(iNumPoints);
      mProgress->CheckStatus();
    }

    for (int i = 0; i < iNumPoints; i++) {
      const ControlPoint *cPoint = mCNet->GetPoint(i);
      int iNumMeasures     = cPoint->GetNumMeasures();
      int iValidMeasures   = cPoint->GetNumValidMeasures();
      int iIgnoredMeasures = iNumMeasures - iValidMeasures;

      // Log into the output file
      ostm << cPoint->GetId().toStdString()   << ", " << sPointType[(int)cPoint->GetType()].toStdString() << ", " << sBoolean[(int)cPoint->IsIgnored()].toStdString() << ", " ;
      ostm << sBoolean[(int)cPoint->IsEditLocked()].toStdString() << ", " << iNumMeasures << ", " << iValidMeasures << ", ";
      ostm << iIgnoredMeasures << ", " << cPoint->GetNumLockedMeasures() << endl;

      // Update Progress
      if (mProgress != NULL)
        mProgress->CheckStatus();
    }

    if (!ostm) {
      std::string msg = "Error writing to file: [" + psPointFile.toStdString() + "]";
      throw IException(IException::Io, msg, _FILEINFO_);
    }

    ostm.close();
  }


  /**
   * Get network statistics for total, valid, ignored, locked points and measures
   *
   * @author sprasad (7/19/2011)
   */
  void ControlNetStatistics::GetPointIntStats() {
    // Init all the entries
    // totalPoints, validPoints, ignoredPoints, fixedPoints, constrainedPoints, editLockedPoints,
    // totalMeasures, validMeasures, ignoredMeasures, editLockedMeasures
    for (int i=0; i<numPointIntStats; i++) {
      mPointIntStats[i] = 0;
    }

    int iNumPoints = mCNet->GetNumPoints();

    // totalPoints
    mPointIntStats[totalPoints] = iNumPoints;

    for (int i = 0; i < iNumPoints; i++) {
      if (!mCNet->GetPoint(i)->IsIgnored()) {
        // validPoints
        mPointIntStats[validPoints]++;
      }
      else {
        // ignoredPoints
        mPointIntStats[ignoredPoints]++;
      }

      // fixedPoints
      if (mCNet->GetPoint(i)->GetType() == ControlPoint::Fixed)
        mPointIntStats[fixedPoints]++;

      // constrainedPoints
      if (mCNet->GetPoint(i)->GetType() == ControlPoint::Constrained)
        mPointIntStats[constrainedPoints]++;

      // free points
      if (mCNet->GetPoint(i)->GetType() == ControlPoint::Free)
        mPointIntStats[freePoints]++;

      // editLockedPoints
      if (mCNet->GetPoint(i)->IsEditLocked()) {
        mPointIntStats[editLockedPoints]++;
      }

      // totalMeasures
      mPointIntStats[totalMeasures] += mCNet->GetPoint(i)->GetNumMeasures();

      // validMeasures
      mPointIntStats[validMeasures] += mCNet->GetPoint(i)->GetNumValidMeasures();

      // editLockedMeasures
      mPointIntStats[editLockedMeasures] +=  mCNet->GetPoint(i)->GetNumLockedMeasures();
    }
    // ignoredMeasures
    mPointIntStats[ignoredMeasures] = mPointIntStats[totalMeasures] -  mPointIntStats[validMeasures];
  }


  /**
   * Initialize Point double stats vector
   *
   * @author Sharmila Prasad (1/3/2012)
   */
  void ControlNetStatistics::InitPointDoubleStats() {
    for (int i = 0; i < numPointDblStats; i++) {
      mPointDoubleStats[i] = Null;
    }
  }


  /**
   * Get the Network Statistics for Residuals (line, sample, magnitude) and
   * Shifts (line, sample, pixel)
   *
   * @author Sharmila Prasad (7/19/2011)
   */
  void ControlNetStatistics::GetPointDoubleStats() {
    InitPointDoubleStats();

    int iNumPoints = mCNet->GetNumPoints();
    double dValue = 0;

    Statistics residualMagStats;
    Statistics pixelShiftStats;

    for (int i = 0; i < iNumPoints; i++) {
      ControlPoint * cp = mCNet->GetPoint(i);

      if (!cp->IsIgnored()) {
        for (int cmIndex = 0; cmIndex < cp->GetNumMeasures(); cmIndex++) {
          ControlMeasure *cm = cp->GetMeasure(cmIndex);

          if (!cm->IsIgnored()) {
            residualMagStats.AddData(cm->GetResidualMagnitude());

            if (!IsSpecial(cm->GetPixelShift()))
              pixelShiftStats.AddData(fabs(cm->GetPixelShift()));
          }
        }
      }

      Statistics resMagStats = cp->GetStatistic(
          &ControlMeasure::GetResidualMagnitude);
      UpdateMinMaxStats(resMagStats, minResidual, maxResidual);

      Statistics resLineStats = cp->GetStatistic(
          &ControlMeasure::GetLineResidual);
      UpdateMinMaxStats(resLineStats, minLineResidual, maxLineResidual);

      Statistics resSampStats = cp->GetStatistic(
          &ControlMeasure::GetSampleResidual);
      UpdateMinMaxStats(resSampStats, minSampleResidual, maxSampleResidual);

      Statistics pixShiftStats = cp->GetStatistic(
          &ControlMeasure::GetPixelShift);
      UpdateMinMaxStats(pixShiftStats, minPixelShift, maxPixelShift);

      Statistics lineShiftStats = cp->GetStatistic(
          &ControlMeasure::GetLineShift);
      UpdateMinMaxStats(lineShiftStats, minLineShift, maxLineShift);

      Statistics sampShiftStats = cp->GetStatistic(
          &ControlMeasure::GetSampleShift);
      UpdateMinMaxStats(sampShiftStats, minSampleShift, maxSampleShift);

      Statistics gFitStats = cp->GetStatistic(
          ControlMeasureLogData::GoodnessOfFit);
      UpdateMinMaxStats(gFitStats, minGFit, maxGFit);

      Statistics minPixelZScoreStats = cp->GetStatistic(
          ControlMeasureLogData::MinimumPixelZScore);

      if (minPixelZScoreStats.ValidPixels()) {
        dValue = fabs(minPixelZScoreStats.Minimum());
        if (mPointDoubleStats[minPixelZScore] > dValue)
          mPointDoubleStats[minPixelZScore] = dValue;
      }

      Statistics maxPixelZScoreStats = cp->GetStatistic(
          ControlMeasureLogData::MaximumPixelZScore);

      if (maxPixelZScoreStats.ValidPixels()) {
        dValue = fabs(maxPixelZScoreStats.Maximum());
        if (mPointDoubleStats[maxPixelZScore] > dValue)
          mPointDoubleStats[maxPixelZScore] = dValue;
      }
    }

    // Average Residuals
    mPointDoubleStats[avgResidual] = residualMagStats.Average();

    // Average Shift
    mPointDoubleStats[avgPixelShift] = pixelShiftStats.Average();
  }


  void ControlNetStatistics::UpdateMinMaxStats(const Statistics & stats,
      ePointDoubleStats min, ePointDoubleStats max) {
    if (stats.ValidPixels()) {
      if (mPointDoubleStats[min] != Null) {
        mPointDoubleStats[min] = qMin(
            mPointDoubleStats[min], fabs(stats.Minimum()));
      }
      else {
        mPointDoubleStats[min] = fabs(stats.Minimum());
      }

      if (mPointDoubleStats[max] != Null) {
        mPointDoubleStats[max] = qMax(
            mPointDoubleStats[max], fabs(stats.Maximum()));
      }
      else {
        mPointDoubleStats[max] = fabs(stats.Maximum());
      }
    }
  }
}
