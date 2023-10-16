#include <QString>
#include <cmath>

#include "Application.h"
#include "Brick.h"
#include "FileName.h"
#include "IException.h"
#include "IString.h"
#include "Process.h"
#include "TProjection.h"
#include "ProjectionFactory.h"
#include "SpecialPixel.h"
#include "CSVReader.h"

#include "mappt.h"

using namespace std;


namespace Isis {

QList< QPair<double, double> > getMapPoints(const UserInterface &ui, bool usePointList);
PvlGroup getProjPointInfo(Cube *icube, QPair<double, double> point, UserInterface &ui, Pvl *log);
  
void mappt(UserInterface &ui, Pvl *log) {
  Cube *cube = new Cube();
  CubeAttributeInput inAtt = ui.GetInputAttribute("FROM");

  if (inAtt.bands().size() != 0) {
    cube->setVirtualBands(inAtt.bands());
  }
  
  cube->open(ui.GetCubeName("FROM"), "r");
  mappt(cube, ui, log, &inAtt);
}

void mappt(Cube *icube, UserInterface &ui, Pvl *log, CubeAttributeInput* inAtt) {
  // Open the input cube and initialize the projection
  
  QList<QPair<double, double>> points = getMapPoints(ui, ui.WasEntered("COORDLIST"));
   
  if(log) {
    for(int i = 0; i < points.size(); i++) {
      PvlGroup g = getProjPointInfo(icube, points[i], ui, log);
      Application::AppendAndLog(g, log);
    } 
  }

  // Write an output label file if necessary
  if(ui.WasEntered("TO")) {
    // Get user params from ui
    QString outFile = FileName(ui.GetFileName("TO")).expanded();
    bool exists = FileName(outFile).fileExists();
    bool append = ui.GetBoolean("APPEND");

    // Write the pvl group out to the file
    if(ui.GetString("FORMAT") == "PVL") {
      if(append) {
        log->append(outFile);
      }
      else {
        log->write(outFile);
      }
    }

    // Create a flatfile of the same data
    // The flatfile is comma delimited and can be imported into Excel
    else {
      ofstream os;
      bool writeHeader = false;
      if(append) {
        os.open(outFile.toLatin1().data(), ios::app);
        if(!exists) {
          writeHeader = true;
        }
      }
      else {
        os.open(outFile.toLatin1().data(), ios::out);
        writeHeader = true;
      }
      
      PvlGroup fResult = log->group(0); 

      if(writeHeader) {
        for(int i = 0; i < fResult.keywords(); i++) {
          os << fResult[i].name();

          if(i < fResult.keywords() - 1) {
            os << ",";
          }
        }
        os << endl;
      }
      
      for(int i = 0; i < log->groups(); i++) {
        PvlGroup group = log->group(i);
        for(int j = 0; j < group.keywords(); j++) {
          os << (QString)group[j];
          if(j < group.keywords() - 1) {
            os << ",";
          }
        }
        os << endl;    
      } // end of keyword loop
    } // end of group loop 
  
  }
  else if(ui.GetString("FORMAT") == "FLAT") {
    std::string msg = "Flat file must have a name.";
    throw IException(IException::User, msg, _FILEINFO_);
  }
}


PvlGroup getProjPointInfo(Cube *icube, QPair<double, double> point, UserInterface &ui, Pvl *log) { 
  // Get the coordinate
  bool outsideAllowed = ui.GetBoolean("ALLOWOUTSIDE");
  int cubeLineLimit = icube->lineCount() + .5;
  int cubeSampleLimit = icube->sampleCount() + .5;

  TProjection *proj = (TProjection *) icube->projection();
  // Get the sample/line position if we have an image point
  if(ui.GetString("TYPE") == "IMAGE") {
    double samp = point.first;
    double line = point.second;

    if (!outsideAllowed) {
      if (samp < .5 || line < .5 || samp > cubeSampleLimit || line > cubeLineLimit) {
        QString error = "Requested line,sample is not on the image";
        throw IException(IException::Unknown, error, _FILEINFO_);
      }
    }
    proj->SetWorld(samp, line);
  }

  // Get the lat/lon position if we have a ground point
  else if(ui.GetString("TYPE") == "GROUND") {
    double lat = point.first;
    double lon = point.second;

    // Make sure we have a valid latitude value
    if(fabs(lat) > 90.0) {
      QString msg = "Invalid value for LATITUDE ["
                   + toString(lat) + "] outside range of ";
      msg += "[-90,90]";
      throw IException(IException::User, msg, _FILEINFO_);
    }

    IString coordsys = ui.GetString("COORDSYS");
    coordsys.UpCase();

    // All of these ifs will finish by setting the ground in the projection,
    // there are 4 options, Universal, InputFileSystem, Mapfile, and
    // Userdefined.

    // Positive East, 0-360, Planetocentric
    if(coordsys == "UNIVERSAL") {
      proj->SetUniversalGround(lat, lon);
    }

    // Use the coordinate system of the input file
    else if(coordsys == "INPUTFILESYS") {
      proj->SetGround(lat, lon);
    }

    // Use the mapping group from a given file
    else if(coordsys == "MAP") {
      FileName mapFile = ui.GetFileName("MAP");

      // Does it exist?
      if(!mapFile.fileExists()) {
        QString msg = "Filename [" + ui.GetFileName("MAP") + "] does not exist";
        throw IException(IException::User, msg, _FILEINFO_);
      }

      // Load it up into a new projection
      Pvl mapPvl;
      mapPvl.read(mapFile.expanded());
      TProjection *altmap = (TProjection *) ProjectionFactory::CreateFromCube(mapPvl);

      // Set lat and lon in its system
      altmap->SetGround(lat, lon);

      // Set universal in input cube from universal from given
      // mapping/projection
      proj->SetUniversalGround(
        altmap->UniversalLatitude(), altmap->UniversalLongitude());

      // I think this is right, no cube owns it, and the factory doesn't.
      delete altmap;
      altmap = NULL;
    }

    // User defined coordinate system, 8 possible combinations
    // we only have to make changes for some of them.
    // Convert from given system to universal and then set.
    else if(coordsys == "USERDEFINED") {
      double lat2 = lat;
      double lon2 = lon;

      if(ui.GetString("LATTYPE") == "PLANETOGRAPHIC") {
        lat2 = proj->ToPlanetocentric(lat);
      }

      if(ui.GetString("LONDOM") == "180") {
        lon2 = proj->To360Domain(lon);
      }

      if(ui.GetString("LONDIR") == "POSITIVEWEST") {
        // Use lon2, we know its already in 0-360
        lon2 = proj->ToPositiveEast(lon2, 360);
      }

      proj->SetUniversalGround(lat2, lon2);
    }
  }

  // Get the x/y position if we have a projection point
  else {
    double x = point.first;
    double y = point.second;
    proj->SetCoordinate(x, y);
  }

  PvlGroup results("Results");
  if (proj->WorldX() < .5 || proj->WorldY() < .5 || proj->WorldX() > cubeSampleLimit ||
      proj->WorldY() > cubeLineLimit) {
    if (!outsideAllowed) {
      QString error = "Resulting line,sample is not on the image";
      throw IException(IException::Unknown, error, _FILEINFO_);
    }
    else {
      results += PvlKeyword("OutsideOfImage", "Requested point falls outside of image boundaries");
    }
  }

  // Create Brick on samp, line to get the dn value of the pixel
  Brick b(1, 1, 1, icube->pixelType());
  int intSamp = (int)(proj->WorldX() + 0.5);
  int intLine = (int)(proj->WorldY() + 0.5);
  
  b.SetBasePosition(intSamp, intLine, 1);
  icube->read(b);
  
  QString filterName = "Null";

  if ( icube->label()->findObject("IsisCube").hasGroup("BandBin")) {
    PvlGroup bandBin = icube->label()->findObject("IsisCube").findGroup("BandBin");
    if (bandBin.hasKeyword("FilterName")) {
        filterName = bandBin.findKeyword("FilterName")[0];
    }
  }

  // Log the position
  if(proj->IsGood()) {
    results += PvlKeyword("Filename",
                          FileName(icube->fileName()).expanded());
    results += PvlKeyword("Sample", toString(proj->WorldX()));
    results += PvlKeyword("Line", toString(proj->WorldY()));
    results += PvlKeyword("Band", toString(icube->physicalBand(1)));
    results += PvlKeyword("FilterName", filterName);
    results += PvlKeyword("PixelValue", PixelToString(b[0]));
    results += PvlKeyword("X", toString(proj->XCoord()));
    results += PvlKeyword("Y", toString(proj->YCoord()));

    // Put together all the keywords for different coordinate systems.
    PvlKeyword centLat =
      PvlKeyword("PlanetocentricLatitude", toString(proj->UniversalLatitude()));

    PvlKeyword graphLat =
      PvlKeyword("PlanetographicLatitude",
                 toString(proj->ToPlanetographic(proj->UniversalLatitude())));

    PvlKeyword pE360 =
      PvlKeyword("PositiveEast360Longitude", toString(proj->UniversalLongitude()));

    PvlKeyword pW360 =
      PvlKeyword("PositiveWest360Longitude",
                 toString(proj->ToPositiveWest(proj->UniversalLongitude(), 360)));

    PvlKeyword pE180 =
      PvlKeyword("PositiveEast180Longitude",
                 toString(proj->To180Domain(proj->UniversalLongitude())));

    PvlKeyword pW180 =
      PvlKeyword("PositiveWest180Longitude",
                 toString(proj->To180Domain(proj->ToPositiveEast(
                            proj->UniversalLongitude(), 360))));


    // Input map coordinate system location
    // Latitude
    if(proj->IsPlanetocentric()) {
      centLat.addComment("Input map coordinate system");
      results += centLat;
    }
    else {
      graphLat.addComment("Input map coordinate system");
      results += graphLat;
    }

    // Longitude
    if(proj->IsPositiveEast()) {
      if(proj->Has360Domain()) {
        results += pE360;
      }
      else {
        results += pE180;
      }
    }
    else {
      if(proj->Has360Domain()) {
        results += pW360;
      }
      else {
        results += pW180;
      }
    }

    // Non input corrdinate system locations
    // Latitude
    if(proj->IsPlanetocentric()) {
      graphLat.addComment("Location in other coordinate systems");
      results += graphLat;
    }
    else {
      centLat.addComment("Location in other coordinate systems");
      results += centLat;
    }

    // Longitude
    if(proj->IsPositiveEast()) {
      if(proj->Has360Domain()) {
        results += pW360;
        results += pE180;
        results += pW180;
      }
      else {
        results += pE360;
        results += pW360;
        results += pW180;
      }
    }
    else {
      if(proj->Has360Domain()) {
        results += pE360;
        results += pE180;
        results += pW180;
      }
      else {
        results += pE360;
        results += pE180;
        results += pW360;
      }
    }
    
    if (ui.GetString("FORMAT") == "FLAT") {
      // Rearrange the order of the lat/lons for the csv
      results.deleteKeyword( pE360.name() );
      results.deleteKeyword( pE180.name() );
      results.deleteKeyword( pW360.name() );
      results.deleteKeyword( pW180.name() );
      results.deleteKeyword( centLat.name() );
      results.deleteKeyword( graphLat.name() );
      //Correct order.
      results += centLat;
      results += graphLat;
      results += pE360;
      results += pE180;
      results += pW360;
      results += pW180;
    }
  }

  return results; 
}


QList< QPair<double, double> > getMapPoints(const UserInterface &ui, bool usePointList) {
    double point1 = 0.0;
    double point2 = 0.0;
    QList< QPair<double, double> > points;
    QString pointType = ui.GetString("TYPE");

    // Check if the provided coordinate list is valid, i.e. a Samp/Line or Lat/Long coordinate per row
    if (usePointList) {

      CSVReader reader;
      reader.read(FileName(ui.GetFileName("COORDLIST")).expanded());

      if (!reader.isTableValid(reader.getTable()) || reader.columns() != 2) {
        std::string msg = "Coordinate file formatted incorrectly.\n"
                      "Each row must have two columns: a sample,line or a latitude,longitude pair.";
        throw IException(IException::User, msg, _FILEINFO_);
      }
      for (int row = 0; row < reader.rows(); row++) {
        point1 = toDouble(reader.getRow(row)[0]);
        point2 = toDouble(reader.getRow(row)[1]);
        points.append(QPair<double, double>(point1, point2));
      }
    }
    // Grab the coordinate from the ui position parameters if no coordinate list is provided
    else {
      if (pointType == "IMAGE") {
        if (ui.WasEntered("SAMPLE"))
          point1 = ui.GetDouble("SAMPLE");
        if (ui.WasEntered("LINE"))
          point2 = ui.GetDouble("LINE");
      }
      else if (pointType == "GROUND") {
        point1 = ui.GetDouble("LATITUDE");
        point2 = ui.GetDouble("LONGITUDE");
      }
      else {
        // Projection type selected
        point1 = ui.GetDouble("X");
        point2 = ui.GetDouble("Y");
      }
      points.append(QPair<double, double>(point1, point2));
    }
    
    return points;
}
}
