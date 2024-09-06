/** This is free and unencumbered software released into the public domain.
The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */

#include "SpectralDefinition2D.h"

#include <cassert>
#include <QList>

#include "ProcessBySample.h"
#include "Spectel.h"

namespace Isis {

  /**
   * @brief Construct a SpectralDefinition2D object using a
   *        filename. Isis Cubes are the only supported format.
   *
   * @author 2015-05021 Kristin Berry
   *
   * @param smileDefFilename the filename of the spectral definition
   */
  SpectralDefinition2D::SpectralDefinition2D  (FileName smileDefFilename) {
    ProcessBySample importCube;
    CubeAttributeInput cai;
    m_spectelList = NULL;
    m_sectionList = NULL;
    Cube *smileCube = importCube.SetInputCube(QString::fromStdString(smileDefFilename.expanded()), cai);

    try {
      // Check to see if input definition has the correct format
      if (smileCube->lineCount() != 2) {
        std::string msg = "Input calibration file [" + smileDefFilename.toString() + "] must have 2 lines: "
                                  "one containing wavelength centers and one containing widths";
        throw IException(IException::User, msg, _FILEINFO_);
      }

      m_spectelList = new QList< QList<Spectel> *>;
      m_sectionList = new QList<int>;

      m_ns = smileCube->sampleCount();
      m_nl = 0;
      m_nb = smileCube->bandCount();

      m_sectionList->append(1); //first section will always be band 1
      importCube.SetProcessingDirection(ProcessByBrick::BandsFirst);
      importCube.Progress()->SetText("Importing Spectral Definition Cube");
      importCube.ProcessCubeInPlace(*this, false);
      importCube.Finalize();

      m_numSections = m_sectionList->length();

    } catch (IException &e) {
      //delete dynamically allocated memory, since destructor won't be called.
      if (m_spectelList != NULL) {
        delete m_spectelList;
      }
      if (m_sectionList != NULL) {
        delete m_sectionList;
      }
      std::string msg = "Unable to open input cube [" + smileDefFilename.toString() + "] and read it into a spectral definition.";
      throw IException(e, IException::Unknown, msg, _FILEINFO_);
    }
  }

  int SpectralDefinition2D::sectionCount() const {
    return m_numSections;
  }


  //! returns section number given (s,l,b)
  int SpectralDefinition2D::sectionNumber(int s, int l, int b) const {
    if (m_sectionList->contains(b)) {
      return m_sectionList->indexOf(b);
    }
    else if (m_sectionList->size() == 1) {
      return 0;
    }
    else {
      for (int i=0; i<m_sectionList->size()-1; i++ ) {
        if ( (b > m_sectionList->at(i)) && (b < m_sectionList->at(i+1)) ) {
          return i;
        }
      }
      return m_sectionList->size() - 1;
     }
  }


  //! destructor
  SpectralDefinition2D::~SpectralDefinition2D  () {
    delete m_spectelList;
    delete m_sectionList;
  }


  /**
   * @brief Get the Spectel at some sample, line, band (associated with your input/calibration file)
   *
   * @author 2015-05-21 Kristin Berry
   *
   * @param sample input sample
   * @param line input line. Not used for a 2D definition
   * @param band input band
   *
   * @return Spectel
   */
  Spectel SpectralDefinition2D::findSpectel(const int sample, const int line,
                                            const int band) const {
    // There is no DN, since the imported image's DNs are centers and widths

    // TODO:
    // Test the QList bounds ourselves and throw an ISIS exception if not valid. QList.at does
    // not throw if invalid. It aborts using assert
    return m_spectelList->at(sample - 1)->at(band - 1);
  }


  Spectel SpectralDefinition2D::findSpectelByWavelength(const double wavelength,
                                                        const int sectionNumber) const {
    // Can not search a 2D definistion with only one value (i.e. wavelength), so abort
    // NOTE: This is ignored for code coverage until a good way to test it is found
    //       May not want to use the assert, but this function should not be called

    assert(0);
    return Spectel();
  }

  // TODO: If the requested wavelength is outside all the definitioin wavelength we should do something
  Spectel SpectralDefinition2D::findSpectel(const Spectel &inSpectel,
                                            const int sectionNumber) const {
    double diff;
    double bestdiff = DBL_MAX;
    double bestband = 0;
    double wavelength = inSpectel.centerWavelength();
    QList<Spectel> *spectrum = m_spectelList->at(inSpectel.sample()-1);

    //only search in correct section
    int start = m_sectionList->at(sectionNumber) - 1; //bands are 1-indexed, but are stored as 0-indexed
    int end;
    if (sectionNumber == m_sectionList->size()-1) { //we're in the last section
      end = spectrum->length();
    }
    else {
      end = m_sectionList->at(sectionNumber + 1) - 1;
    }

    for (int i=start; i<end; i++) {
      diff = spectrum->at(i).centerWavelength() - wavelength;
      if (std::abs(diff) < std::abs(bestdiff)) {
        bestdiff = diff;
        bestband = i;
      }
    }

    // TODO: QList aborts if at(arg) is out of bounds do the necessary error check and throw if needed
    return spectrum->at(bestband);
  }

  /**
  * @brief Returns QString representation of SpectralDefinition2D
  *
  * @author Kristin L Berry
  *
  * @return QString String representation of SpectralDefinition2D
  */
  QString SpectralDefinition2D::toString(){
    QString temp;
    for (int samp=0; samp<m_spectelList->size(); samp++) {
      for(int band=0; band<m_spectelList->at(samp)->size(); band++){
        Spectel spec = m_spectelList->at(samp)->at(band);
        temp+="Spectel at (s,b) (";
        temp+= QString::number(samp);
        temp+= ", ";
        temp+= QString::number(band);
        temp+=") : Wavelength=";
        temp+=QString::number(spec.centerWavelength());
        temp+=" Width=";
        temp+=QString::number(spec.filterWidth());
        temp+="\n";
      }
    }
    return temp;
  }


  // TODO: Test for equal wavelengths within a single section (currently ignored)
  //! pull information out of the input cube and store it.
  //! Each buffer contains two pixels, one center and one width
  void SpectralDefinition2D::operator()(Buffer &in) const {

    // Each time we come back to the first band allocate space for this new spectrum of spectels
    if (in.Band() == 1) {
      QList<Spectel> *tempList = new QList<Spectel>;
      m_spectelList->append(tempList);
    }

    // Store the spectel
    Spectel temp(in.Sample(), Isis::NULL8, in.Band(), Isis::NULL8, in[0], in[1]);
    m_spectelList->at(in.Sample()-1)->append(temp);

    // Check for sections (change in wavelength direction) in the first spectrum only
    // TODO: Consider doing this for all spectrums as a saftey check
    // The first two spectels define the initial wavelength direction.
    if (in.Sample() == 1) {

      QList<Spectel> *spectrum = m_spectelList->at(in.Sample()-1);
      int index = in.Band() - 1;

      if ((index > 1) &&                                      // We now have at least 3 saved
          (index != m_sectionList->last()) &&                 // Don't look for a switch until one after the most recent
          (((spectrum->at(index-2).centerWavelength() <
             spectrum->at(index-1).centerWavelength()) &&  // previous 2 are ascending
           (spectrum->at(index-1).centerWavelength() >
            spectrum->at(index).centerWavelength())) ||    // previous and this are descending
          ((spectrum->at(index-2).centerWavelength() >
            spectrum->at(index-1).centerWavelength()) &&   // previous 2 are descending
           (spectrum->at(index-1).centerWavelength() <
            spectrum->at(index).centerWavelength())))) {   // previous and this are ascending

        m_sectionList->append(in.Band());
      }
    }
  }
}
