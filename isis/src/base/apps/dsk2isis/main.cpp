#include "Isis.h"

#include "AbstractPlate.h"
#include "Brick.h"
#include "CSVReader.h"
#include "FileName.h"
#include "ImportPdsTable.h"
#include "IException.h"
#include "Intercept.h"
#include "Latitude.h"
#include "Longitude.h"
#include "NaifDskApi.h"
#include "NaifDskPlateModel.h"
#include "NaifStatus.h"
#include "PixelType.h"
#include "ProcessByLine.h"
#include "Progress.h"
#include "Projection.h"
#include "ProjectionFactory.h"
#include "SpecialPixel.h"
#include "SurfacePoint.h"
#include "Table.h"
#include "TileManager.h"
#include "TProjection.h"

using namespace std;
using namespace Isis;

void initialize(Buffer &out) {
  out = Null;
  return;
}

void IsisMain() {
 
  // We will be processing by line
  ProcessByLine p;
  UserInterface &ui = Application::GetUserInterface();

  QString dskfile = ui.GetFileName("FROM");
  NaifDskPlateModel dsk(dskfile);

  cout << "Total #Plates:   " << dsk.size() << "\n";
  cout << "Total #Vertices: " << dsk.numberVertices() << "\n";

  //Get the map projection file provided by the user
  Pvl userMap;
  userMap.read(ui.GetFileName("MAP").toStdString());

  // Create the ISIS map projection
  int ns, nl;
  Projection *proj = ProjectionFactory::CreateForCube(userMap, ns, nl, true);
  PvlGroup cleanMap = proj->Mapping();
  delete proj;

  // Create the cube from the projection parameters
  Cube *ocube = p.SetOutputCube("TO", ns, nl, 1);
  ocube->putGroup(cleanMap);
  TProjection *tproj = (TProjection *) ocube->projection();

  // Initialize the output cube
  p.StartProcess(initialize);

  QString method = ui.GetString("METHOD").toLower();
  bool useGridMethod = ( "grid" == method ) ? true : false;
 
  // Get the real tile sizes and allocate the buffer accordingly
  PvlObject &core = ocube->label()->findObject("IsisCube").findObject("Core");
  int tsamps = core["TileSamples"];
  int tlines = core["TileLines"];

  TileManager pixels(*ocube, tsamps, tlines);

  Progress mapper;
  mapper.SetText("mapping");
  mapper.SetMaximumSteps(pixels.Tiles());
  mapper.CheckStatus();

  pixels.SetTile(1);
  SurfacePoint point;

  //  Now process the table writing the output
  while ( !pixels.end() ) {
    for (int i = 0  ; i < pixels.size()  ; i++) {

      int samp(pixels.Sample(i));
      int line(pixels.Line(i));
      pixels[i] = Null;

      if ( tproj->SetWorld(samp, line) ) {
        double lat = tproj->UniversalLatitude();
        double lon = tproj->UniversalLongitude();
        double radius = tproj->LocalRadius(lat);
  
        point.SetSphericalCoordinates(Latitude(lat, Angle::Degrees), 
                                      Longitude(lon, Angle::Degrees),
                                      Distance(radius, Distance::Meters));

        // Calculate the grid point intersection of the DEM
        if ( useGridMethod ) {  // ( "grid" == method )
          // Get the grid point (lat, lon) from the DEM
          QScopedPointer<SurfacePoint> dempt(dsk.point(point.GetLatitude(), 
                                                       point.GetLongitude()));

          pixels[i] = dempt->GetLocalRadius().meters();
        }
        else { // ( "ray" == method )
          // Get the intercept of the position and look vector of point on the DEM 
          // by using the lat/lon coordinate in X/Y/Z.  This vector is scaled to
          // be outside the body by 1.5 times.  The look vector is then 
          // determined by negating the vector and an intersection is computed.
          // Grid method winds up doing the same thing - this is the direct
          // computation.
          NaifVertex observer(3);
          point.ToNaifArray(&observer[0]);

          NaifStatus::CheckErrors();
          vscl_c(1.5, &observer[0], &observer[0]);
          NaifStatus::CheckErrors();

          // Get look vector
          NaifVector raydir(3);
          vminus_c(&observer[0], &raydir[0]);
          NaifStatus::CheckErrors();
  
          // Check for valid intercept
          NaifVertex xpt;
          if ( dsk.isPlateIdValid(dsk.plateIdOfIntercept(observer, raydir, xpt)) ) { 
            point.FromNaifArray(&xpt[0]);
            pixels[i] = point.GetLocalRadius().meters();
          }
        }
      }
    }
    ocube->write(pixels);
    mapper.CheckStatus();
    pixels.next();
  }

  p.EndProcess();
}

