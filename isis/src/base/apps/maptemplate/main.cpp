#define GUIHELPERS

#include "Isis.h"

#include <map>
#include <cctype>

#include "Camera.h"
#include "Cube.h"
#include "FileList.h"
#include "IException.h"
#include "IString.h"
#include "Target.h"
#include "ProjectionFactory.h"
#include "PvlGroup.h"
#include "SpecialPixel.h"
#include "Spice.h"
#include "UserInterface.h"

using namespace std;
using namespace Isis;


// functions in the code
void addProject(PvlGroup &mapping);
void addTarget(PvlGroup &mapping);
void addRange(PvlGroup &mapping);
void calcRange(double &minLat, double &maxLat,
               double &minLon, double &maxLon);
void addResolution(PvlGroup &mapping);
double calcResolution();

// helper button functins in the code
void helperButtonLogMap();
void helperButtonLoadMap();
void helperButtonLogTargDef();
void helperButtonLoadTargDef();
void helperButtonLogRadius();
void helperButtonCalcRange();
double helperButtonCalcResolution();

map <QString, void *> GuiHelpers() {
  map <QString, void *> helper;
  helper ["helperButtonLogMap"] = (void *) helperButtonLogMap;
  helper ["helperButtonLoadMap"] = (void *) helperButtonLoadMap;
  helper ["helperButtonLogTargDef"] = (void *) helperButtonLogTargDef;
  helper ["helperButtonLoadTargDef"] = (void *) helperButtonLoadTargDef;
  helper ["helperButtonLogRadius"] = (void *) helperButtonLogRadius;
  helper ["helperButtonCalcRange"] = (void *) helperButtonCalcRange;
  helper ["helperButtonCalcResolution"] = (void *) helperButtonCalcResolution;
  return helper;
}

void IsisMain() {
  // Make the mapping pvl group and add info to it.
  PvlGroup mapping("Mapping");
  addProject(mapping);
  addTarget(mapping);
  addRange(mapping);
  addResolution(mapping);

  // Get map file name from GUI and write the mapping group pvl
  // to the output file, add .map extension if the
  // user did not enter an extension
  UserInterface &ui = Application::GetUserInterface();
  FileName out = ui.GetFileName("MAP").toStdString();
  QString output = ui.GetFileName("MAP");
  if(out.extension() == "") {
    output += ".map";
  }
  Pvl p;
  p.addGroup(mapping);
  p.write(output.toStdString());
}

// Helper function to output map file to log.
void helperButtonLogMap() {
  UserInterface &ui = Application::GetUserInterface();
  QString mapFile(ui.GetFileName("MAP"));
  Pvl p;
  p.read(mapFile.toStdString());
  PvlGroup t = p.findGroup("mapping", Pvl::Traverse);
  QString OQString = "***** Output of [" + mapFile + "] *****";
  Application::GuiLog(OQString);
  Application::GuiLog(t);
}
//...........end of helper function LogMap ........

// Helper 2 function to update GUI with map file values.
void helperButtonLoadMap() {
  UserInterface &ui = Application::GetUserInterface();
  QString mapFile(ui.GetFileName("MAP"));
  Pvl p;
  p.read(mapFile.toStdString());
  PvlGroup t = p.findGroup("mapping", Pvl::Traverse);
  // Projection Stuff
  ui.Clear("CLON");
  ui.Clear("CLAT");
  ui.Clear("SCALEFACTOR");
  ui.Clear("PAR1");
  ui.Clear("PAR2");
  ui.Clear("PLAT");
  ui.Clear("PLON");
  ui.Clear("PROT");
  ui.Clear("DIST");

  if(t.hasKeyword("ProjectionName")) {
    QString projIn = QString::fromStdString(t["ProjectionName"]);
    projIn = projIn.toUpper();
    ui.Clear("PROJECTION");
    ui.PutAsString("PROJECTION", projIn);
  }
  if(t.hasKeyword("CenterLongitude")) {
    double clonIn = t["CenterLongitude"];
    ui.Clear("CLON");
    ui.PutDouble("CLON", clonIn);
  }
  if(t.hasKeyword("CenterLatitude")) {
    double clatIn = t["CenterLatitude"];
    ui.Clear("CLAT");
    ui.PutDouble("CLAT", clatIn);
  }
  if(t.hasKeyword("ScaleFactor")) {
    double scaleFactorIn = t["ScaleFactor"];
    ui.Clear("SCALEFACTOR");
    ui.PutDouble("SCALEFACTOR", scaleFactorIn);
  }
  if(t.hasKeyword("FirstStandardParallel")) {
    double par1In = t["FirstStandardParallel"];
    ui.Clear("PAR1");
    ui.PutDouble("PAR1", par1In);
  }
  if(t.hasKeyword("SecondStandardParallel")) {
    double par2In = t["SecondStandardParallel"];
    ui.Clear("PAR2");
    ui.PutDouble("PAR2", par2In);
  }
  if(t.hasKeyword("PoleLatitude")) {
    double pLatIn = t["PoleLatitude"];
    ui.Clear("PLAT");
    ui.PutDouble("PLAT", pLatIn);
  }
  if(t.hasKeyword("PoleLongitude")) {
    double pLonIn = t["PoleLongitude"];
    ui.Clear("PLON");
    ui.PutDouble("PLON", pLonIn);
  }
  if(t.hasKeyword("PoleRotation")) {
    double pRotIn = t["PoleRotation"];
    ui.Clear("PROT");
    ui.PutDouble("PROT", pRotIn);
  }
  if(t.hasKeyword("Distance")) {
    double distIn = t["Distance"];
    ui.Clear("DIST");
    ui.PutDouble("DIST", distIn);
  }

  // Target Parameters stuff
  QString use = "NONE";
  ui.Clear("TARGOPT");
  ui.PutAsString("TARGOPT", use);
  ui.Clear("FILE");
  ui.Clear("TARGETNAME");
  ui.Clear("LATTYPE");
  ui.Clear("LONDIR");
  ui.Clear("EQRADIUS");
  ui.Clear("POLRADIUS");

  if(t.hasKeyword("TargetName")) {
    QString use = "USER";
    ui.Clear("TARGOPT");
    ui.PutAsString("TARGOPT", use);
    QString tNameIn = QString::fromStdString(t["TargetName"]);
    ui.Clear("TARGETNAME");
    ui.PutAsString("TARGETNAME", tNameIn);

    QString LTIn = QString::fromStdString(t["LatitudeType"]);
    LTIn = LTIn.toUpper();
    ui.Clear("LATTYPE");
    ui.PutAsString("LATTYPE", LTIn);

    QString LDIn = QString::fromStdString(t["LongitudeDirection"]);
    LDIn = LDIn.toUpper();
    ui.Clear("LONDIR");
    ui.PutAsString("LONDIR", LDIn);

    QString LDomIn = QString::fromStdString(t["LongitudeDomain"]);
    ui.Clear("LONDOM");
    ui.PutAsString("LONDOM", LDomIn);

    QString EQIn = QString::fromStdString(t["EquatorialRadius"]);
    ui.Clear("EQRADIUS");
    ui.PutAsString("EQRADIUS", EQIn);
    QString PRIn = QString::fromStdString(t["PolarRadius"]);
    ui.Clear("POLRADIUS");
    ui.PutAsString("POLRADIUS", PRIn);
  }

  // Ground Range Parameter stuff
  ui.Clear("MINLAT");
  ui.Clear("MAXLAT");
  ui.Clear("MINLON");
  ui.Clear("MAXLON");
  QString useR = "NONE";
  ui.Clear("RNGOPT");
  ui.PutAsString("RNGOPT", useR);

  if(t.hasKeyword("MinimumLatitude")) {
    double minlatIn = t["MinimumLatitude"];
    QString useR = "USER";
    ui.Clear("RNGOPT");
    ui.PutAsString("RNGOPT", useR);
    ui.Clear("MINLAT");
    ui.PutDouble("MINLAT", minlatIn);
    double maxlatIn = t["MaximumLatitude"];
    ui.Clear("MAXLAT");
    ui.PutDouble("MAXLAT", maxlatIn);
    double minlonIn = t["MinimumLongitude"];
    ui.Clear("MINLON");
    ui.PutDouble("MINLON", minlonIn);
    double maxlonIn = t["MaximumLongitude"];
    ui.Clear("MAXLON");
    ui.PutDouble("MAXLON", maxlonIn);
  }

  // Resolution Parameter stuff
  if(t.hasKeyword("PixelResolution")) {
    QString useM = "MPP";
    ui.Clear("RESOPT");
    ui.PutAsString("RESOPT", useM);
    double pixresIn = t["PixelResolution"];
    ui.Clear("RESOLUTION");
    ui.PutDouble("RESOLUTION", pixresIn);
  }
  if(t.hasKeyword("Scale")) {
    QString useM = "PPD";
    ui.Clear("RESOPT");
    ui.PutAsString("RESOPT", useM);
    double Mscale = t["Scale"];
    ui.Clear("RESOLUTION");
    ui.PutDouble("RESOLUTION", Mscale);
  }
}
//----------End Helper function to load map file into GUI -----------

// helper function to output targdef to log
void helperButtonLogTargDef() {
  UserInterface &ui = Application::GetUserInterface();
  QString targetFile(ui.GetFileName("FILE"));
  Pvl p;
  p.read(targetFile.toStdString());
  PvlGroup t = p.findGroup("mapping", Pvl::Traverse);
  QString OQString = "***** Output of [" + targetFile + "] *****";
  Application::GuiLog(OQString);
  Application::GuiLog(t);
}
//------------End Helper function to display targdef info to log -----

// helper function to load target def. to GUI
void helperButtonLoadTargDef() {
  UserInterface &ui = Application::GetUserInterface();
  QString targetFile(ui.GetFileName("FILE"));

  // test if targdef was entered
  Pvl p;
  p.read(targetFile.toStdString());
  PvlGroup t = p.findGroup("mapping", Pvl::Traverse);
  // Load the targdef values into the GUI
  QString tOpt = "USER";
  ui.Clear("TARGOPT");
  ui.PutAsString("TARGOPT", tOpt);
  if(t.hasKeyword("TargetName")) {
    QString tNameIn = QString::fromStdString(t["TargetName"]);
    ui.Clear("TARGETNAME");
    ui.PutAsString("TARGETNAME", tNameIn);
  }
  if(t.hasKeyword("LatitudeType")) {
    QString LTIn = QString::fromStdString(t["LatitudeType"]);
    LTIn = LTIn.toUpper();
    ui.Clear("LATTYPE");
    ui.PutAsString("LATTYPE", LTIn);
  }
  if(t.hasKeyword("LongitudeDirection")) {
    QString LDIn = QString::fromStdString(t["LongitudeDirection"]);
    LDIn = LDIn.toUpper();
    ui.Clear("LONDIR");
    ui.PutAsString("LONDIR", LDIn);
  }
  if(t.hasKeyword("LongitudeDomain")) {
    QString LDomIn = QString::fromStdString(t["LongitudeDomain"]);
    ui.Clear("LONDOM");
    ui.PutAsString("LONDOM", LDomIn);
  }
  if(t.hasKeyword("EquatorialRadius")) {
    QString EQIn = QString::fromStdString(t["EquatorialRadius"]);
    ui.Clear("EQRADIUS");
    ui.PutAsString("EQRADIUS", EQIn);
  }
  if(t.hasKeyword("PolarRadius")) {
    QString PRIn = QString::fromStdString(t["PolarRadius"]);
    ui.Clear("POLRADIUS");
    ui.PutAsString("POLRADIUS", PRIn);
  }
}
//-----------end Helper Function to Load target def into GUI --------

// Helper function to show system radius in log.
void helperButtonLogRadius() {
  UserInterface &ui = Application::GetUserInterface();
  QString targetName = ui.GetString("TARGETNAME");
  Pvl tMap;
  PvlGroup tGrp;
  // call function to get system radius
  try {
    tGrp = Target::radiiGroup(targetName);
  }
  catch (IException &e) {
    throw IException(e, 
                     IException::Unknown, 
                     "Unrecognized target. User must enter EQRADIUS and POLRADIUS values.", 
                     _FILEINFO_);
  }
  tMap.addGroup(tGrp);
  QString OQString = "***** System radii for " + targetName + "*****";
  // write to log QString(OQString and mapping group
  Application::GuiLog(OQString);
  Application::GuiLog(tMap);
}
//-----------end ot log radius helper function---------

// Helper function to run calcRange function.
void helperButtonCalcRange() {
  UserInterface &ui = Application::GetUserInterface();
  double minLat;
  double maxLat;
  double minLon;
  double maxLon;
  // Run the function calcRange of calculate range info
  calcRange(minLat, maxLat, minLon, maxLon);
  // Write ranges to the GUI
  QString use = "USER";
  ui.Clear("RNGOPT");
  ui.PutAsString("RNGOPT", use);
  ui.Clear("MINLAT");
  ui.PutDouble("MINLAT", minLat);
  ui.Clear("MAXLAT");
  ui.PutDouble("MAXLAT", maxLat);
  ui.Clear("MINLON");
  ui.PutDouble("MINLON", minLon);
  ui.Clear("MAXLON");
  ui.PutDouble("MAXLON", maxLon);
}
//--------------End Helper function to run calcRange ----------------

// Helper function to run calcResolution function.
double helperButtonCalcResolution() {
  UserInterface &ui = Application::GetUserInterface();
  // Call calcResolution which will return a double
  double Res = calcResolution();
  // update the GUI with the new resolution
  QString resUse = "MPP";
  ui.Clear("RESOPT");
  ui.PutAsString("RESOPT", resUse);
  ui.Clear("RESOLUTION");
  ui.PutDouble("RESOLUTION", Res);
  return Res;
}
//..............End Helper function to run calcResolution

// Function to Add the projection information to the Mapping PVL
void addProject(PvlGroup &mapping) {
  UserInterface &ui = Application::GetUserInterface();
  QString projName = ui.GetString("PROJECTION");
  // setup a look up table for projection names
  map <QString, QString> projLUT;
  projLUT ["SINUSOIDAL"] = "Sinusoidal";
  projLUT ["MERCATOR"] = "Mercator";
  projLUT ["TRANSVERSEMERCATOR"] = "TransverseMercator";
  projLUT ["ORTHOGRAPHIC"] = "Orthographic";
  projLUT ["POLARSTEREOGRAPHIC"] = "PolarStereographic";
  projLUT ["SIMPLECYLINDRICAL"] = "SimpleCylindrical";
  projLUT ["EQUIRECTANGULAR"] = "Equirectangular";
  projLUT ["LAMBERTCONFORMAL"] = "LambertConformal";
  projLUT ["LAMBERTAZIMUTHALEQUALAREA"] = "LambertAzimuthalEqualArea";
  projLUT ["OBLIQUECYLINDRICAL"] = "ObliqueCylindrical";
  projLUT ["POINTPERSPECTIVE"] = "PointPerspective";
  projLUT ["ROBINSON"] = "Robinson";

  // Add Projection keywords to the mappping PVL
  mapping += PvlKeyword("ProjectionName", projLUT[projName].toStdString());
  if(ui.WasEntered("CLON")) {
    double clonOut = ui.GetDouble("CLON");
    mapping += PvlKeyword("CenterLongitude", std::to_string(clonOut));
  }
  if(ui.WasEntered("CLAT")) {
    double clatOut = ui.GetDouble("CLAT");
    mapping += PvlKeyword("CenterLatitude", std::to_string(clatOut));
  }
  if(ui.WasEntered("SCALEFACTOR")) {
    double scaleFactorOut = ui.GetDouble("SCALEFACTOR");
    mapping += PvlKeyword("ScaleFactor", std::to_string(scaleFactorOut));
  }
  if(ui.WasEntered("PAR1")) {
    double par1 = ui.GetDouble("PAR1");
    mapping += PvlKeyword("FirstStandardParallel", std::to_string(par1));
  }
  if(ui.WasEntered("PAR2")) {
    double par2 = ui.GetDouble("PAR2");
    mapping += PvlKeyword("SecondStandardParallel", std::to_string(par2));
  }
  if(ui.WasEntered("PLAT")) {
    double plat = ui.GetDouble("PLAT");
    mapping += PvlKeyword("PoleLatitude", std::to_string(plat));
  }
  if(ui.WasEntered("PLON")) {
    double plon = ui.GetDouble("PLON");
    mapping += PvlKeyword("PoleLongitude", std::to_string(plon));
  }
  if(ui.WasEntered("PROT")) {
    double prot = ui.GetDouble("PROT");
    mapping += PvlKeyword("PoleRotation", std::to_string(prot));
  }
  if(ui.WasEntered("DIST")) {
    double dist = ui.GetDouble("DIST");
    mapping += PvlKeyword("Distance", std::to_string(dist));
  }
}

// Function to Add the target information to the Mapping PVL
void addTarget(PvlGroup &mapping) {
  UserInterface &ui = Application::GetUserInterface();
  if(ui.GetString("TARGOPT") == "SELECT") {
    QString targetFile(ui.GetFileName("FILE"));
    Pvl p;
    p.read(targetFile.toStdString());
    PvlGroup t = p.findGroup("mapping");
    if(t.hasKeyword("TargetName")) {
      mapping += t["TargetName"];
    }
    if(t.hasKeyword("EquatorialRadius")) {
      mapping += t["EquatorialRadius"];
    }
    if(t.hasKeyword("PolarRadius")) {
      mapping += t["PolarRadius"];
    }
    if(t.hasKeyword("LatitudeType")) {
      mapping += t["LatitudeType"];
    }
    if(t.hasKeyword("LongitudeDirection")) {
      mapping += t["LongitudeDirection"];
    }
  }
  // if TARGOPT is user and no radii have been entered by the user,
  // then call TargetRadii to get the system radii for target name.
  else if(ui.GetString("TARGOPT") == "USER") {

    QString targetName = ui.GetString("TARGETNAME");
    mapping += PvlKeyword("TargetName", targetName.toStdString());

    PvlGroup radii;
    // if either radius value was not entered, then we will attempt to get it from the target name
    if ( !ui.WasEntered("EQRADIUS") || !ui.WasEntered("POLRADIUS") ) {
      try {
        // this group will contain TargetName, EquatorialRadius, and PolarRadius
        radii = Target::radiiGroup(targetName);
      }
      catch (IException &e) {
        std::string msg = "Unable to find target radii automatically. "
                      "User must provide EQRADIUS and POLRADIUS values for this target.";
        throw IException(e, IException::Unknown, msg, _FILEINFO_);
      }
    }

    if(ui.WasEntered("EQRADIUS")) {
      mapping += PvlKeyword("EquatorialRadius", ui.GetAsString("EQRADIUS").toStdString(), "meters");
    }
    else {
      mapping += radii.findKeyword("EquatorialRadius"); // already formatted in meters
    }

    if(ui.WasEntered("POLRADIUS")) {
      mapping += PvlKeyword("PolarRadius", ui.GetAsString("POLRADIUS").toStdString(), "meters");
    }
    else {
      mapping += radii.findKeyword("PolarRadius"); // already formatted in meters
    }

    if (QString::compare(ui.GetString("LATTYPE"), "Planetocentric", Qt::CaseInsensitive) == 0) {
      mapping += PvlKeyword("LatitudeType", "Planetocentric");
    }
    else {
      mapping += PvlKeyword("LatitudeType", "Planetographic");
    }


    if (QString::compare(ui.GetString("LONDIR"), "PositiveEast", Qt::CaseInsensitive) == 0) {
      mapping += PvlKeyword("LongitudeDirection", "PositiveEast");
    }
    else {
      mapping += PvlKeyword("LongitudeDirection", "PositiveWest");
    }

    mapping += PvlKeyword("LongitudeDomain", ui.GetString("LONDOM").toStdString());
  }
}

// Function to add the range information to the mapping PVL
void addRange(PvlGroup &mapping) {
  UserInterface &ui = Application::GetUserInterface();
  // Use the values that have been entered in the GUI
  if(ui.GetString("RNGOPT") == "USER") {
    double minLat = ui.GetDouble("MINLAT");
    mapping += PvlKeyword("MinimumLatitude", std::to_string(minLat));
    double maxLat = ui.GetDouble("MAXLAT");
    mapping += PvlKeyword("MaximumLatitude", std::to_string(maxLat));
    double minLon = ui.GetDouble("MINLON");
    mapping += PvlKeyword("MinimumLongitude", std::to_string(minLon));
    double maxLon = ui.GetDouble("MAXLON");
    mapping += PvlKeyword("MaximumLongitude", std::to_string(maxLon));
  }
  else if(ui.GetString("RNGOPT") == "CALC") {
    // calculate range values using function calcRange and fromlist
    double minLat;
    double maxLat;
    double minLon;
    double maxLon;
    // Call calcRange to calculate min and max ground range values
    calcRange(minLat, maxLat, minLon, maxLon);
    mapping += PvlKeyword("MinimumLatitude", std::to_string(minLat));
    mapping += PvlKeyword("MaximumLatitude", std::to_string(maxLat));
    mapping += PvlKeyword("MinimumLongitude", std::to_string(minLon));
    mapping += PvlKeyword("MaximumLongitude", std::to_string(maxLon));
  }
}

// Function to add the resolution information to the mapping PVL
void addResolution(PvlGroup &mapping) {
  // Use values that have been entered in GUI
  UserInterface &ui = Application::GetUserInterface();
  if(ui.GetString("RESOPT") == "PPD") {
    double res = ui.GetDouble("RESOLUTION");
    mapping += PvlKeyword("Scale", std::to_string(res), "pixels/degree");
  }
  else if(ui.GetString("RESOPT") == "MPP") {
    double res = ui.GetDouble("RESOLUTION");
    mapping += PvlKeyword("PixelResolution", std::to_string(res), "meters/pixel");
  }
  else if(ui.GetString("RESOPT") == "CALC") {
    // run the function to calculate the resolution
    double Res = calcResolution();
    mapping += PvlKeyword("PixelResolution", std::to_string(Res), "meters/pixel");
  }
}

// Function to calculate the ground range from multiple inputs (list of images)
void calcRange(double &minLat, double &maxLat,
               double &minLon, double &maxLon) {
  UserInterface &ui = Application::GetUserInterface();
  FileList flist(FileName(ui.GetFileName("FROMLIST").toStdString()));
  minLat = DBL_MAX;
  maxLat = -DBL_MAX;
  minLon = DBL_MAX;
  maxLon = -DBL_MAX;
  // set up PVL of mapping info from GUI:: need to do this so we use the
  // most current info from GUI to calculate the range
  Pvl userMap;
  PvlGroup userGrp("Mapping");
  if(ui.GetString("TARGOPT") == "SELECT") {
    userMap.read(ui.GetFileName("FILE").toStdString());
  }
  else if(ui.GetString("TARGOPT") == "USER") {
    QString targetName = ui.GetString("TARGETNAME");

    PvlGroup radii;
    // if either radius value was not entered, then we will attempt to get it from the target name
    if ( !ui.WasEntered("EQRADIUS") || !ui.WasEntered("POLRADIUS") ) {
      try {
        // this ensures that we look for the target radii of the TARGET specified by the user
        radii += PvlKeyword("TargetName", targetName.toStdString());
        // this group will contain TargetName, EquatorialRadius, and PolarRadius
        Pvl cubeLab(flist[0].expanded());
        radii = Target::radiiGroup(cubeLab, radii);
      }
      catch (IException &e) {
        std::string msg = "Unable to find target radii automatically. "
                      "User must provide EQRADIUS and POLRADIUS values for this target.";
        throw IException(e, IException::Unknown, msg, _FILEINFO_);
      }
    }

    userGrp += PvlKeyword("TargetName", targetName.toStdString());

    // if radii were entered in GUI then set radii to entered value
    if(ui.WasEntered("EQRADIUS")) {
      userGrp += PvlKeyword("EquatorialRadius", ui.GetAsString("EQRADIUS").toStdString(), "Meters");
    }
    else {
      userGrp += radii.findKeyword("EquatorialRadius");
    }

    if(ui.WasEntered("POLRADIUS")) {
      userGrp += PvlKeyword("PolarRadius", ui.GetAsString("POLRADIUS").toStdString(), "Meters");
    }
    else {
      userGrp += radii.findKeyword("PolarRadius");
    }

    if (QString::compare(ui.GetString("LATTYPE"), "Planetocentric", Qt::CaseInsensitive) == 0) {
      userGrp += PvlKeyword("LatitudeType", "Planetocentric");
    }
    else {
      userGrp += PvlKeyword("LatitudeType", "Planetographic");
    }


    if (QString::compare(ui.GetString("LONDIR"), "PositiveEast", Qt::CaseInsensitive) == 0) {
      userGrp += PvlKeyword("LongitudeDirection", "PositiveEast");
    }
    else {
      userGrp += PvlKeyword("LongitudeDirection", "PositiveWest");
    }

    userGrp += PvlKeyword("LongitudeDomain", ui.GetString("LONDOM").toStdString());
    userMap.addGroup(userGrp);
  }

  for(int i = 0; i < flist.size(); i++) {
    // Get the mapping group from the camera
    double camMinLat;
    double camMaxLat;
    double camMinLon;
    double camMaxLon;
    Cube c;
    c.open(QString::fromStdString(flist[i].toString()));
    Camera *cam = c.camera();
    Pvl defaultMap;
    cam->BasicMapping(defaultMap);
    PvlGroup &defaultGrp = defaultMap.findGroup("Mapping");
    // Move any defaults that are not in the user map
    for(int k = 0; k < defaultGrp.keywords(); k++) {
      if(!userGrp.hasKeyword(defaultGrp[k].name())) {
        userGrp += defaultGrp[k];
      }
    }
    userMap.addGroup(userGrp);
    // get the camera ground range min and max and solve for range
    cam->GroundRange(camMinLat, camMaxLat, camMinLon, camMaxLon, userMap);
    if(camMinLat < minLat) {
      minLat = camMinLat;
    }
    if(camMaxLat > maxLat) {
      maxLat = camMaxLat;
    }
    if(camMinLon < minLon) {
      minLon = camMinLon;
    }
    if(camMaxLon > maxLon) {
      maxLon = camMaxLon;
    }
  }
}

// Function to calculate the resolution for images in FROMLIST
//  value will be in meters
double calcResolution() {
  UserInterface &ui = Application::GetUserInterface();
  FileList flist(FileName(ui.GetFileName("FROMLIST").toStdString()));
  double sumRes = 0.0;
  double highRes = DBL_MAX;
  double lowRes = -DBL_MAX;
  // Loop through the from list at get high and low camera resolution
  for(int i = 0; i < flist.size(); i++) {
    Cube c;
    c.open(QString::fromStdString(flist[i].toString()));
    Camera *cam = c.camera();
    double camLowRes = cam->LowestImageResolution();
    double camHighRes = cam->HighestImageResolution();
    if(camLowRes > lowRes) lowRes = camLowRes;
    if(camHighRes < highRes) highRes = camHighRes;
    sumRes = sumRes + (camLowRes + camHighRes) / 2.0;
  }

  // user input (RESCALCOPT) at GUI will determain what is output
  // highest, lowest, or avg
  if(ui.GetString("RESCALCOPT") == "HIGH") return highRes;
  if(ui.GetString("RESCALCOPT") == "LOW") return lowRes;
  return(sumRes / flist.size());
}


