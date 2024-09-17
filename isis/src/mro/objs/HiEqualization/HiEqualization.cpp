/** This is free and unencumbered software released into the public domain.

The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */

#include "HiEqualization.h"

#include <iomanip>

#include "Buffer.h"
#include "Cube.h"
#include "IException.h"
#include "LineManager.h"
#include "OverlapNormalization.h"
#include "OverlapStatistics.h"
#include "Process.h"
#include "ProcessByLine.h"

using std::string;
using std::vector;


namespace Isis {


  HiEqualization::HiEqualization(QString fromListName) :
      Equalization() {
    loadInputs(fromListName);
  }


  HiEqualization::~HiEqualization() {
  }


  void HiEqualization::calculateStatistics() {
    // TODO member variable
    const FileList &imageList = getInputs();
    QString maxCubeStr = QString::number((int) imageList.size());

    // Adds statistics for whole and side regions of every cube
    vector<Statistics *> statsList;
    vector<Statistics *> leftStatsList;
    vector<Statistics *> rightStatsList;
    for (int img = 0; img < imageList.size(); img++) {
      Statistics *stats = new Statistics();
      Statistics *statsLeft = new Statistics();
      Statistics *statsRight = new Statistics();

      QString cubeStr = QString::number((int) img + 1);

      ProcessByLine p;
      p.Progress()->SetText("Calculating Statistics for Cube " +
          cubeStr + " of " + maxCubeStr);
      CubeAttributeInput att;
      QString inp = QString::fromStdString(imageList[img].toString());
      p.SetInputCube(inp, att);
      HiCalculateFunctor func(stats, statsLeft, statsRight, 100.0);
      p.ProcessCubeInPlace(func, false);

      statsList.push_back(stats);
      leftStatsList.push_back(statsLeft);
      rightStatsList.push_back(statsRight);
    }

    // Initialize the object that will calculate the gains and offsets
    OverlapNormalization oNorm(statsList);

    // Add the known overlaps between two cubes, and apply a weight to each
    // overlap equal the number of pixels in the overlapping area
    for (int i = 0; i < imageList.size() - 1; i++) {
      int j = i + 1;
      oNorm.AddOverlap(*rightStatsList[i], i, *leftStatsList[j], j,
          rightStatsList[i]->ValidPixels());
    }

    loadHolds(&oNorm);

    // Attempt to solve the least squares equation
    oNorm.Solve(OverlapNormalization::Both);
    setSolved(true);

    clearAdjustments();
    for (int img = 0; img < imageList.size(); img++) {
      ImageAdjustment *adjustment = new ImageAdjustment(OverlapNormalization::Both);
      adjustment->addGain(oNorm.Gain(img));
      adjustment->addOffset(oNorm.Offset(img));
      adjustment->addAverage(oNorm.Average(img));
      addAdjustment(adjustment);
    }

    addValid(imageList.size() - 1);
    setResults();
  }


  void HiEqualization::fillOutList(FileList &outList, QString toListName) {
    if (toListName.isEmpty()) {
      generateOutputs(outList);
    }
    else {
      FileList tempList;
      loadOutputs(tempList, toListName);

      for (unsigned int i = 0; i < movedIndices.size(); i++) {
        outList.push_back(tempList[movedIndices[i]]);
      }
    }
  }


  void HiEqualization::errorCheck(QString fromListName) {
    const FileList &imageList = getInputs();

    // Ensures number of images is within bound
    if (imageList.size() > 10) {
      std::string msg = "The input file [" + fromListName.toStdString() +
        "] cannot contain more than 10 file names";
      throw IException(IException::User, msg, _FILEINFO_);
    }

    // Reference for converting a CPMM number to a CCD number
    const int cpmm2ccd[] = {0, 1, 2, 3, 12, 4, 10, 11, 5, 13, 6, 7, 8, 9};

    // Place ccd in vector, try-catch opening cubes
    vector<int> ccds;
    for (int i = 0; i < imageList.size(); i++) {
      try {
        Cube cube1;
        cube1.open(QString::fromStdString(imageList[i].toString()));
        PvlGroup &from1Instrument = cube1.group("INSTRUMENT");
        int cpmmNumber = from1Instrument["CpmmNumber"];
        ccds.push_back(cpmm2ccd[cpmmNumber]);

        // In case we need to alter the order of the input list, keep a record
        // of how the indices changed so we can rearrange the output list later
        movedIndices.push_back(i);
      }
      catch (IException &e) {
        std::string msg = "The [" + imageList[i].toString() +
          "] file is not a valid HiRise image";
        throw IException(e, IException::User, msg, _FILEINFO_);
      }
      catch (...) {
        // If any part of the above didn't work, we can safely assume the
        // current file is not a valid HiRise image
        std::string msg = "The [" + imageList[i].toString() +
          "] file is not a valid HiRise image";
        throw IException(IException::User, msg, _FILEINFO_);
      }
    }

    // Error checking to ensure CCDID types match
    for (int i = 0; i < imageList.size() - 1; i++) {
      int id1 = getCCDType(ccds[i]);
      int id2 = getCCDType(ccds[i + 1]);

      // CCDID types don't match
      if (id1 != id2) {
        string msg = "The list of input images must be all RED, all IR, or ";
        msg += "all BG";
        throw IException(IException::User, msg, _FILEINFO_);
      }
    }

    // Insertion sorts a list of filenames by their CCD numbers
    for (int i = 1; i < imageList.size(); i++) {
      QString temp = QString::fromStdString(imageList[i].toString());
      int ccd1 = ccds[i];
      int movedIndex = movedIndices[i];

      int j = i - 1;
      int ccd2 = ccds[j];

      while (j >= 0 && ccd2 > ccd1) {
        setInput(j + 1, QString::fromStdString(imageList[j].toString()));
        ccds[j + 1] = ccds[j];
        movedIndices[j + 1] = movedIndices[j];

        j--;
        if (j >= 0) ccd2 = ccds[j];
      }

      setInput(j + 1, temp);
      ccds[j + 1] = ccd1;
      movedIndices[j + 1] = movedIndex;
    }

    // Ensures BG and IR only have two files
    if (ccds[0] == 10 || ccds[0] == 11) {
      if (imageList.size() != 2) {
        string msg = "A list of IR images must have exactly two ";
        msg += "file names";
        throw IException(IException::User, msg, _FILEINFO_);
      }
    }
    else if (ccds[0] == 12 || ccds[0] == 13) {
      if (imageList.size() != 2) {
        string msg = "A list of BG images must have exactly two ";
        msg += "file names";
        throw IException(IException::User, msg, _FILEINFO_);
      }
    }
  }


  // CCD ID Type
  int HiEqualization::getCCDType(int ccd) {
    // Red, IR, or BG
    return (ccd >= 0 && ccd <= 9) ? 0 : (ccd == 10 || ccd == 11) ? 1 : 2;
  }


  void HiEqualization::HiCalculateFunctor::addStats(Buffer &in) const {
    Equalization::CalculateFunctor::addStats(in);

    // Number of samples per line that intersect with the next and the
    // previous images.  Check if samples equal 682 or 683.  If not the above
    // case, then we perform an algorithm to account for binning.  Number of
    // intersecting samples is directly related to total number of samples in
    // the line, with 2048 being the maximum possible.
    unsigned int intersect = (in.size() == 682 || in.size() == 683) ?
      18 : (48 * in.size()) / 2048;

    m_statsLeft->AddData(&in[0], intersect);
    m_statsRight->AddData(&in[in.size() - intersect], intersect);
  }


}
