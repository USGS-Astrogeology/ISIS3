/** This is free and unencumbered software released into the public domain.
The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */

#include <string>
#include <iostream>
#include <sstream>
#include <iomanip>
#include <vector>
#include <cmath>

#include <QDebug>

#include "geos/geom/CoordinateArraySequence.h"
#include "geos/geom/CoordinateSequence.h"
#include "geos/geom/LinearRing.h"
#include "geos/geom/Point.h"
#include "geos/geom/Polygon.h"
#include "geos/operation/distance/DistanceOp.h"
#include "geos/operation/overlay/OverlayOp.h"
#include "geos/operation/overlay/snap/GeometrySnapper.h"

#include "SpecialPixel.h"
#include "PolygonTools.h"
#include "TProjection.h"
#include "ProjectionFactory.h"
#include "UniversalGroundMap.h"

using namespace std;
namespace Isis {

  /**
   * This method will return a geos::geom::MultiPolygon which contains the X/Y
   * coordinates of the LonLat polygon. The Lat/Lon polygon must
   * have coordinates (x direction, y direction) or (Lon,Lat).
   *
   * @param lonLatPolygon  A multipolygon in (Lon,Lat) order
   * @param projection     The projection to be used to convert the Lons and Lat
   *                       to X and Y
   *
   * @return  Returns a multipolygon which is the result of converting the input
   *          multipolygon from (Lon,Lat) to (X,Y).
   */
  geos::geom::MultiPolygon *PolygonTools::LatLonToXY(
      const geos::geom::MultiPolygon &lonLatPolygon, TProjection *projection) {
    if (projection == NULL) {
      string msg = "Unable to convert Lon/Lat polygon to X/Y. ";
      msg += "No projection has was supplied";
      throw IException(IException::Programmer, msg, _FILEINFO_);
    }

    // Convert the Lat/Lon poly coordinates to X/Y coordinates
    if(lonLatPolygon.isEmpty()) {
      return globalFactory->createMultiPolygon().release();
    }
    else {
      vector<geos::geom::Geometry *> *xyPolys = new vector<geos::geom::Geometry *>;
      // Convert each polygon in this multi-polygon
      for(unsigned int g = 0; g < lonLatPolygon.getNumGeometries(); ++g) {
        const geos::geom::Polygon *poly =
            dynamic_cast<const geos::geom::Polygon *>(
              lonLatPolygon.getGeometryN(g));

        // Convert each hole inside this polygon
        vector<geos::geom::LinearRing *> *holes = new vector<geos::geom::LinearRing *>;
        for(unsigned int h = 0; h < poly->getNumInteriorRing(); ++h) {
          geos::geom::CoordinateArraySequence *xycoords = new geos::geom::CoordinateArraySequence();
          geos::geom::CoordinateSequence *llcoords =
            poly->getInteriorRingN(h)->getCoordinates().release();

          // Convert each coordinate in this hole
          for(unsigned int cord = 0; cord < llcoords->getSize(); ++cord) {
            projection->SetGround(llcoords->getAt(cord).y,
                                  llcoords->getAt(cord).x);
            xycoords->add(geos::geom::Coordinate(projection->XCoord(),
                                                 projection->YCoord()));
          } // end num coords in hole loop

          geos::geom::LinearRing *hole = globalFactory->createLinearRing(xycoords);

          if(hole->isValid() && !hole->isEmpty()) {
            holes->push_back(hole);
          }
          else {
            delete hole;
          }
        } // end num holes in polygon loop

        // Convert the exterior ring of this polygon
        geos::geom::CoordinateArraySequence *xycoords = new geos::geom::CoordinateArraySequence();
        geos::geom::CoordinateSequence *llcoords =
          poly->getExteriorRing()->getCoordinates().release();

        // Convert each coordinate in the exterior ring of this polygon
        for (unsigned int cord = 0; cord < llcoords->getSize(); ++cord) {
          projection->SetGround(llcoords->getAt(cord).y,
                                llcoords->getAt(cord).x);
          xycoords->add(geos::geom::Coordinate(projection->XCoord(),
                                               projection->YCoord()));
        } // end exterior ring coordinate loop

        geos::geom::Polygon *newPoly = globalFactory->createPolygon(
                                         globalFactory->createLinearRing(xycoords), holes);

        if(!newPoly->isEmpty() && newPoly->getArea() > 1.0e-14) {
          xyPolys->push_back(newPoly);
        }
        else {
          delete newPoly;
        }
      } // end num geometry in multi-poly

      // Create a new multipoly from all the new X/Y polygon(s)
      geos::geom::MultiPolygon *spikedPoly = globalFactory->createMultiPolygon(xyPolys);

      if(spikedPoly->isValid() && !spikedPoly->isEmpty()) {
        return spikedPoly;
      }
      else {
        try {
          geos::geom::MultiPolygon *despikedPoly = Despike(spikedPoly);

          delete spikedPoly;
          spikedPoly = NULL;

          return despikedPoly;
        }
        catch(IException &e) {
          IString msg = "Unable to convert polygon from Lat/Lon to X/Y";
          throw IException(IException::Programmer, msg, _FILEINFO_);
        }
      }
    } // end else
  }


  /**
   * This method will return a geos::geom::MultiPolygon which contains the
   * (Lon,Lat) coordinates of the XY polygon. The Lat/Lon polygon will
   * have coordinates (x direction, y direction) or (Lon,Lat).
   *
   * @param xYPolygon  A multipolygon in (X,Y) order
   * @param projection The projection to be used to convert the Xs and Ys to Lon
   *                   and Lats
   *
   * @return  Returns a multipolygon which is the result of converting the input
   *          multipolygon from (X,Y) to (Lon,Lat).
   */
  geos::geom::MultiPolygon *PolygonTools::XYToLatLon(
    const geos::geom::MultiPolygon &xYPolygon, TProjection *projection) {

    if(projection == NULL) {
      string msg = "Unable to convert X/Y polygon to Lon/Lat. ";
      msg += "No projection was supplied";
      throw IException(IException::Programmer, msg, _FILEINFO_);
    }

    // Convert the X/Y poly coordinates to Lat/Lon coordinates
    if(xYPolygon.isEmpty()) {
      return globalFactory->createMultiPolygon().release();
    }
    else {
      vector<geos::geom::Geometry *> *llPolys = new vector<geos::geom::Geometry *>;
      // Convert each polygon in this multi-polygon
      for(unsigned int g = 0; g < xYPolygon.getNumGeometries(); ++g) {
        const geos::geom::Polygon *poly =
            dynamic_cast<const geos::geom::Polygon *>(
              xYPolygon.getGeometryN(g));

        // Convert each hole inside this polygon
        vector<geos::geom::LinearRing *> *holes = new vector<geos::geom::LinearRing *>;
        for(unsigned int h = 0; h < poly->getNumInteriorRing(); ++h) {
          geos::geom::CoordinateArraySequence *llcoords = new geos::geom::CoordinateArraySequence();
          geos::geom::CoordinateSequence *xycoords =
            poly->getInteriorRingN(h)->getCoordinates().release();

          // Convert each coordinate in this hole
          for(unsigned int cord = 0; cord < xycoords->getSize(); ++cord) {
            projection->SetWorld(xycoords->getAt(cord).x,
                                 xycoords->getAt(cord).y);
            llcoords->add(geos::geom::Coordinate(projection->Longitude(),
                                                 projection->Latitude()));
          } // end num coords in hole loop
          holes->push_back(globalFactory->createLinearRing(llcoords));
        } // end num holes in polygon loop

        // Convert the exterior ring of this polygon
        geos::geom::CoordinateArraySequence *llcoords = new geos::geom::DefaultCoordinateSequence();
        geos::geom::CoordinateSequence *xycoords =
          poly->getExteriorRing()->getCoordinates().release();

        // Convert each coordinate in the exterior ring of this polygon
        for(unsigned int cord = 0; cord < xycoords->getSize(); ++cord) {
          projection->SetWorld(xycoords->getAt(cord).x,
                               xycoords->getAt(cord).y);
          llcoords->add(geos::geom::Coordinate(projection->Longitude(),
                                               projection->Latitude()));
        } // end exterior ring coordinate loop

        llPolys->push_back(globalFactory->createPolygon(
                             globalFactory->createLinearRing(llcoords), holes));
      } // end num geometry in multi-poly


      // Create a new multipoly from all the new Lat/Lon polygon(s)
      geos::geom::MultiPolygon *spikedPoly = globalFactory->createMultiPolygon(llPolys);

      if(spikedPoly->isValid() && !spikedPoly->isEmpty()) {
        return spikedPoly;
      }
      else {
        try {
          geos::geom::MultiPolygon *despikedPoly = Despike(spikedPoly);

          delete spikedPoly;
          spikedPoly = NULL;

          return despikedPoly;
        }
        catch(IException &e) {
          IString msg = "Unable to convert polygon from X/Y to Lat/Lon";
          throw IException(IException::Programmer, msg, _FILEINFO_);
        }
      }
    } // end else
  }


  /**
   * This method will return a geos::geom::MultiPolygon which contains the
   * sample/line coordinates of the Lat/Lon polygon. The Lat/Lon polygon must
   * have coordinates (x direction, y direction) or (Lon,Lat).
   *
   * @param lonLatPolygon  A multipolygon in (Lon,Lat order)
   * @param ugm            The UniversalGroundMap to be used to convert the Lons
   *                       and Lat to Samples and Lines
   *
   * @return  Returns a multipolygon which is the result of converting the input
   *          multipolygon from (Lon,Lat) to (Sample,Line).
   */
  geos::geom::MultiPolygon *PolygonTools::LatLonToSampleLine(
    const geos::geom::MultiPolygon &lonLatPolygon, UniversalGroundMap *ugm) {

    if(ugm == NULL) {
      string msg = "Unable to convert Lon/Lat polygon to Sample/Line. ";
      msg += "No UniversalGroundMap was supplied";
      throw IException(IException::Programmer, msg, _FILEINFO_);
    }

    // Convert the Lon/Lat poly coordinates to Sample/Line coordinates
    if(lonLatPolygon.isEmpty()) {
      return globalFactory->createMultiPolygon().release();
    }
    else {
      vector<geos::geom::Geometry *> *slPolys = new vector<geos::geom::Geometry *>;
      // Convert each polygon in this multi-polygon
      for(unsigned int g = 0; g < lonLatPolygon.getNumGeometries(); g++) {
        const geos::geom::Polygon *poly =
            dynamic_cast<const geos::geom::Polygon *>(
              lonLatPolygon.getGeometryN(g));

        // Convert each hole inside this polygon
        vector<geos::geom::LinearRing *> *holes = new vector<geos::geom::LinearRing *>;
        for(unsigned int h = 0; h < poly->getNumInteriorRing(); ++h) {
          geos::geom::CoordinateArraySequence *slcoords = new geos::geom::DefaultCoordinateSequence();
          geos::geom::CoordinateSequence *llcoords =
            poly->getInteriorRingN(h)->getCoordinates().release();

          // Convert each coordinate in this hole
          for(unsigned int cord = 0; cord < llcoords->getSize(); ++cord) {
            ugm->SetUniversalGround(llcoords->getAt(cord).y,
                                    llcoords->getAt(cord).x);
            slcoords->add(geos::geom::Coordinate(ugm->Sample(),
                                                 ugm->Line()));
          } // end num coords in hole loop
          holes->push_back(globalFactory->createLinearRing(slcoords));
          delete slcoords;
          delete llcoords;
        } // end num holes in polygon loop

        // Convert the exterior ring of this polygon
        geos::geom::CoordinateArraySequence *slcoords = new geos::geom::CoordinateArraySequence();
        geos::geom::CoordinateSequence *llcoords =
          poly->getExteriorRing()->getCoordinates().release();

        // Convert each coordinate in the exterior ring of this polygon
        for(unsigned int cord = 0; cord < llcoords->getSize(); ++cord) {
          if (ugm->SetUniversalGround(llcoords->getAt(cord).y,
                                      llcoords->getAt(cord).x)) {
            slcoords->add(geos::geom::Coordinate(ugm->Sample(),
                                                ugm->Line()));
          }
        } // end exterior ring coordinate loop

        // Make sure that the line string is closed.
        if (slcoords->getSize() > 0 && !slcoords->front().equals(slcoords->back())) {
          slcoords->add(slcoords->front());
        }

        try {
          slPolys->push_back(globalFactory->createPolygon(
                              globalFactory->createLinearRing(slcoords), holes));
        }
        catch (std::exception &e) {
          throw IException(IException::Unknown,
                           QObject::tr("Unable to convert polygon from Lat/Lon to Sample/Line. The "
                                       "error given was [%1].").arg(e.what()),
                           _FILEINFO_);
        }

        delete llcoords;
      } // end num geometry in multi-poly

      // Create a new multipoly from all the new Sample/Line polygon(s)
      geos::geom::MultiPolygon *spikedPoly = globalFactory->createMultiPolygon(slPolys);

      if(spikedPoly->isValid() && !spikedPoly->isEmpty()) {
        return spikedPoly;
      }
      else {
        try {
          geos::geom::MultiPolygon *despikedPoly = Despike(spikedPoly);

          delete spikedPoly;
          spikedPoly = NULL;

          return despikedPoly;
        }
        catch (IException &e) {
          IString msg = "Unable to convert polygon from Lat/Lon to Sample/Line";
          throw IException(IException::Programmer, msg, _FILEINFO_);
        }
      }
    } // end else
  }


  /**
   * This static method will create a deep copy of a geos::geom::MultiPolygon. The
   * caller assumes responsibility for the memory associated with the new
   * polygon.
   *
   * @param mpolygon The multipolygon to be copied.
   *
   * @return  Returns a pointer to a multipolygon which is a deep copy of the
   *          input multipolygon. This is necessary because at the time of
   *          writing the geos package does not create multipolygons when
   *          copying. It produdes geometryCollections
   */
  geos::geom::MultiPolygon *PolygonTools::CopyMultiPolygon(const geos::geom::MultiPolygon *mpolygon) {

    vector<geos::geom::Geometry *> *polys = new vector<geos::geom::Geometry *>;
    for(unsigned int i = 0; i < mpolygon->getNumGeometries(); ++i) {
      polys->push_back((mpolygon->getGeometryN(i))->clone().release());
    }
    return globalFactory->createMultiPolygon(polys);
  }


  /**
   * This static method will create a deep copy of a geos::geom::MultiPolygon. The
   * caller assumes responsibility for the memory associated with the new
   * polygon.
   *
   * @param mpolygon The multipolygon to be copied.
   *
   * @return  Returns a pointer to a multipolygon which is a deep copy of the
   *          input multipolygon. This is necessary because at the time of
   *          writing the geos package does not create multipolygons when
   *          copying. It produdes geometryCollections
   */
  geos::geom::MultiPolygon *PolygonTools::CopyMultiPolygon(const geos::geom::MultiPolygon &mpolygon) {

    vector<geos::geom::Geometry *> *polys = new vector<geos::geom::Geometry *>;
    for(unsigned int i = 0; i < mpolygon.getNumGeometries(); ++i) {
      polys->push_back((mpolygon.getGeometryN(i))->clone().release());
    }
    return globalFactory->createMultiPolygon(polys);
  }


  /**
   * Return the polygon with gml header
   *
   *
   * @param [in] mpolygon Polygon with lat/lon vertices
   * @param idString mpolygon's Id
   * @return QString  Returns the polygon with lon,lat lon,lat format vertices and GML header
   */
  QString PolygonTools::ToGML(const geos::geom::MultiPolygon *mpolygon, QString idString,
                              QString schema) {

    ostringstream os;

    //Write out the GML header
    os << "<?xml version=\"1.0\" encoding=\"utf-8\" ?>" << endl;
    os << "<ogr:FeatureCollection" << endl;
    os << "    xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\"" << endl;
    os << "    xsi:schemaLocation=\"http://ogr.maptools.org/ " << schema << "\"" << endl;
    os << "    xmlns:ogr=\"http://ogr.maptools.org/\"" << endl;
    os << "    xmlns:gml=\"http://www.opengis.net/gml\">" << endl;
    os << "  <gml:boundedBy>" << endl;
    os << "    <gml:Box>" << endl;
    os << "      <gml:coord>";
    os <<          "<gml:X>" <<
                       setprecision(15) << mpolygon->getEnvelopeInternal()->getMinX() <<
                       "</gml:X>";
    os <<          "<gml:Y>" <<
                       setprecision(15) << mpolygon->getEnvelopeInternal()->getMinY() <<
                       "</gml:Y>";
    os <<      "</gml:coord>" << endl;
    os << "      <gml:coord>";
    os <<          "<gml:X>" <<
                       setprecision(15) << mpolygon->getEnvelopeInternal()->getMaxX() <<
                       "</gml:X>";
    os <<          "<gml:Y>" <<
                       setprecision(15) << mpolygon->getEnvelopeInternal()->getMaxY() <<
                       "</gml:Y>";
    os <<        "</gml:coord>" << endl;
    os << "    </gml:Box>" << endl;
    os << "  </gml:boundedBy>" << endl << endl;
    os << "  <gml:featureMember>" << endl;
    os << "   <ogr:multi_polygon fid=\"0\">" << endl;
    os << "      <ogr:ID>" << idString << "</ogr:ID>" << endl;
    os << "      <ogr:geometryProperty><gml:MultiPolygon><gml:polygonMember>" <<
                     "<gml:Polygon><gml:outerBoundaryIs><gml:LinearRing><gml:coordinates>";


    for(unsigned int polyN = 0; polyN < mpolygon->getNumGeometries(); polyN++) {
      geos::geom::CoordinateSequence *pts = mpolygon->getGeometryN(polyN)->getCoordinates().release();

      for(unsigned int i = 0; i < pts->getSize(); i++) {
        double lon = pts->getAt(i).x;
        double lat = pts->getAt(i).y;

        os << setprecision(15) << lon << "," << setprecision(15) << lat << " ";
      }
    }

    os << "</gml:coordinates></gml:LinearRing></gml:outerBoundaryIs>" <<
              "</gml:Polygon></gml:polygonMember></gml:MultiPolygon>" <<
              "</ogr:geometryProperty>" << endl;
    os << "    </ogr:multi_polygon>" << endl;
    os << "  </gml:featureMember>" << endl;
    os << "</ogr:FeatureCollection>";

    return os.str().c_str();
  }


  /**
   * Return the gml schema
   *
   * @return QString  Returns the polygon with lon,lat lon,lat format vertices and GML header
   */

  QString PolygonTools::GMLSchema() {

    ostringstream os;

    os << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>" << endl;
    os << "<xs:schema targetNamespace=\"http://ogr.maptools.org/\" "
              "xmlns:ogr=\"http://ogr.maptools.org/\" "
              "xmlns:xs=\"http://www.w3.org/2001/XMLSchema\" "
              "xmlns:gml=\"http://www.opengis.net/gml\" "
              "elementFormDefault=\"qualified\" "
              "version=\"1.0\">" << endl;
    os << "  <xs:import namespace=\"http://www.opengis.net/gml\" "
              "schemaLocation=\"http://schemas.opengis.net/gml/2.1.2/feature.xsd\"/>" << endl;
    os << "  <xs:element name=\"FeatureCollection\" "
                 "type=\"ogr:FeatureCollectionType\" "
                 "substitutionGroup=\"gml:_FeatureCollection\"/>" << endl;
    os << "  <xs:complexType name=\"FeatureCollectionType\">" << endl;
    os << "    <xs:complexContent>" << endl;
    os << "      <xs:extension base=\"gml:AbstractFeatureCollectionType\">" << endl;
    os << "        <xs:attribute name=\"lockId\" type=\"xs:string\" use=\"optional\"/>" << endl;
    os << "        <xs:attribute name=\"scope\" type=\"xs:string\" use=\"optional\"/>" << endl;
    os << "      </xs:extension>" << endl;
    os << "    </xs:complexContent>" << endl;
    os << "  </xs:complexType>" << endl;
    os << "  <xs:element name=\"multi_polygon\" "
                 "type=\"ogr:multi_polygon_Type\" "
                 "substitutionGroup=\"gml:_Feature\"/>" << endl;
    os << "  <xs:complexType name=\"multi_polygon_Type\">" << endl;
    os << "    <xs:complexContent>" << endl;
    os << "      <xs:extension base=\"gml:AbstractFeatureType\">" << endl;
    os << "        <xs:sequence>" << endl;
    os << "          <xs:element name=\"geometryProperty\" "
                         "type=\"gml:MultiPolygonPropertyType\" "
                         "nillable=\"true\" minOccurs=\"0\" maxOccurs=\"1\"/>" << endl;
    os << "          <xs:element name=\"fid\" nillable=\"true\" minOccurs=\"0\" "
                         "maxOccurs=\"1\">" << endl;
    os << "            <xs:simpleType>" << endl;
    os << "              <xs:restriction base=\"xs:string\">" << endl;
    os << "              </xs:restriction>" << endl;
    os << "            </xs:simpleType>" << endl;
    os << "          </xs:element>" << endl;
    os << "          <xs:element name=\"ID\" nillable=\"true\" minOccurs=\"0\" "
                         "maxOccurs=\"1\">" << endl;
    os << "            <xs:simpleType>" << endl;
    os << "              <xs:restriction base=\"xs:integer\">" << endl;
    os << "                <xs:totalDigits value=\"16\"/>" << endl;
    os << "              </xs:restriction>" << endl;
    os << "            </xs:simpleType>" << endl;
    os << "          </xs:element>" << endl;
    os << "        </xs:sequence>" << endl;
    os << "      </xs:extension>" << endl;
    os << "    </xs:complexContent>" << endl;
    os << "  </xs:complexType>" << endl;
    os << "</xs:schema>";

    return os.str().c_str();
  }


  /**
   * Convert polygon coordinates from 360 system to 180.
   *
   * @param[in] poly360  (geos::geom::MultiPolygon)poly split by 360 boundary
   *
   * @return geos::geom::MultiPolygon  Returns a 180 multi-polygon
   */
  geos::geom::MultiPolygon *PolygonTools::To180(geos::geom::MultiPolygon *poly360) {
    try {
      // Let's take the 360 pieces that sit between 180 and 360 and move them
      //   to -180 to 0. To accomplish this, make a poly that fits 180 -> 360
      //   degrees longitude and intersect (this gives us the pieces that sit
      //   >180). Move this intersection to the left. Then make a poly that fits
      //   0 to 180 and intersect with the original. These two combined are the
      //   result.
      geos::geom::CoordinateArraySequence *leftOf180Pts =
          new geos::geom::CoordinateArraySequence();
      leftOf180Pts->add(geos::geom::Coordinate(0, -90));
      leftOf180Pts->add(geos::geom::Coordinate(0, 90));
      leftOf180Pts->add(geos::geom::Coordinate(180, 90));
      leftOf180Pts->add(geos::geom::Coordinate(180, -90));
      leftOf180Pts->add(geos::geom::Coordinate(0, -90));

      geos::geom::LinearRing *leftOf180Geom =
          globalFactory->createLinearRing(leftOf180Pts);

      geos::geom::Polygon *leftOf180Poly =
          globalFactory->createPolygon(leftOf180Geom, NULL);

      geos::geom::CoordinateArraySequence *rightOf180Pts =
          new geos::geom::CoordinateArraySequence();
      rightOf180Pts->add(geos::geom::Coordinate(180, -90));
      rightOf180Pts->add(geos::geom::Coordinate(180, 90));
      rightOf180Pts->add(geos::geom::Coordinate(360, 90));
      rightOf180Pts->add(geos::geom::Coordinate(360, -90));
      rightOf180Pts->add(geos::geom::Coordinate(180, -90));

      geos::geom::LinearRing *rightOf180Geom =
          globalFactory->createLinearRing(rightOf180Pts);

      geos::geom::Polygon *rightOf180Poly =
          globalFactory->createPolygon(rightOf180Geom, NULL);

      geos::geom::Geometry *preserved = Intersect(leftOf180Poly, poly360);
      geos::geom::Geometry *moving = Intersect(rightOf180Poly, poly360);

      geos::geom::CoordinateSequence *movingPts = moving->getCoordinates().release();
      geos::geom::CoordinateArraySequence *movedPts =
          new geos::geom::CoordinateArraySequence();

      for(unsigned int i = 0; i < movingPts->getSize(); i ++) {
        movedPts->add(geos::geom::Coordinate(movingPts->getAt(i).x - 360.0,
                                             movingPts->getAt(i).y));
      }

      if(movedPts->getSize()) {
        movedPts->add(geos::geom::Coordinate(movedPts->getAt(0).x,
                                            movedPts->getAt(0).y));
      }

      geos::geom::Geometry *moved = globalFactory->createPolygon(
          globalFactory->createLinearRing(movedPts), NULL);

      std::vector<geos::geom::Geometry *> *geomsForCollection = new
          std::vector<geos::geom::Geometry *>;
      geomsForCollection->push_back(preserved);
      geomsForCollection->push_back(moved);

      geos::geom::GeometryCollection *the180Polys =
          Isis::globalFactory->createGeometryCollection(geomsForCollection);

      geos::geom::MultiPolygon *result = MakeMultiPolygon(the180Polys);
      delete the180Polys;
      the180Polys = NULL;

      geos::geom::MultiPolygon *fixedResult = FixSeam(result);
      delete result;
      result = NULL;

      return fixedResult;
    }
    catch(geos::util::GEOSException *exc) {
      IString msg = "Conversion to 180 failed. The reason given was [" +
          IString(exc->what()) + "]";
      delete exc;
      throw IException(IException::Programmer, msg, _FILEINFO_);
    }
    catch(...) {
      IString msg = "Conversion to 180 failed. Could not determine the reason";
      throw IException(IException::Programmer, msg, _FILEINFO_);
    }
  }


  /**
   * Calculates the thickness of a polygon using:
   *      thickness = area / max(Xextent,Yextent)**2
   * The provided polygon SHOULD be an XY polygon, not a lat/lon polygon, but this
   * is not enforced.
   *
   * @param mpolygon The XY polygon to calculate the thickness of
   *
   * @return double The thikness of the provided polygon
   */
  double PolygonTools::Thickness(const geos::geom::MultiPolygon *mpolygon) {
    const geos::geom::Envelope *envelope = mpolygon->getEnvelopeInternal();

    double x = fabs(envelope->getMaxX() - envelope->getMinX());
    double y = fabs(envelope->getMaxY() - envelope->getMinY());
    double extent = max(x, y);

    return mpolygon->getArea() / (extent * extent);
  }


  /**
   * This method attempts to convert the geom to a MultiPolygon and then despike
   * it.
   *
   * This method does not take ownership of the argument geom. The ownership
   * of the return value is given to the caller.
   *
   * @param geom
   *
   * @return geos::geom::MultiPolygon*
   */
  geos::geom::MultiPolygon *PolygonTools::Despike(const geos::geom::Geometry *geom) {
    geos::geom::MultiPolygon *spiked = MakeMultiPolygon(geom);
    geos::geom::MultiPolygon *despiked = Despike(spiked);

    delete spiked;
    spiked = NULL;

    return despiked;
  }


  /**
   * Create a new multipolygon without the spikes associated with
   * some versions of the geos package.
   *
   * This method does not take ownership of the argument multiPoly. The ownership
   * of the return value is given to the caller.
   *
   * @param multiPoly The original geos::geom::MultiPolygon to be despiked.
   *
   */
  geos::geom::MultiPolygon *PolygonTools::Despike(const geos::geom::MultiPolygon *multiPoly) {
    // Despike each polygon in the multipolygon
    vector<geos::geom::Geometry *> *newPolys = new vector<geos::geom::Geometry *>;
    for(unsigned int g = 0; g < multiPoly->getNumGeometries(); ++g) {
      const geos::geom::Polygon *poly =
          dynamic_cast<const geos::geom::Polygon *>(multiPoly->getGeometryN(g));

      // Despike each hole inside this polygon
      vector<geos::geom::LinearRing *> *holes = new vector<geos::geom::LinearRing *>;
      for(unsigned int h = 0; h < poly->getNumInteriorRing(); ++h) {
        const geos::geom::LineString *ls = poly->getInteriorRingN(h);
        geos::geom::LinearRing *lr;

        // If the hole is not valid fix it
        // If the hole is NOT valid despike it
        lr = Despike(ls);

        if(!lr->isValid()) {
          geos::geom::LinearRing *fixed = FixGeometry(lr);
          delete lr;
          lr = fixed;
        }

        // Save this hole if it is not too small
        if(!lr->isEmpty()) {
          holes->push_back(lr);
        }
        else {
          delete lr;
        }
      } // End holes loop

      // Work on the main polygon
      const geos::geom::LineString *ls = poly->getExteriorRing();
      geos::geom::LinearRing *lr;

      lr = Despike(ls);

      try {
        if(!lr->isValid()) {
          geos::geom::LinearRing *fixed = FixGeometry(lr);
          delete lr;
          lr = fixed;
        }
      }
      catch(IException &e) {
        // Sometimes despike and fix fail, but the input is really valid. We can just go
        // with the non-despiked polygon.
        if(ls->isValid() && ls->getGeometryTypeId() == geos::geom::GEOS_LINEARRING) {
          lr = dynamic_cast<geos::geom::LinearRing *>(ls->clone().release());
        }
        else {
          throw;
        }
      }

      // Create a new polygon with the holes and save it
      if(!lr->isEmpty()) {
        geos::geom::Polygon *tp = Isis::globalFactory->createPolygon(lr, holes);

        if(tp->isEmpty() || !tp->isValid()) {
          delete tp;
          newPolys->push_back(poly->clone().release());
        }
        else {
          newPolys->push_back(tp);
        }
      }
    } // End polygons loop

    // Create a new multipoly from the polygon(s)
    geos::geom::MultiPolygon *mp = Isis::globalFactory->createMultiPolygon(newPolys);

    if(!mp->isValid() || mp->isEmpty()) {
      delete mp;
      mp = NULL;
      IString msg = "Despike failed to correct the polygon";
      throw IException(IException::Programmer, msg, _FILEINFO_);
    }

    // if multipoly area changes more than 25% we did something bad to the multipolygon
    if(fabs((mp->getArea() / multiPoly->getArea()) - 1.0) > 0.50) {
      IString msg = "Despike failed to correct the polygon " + mp->toString();
      delete mp;

      throw IException(IException::Programmer, msg, _FILEINFO_);
    }

    return mp;
  }


  /**
   * Create a new LinearRing from a LineString without the spikes associated
   * with some versions of the geos package. These "spikes" are created when
   * intersections and differences are calculated.
   *
   * @param lineString The original geos::geom::lineString to be despiked. This
   * can be the lineString associated with the outside of a polygon or one of its
   * holes.
   *
   * @return A pointer to a LinearRing. If the despiking causes the number of
   *         unique verticies to fall below 3, an empty LinearRing will be
   *         returned.
   */
  geos::geom::LinearRing *PolygonTools::Despike(const geos::geom::LineString *lineString) {
    geos::geom::CoordinateArraySequence *vertices =
        new geos::geom::CoordinateArraySequence(*(lineString->getCoordinates()));

    // We need a full polygon to despike = 3 points (+1 for end==first) = at least 4 points
    if(vertices->getSize() < 4) {
      delete vertices;
      return Isis::globalFactory->createLinearRing(geos::geom::CoordinateArraySequence());
    }

    // delete one of the duplicate first/end coordinates,
    //   spikes can occur here and the duplicate points throws off the test
    // deleteAt was removed in geos 3.8, this fix needs a better answer
    {
      std::vector<geos::geom::Coordinate> coords;
      vertices->toVector(coords);
      coords.pop_back();
      vertices->setPoints(coords);
      //vertices->deleteAt(vertices->getSize() - 1);
    }

    // Index will become negative if our first points are spiked, so we need
    //   to make it big enough to encapsulate the (unsigned int) from getSize()
    //   and allow negative numbers.
    for(long index = 0; index < (long)vertices->getSize(); index++) {
      // If we're under 3 vertices, we've despiked all the points out of the polygon :(
      if(vertices->getSize() < 3) {
        break;
      }

      // These are the array indices that we're going to test for a spike,
      //   the middle one is the one that's spiked if IsSpiked(...) is true.
      long testCoords[3] = {
        index - 1,
        index,
        index + 1
      };

      // Make sure the index is inside of our coordinates array (we'll have both too small/too
      // big of an index)
      for(int j = 0; j < 3; j++) {
        while(testCoords[j] < 0) {
          testCoords[j] += vertices->getSize();
        }

        while(testCoords[j] >= (long)vertices->getSize()) {
          testCoords[j] -= vertices->getSize();
        }
      }

      // Test the middle point for a spike
      if(IsSpiked(vertices->getAt(testCoords[0]),
                  vertices->getAt(testCoords[1]),
                  vertices->getAt(testCoords[2]))) {
        // It's spiked, delete it
        std::vector<geos::geom::Coordinate> coords;
        vertices->toVector(coords);
        coords.erase(coords.begin()+testCoords[1]);
        vertices->setPoints(coords);

        // Back up to the first test that is affected by this change
        index -= 2;
      }
    }

    if(vertices->getSize() < 3) {
      delete vertices;
      vertices = NULL;

      return Isis::globalFactory->createLinearRing(geos::geom::CoordinateArraySequence());
    }
    else {
      // Duplicate the first vertex as the last to close the polygon
      vertices->add(vertices->getAt(0));
      return Isis::globalFactory->createLinearRing(vertices);
    }
  }

  /**
   * Returns true if the middle point is spiked.
   *
   * @param first
   * @param middle
   * @param last
   *
   * @return bool Returns true if middle point is spiked
   */
  bool PolygonTools::IsSpiked(geos::geom::Coordinate first,
                              geos::geom::Coordinate middle,
                              geos::geom::Coordinate last) {
    return TestSpiked(first, middle, last) || TestSpiked(last, middle, first);
  }

  /**
   * This method tests for spikes. The first/last coordinate matter. If the line
   * between the first point and middle point are the base of a triangle, with the
   * last point as the tip, then if the base of this triangle is substantially
   * larger than the height we have a spike (fig A). If the middle point is near
   * the edges of the triangle, however, this is valid data (fig B/C).
   *
   *  (A)             (B)                 (C)
   *   1               1                   1-2
   *   |               |                     |
   *   |               |                     |
   * 3-|               |                     |
   *   |               |                     |
   *   |               |                     |
   *   2               2-3                   3
   *
   *   *1 is start, 2 is middle, 3 is end
   *
   *   Spikes are a problem because when we convert from lat/lons to meters the
   *   points get shifted relative to each other (lower points might be shifted
   *   more right then higher points, for example).
   *
   * @param first
   * @param middle
   * @param last
   *
   * @return bool Returns true if middle point spiked given this first/last pt
   */
  bool PolygonTools::TestSpiked(geos::geom::Coordinate first, geos::geom::Coordinate middle,
                                geos::geom::Coordinate last) {
    geos::geom::Point *firstPt = Isis::globalFactory->createPoint(first);
    geos::geom::Point *middlePt = Isis::globalFactory->createPoint(middle);
    geos::geom::Point *lastPt = Isis::globalFactory->createPoint(last);

    geos::geom::CoordinateArraySequence *coords = new geos::geom::CoordinateArraySequence();
    coords->add(first);
    coords->add(middle);
    geos::geom::LineString *line = Isis::globalFactory->createLineString(coords); // line takes ownership

    // The lower the tolerance, the less this algorithm removes and thus
    //   the better chance of success in findimageoverlaps. However, if you
    //   lower the tolerance then there is also a greater chance of programs
    //   such as autoseed failing. 1% is the current tolerance.
    double tolerance = line->getLength() / 100.0;

    bool spiked = true;

    double distanceLastMiddle = geos::operation::distance::DistanceOp::distance(lastPt, middlePt);
    double distanceLastLine = geos::operation::distance::DistanceOp::distance(lastPt, line);

    if(distanceLastMiddle == 0.0) return true; // get rid of same point

    // Checks the ratio of the distance between the last point and the line, and the last point
    // and the middle point if the ratio is very small then there is a spike
    if(distanceLastLine / distanceLastMiddle >= .05) {
      spiked = false;
    }

    // If the point is away from the line, keep it
    if(spiked && distanceLastLine > tolerance) {
      spiked = false;
    }

    if(!spiked) {
      geos::geom::CoordinateArraySequence *coords = new geos::geom::CoordinateArraySequence();
      coords->add(first);
      coords->add(middle);
      coords->add(last);
      coords->add(first);

      // shell takes ownership of coords
      geos::geom::LinearRing *shell = Isis::globalFactory->createLinearRing(coords);
      std::vector<geos::geom::LinearRing *> *empty = new std::vector<geos::geom::LinearRing *>;

      // poly takes ownership of shell and empty
      geos::geom::Polygon *poly = Isis::globalFactory->createPolygon(shell, empty);


      // if these 3 points define a straight line then the middle is worthless (defines nothing)
      // or problematic
      if(poly->getArea() < 1.0e-10) {
        spiked = true;
      }

      delete poly;
    }


    delete firstPt;
    delete middlePt;
    delete lastPt;
    delete line;

    return spiked;
  }


  /**
   * This applies the geos Intersect operator. Due to "BinaryOp.h" having
   * implementations in it, only one Isis object may perform these operations. If
   * that file is included anywhere else in Isis, the library will not build!
   * Please use this method to intersect two geometries. If the geometry is a
   * linear ring or multi polygon, corrections may be applied if the geos
   * intersection operator fails.
   *
   * @param geom1 First geometry to intersect
   * @param geom2 Second geometry to intersect
   *
   * @return geos::geom::Geometry* geom1 intersected with geom2
   */
  geos::geom::Geometry *PolygonTools::Intersect(const geos::geom::Geometry *geom1,
                                                const geos::geom::Geometry *geom2) {
    try {
      return Operate(geom1, geom2, (unsigned int)geos::operation::overlay::OverlayOp::opINTERSECTION);
    }
    catch(geos::util::GEOSException *exc) {
      IString msg = "Intersect operation failed. The reason given was [" + IString(exc->what()) + "]";
      delete exc;
      throw IException(IException::Programmer, msg, _FILEINFO_);
    }
    catch(IException &e) {
      IString msg = "Intersect operation failed";
      throw IException(e, IException::Programmer, msg, _FILEINFO_);
    }
    catch(...) {
      IString msg = "Intersect operation failed for an unknown reason";
      throw IException(IException::Programmer, msg, _FILEINFO_);
    }
  }


  geos::geom::Geometry *PolygonTools::Operate(const geos::geom::Geometry *geom1,
                                              const geos::geom::Geometry *geom2,
                                              unsigned int opcode) {

    geos::operation::overlay::OverlayOp::OpCode code =
      (geos::operation::overlay::OverlayOp::OpCode)opcode;

    geos::geom::Geometry *result = NULL;
    bool failed = true;
    geos::geom::Geometry *geomFirst  = MakeMultiPolygon(geom1);
    geos::geom::Geometry *geomSecond = MakeMultiPolygon(geom2);

    geos::operation::overlay::snap::GeometrySnapper snap(*geomFirst);
    geos::geom::Geometry *geomSnapped = snap.snapTo(*geomSecond, 1.0e-10)->clone().release();
    if(!geomSnapped->isValid()) {
      delete geomSnapped;
    }
    else {
      delete geomFirst;
      geomFirst = geomSnapped;
    }

    unsigned int precision = 15;
    unsigned int minPrecision = 13;
    while(failed) {
      try {
        // C++11: the geos BinaryOp returns an auto_ptr, we use release() to create a unique_ptr.
        std::unique_ptr< geos::geom::Geometry > resultAuto(
          geos::operation::overlay::OverlayOp::overlayOp(geomFirst, geomSecond, code));
        failed = false;
        result = resultAuto->clone().release();
      }
      catch(geos::util::GEOSException *exc) {
        // Just in case the clone failed....
        if(!failed || precision == minPrecision) throw;

        delete exc;
      }
      catch(...) {
        if(precision == minPrecision) {
          IString msg = "An unknown geos error occurred";
          throw IException(IException::Programmer, msg, _FILEINFO_);
        }

        if(!failed) {
          IString msg = "An unknown geos error occurred when attempting to clone a geometry";
          throw IException(IException::Programmer, msg, _FILEINFO_);
        }
      }

      // try reducing the precision
      if(failed) {
        precision --;
        geos::geom::Geometry *tmp = ReducePrecision(geomFirst, precision);
        delete geomFirst;
        geomFirst = tmp;

        tmp = ReducePrecision(geomSecond, precision);
        delete geomSecond;
        geomSecond = tmp;
      }
    }

    delete geomFirst;
    geomFirst = NULL;

    delete geomSecond;
    geomSecond = NULL;

    if(result && !result->isValid()) {
      try {
        geos::geom::Geometry *newResult = FixGeometry(result);

        if(fabs(newResult->getArea() / result->getArea() - 1.0) > 0.50) {
          delete newResult;
          delete result;

          IString msg = "Operation [" + IString((int)opcode) + "] failed";
          throw IException(IException::Programmer, msg, _FILEINFO_);
        }

        delete result;
        result = newResult;
      }
      catch(IException &e) {
        IString msg = "Operation [" + IString((int)opcode) + "] failed";
        throw IException(IException::Programmer, msg, _FILEINFO_);
      }
    }

    if(result == NULL) {
      IString msg = "Operation [" + IString((int)opcode) + " failed";
      throw IException(IException::Programmer, msg, _FILEINFO_);
    }

    return result;
  }

  /**
   * This method attempts to seek out known problems with geometries and repair
   * them. Currently the only known problem is when two points lie on top of each
   * other, which is a result of various operations. Currently only LinearRings
   * and MultiPolygons are supported.
   *
   * @param geom The geometry to be fixed
   *
   * @return geos::geom::Geometry* A fixed geometry.
   */
  geos::geom::Geometry *PolygonTools::FixGeometry(const geos::geom::Geometry *geom) {
    if(geom->getGeometryTypeId() == geos::geom::GEOS_MULTIPOLYGON) {
      return FixGeometry(dynamic_cast<const geos::geom::MultiPolygon *>(geom));
    }
    if(geom->getGeometryTypeId() == geos::geom::GEOS_LINEARRING) {
      return FixGeometry(dynamic_cast<const geos::geom::LinearRing *>(geom));
    }
    if(geom->getGeometryTypeId() == geos::geom::GEOS_POLYGON) {
      return FixGeometry(dynamic_cast<const geos::geom::Polygon *>(geom));
    }
    if(geom->getGeometryTypeId() == geos::geom::GEOS_GEOMETRYCOLLECTION) {
      return FixGeometry(MakeMultiPolygon(geom));
    }
    else {
      IString msg = "PolygonTools::FixGeometry does not support [" + GetGeometryName(geom) + "]";
      throw IException(IException::Programmer, msg, _FILEINFO_);
    }
  }

  /**
   * This applies the LinearRing FixGeometry method to all of the interior and
   * exterior rings in the multipolygon. See FixGeometry(LinearRing) for more
   * explanation.
   *
   * @param poly An invalid multipolygon
   *
   * @return geos::geom::MultiPolygon* A possibly valid multipolygon
   */
  geos::geom::MultiPolygon *PolygonTools::FixGeometry(const geos::geom::MultiPolygon *poly) {
    // Maybe two points are on top of each other
    vector<geos::geom::Geometry *> *newPolys = new vector<geos::geom::Geometry *>;

    // Convert each polygon in this multi-polygon
    for(unsigned int geomIndex = 0; geomIndex < poly->getNumGeometries(); geomIndex ++) {
      geos::geom::Polygon *fixedpoly = FixGeometry(
          dynamic_cast<const geos::geom::Polygon *>(
            poly->getGeometryN(geomIndex)));
      if(fixedpoly->isValid()) {
        newPolys->push_back(fixedpoly);
      }
      else {
        delete fixedpoly;
      }
      fixedpoly = NULL;
    }

    geos::geom::MultiPolygon *mp = Isis::globalFactory->createMultiPolygon(newPolys);
    return mp;
  }


  geos::geom::Polygon *PolygonTools::FixGeometry(const geos::geom::Polygon *poly) {

    // Convert each hole inside this polygon
    vector<geos::geom::LinearRing *> *holes = new vector<geos::geom::LinearRing *>;
    for(unsigned int holeIndex = 0; holeIndex < poly->getNumInteriorRing(); holeIndex ++) {
      const geos::geom::LinearRing *thisHole = poly->getInteriorRingN(holeIndex);

      // We hope they are all linear rings (closed/simple), but if not just leave it be
      if(thisHole->getGeometryTypeId() != geos::geom::GEOS_LINEARRING) {
        holes->push_back(dynamic_cast<geos::geom::LinearRing *>(thisHole->clone().release()));
//      newRing = dynamic_cast<geos::geom::LinearRing *>(ring->clone());
        continue;
      }

      try {
        geos::geom::LinearRing *newHole = FixGeometry((geos::geom::LinearRing *)thisHole);
        holes->push_back(newHole);
      }
      catch (IException &e) {
        IString msg = "Failed when attempting to fix interior ring of multipolygon";
        throw IException(IException::Programmer, msg, _FILEINFO_);
      }
    } // end num holes in polygon loop


    const geos::geom::LineString *exterior = poly->getExteriorRing();

    try {
      geos::geom::LinearRing *newExterior = NULL;

      if(exterior->getGeometryTypeId() == geos::geom::GEOS_LINEARRING) {
        newExterior = FixGeometry((geos::geom::LinearRing *)exterior);
      }
      else {
        IString msg = "Failed when attempting to fix exterior ring of polygon. The exterior "
                      "ring is not simple and closed";
        throw IException(IException::Programmer, msg, _FILEINFO_);
      }

      return globalFactory->createPolygon(newExterior, holes);
    }
    catch (IException &e) {
      IString msg = "Failed when attempting to fix exterior ring of polygon";
      throw IException(e, IException::Programmer, msg, _FILEINFO_);
    }
  }


  /**
   * One problem we know of seems to happen when two points are right on top of
   * each other. We're going to look for those points and remove them, then see if
   * the linear ring is valid. Assumes input fails its isValid() test.
   *
   * Point end up on top of each other for two (2) known reasons. First, Despike
   * removes a spike that simply went back and forth. Second, the geos Intersect
   * operator can return invalid polygons because of this problem. Every geometry
   * thus far is being broken down into LinearRings and this method cleans those
   * up.
   *
   * @param ring An invalid linear ring
   *
   * @return geos::geom::LinearRing* A possibly valid linear ring
   */
  geos::geom::LinearRing *PolygonTools::FixGeometry(const geos::geom::LinearRing *ring) {

    geos::geom::CoordinateSequence *coords = ring->getCoordinates().release();

    // Check this, just in case
    if(coords->getSize() < 4) {
      return globalFactory->createLinearRing(new geos::geom::DefaultCoordinateSequence());
    }

    geos::geom::CoordinateArraySequence *newCoords = new geos::geom::DefaultCoordinateSequence();
    const geos::geom::Coordinate *lastCoordinate = &coords->getAt(0);
    newCoords->add(*lastCoordinate);

    // Convert each coordinate in this hole
    for(unsigned int coordIndex = 1; coordIndex < coords->getSize() - 1; coordIndex ++) {
      const geos::geom::Coordinate *thisCoordinate = &coords->getAt(coordIndex);

      // we're going to compare the decimal place of the current point to the decimal place
      //   of the difference, if they are drastically different then geos might not be seeing them
      //   correctly.
      double difference[2] = {
        lastCoordinate->x - thisCoordinate->x,
        lastCoordinate->y - thisCoordinate->y,
      };

      // geos isnt differentiating between points this close
      double minDiff = fabs(DecimalPlace(thisCoordinate->x) - DecimalPlace(difference[0]));

      minDiff = min(minDiff, fabs(DecimalPlace(thisCoordinate->y) - DecimalPlace(difference[1])));

      // Cases where the difference in one direction is exactly zero, and the other direction is
      // next to zero appear often enough (especially in despike).
      if(difference[0] == 0.0 && difference[1] != 0.0) {
        // subtracted the two points, got deltaX = 0.0, use the y difference decimal place
        minDiff = fabs(DecimalPlace(thisCoordinate->y) - DecimalPlace(difference[1]));
      }
      else if(difference[1] == 0.0 && difference[0] != 0.0) {
        // subtracted the two points, got deltaY = 0.0, use the x difference decimal place
        minDiff = fabs(DecimalPlace(thisCoordinate->x) - DecimalPlace(difference[0]));
      }
      else if(difference[0] == 0.0 && difference[1] == 0.0) {
        // subtracted the two points, got 0.0, so it's same point... make sure it gets ignored!
        minDiff = 1E99;
      }

      // geos has a hard time differentiating when points get too close...
      if(minDiff <= 15) {
        newCoords->add(*thisCoordinate);
        lastCoordinate = thisCoordinate;
      }
    } // end num coords in hole loop

    newCoords->add(geos::geom::Coordinate(newCoords->getAt(0).x, newCoords->getAt(0).y));
    geos::geom::LinearRing *newRing = NULL;

    // Now that we've weeded out any bad coordinates, let's rebuild the geometry
    try {
      if(newCoords->getSize() > 3) {
        newRing = globalFactory->createLinearRing(newCoords);
      }
      else {
        delete newCoords;
        newCoords = NULL;
      }
    }
    catch(geos::util::GEOSException *exc) {
      delete exc;
      exc = NULL;

      IString msg = "Error when attempting to fix linear ring";
      throw IException(IException::Programmer, msg, _FILEINFO_);
    }

    if(newRing && !newRing->isValid() && ring->isValid()) {
      IString msg = "Failed when attempting to fix linear ring";
      throw IException(IException::Programmer, msg, _FILEINFO_);
    }
    else if(!newRing || !newRing->isValid()) {
      if(newRing) {
        delete newRing;
      }

      newRing = dynamic_cast<geos::geom::LinearRing *>(ring->clone().release());
    }

    return newRing;
  }


  /**
   * This returns the location of the decimal place in the number. This method
   * is used as a helper method for FixGeometry(...), to help figure out how close
   * two numbers are in significant figures.
   *
   * 1.0 = decimal place 1, .1 = decimal place 0, 10.0 = decimal place 2
   *
   * @param num The number to find the decimal place in
   *
   * @return int The decimal place relative to the first significant digit
   */
  int PolygonTools::DecimalPlace(double num) {
    // 0.0 = decimal place 0
    if(num == 0.0) return 0;

    num = fabs(num);

    int decimalPlace = 1;
    while(num < 1.0) {
      num *= 10.0;
      decimalPlace --;
    }

    while(num > 10.0) {
      num /= 10.0;
      decimalPlace ++;
    }

    return decimalPlace;
  }

  /**
   * This method is used to subtract two polygons.
   *
   * @param geom1
   * @param geom2
   *
   * @return geos::geom::Geometry*
   */
  geos::geom::Geometry *PolygonTools::Difference(const geos::geom::Geometry *geom1,
                                                 const geos::geom::Geometry *geom2) {
    try {
      return Operate(geom1, geom2, (unsigned int)geos::operation::overlay::OverlayOp::opDIFFERENCE);
    }
    catch(geos::util::GEOSException *exc) {
      IString msg = "Difference operation failed. The reason given was [" +
                    IString(exc->what()) + "]";
      delete exc;
      throw IException(IException::Programmer, msg, _FILEINFO_);
    }
    catch(IException &e) {
      IString msg = "Difference operation failed";
      throw IException(e, IException::Programmer, msg, _FILEINFO_);
    }
    catch(...) {
      IString msg = "Difference operation failed for an unknown reason";
      throw IException(IException::Programmer, msg, _FILEINFO_);
    }
  }


  /**
   * Make a geos::geom::MultiPolygon out of the components of the argument
   *
   * Create a new geos::geom::MultiPolygon out of the general geometry that is
   * passed in. This can be useful after an intersection or some other
   * operator on two MultiPolygons. The result of the operator is often a
   * collection of different geometries such as points, lines, polygons...
   * This member extracts all polygons and multipolygons into a new
   * multipolygon. The original geometry is deleted. The resulting multipolygon is
   * not necessarily valid.
   *
   * @param geom The geometry to be converted into a multipolygon
   */
  geos::geom::MultiPolygon *PolygonTools::MakeMultiPolygon(const geos::geom::Geometry *geom) {
    // The area of the geometry is too small, so just ignore it.
    if(geom->isEmpty()) {
      return Isis::globalFactory->createMultiPolygon().release();
    }

    else if(geom->getArea() - DBL_EPSILON <= DBL_EPSILON) {
      return Isis::globalFactory->createMultiPolygon().release();
    }

    else if(geom->getGeometryTypeId() == geos::geom::GEOS_MULTIPOLYGON) {
      return dynamic_cast<geos::geom::MultiPolygon *>(geom->clone().release());
    }

    else if(geom->getGeometryTypeId() == geos::geom::GEOS_POLYGON) {
      vector<geos::geom::Geometry *> *polys = new vector<geos::geom::Geometry *>;
      polys->push_back(geom->clone().release());
      geos::geom::MultiPolygon *mp = Isis::globalFactory->createMultiPolygon(polys);
      return mp;
    }

    else if(geom->getGeometryTypeId() == geos::geom::GEOS_GEOMETRYCOLLECTION) {
      vector<geos::geom::Geometry *> * polys =
          new vector<geos::geom::Geometry *>;
      const geos::geom::GeometryCollection *gc =
          dynamic_cast<const geos::geom::GeometryCollection *>(geom);
      for(unsigned int i = 0; i < gc->getNumGeometries(); ++i) {
        geos::geom::MultiPolygon *subMultiPoly =
            MakeMultiPolygon(gc->getGeometryN(i));

        for(unsigned int subPoly = 0;
            subPoly < subMultiPoly->getNumGeometries();
            subPoly ++) {
          const geos::geom::Polygon *poly =
              dynamic_cast<const geos::geom::Polygon *>(
                subMultiPoly->getGeometryN(subPoly));
          polys->push_back(dynamic_cast<geos::geom::Polygon *>(poly->clone().release()));
        }
      }

      geos::geom::MultiPolygon *mp =
          Isis::globalFactory->createMultiPolygon(polys);
      if(mp->getArea() - DBL_EPSILON <= DBL_EPSILON) {
        delete mp;
        mp = Isis::globalFactory->createMultiPolygon().release();
      }

      return mp;
    }

    // All other geometry types are invalid so ignore them
    else {
      return Isis::globalFactory->createMultiPolygon().release();
    }
  }


  geos::geom::MultiPolygon *PolygonTools::FixSeam(
      const geos::geom::Polygon *polyA, const geos::geom::Polygon *polyB) {
    geos::geom::CoordinateSequence *polyAPoints = polyA->getCoordinates().release();
    geos::geom::CoordinateSequence *polyBPoints = polyB->getCoordinates().release();

    unsigned int aIntersectionBegin = 0;
    unsigned int aIntersectionEnd = 0;
    unsigned int bIntersectionBegin = 0;
    unsigned int bIntersectionEnd = 0;

    bool intersectionStarted = false;
    bool intersectionEnded = false;

    unsigned int lastBMatch  = 0;
    for (unsigned int i = 0;
        !intersectionEnded && i < polyAPoints->getSize();
        i++) {

      bool foundEquivalent = false;

      geos::geom::Coordinate coordA = polyAPoints->getAt(i);
      coordA = *ReducePrecision(&coordA, 13);

      for (unsigned int j = 0;
           !foundEquivalent && j < polyBPoints->getSize();
           j++) {
        geos::geom::Coordinate coordB = polyBPoints->getAt(j);
        coordB = *ReducePrecision(&coordB, 13);

        foundEquivalent = coordA.equals(coordB);

        if (foundEquivalent) lastBMatch = j;

        if (foundEquivalent && !intersectionStarted) {
          intersectionStarted = true;
          aIntersectionBegin = i;
          bIntersectionBegin = j;
        }
      }

      if (!foundEquivalent && intersectionStarted && !intersectionEnded) {
        intersectionEnded = true;
        aIntersectionEnd = i;
        bIntersectionEnd = lastBMatch;
      }
    }

    geos::geom::MultiPolygon * result = NULL;
    if (intersectionStarted && intersectionEnded) {
      geos::geom::CoordinateArraySequence *merged =
          new geos::geom::CoordinateArraySequence;

      unsigned int i = 0;
      for (i = 0; i < aIntersectionBegin; i ++) {
        merged->add(polyAPoints->getAt(i));
      }

      i = bIntersectionBegin;
      while (i != bIntersectionEnd) {
        merged->add(polyBPoints->getAt(i));
        i++;
        if (i >= polyBPoints->getSize()) i = 0;
      }

      for (i = aIntersectionEnd; i < polyAPoints->getSize() - 1; i++) {
        merged->add(polyAPoints->getAt(i));
      }

      merged->add(merged->getAt(0));
      result = MakeMultiPolygon(globalFactory->createPolygon(
          globalFactory->createLinearRing(merged), NULL));
    }

    return result;
  }


  geos::geom::MultiPolygon *PolygonTools::FixSeam(
      const geos::geom::MultiPolygon *poly) {

    std::vector<geos::geom::Geometry *> *polys =
        new std::vector<geos::geom::Geometry *>;


    for(unsigned int copyIndex = 0;
        copyIndex < poly->getNumGeometries();
        copyIndex ++) {
      polys->push_back(poly->getGeometryN(copyIndex)->clone().release());
    }

    unsigned int outerPolyIndex = 0;

    while(outerPolyIndex + 1 < polys->size()) {
      unsigned int innerPolyIndex = outerPolyIndex + 1;

      while(innerPolyIndex < polys->size()) {
        geos::geom::MultiPolygon *fixedPair = FixSeam(
            dynamic_cast<geos::geom::Polygon *>(polys->at(outerPolyIndex)),
            dynamic_cast<geos::geom::Polygon *>(polys->at(innerPolyIndex)));

        if(fixedPair != NULL) {
          geos::geom::Geometry *oldInnerPoly = polys->at(innerPolyIndex);
          geos::geom::Geometry *oldOuterPoly = polys->at(outerPolyIndex);

          polys->erase(polys->begin() + innerPolyIndex);
          (*polys)[outerPolyIndex] = fixedPair->getGeometryN(0)->clone().release();
          innerPolyIndex = outerPolyIndex + 1;

          delete fixedPair;
          fixedPair = NULL;

          delete oldInnerPoly;
          oldInnerPoly = NULL;

          delete oldOuterPoly;
          oldOuterPoly = NULL;
        }
        else {
          innerPolyIndex ++;
        }
      }

      outerPolyIndex ++;
    }

    return globalFactory->createMultiPolygon(polys);
  }


  /**
   * This method reduces the precision of the geometry to precision significant
   * figures.
   *
   * @param geom The geometry to reduce precision on
   * @param precision The precision to reduce to
   *
   * @return geos::geom::Geometry* The lower precision geometry
   */
  geos::geom::Geometry *PolygonTools::ReducePrecision(const geos::geom::Geometry *geom,
                                                      unsigned int precision) {
    if(geom->getGeometryTypeId() == geos::geom::GEOS_MULTIPOLYGON) {
      return ReducePrecision(
          dynamic_cast<const geos::geom::MultiPolygon *>(geom), precision);
    }
    if(geom->getGeometryTypeId() == geos::geom::GEOS_LINEARRING) {
      return ReducePrecision(
          dynamic_cast<const geos::geom::LinearRing *>(geom), precision);
    }
    if(geom->getGeometryTypeId() == geos::geom::GEOS_POLYGON) {
      return ReducePrecision(
          dynamic_cast<const geos::geom::Polygon *>(geom), precision);
    }
    if(geom->getGeometryTypeId() == geos::geom::GEOS_GEOMETRYCOLLECTION) {
      return ReducePrecision(MakeMultiPolygon(geom), precision);
    }
    else {
      IString msg = "PolygonTools::ReducePrecision does not support [" +
                    GetGeometryName(geom) + "]";
      throw IException(IException::Programmer, msg, _FILEINFO_);
    }
  }


  /**
   * This method reduces the precision of the MultiPolygon to precision
   * significant figures.
   *
   * @param poly The MultiPolygon to reduce precision on
   * @param precision The precision to reduce to
   *
   * @return geos::geom::MultiPolygon* The lower precision MultiPolygon
   */
  geos::geom::MultiPolygon *PolygonTools::ReducePrecision(const geos::geom::MultiPolygon *poly,
                                                          unsigned int precision) {
    // Maybe two points are on top of each other
    vector<geos::geom::Geometry *> *newPolys = new vector<geos::geom::Geometry *>;

    // Convert each polygon in this multi-polygon
    for(unsigned int geomIndex = 0; geomIndex < poly->getNumGeometries(); geomIndex ++) {
      geos::geom::Geometry *lowerPrecision = ReducePrecision(
          dynamic_cast<const geos::geom::Polygon *>(
            poly->getGeometryN(geomIndex)),
          precision);

      if(!lowerPrecision->isEmpty()) {
        newPolys->push_back(lowerPrecision);
      }
      else {
        delete lowerPrecision;
      }
    }

    geos::geom::MultiPolygon *mp = Isis::globalFactory->createMultiPolygon(newPolys);
    return mp;
  }


  /**
   * This method reduces the precision of the Polygon to precision significant
   * figures.
   *
   * @param poly The polygon to reduce precision on
   * @param precision The precision to reduce to
   *
   * @return geos::geom::Polygon* The lower precision polygon
   */
  geos::geom::Polygon *PolygonTools::ReducePrecision(const geos::geom::Polygon *poly,
                                                     unsigned int precision) {
    // Convert each hole inside this polygon
    vector<geos::geom::LinearRing *> *holes = new vector<geos::geom::LinearRing *>;
    for(unsigned int holeIndex = 0; holeIndex < poly->getNumInteriorRing(); holeIndex ++) {
      const geos::geom::LineString *thisHole = poly->getInteriorRingN(holeIndex);

      // We hope they are all linear rings (closed/simple), but if not just leave it be
      if(thisHole->getGeometryTypeId() != geos::geom::GEOS_LINEARRING) {
        holes->push_back(dynamic_cast<geos::geom::LinearRing *>(thisHole->clone().release()));
        continue;
      }

      try {
        geos::geom::LinearRing *newHole = ReducePrecision((geos::geom::LinearRing *)thisHole,
                                                          precision);

        if(!newHole->isEmpty()) {
          holes->push_back(newHole);
        }
        else {
          delete newHole;
        }

      }
      catch(IException &e) {
        IString msg = "Failed when attempting to fix interior ring of multipolygon";
        throw IException(e, IException::Programmer, msg, _FILEINFO_);
      }
    } // end num holes in polygon loop


    const geos::geom::LineString *exterior = poly->getExteriorRing();

    try {
      geos::geom::LinearRing *newExterior = NULL;

      if(exterior->getGeometryTypeId() == geos::geom::GEOS_LINEARRING) {
        newExterior = ReducePrecision((geos::geom::LinearRing *)exterior, precision);
      }
      else {
        IString msg = "Failed when attempting to fix exterior ring of polygon. The exterior "
                      "ring is not simple and closed";
        throw IException(IException::Programmer, msg, _FILEINFO_);
      }

      return globalFactory->createPolygon(newExterior, holes);
    }
    catch(IException &e) {
      IString msg = "Failed when attempting to fix exterior ring of polygon";
      throw IException(e, IException::Programmer, msg, _FILEINFO_);
    }
  }


  /**
   * This method reduces the precision of the LinearRing to precision significant
   * figures.
   *
   * @param ring The linear ring to reduce precision on
   * @param precision The precision to reduce to
   *
   * @return geos::geom::LinearRing* The lower precision linear ring
   */
  geos::geom::LinearRing *PolygonTools::ReducePrecision(const geos::geom::LinearRing *ring,
                                                        unsigned int precision) {
    geos::geom::CoordinateSequence *coords = ring->getCoordinates().release();

    // Check this, just in case
    if(coords->getSize() <= 0) {
      return dynamic_cast<geos::geom::LinearRing *>(ring->clone().release());
    }

    geos::geom::CoordinateArraySequence *newCoords = new geos::geom::DefaultCoordinateSequence();
    geos::geom::Coordinate *coord = ReducePrecision(&coords->getAt(0), precision);
    newCoords->add(*coord);
    delete coord;
    coord = NULL;

    // Convert each coordinate in this ring
    for(unsigned int coordIndex = 1; coordIndex < coords->getSize() - 1; coordIndex ++) {
      const geos::geom::Coordinate *thisCoordinate = &coords->getAt(coordIndex);
      coord = ReducePrecision(thisCoordinate, precision);
      newCoords->add(*coord);
      delete coord;
      coord = NULL;
    }

    newCoords->add(geos::geom::Coordinate(newCoords->getAt(0).x, newCoords->getAt(0).y));
    geos::geom::LinearRing *newRing = NULL;

    // Now that we've weeded out any bad coordinates, let's rebuild the geometry
    try {
      newRing = globalFactory->createLinearRing(newCoords);
    }
    catch(geos::util::GEOSException *exc) {
      delete exc;
      exc = NULL;

      IString msg = "Error when attempting to reduce precision of linear ring";
      throw IException(IException::Programmer, msg, _FILEINFO_);
    }

    // try to despike
    try {
      geos::geom::LinearRing *tmp = Despike(newRing);
      delete newRing;
      newRing = tmp;
    }
    catch(IException &e) {
    }

    if(!newRing->isValid()) {
      IString msg = "Failed when attempting to reduce precision of linear ring";
      throw IException(IException::Programmer, msg, _FILEINFO_);
    }

    return newRing;
  }


  /**
   * This method reduces the precision of the coordinate to precision significant
   * figures.
   *
   * @param coord The coordinate to reduce precision on
   * @param precision The precision to reduce to
   *
   * @return geos::geom::Coordinate* The lower precision coordinate
   */
  geos::geom::Coordinate *PolygonTools::ReducePrecision(const geos::geom::Coordinate *coord,
                                                        unsigned int precision) {
    return new geos::geom::Coordinate(
             ReducePrecision(coord->x, precision),
             ReducePrecision(coord->y, precision),
             ReducePrecision(coord->z, precision)
           );
  }


  /**
   * This method will reduce the decimal precision of the inputted num to
   * precision decimal places.
   *
   * @param num The original number
   * @param precision The new precision
   *
   * @return double The reduced precision number
   */
  double PolygonTools::ReducePrecision(double num, unsigned int precision) {
    double result = num;

    // If num == nan then this test will fail
    if (num == num) {
      int decimalPlace = DecimalPlace(num);
      double factor = pow(10.0, (int)decimalPlace);

      // reduced num is in the form 0.nnnnnnnnnn...
      double reducedNum = num / factor;

      double cutoff = pow(10.0, (int)precision);
      double round_offset = (num < 0) ? -0.5 : 0.5;

      // cast off the digits past the precision's place
      reducedNum = ((long long)(reducedNum * cutoff + round_offset)) / cutoff;
      result = reducedNum * factor;
    }

    return result;
  }


  /**
   * This method returns the name of the type of geometry passed in. This is
   * useful for error reporting (i.e. Geometry Type [...] not supported).
   *
   * @param geom The geometry to test which type it really is
   *
   * @return QString
   */
  QString PolygonTools::GetGeometryName(const geos::geom::Geometry *geom) {
    switch(geom->getGeometryTypeId()) {
      case geos::geom::GEOS_POINT:
        return "Point";
      case geos::geom::GEOS_LINESTRING:
        return "Line String";
      case geos::geom::GEOS_LINEARRING:
        return "Linear Ring";
      case geos::geom::GEOS_POLYGON:
        return "Polygon";
      case geos::geom::GEOS_MULTIPOINT:
        return "Multi Point";
      case geos::geom::GEOS_MULTILINESTRING:
        return "Multi Line String";
      case geos::geom::GEOS_MULTIPOLYGON:
        return "Multi Polygon";
      case geos::geom::GEOS_GEOMETRYCOLLECTION:
        return "Geometry Collection";
      default:
        return "UNKNOWN";
    }
  }


  bool PolygonTools::Equal(const geos::geom::MultiPolygon *poly1,
                           const geos::geom::MultiPolygon *poly2) {

    vector<const geos::geom::Polygon *> polys1;
    vector<const geos::geom::Polygon *> polys2;

    if(poly1->getNumGeometries() != poly2->getNumGeometries())  return false;

    // Convert each polygon in this multi-polygon
    for(unsigned int geomIndex = 0; geomIndex < poly1->getNumGeometries(); geomIndex ++) {
      polys1.push_back(dynamic_cast<const geos::geom::Polygon *>(
          poly1->getGeometryN(geomIndex)));
      polys2.push_back(dynamic_cast<const geos::geom::Polygon *>(
          poly2->getGeometryN(geomIndex)));
    }

    for(int p1 = polys1.size() - 1; (p1 >= 0) && polys1.size(); p1 --) {
      for(int p2 = polys2.size() - 1; (p2 >= 0) && polys2.size(); p2 --) {
        if(Equal(polys1[p1], polys2[p2])) {
          // Delete polys1[p1] by replacing it with the last Polygon in polys1
          polys1[p1] = polys1[polys1.size()-1];
          polys1.resize(polys1.size() - 1);
          // Delete polys2[p2] by replacing it with the last Polygon in polys2
          polys2[p2] = polys2[polys2.size()-1];
          polys2.resize(polys2.size() - 1);
        }
      }
    }

    return (polys1.size() == 0) && (polys2.size() == 0);
  }


  bool PolygonTools::Equal(const geos::geom::Polygon *poly1, const geos::geom::Polygon *poly2) {
    vector<const geos::geom::LineString *> holes1;
    vector<const geos::geom::LineString *> holes2;

    if(poly1->getNumInteriorRing() != poly2->getNumInteriorRing())  return false;

    if(!Equal(poly1->getExteriorRing(), poly2->getExteriorRing()))  return false;

    // Convert each hole inside this polygon
    for(unsigned int holeIndex = 0; holeIndex < poly1->getNumInteriorRing(); holeIndex ++) {

      // We hope they are all linear rings (closed/simple), but if not just leave it be
      if(poly1->getInteriorRingN(holeIndex)->getGeometryTypeId() == geos::geom::GEOS_LINESTRING) {
        holes1.push_back(poly1->getInteriorRingN(holeIndex));
      }

      if(poly2->getInteriorRingN(holeIndex)->getGeometryTypeId() == geos::geom::GEOS_LINESTRING) {
        holes2.push_back(poly2->getInteriorRingN(holeIndex));
      }

    }

    if(holes1.size() != holes2.size())  return false;

    for(int h1 = holes1.size() - 1; (h1 >= 0) && holes1.size(); h1 --) {
      for(int h2 = holes2.size() - 1; (h2 >= 0) && holes2.size(); h2 --) {
        if(Equal(holes1[h1], holes2[h2])) {
          // Delete holes1[h1] by replacing it with the last Polygon in holes1
          holes1[h1] = holes1[holes1.size()-1];
          holes1.resize(holes1.size() - 1);
          // Delete holes2[h2] by replacing it with the last Polygon in holes2
          holes2[h2] = holes2[holes2.size()-1];
          holes2.resize(holes2.size() - 1);
        }
      }
    }

    return (holes1.size() == 0) && (holes2.size() == 0);
  }


  bool PolygonTools::Equal(const geos::geom::LineString *lineString1,
                           const geos::geom::LineString *lineString2) {

    geos::geom::CoordinateSequence *coords1 = lineString1->getCoordinates().release();
    geos::geom::CoordinateSequence *coords2 = lineString2->getCoordinates().release();
    bool isEqual = true;

    if(coords1->getSize() != coords2->getSize()) isEqual = false;

    unsigned int index1 = 0;
    unsigned int index2 = 0;

    // -1 extra for dupicate start/end coordinates
    for(; index2 < coords2->getSize() - 1 && isEqual; index2 ++) {
      if(Equal(coords1->getAt(index1), coords2->getAt(index2)))  break;
    }

    if(index2 == coords2->getSize() - 1) isEqual = false;

    for(; index1 < coords1->getSize() - 1 && isEqual; index1 ++, index2 ++) {
      if(!Equal(coords1->getAt(index1), coords2->getAt(index2 % (coords2->getSize() - 1)))) {
        isEqual = false;
      }
    }

    delete coords1;
    delete coords2;
    return isEqual;
  }


  bool PolygonTools::Equal(const geos::geom::Coordinate &coord1,
                           const geos::geom::Coordinate &coord2) {

    if(!Equal(coord1.x, coord2.x))  return false;
    if(!Equal(coord1.y, coord2.y))  return false;
    if(!Equal(coord1.y, coord2.y))  return false;

    return true;
  }


  bool PolygonTools::Equal(const double d1, const double d2) {
    const double cutoff = 1e15;

    if(DecimalPlace(d1) != DecimalPlace(d2)) return false;

    int decimalPlace = DecimalPlace(d1);
    double factor = pow(10.0, (int)decimalPlace);

    // reduced num is in the form 0.nnnnnnnnnn...
    double reducedNum = d1 / factor;

    double round_offset = (d1 < 0) ? -0.5 : 0.5;

    // cast off the digits past the precision's place
    long long num1 = ((long long)(reducedNum * cutoff + round_offset));

    factor = pow(10.0, (int)decimalPlace);

    // reduced num is in the form 0.nnnnnnnnnn...
    reducedNum = d2 / factor;

    round_offset = (d2 < 0) ? -0.5 : 0.5;

    // cast off the digits past the precision's place
    long long num2 = ((long long)(reducedNum * cutoff + round_offset));


    return (num1 == num2);
  }


  /**
   * If the cube crosses the 0/360 boundary and does not include a pole, this will divide the
   * polygon into multiple polygons (one for each time the polygon crosses the boundry and back).
   * These polygons are put into a geos Multipolygon. If the cube does not cross the 0/360 boundary
   * then the returned Multipolygon will contain a single Polygon.
   *
   * @param polygon projection The projection to be used to convert the Xs and Ys to Lon
   *                   and Lats
   *
   * @return  Returns a pointer to a multipolygon which is the result of splitting the input
   *          parameter polygon every where it crosses the 0/360 longitude boundry. The caller
   *          assumes responsibility for deleting the returned multipolygon
   *
   */
  geos::geom::MultiPolygon *PolygonTools::SplitPolygonOn360(const geos::geom::Polygon *inPoly) {
    bool convertLon = false;
    bool negAdjust = false;
    bool newCoords = false;  //  coordinates have been adjusted
    geos::geom::CoordinateArraySequence *newLonLatPts = new geos::geom::CoordinateArraySequence();
    double lon, lat;
    double lonOffset = 0;
    geos::geom::CoordinateSequence *inPolyCoords = inPoly->getCoordinates().release();
    double prevLon = inPolyCoords->getAt(0).x;
    double prevLat = inPolyCoords->getAt(0).y;

    newLonLatPts->add(geos::geom::Coordinate(prevLon, prevLat));
    double dist = 0.0;
    for (unsigned int i = 1; i < inPolyCoords->getSize(); i++) {
      lon = inPolyCoords->getAt(i).x;
      lat = inPolyCoords->getAt(i).y;
      // check to see if you just crossed the Meridian
      if (abs(lon - prevLon) > 180 && (prevLat != 90 && prevLat != -90)) {
        newCoords = true;
        // if you were already converting then stop (crossed Meridian even number of times)
        if (convertLon) {
          convertLon = false;
          lonOffset = 0;
        }
        else {   // Need to start converting again, deside how to adjust coordinates
          if ((lon - prevLon) > 0) {
            lonOffset = -360.;
            negAdjust = true;
          }
          else if ((lon - prevLon) < 0) {
            lonOffset = 360.;
            negAdjust = false;
          }
          convertLon = true;
        }
      }

      // Change to a minimum calculation
      if (newCoords  &&  dist == 0.0) {
        double longitude = (lon + lonOffset) - prevLon;
        double latitude = lat - prevLat;
        dist = std::sqrt((longitude * longitude) + (latitude * latitude));
      }

      // add coord
      newLonLatPts->add(geos::geom::Coordinate(lon + lonOffset, lat));

      // set current to old
      prevLon = lon;
      prevLat = lat;
    }

    delete inPolyCoords;

    // Nothing was done so return
    if (!newCoords) {
      geos::geom::Polygon *newPoly = globalFactory->createPolygon
                                     (globalFactory->createLinearRing(newLonLatPts), NULL);
      geos::geom::MultiPolygon *multi_polygon = PolygonTools::MakeMultiPolygon(newPoly);
      delete newLonLatPts;
      return multi_polygon;
    }

    // bisect into seperate polygons
    try {
      geos::geom::Polygon *newPoly = globalFactory->createPolygon
                                     (globalFactory->createLinearRing(newLonLatPts), NULL);

      geos::geom::CoordinateArraySequence *pts = new geos::geom::CoordinateArraySequence();
      geos::geom::CoordinateArraySequence *pts2 = new geos::geom::CoordinateArraySequence();

      // Depending on direction of compensation bound accordingly
      //***************************************************

      // please verify correct if you change these values
      //***************************************************
      if (negAdjust) {
        pts->add(geos::geom::Coordinate(0., 90.));
        pts->add(geos::geom::Coordinate(-360., 90.));
        pts->add(geos::geom::Coordinate(-360., -90.));
        pts->add(geos::geom::Coordinate(0., -90.));
        for (double lat = -90.0 + dist; lat < 90.0; lat += dist) {
          pts->add(geos::geom::Coordinate(0.0, lat));
        }
        pts->add(geos::geom::Coordinate(0., 90.));
        pts2->add(geos::geom::Coordinate(0., 90.));
        pts2->add(geos::geom::Coordinate(360., 90.));
        pts2->add(geos::geom::Coordinate(360., -90.));
        pts2->add(geos::geom::Coordinate(0., -90.));
        for (double lat = -90.0 + dist; lat < 90.0; lat += dist) {
          pts2->add(geos::geom::Coordinate(0.0, lat));
        }
        pts2->add(geos::geom::Coordinate(0., 90.));
      }
      else {
        pts->add(geos::geom::Coordinate(360., 90.));
        pts->add(geos::geom::Coordinate(720., 90.));
        pts->add(geos::geom::Coordinate(720., -90.));
        pts->add(geos::geom::Coordinate(360., -90.));
        for (double lat = -90.0 + dist; lat < 90.0; lat += dist) {
          pts->add(geos::geom::Coordinate(360.0, lat));
        }
        pts->add(geos::geom::Coordinate(360., 90.));
        pts2->add(geos::geom::Coordinate(360., 90.));
        pts2->add(geos::geom::Coordinate(0., 90.));
        pts2->add(geos::geom::Coordinate(0., -90.));
        pts2->add(geos::geom::Coordinate(360., -90.));
        for (double lat = -90.0 + dist; lat < 90.0; lat += dist) {
          pts2->add(geos::geom::Coordinate(360.0, lat));
        }
        pts2->add(geos::geom::Coordinate(360., 90.));
      }

      geos::geom::Polygon *boundaryPoly = globalFactory->createPolygon
                                          (globalFactory->createLinearRing(pts), NULL);
      geos::geom::Polygon *boundaryPoly2 = globalFactory->createPolygon
                                           (globalFactory->createLinearRing(pts2), NULL);
      /*------------------------------------------------------------------------
      /  Intersecting the original polygon (converted coordinates) with the
      /  boundary polygons will create the multi polygons with the converted coordinates.
      /  These will need to be converted back to the original coordinates.
      /-----------------------------------------------------------------------*/
      geos::geom::Geometry *intersection = PolygonTools::Intersect(newPoly, boundaryPoly);
      geos::geom::MultiPolygon *convertPoly = PolygonTools::MakeMultiPolygon(intersection);
      delete intersection;

      intersection = PolygonTools::Intersect(newPoly, boundaryPoly2);
      geos::geom::MultiPolygon *convertPoly2 = PolygonTools::MakeMultiPolygon(intersection);
      delete intersection;

      /*------------------------------------------------------------------------
      / Adjust points created in the negative space or >360 space to be back in
      / the 0-360 world.  This will always only need to be done on convertPoly.
      / Then add geometries to finalpolys.
      /-----------------------------------------------------------------------*/
      vector<geos::geom::Geometry *> *finalpolys = new vector<geos::geom::Geometry *>;
      geos::geom::Geometry *newGeom = NULL;

      for (unsigned int i = 0; i < convertPoly->getNumGeometries(); i++) {
        newGeom = (convertPoly->getGeometryN(i))->clone().release();
        geos::geom::CoordinateSequence *pts3 = convertPoly->getGeometryN(i)->getCoordinates().release();
        geos::geom::CoordinateArraySequence *newLonLatPts = new geos::geom::CoordinateArraySequence();

        // fix the points
        for (unsigned int k = 0; k < pts3->getSize() ; k++) {
          double lon = pts3->getAt(k).x;
          double lat = pts3->getAt(k).y;
          if (negAdjust) {
            lon = lon + 360;
          }
          else {
            lon = lon - 360;
          }
          newLonLatPts->add(geos::geom::Coordinate(lon, lat), k);
        }
        // Add the points to polys
        finalpolys->push_back(globalFactory->createPolygon
                              (globalFactory->createLinearRing(newLonLatPts), NULL));
      }

      // This loop is over polygons that will always be in 0-360 space no need to convert
      for (unsigned int i = 0; i < convertPoly2->getNumGeometries(); i++) {
        newGeom = (convertPoly2->getGeometryN(i))->clone().release();
        finalpolys->push_back(newGeom);
      }

      geos::geom::MultiPolygon *multi_polygon = globalFactory->createMultiPolygon(finalpolys);

      delete newLonLatPts;
      delete pts;
      delete pts2;
      return multi_polygon;
    }
    catch(geos::util::IllegalArgumentException *geosIll) {
      std::string msg = "Unable to split polygon on longitude boundry (SplitPolygonOn360) due to ";
      msg += "geos illegal argument [" + IString(geosIll->what()) + "]";
      delete geosIll;
      throw IException(IException::Unknown, msg, _FILEINFO_);
    }
    catch(geos::util::GEOSException *geosExc) {
      std::string msg = "Unable to split polygon on longitude boundry (SplitPolygonOn360) due to ";
      msg += "geos exception [" + IString(geosExc->what()) + "]";
      delete geosExc;
      throw IException(IException::Unknown, msg, _FILEINFO_);
    }
    catch(IException &e) {
      std::string msg = "Unable to split polygon on longitude boundry (SplitPolygonOn360) due to ";
      msg += "isis operation exception [" + IString(e.what()) + "]";
      throw IException(e, IException::Unknown, msg, _FILEINFO_);
    }
    catch(...) {
      std::string msg = "Unable to split polygon on longitude boundry (SplitPolygonOn360) due to ";
      msg += "unknown exception";
      throw IException(IException::Unknown, msg, _FILEINFO_);
    }
  }
} // end namespace isis

