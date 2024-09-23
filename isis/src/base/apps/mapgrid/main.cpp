#include "Isis.h"

#include <cmath>

#include "IException.h"
#include "ProjectionFactory.h"
#include "Progress.h"
#include "TProjection.h"
#include "Pvl.h"
#include "UserInterface.h"

using namespace std;
using namespace Isis;

void StartNewLine(std::ofstream &);
void AddPointToLine(std::ofstream &, double, double);
void EndLine(std::ofstream &);
void CheckContinuous(double latlon, double latlon_start, double X, double Y, double lastX, double lastY, double maxChange, std::ofstream &os);

void IsisMain() {
  // Get user interface
  UserInterface &ui = Application::GetUserInterface();

  // Get necessary variables from the user
  double latStart = ui.GetDouble("STARTLAT");
  double lonStart = ui.GetDouble("STARTLON");
  double latEnd = ui.GetDouble("ENDLAT");
  double lonEnd = ui.GetDouble("ENDLON");
  double latSpacing = ui.GetDouble("LATSPACING");
  double lonSpacing = ui.GetDouble("LONSPACING");
  double latInc = ui.GetDouble("LATINCREMENT");
  double lonInc = ui.GetDouble("LONINCREMENT");

  // Get mapfile, add values for range and create projection
  QString mapFile = ui.GetFileName("MAPFILE");
  Pvl p(mapFile.toStdString());
  PvlGroup &mapping = p.findGroup("Mapping", Pvl::Traverse);

  if(mapping.hasKeyword("MinimumLatitude")) {
    mapping.deleteKeyword("MinimumLatitude");
  }

  if(mapping.hasKeyword("MaximumLatitude")) {
    mapping.deleteKeyword("MaximumLatitude");
  }

  if(mapping.hasKeyword("MinimumLongitude")) {
    mapping.deleteKeyword("MinimumLongitude");
  }

  if(mapping.hasKeyword("MaximumLongitude")) {
    mapping.deleteKeyword("MaximumLongitude");
  }

  mapping += PvlKeyword("MinimumLatitude", Isis::toString(latStart));
  mapping += PvlKeyword("MaximumLatitude", Isis::toString(latEnd));
  mapping += PvlKeyword("MinimumLongitude", Isis::toString(lonStart));
  mapping += PvlKeyword("MaximumLongitude", Isis::toString(lonEnd));

  TProjection *proj;
  try {
    proj = (TProjection *) ProjectionFactory::Create(p);
  }
  catch(IException &e) {
    std::string msg = "Cannot create grid - MapFile [" + mapFile.toStdString() +
                  "] does not contain necessary information to create a projection";
    throw IException(e, IException::User, msg, _FILEINFO_);
  }


  // Write grid to well known text output
  QString out = QString::fromStdString(FileName(ui.GetFileName("TO").toStdString()).expanded());
  std::ofstream os;
  os.open(out.toLatin1().data(), std::ios::out);

  // Display the progress...10% 20% etc.
  Progress prog;
  int steps = (int)(abs((latEnd - latStart) / latSpacing) +
                    abs((lonEnd - lonStart) / lonSpacing) + 0.5) + 3;
  prog.SetMaximumSteps(steps);
  prog.CheckStatus();

  /**
   * Initialize document. GML is XML-based, so we need the necessary XML headers. These
   * are necessary for the GML file to be recognized.
   */
  os << "<?xml version=\"1.0\" encoding=\"utf-8\" ?>" << endl;
  os << "<ogr:FeatureCollection " << endl <<
     "xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\"" << endl <<
     "xsi:schemaLocation=\"http://org.maptools.org/\"" << endl <<
     "xmlns:ogr=\"http://org.maptools.org/\"" << endl <<
     "xmlns:gml=\"http://www.opengis.net/gml\">" << endl;

  /**
   * Draw the interior longitude lines by looping through the longitude
   * range and drawing across each latitude line using the longitude increment. The first
   * and last line will be skipped for now.
   */
  for(double j = lonStart + lonSpacing; j < lonEnd; j += lonSpacing) {
    StartNewLine(os);
    for(double k = latStart; k <= latEnd; k += lonInc) {
      proj->SetGround(k, j);
      AddPointToLine(os, proj->XCoord(), proj->YCoord());
    }

    EndLine(os);
    prog.CheckStatus();
  }

  /**
   * Draw the exterior longitude boundary lines. This happens by drawing just the first and
   * last longitude lines.
   */
  for(double r = lonStart; r <= lonEnd; r += (lonEnd - lonStart)) {
    StartNewLine(os);
    for(double s = latStart; s <= latEnd; s += lonInc) {
      proj->SetGround(s, r);
      AddPointToLine(os, proj->XCoord(), proj->YCoord());
    }
    EndLine(os);
    prog.CheckStatus();
  }

  /**
   * Draw the interior latitude lines by looping through the latitude
   * range and drawing across each longitude line using the latitude increment. The first
   * and last line will be skipped for now.
   */
  for(double i = latStart + latSpacing; i < latEnd; i += latSpacing) {

    // Get Latitude Line
    StartNewLine(os);
    for(double l = lonStart; l <= lonEnd; l += latInc) {
      proj->SetGround(i, l);
      AddPointToLine(os, proj->XCoord(), proj->YCoord());
    }
    EndLine(os);
    prog.CheckStatus();
  }

  /**
   * Draw the exterior latitude boundary lines. This happens by drawing just the first and
   * last longitude lines.
   */
  for(double m = latStart; m <= latEnd; m += (latEnd - latStart)) {
    StartNewLine(os);

    for(double n = lonStart; n <= lonEnd; n += latInc) {
      proj->SetGround(m, n);
      AddPointToLine(os, proj->XCoord(), proj->YCoord());
    }

    EndLine(os);
    prog.CheckStatus();
  }

  /**
   * Draw the bounding box using a series of lines.
   */
  if(ui.GetBoolean("BOUNDED")) {
    double minX, maxX, minY, maxY;
    proj->XYRange(minX, maxX, minY, maxY);

    StartNewLine(os);
    AddPointToLine(os, minX, minY);
    AddPointToLine(os, minX, maxY);
    EndLine(os);

    StartNewLine(os);
    AddPointToLine(os, minX, maxY);
    AddPointToLine(os, maxX, maxY);
    EndLine(os);

    StartNewLine(os);
    AddPointToLine(os, maxX, minY);
    AddPointToLine(os, maxX, maxY);
    EndLine(os);

    StartNewLine(os);
    AddPointToLine(os, minX, minY);
    AddPointToLine(os, maxX, minY);
    EndLine(os);
  }

  os << "</ogr:FeatureCollection>" << endl;

  // add mapping to print.prt
  PvlGroup projMapping = proj->Mapping();
  Application::Log(projMapping);
}

/**
 * This will prepare a new line start in GML. This should be called every time a new line is
 * started and generates unique IDs for each line.
 *
 * @param os output file stream
 */
void StartNewLine(std::ofstream &os) {
  static int lineID = 0;

  os << "<gml:featureMember>" << endl;
  os << "  <ogr:mapLine fid=\"F" << lineID << "\">" << endl;
  os << "    <ogr:ID>" << lineID << "</ogr:ID>" << endl;
  os << "    <ogr:geometryProperty>" << "<gml:LineString>" << "<gml:coordinates>";

  lineID ++;
}

/**
 * This will add a point to a line started with StartNewLine. StartNewLine must be
 * called before using this method, and EndLine after all of the points in the line have
 * been added.
 *
 * @param os output file stream
 * @param x x coordinate
 * @param y y coordinate
 */
void AddPointToLine(std::ofstream &os, const double x, const double y) {
  os << x << "," << y << " ";
}

/**
 * This will end a line in GML. This should be called after each line has the necessary points
 * added using AddPointToLine.
 *
 * @param os output file stream
 */
void EndLine(std::ofstream &os) {
  os << "</gml:coordinates>" << "</gml:LineString>" << "</ogr:geometryProperty>" << endl;
  os << "  </ogr:mapLine>" << endl;
  os << "</gml:featureMember>" << endl;
}

/**
 * This function was created to deal with potential discontinuities in mapping patterns in order to not
 * connect them. It will create a new line if there is more than a maxChange difference in the points.
 * This was coded for ObliqueCylindrical.
 *
 * @param latlon Current latitude or longitude value
 * @param latlon_start Initial latitude or longitude value of this line
 * @param X X coordinate of this point
 * @param Y Y coordinate of this point
 * @param lastX Previous X coordinate of this point
 * @param lastY Previous Y coordinate of this point
 * @param maxChange Maximum distance between these two points
 * @param os Output file stream
 */
/*void CheckContinuous(double latlon, double latlon_start, double X, double Y, double lastX, double lastY, double maxChange, std::ofstream &os) {
  if(latlon != latlon_start) {
    if(sqrt(pow(lastX - X,2) + (pow(lastX - X,2))) > maxChange) {
      EndLine(os);
      StartNewLine(os);
    }
  }
}*/
