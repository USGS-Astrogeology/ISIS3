/** This is free and unencumbered software released into the public domain.
The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */

#include "cnetdiff.h"

#include <cmath>
#include <float.h>

#include <QList>
#include <QString>

#include "Application.h"
#include "ControlNet.h"
#include "ControlNetDiff.h"
#include "ControlNetVersioner.h"
#include "IException.h"
#include "PvlContainer.h"
#include "PvlGroup.h"

using namespace std;

namespace Isis {
  
  bool filesMatch;
  QString differenceReason;
  PvlGroup tolerances;
  PvlGroup ignorekeys;

  void CompareKeywords(const PvlKeyword &pvl1, const PvlKeyword &pvl2);
  void CompareGroups(const PvlContainer &pvl1, const PvlContainer &pvl2);
  void Compare(const PvlObject &point1, const PvlObject &point2);
  void Compare(ControlNetVersioner &net1, ControlNetVersioner &net2);

  /**
   * Compare two control networks
   *
   * @param ui UserInterface object containing parameters
   * @return Pvl results log file
   */
  Pvl cnetdiff(UserInterface &ui) {

    // input control networks
    ControlNet cnet1 = ControlNet(ui.GetFileName("FROM"));
    ControlNet cnet2 = ControlNet(ui.GetFileName("FROM2"));

    if(ui.WasEntered("DIFF")) {
      Pvl diffFile(ui.GetFileName("DIFF"));
      return cnetdiff(cnet1, cnet2, ui, &diffFile);
    }
    
    return cnetdiff(cnet1, cnet2, ui);
  }


  /**
   * Compare two control networks
   *
   * @param ControlNet cnet1 1st control net for comparison
   * @param ControlNet cnet2 2nd control net for comparison
   * @return Pvl results log file
   */
  Pvl cnetdiff(ControlNet &cnet1, ControlNet &cnet2, UserInterface &ui,
               Pvl *diffFile) {
    Pvl log;

    // create ControlNetVersioner objects for each net
    ControlNetVersioner cnv1(&cnet1);
    ControlNetVersioner cnv2(&cnet2);

    // report first difference only
    if (ui.GetString("REPORT") == "FIRST") {
      tolerances = PvlGroup();
      ignorekeys = PvlGroup();

      differenceReason = "";
      filesMatch = true;

      if (diffFile != nullptr) {
        if(diffFile->hasGroup("Tolerances")) {
          tolerances = diffFile->findGroup("Tolerances");
        }

        if(diffFile->hasGroup("IgnoreKeys")) {
          ignorekeys = diffFile->findGroup("IgnoreKeys");
        }
      }

      // Don't want to consider the DateTime of a Point or Measure was set by
      // default.
      if(!ignorekeys.hasKeyword("DateTime")) {
        ignorekeys += PvlKeyword("DateTime", "true");
      }

      // compare ControlNetVersioners
      Compare(cnv1, cnv2);

      PvlGroup differences("Results");
      if (filesMatch) {
        differences += PvlKeyword("Compare", "Identical");
      }
      else {
        differences += PvlKeyword("Compare", "Different");
        differences += PvlKeyword("Reason", differenceReason);
      }

      Application::AppendAndLog(differences, &log);

      if (ui.WasEntered("TO")) log.write(ui.GetFileName("TO"));

      differenceReason = "";
    
      return log;
    }
    else { // do full report
      FileName fileName1(ui.GetFileName("FROM"));
      FileName fileName2(ui.GetFileName("FROM2"));

      ControlNetDiff differencer;
      if (diffFile != nullptr) {
         differencer.addTolerances(*diffFile);
      }

      Pvl out = differencer.compare(fileName1, fileName2);
      if (ui.WasEntered("TO")) out.write(ui.GetFileName("TO"));

      PvlGroup results("Results");

      // Get a count of all the differences: just the keywords at the object level
      // (network data) and the number of objects (different points).  Ignore the
      // FileName keyword as it's a superficial difference.
      PvlObject &differences = out.findObject("Differences");
      int count = differences.objects() + differences.keywords();

      if (differences.hasKeyword("Filename")) count--;

      results += PvlKeyword("Compare", count > 0 ? "Different" : "Identical");
      Application::AppendAndLog(results, &log);
      return log;
    }
  }


  /**
   * Compare two ControlNetVersioner objects
   *
   * @param ControlNetVersioner net1
   * @param ControlNetVersioner net2
   */
  void Compare(ControlNetVersioner &net1, ControlNetVersioner &net2) {

    Pvl net1Pvl(net1.toPvl());
    Pvl net2Pvl(net2.toPvl());

    PvlObject &net1Obj = net1Pvl.findObject("ControlNetwork");
    PvlObject &net2Obj = net2Pvl.findObject("ControlNetwork");

    BigInt net1NumPts = net1Obj.objects();
    BigInt net2NumPts = net2Obj.objects();

    if (net1NumPts != net2NumPts) {
      differenceReason = "The number of control points in the networks, [" +
                         toString(net1NumPts) + "] and [" +
                         toString(net2NumPts) + "], differ.";
      filesMatch = false;
      return;
    }

    QString id1 = net1Obj["NetworkId"][0];
    QString id2 = net2Obj["NetworkId"][0];

    if (id1 != id2) {
      differenceReason = "The network IDs [" +
                         id1 + "] and [" +
                         id2 + "] differ.";
      filesMatch = false;
      return;
    }

    QString target1 = net1Obj["TargetName"][0];
    QString target2 = net2Obj["TargetName"][0];

    if (target1 != target2) {
      differenceReason = "The TargetName values [" +
                         target1 + "] and [" +
                         target2 + "] differ.";
      filesMatch = false;
      return;
    }

    for(int cpIndex = 0; cpIndex < net1NumPts; cpIndex ++) {
      PvlObject &cp1 = net1Obj.object(cpIndex);
      PvlObject &cp2 = net2Obj.object(cpIndex);

      Compare(cp1, cp2);

      if (!filesMatch) {
        return;
      }
    }
  }


  /**
   * Compare two point PvlObjects
   *
   * @param const PvlObject point1Pvl
   * @param const PvlObject point2Pvl
   */
  void Compare(const PvlObject &point1Pvl, const PvlObject &point2Pvl) {
    // both names must be at least equal, should be named ControlPoint
    if (point1Pvl.name() != point2Pvl.name()) {
      QString msg = "The control points' CreatePvlOject method returned an "
                    "unexpected result.";
      throw IException(IException::Programmer, msg, _FILEINFO_);
    }

    if (point1Pvl.groups() != point2Pvl.groups()) {
      filesMatch = false;
      differenceReason = "The number of control measures, [" +
                         toString(point1Pvl.groups()) + "] and [" +
                         toString(point2Pvl.groups()) + "] does not match.";
    }

    // Start by comparing top level control point keywords.
    if (filesMatch) CompareGroups(point1Pvl, point2Pvl);

    // Now compare each measure
    for(int cmIndex = 0; filesMatch && cmIndex < point1Pvl.groups(); cmIndex ++) {
      const PvlGroup &measure1 = point1Pvl.group(cmIndex);
      const PvlGroup &measure2 = point2Pvl.group(cmIndex);

      CompareGroups(measure1, measure2);

      if(!filesMatch) {
        differenceReason = "Control Measure for Cube [" +
                           measure1["SerialNumber"][0] + "] " + differenceReason;
      }
    }

    if (!filesMatch) {
      differenceReason = "Control Point [" + point1Pvl["PointId"][0] +
                         "] " + differenceReason;
    }
  }


  /**
   * Compare two PvlContainer objects
   *
   * @param const PvlContainer pvl1
   * @param const PvlContainer pvl2
   */
  void CompareGroups(const PvlContainer &pvl1, const PvlContainer &pvl2) {
    // Create equivalent PvlGroups that can easily be compared to each other
    PvlGroup point1FullKeys;
    PvlGroup point2FullKeys;

    for(int keywordIndex = 0; keywordIndex < pvl1.keywords(); keywordIndex++) {
      PvlKeyword thisKey = pvl1[keywordIndex];
      point1FullKeys += thisKey;

      if (!pvl2.hasKeyword(thisKey.name())) {
        point2FullKeys += PvlKeyword(thisKey.name(), "");
      }
    }

    for(int keywordIndex = 0; keywordIndex < pvl2.keywords(); keywordIndex++) {
      PvlKeyword thisKey = pvl2[keywordIndex];
      point2FullKeys += thisKey;

      if (!pvl1.hasKeyword(thisKey.name())) {
        point1FullKeys += PvlKeyword(thisKey.name(), "");
      }
    }

    // Now compare the PvlGroups
    for(int keywordIndex = 0;
        keywordIndex < point1FullKeys.keywords();
        keywordIndex++) {
      PvlKeyword key1 = point1FullKeys[keywordIndex];
      PvlKeyword key2 = point2FullKeys[key1.name()];
      CompareKeywords(key1, key2);
    }
  }


  /**
   * Compare two PvlKeyword objects
   *
   * @param const PvlKeyword pvl1
   * @param const PvlKeyword pvl2
   */
  void CompareKeywords(const PvlKeyword &pvl1, const PvlKeyword &pvl2) {
    if (pvl1.name().compare(pvl2.name()) != 0) {
      QString msg = "CompareKeywords should always be called with keywords that "
                    "have the same name.";
      throw IException(IException::Programmer, msg, _FILEINFO_);
    }

    if (pvl1.size() != pvl2.size()) {
      filesMatch = false;
      differenceReason = "Value '" + pvl1.name() + "' array size does not match.";
      return;
    }

    if (tolerances.hasKeyword(pvl1.name()) &&
        tolerances[pvl1.name()].size() > 1 &&
        pvl1.size() != tolerances[pvl1.name()].size()) {
      QString msg = "Size of value '" + pvl1.name() + "' does not match with ";
      msg += "its number of tolerances in the DIFF file.";
      throw IException(IException::User, msg, _FILEINFO_);
    }

    if (ignorekeys.hasKeyword(pvl1.name()) &&
        ignorekeys[pvl1.name()].size() > 1 &&
        pvl1.size() != ignorekeys[pvl1.name()].size()) {
      QString msg = "Size of value '" + pvl1.name() + "' does not match with ";
      msg += "its number of ignore keys in the DIFF file.";
      throw IException(IException::User, msg, _FILEINFO_);
    }

    for(int i = 0; i < pvl1.size() && filesMatch; i++) {
      QString val1 = pvl1[i];
      QString val2 = pvl2[i];
      QString unit1 = pvl1.unit(i);
      QString unit2 = pvl2.unit(i);

      int ignoreIndex = 0;
      if (ignorekeys.hasKeyword(pvl1.name()) && ignorekeys[pvl1.name()].size() > 1) {
        ignoreIndex = i;
      }

      try {
        if (!ignorekeys.hasKeyword(pvl1.name()) ||
            ignorekeys[pvl1.name()][ignoreIndex] == "false") {

          if (unit1.toLower() != unit2.toLower()) {
            filesMatch = false;
            differenceReason = "Value '" + pvl1.name() + "': units do not match.";
            return;
          }

          double tolerance = 0.0;
          double difference = abs(toDouble(val1) - toDouble(val2));

          if (tolerances.hasKeyword(pvl1.name())) {
            tolerance = toDouble((tolerances[pvl1.name()].size() == 1) ?
                        tolerances[pvl1.name()][0] : tolerances[pvl1.name()][i]);
          }

          if (difference > tolerance) {
            filesMatch = false;
            if (pvl1.size() == 1) {
              differenceReason = "Value [" + pvl1.name() + "] difference is " +
                                 toString(difference);
            }
            else {
              differenceReason = "Value [" + pvl1.name() + "] at index " +
                                 toString(i) + ": difference is " + toString(difference);
            }
            differenceReason += " (values are [" + val1 + "] and [" +
                                val2 + "], tolerance is [" + toString(tolerance) + "])";
          }
        }
      }
      catch(IException &e) {
        if (val1.toLower() != val2.toLower()) {
          filesMatch = false;
          differenceReason = "Value '" + pvl1.name() + "': values do not match.";
        }
      }
    }
  }

}


