/** This is free and unencumbered software released into the public domain.
The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */
#include "Equalization.h"

#include <iomanip>
#include <vector>

#include <QString>
#include <QStringList>
#include <QVector>

#include "Buffer.h"
#include "Cube.h"
#include "FileList.h"
#include "IException.h"
#include "LeastSquares.h"
#include "LineManager.h"
#include "OverlapNormalization.h"
#include "OverlapStatistics.h"
#include "Process.h"
#include "ProcessByLine.h"
#include "Projection.h"
#include "Pvl.h"
#include "PvlGroup.h"
#include "PvlObject.h"
#include "Statistics.h"

using namespace std;

namespace Isis {


  /**
   * Default constructor
   */
  Equalization::Equalization() {
    init();
  }


  /**
   * Constructs an Equalization with the specified solution type and loads input images
   *
   * @param sType An integer value corresponding to the enumerated value of the
   *              OverlapNormalization::SolutionType to be used.
   * @param fromListName Name of the input image list.
   */
  Equalization::Equalization(OverlapNormalization::SolutionType sType, QString fromListName) {
    init();

    m_sType = sType;
    loadInputs(fromListName);
  }


  /**
   * Destructor
   */
  Equalization::~Equalization() {
    clearAdjustments();
    clearNormalizations();
    clearOverlapStatistics();

    if (m_results != NULL) {
      delete m_results;
      m_results = NULL;
    }
  }


  /**
   * Adds a list of images to be held in the equalization
   *
   * @param holdListName Name of the file containing a list of images to hold.
   *
   * @throws IException::User "The list of identifiers to be held must be less than or equal
   *                           to the total number of identifiers."
   * @throws IException::User "The hold list file does not match a file in the from list"
   */
  void Equalization::addHolds(QString holdListName) {
    FileList holdList;
    holdList.read(FileName(holdListName.toStdString()));

    if (holdList.size() > m_imageList.size()) {
      std::string msg = "The list of identifiers to be held must be less than or ";
      msg += "equal to the total number of identitifers.";
      throw IException(IException::User, msg, _FILEINFO_);
    }

    // Make sure each file in the holdlist matches a file in the fromlist
    for (int i = 0; i < holdList.size(); i++) {
      bool matched = false;
      for (int j = 0; j < m_imageList.size(); j++) {
        if (holdList[i] == m_imageList[j]) {
          matched = true;
          m_holdIndices.push_back(j);
          break;
        }
      }
      if (!matched) {
        std::string msg = "The hold list file [" + holdList[i].toString() +
                          "] does not match a file in the from list";
        throw IException(IException::User, msg, _FILEINFO_);
      }
    }
  }


  /**
   * @brief Calculates the image and overlap statistics, and then determines corrective factors if
   * possible
   *
   * This method calculates image statistics on a band-by-band basis and calculates overlap
   * statistics for the input images. Overlaps are considered valid if the number of valid pixels
   * in the overlapping area is greater than or equal to the provided mincnt parameter. Corrective
   * factors will only be determined if all of the input images have at least one valid overlap.
   *
   * @param percent Percentage of the lines to consider when gathering overall
   *            cube statistics and overlap statistics
   * @param mincnt Minimum number of points in overlapping area required to be
   *            used in the solution
   * @param wtopt Indicates whether overlaps should be weighted
   * @param methodType An integer value corresponding to the enumerated value of
   *            the desired LeastSquares::SolveMethod to be used.
   *
   * @throws IException::User "There are input images that do not overlap with enough valid pixels."
   *                          "See application log or Nonoverlaps keyword in output statistics file"
   * @throws IException::Unknown "Calculation for equalization statistics failed. Gain = 0."
   * @throws IException::Unknown "Unable to calculate the equalization statistics. You may "
   *                             "want to try another LeastSquares::SolveMethod."
   */
  void Equalization::calculateStatistics(double percent, int mincnt, bool wtopt,
      LeastSquares::SolveMethod methodType) {

    // We're going to redetermine which file are non-overlapping (if recalculating)
    m_badFiles.clear();

    m_mincnt = mincnt;
    m_samplingPercent = percent;
    m_wtopt = wtopt;
    m_lsqMethod = methodType;

    // Calculate statistics for each image+band (they've already been calculated if recalculating)
    if (!m_recalculating) {
      calculateBandStatistics();
    }

    calculateOverlapStatistics();

    // We can't solve the normalizations if we have invalid overlaps
    for (int img = 0; img < m_imageList.size(); img++) {
      // Record name of each input cube without an overlap
      if (!m_doesOverlapList[img]) {
        m_badFiles += QString::fromStdString(m_imageList[img].toString());
      }
    }
    if (!m_badFiles.isEmpty()) {
      // Make sure we set the results for the already calculated overlap statistics
      setResults();

      std::string msg;
      // Let user know where to find list of non-overlapping files so they can make corrections.
      msg = "There are input images that do not overlap with enough valid pixels. ";
      msg += "See application log or \"NonOverlaps\" keyword in output statistics file.";
      throw IException(IException::User, msg, _FILEINFO_);
    }

    // Loop through each band making all necessary calculations
    try {
      for (int band = 0; band < m_maxBand; band++) {
        m_overlapNorms[band]->Solve(m_sType, methodType);

        for (unsigned int img = 0; img < m_adjustments.size(); img++) {
          m_adjustments[img]->addGain(m_overlapNorms[band]->Gain(img));
          if (qFuzzyCompare(m_overlapNorms[band]->Gain(img), 0.0)) { // if gain == 0
            cout << img << endl;
            cout << band << endl;
            cout << m_overlapNorms[band]->Gain(img) << endl;
            setResults();
            throw IException(IException::Unknown,
                             "Calculation for equalization statistics failed. Gain = 0.",
                             _FILEINFO_);
          }
          m_adjustments[img]->addOffset(m_overlapNorms[band]->Offset(img));
          m_adjustments[img]->addAverage(m_overlapNorms[band]->Average(img));
        }
      }
      m_normsSolved = true;
    }
    catch (IException &e) {
      setResults();
      std::string msg = "Unable to calculate the equalization statistics. You may "
      "want to try another LeastSquares::SolveMethod.";
      throw IException(e, IException::Unknown,  msg, _FILEINFO_);
    }

    setResults();
  }


  /**
   * @brief Calculates the image statistics on a band-by-band basis
   *
   * This method calculates statistics by band for the input images. Each set of
   * band statistics is used to initialize the OverlapNormalizations that will
   * be used to determine gains and offsets for equalization.
   */
  void Equalization::calculateBandStatistics() {
    // Loop through all the input cubes, calculating statistics for each cube
    // to use later
    for (int band = 1; band <= m_maxBand; band++) {
      // OverlapNormalization will take ownership of these pointers
      vector<Statistics *> statsList;
      for (int img = 0; img < (int) m_imageList.size(); img++) {
        ProcessByLine p;
        QString bandStr(QString::number(band));
        QString statMsg = "Calculating Statistics for Band " + bandStr +
            " of " + QString::number(m_maxBand) + " in Cube " + QString::number(img + 1) +
            " of " + QString::number(m_maxCube);
        p.Progress()->SetText(statMsg);
        CubeAttributeInput att("+" + bandStr.toStdString());
        QString inp = QString::fromStdString(m_imageList[img].toString());
        p.SetInputCube(inp, att);

        Statistics *stats = new Statistics();

        CalculateFunctor func(stats, m_samplingPercent);
        p.ProcessCubeInPlace(func, false);
        p.EndProcess();

        statsList.push_back(stats);
      }

      // Create a separate OverlapNormalization object for every band
      OverlapNormalization *oNorm = new OverlapNormalization(statsList);
      loadHolds(oNorm);
      m_overlapNorms.push_back(oNorm);
    }
  }


  /**
   * @brief Calculates the overlap statistics for each pair of input images
   *
   * This method calculates any overlap statistics that have not been previously
   * calculated for the input images.
   */
  void Equalization::calculateOverlapStatistics() {
    // Add adjustments for all input images
    for (int img = 0; img < m_imageList.size(); img++) {
      addAdjustment(new ImageAdjustment(m_sType));
    }

    // Find overlapping areas and add them to the set of known overlaps for
    // each band shared amongst cubes
    for (int i = 0; i < m_imageList.size(); i++) {
      Cube cube1;
      cube1.open(QString::fromStdString(m_imageList[i].toString()));

      for (int j = (i + 1); j < m_imageList.size(); j++) {
        // Skip if overlap already calculated
        if (m_alreadyCalculated[i] == true && m_alreadyCalculated[j] == true) {
          //cout << " *** Cube " << i << " vs Cube " << j << " already calculated." << endl;
          //cout << endl << "    " << i << " = " << m_imageList[i].toString() << " ; " << j
          //     << " = " << m_imageList[j].toString() << endl << endl;
          continue;
        }

        Cube cube2;
        cube2.open(QString::fromStdString(m_imageList[j].toString()));
        QString cubeStr1 = QString::number((int)(i + 1));
        QString cubeStr2 = QString::number((int)(j + 1));
        QString statMsg = "Gathering Overlap Statisitcs for Cube " +
                         cubeStr1 + " vs " + cubeStr2 + " of " +
                         QString::number(m_maxCube);

        // Get overlap statistics for new cubes
        OverlapStatistics *oStats = new OverlapStatistics(cube1, cube2, statMsg, m_samplingPercent);
        // Only push the stats onto the overlap statistics vector if there is an overlap in at
        // least one of the bands
        if (oStats->HasOverlap()) {
          m_overlapStats.push_back(oStats);
          oStats->SetMincount(m_mincnt);
          for (int band = 1; band <= m_maxBand; band++) {
            // Fill wt vector with 1's if the overlaps are not to be weighted, or
            // fill the vector with the number of valid pixels in each overlap
            int weight = 1;
            if (m_wtopt) weight = oStats->GetMStats(band).ValidPixels();

            // Make sure overlap has at least MINCOUNT valid pixels and add
            if (oStats->GetMStats(band).ValidPixels() >= m_mincnt) {
              m_overlapNorms[band - 1]->AddOverlap(
                  oStats->GetMStats(band).X(), i,
                  oStats->GetMStats(band).Y(), j, weight);
              m_doesOverlapList[i] = true;
              m_doesOverlapList[j] = true;
            }
          }
        }
      }
    }

    // Compute the number valid and invalid overlaps
    for (unsigned int o = 0; o < m_overlapStats.size(); o++) {
      for (int band = 1; band <= m_maxBand; band++) {
        (m_overlapStats[o]->IsValid(band)) ? m_validCnt++ : m_invalidCnt++;
      }
    }
  }


  /**
   * @brief Creates the results pvl containing statistics and corrective factors
   *
   * This method creates the results pvl containing what is essentially serialized Equalization
   * data, which can be unserialized via the fromPvl() method. Note that the overlap statistics
   * in the results pvl may not be ordered if recalculating statistics with a modified input
   * image list.
   */
  void Equalization::setResults() {
    if (m_results != NULL) {
      delete m_results;
      m_results = NULL;
    }

    m_results = new Pvl();
    m_results->setTerminator("");

    PvlObject equ("EqualizationInformation");
    PvlGroup gen("General");
    gen += PvlKeyword("TotalOverlaps", std::to_string(m_validCnt + m_invalidCnt));
    gen += PvlKeyword("ValidOverlaps", std::to_string(m_validCnt));
    gen += PvlKeyword("InvalidOverlaps", std::to_string(m_invalidCnt));
    gen += PvlKeyword("MinCount", std::to_string(m_mincnt));
    gen += PvlKeyword("SamplingPercent", std::to_string(m_samplingPercent));
    gen += PvlKeyword("Weighted", (m_wtopt) ? "true" : "false");
    int solType = m_sType;
    int lsqMethod = m_lsqMethod;
    gen += PvlKeyword("SolutionType", std::to_string(solType));
    gen += PvlKeyword("SolveMethod" , std::to_string(lsqMethod));
    PvlKeyword nonOverlaps("NonOverlaps");
    for (int img = 0; img < m_badFiles.size(); img++) {
      nonOverlaps += m_badFiles[img].toStdString();
    }
    gen += nonOverlaps;
    gen += PvlKeyword("HasCorrections", (m_normsSolved) ? "true" : "false");
    equ.addGroup(gen);

    // Add normalization statistics
    for (int img = 0; img < m_imageList.size(); img++) {
      // Format and name information
      // It is important that the variable names are listed in the same
      // order as they are added to the PvlKeyword in the for loop.
      PvlGroup norm("Normalization");
      switch(m_sType) {
        case OverlapNormalization::Both:
          norm.addComment("Formula: newDN = (oldDN - AVERAGE) * GAIN + AVERAGE + OFFSET");
          norm.addComment("BandN = (GAIN, OFFSET, AVERAGE)");
          break;
        case OverlapNormalization::Gains:
          norm.addComment("Formula: newDN = oldDN * GAIN + AVERAGE * GAIN");
          norm.addComment("BandN = (GAIN, AVERAGE)");
          break;
        case OverlapNormalization::Offsets:
          norm.addComment("Formula: newDN = OFFSET + AVERAGE");
          norm.addComment("BandN = (OFFSET, AVERAGE)");
          break;
        case OverlapNormalization::GainsWithoutNormalization:
          norm.addComment("Formula: newDN = oldDN * GAIN");
          norm.addComment("BandN = (GAIN)");
          break;
      }
      norm += PvlKeyword("FileName", m_imageList[img].original());

      if (m_normsSolved) {
        // Band by band statistics
        for (int band = 1; band <= m_maxBand; band++) {
          QString bandNum = QString::number(band);
          QString bandStr = "Band" + bandNum;
          PvlKeyword bandStats(bandStr.toStdString());
          // GAIN
          if (m_sType == OverlapNormalization::Both ||
              m_sType == OverlapNormalization::Gains ||
              m_sType == OverlapNormalization::GainsWithoutNormalization) {
            bandStats += std::to_string(m_adjustments[img]->getGain(band - 1));
          }
          // OFFSET
          if (m_sType == OverlapNormalization::Both ||
              m_sType == OverlapNormalization::Offsets) {
            bandStats += std::to_string(m_adjustments[img]->getOffset(band - 1));
          }
          // AVERAGE
          if (m_sType == OverlapNormalization::Both ||
              m_sType == OverlapNormalization::Gains ||
              m_sType == OverlapNormalization::Offsets) {
            bandStats += std::to_string(m_adjustments[img]->getAverage(band - 1));
          }
          norm += bandStats;
        }
      }

      equ.addGroup(norm);
    }

    m_results->addObject(equ);

    // Add overlap statistics
    for (unsigned int i = 0; i < m_overlapStats.size(); i++) {
      PvlObject oStat = m_overlapStats[i]->toPvl();
      m_results->addObject(m_overlapStats[i]->toPvl());
    }
  }


  /**
   * @brief Recalculates statistics for any new input images
   *
   * This method loads a previous Equalization state from an input pvl file and calculates
   * overlap statistics for any new input images. Corrective factors will also be
   * calculated if there are no non-overlapping images and there are enough valid overlaps.
   *
   * @param instatsFileName Name of input pvl file containing previously calculated statistics
   */
  void Equalization::recalculateStatistics(QString instatsFileName) {
    m_recalculating = true;
    Pvl inStats(instatsFileName.toStdString());
    fromPvl(inStats);
    calculateStatistics(m_samplingPercent, m_mincnt, m_wtopt, m_lsqMethod);
  }


  /**
   * @brief Imports statistics for applying correction
   *
   * This method obtains corrective factors from an input statistics pvl file so that
   * input images can be equalized. These corrective factors are obtained from Normalization
   * groups within the EqualizationInformation object in the input pvl.
   *
   * @see Equalization::applyCorrection()
   *
   * @param instatsFileName Name of the input statistics pvl file
   */
  void Equalization::importStatistics(QString instatsFileName) {

    // Check for errors with the input statistics
    QVector<int> normIndices = validateInputStatistics(instatsFileName);
    Pvl inStats(instatsFileName.toStdString());
    PvlObject &equalInfo = inStats.findObject("EqualizationInformation");
    PvlGroup &general = equalInfo.findGroup("General");

    // Determine if normalizations were solved
    // First condition allows backward compatibility so users can use old stats files
    if (!general.hasKeyword("HasCorrections") || general["HasCorrections"][0] == "true") {
      m_normsSolved = true;

      clearAdjustments();
      for (int img = 0; img < (int) m_imageList.size(); img++) {
        // Apply correction based on pre-determined statistics information
        PvlGroup &normalization = equalInfo.group(normIndices[img]);

        // TODO should we also get the valid and invalid count?

        ImageAdjustment *adjustment = new ImageAdjustment(m_sType);

        // Get and store the modifiers for each band
        for (int band = 1; band < normalization.keywords(); band++) {
          adjustment->addGain(IString::ToDouble(normalization[band][0]));
          adjustment->addOffset(IString::ToDouble(normalization[band][1]));
          adjustment->addAverage(IString::ToDouble(normalization[band][2]));
        }

        addAdjustment(adjustment);
      }
    }
    else {
      m_normsSolved = false;
    }

  }


 /**
  * @brief Equalizes the input images
  *
  * This method applies corretive factors to the input images, thereby equalizing them.
  *
  * @param toListName (Default value is "") List of cube names to save the equalized images as
  *
  * @throws IException::User "Corrective factors have not yet been determined. Fix any "
  *                          "non-overlapping images and recalculate the image statistics."
  */
  void Equalization::applyCorrection(QString toListName="") {
    if (!isSolved()) {
      std::string msg = "Corrective factors have not yet been determined. ";
      if (m_badFiles.size() > 0) {
        msg += "Fix any non-overlapping images and recalculate the image statistics. ";
        msg += "File(s) without overlaps: ";
        for (int img = 0; img < m_badFiles.size(); img++) {
          msg += " [" + m_badFiles[img].toStdString() + "] ";
        }
      }
      else {
        msg += "Add more images to create more overlaps and recalculate, ";
        msg += "or try another solve method.";
      }
      throw IException(IException::User, msg, _FILEINFO_);
    }

    FileList outList;
    fillOutList(outList, toListName);

    QString maxCubeStr = QString::number((int) m_imageList.size());
    for (int img = 0; img < m_imageList.size(); img++) {
      // Set up for progress bar
      ProcessByLine p;
      p.Progress()->SetText("Equalizing Cube " + QString::number((int) img + 1) +
          " of " + maxCubeStr);

      // Open input cube
      CubeAttributeInput att;
      const QString inp = QString::fromStdString(m_imageList[img].toString());
      Cube *icube = p.SetInputCube(inp, att);

      // Allocate output cube
      QString out = QString::fromStdString(outList[img].toString());
      CubeAttributeOutput outAtt;
      p.SetOutputCube(out, outAtt, icube->sampleCount(),
          icube->lineCount(), icube->bandCount());

      // Apply gain/offset to the image
      ApplyFunctor func(m_adjustments[img]);
      p.ProcessCube(func, false);
      p.EndProcess();
    }
  }


  /**
   * @brief Returns general information about the equalization.
   *
   * This method returns general information about the equalization, including number of valid and
   * invalid overlaps, any non-overlapping images, the LeastSquares solution type, the type of
   * equalization adjustmnet, and corrective factors (if solved).
   */
  PvlGroup Equalization::getResults() {
    // TODO combine summary info into getSummary method and use with write
    PvlGroup results = m_results->findObject("EqualizationInformation").findGroup("General");
    if (m_normsSolved) {

      // Name and band modifiers for each image
      for (int img = 0; img < m_imageList.size(); img++) {
        results += PvlKeyword("FileName", m_imageList[img].toString());

        // Band by band statistics
        for (int band = 1; band <= m_maxBand; band++) {
          QString mult = QString::number(m_adjustments[img]->getGain(band - 1));
          QString base = QString::number(m_adjustments[img]->getOffset(band - 1));
          QString avg = QString::number(m_adjustments[img]->getAverage(band - 1));
          QString bandNum = QString::number(band);
          QString bandStr = "Band" + bandNum;
          PvlKeyword bandStats(bandStr.toStdString());
          bandStats += mult.toStdString();
          bandStats += base.toStdString();
          bandStats += avg.toStdString();
          results += bandStats;
        }
      }
    }
    return results;
  }


  /**
   * @brief Write the equalization information (results) to a file
   *
   * @param outstatsFileName The name of the file to write the results to
   */
  void Equalization::write(QString outstatsFileName) {
    // Write the equalization and overlap statistics to the file
    m_results->write(outstatsFileName.toStdString());
  }


  double Equalization::evaluate(double dn, int imageIndex, int bandIndex) const {
    return m_adjustments[imageIndex]->evaluate(dn, bandIndex);
  }


  /**
   * @brief Loads in the input images
   *
   * This method loads in the input images contained in the provided file name.
   *
   * @param fromListName Name of the file containing the input images
   *
   * @throws IException::User "The input file must contain at least 2 file names"
   */
  void Equalization::loadInputs(QString fromListName) {
    // Get the list of cubes to mosaic
    m_imageList.read(fromListName.toStdString());
    m_maxCube = m_imageList.size();

    if (m_imageList.size() < 2) {
      std::string msg = "The input file [" + fromListName.toStdString() +
        "] must contain at least 2 file names";
      throw IException(IException::User, msg, _FILEINFO_);
    }

    Cube tempCube;
    tempCube.open(QString::fromStdString(m_imageList[0].toString()));
    m_maxBand = tempCube.bandCount();

    m_doesOverlapList.resize(m_imageList.size(), false);
    m_alreadyCalculated.resize(m_imageList.size(), false);

    errorCheck(fromListName);
  }


  void Equalization::setInput(int index, QString value) {
    m_imageList[index] = value.toStdString();
  }


  const FileList &Equalization::getInputs() const {
    return m_imageList;
  }


  void Equalization::fillOutList(FileList &outList, QString toListName) {
    if (toListName.isEmpty()) {
      generateOutputs(outList);
    }
    else {
      loadOutputs(outList, toListName);
    }
  }


  /**
   * @brief Checks that the input images have the same mapping groups and same number of bands
   *
   * @throws IException::User "Number of bands do not match between cubes"
   * @throws IException::User "Mapping groups do not match between cubes"
   */
  void Equalization::errorCheck(QString fromListName) {
    for (int i = 0; i < m_imageList.size(); i++) {
      Cube cube1;
      cube1.open(QString::fromStdString(m_imageList[i].toString()));

      for (int j = (i + 1); j < m_imageList.size(); j++) {
        Cube cube2;
        cube2.open(QString::fromStdString(m_imageList[j].toString()));

        // Make sure number of bands match
        if (m_maxBand != cube2.bandCount()) {
          std::string msg = "Number of bands do not match between cubes [" +
            m_imageList[i].toString() + "] and [" + m_imageList[j].toString() + "]";
          throw IException(IException::User, msg, _FILEINFO_);
        }

        //Create projection from each cube
        Projection *proj1 = cube1.projection();
        Projection *proj2 = cube2.projection();

        // Test to make sure projection parameters match
        if (*proj1 != *proj2) {
          std::string msg = "Mapping groups do not match between cubes [" +
            m_imageList[i].toString() + "] and [" + m_imageList[j].toString() + "]";
          throw IException(IException::User, msg, _FILEINFO_);
        }
      }
    }
  }


  /**
   * @brief Generates the names of the equalized cubes if no output list is provided
   */
  void Equalization::generateOutputs(FileList &outList) {
    for (int img = 0; img < m_imageList.size(); img++) {
      FileName file(m_imageList[img]);
      std::string filename = file.path() + "/" + file.baseName() +
        ".equ." + file.extension();
      outList.push_back(FileName(filename));
    }
  }


  /**
   * @brief Checks that the output image list is correct.
   *
   * @throws IException::User "Each input file in the FROM LIST must have a corresponding output
   *                           file in the TO LIST."
   * @throws IException::User "The to list file has the same name as its corresponding from list
   *                           fil."
   */
  void Equalization::loadOutputs(FileList &outList, QString toListName) {
    outList.read(FileName(toListName.toStdString()));

    // Make sure each file in the tolist matches a file in the fromlist
    if (outList.size() != m_imageList.size()) {
      std::string msg = "Each input file in the FROM LIST must have a ";
      msg += "corresponding output file in the TO LIST.";
      throw IException(IException::User, msg, _FILEINFO_);
    }

    // Make sure that all output files do not have the same names as their
    // corresponding input files
    for (int i = 0; i < outList.size(); i++) {
      if (outList[i].toString().compare(m_imageList[i].toString()) == 0) {
        std::string msg = "The to list file [" + outList[i].toString() +
          "] has the same name as its corresponding from list file.";
        throw IException(IException::User, msg, _FILEINFO_);
      }
    }
  }


  void Equalization::loadHolds(OverlapNormalization *oNorm) {
    for (unsigned int h = 0; h < m_holdIndices.size(); h++)
      oNorm->AddHold(m_holdIndices[h]);
  }


  /**
   * @brief Frees image adjustments
   */
  void Equalization::clearAdjustments() {
    for (unsigned int adj = 0; adj < m_adjustments.size(); adj++) {
      delete m_adjustments[adj];
    }
    m_adjustments.clear();
  }


  /**
   * @brief Adds an image adjustment
   *
   * This method adds an image adjustment to be used for equalizing.
   *
   * @param adjustment - ImageAdjustment pointer to add.
   */
  void Equalization::addAdjustment(ImageAdjustment *adjustment) {
    m_adjustments.push_back(adjustment);
  }


  /**
   * @brief Frees overlap normalizations
   */
  void Equalization::clearNormalizations() {
    for (unsigned int oNorm = 0; oNorm < m_overlapNorms.size(); oNorm++) {
      delete m_overlapNorms[oNorm];
    }
    m_overlapNorms.clear();
  }


  /**
   * @brief Frees overlap statistics
   */
  void Equalization::clearOverlapStatistics() {
    for (unsigned int oStat = 0; oStat < m_overlapStats.size(); oStat++) {
      delete m_overlapStats[oStat];
    }
    m_overlapStats.clear();
  }


  /**
   * Increments the number of valid overlaps by a given amount
   *
   * @param count An integer value to increment the internally stored number of valid overlaps
   */
  void Equalization::addValid(int count) {
    m_validCnt += count;
  }


  /**
   * Increments the number of invalid overlaps by a given amount
   *
   * @param count An integer value to increment the internally stored number of invalid overlaps
   */
  void Equalization::addInvalid(int count) {
    m_invalidCnt += count;
  }


  /**
   * @brief Loads a previous Equalization state from an input pvl object
   *
   * This method will unserialize Equalization data from an input pvl, obtaining any previously
   * calculated overlap statistics and corrective factors (if solved).
   *
   * @param const PvlObject& Input pvl object containing previous Equalization state
   */
  void Equalization::fromPvl(const PvlObject &inStats) {

    // Make a copy of our image list with names only (instead of full path)
    QList<QString> imgNames;
    for (int img = 0; img < m_imageList.size(); img++) {
      imgNames.append(QString::fromStdString(m_imageList[img].name()));
    }

    // Load in previous user params
    const PvlObject &eqInfo = inStats.findObject("EqualizationInformation");
    const PvlGroup &eqGen = eqInfo.findGroup("General");
    m_samplingPercent = eqGen["SamplingPercent"];
    m_mincnt = eqGen["MinCount"];
    m_wtopt = (eqGen["Weighted"][0] == "true") ? true : false;
    m_sType = static_cast<OverlapNormalization::SolutionType>((int)eqGen["SolutionType"]);
    m_lsqMethod = static_cast<LeastSquares::SolveMethod>(IString::ToInteger(eqGen["SolveMethod"][0]));

    // Unserialize previous overlap statistics
    PvlObject::ConstPvlObjectIterator curObj = inStats.beginObject();
    while (curObj != inStats.endObject()) {
      if (curObj->isNamed("OverlapStatistics")) {
        OverlapStatistics *o = new OverlapStatistics(*curObj);
        m_overlapStats.push_back(o);

        const PvlObject &oStat = *curObj;
        QString fileX = QString::fromStdString(oStat["File1"][0]);
        QString fileY = QString::fromStdString(oStat["File2"][0]);

        // Determine already calculated overlaps
        int x = imgNames.indexOf(fileX);
        int y = imgNames.indexOf(fileY);
        m_alreadyCalculated[x] = true;
        m_alreadyCalculated[y] = true;

        // Determine which calculated overlaps have valid overlaps
        // (i.e. valid pixels > mincount)
        if (oStat["Valid"][0] == "true") {
          m_doesOverlapList[x] = true;
          m_doesOverlapList[y] = true;
        }
      }
      curObj++;
    }

    // Calculate the image+band statistics
    calculateBandStatistics();

    // Calculate x/y indices that map the overlap x/y statistics to the input image index
    // { {olap1idxX, olap1indxY}, {olap2idxX, olap12idxY} ... }
   // vector< vector<int> > overlapIndices(m_overlapStats.size(),
   //                                      vector<int>(2));
   //int overlapIndices[m_overlapStats.size()][2];
   QVector< QVector<int> > overlapIndices(m_overlapStats.size());
    for (int o = 0; o < (int) m_overlapStats.size(); o++) {
      OverlapStatistics *oStat = m_overlapStats[o];

      // Calculate the indices - we want to ensure that no matter how the input list of images
      // changes (e.g. the order is changed), we always add overlaps that same way.
      // This means ensuring that each overlap has the X overlap statistics associated with
      // the X index and the Y overlap statistics associated with the Y index.
      for (int i = 0; i < m_imageList.size(); i++) {
        QString imageName(QString::fromStdString(m_imageList[i].name()));

        // Always push the X file index to the front (0)
        if (oStat->FileNameX() == imageName.toStdString())
          //overlapIndices[o].insert(overlapIndices[o].begin(), i);
          //overlapIndices[o][0] = i;
          overlapIndices[o].push_front(i);
        // Always push the Y file index to the back (1)
        if (oStat->FileNameY() == imageName.toStdString())
          //overlapIndices[o].push_back(i);
          //overlapIndices[o][1] = i;
          overlapIndices[o].push_back(i);
      }
    }

    // Add the overlap
    for (int o = 0; o < (int) m_overlapStats.size(); o++) {
      OverlapStatistics *oStat = m_overlapStats[o];
      for (int band = 1; band <= oStat->Bands(); band++) {
        int weight = 1;
        if (m_wtopt) weight = oStat->GetMStats(band).ValidPixels();
        if (oStat->GetMStats(band).ValidPixels() >= m_mincnt) {
          m_overlapNorms[band-1]->AddOverlap(oStat->GetMStats(band).X(), overlapIndices[o][0],
                                             oStat->GetMStats(band).Y(), overlapIndices[o][1], weight);
        }
      }
    }
  }


  /**
   * @brief Sets solved state indicating if OverlapNormalizations (corrective factors) were solved
   *
   * @param bool Indiciates if corrective factors were solved
   */
  void Equalization::setSolved(bool solved) {
    m_normsSolved = solved;
  }


  /**
   * @brief Indicates if the corrective factors were solved
   *
   * @return @b bool Returns true if corrective factors were solved
   */
  bool Equalization::isSolved() const {
    return m_normsSolved;
  }


  /**
   * @brief Initializes member variables to default values
   */
  void Equalization::init() {
    m_validCnt = 0;
    m_invalidCnt = 0;

    m_mincnt = 1000;
    m_samplingPercent = 100.0;
    m_wtopt = false;

    m_maxCube = 0;
    m_maxBand = 0;

    m_sType = OverlapNormalization::Both;
    m_lsqMethod = LeastSquares::SPARSE;

    m_badFiles.clear();
    m_doesOverlapList.clear();
    m_alreadyCalculated.clear();
    m_normsSolved = false;
    m_recalculating = false;

    m_results = NULL;
  }


  /**
   * @brief Validates the input statistics pvl file
   *
   * This method determines if each input image has a corresponding set of corrective factors (i.e.
   * a normalization group). Throws an exception if this condition is not met.
   *
   * @param instatsFileName Name of the input statistics pvl file
   *
   * @return QVector<int> The indices of the corrective factors for the input images
   *
   * @throws IException::User "Each input file in the FROM LIST must have a corresponding input
   *                           file in the INPUT STATISTICS."
   * @throws IException::User "The from list file does not have any corresponding file in the
   *                           stats list."
   */
  QVector<int> Equalization::validateInputStatistics(QString instatsFileName) {
    QVector<int> normIndices;

    Pvl inStats(instatsFileName.toStdString());
    PvlObject &equalInfo = inStats.findObject("EqualizationInformation");

    // Make sure each file in the instats matches a file in the fromlist
    if (m_imageList.size() > equalInfo.groups() - 1) {
      std::string msg = "Each input file in the FROM LIST must have a ";
      msg += "corresponding input file in the INPUT STATISTICS.";
      throw IException(IException::User, msg, _FILEINFO_);
    }

    // Check that each file in the FROM LIST is present in the INPUT STATISTICS
    for (int i = 0; i < m_imageList.size(); i++) {
      QString fromFile = QString::fromStdString(m_imageList[i].original());
      bool foundFile = false;
      for (int j = 1; j < equalInfo.groups(); j++) {
        PvlGroup &normalization = equalInfo.group(j);
        QString normFile  = QString::fromStdString(normalization["FileName"][0]);
        if (fromFile == normFile) {

          // Store the index in INPUT STATISTICS file corresponding to the
          // current FROM LIST file
          normIndices.push_back(j);
          foundFile = true;
        }
      }
      if (!foundFile) {
        std::string msg = "The from list file [" + fromFile.toStdString() +
                          "] does not have any corresponding file in the stats list.";
        throw IException(IException::User, msg, _FILEINFO_);
      }
    }

    return normIndices;
  }


  void Equalization::CalculateFunctor::operator()(Buffer &in) const {
    // Make sure we consider the last line
    if ((in.Line() - 1) % m_linc == 0 || in.Line() == in.LineDimension()) {
      addStats(in);
    }
  }


  void Equalization::CalculateFunctor::addStats(Buffer &in) const {
    // Add data to Statistics object by line
    m_stats->AddData(&in[0], in.size());
  }


  void Equalization::ApplyFunctor::operator()(Buffer &in, Buffer &out) const {
    int index = in.Band() - 1;
    for (int i = 0; i < in.size(); i++) {
      out[i] = (IsSpecial(in[i])) ?
        in[i] : m_adjustment->evaluate(in[i], index);
    }
  }


}
