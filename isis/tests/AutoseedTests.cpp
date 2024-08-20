#include <iostream>
#include <QTemporaryFile>
#include <QTemporaryDir>
#include <QVector>

#include <geos/geom/GeometryFactory.h>
#include <geos/geom/Point.h>
#include <geos/io/WKTReader.h>
#include <geos/io/WKTWriter.h>

#include "autoseed.h"
#include "findimageoverlaps.h"
#include "footprintinit.h"

#include "Camera.h"
#include "CameraFactory.h"
#include "Cube.h"
#include "CubeAttribute.h"
#include "FileName.h"
#include "ImageOverlapSet.h"
#include "SerialNumber.h"
#include "PixelType.h"
#include "Pvl.h"
#include "PvlGroup.h"
#include "PvlKeyword.h"
#include "SerialNumberList.h"
#include "TestUtilities.h"

#include "NetworkFixtures.h"

#include "gmock/gmock.h"

using namespace Isis;

static QString APP_XML = FileName("$ISISROOT/bin/xml/autoseed.xml").expanded();

TEST_F(ThreeImageNetwork, FunctionalTestAutoseedDefault) {
  Pvl *log = NULL;
  QString defFile = tempDir.path()+"/gridPixels.pvl";
  QString outnet = tempDir.path()+"/seeded.net";

  PvlObject autoseedObject("AutoSeed");
  PvlGroup autoseedGroup("PolygonSeederAlgorithm");
  autoseedGroup.addKeyword(PvlKeyword("Name", "Grid"));
  autoseedGroup.addKeyword(PvlKeyword("MinimumThickness", "0.0"));
  autoseedGroup.addKeyword(PvlKeyword("MinimumArea", "1000"));
  autoseedGroup.addKeyword(PvlKeyword("XSpacing", "24000"));
  autoseedGroup.addKeyword(PvlKeyword("YSpacing", "24000"));
  autoseedGroup.addKeyword(PvlKeyword("PixelsFromEdge", "20.0"));
  autoseedObject.addGroup(autoseedGroup);

  Pvl autoseedDef;
  autoseedDef.addObject(autoseedObject);
  autoseedDef.write(defFile.toStdString());

  QVector<QString> autoseedArgs = {"fromlist="+cubeListFile,
                                    "onet="+outnet,
                                    "deffile="+defFile,
                                    "overlaplist="+threeImageOverlapFile->original(),
                                    "networkid=1",
                                    "pointid=??",
                                    "description=autoseed test network"};
  UserInterface autoseedUi(APP_XML, autoseedArgs);

  autoseed(autoseedUi, log);
  ControlNet onet(outnet);
  ASSERT_EQ(onet.GetNumPoints(), 26);
}

TEST_F(ThreeImageNetwork, FunctionalTestAutoseedCnetInput) {
  Pvl *log = NULL;
  QString defFile = tempDir.path()+"/gridPixels.pvl";
  QString outnet1 = tempDir.path()+"/seeded_two_file.net";
  QString outnet2 = tempDir.path()+"/seeded_three_file.net";

  PvlObject autoseedObject("AutoSeed");
  PvlGroup autoseedGroup("PolygonSeederAlgorithm");
  autoseedGroup.addKeyword(PvlKeyword("Name", "Grid"));
  autoseedGroup.addKeyword(PvlKeyword("MinimumThickness", "0.0"));
  autoseedGroup.addKeyword(PvlKeyword("MinimumArea", "1000"));
  autoseedGroup.addKeyword(PvlKeyword("XSpacing", "24000"));
  autoseedGroup.addKeyword(PvlKeyword("YSpacing", "24000"));
  autoseedGroup.addKeyword(PvlKeyword("PixelsFromEdge", "20.0"));
  autoseedObject.addGroup(autoseedGroup);

  Pvl autoseedDef;
  autoseedDef.addObject(autoseedObject);
  autoseedDef.write(defFile.toStdString());

  cubeList->removeLast();
  cubeList->write(cubeListFile);

  QVector<QString> autoseedArgs = {"fromlist="+cubeListFile,
                                    "onet="+outnet1,
                                    "deffile="+defFile,
                                    "overlaplist="+twoImageOverlapFile->original(),
                                    "networkid=1",
                                    "pointid=??",
                                    "description=autoseed test network"};
  UserInterface autoseedUi(APP_XML, autoseedArgs);

  autoseed(autoseedUi, log);
  ControlNet onet(outnet1);
  ASSERT_EQ(onet.GetNumPoints(), 18);

  cubeList->append(cube3->fileName());
  cubeList->write(cubeListFile);
  SerialNumberList serialNumList(cubeListFile);

  autoseedArgs.removeAt(0);
  autoseedArgs.replace(1, "onet="+outnet2);
  autoseedArgs.replace(3, "overlaplist="+threeImageOverlapFile->original());
  autoseedUi = UserInterface(APP_XML, autoseedArgs);
  autoseed(autoseedUi, serialNumList, &onet, log);
  onet = ControlNet(outnet2);
  ASSERT_EQ(onet.GetNumPoints(), 7);
}

TEST_F(ThreeImageNetwork, FunctionalTestAutoseedDeffiles) {
  Pvl *log = NULL;
  QString defFile = tempDir.path()+"/pixelGrid.pvl";
  QString outnet = tempDir.path()+"/new.net";

  QVector<QString> autoseedArgs = {"fromlist="+cubeListFile,
                                    "onet="+outnet,
                                    "deffile="+defFile,
                                    "overlaplist="+threeImageOverlapFile->original(),
                                    "networkid=1",
                                    "pointid=???",
                                    "description=autoseed test network"};
  UserInterface autoseedUi(APP_XML, autoseedArgs);

  // Grid DN
  PvlObject autoseedObject("AutoSeed");
  PvlGroup autoseedGroup("PolygonSeederAlgorithm");
  autoseedGroup.addKeyword(PvlKeyword("Name", "Grid"));
  autoseedGroup.addKeyword(PvlKeyword("MinimumThickness", "0.0"));
  autoseedGroup.addKeyword(PvlKeyword("MinimumArea", "1000"));
  autoseedGroup.addKeyword(PvlKeyword("XSpacing", "20000"));
  autoseedGroup.addKeyword(PvlKeyword("YSpacing", "10000"));
  autoseedGroup.addKeyword(PvlKeyword("MinDN", "0.145"));
  autoseedGroup.addKeyword(PvlKeyword("MaxDN", "0.175"));
  autoseedObject.addGroup(autoseedGroup);

  Pvl autoseedDef;
  autoseedDef.addObject(autoseedObject);
  autoseedDef.write(defFile.toStdString());

  autoseed(autoseedUi, log);
  ControlNet onet(outnet);
  ASSERT_EQ(onet.GetNumValidPoints(), 0);
  ASSERT_EQ(onet.GetNumValidMeasures(), 0);

  // Grid Emission
  autoseedDef.findObject("AutoSeed").findGroup("PolygonSeederAlgorithm").deleteKeyword("MinDN");
  autoseedDef.findObject("AutoSeed").findGroup("PolygonSeederAlgorithm").deleteKeyword("MaxDN");
  autoseedDef.findObject("AutoSeed").findGroup("PolygonSeederAlgorithm").addKeyword(PvlKeyword("MinEmission", "18.0"));
  autoseedDef.findObject("AutoSeed").findGroup("PolygonSeederAlgorithm").addKeyword(PvlKeyword("MaxEmission", "75.0"));
  autoseedDef.write(defFile.toStdString());

  autoseed(autoseedUi, log);
  onet = ControlNet(outnet);
  ASSERT_EQ(onet.GetNumValidPoints(), 50);
  ASSERT_EQ(onet.GetNumValidMeasures(), 126);

  // Grid Pixels
  autoseedDef.findObject("AutoSeed").findGroup("PolygonSeederAlgorithm").deleteKeyword("MinEmission");
  autoseedDef.findObject("AutoSeed").findGroup("PolygonSeederAlgorithm").deleteKeyword("MaxEmission");
  autoseedDef.findObject("AutoSeed").findGroup("PolygonSeederAlgorithm").addKeyword(PvlKeyword("PixelsFromEdge", "20.0"));
  autoseedDef.write(defFile.toStdString());

  autoseed(autoseedUi, log);
  onet = ControlNet(outnet);
  ASSERT_EQ(onet.GetNumValidPoints(), 57);
  ASSERT_EQ(onet.GetNumValidMeasures(), 144);

  // Grid Incidence
  autoseedDef.findObject("AutoSeed").findGroup("PolygonSeederAlgorithm").deleteKeyword("PixelsFromEdge");
  autoseedDef.findObject("AutoSeed").findGroup("PolygonSeederAlgorithm").addKeyword(PvlKeyword("MinIncidence", "35.0"));
  autoseedDef.findObject("AutoSeed").findGroup("PolygonSeederAlgorithm").addKeyword(PvlKeyword("MaxIncidence", "65.0"));
  autoseedDef.write(defFile.toStdString());

  autoseed(autoseedUi, log);
  onet = ControlNet(outnet);
  ASSERT_EQ(onet.GetNumValidPoints(), 72);
  ASSERT_EQ(onet.GetNumValidMeasures(), 181);

  // Grid Resolution
  autoseedDef.findObject("AutoSeed").findGroup("PolygonSeederAlgorithm").deleteKeyword("MinIncidence");
  autoseedDef.findObject("AutoSeed").findGroup("PolygonSeederAlgorithm").deleteKeyword("MaxIncidence");
  autoseedDef.findObject("AutoSeed").findGroup("PolygonSeederAlgorithm").addKeyword(PvlKeyword("MinResolution", "255.0"));
  autoseedDef.findObject("AutoSeed").findGroup("PolygonSeederAlgorithm").addKeyword(PvlKeyword("MaxResolution", "259.0"));
  autoseedDef.write(defFile.toStdString());

  autoseed(autoseedUi, log);
  onet = ControlNet(outnet);
  ASSERT_EQ(onet.GetNumValidPoints(), 60);
  ASSERT_EQ(onet.GetNumValidMeasures(), 147);
}
