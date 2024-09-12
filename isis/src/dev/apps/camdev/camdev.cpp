/** This is free and unencumbered software released into the public domain.

The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */

#include <cmath>

#include "Angle.h"
#include "Camera.h"
#include "CameraDistortionMap.h"
#include "CameraFocalPlaneMap.h"
#include "Cube.h"
#include "CubeManager.h"
#include "Distance.h"
#include "FileName.h"
#include "IException.h"
#include "iTime.h"
#include "Longitude.h"
#include "ProjectionFactory.h"
#include "ProcessByBrick.h"
#include "ProcessByLine.h"
#include "SpecialPixel.h"
#include "TProjection.h"

#include "camdev.h"

using namespace std;


namespace Isis {

  // Global variables
  static Camera *cam;
  static TProjection *proj;
  static int nbands;
  static bool noCamera;

  static bool dn;
  static bool ra;
  static bool declination;
  static bool planetocentricLatitude;
  static bool planetographicLatitude;
  static bool positiveEast360Longitude;
  static bool positiveEast180Longitude;
  static bool positiveWest360Longitude;
  static bool positiveWest180Longitude;
  static bool bodyFixedX;
  static bool bodyFixedY;
  static bool bodyFixedZ;
  static bool localRadius;
  static bool pixelResolution;
  static bool lineResolution;
  static bool sampleResolution;
  static bool detectorResolution;
  static bool spacecraftPositionX;
  static bool spacecraftPositionY;
  static bool spacecraftPositionZ;
  static bool spacecraftAzimuth;
  static bool slantDistance;
  static bool targetCenterDistance;
  static bool subSpacecraftLatitude;
  static bool subSpacecraftLongitude;
  static bool subSpacecraftGroundAzimuth;
  static bool spacecraftAltitude;
  static bool offnadirAngle;
  static bool sunPositionX;
  static bool sunPositionY;
  static bool sunPositionZ;
  static bool sunAzimuth;
  static bool solarDistance;
  static bool subSolarLatitude;
  static bool subSolarLongitude;
  static bool subSolarGroundAzimuth;
  static bool phase;
  static bool emission;
  static bool incidence;
  static bool localEmission;
  static bool localIncidence;
  static bool northAzimuth;
  static bool distortedFocalPlaneX;
  static bool distortedFocalPlaneY;
  static bool undistortedFocalPlaneX;
  static bool undistortedFocalPlaneY;
  static bool undistortedFocalPlaneZ;
  static bool ephemerisTime;
  static bool UTC;
  static bool localSolarTime;
  static bool solarLongitude;
  static bool morphologyRank;
  static bool albedoRank;


  static void camdevDN(Buffer &in, Buffer &out);
  static void camdev(Buffer &out);


  // Function to create a keyword with same values of a specified count
  template <typename T> static PvlKeyword makeKey(const QString &name,
                                           const int &nvals,
                                           const T &value);

  // Structure containing new mosaic planes
  struct MosData {
    MosData() :  m_morph(Null), m_albedo(Null) {  }
    double m_morph;
    double m_albedo;
  };

  // Computes the special MORPHOLOGYRANK and ALBEDORANK planes
  MosData *getMosaicIndicies(Camera &camera, MosData &md);
  // Updates BandBin keyword
  static void UpdateBandKey(const QString &keyname, PvlGroup &bb, const int &nvals,
                     const QString &default_value = "Null");


  void camdev(UserInterface &ui) {
    Cube icube(ui.GetCubeName("FROM").toStdString(), "r");
    camdev(&icube, ui);
  }


  void camdev(Cube *icube, UserInterface &ui) {

    // Get the camera information if this is not a mosaic. Otherwise, get the
    // projection information
    Process p1;
    p1.SetInputCube(icube, OneBand);
    if (ui.GetString("SOURCE") == "CAMERA") {
      noCamera = false;
    }
    else {
      noCamera = true;
    }

    if(noCamera) {
      try {
        proj = (TProjection *) icube->projection();
      }
      catch(IException &e) {
        std::string msg = "Mosaic files must contain mapping labels";
        throw IException(e, IException::User, msg, _FILEINFO_);
      }
    }
    else {
      try {
        cam = icube->camera();
      }
      catch(IException &e) {
        std::string msg = "If " + FileName(ui.GetCubeName("FROM").toStdString()).name() + " is a mosaic, make sure the SOURCE "
        "option is set to PROJECTION";
        throw IException(e, IException::User, msg, _FILEINFO_);
      }
    }

    // We will be processing by brick.
    ProcessByBrick p;

    // Find out which bands are to be created
    nbands = 0;
    ra = false;
    declination = false;
    planetographicLatitude = false;
    positiveEast180Longitude = false;
    positiveWest360Longitude = false;
    positiveWest180Longitude = false;
    bodyFixedX = false;
    bodyFixedY = false;
    bodyFixedZ = false;
    localRadius = false;
    lineResolution = false;
    sampleResolution = false;
    detectorResolution = false;
    spacecraftPositionX = false;
    spacecraftPositionY = false;
    spacecraftPositionZ = false;
    spacecraftAzimuth = false;
    slantDistance = false;
    targetCenterDistance = false;
    subSpacecraftLatitude = false;
    subSpacecraftLongitude = false;
    subSpacecraftGroundAzimuth = false;
    spacecraftAltitude = false;
    offnadirAngle = false;
    sunPositionX = false;
    sunPositionY = false;
    sunPositionZ = false;
    sunAzimuth = false;
    solarDistance = false;
    subSolarLatitude = false;
    subSolarLongitude = false;
    subSolarGroundAzimuth = false;
    phase = false;
    emission = false;
    incidence = false;
    localEmission = false;
    localIncidence = false;
    northAzimuth = false;
    distortedFocalPlaneX = false;
    distortedFocalPlaneY = false;
    undistortedFocalPlaneX = false;
    undistortedFocalPlaneY = false;
    undistortedFocalPlaneZ = false;
    ephemerisTime = false;
    UTC = false;
    localSolarTime = false;
    solarLongitude = false;
    morphologyRank = false;
    albedoRank = false;


    if (!noCamera) {
      if ((ra = ui.GetBoolean("RADEC"))) nbands++;
      if ((declination = ui.GetBoolean("RADEC"))) nbands++;
      if ((planetographicLatitude = ui.GetBoolean("PLANETOGRAPHICLATITUDE"))) nbands++;
      if ((positiveEast180Longitude = ui.GetBoolean("POSITIVEEAST180LONGITUDE"))) nbands++;
      if ((positiveWest360Longitude = ui.GetBoolean("POSITIVEWEST360LONGITUDE"))) nbands++;
      if ((positiveWest180Longitude = ui.GetBoolean("POSITIVEWEST180LONGITUDE"))) nbands++;
      if ((bodyFixedX = ui.GetBoolean("BODYFIXED"))) nbands++;
      if ((bodyFixedY = ui.GetBoolean("BODYFIXED"))) nbands++;
      if ((bodyFixedZ = ui.GetBoolean("BODYFIXED"))) nbands++;
      if ((localRadius = ui.GetBoolean("LOCALRADIUS"))) nbands++;
      if ((lineResolution = ui.GetBoolean("LINERESOLUTION"))) nbands++;
      if ((sampleResolution = ui.GetBoolean("SAMPLERESOLUTION"))) nbands++;
      if ((detectorResolution = ui.GetBoolean("DETECTORRESOLUTION"))) nbands++;
      if ((spacecraftPositionX = ui.GetBoolean("SPACECRAFTPOSITION"))) nbands++;
      if ((spacecraftPositionY = ui.GetBoolean("SPACECRAFTPOSITION"))) nbands++;
      if ((spacecraftPositionZ = ui.GetBoolean("SPACECRAFTPOSITION"))) nbands++;
      if ((spacecraftAzimuth = ui.GetBoolean("SPACECRAFTAZIMUTH"))) nbands++;
      if ((slantDistance = ui.GetBoolean("SLANTDISTANCE"))) nbands++;
      if ((targetCenterDistance = ui.GetBoolean("TARGETCENTERDISTANCE"))) nbands++;
      if ((subSpacecraftLatitude = ui.GetBoolean("SUBSPACECRAFTLATITUDE"))) nbands++;
      if ((subSpacecraftLongitude = ui.GetBoolean("SUBSPACECRAFTLONGITUDE"))) nbands++;
      if ((subSpacecraftGroundAzimuth = ui.GetBoolean("SUBSPACECRAFTGROUNDAZIMUTH"))) nbands++;
      if ((spacecraftAltitude = ui.GetBoolean("SPACECRAFTALTITUDE"))) nbands++;
      if ((offnadirAngle = ui.GetBoolean("OFFNADIRANGLE"))) nbands++;
      if ((sunPositionX = ui.GetBoolean("SUNPOSITION"))) nbands++;
      if ((sunPositionY = ui.GetBoolean("SUNPOSITION"))) nbands++;
      if ((sunPositionZ = ui.GetBoolean("SUNPOSITION"))) nbands++;
      if ((sunAzimuth = ui.GetBoolean("SUNAZIMUTH"))) nbands++;
      if ((solarDistance = ui.GetBoolean("SOLARDISTANCE"))) nbands++;
      if ((subSolarLatitude = ui.GetBoolean("SUBSOLARLATITUDE"))) nbands++;
      if ((subSolarLongitude = ui.GetBoolean("SUBSOLARLONGITUDE"))) nbands++;
      if ((subSolarGroundAzimuth = ui.GetBoolean("SUBSOLARGROUNDAZIMUTH"))) nbands++;
      if ((phase = ui.GetBoolean("PHASE"))) nbands++;
      if ((incidence = ui.GetBoolean("INCIDENCE"))) nbands++;
      if ((emission = ui.GetBoolean("EMISSION"))) nbands++;
      if ((localEmission = ui.GetBoolean("LOCALEMISSION"))) nbands++;
      if ((localIncidence = ui.GetBoolean("LOCALINCIDENCE"))) nbands++;
      if ((northAzimuth = ui.GetBoolean("NORTHAZIMUTH"))) nbands++;
      if ((distortedFocalPlaneX = ui.GetBoolean("DISTORTEDFOCALPLANE"))) nbands++;
      if ((distortedFocalPlaneY = ui.GetBoolean("DISTORTEDFOCALPLANE"))) nbands++;
      if ((undistortedFocalPlaneX = ui.GetBoolean("UNDISTORTEDFOCALPLANE"))) nbands++;
      if ((undistortedFocalPlaneY = ui.GetBoolean("UNDISTORTEDFOCALPLANE"))) nbands++;
      if ((undistortedFocalPlaneZ = ui.GetBoolean("UNDISTORTEDFOCALPLANE"))) nbands++;
      if ((ephemerisTime = ui.GetBoolean("EPHEMERISTIME"))) nbands++;
      if ((UTC = ui.GetBoolean("UTC"))) nbands++;
      if ((localSolarTime = ui.GetBoolean("LOCALSOLARTIME"))) nbands++;
      if ((solarLongitude = ui.GetBoolean("SOLARLONGITUDE"))) nbands++;
      if ((morphologyRank = ui.GetBoolean("MORPHOLOGYRANK"))) nbands++;
      if ((albedoRank = ui.GetBoolean("ALBEDORANK"))) nbands++;

    }
    if((dn = ui.GetBoolean("DN"))) nbands++;
    if((planetocentricLatitude = ui.GetBoolean("PLANETOCENTRICLATITUDE"))) nbands++;
    if((positiveEast360Longitude = ui.GetBoolean("POSITIVEEAST360LONGITUDE"))) nbands++;
    if((pixelResolution = ui.GetBoolean("PIXELRESOLUTION"))) nbands++;

    if(nbands < 1) {
      std::string message = "At least one parameter must be entered"
                       "[PHASE, EMISSION, INCIDENCE, LATITUDE, LONGITUDE...]";
      throw IException(IException::User, message, _FILEINFO_);
    }

    // If outputting a a dn band, retrieve the orignal values for the filter name from the input cube,
    // if it exists.  Otherwise, the default will be "DN"
    QString bname = "DN";
    if ( dn && icube->hasGroup("BandBin") ) {
      PvlGroup &mybb = icube->group("BandBin");
      if ( mybb.hasKeyword("Name") ) {
        bname = QString::fromStdString(mybb["Name"][0]);
      }
      else if ( mybb.hasKeyword("FilterName") ) {
        bname = QString::fromStdString(mybb["FilterName"][0]);
      }
    }

    // Create a bandbin group for the output label
    PvlKeyword name("Name");
    if (dn) name += bname.toStdString();
    if (ra) name += "Right Ascension";
    if (declination) name += "Declination";
    if (planetocentricLatitude) name += "Planetocentric Latitude";
    if (planetographicLatitude) name += "Planetographic Latitude";
    if (positiveEast360Longitude) name += "Positive East 360 Longitude";
    if (positiveEast180Longitude) name += "Positive East 180 Longitude";
    if (positiveWest360Longitude) name += "Positive West 360 Longitude";
    if (positiveWest180Longitude) name += "Positive West 180 Longitude";
    if (bodyFixedX) name += "Body Fixed X";
    if (bodyFixedY) name += "Body Fixed Y";
    if (bodyFixedZ) name += "Body Fixed Z";
    if (localRadius) name += "Local Radius";
    if (pixelResolution) name += "Pixel Resolution";
    if (lineResolution) name += "Line Resolution";
    if (sampleResolution) name += "Sample Resolution";
    if (detectorResolution) name += "Detector Resolution";
    if (spacecraftPositionX) name += "Spacecraft Position X";
    if (spacecraftPositionY) name += "Spacecraft Position Y";
    if (spacecraftPositionZ) name += "Spacecraft Position Z";
    if (spacecraftAzimuth) name += "Spacecraft Azimuth";
    if (slantDistance) name += "Slant Distance";
    if (targetCenterDistance) name += "Target Center Distance";
    if (subSpacecraftLatitude) name += "Sub Spacecraft Latitude";
    if (subSpacecraftLongitude) name += "Sub Spacecraft Longitude";
    if (subSpacecraftGroundAzimuth) name += "Sub Spacecraft Ground Azimuth";
    if (spacecraftAltitude) name += "Spacecraft Altitude";
    if (offnadirAngle) name += "OffNadir Angle";
    if (sunPositionX) name += "Sun Position X";
    if (sunPositionY) name += "Sun Position Y";
    if (sunPositionZ) name += "Sun Position Z";
    if (sunAzimuth) name += "Sun Azimuth";
    if (solarDistance) name += "Solar Distance";
    if (subSolarLatitude) name += "Sub Solar Latitude";
    if (subSolarLongitude) name += "Sub Solar Longitude";
    if (subSolarGroundAzimuth) name += "Sub Solar Ground Azimuth";
    if (phase) name += "Phase Angle";
    if (incidence) name += "Incidence Angle";
    if (emission) name += "Emission Angle";
    if (localEmission) name += "Local Emission Angle";
    if (localIncidence) name += "Local Incidence Angle";
    if (northAzimuth) name += "North Azimuth";
    if (distortedFocalPlaneX) name += "Distorted Focal Plane X";
    if (distortedFocalPlaneY) name += "Distorted Focal Plane Y";
    if (undistortedFocalPlaneX) name += "Undistorted Focal Plane X";
    if (undistortedFocalPlaneY) name += "Undistorted Focal Plane Y";
    if (undistortedFocalPlaneZ) name += "Undistorted Focal Plane Z";
    if (ephemerisTime) name += "Ephemeris Time";
    if (UTC) name += "Coordinated Universal Time";
    if (localSolarTime) name += "Local Solar Time";
    if (solarLongitude) name += "Solar Longitude";
    if (morphologyRank) name += "morphologyRank";
    if (albedoRank) name += "albedoRank";




    // Create the output cube.  Note we add the input cube to expedite propagation
    // of input cube elements (label, blobs, etc...).  It will be cleared
    // prior to systematic processing only if the DN option is not selected.
    // If DN is chosen by the user, then we propagate the input buffer with a
    // different function - one that accepts both input and output buffers.
    p.SetInputCube(icube, OneBand);
    Cube *ocube = p.SetOutputCube(ui.GetCubeName("TO"), ui.GetOutputAttribute("TO"), icube->sampleCount(),
                                  icube->lineCount(), nbands);
    p.SetBrickSize(64, 64, nbands);
    if (dn) {
      // Process with input and output buffers
      p.StartProcess(camdevDN);
    }
    else {
      // Toss the input file as stated above
      p.ClearInputCubes();
      // Start the processing
      p.StartProcess(camdev);
    }
    // Add the bandbin group to the output label.  If a BandBin group already
    // exists, remove all existing keywords and add the keywords for this app.
    // Otherwise, just put the group in.
    PvlObject &cobj = ocube->label()->findObject("IsisCube");
    if(!cobj.hasGroup("BandBin")) {
      cobj.addGroup(PvlGroup("BandBin"));
    }

    PvlGroup &bb = cobj.findGroup("BandBin");
    bb.addKeyword(name, PvlContainer::Replace);
    int nvals = name.size();
    UpdateBandKey("Center", bb, nvals, "1.0");

    if ( bb.hasKeyword("OriginalBand") ) {
      UpdateBandKey("OriginalBand", bb, nvals, "1.0");
    }

    if ( bb.hasKeyword("Number") ) {
      UpdateBandKey("Number", bb, nvals, "1.0");
    }

    UpdateBandKey("Width", bb, nvals, "1.0");
    p.EndProcess();
  }


  //  This propagates the input plane to the output plane, then passes it off to
  //  the general routine
  void camdevDN(Buffer &in, Buffer &out) {
    for (int i = 0 ; i < in.size() ; i++) {
      out[i] = in[i];
    }
    camdev(out);
  }


  //  Computes all the geometric properties for the output buffer.  Certain
  //  knowledge of the buffers size is assumed below, so ensure the buffer
  //  is still of the expected size.
  void camdev(Buffer &out) {
    // If the DN option is selected, it is already added by the camdevDN
    // function.  We must compute the offset to start at the second band.
    int skipDN = (dn) ? 64 * 64   :  0;
    for(int i = 0; i < 64; i++) {
      for(int j = 0; j < 64; j++) {

        MosData mosd, *p_mosd(0);  // For special mosaic angles

        int index = i * 64 + j + skipDN;
        double samp = out.Sample(index);
        double line = out.Line(index);

        bool isGood=false;
        if (noCamera) {
          isGood = proj->SetWorld(samp, line);
        }
        else {
          isGood = cam->SetImage(samp, line);
        }

        if (isGood) {
          if (ra) {
            out[index] = cam->RightAscension();
            index += 64 * 64;
          }
          if (declination) {
            out[index] = cam->Declination();
            index += 64 * 64;
          }
          if(planetocentricLatitude) {
            if(noCamera) {
              out[index] = proj->UniversalLatitude();
            }
            else {
              out[index] = cam->UniversalLatitude();
            }
            index += 64 * 64;
          }
          if (!noCamera) {
            if (planetographicLatitude) {
              Distance radii[3];
              cam->radii(radii);
              double ocentricLat;
              ocentricLat = cam->UniversalLatitude();
              out[index] = TProjection::ToPlanetographic(ocentricLat, radii[0].kilometers(),
                                                   radii[2].kilometers());
              index += 64 * 64;
            }
          }
          double pe360Lon, pw360Lon;
          if (positiveEast360Longitude) {
            if(noCamera) {
              pe360Lon = proj->UniversalLongitude();
            }
            else {
              pe360Lon = cam->UniversalLongitude();
            }
            out[index] = pe360Lon;
            index += 64 * 64;
          }
          if (!noCamera) {
            pe360Lon = cam->UniversalLongitude();
            pw360Lon = TProjection::ToPositiveWest(pe360Lon, 360);
            if (positiveEast180Longitude) {
              out[index] = TProjection::To180Domain(pe360Lon);
              index += 64 * 64;
            }
            if (positiveWest360Longitude) {
              out[index] = pw360Lon;
              index += 64 * 64;
            }
            if (positiveWest180Longitude) {
              out[index] = TProjection::To180Domain(pw360Lon);
              index += 64 * 64;
            }
          }
          //If bodyFixedX is true, Y and Z are true as well so compute them all
          if (bodyFixedX) {
            double pB[3];
            cam->Coordinate(pB);
            out[index] = pB[0];
            index += 64 * 64;
            out[index] = pB[1];
            index += 64 * 64;
            out[index] = pB[2];
            index += 64 * 64;
          }
          if (localRadius) {
            out[index] = cam->LocalRadius().meters();
            index += 64 * 64;
          }
          if (pixelResolution) {
            if (noCamera) {
              out[index] = proj->Resolution();
            }
            else {
              out[index] = cam->PixelResolution();
            }
            index += 64 * 64;
          }
          if(lineResolution) {
            out[index] = cam->LineResolution();
            index += 64 * 64;
          }
          if(sampleResolution) {
            out[index] = cam->SampleResolution();
            index += 64 * 64;
          }
          if(detectorResolution) {
            out[index] = cam->DetectorResolution();
            index += 64 * 64;
          }
          //If spacecraftPositionX is true, Y and Z are true as well so compute them all
          if(spacecraftPositionX) {
            double spB[3];
            cam->instrumentPosition(spB);
            out[index] = spB[0];
            index += 64 * 64;
            out[index] = spB[1];
            index += 64 * 64;
            out[index] = spB[2];
            index += 64 * 64;
          }
          if(spacecraftAzimuth) {
            out[index] = cam->SpacecraftAzimuth();
            index += 64 * 64;
          }
          if(slantDistance) {
            out[index] = cam->SlantDistance();
            index += 64 * 64;
          }
          if(targetCenterDistance) {
            out[index] = cam->targetCenterDistance();
            index += 64 * 64;
          }
          if(!noCamera) {
            double ssplat, ssplon;
            ssplat = 0.0;
            ssplon = 0.0;
            cam->subSpacecraftPoint(ssplat, ssplon);
            if(subSpacecraftLatitude) {
              out[index] = ssplat;
              index += 64 * 64;
            }
            if(subSpacecraftLongitude) {
              out[index] = ssplon;
              index += 64 * 64;
            }
            if(subSpacecraftGroundAzimuth) {
              out[index] = cam->GroundAzimuth(cam->UniversalLatitude(),
                  cam->UniversalLongitude(), ssplat, ssplon);
              index += 64 * 64;
            }
          }
          if(spacecraftAltitude) {
            out[index] = cam->SpacecraftAltitude();
            index += 64 * 64;
          }
          if(offnadirAngle) {
            out[index] = cam->OffNadirAngle();
            index += 64 * 64;
          }
          //If sunPositionX is true, Y and Z are true as well so compute them all
          if(sunPositionX) {
            double sB[3];
            cam->sunPosition(sB);
            out[index] = sB[0];
            index += 64 * 64;
            out[index] = sB[1];
            index += 64 * 64;
            out[index] = sB[2];
            index += 64 * 64;
          }
          if(sunAzimuth) {
            out[index] = cam->SunAzimuth();
            index += 64 * 64;
          }
          if(solarDistance) {
            out[index] = cam->SolarDistance();
            index += 64 * 64;
          }
          if(!noCamera) {
            double sslat, sslon;
            sslat = 0.0;
            sslon = 0.0;
            cam->subSolarPoint(sslat, sslon);
            if(subSolarLatitude) {
              out[index] = sslat;
              index += 64 * 64;
            }
            if(subSolarLongitude) {
              out[index] = sslon;
              index += 64 * 64;
            }
            if(subSolarGroundAzimuth) {
              out[index] = cam->GroundAzimuth(cam->UniversalLatitude(),
                  cam->UniversalLongitude(), sslat, sslon);
              index += 64 * 64;
            }
          }
          if(phase) {
            out[index] = cam->PhaseAngle();
            index += 64 * 64;
          }
          if(incidence) {
            out[index] = cam->IncidenceAngle();
            index += 64 * 64;
          }
          if(emission) {
            out[index] = cam->EmissionAngle();
            index += 64 * 64;
          }
          if(localEmission || localIncidence) {
            Angle phase;
            Angle incidence;
            Angle emission;
            bool success;
            cam->LocalPhotometricAngles(phase, incidence, emission, success);

            if (localEmission) {
              out[index] = emission.degrees();
              index += 64 * 64;
            }

            if (localIncidence) {
              out[index] = incidence.degrees();
              index += 64 * 64;
            }
          }
          if(northAzimuth) {
            out[index] = cam->NorthAzimuth();
            index += 64 * 64;
          }
          //If distortedFocalPlaneX is true, Y is true as well so compute both
          if(distortedFocalPlaneX) {
            CameraFocalPlaneMap *focalPlaneMap = cam->FocalPlaneMap();
            out[index] = focalPlaneMap->FocalPlaneX();
            index += 64 * 64;
            out[index] = focalPlaneMap->FocalPlaneY();
            index += 64 * 64;
          }
          //If undistortedFocalPlaneX is true, Y and Z are true as well so compute them all
          if(undistortedFocalPlaneX) {
            CameraDistortionMap *distortedMap = cam->DistortionMap();
            out[index] = distortedMap->UndistortedFocalPlaneX();
            index += 64 * 64;
            out[index] = distortedMap->UndistortedFocalPlaneY();
            index += 64 * 64;
            out[index] = distortedMap->UndistortedFocalPlaneZ();
            index += 64 * 64;
          }
          if(ephemerisTime) {
            out[index] = cam->time().Et();
            index += 64 * 64;
          }
          if(UTC) {
            QString utcDouble = cam->time().UTC();
            out[index] = utcDouble.toDouble();
            index += 64 * 64;
          }
          if(localSolarTime) {
            out[index] = cam->LocalSolarTime();
            index += 64 * 64;
          }
          if(solarLongitude) {
            out[index] = cam->solarLongitude().degrees();
            index += 64 * 64;
          }
          // Special Mosaic indexes
          if (morphologyRank) {
            if (!p_mosd) { p_mosd = getMosaicIndicies(*cam, mosd); }
            out[index] = mosd.m_morph;
            index += 64 * 64;
          }

          if (albedoRank) {
            if (!p_mosd) { p_mosd = getMosaicIndicies(*cam, mosd); }
            out[index] = mosd.m_albedo;
            index += 64 * 64;
          }
        }
        // Trim outerspace
        else {
          for(int b = (skipDN) ? 1 : 0; b < nbands; b++) {
            out[index] = Isis::NULL8;
            index += 64 * 64;
          }
        }
      }
    }
  }


  // Function to create a keyword with same values of a specified count
  template <typename T>
    PvlKeyword makeKey(const QString &name, const int &nvals,
                       const T &value) {
      PvlKeyword key(name.toStdString());
      for (int i = 0 ; i < nvals ; i++) {
        key += value.toStdString();
      }
      return (key);
    }


  // Computes the special morphologyRank and albedoRank planes
  MosData *getMosaicIndicies(Camera &camera, MosData &md) {
    const double Epsilon(1.0E-8);
    Angle myphase;
    Angle myincidence;
    Angle myemission;
    bool mysuccess;
    camera.LocalPhotometricAngles(myphase, myincidence, myemission, mysuccess);
    if (!mysuccess) {
      myemission.setDegrees(camera.EmissionAngle());
      myincidence.setDegrees(camera.IncidenceAngle());
    }
    double res = camera.PixelResolution();
    if (fabs(res) < Epsilon) res = Epsilon;

    md = MosData();  // Nullifies the data
    if (myemission.isValid()) {
      // Compute morphologyRank
      double cose = cos(myemission.radians());
      if (fabs(cose) < Epsilon) cose = Epsilon;
      // Convert resolution to units of KM
      md.m_morph = (res / 1000.0) / cose;

      if (myincidence.isValid()) {
        // Compute albedoRank
        double cosi = cos(myincidence.radians());
        if (fabs(cosi) < Epsilon) cosi = Epsilon;
        //  Convert resolution to KM
        md.m_albedo = (res / 1000.0 ) * ( (1.0 / cose) + (1.0 / cosi) );
      }
    }

    return (&md);
  }


  //  Updates existing BandBin keywords with additional values to ensure
  //  label compilancy (which should support Camera models).  It checks for the
  //  existance of the keyword and uses its (assumed) first value to set nvals
  //  values to a constant.  If the keyword doesn't exist, it uses the default
  //  value.
  void UpdateBandKey(const QString &keyname, PvlGroup &bb, const int &nvals,
                     const QString &default_value) {

    QString defVal(default_value);
    if ( bb.hasKeyword(keyname.toStdString()) ) {
      defVal = QString::fromStdString(bb[keyname.toStdString()][0]);
    }

    bb.addKeyword(makeKey(keyname, nvals, defVal), PvlContainer::Replace);
    return;
  }

}
