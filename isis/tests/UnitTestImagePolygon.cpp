#include "ImagePolygon.h"
#include "LineManager.h"
#include "PolygonTools.h"
#include "Preference.h"
#include "ProgramLauncher.h"
#include "Pvl.h"
#include "geos/geom/Point.h"
#include "geos/geom/MultiPolygon.h"
#include "geos/geom/CoordinateSequence.h"

#include <gtest/gtest.h>

#include "CameraFixtures.h"
#include "TempFixtures.h"

using namespace Isis;

TEST_F(DefaultCube, UnitTestImagePolygonDefaultParams) {
  ImagePolygon poly;
  try {
    poly.Create(*testCube);
  }
  catch(IException &e) {
    std::string msg = "Cannot create polygon for [" + testCube->fileName() + "]";
    throw IException(IException::Programmer, msg, _FILEINFO_);
  }
  ASSERT_EQ(4517, poly.numVertices());

  geos::geom::Geometry* boundary = poly.Polys()->getEnvelope().release();
  geos::geom::Point* centroid = poly.Polys()->getCentroid().release();

  std::vector<double> lons = {255.645377, 256.146301, 256.146301, 255.645377, 255.645377};
  std::vector<double> lats = {9.928429, 9.928429, 10.434929, 10.434929, 9.928429};

  geos::geom::CoordinateSequence coordArray = *(boundary->getCoordinates().release());
  for (size_t i = 0; i < coordArray.getSize(); i++) {
    EXPECT_NEAR(lons[i], coordArray.getAt(i).x, 1e-6);
    EXPECT_NEAR(lats[i], coordArray.getAt(i).y, 1e-6);
  }

  EXPECT_NEAR(255.895201, centroid->getX(), 1e-6);
  EXPECT_NEAR(10.182391, centroid->getY(), 1e-6);
}

TEST_F(DefaultCube, UnitTestImagePolygonSubPoly) {
  ImagePolygon poly;
  try {
    poly.Create(*testCube, 100, 100, 384, 640, 385);
  }
  catch(IException &e) {
    std::string msg = "Cannot create polygon for [" + testCube->fileName() + "]";
    throw IException(IException::Programmer, msg, _FILEINFO_);
  }
  ASSERT_EQ(19, poly.numVertices());

  geos::geom::Geometry* boundary = poly.Polys()->getEnvelope().release();
  geos::geom::Point* centroid = poly.Polys()->getCentroid().release();

  std::vector<double> lons = {255.894656, 256.081313, 256.081313, 255.894656, 255.894656};
  std::vector<double> lats = {10.039260, 10.039260, 10.213952, 10.213952, 10.039260};

  geos::geom::CoordinateSequence coordArray = *(boundary->getCoordinates().release());
  for (size_t i = 0; i < coordArray.getSize(); i++) {
    EXPECT_NEAR(lons[i], coordArray.getAt(i).x, 1e-6);
    EXPECT_NEAR(lats[i], coordArray.getAt(i).y, 1e-6);
  }

  EXPECT_NEAR(255.987979, centroid->getX(), 1e-6);
  EXPECT_NEAR(10.126704, centroid->getY(), 1e-6);
}

TEST_F(TempTestingFiles, UnitTestImagePolygonCross) {
  FileName isdFile("$ISISROOT/../isis/tests/data/footprintinit/cross.isd");
  FileName labelFile("$ISISROOT/../isis/tests/data/footprintinit/cross.pvl");

  Cube crossCube;
  crossCube.fromIsd(tempDir.path() + "/footprintCube.cub", labelFile, isdFile, "rw");

  ImagePolygon poly;
  try {
    poly.Create(crossCube, 100, 100);
  }
  catch(IException &e) {
    std::string msg = "Cannot create polygon for [" + crossCube.fileName() + "]";
    throw IException(IException::Programmer, msg, _FILEINFO_);
  }
  ASSERT_EQ(40, poly.numVertices());

  geos::geom::Geometry* boundary = poly.Polys()->getEnvelope().release();

  geos::geom::Point* centroid = poly.Polys()->getCentroid().release();

  std::vector<double> lons = {0.000000, 360.000000, 360.000000, 0.000000, 0.000000};
  std::vector<double> lats = {54.208699, 54.208699, 77.858559, 77.858559, 54.208699};

  geos::geom::CoordinateSequence coordArray = *(boundary->getCoordinates().release());
  for (size_t i = 0; i < coordArray.getSize(); i++) {
    EXPECT_NEAR(lons[i], coordArray.getAt(i).x, 1e-6);
    EXPECT_NEAR(lats[i], coordArray.getAt(i).y, 1e-6);
  }

  EXPECT_NEAR(214.399354, centroid->getX(), 1e-6);
  EXPECT_NEAR(67.471751, centroid->getY(), 1e-6);
}

TEST_F(DefaultCube, UnitTestImagePolygonBoundary) {
  Pvl footprintLabel;

  std::ifstream isdFile("data/footprintinit/boundary.isd");

  isdFile >> isd;

  PvlObject &core = label.findObject("IsisCube").findObject("Core");
  PvlGroup &instrument = label.findObject("IsisCube").findGroup("Instrument");
  PvlGroup &kernels = label.findObject("IsisCube").findGroup("Kernels");

  core.findGroup("Dimensions")["Samples"] = "1024";
  core.findGroup("Dimensions")["Lines"] = "1024";
  instrument["SpacecraftName"] = "Messenger";
  instrument["InstrumentId"] = "MDIS-NAC";
  instrument["TargetName"] = "Mercury";
  instrument["SpacecraftClockCount"] = "1/0108821505:976000";
  instrument.addKeyword(PvlKeyword("ExposureDuration", "14", "MS"), Pvl::Replace);
  instrument.addKeyword(PvlKeyword("FpuBinningMode", "0"));
  instrument.addKeyword(PvlKeyword("PixelBinningMode", "0"));

  kernels["CameraVersion"] = "2";
  kernels["NaifFrameCode"] = "-236820";
  kernels["SpacecraftClock"] = "$messenger/kernels/sclk/messenger_2548.tsc";
  kernels["ShapeModel"] = "$base/dems/MSGR_DEM_USG_EQ_I_V02_prep.cub";

  Cube footprintCube;
  footprintCube.fromIsd(tempDir.path() + "/footprintCube.cub", label, isd, "rw");

  ImagePolygon poly;
  try {
    poly.Create(footprintCube, 3000, 3000);
  }
  catch(IException &e) {
    std::string msg = "Cannot create polygon for [" + footprintCube.fileName() + "]";
    throw IException(IException::Programmer, msg, _FILEINFO_);
  }
  footprintCube.close();

  ASSERT_EQ(6, poly.numVertices());

  geos::geom::Geometry* boundary = poly.Polys()->getEnvelope().release();
  geos::geom::Point* centroid = poly.Polys()->getCentroid().release();

  std::vector<double> lons = {222.252839, 262.514600, 262.514600, 222.252839, 222.252839};
  std::vector<double> lats = {12.939321, 12.939321, 26.058448, 26.058448, 12.939321};

  geos::geom::CoordinateSequence coordArray = *(boundary->getCoordinates().release());
  for (size_t i = 0; i < coordArray.getSize(); i++) {
    EXPECT_NEAR(lons[i], coordArray.getAt(i).x, 1e-6);
    EXPECT_NEAR(lats[i], coordArray.getAt(i).y, 1e-6);
  }

  EXPECT_NEAR(242.543860, centroid->getX(), 1e-6);
  EXPECT_NEAR(19.733739, centroid->getY(), 1e-6);
}

TEST_F(TempTestingFiles, UnitTestImagePolygonMosaic) {
  Pvl footprintLabel;
  std::ifstream cubeLabel("data/footprintinit/mosaic.pvl");

  cubeLabel >> footprintLabel;

  Cube footprintCube;
  FileName footprintFile(tempDir.path() + "/footprintCube.cub");
  footprintCube.fromLabel(footprintFile, footprintLabel, "rw");

  LineManager line(footprintCube);
  double pixelValue = 1.0;
  for(line.begin(); !line.end(); line++) {
    for(int i = 0; i < line.size(); i++) {
      line[i] = (double) pixelValue;
    }
    footprintCube.write(line);
  }

  ImagePolygon poly;
  try {
    poly.Create(footprintCube);
  }
  catch(IException &e) {
    std::string msg = "Cannot create polygon for [" + footprintCube.fileName() + "]";
    throw IException(IException::Programmer, msg, _FILEINFO_);
  }
  footprintCube.close();

  ASSERT_EQ(16005, poly.numVertices());

  geos::geom::Geometry* boundary = poly.Polys()->getEnvelope().release();
  geos::geom::Point* centroid = poly.Polys()->getCentroid().release();

  std::vector<double> lons = {347.895055, 349.699395, 349.699395, 347.895055, 347.895055};
  std::vector<double> lats = {-43.643248, -43.643248, -42.323638, -42.323638, -43.643248};

  geos::geom::CoordinateSequence coordArray = *(boundary->getCoordinates().release());
  for (size_t i = 0; i < coordArray.getSize(); i++) {
    EXPECT_NEAR(lons[i], coordArray.getAt(i).x, 1e-6);
    EXPECT_NEAR(lats[i], coordArray.getAt(i).y, 1e-6);
  }

  EXPECT_NEAR(348.797225, centroid->getX(), 1e-6);
  EXPECT_NEAR(-42.983442, centroid->getY(), 1e-6);
}

TEST_F(DefaultCube, UnitTestImagePolygonOutlier) {
  Pvl footprintLabel;

  std::ifstream isdFile("data/footprintinit/outlier.isd");

  isdFile >> isd;

  PvlObject &core = label.findObject("IsisCube").findObject("Core");
  PvlGroup &instrument = label.findObject("IsisCube").findGroup("Instrument");
  PvlGroup &kernels = label.findObject("IsisCube").findGroup("Kernels");

  core.findGroup("Dimensions")["Samples"] = "1024";
  core.findGroup("Dimensions")["Lines"] = "1024";
  instrument["SpacecraftName"] = "Messenger";
  instrument["InstrumentId"] = "MDIS-NAC";
  instrument["TargetName"] = "Mercury";
  instrument["SpacecraftClockCount"] = "1/0215651170:929000";
  instrument.addKeyword(PvlKeyword("ExposureDuration", "14", "MS"), Pvl::Replace);
  instrument.addKeyword(PvlKeyword("FpuBinningMode", "0"));
  instrument.addKeyword(PvlKeyword("PixelBinningMode", "0"));

  PvlKeyword number("Number", "9");
  label.findObject("IsisCube").findGroup("BandBin").addKeyword(number);

  kernels["CameraVersion"] = "2";
  kernels["NaifFrameCode"] = "-236800";
  kernels["SpacecraftClock"] = "$messenger/kernels/sclk/messenger_2548.tsc";
  kernels["ShapeModel"] = "Null";

  Cube footprintCube;
  footprintCube.fromIsd(tempDir.path() + "/footprintCube.cub", label, isd, "rw");

  ImagePolygon poly;
  poly.Emission(89);
  poly.Incidence(89);
  try {
    poly.Create(footprintCube, 10, 10);
  }
  catch(IException &e) {
    std::string msg = "Cannot create polygon for [" + footprintCube.fileName() + "]";
    throw IException(IException::Programmer, msg, _FILEINFO_);
  }
  footprintCube.close();

  ASSERT_EQ(234, poly.numVertices());

  geos::geom::Geometry* boundary = poly.Polys()->getEnvelope().release();
  geos::geom::Point* centroid = poly.Polys()->getCentroid().release();

  std::vector<double> lons = {194.815808, 269.631838, 269.631838, 194.815808, 194.815808};
  std::vector<double> lats = {-66.783521, -66.783521, 5.718550, 5.718550, -66.783521};

  geos::geom::CoordinateSequence coordArray = *(boundary->getCoordinates().release());
  for (size_t i = 0; i < coordArray.getSize(); i++) {
    EXPECT_NEAR(lons[i], coordArray.getAt(i).x, 1e-6);
    EXPECT_NEAR(lats[i], coordArray.getAt(i).y, 1e-6);
  }

  EXPECT_NEAR(239.768815, centroid->getX(), 1e-6);
  EXPECT_NEAR(-32.260187, centroid->getY(), 1e-6);
}
