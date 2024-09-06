/** This is free and unencumbered software released into the public domain.

The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */

#include "phocube.h"

#include "Angle.h"
#include "Camera.h"
#include "Cube.h"
#include "FileName.h"
#include "IException.h"
#include "LinearAlgebra.h"
#include "ProjectionFactory.h"
#include "ProcessByBrick.h"
#include "ProcessByLine.h"
#include "ShapeModel.h"
#include "SpecialPixel.h"
#include "Target.h"
#include "TProjection.h"

#include <cmath>

using namespace std;

namespace Isis {

// Function to create a keyword with same values of a specified count
  template <typename T> PvlKeyword makeKey(const QString &name,
                                           const int &nvals,
                                           const T &value);

  // Structure containing new mosaic planes
  struct MosData {
    MosData() :  m_morph(Null), m_albedo(Null) {  }
    double m_morph;
    double m_albedo;
  };


  // Computes the special MORPHOLOGYRANK and ALBEDORANK planes
  static MosData *getMosaicIndicies(Camera &camera, MosData &md);
  // Updates BandBin keyword
  static void UpdateBandKey(const QString &keyname, PvlGroup &bb, const int &nvals,
                     const QString &default_value = "Null");


  void phocube(UserInterface &ui) {
    Cube icube;
    icube.open(ui.GetCubeName("FROM"));
    phocube(&icube, ui);
  }


  void phocube(Cube *icube, UserInterface &ui)  {

    // Get the camera information if this is not a mosaic. Otherwise, get the
    // projection information
    bool noCamera;
    if (ui.GetString("SOURCE") == "CAMERA") {
      noCamera = false;
    }
    else {
      noCamera = true;
    }

    Camera *cam;
    TProjection *proj;
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
    int nbands = 0;
    bool phase = false;
    bool emission = false;
    bool incidence = false;
    bool ellipsoidNormal = false;
    bool localNormal = false;
    bool slope = false;
    bool localEmission = false;
    bool localIncidence = false;
    bool lineResolution = false;
    bool sampleResolution = false;
    bool detectorResolution = false;
    bool obliqueDetectorResolution = false;
    bool sunAzimuth = false;
    bool spacecraftAzimuth = false;
    bool offnadirAngle = false;
    bool subSpacecraftGroundAzimuth = false;
    bool subSolarGroundAzimuth = false;
    bool morphologyRank = false;
    bool albedoRank = false;
    bool northAzimuth = false;
    bool ra = false;
    bool declination = false;
    bool bodyFixedX = false;
    bool bodyFixedY = false;
    bool bodyFixedZ = false;
    bool localSolarTime = false;
    bool sunilluminationmask = false;
    bool surfaceobliquedetectorresolution = false;
    int raBandNum = 0;  // 0 based, if RA is 5th band, raBandNum will be 4

    if (!noCamera) {
      if ((phase = ui.GetBoolean("PHASE"))) nbands++;
      if ((emission = ui.GetBoolean("EMISSION"))) nbands++;
      if ((incidence = ui.GetBoolean("INCIDENCE"))) nbands++;
      if ((localEmission = ui.GetBoolean("LOCALEMISSION"))) nbands++;
      if ((localIncidence = ui.GetBoolean("LOCALINCIDENCE"))) nbands++;
      if ((lineResolution = ui.GetBoolean("LINERESOLUTION"))) nbands++;
      if ((sampleResolution = ui.GetBoolean("SAMPLERESOLUTION"))) nbands++;
      if ((detectorResolution = ui.GetBoolean("DETECTORRESOLUTION"))) nbands++;
      if ((obliqueDetectorResolution = ui.GetBoolean("OBLIQUEDETECTORRESOLUTION"))) nbands++;
      if ((sunAzimuth = ui.GetBoolean("SUNAZIMUTH"))) nbands++;
      if ((spacecraftAzimuth = ui.GetBoolean("SPACECRAFTAZIMUTH"))) nbands++;
      if ((offnadirAngle = ui.GetBoolean("OFFNADIRANGLE"))) nbands++;
      if ((slope = ui.GetBoolean("SLOPE"))) nbands++;
      if ((localNormal = ui.GetBoolean("LOCALNORMAL"))) nbands = nbands + 3;
      if ((ellipsoidNormal = ui.GetBoolean("ELLIPSOIDNORMAL"))) nbands = nbands + 3;
      if ((subSpacecraftGroundAzimuth = ui.GetBoolean("SUBSPACECRAFTGROUNDAZIMUTH"))) nbands++;
      if ((subSolarGroundAzimuth = ui.GetBoolean("SUBSOLARGROUNDAZIMUTH"))) nbands++;
      if ((morphologyRank = ui.GetBoolean("MORPHOLOGYRANK"))) nbands++;
      if ((albedoRank = ui.GetBoolean("ALBEDORANK"))) nbands++;
      if ((northAzimuth = ui.GetBoolean("NORTHAZIMUTH"))) nbands++;
      if ((ra = ui.GetBoolean("RADEC"))) nbands++;
      if ((declination = ui.GetBoolean("RADEC"))) nbands++;
      if ((bodyFixedX = ui.GetBoolean("BODYFIXED"))) nbands++;
      if ((bodyFixedY = ui.GetBoolean("BODYFIXED"))) nbands++;
      if ((bodyFixedZ = ui.GetBoolean("BODYFIXED"))) nbands++;
      if ((localSolarTime = ui.GetBoolean("LOCALTIME"))) nbands++;
      if ((sunilluminationmask = ui.GetBoolean("SUNILLUMINATIONMASK"))) nbands++;
      if ((surfaceobliquedetectorresolution = ui.GetBoolean("SURFACEOBLIQUEDETECTORRESOLUTION"))) nbands++;
    }

    // ALLDN includes DN so if both are set ignore DN
    bool dn;
    bool alldn;
    if ((dn = ui.GetBoolean("DN"))) nbands++;
    if ((alldn = ui.GetBoolean("ALLDN"))) {
      if (dn) {
        dn = false;
        nbands--;
      }
      nbands += icube->bandCount();
    }

    bool latitude;
    if ((latitude = ui.GetBoolean("LATITUDE"))) nbands++;

    bool longitude;
    if ((longitude = ui.GetBoolean("LONGITUDE"))) nbands++;

    bool pixelResolution;
    if ((pixelResolution = ui.GetBoolean("PIXELRESOLUTION"))) nbands++;

    if (nbands < 1) {
      std::string message = "At least one photometry parameter must be entered"
                       "[PHASE, EMISSION, INCIDENCE, LATITUDE, LONGITUDE...]";
      throw IException(IException::User, message, _FILEINFO_);
    }

    // If outputting a dn band, retrieve the original values for the filter name(s) from the input cube,
    // if they are in the band bin group.  Otherwise, the default will be "DN"
    QString bname = "DN";
    PvlKeyword bnames;
    if (dn && icube->hasGroup("BandBin")) {
      PvlGroup &mybb = icube->group("BandBin");
      if ( mybb.hasKeyword("Name") ) {
        bname = QString::fromStdString(mybb["Name"][0]);
      }
      else if (mybb.hasKeyword("FilterName")) {
        bname = QString::fromStdString(mybb["FilterName"][0]);
      }
    }
    else if (alldn && icube->hasGroup("BandBin")) {
      PvlGroup &mybb = icube->group("BandBin");
      if (mybb.hasKeyword("Name")) {
        bnames = mybb.findKeyword("Name");
      }
      else if (mybb.hasKeyword("FilterName")) {
        bnames = mybb.findKeyword("FilterName");
      }
    }

    // Create a bandbin group for the output label
    PvlKeyword name("Name");
    if (dn) {
      name += bname.toStdString();
      raBandNum++;
    }
    else if (alldn) {
      for (int i = 0; i<bnames.size(); i++) {
        name += bnames[i];
        raBandNum++;
      }
    }
    if (phase) {
      name += "Phase Angle";
      raBandNum++;
    }
    if (emission) {
      name += "Emission Angle";
      raBandNum++;
    }
    if (incidence) {
      name += "Incidence Angle";
      raBandNum++;
    }
    if (ellipsoidNormal) {
      name += "Ellipsoid Normal X";
      raBandNum++;

      name += "Ellipsoid Normal Y";
      raBandNum++;

      name += "Ellipsoid Normal Z";
      raBandNum++;
    }
    if (localNormal) {
      name += "Local Normal X";
      raBandNum++;

      name += "Local Normal Y";
      raBandNum++;

      name += "Local Normal Z";
      raBandNum++;
    }
    if (slope) {
      name += "Slope";
      raBandNum++;
    }
    if (localEmission) {
      name += "Local Emission Angle";
      raBandNum++;
    }
    if (localIncidence) {
      name += "Local Incidence Angle";
      raBandNum++;
    }
    if (latitude) {
      name += "Latitude";
      raBandNum++;
    }
    if (longitude) {
      name += "Longitude";
      raBandNum++;
    }
    if (pixelResolution) {
      name += "Pixel Resolution";
      raBandNum++;
    }
    if (lineResolution) {
      name += "Line Resolution";
      raBandNum++;
    }
    if (sampleResolution) {
      name += "Sample Resolution";
      raBandNum++;
    }
    if (detectorResolution) {
      name += "Detector Resolution";
      raBandNum++;
    }
    if (obliqueDetectorResolution) {
      name += "Oblique Detector Resolution";
      raBandNum++;
    }
    if (northAzimuth) {
      name += "North Azimuth";
      raBandNum++;
    }
    if (sunAzimuth) {
      name += "Sun Azimuth";
      raBandNum++;
    }
    if (spacecraftAzimuth) {
      name += "Spacecraft Azimuth";
      raBandNum++;
    }
    if (offnadirAngle) {
      name += "OffNadir Angle";
      raBandNum++;
    }
    if (subSpacecraftGroundAzimuth) {
      name += "Sub Spacecraft Ground Azimuth";
      raBandNum++;
    }
    if (subSolarGroundAzimuth) {
      name += "Sub Solar Ground Azimuth";
      raBandNum++;
    }
    if (morphologyRank) {
      name += "Morphology Rank";
      raBandNum++;
    }
    if (albedoRank) {
      name += "Albedo Rank";
      raBandNum++;
    }
    if (ra) {
      name += "Right Ascension";
    }
    if (declination) {
      name += "Declination";
    }
    if (bodyFixedX) {
      name += "Body Fixed X";
    }
    if (bodyFixedY) {
      name += "Body Fixed Y";
    }
    if (bodyFixedZ) {
      name += "Body Fixed Z";
    }
    if (localSolarTime) {
      name += "Local Solar Time";
    }
    if (sunilluminationmask) {
      name += "Sun Illumination Mask";
    }
    if (surfaceobliquedetectorresolution) {
      name += "Surface Oblique Detector Resolution";
    }
    bool specialPixels = ui.GetBoolean("SPECIALPIXELS");

    /**
     * Computes all the geometric properties for the output buffer. Certain
     * knowledge of the buffers size is assumed below, so ensure the buffer
     * is still of the expected size.
     *
     * @param in  The input cube buffer.
     * @param out The output cube buffer.
     */
    auto phocube = [&](Buffer &in, Buffer &out)->void {
      for (int i = 0; i < 64; i++) {
        for (int j = 0; j < 64; j++) {

          MosData mosd, *p_mosd(0);  // For special mosaic angles

          int index = i * 64 + j;
          int inIndex = index; // Points to the first band's DN value of the current spectra

          // Always transfer the DN(s) to the output cube
          if (dn) {
            out[index] = in[index];
            index += 64 * 64;
          }
          else if (alldn) {
            for (int i = 0; i < icube->bandCount(); i++) {
              out[index] = in[index];
              index += 64 * 64;
            }
          }

          // May need to skip the pho  calculations for the rest of this spectra. If so,
          // fill the pho bands with ISIS Null, but leave the DN band(s) alone.
          // NOTE: index -vs- inIndex below
          if (!specialPixels && IsSpecial(in[inIndex])) {
            int startBand = 0;
            if (dn) {
              startBand = 1;
            }
            else if (alldn) {
              startBand = icube->bandCount();
            }
            for (int band = startBand; band < nbands; band++) {
              out[index] = Isis::Null;
              index += 64 * 64;
            }

            continue;
          }

          // Checks to see if the point is off the body
          double samp = out.Sample(index);
          double line = out.Line(index);
          bool isGood = false;
          if (noCamera) {
            isGood = proj->SetWorld(samp, line);
          }
          else {
            isGood = cam->SetImage(samp, line);
          }

          if (isGood) {

            if (phase) {
              out[index] = cam->PhaseAngle();
              index += 64 * 64;
            }
            if (emission) {
              out[index] = cam->EmissionAngle();
              index += 64 * 64;
            }
            if (incidence) {
              out[index] = cam->IncidenceAngle();
              index += 64 * 64;
            }
            if (ellipsoidNormal) {
              ShapeModel *shapeModel = cam->target()->shape();
              std::vector<double> ellipsoidNormal = shapeModel->normal();

              LinearAlgebra::Vector ellipsoidNormalXYZ = LinearAlgebra::vector(ellipsoidNormal[0], ellipsoidNormal[1], ellipsoidNormal[2]);
              ellipsoidNormalXYZ = LinearAlgebra::normalize(ellipsoidNormalXYZ);

              // Generate X, Y, and Z back plan
              out[index] = ellipsoidNormalXYZ[0];
              index += 64 * 64;

              out[index] = ellipsoidNormalXYZ[1];
              index += 64 * 64;

              out[index] = ellipsoidNormalXYZ[2];
              index += 64 * 64;
            }
            if (localEmission || localIncidence) {
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
            // This if block sets the normal within the camera/shapemodel to the
            // local normal, any code that needs the ellipsoid normal should be
            // placed before this if block
            if (localNormal) {
              double localNormal[3];
              cam->GetLocalNormal(localNormal);
              LinearAlgebra::Vector localNormalXYZ = LinearAlgebra::vector(localNormal[0], localNormal[1], localNormal[2]);
              localNormalXYZ = LinearAlgebra::normalize(localNormalXYZ);

              // Generate X, Y, and Z back plan
              out[index] = localNormalXYZ[0];
              index += 64 * 64;

              out[index] = localNormalXYZ[1];
              index += 64 * 64;

              out[index] = localNormalXYZ[2];
              index += 64 * 64;
            }
            if (slope) {
              double slope;
              bool success;
              cam->Slope(slope, success);
              if (success) {
                out[index] = slope;
              }
              else {
                out[index] = Isis::Null;
              }
              index += 64 * 64;
            }
            if (latitude) {
              if (noCamera) {
                out[index] = proj->UniversalLatitude();
              }
              else {
                out[index] = cam->UniversalLatitude();
              }
              index += 64 * 64;
            }
            if (longitude) {
              if (noCamera) {
                out[index] = proj->UniversalLongitude();
              }
              else {
                out[index] = cam->UniversalLongitude();
              }
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
            if (lineResolution) {
              out[index] = cam->LineResolution();
              index += 64 * 64;
            }
            if (sampleResolution) {
              out[index] = cam->SampleResolution();
              index += 64 * 64;
            }
            if (detectorResolution) {
              out[index] = cam->DetectorResolution();
              index += 64 * 64;
            }
            if (obliqueDetectorResolution) {
              out[index] = cam->ObliqueDetectorResolution();
              index += 64 * 64;
            }
            if (northAzimuth) {
              out[index] = cam->NorthAzimuth();
              index += 64 * 64;
            }
            if (sunAzimuth) {
              out[index] = cam->SunAzimuth();
              index += 64 * 64;
            }
            if (spacecraftAzimuth) {
              out[index] = cam->SpacecraftAzimuth();
              index += 64 * 64;
            }
            if (offnadirAngle) {
              out[index] = cam->OffNadirAngle();
              index += 64 * 64;
            }
            if (subSpacecraftGroundAzimuth) {
              double ssplat, ssplon;
              ssplat = ssplon = 0.0;
              cam->subSpacecraftPoint(ssplat, ssplon);
              out[index] = cam->GroundAzimuth(cam->UniversalLatitude(),
                  cam->UniversalLongitude(), ssplat, ssplon);
              index += 64 * 64;
            }
            if (subSolarGroundAzimuth) {
              double sslat, sslon;
              sslat = sslon = 0.0;
              cam->subSolarPoint(sslat,sslon);
              out[index] = cam->GroundAzimuth(cam->UniversalLatitude(),
                  cam->UniversalLongitude(), sslat, sslon);
              index += 64 * 64;
            }

            // Special Mosaic indexes
            if (morphologyRank) {
              if (!p_mosd) {
                p_mosd = getMosaicIndicies(*cam, mosd);
              }
              out[index] = mosd.m_morph;
              index += 64 * 64;
            }

            if (albedoRank) {
              if (!p_mosd) {
                p_mosd = getMosaicIndicies(*cam, mosd);
              }
              out[index] = mosd.m_albedo;
              index += 64 * 64;
            }

            if (ra) {
              out[index] = cam->RightAscension();
              index += 64 * 64;
            }

            if (declination) {
              out[index] = cam->Declination();
              index += 64 * 64;
            }

            if (!noCamera) {
              double pB[3];
              cam->Coordinate(pB);
              if (bodyFixedX) {
                out[index] = pB[0];
                index += 64 * 64;
              }

              if (bodyFixedY) {
                out[index] = pB[1];
                index += 64 * 64;
              }
              if (bodyFixedZ) {
                out[index] = pB[2];
                index += 64 * 64;
              }
              
              // Compute the solar illumination mask
              if ( sunilluminationmask ) {
                  vector<double> sB(3), pB(3);
                  cam->sunPosition(&sB[0]);
                  cam->Coordinate(&pB[0]);

                  // Look direction is the sun position to the surface point
                  // in body-fixed
                  vector<double> slookdir(3);
                  vsub_c(&pB[0], &sB[0], &slookdir[0]);

                  out[index]= (int) cam->target()->shape()->isVisibleFrom(sB, slookdir);
                  index += 64 * 64;
              }
              
              // Compute surface oblique detector resolution
              if ( surfaceobliquedetectorresolution ) {
                  out[index] = cam->ObliqueDetectorResolution(false);
                  index += 64 * 64;
              }
            }

            if (localSolarTime) {
              out[index] = cam->LocalSolarTime();
              index += 64 * 64;
            }
          }

          // Trim no target intersection except DN)s), RA and DEC bands
          else {
            int startBand = 0;
            if (dn) {
              startBand = 1;
            }
            else if (alldn) {
              startBand = icube->bandCount();
            }

            for (int band = startBand; band < nbands; band++) {
              if (ra && band == raBandNum) {
                out[index] = cam->RightAscension();
              }
              else if (declination && band == raBandNum + 1) {
                out[index] = cam->Declination();
              }
              else {
                out[index] = Isis::Null;
              }
              index += 64 * 64;
            }
          }
        }
      }
    }; // End processing function


    if (alldn) {
      p.SetInputCube(icube);
    }
    else {
      p.SetInputCube(icube, OneBand);
    }

    Cube *ocube = p.SetOutputCube(ui.GetCubeName("TO"), ui.GetOutputAttribute("TO"),
                                  icube->sampleCount(), icube->lineCount(), nbands);
    p.SetBrickSize(64, 64, nbands);
    p.StartProcess(phocube);

    // Add the bandbin group to the output label.  If a BandBin group already
    // exists, remove all existing keywords and add the keywords for this app.
    // Otherwise, just put the group in.
    PvlObject &cobj = ocube->label()->findObject("IsisCube");
    if (!cobj.hasGroup("BandBin")) {
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


  // Computes the special MORPHOLOGYRANK and ALBEDORANK planes
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
      // Compute MORPHOLOGYRANK
      double cose = cos(myemission.radians());
      if (fabs(cose) < Epsilon) cose = Epsilon;
      // Convert resolution to units of KM
      md.m_morph = (res / 1000.0) / cose;

      if (myincidence.isValid()) {
        // Compute ALBEDORANK
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
