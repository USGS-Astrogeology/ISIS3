#include <iostream>
#include <QTemporaryFile>
#include <QTemporaryDir>

#include <geos/geom/Geometry.h>
#include <geos/geom/CoordinateSequence.h>

#include "footprintinit.h"

#include "Cube.h"
#include "ImagePolygon.h"
#include "Pvl.h"
#include "TestUtilities.h"
#include "FileName.h"
#include "LineManager.h"

#include "CameraFixtures.h"

#include "gmock/gmock.h"

using namespace Isis;

static QString APP_XML = QString::fromStdString(FileName("$ISISROOT/bin/xml/footprintinit.xml").expanded());

TEST_F(DefaultCube, FunctionalTestFootprintinitDefault) {
  QVector<QString> footprintArgs = {};
  UserInterface footprintUi(APP_XML, footprintArgs);

  footprintinit(testCube, footprintUi);
  ASSERT_TRUE(testCube->label()->hasObject("Polygon"));

  ImagePolygon poly = testCube->readFootprint();

  ASSERT_EQ(49, poly.numVertices());
  geos::geom::Geometry* boundary = poly.Polys()->getEnvelope().release();

  std::vector<double> lons = {255.645358, 256.146267, 256.146267, 255.645358, 255.645358};
  std::vector<double> lats = {9.928502, 9.928502, 10.434859, 10.434859, 9.928502};

  geos::geom::CoordinateSequence coordArray = *boundary->getCoordinates().release();
  for (size_t i = 0; i < coordArray.getSize(); i++) {
    EXPECT_NEAR(lons[i], coordArray.getAt(i).x, 1e-6);
    EXPECT_NEAR(lats[i], coordArray.getAt(i).y, 1e-6);
  }
}

TEST_F(DefaultCube, FunctionalTestFootprintinitLincSinc) {
  QVector<QString> footprintArgs = {"linc=50", "sinc=50"};
  UserInterface footprintUi(APP_XML, footprintArgs);

  footprintinit(testCube, footprintUi);
  ASSERT_TRUE(testCube->label()->hasObject("Polygon"));

  ImagePolygon poly = testCube->readFootprint();

  ASSERT_EQ(95, poly.numVertices());
  geos::geom::Geometry* boundary = poly.Polys()->getEnvelope().release();

  std::vector<double> lons = {255.645390, 256.146233, 256.146233, 255.645390, 255.645390};
  std::vector<double> lats = {9.928500, 9.928500, 10.434861, 10.434861, 9.928500};

  geos::geom::CoordinateSequence coordArray = *boundary->getCoordinates().release();
  for (size_t i = 0; i < coordArray.getSize(); i++) {
    EXPECT_NEAR(lons[i], coordArray.getAt(i).x, 1e-6);
    EXPECT_NEAR(lats[i], coordArray.getAt(i).y, 1e-6);
  }
}

TEST_F(DefaultCube, FunctionalTestFootprintinitVertices) {
  QVector<QString> footprintArgs = {"incType=vertices", "numvertices=40"};
  UserInterface footprintUi(APP_XML, footprintArgs);

  footprintinit(testCube, footprintUi);
  ASSERT_TRUE(testCube->label()->hasObject("Polygon"));

  ImagePolygon poly = testCube->readFootprint();

  ASSERT_EQ(43, poly.numVertices());
  geos::geom::Geometry* boundary = poly.Polys()->getEnvelope().release();

  std::vector<double> lons = {255.645374, 256.146251, 256.146251, 255.645374, 255.645374};
  std::vector<double> lats = {9.928456, 9.928456, 10.434903, 10.434903, 9.928456};

  geos::geom::CoordinateSequence coordArray = *boundary->getCoordinates().release();
  for (size_t i = 0; i < coordArray.getSize(); i++) {
    EXPECT_NEAR(lons[i], coordArray.getAt(i).x, 1e-6);
    EXPECT_NEAR(lats[i], coordArray.getAt(i).y, 1e-6);
  }
}

TEST_F(DefaultCube, FunctionalTestFootprintinitCamera) {
  QVector<QString> footprintArgs = {"maxemission=69", "maxincidence=70"};
  UserInterface footprintUi(APP_XML, footprintArgs);

  footprintinit(testCube, footprintUi);
  ASSERT_TRUE(testCube->label()->hasObject("Polygon"));

  ImagePolygon poly = testCube->readFootprint();

  ASSERT_EQ(34, poly.numVertices());
  geos::geom::Geometry* boundary = poly.Polys()->getEnvelope().release();

  std::vector<double> lons = {255.923821, 256.215272, 256.215272, 255.923821, 255.923821};
  std::vector<double> lats = {9.924583, 9.924583, 10.329275, 10.329275, 9.924583};

  geos::geom::CoordinateSequence coordArray = *boundary->getCoordinates().release();
  for (size_t i = 0; i < coordArray.getSize(); i++) {
    EXPECT_NEAR(lons[i], coordArray.getAt(i).x, 1e-6);
    EXPECT_NEAR(lats[i], coordArray.getAt(i).y, 1e-6);
  }
}

TEST_F(DefaultCube, FunctionalTestFootprintinitTestXY) {
  QVector<QString> footprintArgs = {"testxy=yes"};
  UserInterface footprintUi(APP_XML, footprintArgs);

  footprintinit(testCube, footprintUi);
  ASSERT_TRUE(testCube->label()->hasObject("Polygon"));

  ImagePolygon poly = testCube->readFootprint();

  ASSERT_EQ(49, poly.numVertices());
  geos::geom::Geometry* boundary = poly.Polys()->getEnvelope().release();

  std::vector<double> lons = {255.645358, 256.146267, 256.146267, 255.645358, 255.645358};
  std::vector<double> lats = {9.928502, 9.928502, 10.434859, 10.434859, 9.928502};

  geos::geom::CoordinateSequence coordArray = *boundary->getCoordinates().release();
  for (size_t i = 0; i < coordArray.getSize(); i++) {
    EXPECT_NEAR(lons[i], coordArray.getAt(i).x, 1e-6);
    EXPECT_NEAR(lats[i], coordArray.getAt(i).y, 1e-6);
  }
}

TEST_F(DefaultCube, FunctionalTestFootprintinitPrecision) {
  QVector<QString> footprintArgs = {"increaseprecision=yes"};
  UserInterface footprintUi(APP_XML, footprintArgs);

  Pvl log;

  footprintinit(testCube, footprintUi, &log);
  ASSERT_TRUE(testCube->label()->hasObject("Polygon"));

  ASSERT_TRUE(log.hasGroup("Results"));
  ASSERT_EQ(100, Isis::toInt(log.findGroup("Results").findKeyword("LINC")[0]));
  ASSERT_EQ(100, Isis::toInt(log.findGroup("Results").findKeyword("SINC")[0]));

  ImagePolygon poly = testCube->readFootprint();

  ASSERT_EQ(49, poly.numVertices());
  geos::geom::Geometry* boundary = poly.Polys()->getEnvelope().release();

  std::vector<double> lons = {255.645358, 256.146267, 256.146267, 255.645358, 255.645358};
  std::vector<double> lats = {9.928502, 9.928502, 10.434859, 10.434859, 9.928502};

  geos::geom::CoordinateSequence coordArray = *boundary->getCoordinates().release();
  for (size_t i = 0; i < coordArray.getSize(); i++) {
    EXPECT_NEAR(lons[i], coordArray.getAt(i).x, 1e-6);
    EXPECT_NEAR(lats[i], coordArray.getAt(i).y, 1e-6);
  }
}
