#include "footprintinit.h"

#include "Application.h"
#include "IException.h"
#include "ImagePolygon.h"
#include "PolygonTools.h"
#include "Process.h"
#include "Progress.h"
#include "PvlGroup.h"
#include "SerialNumber.h"
#include "Target.h"
#include "TProjection.h"


using namespace std;

namespace Isis {

  void footprintinit(UserInterface &ui, Pvl *log) {
    Cube cube;
    cube.open(ui.GetCubeName("FROM"), "rw");

    footprintinit(&cube, ui, log);
    cube.close();
  }

  void footprintinit(Cube *cube, UserInterface &ui, Pvl *log) {
    bool testXY = ui.GetBoolean("TESTXY");

    // Make sure cube has been run through spiceinit
    try {
      cube->camera();
    }
    catch (IException &e) {
      if (!cube->projection()) {
        string msg = "Spiceinit must be run before initializing the polygon";
        throw IException(e, IException::User, msg, _FILEINFO_);
      }
      testXY = false;
    }

    Progress prog;
    prog.SetMaximumSteps(1);
    prog.CheckStatus();

    QString sn = SerialNumber::Compose(*cube);

    ImagePolygon poly;
    if (ui.WasEntered("MAXEMISSION")) {
      poly.Emission(ui.GetDouble("MAXEMISSION"));
    }
    if (ui.WasEntered("MAXINCIDENCE")) {
      poly.Incidence(ui.GetDouble("MAXINCIDENCE"));
    }
    if (ui.GetString("LIMBTEST") == "ELLIPSOID") {
      poly.EllipsoidLimb(true);
    }

    int sinc = 1;
    int linc = 1;
    IString incType = ui.GetString("INCTYPE");
    if (incType.UpCase() == "VERTICES") {
      poly.initCube(*cube);
      sinc = linc = (int)(0.5 + (((poly.validSampleDim() * 2) +
                         (poly.validLineDim() * 2) - 3.0) /
                         ui.GetInteger("NUMVERTICES")));
      if (sinc < 1.0 || linc < 1.0)
        sinc = linc = 1.0;
    }
    else if (incType.UpCase() == "LINCSINC"){
      sinc = ui.GetInteger("SINC");
      linc = ui.GetInteger("LINC");
    }
    else {
      string msg = "Invalid INCTYPE option[" + incType + "]";
      throw IException(IException::Programmer, msg, _FILEINFO_);
    }

    bool precision = ui.GetBoolean("INCREASEPRECISION");
    try {
      poly.Create(*cube, sinc, linc, 1, 1, 0, 0, 1, precision);
    }
    catch (IException &e) {
      QString msg = "Cannot generate polygon for [" + cube->fileName() + "]";
      throw IException(e, IException::User, msg, _FILEINFO_);
    }

    if (testXY) {
      Pvl map(ui.GetFileName("MAP"));
      PvlGroup &mapGroup = map.findGroup("MAPPING");

      // This call adds TargetName, EquatorialRadius and PolarRadius to mapGroup
      mapGroup = Target::radiiGroup(*(cube->label()), mapGroup);
      // add/replace the rest of the keywords
      mapGroup.addKeyword( PvlKeyword("LatitudeType", "Planetocentric"),
                           PvlContainer::Replace );
      mapGroup.addKeyword( PvlKeyword("LongitudeDirection", "PositiveEast"),
                           PvlContainer::Replace );
      mapGroup.addKeyword( PvlKeyword("LongitudeDomain", "360"),
                           PvlContainer::Replace );
      mapGroup.addKeyword( PvlKeyword("CenterLatitude", "0.0"),
                           PvlContainer::Replace );
      mapGroup.addKeyword( PvlKeyword("CenterLongitude", "0.0"),
                           PvlContainer::Replace);

      sinc = poly.getSinc();
      linc = poly.getLinc();
      bool polygonGenerated = false;
      while (!polygonGenerated) {
        TProjection *proj = NULL;
        geos::geom::MultiPolygon *xyPoly = NULL;

        try {
          proj = (TProjection *) ProjectionFactory::Create(map, true);
          xyPoly = PolygonTools::LatLonToXY(*poly.Polys(), proj);

          polygonGenerated = true;
        }
        catch (IException &e) {
          if (precision && sinc > 1 && linc > 1) {
            sinc = sinc * 2 / 3;
            linc = linc * 2 / 3;
            poly.Create(*cube, sinc, linc);
          }
          else {
            delete proj;
            delete xyPoly;
            e.print(); // This should be a NAIF error
            QString msg = "Cannot calculate XY for [";
            msg += cube->fileName() + "]";
            throw IException(e, IException::User, msg, _FILEINFO_);
          }
        }

        delete proj;
        delete xyPoly;
      }
    }

    cube->deleteBlob(sn, "Polygon");
    cube->write(poly);

    if (precision) {
      PvlGroup results("Results");
      results.addKeyword(PvlKeyword("SINC", toString(sinc)));
      results.addKeyword(PvlKeyword("LINC", toString(linc)));
      Application::AppendAndLog(results, log);
    }

    Process p;
    p.SetInputCube(cube);
    p.WriteHistory(*cube);

    prog.CheckStatus();
  }
}
