/** This is free and unencumbered software released into the public domain.
The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */

#include <cmath>
#include <QList>
#include <QObject>

#include "SpectralDefinition1D.h"
#include "CSVReader.h"

namespace Isis {

  /**
   * @brief Constructs a SpectralDefinition1D, typically used as
   *        the target SpectralDefinition in the desmile
   *        application.
   *
   * @param smileDefFilename The filename for the source of the 1D
   *                         SpectralDefinition. Currently, only
   *                         CSVs are accepted as input.
   */
  SpectralDefinition1D::SpectralDefinition1D(FileName smileDefFilename) {
    m_spectelList = NULL;

    try {
      CSVReader csv(QString::fromStdString(smileDefFilename.toString()));

      if (csv.columns() != 2) {
        std::string msg = "Input calibration file [" + smileDefFilename.toString() + "] must have 2 columns with "
                                  "the format: wavelength centers, wavelength widths";
        throw IException(IException::User, msg, _FILEINFO_);
      }

      if (csv.rows() < 2) {
        std::string msg = "Input calibration file [" +  smileDefFilename.toString() + "] must have at least 2 lines.";
        throw IException(IException::User, msg, _FILEINFO_);
      }

      m_spectelList = new QList< QList<Spectel> *>;

      CSVReader::CSVAxis centers = csv.getColumn(0);
      CSVReader::CSVAxis widths = csv.getColumn(1);

      // Find wavelength change direction
      if (centers[0] < centers[1]) {
        m_ascendingWavelengths = true;
      } else{
        m_ascendingWavelengths = false;
      }

      // Determine number of sections and populate internal data storage
      double lastWavelength = centers[0].toDouble();
      QList<Spectel> *tempList = new QList<Spectel>;

      for (int i=0; i <csv.rows(); i++) {
        if (i >= 2) {
          //Do we change direction at this index?
           if (((lastWavelength > centers[i].toDouble()) && m_ascendingWavelengths) ||
               ((lastWavelength < centers[i].toDouble())&& !m_ascendingWavelengths)) {
            m_spectelList->append(tempList); // We have a comeplete section, so add it.
            tempList = new QList<Spectel>; // Create a new templist
          }
        }
        Spectel temp(Isis::NULL8, Isis::NULL8, i+1, Isis::NULL8, centers[i].toDouble(),
                     widths[i].toDouble());
        tempList->append(temp);
        lastWavelength = centers[i].toDouble();
      }

      m_spectelList->append(tempList);
      m_numSections = m_spectelList->length();

      //Set # of bands (no real line, samp dimensions)
      m_ns = 1;
      m_nl = 1;
      m_nb = csv.rows();

    } catch (IException &e) {
      //delete dynamically allocated memory, since destructor won't be called
      if (m_spectelList != NULL) {
        delete m_spectelList;
      }
      std::string msg = "Unable to open input file [" + smileDefFilename.toString() + "]. Is it a valid CSV?";
      throw IException(e, IException::Unknown, msg, _FILEINFO_);
    }
  }

  /** @brief Returns the QString representation of the SpectralDefinition1D.
   *
   * @author Kristin Berry
   */
  QString SpectralDefinition1D::toString(){
    QString temp;
    for (int i=0; i<m_spectelList->length(); i++){
      temp +="----Section ";
      temp +=QString::number(i);
      temp +="----\n";
      for(int j=0; j<m_spectelList->at(i)->length(); j++){
        temp+="Wavelength= ";
        temp+=QString::number(m_spectelList->at(i)->at(j).centerWavelength());
        temp+=", Width= ";
        temp+=QString::number(m_spectelList->at(i)->at(j).filterWidth());
        temp+="\n";
      }
    }
    return temp;
  }


  /**
   * @brief Returns the number of sections in this Spectral
   *        Definition
   *
   * @return int The number of sections in the SpectralDefinition
   */
  int SpectralDefinition1D::sectionCount() const {
    return m_numSections;
  }


  /**
   * @brief Returns the section number that a spectel is in.
   *
   * @param s Sample coordinate of spectel
   * @param l Line coordinate of spectel
   * @param b Band coordinate of spectel
   *
   * @return int The section number this spectel is in
   */
  int SpectralDefinition1D::sectionNumber(int s, int l, int b) const {
    int section = 0;

    while (b > m_spectelList->at(section)->length()) {
      b-=m_spectelList->at(section)->length();
      section++;
    }
    return section;
  }


  //! construct an empty 1D SpectralDef
  SpectralDefinition1D::SpectralDefinition1D() {
  }


  //! destructor
  SpectralDefinition1D::~SpectralDefinition1D() {
    delete m_spectelList;
  }


  /**
   * @brief Get the Spectel from this SpectralDefinition at a
   *        (s,l,b). This can be used to determine the center
   *        wavelength and filter width in a particular band.
   *
   *        If the (s,l,b) is not in the spectral definition,
   *        an empty Spectel is returned.
   *
   * @param sample Sample
   * @param line Line
   * @param band Band
   *
   * @return Spectel
   */
  Spectel SpectralDefinition1D::findSpectel(const int sample, const int line, const int band) const {
    int tempBand = band;
    if (m_numSections == 1) {
      return m_spectelList->at(0)->at(tempBand - 1);
    }
    else {
      for (int i=0; i<m_spectelList->length(); i++) {
        if (tempBand <= m_spectelList->at(i)->length()) {
          return m_spectelList->at(i)->at(tempBand - 1);
        }
        else {
          tempBand -= m_spectelList->at(i)->length();
        }
      }
    }
    return Spectel();
  }


  // TODO: This should eventually make sure the returned spectel is within the filter width
  /**
   * @brief Gets the Spectel from this SpectralDefinition at the
   *        same location (s,l,b) as the input Spectel.
   *
   * @author 2015-05-15 Kristin Berry
   *
   * @param inSpectel The input Spectel used to look up
   *                  information.
   * @param sectionNumber The SpectralDefinition's section number that the input Spectel is in.
   *
   * @return Spectel
   */
  Spectel SpectralDefinition1D::findSpectel(const Spectel &inSpectel, const int sectionNumber) const {
    return findSpectelByWavelength(inSpectel.centerWavelength(), sectionNumber);
  }


  //For now, find wavelength the input wavelength is closest to! (Nearest-neighbor)
  //Better alogrithm later? Interpolation?
  /**
   * @brief Finds the Spectel with the closest center wavelength
   *        (in the given sectionNumber) to the input wavelength.
   *
   * @author 2015-05-15 Kristin Berry
   *
   * @param wavelength The closest wavelength to this input
   *                   wavelength is searched for in the
   *                   SpectralDefinition.
   * @param sectionNumber The section number of the definition to
   *                      search in.
   * @return Spectel The Spectel with the closest center
   *         wavelength to the input wavelength.
   */
  Spectel SpectralDefinition1D::findSpectelByWavelength(double wavelength, const int sectionNumber)
    const {
    double diff;
    double bestDiff = DBL_MAX;
    double bestBand = -DBL_MAX;

    if (sectionNumber >= m_numSections) {
      std::string msg = QObject::tr("Input section number is greater than total number of sections.");
      throw IException(IException::User, msg, _FILEINFO_);
    }

    for (int i=0; i<m_spectelList->at(sectionNumber)->length(); i++) {
      diff = m_spectelList->at(sectionNumber)->at(i).centerWavelength() - wavelength;
      if (std::abs(diff) < std::abs(bestDiff)) {
        bestDiff = diff;
        bestBand = i;
      }
    }
    // TODO: QList abort if at arg is out of bounds do the necessary error check and throw if needed
    // This should never happen!
    if (bestBand == -DBL_MAX) {
      return Spectel(Isis::Null, Isis::Null, Isis::Null, Isis::Null, 0.0, 0.0);
    }
    else {
      return m_spectelList->at(sectionNumber)->at(bestBand);
    }
  }
}
