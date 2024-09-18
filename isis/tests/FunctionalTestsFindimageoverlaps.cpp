#include <iostream>
#include <QTemporaryFile>

#include "findimageoverlaps.h"
#include "NetworkFixtures.h"
#include "TestUtilities.h"
#include "IException.h"
#include "FileList.h"
#include "ImagePolygon.h"
#include "PolygonTools.h"
#include "ImageOverlap.h"
#include "ImageOverlapSet.h"
#include "Blob.h"
#include "Pvl.h"

#include <geos/io/WKTReader.h>
#include <geos/io/WKTWriter.h>
#include "geos/geom/CoordinateSequence.h"
#include "geos/geom/LinearRing.h"
#include "geos/geom/Polygon.h"

#include "gmock/gmock.h"

using json = nlohmann::json;

using namespace Isis;

static QString APP_XML = QString::fromStdString(FileName("$ISISROOT/bin/xml/findimageoverlaps.xml").expanded());

TEST_F(ThreeImageNetwork, FunctionalTestFindimageoverlapsNoOverlap) {
  ImagePolygon fp1;
  fp1.Create(*cube1);
  cube1->write(fp1);

  Cube newCube2;
  json newIsd2;
  std::ifstream i(isdPath2->expanded());
  i >> newIsd2;

  newIsd2["instrument_position"]["positions"] = {{1,1,1}, {2,2,2}, {3,3,3}};
  newCube2.fromIsd(tempDir.path().toStdString()+"/new2.cub", *cube2->label(), newIsd2, "rw");

  ImagePolygon fp2;
  fp2.Create(newCube2);
  newCube2.write(fp2);

  FileList cubes;
  cubes.append(cube1->fileName().toStdString());
  cubes.append(newCube2.fileName().toStdString());
  cube1->close();
  cube2->close();
  newCube2.close();

  QString cubeListPath = tempDir.path() + "/cubes.lis";
  cubes.write(cubeListPath.toStdString());
  QVector<QString> args = {"from="+cubeListPath, "overlapList="+tempDir.path()+"/overlaps.txt"};
  UserInterface options(APP_XML, args);
  Pvl appLog;

  try {
    findimageoverlaps(options, false, &appLog);
    FAIL() << "Expected an IException with message: \"No overlaps were found\".";
  }
  catch(IException &e) {
    EXPECT_TRUE(e.toString().find("No overlaps were found") != std::string::npos)
      <<  e.toString();
  }
}

TEST_F(ThreeImageNetwork, FunctionalTestFindimageoverlapsTwoImageOverlap) {

  QVector<QString> args = {"OVERLAPLIST=" + tempDir.path() + "/overlaps.txt", "detailed=true", "errors=true"};
  UserInterface ui(APP_XML, args);
  FileList images;
  images.append(FileName(cube1->fileName().toStdString()));
  images.append(FileName(cube2->fileName().toStdString()));
  findimageoverlaps(images, ui, false, nullptr);

  // Find all the overlaps between the images in the FROMLIST
  // The overlap polygon coordinates are in Lon/Lat order
  ImageOverlapSet overlaps;
  overlaps.ReadImageOverlaps(ui.GetFileName("OVERLAPLIST"));
  ASSERT_EQ(overlaps.Size(), 3);
  const ImageOverlap *poi;
  const geos::geom::MultiPolygon *mp;

  poi = overlaps[0];
  mp = poi->Polygon();
  ASSERT_EQ(mp->getArea(), 14);
  ASSERT_EQ(poi->Size(), 1);
  ASSERT_EQ((*poi)[0], "MGS/688540926:0/MOC-WA/RED");

  poi = overlaps[1];
  mp = poi->Polygon();
  ASSERT_EQ(mp->getArea(), 14);
  ASSERT_EQ(poi->Size(), 1);
  ASSERT_EQ((*poi)[0], "MGS/691204200:96/MOC-WA/RED");

  poi = overlaps[2];
  mp = poi->Polygon();
  ASSERT_EQ(mp->getArea(), 36);
  ASSERT_EQ(poi->Size(), 2);
  ASSERT_EQ((*poi)[0], "MGS/688540926:0/MOC-WA/RED");
  ASSERT_EQ((*poi)[1], "MGS/691204200:96/MOC-WA/RED");
}

TEST_F(ThreeImageNetwork, FunctionalTestFindimageoverlapsFullOverlap) {

  ImagePolygon poly;
  coords = {{31, 1},
            {31, 9},
            {34, 9},
            {34, 1},
            {31, 1}};
  poly.Create(coords);
  cube2->write(poly);
  cube2->reopen("rw");

  QVector<QString> args = {"OVERLAPLIST=" + tempDir.path() + "/overlaps.txt", "detailed=true", "errors=true"};
  UserInterface ui(APP_XML, args);
  FileList images;
  images.append(FileName(cube1->fileName().toStdString()));
  images.append(FileName(cube2->fileName().toStdString()));
  findimageoverlaps(images, ui, false, nullptr);

  // Find all the overlaps between the images in the FROMLIST
  // The overlap polygon coordinates are in Lon/Lat order
  ImageOverlapSet overlaps;
  overlaps.ReadImageOverlaps(ui.GetFileName("OVERLAPLIST"));
  ASSERT_EQ(overlaps.Size(), 2);
  const ImageOverlap *poi;
  const geos::geom::MultiPolygon *mp;

  poi = overlaps[0];
  mp = poi->Polygon();
  ASSERT_EQ(mp->getArea(), 26);
  ASSERT_EQ(poi->Size(), 1);
  ASSERT_EQ((*poi)[0], "MGS/688540926:0/MOC-WA/RED");

  poi = overlaps[1];
  mp = poi->Polygon();
  ASSERT_EQ(mp->getArea(), 24);
  ASSERT_EQ(poi->Size(), 2);
  ASSERT_EQ((*poi)[0], "MGS/691204200:96/MOC-WA/RED");
  ASSERT_EQ((*poi)[1], "MGS/688540926:0/MOC-WA/RED");
}
