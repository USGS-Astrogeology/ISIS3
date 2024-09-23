#define GUIHELPERS

#include "Isis.h"

#include <QString>
#include <cmath>

#include "Brick.h"
#include "FileName.h"
#include "IException.h"
#include "IString.h"
#include "Process.h"
#include "RingPlaneProjection.h"
#include "ProjectionFactory.h"
#include "SpecialPixel.h"

using namespace std;
using namespace Isis;

void PrintMap();

map <QString, void *> GuiHelpers() {
  map <QString, void *> helper;
  helper ["PrintMap"] = (void *) PrintMap;
  return helper;
}

void IsisMain() {
  // Use a regular Process
  Process p;

  // Open the input cube and initialize the projection
  Cube *icube = p.SetInputCube("FROM");
  RingPlaneProjection *proj = (RingPlaneProjection *) icube->projection();

  // Get the coordinate
  UserInterface &ui = Application::GetUserInterface();

  // Get the sample/line position if we have an image point
  if(ui.GetString("TYPE") == "IMAGE") {
    double samp = ui.GetDouble("SAMPLE");
    double line = ui.GetDouble("LINE");
    proj->SetWorld(samp, line);
  }

  // Get the lat/lon position if we have a ground point
  else if(ui.GetString("TYPE") == "GROUND") {
    double rad = ui.GetDouble("RINGRADIUS");
    double ringLongitude = ui.GetDouble("RINGLONGITUDE");

    IString coordsys = ui.GetString("COORDSYS").toStdString();
    coordsys.UpCase();

    // All of these ifs will finish by setting the ground in the projection,
    // there are 4 options, Universal, InputFileSystem, Mapfile, and
    // Userdefined.

    // Counterclockwise, 0-360
    if(coordsys == "UNIVERSAL") {
      proj->SetUniversalGround(rad, ringLongitude);
    }

    // Use the coordinate system of the input file
    else if(coordsys == "INPUTFILESYS") {
      proj->SetGround(rad, ringLongitude);
    }

    // Use the mapping group from a given file
    else if(coordsys == "MAP") {
      FileName mapFile = ui.GetFileName("MAP").toStdString();

      // Does it exist?
      if(!mapFile.fileExists()) {
        std::string msg = "Filename [" + ui.GetFileName("MAP").toStdString() + "] does not exist";
        throw IException(IException::User, msg, _FILEINFO_);
      }

      // Load it up into a new projection
      Pvl mapPvl;
      mapPvl.read(mapFile.expanded());
      RingPlaneProjection *altmap = (RingPlaneProjection *) ProjectionFactory::CreateFromCube(mapPvl);

      // Set lat and lon in its system
      altmap->SetGround(rad, ringLongitude);

      // Set universal in input cube from universal from given
      // mapping/projection
      proj->SetUniversalGround(altmap->UniversalRingRadius(), altmap->UniversalRingLongitude());

      // I think this is right, no cube owns it, and the factory doesn't.
      delete altmap;
      altmap = NULL;
    }

    // User defined coordinate system, 7 possible combinations
    // we only have to make changes for some of them.
    // Convert from given system to universal and then set.
    else { // if(coordsys == "USERDEFINED") {
      double rad2 = rad;
      double ringLongitude2 = ringLongitude;

      if(ui.GetString("RINGLONDOM") == "180") {
        ringLongitude2 = proj->To360Domain(ringLongitude);
      }

      if(ui.GetString("RINGLONDIR") == "CLOCKWISE") {
        // Use ringLongitude2, we know its already in 0-360
        ringLongitude2 = proj->ToCounterClockwise(ringLongitude2, 360);
      }

      proj->SetUniversalGround(rad2, ringLongitude2);
    }
  }

  // Get the x/y position if we have a projection point
  else {
    double x = ui.GetDouble("X");
    double y = ui.GetDouble("Y");
    proj->SetCoordinate(x, y);
  }

  // Create Brick on samp, line to get the dn value of the pixel
  Brick b(1, 1, 1, icube->pixelType());
  int intSamp = (int)(proj->WorldX() + 0.5);
  int intLine = (int)(proj->WorldY() + 0.5);
  b.SetBasePosition(intSamp, intLine, 1);
  icube->read(b);

  // Log the position
  if(proj->IsGood()) {
    PvlGroup results("Results");
    results += PvlKeyword("Filename",
                          FileName(ui.GetCubeName("FROM").toStdString()).expanded());
    results += PvlKeyword("Sample", Isis::toString(proj->WorldX()));
    results += PvlKeyword("Line", Isis::toString(proj->WorldY()));
    results += PvlKeyword("PixelValue", PixelToString(b[0]));
    results += PvlKeyword("X", Isis::toString(proj->XCoord()));
    results += PvlKeyword("Y", Isis::toString(proj->YCoord()));

    // Put together all the keywords for different coordinate systems.
    PvlKeyword ringRad =
      PvlKeyword("RingRadius", Isis::toString(proj->UniversalRingRadius()));

    PvlKeyword cC360 =
      PvlKeyword("CounterClockwise360RingLongitude", Isis::toString(proj->UniversalRingLongitude()));

    PvlKeyword c360 =
      PvlKeyword("Clockwise360RingLongitude",
                 Isis::toString(proj->ToClockwise(proj->UniversalRingLongitude(), 360)));

    PvlKeyword cC180 =
      PvlKeyword("CounterClockwise180RingLongitude",
                 Isis::toString(proj->To180Domain(proj->UniversalRingLongitude())));

    PvlKeyword c180 =
      PvlKeyword("Clockwise180RingLongitude",
                 Isis::toString(proj->To180Domain(proj->ToCounterClockwise(
                            proj->UniversalRingLongitude(), 360))));

    // Input map coordinate system location
    if(proj->IsCounterClockwise()) {
      if(proj->Has360Domain()) {
        cC360.addComment("Input map direction/domain for ring longitude.");       
      }
      else {
        cC180.addComment("Input map direction/domain for ring longitude.");
      }
    }
    else {
      if(proj->Has360Domain()) {
        c360.addComment("Input map direction/domain for ring longitude.");
      }
      else {
        c180.addComment("Input map direction/domain for ring longitude.");
      }
    }

    // Radius
    results += ringRad;

    // RingLongitude
    results += cC360;
    results += c360;
    results += cC180;
    results += c180;

    Application::Log(results);

    // Write an output label file if necessary
    if(ui.WasEntered("TO")) {
      // Get user params from ui
      QString outFile = QString::fromStdString(FileName(ui.GetFileName("TO").toStdString()).expanded());
      bool exists = FileName(outFile.toStdString()).fileExists();
      bool append = ui.GetBoolean("APPEND");

      // Write the pvl group out to the file
      if(ui.GetString("FORMAT") == "PVL") {
        Pvl temp;
        temp.addGroup(results);
        if(append) {
          temp.append(outFile.toStdString());
        }
        else {
          temp.write(outFile.toStdString());
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

        if(writeHeader) {
          for(int i = 0; i < results.keywords(); i++) {
            os << results[i].name();

            if(i < results.keywords() - 1) {
              os << ",";
            }
          }
          os << endl;
        }

        for(int i = 0; i < results.keywords(); i++) {
          os << results[i];

          if(i < results.keywords() - 1) {
            os << ",";
          }
        }
        os << endl;
      }
    }
    else if(ui.GetString("FORMAT") == "FLAT") {
      std::string msg = "Flat file must have a name.";
      throw IException(IException::User, msg, _FILEINFO_);
    }
  }
  else {
    std::string msg = "Could not project requested position";
    throw IException(IException::Unknown, msg, _FILEINFO_);
  }
}

// Helper function to print out mapfile to session log
void PrintMap() {
  UserInterface &ui = Application::GetUserInterface();

  // Get mapping group from map file
  Pvl userMap;
  userMap.read(ui.GetFileName("MAP").toStdString());
  PvlGroup &userGrp = userMap.findGroup("Mapping", Pvl::Traverse);

  //Write map file out to the log
  Isis::Application::GuiLog(userGrp);
}
