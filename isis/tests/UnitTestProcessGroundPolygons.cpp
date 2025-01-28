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

#include "ProcessGroundPolygons.h"

using namespace Isis;

// Default test to check integrity of the geometry
TEST( UnitTestVectorize, Default ) {
		
  Isis::ProcessGroundPolygons g_processGroundPolygons;
  geos::geom::Geometry* groundpixel;
 
  std::vector<double> lons = {255.645377, 256.146301, 256.146301, 255.645377};
  std::vector<double> lats = {9.928429, 9.928429, 10.434929, 10.434929};

  groundpixel = g_processGroundPolygons.Vectorize(lats, lons);

  // Check that the geometry is valid
  EXPECT_EQ( groundpixel->isValid(), 1 );
  EXPECT_EQ( groundpixel->isPolygonal(), 1 );
  EXPECT_EQ( groundpixel->getGeometryType(), "Polygon");
  EXPECT_EQ( groundpixel->getNumGeometries(), 1); 
  EXPECT_EQ( groundpixel->getNumPoints(), lons.size() + 1 );
    
};

TEST( UnitTestVectorize, Crosses360 ) {
	
    Isis::ProcessGroundPolygons g_processGroundPolygons;
    geos::geom::Geometry* groundpixel;
	
    std::vector<double> lons = {359.0,   1.0,   1.0, 359.0, 359.0};
    std::vector<double> lats = {  0.0,   0.0,   1.0,   1.0,   0.0};	
	
	groundpixel = g_processGroundPolygons.Vectorize(lats, lons);
	
    // Check that the geometry is valid
    EXPECT_EQ( groundpixel->isValid(), 1 );
    EXPECT_EQ( groundpixel->isPolygonal(), 1 );
	EXPECT_EQ( groundpixel->getGeometryType(), "MultiPolygon");
    EXPECT_EQ( groundpixel->getNumGeometries(), 2); 	
};