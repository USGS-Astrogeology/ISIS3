#include "ShadowFunctor.h"

#include <algorithm>

#include <QDebug>
#include <QFile>

#include "Angle.h"
#include "Buffer.h"
#include "Camera.h"
#include "Cube.h"
#include "Displacement.h"
#include "Distance.h"
#include "FileName.h"
#include "Hillshade.h"
#include "IException.h"
#include "iTime.h"
#include "Latitude.h"
#include "Longitude.h"
#include "Portal.h"
#include "Projection.h"
#include "PvlGroup.h"
#include "Statistics.h"
#include "SurfacePoint.h"
#include "TProjection.h"

namespace Isis {
  /**
   * The input DEM cube must remain in memory as long as this class (or any copy thereof) remains
   *   in memory. Any method calls (excluding the destructor) made on this class after the input
   *   DEM cube is deleted will cause undefined behavior. This does NOT take ownership of the input
   *   DEM.
   */
  ShadowFunctor::ShadowFunctor(Cube *inputDem) {
    nullify();

    m_inputDem = inputDem;

    try {
      Table shapeModelStats = inputDem->readTable("ShapeModelStatistics");

      // We store this in KM, not M, in this class.
      m_inputDemMax = (double)shapeModelStats[0]["MaximumRadius"] * 1000.0;

      if ((double)shapeModelStats[0]["MinimumRadius"] <= 0.0) {
        throw IException(IException::Unknown,"The input cube [" + inputDem->fileName().toStdString() + "] to the shadowing algorithm must be a DEM "
                             "which stores radii; The input DEM contains zero or negative radii",
                         _FILEINFO_);
      }
    }
    catch (IException &e) {
      throw IException(e, IException::Unknown, "The input cube [" + inputDem->fileName().toStdString() + "] is not a proper DEM. All DEM "
                                   "files must now be padded at the poles and contain a "
                                   "ShapeModelStatistics table defining their minimum and maximum "
                                   "radii values. The demprep program should be used to prepare the "
                                   "DEM before you can run this program. There is more information "
                                   "available in the documentation of the demprep program.",
                       _FILEINFO_);
    }

    m_enableShadowCalculations = true;

    m_sunPositionInBodyFixed = new double[3];
    m_sunPositionInBodyFixed[0] = 0.0;
    m_sunPositionInBodyFixed[1] = 0.0;
    m_sunPositionInBodyFixed[2] = 0.0;

    m_rayPrecision = 1.0;

    m_rayLengthStats = new Statistics;
    m_azimuthStats = new Statistics;
    m_elevationStats = new Statistics;
    m_shadowedStats = new Statistics;
    m_shadowedByRayStats = new Statistics;

    // 1.001211 solar radii is our current best known value for the radius of the sun
    m_sunRadius = new Distance(1.001211, Distance::SolarRadii);
  }


  /*
   * This is left here for future use when we actually use the functionality. This was bug free
   * at the time of coding.
   *
   * Copy the shadow functor. This new copy references the original input DEM cube and thus the
   *   input DEM must still be in memory.
   */
//   ShadowFunctor::ShadowFunctor(const ShadowFunctor &other) {
//     nullify();
//
//     m_inputDem = other.m_inputDem;
//     m_inputDemMax = other.m_inputDemMax;
//
//     m_enableShadowCalculations = other.m_enableShadowCalculations;
//
//     m_rayPrecision = other.m_rayPrecision;
//
//     m_sunPositionInBodyFixed = new double[3];
//     memcpy(m_sunPositionInBodyFixed, other.m_sunPositionInBodyFixed, sizeof(double) * 3);
//
//     m_rayLengthStats = new Statistics(*other.m_rayLengthStats);
//     m_azimuthStats = new Statistics(*other.m_azimuthStats);
//     m_elevationStats = new Statistics(*other.m_elevationStats);
//     m_shadowedStats = new Statistics(*other.m_shadowedStats);
//     m_shadowedByRayStats = new Statistics(*other.m_shadowedByRayStats);
//
//     m_sunRadius = new Distance(*other.m_sunRadius);
//   }


  /**
   * Clean up/free this instance.
   */
  ShadowFunctor::~ShadowFunctor() {
    delete [] m_sunPositionInBodyFixed;
    delete m_sunRadius;
    delete m_shadowedPositions;
    delete m_lightedElevations;

    delete m_rayLengthStats;
    delete m_azimuthStats;
    delete m_elevationStats;
    delete m_shadowedStats;
    delete m_shadowedByRayStats;

    nullify();
  }


  /**
   * Get a report that details how the process went for users.
   */
  PvlGroup ShadowFunctor::report() const {
    PvlGroup shadowStats("ShadowStatistics");

    shadowStats += PvlKeyword("NumComputedAzimuthElevations",
                              Isis::toString(m_azimuthStats->ValidPixels()));

    if (m_azimuthStats->ValidPixels() > 0) {
      PvlKeyword averageAzimuth("AverageAzimuth",
                                Isis::toString(m_azimuthStats->Average()));
      averageAzimuth.addCommentWrapped("The azimuth is measured from three o'clock, going "
                                       "clockwise, in degrees");
      shadowStats += averageAzimuth;

      shadowStats += PvlKeyword("MinimumAzimuth", Isis::toString(m_azimuthStats->Minimum()));
      shadowStats += PvlKeyword("MaximumAzimuth", Isis::toString(m_azimuthStats->Maximum()));

      PvlKeyword averageElevation("AverageElevation", Isis::toString(m_elevationStats->Average()));
      averageElevation.addCommentWrapped("The elevation is measured from the normal, with directly "
          "overhead being 0 degrees and the horizon 90 degrees. Elevations are prevented from "
          "going below the horizon.");
      shadowStats += averageElevation;

      shadowStats += PvlKeyword("MinimumElevation", Isis::toString(m_elevationStats->Minimum()));
      shadowStats += PvlKeyword("MaximumElevation", Isis::toString(m_elevationStats->Maximum()));
    }

    PvlKeyword numRays("NumRays", Isis::toString(m_rayLengthStats->ValidPixels()));
    numRays.addCommentWrapped("This is the total number of rays traced from the surface towards "
                              "the sun in order to detect if any given pixel is in shadow");
    shadowStats += numRays;

    shadowStats += PvlKeyword("NumRayDemIntersections",
                              Isis::toString(qRound(m_rayLengthStats->Sum())));

    if (m_rayLengthStats->ValidPixels() > 0) {
      shadowStats += PvlKeyword("AverageRayDemIntersectionsPerRay",
                                Isis::toString(m_rayLengthStats->Average()));
    }

    shadowStats += PvlKeyword("NumLightedPixels",
        Isis::toString(qRound((1.0 - m_shadowedStats->Average()) * m_shadowedStats->ValidPixels())));
    shadowStats += PvlKeyword("NumShadowedPixels",
        Isis::toString(qRound(m_shadowedStats->Average() * m_shadowedStats->ValidPixels())));
    shadowStats += PvlKeyword("NumSpecialPixels",
        Isis::toString(m_shadowedStats->TotalPixels() - m_shadowedStats->ValidPixels()));
    shadowStats += PvlKeyword("NumPixelsShadowedByRays",
        Isis::toString(qRound(m_shadowedByRayStats->Sum())));

    return shadowStats;
  }


  /**
   * Compute shadows for the given input DEM area and store the results in output.
   *
   * @param input A section of the input DEM cube
   * @param output The equivalent result
   */
  void ShadowFunctor::operator()(Buffer &input, Buffer &output) const {
    // We want to process the input in random order and not linearly to minimize any systematic
    //   optimization failures later on. Store off all possible positions, shuffle, and work from
    //   the shuffled list of positions.
    QList< QPair<int, int> > sampleLines;

    for (int line = input.Line() - 1; line < (input.Line() - 1) + input.LineDimension(); line++) {
      for (int sample = 0; sample < input.SampleDimension(); sample++) {
        sampleLines.append(qMakePair(sample, line));
      }
    }

    // Shuffle sampleLines
    for (int i = 2; i < sampleLines.count(); i++) {
      std::swap(sampleLines[i], sampleLines[ rand() % i ]);
    }

    TProjection *elevationModelProjection = (TProjection *)m_inputDem->projection();

    QPair<int, int> sampleLine;
    foreach (sampleLine, sampleLines) {
      int sample = sampleLine.first;
      int line = sampleLine.second;
      int bufferIndex = output.Index(sample + 1, line + 1, output.Band());


      if (!IsSpecial(input[bufferIndex])) {
        Distance demElevation = Distance(input[bufferIndex], Distance::Meters);

        // We need to calculate the direction of the light source (sun) relative to the surface
        //   point.
        if (elevationModelProjection->SetWorld(sample + 1, line + 1)) {
          SurfacePoint startSurfacePoint(
              Latitude(elevationModelProjection->UniversalLatitude(), Angle::Degrees),
              Longitude(elevationModelProjection->UniversalLongitude(), Angle::Degrees),
              demElevation);

          // Meters
          double rayStartPointInBodyFixed[3] = {
              startSurfacePoint.GetX().meters(),
              startSurfacePoint.GetY().meters(),
              startSurfacePoint.GetZ().meters()
          };

          double rayFromSurfaceToSunCenter[3] = {
              m_sunPositionInBodyFixed[0] - rayStartPointInBodyFixed[0],
              m_sunPositionInBodyFixed[1] - rayStartPointInBodyFixed[1],
              m_sunPositionInBodyFixed[2] - rayStartPointInBodyFixed[2]
          };

          double rayFromSurfaceToSunCenterSize = sqrt(
              rayFromSurfaceToSunCenter[0] * rayFromSurfaceToSunCenter[0] +
              rayFromSurfaceToSunCenter[1] * rayFromSurfaceToSunCenter[1] +
              rayFromSurfaceToSunCenter[2] * rayFromSurfaceToSunCenter[2]);

          double rayFromSurfaceToSunCenterNormalized[3] = {
              rayFromSurfaceToSunCenter[0] / rayFromSurfaceToSunCenterSize,
              rayFromSurfaceToSunCenter[1] / rayFromSurfaceToSunCenterSize,
              rayFromSurfaceToSunCenter[2] / rayFromSurfaceToSunCenterSize
          };

          double rayFromSurfaceToSun[3] = {
              rayFromSurfaceToSunCenter[0],
              rayFromSurfaceToSunCenter[1],
              rayFromSurfaceToSunCenter[2]
          };

          bool couldBeShadowed = true;
          if (m_traceLightToSunEdge) {
            // To find the edge of the sun, we need to conceptually convert the spherical sun to a
            //   disc. The disc is defined by the normal of the vector from surface to sun, centered
            //   on the center of the sun, with a radius equal to the radius of the sun. We then find
            //   the nearest point on the disc to the normal vector of the point on the surface (up).
            //
            // You find the nearest point on the disc to the normal vector by intersecting the normal
            //   vector with the plane that the disc lies on.
            //
            // If the normal vector intersects the sun's disc, then there can be no shadow. The
            //   purpose of doing this computation is to minimize the incidence angle, giving us the
            //   most sun possible. Maximizing the sun is important for using hard shadows to estimate
            //   soft shadows.

            // scalar = (rayFromSurfaceToSun dot sunPositionInBodyFixed) /
            //          (rayFromSurfaceToSun dot rayStartPointInBodyFixed)
            // scalar is the multiplier on the ray to get the point where it intersects the surface
            // Equation is derived from:
            //   http://www.softsurfer.com/Archive/algorithm_0104/algorithm_0104B.htm
            double normalVectorScalar =
                (rayFromSurfaceToSun[0] * m_sunPositionInBodyFixed[0] +
                 rayFromSurfaceToSun[1] * m_sunPositionInBodyFixed[1] +
                 rayFromSurfaceToSun[2] * m_sunPositionInBodyFixed[2]) /
                (rayFromSurfaceToSun[0] * rayStartPointInBodyFixed[0] +
                 rayFromSurfaceToSun[1] * rayStartPointInBodyFixed[1] +
                 rayFromSurfaceToSun[2] * rayStartPointInBodyFixed[2]);

            double planeIntersectionPoint[3] = {
                normalVectorScalar * rayStartPointInBodyFixed[0],
                normalVectorScalar * rayStartPointInBodyFixed[1],
                normalVectorScalar * rayStartPointInBodyFixed[2]
            };

            double vectorFromSunCenterToPlaneIntersection[3] = {
                planeIntersectionPoint[0] - m_sunPositionInBodyFixed[0],
                planeIntersectionPoint[1] - m_sunPositionInBodyFixed[1],
                planeIntersectionPoint[2] - m_sunPositionInBodyFixed[2]
            };

            double vectorFromSunCenterToPlaneIntersectionSize = sqrt(
                vectorFromSunCenterToPlaneIntersection[0] *
                vectorFromSunCenterToPlaneIntersection[0] +
                vectorFromSunCenterToPlaneIntersection[1] *
                vectorFromSunCenterToPlaneIntersection[1] +
                vectorFromSunCenterToPlaneIntersection[2] *
                vectorFromSunCenterToPlaneIntersection[2]);

            // If the sun's disc is literally directly above, our pixel can't be shadowed.
            if (*m_sunRadius >
                Distance(vectorFromSunCenterToPlaneIntersectionSize, Distance::Meters)) {
              couldBeShadowed = false;
            }

            double sunCenterToPlaneIntersectionNormalized[3] = {
                vectorFromSunCenterToPlaneIntersection[0] /
                vectorFromSunCenterToPlaneIntersectionSize,
                vectorFromSunCenterToPlaneIntersection[1] /
                vectorFromSunCenterToPlaneIntersectionSize,
                vectorFromSunCenterToPlaneIntersection[2] /
                vectorFromSunCenterToPlaneIntersectionSize
            };

            // 1 solar radii isn't completely accurate, but it's close (within several significant
            //   figures of precision).
            double sunEdgeInBoxyFixed[3] = {
                m_sunPositionInBodyFixed[0] + m_sunRadius->meters() *
                                            sunCenterToPlaneIntersectionNormalized[0],
                m_sunPositionInBodyFixed[1] + m_sunRadius->meters() *
                                            sunCenterToPlaneIntersectionNormalized[1],
                m_sunPositionInBodyFixed[2] + m_sunRadius->meters() *
                                            sunCenterToPlaneIntersectionNormalized[2]
            };

            rayFromSurfaceToSun[0] = sunEdgeInBoxyFixed[0] - rayStartPointInBodyFixed[0];
            rayFromSurfaceToSun[1] = sunEdgeInBoxyFixed[1] - rayStartPointInBodyFixed[1];
            rayFromSurfaceToSun[2] = sunEdgeInBoxyFixed[2] - rayStartPointInBodyFixed[2];
          }

          SurfacePoint secondSurfacePoint(
              Displacement(rayStartPointInBodyFixed[0] + rayFromSurfaceToSunCenterNormalized[0],
                           Displacement::Meters),
              Displacement(rayStartPointInBodyFixed[1] + rayFromSurfaceToSunCenterNormalized[1],
                           Displacement::Meters),
              Displacement(rayStartPointInBodyFixed[2] + rayFromSurfaceToSunCenterNormalized[2],
                           Displacement::Meters));


          elevationModelProjection->SetUniversalGround(secondSurfacePoint.GetLatitude().degrees(),
                                                       secondSurfacePoint.GetLongitude().degrees());

          double y = elevationModelProjection->WorldY() - (line + 1);
          double x = elevationModelProjection->WorldX() - (sample + 1);

          Angle azimuthFromThree = Angle(atan2(y, x), Angle::Radians);
          m_azimuthStats->AddData(azimuthFromThree.degrees());

          Angle azimuth = azimuthFromThree + Angle(90, Angle::Degrees);

          // Bring the azimuth into 0-360
          if (azimuth > Angle::fullRotation()) {
            azimuth -= Angle::fullRotation();
          }
          else if (azimuth < Angle(0, Angle::Degrees)) {
            azimuth += Angle::fullRotation();
          }

          // Elevation is the angle between the plane, defined by the normal vector (surface point)
          //    and intersecting the surface point. To find the angle from normal, we know:
          //  cos(elevation) = normal dot sun direction
          //  elevation = acos(normal dot sun direction)
          double rayStartPointInBodyFixedSize = sqrt(
              rayStartPointInBodyFixed[0] * rayStartPointInBodyFixed[0] +
              rayStartPointInBodyFixed[1] * rayStartPointInBodyFixed[1] +
              rayStartPointInBodyFixed[2] * rayStartPointInBodyFixed[2]);

          double rayStartPointInBodyFixedNormalized[3] = {
              rayStartPointInBodyFixed[0] / rayStartPointInBodyFixedSize,
              rayStartPointInBodyFixed[1] / rayStartPointInBodyFixedSize,
              rayStartPointInBodyFixed[2] / rayStartPointInBodyFixedSize
          };

          Angle elevation(
              acos(rayStartPointInBodyFixedNormalized[0] * rayFromSurfaceToSunCenterNormalized[0] +
                   rayStartPointInBodyFixedNormalized[1] * rayFromSurfaceToSunCenterNormalized[1] +
                   rayStartPointInBodyFixedNormalized[2] * rayFromSurfaceToSunCenterNormalized[2]),
              Angle::Radians);

          elevation = qMin(elevation, Angle(90, Angle::Degrees));
          m_elevationStats->AddData(elevation.degrees());

          Hillshade hillshade(azimuth, elevation, elevationModelProjection->Resolution());


          Portal portal(3, 3, m_inputDem->pixelType(), -0.5, -0.5);
          portal.SetPosition(sample, line, input.Band());

          if (!portal.CopyOverlapFrom(input)) {
            m_inputDem->read(portal);
          }

          double shadedValue = hillshade.shadedValue(portal);
          if (shadedValue > 0) {
            bool shadowed = m_enableShadowCalculations && couldBeShadowed &&
                isShadowed(rayStartPointInBodyFixed, sample + 1, line + 1,
                           demElevation, rayFromSurfaceToSun, &input);

            if (shadowed) {
              m_shadowedStats->AddData(1.0);
              m_shadowedByRayStats->AddData(1.0);
              output[bufferIndex] = Lrs;
            }
            else {
              m_shadowedStats->AddData(0.0);
              m_shadowedByRayStats->AddData(0.0);
              output[bufferIndex] = shadedValue;
            }
          }
          else {
            m_shadowedByRayStats->AddData(0.0);
            output[bufferIndex] = Lrs;
          }
        }
        else {
          // Set output to Null and add Null to the stats
          output[bufferIndex] = Null;
          m_shadowedStats->AddData(Null);
        }
      }
      else {
        // Add the special pixels to our shadowed stats
        m_shadowedStats->AddData(input[bufferIndex]);

        // Preserve special pixels
        output[bufferIndex] = input[bufferIndex];
      }
    }

    shrinkCaches();
  }


  /**
   * Set the precision of the ray that we trace across the input DEM.
   */
  void ShadowFunctor::setRayPrecision(double approxDemRayTracePrecisionInPixels) {
    if (approxDemRayTracePrecisionInPixels <= 0.0) {
      throw IException(IException::Unknown,"Ray precision [" + toString(approxDemRayTracePrecisionInPixels) + "] must be positive; the algorithm does not "
                                   "support ray tracing backwards through the target",
                       _FILEINFO_);
    }

    m_rayPrecision = approxDemRayTracePrecisionInPixels;
  }


  /**
   * Set the sun position from the camera information in the given cube.
   */
  void ShadowFunctor::setSunPosition(FileName cubeFileNameWithCamToMatch) {
    try {
      Cube matchCube;
      matchCube.open(QString::fromStdString(cubeFileNameWithCamToMatch.original()), "r");

      setSunPosition(&matchCube);
    }
    catch (IException &e) {
      throw IException(e,
          IException::User,"Could not find the sun position from the match file [" + cubeFileNameWithCamToMatch.original() + "]",
          _FILEINFO_);
    }
  }


  /**
   * Set the sun position from the camera information in the given cube.
   */
  void ShadowFunctor::setSunPosition(Cube *cubeWithCamForSunPos) {
    Camera *camForSunPosition = NULL;

    try {
      camForSunPosition = cubeWithCamForSunPos->camera();
    }
    catch (IException &e) {
      throw IException(e,
          IException::User,"The match file [" + cubeWithCamForSunPos->fileName().toStdString() + "] must have camera information in order to identify the "
                      "sun's position.",
          _FILEINFO_);
    }

    setSunPosition(camForSunPosition);
  }


  /**
   * Set the sun position from the time of the center pixel in the given camera.
   */
  void ShadowFunctor::setSunPosition(Camera *camForSunPosition) {
    // Set the camera to the center of the image
    camForSunPosition->SetImage(
        camForSunPosition->ParentSamples() / 2.0 + 0.5,
        camForSunPosition->ParentLines() / 2.0 + 0.5);

    setSunPosition((Spice *)camForSunPosition);
  }


  /**
   * Set the sun position from the time set in the Spice object (which has not been corrected for
   *   light time).
   */
  void ShadowFunctor::setSunPosition(Spice *spiceWithTimeSet) {
    double naifSunPosition[3] = {0.0, 0.0, 0.0};
    spiceWithTimeSet->sunPosition(naifSunPosition);

    double uncorrectedSunPositionInBodyFixed[3] = {
        naifSunPosition[0] * 1000.0,
        naifSunPosition[1] * 1000.0,
        naifSunPosition[2] * 1000.0
    };

    // Meters
    double uncorrectedSunPositionInBodyFixedSize = sqrt(
        uncorrectedSunPositionInBodyFixed[0] * uncorrectedSunPositionInBodyFixed[0] +
        uncorrectedSunPositionInBodyFixed[1] * uncorrectedSunPositionInBodyFixed[1] +
        uncorrectedSunPositionInBodyFixed[2] * uncorrectedSunPositionInBodyFixed[2]);

    // Distance (m) / Speed of Light (m/s) = Rough estimation of time elapsed
    double lightTimeOffsetInSeconds = uncorrectedSunPositionInBodyFixedSize / 299792458;

    spiceWithTimeSet->setTime(spiceWithTimeSet->time().Et() - lightTimeOffsetInSeconds);

    spiceWithTimeSet->sunPosition(naifSunPosition);

    // Convert KM to M
    naifSunPosition[0] *= 1000.0;
    naifSunPosition[1] *= 1000.0;
    naifSunPosition[2] *= 1000.0;

    setSunPosition(naifSunPosition);
  }


  /**
   * Input must be a C-array of size 3. Invalid values are not supported.
   */
  void ShadowFunctor::setSunPosition(double *lightTimeCorrectedSunPositionInBodyFixed) {
    m_sunPositionInBodyFixed[0] = lightTimeCorrectedSunPositionInBodyFixed[0];
    m_sunPositionInBodyFixed[1] = lightTimeCorrectedSunPositionInBodyFixed[1];
    m_sunPositionInBodyFixed[2] = lightTimeCorrectedSunPositionInBodyFixed[2];
  }


  /**
   * Enable/disable interpolated (between ray precision points) values in the caches.
   */
  void ShadowFunctor::enableInterpolatedOptimizations(bool enable) {
    m_allowInterpolatedOptimizations = enable;
  }


  /**
   * Enable/disable using the light curtain optimization with the given settings.
   */
  void ShadowFunctor::enableLightCurtain(
      bool enable, bool adjustElevations, int targetCurtainSize) {
    m_adjustLightCurtainElevations = adjustElevations;
    m_targetLightedElevationsSize = targetCurtainSize;

    delete m_lightedElevations;
    m_lightedElevations = NULL;

    if (enable) {
      m_lightedElevations = new QHash< QPair<int, int>, double >;
    }
  }


  /**
   * This enables/disables calculating shadows at all.
   */
  void ShadowFunctor::enableShadowCalculations(bool enable) {
    m_enableShadowCalculations = enable;
  }


  /**
   * Enable/disable tracing the light rays to the edge of the sun. A larger radius causes smaller
   *   shadows, a smaller radius causes larger shadows.
   */
  void ShadowFunctor::enableShadowTraceToSunEdge(bool enable, Distance sunRadius) {
    m_traceLightToSunEdge = enable;

    if (sunRadius.isValid()) {
      *m_sunRadius = sunRadius;
    }
  }


  /**
   * Enable/disable using the shadow positions optimization with the given target number of points.
   */
  void ShadowFunctor::enableShadowMap(bool enable, int targetMapSize) {
    m_targetShadowedPositionsSize = targetMapSize;

    delete m_shadowedPositions;
    m_shadowedPositions = NULL;

    if (enable) {
      m_shadowedPositions = new QHash< QPair<int, int>, bool >;
    }
  }


  /**
   * Enable/disable trying to skip over shadows - this won't function without the shadow map.
   */
  void ShadowFunctor::enableWalkingOverShadows(bool enable, int maxSteps) {
    m_walkOverShadowMaxSteps = enable? maxSteps : 0;
  }


  /**
   * Load a preset settings profile
   *
   * @param settingsProfile The settings preset to use
   */
  void ShadowFunctor::setQuickSettings(QuickSettings settingsProfile) {
    switch(settingsProfile) {
      case HighAccuracy:
        setRayPrecision(0.98);
        enableLightCurtain(false);
        enableShadowMap(false);
        break;

      case BalancedPerformance:
        setRayPrecision(1.0);
        enableInterpolatedOptimizations(false);
        enableLightCurtain(true, true, 1000000);
        enableShadowMap(true, 1000000);
        enableWalkingOverShadows(true, 5);
        break;
    }
  }


  /*
   * This is left here for future use when we actually use the functionality. This was bug free
   * at the time of coding.
   *
   * Swap this instance's state with the given instance. This should never throw an exception.
   *
   * @param other The ShadowFunctor to swap instance data with
   */
//   void ShadowFunctor::swap(ShadowFunctor &other) {
//     std::swap(m_inputDem, other.m_inputDem);
//     std::swap(m_inputDemMax, other.m_inputDemMax);
//
//     std::swap(m_enableShadowCalculations, other.m_enableShadowCalculations);
//
//     std::swap(m_sunPositionInBodyFixed, other.m_sunPositionInBodyFixed);
//
//     std::swap(m_adjustLightCurtainElevations, other.m_adjustLightCurtainElevations);
//     std::swap(m_allowInterpolatedOptimizations, other.m_allowInterpolatedOptimizations);
//     std::swap(m_walkOverShadowMaxSteps, other.m_walkOverShadowMaxSteps);
//     std::swap(m_rayPrecision, other.m_rayPrecision);
//     std::swap(m_sunRadius, other.m_sunRadius);
//     std::swap(m_targetShadowedPositionsSize, other.m_targetShadowedPositionsSize);
//     std::swap(m_targetLightedElevationsSize, other.m_targetLightedElevationsSize);
//     std::swap(m_traceLightToSunEdge, other.m_traceLightToSunEdge);
//
//     std::swap(m_rayLengthStats, other.m_rayLengthStats);
//     std::swap(m_azimuthStats, other.m_azimuthStats);
//     std::swap(m_elevationStats, other.m_elevationStats);
//     std::swap(m_shadowedStats, other.m_shadowedStats);
//     std::swap(m_shadowedByRayStats, other.m_shadowedByRayStats);
//
//     std::swap(m_shadowedPositions, other.m_shadowedPositions);
//     std::swap(m_lightedElevations, other.m_lightedElevations);
//   }


  /*
   * This is left here for future use when we actually use the functionality. This was bug free
   * at the time of coding.
   *
   * Assignment operator. This instance will become identical to the given ShadowFunctor.
   */
//   ShadowFunctor &ShadowFunctor::ShadowFunctor::operator=(const ShadowFunctor &rhs) {
//     ShadowFunctor copy(rhs);
//     swap(copy);
//     return *this;
//   }


  /**
   * This method expects the class state to be uninitialized or invalid. This resets all variables
   *   to NULL or 0 values.
   */
  void ShadowFunctor::nullify() {
    m_inputDem = NULL;
    m_inputDemMax = 0.0;

    m_enableShadowCalculations = false;

    m_sunPositionInBodyFixed = NULL;

    m_adjustLightCurtainElevations = false;
    m_allowInterpolatedOptimizations = false;
    m_walkOverShadowMaxSteps = 0;
    m_rayPrecision = 0.0;
    m_sunRadius = NULL;
    m_targetShadowedPositionsSize = 0;
    m_targetLightedElevationsSize = 0;
    m_traceLightToSunEdge = 0;

    m_rayLengthStats = NULL;
    m_azimuthStats = NULL;
    m_elevationStats = NULL;
    m_shadowedStats = NULL;
    m_shadowedByRayStats = NULL;

    m_shadowedPositions = NULL;
    m_lightedElevations = NULL;
  }


  /**
   * Test if a point in the input DEM is in shadow. This uses and adds to the optimization caches if
   *   enabled.
   *
   * @param rayStartPointInBodyFixed This needs to be the originating point on the planet in meters
   * @param demSample This must be the sample we care about in the input DEM.
   * @param demLine This must be the line we care about in the input DEM.
   * @param demStartRadius This should be the magnitude of rayStartPointInBodyFixed
   * @param directionInBodyFixed This should be directional ray from the start point to the light
   *                             source; optimally, this is to the top edge of the sun.
   * @param quickCache Input DEM reads will be attempted here before going back to the DEM cube.
   *
   * @return True if the input pixel is determined to be in shadow
   */
  bool ShadowFunctor::isShadowed(double *rayStartPointInBodyFixed,
                  int demSample, int demLine, Distance demStartRadius,
                  double *directionInBodyFixed, Buffer *quickCache) const {
    Interpolator interpolator(Interpolator::BiLinearType);
    Portal portal(interpolator.Samples(), interpolator.Lines(), m_inputDem->pixelType(),
                  interpolator.HotSample(), interpolator.HotLine());

    bool tooFarFromTarget = false;
    bool shadowed = false;

    double directionMagnitude = sqrt(directionInBodyFixed[0] * directionInBodyFixed[0] +
                                     directionInBodyFixed[1] * directionInBodyFixed[1] +
                                     directionInBodyFixed[2] * directionInBodyFixed[2]);

    double xStep = directionInBodyFixed[0] / directionMagnitude;
    double yStep = directionInBodyFixed[1] / directionMagnitude;
    double zStep = directionInBodyFixed[2] / directionMagnitude;

    double pointOnRayFromSurfaceToSun[3] = {
        0.0,
        0.0,
        0.0
    };

    QScopedPointer<double> demSampleAlongRay;
    QScopedPointer<double> demLineAlongRay;

    QList< QPair<int, int> > crossedPositions;
    QList<double> elevations;

    if (m_shadowedPositions && m_shadowedPositions->contains(qMakePair(demSample, demLine))) {
      shadowed = true;
    }

    double minimumDistanceFromRayToApproachingGround = DBL_MAX;
    double lastRayGroundDistance = DBL_MAX;

    Projection *elevationModelProjection = m_inputDem->projection();
    int step = 0;
    while (!tooFarFromTarget && !shadowed) {
      step++;

      pointOnRayFromSurfaceToSun[0] += xStep;
      pointOnRayFromSurfaceToSun[1] += yStep;
      pointOnRayFromSurfaceToSun[2] += zStep;

      double pointOnRayFromSurfaceToSunInBodyFixed[3] = {
          rayStartPointInBodyFixed[0] + pointOnRayFromSurfaceToSun[0],
          rayStartPointInBodyFixed[1] + pointOnRayFromSurfaceToSun[1],
          rayStartPointInBodyFixed[2] + pointOnRayFromSurfaceToSun[2]
      };

      double pointOnRayFromSurfaceToSunInBodyFixedSize = sqrt(
          pointOnRayFromSurfaceToSunInBodyFixed[0] * pointOnRayFromSurfaceToSunInBodyFixed[0] +
          pointOnRayFromSurfaceToSunInBodyFixed[1] * pointOnRayFromSurfaceToSunInBodyFixed[1] +
          pointOnRayFromSurfaceToSunInBodyFixed[2] * pointOnRayFromSurfaceToSunInBodyFixed[2]);

      Distance rayPointElevation(pointOnRayFromSurfaceToSunInBodyFixedSize, Distance::Meters);
      tooFarFromTarget = (rayPointElevation > Distance(m_inputDemMax, Distance::Meters));

      // We need to find the DEM line/sample that corresponds to this ray point.
      SurfacePoint surfacePoint(
          Displacement(pointOnRayFromSurfaceToSunInBodyFixed[0], Displacement::Meters),
          Displacement(pointOnRayFromSurfaceToSunInBodyFixed[1], Displacement::Meters),
          Displacement(pointOnRayFromSurfaceToSunInBodyFixed[2], Displacement::Meters));

      elevationModelProjection->SetUniversalGround(surfacePoint.GetLatitude().degrees(),
                                                   surfacePoint.GetLongitude().degrees());

      if (demSampleAlongRay && demLineAlongRay) {
        double lastDemXStep = *demSampleAlongRay - elevationModelProjection->WorldX();
        double lastDemYStep = *demLineAlongRay - elevationModelProjection->WorldY();

        // Aim for precision pixel accuracy
        lastDemXStep /= m_rayPrecision;
        lastDemYStep /= m_rayPrecision;

        double lastDemStepSize = sqrt(lastDemXStep * lastDemXStep + lastDemYStep * lastDemYStep);

        xStep = (xStep / lastDemStepSize);
        yStep = (yStep / lastDemStepSize);
        zStep = (zStep / lastDemStepSize);

        if (m_shadowedPositions && m_walkOverShadowMaxSteps > 0) {
          // If the next estimated point is shadowed, skip past it. We don't need to try to
          //   intersect already-shadowed features. Since the ray doesn't project straight, we can
          //   only estimate this for a short distance.
          int estimateDistance = 0;

          double enlargedXStep = xStep;
          double enlargedYStep = yStep;
          double enlargedZStep = zStep;

          double nextEstimatedSample = qRound(elevationModelProjection->WorldX() +
                                              lastDemXStep / lastDemStepSize);
          double nextEstimatedLine = qRound(elevationModelProjection->WorldY() +
                                              lastDemYStep / lastDemStepSize);
          while (estimateDistance < m_walkOverShadowMaxSteps &&
                 m_shadowedPositions->contains(
                   qMakePair(qRound(nextEstimatedSample), qRound(nextEstimatedLine)))) {
            nextEstimatedSample += (lastDemXStep / lastDemStepSize);
            nextEstimatedLine += (lastDemYStep / lastDemStepSize);

            enlargedXStep += xStep;
            enlargedYStep += yStep;
            enlargedZStep += zStep;

            estimateDistance++;
          }

          xStep = enlargedXStep;
          yStep = enlargedYStep;
          zStep = enlargedZStep;
        }
      }
      else {
        demSampleAlongRay.reset(new double(0.0));
        demLineAlongRay.reset(new double(0.0));
      }

      *demSampleAlongRay = elevationModelProjection->WorldX();
      *demLineAlongRay = elevationModelProjection->WorldY();
      double rayElevationMeters = rayPointElevation.meters();

      QPair<int, int> demPositionAlongRay =
          qMakePair(qRound(*demSampleAlongRay), qRound(*demLineAlongRay));

      double distanceFromDemOrigin = sqrt(
          (demPositionAlongRay.first - demSample) * (demPositionAlongRay.first - demSample) +
          (demPositionAlongRay.second - demLine) * (demPositionAlongRay.second - demLine));

      if (distanceFromDemOrigin > 9) {
        crossedPositions.append(demPositionAlongRay);
        elevations.append(rayElevationMeters);

        portal.SetPosition(*demSampleAlongRay, *demLineAlongRay, 1);

        if (!portal.CopyOverlapFrom(*quickCache)) {
          m_inputDem->read(portal);
        }

        double demValue = interpolator.Interpolate(*demSampleAlongRay, *demLineAlongRay,
                                                   portal.DoubleBuffer());


        if (!IsSpecial(demValue)) {
          if (lastRayGroundDistance < rayElevationMeters) {
            minimumDistanceFromRayToApproachingGround =
                qMin(minimumDistanceFromRayToApproachingGround, rayElevationMeters - demValue);
          }

          lastRayGroundDistance = rayElevationMeters;
        }

        // If the DEM has a special pixel value, either we're off the DEM or in an unknown area.
        //   Regardless, stop looking for something that blocks light.
        if (IsSpecial(demValue)) {
          tooFarFromTarget = true;
        }
        else {
          shadowed = (rayElevationMeters <= demValue);
          if (!shadowed && m_lightedElevations &&
              m_lightedElevations->contains(demPositionAlongRay)) {
            double lightCurtainElevationMeters = (*m_lightedElevations)[demPositionAlongRay];
            if (rayElevationMeters >=  lightCurtainElevationMeters ||
                qFuzzyCompare(rayElevationMeters, lightCurtainElevationMeters)) {
              tooFarFromTarget = true;
            }
          }
        }
      }
    }

    m_rayLengthStats->AddData(step);

    if (crossedPositions.count() && elevations.count()) {
      crossedPositions.removeLast();
      elevations.removeLast();
    }

    if (m_allowInterpolatedOptimizations) {
      QMutableListIterator< QPair<int, int> > crossedPositionsIt(crossedPositions);
      QMutableListIterator< double > elevationsIt(elevations);

      while (crossedPositionsIt.hasNext() && elevationsIt.hasNext()) {
        QPair<int, int> crossedPosition = crossedPositionsIt.next();
        double elevation = elevationsIt.next();

        if (crossedPositionsIt.hasNext() && elevationsIt.hasNext()) {
          QPair<int, int> nextPosition = crossedPositionsIt.peekNext();
          double nextElevation = elevationsIt.peekNext();

          double deltaSample = nextPosition.first - crossedPosition.first;
          double deltaLine = nextPosition.second - crossedPosition.second;

          double distanceBetweenPositions = sqrt(deltaSample * deltaSample +
                                                 deltaLine * deltaLine);

          // Is there a gap in the ray step that needs 'filled' in?
          if (distanceBetweenPositions >= 2) {
            double desiredDeltaSample = deltaSample / distanceBetweenPositions;
            double desiredDeltaLine = deltaLine / distanceBetweenPositions;

            QPair<int, int> desiredCrossedPosition = qMakePair(
                qRound(crossedPosition.first + desiredDeltaSample),
                qRound(crossedPosition.second + desiredDeltaLine));

            if (desiredCrossedPosition != nextPosition) {
              double deltaElevation = nextElevation - elevation;
              double interpolatedElevation = elevation +
                  (deltaElevation / distanceBetweenPositions);

              crossedPositionsIt.insert(desiredCrossedPosition);
              elevationsIt.insert(interpolatedElevation);

              // The insert put us after the newly inserted items, but we want to re-evaluate with
              //   them considered. Back us off by one.
              crossedPositionsIt.previous();
              elevationsIt.previous();
            }
          }
        }

      }
    }

    // Remember crossed shadows
    if (shadowed && m_shadowedPositions) {
      QPair<int, int> crossedPosition;
      foreach (crossedPosition, crossedPositions) {
        (*m_shadowedPositions)[crossedPosition] = true;
      }
    }

    // Remember lighted elevations
    if (!shadowed && m_lightedElevations) {
      double adjustment = 0;

      if (m_adjustLightCurtainElevations &&
          minimumDistanceFromRayToApproachingGround != DBL_MAX &&
          minimumDistanceFromRayToApproachingGround > 0) {
        adjustment = minimumDistanceFromRayToApproachingGround;
      }

      for (int i = 0; i < elevations.count() && i < crossedPositions.count(); i++) {
        QPair<int, int> crossedPosition = crossedPositions[i];
        double elevation = elevations[i];

        double existingElevation = (*m_lightedElevations)[crossedPosition];

        // If the hash gave us a zero, then it didn't exist in the hash (a zero radius won't happen)
        if (existingElevation == 0.0) {
          (*m_lightedElevations)[crossedPosition] = elevation;
        }
        else {
          (*m_lightedElevations)[crossedPosition] = qMin(elevation - adjustment,
                                                         existingElevation);
        }
      }
    }

    return shadowed;
  }


  /**
   * Shrink the light curtain and shadowed position caches down to their respective target sizes.
   *   This uses a random-based approach to quickly get a good answer and prevent systematic errors,
   *   but because it's random-based this method could remove too many and too few items from the
   *   caches. However, after extensive time testing, this seems to work well.
   */
  void ShadowFunctor::shrinkCaches() const {
    if (m_shadowedPositions && m_shadowedPositions->count()) {
      double probabilityOfKeeping =
          (double)m_targetShadowedPositionsSize / (double)m_shadowedPositions->count();

      if (qFuzzyCompare(probabilityOfKeeping, 0.0)) {
        m_shadowedPositions->clear();
      }
      else if (probabilityOfKeeping < 0.99) {
        QMutableHashIterator< QPair<int, int>, bool > shadowedPositionsIterator(
            *m_shadowedPositions);
        while (shadowedPositionsIterator.hasNext()) {
          shadowedPositionsIterator.next();

          bool keepIt = (rand() % 1000 < qRound(probabilityOfKeeping * 1000.0));

          if (!keepIt) {
            shadowedPositionsIterator.remove();
          }
        }
      }
    }

    if (m_lightedElevations && m_lightedElevations->count()) {
      double probabilityOfKeeping =
          (double)m_targetLightedElevationsSize / (double)m_lightedElevations->count();

      if (qFuzzyCompare(probabilityOfKeeping, 0.0)) {
        m_lightedElevations->clear();
      }
      else if (probabilityOfKeeping < 0.99) {
        QMutableHashIterator< QPair<int, int>, double > lightedElevationsIterator(
            *m_lightedElevations);

        while (lightedElevationsIterator.hasNext()) {
          lightedElevationsIterator.next();

          bool keepIt = (rand() % 1000 < qRound(probabilityOfKeeping * 1000.0));

          if (!keepIt) {
            lightedElevationsIterator.remove();
          }
        }
      }
    }
  }
}
