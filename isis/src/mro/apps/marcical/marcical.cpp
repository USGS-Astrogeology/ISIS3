/** This is free and unencumbered software released into the public domain.

The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */

#include "SpecialPixel.h"
#include "Blob.h"
#include "CubeAttribute.h"
#include "Cube.h"
#include "LineManager.h"
#include "Progress.h"
#include "Camera.h"
#include "Constants.h"
#include "Preference.h"
#include "Statistics.h"
#include "TextFile.h"
#include "Stretch.h"
#include "iTime.h"
#include "CSVReader.h"
#include "FileName.h"
#include "IString.h"

#include <QProcess>
#include <QDebug>
#include <QRegExp>
#include <string>
#include <vector>
#include <cmath>

#include "marcical.h"

using namespace std;

/*

PDS_VERSION_ID      = PDS3
LABEL_REVISION_NOTE = "2007-05-16, K. Supulver;
                       2007-11-07, K. Supulver;
                       2009-07-13, K. Supulver"
RECORD_TYPE         = STREAM

OBJECT              = TEXT
 PUBLICATION_DATE   = 2007-05-25
 NOTE               = "Description of MARCI calibration process"
END_OBJECT          = TEXT
END


The following explanation of the MARCI calibration process assumes
0-relative indexing; e.g. the first pixel of any given line has
index 0.  File lines and dataset lines are also numbered starting
with 0.

There are eight input ancillary data files required for this process:
the decompanding table (marcidec.txt) and the per-band flattening
tables (vis1flat.ddd - vis5flat.ddd, uv6flat.ddd - uv7flat.ddd).
The decompanding table is a simple ASCII file; the flattening tables
are big-endian binary files.

Note:  All seven per-band flattening tables were updated starting
with volume MROM_0033.

0) Read the decompanding table file, marcidec.txt.  marcidec.txt
contains 256 lines, one line for each possible byte value [0, 255].
There is one value per line, namely the decompanded value
corresponding to the line number.

1) Read the flattening table file corresponding to the given MARCI
band.  The visible band flattening tables contain 16 1024-byte rows,
preceded by a 1024-byte header.  The UV flattening tables contain
2 128-floating point rows, preceded by a 1024-byte header.

The 1024-byte header has the following structure (big-endian):

0-rel byte offset   value
         0          32-bit integer magic number
         4          32-bit integer number of image lines
         8          32-bit integer number of bytes per image line
        12          32-bit integer number of bits per image elements
        16          32-bit integer currently unused
        20          32-bit integer currently unused
        24          ASCII label up to 1000 characters long
                    The label is NUL-terminated

The first word of the label contains the normalization factor for the
flattening table.  For UV flats, this factor is 1.; for VIS flats, we
currently have
                    201.66 norm band 1
                    211.30 norm band 2
                    202.42 norm band 3
                    198.74 norm band 4
                    208.89 norm band 5

2) Align the flattening table with the given MARCI band frame.  This
is only necessary when the visible band summing is not one; in this
case, the flattening table has to be averaged down so it has the same
width and height as the MARCI band frame.  For example, if the visible
band summing is 2, then the new flat has 8 512-element rows:

    new flat[i][j] = (flat[2i  ,2j] + flat[2i  ,2j+1]
                      flat[2i+1,2j] + flat[2i+1,2j+1])/4

3) Convert the flattening table to a numerator flat, e.g.,

    flat[i][j] = (flat[i][j] < 0.25) ? 0. : 1/flat[i][j]

4) For every frame for a given MARCI band, decompand the data and
apply the numerator flat:

    flattened value = raw decompanded value * numerator flat value

5) Convert the flattened data to radiance I/F using the following
formula:

   I = DN / Exposure Time / Summing / Response Coefficient

   F = Solar Irradiance / Pi / Solar Distance^2

   where DN is the flattened value

   Exposure Time is in milliseconds

   Solar Distance is in Astronomical Units (AU)

   # band   coeff     rms   solar_irradiance(1 AU)
     1      0.793     0.014    1798.4
     2      1.124     0.009    1875.7
     3      0.751     0.005    1742.7
     4      0.882     0.006    1580.7
     5      0.777     0.007    1360.3
     6      0.0101    0.00026  132.08
     7      0.0250    0.0004   755.64

   For UV band 7, summing should be multiplied by
   (1 - decimation_factor) when calculating I.
   decimation_factor = 0 for images acquired before
   2006-11-06T21:30:00 SCET and 0.75 for images acquired
   after that time.

   Note:  The UV coefficients (bands 6 and 7) were updated
   starting with volume MROM_0033.

   Starting with volume MROM_0049, the exposure time may have
   varied over the course of a single MARCI image.  See
   varexp.lbl in the index subdirectory of MROM_0049 and
   subsequent volumes for details.

*/

namespace Isis {

  Stretch stretch;

  void marcical(UserInterface &ui) {
    CubeAttributeInput inAtt = ui.GetInputAttribute("FROM");
    Cube icube;

    if (inAtt.bands().size() != 0) {
      icube.setVirtualBands(inAtt.bands());
    }

    icube.open(QString::fromStdString(FileName(ui.GetCubeName("FROM").toStdString()).expanded()));

    // Make sure it is a Marci cube
    try {
      if (icube.group("Instrument")["InstrumentID"][0] != "Marci") {
        throw IException();
      }

      if (!icube.group("Archive").hasKeyword("SampleBitModeId")) {
        throw IException();
      }
    }
    catch (IException &) {
      FileName inFileName = ui.GetCubeName("FROM").toStdString();
      std::string msg = "This program is intended for use on MARCI images only. [";
      msg += inFileName.expanded() + "] does not appear to be a MARCI image.";
      throw IException(IException::User, msg, _FILEINFO_);
    }

    if (icube.group("Archive")["SampleBitModeId"][0] != "SQROOT") {
      std::string msg = "Sample bit mode [" + icube.group("Archive")["SampleBitModeId"][0] + "] is not supported.";
      throw IException(IException::User, msg, _FILEINFO_);
    }

    // Read in calibration coefficients
    FileName calFile =
        FileName("$mro/calibration/marci/marciCoefficients_v???.pvl").highestVersion();

    vector<double> decimation;

    // Decimation is described in the MRO MARCI Instrument and Calibration document pg. 63
    for (int i = 0; i < 6; i++) {
      // Decimation is 1.0 for bands 1-6
      decimation.push_back(1.0);
    }

    QString startTime = QString::fromStdString(icube.label()->findGroup("Instrument", Pvl::Traverse)["StartTime"][0]);
    iTime start(startTime);
    iTime changeTime("November 6, 2006 21:30:00 UTC");

    if (start < changeTime) {
      decimation.push_back(1.0);
    }
    else {
      decimation.push_back(0.25);
    }

    // Get the LUT data
    FileName temp = FileName("$mro/calibration/marcisqroot_???.lut").highestVersion();
    TextFile stretchPairs(QString::fromStdString(temp.expanded()));

    // Create the stretch pairs
    stretch.ClearPairs();
    for (int i = 0; i < stretchPairs.LineCount(); i++) {
      QString line;
      stretchPairs.GetLine(line, true);
      int temp1 = line.split(" ").first().toInt();
      int temp2 = line.split(" ").last().toInt();
      stretch.AddPair(temp1, temp2);
    }

    stretchPairs.Close();

    // This file stores radiance/spectral distance coefficients
    Pvl calibrationData(calFile.expanded());

    // This will store the radiance coefficient and solar spectral distance coefficients
    // for each band.
    //   calibrationCoeffs[band].first gives the radiance coefficient
    //   calibrationCoeffs[band].second gives the spectral distance
    vector< pair<double, double> > calibrationCoeffs;

    // Check our coefficient file
    if (calibrationData.objects() != 7) {
      std::string msg = "Calibration file [" + calFile.expanded() + "] must contain data for 7 filters in ascending order;";
      msg += " only [" + toString(calibrationData.objects()) + "] objects were found";
      throw IException(IException::Programmer, msg, _FILEINFO_);
    }

    // Read it, make sure it's ordered
    for (int obj = 0; obj < calibrationData.objects(); obj ++) {
      PvlObject calObj = calibrationData.object(obj);

      if ((int)calObj["FilterNumber"] != obj + 1) {
        std::string msg = "Calibration file [" + calFile.expanded() + "] must have the filters in ascending order";
        throw IException(IException::Programmer, msg, _FILEINFO_);
      }

      pair<double, double> calData(calObj["RadianceCoefficient"], calObj["SolarSpectralDistance"]);
      calibrationCoeffs.push_back(calData);
    }

    vector<Cube *> flatcubes;
    vector<LineManager *> fcubeMgrs;
    int summing = IString::ToInteger(icube.group("Instrument")["SummingMode"][0]);
    double ifdelay = IString::ToDouble(icube.group("Instrument")["InterframeDelay"][0]) * 1000.0;
    int flipped = IString::ToInteger(icube.group("Instrument")["DataFlipped"][0]);

    // Read in the flat files
    for (int band = 0; band < 7; band++) {
      QString filePattern = "$mro/calibration/marci/";

      if (band < 5) {
        filePattern += "vis";
      }
      else {
        filePattern += "uv";
      }

      // UV cubes are always summing mode = 8, we can assume this rule will never
      // be broken
      if (band >= 5 && summing != 8) {
        continue;
      }

      filePattern += "flat_band" + QString::number(band + 1);
      filePattern += "_summing" + QString::number(summing) + "_v???.cub";

      FileName flatFile = FileName(filePattern.toStdString()).highestVersion();
      Cube *fcube = new Cube();
      fcube->open(QString::fromStdString(flatFile.expanded()));
      flatcubes.push_back(fcube);

      LineManager * fcubeMgr = new LineManager(*fcube);
      fcubeMgr->SetLine(1, 1);
      fcubeMgrs.push_back(fcubeMgr);
    }

    // Prepare the output cube
    Cube ocube;

    CubeAttributeOutput outAtt = ui.GetOutputAttribute("TO");
    ocube.setDimensions(icube.sampleCount(), icube.lineCount(), icube.bandCount());
    ocube.setByteOrder(outAtt.byteOrder());
    ocube.setFormat(outAtt.fileFormat());
    ocube.setLabelsAttached(outAtt.labelAttachment() == AttachedLabel);
    ocube.setPixelType(outAtt.pixelType());

    ocube.create(QString::fromStdString(FileName(ui.GetCubeName("TO").toStdString()).expanded()));

    LineManager icubeMgr(icube);

    // This will store a direct translation from band to filter index
    vector<int> filter;

    // Conversion from filter name to filter index
    map<QString, int> filterNameToFilterNumber;
    filterNameToFilterNumber.insert(pair<QString, int>("BLUE",     1));
    filterNameToFilterNumber.insert(pair<QString, int>("GREEN",    2));
    filterNameToFilterNumber.insert(pair<QString, int>("ORANGE",   3));
    filterNameToFilterNumber.insert(pair<QString, int>("RED",      4));
    filterNameToFilterNumber.insert(pair<QString, int>("NIR",      5));
    filterNameToFilterNumber.insert(pair<QString, int>("SHORT_UV", 6));
    filterNameToFilterNumber.insert(pair<QString, int>("LONG_UV",  7));

    PvlKeyword filtNames = icube.label()->findGroup("BandBin", Pvl::Traverse)["FilterName"];
    int numFilters = filtNames.size();
    for (int i = 0; i < filtNames.size(); i++) {
      if (filterNameToFilterNumber.find(QString::fromStdString(filtNames[i])) != filterNameToFilterNumber.end()) {
        filter.push_back(filterNameToFilterNumber.find(QString::fromStdString(filtNames[i]))->second);
      }
      else {
        std::string msg = "Unrecognized filter name [" + filtNames[i] + "]";
        throw IException(IException::Programmer, msg, _FILEINFO_);
      }
    }

    PvlKeyword &sumMode = icube.label()->findGroup("Instrument", Pvl::Traverse)["SummingMode"];
    int summingMode = IString::ToInteger(sumMode[0]);
    int filterHeight = 16 / summingMode;
    std::vector<int> padding;
    padding.resize(numFilters);
    PvlKeyword &colOff = icube.label()->findGroup("Instrument", Pvl::Traverse)["ColorOffset"];
    int colorOffset = IString::ToInteger(colOff[0]);

    for (int i = 0; i < numFilters; i++) {
      if (colorOffset > 0) {
        int filtNum = filterNameToFilterNumber[QString::fromStdString(filtNames[i])] - 1; // if first vis filter (Blue) there is no offset 1 -> 0

        padding[i] = (colorOffset * filterHeight) * filtNum;
      }
      else {
        padding[i] = 0;
      }
    }

    int maxOffset = *max_element(padding.begin(),padding.end());


    QString prodId = QString::fromStdString(icube.label()->findGroup("Archive", Pvl::Traverse)["ProductId"][0]);
    prodId = prodId.toUpper();
    vector<int> frameseq;
    vector<double> exptime;

    // Get the exposure duration(s) and coorisponding frame number(s) (zero based) from the label
    PvlGroup inst = icube.group("Instrument");

    if (!inst.hasKeyword("VariableExposureDuration") || !inst.hasKeyword("FrameNumber")) {
      std::string msg = "The instrument keywords VariableExposureDuration and FrameNumber"
                    "must exist to calibrate this MARCI file. Prior to isis3.10.0 these"
                    "keywords were not added by marci2isis; you may need to rerun isis3.10+"
                    "marci2isis on your images.";
      throw IException(IException::User, msg, _FILEINFO_);
    }

    // The previous version of marcical read the first exposure duration and frame number from
    // the label and the other durations/frames (if any) from the mission's varexp.tab file.
    // In order to minimize the changes to this code while modifying it to use the keywords instead of
    // reading the varexp.tab file directly, the duration/frame where the frame is zero are dropped from
    // the array when converting them from the keywords. This calibration code should be
    // able to be simplified significantly by not removing the frameNumber=0 exposure/frame and
    // removing the first frame ifs in the calibration code below.
    PvlKeyword expTimesKey = inst["VariableExposureDuration"];
    PvlKeyword frameNumbersKey = inst["FrameNumber"];
    for (int i=0; i<expTimesKey.size(); i++) {
      if (IString::ToInteger(frameNumbersKey[i]) != 0) {
        exptime.push_back(IString::ToDouble(expTimesKey[i]));
        frameseq.push_back(IString::ToInteger(frameNumbersKey[i]));
      }
    }

    bool iof = ui.GetBoolean("IOF");
    double exposure = ((double)icube.label()->findGroup("Instrument", Pvl::Traverse)["ExposureDuration"]) * 1000.0;
    Camera *cam = NULL;
    double solarDist = Isis::Null;

    if (iof) {
      cam = icube.camera();
      cam->SetImage(icubeMgr.size() / 2.0, 0.5 + (16 / 2) / summing);
      solarDist = cam->SolarDistance();
    }

    LineManager ocubeMgr(ocube);
    ocubeMgr.SetLine(1, 1);

    Progress prog;
    prog.SetText("Calibrating Image");
    prog.SetMaximumSteps(ocube.lineCount() * ocube.bandCount());
    prog.CheckStatus();

    Statistics stats;

    int band = 0;
    int frame = 0;
    int line = 0;
    int seqno = 0;
    int fsize = frameseq.size();
    do {
      icube.read(icubeMgr);
      ocube.read(ocubeMgr);

      int fcubeIndex = filter[ocubeMgr.Band()-1] - 1;
      // First time through the loop or the Mgr finished with one band and has incremented to the next
      if (band != ocubeMgr.Band()) {
        band = ocubeMgr.Band();
        line = 0;
        if (!flipped) {
          // Question: for the first time thru the do loop maxOffset is set above to the max of padding, but
          // for the first framelet of the second band maxOffset seems to be left over from the previous itteration
          // Seems like something might be wrong here.
          frame = 0;
          exposure = ((double)icube.label()->findGroup("Instrument", Pvl::Traverse)["ExposureDuration"]) * 1000.0;
        }
        else {
          maxOffset = padding[band-1];
          frame = (icube.lineCount() - maxOffset) / filterHeight - 1;
          if (exptime.size() > 0) {
            exposure = exptime[0];
          }
          else {
            exposure = ((double)icube.label()->findGroup("Instrument", Pvl::Traverse)["ExposureDuration"]) * 1000.0;
          }
        }
        seqno = 0;
      }

      flatcubes[fcubeIndex]->read((*fcubeMgrs[fcubeIndex]));

      line++;
      if (line > padding[band-1] || flipped) {
        if (!flipped) {
          frame = (int)((line-padding[band-1]-1)/filterHeight);
        }
        else {
          frame = (icube.lineCount() - maxOffset) / filterHeight - 1 - (int)((line-1)/filterHeight);
        }
        if (!flipped) {
          if (seqno < fsize) {
            if (frame >= frameseq[seqno]) {
              if (exptime.size() > 0) {
                exposure = exptime[seqno];
              }
              else {
                exposure = ((double)icube.label()->findGroup("Instrument", Pvl::Traverse)["ExposureDuration"]) * 1000.0;
              }
              // Exposure duration for the UV filters are calculated from the non-UV exposure duration
              if (QString::fromStdString(icube.label()->findGroup("BandBin", Pvl::Traverse)["FilterName"][band-1]) == "LONG_UV" ||
                  QString::fromStdString(icube.label()->findGroup("BandBin", Pvl::Traverse)["FilterName"][band-1]) == "SHORT_UV") {
                exposure = ifdelay - 57.763 - exposure;
              }
              seqno++;
            }
          }
        }
        else {
          if (seqno < fsize) {
            if (frame < frameseq[seqno]) {
              exposure = exptime[seqno];
              seqno++;
              if (QString::fromStdString(icube.label()->findGroup("BandBin", Pvl::Traverse)["FilterName"][band-1]) == "LONG_UV" ||
                  QString::fromStdString(icube.label()->findGroup("BandBin", Pvl::Traverse)["FilterName"][band-1]) == "SHORT_UV") {
                exposure = ifdelay - 57.763 - exposure;
              }
            }
          }
          else {
            exposure = ((double)icube.label()->findGroup("Instrument", Pvl::Traverse)["ExposureDuration"]) * 1000.0;
          }
        }
      }

      for (int i = 0; i < ocubeMgr.size(); i++) {
        if (IsSpecial((*fcubeMgrs[fcubeIndex])[i]) || (*fcubeMgrs[fcubeIndex])[i] == 0.0) {
          ocubeMgr[i] = Isis::Null;
        }
        else if (IsSpecial(icubeMgr[i])) {
          ocubeMgr[i] = icubeMgr[i];
        }
        else {
          if ((*fcubeMgrs[fcubeIndex])[i] < 0.25) {
            ocubeMgr[i] = 0;
          }
          else {
            ocubeMgr[i] = stretch.Map(icubeMgr[i]) / (*fcubeMgrs[fcubeIndex])[i];
          }

          ocubeMgr[i] = ocubeMgr[i] / exposure / (summing * decimation[fcubeIndex]) / calibrationCoeffs[fcubeIndex].first;

          // Convert to I/F?
          if (iof) {
            ocubeMgr[i] /= calibrationCoeffs[fcubeIndex].second / Isis::PI / (solarDist * solarDist);
          }
        }
      }

      ocube.write(ocubeMgr);

      icubeMgr++;
      ocubeMgr++;

      // checks if we have reached the end before setting higher band
      if (ocubeMgr.end()) {
        break;
      }

      bool newFramelet = false;

      for (int i = 0; i < (int)fcubeMgrs.size(); i++) {
        (*fcubeMgrs[i]) ++;

        if (fcubeMgrs[i]->end()) {
          fcubeMgrs[i]->SetLine(1, 1);
          newFramelet = true;
        }
      }

      if (newFramelet && cam != NULL) {
        // center the cameras position on the new framelet to keep the solar distance accurate
        cam->SetBand(icubeMgr.Band());
        cam->SetImage(icubeMgr.size() / 2.0 + 0.5, (icubeMgr.Line() - 0.5) + (16 / 2) / summing);
        solarDist = cam->SolarDistance();
      }

      prog.CheckStatus();
    } while (!ocubeMgr.end());


    // Propagate labels and objects (in case of spice data)
    PvlObject &inCubeObj = icube.label()->findObject("IsisCube");
    PvlObject &outCubeObj = ocube.label()->findObject("IsisCube");

    for (int g = 0; g < inCubeObj.groups(); g++) {
      outCubeObj.addGroup(inCubeObj.group(g));
    }

    for (int o = 0; o < icube.label()->objects(); o++) {
      if (icube.label()->object(o).isNamed("Table")) {
        Blob t(icube.label()->object(o)["Name"], icube.label()->object(o).name());
        icube.read(t);
        ocube.write(t);
      }
    }

    // The cube still owns this
    cam = NULL;

    for (int i = 0; i < (int)flatcubes.size(); i++) {
      delete fcubeMgrs[i];
      delete flatcubes[i];
    }

    // Clean up.
    fcubeMgrs.clear();
    flatcubes.clear();

    icube.close();
    ocube.close();
  }

  /*
    From the MARCI calibration report, Bell et al., 2009,
    Mars Reconnaissance Orbiter Mars Color Imager (MARCI):
    Instrument description, calibration, and performance, JGR, 114 E08S92
    doi:10.1029/2008JE003315

    It is also important to note that in order to maximize
  SNR during each orbit track, the exposure times of MARCI
  image sequences may vary over the course of individual
  MARCI raw images acquired after 28 April 2007 (when this
  strategy was implemented). For example, the visible band
  exposure time may have initially been set to a low value for
  imaging beginning over the bright south polar cap, then
  reset to a higher value for imaging over darker midlatitude
  and equatorial terrain, then reset to a lower value for imaging
  over the bright north polar cap. A text-formatted table called
  varexp.tab is provided in the ‘‘index’’ subdirectory associated
  with each MARCI PDS data volume release after that date,
  and this table describes these exposure time changes.
    Specifically, each table entry lists the file for which an
  exposure time change occurs, the first frame (in a zero-based
  counting system) having a new exposure time, as well as that
  new visible band exposure time itself, in milliseconds. The
  corresponding UV filter exposure time can be derived from
  the visible exposure time using equation (3) above. If the
  exposure time was changed more than once during an
  image, that image will have multiple entries in the table.
  A text file called varexp.lbl is also available in the PDS
  release that provides more details on the exposure time
  change table. More generally, additional details of the
  MARCI calibration pipeline process are described within
  the document marcical.txt that is stored online within the
  calib subdirectory associated with each MARCI PDS data
  volume release.


  PDS varexp.lbl description:

  PDS_VERSION_ID                 = PDS3
  RECORD_TYPE                    = FIXED_LENGTH
  RECORD_BYTES                   = 45
  FILE_RECORDS                   = 12644
  ^TABLE                         = "VAREXP.TAB"
  DATA_SET_ID                    = "MRO-M-MARCI-2-EDR-L0-V1.0"
  SPACECRAFT_NAME                = "MARS RECONNAISSANCE ORBITER"
  INSTRUMENT_NAME                = "MARS COLOR IMAGER"
  TARGET_NAME                    = MARS
  MISSION_PHASE_NAME             = "N/A"
  PRODUCT_CREATION_TIME          = 2007-10-31T19:00:00

  OBJECT                         = TABLE
  INTERCHANGE_FORMAT            = ASCII
  ROWS                          = 12644
  COLUMNS                       = 3
  ROW_BYTES                     = 45
  INDEX_TYPE                    = CUMULATIVE

  OBJECT = COLUMN
  NAME = PRODUCT_ID
  COLUMN_NUMBER = 1
  DATA_TYPE = CHARACTER
  START_BYTE = 2
  BYTES = 26
  FORMAT = "A26"
  DESCRIPTION = "product id"
  END_OBJECT = COLUMN

  OBJECT = COLUMN
  NAME = FRAME_SEQUENCE_NUMBER
  COLUMN_NUMBER = 2
  DATA_TYPE = ASCII_INTEGER
  START_BYTE = 30
  BYTES = 5
  FORMAT = "I5"
  DESCRIPTION = "The first frame (0-based) of this image for which
  the visible frame exposure time was changed to the given
  LINE_EXPOSURE_DURATION.  The associated UV frame exposure time
  (in milliseconds) is
  INTERFRAME_DELAY - 57.763 - LINE_EXPOSURE_DURATION."
  END_OBJECT = COLUMN

  OBJECT = COLUMN
  NAME = LINE_EXPOSURE_DURATION
  COLUMN_NUMBER = 3
  DATA_TYPE = ASCII_REAL
  START_BYTE = 36
  BYTES = 8
  FORMAT = "F8.4"
  DESCRIPTION = "frame exposure time in milliseconds"
  END_OBJECT = COLUMN

  END_OBJECT                     = TABLE

  END

    */
}