/** This is free and unencumbered software released into the public domain.
The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */
#include "ObservationNumberList.h"

#include "FileName.h"
#include "IException.h"
#include "Pvl.h"
#include "SerialNumberList.h"

using namespace std;
namespace Isis {

  /**
   * Creates an ObservationNumberList from a filename
   *
   * @param listfile The list of files to be given observation
   *                 numbers
   * @param checkTarget Boolean value that specifies whether or not to check
   *                    to make sure the target names match between files added
   *                    to the observationnumber list
   */
  ObservationNumberList::ObservationNumberList(const QString &listfile, bool checkTarget) :
      SerialNumberList(listfile, checkTarget) {
    init(this);
  }


  /**
   * Creates an ObservationNumberList from a SerialNumberList
   *
   * @param snlist The serial number list from which to generate an observation number list
   */
  ObservationNumberList::ObservationNumberList(SerialNumberList *snlist) :
      SerialNumberList(*snlist) {
    init(snlist);
  }


  /**
   * Initiates the ObservationNumberList
   *
   * @param snlist The already created SerialNumberList used to
   *               create the ObservationNumberList object
   *
   * @throws IException::User "Serial numberList is empty"
   */
  void ObservationNumberList::init(SerialNumberList *snlist) {

    if (snlist->size() == 0) {
      std::string msg = "Serial number list is empty";
      throw IException(IException::User, msg, _FILEINFO_);
    }

    map<QString, int> observationMap;
    QString observationNumber;
    int currentIndex = 0;
    int observationIndex;

    // Fill the temporary map to generate observation sets
    for (int isn = 0; isn < snlist->size(); isn++) {
      observationNumber = snlist->observationNumber(isn);

      if (observationMap.find(observationNumber) == observationMap.end()) {
        observationMap.insert(pair<QString, int>(observationNumber, currentIndex));
        observationIndex = currentIndex++;
      }
      else {
        observationIndex = observationMap.find(observationNumber)->second;
      }

      add(isn, observationIndex, observationNumber);
    }
    m_numberObservations = currentIndex;
  }


  /**
   * Destructor
   */
  ObservationNumberList::~ObservationNumberList() {
  }


  /**
   * Removes all of the listed serial numbers from the observation
   *
   * @param snlist The list of SerialNumbers to remove
   *
   * @throws IException::User "Cannot remove, serial number list is empty"
   */
  void ObservationNumberList::remove(SerialNumberList *snlist) {

    if (snlist->size() == 0) {
      std::string msg = "Cannot remove, serial number list is empty";
      throw IException(IException::User, msg, _FILEINFO_);
    }

    m_sets.clear();
    m_indexMap.clear();

    map<QString, int> observationMap;
    QString observationNumber;
    int currentIndex = 0;
    int observationIndex;

    // Fill the temporary map to generate observation sets
    for (int isn = 0; isn < this->size(); isn++) {
      if ( (snlist->hasSerialNumber(this->serialNumber(isn))) ) {
        continue;
      }

      observationNumber = this->observationNumber(isn);

      if (observationMap.find(observationNumber) == observationMap.end()) {
        observationMap.insert(pair<QString, int>(observationNumber, currentIndex));
        observationIndex = currentIndex++;
      }
      else {
        observationIndex = observationMap.find(observationNumber)->second;
      }

      add(isn, observationIndex, observationNumber);
    }
    m_numberObservations = currentIndex;
  }


  /**
   * Removes all of the listed serial numbers from the observation
   *
   * @param listfile The list of SerialNumbers to remove
   */
  void ObservationNumberList::remove(const QString &listfile) {
    SerialNumberList snlist(listfile);
    remove(&snlist);
  }


  /**
   * Adds a new serial number index / observation number index / observation
   * number to the SerialNumberList
   *
   * @param isn The serial number index of the observation set to be added
   * @param observationIndex The observation number index of the observation set
   *                         to be added
   * @param observationNumber The observation number of the observation set to
   *                          be added
   *
   */
  void ObservationNumberList::add(const int isn, const int observationIndex, 
                                  QString observationNumber) {

    ObservationSet nextset;
    nextset.serialNumberIndex = isn;
    nextset.observationNumberIndex = observationIndex;
    nextset.observationNumber = observationNumber;

    m_sets.push_back(nextset);
    m_indexMap.insert(pair<int, int>(isn, observationIndex));
  }


  /**
   * How many unique observations are in the list?
   *
   * @return @b int Returns number of unique observations currently in the list
   */
  int ObservationNumberList::observationSize() const {
    return m_numberObservations;
  }


  /**
   * Determines whether or not the requested observation number
   * exists in the list
   *
   * @param on The observation number to be checked for
   *
   * @return @b bool
   */
  bool ObservationNumberList::hasObservationNumber(const QString &on) {
    for (unsigned index = 0; index < m_pairs.size(); index++) {
      if (m_pairs[index].observationNumber == on) {
        return true;
      }
    }
    return false;
  }


  /**
   * Return a observation index given a serial number index
   *
   * @param serialNumberIndex The index of the serial number to map
   *
   * @throws IException::Programmer "Serial Number Index is invalid"
   *
   * @return @b int The observation index mapped to the serial number
   */
  int ObservationNumberList::observationNumberMapIndex(int serialNumberIndex) {
    //    if (serialNumberIndex >= 0 && serialNumberIndex < (int) m_indexMap.size()) {
    if (serialNumberIndex >= 0) {
      return m_indexMap.find(serialNumberIndex)->second;
    }
    else {
      std::string msg = "Serial Number Index [" + toString(serialNumberIndex) + "] is invalid";
      throw IException(IException::Programmer, msg, _FILEINFO_);
    }
  }


  /**
   * Return an observation number given a filename
   *
   * @param filename The filename to be matched
   *
   * @throws IException::Programmer "Requested filename does not exist in the list"
   *
   * @return @b QString The observation number corresponding to
   *         the input filename
   */
  QString ObservationNumberList::observationNumber(const QString &filename) {
    if (m_fileMap.find(QString::fromStdString(FileName(filename.toStdString()).expanded())) == m_fileMap.end()) {
      std::string msg = "Requested filename [" + FileName(filename.toStdString()).expanded() + "] ";
      msg += "does not exist in the list";
      throw IException(IException::Programmer, msg, _FILEINFO_);
    }
    int index = fileNameIndex(filename);
    return m_pairs[index].observationNumber;
  }


  /**
   * Return a observation number given an index
   *
   * @param index The index of the desired observation number
   *
   * @throws IException::Programmer "Index is invalid"
   *
   * @return @b QString The observation number returned
   */
  QString ObservationNumberList::observationNumber(int index) {
    if (index >= 0 && index < (int) m_pairs.size()) {
      return m_pairs[index].observationNumber;
    }
    else {
      std::string msg = "Index [" + toString(index) + "] is invalid";
      throw IException(IException::Programmer, msg, _FILEINFO_);
    }
  }


  /**
   * Return possible filenames given an observation number
   *
   * @param on  The observation number of the desired filename
   *
   * @throws IException::Programmer "Requested observation number does not exist in the list"
   *
   * @return @b vector<QString> The list of possible filenames
   *            matching the input observation number
   */
  vector<QString> ObservationNumberList::possibleFileNames(const QString &on) {
    vector<QString> filenames;
    for (unsigned index = 0; index < m_pairs.size(); index++) {
      if (m_pairs[index].observationNumber == on) {
        filenames.push_back(m_pairs[index].filename);
      }
    }
    if (filenames.size() > 0) {
      return filenames;
    }
    else {
      std::string msg = "Requested observation number [" + on + "] ";
      msg += "does not exist in the list";
      throw IException(IException::Programmer, msg, _FILEINFO_);
    }
  }

}
