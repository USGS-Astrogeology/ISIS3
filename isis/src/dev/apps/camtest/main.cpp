/** This is free and unencumbered software released into the public domain.

The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */

#include "Isis.h"
#include "ProcessByLine.h"
#include "Camera.h"
#include "SpecialPixel.h"
#include "Pvl.h"

using namespace std;
using namespace Isis;

// Globals and prototypes

//void doIt(Buffer &in, Buffer &out);

enum OutputType {
  Lat,
  Lon,
  Err,
  Samp,
  Line
};

/**
 * Functor for collecting camera statistics.
 *
 * @author 2016-11-16 Jesse Mapel
 * @internal
 *   @history 2016-11-16 Original Version.
 */
class CamTestFunctor {
public:
  CamTestFunctor() {};
  ~CamTestFunctor() {};
  void setCamera(Camera* cam);
  void setOutType(OutputType outType);
  void setResults(Statistics* results);
  void operator()(Buffer &in, Buffer &out) const;

private:
  Camera* m_cam;
  OutputType m_outType;
  Statistics* m_resultsStats;
};

void IsisMain() {
  UserInterface &ui = Application::GetUserInterface();

  ProcessByBrick p;

  // Open the input cube
  Cube *iCube = p.SetInputCube("FROM");
  Camera *cam = iCube->camera();
  p.SetOutputCube("TO");

  // Set to process by line
  p.SetBrickSize(iCube->sampleCount(), 1, 1);

  IString format = ui.GetString("FORMAT").toStdString();

  OutputType outFormat = Lat;
  if (format == "LAT") {
    outFormat = Lat;
  }
  else if (format == "LON") {
    outFormat = Lon;
  }
  else if (format == "ERR") {
    outFormat = Err;
  }
  else if (format == "SAMP") {
    outFormat = Samp;
  }
  else if (format == "LINE") {
    outFormat = Line;
  }

  // Create process functor
  CamTestFunctor func;
  func.setCamera(cam);
  func.setOutType(outFormat);
  Statistics resultsStats;
  func.setResults(&resultsStats);

  p.ProcessCube(func, false);

  // Collect results
  PvlGroup results = PvlGroup("CamTestResults");
  results += PvlKeyword("FailedConversionsToLatLong", Isis::toString(resultsStats.LrsPixels()));
  results += PvlKeyword("FailedConversionsToSampleLine", Isis::toString(resultsStats.HrsPixels()));
  results += PvlKeyword("SuccessfulConversions", Isis::toString(resultsStats.ValidPixels()));
  if (outFormat == Err) {
    results += PvlKeyword("Average", Isis::toString(resultsStats.Average()));
    results += PvlKeyword("StandardDeviation", Isis::toString(resultsStats.StandardDeviation()));
    results += PvlKeyword("Minimum", Isis::toString(resultsStats.Minimum()));
    results += PvlKeyword("Maximum", Isis::toString(resultsStats.Maximum()));
  }

  // Log output results
  Application::Log(results);

  p.EndProcess();
}

// Functor Definitions

void CamTestFunctor::setCamera(Camera* cam) {
  m_cam = cam;
}

void CamTestFunctor::setOutType(OutputType outType) {
  m_outType = outType;
}

void CamTestFunctor::setResults(Statistics* resultsStats) {
  m_resultsStats = resultsStats;
}

void CamTestFunctor::operator()(Buffer &in, Buffer &out) const {
  if (in.Line() == 1) {
    m_cam->SetBand(in.Band());
  }

  double line = in.Line();
  for (int samp = 0; samp < in.SampleDimension(); samp++) {
    double sample = in.Sample(samp);
    if (!m_cam->SetImage(sample, line)) {
      out[samp] = Lrs;
      continue;
    }

    if (m_outType == Lat) {
      out[samp] = m_cam->UniversalLatitude();
    }
    else if (m_outType == Lon) {
      out[samp] = m_cam->UniversalLongitude();
    }
    else {
      if (!m_cam->SetUniversalGround(m_cam->UniversalLatitude(), m_cam->UniversalLongitude())) {
        out[samp] = Hrs;
        continue;
      }


      if (m_outType == Samp) {
        out[samp] = m_cam->Sample();
      }
      else if (m_outType == Line) {
        out[samp] = m_cam->Line();
      }
      else {
        double deltaS = m_cam->Sample() - sample;
        double deltaL = m_cam->Line()   - line;
        out[samp] = sqrt(deltaS * deltaS + deltaL * deltaL);
      }

    }
  }

  m_resultsStats->AddData(out.DoubleBuffer(), out.size());
}
/* Old Processing Routine
// Line processing routine
void doIt(Buffer &in, Buffer &out) {
  if(in.Line() == 1) {
    cam->SetBand(in.Band());
  }

  double line = in.Line();
  for(int samp = 0; samp < in.SampleDimension(); samp++) {
    double sample = in.Sample(samp);
    if(!cam->SetImage(sample, line)) {
      out[samp] = Lrs;
      continue;
    }

    if(OutputFormat == Lat) {
      out[samp] = cam->UniversalLatitude();
    }
    else if(OutputFormat == Lon) {
      out[samp] = cam->UniversalLongitude();
    }
    else {
      if(!cam->SetUniversalGround(cam->UniversalLatitude(), cam->UniversalLongitude())) {
        out[samp] = Hrs;
        continue;
      }


      if(OutputFormat == Samp) {
        out[samp] = cam->Sample();
      }
      else if(OutputFormat == Line) {
        out[samp] = cam->Line();
      }
      else {
        double deltaS = cam->Sample() - sample;
        double deltaL = cam->Line()   - line;
        out[samp] = sqrt(deltaS * deltaS + deltaL * deltaL);
      }

    }
  }
}
*/
