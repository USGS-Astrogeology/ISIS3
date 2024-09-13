/** This is free and unencumbered software released into the public domain.
The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */
#include <iostream>

#include <geos/geom/CoordinateSequence.h>
#include <geos/geom/Polygon.h>
#include <geos/geom/MultiPolygon.h>
#include <geos/util/GEOSException.h>

#include "ImageOverlap.h"
#include "IString.h"
#include "PolygonTools.h"
#include "Preference.h"

using namespace Isis;
using namespace std;

int main() {
  Preference::Preferences(true);
  void PrintImageOverlap(ImageOverlap & poi);

  {
    cout << "One ----------------------------------------------" << endl;

    // Create coordinate sequence so we can later create a multi polygon
    geos::geom::CoordinateSequence pts;
    pts.add(geos::geom::Coordinate(0, 0));
    pts.add(geos::geom::Coordinate(0, 10));
    pts.add(geos::geom::Coordinate(5, 15));
    pts.add(geos::geom::Coordinate(8, -5));
    pts.add(geos::geom::Coordinate(0, 0));

    // Write the original coordinates to the screen
    cout << "Original coordinates:" << endl;
    cout << pts.toString() << endl;

    // Create a multi polygon
    vector <const geos::geom::Geometry *> polys;
    polys.push_back(globalFactory->createPolygon(
                      globalFactory->createLinearRing(pts)).release());
    geos::geom::MultiPolygon *mPolygon = globalFactory->createMultiPolygon(polys).release();

    // Add more coordinates so we can make sure we did a deep copy when we
    // created the multipolygon
    pts.add(geos::geom::Coordinate(-1, -1));
    pts.add(geos::geom::Coordinate(-2, -2));

    ImageOverlap a("idOne", *mPolygon);

    PrintImageOverlap(a);

    // Add some serial numbers and print it again
    cout << "Two ----------------------------------------------" << endl;
    QString st = "sn2";
    a.Add(st);
    st = "sn3";
    a.Add(st);
    st = "sn4";
    a.Add(st);

    PrintImageOverlap(a);

    // Create a new multipolygon and test the SetPolygon method
    cout << "Three --------------------------------------------" << endl;
    geos::geom::CoordinateSequence pts3;
    pts3.add(geos::geom::Coordinate(0.123456789, 0.123456789));
    pts3.add(geos::geom::Coordinate(0.123456789, 10.123456789));
    pts3.add(geos::geom::Coordinate(5.123456789, 15.123456789));
    pts3.add(geos::geom::Coordinate(8.123456789, -5.123456789));
    pts3.add(geos::geom::Coordinate(0.123456789, 0.123456789));

    try {
      vector <const geos::geom::Geometry *> polys3;
      polys3.push_back(globalFactory->createPolygon(
                         globalFactory->createLinearRing(pts3)).release());
      geos::geom::MultiPolygon *mPolygon3 = globalFactory->createMultiPolygon(polys3).release();
      a.SetPolygon(*mPolygon3);
      delete mPolygon3;
    }
    catch(geos::util::GEOSException *exc) {
      cout << "GEOS Exception: " << exc->what() << endl;
      delete exc;
    }

    PrintImageOverlap(a);


    geos::geom::CoordinateSequence pts4;
    pts4.add(geos::geom::Coordinate(10.123456789, 10.123456789));
    pts4.add(geos::geom::Coordinate(10.123456789, 110.123456789));
    pts4.add(geos::geom::Coordinate(15.123456789, 115.123456789));
    pts4.add(geos::geom::Coordinate(18.123456789, -15.123456789));
    pts4.add(geos::geom::Coordinate(10.123456789, 10.123456789));

    try {
      vector<const geos::geom::Geometry *> polys4;
      polys4.push_back(globalFactory->createPolygon(
                         globalFactory->createLinearRing(pts4)).release());
      geos::geom::MultiPolygon *mPolygon4 = globalFactory->createMultiPolygon(polys4).release();
      a.SetPolygon(mPolygon4);
      delete mPolygon4;
    }
    catch(geos::util::GEOSException *exc) {
      cout << "GEOS Exception: " << exc->what() << endl;
      delete exc;
    }

    PrintImageOverlap(a);


  }

  {
    //
    cout << "Four ---------------------------------------------" << endl;
    geos::geom::CoordinateSequence pts;
    pts.add(geos::geom::Coordinate(0.123456789, 0.123456789));
    pts.add(geos::geom::Coordinate(0.123456789, 10.123456789));
    pts.add(geos::geom::Coordinate(5.123456789, 15.123456789));
    pts.add(geos::geom::Coordinate(8.123456789, -5.123456789));
    pts.add(geos::geom::Coordinate(0.123456789, 0.123456789));

    try {
      vector <const geos::geom::Geometry *> polys;
      polys.push_back(globalFactory->createPolygon(
                        globalFactory->createLinearRing(pts)).release());

      geos::geom::MultiPolygon *mPolygon = globalFactory->createMultiPolygon(polys).release();

      ImageOverlap a("idFour", *mPolygon);
      PrintImageOverlap(a);
    }
    catch(geos::util::GEOSException *exc) {
      cout << "GEOS Exception: " << exc->what() << endl;
      delete exc;
    }
  }

}

// Print a ImageOverlap
void PrintImageOverlap(ImageOverlap &poi) {

  // Write the wkt version of the multi polygon to the screen
  const geos::geom::MultiPolygon *mp = poi.Polygon();
  cout << "Well Known Text version of the multi polygon" << endl;
  cout << mp->toString() << endl << endl;

  cout << "Area of the polygon " << poi.Area() << endl;
  cout << "Number of serial numbers: " << poi.Size() << endl;;
  cout << "Serial numbers: " << endl;
  for(int i = 0; i < poi.Size(); i++) {
    cout << "  " << poi[i].toStdString() << endl;
  }
  cout << endl;
  return;
}
