/** This is free and unencumbered software released into the public domain.

The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */

#include "Isis.h"
#include "PvlGroup.h"
#include "UserInterface.h"
#include "CameraDistortionMap.h"
#include "Cube.h"
#include "Progress.h"
#include "IException.h"
#include "ControlNet.h"
#include "SerialNumber.h"
#include "ControlMeasure.h"
#include "ControlMeasureLogData.h"
#include "ControlNet.h"
#include "ControlPoint.h"
#include "Brick.h"

using namespace std;
using namespace Isis;

struct Coordinate {
  double line;         //!< Old line
  double samp;         //!< Old sample
  double errLine;      //!< Coorected line
  double errSamp;      //!< Corrected sample
  double olddetX;      //!< Old detector sample coordinate
  double olddetY;      //!< Old detector line coordinate
  double newdetX;      //!< Correct detector sample coordinate
  double newdetY;      //!< Correct detector line coordinate
  double gof;          //!< Goodness of fit
  double latitude;     //!< Latitude of the point
  double longitude;    //!< Longitude of the point
};

inline double distance(const double &x1, const double &y1,
                       const double &x2, const double &y2) {
  return (sqrt((x2-x1)*(x2-x1) + (y2-y1)*(y2-y1)));
}


void IsisMain() {
  // Get user interface
  UserInterface &ui = Application::GetUserInterface();

  // Open the FROM cube. It must have a camera model associated with it
  Cube from;
  CubeAttributeInput &attFrom = ui.GetInputAttribute("FROM");
  vector<QString> bandFrom = attFrom.bands();
  from.setVirtualBands(bandFrom);
  from.open(ui.GetCubeName("FROM"), "r");
  Camera *fcamera = from.camera();
  CameraDistortionMap *dmap = fcamera->DistortionMap();


  // Initialize control point network
  ControlNet cn(ui.GetFileName("CNETFILE"));
  bool checkForNulls = ui.GetBoolean("CHECKDN");

  double dtol = ui.GetDouble("TOLERANCE");
  double pp = fcamera->PixelPitch();
  double mmTol = fabs(pp * dtol);

  Progress prog;
  prog.SetMaximumSteps(cn.GetNumPoints());
  prog.CheckStatus();

  // Loop through grid of points and get statistics to compute
  // translation values
  std::vector<Coordinate> coords;
  Brick pixel(from, 1, 1, 1);
  BigInt badPoint(0), nulls(0), oldNotInImage(0), newNotInImage(0), badTol(0);
  for(int p = 0; p < cn.GetNumPoints(); p++) {
    const ControlPoint & pnt = *cn[p];
    if((!pnt.IsIgnored()) && (pnt.GetNumMeasures() == 2)) {
      Coordinate c;
      if (pnt.IndexOfRefMeasure() == 0) {
        c.samp = pnt[0]->GetSample();
        c.line = pnt[0]->GetLine();
        c.errSamp = pnt[1]->GetSample();
        c.errLine = pnt[1]->GetLine();

        c.gof = -1;
        if(pnt[1]->HasLogData(ControlMeasureLogData::GoodnessOfFit)) {
          c.gof = pnt[1]->GetLogData(
              ControlMeasureLogData::GoodnessOfFit).GetNumericalValue();
        }
      }
      else {
        c.samp = pnt[1]->GetSample();
        c.line = pnt[1]->GetLine();
        c.errSamp = pnt[0]->GetSample();
        c.errLine = pnt[0]->GetLine();
        c.gof = -1;
        if(pnt[0]->HasLogData(ControlMeasureLogData::GoodnessOfFit)) {
          c.gof = pnt[0]->GetLogData(
              ControlMeasureLogData::GoodnessOfFit).GetNumericalValue();
        }
      }

      //  Check for valid point if requested
      bool isGood(true);
      if (checkForNulls) {
        pixel.SetBasePosition((int) c.samp, (int) c.line, 1);
        from.read(pixel);
        isGood = !IsSpecial(pixel[0]);
      }

      if (isGood) {
        if (fcamera->SetImage(c.samp, c.line)) {
          c.latitude = fcamera->UniversalLatitude();
          c.longitude = fcamera->UniversalLongitude();
          double o_ux = dmap->UndistortedFocalPlaneX();
          double o_uy = dmap->UndistortedFocalPlaneY();
          double o_dx = dmap->FocalPlaneX();
          double o_dy = dmap->FocalPlaneY();
          if (fcamera->SetImage(c.errSamp, c.errLine)) {
            double c_ux = dmap->UndistortedFocalPlaneX();
            double c_uy = dmap->UndistortedFocalPlaneY();
            double c_dx = dmap->FocalPlaneX();
            double c_dy = dmap->FocalPlaneY();

            double ddist = distance(o_dx, o_dy, c_dx, c_dy);
            double udist = distance(o_ux, o_uy, c_ux, c_uy);

            if ((ddist <= mmTol) && (udist <= mmTol)) {
              c.olddetX = o_dx;
              c.olddetY = o_dy;
              c.newdetX = c_ux;
              c.newdetY = c_uy;
              coords.push_back(c);
            }
            else badTol++;
          }
          else newNotInImage++;
        }
        else oldNotInImage++;
      }
      else nulls++;
    }
    else {
       badPoint++;
    }
    prog.CheckStatus();
  }

  PvlGroup results("Results");
  results += PvlKeyword("PixelPitch", std::to_string(pp), "millimeters");
  results += PvlKeyword("TotalPoints", std::to_string(cn.GetNumPoints()));
  results += PvlKeyword("ValidPoints", std::to_string((BigInt) coords.size()));
  results += PvlKeyword("InvalidPoints", std::to_string(badPoint));
  if (checkForNulls) results += PvlKeyword("NullDNs", std::to_string(nulls));
  results += PvlKeyword("OldPointNotInImage", std::to_string(oldNotInImage));
  results += PvlKeyword("NewPointNotInImage", std::to_string(newNotInImage));
  results += PvlKeyword("ToleranceExceeded", std::to_string(badTol));

  // Log it
  Application::Log(results);

  // Don't need the cubes opened anymore
  from.close();

  // If flatfile was entered, create the flatfile
  // The flatfile is comma seperated and can be imported into an excel
  // spreadsheet
  if(ui.WasEntered("TO")) {
    QString fFile = QString::fromStdString(FileName(ui.GetFileName("TO").toStdString()).expanded());
    ofstream os;
    os.open(fFile.toLatin1().data(), ios::out);
    os << "OldSample,OldLine,NewSample,NewLine," <<
       "X,Y,XC,YC,"<<
       "GoodnessOfFit,Latitude,Longitude" << endl;
    for(unsigned int i = 0; i < coords.size(); i++) {
      Coordinate &c = coords[i];
      os << c.samp << "," << c.line << "," << c.errSamp << "," << c.errLine << ","
         << c.olddetX << "," << c.olddetY << "," << c.newdetX << "," << c.newdetY << ","
         << c.gof << "," << c.latitude << "," << c.longitude << endl;

    }
  }

  return;
}
