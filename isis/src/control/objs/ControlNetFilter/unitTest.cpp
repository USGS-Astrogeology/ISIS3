/** This is free and unencumbered software released into the public domain.

The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */

#include "Isis.h"
#include "Application.h"
#include "ControlNet.h"
#include "ControlPoint.h"
#include "ControlMeasure.h"
#include "ControlNetFilter.h"
#include "IException.h"
#include "Preference.h"
#include "PvlGroup.h"
#include "PvlKeyword.h"
#include "SpecialPixel.h"
#include "TextFile.h"

#include <string>
#include <iostream>
#include <stdio.h>
#include <stdlib.h>

using namespace Isis;
using namespace std;

void PrintControlNetInfo(ControlNet &pCNet);

void IsisMain() {
  Preference::Preferences(true);
  cout << "UnitTest for ControlNetFilter ...." << endl << endl;

  UserInterface &ui = Application::GetUserInterface();

  ControlNet cnetOrig(ui.GetFileName("CNET"));
  ControlNet *cnet = new ControlNet(ui.GetFileName("CNET"));

  //test filters
  QString sSerialFile = ui.GetFileName("FROMLIST");
  ControlNetFilter *cnetFilter = new ControlNetFilter(cnet, sSerialFile);

  PvlGroup filterGrp;
  PvlKeyword keyword;

  // Point ResidualMagnitude Filter
  cout << "****************** Point_ResidualMagnitude Filter ******************" << endl;
  filterGrp = PvlGroup("Point_ResidualMagnitude");
  keyword = PvlKeyword("LessThan", toString(1));
  filterGrp += keyword;
  cnetFilter->PointResMagnitudeFilter(filterGrp, false);
  cout << filterGrp << endl;
  filterGrp.clear();
  PrintControlNetInfo(*cnet);
  delete (cnet);
  delete (cnetFilter);
  cnetFilter = NULL;
  cnet = NULL;
  cout << "************************************************************************\n\n";

  // PointEditLock Filter
  cout << "****************** Point_EditLock Filter ******************" << endl;
  cnet = new ControlNet(ui.GetFileName("CNET"));
  cnetFilter = new ControlNetFilter(cnet, sSerialFile);
  filterGrp = PvlGroup("Point_EditLock");
  keyword= PvlKeyword("EditLock", "1");
  filterGrp += keyword;
  cnetFilter->PointEditLockFilter(filterGrp, false);
  cout << filterGrp << endl;
  filterGrp.clear();
  PrintControlNetInfo(*cnet);
  delete (cnet);
  delete (cnetFilter);
  cnetFilter = NULL;
  cnet = NULL;
  cout << "************************************************************************\n\n";

  // PointMeasureEditLock Filter
  cout << "****************** Point_NumMeasuresEditLock Filter ******************" << endl;
  cnet = new ControlNet(ui.GetFileName("CNET"));
  cnetFilter = new ControlNetFilter(cnet, sSerialFile);
  filterGrp = PvlGroup("Point_NumMeasuresEditLock");
  keyword = PvlKeyword("LessThan", toString(1));
  filterGrp += keyword;
  cnetFilter->PointNumMeasuresEditLockFilter(filterGrp, false);
  cout << filterGrp << endl;
  filterGrp.clear();
  PrintControlNetInfo(*cnet);
  delete (cnet);
  delete (cnetFilter);
  cnetFilter = NULL;
  cnet = NULL;
  cout << "************************************************************************\n\n";

  // Pixel Shift
  cout << "****************** Point_PixelShift Filter ******************" << endl;
  cnet = new ControlNet(ui.GetFileName("CNET"));
  cnetFilter = new ControlNetFilter(cnet, sSerialFile);
  filterGrp = PvlGroup("Point_PixelShift");
  keyword = PvlKeyword("LessThan", toString(10));
  filterGrp += keyword;
  keyword = PvlKeyword("GreaterThan", toString(1));
  filterGrp += keyword;
  cnetFilter->PointPixelShiftFilter(filterGrp, false);
  cout << filterGrp << endl;
  filterGrp.clear();
  PrintControlNetInfo(*cnet);
  delete (cnet);
  delete (cnetFilter);
  cnetFilter = NULL;
  cnet = NULL;
  cout << "************************************************************************\n\n";

  // PointID Filter
  cout << "******************  Point_ID Filter ******************" << endl;
  cnet = new ControlNet(ui.GetFileName("CNET"));
  cnetFilter = new ControlNetFilter(cnet, sSerialFile);
  filterGrp = PvlGroup("Point_IdExpression");
  keyword = PvlKeyword("Expression", "P01*");
  filterGrp += keyword;
  cnetFilter->PointIDFilter(filterGrp, false);
  cout << filterGrp << endl;
  filterGrp.clear();
  PrintControlNetInfo(*cnet);
  delete (cnet);
  delete (cnetFilter);
  cnetFilter = NULL;
  cnet = NULL;
  cout << "************************************************************************\n\n";

  // PointNumMeasures Filter
  cout << "****************** Point_NumMeasures Filter ******************" << endl;
  cnet = new ControlNet(ui.GetFileName("CNET"));
  cnetFilter = new ControlNetFilter(cnet, sSerialFile);
  filterGrp = PvlGroup("Point_NumMeasures");
  keyword = PvlKeyword("GreaterThan", toString(2));
  filterGrp += keyword;
  keyword = PvlKeyword("LessThan", toString(2));
  filterGrp += keyword;
  cnetFilter->PointMeasuresFilter(filterGrp,  false);
  cout << filterGrp << endl;
  filterGrp.clear();
  PrintControlNetInfo(*cnet);
  delete (cnet);
  delete (cnetFilter);
  cnetFilter = NULL;
  cnet = NULL;
  cout << "************************************************************************\n\n";

  // PointsProperties Filter
  cout << "****************** Points_Properties Filter ******************" << endl;
  cnet = new ControlNet(ui.GetFileName("CNET"));
  cnetFilter = new ControlNetFilter(cnet, sSerialFile);
  filterGrp = PvlGroup("Point_Properties");
  keyword = PvlKeyword("Ignore", "0");
  filterGrp += keyword;
  keyword = PvlKeyword("PointType", "constraineD");
  filterGrp += keyword;
  cnetFilter->PointPropertiesFilter(filterGrp,  false);
  cout << filterGrp << endl;
  filterGrp.clear();
  PrintControlNetInfo(*cnet);
  delete (cnet);
  delete (cnetFilter);
  cnetFilter = NULL;
  cnet = NULL;
  cout << "************************************************************************\n\n";

  // Point_LatLon  Filter
  cout << "****************** Point_LatLon Filter ******************" << endl;
  cnet = new ControlNet(ui.GetFileName("CNET"));
  cnetFilter = new ControlNetFilter(cnet, sSerialFile);
  filterGrp = PvlGroup("Point_LatLon");
  PvlKeyword keyword1("MinLat", toString(-100));
  filterGrp += keyword1;

  PvlKeyword keyword2("MaxLat", toString(100));
  filterGrp += keyword2;

  PvlKeyword keyword3("MinLon", toString(0));
  filterGrp += keyword3;

  PvlKeyword keyword4("MaxLon", toString(238));
  filterGrp += keyword4;

  cnetFilter->PointLatLonFilter(filterGrp,  false);
  cout << filterGrp << endl;
  filterGrp.clear();
  PrintControlNetInfo(*cnet);
  delete (cnet);
  delete (cnetFilter);
  cnetFilter = NULL;
  cnet = NULL;
  cout << "************************************************************************\n\n";

  // Point_Distance Filter
  cout << "****************** Point_Distance Filter ******************" << endl;
  cnet = new ControlNet(ui.GetFileName("CNET"));
  cnetFilter = new ControlNetFilter(cnet, sSerialFile);
  filterGrp = PvlGroup("Point_Distance");
  keyword = PvlKeyword("MaxDistance", toString(50000));
  filterGrp += keyword;
  keyword = PvlKeyword("Units", "meters");
  filterGrp += keyword;

  cnetFilter->PointDistanceFilter(filterGrp,  false);
  cout << filterGrp << endl;
  filterGrp.clear();
  PrintControlNetInfo(*cnet);
  delete (cnet);
  delete (cnetFilter);
  cnetFilter = NULL;
  cnet = NULL;
  cout << "************************************************************************\n\n";

  // Points_MeasureProperties Filter
  cout << "****************** Points_MeasureProperties Filter ******************" << endl;
  cnet = new ControlNet(ui.GetFileName("CNET"));
  cnetFilter = new ControlNetFilter(cnet, sSerialFile);
  filterGrp = PvlGroup("Point_MeasureProperties");
  keyword = PvlKeyword("MeasureType", "Candidate");
  filterGrp += keyword;
  cnetFilter->PointMeasurePropertiesFilter(filterGrp,  false);
  cout << filterGrp << endl;
  filterGrp.clear();
  PrintControlNetInfo(*cnet);
  delete (cnet);
  delete (cnetFilter);
  cnetFilter = NULL;
  cnet = NULL;
  cout << "************************************************************************\n\n";

  // Filter Points by Goodness of Fit
  cout << "****************** Points_GoodnessOfFit Filter ******************" << endl;
  cnet = new ControlNet(ui.GetFileName("CNET"));
  cnetFilter = new ControlNetFilter(cnet, sSerialFile);
  filterGrp = PvlGroup("Point_GoodnessOfFit");
  keyword = PvlKeyword("LessThan", toString(0.5));
  filterGrp += keyword;
  cnetFilter->PointGoodnessOfFitFilter(filterGrp,  false);
  cout << filterGrp << endl;
  PrintControlNetInfo(*cnet);
  filterGrp.clear();
  delete (cnet);
  delete (cnetFilter);
  cnetFilter = NULL;
  cnet = NULL;
  cout << "************************************************************************\n\n";

  // Point_CubeNames Filter
  cout << "****************** Point_CubeNames Filter ******************" << endl;
  cnet = new ControlNet(ui.GetFileName("CNET"));
  cnetFilter = new ControlNetFilter(cnet, sSerialFile);
  filterGrp = PvlGroup("Point_CubeNames");
  //keyword = PvlKeyword("Cube1", "Clementine1/UVVIS/1994-04-05T12:17:21.337");
  //filterGrp += keyword;
  //keyword = PvlKeyword("Cube2", "Clementine1/UVVIS/1994-03-08T20:03:40.056");
  //filterGrp += keyword;
  keyword = PvlKeyword("Cube3", "Clementine1/UVVIS/1994-03-08T20:04:59.856");
  filterGrp += keyword;
  keyword = PvlKeyword("Cube4", "Clementine1/UVVIS/1994-04-05T12:18:07.957");
  filterGrp += keyword;
  cnetFilter->PointCubeNamesFilter(filterGrp,  false);
  cout << filterGrp << endl;
  filterGrp.clear();
  PrintControlNetInfo(*cnet);
  delete (cnet);
  delete (cnetFilter);
  cnetFilter = NULL;
  cnet = NULL;
  cout << "************************************************************************\n\n";

  // Cube Filters
  // Cube_NameExpression Filter
  cout << "****************** Cube_NameExpression Filter ******************" << endl;
  cnet = new ControlNet(ui.GetFileName("CNET"));
  cnetFilter = new ControlNetFilter(cnet, sSerialFile);
  filterGrp = PvlGroup("Cube_NameExpression");
  keyword = PvlKeyword("Expression", "Clementine1/UVVIS/1994-04*");
  filterGrp += keyword;
  cnetFilter->CubeNameExpressionFilter(filterGrp,  false);
  cout << filterGrp << endl;
  filterGrp.clear();
  PrintControlNetInfo(*cnet);
  delete (cnet);
  delete (cnetFilter);
  cnetFilter = NULL;
  cnet = NULL;
  cout << "************************************************************************\n\n";

  // Cube_NumPoints Filter
  cout << "****************** Cube_NumPoints Filter ******************" << endl;
  cnet = new ControlNet(ui.GetFileName("CNET"));
  cnetFilter = new ControlNetFilter(cnet, sSerialFile);
  filterGrp = PvlGroup("Cube_NumPoints");
  keyword = PvlKeyword("GreaterThan", toString(10));
  filterGrp += keyword;
  keyword = PvlKeyword("LessThan", toString(20));
  filterGrp += keyword;
  cnetFilter->CubeNumPointsFilter(filterGrp,  false);
  cout << filterGrp << endl;
  filterGrp.clear();
  PrintControlNetInfo(*cnet);
  delete (cnet);
  delete (cnetFilter);
  cnetFilter = NULL;
  cnet = NULL;
  cout << "************************************************************************\n\n";

  // Cube_Distance Filter
  cout << "****************** Cube_Distance Filter ******************" << endl;
  cnet = new ControlNet(ui.GetFileName("CNET"));
  cnetFilter = new ControlNetFilter(cnet, sSerialFile);
  filterGrp = PvlGroup("Cube_Distance");
  keyword = PvlKeyword("MaxDistance", toString(100000));
  filterGrp += keyword;
  keyword = PvlKeyword("Units=", "meters");
  filterGrp += keyword;
  cnetFilter->CubeDistanceFilter(filterGrp,  false);
  cout << filterGrp << endl;
  filterGrp.clear();
  PrintControlNetInfo(*cnet);
  delete (cnet);
  delete (cnetFilter);
  cnetFilter = NULL;
  cnet = NULL;
  cout << "************************************************************************\n\n";

  // Cube_ConvexHullRatio Filter
  cout << "****************** Cube_ConvexHullRatio Filter ******************" << endl;
  cnet = new ControlNet(ui.GetFileName("CNET"));
  cnetFilter = new ControlNetFilter(cnet, sSerialFile);
  filterGrp = PvlGroup("Cube_ConvexHullRatio");
  keyword = PvlKeyword("GreaterThan", toString(0.2));
  filterGrp += keyword;
  keyword = PvlKeyword("LessThan", toString(0.3));
  filterGrp += keyword;
  cnetFilter->CubeConvexHullFilter(filterGrp,  false);
  cout << filterGrp << endl;
  filterGrp.clear();
  PrintControlNetInfo(*cnet);
  delete (cnet);
  delete (cnetFilter);
  cnetFilter = NULL;
  cnet = NULL;
  cout << "************************************************************************\n\n";
}

/**
 * Print Control Net info like Point ID and Measure SerialNum
 *
 * @author Sharmila Prasad (12/28/2010)
 *
 * @param pCNet - Control Net
 */
void PrintControlNetInfo(ControlNet &pCNet) {
  QList<QString> pointIds(pCNet.GetPointIds());
  pointIds.sort();

  for (int i = 0; i < pointIds.size(); i++) {
    const QString &pointId = pointIds[i];
    ControlPoint *controlPoint = pCNet[pointId];

    cerr << "Control Point ID  " << pointId.toStdString() << endl;
    QList<QString> serialNums(controlPoint->getCubeSerialNumbers());
    serialNums.sort();
    for (int j = 0; j < serialNums.size(); j++) {
      const QString &serialNum = serialNums.at(j);
      cerr << "   Measure SerialNum "
          << serialNum.toStdString() << endl;
    }
    cerr << endl;
  }
}
