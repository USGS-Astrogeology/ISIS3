/** This is free and unencumbered software released into the public domain.
The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */
#include <iostream>
#include <cstdlib>
#include <cmath>
#include <iomanip>

#include "geos/geom/CoordinateSequence.h"
#include "geos/geom/Geometry.h"
#include "geos/geom/Polygon.h"

#include "IException.h"
#include "PolygonTools.h"
#include "PolygonSeeder.h"
#include "PolygonSeederFactory.h"
#include "Pvl.h"
#include "PvlGroup.h"
#include "ProjectionFactory.h"
#include "GridPolygonSeeder.h"
#include "Preference.h"
#include "Target.h"
#include "TProjection.h"

using namespace std;
using namespace Isis;

int main() {
  Isis::Preference::Preferences(true);
  try {
    cout << "Test 1, create a seeder" << endl;

    PvlGroup alg("PolygonSeederAlgorithm");

    do {
      if(!alg.hasKeyword("Name")) {
        cout << "Test without subgrid" << endl;
        alg += PvlKeyword("Name", "Grid");
        alg += PvlKeyword("MinimumThickness", toString(0.3));
        alg += PvlKeyword("MinimumArea", toString(10));
        alg += PvlKeyword("XSpacing", toString(1500));
        alg += PvlKeyword("YSpacing", toString(1500));
      }
      else {
        cout << "Test with subgrid" << endl;
        alg += PvlKeyword("SubGrid", toString(true));
      }

      PvlObject o("AutoSeed");
      o.addGroup(alg);

      Pvl pvl;
      pvl.addObject(o);
      cout << pvl << endl << endl;

      PolygonSeeder *ps = PolygonSeederFactory::Create(pvl);

      std::cout << "Test to make sure Parse did it's job" << std::endl;
      std::cout << "MinimumThickness = " << ps->MinimumThickness() << std::endl;
      std::cout << "MinimumArea = " << ps->MinimumArea() << std::endl;
      std::cout << "SubGrid = " << ((GridPolygonSeeder *)ps)->SubGrid() << std::endl;
      //    std::cout << "Spaceing = " << ((GridPolygonSeeder*)ps)->Spacing() << std::endl;


      cout << "Test 2, test a square polygon" << endl;
      try {
        // Call the seed member with a polygon
        geos::geom::CoordinateSequence pts;
        vector<const geos::geom::Geometry *> polys;

        // Create the A polygon
        pts.add(geos::geom::Coordinate(0, 0));
        pts.add(geos::geom::Coordinate(0, 0.5));
        pts.add(geos::geom::Coordinate(0.5, 0.5));
        pts.add(geos::geom::Coordinate(0.5, 0));
        pts.add(geos::geom::Coordinate(0, 0));

        polys.push_back(Isis::globalFactory->createPolygon(
                        Isis::globalFactory->createLinearRing(pts)).release());

        geos::geom::MultiPolygon *mp = Isis::globalFactory->createMultiPolygon(polys).release();

        cout << "Lon/Lat polygon = " << mp->toString() << endl;
        // Create the projection necessary for seeding
        PvlGroup radii = Target::radiiGroup("MARS");
        Isis::Pvl maplab;
        maplab.addGroup(Isis::PvlGroup("Mapping"));
        Isis::PvlGroup &mapGroup = maplab.findGroup("Mapping");
        mapGroup += Isis::PvlKeyword("EquatorialRadius", radii["EquatorialRadius"]);
        mapGroup += Isis::PvlKeyword("PolarRadius", radii["PolarRadius"]);
        mapGroup += Isis::PvlKeyword("LatitudeType", "Planetocentric");
        mapGroup += Isis::PvlKeyword("LongitudeDirection", "PositiveEast");
        mapGroup += Isis::PvlKeyword("LongitudeDomain", toString(360));
        mapGroup += Isis::PvlKeyword("CenterLatitude", toString(0));
        mapGroup += Isis::PvlKeyword("CenterLongitude", toString(0));
        mapGroup += Isis::PvlKeyword("ProjectionName", "Sinusoidal");

        TProjection *proj = (TProjection *) Isis::ProjectionFactory::Create(maplab);

        geos::geom::MultiPolygon *xymp = PolygonTools::LatLonToXY(*mp, proj);
        vector<geos::geom::Point *> seedValues = ps->Seed(xymp);

        vector<geos::geom::Point *> points;
        for(unsigned int pt = 0; pt < seedValues.size(); pt ++) {
          if(proj->SetCoordinate(seedValues[pt]->getX(), seedValues[pt]->getY())) {
            points.push_back(Isis::globalFactory->createPoint(
                               geos::geom::Coordinate(proj->UniversalLongitude(),
                                   proj->UniversalLatitude())).release());
          }
          else {
            IString msg = "Unable to convert to a (lon,lat)";
            throw IException(IException::Programmer, msg, _FILEINFO_);
          }
        }

        cout << setprecision(13);
        for(unsigned int i = 0; i < points.size(); i++) {
          cout << "  POINT (";
          cout << points[i]->getX() << " " << points[i]->getY() << ")" << endl;
        }
      }
      catch(IException &e) {
        e.print();
      }

      cout << "Test 3, test for too thin" << endl;
      try {
        // Call the seed member with a polygon
        geos::geom::CoordinateSequence pts;
        vector <const geos::geom::Geometry *> polys;

        // Create the A polygon
        pts.add(geos::geom::Coordinate(0, 0));
        pts.add(geos::geom::Coordinate(0, 0.5));
        pts.add(geos::geom::Coordinate(0.0125, 0.5));
        pts.add(geos::geom::Coordinate(0.0125, 0));
        pts.add(geos::geom::Coordinate(0, 0));

        polys.push_back(Isis::globalFactory->createPolygon(
                         Isis::globalFactory->createLinearRing(pts)).release());

        geos::geom::MultiPolygon *mp = Isis::globalFactory->createMultiPolygon(polys).release();

        cout << "Lon/Lat polygon = " << mp->toString() << endl;

        // Create the projection necessary for seeding
        PvlGroup radii = Target::radiiGroup("MARS");
        Isis::Pvl maplab;
        maplab.addGroup(Isis::PvlGroup("Mapping"));
        Isis::PvlGroup &mapGroup = maplab.findGroup("Mapping");
        mapGroup += Isis::PvlKeyword("EquatorialRadius", radii["EquatorialRadius"]);
        mapGroup += Isis::PvlKeyword("PolarRadius", radii["PolarRadius"]);
        mapGroup += Isis::PvlKeyword("LatitudeType", "Planetocentric");
        mapGroup += Isis::PvlKeyword("LongitudeDirection", "PositiveEast");
        mapGroup += Isis::PvlKeyword("LongitudeDomain", toString(360));
        mapGroup += Isis::PvlKeyword("CenterLatitude", toString(0));
        mapGroup += Isis::PvlKeyword("CenterLongitude", toString(0));
        mapGroup += Isis::PvlKeyword("ProjectionName", "Sinusoidal");
        TProjection *proj = (TProjection *) Isis::ProjectionFactory::Create(maplab);

        // NOTHING SHOULD BE PRINTED (the thickness test should not have been met)
        geos::geom::MultiPolygon *xymp = PolygonTools::LatLonToXY(*mp, proj);
        vector<geos::geom::Point *> seedValues = ps->Seed(xymp);
        for(unsigned int i = 0; i < seedValues.size(); i++) {
          cout << "Point(" << i << ") = " << seedValues[i]->toString() << endl;
        }
      }
      catch(IException &e) {
        e.print();
      }
    }
    while(!alg.hasKeyword("SubGrid"));
  }
  catch(IException &e) {
    e.print();
  }

  return 0;
}
