/**
 * @file
 * $Revision: 1.7 $
 * $Date: 2010/06/07 22:42:38 $
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
 *   http://www.usgs.gov/privacy.html.
 */
#include "CameraRingsPointInfo.h"

#include "Brick.h"
#include "Camera.h"
#include "CameraPointInfo.h"
#include "Cube.h"
#include "CubeManager.h"
#include "Distance.h"
#include "IException.h"
#include "iTime.h"
#include "Longitude.h"
#include "RingPlaneProjection.h"
#include "PvlGroup.h"

using namespace std;

namespace Isis {
  CameraRingsPointInfo::CameraRingsPointInfo() : CameraPointInfo(){};
  CameraRingsPointInfo::~CameraRingsPointInfo(){};


  /**
   * SetImage sets a sample, line image coordinate in the camera
   * so data can be accessed.
   *
   * @param sample A sample coordinate in or almost in the cube
   * @param line A line coordinate in or almost in the cubei
   *
   * @return PvlGroup* The pertinent data from the Camera class on
   *         the point. Ownership is passed to caller.
   */
  // PvlGroup *CameraRingsPointInfo::SetImage(const double sample, const double line,
  //                                     const bool outside, const bool errors) {
  //   if (CheckCube()) {
  //     bool passed = camera->SetImage(sample, line);
  //     return GetPointInfo(passed, outside, errors);
  //   }
    // Should never get here, error will be thrown in CheckCube()
  //   return NULL;
  // }


  /**
   * GetPointInfo builds the PvlGroup containing all the important
   * information derived from the Camera.
   *
   * @return PvlGroup* Data taken directly from the Camera and
   *         drived from Camera information. Ownership passed.
   */
  PvlGroup *CameraRingsPointInfo::GetPointInfo(bool passed, bool allowOutside, bool allowErrors) {
    PvlGroup *gp = new PvlGroup("GroundPoint");
    {
      gp->addKeyword(PvlKeyword("Filename"));
      gp->addKeyword(PvlKeyword("Sample"));
      gp->addKeyword(PvlKeyword("Line"));
      gp->addKeyword(PvlKeyword("PixelValue"));
      gp->addKeyword(PvlKeyword("RightAscension"));
      gp->addKeyword(PvlKeyword("Declination"));

      // gp->addKeyword(PvlKeyword("PlanetocentricLatitude"));
      // gp->addKeyword(PvlKeyword("PlanetographicLatitude"));
      // gp->addKeyword(PvlKeyword("PositiveEast360Longitude"));
      // gp->addKeyword(PvlKeyword("PositiveEast180Longitude"));
      // gp->addKeyword(PvlKeyword("PositiveWest360Longitude"));
      // gp->addKeyword(PvlKeyword("PositiveWest180Longitude"));

      // Should these be prograde instead of counterclockwise and retrograde for clockwise?
      gp->addKeyword(PvlKeyword("LocalRingRadius"));
      gp->addKeyword(PvlKeyword("CounterClockwise360RingLongitude"));
      gp->addKeyword(PvlKeyword("CounterClockwise180RingLongitude"));
      gp->addKeyword(PvlKeyword("Clockwise360RingLongitude"));
      gp->addKeyword(PvlKeyword("Clockwise180RingLongitude"));
      gp->addKeyword(PvlKeyword("BodyFixedCoordinate"));
      gp->addKeyword(PvlKeyword("SampleResolution"));
      gp->addKeyword(PvlKeyword("LineResolution"));
      gp->addKeyword(PvlKeyword("SpacecraftPosition"));
      gp->addKeyword(PvlKeyword("SpacecraftAzimuth"));
      gp->addKeyword(PvlKeyword("SlantDistance"));
      gp->addKeyword(PvlKeyword("TargetCenterDistance"));
      gp->addKeyword(PvlKeyword("SubSpacecraftLatitude"));
      gp->addKeyword(PvlKeyword("SubSpacecraftLongitude"));
      gp->addKeyword(PvlKeyword("SpacecraftAltitude"));
      gp->addKeyword(PvlKeyword("OffNadirAngle"));
      gp->addKeyword(PvlKeyword("SubSpacecraftGroundAzimuth"));
      gp->addKeyword(PvlKeyword("SunPosition"));
      gp->addKeyword(PvlKeyword("SubSolarAzimuth"));
      gp->addKeyword(PvlKeyword("SolarDistance"));
      gp->addKeyword(PvlKeyword("SubSolarLatitude"));
      gp->addKeyword(PvlKeyword("SubSolarLongitude"));
      gp->addKeyword(PvlKeyword("SubSolarGroundAzimuth"));
      gp->addKeyword(PvlKeyword("Phase"));
      gp->addKeyword(PvlKeyword("Incidence"));
      gp->addKeyword(PvlKeyword("Emission"));
      gp->addKeyword(PvlKeyword("EphemerisTime"));
      gp->addKeyword(PvlKeyword("UTC"));
      gp->addKeyword(PvlKeyword("LocalSolarTime"));
      gp->addKeyword(PvlKeyword("SolarLongitude"));
      if (allowErrors) gp->addKeyword(PvlKeyword("Error"));
    }

    bool noErrors = passed;
    std::string error = "";
    if (!camera()->HasSurfaceIntersection()) {
      error = "Requested position does not project in camera model; no surface intersection";
      noErrors = false;
      if (!allowErrors) throw IException(IException::Unknown, error, _FILEINFO_);
    }
    if (!camera()->InCube() && !allowOutside) {
      error = "Requested position does not project in camera model; not inside cube";
      noErrors = false;
      if (!allowErrors) throw IException(IException::Unknown, error, _FILEINFO_);
    }

    if (!noErrors) {
      for (int i = 0; i < gp->keywords(); i++) {
        QString name = QString::fromStdString((*gp)[i].name());
        // These three keywords have 3 values, so they must have 3 NULLs
        if (name == "BodyFixedCoordinate" || name == "SpacecraftPosition" ||
            name == "SunPosition") {
          (*gp)[i].addValue("NULL");
          (*gp)[i].addValue("NULL");
          (*gp)[i].addValue("NULL");
        }
        else {
          (*gp)[i].setValue("NULL");
        }
      }
      // Set all keywords that still have valid information
      gp->findKeyword("Error").setValue(error.toStdString());
      gp->findKeyword("FileName").setValue(cube()->fileName().toStdString());
      gp->findKeyword("Sample").setValue(std::to_string(camera()->Sample()));
      gp->findKeyword("Line").setValue(std::to_string(camera()->Line()));
      gp->findKeyword("EphemerisTime").setValue(std::to_string(camera()->time().Et()), "seconds");
      gp->findKeyword("EphemerisTime").addComment("Time");
      QString utc = camera()->time().UTC();
      gp->findKeyword("UTC").setValue(utc.toStdString());
      gp->findKeyword("SpacecraftPosition").addComment("Spacecraft Information");
      gp->findKeyword("SunPosition").addComment("Sun Information");
      gp->findKeyword("Phase").addComment("Illumination and Other");
    }

    else {

      Brick b(3, 3, 1, cube()->pixelType());

      int intSamp = (int)(camera()->Sample() + 0.5);
      int intLine = (int)(camera()->Line() + 0.5);
      b.SetBasePosition(intSamp, intLine, 1);
      cube()->read(b);

      double pB[3], spB[3], sB[3];
      QString utc;
      double ssplat, ssplon, sslat, sslon, cwaz;
      // double ssplat, ssplon, sslat, sslon, pwlon, oglat;

      {
        gp->findKeyword("FileName").setValue(cube()->fileName().toStdString());
        gp->findKeyword("Sample").setValue(std::to_string(camera()->Sample()));
        gp->findKeyword("Line").setValue(std::to_string(camera()->Line()));
        gp->findKeyword("PixelValue").setValue(PixelToString(b[0]).toStdString());
        gp->findKeyword("RightAscension").setValue(std::to_string(camera()->RightAscension()));
        gp->findKeyword("Declination").setValue(std::to_string(camera()->Declination()));
        // gp->findKeyword("PlanetocentricLatitude").setValue(std::to_string(
        //                 camera()->UniversalLatitude()));

        // Convert lat to planetographic
        // Distance radii[3];
        // camera()->radii(radii);
        // oglat = Isis::TProjection::ToPlanetographic(camera()->UniversalLatitude(),
        //         radii[0].kilometers(), radii[2].kilometers());
        // gp->findKeyword("PlanetographicLatitude").setValue(std::to_string(oglat));

        gp->findKeyword("CounterClockwise360RingLongitude").setValue(std::to_string(
          camera()->UniversalLongitude()));
        // gp->findKeyword("PositiveEast360Longitude").setValue(std::to_string(
        //   camera()->UniversalLongitude()));

        //Convert lon to -180 - 180 range
        gp->findKeyword("CounterClockwise180RingLongitude").setValue(std::to_string(
                        RingPlaneProjection::To180Domain(
                            camera()->UniversalLongitude())));
        // gp->findKeyword("PositiveEast180Longitude").setValue(std::to_string(
        //   Isis::TProjection::To180Domain(
        //     camera()->UniversalLongitude())));

        //Convert ring longitude (az) to clockwise
        cwaz = Isis::RingPlaneProjection::ToClockwise(camera()->UniversalLongitude(),
                360);
        gp->findKeyword("Clockwise360RingLongitude").setValue(std::to_string(cwaz));
        //Convert lon to positive west
        // pwlon = Isis::TProjection::ToPositiveWest(camera()->UniversalLongitude(),
        //         360);
        // gp->findKeyword("PositiveWest360Longitude").setValue(std::to_string(pwlon));

        //Convert cwaz to -180 - 180 range
        gp->findKeyword("Clockwise180RingLongitude").setValue(std::to_string(
          Isis::RingPlaneProjection::To180Domain(cwaz)));
        //Convert pwlon to -180 - 180 range
        // gp->findKeyword("PositiveWest180Longitude").setValue(std::to_string(
        //   Isis::TProjection::To180Domain(pwlon)));

        camera()->Coordinate(pB);
        gp->findKeyword("BodyFixedCoordinate").addValue(std::to_string(pB[0]), "km");
        gp->findKeyword("BodyFixedCoordinate").addValue(std::to_string(pB[1]), "km");
        gp->findKeyword("BodyFixedCoordinate").addValue(std::to_string(pB[2]), "km");

        gp->findKeyword("LocalRingRadius").setValue(std::to_string(
                        camera()->LocalRadius().meters()), "meters");
        gp->findKeyword("SampleResolution").setValue(std::to_string(
                        camera()->SampleResolution()), "meters/pixel");
        gp->findKeyword("LineResolution").setValue(std::to_string(
                        camera()->LineResolution()), "meters/pixel");

        //body fixed
        camera()->instrumentPosition(spB);
        gp->findKeyword("SpacecraftPosition").addValue(std::to_string(spB[0]), "km");
        gp->findKeyword("SpacecraftPosition").addValue(std::to_string(spB[1]), "km");
        gp->findKeyword("SpacecraftPosition").addValue(std::to_string(spB[2]), "km");
        gp->findKeyword("SpacecraftPosition").addComment("Spacecraft Information");

        // if IsValid
        if (Isis::IsValidPixel(camera()->SpacecraftAzimuth())) {
          double spacecraftAzi = camera()->SpacecraftAzimuth();
          gp->findKeyword("SpacecraftAzimuth").setValue(std::to_string(spacecraftAzi));
        }
        else {
          gp->findKeyword("SpacecraftAzimuth").setValue("NULL");
        }
        gp->findKeyword("SlantDistance").setValue(std::to_string(camera()->SlantDistance()), "km");
        gp->findKeyword("TargetCenterDistance").setValue(std::to_string(
                        camera()->targetCenterDistance()), "km");
        camera()->subSpacecraftPoint(ssplat, ssplon);
        gp->findKeyword("SubSpacecraftLatitude").setValue(std::to_string(ssplat));
        gp->findKeyword("SubSpacecraftLongitude").setValue(std::to_string(ssplon));
        gp->findKeyword("SpacecraftAltitude").setValue(std::to_string(
                        camera()->SpacecraftAltitude()), "km");
        gp->findKeyword("OffNadirAngle").setValue(std::to_string(camera()->OffNadirAngle()));

        double subspcgrdaz = camera()->GroundAzimuth(camera()->UniversalLatitude(), 
                                                     camera()->UniversalLongitude(), 
                                                     ssplat, ssplon);
        gp->findKeyword("SubSpacecraftGroundAzimuth").setValue(std::to_string(subspcgrdaz));

        camera()->sunPosition(sB);
        gp->findKeyword("SunPosition").addValue(std::to_string(sB[0]), "km");
        gp->findKeyword("SunPosition").addValue(std::to_string(sB[1]), "km");
        gp->findKeyword("SunPosition").addValue(std::to_string(sB[2]), "km");
        gp->findKeyword("SunPosition").addComment("Sun Information");
        
        if (Isis::IsValidPixel(camera()->SunAzimuth())) {
          double sunAzi = camera()->SunAzimuth();
          gp->findKeyword("SubSolarAzimuth").setValue(std::to_string(sunAzi));
        }
        else {
          gp->findKeyword("SubSolarAzimuth").setValue("NULL");
        }

        gp->findKeyword("SolarDistance").setValue(std::to_string(camera()->SolarDistance()), "AU");
        camera()->subSolarPoint(sslat, sslon);
        gp->findKeyword("SubSolarLatitude").setValue(std::to_string(sslat));
        gp->findKeyword("SubSolarLongitude").setValue(std::to_string(sslon));
        double subsolgrdaz;
        subsolgrdaz = camera()->GroundAzimuth(camera()->UniversalLatitude(), 
                                              camera()->UniversalLongitude(),
                                              sslat, sslon);
        gp->findKeyword("SubSolarGroundAzimuth").setValue(std::to_string(subsolgrdaz));

        gp->findKeyword("Phase").setValue(std::to_string(camera()->PhaseAngle()));
        gp->findKeyword("Phase").addComment("Illumination and Other");
        gp->findKeyword("Incidence").setValue(std::to_string(camera()->IncidenceAngle()));
        gp->findKeyword("Emission").setValue(std::to_string(camera()->EmissionAngle()));

        gp->findKeyword("EphemerisTime").setValue(std::to_string(camera()->time().Et()), "seconds");
        gp->findKeyword("EphemerisTime").addComment("Time");
        utc = camera()->time().UTC();
        gp->findKeyword("UTC").setValue(utc.toStdString());
        gp->findKeyword("LocalSolarTime").setValue(std::to_string(camera()->LocalSolarTime()), "hour");
        gp->findKeyword("SolarLongitude").setValue(std::to_string(camera()->solarLongitude().degrees()));
        if (allowErrors) gp->findKeyword("Error").setValue("NULL");
      }
    }
    return gp;
  }
}
