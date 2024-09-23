/* @file
 * $Revision: 1.16 $
 * $Date: 2009/12/29 23:03:47 $
 * $Id: CamTools.cpp,v 1.16 2009/12/29 23:03:47 ehyer Exp $
 *
 *   Unless noted otherwise, the portions of Isis written by the USGS are
 *   public domain. See individual third-party library and package descriptions
 *   for intellectual property information, user agreements, and related
 *   information.
 *
 *   Although Isis has been used by the USGS, no warranty, expressed or
 *   implied, is made by the USGS as to the accuracy and functioning of such
 *   software and related material nor shall the fact of distribution
 *   constitute any such warranty, and no responsibility is assumed by the
 *   USGS in connection therewith.
 *
 *   For additional information, launch
 *   $ISISROOT/doc//documents/Disclaimers/Disclaimers.html
 *   in a browser or see the Privacy &amp; Disclaimers page on the Isis website,
 *   http://isis.astrogeology.usgs.gov, and the USGS privacy and disclaimers on
 *   http://www.usgs.gov/privdacy.html.
 */
#include "CamTools.h"

#include <algorithm>
#include <cmath>
#include <iomanip>
#include <iostream>
#include <numeric>
#include <sstream>
#include <string>
#include <vector>

#include <SpiceUsr.h>

#include <geos/geom/GeometryFactory.h>
#include <geos/geom/Geometry.h>
#include <geos/geom/Point.h>

#include "CameraFactory.h"
#include "Distance.h"
#include "IException.h"
#include "IString.h"
#include "iTime.h"
#include "Latitude.h"
#include "Longitude.h"
#include "PolygonTools.h"
#include "ProjectionFactory.h"
#include "Pvl.h"
#include "SpecialPixel.h"
#include "Statistics.h"
#include "SurfacePoint.h"
#include "Target.h"
#include "TProjection.h"

using namespace std;

namespace Isis {

  /**
   * @brief Round values to specified precision
   *
   * @param value  Value to round
   * @param precision Precision to round value to
   *
   * @return double Rounded value
   */
  inline double SetRound(double value, const int precision) {
    double scale = pow(10.0, precision);
    value = round(value * scale) / scale;
    return (value);
  }

  /**
   * @brief Helper function to convert values to doubles
   *
   * @param T Type of value to convert
   * @param value Value to convert
   *
   * @return double Converted value
   */
  template <typename T> double ToDouble(const T &value) {
    return (IString(value).Trim(" \r\t\n").ToDouble());
  }

  /**
   * @brief Helper function to convert values to strings
   *
   * @param T Type of value to convert
   * @param value Value to convert
   *
   * @return string Converted value
   */
  template <typename T> QString ToString(const T &value) {
    return (QString::number(value).trimmed());
  }



  bool BandGeometry::isPointValid(const double &sample, const double &line,
                                  const Camera *camera) const {
    int nl(_nLines), ns(_nSamps);
    if(camera != 0) {
      nl = camera->Lines();
      ns = camera->Samples();
    }

    if(line < 0.5) return (false);
    if(line > (nl + 0.5)) return (false);
    if(sample < 0.5) return (false);
    if(sample > (ns + 0.5)) return (false);
    return (true);
  }

  bool BandGeometry::hasCenterGeometry() const {
    BandPropertiesListConstIter b;
    for(b = _gBandList.begin() ; b != _gBandList.end() ; ++b) {
      if(!IsSpecial(b->centerLatitude)) return (true);
    }
    // No valid center exists
    return (false);
  }

  /**
   * @brief Check geometry for presence of limb
   *
   * This method checks corner geometry coordinates for the presence of a planet
   * limb.  This is a simple check for validity of the latitude coordinate at each
   * image corner.  If any of them are invalid (Nulls), then it is deamed to have
   * a limb in the image.
   *
   * Note that this check is only valid if the determination of the geometry has
   * been performed.  So care should be used as to when this check is made.
   *
   *
   * @return bool  True if one of the corner latitude coordinates is NULL.
   */
  bool BandGeometry::hasLimb() const {
    BandPropertiesListConstIter b;
    for(b = _gBandList.begin() ; b != _gBandList.end() ; ++b) {
      if(IsSpecial(b->upperLeftLatitude)) return (true);
      if(IsSpecial(b->upperRightLatitude)) return (true);
      if(IsSpecial(b->lowerRightLatitude)) return (true);
      if(IsSpecial(b->lowerLeftLatitude)) return (true);
    }
    // All outer geometry points are defined
    return (false);
  }


  void BandGeometry::destruct() {
    // Ensure this can be applied for reentrant operations
    _gBandList.clear();
    for_each(_polys.begin(), _polys.end(), DeleteObject());
    delete _combined;
    _combined = 0;
    _radius = 1.0;
  }

  void BandGeometry::collect(Camera &camera, Cube &cube, bool doGeometry,
                             bool doPolygon, bool getFootBlob,
                             bool increasePrecision) {
    destruct();

    _nLines  = cube.lineCount();
    _nSamps  = cube.sampleCount();
    _nBands  = cube.bandCount();

    //  Compute average planetary radius in meters.  This is used as a fallback
    //  to compute surface area if no geoemetry has a center intersect point.
    Distance radii[3];
    camera.radii(radii);
    _radius = ((radii[0] + radii[1] + radii[2]) / 3.0).meters();

    double cLine = _nLines;
    double cSamp = _nSamps;
    double centerLine = cLine / 2.0;
    double centerSamp = cSamp / 2.0;

    // Check to determine if geometry is band independant
    _isBandIndependent = camera.IsBandIndependent();
    _hasCenterGeom = false;
    int nbands = (_isBandIndependent ? 1 : _nBands);
    for(int band = 0 ; band < nbands ; band++) {
      //  Compute band geometry properties
      GProperties g;
      g.lines = _nLines;
      g.samples = _nSamps;
      g.bands = _nBands;
      g.band = band + 1;
      camera.SetBand(band + 1);
      g.realBand = cube.physicalBand(band + 1);


      g.target = camera.target()->name();

      iTime t1(camera.cacheStartTime());
      g.startTime = t1.UTC();
      iTime t2(camera.cacheEndTime());
      g.endTime = t2.UTC();

      g.centerLine = centerLine;
      g.centerSamp = centerSamp;

      // Now compute elements for the center pixel
      if(camera.SetImage(centerSamp, centerLine)) {
        _hasCenterGeom = true;
        g.centerLatitude  = camera.UniversalLatitude();
        g.centerLongitude = camera.UniversalLongitude();
        g.radius = camera.LocalRadius().meters();

        g.rightAscension = camera.RightAscension();
        g.declination    = camera.Declination();

        g.sampRes = camera.SampleResolution();
        g.lineRes = camera.LineResolution();

        g.obliqueSampRes = camera.ObliqueSampleResolution();
        g.obliqueLineRes = camera.ObliqueLineResolution();
        g.obliquePixelRes = camera.ObliquePixelResolution();
        g.obliqueDetectorRes = camera.ObliqueDetectorResolution();

        g.solarLongitude = camera.solarLongitude().degrees();
        g.northAzimuth = camera.NorthAzimuth();
        g.offNader = camera.OffNadirAngle();
        g.subSolarAzimuth = camera.SunAzimuth();
        g.subSpacecraftAzimuth = camera.SpacecraftAzimuth();
        g.localSolartime = camera.LocalSolarTime();
        g.targetCenterDistance = camera.targetCenterDistance();
        g.slantDistance = camera.SlantDistance();

        camera.subSolarPoint(g.subSolarLatitude, g.subSolarLongitude);
        g.subSolarGroundAzimuth = camera.GroundAzimuth(g.centerLatitude,
                                  g.centerLongitude,
                                  g.subSolarLatitude,
                                  g.subSolarLongitude);
        camera.subSpacecraftPoint(g.subSpacecraftLatitude, g.subSpacecraftLongitude);
        g.subSpacecraftGroundAzimuth = camera.GroundAzimuth(g.centerLatitude,
                                       g.centerLongitude,
                                       g.subSpacecraftLatitude,
                                       g.subSpacecraftLongitude);


        //  solve for the parallax and shadow stuff
        g.phase = camera.PhaseAngle();
        g.emi   = camera.EmissionAngle();
        g.inc   = camera.IncidenceAngle();

        //  Parallax values (Corrected 2012-11-23, KJB)
        if(!IsSpecial(g.emi) && !IsSpecial(g.subSpacecraftGroundAzimuth)) {
          double emi_r  = DegToRad(g.emi);
          double ssga_r = DegToRad(g.subSpacecraftGroundAzimuth);
          g.parallaxx = -tan(emi_r) * cos(ssga_r);
          g.parallaxy =  tan(emi_r) * sin(ssga_r);
        }

        // Shadow values (Corrected 2012-11-23, KJB)
        if(!IsSpecial(g.inc) && !IsSpecial(g.subSolarGroundAzimuth)) {
          double inc_r  = DegToRad(g.inc);
          double ssga_r = DegToRad(g.subSolarGroundAzimuth);
          g.shadowx = -tan(inc_r) * cos(ssga_r);
          g.shadowy =  tan(inc_r) * sin(ssga_r);
        }
      }
      //  OK...now get corner pixel geometry.  NOTE this resets image
      //  pixel location from center!!!
      if(camera.SetImage(1.0, 1.0)) {
        g.upperLeftLongitude = camera.UniversalLongitude();
        g.upperLeftLatitude =  camera.UniversalLatitude();
      }

      if(camera.SetImage(1.0, cLine)) {
        g.lowerLeftLongitude = camera.UniversalLongitude();
        g.lowerLeftLatitude =  camera.UniversalLatitude();
      }

      if(camera.SetImage(cSamp, cLine)) {
        g.lowerRightLongitude = camera.UniversalLongitude();
        g.lowerRightLatitude =  camera.UniversalLatitude();
      }

      if(camera.SetImage(cSamp, 1.0)) {
        g.upperRightLongitude = camera.UniversalLongitude();
        g.upperRightLatitude =  camera.UniversalLatitude();
      }

      double minRes = camera.LowestImageResolution();
      double maxRes = camera.HighestImageResolution();
      if(!(IsSpecial(minRes) || IsSpecial(maxRes))) {
        g.grRes = (minRes + maxRes) / 2.0;
      }

      Pvl camMap;
      camera.BasicMapping(camMap);
      _mapping = camMap;

      // Test for interesting intersections
      if(camera.IntersectsLongitudeDomain(camMap)) g.hasLongitudeBoundary = true;
      camera.SetBand(band + 1);
      if(camera.SetUniversalGround(90.0, 0.0)) {
        if(isPointValid(camera.Sample(), camera.Line(), &camera)) {
          g.hasNorthPole = true;
        }
      }
      if(camera.SetUniversalGround(-90.0, 0.0)) {
        if(isPointValid(camera.Sample(), camera.Line(), &camera)) {
          g.hasSouthPole = true;
        }
      }

      if(doPolygon) {
        // Now compute the the image polygon
        ImagePolygon poly;
        poly.Incidence(_maxIncidence);
        poly.Emission(_maxEmission);
        poly.EllipsoidLimb(true);  // Allow disabling of shape model for limbs
        poly.Create(cube, _sampleInc, _lineInc, 1, 1, 0, 0, band + 1,
            increasePrecision);
        geos::geom::MultiPolygon *multiP = poly.Polys();
        _polys.push_back(multiP->clone().release());
        if(_combined == 0) {
          _combined = multiP->clone().release();
        }
        else {
          //  Construct composite (union) polygon
          geos::geom::Geometry *old(_combined);
          _combined = old->Union(multiP).release();
          delete old;
        }

        // multiP is freed by ImagePolygon object
        _mapping = getProjGeometry(camera, multiP, g);
      }

      if (getFootBlob && band == 0) {
        // Read the footprint from the image labels
        ImagePolygon poly = cube.readFootprint();
        geos::geom::MultiPolygon *multiP = poly.Polys();
        _polys.push_back(multiP->clone().release());
        _combined = multiP->clone().release();
        _mapping = getProjGeometry(camera, multiP, g);
      }

      // Save off this band geometry property
      _gBandList.push_back(g);
    }


    //  Compute the remainder of the summary bands since some of the operations
    //  need the camera model
    _summary = getGeometrySummary();
    if((size() != 1) && doPolygon) {
      geos::geom::MultiPolygon *multiP = makeMultiPolygon(_combined);
      _mapping = getProjGeometry(camera, multiP, _summary);
      delete multiP;
    }

    return;
  }


  void BandGeometry::generateGeometryKeys(PvlObject &pband) {
    if(size() <= 0) {
      std::string mess = "No Band geometry available!";
      throw IException(IException::Programmer, mess, _FILEINFO_);
    }

    GProperties g = getGeometrySummary();

//geometry keywords for band output
    pband += PvlKeyword("BandsUsed", toString(size()));
    pband += PvlKeyword("ReferenceBand", toString(g.band));
    pband += PvlKeyword("OriginalBand", toString(g.realBand));

    pband += PvlKeyword("Target", g.target.toStdString());

    pband += PvlKeyword("StartTime", g.startTime.toStdString());
    pband += PvlKeyword("EndTime", g.endTime.toStdString());

    pband += ValidateKey("CenterLine", g.centerLine);
    pband += ValidateKey("CenterSample", g.centerSamp);
    pband += ValidateKey("CenterLatitude", g.centerLatitude);
    pband += ValidateKey("CenterLongitude", g.centerLongitude);
    pband += ValidateKey("CenterRadius", g.radius);

    pband += ValidateKey("RightAscension", g.rightAscension);
    pband += ValidateKey("Declination", g.declination);

    pband += ValidateKey("UpperLeftLongitude", g.upperLeftLongitude);
    pband += ValidateKey("UpperLeftLatitude", g.upperLeftLatitude);
    pband += ValidateKey("LowerLeftLongitude", g.lowerLeftLongitude);
    pband += ValidateKey("LowerLeftLatitude", g.lowerLeftLatitude);
    pband += ValidateKey("LowerRightLongitude", g.lowerRightLongitude);
    pband += ValidateKey("LowerRightLatitude", g.lowerRightLatitude);
    pband += ValidateKey("UpperRightLongitude", g.upperRightLongitude);
    pband += ValidateKey("UpperRightLatitude", g.upperRightLatitude);

    pband += ValidateKey("PhaseAngle", g.phase);
    pband += ValidateKey("EmissionAngle", g.emi);
    pband += ValidateKey("IncidenceAngle", g.inc);

    pband += ValidateKey("NorthAzimuth", g.northAzimuth);
    pband += ValidateKey("OffNadir", g.offNader);
    pband += ValidateKey("SolarLongitude", g.solarLongitude);
    pband += ValidateKey("LocalTime", g.localSolartime);
    pband += ValidateKey("TargetCenterDistance", g.targetCenterDistance);
    pband += ValidateKey("SlantDistance", g.slantDistance);

    double aveRes(Null);
    if(!IsSpecial(g.sampRes) && !IsSpecial(g.lineRes)) {
      aveRes = (g.sampRes + g.lineRes) / 2.0;
    }

    pband += ValidateKey("SampleResolution", g.sampRes);
    pband += ValidateKey("LineResolution", g.lineRes);
    pband += ValidateKey("PixelResolution", aveRes);
    pband += ValidateKey("MeanGroundResolution", g.grRes);

    pband += ValidateKey("SubSolarAzimuth", g.subSolarAzimuth);
    pband += ValidateKey("SubSolarGroundAzimuth", g.subSolarGroundAzimuth);
    pband += ValidateKey("SubSolarLatitude", g.subSolarLatitude);
    pband += ValidateKey("SubSolarLongitude", g.subSolarLongitude);

    pband += ValidateKey("SubSpacecraftAzimuth", g.subSpacecraftAzimuth);
    pband += ValidateKey("SubSpacecraftGroundAzimuth", g.subSpacecraftGroundAzimuth);
    pband += ValidateKey("SubSpacecraftLatitude", g.subSpacecraftLatitude);
    pband += ValidateKey("SubSpacecraftLongitude", g.subSpacecraftLongitude);

    pband += ValidateKey("ParallaxX", g.parallaxx);
    pband += ValidateKey("ParallaxY", g.parallaxy);

    pband += ValidateKey("ShadowX", g.shadowx);
    pband += ValidateKey("ShadowY", g.shadowy);

    //  Determine if image crosses Longitude domain
    if(g.hasLongitudeBoundary) {
      pband += PvlKeyword("HasLongitudeBoundary", "TRUE");
    }
    else {
      pband += PvlKeyword("HasLongitudeBoundary", "FALSE");
    }

    //  Add test for North pole in image
    if(g.hasNorthPole) {
      pband += PvlKeyword("HasNorthPole", "TRUE");
    }
    else {
      pband += PvlKeyword("HasNorthPole", "FALSE");
    }

    //  Add test for South pole in image
    if(g.hasSouthPole) {
      pband += PvlKeyword("HasSouthPole", "TRUE");
    }
    else {
      pband += PvlKeyword("HasSouthPole", "FALSE");
    }

    pband += ValidateKey("ObliqueSampleResolution", g.obliqueSampRes);
    pband += ValidateKey("ObliqueLineResolution", g.obliqueLineRes);
    pband += ValidateKey("ObliquePixelResolution", g.obliquePixelRes);
    pband += ValidateKey("ObliqueDetectorResolution", g.obliqueDetectorRes);

    return;
  }

  BandGeometry::GProperties BandGeometry::getGeometrySummary() const {
    if(_isBandIndependent  || (size() == 1)) {
      return (_gBandList[0]);
    }

    //  Get the centroid point of the union polygon
    double plon(Null), plat(Null);
    if(_combined != 0) {
      geos::geom::Point *center = _combined->getCentroid().release();
      plon = center->getX();
      plat = center->getY();
      delete center;
    }

    GProperties bestBand;
    double centerDistance(DBL_MAX);

    GProperties corners;
    double ulDist(DBL_MIN), urDist(DBL_MIN),
           lrDist(DBL_MIN), llDist(DBL_MIN);

    double radius = getRadius();

    BandPropertiesListConstIter b;
    for(b = _gBandList.begin() ; b != _gBandList.end() ; ++b) {
      double thisDist;

      // Ensure the center latitude/logitude is defined (typically occurs when
      // no polygon data is available).  This scheme uses the first one defined.
      if(IsSpecial(plat) || IsSpecial(plon)) {
        plat = b->centerLatitude;
        plon = b->centerLongitude;
      }

      // Now check all data
      bool isCloser = isDistShorter(centerDistance, plat, plon,
                                    b->centerLatitude, b->centerLongitude,
                                    radius, thisDist) ;
      if(isCloser) {
        bestBand = *b;
        centerDistance = thisDist;
      }

      //  Do upper left and right corners
      isCloser = isDistShorter(ulDist, plat, plon,
                               b->upperLeftLatitude, b->upperLeftLongitude,
                               radius, thisDist);
      if(!isCloser) {
        corners.upperLeftLatitude = b->upperLeftLatitude;
        corners.upperLeftLongitude = b->upperLeftLongitude;
        ulDist = thisDist;
      }

      isCloser = isDistShorter(urDist, plat, plon,
                               b->upperRightLatitude, b->upperRightLongitude,
                               radius, thisDist);
      if(!isCloser) {
        corners.upperRightLatitude = b->upperRightLatitude;
        corners.upperRightLongitude = b->upperRightLongitude;
        urDist = thisDist;
      }

      //  Do lower left and right corners
      isCloser = isDistShorter(llDist, plat, plon,
                               b->lowerLeftLatitude, b->lowerLeftLongitude,
                               radius, thisDist);
      if(!isCloser) {
        corners.lowerLeftLatitude = b->lowerLeftLatitude;
        corners.lowerLeftLongitude = b->lowerLeftLongitude;
        llDist = thisDist;
      }

      isCloser = isDistShorter(lrDist, plat, plon,
                               b->lowerRightLatitude, b->lowerRightLongitude,
                               radius, thisDist);
      if(!isCloser) {
        corners.lowerRightLatitude = b->lowerRightLatitude;
        corners.lowerRightLongitude = b->lowerRightLongitude;
        lrDist = thisDist;
      }
    }

    // Add the corners to the returning property
    bestBand.upperLeftLatitude = corners.upperLeftLatitude;
    bestBand.upperLeftLongitude = corners.upperLeftLongitude;
    bestBand.upperRightLatitude = corners.upperRightLatitude;
    bestBand.upperRightLongitude = corners.upperRightLongitude;
    bestBand.lowerLeftLatitude = corners.lowerLeftLatitude;
    bestBand.lowerLeftLongitude = corners.lowerLeftLongitude;
    bestBand.lowerRightLatitude = corners.lowerRightLatitude;
    bestBand.lowerRightLongitude = corners.lowerRightLongitude;
    return (bestBand);
  }


  Pvl BandGeometry::getProjGeometry(Camera &camera,
                                    geos::geom::MultiPolygon *footprint,
                                    GProperties &g) {
    // Get basic projection information.  Assumes a Sinusoidal projection with
    // East 360 longitude domain and planetocentric laitudes.
    Pvl sinuMap;
    camera.BasicMapping(sinuMap);
    PvlGroup &mapping = sinuMap.findGroup("Mapping");

    double clon = g.centerLongitude;
    double minLon = (double) mapping["MinimumLongitude"];
    double maxLon = (double) mapping["MaximumLongitude"];
    if(IsSpecial(clon)) clon = (minLon + maxLon) / 2.0;

    //  Make adjustments for center projection type/ranges.
    //  To be consistant with other implementations, do not
    //  convert poles to 180 domain.
    geos::geom::MultiPolygon *poly180(0), *poly(footprint);
    if(g.hasLongitudeBoundary) {
      if(!(g.hasNorthPole || g.hasSouthPole)) {
        // Convert the mapping group contents to 180 Longitude domain
        PvlKeyword &ldkey = mapping["LongitudeDomain"];
        ldkey.setValue("180");

        PvlKeyword &minkey = mapping["MinimumLongitude"];
        PvlKeyword &maxkey = mapping["MaximumLongitude"];
        minkey.setValue("-180.0");
        maxkey.setValue("180.0");

        // Compute new ranges
        double minLat180, maxLat180, minLon180, maxLon180;
        camera.GroundRange(minLat180, maxLat180, minLon180, maxLon180, sinuMap);
        minkey.setValue(ToString(minLon180).toStdString());
        maxkey.setValue(ToString(maxLon180).toStdString());
        clon = (minLon180 + maxLon180) / 2.0;

        // Convert the polygon to 180 domain
        poly = poly180 = PolygonTools::To180(footprint);
      }
    }

    mapping += PvlKeyword("CenterLongitude", toString(clon));

    TProjection *sinu = (TProjection *) ProjectionFactory::Create(sinuMap, true);
    geos::geom::MultiPolygon *sPoly = PolygonTools::LatLonToXY(*poly, sinu);
    geos::geom::Point *center = sPoly->getCentroid().release();

    sinu->SetCoordinate(center->getX(), center->getY());
    g.centroidLongitude = TProjection::To360Domain(sinu->UniversalLongitude());
    g.centroidLatitude  = sinu->UniversalLatitude();
    g.surfaceArea = sPoly->getArea() / (1000.0 * 1000.0);
    delete center;
    delete sPoly;
    delete sinu;
    delete poly180;

    if(camera.SetUniversalGround(g.centroidLatitude, g.centroidLongitude)) {
      g.centroidLine = camera.Line();
      g.centroidSample = camera.Sample();
      g.centroidRadius = camera.LocalRadius().meters();
    }

    return (sinuMap);
  }

  void BandGeometry::generatePolygonKeys(PvlObject &pband) {
    if(size() <= 0) {
      std::string mess = "No Band geometry available!";
      throw IException(IException::Programmer, mess, _FILEINFO_);
    }

    // Compute surface area - already done in collection phase
    double radius = getRadius();
    double globalCoverage(Null);
    if(!IsSpecial(radius)) {
      double globalArea = 4.0 * pi_c() * (radius * radius) / (1000.0 * 1000.0);
      globalCoverage = _summary.surfaceArea / globalArea * 100.0;
      globalCoverage = SetRound(globalCoverage, 6);
    }

    pband += ValidateKey("CentroidLine", _summary.centroidLine);
    pband += ValidateKey("CentroidSample", _summary.centroidSample);
    pband += ValidateKey("CentroidLatitude", _summary.centroidLatitude);
    pband += ValidateKey("CentroidLongitude", _summary.centroidLongitude);
    pband += ValidateKey("CentroidRadius", _summary.centroidRadius, "meters");
    pband += ValidateKey("SurfaceArea", _summary.surfaceArea, "km^2");
    pband += ValidateKey("GlobalCoverage", globalCoverage, "percent");
    if(_combined != 0) {
      pband += PvlKeyword("SampleIncrement", toString(_sampleInc));
      pband += PvlKeyword("LineIncrement", toString(_lineInc));
      if(_combined->getGeometryTypeId() != geos::geom::GEOS_MULTIPOLYGON) {
        geos::geom::MultiPolygon *geom = makeMultiPolygon(_combined);
        pband += PvlKeyword("GisFootprint", geom->toString());
        delete geom;
      }
      else {
        pband += PvlKeyword("GisFootprint", _combined->toString());
      }
    }
    else {
      pband += PvlKeyword("GisFootprint", "Null");
    }

    // Add the mapping group used to project polygon
    pband.addGroup(_mapping.findGroup("Mapping"));
    return;
  }

  double BandGeometry::getRadius() const {
    Statistics polyRadius, centRadius;
    BandPropertiesListConstIter b;
    for(b = _gBandList.begin() ; b != _gBandList.end() ; ++b) {
      polyRadius.AddData(b->centroidRadius);
      centRadius.AddData(b->radius);
    }
    double radius = polyRadius.Average();
    if(IsSpecial(radius)) radius = centRadius.Average();
    if(IsSpecial(radius)) radius = _radius;
    return (radius);
  }

  double BandGeometry::getPixelResolution() const {
    Statistics groundRes, pixelRes;
    BandPropertiesListConstIter b;
    for(b = _gBandList.begin() ; b != _gBandList.end() ; ++b) {
      pixelRes.AddData(b->sampRes);
      pixelRes.AddData(b->lineRes);
      groundRes.AddData(b->grRes);
      pixelRes.AddData(b->obliqueLineRes);
      pixelRes.AddData(b->obliqueSampRes);
      pixelRes.AddData(b->obliquePixelRes);
      pixelRes.AddData(b->obliqueDetectorRes);
    }

    double res = groundRes.Average();
    if(IsSpecial(res)) res = pixelRes.Average();
    return (res);
  }

  double BandGeometry::getPixelsPerDegree(double pixres,
                                          double radius) const {
    double circumference = 2.0  * pi_c() * radius;
    double metersPerDegree = circumference / 360.0;
    double pixelsPerDegree = metersPerDegree / pixres;
    return (pixelsPerDegree);
  }


  bool BandGeometry::isDistShorter(double bestDist, double lat1, double lon1,
                                   double lat2, double lon2, double radius,
                                   double &thisDist) const {
    if(IsSpecial(lat1)) return (false);
    if(IsSpecial(lon1)) return (false);
    if(IsSpecial(lat2)) return (false);
    if(IsSpecial(lon2)) return (false);
    if(IsSpecial(radius)) return (false);

    SurfacePoint point1(
      Latitude(lat1, Angle::Degrees),
      Longitude(lon1, Angle::Degrees),
      Distance(radius, Distance::Meters));
    SurfacePoint point2(
      Latitude(lat2, Angle::Degrees),
      Longitude(lon2, Angle::Degrees),
      Distance(radius, Distance::Meters));
    thisDist = point1.GetDistanceToPoint(point2,
                      Distance(radius, Distance::Meters)).meters();
    return (thisDist < bestDist);
  }

  geos::geom::MultiPolygon *BandGeometry::makeMultiPolygon(
    geos::geom::Geometry *g) const {
    vector<const geos::geom::Geometry *> polys;
    polys.push_back(g);
    const geos::geom::GeometryFactory *gfactory = geos::geom::GeometryFactory::getDefaultInstance();
    return (gfactory->createMultiPolygon(polys).release());
  }


} // namespace Isis
