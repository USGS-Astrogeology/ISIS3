/** This is free and unencumbered software released into the public domain.

The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */

#include "cnetwinnow.h"

#include <geos_c.h>
#include <geos/algorithm/ConvexHull.h>
#include <geos/geom/CoordinateSequence.h>
#include <geos/geom/Envelope.h>
#include <geos/geom/Geometry.h>
#include <geos/geom/GeometryFactory.h>
#include <geos/geom/Polygon.h>

#include <QHash>
#include <QString>
#include <QList>
#include <QVector>

#include "ControlMeasure.h"
#include "ControlPoint.h"
#include "PvlGroup.h"
#include "ImageHistogram.h"
#include "IException.h"
#include "IString.h"
#include "Progress.h"
#include "SerialNumber.h"
#include "SerialNumberList.h"
#include "UserInterface.h"

#include "stdio.h"
#include "string.h"

using namespace std;
using namespace Isis;

namespace Isis {

  void cubeConvexHullAndMeasures(QString &serialNum,ControlNet &net, double &area, int &validMeasures,
                                 QList <ControlMeasure *> *measToIgnor=NULL);

  /**
    * Default method for CNET winnow that takes a UI object from the
    * application, parses the necessary UI elements and winnows points within
    * the network.
    *
    *
    * @param ui UserInterface object generated using the cnetwinnow.xml file
    *
    * @param progress A progress object for the UI to get progress reports
    *                 through
    */
  void cnetwinnow(UserInterface &ui, Progress *progress) {
    //read the ControlNet
    ControlNet net(ui.GetFileName("CNET"), progress);
    //read the file list
    SerialNumberList serialNumList(ui.GetFileName("FROMLIST"), true, progress);

    cnetwinnow(net, serialNumList, ui, progress);
  }

  /**
    * Given some control network and criteria passed in through the UI, remove
    * points within the network whose residuals (often generated through jigsaw)
    * do not match the given set of criteria
    *
    *
    * @param net A control network object containing points with residuals
    *
    * @param serialNumList A SerialNumberList object that holds the serial
    *        numbers for all cubes within the network often generated from
    *        a filelist
    *
    * @param progress A progress object for the UI to get progress reports
    *                 through
    *
    * @param ui UserInterface object generated using the cnetwinnow.xml file
    *
    */
  void cnetwinnow(ControlNet &net, SerialNumberList &serialNumList, UserInterface &ui, Progress *progress) {
    //check to make sure all the serial numbers in the net have an associated file name
    QList<QString> cubeSerials = net.GetCubeSerials();
    bool serFlag=true;
    for (int i=0;i<cubeSerials.size();i++) {
      std::string msg = "No file paths for the following serial numbers:\n";
      if (!serialNumList.hasSerialNumber(cubeSerials[i])) {
        msg += cubeSerials[i] + "\n";
        serFlag = false;
      }
      if (!serFlag) {
        throw IException(IException::User, msg, _FILEINFO_);
        return;
      }
    }
    //build a Histogram of the residual Magnitudes from the ControlNet
    Histogram hist(net, &ControlMeasure::GetResidualMagnitude, ui.GetDouble("BIN_WIDTH"));

      //make sure there was some residual data in the control network
    if (hist.ValidPixels() < 1) {
      std::string msg = "Error no valid residual data found in network [" + ui.GetFileName("CNET") + "]";
      throw IException(IException::User, msg, _FILEINFO_);
      return;
    }

    FILE *guiltyFile;
    char fileName[1028];
    strcpy(fileName,ui.GetString("FILE_PREFIX").toLatin1().data());
    strcat(fileName,"Guilty.csv");
    guiltyFile = fopen(fileName,"w");
    if (guiltyFile == NULL) {
      string msg = "Unable to open file [" + IString(fileName) + "]";
      throw IException(IException::User, msg, _FILEINFO_);
      return;
    }

    FILE *ignoredReport;
    strcpy(fileName,ui.GetString("FILE_PREFIX").toLatin1().data());
    strcat(fileName,"Ignored.csv");
    ignoredReport = fopen(fileName,"w");
    if (guiltyFile == NULL) {
      string msg = "Unable to open file [" + IString(fileName) + "]";
      throw IException(IException::User, msg, _FILEINFO_);
      return;
    }

    //analyze the histrogram to determine the boundaries between
      //inocent, suspect, and guilty measures
    double percentile80 = hist.Percent(80);
      //starting at the 80th percentile look
    int nBins,bin;
    nBins = hist.Bins(); //get the number of bins
    for ( bin = nBins-1;bin>=0;bin--) { //work backwards through the bins to find the starting bin
      double min,max;
      hist.BinRange(bin,min,max);
      if (min < percentile80) break;
    }

    if ( bin <0) {  //if Histogram is being inconsistent throw an error
      string msg = "Histogram resturns the 80th percentile of " + IString(percentile80) +
                   " but has no bin containing values that small";
      throw IException(IException::Programmer, msg, _FILEINFO_);
    }


      //Now work forward back through the bins looking for local minima and maxima
      //  The idea hear is that if the residuals really belong to a Gaussian distribution
      //  then the histogram of the distribution tail should be strickly decreasing
      //  if there are bumps in the histogram it is indicative of non-Gaussian noise, and
      //  hence shows where the distrubition breaks down, and noise dominates signal.
    QList<double> binCenter;
    QList<double> binHeight;

      //Get the user specified ratios for significant bump heights
    double suspectRatio = ui.GetDouble("SUSPECT_BUMP_PERCENT")/100.0;
    double  guiltyRatio = ui.GetDouble("GUILTY_BUMP_PERCENT" )/100.0;

      //load all the histogram data
    for (;bin<nBins;bin++) {
      binCenter.push_back(hist.BinMiddle(bin));
      binHeight.push_back(hist.BinCount (bin));
    }

      //initialize boundaries
    double inocentSuspectB =0.0;
    double suspectGuiltyB  = 0.0;

    nBins = binCenter.size();
      //look for significant bumps
    for (int i=1;i<nBins-2;i++) {
      //if i is a local minima
      if ( binHeight[i-1] > binHeight[i] && binHeight[i] < binHeight[i+1] ) {
        for (int j=i+1;j<nBins-1;j++) {  //find the next local maxima
          //if j is a local maxima
          if ( binHeight[j-1] < binHeight[j] && binHeight[j] > binHeight[j+1] ) {
             double ratio;
             //calculate the height of the bump as a ratio of the height of the local minima
             if (binHeight[i] >0) ratio = (binHeight[j]-binHeight[i])/binHeight[i];
             else ratio = DBL_MAX;
             //if the first significant bumps save them
             if (ratio > suspectRatio && inocentSuspectB ==0) inocentSuspectB = binCenter[i];
             if (ratio >  guiltyRatio &&  suspectGuiltyB ==0)  suspectGuiltyB = binCenter[i];
             i=j;  //advance to the j position
             break; //only search through the nearest local maxima
          }
        }
      }
      //if both of the first boundaries are found stop the search
      if (inocentSuspectB != 0.0 && suspectGuiltyB != 0.0) break;
    }



      //rembering that the inocentSuspectB can not be less than the user defined floor
    if ( inocentSuspectB < ui.GetDouble("SUSPECT_FLOOR") )
      inocentSuspectB = ui.GetDouble("SUSPECT_FLOOR");
      //add also remembering that suspectGuiltyB can not be greater than the user defined ceiling
    if ( suspectGuiltyB > ui.GetDouble("GUILTY_FLOOR") )
      suspectGuiltyB = ui.GetDouble("GUILTY_FLOOR");
      //rembering that the suspectGuiltyB can not be less than the user defined floor either
    if ( suspectGuiltyB < ui.GetDouble("SUSPECT_FLOOR") )
      suspectGuiltyB = ui.GetDouble("SUSPECT_FLOOR");
      //finally make sure that inocentSuspectB <= suspectGuiltyB
    if (inocentSuspectB > suspectGuiltyB) inocentSuspectB = suspectGuiltyB;
      //the boundaries between inocent, suspect, and guilty measures have been established

    //get an ordered list of all the suspect and guilty measures
    QList <ControlMeasure *> suspectMeasures
         = net.sortedMeasureList(&ControlMeasure::GetResidualMagnitude,inocentSuspectB,DBL_MAX);


    //print csv column headers
    fprintf(guiltyFile,/*"Guilty points (Residual Magnitudes > %lf) that could not be ignored.\n"
               "PtID:  Point ID Name\n"
               "ImageSerial: Image serial Number\n"
               "ResidualMagnitude: The two-deminsional image residual length\n"
               "HullAreaReduction: The total reduction in the area of the convex hull (as a "
                   "percent of the original area) from ignoring this point and all previously "
                   "ignored points.\n"
               "MeasureReduction: The total reduction in the number of active measures (as a "
                   "percent of the original number) from ignoring this point and all previously "
                   "ignored points.\n"
               "NetWouldSplit: The network would be split if this point was ignored, 'Yes' or 'No'\n"
               "Editable: This measure could have been ignored (it wasn't edit locked, etc.)"
                   ", 'Yes' or 'No'.\n"
               "PtGroupFailure: measures are ignored in groups of 1 or 2.  All measures in the "
                   "group must be cleared to be ignored or none of them are, 'Yes' or 'No'.\n"*/
               "PtID,ImageFile,ImageSerial,ResidualMagnitude,HullAreaReduction%%,"
               "MeasureReduction%%,OrignialNumMeasures,ResultantNumMeasures,NetWouldSplit,Editable,"
               "PtGroupFailure\n");

    fprintf(ignoredReport,/*"Points that were set to ignored\n"
               "PtID:  Point ID Name\n"
               "ImageSerial: Image serial Number\n"
               "ResidualMagnitude: The two-deminsional image residual length\n"
               "HullAreaReduction: The total reduction in the area of the convex hull (as a "
                   "percent of the original area) from ignoring this point and all previously "
                   "ignored points.\n"
               "MeasureReduction: The total reduction in the number of active measures (as a "
                   "percent of the original number) from ignoring this point and all previously "
                   "ignored points.\n"
               "NetWouldSplit: The network would be split if this point was ignored, 'Yes' or 'No'\n"
               "Editable: This measure could have been ignored (it wasn't edit locked, etc.)"
                   ", 'Yes' or 'No'.\n"
               "PtGroupFailure: measures are ignored in groups of 1 or 2.  All measures in the "
                   "group must be cleared to be ignored or none of them are, 'Yes' or 'No'.\n"*/
               "PtID,ImageFile,ImageSerial,ResidualMagnitude,HullAreaReduction%%,"
               "MeasureReduction%%,OrignialNumMeasures,ResultantNumMeasures,NetWouldSplit,Editable,"
               "PtGroupFailure\n");

    //we will need a hash of the image original convex hulls and
      //numbers of measures, it will be built on the fly
    QHash<QString,QList<double> > originalCubeStats;


    //we will also need to know how many islands we started with
    int numInitialIslands = net.GetSerialConnections().size();

    //user parameters for allowing measure rejection
    double hullReductionLimit = ui.GetDouble("HULL_REDUCTION_PERCENT")/100.0;
    double measureReductionLimit = ui.GetDouble("MEASURE_REDUCTION_PERCENT")/100.0;


    //now work through the list from the end to the begining setting measures to ignor if we are able
    if (progress) {
      progress->SetText("Winnowing points");
      progress->SetMaximumSteps((int)suspectMeasures.size() + 1);
      progress->CheckStatus();
    }
    for (int i = suspectMeasures.size() - 1; i > -1; i--) {
      //if the measure to be ignored is one of the last two active measures of a point then both of
       //the measures and the point must be ignored together so we need the flexibility to test
       //two points at a time
      QList<ControlMeasure*> measGroup;
      bool hullFlag[2]; //hull test pass flags
      double hullReduction[2]; //ration of hull redutions
      double hullArea[2]; //hull areas
      bool measFlag[2]; //measure reduction test flags
      double measReduction[2]; //ratio of measure reduction
      int measNum[2];  //num of valid measures remaining
      int measOriNum[2];
      bool islandFlag;  //flag to indicate if ignoring this group of measures will split the network
      bool ableToEditFlag[2]; //flag to indicate if the measure's ignor status could be changed

      //find out if this is a group of one or two
      if ( suspectMeasures[i]->Parent()->GetNumValidMeasures() <= 2) {
        QList<ControlMeasure *> tempList = suspectMeasures[i]->Parent()->getMeasures();
        //pull the valid measures out of the list
        for (int j=0;j<tempList.size();j++)
          if(!tempList[j]->IsIgnored())
            measGroup.push_back(tempList[j]);
      }
      else
        measGroup.push_back(suspectMeasures[i]);

      //make sure we have initial cube stats for the cubes in the group
      for (int j=0;j<measGroup.size();j++) {
        //short list to hold convexHull, and original number of measures, in that order
        QList<double> hullAndValidMeasures;
        double area;
        int validMeasures;
        QString serialNum = measGroup[j]->GetCubeSerialNumber();
        //check to see if the initial stats are already calculated for this image
        if (originalCubeStats.contains(serialNum)) continue;
        //otherwise do the calculations
        cubeConvexHullAndMeasures(serialNum,net,area,validMeasures);
        hullAndValidMeasures.push_back(area);
        hullAndValidMeasures.push_back(validMeasures);
        //and add it to the map
        originalCubeStats.insert(serialNum,hullAndValidMeasures);
      }

      //check each measure to see if it can be ignored
      for (int j=0;j<measGroup.size();j++) {
        //set the measure to ignored and then test it
        if (measGroup[j]->IsEditLocked() == false)
          ableToEditFlag[j] = true;  //test pass
        else
          ableToEditFlag[j] = false; //test fail

        QString serialNum = measGroup[j]->GetCubeSerialNumber();
        cubeConvexHullAndMeasures(serialNum,net,hullArea[j],measNum[j],&measGroup);

        //get the original values for this cube
        QList<double> origValues = originalCubeStats.value(serialNum);
        measOriNum[j] = origValues[1];

        //the first value is the area, check the reduced convex hull against the original
        if ( origValues[0] != 0) {
          hullReduction[j] = (origValues[0]-hullArea[j])/origValues[0];
          if ( hullReduction[j] > hullReductionLimit)
            hullFlag[j] = false; //test failed
          else
            hullFlag[j] = true;  //test passed
        }
        else {
          //if the hull started out at area = 0.0 then this test is irrelevant
          hullReduction[j] = 0.0;
          hullFlag[j] = true;  //test passed
        }

        //the second value is the number of measures, check the percentage of the total reduction
        measReduction[j] = (origValues[1]-double(measNum[j]))/origValues[1];
        if ( measReduction[j] > measureReductionLimit)
          measFlag[j] = false; //test failed
        else
          measFlag[j] = true;  //test passed
      }

      //for now it is necessary to set measures to ignor before testing to see if they would split the network
        //this will change when the expected ControlNet::isCriticalMeasure(...) method is written
      for (int j=0; j<measGroup.size(); j++) {
        measGroup[j]->SetIgnored(true);
      }

      //if the number of islands has increased the network has split
      if (net.GetSerialConnections().size() > numInitialIslands) {
        islandFlag = false; //test failed
      }
      else {
        islandFlag = true; //test passed
      }

      //again we will temporarily be setting the measure back to unignored
        //this will change when the expected ControlNet::isCriticalMeasure(...) method is written
      for (int j=0;j<measGroup.size();j++) {
        measGroup[j]->SetIgnored(false);
      }

          //group success flag
      bool groupFlag=true; //assuming success for the whole group
      for( int j=0;j<measGroup.size();j++) {
        if (!hullFlag[j] || !measFlag[j] || !islandFlag || !groupFlag)
          groupFlag = false;
      }

      //print a report for guilty points that could not be ignored, and points that were ignored
      char line[1024];

      for( int j=0;j<measGroup.size();j++) {
        snprintf(line, sizeof(line), "%s,%s,%s,%lf,%4.2lf,%4.2lf,%d,%d,%s,%s,%s\n",
                      measGroup[j]->Parent()->GetId().toStdString().data(),
                      serialNumList.fileName(measGroup[j]->GetCubeSerialNumber()
                        ).toLatin1().data(),
                      measGroup[j]->GetCubeSerialNumber().toLatin1().data(),
                      measGroup[j]->GetResidualMagnitude(),
                      hullReduction[j]*100.0,
                      measReduction[j]*100.0,
                      measOriNum[j],
                      measNum[j],
                      !islandFlag ? "Yes":"No",
                      ableToEditFlag[j] ? "Yes":"No",
                      !groupFlag ? "Yes":"No");

        if (!groupFlag && measGroup[j]->GetResidualMagnitude() >= suspectGuiltyB)
          fputs(line, guiltyFile);
        else  //print the stats of ingored points to the other report
          fputs(line,ignoredReport);
      }


      //if all within this group of measures can be ignored ignor them
      if (groupFlag) {
        for (int j=0;j<measGroup.size();j++)
          measGroup[j]->SetIgnored(true);
        //if the group can be successfully ignored and the group was the last two measures
          //of an object point then ignor the point too
        if (measGroup.size() >1)
          measGroup[0]->Parent()->SetIgnored(true);
      }
      if (progress) progress->CheckStatus();
    }

    //close the report file
    fclose(guiltyFile);
    fclose(ignoredReport);

    //save out the winnowed ControlNet
    net.Write(ui.GetFileName("ONET"));
  }

  void cubeConvexHullAndMeasures(QString &serialNum,ControlNet &net, double &area, int &validMeasures,
                            QList <ControlMeasure *> *measToIgnor) {

    int i,j,firstIndex=0;
    bool firstFlag=false;
    bool ignorMeas;
    QList<ControlMeasure *> cubeMeasures = net.GetMeasuresInCube(serialNum);
    static  geos::geom::GeometryFactory::Ptr geosFactory = geos::geom::GeometryFactory::create();
    geos::geom::CoordinateSequence pts;
    for (i=0;i<cubeMeasures.size();i++) {
      if (cubeMeasures[i]->IsIgnored()) continue;  //skip ignored measures
      if (cubeMeasures[i]->Parent()->IsIgnored()) continue; //skip measures of ignored points
      //skip measure in the ToIgnor list
      ignorMeas=false;
      if (measToIgnor != NULL) {
        for (j=0; j<measToIgnor->size() && !ignorMeas; j++) {
          if (cubeMeasures[i] == (*measToIgnor)[j]) {
            ignorMeas = true;
            break;
          }
        }
      }
      if (ignorMeas) continue;
      if (!firstFlag) {
        firstFlag =true;
        firstIndex =i;
      }
      //build point sequence
      pts.add(geos::geom::Coordinate(cubeMeasures[i]->GetSample(), cubeMeasures[i]->GetLine()));
    }
    //Adding the first active point again closes the "linestring"
    pts.add(geos::geom::Coordinate(cubeMeasures[firstIndex]->GetSample(),
                                    cubeMeasures[firstIndex]->GetLine()));
    if (pts.size() >= 4) {
      // Calculate the convex hull
      geos::geom::Geometry * convexHull = geosFactory->createPolygon(
          geosFactory->createLinearRing(pts))->convexHull().release();
      // Calculate the area of the convex hull
      area = convexHull->getArea();
    }
    else {
      area = 0.0;
    }
    validMeasures = pts.size()-1; //subtract one because one point is in there twice
    return;
  }
}
