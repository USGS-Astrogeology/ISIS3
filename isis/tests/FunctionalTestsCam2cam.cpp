#include <iostream>
#include <QTemporaryFile>

#include "cam2cam.h"

#include "Cube.h"
#include "CubeAttribute.h"
#include "IException.h"
#include "PixelType.h"
#include "Pvl.h"
#include "PvlGroup.h"
#include "PvlKeyword.h"
#include "TestUtilities.h"
#include "FileName.h"
#include "ProjectionFactory.h"
#include "CameraFixtures.h"
#include "Mocks.h"

using namespace Isis;
using ::testing::Return;
using ::testing::AtLeast;

static QString APP_XML = FileName("$ISISROOT/bin/xml/cam2cam.xml").expanded();

TEST_F(DefaultCube, FunctionalTestCam2CamNoChange) {

  QVector<QString> args = {"to="+tempDir.path()+"/Cam2CamNoChange.cub", "INTERP=BILINEAR"};
  UserInterface ui(APP_XML, args);

  testCube->reopen("r");
  QString inFile = testCube->fileName();
  Cube mcube(inFile,"r");

  cam2cam(testCube, &mcube, ui);

  Cube icube(inFile);
  PvlGroup icubeInstrumentGroup = icube.label()->findGroup("Instrument", Pvl::Traverse);

  Cube ocube(tempDir.path()+"/Cam2CamNoChange.cub");
  PvlGroup ocubeInstrumentGroup = ocube.label()->findGroup("Instrument", Pvl::Traverse);

  ASSERT_EQ(icubeInstrumentGroup.findKeyword("SpacecraftName"), ocubeInstrumentGroup.findKeyword("SpacecraftName"));
  ASSERT_EQ(icubeInstrumentGroup.findKeyword("InstrumentId"), ocubeInstrumentGroup.findKeyword("InstrumentID"));
  ASSERT_EQ(icubeInstrumentGroup.findKeyword("TargetName"), ocubeInstrumentGroup.findKeyword("TargetName"));
  ASSERT_EQ(icubeInstrumentGroup.findKeyword("StartTime"), ocubeInstrumentGroup.findKeyword("StartTime"));
  ASSERT_EQ(icubeInstrumentGroup.findKeyword("ExposureDuration"), ocubeInstrumentGroup.findKeyword("ExposureDuration"));
  ASSERT_EQ(icubeInstrumentGroup.findKeyword("SpacecraftClockCount"), ocubeInstrumentGroup.findKeyword("SpacecraftClockCount"));
  ASSERT_EQ(icubeInstrumentGroup.findKeyword("FloodModeId"), ocubeInstrumentGroup.findKeyword("FloodModeId"));
  ASSERT_EQ(icubeInstrumentGroup.findKeyword("GainModeId"), ocubeInstrumentGroup.findKeyword("GainModeId"));
  ASSERT_EQ(icubeInstrumentGroup.findKeyword("OffsetModeId"), ocubeInstrumentGroup.findKeyword("OffsetModeId"));
}

// This test evaluates the behavior of example 1 in the cam2cam app. The pixel
// locations are arbitrary but in the regions described in the example
// explanation. Unfortunately, the OREX OCAMS ALE implementation is broken
// in that the lat/lons are way off. However, the mapping behavior seems 
// consistent as described...maybe. ALE is preferred since it does not require
// repo storage of a complete or subsampled binary ISIS image.

																							//TEST_F( OrexManyIsdCameraCubes, FunctionalTestCam2CamOffbody ) {
																							// typedef std::unique_ptr<Cube>   LocalCubePtr;
/**
   * Test the behavior of OFFBODY and OFFBODYTRIM.
  */
TEST_F(DefaultCube, FunctionalTestCam2CamOffbody) {

  const double tolerance_d = 0.05;
  const double tolerance_p = 0.00001;

  const bool OffBodyTrue      = true;
  const bool OffBodyFalse     = false;
  const bool OffBodyTrimTrue  = true;
  const bool OffBodyTrimFalse = false;

// Use an actual cube that has been spiceinited with a high-resolution digital
// shape kernel (DSK).
// Cam2cam takes a FROM and a MATCH cube, which are ostensibly different.
// At first, we will use the same cube for both the FROM and the MATCH.
// Later, we will ignore the high resolution topography in one of the input cubes' camera
// models to compare the vector tracing using the low resolution ellipsoid model.
  QString fCubeName = "$ISISROOT/../isis/tests/data/osirisRexImages/20190509T180552S020_DSK_reduced.cub";
  QString mCubeName = fCubeName;
  
  Cube cube_ell;
  Cube cube_dsk; 

  cube_ell.open(fCubeName);
  cube_dsk.open(mCubeName);

// Create the camera model instances from the input cube.
  Camera *from_ell = cube_ell.camera();
  Camera *from_dsk = cube_dsk.camera();

// Vector variables.
  double v_match_sample;
  double v_match_line;
  double v_from_sample;
  double v_from_line;  

  double v_latitude;
  double v_longitude;
  
// See how well pixel locations map to lat/lon, and back to sample/line, where
// both cubes use the DSK (by default).
v_match_sample = 256.0;
v_match_line   = 256.0;

  // Get the latitude and longitude at the specified pixel.
  EXPECT_TRUE( from_dsk->SetImage( v_match_sample, v_match_line ) );
  v_latitude  = from_dsk->UniversalLatitude( );
  v_longitude = from_dsk->UniversalLongitude( );

  // Check that the values are as expected.
  EXPECT_NEAR( v_latitude,  -27.334564355166282,  tolerance_d );
  EXPECT_NEAR( v_longitude, 332.723940352585032,  tolerance_d );  

  // Get the corresponding pixel at that latitude and longitude from the other cube.
  // (Note that both cubes are still identical at this step.)
  EXPECT_TRUE( from_ell->SetUniversalGround( v_latitude, v_longitude) );
  double v_insample = from_ell->Sample();
  double v_inline   = from_ell->Line();
  
  // Check that the sample and line at those coordinates are the same. 
  EXPECT_NEAR( v_insample, v_match_sample, tolerance_p );
  EXPECT_NEAR( v_inline,   v_match_line,   tolerance_p );

  // Check that the RA/DEC coordinates are as expected.
  EXPECT_NEAR( from_dsk->RightAscension( ), 179.38575724578, tolerance_d );
  EXPECT_NEAR( from_dsk->Declination( ),   -1.9176166684378, tolerance_d );


// Now tell the camera model to just use the ellipsoid in the referenced cube.
  from_ell->IgnoreElevationModel( true );

  // Check that the vector intersects the surface in the ellipsoid cube 
  // and in the DSK cube at the given pixel which is near the limb, 
  // but still on the target body surface.
  v_match_sample = 182.0;
  v_match_line   =  25.0;
  EXPECT_TRUE( from_ell->SetImage( v_match_sample, v_match_line ) );
  EXPECT_TRUE( from_dsk->SetImage( v_match_sample, v_match_line ) );

  // The coordinates should not be identical due to the differences between the 
  // ellipsoid and the DSK.
  EXPECT_TRUE( from_dsk->UniversalLatitude()  != from_ell->UniversalLatitude() );
  EXPECT_TRUE( from_dsk->UniversalLongitude() != from_ell->UniversalLongitude() );


// Check a pixel just off the limb.
  v_match_sample = 150.0;
  v_match_line   = 485.0;

  // For the ellipsoid, the pixel is on the target body.
  // For the DSK, this pixel is off the target body.
  EXPECT_TRUE(  from_ell->SetImage( v_match_sample, v_match_line ) );
  EXPECT_FALSE( from_dsk->SetImage( v_match_sample, v_match_line ) );


  // Create individual mapper functors. 
  // In each case, the FROM cube is the ellipsoid, and the MATCH cube is the DSK.
  Cam2CamMapper map_from_default(    &cube_ell, &cube_dsk, OffBodyFalse, OffBodyTrimFalse  ); 
  Cam2CamMapper map_from_off_notrim( &cube_ell, &cube_dsk, OffBodyTrue,  OffBodyTrimFalse ); 
  Cam2CamMapper map_from_off_trim(   &cube_ell, &cube_dsk, OffBodyTrue,  OffBodyTrimTrue ); 

  // Validate functor configurations.
  EXPECT_TRUE( map_from_default.IsValid() );
  EXPECT_TRUE( map_from_off_trim.IsValid() );
  EXPECT_TRUE( map_from_off_notrim.IsValid() );

  // Test a pixel that is off body in upper left corner
  v_match_sample = 25.0;
  v_match_line   = 25.0;
 
  // This pixel should fail to return a valid surface location because it is off-body in both cubes.
  EXPECT_FALSE( map_from_default.from_camera()->SetImage(  v_match_sample, v_match_line) );
  EXPECT_FALSE( map_from_default.match_camera()->SetImage( v_match_sample, v_match_line) );

  // In the default case (OFFBODY=FALSE) this location returns null in both cubes.
  // In the OFFBODY=TRUE, OFFBODYTRIM=FALSE case, this location returns a valid pixel value in both.
  // In the OFFBODY=TRUE, OFFBODYTRIM=TRUE case, this location returns a valid pixel value in both.
  EXPECT_FALSE( map_from_default.mapit(    v_from_sample, v_from_line, v_match_sample, v_match_line) ); 
  EXPECT_TRUE(  map_from_off_notrim.mapit( v_from_sample, v_from_line, v_match_sample, v_match_line) );
  EXPECT_TRUE(  map_from_off_trim.mapit(   v_from_sample, v_from_line, v_match_sample, v_match_line) ); 


// Check a pixel just off the limb that is off body in the DSK cube, 
// but on the target body in the ellipsoid cube.
  v_match_sample = 150.0;
  v_match_line   = 485.0;
   
  // Reset the individual mapper functors. 
  // In each case, the FROM cube is the dsk, and the MATCH cube is the ellipsoid.
  map_from_default    = Cam2CamMapper( &cube_dsk, &cube_ell, OffBodyFalse, OffBodyTrimFalse ); 
  map_from_off_notrim = Cam2CamMapper( &cube_dsk, &cube_ell, OffBodyTrue,  OffBodyTrimFalse ); 
  map_from_off_trim   = Cam2CamMapper( &cube_dsk, &cube_ell, OffBodyTrue,  OffBodyTrimTrue  ); 

  // Validate functor configurations.
  EXPECT_TRUE( map_from_default.IsValid() );
  EXPECT_TRUE( map_from_off_trim.IsValid() );
  EXPECT_TRUE( map_from_off_notrim.IsValid() );

  // The pixel should fail to return a valid surface location in the DSK (FROM) cube, 
  // but should return a valid location in the ellipsoid (MATCH) cube.
  EXPECT_FALSE( map_from_default.from_camera()->SetImage( v_match_sample, v_match_line) );
  EXPECT_TRUE( map_from_default.match_camera()->SetImage( v_match_sample, v_match_line) );
 
  // The result is false in each case because this location is on the ellipsoid,  
  // but is null in the DSK.
  EXPECT_FALSE(  map_from_default.mapit(   v_from_sample, v_from_line, v_match_sample, v_match_line) ); 
  EXPECT_FALSE( map_from_off_notrim.mapit( v_from_sample, v_from_line, v_match_sample, v_match_line) ); 
  EXPECT_FALSE( map_from_off_trim.mapit(   v_from_sample, v_from_line, v_match_sample, v_match_line) ); 

 
// Check a pixel that is on the target, but in an occluded area in the DSK.
  v_match_sample = 220.0;
  v_match_line   =  40.0;

  // This pixel should return a valid surface location in both cubes because it is
  // on the target body surface.
  EXPECT_TRUE( map_from_default.from_camera()->SetImage(  v_match_sample, v_match_line) );
  EXPECT_TRUE( map_from_default.match_camera()->SetImage( v_match_sample, v_match_line) );

  // The expected result is false in each case because although this location is on the ellipsoid,
  // when mapped to the FROM (DSK) cube, it is in an occluded area and so does not return a 
  // valid result.
  EXPECT_FALSE( map_from_default.mapit(    v_from_sample, v_from_line, v_match_sample, v_match_line) ); 
  EXPECT_FALSE( map_from_off_notrim.mapit( v_from_sample, v_from_line, v_match_sample, v_match_line) );  
  EXPECT_FALSE( map_from_off_trim.mapit(   v_from_sample, v_from_line, v_match_sample, v_match_line) ); 


// Check the pixel just off the limb.
  v_match_sample = 150.0;
  v_match_line   = 475.0;  
   
  // Reset the individual mapper functors. 
  // In each case, the 'from' cube is the ellipsoid, and the 'match' cube is the dsk.
  map_from_default    = Cam2CamMapper( &cube_ell, &cube_dsk, OffBodyFalse, OffBodyTrimFalse );
  map_from_off_notrim = Cam2CamMapper( &cube_ell, &cube_dsk, OffBodyTrue,  OffBodyTrimFalse ); 
  map_from_off_trim   = Cam2CamMapper( &cube_ell, &cube_dsk, OffBodyTrue,  OffBodyTrimTrue  ); 

  // Validate functor configurations
  EXPECT_TRUE( map_from_default.IsValid() );
  EXPECT_TRUE( map_from_off_trim.IsValid() );
  EXPECT_TRUE( map_from_off_notrim.IsValid() );

 
  // This pixel should return a valid surface location in the from cube because although
  // it is off the limb (off body) it is on the ellipsoid.
  // In the MATCH cube, this is a null value (off the body and off the DSK). 
  EXPECT_TRUE(  map_from_default.from_camera()->SetImage(  v_match_sample, v_match_line) );
  EXPECT_FALSE( map_from_default.match_camera()->SetImage( v_match_sample, v_match_line) );

  // In the default case, because this pixel is off the body in the MATCH (DSK) cube,
  // it is expected to be invalid in the FROM (ellipsoid) cube.
  // In the OFFBODY=TRUE, OFFBODYTRIM=FALSE case, the pixel is valid because off-body
  // locations are not being trimmed.
  // In the OFFBODY=TRUE, OFFBODYTRIM=TRUE case, the pixel is valid in the MATCH cube,
  // but is being trimmed so is not mapped as valid in the FROM cube.
  EXPECT_FALSE(  map_from_default.mapit(  v_from_sample, v_from_line, v_match_sample, v_match_line) );
  EXPECT_TRUE( map_from_off_notrim.mapit( v_from_sample, v_from_line, v_match_sample, v_match_line) );
  EXPECT_FALSE( map_from_off_trim.mapit(   v_from_sample, v_from_line, v_match_sample, v_match_line) );


// Check a pixel in the center of the image, on body and not occluded.
  v_match_sample = 256.0;
  v_match_line   = 256.0;
 
  // Both cases should be true because this pixel is a valid surface in both the 
  // DSK and ellipsoid
  EXPECT_TRUE( map_from_default.from_camera()->SetImage(  v_match_sample, v_match_line) );
  EXPECT_TRUE( map_from_default.match_camera()->SetImage( v_match_sample, v_match_line) );
 
  // In all cases, this pixel will return a valid surface value. 
  EXPECT_TRUE( map_from_default.mapit( v_from_sample, v_from_line, v_match_sample, v_match_line) );
  EXPECT_TRUE( map_from_off_notrim.mapit( v_from_sample, v_from_line, v_match_sample, v_match_line) );
  EXPECT_TRUE( map_from_off_trim.mapit( v_from_sample, v_from_line, v_match_sample, v_match_line) ); 

}

