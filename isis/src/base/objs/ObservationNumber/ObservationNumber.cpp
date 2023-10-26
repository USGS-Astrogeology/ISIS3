/** This is free and unencumbered software released into the public domain.
The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */
#include "ObservationNumber.h"
#include "IException.h"
#include "Pvl.h"
#include "Cube.h"
#include "Process.h"
#include "PvlToPvlTranslationManager.h"
#include "FileName.h"

namespace Isis {
  /**
  * Create an empty SerialNumber object.
  */
  ObservationNumber::ObservationNumber() {}

  /**
   * Destroy a SerialNumber object.
  */
  ObservationNumber::~ObservationNumber() {}

  /**
   * Compose a ObservationNumber from a PVL.
   *
   * @param label A pvl formatted label to be used to generate the serial number
  */
  QString ObservationNumber::Compose(Pvl &label, bool def2filename) {
    QString sn;
    try {
      PvlGroup snGroup = FindObservationTranslation(label);
      sn = CreateSerialNumber(snGroup, (int)snGroup["ObservationKeys"]);
    }
    catch(IException &e) {
      if(def2filename) {
        //  Try to return the filename if it exists in the label, otherwise use
        //  "Unknown" as a last resort.
        QString snTemp = QString::fromStdString(label.fileName());
        if(!snTemp.isEmpty()) {
          sn = FileName(snTemp).name();
        }
        else {
          sn = "Unknown";
        }
      }
      else {
        sn = "Unknown";
      }
    }

    return sn;
  }

  /**
   * Compose a SerialNumber fPvlGroup snGrouprom a Cube.
   *
   * @param cube An opened Isis cube
   */
  QString ObservationNumber::Compose(Cube &cube, bool def2filename) {
    return Compose(*cube.label(), def2filename);
  }

  /**
   * Compose a SerialNumber from a file.
   *
   * @param filename a filename to open
   */
  QString ObservationNumber::Compose(const QString &filename, bool def2filename) {
    Pvl p(filename.toStdString());
    return Compose(p, def2filename);
  }

  /**
   * Get Groups by translating from correct Translation table
   *
   * @param label A pvl formatted label to be used to generate the serial number
   */
  PvlGroup ObservationNumber::FindObservationTranslation(Pvl &label) {
    Pvl outLabel;

    // Get the mission name
    static QString missionTransFile = "$ISISROOT/appdata/translations/MissionName2DataDir.trn";
    static PvlToPvlTranslationManager missionXlater(missionTransFile);
    missionXlater.SetLabel(label);
    QString mission = missionXlater.Translate("MissionName");

    // Get the instrument name
    static QString instTransFile = "$ISISROOT/appdata/translations/Instruments.trn";
    static PvlToPvlTranslationManager instrumentXlater(instTransFile);
    instrumentXlater.SetLabel(label);
    QString instrument = instrumentXlater.Translate("InstrumentName");

    // We want to use this instrument's translation manager. It's much faster for
    //   ObservationNumberList if we keep the translation manager in memory, so re-reading
    //   from the disk is not necessary every time. To do this, we'll use a map to store
    //   the translation managers and observation number keys with a string identifier to find them.
    //   This identifier needs to have the mission name and the instrument name.
    static std::map<QString, std::pair<PvlToPvlTranslationManager, PvlKeyword> > missionTranslators;
    QString key = mission + "_" + instrument;
    std::map<QString, std::pair<PvlToPvlTranslationManager, PvlKeyword> >::iterator
    translationIterator = missionTranslators.find(key);

    if(translationIterator == missionTranslators.end()) {
      // Get the file
      FileName snFile((QString) "$ISISROOT/appdata/translations/" + mission + 
                                 instrument + "SerialNumber.trn");

      // Delets the extra
      Pvl translation(snFile.expanded().toStdString());
      PvlKeyword observationKeys;
      if(translation.hasKeyword("ObservationKeys")) {
        observationKeys = translation["ObservationKeys"];
      }

      // use the translation file to generate keywords
      missionTranslators.insert(
        std::pair<QString, std::pair<PvlToPvlTranslationManager, PvlKeyword> >
        (key, std::pair<PvlToPvlTranslationManager, PvlKeyword>(PvlToPvlTranslationManager(snFile.expanded()), observationKeys))
      );

      translationIterator = missionTranslators.find(key);
    }

    translationIterator->second.first.SetLabel(label);
    translationIterator->second.first.Auto(outLabel);
    PvlGroup snGroup = outLabel.findGroup("SerialNumberKeywords");

    // Delets the extra
    if(!QString::fromStdString(translationIterator->second.second.name()).isEmpty()) {
      snGroup += translationIterator->second.second;
    }
    else {
      snGroup += PvlKeyword("ObservationKeys", std::to_string(snGroup.keywords()));
    }

    return snGroup;
  }

  /**
   * Creates a vector of plasible SerialNumbers from a string
   * representing the ObservationNumber and a SerialList.
   *
   * @param on the string representing the ObservationNumber
   *
   * @param list the SerialNumberList
   */
  std::vector<QString> ObservationNumber::PossibleSerial(const QString &on, SerialNumberList &list) {
    std::vector<QString> sn;
    for(int i = 0; i < list.size(); i++) {
      if(list.serialNumber(i).startsWith(on)) {
        sn.push_back(list.serialNumber(i));
      }
    }
    return sn;
  }



}
