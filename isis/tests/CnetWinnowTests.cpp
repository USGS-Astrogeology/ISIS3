#include <iostream>

#include "cnetwinnow.h"

#include "PixelType.h"
#include "Pvl.h"
#include "PvlGroup.h"
#include "PvlKeyword.h"
#include "TestUtilities.h"
#include "FileName.h"
#include "SerialNumber.h"

#include "NetworkFixtures.h"

#include "gmock/gmock.h"

using namespace Isis;

static QString APP_XML = QString::fromStdString(FileName("$ISISROOT/bin/xml/cnetwinnow.xml").expanded());

TEST_F(ThreeImageNetwork, FunctionalTestCnetwinnowDefault) {
  QString onetPath = tempDir.path()+"/winnowedNetwork.net";
  QVector<QString> args = {"onet="+onetPath,
                           "file_prefix=" + tempDir.path() + "/winnow"};
  UserInterface ui(APP_XML, args);

  int initialMeasureCount = network->GetNumValidMeasures();
  int initialPointCount = network->GetNumValidPoints();

  ASSERT_EQ(initialMeasureCount, 41);
  ASSERT_EQ(initialPointCount, 16);

  QList <ControlPoint*> pointList = network->GetPoints();

  QList <ControlMeasure*> measures;
  for (int count = 0; count < pointList.size(); count++) {
    measures = pointList[count]->getMeasures();
    for (ControlMeasure *measure: measures) {
      measure->SetResidual(count, count);
    }
  }

  SerialNumberList serialNumList(cubeListFile, true);
  cnetwinnow(*network, serialNumList, ui);

  int postWinnowMeasureCount = network->GetNumValidMeasures();
  int postWinnowPointCount = network->GetNumValidPoints();
  ASSERT_EQ(postWinnowMeasureCount, 31);
  ASSERT_EQ(postWinnowPointCount, 13);

  ControlNet onet(onetPath);
  postWinnowMeasureCount = onet.GetNumValidMeasures();
  postWinnowPointCount = onet.GetNumValidPoints();
  ASSERT_EQ(postWinnowMeasureCount, 31);
  ASSERT_EQ(postWinnowPointCount, 13);
}
