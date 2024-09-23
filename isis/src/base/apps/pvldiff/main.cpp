#include "Isis.h"

#include <cmath>
#include <float.h>

#include <QRegularExpression>

#include "IException.h"
#include "PvlContainer.h"
#include "Pvl.h"

using namespace std;
using namespace Isis;

bool filesMatch;
QString differenceReason;
PvlGroup tolerances;
PvlGroup ignoreKeys;
PvlGroup ignoreFilePaths;

void compareKeywords(PvlKeyword &pvl1, PvlKeyword &pvl2);
void compareObjects(PvlObject &pvl1, PvlObject &pvl2);
void compareGroups(PvlGroup &pvl1, PvlGroup &pvl2);
void removeIngoredKeys(PvlContainer &pvl1, PvlContainer &pvl2);
  
void IsisMain() {
  UserInterface &ui = Application::GetUserInterface();

  tolerances = PvlGroup();
  ignoreKeys = PvlGroup();
  ignoreFilePaths = PvlGroup();
  differenceReason = "";
  filesMatch = true;

  Pvl file1(ui.GetFileName("FROM").toStdString());
  Pvl file2(ui.GetFileName("FROM2").toStdString());

  if ( ui.WasEntered("DIFF") ) {
    Pvl diffFile(ui.GetFileName("DIFF").toStdString());

    if ( diffFile.hasGroup("Tolerances") ) {
      tolerances = diffFile.findGroup("Tolerances");
    }

    if (diffFile.hasGroup("IgnoreKeys") ) {
      ignoreKeys = diffFile.findGroup("IgnoreKeys");
    }

    if (diffFile.hasGroup("IgnoreFilePaths") ) {
      ignoreFilePaths = diffFile.findGroup("IgnoreFilePaths");
    }
  }

  compareObjects(file1, file2);

  PvlGroup differences("Results");
  if (filesMatch) {
    differences += PvlKeyword("Compare", "Identical");
  }
  else {
    differences += PvlKeyword("Compare", "Different");
    differences += PvlKeyword("Reason", differenceReason.toStdString());
  }

  Application::Log(differences);

  if ( ui.WasEntered("TO") ) {
    Pvl out;
    out.addGroup(differences);
    out.write(ui.GetFileName("TO").toStdString());
  }

  differenceReason = "";
}

void compareKeywords(PvlKeyword &pvl1, PvlKeyword &pvl2) { 
  if ( QString::fromStdString(pvl1.name()).compare(QString::fromStdString(pvl2.name())) != 0 ) {
    filesMatch = false;
    differenceReason = "Keyword '" + QString::fromStdString(pvl1.name()) + "' does not match keyword '" + QString::fromStdString(pvl2.name()) + "'";
  }

  if ( pvl1.size() != pvl2.size() ) {
    filesMatch = false;
    differenceReason = "Keyword '" + QString::fromStdString(pvl1.name()) + "' size does not match.";
    return;
  }

  // Make sure that  Tolerances, IgnoreKeys and IgnoreFilePaths each have either one argument that
  // applies to all values in the keyword OR a one-to-one relationship between the two
  if ( tolerances.hasKeyword(pvl1.name() ) &&
       tolerances[pvl1.name()].size() > 1 &&
       pvl1.size() != tolerances[pvl1.name()].size() ) {

    std::string msg = "Size of keyword '" + pvl1.name() + "' does not match with ";
    msg += "its number of tolerances in the DIFF file.";
    throw IException(IException::User, msg, _FILEINFO_);

  }

  if ( ignoreKeys.hasKeyword(pvl1.name()) &&
       ignoreKeys[pvl1.name()].size() > 1 &&
       pvl1.size() != ignoreKeys[pvl1.name()].size() ) {

    std::string msg = "Size of keyword '" + pvl1.name() + "' does not match with ";
    msg += "its number of ignore keys in the DIFF file.";
    throw IException(IException::User, msg, _FILEINFO_);

  }

  if ( ignoreFilePaths.hasKeyword(pvl1.name()) &&
       ignoreFilePaths[pvl1.name()].size() > 1 &&
       pvl1.size() != ignoreFilePaths[pvl1.name()].size() ) {

    std::string msg = "Size of keyword '" + pvl1.name() + "' does not match with ";
    msg += "its number of filepath ignores in the DIFF file.";
    throw IException(IException::User, msg, _FILEINFO_);

  }

  for (int i = 0; i < pvl1.size() && filesMatch; i++) {
    QString val1 =  QString::fromStdString(pvl1[i]);
    QString val2 = QString::fromStdString(pvl2[i]);

    QString unit1 = QString::fromStdString(pvl1.unit(i));
    QString unit2 = QString::fromStdString(pvl2.unit(i));

    int ignoreIndex = 0;
    if ( ignoreKeys.hasKeyword(pvl1.name()) && ignoreKeys[pvl1.name()].size() > 1 ) {
      ignoreIndex = i;
    }

    // If we only have one value for the keyword, use it for all paths in the keyword's array,
    // otherwise use the corresponding IgnoreFilePaths value
    if ( ignoreFilePaths.hasKeyword(pvl1.name()) &&
         (((ignoreFilePaths[pvl1.name()].size() > 1 && ignoreFilePaths[pvl1.name()][i] == "true") ||
         (ignoreFilePaths[pvl1.name()].size() == 1 && ignoreFilePaths[pvl1.name()][0] == "true"))) ) {
      val1 =  val1.replace(QRegularExpression("(\[\\w\\-\\$\\. ]*)+\\/"), "");
      val2 =  val2.replace(QRegularExpression("(\[\\w\\-\\$\\. ]*)+\\/"), "");
    }


    try {
      if ( !ignoreKeys.hasKeyword(pvl1.name()) ||
          ignoreKeys[pvl1.name()][ignoreIndex] == "false" ) {

        if ( unit1.toLower() != unit2.toLower() ) {
          filesMatch = false;
          differenceReason = "Keyword '" + QString::fromStdString(pvl1.name()) + "': units do not match.";
          return;
        }

        double tolerance = 0.0;
        double difference = abs(val1.toDouble() - val2.toDouble());

        if ( tolerances.hasKeyword(pvl1.name()) ) {
          tolerance = Isis::toDouble(
              (tolerances[pvl1.name()].size() == 1) ?
                 tolerances[pvl1.name()][0] : tolerances[pvl1.name()][i]);
        }

        if ( difference > tolerance ) {
          filesMatch = false;
          if ( pvl1.size() == 1 ) {
            differenceReason = "Keyword '" + QString::fromStdString(pvl1.name()) + "': difference is " +
                               QString::number(difference);
          }
          else {
            differenceReason = "Keyword '" + QString::fromStdString(pvl1.name()) + "' at index " +
                               QString::number(i) + ": difference is " + QString::number(difference);
          }
          differenceReason += " (tolerance is " + QString::number(tolerance) + ")";
        }
      }
    }
    catch (IException &) {
      if ( val1.toLower() != val2.toLower() ) {
        filesMatch = false;
        differenceReason = "Keyword '" + QString::fromStdString(pvl1.name()) + "': values do not "
            "match.";
      }
    }
  }
}

void compareObjects(PvlObject &pvl1, PvlObject &pvl2) {
  
  removeIngoredKeys(pvl1, pvl2);
  
  if ( QString::fromStdString(pvl1.name()).compare(QString::fromStdString(pvl2.name())) != 0 ) {
    filesMatch = false;
    differenceReason = "Object " + QString::fromStdString(pvl1.name()) + " does not match " +
        QString::fromStdString(pvl2.name());
  }

  if ( pvl1.keywords() != pvl2.keywords() ) {
    filesMatch = false;
    differenceReason = "Object " + QString::fromStdString(pvl1.name()) + " has varying keyword counts.";
  }

  if ( pvl1.groups() != pvl2.groups() ) {
    filesMatch = false;
    differenceReason = "Object " + QString::fromStdString(pvl1.name()) + " has varying group counts.";
  }

  if ( pvl1.objects() != pvl2.objects())  {
    filesMatch = false;
    differenceReason = "Object " + QString::fromStdString(pvl1.name()) + " has varying object counts.";
  }

  if (!filesMatch) {
    return;
  }

  for (int keyword = 0; keyword < pvl1.keywords() && filesMatch; keyword++) {
    compareKeywords(pvl1[keyword], pvl2[keyword]);
  }

  for (int object = 0; object < pvl1.objects() && filesMatch; object++) {
    compareObjects(pvl1.object(object), pvl2.object(object));
  }

  for (int group = 0; group < pvl1.groups() && filesMatch; group++) {
    compareGroups(pvl1.group(group), pvl2.group(group));
  }

  if ( !filesMatch && QString::fromStdString(pvl1.name()).compare("Root") != 0 ) {
    differenceReason = "Object " + QString::fromStdString(pvl1.name()) + ": " + differenceReason;
  }
}

void removeIngoredKeys(PvlContainer &pvl1, PvlContainer &pvl2) {
  for (auto it=ignoreKeys.begin(); it!=ignoreKeys.end();it++ ) {
      if (pvl1.hasKeyword(it->name()) && ignoreKeys[it->name()][0] != "false") {
        pvl1 -= *it;    
      }

      if (pvl2.hasKeyword(it->name()) && ignoreKeys[it->name()][0] != "false") {
        pvl2 -= *it;    
      }
    }
}


void compareGroups(PvlGroup &pvl1, PvlGroup &pvl2) {
  removeIngoredKeys(pvl1, pvl2);

  if ( pvl1.keywords() != pvl2.keywords() ) {
    filesMatch = false;
    return;
  }

  for (int keyword = 0; keyword < pvl1.keywords() && filesMatch; keyword++) {
    compareKeywords(pvl1[keyword], pvl2[keyword]);
  }

  if (!filesMatch) {
    differenceReason = "Group " + QString::fromStdString(pvl1.name()) + ": " + differenceReason;
  }
}
