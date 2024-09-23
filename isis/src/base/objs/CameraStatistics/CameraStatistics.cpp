/** This is free and unencumbered software released into the public domain.
The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */
#include "CameraStatistics.h"

#include "Camera.h"
#include "Cube.h"
#include "Distance.h"
#include "Progress.h"
#include "Statistics.h"

namespace Isis {


  /**
   * Constructs the Camera Statistics object from a Cube filename.  This
   * constructor will first open the cube corresponding to the "filename"
   * parameter, then gather statistics with the Cube's Camera.  Neither the
   * Cube nor its Camera is retained after statistics gathering has completed,
   * but the filename used to open the Cube will be output in the "User
   * Parameters" group of the "toPvl" method.  The invoker of this constructor
   * must also specify the sample and line increments to be used during
   * statistics gathering.
   *
   * @param filename String filename of the Cube whose Camera will be used
   * @param sinc Sample increment for gathering statistics
   * @param linc Line increment for gathering statistics
   */
  CameraStatistics::CameraStatistics(QString filename, int sinc, int linc) {
    Cube cube;
    cube.open(filename);
    Camera *cam = cube.camera();
    init(cam, sinc, linc, filename);
  }


  /**
   * Constructs the Camera Statistics object from an already-existent Camera
   * pointer.  Specifying sample and line increments of 1 will gather
   * statistics on the entire area encompassed by the Camera, but higher
   * numbers can be used to improve performance.  Using this
   * constructor--lacking a Cube filename--the "toPvl" method will not output
   * the Cube filename associated with the Camera.  If the user desires this
   * information, there is a constructor that will take the filename purely for
   * this purpose.
   *
   * @param cam Camera pointer upon which statistics will be gathered
   * @param sinc Sample increment for gathering statistics
   * @param linc Line increment for gathering statistics
   */
  CameraStatistics::CameraStatistics(Camera *cam, int sinc, int linc) {
    init(cam, sinc, linc, "");
  }


  /**
   * Constructs the Camera Statistics object from an already-existent Camera
   * pointer.  Specifying sample and line increments of 1 will gather
   * statistics on the entire area encompassed by the Camera, but higher
   * numbers can be used to improve performance.  The filename provided does
   * not serve a functional purpose during the statistics gathering process,
   * but will report the filename used to create the Camera instance in the
   * "UserParameters" section of the PVL output from the "toPvl" method.
   *
   * @param cam Camera pointer upon which statistics will be gathered
   * @param sinc Sample increment for gathering statistics
   * @param linc Line increment for gathering statistics
   * @param filename String filename of the Cube whose Camera is being used
   */
  CameraStatistics::CameraStatistics(Camera *cam, int sinc, int linc,
      QString filename) {
    init(cam, sinc, linc, filename);
  }


  /**
   * Initializes this collection of statistics by incrementing over sample/line
   * positions in the Camera and compiling various Camera values at those
   * locations into all the Statistics objects maintained as the persistent
   * state of the object.  Statistics can be added to these objects later using
   * the "addStats" method.
   *
   * @param cam Camera pointer upon which statistics will be gathered
   * @param sinc Sample increment for gathering statistics
   * @param linc Line increment for gathering statistics
   * @param filename String filename of the Cube whose Camera is being used
   */
  void CameraStatistics::init(Camera *cam, int sinc, int linc,
      QString filename) {

    m_filename = filename;
    m_sinc = sinc;
    m_linc = linc;

    m_latStat = new Statistics();
    m_lonStat = new Statistics();
    m_resStat = new Statistics();


    m_obliqueResStat = new Statistics();
    m_obliqueSampleResStat = new Statistics();
    m_obliqueLineResStat = new Statistics();


    m_sampleResStat = new Statistics();
    m_lineResStat = new Statistics();
    m_aspectRatioStat = new Statistics();
    m_phaseStat = new Statistics();
    m_emissionStat = new Statistics();
    m_incidenceStat = new Statistics();
    m_localSolarTimeStat = new Statistics();
    m_localRaduisStat = new Statistics();
    m_northAzimuthStat = new Statistics();

    int eband = cam->Bands();

    // If the camera is band independent then only run one band
    if (cam->IsBandIndependent()) eband = 1;

    int pTotal = eband * ((cam->Lines() - 2) / linc + 2);
    Progress progress;
    progress.SetMaximumSteps(pTotal);
    progress.CheckStatus();

    for (int band = 1; band <= eband; band++) {
      cam->SetBand(band);
      for (int line = 1; line < (int)cam->Lines(); line = line + linc) {
        for (int sample = 1; sample < cam->Samples(); sample = sample + sinc) {
          addStats(cam, sample, line);
        }

        // Set the sample value to the last sample and run buildstats
        int sample = cam->Samples();
        addStats(cam, sample, line);
        progress.CheckStatus();
      }

      // Set the line value to the last line and run on all samples (sample +
      // sinc)
      int line = cam->Lines();
      for (int sample = 1; sample < cam->Samples(); sample = sample + sinc) {
        addStats(cam, sample, line);
      }

      // Set last sample and run with last line
      int sample = cam->Samples();
      addStats(cam, sample, line);
      progress.CheckStatus();
    }
  }


  /**
   * Destroy this instance, deletes all the Statistics objects.
   */
  CameraStatistics::~CameraStatistics() {
    if (m_latStat != NULL) {
      delete m_latStat;
      m_latStat = NULL;
    }
    if (m_lonStat != NULL) {
      delete m_lonStat;
      m_lonStat = NULL;
    }
    if (m_resStat != NULL) {
      delete m_resStat;
      m_resStat = NULL;
    }


   if (m_obliqueLineResStat != NULL) {
     delete m_obliqueLineResStat;
     m_obliqueLineResStat = NULL;

   }

   if (m_obliqueSampleResStat != NULL) {
     delete m_obliqueSampleResStat;
     m_obliqueSampleResStat = NULL;

   }

   if (m_obliqueResStat != NULL) {
     delete m_obliqueResStat;
     m_obliqueResStat = NULL;
   }

    if (m_sampleResStat != NULL) {
      delete m_sampleResStat;
      m_sampleResStat = NULL;
    }
    if (m_lineResStat != NULL) {
      delete m_lineResStat;
      m_lineResStat = NULL;
    }
    if (m_aspectRatioStat != NULL) {
      delete m_aspectRatioStat;
      m_aspectRatioStat = NULL;
    }
    if (m_phaseStat != NULL) {
      delete m_phaseStat;
      m_phaseStat = NULL;
    }
    if (m_emissionStat != NULL) {
      delete m_emissionStat;
      m_emissionStat = NULL;
    }
    if (m_incidenceStat != NULL) {
      delete m_incidenceStat;
      m_incidenceStat = NULL;
    }
    if (m_localSolarTimeStat != NULL) {
      delete m_localSolarTimeStat;
      m_localSolarTimeStat = NULL;
    }
    if (m_localRaduisStat != NULL) {
      delete m_localRaduisStat;
      m_localRaduisStat = NULL;
    }
    if (m_northAzimuthStat != NULL) {
      delete m_northAzimuthStat;
      m_northAzimuthStat = NULL;
    }
  }


  /**
   * Add statistics data to Statistics objects if the Camera position given by
   * the provided line and sample is looking at the surface of the target.
   *
   * @param cam Camera pointer upon which statistics are being gathered
   * @param sample Sample of the image to gather Camera information on
   * @param line Line of the image to gather Camera information on
   */
  void CameraStatistics::addStats(Camera *cam, int &sample, int &line) {
    cam->SetImage(sample, line);
    if(cam->HasSurfaceIntersection()) {
      m_latStat->AddData(cam->UniversalLatitude());
      m_lonStat->AddData(cam->UniversalLongitude());


      m_obliqueResStat->AddData(cam->ObliquePixelResolution());
      m_obliqueSampleResStat->AddData(cam->ObliqueSampleResolution());
      m_obliqueLineResStat->AddData(cam->ObliqueLineResolution());



      m_resStat->AddData(cam->PixelResolution());
      m_sampleResStat->AddData(cam->SampleResolution());
      m_lineResStat->AddData(cam->LineResolution());
      m_phaseStat->AddData(cam->PhaseAngle());
      m_emissionStat->AddData(cam->EmissionAngle());
      m_incidenceStat->AddData(cam->IncidenceAngle());
      m_localSolarTimeStat->AddData(cam->LocalSolarTime());
      m_localRaduisStat->AddData(cam->LocalRadius().meters());
      // if IsValid
      m_northAzimuthStat->AddData(cam->NorthAzimuth());

      // if resolution not equal to -1.0
      double aspectRatio = cam->LineResolution() / cam->SampleResolution();
      m_aspectRatioStat->AddData(aspectRatio);
    }
  }


  /**
   * Takes a name, value, and optionally units and constructs a PVL Keyword.
   * If the value is determined to be a "special pixel", then the string NULL
   * will be used for the value.
   *
   * @param keyname Name of keyword to generate
   * @param value Value to write to keyword
   * @param unit Optional units for keywords
   *
   * @return PvlKeyword Keyword constructed from input parameters
   */
  PvlKeyword CameraStatistics::constructKeyword(QString keyname,
      double value, QString unit="") const {

    if(IsSpecial(value)) {
      return (PvlKeyword(keyname.toStdString(), "NULL"));
    }
    else {
      return (PvlKeyword(keyname.toStdString(), Isis::toString(value), unit.toStdString()));
    }
  }


  /**
   * Constructs a Pvl object from the values in the various statistics objects.
   * The general format will look as follows:
   *
   * @code
   *   Group = UserParameters
   *     Filename (not provided for constructor w/ Camera but not filename)
   *     Linc
   *     Sinc
   *   End_Group
   *   Group = Latitude
   *     LatitudeMinimum
   *     LatitudeMaximum
   *     LatitudeStandardDeviation
   *   End_Group
   *   Group = Longitude
   *     LongitudeMinimum
   *     LongitudeMaximum
   *     LongitudeStandardDeviation
   *   End_Group
   *   Group = SampleResolution
   *     SampleResolutionMinimum
   *     SampleResolutionMaximum
   *     SampleResolutionStandardDeviation
   *   End_Group
   *   Group = LineResolution
   *     LineResolutionMinimum
   *     LineResolutionMaximum
   *     LineResolutionStandardDeviation
   *   End_Group
   *   Group = Resolution
   *     ResolutionMinimum
   *     ResolutionMaximum
   *     ResolutionStandardDeviation
   *   End_Group
   *   Group = AspectRatio
   *     AspectRatioMinimum
   *     AspectRatioMaximum
   *     AspectRatioStandardDeviation
   *   End_Group
   *   Group = PhaseAngle
   *     PhaseMinimum
   *     PhaseMaximum
   *     PhaseStandardDeviation
   *   End_Group
   *   Group = EmissionAngle
   *     EmissionMinimum
   *     EmissionMaximum
   *     EmissionStandardDeviation
   *   End_Group
   *   Group = IncidenceAngle
   *     IncidenceMinimum
   *     IncidenceMaximum
   *     IncidenceStandardDeviation
   *   End_Group
   *   Group = LocalSolarTime
   *     LocalSolarTimeMinimum
   *     LocalSolarTimeMaximum
   *     LocalSolarTimeStandardDeviation
   *   End_Group
   *   Group = LocalRadius
   *     LocalRadiusMinimum
   *     LocalRadiusMaximum
   *     LocalRadiusStandardDeviation
   *   End_Group
   *   Group = NorthAzimuth
   *     NorthAzimuthMinimum
   *     NorthAzimuthMaximum
   *     NorthAzimuthStandardDeviation
   *   End_Group
   * @endcode
   *
   * @return Pvl PVL collection of all values for all statistics gathered
   */
  Pvl CameraStatistics::toPvl() const {
    // Set up the Pvl groups and get min, max, avg, and sd for each statstics
    // object
    PvlGroup pUser("UserParameters");
    if (m_filename != "") pUser += PvlKeyword("Filename", m_filename);
    pUser += PvlKeyword("Linc", toString(m_linc));
    pUser += PvlKeyword("Sinc", toString(m_sinc));

    PvlGroup pLat("Latitude");
    pLat += constructKeyword("LatitudeMinimum", m_latStat->Minimum());
    pLat += constructKeyword("LatitudeMaximum", m_latStat->Maximum());
    pLat += constructKeyword("LatitudeAverage", m_latStat->Average());
    pLat += constructKeyword("LatitudeStandardDeviation", m_latStat->StandardDeviation());

    PvlGroup pLon("Longitude");
    pLon += constructKeyword("LongitudeMinimum", m_lonStat->Minimum());
    pLon += constructKeyword("LongitudeMaximum", m_lonStat->Maximum());
    pLon += constructKeyword("LongitudeAverage", m_lonStat->Average());
    pLon += constructKeyword("LongitudeStandardDeviation", m_lonStat->StandardDeviation());

    PvlGroup pObliqueSampleRes("ObliqueSampleResolution");
    pObliqueSampleRes += constructKeyword("ObliqueSampleResolutionMinimum",
                                          m_obliqueSampleResStat->Minimum(), "meters/pixel");
    pObliqueSampleRes += constructKeyword("ObliqueSampleResolutionMaximum",
                                          m_obliqueSampleResStat->Maximum(),"meters/pixel");
    pObliqueSampleRes += constructKeyword("ObliqueSampleResolutionAverage",
                                          m_obliqueSampleResStat->Average(),"meters/pixel");
    pObliqueSampleRes += constructKeyword("ObliqueSampleResolutionStandardDeviation",
        m_obliqueSampleResStat->StandardDeviation(), "meters/pixel");

    PvlGroup pObliqueLineRes("ObliqueLineResolution");
    pObliqueLineRes += constructKeyword("ObliqueLineResolutionMinimum", m_obliqueLineResStat->Minimum(),
        "meters/pixel");
    pObliqueLineRes += constructKeyword("ObliqueLineResolutionMaximum", m_obliqueLineResStat->Maximum(),
        "meters/pixel");
    pObliqueLineRes += constructKeyword("ObliqueLineResolutionAverage", m_obliqueLineResStat->Average(),
        "meters/pixel");
    pObliqueLineRes += constructKeyword("ObliqueLineResolutionStandardDeviation",
        m_obliqueLineResStat->StandardDeviation(), "meters/pixel");

    PvlGroup pObliqueResolution("ObliqueResolution");
    pObliqueResolution += constructKeyword("ObliqueResolutionMinimum", m_obliqueResStat->Minimum(),
        "meters/pixel");
    pObliqueResolution += constructKeyword("ObliqueResolutionMaximum", m_obliqueResStat->Maximum(),
        "meters/pixel");
    pObliqueResolution += constructKeyword("ObliqueResolutionAverage", m_obliqueResStat->Average(),
        "meters/pixel");
    pObliqueResolution += constructKeyword("ObliqueResolutionStandardDeviation",
        m_obliqueResStat->StandardDeviation(), "meters/pixel");


    PvlGroup pSampleRes("SampleResolution");
    pSampleRes += constructKeyword("SampleResolutionMinimum", m_sampleResStat->Minimum(),
        "meters/pixel");
    pSampleRes += constructKeyword("SampleResolutionMaximum", m_sampleResStat->Maximum(),
        "meters/pixel");
    pSampleRes += constructKeyword("SampleResolutionAverage", m_sampleResStat->Average(),
        "meters/pixel");
    pSampleRes += constructKeyword("SampleResolutionStandardDeviation",
        m_sampleResStat->StandardDeviation(), "meters/pixel");

    PvlGroup pLineRes("LineResolution");
    pLineRes += constructKeyword("LineResolutionMinimum", m_lineResStat->Minimum(),
        "meters/pixel");
    pLineRes += constructKeyword("LineResolutionMaximum", m_lineResStat->Maximum(),
        "meters/pixel");
    pLineRes += constructKeyword("LineResolutionAverage", m_lineResStat->Average(),
        "meters/pixel");
    pLineRes += constructKeyword("LineResolutionStandardDeviation",
        m_lineResStat->StandardDeviation(), "meters/pixel");

    PvlGroup pResolution("Resolution");
    pResolution += constructKeyword("ResolutionMinimum", m_resStat->Minimum(),
        "meters/pixel");
    pResolution += constructKeyword("ResolutionMaximum", m_resStat->Maximum(),
        "meters/pixel");
    pResolution += constructKeyword("ResolutionAverage", m_resStat->Average(),
        "meters/pixel");
    pResolution += constructKeyword("ResolutionStandardDeviation",
        m_resStat->StandardDeviation(), "meters/pixel");

    PvlGroup pAspectRatio("AspectRatio");
    pAspectRatio += constructKeyword("AspectRatioMinimum", m_aspectRatioStat->Minimum());
    // Note: Maximum is spelled wrong here.
    pAspectRatio += constructKeyword("AspectRatioMaximun", m_aspectRatioStat->Maximum());
    pAspectRatio += constructKeyword("AspectRatioAverage", m_aspectRatioStat->Average());
    pAspectRatio += constructKeyword("AspectRatioStandardDeviation",
        m_aspectRatioStat->StandardDeviation());

    PvlGroup pPhase("PhaseAngle");
    pPhase += constructKeyword("PhaseMinimum", m_phaseStat->Minimum());
    pPhase += constructKeyword("PhaseMaximum", m_phaseStat->Maximum());
    pPhase += constructKeyword("PhaseAverage", m_phaseStat->Average());
    pPhase += constructKeyword("PhaseStandardDeviation", m_phaseStat->StandardDeviation());

    PvlGroup pEmission("EmissionAngle");
    pEmission += constructKeyword("EmissionMinimum", m_emissionStat->Minimum());
    pEmission += constructKeyword("EmissionMaximum", m_emissionStat->Maximum());
    pEmission += constructKeyword("EmissionAverage", m_emissionStat->Average());
    pEmission += constructKeyword("EmissionStandardDeviation",
        m_emissionStat->StandardDeviation());

    PvlGroup pIncidence("IncidenceAngle");
    pIncidence += constructKeyword("IncidenceMinimum", m_incidenceStat->Minimum());
    pIncidence += constructKeyword("IncidenceMaximum", m_incidenceStat->Maximum());
    pIncidence += constructKeyword("IncidenceAverage", m_incidenceStat->Average());
    pIncidence += constructKeyword("IncidenceStandardDeviation",
        m_incidenceStat->StandardDeviation());

    PvlGroup pTime("LocalSolarTime");
    pTime += constructKeyword("LocalSolarTimeMinimum", m_localSolarTimeStat->Minimum(),
        "hours");
    pTime += constructKeyword("LocalSolarTimeMaximum", m_localSolarTimeStat->Maximum(),
        "hours");
    pTime += constructKeyword("LocalSolarTimeAverage", m_localSolarTimeStat->Average(),
        "hours");
    pTime += constructKeyword("LocalSolarTimeStandardDeviation",
        m_localSolarTimeStat->StandardDeviation(), "hours");

    PvlGroup pLocalRadius("LocalRadius");
    pLocalRadius += constructKeyword("LocalRadiusMinimum", m_localRaduisStat->Minimum());
    pLocalRadius += constructKeyword("LocalRadiusMaximum", m_localRaduisStat->Maximum());
    pLocalRadius += constructKeyword("LocalRadiusAverage", m_localRaduisStat->Average());
    pLocalRadius += constructKeyword("LocalRadiusStandardDeviation",
        m_localRaduisStat->StandardDeviation());

    PvlGroup pNorthAzimuth("NorthAzimuth");
    pNorthAzimuth += constructKeyword("NorthAzimuthMinimum", m_northAzimuthStat->Minimum());
    pNorthAzimuth += constructKeyword("NorthAzimuthMaximum", m_northAzimuthStat->Maximum());
    pNorthAzimuth += constructKeyword("NorthAzimuthAverage", m_northAzimuthStat->Average());
    pNorthAzimuth += constructKeyword("NorthAzimuthStandardDeviation",
        m_northAzimuthStat->StandardDeviation());

    Pvl returnPvl;
    returnPvl.setTerminator("");
    returnPvl.addGroup(pUser);
    returnPvl.addGroup(pLat);
    returnPvl.addGroup(pLon);
    returnPvl.addGroup(pSampleRes);
    returnPvl.addGroup(pLineRes);
    returnPvl.addGroup(pResolution);

    returnPvl.addGroup(pObliqueSampleRes);
    returnPvl.addGroup(pObliqueLineRes);
    returnPvl.addGroup(pObliqueResolution);

    returnPvl.addGroup(pAspectRatio);
    returnPvl.addGroup(pPhase);
    returnPvl.addGroup(pEmission);
    returnPvl.addGroup(pIncidence);
    returnPvl.addGroup(pTime);
    returnPvl.addGroup(pLocalRadius);
    returnPvl.addGroup(pNorthAzimuth);
    return returnPvl;
  }
}
