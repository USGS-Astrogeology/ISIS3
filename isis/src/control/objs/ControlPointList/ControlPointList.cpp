/** This is free and unencumbered software released into the public domain.

The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */

#include "ControlPointList.h"

#include <QList>

#include "IException.h"
#include "FileList.h"
#include "FileName.h"
#include "IString.h"

namespace Isis {
  /**
   * Creates a ControlPointList from a list of control point ids'
   *
   * @param psListFile The file withe list of control point ids'
   */
  ControlPointList::ControlPointList(const FileName &psListFile) {
    try {
      QList<QString> qList;
      FileList list(psListFile);
      int size = list.size();
      for(int i = 0; i < size; i++) {
        qList.insert(i, QString::fromStdString(list[i].toString()));
        mbFound.push_back(false);
      }
      mqCpList = QStringList(qList);

      //sort the list for faster searches - internally uses std::sort()
      mqCpList.sort();
    }
    catch(IException &e) {
      std::string msg = "Can't open or invalid file list [" +
          psListFile.expanded() + "]";
      throw IException(e, IException::User, msg, _FILEINFO_);
    }
  }

  /**
   * Destructor
   */
  ControlPointList::~ControlPointList() {
  }


  /**
   * Determines whether or not the requested control point id
   * exists in the list
   *
   * @param psCpId The control point id to be checked for
   *
   * @return bool
   */
  bool ControlPointList::HasControlPoint(const QString &psCpId) {
    int index = mqCpList.indexOf(QString(psCpId));

    if(index == -1 || index >= Size())
      return false;

    mbFound[index] = true;
    return true;
  }


  /**
   * How many control points in the list
   *
   * @return int Returns number of control point in the list
   */
  int ControlPointList::Size() const {
    return mqCpList.size();
  }


  /**
   * Return a control point id given an index
   *
   * @param piIndex The index of the desired control point id
   *
   * @return QString The control point id returned
   */
  QString ControlPointList::ControlPointId(int piIndex) {
    int size = Size();
    if(piIndex >= 0 && piIndex < size) {
      return (mqCpList.value(piIndex));
    }
    else {
      QString num = toString(piIndex);
      std::string msg = "Index [" + num + "] is invalid";
      throw IException(IException::Programmer, msg, _FILEINFO_);
    }
  }

  /**
   * return a list index given a control point id
   *
   * @param ps_cpId The control point id to be searched for
   *
   * @return int The index of the control point id
   */
  int ControlPointList::ControlPointIndex(const QString &psCpId) {
    if(HasControlPoint(psCpId)) {
      return mqCpList.indexOf(QString(psCpId));
    }
    else {
      std::string msg = "Requested control point id [" + psCpId + "] ";
      msg += "does not exist in the list";
      throw IException(IException::Programmer, msg, _FILEINFO_);
    }
  }

  /**
   * Register invalid control point and calculate the valid &
   * invalid point count
   *
   * @param Pvl  Will contain stats in pvl keywords
   */
  void ControlPointList::RegisterStatistics(Pvl &pcPvlLog) {
    int size = Size();
    int iNotFound = 0;
    QString sPointsNotFound = "";

    for(int i = 0; i < size; i++) {
      if(!mbFound[i]) {
        if(iNotFound) {
          sPointsNotFound += ", ";
        }
        sPointsNotFound += mqCpList.value(i);
        iNotFound++;
      }
    }

    pcPvlLog += Isis::PvlKeyword("TotalPoints", std::to_string(size));
    pcPvlLog += Isis::PvlKeyword("ValidPoints", std::to_string(size - iNotFound));
    pcPvlLog += Isis::PvlKeyword("InvalidPoints", std::to_string(iNotFound));
    pcPvlLog += Isis::PvlKeyword("InvalidPointIds", sPointsNotFound.toStdString());
  }
}
