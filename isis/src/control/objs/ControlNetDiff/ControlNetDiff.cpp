/** This is free and unencumbered software released into the public domain.

The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */

#include "ControlNetDiff.h"

#include <QMap>
#include <QSet>
#include <QString>

#include "ControlMeasure.h"
#include "ControlNet.h"
#include "ControlNetVersioner.h"
#include "ControlPoint.h"
#include "FileName.h"
#include "Pvl.h"
#include "PvlContainer.h"
#include "PvlGroup.h"
#include "PvlKeyword.h"
#include "PvlObject.h"

namespace Isis {

  /**
   * Create a ControlNetDiff without any tolerances.
   */
  ControlNetDiff::ControlNetDiff() {
    init();
  }


  /**
   * Create a ControlNetDiff with persistent tolerances.
   *
   * @param diffFile The PVL of ignore keywords and tolerance values
   */
  ControlNetDiff::ControlNetDiff(Pvl &diffFile) {
    init();
    addTolerances(diffFile);
  }


  /**
   * Destroy the ControlNetDiff.  Does not own any passed-in data.
   */
  ControlNetDiff::~ControlNetDiff() {
    delete m_tolerances;
    m_tolerances = NULL;

    delete m_ignoreKeys;
    m_ignoreKeys = NULL;
  }


  /**
   * Add the given ignore keys and tolerances to the persisent collections of
   * such values.  Does not clear any previously added ignore keys or
   * tolerances.  The DateTime keyword is always ignored
   *
   * @param diffFile The PVL of ignore keywords and tolerance values
   */
  void ControlNetDiff::addTolerances(Pvl &diffFile) {
    if (diffFile.hasGroup("Tolerances")) {
      PvlGroup tolerances = diffFile.findGroup("Tolerances");
      for (int i = 0; i < tolerances.keywords(); i++)
        m_tolerances->insert(QString::fromStdString(tolerances[i].name()),
            IString::ToDouble(tolerances[i][0]));
    }

    if (diffFile.hasGroup("IgnoreKeys")) {
      PvlGroup ignoreKeys = diffFile.findGroup("IgnoreKeys");
      for (int i = 0; i < ignoreKeys.keywords(); i++)
        m_ignoreKeys->insert(QString::fromStdString(ignoreKeys[i].name()));
    }
  }


  /**
   * Compare two Control Networks given their file names, and return their
   * differences.
   *
   * @param net1Name Name of the first Control Network
   * @param net2Name Name of the second Control Network
   * @return Pvl The collection of all differences
   */
  Pvl ControlNetDiff::compare(FileName &net1Name, FileName &net2Name) {
    Pvl results;
    PvlObject report("Differences");

    diff("Filename", QString::fromStdString(net1Name.name()), QString::fromStdString(net2Name.name()), report);

    ControlNetVersioner cnv1(net1Name);
    ControlNetVersioner cnv2(net2Name);

    BigInt net1NumPts = cnv1.numPoints();
    BigInt net2NumPts = cnv2.numPoints();
    diff("Points", QString::number(net1NumPts), QString::number(net2NumPts), report);

    diff("NetworkId", cnv1.netId(), cnv2.netId(), report);
    diff("TargetName", cnv1.targetName(), cnv2.targetName(), report);

    Pvl net1Pvl = cnv1.toPvl();
    Pvl net2Pvl = cnv2.toPvl();

    PvlObject &net1Obj = net1Pvl.findObject("ControlNetwork");
    PvlObject &net2Obj = net2Pvl.findObject("ControlNetwork");

    QMap< QString, QMap<int, PvlObject> > pointMap;

    for (int p = 0; p < net1NumPts; p++) {
      PvlObject &point = net1Obj.object(p);
      pointMap[QString::fromStdString(point.findKeyword("PointId")[0])].insert(
          0, point);
    }

    for (int p = 0; p < net2NumPts; p++) {
      PvlObject &point = net2Obj.object(p);
      pointMap[QString::fromStdString(point.findKeyword("PointId")[0])].insert(
          1, point);
    }

    QList<QString> pointNames = pointMap.keys();

    for (int i = 0; i < pointNames.size(); i++) {
      QMap<int, PvlObject> idMap = pointMap[pointNames[i]];
      if (idMap.size() == 2) {
        compare(idMap[0], idMap[1], report);
      }
      else if (idMap.contains(0)) {
        addUniquePoint("PointId", QString::fromStdString(idMap[0].findKeyword("PointId")[0]), "N/A", report);
      }
      else if (idMap.contains(1)) {
        addUniquePoint("PointId", "N/A", QString::fromStdString(idMap[1].findKeyword("PointId")[0]), report);
      }
    }

    results.addObject(report);
    return results;
  }


  /**
   * Compare two Control Points stored as PvlObjects, and add any differences to
   * the report object.  A new Point object will only be added to the report if
   * there were top-level point data differences or measure differences.
   *
   * @param point1Pvl Container for the first Control Point
   * @param point2Pvl Container for the second Control Point
   * @param report Container for reporting all differences between these points
   */
  void ControlNetDiff::compare(PvlObject &point1Pvl, PvlObject &point2Pvl, PvlObject &report) {
    PvlObject pointReport("Point");

    QString id1 = QString::fromStdString(point1Pvl.findKeyword("PointId")[0]);
    QString id2 = QString::fromStdString(point2Pvl.findKeyword("PointId")[0]);
    pointReport.addKeyword(makeKeyword("PointId", id1, id2));

    int p1Measures = point1Pvl.groups();
    int p2Measures = point2Pvl.groups();
    diff("Measures", QString::number(p1Measures), QString::number(p2Measures), pointReport);

    compareGroups(point1Pvl, point2Pvl, pointReport);

    QMap< QString, QMap<int, PvlGroup> > measureMap;
    for (int m = 0; m < p1Measures; m++) {
      PvlGroup &measure = point1Pvl.group(m);
      measureMap[QString::fromStdString(measure.findKeyword("SerialNumber")[0])].insert(
          0, measure);
    }

    for (int m = 0; m < p2Measures; m++) {
      PvlGroup &measure = point2Pvl.group(m);
      measureMap[QString::fromStdString(measure.findKeyword("SerialNumber")[0])].insert(
          1, measure);
    }

    QList<QString> measureNames = measureMap.keys();
    for (int i = 0; i < measureNames.size(); i++) {
      QMap<int, PvlGroup> idMap = measureMap[measureNames[i]];
      if (idMap.size() == 2) {
        compareGroups(idMap[0], idMap[1], pointReport);
      }
      else if (idMap.contains(0)) {
        addUniqueMeasure("SerialNumber", QString::fromStdString(idMap[0].findKeyword("SerialNumber")[0]), "N/A", pointReport);
      }
      else if (idMap.contains(1)) {
        addUniqueMeasure("SerialNumber", "N/A", QString::fromStdString(idMap[1].findKeyword("SerialNumber")[0]), pointReport);
      }
    }

    if (pointReport.keywords() > 2 || pointReport.groups() > 0)
      report.addObject(pointReport);
  }


  /**
   * Compare two collections, or groupings, of PvlKeywords.  If the container
   * has the keyword "SerialNumber", we assume it is a Control Measure, and thus
   * create a PvlGroup to add to our report containing all the differences.
   * Otherwise we add differences directly to the given report object.  The
   * measure PvlGroup will only be added if there are data differences.
   *
   * @param g1 Container for the first keyword collection
   * @param g2 Container for the second keyword collection
   * @param report Container for reporting all differences between these
   *               groupings
   */
  void ControlNetDiff::compareGroups(PvlContainer &g1, PvlContainer &g2, PvlObject &report) {
    PvlGroup measureReport("Measure");
    if (g1.hasKeyword("SerialNumber")) {
      QString sn1 = QString::fromStdString(g1.findKeyword("SerialNumber")[0]);
      QString sn2 = QString::fromStdString(g1.findKeyword("SerialNumber")[0]);
      measureReport.addKeyword(makeKeyword("SerialNumber", sn1, sn2));
    }
    PvlContainer &groupReport = g1.hasKeyword("SerialNumber") ?
        (PvlContainer &) measureReport : (PvlContainer &) report;

    QMap< QString, QMap<int, PvlKeyword> > keywordMap;
    for (int k = 0; k < g1.keywords(); k++)
      keywordMap[QString::fromStdString(g1[k].name())].insert(0, g1[k]);
    for (int k = 0; k < g2.keywords(); k++)
      keywordMap[QString::fromStdString(g2[k].name())].insert(1, g2[k]);

    QList<QString> keywordNames = keywordMap.keys();
    for (int i = 0; i < keywordNames.size(); i++) {
      QMap<int, PvlKeyword> idMap = keywordMap[keywordNames[i]];
      if (idMap.size() == 2) {
        compare(idMap[0], idMap[1], groupReport);
      }
      else if (idMap.contains(0)) {
        QString name = QString::fromStdString(idMap[0].name());
        if (!m_ignoreKeys->contains(name))
          diff(name, QString::fromStdString(idMap[0][0]), "N/A", groupReport);
      }
      else if (idMap.contains(1)) {
        QString name = QString::fromStdString(idMap[1].name());
        if (!m_ignoreKeys->contains(name))
          diff(name, "N/A", QString::fromStdString(idMap[1][0]), groupReport);
      }
    }

    if (measureReport.keywords() > 1) report.addGroup(measureReport);
  }


  /**
   * Compare two keywords and report the differences to the given report
   * container.  If a tolerance value exists for the keyword name, the
   * difference will only be reported if the numerical difference magnitude
   * between the values is greater than the tolerance.  Otherwise, the keywords
   * must have equal values.  At present, only the first value of the keywords
   * is compared.  Keywords with multiple values are not considered, nor are
   * their units.
   *
   * @param k1 The first keyword
   * @param k2 The second keyword
   * @param report Container for reporting all differences between these
   *               keywords
   */
  void ControlNetDiff::compare(PvlKeyword &k1, PvlKeyword &k2, PvlContainer &report) {
    QString name = QString::fromStdString(k1.name());
    if (m_tolerances->contains(name))
      diff(name, IString::ToDouble(k1[0]), IString::ToDouble(k2[0]), (*m_tolerances)[name], report);
    else
      diff(name, QString::fromStdString(k1[0]), QString::fromStdString(k2[0]), report);
  }


  /**
   * Add a new difference keyword to the given report if the PvlObjects have
   * different values for the keyword with the given name.
   *
   * @param name The name of the keyword to be compared
   * @param o1 The first object
   * @param o2 The second object
   * @param report Container for reporting differences between these objects for
   *               the keyword with the given name
   */
  void ControlNetDiff::diff(QString name, PvlObject &o1, PvlObject &o2, PvlContainer &report) {
    QString v1 = QString::fromStdString(o1[name.toStdString()][0]);
    QString v2 = QString::fromStdString(o2[name.toStdString()][0]);
    diff(name, v1, v2, report);
  }


  /**
   * Add a new difference keyword to the given report if the two string are not
   * equal.  The two value strings represent values of keywords to be directly
   * compared.  The first name string is the label of the keyword.
   *
   * @param name The label of the keyword whose values are being compared
   * @param v1 The first keyword's value
   * @param v2 The second keyword's value
   * @param report Container for reporting differences between these values for
   *               the keyword with the given name
   */
  void ControlNetDiff::diff(QString name, QString v1, QString v2, PvlContainer &report) {
    if (!m_ignoreKeys->contains(name)) {
      if (v1 != v2) report.addKeyword(makeKeyword(name, v1, v2));
    }
  }


  /**
   * Create a new keyword with the given label name and the given values.  If
   * the two values are equal, only add one of them to the keyword.
   *
   * @param name The label of the keyword whose values are being compared
   * @param v1 The first keyword's value
   * @param v2 The second keyword's value
   * @return PvlKeyword Container for the two values
   */
  PvlKeyword ControlNetDiff::makeKeyword(QString name, QString v1, QString v2) {
    PvlKeyword keyword(name.toStdString());
    keyword.addValue(v1.toStdString());
    if (v1 != v2) keyword.addValue(v2.toStdString());
    return keyword;
  }


  /**
   * Add a new difference keyword to the given report if the two numerical
   * values are not equal within the given tolerance.  The two values represent
   * values of keywords to be directly compared.  The first name string is the
   * label of the keyword.
   *
   * @param name The label of the keyword whose values are being compared
   * @param v1 The first keyword's value
   * @param v2 The second keyword's value
   * @param tol The tolerance to compare against the values
   * @param report Container for reporting differences between these values for
   *               the keyword with the given name
   */
  void ControlNetDiff::diff(QString name, double v1, double v2, double tol, PvlContainer &report) {
    if (!m_ignoreKeys->contains(name)) {
      if (fabs(v1 - v2) > tol) report.addKeyword(makeKeyword(name, v1, v2, tol));
    }
  }


  /**
   * Create a new keyword with the given label name and the given values.  If
   * the two values are equal within the given tolerance, only add one of them
   * to the keyword.
   *
   * @param name The label of the keyword whose values are being compared
   * @param v1 The first keyword's value
   * @param v2 The second keyword's value
   * @param tol The tolerance to compare against the values
   * @return PvlKeyword Container for the two values
   */
  PvlKeyword ControlNetDiff::makeKeyword(QString name, double v1, double v2, double tol) {
    PvlKeyword keyword(name.toStdString());
    keyword.addValue(toString(v1));
    if (fabs(v1 - v2) > tol) {
      keyword.addValue(toString(v2));
      keyword.addValue(toString(tol));
    }
    return keyword;
  }


  /**
   * Add a new keyword for the given point to the parent object.  It is assumed
   * that one of the two values provided represents a missing PointId.
   *
   * @param name The label of the point uniquely identifying it
   * @param v1 The first points's value for the unique label
   * @param v2 The second points's value for the unique label
   * @param parent Container for the point object
   */
  void ControlNetDiff::addUniquePoint(QString label, QString v1, QString v2, PvlObject &parent) {
    PvlObject point("Point");

    PvlKeyword keyword(label.toStdString());
    keyword.addValue(v1.toStdString());
    keyword.addValue(v2.toStdString());
    point.addKeyword(keyword);

    parent.addObject(point);
  }


  /**
   * Add a new keyword for the given measure to the parent object.  It is
   * assumed that one of the two values provided represents a missing
   * SerialNumber.
   *
   * @param name The label of the measure uniquely identifying it
   * @param v1 The first measure's value for the unique label
   * @param v2 The second measure's value for the unique label
   * @param parent Container for the measure group
   */
  void ControlNetDiff::addUniqueMeasure(QString label, QString v1, QString v2, PvlObject &parent) {
    PvlGroup measure("Measure");

    PvlKeyword keyword(label.toStdString());
    keyword.addValue(v1.toStdString());
    keyword.addValue(v2.toStdString());
    measure.addKeyword(keyword);

    parent.addGroup(measure);
  }


  /**
   * Initialize the persistent structures used to maintain the state of this
   * instance: its ignore keys and tolerances.  Automatically ignore the
   * "DateTime" keyword.
   */
  void ControlNetDiff::init() {
    m_tolerances = NULL;
    m_ignoreKeys = NULL;

    m_tolerances = new QMap<QString, double>;
    m_ignoreKeys = new QSet<QString>;

    m_ignoreKeys->insert("DateTime");
  }
}
