/** This is free and unencumbered software released into the public domain.
The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */
#include <geos/geom/CoordinateSequence.h>
#include <geos/geom/LinearRing.h>
#include <geos/geom/Polygon.h>
#include <geos/util/GEOSException.h>

#include <QDebug>

#include "Cube.h"
#include "FileName.h"
#include "IException.h"
#include "PolygonTools.h"
#include "Projection.h"
#include "ProjectionFactory.h"
#include "Preference.h"
#include "UniversalGroundMap.h"
#include "TProjection.h"

using namespace std;
using namespace Isis;

int main() {
  Isis::Preference::Preferences(true);

  try {
    cout << "Unit test for PolygonTools" << endl << endl;

    // Create coordinate sequence for the first of two polygons
    geos::geom::CoordinateSequence pts;
    pts.add(geos::geom::Coordinate(0.0, 0.0));
    pts.add(geos::geom::Coordinate(0.0, 1.0));
    pts.add(geos::geom::Coordinate(1.0, 1.0));
    pts.add(geos::geom::Coordinate(1.0, 0.0));
    pts.add(geos::geom::Coordinate(0.0, 0.0));
    cout << "Coordinates of polygon 1:" << pts.toString() << endl << endl;

    // Create the first polygon
    vector <const geos::geom::Geometry *> polys;
    polys.push_back(Isis::globalFactory->createPolygon(
                      Isis::globalFactory->createLinearRing(pts)).release());

    // Create coordinate sequence for the second of two polygons
    geos::geom::CoordinateSequence pts2;
    pts2.add(geos::geom::Coordinate(360.0, 1.0));
    pts2.add(geos::geom::Coordinate(359.0, 1.0));
    pts2.add(geos::geom::Coordinate(359.0, 0.0));
    pts2.add(geos::geom::Coordinate(360.0, 0.0));
    pts2.add(geos::geom::Coordinate(360.0, 1.0));
    cout << "Coordinates of polygon 2:" << pts2.toString() << endl << endl;

    // Create coordinate sequence for the hole in the second polygon
    geos::geom::CoordinateSequence pts3;
    pts3.add(geos::geom::Coordinate(359.75, 0.75));
    pts3.add(geos::geom::Coordinate(359.25, 0.75));
    pts3.add(geos::geom::Coordinate(359.25, 0.25));
    pts3.add(geos::geom::Coordinate(359.75, 0.25));
    pts3.add(geos::geom::Coordinate(359.75, 0.75));
    cout << "Coordinates of hole for polygon 2:" << pts3.toString() << endl << endl;

    vector<std::unique_ptr<geos::geom::LinearRing>> hole2;
    hole2.push_back(Isis::globalFactory->createLinearRing(pts3));

    // Create the second polygon
    polys.push_back(Isis::globalFactory->createPolygon(
                    Isis::globalFactory->createLinearRing(pts2), std::move(hole2)).release());

    // Create a multipolygon from the two polygons
    geos::geom::MultiPolygon *mPolygon = Isis::globalFactory->createMultiPolygon(polys).release();

    // Create a copy of the multipolygon
    geos::geom::MultiPolygon *tmpMp = PolygonTools::CopyMultiPolygon(mPolygon);
    cout << "Copy of the multipolygon = " << tmpMp->toString() << endl << endl;

    Isis::Pvl lab;
    lab.addGroup(Isis::PvlGroup("Mapping"));
    Isis::PvlGroup &mapGroup = lab.findGroup("Mapping");
    mapGroup += Isis::PvlKeyword("EquatorialRadius", "1.0");
    mapGroup += Isis::PvlKeyword("PolarRadius", "1.0");
    mapGroup += Isis::PvlKeyword("LatitudeType", "Planetocentric");
    mapGroup += Isis::PvlKeyword("LongitudeDirection", "PositiveEast");
    mapGroup += Isis::PvlKeyword("LongitudeDomain", "360");
    mapGroup += Isis::PvlKeyword("CenterLatitude", "0");
    mapGroup += Isis::PvlKeyword("CenterLongitude", "0");
    mapGroup += Isis::PvlKeyword("ProjectionName", "Sinusoidal");

    TProjection *proj = (TProjection *) ProjectionFactory::Create(lab);

    cout << "Lon/Lat polygon = " << mPolygon->toString() << endl << endl;

    cout << "X/Y polygon radius (1) = "
         << (PolygonTools::LatLonToXY(*mPolygon, proj))->toString() << endl << endl;

    delete proj;

    // Change the radius and set a new projection
    // Then get the XYPolygon again
    mapGroup.deleteKeyword("EquatorialRadius");
    mapGroup += Isis::PvlKeyword("EquatorialRadius", "10.0");
    mapGroup.deleteKeyword("PolarRadius");
    mapGroup += Isis::PvlKeyword("PolarRadius", "10.0");

    proj = (TProjection *) ProjectionFactory::Create(lab);
    cout << "X/Y polygon radius (10) = "
         << (PolygonTools::LatLonToXY(*mPolygon, proj))->toString() << endl << endl;

    // Convert a Lon/Lat poly to X/Y and then back to Lon/Lat
    cout << "Lat/Lon polygon from X/Y with radius (10) = "
         << PolygonTools::XYToLatLon(*PolygonTools::LatLonToXY(*mPolygon, proj), proj)->toString()
         << endl << endl;

    // Create a UniversalGroundMap so we can test the SampleLinePolygon stuff
    Cube cube(FileName("$ISISTESTDATA/isis/src/base/unitTestData/PolygonTools/unitTest.lbl").expanded(), "r");
    UniversalGroundMap ugm = UniversalGroundMap(cube);

    // Create coordinate sequence for the first of two polygons
    geos::geom::CoordinateSequence llpts;
    ugm.SetImage(1.0, 1.0);
    llpts.add(geos::geom::Coordinate(
          qRound(ugm.UniversalLongitude()),
          qRound(ugm.UniversalLatitude())));
    ugm.SetImage(1204.0, 1.0);
    llpts.add(geos::geom::Coordinate(
          qRound(ugm.UniversalLongitude()),
          qRound(ugm.UniversalLatitude())));
    ugm.SetImage(1204.0, 1056.0);
    llpts.add(geos::geom::Coordinate(
          qRound(ugm.UniversalLongitude()),
          qRound(ugm.UniversalLatitude())));
    ugm.SetImage(1.0, 1056.0);
    llpts.add(geos::geom::Coordinate(
          qRound(ugm.UniversalLongitude()),
          qRound(ugm.UniversalLatitude())));
    ugm.SetImage(1.0, 1.0);
    llpts.add(geos::geom::Coordinate(
          qRound(ugm.UniversalLongitude()),
          qRound(ugm.UniversalLatitude())));
    cout << "Coordinates of Lon/Lat polygon:" << llpts.toString() << endl << endl;

    // Create the L/L polygon
    vector<const geos::geom::Geometry *> llpolys;
    llpolys.push_back(Isis::globalFactory->createPolygon(
                        Isis::globalFactory->createLinearRing(llpts)).release());

    geos::geom::MultiPolygon *llmPolygon = Isis::globalFactory->createMultiPolygon(llpolys).release();

    geos::geom::MultiPolygon *slmPolygon = PolygonTools::LatLonToSampleLine(*llmPolygon, &ugm);
    cout << "Coordinates of Sample/Line polygon:" <<
        PolygonTools::ReducePrecision(slmPolygon, 2)->toString() << endl;

    cout << endl << "Testing LatLonToSampleLine() with coords outside of the valid range." << endl;
    // Create coordinate sequence for the first of two polygons
    geos::geom::CoordinateSequence llpts2;
    llpts2.add(geos::geom::Coordinate(175, 0));
    llpts2.add(geos::geom::Coordinate(175.6, -28));
    llpts2.add(geos::geom::Coordinate(181.5, -30.6));
    llpts2.add(geos::geom::Coordinate(186.8, -27));
    llpts2.add(geos::geom::Coordinate(183.7, -16.6));
    llpts2.add(geos::geom::Coordinate(175, 0));

    // Create the L/L polygon
    vector<const geos::geom::Geometry *> llpolys2;
    llpolys2.push_back(Isis::globalFactory->createPolygon(
                        Isis::globalFactory->createLinearRing(llpts2)).release());

    geos::geom::MultiPolygon *llmPolygon2 = Isis::globalFactory->createMultiPolygon(llpolys2).release();

    geos::geom::MultiPolygon *slmPolygon2 = PolygonTools::LatLonToSampleLine(*llmPolygon2, &ugm);
    cout << "Coordinates of Sample/Line polygon:" <<
        PolygonTools::ReducePrecision(slmPolygon2, 2)->toString() << endl;

    cout << endl;

    cout << "Well Knowen Text Polygon:" << endl;
    std::cout << mPolygon->toString() << std::endl;

    cout << endl;

    cout << "GML Ploygon:" << endl;
    QString GMLpolygon = PolygonTools::ToGML(mPolygon, "test");
    cout << GMLpolygon.toStdString() << endl;

    cout << "GML Thickness:" << endl;
    double th = PolygonTools::Thickness(mPolygon);
    cout << IString(th) << endl;

    cout << endl << endl;

    cout << "Testing Despike" << endl;
    pts.clear();
    pts.add(geos::geom::Coordinate(1.0, 1.0));
    pts.add(geos::geom::Coordinate(5.0, 1.0));
    pts.add(geos::geom::Coordinate(5.00000000001, -10.0));
    pts.add(geos::geom::Coordinate(5.00000000001, 5.0));
    pts.add(geos::geom::Coordinate(1.0, 5.0));
    pts.add(geos::geom::Coordinate(1.0, 1.0));
    cout << "Input: " << Isis::globalFactory->createLinearRing(pts)->toString() << endl;
    cout << "Output: " << PolygonTools::Despike(Isis::globalFactory->createLinearRing(pts).release())->toString() << endl;

    cout << endl << endl;

    cout << "Testing FixGeometry" << endl;
    pts.clear();
    pts.add(geos::geom::Coordinate(1.0, 1.0));
    pts.add(geos::geom::Coordinate(5.0, 1.0));
    pts.add(geos::geom::Coordinate(5.0, 5.0));
    pts.add(geos::geom::Coordinate(5.00000000000001, 5.0));
    pts.add(geos::geom::Coordinate(1.0, 5.0));
    pts.add(geos::geom::Coordinate(1.0, 1.0));
    cout << "Input: " << Isis::globalFactory->createLinearRing(pts)->toString() << endl;
    cout << "Output: " << PolygonTools::Despike(Isis::globalFactory->createLinearRing(pts).release())->toString() << endl;

    cout << endl << endl;

    cout << "Testing Equal" << endl;
    pts.clear();
    pts.add(geos::geom::Coordinate(1.0, 1.0));
    pts.add(geos::geom::Coordinate(5.0, 1.0));
    pts.add(geos::geom::Coordinate(5.0, 5.0));
    pts.add(geos::geom::Coordinate(1.0, 5.0));
    pts.add(geos::geom::Coordinate(1.0, 1.0));
    geos::geom::Polygon *poly1 = Isis::globalFactory->createPolygon(Isis::globalFactory->createLinearRing(pts)).release();
    cout << "Same Poly Equal?                     " << PolygonTools::Equal(poly1, poly1) << " - " << poly1->equals(poly1) << endl;
    pts2.clear();
    pts2.add(geos::geom::Coordinate(5.0, 1.0));
    pts2.add(geos::geom::Coordinate(5.0, 5.0));
    pts2.add(geos::geom::Coordinate(1.0, 5.0));
    pts2.add(geos::geom::Coordinate(1.0, 1.0));
    pts2.add(geos::geom::Coordinate(5.0, 1.0));
    geos::geom::Polygon *poly2 = Isis::globalFactory->createPolygon(Isis::globalFactory->createLinearRing(pts2)).release();
    cout << "Rearranged Poly Equal?               " << PolygonTools::Equal(poly1, poly2) << " - " << poly1->equals(poly2) << endl;
    pts2.clear();
    pts2.add(geos::geom::Coordinate(5.0, 1.0));
    pts2.add(geos::geom::Coordinate(5.0, 5.0));
    pts2.add(geos::geom::Coordinate(1.000000000000001, 5.0));
    pts2.add(geos::geom::Coordinate(1.0, 1.0));
    pts2.add(geos::geom::Coordinate(5.0, 1.0));
    poly2 = Isis::globalFactory->createPolygon(Isis::globalFactory->createLinearRing(pts2)).release();
    cout << "Past 15 Places Equal?                " << PolygonTools::Equal(poly1, poly2) << " - " << poly1->equals(poly2) << endl;
    pts2.clear();
    pts2.add(geos::geom::Coordinate(5.0, 1.0));
    pts2.add(geos::geom::Coordinate(5.0, 5.0));
    pts2.add(geos::geom::Coordinate(1.00000000000001, 5.0));
    pts2.add(geos::geom::Coordinate(1.0, 1.0));
    pts2.add(geos::geom::Coordinate(5.0, 1.0));
    poly2 = Isis::globalFactory->createPolygon(Isis::globalFactory->createLinearRing(pts2)).release();
    cout << "At 15 Place Difference Equal?        " << PolygonTools::Equal(poly1, poly2) << " - " << poly1->equals(poly2) << endl;
    pts2.clear();
    pts2.add(geos::geom::Coordinate(5.0, 1.0));
    pts2.add(geos::geom::Coordinate(5.0, 5.0));
    pts2.add(geos::geom::Coordinate(1.0000000000001, 5.0));
    pts2.add(geos::geom::Coordinate(1.0, 1.0));
    pts2.add(geos::geom::Coordinate(5.0, 1.0));
    poly2 = Isis::globalFactory->createPolygon(Isis::globalFactory->createLinearRing(pts2)).release();
    cout << "Significantly Different Equal?       " << PolygonTools::Equal(poly1, poly2) << " - " << poly1->equals(poly2) << endl;

    return 0;
  }
  catch(Isis::IException &e) {
    cout << "ERROR " << e.what() << endl;
    e.print();
  }
  catch(geos::util::GEOSException *exc) {
    cout << "GEOS Exception: " << exc->what() << endl;
    delete exc;
  }
  catch(std::exception const &se) {
    cout << "std::exception " << se.what() << endl;
  }
  catch(...) {
    cout << " Other error" << endl;
  }
}
