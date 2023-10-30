/** This is free and unencumbered software released into the public domain.

The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */

#include "MiniRF.h"

#include <QString>

#include "IString.h"
#include "iTime.h"
#include "IException.h"
#include "RadarPulseMap.h"
#include "RadarGroundRangeMap.h"
#include "RadarSlantRangeMap.h"
#include "RadarGroundMap.h"
#include "RadarSkyMap.h"

using namespace std;

namespace Isis {
  /**
   * @brief Initialize the Mini-RF SAR radar model for LRO and Chandrayaan 1
   *
   * This constructor reads the image labels of a Mini-RF SAR file to acquire
   * its default parameters.
   *
   * @param lab The label provided to initialize the radar model.
   * @internal
   * @history 2009-07-31 Jeannie Walldren - Added tolerance
   *            parameter value of -1 to call to
   *            Spice::CreateCache() method.
   * @history 2009-07-31 Debbie Cook - Calculated actual tolerance
   *          value to pass into Spice::CreateCache() method.
   */
  MiniRF::MiniRF(Isis::Cube &cube) : Isis::RadarCamera(cube) {

    // LRO MiniRF naif instrument code = -85700
    if (naifIkCode() == -85700) {
      m_instrumentNameLong = "Miniature Radio Frequency";
      m_instrumentNameShort = "Mini-RF";
      m_spacecraftNameLong = "Lunar Reconnaissance Orbiter";
      m_spacecraftNameShort = "LRO";
    }
    // Chandrayaan Mini-SAR instrument code = -86001
    else if (naifIkCode() == -86001) {
      m_instrumentNameLong = "Miniature Synthetic Aperture Radar";
      m_instrumentNameShort = "Mini-SAR";
      m_spacecraftNameLong = "Chandrayaan 1";
      m_spacecraftNameShort = "Chan1";
    }
    else {
      QString msg = "Cube does not appear to be a mini RF image";
      throw IException(IException::Programmer, msg, _FILEINFO_);
    }

    // Get the ground range resolution (ScaledPixelHeight and ScaledPixelWidth
    // are expected to be equal - mrf2isis checks for this)
    Pvl &lab = *cube.label();
    Isis::PvlGroup &inst = lab.findGroup("Instrument", Isis::Pvl::Traverse);
    double groundRangeResolution = inst["ScaledPixelHeight"]; // meters

    // Synthesize the pixel pitch to the ground range resolution
    SetPixelPitch(groundRangeResolution); // meters/pix

    // Focal length should always be slant range to the current ground
    // point. This will be set each time the slant range is calculated.
    // For now, set the focal length to 1.0.
    SetFocalLength(1.0);

    // Get the start time from labels (the SpacecraftClockStartCount is set to
    // is set to UNK in the PDS labels, so StartTime is used instead)
    SpiceDouble etStart = iTime(QString::fromStdString(inst["StartTime"])).Et();

    // The line rate is in units of seconds in the PDS label. The exposure
    // is the sum of the burst and the delay for the return. The movement of the
    // spacecraft is negligible compared to the speed of light, so we are assuming
    // the spacecraft hasn't moved between the burst and the return.
    double lineRate = (double) inst["LineExposureDuration"];

    // Get the incidence angle at the center of the image
    double incidenceAngle = (double) inst["IncidenceAngle"];
    incidenceAngle = incidenceAngle * Isis::PI / 180.0;

    // Get the azimuth resolution at the center of the image
    double azimuthResolution = (double) inst["AzimuthResolution"]; // label units are meters
    azimuthResolution = azimuthResolution / 1000.0; // change to km

    // Get the range resolution at the center of the image
    double rangeResolution = (double) inst["RangeResolution"]; // label units are meters

    // Get the wavelength or frequency of the instrument. This does not
    // exist in the PDS labels, so it will need to be hardcoded until the
    // PDS labels are updated. Right now, the mrf2isis program is putting
    // a frequency value in the labels based on the instrument mode id.
    double frequency = (double) inst["Frequency"]; // units are htz
    double waveLength = clight_c() / frequency;    // units are km/sec/htz

    // Setup map from image(sample,line) to radar(sample,time)
    new RadarPulseMap(this, etStart, lineRate);

    // Setup map from radar(sample,time) to radar(groundrange,time)
    Radar::LookDirection ldir = Radar::Right;
    if(QString::fromStdString(inst["LookDirection"]) == "LEFT") {
      ldir = Radar::Left;
    }
    RadarGroundRangeMap::setTransform(naifIkCode(), groundRangeResolution,
                                      this->Samples(), ldir);
    new RadarGroundRangeMap(this, naifIkCode());

    // Calculate weighting for focal plane coordinates. This is done
    // because the focal plane coordinates (slant range and Doppler
    // shift) do not have the same units of measurement and cannot
    // be used by jigsaw as is. The weighting factors convert the
    // focal plane coordinates into comparitive values. The weighting
    // factor for the Doppler shift requires spacecraft pointing and
    // velocity at the center of the image, so it is calculated
    // after Spice gets loaded.
    double range_sigma = rangeResolution * sin(incidenceAngle) * 100; // scaled meters
    double etMid = etStart + 0.5 * (this->ParentLines() + 1) * lineRate;

    // Setup the map from Radar(groundRange,t) to Radar(slantRange,t)
    RadarSlantRangeMap *slantRangeMap = new RadarSlantRangeMap(this,
        groundRangeResolution);
    slantRangeMap->SetCoefficients(inst["RangeCoefficientSet"]);

    // Setup the ground and sky map
    RadarGroundMap *groundMap = new RadarGroundMap(this, ldir, waveLength);
    groundMap->SetRangeSigma(range_sigma);
    new RadarSkyMap(this);

    // Set the time range to cover the cube
    // Must be done last as the naif kernels will be unloaded
    double etEnd = etStart + this->ParentLines() * lineRate + lineRate;
    etStart = etStart - lineRate;
    double tol = PixelResolution() / 100.;

    if(tol < 0.) {
      // Alternative calculation of .01*ground resolution of a pixel
      setTime(etMid);
      tol = PixelPitch() * SpacecraftAltitude() / FocalLength() / 100.;
    }
    Spice::createCache(etStart, etEnd, this->ParentLines() + 1, tol);
    setTime(etMid);
    SpiceRotation *bodyFrame = this->bodyRotation();
    SpicePosition *spaceCraft = this->instrumentPosition();

    SpiceDouble Ssc[6];
    // Load the state into Ssc
    vequ_c((SpiceDouble *) & (spaceCraft->Coordinate()[0]), Ssc);
    vequ_c((SpiceDouble *) & (spaceCraft->Velocity()[0]), Ssc + 3);
    // Create the J2000 to body-fixed state transformation matrix BJ
    SpiceDouble BJ[6][6];
    rav2xf_c(&(bodyFrame->Matrix()[0]), (SpiceDouble *) & (bodyFrame->AngularVelocity()[0]), BJ);
    // Rotate the spacecraft state from J2000 to body-fixed
    mxvg_c(BJ, Ssc, 6, 6, Ssc);
    // Extract the body-fixed position and velocity
    double Vsc[3];
    double Xsc[3];
    vequ_c(Ssc, Xsc);
    vequ_c(Ssc + 3, Vsc);

    Isis::Distance radii[3];
    this->radii(radii);
    double R = radii[0].kilometers();
    double height = sqrt(Xsc[0] * Xsc[0] + Xsc[1] * Xsc[1] + Xsc[2] * Xsc[2]) - R;
    double speed = vnorm_c(Vsc);
    double dopplerSigma = 2.0 * speed * azimuthResolution / (waveLength *
                          height / cos(incidenceAngle)) * 100.;
    groundMap->SetDopplerSigma(dopplerSigma);
    slantRangeMap->SetWeightFactors(range_sigma, dopplerSigma);
  }

  /**
   * CK frame ID.  This is an overridden method for the Camera class pure
   * virtual method. It will always throw an error for MiniRF models.
   * @throw iException - "Cannot generate CK for MiniRF."
   * @return @b int
   */
  int MiniRF::CkFrameId() const {
    std::string msg = "Cannot generate CK for MiniRF";
    throw IException(IException::User, msg, _FILEINFO_);
  }

  /**
   * CK Reference ID.  This is an overridden method for the Camera class pure
   * virtual method. It will always throw an error for MiniRF models.
   * @throw iException - "Cannot generate CK for MiniRF."
   * @return @b int
   */
  int MiniRF::CkReferenceId() const {
    std::string msg = "Cannot generate CK for MiniRF";
    throw IException(IException::User, msg, _FILEINFO_);
  }
}

/**
 * This is the function that is called in order to instantiate a
 * MiniRF object.
 *
 * @param lab Cube labels
 *
 * @return Isis::Camera* MiniRF
 * @internal
 *   @history 2011-05-03 Jeannie Walldren - Removed Lro namespace.
 */
extern "C" Isis::Camera *MiniRFPlugin(Isis::Cube &cube) {
  return new Isis::MiniRF(cube);
}
