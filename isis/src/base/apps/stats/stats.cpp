#include "stats.h"

#include <string>
#include <iostream>

#include "Application.h"
#include "CubeAttribute.h"
#include "Cube.h"
#include "FileName.h"
#include "Histogram.h"
#include "Pvl.h"
#include "UserInterface.h"

using namespace std;
using namespace Isis;

namespace Isis {

  /**
   * Compute the stats for an ISIS cube. This is the programmatic interface to
   * the ISIS stats application.
   *
   * @param ui The User Interface to parse the parameters from
   */
  void stats(UserInterface &ui) {
    Cube *inputCube = new Cube();
    inputCube->open(ui.GetCubeName("FROM"));
    stats(inputCube, ui);
  }

  /**
   * Compute the stats for an ISIS cube. This is the programmatic interface to
   * the ISIS stats application.
   *
   * @param inputCube input Cube to compute stats for
   * @param ui The User Interface to parse the parameters from
   */
  void stats(Cube *inputCube, UserInterface &ui) {
    double validMin = Isis::ValidMinimum;
    double validMax = Isis::ValidMaximum;

    if ( ui.WasEntered("VALIDMIN") ) {
      validMin = ui.GetDouble("VALIDMIN");
    }

    if ( ui.WasEntered("VALIDMAX") ) {
      validMax = ui.GetDouble("VALIDMAX");
    }

    Pvl statsPvl = stats(inputCube, validMin, validMax);

    for (int resultIndex = 0; resultIndex < statsPvl.groups(); resultIndex++) {
      if (statsPvl.group(resultIndex).name() == "Results") {
        Application::Log(statsPvl.group(resultIndex));
      }
    }

    delete inputCube;
    inputCube = nullptr;

    if ( ui.WasEntered("TO") ) {
      QString outFile = QString::fromStdString(FileName(ui.GetFileName("TO").toStdString()).expanded());
      bool append = ui.GetBoolean("APPEND");
      //write the results in the requested format.
      if ( ui.GetString("FORMAT") == "PVL" ) {
        if (append) {
          statsPvl.append(outFile.toStdString());
        }
        else {
          statsPvl.write(outFile.toStdString());
        }
      }
      else {
        bool exists = FileName(outFile.toStdString()).fileExists();
        bool writeHeader = false;
        ofstream *os = new ofstream;
        if (append) {
          os->open(outFile.toLatin1().data(), ios::app);
          if (!exists) {
            writeHeader = true;
          }
        }
        else {
          os->open(outFile.toLatin1().data(), ios::out);
          writeHeader = true;
        }
        writeStatsStream(statsPvl, writeHeader, os);
        delete os;
        os = nullptr;
      }
    }
  }


  /**
   * Compute statistics about a Cube and store them in a PVL object.
   *
   * @param cube The cube to compute the statistics of
   * @param validMin The minimum pixel value to include in the statistics
   * @param validMax The maximum pixel value to include in the statistics
   *
   * @return @b Pvl The statistics for the cube in a Pvl object
   *
   * @see Cube::histogram
   */
  Pvl stats(Cube *cube, double validMin, double validMax) {

    // Set a global Pvl for storing results
    Pvl statsPvl;

    // Get the number of bands to process
    int bandCount = cube->bandCount();

    for (int i = 1; i <= bandCount; i++) {
      Histogram *stats = cube->histogram(i, validMin, validMax);

      // Construct a label with the results
      PvlGroup results("Results");
      results += PvlKeyword("From", cube->fileName().toStdString());
      results += PvlKeyword("Band", Isis::toString(cube->physicalBand(i)));
      if ( stats->ValidPixels() != 0 ) {
        results += PvlKeyword("Average", Isis::toString(stats->Average()));
        results += PvlKeyword("StandardDeviation", Isis::toString(stats->StandardDeviation()));
        results += PvlKeyword("Variance", Isis::toString(stats->Variance()));
        // These statistics only worked on a histogram
        results += PvlKeyword("Median", Isis::toString(stats->Median()));
        results += PvlKeyword("Mode", Isis::toString(stats->Mode()));
        results += PvlKeyword("Skew", Isis::toString(stats->Skew()));
        results += PvlKeyword("Minimum", Isis::toString(stats->Minimum()));
        results += PvlKeyword("Maximum", Isis::toString(stats->Maximum()));
        results += PvlKeyword("Sum", Isis::toString(stats->Sum()));
      }
      else {
        results += PvlKeyword("Average", "N/A");
        results += PvlKeyword("StandardDeviation", "N/A");
        results += PvlKeyword("Variance", "N/A");
        results += PvlKeyword("Median", "N/A");
        results += PvlKeyword("Mode", "N/A");
        results += PvlKeyword("Skew", "N/A");
        results += PvlKeyword("Minimum", "N/A");
        results += PvlKeyword("Maximum", "N/A");
        results += PvlKeyword("Sum", "N/A");
      }
      results += PvlKeyword("TotalPixels", Isis::toString(stats->TotalPixels()));
      results += PvlKeyword("ValidPixels", Isis::toString(stats->ValidPixels()));
      results += PvlKeyword("OverValidMaximumPixels", Isis::toString(stats->OverRangePixels()));
      results += PvlKeyword("UnderValidMinimumPixels", Isis::toString(stats->UnderRangePixels()));
      results += PvlKeyword("NullPixels", Isis::toString(stats->NullPixels()));
      results += PvlKeyword("LisPixels", Isis::toString(stats->LisPixels()));
      results += PvlKeyword("LrsPixels", Isis::toString(stats->LrsPixels()));
      results += PvlKeyword("HisPixels", Isis::toString(stats->HisPixels()));
      results += PvlKeyword("HrsPixels", Isis::toString(stats->HrsPixels()));

      statsPvl.addGroup(results);

      delete stats;
      stats = nullptr;
    }

    return statsPvl;
  }


  /**
   * Write a statistics Pvl to an output stream in a CSV format.
   *
   * @param statsPvl The Pvl to write out
   * @param writeHeader If a header line should be written
   * @param stram The stream to write to
   */
  void writeStatsStream(const Pvl &statsPvl, bool writeHeader, ostream *stream) {
    if (writeHeader) {
      for (int i = 0; i < statsPvl.group(0).keywords(); i++) {
        *stream << statsPvl.group(0)[i].name();
        if ( i < statsPvl.group(0).keywords() - 1 ) {
          *stream << ",";
        }
      }
      *stream << endl;
    }

    for (int i = 0; i < statsPvl.groups(); i++) {
      for (int j = 0; j < statsPvl.group(i).keywords(); j++) {
        *stream << statsPvl.group(i)[j][0];
        if ( j < statsPvl.group(i).keywords() - 1 ) {
          *stream << ",";
        }
      }
      *stream << endl;
    }
  }

}
