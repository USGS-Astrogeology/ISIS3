/** This is free and unencumbered software released into the public domain.

The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */

#include "skypt.h"

#include <string>
#include <iomanip>

#include "Brick.h"
#include "Camera.h"
#include "IException.h"
#include "iTime.h"
#include "PvlGroup.h"

using namespace std;
using namespace Isis;

namespace Isis{

  void skypt(UserInterface &ui, Pvl *log) {
    Cube *cube = new Cube(ui.GetCubeName("FROM"));
    skypt(cube, ui, log);
  }

  void skypt(Cube *cube, UserInterface &ui, Pvl *log){
    // Get camera model for the input cube.
    QString channel = ui.GetCubeName("FROM");
    Camera *cam = cube->camera();

    // Get the type of conversion that we are doing
    QString type = ui.GetString("TYPE");
    double samp, line;

    // Do conversion from samp/line to ra/dec
    if (type == "IMAGE") {
      // Get users sample & line values and do a setImage for the camera
      samp = ui.GetDouble("SAMPLE");
      line = ui.GetDouble("LINE");
      cam->SetImage(samp, line);
    }
  
    // Do conversion from ra/dec to samp/line
    else {
      double ra = ui.GetDouble("RA");
      double dec = ui.GetDouble("DEC");
      if (!cam->SetRightAscensionDeclination(ra, dec)) {
        QString msg = "Invalid Ra/Dec coordinate";
        throw IException(IException::User, msg, _FILEINFO_);
      }
      samp = cam->Sample();
      line = cam->Line();
    }

    // Create Brick on samp, line to get the dn value of the pixel
    Brick b(3, 3, 1, cube->pixelType());
    int intSamp = (int)(samp + 0.5);
    int intLine = (int)(line + 0.5);
    b.SetBasePosition(intSamp, intLine, 1);
    cube->read(b);

    double rot = cam->CelestialNorthClockAngle();

    // Create group with sky position
    PvlGroup sp("SkyPoint");
     {
        sp += PvlKeyword("Filename", FileName(channel).expanded());
        sp += PvlKeyword("Sample", toString(cam->Sample()));
        sp += PvlKeyword("Line", toString(cam->Line()));
        sp += PvlKeyword("RightAscension", toString(cam->RightAscension()));
        sp += PvlKeyword("Declination", toString(cam->Declination()));
        sp += PvlKeyword("EphemerisTime", toString(cam->time().Et()));
        sp += PvlKeyword("PixelValue", PixelToString(b[0]));
        sp += PvlKeyword("CelestialNorthClockAngle", toString(rot), "degrees");
     }

    //Write the group to the screen
    log->addLogGroup(sp);

    // Write an output label file if necessary
    if (ui.WasEntered("TO")) {
      // Get user params from ui
      QString outFile = FileName(ui.GetFileName("TO")).expanded();
      bool exists = FileName(outFile).fileExists();
      bool append = ui.GetBoolean("APPEND");

      // Write the pvl group out to the file
      if (ui.GetString("FORMAT") == "PVL") {
        Pvl temp;
        temp.setTerminator("");
        temp.addGroup(sp);
        if (append) {
          temp.append(ui.GetAsString("TO"));
        }
        else {
          temp.write(ui.GetAsString("TO"));
        }
      }
  
      // Create a flatfile of the same data
      // The flatfile is comma delimited and can be imported into Excel
      else {
        ofstream os;
        bool writeHeader = false;
        if (append) {
          os.open(outFile.toLatin1().data(), ios::app);
          if (!exists) {
            writeHeader = true;
          }
        }
        else {
          os.open(outFile.toLatin1().data(), ios::out);
          writeHeader = true;
        }

        if (writeHeader) {
          for(int i = 0; i < sp.keywords(); i++) {
            os << sp[i].name();
            if (i < sp.keywords() - 1) {
              os << ",";
            }
          }
          os << endl;
        }

        for(int i = 0; i < sp.keywords(); i++) {
          os << (QString)sp[i];
          if (i < sp.keywords() - 1) {
            os << ",";
          }
        }
        os << endl;
      }
    }
    else if (ui.GetString("FORMAT") == "FLAT") {
      QString msg = "Flat file must have a name.";
      throw IException(IException::User, msg, _FILEINFO_);
    }
  }

}







