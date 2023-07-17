#include "campt.h"

#include <string>
#include <iomanip>

#include "Application.h"
#include "Brick.h"
#include "Camera.h"
#include "CameraPointInfo.h"
#include "CSVReader.h"
#include "Distance.h"
#include "IException.h"
#include "iTime.h"
#include "Longitude.h"
#include "Progress.h"
#include "PvlGroup.h"
#include "SpecialPixel.h"
#include "TProjection.h"

using namespace std;
using namespace Isis;

namespace Isis{

  QList< QPair<double, double> > getPoints(const UserInterface &ui, bool usePointList);
  QList<PvlGroup *> getCameraPointInfo(const UserInterface &ui,
                                      QList< QPair<double, double> > points,
                                      CameraPointInfo &campt);
  void writePoints(const UserInterface &ui, QList<PvlGroup*> camPoints, Pvl *log);

  void campt(UserInterface &ui, Pvl *log) {
    Cube *cube = new Cube(ui.GetCubeName("FROM"), "r");
    campt(cube, ui, log);
  }

  void campt(Cube *cube, UserInterface &ui, Pvl *log) {
    // Setup our input cube
    CameraPointInfo campt;

    QString fileFormat = ui.GetString("FORMAT");
    if(fileFormat=="PVL")
        campt.SetCSVOutput(false);
    else
        campt.SetCSVOutput(true);

    QString inputCubePath = "";
    try {
      inputCubePath = ui.GetCubeName("FROM");
    }
    catch (IException &e) {
      inputCubePath = cube->fileName();
    }
    campt.SetCube(inputCubePath);

    // Grab the provided points (coordinates)
    QList< QPair<double, double> > points = getPoints(ui, ui.WasEntered("COORDLIST"));

    // Get the camera point info for coordiante
    QList<PvlGroup*> camPoints = getCameraPointInfo(ui, points, campt);

    writePoints(ui, camPoints, log);
  }


  // We can grab our coordinates, either from the ui position parameters or the coordlist parameter
  // This method returns a list of double pairs (i.e. a list of coordinates)
  QList< QPair<double, double> > getPoints(const UserInterface &ui, bool usePointList) {
    double point1 = 0.0;
    double point2 = 0.0;
    QList< QPair<double, double> > points;
    QString pointType = ui.GetString("TYPE");

    // Check if the provided coordinate list is valid, i.e. a Samp/Line or Lat/Long coordinate per row
    if (usePointList) {

      CSVReader reader;
      reader.read(FileName(ui.GetFileName("COORDLIST")).expanded());

      if (!reader.isTableValid(reader.getTable()) || reader.columns() != 2) {
        QString msg = "Coordinate file formatted incorrectly.\n"
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
      else {
        point1 = ui.GetDouble("LATITUDE");
        point2 = ui.GetDouble("LONGITUDE");
      }
      points.append(QPair<double, double>(point1, point2));
    }

    return points;
  }


  // Gets the camera information for each point (coordinate).
  // Passed in a list of coordinates, passed in by reference a CameraPointInfo object.
  // Returns a list of PvlGroup pointers - these groups contain the camera info for each coordinate.
  QList<PvlGroup*> getCameraPointInfo(const UserInterface &ui,
                                      QList< QPair<double, double> > points,
                                      CameraPointInfo &campt) {
    // Setup our parameters from ui and variables
    QList<PvlGroup*> cameraPoints;
    bool usePointList = ui.WasEntered("COORDLIST");
    bool allowOutside = ui.GetBoolean("ALLOWOUTSIDE");
    bool allowError = ui.GetBoolean("ALLOWERROR");
    QString type;
    if (ui.WasEntered("COORDLIST")) {
      type = ui.GetString("COORDTYPE");
    }
    else {
      type = ui.GetString("TYPE");
    }
    PvlGroup *camPoint = NULL;

    // Depending on what type is selected, set values accordingly
    for (int i = 0; i < points.size(); i++) {

      QPair<double, double> pt = points[i];
      if (type == "GROUND") {
        if (allowError) {
          camPoint = campt.SetGround(pt.first, pt.second, allowOutside, allowError);
        }
        else {
          camPoint = campt.SetGround(pt.first, pt.second, allowOutside, usePointList);
        }      
      }
      else {
        if (usePointList) {
          camPoint = campt.SetImage(pt.first, pt.second, allowOutside, usePointList);
        }
        else {
          if (ui.WasEntered("SAMPLE") && ui.WasEntered("LINE")) {
            camPoint = campt.SetImage(pt.first, pt.second, allowOutside, allowError);
          }
          else {
            if (ui.WasEntered("SAMPLE")) {
              camPoint = campt.SetSample(pt.first, allowOutside, allowError);
            }
            else if (ui.WasEntered("LINE")) {
              camPoint = campt.SetLine(pt.second, allowOutside, allowError);
            }
            else {
              camPoint = campt.SetCenter(allowOutside, allowError);
            }
          }
        }
      }
      cameraPoints.append(camPoint);
    }
    camPoint = NULL;
    return cameraPoints;
  }


  // Write our point coordinates to std out in pvl format, or to a pvl or some type of flat file
  void writePoints(const UserInterface &ui, QList<PvlGroup*> camPoints, Pvl *log) {
    // Progress should increment for each point we process
    Progress prog;
    prog.SetMaximumSteps(camPoints.size());
    QString outFile;
    // Get user params from ui
    if (ui.WasEntered("TO")) {
      outFile = FileName(ui.GetFileName("TO")).expanded();
    }
    bool append = ui.GetBoolean("APPEND");
    QString fileFormat = ui.GetString("FORMAT");
    PvlGroup *point = NULL;

    for (int p = 0; p < camPoints.size(); p++) {
        bool fileExists = FileName(outFile).fileExists();

      prog.CheckStatus();
      point = camPoints[p];

      // Remove units on look direction vectors
      point -> findKeyword("LookDirectionBodyFixed").setUnits("");
      point -> findKeyword("LookDirectionJ2000").setUnits("");
      point -> findKeyword("LookDirectionCamera").setUnits("");

      // write to output file
      if (ui.WasEntered("TO")) {
        // Write the pvl group out to the file
        if (fileFormat == "PVL") {
          Pvl temp;
          temp.setTerminator("");
          temp.addGroup((*point));

         // we don't want to overwrite successive points in outfile
          if (append || p > 0) {
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
          if (append || p > 0) {
          os.open(outFile.toLatin1().data(), ios::app);
            if (!fileExists) {
              writeHeader = true;
            }
          }
          else {
            os.open(outFile.toLatin1().data(), ios::out);
            writeHeader = true;
          }

          if (writeHeader) {
            for (int i = 0; i < (*point).keywords(); i++) {
              if ((*point)[i].size() == 3) {
                os << (*point)[i].name() << "X,"
                << (*point)[i].name() << "Y,"
                << (*point)[i].name() << "Z";
              }
              else {
                os << (*point)[i].name();
              }

              if (i < point->keywords() - 1) {
                os << ",";
              }
            }
            os << endl;
          }



          for (int i = 0; i < (*point).keywords(); i++) {
            if ((*point)[i].size() == 3) {
              os << (QString)(*point)[i][0] << ","
              << (QString)(*point)[i][1] << ","
              << (QString)(*point)[i][2];
            }
            else {
              os << (QString)(*point)[i];
            }

            if (i < (*point).keywords() - 1) {
              os << ",";
            }
          }
          os << endl;
        }
      }

      // No output file specified
      else {
        // don't log data -
        if (ui.GetString("FORMAT") == "FLAT") {
          string msg = "Flat file must have a name.";
          throw IException(IException::User, msg, _FILEINFO_);
        }
      }

      // we still want to output the results
      Application::Log((*point));
      delete point;
      point = NULL;
    }
    prog.CheckStatus();
  }
}
