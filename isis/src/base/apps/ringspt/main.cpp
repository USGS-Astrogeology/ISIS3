#include "Isis.h"

#include <string>
#include <iomanip>

#include "Brick.h"
#include "Camera.h"
#include "CameraRingsPointInfo.h"
#include "IException.h"
#include "iTime.h"
#include "Progress.h"
#include "SpecialPixel.h"

using namespace std;
using namespace Isis;

void IsisMain() {
  UserInterface &ui = Application::GetUserInterface();
  bool outsideAllowed = ui.GetBoolean("ALLOWOUTSIDE");

  // Set up CameraRingsPointInfo and set  file
  CameraRingsPointInfo ringspt;
  ringspt.SetCube(ui.GetCubeName("FROM"));

  Progress prog;
  prog.SetMaximumSteps(1);

  // Depending on what type is selected, set values accordingly
  PvlGroup *point = NULL;
  if (ui.GetString("TYPE") == "IMAGE") {
    double sample = 0.0;
    double line = 0.0;
    if (ui.WasEntered("SAMPLE") && ui.WasEntered("LINE")) {
      sample = ui.GetDouble("SAMPLE");
      line = ui.GetDouble("LINE");
      point = ringspt.SetImage(sample, line, outsideAllowed);
    }
    else {
      if (ui.WasEntered("SAMPLE")) {
        sample = ui.GetDouble("SAMPLE");
        point = ringspt.SetSample(sample, outsideAllowed);
      }
      else if (ui.WasEntered("LINE")) {
        line = ui.GetDouble("LINE");
        point = ringspt.SetLine(line, outsideAllowed);
      }
      else {
        point = ringspt.SetCenter(outsideAllowed);
      }
    }
  }
  else {
    double ringRadius = ui.GetDouble("RINGRADIUS");
    double ringLongitude = ui.GetDouble("RINGLONGITUDE");
    point = ringspt.SetGround(ringRadius, ringLongitude, outsideAllowed);
  }

  prog.CheckStatus();

  // Log it
  Application::Log((*point));

  if(ui.WasEntered("TO")) {
    // Get user params from ui
    std::string outFile = FileName(ui.GetFileName("TO").toStdString()).expanded();
    bool exists = FileName(outFile).fileExists();
    bool append = ui.GetBoolean("APPEND");

    // Write the pvl group out to the file
    if(ui.GetString("FORMAT") == "PVL") {
      Pvl temp;
      temp.setTerminator("");
      temp.addGroup((*point));
      if(append) {
        temp.append(outFile);
      }
      else {
        temp.write(outFile);
      }
    }

    // Create a flatfile from PVL data
    // The flatfile is comma delimited and can be imported into Excel
    else {
      ofstream os;
      bool writeHeader = false;
      if(append) {
        os.open(outFile.c_str(), ios::app);
        if(!exists) {
          writeHeader = true;
        }
      }
      else {
        os.open(outFile.c_str(), ios::out);
        writeHeader = true;
      }

      if(writeHeader) {
        for(int i = 0; i < (*point).keywords(); i++) {
          if((*point)[i].size() == 3) {
            os << (*point)[i].name() << "X,"
               << (*point)[i].name() << "Y,"
               << (*point)[i].name() << "Z";
          }
          else {
            os << (*point)[i].name();
          }

          if(i < point->keywords() - 1) {
            os << ",";
          }
        }
        os << endl;
      }

      for(int i = 0; i < (*point).keywords(); i++) {
        if((*point)[i].size() == 3) {
          os << (*point)[i][0] << ","
             << (*point)[i][1] << ","
             << (*point)[i][2];
        }
        else {
          os << (*point)[i];
        }

        if(i < (*point).keywords() - 1) {
          os << ",";
        }
      }
      os << endl;
    }
  }
  else {
    if(ui.GetString("FORMAT") == "FLAT") {
      std::string msg = "Flat file must have a name.";
      throw IException(IException::User, msg, _FILEINFO_);
    }
  }
  delete point;
  point = NULL;
  prog.CheckStatus();
}
