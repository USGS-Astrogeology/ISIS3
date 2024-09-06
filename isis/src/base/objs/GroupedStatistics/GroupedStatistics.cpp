/** This is free and unencumbered software released into the public domain.
The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */
#include "GroupedStatistics.h"
#include "Statistics.h"

#include <QMap>
#include <QVector>
#include <QString>

namespace Isis {

  //! construct a GroupedStatistics object
  GroupedStatistics::GroupedStatistics() {
    groupedStats = NULL;
    groupedStats = new QMap< QString, Statistics >();
  }


  /**
   *  copy construct a GroupedStatistics object
   *
   *  @param other The GroupedStatistics to copy
   */
  GroupedStatistics::GroupedStatistics(const GroupedStatistics &other) {
    groupedStats = NULL;
    groupedStats = new QMap< QString, Statistics >(*other.groupedStats);
  }


  //! destroy a GroupedStatistics object
  GroupedStatistics::~GroupedStatistics() {
    if(groupedStats) {
      delete groupedStats;
      groupedStats = NULL;
    }
  }


  /**
   *  Add a new data entry for a given type of data
   *
   *  @param statType Type of data (GoodnessOfFit for example)
   *
   *  @param newStat New statistical data to be added
   *
   */
  void GroupedStatistics::AddStatistic(const QString &statType, const
                                       double &newStat) {
    (*groupedStats)[statType].AddData(newStat);
  }


  /**
   *  Get statistics for a given type of data
   *
   *  @param statType Type of data
   *
   *  @returns Statisticts for the given type of data
   *
   *  @throws IException When the given type of data does not exist
   */
  const Statistics &GroupedStatistics::GetStatistics(const QString &statType)
  const {
    QMap< QString, Statistics >::const_iterator i;
    i = groupedStats->constFind(statType);

    if(i == groupedStats->constEnd()) {
      std::string msg = statType.toStdString();
      msg += " passed to GetStats but does not exist within the map";
      throw IException(IException::Programmer, msg, _FILEINFO_);
    }

    return i.value();
  }


  /**
   *  Return a list of all the different statistic tyes that this
   *  GroupedStatistics has
   *
   *  @returns A list of statistic types that this GroupedStatistics has
   */
  const QVector< QString > GroupedStatistics::GetStatisticTypes()
  const {
    QVector< QString > statTypes;

    // for each key in the groupedStats QMap add the key to a vector
    QMap< QString, Statistics >::const_iterator i = groupedStats->constBegin();
    while(i != groupedStats->constEnd()) {
      statTypes.push_back(i.key());
      i++;
    }

    return statTypes;
  }


  /**
   *  Assign a GroupedStatistics with another GroupedStatistics using =
   *
   *  @param other The GroupedStatistics to copy
   */
  GroupedStatistics &GroupedStatistics::operator=(const GroupedStatistics
      & other) {
    delete groupedStats;
    groupedStats = NULL;

    groupedStats = new QMap< QString, Statistics >(*other.groupedStats);

    return *this;
  }
}
