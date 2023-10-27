#include "Isis.h"

#include <cmath>

#include "Angle.h"
#include "Camera.h"
#include "Distance.h"
#include "Latitude.h"
#include "Longitude.h"
#include "ProcessByBoxcar.h"
#include "Projection.h"
#include "SurfacePoint.h"
#include "Target.h"
#include "UniversalGroundMap.h"

using namespace std;
using namespace Isis;

UniversalGroundMap *g_groundMap;
SurfacePoint g_upperLeft;
SurfacePoint g_lowerLeft;
double g_conversionFactor;

enum OutputType {
  Aspect,
  Slope,
  PercentSlope
};
OutputType g_outputType;

enum Units {
  Degrees,
  Radians,
};
Units g_units;

double g_resolution;

void createSlpCube(Buffer &in, double &v);
void createSlpCubeAutomatic(Buffer &in, double &v);
void createAspectCube(Buffer &in, double &v);

void IsisMain() {
  // Process using a 3x3 boxcar
  ProcessByBoxcar p;
  Cube *icube = p.SetInputCube("FROM");
  p.SetBoxcarSize(3, 3);

  // Get the output type either ASPECT, SLOPE, or SLOPEPERCENT
  UserInterface &ui = Application::GetUserInterface();
  if (ui.GetString("OUTPUT") == "ASPECT") {
    g_outputType = Aspect;
  }
  else if (ui.GetString("OUTPUT") == "SLOPE") {
    g_outputType = Slope;
  }
  else if (ui.GetString("OUTPUT") == "PERCENTSLOPE") {
    g_outputType = PercentSlope;
  }

  // Get the units for ASPECT or SLOPE output type
  g_units = Degrees;
  if (g_outputType != PercentSlope) {
    if (ui.GetString("UNITS") == "RADIANS") {
      g_units = Radians;
    }
  }

  // Create output cube
  Cube *ocube = p.SetOutputCube("TO");

  PvlObject &lblCubeObj = ocube->label()->findObject("IsisCube");
  lblCubeObj.addGroup( PvlGroup("BandBin") );
  PvlGroup &bbGroup = lblCubeObj.findGroup("BandBin");
  
  g_groundMap = 0;
  if(g_outputType == Aspect) {
    p.StartProcess(createAspectCube);
    bbGroup.addKeyword( PvlKeyword( "Name", "Aspect", ui.GetString("UNITS").toLower().toStdString() ) );
  }
  else {
    if (ui.GetString("PIXRES") == "AUTOMATIC") {
      g_upperLeft = g_lowerLeft = SurfacePoint();
      g_groundMap = new UniversalGroundMap(*icube);
      p.StartProcess(createSlpCubeAutomatic);
    }
    else {
      if (ui.GetString("PIXRES") == "FILE") {
        g_groundMap = new UniversalGroundMap(*icube);
        g_resolution = g_groundMap->Resolution();
      }
      else {
        g_resolution = ui.GetDouble("RESOLUTION");
      }
      g_conversionFactor = ui.GetDouble("CONVERSION");
      p.StartProcess(createSlpCube);
    }

    if (g_outputType == PercentSlope) {
      bbGroup.addKeyword( PvlKeyword("Name", "Slope", "percent") );
    }
    else {
      bbGroup.addKeyword( PvlKeyword( "Name", "Slope", ui.GetString("UNITS").toLower().toStdString() ) );
    }
  }

  // Cleanup
  delete g_groundMap;
  g_groundMap = 0;

  p.EndProcess();
}


/**
 * http://webhelp.esri.com/arcgisdesktop/9.3/index.cfm?TopicName=How%20Slope%20(3D%20Analyst)%20works
 *
 *  "Conceptually, the Slope function fits a plane to the z-values of a 3 x 3
 *  cell neighborhood around the processing or center cell. The slope value of
 *  this plane is calculated using the average maximum technique (see
 *  References). The direction the plane faces is the aspect for the processing
 *  cell. The lower the slope value, the flatter the terrain; the higher the
 *  slope value, the steeper the terrain."
 *
 * @param in Input cube data (3x3 matrix) represented as a one 
 *           deminsional array.
 * @param v Output value
 */
void createSlpCubeAutomatic(Buffer &in, double &v) {
  // Can't do anything if the center pixel is bad
  if (in[4] == Isis::Null) {
    g_upperLeft = g_lowerLeft = SurfacePoint();
    v = Isis::Null;
    return;
  }
  
  try {
    Distance(in[4], Distance::Meters);
  }
  catch (IException &e) {
    QString msg = QString("The input cube contains a negative DN at (sample,line,band) "
        "[(%1,%2,%3)]. The automatic pixel resolution option requires the input cube contain "
        "raduis values. It is possible the input cube contains elevation or other data.").
        arg( in.Sample(4) ).arg( in.Line(4) ).arg( in.Band(4) );
    throw IException(e, IException::User, msg, _FILEINFO_);
  }

  // Get the lat/lons of the four corners of the pixel
  if ( !g_upperLeft.Valid() ) {
    if ( g_groundMap->SetImage(in.Sample(4) - 0.5, in.Line(4) - 0.5) ) {
      g_upperLeft.SetSphericalCoordinates(
                     Latitude(g_groundMap->UniversalLatitude(), Angle::Degrees),
                     Longitude(g_groundMap->UniversalLongitude(), Angle::Degrees),
                     Distance(in[4],Distance::Meters));
    }  
  }

  if ( !g_lowerLeft.Valid() ) {
    if ( g_groundMap->SetImage(in.Sample(4) - 0.5, in.Line(4) + 0.5) ) {
      g_lowerLeft.SetSphericalCoordinates(
                     Latitude(g_groundMap->UniversalLatitude(), Angle::Degrees),
                     Longitude(g_groundMap->UniversalLongitude(), Angle::Degrees),
                     Distance(in[4],Distance::Meters));
    }  
  }

  SurfacePoint upperRight;
  if ( g_groundMap->SetImage(in.Sample(4) + 0.5, in.Line(4) - 0.5) ) {
    upperRight.SetSphericalCoordinates(
                   Latitude(g_groundMap->UniversalLatitude(), Angle::Degrees),
                   Longitude(g_groundMap->UniversalLongitude(), Angle::Degrees),
                   Distance(in[4],Distance::Meters));
  }  

  SurfacePoint lowerRight;
  if ( g_groundMap->SetImage(in.Sample(4) + 0.5, in.Line(4) + 0.5) ) {
    lowerRight.SetSphericalCoordinates(
                   Latitude(g_groundMap->UniversalLatitude(), Angle::Degrees),
                   Longitude(g_groundMap->UniversalLongitude(), Angle::Degrees),
                   Distance(in[4],Distance::Meters) );
  }  

  // Are all four corners good?
  if ( !g_upperLeft.Valid() || !g_lowerLeft.Valid() ||
       !upperRight.Valid() || !lowerRight.Valid() ) {
    g_upperLeft = upperRight;
    g_lowerLeft = lowerRight;
    v = Isis::Null;
    return;
  }

  // Have four good corners so compute the resolutions
  // Do not apply the conversion factor to the resolutions because the projection/camera
  // has already been used and the Z value (DN) was assumed to be meters.
  double xResolution = ( ( g_upperLeft.GetDistanceToPoint(upperRight) ).meters() +
                         ( g_lowerLeft.GetDistanceToPoint(lowerRight) ).meters() ) / 2.0;
  double yResolution = ( ( g_upperLeft.GetDistanceToPoint(g_lowerLeft) ).meters() +
                         ( upperRight.GetDistanceToPoint(lowerRight) ).meters() ) / 2.0;

  // Pull height values out of 3x3 buffer (in)
  const double &a = in[0];
  const double &b = in[1];
  const double &c = in[2];
  const double &d = in[3];
  // The middle pixel isn't needed: const double &e = in[4];
  const double &f = in[5];
  const double &g = in[6];
  const double &h = in[7];
  const double &i = in[8];

  // If anything we're actually calculating with is special, fail
  // NOTE: When the 3x3 kernel wraps from the right edge of one line to the left edge of the next
  // line this test will fail due to the 3x3 having NULL pixels from sample zero (Outside the cube
  // boundries)
  if ( Isis::IsSpecial(a) ||
       Isis::IsSpecial(b) ||
       Isis::IsSpecial(c) ||
       Isis::IsSpecial(f) ||
       Isis::IsSpecial(g) ||
       Isis::IsSpecial(h) ||
       Isis::IsSpecial(i) ) {
    g_upperLeft = upperRight;
    g_lowerLeft = lowerRight;

    v = Isis::Null;
    return;
  }

  // [dz/dx] = ((c + 2f + i) - (a + 2d + g)) / (8 * x_cell_size)
  double changeInX = ( (c + 2 * f + i) - (a + 2 * d + g) ) / (8 * xResolution);

  // [dz/dy] = ((g + 2h + i) - (a + 2b + c)) / (8 * y_cell_size)
  double changeInY = ( (g + 2 * h + i) - (a + 2 * b + c) ) / (8 * yResolution);

  double changeMag = sqrt(changeInX * changeInX + changeInY * changeInY);

  double slopeRadians = atan(changeMag);

  if (g_outputType == PercentSlope) {
    v = 100.0 * slopeRadians / (Isis::PI / 2.0);
  }
  else if (g_units == Degrees) {
    v = slopeRadians * 180.0 / Isis::PI;
  }
  else {
    v = slopeRadians;
  }

  g_upperLeft = upperRight;
  g_lowerLeft = lowerRight;
}


/**
 * http://webhelp.esri.com/arcgisdesktop/9.3/index.cfm?TopicName=How%20Slope%20(3D%20Analyst)%20works
 *
 *  "Conceptually, the Slope function fits a plane to the z-values of a 3 x 3
 *  cell neighborhood around the processing or center cell. The slope value of
 *  this plane is calculated using the average maximum technique (see
 *  References). The direction the plane faces is the aspect for the processing
 *  cell. The lower the slope value, the flatter the terrain; the higher the
 *  slope value, the steeper the terrain."
 *
 * @param in Input cube data (3x3 matrix)
 * @param v Output value
 */
void createSlpCube(Buffer &in, double &v) {
  // Can't do anything if the center pixel is bad
  if (in[4] == Isis::Null) {
    v = Isis::Null;
    return;
  }

  // Provide what the user defined
  double xResolution = g_resolution;
  double yResolution = g_resolution;

  // Convert the spatial units to the height units
  xResolution /= g_conversionFactor;
  yResolution /= g_conversionFactor;

  // Pull height values out of 3x3 buffer (in)
  const double &a = in[0];
  const double &b = in[1];
  const double &c = in[2];
  const double &d = in[3];
  //const double &e = in[4];
  const double &f = in[5];
  const double &g = in[6];
  const double &h = in[7];
  const double &i = in[8];

  // If anything we're actually calculating with is special, fail
  if ( Isis::IsSpecial(a) ||
       Isis::IsSpecial(b) ||
       Isis::IsSpecial(c) ||
       Isis::IsSpecial(f) ||
       Isis::IsSpecial(g) ||
       Isis::IsSpecial(h) ||
       Isis::IsSpecial(i) ) {
    v = Isis::Null;
    return;
  }

  // [dz/dx] = ((c + 2f + i) - (a + 2d + g)) / (8 * x_cell_size)
  double changeInX = ( (c + 2 * f + i) - (a + 2 * d + g) ) / (8 * xResolution);

  // [dz/dy] = ((g + 2h + i) - (a + 2b + c)) / (8 * y_cell_size)
  double changeInY = ( (g + 2 * h + i) - (a + 2 * b + c) ) / (8 * yResolution);

  double changeMag = sqrt(changeInX * changeInX + changeInY * changeInY);

  double slopeRadians = atan(changeMag);

  if (g_outputType == PercentSlope) {
    v = 100.0 * slopeRadians / (Isis::PI / 2.0);
  }
  else if (g_units == Degrees) {
    v = slopeRadians * 180.0 / Isis::PI;
  }
  else {
    v = slopeRadians;
  }
}



/**
 * http://webhelp.esri.com/arcgisdesktop/9.3/index.cfm?TopicName=How%20Aspect%20(3D%20Analyst)%20works
 *
 *  "Conceptually, the Aspect function fits a plane to the z-values of a 3 x 3
 *  cell neighborhood around the processing or center cell. The direction the
 *  plane faces is the aspect for the processing cell."
 *
 * @param in Input cube data (3x3 matrix)
 * @param v Output value
 */
void createAspectCube(Buffer &in, double &v) {
  const double &a = in[0];
  const double &b = in[1];
  const double &c = in[2];
  const double &d = in[3];
  //const double &e = in[4];
  const double &f = in[5];
  const double &g = in[6];
  const double &h = in[7];
  const double &i = in[8];

  // If anything we're actually calculating with is special, fail
  if ( Isis::IsSpecial(a) ||
       Isis::IsSpecial(b) ||
       Isis::IsSpecial(c) ||
       Isis::IsSpecial(f) ||
       Isis::IsSpecial(g) ||
       Isis::IsSpecial(h) ||
       Isis::IsSpecial(i) ) {
    v = Isis::Null;
    return;
  }

  // [dz/dx] = ((c + 2f + i) - (a + 2d + g)) / 8
  double changeInX = ( (c + 2 * f + i) - (a + 2 * d + g) ) / 8;

  // [dz/dy] = ((g + 2h + i) - (a + 2b + c)) / 8
  double changeInY = ( (g + 2 * h + i) - (a + 2 * b + c) ) / 8;

  // aspect = atan2 ([dz/dy], -[dz/dx])
  double aspectRadians = atan2(changeInY, -changeInX);

  // aspect needs to be converted to is 0 north and positive clockwise and 0 to 360
  aspectRadians = Isis::PI / 2.0 - aspectRadians;
  if (aspectRadians < 0.0) aspectRadians += 2 * Isis::PI;

  // Output in degrees if selected
  if (g_units == Degrees) {
    v = aspectRadians * 180.0 / Isis::PI;
  }
  else {
    v = aspectRadians;
  }
}
