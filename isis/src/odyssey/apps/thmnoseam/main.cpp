/** This is free and unencumbered software released into the public domain.

The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */

#include "Isis.h"

#include <QString>

#include <QList>
#include <QPair>

#include "Chip.h"
#include "ThemisVisCamera.h"
#include "SpecialPixel.h"
#include "ProcessByLine.h"
#include "CameraFactory.h"
#include "PushFrameCameraGroundMap.h"

using namespace Isis;
using namespace std;

//! This is the method called by ProcessByBrick.
void FixSeams(vector<Buffer *> &inBuffers, vector<Buffer *> &outBuffers);

//! This function calculates about how much the framelets overlap
int FrameletOverlapSize();
int frameletSize; //!< This is the number of lines in a framelet
Cube *evenCube; //!< Input even framelet cube
Cube *oddCube; //!< Input odd framelet cube
Cube *outEven; //!< Output even framelet cube
Cube *outOdd; //!< Output odd framelet cube
int overlapSize; //!< This stores the result of FrameletOverlapSize()

// sample,line -> DN
QList< QPair< QPair<int, int>, double> > nextFrameletFixes;

/**
 *
 * This class is used to remember a translation between the bottom of one
 *   framelet and the top of the next. These stay fairly constant for one band.
 *
 * @author ????-??-?? Unknown
 *
 * @internal
 */
class Offset {
  public:
    Offset() {
      p_valid = false;
    }

    Offset(int sample, int frameletLine, int sampleOffset, int lineOffset) {
      p_valid = true;
      p_sample = sample;
      p_frameletLine = frameletLine;
      p_sampleOffset = sampleOffset;
      p_lineOffset = lineOffset;
    }

    //! Offset should be used
    bool Valid() { return p_valid; }
    //! Used to verify we're at the correct sample
    int Sample() { return p_sample; }
    //! Used to verify we're at the correct line
    int FrameletLine() { return p_frameletLine; }
    //! Translation between current sample and next framelet's sample
    int SampleOffset() { return p_sampleOffset; }
    //! Translation between current line and next framelet's line
    int LineOffset() { return p_lineOffset; }

  private:
    bool p_valid;
    int p_sample;
    int p_frameletLine;
    int p_sampleOffset;
    int p_lineOffset;
};

//! This is a list of all translations for a band between the bottom of one
//!   framelet and the top of the next.
QList< Offset > frameletOffsetsForBand;

void IsisMain() {
  // Grab the file to import
  ProcessByBrick p;
  evenCube = p.SetInputCube("INEVEN");
  oddCube = p.SetInputCube("INODD");
  outEven = p.SetOutputCube("OUTEVEN");
  outOdd = p.SetOutputCube("OUTODD");

  UserInterface &ui = Application::GetUserInterface();
  // Make sure it is a Themis EDR/RDR
  try {
    if(evenCube->group("Instrument")["InstrumentID"][0] != "THEMIS_VIS") {
      std::string msg = "This program is intended for use on THEMIS VIS images only";
      msg += " [" + ui.GetCubeName("INEVEN").toStdString() + "] does not appear to be a ";
      msg += "THEMIS VIS image.";
      throw IException(IException::User, msg, _FILEINFO_);
    }
    if (evenCube->group("Instrument")["Framelets"][0] != "Even") {
      std::string msg = "The image [" + ui.GetCubeName("INEVEN").toStdString() + "] does not appear "
          "to contain the EVEN framelets of a Themis VIS cube";
      throw IException(IException::User, msg, _FILEINFO_);
    }
  }
  catch(IException &e) {
    throw IException(e, IException::User,
                     "Unable to run thmnoseam with the given even input cube.", _FILEINFO_);
  }

  try {
    if(oddCube->group("Instrument")["InstrumentID"][0] != "THEMIS_VIS") {
      std::string msg = "This program is intended for use on THEMIS VIS images only";
      msg += " [" + ui.GetCubeName("INODD").toStdString() + "] does not appear to be a ";
      msg += "THEMIS VIS image.";
      throw IException(IException::User, msg, _FILEINFO_);
    }
    if (oddCube->group("Instrument")["Framelets"][0] != "Odd") {
      std::string msg = "The image [" + ui.GetCubeName("INODD").toStdString() + "] does not appear "
          "to contain the ODD framelets of a Themis VIS cube";
      throw IException(IException::User, msg, _FILEINFO_);
    }
  }
  catch(IException &e) {
    throw IException(e, IException::User,
                     "Unable to run thmnoseam with the given odd input cube.", _FILEINFO_);
  }


  PvlGroup &inputInstrumentGrp = evenCube->group("Instrument");
  PvlKeyword &spatialSumming = inputInstrumentGrp["SpatialSumming"];
  frameletSize = 192 / IString::ToInteger(spatialSumming[0]);
  overlapSize = FrameletOverlapSize();

  if(overlapSize == 0) {
    IString msg = "There must be overlap to remove seams";
    throw IException(IException::Unknown, msg, _FILEINFO_);
  }

  p.SetBrickSize(evenCube->sampleCount(), frameletSize, 1);

  p.StartProcess(FixSeams);

  PvlGroup &evenInst = outEven->group("Instrument");
  evenInst["Framelets"] = "Even";

  PvlGroup &oddInst = outOdd->group("Instrument");
  oddInst["Framelets"] = "Odd";

  p.EndProcess();
}

//! This function corrects the DNs in a given brick.
//! It also stores results in frameletOffsetsForBand every time the
//! band changes so it doesn't have to re-project at every framelet.
//! Equivalent changes are calculated for the next framelet... that is,
//! equivalent pixels. This function both calculates and applies these.
void RemoveSeam(Buffer &out, int framelet, int band,
                bool matchIsEven) {
  // Apply fixes from last pass. Basically all changes happen in two
  //   places, because the DNs exist in two cubes, this is the second
  //   place.
  for(int fix = 0; fix < nextFrameletFixes.size(); fix ++) {
    QPair<int, int> fixLoc = nextFrameletFixes[fix].first;
    double fixDn = nextFrameletFixes[fix].second;

    try {
      int outIndex = out.Index(fixLoc.first, fixLoc.second, band);
      out[outIndex] = fixDn;
    }
    catch(IException &) {
    }
  }

  nextFrameletFixes.clear();

  // Match == goodData. "goodData" is the top of the next framelet.
  Cube *goodDataCube = (matchIsEven) ? evenCube : oddCube;
  // "badData" is the bottom of the current framelet, what we were given.
  Cube *badDataCube  = (matchIsEven) ? oddCube  : evenCube;

  Camera *goodCam = goodDataCube->camera();
  Camera *badCam  = badDataCube->camera();

  // Verify we're at the correct band
  goodCam->SetBand(band);
  badCam->SetBand(band);

  // Absolute line number for top of framelets.
  int goodDataStart = frameletSize * (framelet + 1);
  int badDataStart = frameletSize * framelet;

  // Start corrections to the current brick at this line
  int badLineStart = goodDataStart - overlapSize - 1;
  // End corrections to the current brick at this line
  int badLineEnd = goodDataStart - 1;

  int offsetSample = 0;
  int offsetLine = 0;

  // Loop left to right, top to bottom of problematic area at bottom of framelet
  for(int badLine = badLineStart; badLine <= badLineEnd; badLine ++) {
    for(int sample = 1; sample <= out.SampleDimension(); sample ++) {
      // A fair good data weight is the % across problematic area so fair
      double goodDataWeight = (double)(badLine - badLineStart) /
                             (double)(badLineEnd - badLineStart);

      // But good data is good, so let's bias it towards the good data
      goodDataWeight *= 2;
      if(goodDataWeight > 1) goodDataWeight = 1;

      // Bad data weight is the inverse of the good data's weight.
      double badDataWeight = 1.0 - goodDataWeight;

      int outIndex = out.Index(sample, badLine, band);
      // This is the indexing scheme for frameletOffsetsForBand
      int optimizeIndex = (badLine - badLineStart) * out.SampleDimension() +
                          sample - 1;

      // Does the optimized (pre-calculated) translation from bad to good
      //  exist?
      if(optimizeIndex < frameletOffsetsForBand.size()) {
        // This offset any good? If not then do nothing.
        if(!frameletOffsetsForBand[optimizeIndex].Valid())
          continue;

        // Use optimization!
        offsetSample = frameletOffsetsForBand[optimizeIndex].SampleOffset();
        offsetLine = frameletOffsetsForBand[optimizeIndex].LineOffset();
      }
      // There is no pre-calculated translation, calculate it
      else if(badCam->SetImage(sample, badLine)) {
        double lat = badCam->UniversalLatitude();
        double lon = badCam->UniversalLongitude();

        if(goodCam->SetUniversalGround(lat, lon)) {
          double goodSample = goodCam->Sample();
          double goodLine = goodCam->Line();

          // Set the current offset for correction
          offsetSample = (int)(goodSample - sample + 0.5);
          offsetLine = (int)(goodLine - badLine + 0.5);

          // Remember this calculation for future passes
          frameletOffsetsForBand.push_back(Offset(sample, badLine - badDataStart,
                                                  offsetSample, offsetLine));
        }
        else {
          // Don't do anything since we failed at this pixel; it will be copied
          //   from the input directly
          frameletOffsetsForBand.push_back(Offset());
          continue;
        }
      }

      // Translate current sample,line (bad) to (good) data's sample,line
      double goodSample = offsetSample + sample;
      double goodLine = offsetLine + badLine;

      // Get the pixel we're missing (good)
      Portal p(1, 1, goodDataCube->pixelType());
      p.SetPosition(goodSample, goodLine, band);
      goodDataCube->read(p);

      // Attempt to apply weighted average
      if(!Isis::IsSpecial(p[0]) && !Isis::IsSpecial(out[outIndex])) {
        out[outIndex] = p[0] * goodDataWeight + out[outIndex] * badDataWeight;
      }
      else if(!Isis::IsSpecial(p[0])) {
        out[outIndex] = p[0];
      }

      // Apply change to next framelet also
      QPair<int, int> fixLoc((int)(goodSample + 0.5),
                             (int)(goodLine   + 0.5));
      QPair< QPair<int, int>, double > fix(fixLoc, out[outIndex]);
      nextFrameletFixes.push_back(fix);
    }
  }
}


/**
 * This is the main loop over the cube data. Statistics are used to
 *   calculate which brick actually contains DNs. The framelet with DNs
 *   is corrected by RemoveSeam and this also clears remembered offsets
 *   (used for speed optimization) when the band changes.
 */
void FixSeams(vector<Buffer *> &inBuffers, vector<Buffer *> &outBuffers) {
  Buffer &evenBuffer    = *inBuffers[0];
  Buffer &oddBuffer     = *inBuffers[1];

  Buffer &outEvenBuffer = *outBuffers[0];
  Buffer &outOddBuffer  = *outBuffers[1];

  outEvenBuffer.Copy(evenBuffer);
  outOddBuffer.Copy(oddBuffer);

  Statistics evenStats;
  evenStats.AddData(evenBuffer.DoubleBuffer(), evenBuffer.size());

  Statistics oddStats;
  oddStats.AddData(oddBuffer.DoubleBuffer(), oddBuffer.size());

  int framelet = (evenBuffer.Line() - 1) / frameletSize;

  if(framelet == 0) {
    frameletOffsetsForBand.clear();
  }

  if(evenStats.ValidPixels() > oddStats.ValidPixels()) {
    RemoveSeam(outEvenBuffer, framelet, evenBuffer.Band(), false);
  }
  else {
    RemoveSeam(outOddBuffer, framelet, oddBuffer.Band(), true);
  }
}


/**
 * This calculates the number of lines of overlap between framelets.
 */
int FrameletOverlapSize() {
  Camera *camEven = evenCube->camera();
  Camera *camOdd = oddCube->camera();

  if(camEven == NULL || camOdd == NULL) {
    std::string msg = "A camera is required to automatically calculate the overlap "
                 "between framelets. Please run spiceinit on the input cube";
    throw IException(IException::Unknown, msg, _FILEINFO_);
  }

  int frameletOverlap = 0;

  // Framelet 2 is even, so let's use the even camera to find the lat,lon at it's beginning
  if(camEven->SetImage(1, frameletSize + 1)) {
    double framelet2StartLat = camEven->UniversalLatitude();
    double framelet2StartLon = camEven->UniversalLongitude();

    // Let's figure out where this is in the nearest odd framelet (hopefully framelet 1)
    if(camOdd->SetUniversalGround(framelet2StartLat, framelet2StartLon)) {
      // The equivalent line to the start of framelet 2 is this found line
      int equivalentLine = (int)(camOdd->Line() + 0.5);
      frameletOverlap = frameletSize - equivalentLine;
      if(frameletOverlap < 0) frameletOverlap = 0;
    }
  }

  return frameletOverlap;
}
