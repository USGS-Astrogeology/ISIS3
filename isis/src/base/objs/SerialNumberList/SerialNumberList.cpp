/** This is free and unencumbered software released into the public domain.
The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */
#include "SerialNumberList.h"

#include <QString>

#include "IException.h"
#include "FileList.h"
#include "FileName.h"
#include "ObservationNumber.h"
#include "Progress.h"
#include "Pvl.h"
#include "SerialNumber.h"

namespace Isis {
  /**
   * Creates an empty SerialNumberList
   *
   * @param checkTarget Specifies whether or not to check to make sure the target names
   *                    match between files added to the serialnumber list
   */
  SerialNumberList::SerialNumberList(bool checkTarget) {
    m_checkTarget = checkTarget;
    m_target.clear();
  }


  /**
   * Creates a SerialNumberList from a list of filenames
   *
   * @param listfile The list of files to be given serial numbers
   * @param checkTarget Specifies whether or not to check to make sure the target names
   *                    match between files added to the serialnumber list
   * @param progress Monitors progress of serial number creation
   *
   * @throws IException::User "Can't open or invalid file list"
   *
   * @internal
   *   @history 2009-10-20 Jeannie Walldren - Added Progress flag
   *   @history 2009-11-05 Jeannie Walldren - Modified number of maximum steps for Progress flag
   */
  SerialNumberList::SerialNumberList(const QString &listfile,
                                     bool checkTarget,
                                     Progress *progress) {
    m_checkTarget = checkTarget;
    m_target.clear();

    try {
      FileList flist(listfile.toStdString());
      if (progress != NULL) {
        progress->SetText("Creating Isis serial numbers from list file.");
        progress->SetMaximumSteps((int) flist.size() + 1);
        progress->CheckStatus();
      }
      for (int i = 0; i < flist.size(); i++) {
        add(QString::fromStdString(flist[i].toString()));
        if (progress != NULL) {
          progress->CheckStatus();
        }
      }
    }
    catch (IException &e) {
      std::string msg = "Can't open or invalid file list [" + listfile.toStdString() + "].";
      throw IException(e, IException::User, msg, _FILEINFO_);
    }

  }


  /**
   * Destructor
   */
  SerialNumberList::~SerialNumberList() {
  }


  /**
   * Remove the specified serial number from the list
   *
   * @param sn Name of serial number to remove
   *
   * @author 2010-09-09 Sharmila Prasad
   */
  void SerialNumberList::remove(const QString &sn) {
    int index = serialNumberIndex(sn);
    QString sFileName = fileName(sn);

    // Delete the reference to this serial number in the vector and the maps
    m_pairs.erase(m_pairs.begin() + index);
    m_serialMap.erase(sn);
    m_fileMap.erase(sFileName);
  }


  /**
   * Adds a new filename / serial number pair to the SerialNumberList
   *
   * @param filename The filename to be added
   * @param def2filename If a serial number could not be found, try to return the filename
   *
   * @throws IException::User "Unable to find Instrument or Mapping group for comparing target."
   * @throws IException::User "Unable to find Instrument group for comparing target."
   * @throws IException::User "Target name from file does not match."
   * @throws IException::User "Invalid serial number [Unknown] from file."
   * @throws IException::User "Duplicate serial number from files [file1] and [file2]."
   * @throws IException::User "FileName cannot be added to serial number list."
   *
   * @internal
   *   @history 2007-06-04 Tracie Sucharski - Expand filename to include full path before adding
   *                           to list.
   *   @history 2010-11-24 Tracie Sucharski - Added bool def2filename parameter. This will allow
   *                           level 2 images to be added to a serial number list.
   *   @history 2010-11-29 Tracie Sucharski - Only read the Instrument group if m_checkTarget is
   *                           True.  If def2filename is True, check for Mapping group if Target
   *                           does not exist.
   */
  void SerialNumberList::add(const QString &filename, bool def2filename) {
    Pvl p(Isis::FileName(filename.toStdString()).expanded());
    PvlObject cubeObj = p.findObject("IsisCube");

    try {

      // Test the target name if desired
      if (m_checkTarget) {
        QString target;
        PvlGroup targetGroup;
        if (cubeObj.hasGroup("Instrument")) {
          targetGroup = cubeObj.findGroup("Instrument");
        }
        else if (def2filename) {
          // No instrument, try Mapping
          if (cubeObj.hasGroup("Mapping")) {
            targetGroup = cubeObj.findGroup("Mapping");
          }
          else {
            std::string msg = "Unable to find Instrument or Mapping group in "
                          + filename.toStdString() + " for comparing target.";
            throw IException(IException::User, msg, _FILEINFO_);
          }
        }
        else {
          // No Instrument group
          std::string msg = "Unable to find Instrument group in " + filename.toStdString()
                        + " for comparing target.";
          throw IException(IException::User, msg, _FILEINFO_);
        }

        target = QString::fromStdString(targetGroup["TargetName"][0]);
        target = target.toUpper();
        if (m_target.isEmpty()) {
          m_target = target;
        }
        else if (m_target != target) {
          std::string msg = "Target name of [" + target.toStdString() + "] from file ["
                        + filename.toStdString() + "] does not match [" + m_target.toStdString() + "].";
          throw IException(IException::User, msg, _FILEINFO_);
        }
      }

      // Create the SN
      QString sn = SerialNumber::Compose(p, def2filename);
      QString on = ObservationNumber::Compose(p, def2filename);
      if (sn == "Unknown") {
        std::string msg = "Invalid serial number [Unknown] from file ["
                      + filename.toStdString() + "].";
        throw IException(IException::User, msg, _FILEINFO_);
      }
      else if (hasSerialNumber(sn)) {
        int index = serialNumberIndex(sn);
        std::string msg = "Duplicate serial number ["+ sn.toStdString() + "] from files ["
                      + SerialNumberList::fileName(sn).toStdString() + "] and [" + fileName(index).toStdString() + "].";
        throw IException(IException::User, msg, _FILEINFO_);
      }

      Pair nextpair;
      nextpair.filename = QString::fromStdString(Isis::FileName(filename.toStdString()).expanded());
      nextpair.serialNumber = sn;
      nextpair.observationNumber = on;

      // If a CSM cube, obtain the CSMPlatformID and CSMInstrumentId from the CsmInfo
      // group for use in bundle adjustment
      if (cubeObj.hasGroup("CsmInfo")) {
        PvlGroup csmGroup = cubeObj.findGroup("CsmInfo");
        if (csmGroup.hasKeyword("CSMPlatformID") && csmGroup.hasKeyword("CSMInstrumentId")) {
          nextpair.spacecraftName = QString::fromStdString(cubeObj.findGroup("CsmInfo")["CSMPlatformID"][0]);
          nextpair.instrumentId = QString::fromStdString(cubeObj.findGroup("CsmInfo")["CSMInstrumentId"][0]);
        }
      }

      // Otherwise obtain the SpacecraftName and InstrumentId from the Instrument
      // group for use in bundle adjustment
      else if (cubeObj.hasGroup("Instrument")) {
        PvlGroup instGroup = cubeObj.findGroup("Instrument");
        if (instGroup.hasKeyword("SpacecraftName") && instGroup.hasKeyword("InstrumentId")) {
          nextpair.spacecraftName = QString::fromStdString(cubeObj.findGroup("Instrument")["SpacecraftName"][0]);
          nextpair.instrumentId = QString::fromStdString(cubeObj.findGroup("Instrument")["InstrumentId"][0]);
        }
      }

      m_pairs.push_back(nextpair);
      m_serialMap.insert(std::pair<QString, int>(sn, (int)(m_pairs.size() - 1)));
      m_fileMap.insert(std::pair<QString, int>(nextpair.filename, (int)(m_pairs.size() - 1)));
    }
    catch (IException &e) {
      std::string msg = "FileName [" + Isis::FileName(filename.toStdString()).expanded() +
                        "] can not be added to serial number list.";
      throw IException(e, IException::User, msg, _FILEINFO_);
    }

  }


  /**
   * @brief Overloaded add method that takes char * parameters
   *
   * Adds a new filename / serial number pair to the SerialNumberList
   *
   * @param serialNumber The serial number to be added
   * @param filename The filename to be added
   *
   * @see add(QString, QString)
   */
  void SerialNumberList::add(const char *serialNumber, const char *filename) {
    add((QString)serialNumber, (QString)filename);
  }


  /**
   * Adds a new filename / and pre-composed serial number pair to the SerialNumberList
   *
   * @param serialNumber the serial number to be added
   * @param filename the filename to be added
   *
   * @throws IException::User "Unable to find Instrument or Mapping group for comparing target."
   * @throws IException::User "Target name from file does not match."
   * @throws IException::User "Invalid serial number [Unknown] from file."
   * @throws IException::User "Duplicate serial number from files [file1] and [file2]."
   * @throws IException::User "Unable to find Instrument group need for performing bundle
   *                           adjustment."
   * @throws IException::User "Unable to find Spacecraftname or InstrumentId keywords needed for
   *                           performing bundle adjustment."
   * @throws IException::User "[SerialNumber, FileName] can not be added to serial number list."
   *
   * @author 2012-07-12 Tracie Sucharski
   *
   */
  void SerialNumberList::add(const QString &serialNumber, const QString &filename) {
    Pvl p(Isis::FileName(filename.toStdString()).expanded());
    PvlObject cubeObj = p.findObject("IsisCube");

    try {

      // Test the target name if desired
      if (m_checkTarget) {
        QString target;
        PvlGroup targetGroup;
        if (cubeObj.hasGroup("Instrument")) {
          targetGroup = cubeObj.findGroup("Instrument");
        }
        else if (cubeObj.hasGroup("Mapping")) {
          // No instrument, try Mapping
          targetGroup = cubeObj.findGroup("Mapping");
        }
        else {
            std::string msg = "Unable to find Instrument or Mapping group in "
                          + filename.toStdString() + " for comparing target.";
            throw IException(IException::User, msg, _FILEINFO_);
        }

        target = QString::fromStdString(targetGroup["TargetName"][0]);
        target = target.toUpper();
        if (m_target.isEmpty()) {
          m_target = target;
        }
        else if (m_target != target) {
          std::string msg = "Target name of [" + target.toStdString() + "] from file ["
                        + filename.toStdString() + "] does not match [" + m_target.toStdString() + "].";
          throw IException(IException::User, msg, _FILEINFO_);
        }
      }

      QString observationNumber = "Unknown";
      if (serialNumber == "Unknown") {
        std::string msg = "Invalid serial number [Unknown] from file ["
                      + filename.toStdString() + "].";
        throw IException(IException::User, msg, _FILEINFO_);
      }
      else if (hasSerialNumber(serialNumber)) {
        int index = serialNumberIndex(serialNumber);
        std::string msg = "Duplicate, serial number [" + serialNumber.toStdString() + "] from files ["
                      + SerialNumberList::fileName(serialNumber).toStdString()
                      + "] and [" + fileName(index).toStdString() + "].";
        throw IException(IException::User, msg, _FILEINFO_);
      }

      // Check if it is not a CSM label before checking SpacecraftName and InstrumentId
      if (!cubeObj.hasGroup("CsmInfo")) {

        // Need to obtain the SpacecraftName and InstrumentId from the Instrument
        // group for use in bundle adjustment
        if (!cubeObj.hasGroup("Instrument")) {
          std::string msg = "Unable to find Instrument group in " + filename.toStdString()
                        + " needed for performing bundle adjustment.";
          throw IException(IException::User, msg, _FILEINFO_);
        }
        PvlGroup instGroup = cubeObj.findGroup("Instrument");
        if (!instGroup.hasKeyword("SpacecraftName") || !instGroup.hasKeyword("InstrumentId")) {
          std::string msg = "Unable to find SpacecraftName or InstrumentId keywords in " + filename.toStdString()
                        + " needed for performing bundle adjustment.";
          throw IException(IException::User, msg, _FILEINFO_);
        }
      }
      // Check if CSM label has CSMPlatformID and CSMInstrumentId
      else {
        PvlGroup csmGroup = cubeObj.findGroup("CSMInfo");
        if (!csmGroup.hasKeyword("CSMPlatformID") || !csmGroup.hasKeyword("CSMInstrumentId")) {
          std::string msg = "Unable to find CSMPlatformID or CSMInstrumentId keywords in " + filename.toStdString()
                        + " needed for performing bundle adjustment.";
          throw IException(IException::User, msg, _FILEINFO_);
        }
      }

      Pair nextpair;
      nextpair.filename = QString::fromStdString(Isis::FileName(filename.toStdString()).expanded());
      nextpair.serialNumber = serialNumber;
      nextpair.observationNumber = observationNumber;

      // If a CSM cube, obtain the CSMPlatformID and CSMInstrumentId from the CsmInfo
      // group for use in bundle adjustment
      if (cubeObj.hasGroup("CsmInfo")) {
        PvlGroup csmGroup = cubeObj.findGroup("CsmInfo");
        if (csmGroup.hasKeyword("CSMPlatformID") && csmGroup.hasKeyword("CSMInstrumentId")) {
          nextpair.spacecraftName = QString::fromStdString(cubeObj.findGroup("CsmInfo")["CSMPlatformID"][0]);
          nextpair.instrumentId = QString::fromStdString(cubeObj.findGroup("CsmInfo")["CSMInstrumentId"][0]);
        }
      }

      // Otherwise obtain the SpacecraftName and InstrumentId from the Instrument
      // group for use in bundle adjustment
      else if (cubeObj.hasGroup("Instrument")) {
        PvlGroup instGroup = cubeObj.findGroup("Instrument");
        if (instGroup.hasKeyword("SpacecraftName") && instGroup.hasKeyword("InstrumentId")) {
          nextpair.spacecraftName = QString::fromStdString(cubeObj.findGroup("Instrument")["SpacecraftName"][0]);
          nextpair.instrumentId = QString::fromStdString(cubeObj.findGroup("Instrument")["InstrumentId"][0]);
        }
      }

      m_pairs.push_back(nextpair);
      m_serialMap.insert(std::pair<QString, int>(serialNumber, (int)(m_pairs.size() - 1)));
      m_fileMap.insert(std::pair<QString, int>(nextpair.filename, (int)(m_pairs.size() - 1)));
    }
    catch (IException &e) {
      std::string msg = "[SerialNumber, FileName] = [" + serialNumber.toStdString() + ", "
                    + Isis::FileName(filename.toStdString()).expanded()
                    + "] can not be added to serial number list.";
      throw IException(e, IException::User, msg, _FILEINFO_);
    }

  }


  /**
   * Determines whether or not the requested serial number exists in the list
   *
   * @param sn The serial number to be checked for
   *
   * @return @b bool
   */
  bool SerialNumberList::hasSerialNumber(QString sn) {
    if (m_serialMap.find(sn) == m_serialMap.end()) return false;
    return true;
  }


  /**
   * How many serial number / filename combos are in the list
   *
   * @return @b int Returns number of serial numbers current in the list
   */
  int SerialNumberList::size() const {
    return m_pairs.size();
  }


  /**
   * Return a filename given a serial number
   *
   * @param sn The serial number of the desired filename
   *
   * @throws IException::Programmer "Unable to get the FileName. The given serial number does not
   *                                 exist in the list."
   *
   * @return @b QString The filename matching the input serial number
   */
  QString SerialNumberList::fileName(const QString &sn) {
    if (hasSerialNumber(sn)) {
      int index = m_serialMap.find(sn)->second;
      return m_pairs[index].filename;
    }
    else {
      std::string msg = "Unable to get the FileName. The given serial number ["
                   + sn.toStdString() + "] does not exist in the list.";
      throw IException(IException::Programmer, msg, _FILEINFO_);
    }
  }


  /**
   * Return a serial number given a filename
   *
   * @param filename The filename to be matched
   *
   * @throws IException::Programmer "Unable to get the SerialNumber. The given file name does not
   *                                 exist in the list."
   *
   * @return @b QString The serial number corresponding to the input filename
   *
   * @internal
   *   @history 2007-06-04 Tracie Sucharski - Expand filename to include full path before searching
   *                           list.
   */
  QString SerialNumberList::serialNumber(const QString &filename) {
    if (m_fileMap.find(QString::fromStdString(Isis::FileName(filename.toStdString()).expanded())) == m_fileMap.end()) {
      std::string msg = "Unable to get the SerialNumber. The given file name ["
                    + Isis::FileName(filename.toStdString()).expanded() + "] does not exist in the list.";
      throw IException(IException::Programmer, msg, _FILEINFO_);
    }
    int index = fileNameIndex(filename);
    return m_pairs[index].serialNumber;
  }


  /**
   * Return a serial number given an index
   *
   * @param index The index of the desired serial number
   *
   * @throws IException::Programmer "Unable to get the SerialNumber. The given index is invalid."
   *
   * @return @b QString The serial number returned
   */
  QString SerialNumberList::serialNumber(int index) {
    if (index >= 0 && index < (int) m_pairs.size()) {
      return m_pairs[index].serialNumber;
    }
    else {
      std::string msg = "Unable to get the SerialNumber. The given index ["
                    + toString(index) + "] is invalid.";
      throw IException(IException::Programmer, msg, _FILEINFO_);
    }
  }


  /**
   * Return a observation number given an index
   *
   * @param index The index of the desired observation number
   *
   * @throws IException::Programmer "Unable to get the ObservationNumber. The given index is
   *                                 invalid."
   *
   * @return @b QString The observation number returned
   */
  QString SerialNumberList::observationNumber(int index) {
    if (index >= 0 && index < (int) m_pairs.size()) {
      return m_pairs[index].observationNumber;
    }
    else {
      std::string msg = "Unable to get the ObservationNumber. The given index ["
                    + toString(index) + "] is invalid.";
      throw IException(IException::Programmer, msg, _FILEINFO_);
    }
  }


  /**
   * Return a list index given a serial number
   *
   * @param sn The serial number searched for
   *
   * @throws IException::Programmer "Unable to get the SerialNumber index. The given serial number
   *                                 does not exist in the list."
   *
   * @return @b int The index of the serial number
   */
  int SerialNumberList::serialNumberIndex(const QString &sn) {
    if (hasSerialNumber(sn)) {
      return m_serialMap.find(sn)->second;
    }
    else {
      std::string msg = "Unable to get the SerialNumber index. The given serial number ["
                   + sn.toStdString() + "] does not exist in the list.";
      throw IException(IException::Programmer, msg, _FILEINFO_);
    }
  }


  /**
   * Return a list index given a filename
   *
   * @param filename The filename to be searched for
   *
   * @throws IException::Programmer "Unable to get the FileName index. The given file name does not
   *                                 exist in the list."
   *
   * @return @b int The index of the input filename
   *
   * @internal
   *   @history 2007-06-04 Tracie Sucharski - Expand filename to include full path before
   *                           searching list.
   *   @history 2007-07-11 Stuart Sides - Fixed bug where the correct index was not returned.
   */
  int SerialNumberList::fileNameIndex(const QString &filename) {
    std::map<QString, int>::iterator pos;
    if ((pos = m_fileMap.find(QString::fromStdString(Isis::FileName(filename.toStdString()).expanded()))) == m_fileMap.end()) {
      std::string msg = "Unable to get the FileName index. The given file name ["
                    + Isis::FileName(filename.toStdString()).expanded() + "] does not exist in the list.";
      throw IException(IException::Programmer, msg, _FILEINFO_);
    }
    return pos->second;
  }


  /**
   * Return the filename at the given index
   *
   * @param index The index of the desired filename
   *
   * @throws IException::Programmer "Unable to get the FileName. The given index is invalid."
   *
   * @return @b QString The filename at the given index
   */
  QString SerialNumberList::fileName(int index) {
    if (index >= 0 && index < (int) m_pairs.size()) {
      return m_pairs[index].filename;
    }
    else {
      std::string msg = "Unable to get the FileName. The given index ["
                    + toString(index) + "] is invalid.";
      throw IException(IException::Programmer, msg, _FILEINFO_);
    }
  }


  /**
   * Return the spacecraftname/instrumentid at the given index
   *
   * @param index The index of the desired spacecraftname/instrumentid
   *
   * @throws IException::Programmer "Unable to get the Spacecraft InstrumentId. The given index is
   *                                 invalid."
   *
   * @return @b QString The spacecraftname/instrumentid at the given index
   */
  QString SerialNumberList::spacecraftInstrumentId(int index) {
    if (index >= 0 && index < (int) m_pairs.size()) {
      QString scid = (m_pairs[index].spacecraftName + "/" + m_pairs[index].instrumentId).toUpper();

      // silence 'unused-result' warnings with arbitrary cast
      (void)scid.simplified();
      return scid.replace(" ","");
    }
    else {
      std::string msg = "Unable to get the Spacecraft InstrumentId. The given index ["
                    + toString(index) + "] is invalid.";
      throw IException(IException::Programmer, msg, _FILEINFO_);
    }
  }


  /**
   * Return the spacecraftname/instrumentid given a serial number
   *
   * @param sn  The serial number of the desired spacecraftname/instrumentid
   *
   * @throws IException::Programmer "Unable to get the Spacecraft InstrumentId. The given serial
   *                                 number does not exist in the list."
   *
   * @return @b QString The spacecraftname/instrumentid matching the input serial number
   */
  QString SerialNumberList::spacecraftInstrumentId(const QString &sn) {
    if (hasSerialNumber(sn)) {
      int index = m_serialMap.find(sn)->second;
      QString scid = (m_pairs[index].spacecraftName + "/" + m_pairs[index].instrumentId).toUpper();

      // silence 'unused-result' warnings with arbitrary cast
      (void)scid.simplified();
      return scid.replace(" ","");
    }
    else {
      std::string msg = "Unable to get the Spacecraft InstrumentId. The given serial number ["
                   + sn.toStdString() + "] does not exist in the list.";
      throw IException(IException::Programmer, msg, _FILEINFO_);
    }
  }


  /**
   * Return possible serial numbers given an observation number
   *
   * @param on The observation number of the possible serial number
   *
   * @throws IException::Programmer "Unable to get the possible serial numbers. The given
   *                                 observation number does not exist in the list."
   *
   * @return @b vector<QString> The list of possible serial numbers matching the input observation
   *                         number
   */
  std::vector<QString> SerialNumberList::possibleSerialNumbers(const QString &on) {
    std::vector<QString> numbers;
    for (unsigned index = 0; index < m_pairs.size(); index++) {
      if (m_pairs[index].observationNumber == on) {
        numbers.push_back(m_pairs[index].serialNumber);
      }
    }
    if (numbers.size() > 0) {
      return numbers;
    }
    else {
      std::string msg = "Unable to get the possible serial numbers. The given observation number ["
                    + on.toStdString() + "] does not exist in the list.";
      throw IException(IException::Programmer, msg, _FILEINFO_);
    }
  }


}
