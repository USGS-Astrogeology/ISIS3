#include "Isis.h"

#include <iostream>

#include "geos/geom/MultiPolygon.h"
#include "UserInterface.h"
#include "FileName.h"
#include "Cube.h"
#include "ImagePolygon.h"
#include "PolygonTools.h"

using namespace std;
using namespace Isis;

void IsisMain() {


  UserInterface &ui = Application::GetUserInterface();

  // Get the polygon from the input cube. NOTE: the generated poly is always in the 0 to 360 domain.
  // Use the linc/sinc requested by the user.
  Cube cube;
  cube.open(ui.GetCubeName("FROM"));
  int sinc = ui.GetInteger("SINC");
  int linc = ui.GetInteger("LINC");
  ImagePolygon poly;
  //poly.Create(cube);
  poly.Create(cube, sinc, linc);
  geos::geom::MultiPolygon *mPolygon = poly.Polys();

  // Decide if the 0 to 360 longitude domain polygon needs to be converted to the -180 to 180 domain
  bool convertTo180 = false;
  // If the user wants to match the cube, find out what domain the cube is in
  if (ui.GetString("LONGITUDEDOMAIN") == "DEFAULT") {
    if (cube.hasGroup("Mapping")) {
      PvlGroup &mapping = cube.group("Mapping");
      PvlKeyword &lond = mapping.findKeyword("LONGITUDEDOMAIN");
      if (lond[0] == "180") {
        convertTo180 = true;
      }
    }
  }
  // If the user wants -180 to 180 domain then we need to convert it
  else if (ui.GetString("LONGITUDEDOMAIN") == "180") {
    convertTo180 = true;
  }

  if (convertTo180) {
    mPolygon = PolygonTools::To180(mPolygon);
  }

  // Get the output gml file name
  QString outgml;
  if (ui.WasEntered("TO")) {
    outgml = ui.GetFileName("TO").toStdString();
    FileName out = ui.GetFileName("TO").toStdString();
    if(out.extension() == "") {
      outgml += ".gml";
    }
  }
  else {
    FileName inputFile = ui.GetCubeName("FROM").toStdString();
    outgml = inputFile.removeExtension().addExtension("gml").expanded();
  }

  // Get the output xsd file name
  QString outxsd = FileName(outgml).removeExtension().addExtension("xsd").expanded();

  // Convert the polygon to GML
  QString polyString;

  if (ui.WasEntered("LABEL")) {
    QString fid = ui.GetString("LABEL");
    polyString = PolygonTools::ToGML(mPolygon, fid, FileName(outxsd).name());
  }
  else {
    polyString = PolygonTools::ToGML(mPolygon, "0", outxsd);
  }

  // Write the gml file.
  ofstream fout;
  fout.open(outgml.toLatin1().data());
  fout << polyString << endl;
  fout.close();

  // Write the xsd file.
  fout.open(outxsd.toLatin1().data());
  fout << PolygonTools::GMLSchema() << endl;
  fout.close();
}

