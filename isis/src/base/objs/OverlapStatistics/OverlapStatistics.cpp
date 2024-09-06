/** This is free and unencumbered software released into the public domain.
The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */
#include "OverlapStatistics.h"

#include <cfloat>
#include <iomanip>

#include "Brick.h"
#include "Cube.h"
#include "FileName.h"
#include "IException.h"
#include "MultivariateStatistics.h"
#include "Progress.h"
#include "Projection.h"
#include "ProjectionFactory.h"
#include "PvlGroup.h"
#include "PvlKeyword.h"
#include "PvlObject.h"

using namespace std;
namespace Isis {

  /**
   * Constructs an OverlapStatistics from a PvlObject
   *
   * @param inStats The serialized OverlapStatistics PvlObject
   */
  OverlapStatistics::OverlapStatistics(const PvlObject &inStats) {
    init();
    fromPvl(inStats);
  }

  /**
   * Constructs an OverlapStatistics object.  Compares the two input cubes and
   * finds where they overlap.
   *
   * @param x The first input cube
   * @param y The second input cube
   * @param progressMsg (Default value of "Gathering Overlap Statistics") Text
   *         for indicating progress during statistic gathering
   * @param sampPercent (Default value of 100.0) Sampling percent, or the percentage
   *       of lines to consider during the statistic gathering procedure
   *
   * @throws Isis::IException::User - All images must have the same number of
   *                                  bands
   */
  OverlapStatistics::OverlapStatistics(Isis::Cube &x, Isis::Cube &y,
                                       QString progressMsg, double sampPercent) {

    init();

    // Test to ensure sampling percent in bound
    if (sampPercent <= 0.0 || sampPercent > 100.0) {
      string msg = "The sampling percent must be a decimal (0.0, 100.0]";
      throw IException(IException::Programmer, msg, _FILEINFO_);
    }

    p_sampPercent = sampPercent;

    // Extract filenames and band number from cubes
    p_xFile = x.fileName().toStdString();
    p_yFile = y.fileName().toStdString();

    // Make sure number of bands match
    if (x.bandCount() != y.bandCount()) {
      std::string msg = "Number of bands do not match between cubes [" +
                   p_xFile.name() + "] and [" + p_yFile.name() + "]";
      throw IException(IException::User, msg, _FILEINFO_);
    }
    p_bands = x.bandCount();
    p_stats.resize(p_bands);

    //Create projection from each cube
    Projection *projX = x.projection();
    Projection *projY = y.projection();

    // Test to make sure projection parameters match
    if (*projX != *projY) {
      std::string msg = "Mapping groups do not match between cubes [" +
                   p_xFile.name() + "] and [" + p_yFile.name() + "]";
      throw IException(IException::Programmer, msg, _FILEINFO_);
    }

    // Figure out the x/y range for both images to find the overlap
    double Xmin1 = projX->ToProjectionX(0.5);
    double Ymax1 = projX->ToProjectionY(0.5);
    double Xmax1 = projX->ToProjectionX(x.sampleCount() + 0.5);
    double Ymin1 = projX->ToProjectionY(x.lineCount() + 0.5);

    double Xmin2 = projY->ToProjectionX(0.5);
    double Ymax2 = projY->ToProjectionY(0.5);
    double Xmax2 = projY->ToProjectionX(y.sampleCount() + 0.5);
    double Ymin2 = projY->ToProjectionY(y.lineCount() + 0.5);

    // Find overlap
    if ((Xmin1 < Xmax2) && (Xmax1 > Xmin2) && (Ymin1 < Ymax2) && (Ymax1 > Ymin2)) {
      double minX = Xmin1 > Xmin2 ? Xmin1 : Xmin2;
      double minY = Ymin1 > Ymin2 ? Ymin1 : Ymin2;
      double maxX = Xmax1 < Xmax2 ? Xmax1 : Xmax2;
      double maxY = Ymax1 < Ymax2 ? Ymax1 : Ymax2;

      // Find Sample range of the overlap
      p_minSampX = (int)(projX->ToWorldX(minX) + 0.5);
      p_maxSampX = (int)(projX->ToWorldX(maxX) + 0.5);
      p_minSampY = (int)(projY->ToWorldX(minX) + 0.5);
      p_maxSampY = (int)(projY->ToWorldX(maxX) + 0.5);
      p_sampRange = p_maxSampX - p_minSampX + 1;

      // Test to see if there was only sub-pixel overlap
      if (p_sampRange <= 0) return;

      // Find Line range of overlap
      p_minLineX = (int)(projX->ToWorldY(maxY) + 0.5);
      p_maxLineX = (int)(projX->ToWorldY(minY) + 0.5);
      p_minLineY = (int)(projY->ToWorldY(maxY) + 0.5);
      p_maxLineY = (int)(projY->ToWorldY(minY) + 0.5);
      p_lineRange = p_maxLineX - p_minLineX + 1;

      // Print percent processed
      Progress progress;
      progress.SetText(progressMsg);

      int linc = (int)(100.0 / sampPercent + 0.5); // Calculate our line increment

      // Define the maximum number of steps to be our line range divided by the
      // line increment, but if they do not divide evenly, then because of
      // rounding, we need to do an additional step for each band
      int maxSteps = (int)(p_lineRange / linc + 0.5);

      if (p_lineRange % linc != 0) maxSteps += 1;
      maxSteps *= p_bands;


      progress.SetMaximumSteps(maxSteps);
      progress.CheckStatus();

      // Collect and store off the overlap statistics
      for (int band = 1; band <= p_bands; band++) {
        Brick b1(p_sampRange, 1, 1, x.pixelType());
        Brick b2(p_sampRange, 1, 1, y.pixelType());

        int i = 0;
        while(i < p_lineRange) {
          b1.SetBasePosition(p_minSampX, (i + p_minLineX), band);
          b2.SetBasePosition(p_minSampY, (i + p_minLineY), band);
          x.read(b1);
          y.read(b2);
          p_stats[band-1].AddData(b1.DoubleBuffer(), b2.DoubleBuffer(), p_sampRange);

          // Make sure we consider the last line
          if (i + linc > p_lineRange - 1 && i != p_lineRange - 1) {
            i = p_lineRange - 1;
            progress.AddSteps(1);
          }
          else i += linc; // Increment the current line by our incrementer

          progress.CheckStatus();
        }
      }
    }
  }


  /**
   * Checks all bands of the cubes for an overlap, and will only return false
   * if none of the bands overlap
   *
   * @return bool Returns true if any of the bands overlap, and false if none
   *              of the bands overlap
   */
  bool OverlapStatistics::HasOverlap() const {
    for (int b = 0; b < p_bands; b++) {
      if (p_stats[b].ValidPixels() > 0) return true;
    }
    return false;
  }


  /**
   * Serialize overlap statistics as a PvlObject
   *
   * @param QString name (Default value of "OverlapStatistics") Name of the PvlObject created
   *
   * @return PvlObject A pvl object representing the OverlapStatistics and its data
   *
   * @throws Isis::IException::User - Trivial overlap between [File1] and [File2]
   */
  PvlObject OverlapStatistics::toPvl(QString name) const {
    try {
      // Empty strings are set to default value as well
      if (name.isEmpty()) {
        name = "OverlapStatistics";
      }

      // Add keywords for OverlapStatistics data
      PvlObject o(name.toStdString());
      o += PvlKeyword("File1", FileNameX().name());
      o += PvlKeyword("File2", FileNameY().name());
      o += PvlKeyword("Width", std::to_string(Samples()));
      o += PvlKeyword("Height", std::to_string(Lines()));
      o += PvlKeyword("Bands", std::to_string(Bands()));
      o += PvlKeyword("SamplingPercent", std::to_string(SampPercent()));
      o += PvlKeyword("MinCount", std::to_string(MinCount())); // Do we need this if EqInfo has this?

      // Create group for first file of overlap
      PvlGroup gX("File1");
      PvlKeyword stsX("StartSample", std::to_string(StartSampleX()));
      PvlKeyword ensX("EndSample", std::to_string(EndSampleX()));
      PvlKeyword stlX("StartLine", std::to_string(StartLineX()));
      PvlKeyword enlX("EndLine", std::to_string(EndLineX()));
      PvlKeyword avgX("Average");
      PvlKeyword stdX("StandardDeviation");
      PvlKeyword varX("Variance");
      for (int band = 1; band <= Bands(); band++) {
        if (HasOverlap(band)) {
          avgX += std::to_string(GetMStats(band).X().Average());
          stdX += std::to_string(GetMStats(band).X().StandardDeviation());
          varX += std::to_string(GetMStats(band).X().Variance());
        }
      }
      gX += stsX;
      gX += ensX;
      gX += stlX;
      gX += enlX;
      gX += avgX;
      gX += stdX;
      gX += varX;

      // Create group for second file of overlap
      PvlGroup gY("File2");
      PvlKeyword stsY("StartSample", std::to_string(StartSampleY()));
      PvlKeyword ensY("EndSample", std::to_string(EndSampleY()));
      PvlKeyword stlY("StartLine", std::to_string(StartLineY()));
      PvlKeyword enlY("EndLine", std::to_string(EndLineY()));
      PvlKeyword avgY("Average");
      PvlKeyword stdY("StandardDeviation");
      PvlKeyword varY("Variance");
      for (int band = 1; band <= Bands(); band++) {
        if (HasOverlap(band)) {
          avgY += std::to_string(GetMStats(band).Y().Average());
          stdY += std::to_string(GetMStats(band).Y().StandardDeviation());
          varY += std::to_string(GetMStats(band).Y().Variance());
        }
      }
      gY += stsY;
      gY += ensY;
      gY += stlY;
      gY += enlY;
      gY += avgY;
      gY += stdY;
      gY += varY;

      o.addGroup(gX);
      o.addGroup(gY);

      bool isValid = false;
      // Unserialize the MultivariateStatistics
      for (int band = 1; band <= Bands(); band++) {
        PvlKeyword validBand("ValidOverlap", "false");

        if (HasOverlap(band)) {
          if (IsValid(band)) {
            validBand.setValue("true");
            isValid = true;
          }
        }

        QString mStatsName = "MultivariateStatistics" + toString(band);
        PvlObject mStats(GetMStats(band).toPvl(mStatsName));
        mStats += validBand;
        o.addObject(mStats);
      }
      PvlKeyword valid("Valid", (isValid) ? "true" : "false");
      o += valid;
      return o;
    }

    catch (IException &e) {
      std::string msg = "Trivial overlap between [" + FileNameX().name();
      msg += "] and [" + FileNameY().name() + "]";
      throw IException(IException::User, msg, _FILEINFO_);
    }
  }


  /**
   * Unserialize overlap statistics from a Pvl
   *
   * @param const PvlObject & - The pvl object to initialize the overlap statistics with
   */
  void OverlapStatistics::fromPvl(const PvlObject &inStats) {
    init();

    const PvlGroup &fileX = inStats.findGroup("File1");
    const PvlGroup &fileY = inStats.findGroup("File2");
    p_xFile = inStats["File1"][0];
    p_yFile = inStats["File2"][0];
    p_sampRange = inStats["Width"];
    p_lineRange = inStats["Height"];
    p_bands = inStats["Bands"];
    p_sampPercent = inStats["SamplingPercent"];

    p_minSampX = fileX["StartSample"];
    p_maxSampX = fileX["EndSample"];
    p_minLineX = fileX["StartLine"];
    p_maxLineX = fileX["EndLine"];

    p_minSampY = fileY["StartSample"];
    p_maxSampY = fileY["EndSample"];
    p_minLineY = fileY["StartLine"];
    p_maxLineY = fileY["EndLine"];

    p_mincnt = inStats["MinCount"];

    // unserialize the MStats
    for (int band = 1; band <= Bands(); band++) {
      QString name = "MultivariateStatistics" + toString(band);
      const PvlObject &mStats = inStats.findObject(name.toStdString());
      p_stats.push_back(MultivariateStatistics(mStats)); 
    }

  }


  /**
   * Reset member variables to default values
   */
  void OverlapStatistics::init() {
    p_sampPercent = 0.0;
    p_bands = 0;
    p_sampRange = 0;
    p_lineRange = 0;
    p_minSampX = 0;
    p_maxSampX = 0;
    p_minSampY = 0;
    p_maxSampY = 0;
    p_minLineX = 0;
    p_maxLineX = 0;
    p_minLineY = 0;
    p_maxLineY = 0;
    p_mincnt = 0;
  }


  /**
   * Creates a pvl of various useful data obtained by the overlap statistics
   * class.  The pvl is returned in an output stream
   *
   * @param os The output stream to write to
   * @param stats The OverlapStatistics object to write to os
   * @return ostream Pvl of useful statistics
   */
  std::ostream &operator<<(std::ostream &os, Isis::OverlapStatistics &stats) {
    PvlObject p = stats.toPvl();
    os << p << endl;
    return os;
  }

}

